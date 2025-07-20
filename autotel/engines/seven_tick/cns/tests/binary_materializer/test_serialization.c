/*
 * CNS Binary Materializer - Serialization Tests
 * Comprehensive test suite for graph serialization
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include "cns/binary_materializer.h"
#include "cns/binary_materializer_types.h"

// Test basic serialization
static void test_basic_serialization() {
    printf("Testing basic serialization...\n");
    
    // Create a simple graph
    cns_graph_t* graph = cns_graph_create(CNS_GRAPH_FLAG_DIRECTED);
    assert(graph != NULL);
    
    // Add nodes
    assert(cns_graph_add_node(graph, 1, 0x1001, "Node1", 5) == CNS_SUCCESS);
    assert(cns_graph_add_node(graph, 2, 0x1002, "Node2", 5) == CNS_SUCCESS);
    assert(cns_graph_add_node(graph, 3, 0x1003, "Node3", 5) == CNS_SUCCESS);
    
    // Add edges
    assert(cns_graph_add_edge(graph, 1, 2, 0x2001, 1.0, NULL, 0) == CNS_SUCCESS);
    assert(cns_graph_add_edge(graph, 2, 3, 0x2002, 2.0, NULL, 0) == CNS_SUCCESS);
    assert(cns_graph_add_edge(graph, 3, 1, 0x2003, 3.0, NULL, 0) == CNS_SUCCESS);
    
    // Serialize
    cns_write_buffer_t* buffer = cns_write_buffer_create(1024);
    assert(buffer != NULL);
    
    assert(cns_graph_serialize(graph, buffer, CNS_FLAG_BUILD_INDEX) == CNS_SUCCESS);
    assert(buffer->size > 0);
    
    printf("  Serialized size: %zu bytes\n", buffer->size);
    
    // Cleanup
    cns_write_buffer_destroy(buffer);
    cns_graph_destroy(graph);
    
    printf("  ✓ Basic serialization test passed\n");
}

// Test large graph serialization
static void test_large_graph_serialization() {
    printf("Testing large graph serialization...\n");
    
    const size_t node_count = 10000;
    const size_t edge_count = 50000;
    
    // Create large graph
    cns_graph_t* graph = cns_graph_create(CNS_GRAPH_FLAG_DIRECTED | CNS_GRAPH_FLAG_WEIGHTED);
    assert(graph != NULL);
    
    // Add nodes
    clock_t start = clock();
    for (size_t i = 0; i < node_count; i++) {
        char data[32];
        snprintf(data, sizeof(data), "Node%zu", i);
        assert(cns_graph_add_node(graph, i, 0x1000 + (i % 10), data, strlen(data)) == CNS_SUCCESS);
    }
    
    // Add random edges
    srand(42);  // Deterministic for testing
    for (size_t i = 0; i < edge_count; i++) {
        uint64_t src = rand() % node_count;
        uint64_t dst = rand() % node_count;
        double weight = (double)rand() / RAND_MAX;
        assert(cns_graph_add_edge(graph, src, dst, 0x2000, weight, NULL, 0) == CNS_SUCCESS);
    }
    
    clock_t end = clock();
    double creation_time = (double)(end - start) / CLOCKS_PER_SEC;
    printf("  Graph creation time: %.3f seconds\n", creation_time);
    
    // Serialize
    cns_write_buffer_t* buffer = cns_write_buffer_create(1024 * 1024);
    assert(buffer != NULL);
    
    start = clock();
    assert(cns_graph_serialize(graph, buffer, CNS_FLAG_BUILD_INDEX | CNS_FLAG_COMPRESS_VARINTS) == CNS_SUCCESS);
    end = clock();
    
    double serialize_time = (double)(end - start) / CLOCKS_PER_SEC;
    double throughput = (double)buffer->size / (1024 * 1024) / serialize_time;
    
    printf("  Serialization time: %.3f seconds\n", serialize_time);
    printf("  Serialized size: %.2f MB\n", (double)buffer->size / (1024 * 1024));
    printf("  Throughput: %.2f MB/s\n", throughput);
    
    // Cleanup
    cns_write_buffer_destroy(buffer);
    cns_graph_destroy(graph);
    
    printf("  ✓ Large graph serialization test passed\n");
}

// Test round-trip serialization/deserialization
static void test_round_trip() {
    printf("Testing round-trip serialization/deserialization...\n");
    
    // Create original graph
    cns_graph_t* original = cns_graph_create(CNS_GRAPH_FLAG_DIRECTED | CNS_GRAPH_FLAG_WEIGHTED);
    assert(original != NULL);
    
    // Add nodes with properties
    const char* node_data[] = {"Alpha", "Beta", "Gamma", "Delta"};
    for (size_t i = 0; i < 4; i++) {
        assert(cns_graph_add_node(original, i, 0x1000 + i, node_data[i], strlen(node_data[i])) == CNS_SUCCESS);
    }
    
    // Add edges with weights
    assert(cns_graph_add_edge(original, 0, 1, 0x2001, 1.5, "Edge01", 6) == CNS_SUCCESS);
    assert(cns_graph_add_edge(original, 1, 2, 0x2002, 2.5, "Edge12", 6) == CNS_SUCCESS);
    assert(cns_graph_add_edge(original, 2, 3, 0x2003, 3.5, "Edge23", 6) == CNS_SUCCESS);
    assert(cns_graph_add_edge(original, 3, 0, 0x2004, 4.5, "Edge30", 6) == CNS_SUCCESS);
    
    // Serialize
    cns_write_buffer_t* buffer = cns_write_buffer_create(1024);
    assert(cns_graph_serialize(original, buffer, CNS_FLAG_BUILD_INDEX) == CNS_SUCCESS);
    
    // Deserialize
    cns_read_buffer_t* read_buf = cns_read_buffer_create(buffer->data, buffer->size);
    cns_graph_t* restored = cns_graph_create(0);
    assert(cns_graph_deserialize(restored, read_buf, 0) == CNS_SUCCESS);
    
    // Verify graph properties
    assert(restored->node_count == original->node_count);
    assert(restored->edge_count == original->edge_count);
    assert(restored->flags == original->flags);
    
    // Verify nodes
    for (size_t i = 0; i < original->node_count; i++) {
        assert(restored->nodes[i].id == original->nodes[i].id);
        assert(restored->nodes[i].type == original->nodes[i].type);
        assert(restored->nodes[i].data_size == original->nodes[i].data_size);
        assert(memcmp(restored->nodes[i].data, original->nodes[i].data, original->nodes[i].data_size) == 0);
    }
    
    // Verify edges
    for (size_t i = 0; i < original->edge_count; i++) {
        assert(restored->edges[i].source == original->edges[i].source);
        assert(restored->edges[i].target == original->edges[i].target);
        assert(restored->edges[i].type == original->edges[i].type);
        assert(restored->edges[i].weight == original->edges[i].weight);
        assert(restored->edges[i].data_size == original->edges[i].data_size);
        if (original->edges[i].data_size > 0) {
            assert(memcmp(restored->edges[i].data, original->edges[i].data, original->edges[i].data_size) == 0);
        }
    }
    
    // Cleanup
    cns_read_buffer_destroy(read_buf);
    cns_write_buffer_destroy(buffer);
    cns_graph_destroy(original);
    cns_graph_destroy(restored);
    
    printf("  ✓ Round-trip test passed\n");
}

// Test file I/O
static void test_file_io() {
    printf("Testing file I/O...\n");
    
    const char* test_file = "test_graph.cnsb";
    
    // Create and save graph
    cns_graph_t* graph = cns_graph_create(CNS_GRAPH_FLAG_DIRECTED);
    for (int i = 0; i < 100; i++) {
        assert(cns_graph_add_node(graph, i, 0x1000, NULL, 0) == CNS_SUCCESS);
    }
    for (int i = 0; i < 99; i++) {
        assert(cns_graph_add_edge(graph, i, i + 1, 0x2000, 1.0, NULL, 0) == CNS_SUCCESS);
    }
    
    assert(cns_graph_serialize_to_file(graph, test_file, CNS_FLAG_BUILD_INDEX) == CNS_SUCCESS);
    cns_graph_destroy(graph);
    
    // Load graph
    cns_graph_t* loaded = cns_graph_create(0);
    assert(cns_graph_deserialize_from_file(loaded, test_file, 0) == CNS_SUCCESS);
    
    assert(loaded->node_count == 100);
    assert(loaded->edge_count == 99);
    
    cns_graph_destroy(loaded);
    
    // Remove test file
    remove(test_file);
    
    printf("  ✓ File I/O test passed\n");
}

// Test zero-copy view
static void test_zero_copy_view() {
    printf("Testing zero-copy view...\n");
    
    const char* test_file = "test_view.cnsb";
    
    // Create and save graph with index
    cns_graph_t* graph = cns_graph_create(CNS_GRAPH_FLAG_DIRECTED);
    for (int i = 0; i < 1000; i++) {
        char data[32];
        snprintf(data, sizeof(data), "Node%d", i);
        assert(cns_graph_add_node(graph, i, 0x1000, data, strlen(data)) == CNS_SUCCESS);
    }
    
    assert(cns_graph_serialize_to_file(graph, test_file, CNS_FLAG_BUILD_INDEX) == CNS_SUCCESS);
    cns_graph_destroy(graph);
    
    // Open zero-copy view
    cns_graph_view_t view;
    assert(cns_graph_view_open(&view, test_file) == CNS_SUCCESS);
    
    // Test node access
    cns_node_view_t node_view;
    assert(cns_graph_view_get_node(&view, 500, &node_view) == CNS_SUCCESS);
    assert(node_view.node_id == 500);
    
    // Close view
    cns_graph_view_close(&view);
    
    // Remove test file
    remove(test_file);
    
    printf("  ✓ Zero-copy view test passed\n");
}

// Performance benchmark
static void benchmark_performance() {
    printf("\nPerformance Benchmark:\n");
    printf("======================\n");
    
    size_t node_counts[] = {100, 1000, 10000, 100000};
    
    for (size_t i = 0; i < sizeof(node_counts) / sizeof(node_counts[0]); i++) {
        size_t nodes = node_counts[i];
        size_t edges = nodes * 5;  // Average degree of 5
        
        printf("\nGraph size: %zu nodes, %zu edges\n", nodes, edges);
        
        // Create graph
        cns_graph_t* graph = cns_graph_create(CNS_GRAPH_FLAG_DIRECTED | CNS_GRAPH_FLAG_WEIGHTED);
        
        for (size_t j = 0; j < nodes; j++) {
            cns_graph_add_node(graph, j, 0x1000, NULL, 0);
        }
        
        srand(42);
        for (size_t j = 0; j < edges; j++) {
            uint64_t src = rand() % nodes;
            uint64_t dst = rand() % nodes;
            cns_graph_add_edge(graph, src, dst, 0x2000, 1.0, NULL, 0);
        }
        
        // Benchmark serialization
        cns_write_buffer_t* buffer = cns_write_buffer_create(nodes * 100);
        
        clock_t start = clock();
        cns_graph_serialize(graph, buffer, CNS_FLAG_BUILD_INDEX | CNS_FLAG_COMPRESS_VARINTS);
        clock_t end = clock();
        
        double ser_time = (double)(end - start) / CLOCKS_PER_SEC;
        double ser_throughput = (double)buffer->size / (1024 * 1024) / ser_time;
        
        printf("  Serialization: %.3fs, %.2f MB/s\n", ser_time, ser_throughput);
        
        // Benchmark deserialization
        cns_read_buffer_t* read_buf = cns_read_buffer_create(buffer->data, buffer->size);
        cns_graph_t* restored = cns_graph_create(0);
        
        start = clock();
        cns_graph_deserialize(restored, read_buf, 0);
        end = clock();
        
        double deser_time = (double)(end - start) / CLOCKS_PER_SEC;
        double deser_throughput = (double)buffer->size / (1024 * 1024) / deser_time;
        
        printf("  Deserialization: %.3fs, %.2f MB/s\n", deser_time, deser_throughput);
        printf("  Compressed size: %.2f MB\n", (double)buffer->size / (1024 * 1024));
        printf("  Compression ratio: %.2fx\n", 
               (double)(sizeof(cns_node_t) * nodes + sizeof(cns_edge_t) * edges) / buffer->size);
        
        // Cleanup
        cns_graph_destroy(graph);
        cns_graph_destroy(restored);
        cns_write_buffer_destroy(buffer);
        cns_read_buffer_destroy(read_buf);
    }
}

int main() {
    printf("CNS Binary Materializer Test Suite\n");
    printf("==================================\n\n");
    
    // Initialize buffer cache
    cns_buffer_cache_init();
    
    // Run tests
    test_basic_serialization();
    test_large_graph_serialization();
    test_round_trip();
    test_file_io();
    test_zero_copy_view();
    
    // Run benchmarks
    benchmark_performance();
    
    // Cleanup
    cns_buffer_cache_cleanup();
    
    printf("\n✅ All tests passed!\n");
    
    return 0;
}