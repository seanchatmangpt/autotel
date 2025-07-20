/*
 * CNS Materializer - Direct Binary .plan.bin Generator
 * Zero-copy memory-mappable binary format with single fwrite operation
 * Integrates with existing binary_materializer infrastructure for 7T substrate
 */

#include "cns/materializer.h"
#include "cns/binary_materializer.h"
#include "cns/binary_materializer_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>

// ============================================================================
// .PLAN.BIN SPECIFIC FORMAT CONSTANTS
// ============================================================================

#define CNS_PLAN_MAGIC    0x504C414E  // 'PLAN'
#define CNS_PLAN_VERSION  0x0100
#define CNS_PLAN_HEADER_SIZE 64       // Fixed header size for alignment

// Packed plan binary header (exactly 64 bytes)
typedef struct __attribute__((aligned(64))) {
    uint32_t magic;              // 'PLAN' magic number
    uint16_t version;            // Format version
    uint16_t flags;              // Format flags
    uint32_t triple_count;       // Total triples in plan
    uint32_t node_count;         // Total unique nodes
    uint64_t triples_offset;     // Offset to triples array
    uint64_t nodes_offset;       // Offset to nodes array
    uint64_t strings_offset;     // Offset to string pool
    uint64_t index_offset;       // Offset to ID->index mapping
    uint32_t checksum;           // CRC32 of data section
    uint8_t reserved[12];        // Future expansion
} cns_plan_header_t;

// Packed triple structure for .plan.bin (24 bytes)
typedef struct __attribute__((packed)) {
    uint32_t subject_id;    // Subject node ID
    uint32_t predicate_id;  // Predicate node ID
    uint32_t object_id;     // Object node ID
    uint32_t graph_id;      // Named graph ID (0 for default)
    uint32_t flags;         // Triple flags
    uint32_t data_offset;   // Offset to additional data
} cns_plan_triple_t;

// Packed node structure for .plan.bin (16 bytes)
typedef struct __attribute__((packed)) {
    uint32_t id;           // Node ID
    uint16_t type;         // Node type (IRI, literal, blank)
    uint16_t flags;        // Node flags
    uint32_t string_offset;// Offset to string representation
    uint32_t string_length;// Length of string
} cns_plan_node_t;

// Memory layout structure for zero-copy operations
typedef struct {
    cns_plan_header_t header;
    cns_plan_triple_t *triples;
    cns_plan_node_t *nodes;
    uint8_t *string_pool;
    uint32_t *id_index;
    size_t total_size;
    uint32_t string_pool_size;
} cns_plan_layout_t;

// ============================================================================
// PRIVATE HELPER FUNCTIONS
// ============================================================================

// Calculate total size needed for plan binary
static size_t calculate_plan_size(const cns_graph_t *graph) {
    if (!graph) return 0;
    
    size_t header_size = sizeof(cns_plan_header_t);
    size_t triples_size = graph->edge_count * sizeof(cns_plan_triple_t);
    size_t nodes_size = graph->node_count * sizeof(cns_plan_node_t);
    size_t index_size = graph->node_count * sizeof(uint32_t) * 2; // ID->index sparse map
    
    // Estimate string pool size (conservative)
    size_t string_pool_size = graph->node_count * 64; // Average 64 chars per node
    
    return header_size + triples_size + nodes_size + index_size + string_pool_size;
}

// Build efficient ID to index mapping
static int build_id_index(const cns_graph_t *graph, uint32_t *index, size_t index_size) {
    if (!graph || !index) return CNS_ERROR_INVALID_ARGUMENT;
    
    // Clear index
    memset(index, 0xFF, index_size); // Use 0xFFFFFFFF as "not found"
    
    // Simple linear mapping for now (could be optimized with hash table)
    for (size_t i = 0; i < graph->node_count; i++) {
        uint32_t id = graph->nodes[i].base.id;
        if (id < index_size / sizeof(uint32_t) / 2) {
            index[id] = (uint32_t)i;
        }
    }
    
    return CNS_SUCCESS;
}

// Serialize nodes to plan format
static int serialize_nodes(const cns_graph_t *graph, cns_plan_node_t *plan_nodes,
                          uint8_t *string_pool, size_t string_pool_size,
                          uint32_t *string_offset) {
    if (!graph || !plan_nodes || !string_pool) return CNS_ERROR_INVALID_ARGUMENT;
    
    uint32_t current_offset = 0;
    
    for (size_t i = 0; i < graph->node_count; i++) {
        const cns_node_t *node = &graph->nodes[i];
        cns_plan_node_t *plan_node = &plan_nodes[i];
        
        plan_node->id = node->base.id;
        plan_node->type = node->base.type;
        plan_node->flags = node->base.flags;
        plan_node->string_offset = current_offset;
        
        // Copy node string data to pool
        if (node->base.data_offset > 0 && graph->data_pool) {
            const char *node_string = (const char*)(graph->data_pool + node->base.data_offset);
            size_t string_len = strnlen(node_string, 256); // Reasonable limit
            
            if (current_offset + string_len + 1 <= string_pool_size) {
                memcpy(string_pool + current_offset, node_string, string_len);
                string_pool[current_offset + string_len] = '\0';
                plan_node->string_length = string_len + 1;
                current_offset += string_len + 1;
            } else {
                plan_node->string_length = 0;
            }
        } else {
            plan_node->string_length = 0;
        }
    }
    
    *string_offset = current_offset;
    return CNS_SUCCESS;
}

// Serialize edges as triples to plan format
static int serialize_triples(const cns_graph_t *graph, cns_plan_triple_t *plan_triples) {
    if (!graph || !plan_triples) return CNS_ERROR_INVALID_ARGUMENT;
    
    for (size_t i = 0; i < graph->edge_count; i++) {
        const cns_edge_t *edge = &graph->edges[i];
        cns_plan_triple_t *triple = &plan_triples[i];
        
        triple->subject_id = edge->source_id;
        triple->predicate_id = edge->base.type; // Use edge type as predicate
        triple->object_id = edge->target_id;
        triple->graph_id = 0; // Default graph
        triple->flags = edge->base.flags;
        triple->data_offset = edge->base.data_offset;
    }
    
    return CNS_SUCCESS;
}

// ============================================================================
// MAIN MATERIALIZER IMPLEMENTATION
// ============================================================================

// Generate .plan.bin file with single fwrite operation
int cns_materialize_plan_bin(const cns_graph_t *graph, const char *filename) {
    if (!graph || !filename) {
        return CNS_ERROR_INVALID_ARGUMENT;
    }
    
    // Calculate total size needed
    size_t total_size = calculate_plan_size(graph);
    if (total_size == 0) {
        return CNS_ERROR_INVALID_ARGUMENT;
    }
    
    // Allocate single contiguous buffer for entire file
    uint8_t *buffer = calloc(1, total_size);
    if (!buffer) {
        return CNS_ERROR_MEMORY;
    }
    
    // Setup memory layout pointers
    cns_plan_layout_t layout = {0};
    uint8_t *current = buffer;
    
    // Header at start
    memcpy(&layout.header, current, sizeof(cns_plan_header_t));
    current += sizeof(cns_plan_header_t);
    
    // Triples array
    layout.triples = (cns_plan_triple_t*)current;
    current += graph->edge_count * sizeof(cns_plan_triple_t);
    
    // Nodes array
    layout.nodes = (cns_plan_node_t*)current;
    current += graph->node_count * sizeof(cns_plan_node_t);
    
    // ID index
    layout.id_index = (uint32_t*)current;
    size_t index_size = graph->node_count * sizeof(uint32_t) * 2;
    current += index_size;
    
    // String pool at end
    layout.string_pool = current;
    layout.string_pool_size = total_size - (current - buffer);
    
    // Fill header
    layout.header.magic = CNS_PLAN_MAGIC;
    layout.header.version = CNS_PLAN_VERSION;
    layout.header.flags = 0;
    layout.header.triple_count = graph->edge_count;
    layout.header.node_count = graph->node_count;
    layout.header.triples_offset = sizeof(cns_plan_header_t);
    layout.header.nodes_offset = layout.header.triples_offset + 
                                graph->edge_count * sizeof(cns_plan_triple_t);
    layout.header.index_offset = layout.header.nodes_offset + 
                                graph->node_count * sizeof(cns_plan_node_t);
    layout.header.strings_offset = layout.header.index_offset + index_size;
    
    // Serialize data
    int result = CNS_SUCCESS;
    uint32_t string_used = 0;
    
    // Build ID index
    result = build_id_index(graph, layout.id_index, index_size);
    if (result != CNS_SUCCESS) goto cleanup;
    
    // Serialize nodes
    result = serialize_nodes(graph, layout.nodes, layout.string_pool, 
                           layout.string_pool_size, &string_used);
    if (result != CNS_SUCCESS) goto cleanup;
    
    // Serialize triples
    result = serialize_triples(graph, layout.triples);
    if (result != CNS_SUCCESS) goto cleanup;
    
    // Calculate actual size used
    layout.total_size = layout.header.strings_offset + string_used;
    
    // Calculate checksum
    layout.header.checksum = cns_calculate_crc32(
        buffer + sizeof(cns_plan_header_t),
        layout.total_size - sizeof(cns_plan_header_t)
    );
    
    // Copy header back to buffer
    memcpy(buffer, &layout.header, sizeof(cns_plan_header_t));
    
    // Single fwrite operation for maximum performance
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        result = CNS_ERROR_IO;
        goto cleanup;
    }
    
    size_t written = fwrite(buffer, 1, layout.total_size, fp);
    fclose(fp);
    
    if (written != layout.total_size) {
        result = CNS_ERROR_IO;
        goto cleanup;
    }
    
cleanup:
    free(buffer);
    return result;
}

// Memory-map existing .plan.bin file for zero-copy access
int cns_plan_view_open(cns_graph_view_t *view, const char *filename) {
    if (!view || !filename) {
        return CNS_ERROR_INVALID_ARGUMENT;
    }
    
    // Open file
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        return CNS_ERROR_IO;
    }
    
    // Get file size
    struct stat st;
    if (fstat(fd, &st) < 0) {
        close(fd);
        return CNS_ERROR_IO;
    }
    
    // Memory map file
    void *addr = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (addr == MAP_FAILED) {
        close(fd);
        return CNS_ERROR_IO;
    }
    
    // Validate header
    const cns_plan_header_t *header = (const cns_plan_header_t*)addr;
    if (header->magic != CNS_PLAN_MAGIC) {
        munmap(addr, st.st_size);
        close(fd);
        return CNS_ERROR_INVALID_FORMAT;
    }
    
    // Setup view
    view->region.addr = addr;
    view->region.size = st.st_size;
    view->region.fd = fd;
    view->header = (const cns_binary_header_t*)header; // Compatible cast
    
    // Direct pointers for zero-copy access
    view->nodes = (const cns_node_t*)((uint8_t*)addr + header->nodes_offset);
    view->edges = (const cns_edge_t*)((uint8_t*)addr + header->triples_offset);
    view->data = (const uint8_t*)addr + header->strings_offset;
    
    return CNS_SUCCESS;
}

// Close memory-mapped view
void cns_plan_view_close(cns_graph_view_t *view) {
    if (view && view->region.addr) {
        munmap(view->region.addr, view->region.size);
        close(view->region.fd);
        memset(view, 0, sizeof(*view));
    }
}

// Direct memory-to-file serialization with graph->triples array
int cns_materialize_graph_direct(const cns_graph_t *graph, const char *filename) {
    if (!graph || !filename) {
        return CNS_ERROR_INVALID_ARGUMENT;
    }
    
    // Use existing binary materializer for core serialization
    cns_write_buffer_t *buffer = cns_write_buffer_create(calculate_plan_size(graph));
    if (!buffer) {
        return CNS_ERROR_MEMORY;
    }
    
    // Serialize using existing infrastructure
    int result = cns_graph_serialize(graph, buffer, 
                                    CNS_SERIALIZE_FLAG_CHECKSUM | CNS_FLAG_BUILD_INDEX);
    if (result != CNS_SUCCESS) {
        cns_write_buffer_destroy(buffer);
        return result;
    }
    
    // Single fwrite operation
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        cns_write_buffer_destroy(buffer);
        return CNS_ERROR_IO;
    }
    
    size_t written = fwrite(buffer->data, 1, buffer->size, fp);
    fclose(fp);
    cns_write_buffer_destroy(buffer);
    
    if (written != buffer->size) {
        return CNS_ERROR_IO;
    }
    
    return CNS_SUCCESS;
}

// Enhanced graph to file serialization with memory-mapped loading support
int cns_materialize_with_mmap_support(const cns_graph_t *graph, const char *filename) {
    // First generate the plan.bin format
    int result = cns_materialize_plan_bin(graph, filename);
    if (result != CNS_SUCCESS) {
        return result;
    }
    
    // Verify the file can be memory-mapped correctly
    cns_graph_view_t view = {0};
    result = cns_plan_view_open(&view, filename);
    if (result == CNS_SUCCESS) {
        cns_plan_view_close(&view);
    }
    
    return result;
}

// Integration with existing binary materializer interface
int cns_materialize_binary_plan(cns_materializer_t *materializer, 
                               const cns_graph_t *graph,
                               const char *filename) {
    if (!materializer || !graph || !filename) {
        return CNS_ERROR_INVALID_ARGUMENT;
    }
    
    // Use the direct materialization for .plan.bin files
    if (strstr(filename, ".plan.bin")) {
        return cns_materialize_plan_bin(graph, filename);
    }
    
    // Fall back to standard serialization
    return cns_graph_serialize_to_file(graph, filename, 
                                      CNS_SERIALIZE_FLAG_CHECKSUM | CNS_FLAG_BUILD_INDEX);
}