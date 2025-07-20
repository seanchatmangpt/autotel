/*
 * CNS Binary Materializer - Deserialization Implementation
 * Binary format to graph conversion with zero-copy optimization
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "cns/binary_materializer.h"
#include "cns/binary_materializer_types.h"

// Validate binary header
static int validate_header(const cns_binary_header_t* header, size_t file_size) {
    if (header->magic != CNS_BINARY_MAGIC) {
        return CNS_ERROR_INVALID_FORMAT;
    }
    
    if (header->version > CNS_BINARY_VERSION) {
        return CNS_ERROR_UNSUPPORTED_VERSION;
    }
    
    if (header->metadata_offset >= file_size) {
        return CNS_ERROR_INVALID_FORMAT;
    }
    
    return CNS_SUCCESS;
}

// Read node from buffer
static int read_node(cns_read_buffer_t* buf, cns_node_t* node) {
    // Read node ID
    int ret = cns_read_buffer_read_varint(buf, &node->id);
    if (ret != CNS_SUCCESS) return ret;
    
    // Read type
    uint64_t type;
    ret = cns_read_buffer_read_varint(buf, &type);
    if (ret != CNS_SUCCESS) return ret;
    node->type = (uint32_t)type;
    
    // Read flags
    uint64_t flags;
    ret = cns_read_buffer_read_varint(buf, &flags);
    if (ret != CNS_SUCCESS) return ret;
    node->flags = (uint32_t)flags;
    
    // Read data size
    uint64_t data_size;
    ret = cns_read_buffer_read_varint(buf, &data_size);
    if (ret != CNS_SUCCESS) return ret;
    node->data_size = (size_t)data_size;
    
    // Read data if present
    if (node->data_size > 0) {
        node->data = malloc(node->data_size);
        if (!node->data) return CNS_ERROR_MEMORY;
        
        ret = cns_read_buffer_read(buf, node->data, node->data_size);
        if (ret != CNS_SUCCESS) {
            free(node->data);
            node->data = NULL;
            return ret;
        }
    } else {
        node->data = NULL;
    }
    
    return CNS_SUCCESS;
}

// Read edge from buffer
static int read_edge(cns_read_buffer_t* buf, cns_edge_t* edge, uint32_t flags) {
    // Read source and target IDs
    int ret = cns_read_buffer_read_varint(buf, &edge->source);
    if (ret != CNS_SUCCESS) return ret;
    
    ret = cns_read_buffer_read_varint(buf, &edge->target);
    if (ret != CNS_SUCCESS) return ret;
    
    // Read type
    uint64_t type;
    ret = cns_read_buffer_read_varint(buf, &type);
    if (ret != CNS_SUCCESS) return ret;
    edge->type = (uint32_t)type;
    
    // Read weight if present
    if (flags & CNS_FLAG_WEIGHTED_EDGES) {
        uint64_t weight_bits;
        ret = cns_read_buffer_read(buf, &weight_bits, sizeof(weight_bits));
        if (ret != CNS_SUCCESS) return ret;
        memcpy(&edge->weight, &weight_bits, sizeof(double));
    } else {
        edge->weight = 1.0;
    }
    
    // Read flags
    uint64_t edge_flags;
    ret = cns_read_buffer_read_varint(buf, &edge_flags);
    if (ret != CNS_SUCCESS) return ret;
    edge->flags = (uint32_t)edge_flags;
    
    // Read data size
    uint64_t data_size;
    ret = cns_read_buffer_read_varint(buf, &data_size);
    if (ret != CNS_SUCCESS) return ret;
    edge->data_size = (size_t)data_size;
    
    // Read data if present
    if (edge->data_size > 0) {
        edge->data = malloc(edge->data_size);
        if (!edge->data) return CNS_ERROR_MEMORY;
        
        ret = cns_read_buffer_read(buf, edge->data, edge->data_size);
        if (ret != CNS_SUCCESS) {
            free(edge->data);
            edge->data = NULL;
            return ret;
        }
    } else {
        edge->data = NULL;
    }
    
    return CNS_SUCCESS;
}

// Main deserialization function
int cns_graph_deserialize(cns_graph_t* graph, cns_read_buffer_t* buffer, uint32_t flags) {
    if (!graph || !buffer) {
        return CNS_ERROR_INVALID_ARGUMENT;
    }
    
    // Read and validate header
    cns_binary_header_t header;
    int ret = cns_read_buffer_read(buffer, &header, sizeof(header));
    if (ret != CNS_SUCCESS) return ret;
    
    ret = validate_header(&header, buffer->size);
    if (ret != CNS_SUCCESS) return ret;
    
    // Verify checksum if requested
    if (!(flags & CNS_FLAG_SKIP_CHECKSUM)) {
        uint32_t calculated = cns_calculate_crc32(
            (const uint8_t*)buffer->data + sizeof(header),
            buffer->size - sizeof(header)
        );
        if (calculated != header.checksum) {
            return CNS_ERROR_CHECKSUM_MISMATCH;
        }
    }
    
    // Read metadata
    buffer->position = header.metadata_offset;
    cns_binary_metadata_t metadata;
    ret = cns_read_buffer_read(buffer, &metadata, sizeof(metadata));
    if (ret != CNS_SUCCESS) return ret;
    
    // Initialize graph
    graph->flags = header.graph_flags;
    graph->node_count = header.node_count;
    graph->edge_count = header.edge_count;
    
    // Allocate nodes
    graph->nodes = calloc(graph->node_count, sizeof(cns_node_t));
    if (!graph->nodes) return CNS_ERROR_MEMORY;
    
    // Allocate edges
    graph->edges = calloc(graph->edge_count, sizeof(cns_edge_t));
    if (!graph->edges) {
        free(graph->nodes);
        return CNS_ERROR_MEMORY;
    }
    
    // Read nodes
    buffer->position = metadata.node_data_offset;
    for (size_t i = 0; i < graph->node_count; i++) {
        ret = read_node(buffer, &graph->nodes[i]);
        if (ret != CNS_SUCCESS) {
            cns_graph_destroy(graph);
            return ret;
        }
    }
    
    // Read edges
    buffer->position = metadata.edge_data_offset;
    for (size_t i = 0; i < graph->edge_count; i++) {
        ret = read_edge(buffer, &graph->edges[i], header.flags);
        if (ret != CNS_SUCCESS) {
            cns_graph_destroy(graph);
            return ret;
        }
    }
    
    return CNS_SUCCESS;
}

// Zero-copy view implementation
int cns_graph_view_open(cns_graph_view_t* view, const char* path) {
    if (!view || !path) {
        return CNS_ERROR_INVALID_ARGUMENT;
    }
    
    // Open file
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        return CNS_ERROR_IO;
    }
    
    // Get file size
    struct stat st;
    if (fstat(fd, &st) < 0) {
        close(fd);
        return CNS_ERROR_IO;
    }
    
    // Memory map the file
    void* data = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    
    if (data == MAP_FAILED) {
        return CNS_ERROR_IO;
    }
    
    view->data = data;
    view->size = st.st_size;
    
    // Validate header
    const cns_binary_header_t* header = (const cns_binary_header_t*)data;
    int ret = validate_header(header, st.st_size);
    if (ret != CNS_SUCCESS) {
        munmap(data, st.st_size);
        return ret;
    }
    
    // Set up pointers
    view->header = header;
    view->metadata = (const cns_binary_metadata_t*)((const uint8_t*)data + header->metadata_offset);
    
    // Set up node index if present
    if (view->metadata->node_index_offset > 0) {
        view->node_index = (const uint64_t*)((const uint8_t*)data + view->metadata->node_index_offset);
    } else {
        view->node_index = NULL;
    }
    
    view->node_data = (const uint8_t*)data + view->metadata->node_data_offset;
    view->edge_data = (const uint8_t*)data + view->metadata->edge_data_offset;
    
    return CNS_SUCCESS;
}

// Close zero-copy view
void cns_graph_view_close(cns_graph_view_t* view) {
    if (view && view->data) {
        munmap((void*)view->data, view->size);
        memset(view, 0, sizeof(*view));
    }
}

// Access node in zero-copy view (7-tick optimized)
int cns_graph_view_get_node(const cns_graph_view_t* view, uint64_t node_id, 
                            cns_node_view_t* node_view) {
    if (!view || !node_view || node_id >= view->header->node_count) {
        return CNS_ERROR_INVALID_ARGUMENT;
    }
    
    // Use index for O(1) lookup if available
    if (view->node_index) {
        node_view->data = view->node_data + view->node_index[node_id];
        node_view->node_id = node_id;
        return CNS_SUCCESS;
    }
    
    // Otherwise, sequential scan (not 7-tick compliant)
    return CNS_ERROR_NOT_FOUND;
}

// Deserialize from file
int cns_graph_deserialize_from_file(cns_graph_t* graph, const char* path, uint32_t flags) {
    FILE* file = fopen(path, "rb");
    if (!file) return CNS_ERROR_IO;
    
    // Get file size
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // Read entire file
    void* data = malloc(size);
    if (!data) {
        fclose(file);
        return CNS_ERROR_MEMORY;
    }
    
    size_t read = fread(data, 1, size, file);
    fclose(file);
    
    if (read != size) {
        free(data);
        return CNS_ERROR_IO;
    }
    
    // Create read buffer and deserialize
    cns_read_buffer_t* buffer = cns_read_buffer_create(data, size);
    if (!buffer) {
        free(data);
        return CNS_ERROR_MEMORY;
    }
    
    int ret = cns_graph_deserialize(graph, buffer, flags);
    
    cns_read_buffer_destroy(buffer);
    free(data);
    
    return ret;
}