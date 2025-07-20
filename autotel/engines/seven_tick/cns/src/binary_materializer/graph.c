/*
 * CNS Binary Materializer - Graph Management
 * Core graph data structure operations
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cns/binary_materializer.h"
#include "cns/binary_materializer_types.h"

// Create new graph
cns_graph_t* cns_graph_create(uint32_t flags) {
    cns_graph_t* graph = calloc(1, sizeof(cns_graph_t));
    if (!graph) return NULL;
    
    graph->flags = flags;
    graph->node_capacity = 16;
    graph->edge_capacity = 32;
    
    graph->nodes = calloc(graph->node_capacity, sizeof(cns_node_t));
    if (!graph->nodes) {
        free(graph);
        return NULL;
    }
    
    graph->edges = calloc(graph->edge_capacity, sizeof(cns_edge_t));
    if (!graph->edges) {
        free(graph->nodes);
        free(graph);
        return NULL;
    }
    
    return graph;
}

// Destroy graph and free resources
void cns_graph_destroy(cns_graph_t* graph) {
    if (!graph) return;
    
    // Free node data
    for (size_t i = 0; i < graph->node_count; i++) {
        free(graph->nodes[i].data);
    }
    free(graph->nodes);
    
    // Free edge data
    for (size_t i = 0; i < graph->edge_count; i++) {
        free(graph->edges[i].data);
    }
    free(graph->edges);
    
    free(graph);
}

// Ensure node capacity
static int ensure_node_capacity(cns_graph_t* graph, size_t required) {
    if (graph->node_capacity >= required) return CNS_SUCCESS;
    
    size_t new_capacity = graph->node_capacity * 2;
    while (new_capacity < required) {
        new_capacity *= 2;
    }
    
    cns_node_t* new_nodes = realloc(graph->nodes, new_capacity * sizeof(cns_node_t));
    if (!new_nodes) return CNS_ERROR_MEMORY;
    
    // Zero new memory
    memset(new_nodes + graph->node_capacity, 0, 
           (new_capacity - graph->node_capacity) * sizeof(cns_node_t));
    
    graph->nodes = new_nodes;
    graph->node_capacity = new_capacity;
    return CNS_SUCCESS;
}

// Ensure edge capacity
static int ensure_edge_capacity(cns_graph_t* graph, size_t required) {
    if (graph->edge_capacity >= required) return CNS_SUCCESS;
    
    size_t new_capacity = graph->edge_capacity * 2;
    while (new_capacity < required) {
        new_capacity *= 2;
    }
    
    cns_edge_t* new_edges = realloc(graph->edges, new_capacity * sizeof(cns_edge_t));
    if (!new_edges) return CNS_ERROR_MEMORY;
    
    // Zero new memory
    memset(new_edges + graph->edge_capacity, 0, 
           (new_capacity - graph->edge_capacity) * sizeof(cns_edge_t));
    
    graph->edges = new_edges;
    graph->edge_capacity = new_capacity;
    return CNS_SUCCESS;
}

// Add node to graph
int cns_graph_add_node(cns_graph_t* graph, uint64_t id, uint32_t type, 
                      const void* data, size_t data_size) {
    if (!graph) return CNS_ERROR_INVALID_ARGUMENT;
    
    int ret = ensure_node_capacity(graph, graph->node_count + 1);
    if (ret != CNS_SUCCESS) return ret;
    
    cns_node_t* node = &graph->nodes[graph->node_count];
    node->id = id;
    node->type = type;
    node->flags = 0;
    node->data_size = data_size;
    
    if (data && data_size > 0) {
        node->data = malloc(data_size);
        if (!node->data) return CNS_ERROR_MEMORY;
        memcpy(node->data, data, data_size);
    } else {
        node->data = NULL;
    }
    
    graph->node_count++;
    return CNS_SUCCESS;
}

// Add edge to graph
int cns_graph_add_edge(cns_graph_t* graph, uint64_t source, uint64_t target,
                      uint32_t type, double weight, const void* data, size_t data_size) {
    if (!graph) return CNS_ERROR_INVALID_ARGUMENT;
    
    int ret = ensure_edge_capacity(graph, graph->edge_count + 1);
    if (ret != CNS_SUCCESS) return ret;
    
    cns_edge_t* edge = &graph->edges[graph->edge_count];
    edge->source = source;
    edge->target = target;
    edge->type = type;
    edge->weight = weight;
    edge->flags = 0;
    edge->data_size = data_size;
    
    if (data && data_size > 0) {
        edge->data = malloc(data_size);
        if (!edge->data) return CNS_ERROR_MEMORY;
        memcpy(edge->data, data, data_size);
    } else {
        edge->data = NULL;
    }
    
    graph->edge_count++;
    return CNS_SUCCESS;
}

// Find node by ID (O(n) - use index for O(1))
cns_node_t* cns_graph_find_node(cns_graph_t* graph, uint64_t id) {
    if (!graph) return NULL;
    
    for (size_t i = 0; i < graph->node_count; i++) {
        if (graph->nodes[i].id == id) {
            return &graph->nodes[i];
        }
    }
    
    return NULL;
}

// Get node neighbors (outgoing edges)
int cns_graph_get_neighbors(cns_graph_t* graph, uint64_t node_id,
                           uint64_t** neighbors, size_t* count) {
    if (!graph || !neighbors || !count) {
        return CNS_ERROR_INVALID_ARGUMENT;
    }
    
    // Count outgoing edges
    size_t neighbor_count = 0;
    for (size_t i = 0; i < graph->edge_count; i++) {
        if (graph->edges[i].source == node_id) {
            neighbor_count++;
        }
    }
    
    if (neighbor_count == 0) {
        *neighbors = NULL;
        *count = 0;
        return CNS_SUCCESS;
    }
    
    // Allocate neighbor array
    *neighbors = malloc(neighbor_count * sizeof(uint64_t));
    if (!*neighbors) return CNS_ERROR_MEMORY;
    
    // Fill neighbor array
    size_t idx = 0;
    for (size_t i = 0; i < graph->edge_count; i++) {
        if (graph->edges[i].source == node_id) {
            (*neighbors)[idx++] = graph->edges[i].target;
        }
    }
    
    *count = neighbor_count;
    return CNS_SUCCESS;
}

// Clone graph
cns_graph_t* cns_graph_clone(const cns_graph_t* graph) {
    if (!graph) return NULL;
    
    cns_graph_t* clone = cns_graph_create(graph->flags);
    if (!clone) return NULL;
    
    // Clone nodes
    for (size_t i = 0; i < graph->node_count; i++) {
        const cns_node_t* node = &graph->nodes[i];
        int ret = cns_graph_add_node(clone, node->id, node->type, 
                                    node->data, node->data_size);
        if (ret != CNS_SUCCESS) {
            cns_graph_destroy(clone);
            return NULL;
        }
    }
    
    // Clone edges
    for (size_t i = 0; i < graph->edge_count; i++) {
        const cns_edge_t* edge = &graph->edges[i];
        int ret = cns_graph_add_edge(clone, edge->source, edge->target,
                                    edge->type, edge->weight, 
                                    edge->data, edge->data_size);
        if (ret != CNS_SUCCESS) {
            cns_graph_destroy(clone);
            return NULL;
        }
    }
    
    return clone;
}

// Get graph statistics
void cns_graph_get_stats(const cns_graph_t* graph, cns_graph_stats_t* stats) {
    if (!graph || !stats) return;
    
    memset(stats, 0, sizeof(*stats));
    stats->node_count = graph->node_count;
    stats->edge_count = graph->edge_count;
    
    // Calculate memory usage
    stats->memory_usage = sizeof(cns_graph_t);
    stats->memory_usage += graph->node_capacity * sizeof(cns_node_t);
    stats->memory_usage += graph->edge_capacity * sizeof(cns_edge_t);
    
    // Add data sizes
    for (size_t i = 0; i < graph->node_count; i++) {
        stats->memory_usage += graph->nodes[i].data_size;
    }
    
    for (size_t i = 0; i < graph->edge_count; i++) {
        stats->memory_usage += graph->edges[i].data_size;
    }
    
    // Calculate average degree
    if (graph->node_count > 0) {
        stats->avg_degree = (double)graph->edge_count / graph->node_count;
    }
}