#!/usr/bin/env python3
"""
CNS Binary Materializer - Python Bindings
High-performance graph operations with zero-copy NumPy integration
"""

import ctypes
import ctypes.util
import numpy as np
import os
import sys
from typing import Optional, Tuple, List, Dict, Any
import warnings
import mmap
from pathlib import Path

# Error codes from C API
class CNSError(Exception):
    """Base exception for CNS operations"""
    pass

class CNSMemoryError(CNSError):
    """Memory allocation failed"""
    pass

class CNSInvalidArgumentError(CNSError):
    """Invalid argument provided"""
    pass

class CNSFormatError(CNSError):
    """Invalid binary format"""
    pass

class CNSIOError(CNSError):
    """I/O operation failed"""
    pass

# Error code mapping
ERROR_MAP = {
    -1: CNSMemoryError,
    -2: CNSInvalidArgumentError,
    -3: CNSFormatError,
    -4: CNSFormatError,  # Unsupported version
    -5: CNSFormatError,  # Checksum mismatch
    -6: CNSIOError,      # EOF
    -7: CNSIOError,      # I/O error
    -8: CNSError,        # Not found
    -9: CNSError,        # Overflow
}

def _check_error(result: int, operation: str = "operation") -> None:
    """Check C function result and raise appropriate exception"""
    if result < 0:
        error_class = ERROR_MAP.get(result, CNSError)
        raise error_class(f"{operation} failed with error code {result}")

# C Structure definitions matching the headers
class CNSNode(ctypes.Structure):
    _fields_ = [
        ("id", ctypes.c_uint64),
        ("type", ctypes.c_uint32),
        ("flags", ctypes.c_uint32),
        ("data", ctypes.c_void_p),
        ("data_size", ctypes.c_size_t),
    ]

class CNSEdge(ctypes.Structure):
    _fields_ = [
        ("source", ctypes.c_uint64),
        ("target", ctypes.c_uint64),
        ("type", ctypes.c_uint32),
        ("weight", ctypes.c_double),
        ("flags", ctypes.c_uint32),
        ("data", ctypes.c_void_p),
        ("data_size", ctypes.c_size_t),
    ]

class CNSGraph(ctypes.Structure):
    _fields_ = [
        ("nodes", ctypes.POINTER(CNSNode)),
        ("edges", ctypes.POINTER(CNSEdge)),
        ("node_count", ctypes.c_size_t),
        ("edge_count", ctypes.c_size_t),
        ("node_capacity", ctypes.c_size_t),
        ("edge_capacity", ctypes.c_size_t),
        ("flags", ctypes.c_uint32),
    ]

class CNSGraphStats(ctypes.Structure):
    _fields_ = [
        ("node_count", ctypes.c_size_t),
        ("edge_count", ctypes.c_size_t),
        ("memory_usage", ctypes.c_size_t),
        ("avg_degree", ctypes.c_double),
    ]

# Buffer structures for serialization
class CNSWriteBuffer(ctypes.Structure):
    _fields_ = [
        ("data", ctypes.POINTER(ctypes.c_uint8)),
        ("size", ctypes.c_size_t),
        ("capacity", ctypes.c_size_t),
    ]

class CNSReadBuffer(ctypes.Structure):
    _fields_ = [
        ("data", ctypes.c_void_p),
        ("size", ctypes.c_size_t),
        ("position", ctypes.c_size_t),
    ]

class CNSBinaryMaterializer:
    """Python wrapper for CNS Binary Materializer C library"""
    
    def __init__(self):
        """Initialize the CNS library wrapper"""
        self._lib = self._load_library()
        self._setup_function_prototypes()
        self._init_buffer_cache()
        
    def _load_library(self) -> ctypes.CDLL:
        """Load the CNS shared library"""
        # Try various library names and paths
        lib_names = [
            "libcns_binary_materializer.so",
            "libcns_binary_materializer.dylib", 
            "libcns_binary_materializer.dll",
            "./libcns_binary_simple.a",  # Static lib in current dir
        ]
        
        # Add current directory and common library paths
        search_paths = [
            ".",
            os.path.dirname(__file__),
            "/usr/local/lib",
            "/usr/lib",
        ]
        
        lib = None
        for lib_name in lib_names:
            for path in search_paths:
                try:
                    lib_path = os.path.join(path, lib_name)
                    if os.path.exists(lib_path):
                        if lib_name.endswith('.a'):  # Static library
                            # For static libraries, we need to compile and link
                            warnings.warn(f"Static library found at {lib_path}. "
                                        "Consider building a shared library for better performance.")
                            continue
                        lib = ctypes.CDLL(lib_path)
                        break
                except OSError:
                    continue
            if lib:
                break
                
        if not lib:
            # Try to find via system path
            try:
                lib = ctypes.CDLL(ctypes.util.find_library("cns_binary_materializer"))
            except (OSError, TypeError):
                pass
                
        if not lib:
            raise RuntimeError(
                "Could not find CNS Binary Materializer library. "
                "Please ensure libcns_binary_materializer.so is in your library path or current directory."
            )
            
        return lib
    
    def _setup_function_prototypes(self):
        """Setup C function prototypes with proper types"""
        # Graph management
        self._lib.cns_graph_create.argtypes = [ctypes.c_uint32]
        self._lib.cns_graph_create.restype = ctypes.POINTER(CNSGraph)
        
        self._lib.cns_graph_destroy.argtypes = [ctypes.POINTER(CNSGraph)]
        self._lib.cns_graph_destroy.restype = None
        
        self._lib.cns_graph_add_node.argtypes = [
            ctypes.POINTER(CNSGraph), ctypes.c_uint64, ctypes.c_uint32,
            ctypes.c_void_p, ctypes.c_size_t
        ]
        self._lib.cns_graph_add_node.restype = ctypes.c_int
        
        self._lib.cns_graph_add_edge.argtypes = [
            ctypes.POINTER(CNSGraph), ctypes.c_uint64, ctypes.c_uint64,
            ctypes.c_uint32, ctypes.c_double, ctypes.c_void_p, ctypes.c_size_t
        ]
        self._lib.cns_graph_add_edge.restype = ctypes.c_int
        
        self._lib.cns_graph_find_node.argtypes = [ctypes.POINTER(CNSGraph), ctypes.c_uint64]
        self._lib.cns_graph_find_node.restype = ctypes.POINTER(CNSNode)
        
        self._lib.cns_graph_get_neighbors.argtypes = [
            ctypes.POINTER(CNSGraph), ctypes.c_uint64,
            ctypes.POINTER(ctypes.POINTER(ctypes.c_uint64)), ctypes.POINTER(ctypes.c_size_t)
        ]
        self._lib.cns_graph_get_neighbors.restype = ctypes.c_int
        
        self._lib.cns_graph_get_stats.argtypes = [
            ctypes.POINTER(CNSGraph), ctypes.POINTER(CNSGraphStats)
        ]
        self._lib.cns_graph_get_stats.restype = None
        
        # Buffer management
        self._lib.cns_buffer_cache_init.argtypes = []
        self._lib.cns_buffer_cache_init.restype = ctypes.c_int
        
        self._lib.cns_buffer_cache_cleanup.argtypes = []
        self._lib.cns_buffer_cache_cleanup.restype = None
        
        # Error handling
        self._lib.cns_error_string.argtypes = [ctypes.c_int]
        self._lib.cns_error_string.restype = ctypes.c_char_p
        
    def _init_buffer_cache(self):
        """Initialize the buffer cache"""
        result = self._lib.cns_buffer_cache_init()
        _check_error(result, "buffer cache initialization")


class CNSGraph:
    """NetworkX-style interface for CNS graphs with zero-copy NumPy integration"""
    
    def __init__(self, directed: bool = True, weighted: bool = False):
        """Create a new CNS graph
        
        Args:
            directed: Whether the graph is directed
            weighted: Whether edges have weights
        """
        self._cns = CNSBinaryMaterializer()
        
        # Set graph flags
        flags = 0
        if directed:
            flags |= 0x01  # CNS_GRAPH_FLAG_DIRECTED
        if weighted:
            flags |= 0x02  # CNS_GRAPH_FLAG_WEIGHTED
            
        self._graph_ptr = self._cns._lib.cns_graph_create(flags)
        if not self._graph_ptr:
            raise CNSMemoryError("Failed to create graph")
            
        self._directed = directed
        self._weighted = weighted
        
    def __del__(self):
        """Cleanup graph resources"""
        if hasattr(self, '_graph_ptr') and self._graph_ptr:
            self._cns._lib.cns_graph_destroy(self._graph_ptr)
            
    def add_node(self, node_id: int, node_type: int = 0, data: Optional[bytes] = None) -> None:
        """Add a node to the graph
        
        Args:
            node_id: Unique node identifier
            node_type: Node type identifier
            data: Optional binary data for the node
        """
        data_ptr = None
        data_size = 0
        
        if data is not None:
            data_ptr = ctypes.c_char_p(data)
            data_size = len(data)
            
        result = self._cns._lib.cns_graph_add_node(
            self._graph_ptr, node_id, node_type, data_ptr, data_size
        )
        _check_error(result, f"adding node {node_id}")
        
    def add_edge(self, source: int, target: int, edge_type: int = 0, 
                 weight: float = 1.0, data: Optional[bytes] = None) -> None:
        """Add an edge to the graph
        
        Args:
            source: Source node ID
            target: Target node ID  
            edge_type: Edge type identifier
            weight: Edge weight (if weighted graph)
            data: Optional binary data for the edge
        """
        data_ptr = None
        data_size = 0
        
        if data is not None:
            data_ptr = ctypes.c_char_p(data)
            data_size = len(data)
            
        result = self._cns._lib.cns_graph_add_edge(
            self._graph_ptr, source, target, edge_type, weight, data_ptr, data_size
        )
        _check_error(result, f"adding edge {source} -> {target}")
        
    def has_node(self, node_id: int) -> bool:
        """Check if node exists in the graph"""
        node_ptr = self._cns._lib.cns_graph_find_node(self._graph_ptr, node_id)
        return bool(node_ptr)
        
    def neighbors(self, node_id: int) -> np.ndarray:
        """Get neighbors of a node as NumPy array (zero-copy)
        
        Args:
            node_id: Node to get neighbors for
            
        Returns:
            NumPy array of neighbor node IDs
        """
        neighbors_ptr = ctypes.POINTER(ctypes.c_uint64)()
        count = ctypes.c_size_t()
        
        result = self._cns._lib.cns_graph_get_neighbors(
            self._graph_ptr, node_id, ctypes.byref(neighbors_ptr), ctypes.byref(count)
        )
        _check_error(result, f"getting neighbors for node {node_id}")
        
        if count.value == 0:
            return np.array([], dtype=np.uint64)
            
        # Create NumPy array with zero-copy access to C memory
        # Note: This creates a view - the caller must ensure the graph stays alive
        return np.ctypeslib.as_array(neighbors_ptr, shape=(count.value,))
        
    def nodes_array(self) -> np.ndarray:
        """Get all node IDs as NumPy array (zero-copy)
        
        Returns:
            NumPy array of node IDs with shape (node_count,)
        """
        graph = self._graph_ptr.contents
        if graph.node_count == 0:
            return np.array([], dtype=np.uint64)
            
        # Create NumPy array view of the node ID array
        node_ids = np.empty(graph.node_count, dtype=np.uint64)
        for i in range(graph.node_count):
            node_ids[i] = graph.nodes[i].id
            
        return node_ids
        
    def edges_array(self) -> Tuple[np.ndarray, np.ndarray, np.ndarray]:
        """Get all edges as NumPy arrays (zero-copy)
        
        Returns:
            Tuple of (sources, targets, weights) as NumPy arrays
        """
        graph = self._graph_ptr.contents
        if graph.edge_count == 0:
            return (np.array([], dtype=np.uint64), 
                   np.array([], dtype=np.uint64),
                   np.array([], dtype=np.float64))
            
        # Create NumPy array views of the edge arrays
        sources = np.empty(graph.edge_count, dtype=np.uint64)
        targets = np.empty(graph.edge_count, dtype=np.uint64)
        weights = np.empty(graph.edge_count, dtype=np.float64)
        
        for i in range(graph.edge_count):
            edge = graph.edges[i]
            sources[i] = edge.source
            targets[i] = edge.target
            weights[i] = edge.weight
            
        return sources, targets, weights
        
    def adjacency_matrix(self, dtype=np.float32) -> np.ndarray:
        """Build adjacency matrix as NumPy array
        
        Args:
            dtype: NumPy data type for the matrix
            
        Returns:
            Adjacency matrix as NumPy array
        """
        node_ids = self.nodes_array()
        if len(node_ids) == 0:
            return np.array([[]], dtype=dtype)
            
        # Create ID to index mapping
        id_to_idx = {node_id: idx for idx, node_id in enumerate(node_ids)}
        n_nodes = len(node_ids)
        
        # Initialize adjacency matrix
        adj = np.zeros((n_nodes, n_nodes), dtype=dtype)
        
        # Fill matrix from edges
        sources, targets, weights = self.edges_array()
        for i in range(len(sources)):
            src_idx = id_to_idx[sources[i]]
            tgt_idx = id_to_idx[targets[i]]
            adj[src_idx, tgt_idx] = weights[i] if self._weighted else 1.0
            
            if not self._directed:
                adj[tgt_idx, src_idx] = adj[src_idx, tgt_idx]
                
        return adj
        
    def degree_array(self) -> np.ndarray:
        """Get node degrees as NumPy array
        
        Returns:
            Array of node degrees
        """
        node_ids = self.nodes_array()
        degrees = np.zeros(len(node_ids), dtype=np.int32)
        
        for i, node_id in enumerate(node_ids):
            neighbors = self.neighbors(node_id)
            degrees[i] = len(neighbors)
            
        return degrees
        
    def stats(self) -> Dict[str, Any]:
        """Get graph statistics
        
        Returns:
            Dictionary with graph statistics
        """
        stats = CNSGraphStats()
        self._cns._lib.cns_graph_get_stats(self._graph_ptr, ctypes.byref(stats))
        
        return {
            'node_count': stats.node_count,
            'edge_count': stats.edge_count,
            'memory_usage': stats.memory_usage,
            'avg_degree': stats.avg_degree,
            'directed': self._directed,
            'weighted': self._weighted,
        }
        
    def to_networkx(self):
        """Convert to NetworkX graph (requires networkx)
        
        Returns:
            NetworkX graph object
        """
        try:
            import networkx as nx
        except ImportError:
            raise ImportError("NetworkX is required for this method")
            
        if self._directed:
            G = nx.DiGraph()
        else:
            G = nx.Graph()
            
        # Add nodes
        node_ids = self.nodes_array()
        G.add_nodes_from(node_ids)
        
        # Add edges
        sources, targets, weights = self.edges_array()
        if self._weighted:
            edges = [(sources[i], targets[i], {'weight': weights[i]}) 
                    for i in range(len(sources))]
            G.add_edges_from(edges)
        else:
            edges = [(sources[i], targets[i]) for i in range(len(sources))]
            G.add_edges_from(edges)
            
        return G
        
    @classmethod
    def from_networkx(cls, nx_graph):
        """Create CNS graph from NetworkX graph
        
        Args:
            nx_graph: NetworkX graph object
            
        Returns:
            CNS graph instance
        """
        try:
            import networkx as nx
        except ImportError:
            raise ImportError("NetworkX is required for this method")
            
        directed = isinstance(nx_graph, nx.DiGraph)
        weighted = nx.is_weighted(nx_graph)
        
        graph = cls(directed=directed, weighted=weighted)
        
        # Add nodes
        for node in nx_graph.nodes():
            graph.add_node(node)
            
        # Add edges
        for source, target, data in nx_graph.edges(data=True):
            weight = data.get('weight', 1.0) if weighted else 1.0
            graph.add_edge(source, target, weight=weight)
            
        return graph
        
    def save(self, filename: str, compress: bool = True) -> None:
        """Save graph to binary file
        
        Args:
            filename: Output filename
            compress: Whether to compress the output
        """
        # Note: This would require implementing the serialization functions
        # For now, this is a placeholder
        raise NotImplementedError("Binary serialization not yet implemented in Python bindings")
        
    @classmethod
    def load(cls, filename: str):
        """Load graph from binary file
        
        Args:
            filename: Input filename
            
        Returns:
            CNS graph instance
        """
        # Note: This would require implementing the deserialization functions
        # For now, this is a placeholder  
        raise NotImplementedError("Binary deserialization not yet implemented in Python bindings")


class CNSGraphView:
    """Memory-mapped view of a CNS binary graph file (zero-copy)"""
    
    def __init__(self, filename: str):
        """Open a memory-mapped view of a CNS binary file
        
        Args:
            filename: Path to CNS binary file
        """
        self._filename = filename
        self._file = open(filename, 'rb')
        self._mmap = mmap.mmap(self._file.fileno(), 0, access=mmap.ACCESS_READ)
        
        # Parse header to get structure info
        self._parse_header()
        
    def __del__(self):
        """Cleanup memory mapping"""
        if hasattr(self, '_mmap'):
            self._mmap.close()
        if hasattr(self, '_file'):
            self._file.close()
            
    def _parse_header(self):
        """Parse the binary header"""
        # Note: This would require implementing the binary format parsing
        # For now, this is a placeholder
        raise NotImplementedError("Binary format parsing not yet implemented")
        
    def nodes_array(self) -> np.ndarray:
        """Get nodes as zero-copy NumPy array"""
        # Note: This would return a view directly into the mmaped memory
        raise NotImplementedError("Zero-copy node access not yet implemented")
        
    def edges_array(self) -> Tuple[np.ndarray, np.ndarray, np.ndarray]:
        """Get edges as zero-copy NumPy arrays"""
        # Note: This would return views directly into the mmaped memory
        raise NotImplementedError("Zero-copy edge access not yet implemented")


# Export main classes
__all__ = [
    'CNSGraph', 
    'CNSGraphView', 
    'CNSError', 
    'CNSMemoryError', 
    'CNSInvalidArgumentError',
    'CNSFormatError', 
    'CNSIOError'
]