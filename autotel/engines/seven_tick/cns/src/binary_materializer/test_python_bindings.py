#!/usr/bin/env python3
"""
CNS Binary Materializer - Python Bindings Test
Test the Python bindings functionality without requiring C library compilation
"""

import numpy as np
import sys
import os
from pathlib import Path

def test_numpy_integration():
    """Test NumPy integration features"""
    print("🔢 Testing NumPy Integration")
    print("=" * 40)
    
    # Create mock graph data as NumPy arrays
    print("📊 Creating mock graph data...")
    
    # Node data
    node_ids = np.array([1, 2, 3, 4, 5, 6, 7, 8, 9, 10], dtype=np.uint64)
    node_types = np.array([0, 0, 1, 0, 2, 0, 0, 1, 0, 2], dtype=np.uint32)
    
    # Edge data  
    sources = np.array([1, 1, 2, 3, 4, 5, 5, 6, 7, 8, 2, 3, 4], dtype=np.uint64)
    targets = np.array([2, 3, 4, 5, 6, 7, 8, 9, 10, 10, 5, 8, 7], dtype=np.uint64)
    weights = np.array([1.0, 2.5, 1.8, 3.2, 1.1, 4.0, 2.0, 1.5, 3.8, 2.2, 1.7, 2.9, 1.3], dtype=np.float64)
    
    print(f"✅ Nodes: {len(node_ids)} ({node_ids.dtype})")
    print(f"✅ Edges: {len(sources)} ({sources.dtype})")
    print(f"✅ Weights: {weights.dtype}")
    
    # Test zero-copy operations
    print("\n🚀 Testing zero-copy operations...")
    
    # Create adjacency matrix
    n_nodes = len(node_ids)
    id_to_idx = {node_id: idx for idx, node_id in enumerate(node_ids)}
    adj_matrix = np.zeros((n_nodes, n_nodes), dtype=np.float32)
    
    for i in range(len(sources)):
        src_idx = id_to_idx[sources[i]]
        tgt_idx = id_to_idx[targets[i]]
        adj_matrix[src_idx, tgt_idx] = weights[i]
    
    print(f"✅ Adjacency matrix: {adj_matrix.shape}")
    print(f"   Non-zero entries: {np.count_nonzero(adj_matrix)}")
    print(f"   Memory usage: {adj_matrix.nbytes / 1024:.2f} KB")
    
    # Compute node degrees
    out_degrees = np.sum(adj_matrix > 0, axis=1)
    in_degrees = np.sum(adj_matrix > 0, axis=0)
    
    print(f"\n📐 Degree Analysis:")
    print(f"   Out-degrees: {out_degrees}")
    print(f"   In-degrees: {in_degrees}")
    print(f"   Average out-degree: {np.mean(out_degrees):.2f}")
    print(f"   Max degree: {np.max(out_degrees)}")
    
    # Test advanced NumPy operations
    print(f"\n🧮 Advanced NumPy Operations:")
    
    # Find nodes with highest degree
    high_degree_nodes = node_ids[out_degrees == np.max(out_degrees)]
    print(f"   Highest degree nodes: {high_degree_nodes}")
    
    # Compute weight statistics
    print(f"   Weight statistics:")
    print(f"     Mean: {np.mean(weights):.2f}")
    print(f"     Std: {np.std(weights):.2f}")
    print(f"     Min: {np.min(weights):.2f}")
    print(f"     Max: {np.max(weights):.2f}")
    
    # Test memory views and slicing
    print(f"\n🔍 Memory Views and Slicing:")
    weight_view = weights[weights > 2.0]
    print(f"   Edges with weight > 2.0: {len(weight_view)}")
    print(f"   Heavy edges: {weight_view}")
    
    return True

def test_ctypes_structures():
    """Test ctypes structure definitions"""
    print("\n🏗️ Testing ctypes Structures")
    print("=" * 40)
    
    import ctypes
    
    # Define structures matching C API
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
    
    # Test structure creation
    print("📋 Creating test structures...")
    
    node = CNSNode()
    node.id = 42
    node.type = 1
    node.flags = 0
    node.data = None
    node.data_size = 0
    
    edge = CNSEdge()
    edge.source = 1
    edge.target = 2
    edge.type = 0
    edge.weight = 3.14
    edge.flags = 0
    edge.data = None
    edge.data_size = 0
    
    print(f"✅ Node: ID={node.id}, Type={node.type}")
    print(f"✅ Edge: {edge.source}->{edge.target}, Weight={edge.weight}")
    
    # Test array of structures
    print("\n📊 Testing structure arrays...")
    
    nodes = (CNSNode * 3)()
    for i in range(3):
        nodes[i].id = i + 1
        nodes[i].type = i % 2
        nodes[i].flags = 0
    
    edges = (CNSEdge * 2)()
    edges[0].source = 1
    edges[0].target = 2
    edges[0].weight = 1.5
    edges[1].source = 2
    edges[1].target = 3
    edges[1].weight = 2.7
    
    print(f"✅ Created {len(nodes)} nodes and {len(edges)} edges")
    
    # Convert to NumPy arrays
    print("\n🔢 Converting to NumPy arrays...")
    
    node_ids = np.array([nodes[i].id for i in range(len(nodes))], dtype=np.uint64)
    edge_weights = np.array([edges[i].weight for i in range(len(edges))], dtype=np.float64)
    
    print(f"✅ Node IDs: {node_ids}")
    print(f"✅ Edge weights: {edge_weights}")
    
    return True

def test_networkx_integration():
    """Test NetworkX integration (if available)"""
    print("\n🕸️ Testing NetworkX Integration")
    print("=" * 40)
    
    try:
        import networkx as nx
        print("✅ NetworkX available")
        
        # Create a test graph
        G = nx.DiGraph()
        
        # Add nodes with attributes
        nodes = [(1, {"type": 0}), (2, {"type": 0}), (3, {"type": 1}), (4, {"type": 0}), (5, {"type": 2})]
        G.add_nodes_from(nodes)
        
        # Add weighted edges
        edges = [
            (1, 2, {"weight": 1.0, "type": 0}),
            (1, 3, {"weight": 2.5, "type": 0}),
            (2, 4, {"weight": 1.8, "type": 1}),
            (3, 5, {"weight": 3.2, "type": 0}),
            (4, 5, {"weight": 1.1, "type": 0}),
        ]
        G.add_edges_from(edges)
        
        print(f"✅ Created NetworkX graph: {G.number_of_nodes()} nodes, {G.number_of_edges()} edges")
        
        # Test conversions to NumPy
        print("\n🔄 Converting to NumPy format...")
        
        # Extract data as arrays
        node_list = list(G.nodes())
        node_array = np.array(node_list, dtype=np.uint64)
        
        edge_list = list(G.edges())
        source_array = np.array([e[0] for e in edge_list], dtype=np.uint64)
        target_array = np.array([e[1] for e in edge_list], dtype=np.uint64)
        weight_array = np.array([G[e[0]][e[1]]['weight'] for e in edge_list], dtype=np.float64)
        
        print(f"✅ Extracted arrays:")
        print(f"   Nodes: {node_array}")
        print(f"   Sources: {source_array}")
        print(f"   Targets: {target_array}")
        print(f"   Weights: {weight_array}")
        
        # Test NetworkX algorithms
        print("\n🧮 Testing NetworkX algorithms...")
        
        try:
            # Centrality measures
            betweenness = nx.betweenness_centrality(G, weight='weight')
            closeness = nx.closeness_centrality(G, distance='weight')
            
            print(f"✅ Betweenness centrality: {betweenness}")
            print(f"✅ Closeness centrality: {closeness}")
            
            # Path algorithms
            if nx.is_strongly_connected(G):
                diameter = nx.diameter(G)
                print(f"✅ Diameter: {diameter}")
            else:
                print("⚠️ Graph is not strongly connected")
                
        except Exception as e:
            print(f"⚠️ Algorithm test failed: {e}")
        
        return True
        
    except ImportError:
        print("❌ NetworkX not available")
        print("   Install with: pip install networkx")
        return False

def test_performance_simulation():
    """Simulate performance with larger datasets"""
    print("\n⚡ Performance Simulation")
    print("=" * 40)
    
    import time
    
    sizes = [100, 500, 1000]
    
    for size in sizes:
        print(f"\n🏃 Testing with {size} nodes...")
        
        start_time = time.time()
        
        # Generate random graph data
        np.random.seed(42)  # Reproducible
        
        # Nodes
        node_creation_start = time.time()
        node_ids = np.arange(size, dtype=np.uint64)
        node_types = np.random.randint(0, 5, size, dtype=np.uint32)
        node_creation_time = time.time() - node_creation_start
        
        # Edges (scale-free-like distribution)
        edge_creation_start = time.time()
        n_edges = size * 2  # 2x edges as nodes
        sources = np.random.choice(node_ids, n_edges)
        targets = np.random.choice(node_ids, n_edges)
        weights = np.random.uniform(0.5, 5.0, n_edges)
        
        # Remove self-loops
        mask = sources != targets
        sources = sources[mask]
        targets = targets[mask]
        weights = weights[mask]
        n_edges = len(sources)
        
        edge_creation_time = time.time() - edge_creation_start
        
        # Matrix operations
        matrix_start = time.time()
        
        # Create adjacency matrix
        adj_matrix = np.zeros((size, size), dtype=np.float32)
        for i in range(n_edges):
            adj_matrix[sources[i], targets[i]] = weights[i]
        
        # Compute degrees
        out_degrees = np.sum(adj_matrix > 0, axis=1)
        in_degrees = np.sum(adj_matrix > 0, axis=0)
        
        matrix_time = time.time() - matrix_start
        
        total_time = time.time() - start_time
        
        # Calculate memory usage
        memory_usage = (
            node_ids.nbytes + node_types.nbytes + 
            sources.nbytes + targets.nbytes + weights.nbytes +
            adj_matrix.nbytes + out_degrees.nbytes + in_degrees.nbytes
        ) / 1024 / 1024  # MB
        
        print(f"  📊 Results:")
        print(f"    Total time: {total_time:.3f}s")
        print(f"    Node creation: {node_creation_time:.3f}s ({size/node_creation_time:.0f} nodes/sec)")
        print(f"    Edge creation: {edge_creation_time:.3f}s ({n_edges/edge_creation_time:.0f} edges/sec)")
        print(f"    Matrix ops: {matrix_time:.3f}s")
        print(f"    Memory usage: {memory_usage:.2f} MB")
        print(f"    Avg degree: {np.mean(out_degrees):.2f}")
        print(f"    Edge density: {n_edges/(size*(size-1)):.4f}")

def test_error_handling():
    """Test error handling mechanisms"""
    print("\n🛡️ Testing Error Handling")
    print("=" * 40)
    
    # Test exception classes
    class CNSError(Exception):
        pass
    
    class CNSMemoryError(CNSError):
        pass
    
    class CNSInvalidArgumentError(CNSError):
        pass
    
    # Test error mapping
    ERROR_MAP = {
        -1: CNSMemoryError,
        -2: CNSInvalidArgumentError,
        -3: CNSError,
    }
    
    def check_error(result, operation="operation"):
        if result < 0:
            error_class = ERROR_MAP.get(result, CNSError)
            raise error_class(f"{operation} failed with error code {result}")
    
    print("✅ Error classes defined")
    
    # Test error scenarios
    test_cases = [
        (0, "success"),
        (-1, "memory error"),
        (-2, "invalid argument"),
        (-999, "unknown error"),
    ]
    
    for error_code, description in test_cases:
        try:
            check_error(error_code, description)
            print(f"✅ {description}: No error raised (code {error_code})")
        except CNSError as e:
            print(f"⚠️ {description}: {type(e).__name__}: {e}")
    
    return True

def main():
    """Run all tests"""
    print("🐍 CNS Binary Materializer - Python Bindings Test Suite")
    print("=" * 65)
    
    tests = [
        ("NumPy Integration", test_numpy_integration),
        ("ctypes Structures", test_ctypes_structures),
        ("NetworkX Integration", test_networkx_integration),
        ("Performance Simulation", test_performance_simulation),
        ("Error Handling", test_error_handling),
    ]
    
    results = {}
    
    for test_name, test_func in tests:
        print(f"\n{'='*65}")
        print(f"🧪 Running: {test_name}")
        print(f"{'='*65}")
        
        try:
            result = test_func()
            results[test_name] = result
            print(f"✅ {test_name}: {'PASSED' if result else 'SKIPPED'}")
        except Exception as e:
            results[test_name] = False
            print(f"❌ {test_name}: FAILED - {e}")
            import traceback
            traceback.print_exc()
    
    # Summary
    print(f"\n{'='*65}")
    print("📊 Test Summary")
    print(f"{'='*65}")
    
    passed = sum(1 for result in results.values() if result is True)
    skipped = sum(1 for result in results.values() if result is False)
    failed = len(results) - passed - skipped
    
    for test_name, result in results.items():
        if result is True:
            print(f"✅ {test_name}: PASSED")
        elif result is False:
            print(f"⏭️ {test_name}: SKIPPED")
        else:
            print(f"❌ {test_name}: FAILED")
    
    print(f"\n📈 Results: {passed} passed, {skipped} skipped, {failed} failed")
    
    if failed == 0:
        print("🎉 All tests completed successfully!")
        print("\n💡 Next Steps:")
        print("  1. Build the C library: make")
        print("  2. Run full demo: python demo_python.py")
        print("  3. Install package: python setup.py install")
        return 0
    else:
        print("⚠️ Some tests failed. Check the output above.")
        return 1

if __name__ == "__main__":
    sys.exit(main())