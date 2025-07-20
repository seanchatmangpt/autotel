#!/usr/bin/env python3
"""
CNS Binary Materializer - Python Demo
Demonstrates real-world usage of the Python bindings with performance testing
"""

import numpy as np
import time
import sys
import os
from pathlib import Path

# Add current directory to Python path for importing cns_graph
sys.path.insert(0, os.path.dirname(__file__))

try:
    from cns_graph import CNSGraph, CNSError
except ImportError as e:
    print(f"Error importing CNS Graph: {e}")
    print("Make sure the CNS library is built and available.")
    sys.exit(1)

def create_test_graph() -> CNSGraph:
    """Create a test graph with various structures"""
    print("🔨 Creating test graph...")
    
    # Create a directed, weighted graph
    graph = CNSGraph(directed=True, weighted=True)
    
    # Add nodes with different types
    nodes = [
        (1, 0),   # Type 0: Regular node
        (2, 0),
        (3, 1),   # Type 1: Special node
        (4, 0),
        (5, 2),   # Type 2: Hub node
        (6, 0),
        (7, 0),
        (8, 1),
        (9, 0),
        (10, 2),
    ]
    
    for node_id, node_type in nodes:
        # Add some with binary data
        data = f"node_{node_id}_data".encode() if node_id % 3 == 0 else None
        graph.add_node(node_id, node_type, data)
        
    # Add edges with various weights
    edges = [
        (1, 2, 0, 1.0),    # (source, target, type, weight)
        (1, 3, 0, 2.5),
        (2, 4, 1, 1.8),    # Type 1: Strong connection
        (3, 5, 0, 3.2),
        (4, 6, 0, 1.1),
        (5, 7, 2, 4.0),    # Type 2: Critical path
        (5, 8, 0, 2.0),
        (6, 9, 0, 1.5),
        (7, 10, 1, 3.8),
        (8, 10, 0, 2.2),
        (2, 5, 0, 1.7),    # Additional connections
        (3, 8, 1, 2.9),
        (4, 7, 0, 1.3),
    ]
    
    for source, target, edge_type, weight in edges:
        # Add some with binary data
        data = f"edge_{source}_{target}".encode() if (source + target) % 4 == 0 else None
        graph.add_edge(source, target, edge_type, weight, data)
        
    print(f"✅ Created graph with {len(nodes)} nodes and {len(edges)} edges")
    return graph

def demo_basic_operations(graph: CNSGraph):
    """Demonstrate basic graph operations"""
    print("\n📊 Basic Operations Demo")
    print("=" * 50)
    
    # Check if nodes exist
    print("🔍 Node existence checks:")
    for node_id in [1, 5, 15, 100]:
        exists = graph.has_node(node_id)
        print(f"  Node {node_id}: {'✅ exists' if exists else '❌ not found'}")
    
    # Get neighbors
    print("\n🔗 Neighbor queries:")
    for node_id in [1, 5, 10]:
        if graph.has_node(node_id):
            neighbors = graph.neighbors(node_id)
            print(f"  Node {node_id} neighbors: {list(neighbors)}")
    
    # Graph statistics
    print("\n📈 Graph Statistics:")
    stats = graph.stats()
    for key, value in stats.items():
        if isinstance(value, float):
            print(f"  {key}: {value:.2f}")
        else:
            print(f"  {key}: {value}")

def demo_numpy_integration(graph: CNSGraph):
    """Demonstrate NumPy integration and zero-copy access"""
    print("\n🔢 NumPy Integration Demo")
    print("=" * 50)
    
    # Get nodes as NumPy array
    print("📋 Node arrays:")
    nodes = graph.nodes_array()
    print(f"  Node IDs: {nodes}")
    print(f"  NumPy array type: {type(nodes)}")
    print(f"  Data type: {nodes.dtype}")
    print(f"  Shape: {nodes.shape}")
    
    # Get edges as NumPy arrays
    print("\n🔗 Edge arrays:")
    sources, targets, weights = graph.edges_array()
    print(f"  Sources: {sources[:5]}..." if len(sources) > 5 else f"  Sources: {sources}")
    print(f"  Targets: {targets[:5]}..." if len(targets) > 5 else f"  Targets: {targets}")
    print(f"  Weights: {weights[:5]}..." if len(weights) > 5 else f"  Weights: {weights}")
    
    # Adjacency matrix
    print("\n📊 Adjacency Matrix:")
    adj_matrix = graph.adjacency_matrix(dtype=np.float32)
    print(f"  Shape: {adj_matrix.shape}")
    print(f"  Non-zero entries: {np.count_nonzero(adj_matrix)}")
    print(f"  Matrix preview (first 5x5):")
    print(adj_matrix[:5, :5])
    
    # Degree analysis
    print("\n📐 Degree Analysis:")
    degrees = graph.degree_array()
    print(f"  Node degrees: {degrees}")
    print(f"  Average degree: {np.mean(degrees):.2f}")
    print(f"  Max degree: {np.max(degrees)}")
    print(f"  Min degree: {np.min(degrees)}")

def demo_networkx_integration(graph: CNSGraph):
    """Demonstrate NetworkX integration (if available)"""
    print("\n🕸️ NetworkX Integration Demo")
    print("=" * 50)
    
    try:
        import networkx as nx
        
        # Convert to NetworkX
        print("🔄 Converting CNS graph to NetworkX...")
        nx_graph = graph.to_networkx()
        print(f"✅ Converted: {type(nx_graph)}")
        
        # NetworkX analysis
        print("\n📊 NetworkX Analysis:")
        print(f"  Number of nodes: {nx_graph.number_of_nodes()}")
        print(f"  Number of edges: {nx_graph.number_of_edges()}")
        print(f"  Is directed: {nx.is_directed(nx_graph)}")
        print(f"  Is weighted: {nx.is_weighted(nx_graph)}")
        
        # Centrality measures
        print("\n🎯 Centrality Measures:")
        try:
            betweenness = nx.betweenness_centrality(nx_graph, weight='weight')
            closeness = nx.closeness_centrality(nx_graph, distance='weight')
            
            print("  Top 3 betweenness centrality:")
            for node, score in sorted(betweenness.items(), key=lambda x: x[1], reverse=True)[:3]:
                print(f"    Node {node}: {score:.3f}")
                
            print("  Top 3 closeness centrality:")
            for node, score in sorted(closeness.items(), key=lambda x: x[1], reverse=True)[:3]:
                print(f"    Node {node}: {score:.3f}")
                
        except Exception as e:
            print(f"  Centrality calculation failed: {e}")
        
        # Convert back from NetworkX
        print("\n🔄 Converting NetworkX graph back to CNS...")
        roundtrip_graph = CNSGraph.from_networkx(nx_graph)
        roundtrip_stats = roundtrip_graph.stats()
        original_stats = graph.stats()
        
        print("✅ Roundtrip conversion successful!")
        print(f"  Original: {original_stats['node_count']} nodes, {original_stats['edge_count']} edges")
        print(f"  Roundtrip: {roundtrip_stats['node_count']} nodes, {roundtrip_stats['edge_count']} edges")
        
    except ImportError:
        print("❌ NetworkX not available. Skipping NetworkX integration demo.")
        print("   Install with: pip install networkx")

def performance_benchmark(sizes: list = [100, 500, 1000]):
    """Run performance benchmarks"""
    print("\n⚡ Performance Benchmark")
    print("=" * 50)
    
    for size in sizes:
        print(f"\n🏃 Testing with {size} nodes...")
        
        # Create large graph
        start_time = time.time()
        graph = CNSGraph(directed=True, weighted=True)
        
        # Add nodes
        node_start = time.time()
        for i in range(size):
            graph.add_node(i, i % 5)  # 5 different node types
        node_time = time.time() - node_start
        
        # Add edges (create scale-free network)
        edge_start = time.time()
        np.random.seed(42)  # Reproducible results
        for i in range(size * 2):  # 2x edges as nodes
            source = np.random.randint(0, size)
            target = np.random.randint(0, size)
            if source != target:
                weight = np.random.uniform(0.5, 5.0)
                graph.add_edge(source, target, 0, weight)
        edge_time = time.time() - edge_start
        
        # Test NumPy operations
        numpy_start = time.time()
        nodes = graph.nodes_array()
        sources, targets, weights = graph.edges_array()
        adj_matrix = graph.adjacency_matrix()
        degrees = graph.degree_array()
        numpy_time = time.time() - numpy_start
        
        total_time = time.time() - start_time
        stats = graph.stats()
        
        print(f"  📊 Results for {size} nodes:")
        print(f"    Total time: {total_time:.3f}s")
        print(f"    Node creation: {node_time:.3f}s ({size/node_time:.0f} nodes/sec)")
        print(f"    Edge creation: {edge_time:.3f}s ({stats['edge_count']/edge_time:.0f} edges/sec)")
        print(f"    NumPy operations: {numpy_time:.3f}s")
        print(f"    Memory usage: {stats['memory_usage']/1024/1024:.2f} MB")
        print(f"    Average degree: {stats['avg_degree']:.2f}")

def demo_memory_management():
    """Demonstrate memory management and cleanup"""
    print("\n🧹 Memory Management Demo")
    print("=" * 50)
    
    import gc
    import psutil
    import os
    
    # Get initial memory usage
    process = psutil.Process(os.getpid())
    initial_memory = process.memory_info().rss / 1024 / 1024  # MB
    
    print(f"📊 Initial memory usage: {initial_memory:.2f} MB")
    
    # Create and destroy graphs
    graphs = []
    for i in range(5):
        print(f"🔨 Creating graph {i+1}...")
        graph = CNSGraph(directed=True, weighted=True)
        
        # Add substantial data
        for j in range(200):
            graph.add_node(j, j % 3)
            
        for j in range(400):
            source = np.random.randint(0, 200)
            target = np.random.randint(0, 200)
            if source != target:
                graph.add_edge(source, target, 0, np.random.uniform(1, 5))
        
        graphs.append(graph)
        
        current_memory = process.memory_info().rss / 1024 / 1024
        print(f"  Memory after graph {i+1}: {current_memory:.2f} MB (+{current_memory-initial_memory:.2f} MB)")
    
    # Clear graphs and force garbage collection
    print("\n🗑️ Cleaning up graphs...")
    graphs.clear()
    gc.collect()
    
    final_memory = process.memory_info().rss / 1024 / 1024
    print(f"📊 Final memory usage: {final_memory:.2f} MB")
    print(f"🧹 Memory cleaned up: {initial_memory + (final_memory - initial_memory):.2f} MB")

def main():
    """Main demonstration function"""
    print("🚀 CNS Binary Materializer - Python Bindings Demo")
    print("=" * 60)
    
    try:
        # Create test graph
        graph = create_test_graph()
        
        # Run demonstrations
        demo_basic_operations(graph)
        demo_numpy_integration(graph)
        demo_networkx_integration(graph)
        
        # Performance benchmarks
        print("\n" + "=" * 60)
        performance_benchmark([100, 500])  # Keep reasonable for demo
        
        # Memory management demo  
        try:
            import psutil
            demo_memory_management()
        except ImportError:
            print("\n⏭️ Skipping memory management demo (psutil not available)")
            print("   Install with: pip install psutil")
        
        print("\n" + "=" * 60)
        print("🎉 Demo completed successfully!")
        print("\n💡 Next Steps:")
        print("  1. Build the C library: make")
        print("  2. Install NumPy: pip install numpy")  
        print("  3. Optional: pip install networkx psutil")
        print("  4. Run: python demo_python.py")
        
    except CNSError as e:
        print(f"❌ CNS Error: {e}")
        return 1
    except Exception as e:
        print(f"❌ Unexpected error: {e}")
        import traceback
        traceback.print_exc()
        return 1
        
    return 0

if __name__ == "__main__":
    sys.exit(main())