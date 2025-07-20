/*
 * CNS Binary Materializer - Simple Test Suite
 * Basic functionality testing
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include "cns/binary_materializer.h"

// Test basic graph creation
static void test_graph_creation() {
    printf("Testing graph creation...\n");
    
    cns_graph_t* graph = cns_graph_create(10, 20);
    assert(graph != NULL);
    assert(graph->node_capacity >= 10);
    assert(graph->edge_capacity >= 20);
    assert(graph->nodes != NULL);
    assert(graph->edges != NULL);
    assert(graph->data_pool != NULL);
    
    cns_graph_destroy(graph);
    printf("  ✓ Graph creation test passed\n");
}

// Test buffer operations
static void test_buffer_operations() {
    printf("Testing buffer operations...\n");
    
    // Test write buffer
    cns_write_buffer_t* wbuf = cns_write_buffer_create(1024);
    assert(wbuf != NULL);
    
    uint32_t test_u32 = 0x12345678;
    uint16_t test_u16 = 0xABCD;
    assert(cns_write_buffer_write_u32(wbuf, test_u32) == CNS_SERIALIZE_OK);
    assert(cns_write_buffer_write_u16(wbuf, test_u16) == CNS_SERIALIZE_OK);
    assert(wbuf->position == 6);
    
    // Test read buffer
    cns_read_buffer_t* rbuf = cns_read_buffer_create(wbuf->data, wbuf->size);
    assert(rbuf != NULL);
    
    uint32_t read_u32;
    uint16_t read_u16;
    assert(cns_read_buffer_read_u32(rbuf, &read_u32) == CNS_SERIALIZE_OK);
    assert(cns_read_buffer_read_u16(rbuf, &read_u16) == CNS_SERIALIZE_OK);
    assert(read_u32 == test_u32);
    assert(read_u16 == test_u16);
    
    cns_write_buffer_destroy(wbuf);
    cns_read_buffer_destroy(rbuf);
    
    printf("  ✓ Buffer operations test passed\n");
}

// Test simple serialization
static void test_simple_serialization() {
    printf("Testing simple serialization...\n");
    
    // Create graph with some nodes and edges
    cns_graph_t* graph = cns_graph_create(10, 10);
    
    // Add nodes
    for (uint32_t i = 0; i < 5; i++) {
        graph->nodes[i].base.id = i;
        graph->nodes[i].base.type = 0x1000 + i;
        graph->nodes[i].first_out_edge = i * 2;
        graph->nodes[i].first_in_edge = i * 2 + 1;
    }
    graph->node_count = 5;
    
    // Add edges
    for (uint32_t i = 0; i < 4; i++) {
        graph->edges[i].base.id = i + 100;
        graph->edges[i].base.type = 0x2000;
        graph->edges[i].source_id = i;
        graph->edges[i].target_id = i + 1;
    }
    graph->edge_count = 4;
    
    // Serialize
    cns_write_buffer_t* buffer = cns_write_buffer_create(4096);
    int ret = cns_graph_serialize(graph, buffer, CNS_SERIALIZE_FLAG_CHECKSUM);
    assert(ret == CNS_SERIALIZE_OK);
    
    printf("  Serialized size: %zu bytes\n", buffer->size);
    
    // Deserialize
    cns_graph_t* loaded = cns_graph_create(0, 0);
    cns_read_buffer_t* rbuf = cns_read_buffer_create(buffer->data, buffer->size);
    ret = cns_graph_deserialize(loaded, rbuf, CNS_SERIALIZE_FLAG_CHECKSUM);
    assert(ret == CNS_SERIALIZE_OK);
    
    // Verify
    assert(loaded->node_count == graph->node_count);
    assert(loaded->edge_count == graph->edge_count);
    
    for (uint32_t i = 0; i < graph->node_count; i++) {
        assert(loaded->nodes[i].base.id == graph->nodes[i].base.id);
        assert(loaded->nodes[i].base.type == graph->nodes[i].base.type);
    }
    
    for (uint32_t i = 0; i < graph->edge_count; i++) {
        assert(loaded->edges[i].base.id == graph->edges[i].base.id);
        assert(loaded->edges[i].source_id == graph->edges[i].source_id);
        assert(loaded->edges[i].target_id == graph->edges[i].target_id);
    }
    
    // Cleanup
    cns_graph_destroy(graph);
    cns_graph_destroy(loaded);
    cns_write_buffer_destroy(buffer);
    cns_read_buffer_destroy(rbuf);
    
    printf("  ✓ Simple serialization test passed\n");
}

// Performance benchmark
static void benchmark_performance() {
    printf("\nPerformance Benchmark:\n");
    printf("======================\n");
    
    size_t node_counts[] = {100, 1000, 10000};
    
    for (size_t i = 0; i < sizeof(node_counts) / sizeof(node_counts[0]); i++) {
        size_t nodes = node_counts[i];
        size_t edges = nodes * 5;
        
        printf("\nGraph size: %zu nodes, %zu edges\n", nodes, edges);
        
        // Create graph
        cns_graph_t* graph = cns_graph_create(nodes, edges);
        
        // Fill with data
        for (size_t j = 0; j < nodes; j++) {
            graph->nodes[j].base.id = j;
            graph->nodes[j].base.type = 0x1000;
        }
        graph->node_count = nodes;
        
        for (size_t j = 0; j < edges; j++) {
            graph->edges[j].base.id = j + 10000;
            graph->edges[j].source_id = j % nodes;
            graph->edges[j].target_id = (j + 1) % nodes;
        }
        graph->edge_count = edges;
        
        // Benchmark serialization
        cns_write_buffer_t* buffer = cns_write_buffer_create(nodes * 100);
        
        clock_t start = clock();
        cns_graph_serialize(graph, buffer, 0);
        clock_t end = clock();
        
        double ser_time = (double)(end - start) / CLOCKS_PER_SEC;
        double throughput = (double)buffer->size / (1024 * 1024) / ser_time;
        
        printf("  Serialization: %.3fs, %.2f MB/s\n", ser_time, throughput);
        printf("  Serialized size: %.2f MB\n", (double)buffer->size / (1024 * 1024));
        
        // Cleanup
        cns_graph_destroy(graph);
        cns_write_buffer_destroy(buffer);
    }
}

int main() {
    printf("CNS Binary Materializer - Simple Test Suite\n");
    printf("==========================================\n\n");
    
    // Run tests
    test_graph_creation();
    test_buffer_operations();
    test_simple_serialization();
    
    // Run benchmarks
    benchmark_performance();
    
    printf("\n✅ All tests passed!\n");
    
    return 0;
}