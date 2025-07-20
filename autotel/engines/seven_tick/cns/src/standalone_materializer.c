/*
 * CNS Standalone Binary Materializer for .plan.bin Generation
 * Zero-copy memory-mappable binary format with single fwrite operation
 * Standalone implementation to avoid type conflicts
 */

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
// STANDALONE TYPE DEFINITIONS (NO CONFLICTS)
// ============================================================================

// Standalone result codes
typedef enum {
    PLAN_SUCCESS = 0,
    PLAN_ERROR_MEMORY = -1,
    PLAN_ERROR_IO = -2,
    PLAN_ERROR_INVALID_ARG = -3,
    PLAN_ERROR_FORMAT = -4
} plan_result_t;

// .PLAN.BIN SPECIFIC FORMAT CONSTANTS
#define PLAN_MAGIC    0x504C414E  // 'PLAN'
#define PLAN_VERSION  0x0100
#define PLAN_HEADER_SIZE 64       // Fixed header size for alignment

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
} plan_header_t;

// Packed triple structure for .plan.bin (24 bytes)
typedef struct __attribute__((packed)) {
    uint32_t subject_id;    // Subject node ID
    uint32_t predicate_id;  // Predicate node ID
    uint32_t object_id;     // Object node ID
    uint32_t graph_id;      // Named graph ID (0 for default)
    uint32_t flags;         // Triple flags
    uint32_t data_offset;   // Offset to additional data
} plan_triple_t;

// Packed node structure for .plan.bin (16 bytes)
typedef struct __attribute__((packed)) {
    uint32_t id;           // Node ID
    uint16_t type;         // Node type (IRI, literal, blank)
    uint16_t flags;        // Node flags
    uint32_t string_offset;// Offset to string representation
    uint32_t string_length;// Length of string
} plan_node_t;

// Simple graph structure for standalone operation
typedef struct {
    plan_node_t *nodes;
    plan_triple_t *triples;
    char **node_strings;     // Array of string pointers
    uint32_t node_count;
    uint32_t triple_count;
    uint32_t capacity_nodes;
    uint32_t capacity_triples;
} plan_graph_t;

// Memory-mapped view for zero-copy access
typedef struct {
    void *base_addr;
    size_t total_size;
    int fd;
    plan_header_t *header;
    plan_node_t *nodes;
    plan_triple_t *triples;
    char *string_pool;
    uint32_t *id_index;
} plan_view_t;

// ============================================================================
// CRC32 CALCULATION
// ============================================================================

static const uint32_t crc32_table[256] = {
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
    0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
    0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
    0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
    0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
    0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
    0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
    0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
    0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
    0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
    0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
    0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
    0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
    0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
    0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
    0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
    0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
    0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
    0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
    0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
    0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
    0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
    0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
    0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
    0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
    0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
    0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
    0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
    0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
    0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
    0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
    0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
    0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

static uint32_t calculate_crc32(const void *data, size_t length) {
    const uint8_t *bytes = (const uint8_t*)data;
    uint32_t crc = 0xFFFFFFFF;
    
    for (size_t i = 0; i < length; i++) {
        crc = crc32_table[(crc ^ bytes[i]) & 0xFF] ^ (crc >> 8);
    }
    
    return crc ^ 0xFFFFFFFF;
}

// ============================================================================
// GRAPH MANAGEMENT FUNCTIONS
// ============================================================================

// Create a new graph
plan_graph_t* plan_graph_create(uint32_t initial_nodes, uint32_t initial_triples) {
    plan_graph_t *graph = calloc(1, sizeof(plan_graph_t));
    if (!graph) return NULL;
    
    graph->capacity_nodes = initial_nodes ? initial_nodes : 64;
    graph->capacity_triples = initial_triples ? initial_triples : 128;
    
    graph->nodes = calloc(graph->capacity_nodes, sizeof(plan_node_t));
    graph->triples = calloc(graph->capacity_triples, sizeof(plan_triple_t));
    graph->node_strings = calloc(graph->capacity_nodes, sizeof(char*));
    
    if (!graph->nodes || !graph->triples || !graph->node_strings) {
        free(graph->nodes);
        free(graph->triples);
        free(graph->node_strings);
        free(graph);
        return NULL;
    }
    
    return graph;
}

// Destroy graph
void plan_graph_destroy(plan_graph_t *graph) {
    if (!graph) return;
    
    // Free node strings
    for (uint32_t i = 0; i < graph->node_count; i++) {
        free(graph->node_strings[i]);
    }
    
    free(graph->nodes);
    free(graph->triples);
    free(graph->node_strings);
    free(graph);
}

// Add a node to the graph
plan_result_t plan_graph_add_node(plan_graph_t *graph, uint32_t id, uint16_t type, 
                                 const char *string_value) {
    if (!graph || graph->node_count >= graph->capacity_nodes) {
        return PLAN_ERROR_INVALID_ARG;
    }
    
    plan_node_t *node = &graph->nodes[graph->node_count];
    node->id = id;
    node->type = type;
    node->flags = 0;
    
    // Copy string
    if (string_value) {
        graph->node_strings[graph->node_count] = strdup(string_value);
        if (!graph->node_strings[graph->node_count]) {
            return PLAN_ERROR_MEMORY;
        }
        node->string_length = strlen(string_value) + 1;
    } else {
        graph->node_strings[graph->node_count] = NULL;
        node->string_length = 0;
    }
    
    graph->node_count++;
    return PLAN_SUCCESS;
}

// Add a triple to the graph
plan_result_t plan_graph_add_triple(plan_graph_t *graph, uint32_t subject_id,
                                   uint32_t predicate_id, uint32_t object_id) {
    if (!graph || graph->triple_count >= graph->capacity_triples) {
        return PLAN_ERROR_INVALID_ARG;
    }
    
    plan_triple_t *triple = &graph->triples[graph->triple_count];
    triple->subject_id = subject_id;
    triple->predicate_id = predicate_id;
    triple->object_id = object_id;
    triple->graph_id = 0;
    triple->flags = 0;
    triple->data_offset = 0;
    
    graph->triple_count++;
    return PLAN_SUCCESS;
}

// ============================================================================
// BINARY SERIALIZATION FUNCTIONS
// ============================================================================

// Calculate total size needed for plan binary
static size_t calculate_plan_size(const plan_graph_t *graph) {
    if (!graph) return 0;
    
    size_t header_size = sizeof(plan_header_t);
    size_t triples_size = graph->triple_count * sizeof(plan_triple_t);
    size_t nodes_size = graph->node_count * sizeof(plan_node_t);
    size_t index_size = graph->node_count * sizeof(uint32_t) * 2; // ID->index sparse map
    
    // Calculate string pool size
    size_t string_pool_size = 0;
    for (uint32_t i = 0; i < graph->node_count; i++) {
        if (graph->node_strings[i]) {
            string_pool_size += strlen(graph->node_strings[i]) + 1;
        }
    }
    
    return header_size + triples_size + nodes_size + index_size + string_pool_size;
}

// Generate .plan.bin file with single fwrite operation
plan_result_t plan_materialize_to_file(const plan_graph_t *graph, const char *filename) {
    if (!graph || !filename) {
        return PLAN_ERROR_INVALID_ARG;
    }
    
    // Calculate total size needed
    size_t total_size = calculate_plan_size(graph);
    if (total_size == 0) {
        return PLAN_ERROR_INVALID_ARG;
    }
    
    // Allocate single contiguous buffer for entire file
    uint8_t *buffer = calloc(1, total_size);
    if (!buffer) {
        return PLAN_ERROR_MEMORY;
    }
    
    // Setup memory layout pointers
    uint8_t *current = buffer;
    
    // Header at start
    plan_header_t *header = (plan_header_t*)current;
    current += sizeof(plan_header_t);
    
    // Triples array
    plan_triple_t *triples = (plan_triple_t*)current;
    current += graph->triple_count * sizeof(plan_triple_t);
    
    // Nodes array
    plan_node_t *nodes = (plan_node_t*)current;
    current += graph->node_count * sizeof(plan_node_t);
    
    // ID index
    uint32_t *id_index = (uint32_t*)current;
    size_t index_size = graph->node_count * sizeof(uint32_t) * 2;
    current += index_size;
    
    // String pool at end
    char *string_pool = (char*)current;
    size_t string_pool_capacity = total_size - (current - buffer);
    
    // Fill header
    header->magic = PLAN_MAGIC;
    header->version = PLAN_VERSION;
    header->flags = 0;
    header->triple_count = graph->triple_count;
    header->node_count = graph->node_count;
    header->triples_offset = sizeof(plan_header_t);
    header->nodes_offset = header->triples_offset + 
                          graph->triple_count * sizeof(plan_triple_t);
    header->index_offset = header->nodes_offset + 
                          graph->node_count * sizeof(plan_node_t);
    header->strings_offset = header->index_offset + index_size;
    
    // Copy triples
    memcpy(triples, graph->triples, graph->triple_count * sizeof(plan_triple_t));
    
    // Copy nodes and build string pool
    uint32_t string_offset = 0;
    for (uint32_t i = 0; i < graph->node_count; i++) {
        nodes[i] = graph->nodes[i];
        nodes[i].string_offset = string_offset;
        
        if (graph->node_strings[i]) {
            size_t len = strlen(graph->node_strings[i]) + 1;
            if (string_offset + len <= string_pool_capacity) {
                memcpy(string_pool + string_offset, graph->node_strings[i], len);
                string_offset += len;
                nodes[i].string_length = len;
            } else {
                nodes[i].string_length = 0;
            }
        } else {
            nodes[i].string_length = 0;
        }
    }
    
    // Build ID index (simple linear for now)
    memset(id_index, 0xFF, index_size);
    for (uint32_t i = 0; i < graph->node_count; i++) {
        uint32_t id = graph->nodes[i].id;
        if (id < graph->node_count * 2) {
            id_index[id] = i;
        }
    }
    
    // Calculate actual size used
    size_t actual_size = header->strings_offset + string_offset;
    
    // Calculate checksum
    header->checksum = calculate_crc32(
        buffer + sizeof(plan_header_t),
        actual_size - sizeof(plan_header_t)
    );
    
    // Single fwrite operation for maximum performance
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        free(buffer);
        return PLAN_ERROR_IO;
    }
    
    size_t written = fwrite(buffer, 1, actual_size, fp);
    fclose(fp);
    free(buffer);
    
    if (written != actual_size) {
        return PLAN_ERROR_IO;
    }
    
    return PLAN_SUCCESS;
}

// ============================================================================
// ZERO-COPY MEMORY-MAPPED ACCESS
// ============================================================================

// Memory-map existing .plan.bin file for zero-copy access
plan_result_t plan_view_open(plan_view_t *view, const char *filename) {
    if (!view || !filename) {
        return PLAN_ERROR_INVALID_ARG;
    }
    
    memset(view, 0, sizeof(*view));
    
    // Open file
    view->fd = open(filename, O_RDONLY);
    if (view->fd < 0) {
        return PLAN_ERROR_IO;
    }
    
    // Get file size
    struct stat st;
    if (fstat(view->fd, &st) < 0) {
        close(view->fd);
        return PLAN_ERROR_IO;
    }
    
    view->total_size = st.st_size;
    
    // Memory map file
    view->base_addr = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, view->fd, 0);
    if (view->base_addr == MAP_FAILED) {
        close(view->fd);
        return PLAN_ERROR_IO;
    }
    
    // Validate header
    view->header = (plan_header_t*)view->base_addr;
    if (view->header->magic != PLAN_MAGIC) {
        munmap(view->base_addr, st.st_size);
        close(view->fd);
        return PLAN_ERROR_FORMAT;
    }
    
    // Setup direct pointers for zero-copy access
    uint8_t *base = (uint8_t*)view->base_addr;
    view->triples = (plan_triple_t*)(base + view->header->triples_offset);
    view->nodes = (plan_node_t*)(base + view->header->nodes_offset);
    view->id_index = (uint32_t*)(base + view->header->index_offset);
    view->string_pool = (char*)(base + view->header->strings_offset);
    
    return PLAN_SUCCESS;
}

// Close memory-mapped view
void plan_view_close(plan_view_t *view) {
    if (view && view->base_addr) {
        munmap(view->base_addr, view->total_size);
        close(view->fd);
        memset(view, 0, sizeof(*view));
    }
}

// Access functions for zero-copy view
const char* plan_view_get_node_string(const plan_view_t *view, uint32_t node_index) {
    if (!view || node_index >= view->header->node_count) {
        return NULL;
    }
    
    const plan_node_t *node = &view->nodes[node_index];
    if (node->string_length == 0) {
        return NULL;
    }
    
    return view->string_pool + node->string_offset;
}

// Get triple by index
const plan_triple_t* plan_view_get_triple(const plan_view_t *view, uint32_t triple_index) {
    if (!view || triple_index >= view->header->triple_count) {
        return NULL;
    }
    
    return &view->triples[triple_index];
}

// Get node by index
const plan_node_t* plan_view_get_node(const plan_view_t *view, uint32_t node_index) {
    if (!view || node_index >= view->header->node_count) {
        return NULL;
    }
    
    return &view->nodes[node_index];
}

// Find node index by ID
uint32_t plan_view_find_node_by_id(const plan_view_t *view, uint32_t node_id) {
    if (!view || node_id >= view->header->node_count * 2) {
        return 0xFFFFFFFF; // Not found
    }
    
    return view->id_index[node_id];
}

// ============================================================================
// ERROR HANDLING
// ============================================================================

const char* plan_error_string(plan_result_t error) {
    switch (error) {
        case PLAN_SUCCESS: return "Success";
        case PLAN_ERROR_MEMORY: return "Memory allocation failed";
        case PLAN_ERROR_IO: return "I/O error";
        case PLAN_ERROR_INVALID_ARG: return "Invalid argument";
        case PLAN_ERROR_FORMAT: return "Invalid format";
        default: return "Unknown error";
    }
}