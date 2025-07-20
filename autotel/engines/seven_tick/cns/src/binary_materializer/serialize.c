/*
 * CNS Binary Materializer - Serialization Implementation
 * Graph to binary format conversion with 7-tick optimization
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "cns/binary_materializer.h"
#include "cns/binary_materializer_types.h"

// Write header to buffer
static int write_header(cns_write_buffer_t* buf, const cns_graph_t* graph, uint32_t flags) {
    cns_binary_header_t header = {
        .magic = CNS_BINARY_MAGIC,
        .version = CNS_BINARY_VERSION,
        .flags = flags,
        .timestamp = (uint64_t)time(NULL),
        .graph_flags = graph->flags,
        .node_count = graph->node_count,
        .edge_count = graph->edge_count,
        .checksum = 0  // Will be filled later
    };
    
    // Reserve space for header
    header.metadata_offset = sizeof(cns_binary_header_t);
    
    return cns_write_buffer_append(buf, &header, sizeof(header));
}

// Write node to buffer
static int write_node(cns_write_buffer_t* buf, const cns_node_t* node, uint32_t flags) {
    // Write node ID
    int ret = cns_write_buffer_write_varint(buf, node->id);
    if (ret != CNS_SUCCESS) return ret;
    
    // Write type
    ret = cns_write_buffer_write_varint(buf, node->type);
    if (ret != CNS_SUCCESS) return ret;
    
    // Write flags
    ret = cns_write_buffer_write_varint(buf, node->flags);
    if (ret != CNS_SUCCESS) return ret;
    
    // Write data if present
    if (node->data && node->data_size > 0) {
        ret = cns_write_buffer_write_varint(buf, node->data_size);
        if (ret != CNS_SUCCESS) return ret;
        
        ret = cns_write_buffer_append(buf, node->data, node->data_size);
        if (ret != CNS_SUCCESS) return ret;
    } else {
        ret = cns_write_buffer_write_varint(buf, 0);
        if (ret != CNS_SUCCESS) return ret;
    }
    
    return CNS_SUCCESS;
}

// Write edge to buffer
static int write_edge(cns_write_buffer_t* buf, const cns_edge_t* edge, uint32_t flags) {
    // Write source and target IDs
    int ret = cns_write_buffer_write_varint(buf, edge->source);
    if (ret != CNS_SUCCESS) return ret;
    
    ret = cns_write_buffer_write_varint(buf, edge->target);
    if (ret != CNS_SUCCESS) return ret;
    
    // Write type
    ret = cns_write_buffer_write_varint(buf, edge->type);
    if (ret != CNS_SUCCESS) return ret;
    
    // Write weight if present
    if (flags & CNS_FLAG_WEIGHTED_EDGES) {
        uint64_t weight_bits;
        memcpy(&weight_bits, &edge->weight, sizeof(double));
        ret = cns_write_buffer_append(buf, &weight_bits, sizeof(weight_bits));
        if (ret != CNS_SUCCESS) return ret;
    }
    
    // Write flags
    ret = cns_write_buffer_write_varint(buf, edge->flags);
    if (ret != CNS_SUCCESS) return ret;
    
    // Write data if present
    if (edge->data && edge->data_size > 0) {
        ret = cns_write_buffer_write_varint(buf, edge->data_size);
        if (ret != CNS_SUCCESS) return ret;
        
        ret = cns_write_buffer_append(buf, edge->data, edge->data_size);
        if (ret != CNS_SUCCESS) return ret;
    } else {
        ret = cns_write_buffer_write_varint(buf, 0);
        if (ret != CNS_SUCCESS) return ret;
    }
    
    return CNS_SUCCESS;
}

// Build node index for fast lookup
static int write_node_index(cns_write_buffer_t* buf, const cns_graph_t* graph) {
    // Write index offset table
    uint64_t* offsets = calloc(graph->node_count, sizeof(uint64_t));
    if (!offsets) return CNS_ERROR_MEMORY;
    
    // First pass: calculate offsets
    size_t current_offset = 0;
    for (size_t i = 0; i < graph->node_count; i++) {
        offsets[i] = current_offset;
        // Calculate node size (simplified - would need actual serialization)
        current_offset += 16 + graph->nodes[i].data_size;  // Approximate
    }
    
    // Write offset table
    int ret = cns_write_buffer_append(buf, offsets, graph->node_count * sizeof(uint64_t));
    free(offsets);
    
    return ret;
}

// Main serialization function
int cns_graph_serialize(const cns_graph_t* graph, cns_write_buffer_t* buffer, uint32_t flags) {
    if (!graph || !buffer) {
        return CNS_ERROR_INVALID_ARGUMENT;
    }
    
    // Write header
    int ret = write_header(buffer, graph, flags);
    if (ret != CNS_SUCCESS) return ret;
    
    // Save position for metadata
    size_t metadata_pos = buffer->size;
    cns_binary_metadata_t metadata = {0};
    
    // Reserve space for metadata
    ret = cns_write_buffer_append(buffer, &metadata, sizeof(metadata));
    if (ret != CNS_SUCCESS) return ret;
    
    // Write node index if requested
    if (flags & CNS_FLAG_BUILD_INDEX) {
        metadata.node_index_offset = buffer->size;
        ret = write_node_index(buffer, graph);
        if (ret != CNS_SUCCESS) return ret;
    }
    
    // Write nodes
    metadata.node_data_offset = buffer->size;
    for (size_t i = 0; i < graph->node_count; i++) {
        ret = write_node(buffer, &graph->nodes[i], flags);
        if (ret != CNS_SUCCESS) return ret;
    }
    
    // Write edges
    metadata.edge_data_offset = buffer->size;
    for (size_t i = 0; i < graph->edge_count; i++) {
        ret = write_edge(buffer, &graph->edges[i], flags);
        if (ret != CNS_SUCCESS) return ret;
    }
    
    // Update metadata
    memcpy(buffer->data + metadata_pos, &metadata, sizeof(metadata));
    
    // Calculate and update checksum
    uint32_t checksum = cns_calculate_crc32(buffer->data + sizeof(cns_binary_header_t), 
                                            buffer->size - sizeof(cns_binary_header_t));
    ((cns_binary_header_t*)buffer->data)->checksum = checksum;
    
    return CNS_SUCCESS;
}

// Optimized batch serialization for multiple graphs
int cns_graph_serialize_batch(const cns_graph_t** graphs, size_t count, 
                             cns_write_buffer_t** buffers, uint32_t flags) {
    if (!graphs || !buffers || count == 0) {
        return CNS_ERROR_INVALID_ARGUMENT;
    }
    
    // Process in parallel if possible (simplified sequential version here)
    for (size_t i = 0; i < count; i++) {
        buffers[i] = cns_write_buffer_create(CNS_DEFAULT_BUFFER_SIZE);
        if (!buffers[i]) {
            // Cleanup on error
            for (size_t j = 0; j < i; j++) {
                cns_write_buffer_destroy(buffers[j]);
            }
            return CNS_ERROR_MEMORY;
        }
        
        int ret = cns_graph_serialize(graphs[i], buffers[i], flags);
        if (ret != CNS_SUCCESS) {
            // Cleanup on error
            for (size_t j = 0; j <= i; j++) {
                cns_write_buffer_destroy(buffers[j]);
            }
            return ret;
        }
    }
    
    return CNS_SUCCESS;
}

// Write serialized data to file with .plan.bin support
int cns_graph_serialize_to_file(const cns_graph_t* graph, const char* path, uint32_t flags) {
    // External .plan.bin materializer functions
    extern int cns_materialize_plan_bin(const cns_graph_t *graph, const char *filename);
    extern int cns_materialize_graph_direct(const cns_graph_t *graph, const char *filename);
    
    // Check for .plan.bin extension for optimized serialization
    if (path && strstr(path, ".plan.bin")) {
        return cns_materialize_plan_bin(graph, path);
    }
    
    // Standard binary materializer path
    cns_write_buffer_t* buffer = cns_write_buffer_create(CNS_DEFAULT_BUFFER_SIZE);
    if (!buffer) return CNS_ERROR_MEMORY;
    
    int ret = cns_graph_serialize(graph, buffer, flags);
    if (ret != CNS_SUCCESS) {
        cns_write_buffer_destroy(buffer);
        return ret;
    }
    
    FILE* file = fopen(path, "wb");
    if (!file) {
        cns_write_buffer_destroy(buffer);
        return CNS_ERROR_IO;
    }
    
    size_t written = fwrite(buffer->data, 1, buffer->size, file);
    fclose(file);
    cns_write_buffer_destroy(buffer);
    
    if (written != buffer->size) {
        return CNS_ERROR_IO;
    }
    
    return CNS_SUCCESS;
}