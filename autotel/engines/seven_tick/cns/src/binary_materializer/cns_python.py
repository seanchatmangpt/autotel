#!/usr/bin/env python3
"""
CNS Binary Materializer - Python Bindings
Zero-copy graph processing with NetworkX compatibility
"""

import ctypes
import os
import numpy as np
from typing import List, Tuple, Optional, Iterator
import tempfile
import warnings

# Suppress NumPy warnings for cleaner output
warnings.filterwarnings('ignore', category=RuntimeWarning)

class CNSGraphError(Exception):
    """Base exception for CNS graph operations"""
    pass

class CNSGraphNotFoundError(CNSGraphError):
    """Graph file not found or invalid"""
    pass

# C structure definitions matching our binary format
class GraphHeader(ctypes.Structure):
    _fields_ = [
        ("magic", ctypes.c_uint32),
        ("version", ctypes.c_uint32),
        ("node_count", ctypes.c_uint32),
        ("edge_count", ctypes.c_uint32),
        ("nodes_offset", ctypes.c_uint64),
        ("edges_offset", ctypes.c_uint64),
    ]

class GraphNode(ctypes.Structure):
    _fields_ = [
        ("id", ctypes.c_uint32),
        ("type", ctypes.c_uint16),
        ("flags", ctypes.c_uint16),
        ("data_offset", ctypes.c_uint32),
        ("first_edge", ctypes.c_uint32),
    ]

class GraphEdge(ctypes.Structure):
    _fields_ = [
        ("source", ctypes.c_uint32),
        ("target", ctypes.c_uint32),
        ("next_edge", ctypes.c_uint32),
        ("weight", ctypes.c_float),
    ]

class CNSGraph:
    """
    High-performance graph class with zero-copy NumPy integration
    Compatible with NetworkX for easy migration
    """
    
    def __init__(self, filename: Optional[str] = None):
        """
        Create a CNS graph instance
        
        Args:
            filename: Path to CNS binary file, or None for empty graph
        """
        self.filename = filename
        self._nodes_array = None
        self._edges_array = None
        self._header = None
        self._node_count = 0
        self._edge_count = 0
        
        if filename and os.path.exists(filename):
            self._load_from_file(filename)
    
    def _load_from_file(self, filename: str):
        """Load graph from CNS binary file with memory mapping"""
        try:
            # Memory-map the file for zero-copy access
            self._memmap = np.memmap(filename, dtype=np.uint8, mode='r')
            
            # Parse header
            header_bytes = self._memmap[:ctypes.sizeof(GraphHeader)]
            self._header = GraphHeader.from_buffer_copy(header_bytes)
            
            if self._header.magic != 0x47524150:  # 'GRAP'
                raise CNSGraphError(f"Invalid CNS file: {filename}")
            
            self._node_count = self._header.node_count
            self._edge_count = self._header.edge_count
            
            # Create zero-copy NumPy views of nodes and edges
            nodes_start = self._header.nodes_offset
            nodes_size = self._node_count * ctypes.sizeof(GraphNode)
            nodes_bytes = self._memmap[nodes_start:nodes_start + nodes_size]
            
            edges_start = self._header.edges_offset
            edges_size = self._edge_count * ctypes.sizeof(GraphEdge)
            edges_bytes = self._memmap[edges_start:edges_start + edges_size]
            
            # Convert to NumPy structured arrays for efficient access
            node_dtype = np.dtype([
                ('id', np.uint32),
                ('type', np.uint16),
                ('flags', np.uint16),
                ('data_offset', np.uint32),
                ('first_edge', np.uint32),
            ])
            
            edge_dtype = np.dtype([
                ('source', np.uint32),
                ('target', np.uint32),
                ('next_edge', np.uint32),
                ('weight', np.float32),
            ])
            
            self._nodes_array = np.frombuffer(nodes_bytes, dtype=node_dtype)
            self._edges_array = np.frombuffer(edges_bytes, dtype=edge_dtype)
            
        except Exception as e:
            raise CNSGraphError(f"Failed to load {filename}: {e}")
    
    @property
    def nodes(self) -> np.ndarray:
        """Zero-copy access to nodes as NumPy array"""
        if self._nodes_array is None:
            return np.array([], dtype=np.uint32)
        return self._nodes_array['id']
    
    @property
    def edges(self) -> np.ndarray:
        """Zero-copy access to edges as NumPy array of (source, target) pairs"""
        if self._edges_array is None:
            return np.array([], dtype=[('source', np.uint32), ('target', np.uint32)])
        return self._edges_array[['source', 'target']]
    
    def number_of_nodes(self) -> int:
        """NetworkX compatibility: return node count"""
        return self._node_count
    
    def number_of_edges(self) -> int:
        """NetworkX compatibility: return edge count"""
        return self._edge_count
    
    def neighbors(self, node: int) -> Iterator[int]:
        """
        NetworkX compatibility: iterate over neighbors of a node
        Uses zero-copy traversal on binary format
        """
        if self._nodes_array is None or node >= self._node_count:
            return
        
        edge_idx = self._nodes_array[node]['first_edge']
        while edge_idx != 0xFFFFFFFF and edge_idx < self._edge_count:
            edge = self._edges_array[edge_idx]
            yield int(edge['target'])
            edge_idx = edge['next_edge']
    
    def degree(self, node: int) -> int:
        """NetworkX compatibility: return degree of a node"""
        return len(list(self.neighbors(node)))
    
    def has_edge(self, source: int, target: int) -> bool:
        """NetworkX compatibility: check if edge exists"""
        return target in self.neighbors(source)
    
    def bfs_tree(self, source: int) -> List[int]:
        """
        Breadth-first search returning visit order
        Ultra-fast implementation using binary format
        """
        if self._nodes_array is None or source >= self._node_count:
            return []
        
        visited = np.zeros(self._node_count, dtype=bool)
        queue = [source]
        result = []
        visited[source] = True
        
        while queue:
            current = queue.pop(0)
            result.append(current)
            
            # Direct traversal on binary format
            for neighbor in self.neighbors(current):
                if not visited[neighbor]:
                    visited[neighbor] = True
                    queue.append(neighbor)
        
        return result
    
    def shortest_path(self, source: int, target: int) -> List[int]:
        """
        NetworkX compatibility: find shortest path between nodes
        Returns path as list of node IDs
        """
        if self._nodes_array is None:
            return []
        
        if source == target:
            return [source]
        
        visited = np.zeros(self._node_count, dtype=bool)
        parent = np.full(self._node_count, -1, dtype=np.int32)
        queue = [source]
        visited[source] = True
        
        while queue:
            current = queue.pop(0)
            
            for neighbor in self.neighbors(current):
                if not visited[neighbor]:
                    visited[neighbor] = True
                    parent[neighbor] = current
                    queue.append(neighbor)
                    
                    if neighbor == target:
                        # Reconstruct path
                        path = []
                        node = target
                        while node != -1:
                            path.append(node)
                            node = parent[node]
                        return path[::-1]
        
        return []  # No path found
    
    def connected_components(self) -> List[List[int]]:
        """
        NetworkX compatibility: find all connected components
        Returns list of components (each is a list of node IDs)
        """
        if self._nodes_array is None:
            return []
        
        visited = np.zeros(self._node_count, dtype=bool)
        components = []
        
        for node in range(self._node_count):
            if not visited[node]:
                # BFS from this node to find component
                component = []
                queue = [node]
                visited[node] = True
                
                while queue:
                    current = queue.pop(0)
                    component.append(current)
                    
                    for neighbor in self.neighbors(current):
                        if not visited[neighbor]:
                            visited[neighbor] = True
                            queue.append(neighbor)
                
                components.append(component)
        
        return components
    
    def create_test_graph(self, node_count: int, avg_degree: int = 5) -> str:
        """
        Create a test graph file for benchmarking
        Returns the filename of the created graph
        """
        import struct
        import random
        
        # Create temporary file
        fd, filename = tempfile.mkstemp(suffix='.cns')
        
        try:
            with os.fdopen(fd, 'wb') as f:
                # Write header
                header = GraphHeader()
                header.magic = 0x47524150  # 'GRAP'
                header.version = 1
                header.node_count = node_count
                header.edge_count = 0  # Will update later
                header.nodes_offset = ctypes.sizeof(GraphHeader)
                header.edges_offset = header.nodes_offset + node_count * ctypes.sizeof(GraphNode)
                
                f.write(header)
                
                # Write nodes
                edges_created = 0
                edge_lists = [[] for _ in range(node_count)]
                
                # Generate edges first to know first_edge indices
                random.seed(42)  # Deterministic for testing
                for i in range(node_count):
                    degree = min(random.randint(1, avg_degree * 2), node_count - 1)
                    for _ in range(degree):
                        target = random.randint(0, node_count - 1)
                        if target != i:
                            edge_lists[i].append(target)
                
                # Write nodes with first_edge indices
                current_edge_idx = 0
                for i in range(node_count):
                    node = GraphNode()
                    node.id = i
                    node.type = 0x100 + (i % 10)
                    node.flags = 0
                    node.data_offset = 0
                    node.first_edge = current_edge_idx if edge_lists[i] else 0xFFFFFFFF
                    
                    f.write(node)
                    current_edge_idx += len(edge_lists[i])
                
                # Write edges
                edge_count = 0
                for i in range(node_count):
                    for j, target in enumerate(edge_lists[i]):
                        edge = GraphEdge()
                        edge.source = i
                        edge.target = target
                        edge.weight = 1.0 + random.random()
                        edge.next_edge = (edge_count + 1) if j < len(edge_lists[i]) - 1 else 0xFFFFFFFF
                        
                        f.write(edge)
                        edge_count += 1
                
                # Update header with actual edge count
                f.seek(0)
                header.edge_count = edge_count
                f.write(header)
                
        except Exception as e:
            os.unlink(filename)
            raise CNSGraphError(f"Failed to create test graph: {e}")
        
        return filename
    
    def benchmark(self, iterations: int = 1000) -> dict:
        """
        Run performance benchmarks on the graph
        Returns dictionary with timing results
        """
        import time
        
        if self._nodes_array is None:
            return {"error": "No graph loaded"}
        
        results = {}
        
        # Node access benchmark
        start = time.perf_counter()
        for _ in range(iterations):
            node_id = np.random.randint(0, self._node_count)
            _ = self._nodes_array[node_id]['id']
        end = time.perf_counter()
        
        node_access_time = (end - start) / iterations
        results['node_access_ns'] = node_access_time * 1e9
        results['node_access_rate'] = 1.0 / node_access_time
        
        # Edge traversal benchmark
        start = time.perf_counter()
        total_edges = 0
        for _ in range(min(iterations, self._node_count)):
            node_id = np.random.randint(0, self._node_count)
            total_edges += len(list(self.neighbors(node_id)))
        end = time.perf_counter()
        
        if total_edges > 0:
            edge_time = (end - start) / total_edges
            results['edge_traversal_ns'] = edge_time * 1e9
            results['edge_traversal_rate'] = 1.0 / edge_time
        
        # BFS benchmark
        start = time.perf_counter()
        bfs_result = self.bfs_tree(0)
        end = time.perf_counter()
        
        results['bfs_time_ms'] = (end - start) * 1000
        results['bfs_nodes_per_sec'] = len(bfs_result) / (end - start)
        
        return results
    
    def to_networkx(self):
        """
        Convert to NetworkX graph for compatibility
        Note: This creates a copy, use CNS methods for zero-copy performance
        """
        try:
            import networkx as nx
        except ImportError:
            raise CNSGraphError("NetworkX not installed")
        
        G = nx.DiGraph()
        G.add_nodes_from(self.nodes)
        
        for edge in self._edges_array:
            G.add_edge(edge['source'], edge['target'], weight=edge['weight'])
        
        return G
    
    @classmethod
    def from_networkx(cls, G, filename: Optional[str] = None):
        """
        Create CNS graph from NetworkX graph
        
        Args:
            G: NetworkX graph
            filename: Optional filename to save to
        """
        import struct
        
        if filename is None:
            fd, filename = tempfile.mkstemp(suffix='.cns')
            os.close(fd)
        
        node_list = list(G.nodes())
        edge_list = list(G.edges(data=True))
        
        with open(filename, 'wb') as f:
            # Write header
            header = GraphHeader()
            header.magic = 0x47524150  # 'GRAP'
            header.version = 1
            header.node_count = len(node_list)
            header.edge_count = len(edge_list)
            header.nodes_offset = ctypes.sizeof(GraphHeader)
            header.edges_offset = header.nodes_offset + len(node_list) * ctypes.sizeof(GraphNode)
            
            f.write(header)
            
            # Create node mapping
            node_map = {node: i for i, node in enumerate(node_list)}
            
            # Write nodes
            for i, node_id in enumerate(node_list):
                node = GraphNode()
                node.id = i
                node.type = 0x100
                node.flags = 0
                node.data_offset = 0
                node.first_edge = 0xFFFFFFFF  # Will update in second pass
                f.write(node)
            
            # Write edges  
            for source, target, data in edge_list:
                edge = GraphEdge()
                edge.source = node_map[source]
                edge.target = node_map[target]
                edge.weight = data.get('weight', 1.0)
                edge.next_edge = 0xFFFFFFFF
                f.write(edge)
        
        return cls(filename)
    
    def __len__(self):
        """NetworkX compatibility: return number of nodes"""
        return self.number_of_nodes()
    
    def __iter__(self):
        """NetworkX compatibility: iterate over nodes"""
        return iter(self.nodes)
    
    def __contains__(self, node):
        """NetworkX compatibility: check if node exists"""
        return 0 <= node < self._node_count
    
    def __str__(self):
        return f"CNSGraph({self._node_count} nodes, {self._edge_count} edges)"
    
    def __repr__(self):
        return self.__str__()


def demo():
    """Demonstration of CNS Python bindings"""
    print("🚀 CNS Binary Materializer - Python Demo")
    print("=" * 50)
    
    # Create test graph
    print("\n📊 Creating test graph...")
    graph = CNSGraph()
    test_file = graph.create_test_graph(10000, avg_degree=8)
    
    # Load graph
    print(f"📁 Loading graph from {test_file}")
    graph = CNSGraph(test_file)
    print(f"✅ Loaded: {graph}")
    
    # Zero-copy access demo
    print("\n🔍 Zero-copy NumPy access:")
    print(f"  Nodes array shape: {graph.nodes.shape}")
    print(f"  Edges array shape: {graph.edges.shape}")
    print(f"  First 5 nodes: {graph.nodes[:5]}")
    
    # Graph algorithms demo
    print("\n🧠 Graph algorithms:")
    
    # BFS
    bfs_result = graph.bfs_tree(0)
    print(f"  BFS from node 0: visited {len(bfs_result)} nodes")
    
    # Shortest path
    path = graph.shortest_path(0, 100)
    print(f"  Shortest path 0→100: {len(path)} hops")
    
    # Connected components
    components = graph.connected_components()
    print(f"  Connected components: {len(components)}")
    
    # Performance benchmark
    print("\n⚡ Performance benchmark:")
    results = graph.benchmark(iterations=10000)
    for metric, value in results.items():
        if 'rate' in metric:
            print(f"  {metric}: {value:,.0f} ops/sec")
        elif 'ns' in metric:
            print(f"  {metric}: {value:.1f} ns")
        elif 'ms' in metric:
            print(f"  {metric}: {value:.2f} ms")
    
    # Cleanup
    os.unlink(test_file)
    print("\n✅ Demo complete!")


if __name__ == "__main__":
    demo()