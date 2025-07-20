/*
 * Test Program for .plan.bin Materializer
 * Demonstrates zero-copy memory-mappable binary format generation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "cns/binary_materializer.h"
#include "cns/binary_materializer_types.h"

// External materializer function
extern int cns_materialize_plan_bin(const cns_graph_t *graph, const char *filename);
extern int cns_plan_view_open(cns_graph_view_t *view, const char *filename);
extern void cns_plan_view_close(cns_graph_view_t *view);

// Create a test graph
static cns_graph_t* create_test_graph(void) {
    cns_graph_t *graph = calloc(1, sizeof(cns_graph_t));
    if (!graph) return NULL;
    
    // Allocate nodes and edges
    graph->node_capacity = 10;
    graph->edge_capacity = 20;
    graph->nodes = calloc(graph->node_capacity, sizeof(cns_node_t));
    graph->edges = calloc(graph->edge_capacity, sizeof(cns_edge_t));
    graph->data_pool = calloc(1024, 1); // 1KB data pool
    graph->data_capacity = 1024;
    
    if (!graph->nodes || !graph->edges || !graph->data_pool) {
        free(graph->nodes);
        free(graph->edges);
        free(graph->data_pool);
        free(graph);
        return NULL;
    }
    
    // Create test nodes
    const char* node_data[] = {
        "http://example.org/Person",
        "http://example.org/Alice",
        "http://example.org/Bob",
        "http://example.org/knows",
        "http://example.org/type"
    };
    
    size_t data_offset = 0;
    for (int i = 0; i < 5; i++) {
        graph->nodes[i].base.id = i + 1;
        graph->nodes[i].base.type = (i < 3) ? 1 : 2; // 1=entity, 2=property
        graph->nodes[i].base.flags = 0;
        graph->nodes[i].base.data_offset = data_offset;
        
        // Copy string data
        size_t len = strlen(node_data[i]) + 1;
        if (data_offset + len <= graph->data_capacity) {
            memcpy(graph->data_pool + data_offset, node_data[i], len);
            data_offset += len;
        }
    }
    
    graph->node_count = 5;
    graph->data_size = data_offset;
    
    // Create test edges (triples)
    // Alice rdf:type Person
    graph->edges[0].base.id = 1;
    graph->edges[0].source_id = 2; // Alice
    graph->edges[0].target_id = 1; // Person
    graph->edges[0].base.type = 5; // rdf:type
    graph->edges[0].base.flags = 0;
    
    // Bob rdf:type Person
    graph->edges[1].base.id = 2;
    graph->edges[1].source_id = 3; // Bob
    graph->edges[1].target_id = 1; // Person
    graph->edges[1].base.type = 5; // rdf:type
    graph->edges[1].base.flags = 0;
    
    // Alice knows Bob
    graph->edges[2].base.id = 3;
    graph->edges[2].source_id = 2; // Alice
    graph->edges[2].target_id = 3; // Bob
    graph->edges[2].base.type = 4; // knows
    graph->edges[2].base.flags = 0;
    
    graph->edge_count = 3;
    graph->flags = CNS_GRAPH_FLAG_DIRECTED;
    
    return graph;
}

// Clean up test graph
static void destroy_test_graph(cns_graph_t *graph) {
    if (graph) {
        free(graph->nodes);
        free(graph->edges);
        free(graph->data_pool);
        free(graph);
    }
}

int main(void) {
    printf("CNS .plan.bin Materializer Test\n");
    printf("================================\n\n");
    
    // Initialize buffer cache
    if (cns_buffer_cache_init() != CNS_SUCCESS) {
        fprintf(stderr, "Failed to initialize buffer cache\n");
        return 1;
    }
    
    // Create test graph
    cns_graph_t *graph = create_test_graph();
    if (!graph) {
        fprintf(stderr, "Failed to create test graph\n");
        cns_buffer_cache_cleanup();
        return 1;
    }
    
    printf("Created test graph:\n");
    printf("  Nodes: %zu\n", graph->node_count);
    printf("  Edges: %zu\n", graph->edge_count);
    printf("  Data pool: %zu bytes\n\n", graph->data_size);
    
    // Test .plan.bin serialization
    const char *filename = "test_graph.plan.bin";
    printf("Serializing to %s...\n", filename);
    
    int result = cns_materialize_plan_bin(graph, filename);
    if (result != CNS_SUCCESS) {
        fprintf(stderr, "Serialization failed: %d\n", result);
        destroy_test_graph(graph);
        cns_buffer_cache_cleanup();
        return 1;
    }
    
    printf("Serialization completed successfully!\n\n");
    
    // Test memory-mapped view
    printf("Testing memory-mapped view...\n");
    cns_graph_view_t view = {0};
    result = cns_plan_view_open(&view, filename);
    if (result != CNS_SUCCESS) {
        fprintf(stderr, "Failed to open memory-mapped view: %d\n", result);
        destroy_test_graph(graph);
        cns_buffer_cache_cleanup();
        return 1;
    }
    
    printf("Memory-mapped view opened successfully!\n");
    printf("  Mapped size: %zu bytes\n", view.region.size);
    printf("  Header magic: 0x%08X\n", view.header->magic);
    
    // Close view
    cns_plan_view_close(&view);
    printf("Memory-mapped view closed.\n\n");
    
    // Test integration with existing serializer
    printf("Testing integration with existing serializer...\n");
    result = cns_graph_serialize_to_file(graph, "test_integrated.plan.bin", 
                                        CNS_SERIALIZE_FLAG_CHECKSUM);
    if (result != CNS_SUCCESS) {
        fprintf(stderr, "Integrated serialization failed: %d\n", result);
        destroy_test_graph(graph);
        cns_buffer_cache_cleanup();
        return 1;
    }
    
    printf("Integrated serialization completed successfully!\n");
    
    // Clean up
    destroy_test_graph(graph);
    cns_buffer_cache_cleanup();
    
    printf("\nAll tests passed! ✓\n");
    printf("\nGenerated files:\n");
    printf("  - test_graph.plan.bin (direct materializer)\n");
    printf("  - test_integrated.plan.bin (integrated with existing)\n");
    
    return 0;
}