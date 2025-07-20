#ifndef CNS_BINARY_MATERIALIZER_H
#define CNS_BINARY_MATERIALIZER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// CNS Binary Materializer - High-Performance Graph Serialization
// ============================================================================

// Magic number for CNS Binary format
#define CNS_BINARY_MAGIC 0x434E5342  // 'CNSB'

// Version information
#define CNS_BINARY_VERSION_MAJOR 1
#define CNS_BINARY_VERSION_MINOR 0

// Flags for serialization control
#define CNS_SERIALIZE_FLAG_COMPRESS    0x0001
#define CNS_SERIALIZE_FLAG_CHECKSUM    0x0002
#define CNS_SERIALIZE_FLAG_INCREMENTAL 0x0004
#define CNS_SERIALIZE_FLAG_STREAMING   0x0008

// Error codes
typedef enum {
    CNS_SERIALIZE_OK = 0,
    CNS_SERIALIZE_ERROR_MEMORY = -1,
    CNS_SERIALIZE_ERROR_IO = -2,
    CNS_SERIALIZE_ERROR_FORMAT = -3,
    CNS_SERIALIZE_ERROR_VERSION = -4,
    CNS_SERIALIZE_ERROR_CHECKSUM = -5,
    CNS_SERIALIZE_ERROR_BOUNDS = -6,
    CNS_SERIALIZE_ERROR_COMPRESS = -7,
} cns_serialize_error_t;

// Compression types
typedef enum {
    CNS_COMPRESS_NONE = 0,
    CNS_COMPRESS_LZ4 = 1,
    CNS_COMPRESS_ZSTD = 2,
} cns_compress_type_t;

// ============================================================================
// Core Data Structures
// ============================================================================

// Graph element base (common to nodes and edges)
typedef struct {
    uint32_t id;          // Element identifier
    uint16_t type;        // Type identifier
    uint16_t flags;       // Bit flags
    uint32_t data_offset; // Offset to variable data
} cns_graph_element_t;

// Node representation (16 bytes aligned)
typedef struct {
    cns_graph_element_t base;
    uint32_t first_out_edge;  // First outgoing edge index
    uint32_t first_in_edge;   // First incoming edge index
} cns_node_t;

// Edge representation (24 bytes aligned)
typedef struct {
    cns_graph_element_t base;
    uint32_t source_id;       // Source node ID
    uint32_t target_id;       // Target node ID
    uint32_t next_out_edge;   // Next edge from same source
    uint32_t next_in_edge;    // Next edge to same target
} cns_edge_t;

// Main graph structure
typedef struct {
    // Core arrays
    cns_node_t *nodes;
    cns_edge_t *edges;
    uint8_t *data_pool;       // Variable-length data
    
    // Counts and capacities
    uint32_t node_count;
    uint32_t edge_count;
    uint32_t data_size;
    uint32_t node_capacity;
    uint32_t edge_capacity;
    uint32_t data_capacity;
    
    // Index structures
    uint32_t *node_index;     // ID to array index mapping
    uint32_t *type_index;     // Type-based indexing
    
    // Metadata
    uint16_t version;
    uint16_t flags;
    uint32_t checksum;
} cns_graph_t;

// Binary format header (64 bytes)
typedef struct {
    uint32_t magic;          // 0x434E5342 ('CNSB')
    uint16_t version_major;  // Format version
    uint16_t version_minor;
    uint32_t flags;          // Format flags
    uint32_t header_size;    // Size of this header
    uint32_t node_count;
    uint32_t edge_count;
    uint32_t data_size;
    uint32_t node_offset;    // Offset to node data
    uint32_t edge_offset;    // Offset to edge data
    uint32_t data_offset;    // Offset to variable data
    uint32_t index_offset;   // Offset to indices
    uint32_t checksum;       // CRC32 of entire file
    uint8_t reserved[12];    // Future expansion
} cns_binary_header_t;

// ============================================================================
// Buffer Management
// ============================================================================

// Write buffer with automatic growth
typedef struct {
    uint8_t *data;
    size_t size;
    size_t capacity;
    size_t position;
    uint32_t checksum;
} cns_write_buffer_t;

// Read buffer with bounds checking
typedef struct {
    const uint8_t *data;
    size_t size;
    size_t position;
    uint16_t version;
    bool is_big_endian;
} cns_read_buffer_t;

// Serialization context
typedef struct {
    cns_graph_t *graph;
    void *buffer;            // Either read or write buffer
    uint32_t *id_map;        // ID remapping table
    size_t id_map_size;
    uint64_t start_cycles;
    uint64_t end_cycles;
    char error_msg[256];
} cns_serialize_ctx_t;

// ============================================================================
// Memory Management
// ============================================================================

// Memory pool for reduced fragmentation
typedef struct {
    uint8_t *memory;
    size_t size;
    size_t used;
    size_t alignment;
} cns_memory_pool_t;

// Buffer cache for reuse
typedef struct {
    cns_write_buffer_t **buffers;
    size_t count;
    size_t capacity;
    pthread_mutex_t lock;
} cns_buffer_cache_t;

// ============================================================================
// Zero-Copy Support
// ============================================================================

// Memory-mapped region
typedef struct {
    void *addr;
    size_t size;
    int fd;
} cns_mmap_region_t;

// Zero-copy graph view
typedef struct {
    cns_mmap_region_t region;
    const cns_binary_header_t *header;
    const cns_node_t *nodes;
    const cns_edge_t *edges;
    const uint8_t *data;
} cns_graph_view_t;

// Node view for zero-copy access
typedef struct {
    const uint8_t *data;
    uint64_t node_id;
} cns_node_view_t;

// Graph statistics
typedef struct {
    size_t node_count;
    size_t edge_count;
    size_t memory_usage;
    double avg_degree;
} cns_graph_stats_t;

// ============================================================================
// API Functions
// ============================================================================

// Graph lifecycle
cns_graph_t* cns_graph_create(uint32_t initial_nodes, uint32_t initial_edges);
void cns_graph_destroy(cns_graph_t *graph);
void cns_graph_clear(cns_graph_t *graph);

// Main serialization entry points
int cns_graph_serialize(const cns_graph_t *graph, 
                       cns_write_buffer_t *buffer,
                       uint32_t flags);

int cns_graph_deserialize(cns_graph_t *graph,
                         cns_read_buffer_t *buffer,
                         uint32_t flags);

// Memory-mapped variants for zero-copy
int cns_graph_serialize_mmap(const cns_graph_t *graph,
                            int fd,
                            uint32_t flags);

int cns_graph_deserialize_mmap(cns_graph_t *graph,
                              int fd,
                              uint32_t flags);

// Zero-copy graph views
int cns_graph_view_open(cns_graph_view_t *view, const char *path);
void cns_graph_view_close(cns_graph_view_t *view);

// Element serialization
int cns_node_write(cns_write_buffer_t *buffer,
                  const cns_node_t *node,
                  const uint8_t *data_pool);

int cns_node_read(cns_read_buffer_t *buffer,
                 cns_node_t *node,
                 cns_serialize_ctx_t *ctx);

int cns_edge_write(cns_write_buffer_t *buffer,
                  const cns_edge_t *edge,
                  const uint8_t *data_pool);

int cns_edge_read(cns_read_buffer_t *buffer,
                 cns_edge_t *edge,
                 cns_serialize_ctx_t *ctx);

// Batch operations
int cns_nodes_write_batch(cns_write_buffer_t *buffer,
                         const cns_node_t *nodes,
                         uint32_t count,
                         const uint8_t *data_pool);

int cns_edges_write_batch(cns_write_buffer_t *buffer,
                         const cns_edge_t *edges,
                         uint32_t count,
                         const uint8_t *data_pool);

// Buffer management
cns_write_buffer_t* cns_write_buffer_create(size_t initial_size);
void cns_write_buffer_destroy(cns_write_buffer_t *buffer);
int cns_write_buffer_ensure_capacity(cns_write_buffer_t *buffer, size_t required);
int cns_write_buffer_write_u32(cns_write_buffer_t *buffer, uint32_t value);
int cns_write_buffer_write_u16(cns_write_buffer_t *buffer, uint16_t value);
int cns_write_buffer_write_bytes(cns_write_buffer_t *buffer, const void *data, size_t size);

cns_read_buffer_t* cns_read_buffer_create(const void *data, size_t size);
void cns_read_buffer_destroy(cns_read_buffer_t *buffer);
int cns_read_buffer_read_u32(cns_read_buffer_t *buffer, uint32_t *value);
int cns_read_buffer_read_u16(cns_read_buffer_t *buffer, uint16_t *value);
int cns_read_buffer_read_bytes(cns_read_buffer_t *buffer, void *data, size_t size);

// Memory pool operations
cns_memory_pool_t* cns_pool_create(size_t size, size_t alignment);
void* cns_pool_alloc(cns_memory_pool_t *pool, size_t size);
void cns_pool_reset(cns_memory_pool_t *pool);
void cns_pool_destroy(cns_memory_pool_t *pool);

// Buffer cache operations
cns_buffer_cache_t* cns_buffer_cache_create(size_t max_buffers);
cns_write_buffer_t* cns_buffer_cache_acquire(cns_buffer_cache_t *cache);
void cns_buffer_cache_release(cns_buffer_cache_t *cache, cns_write_buffer_t *buffer);
void cns_buffer_cache_destroy(cns_buffer_cache_t *cache);

// Compression support
int cns_compress_data(const uint8_t *src, size_t src_size,
                     uint8_t *dst, size_t *dst_size,
                     cns_compress_type_t type);

int cns_decompress_data(const uint8_t *src, size_t src_size,
                       uint8_t *dst, size_t *dst_size,
                       cns_compress_type_t type);

// Error handling
const char* cns_serialize_error_string(int error_code);
int cns_serialize_get_last_error(cns_serialize_ctx_t *ctx);

// ============================================================================
// Platform Portability Macros
// ============================================================================

// Endianness detection
static inline bool cns_is_big_endian(void) {
    union { uint32_t i; char c[4]; } test = {0x01020304};
    return test.c[0] == 1;
}

// Byte swapping
static inline uint32_t cns_swap32(uint32_t value) {
    return ((value & 0xFF000000) >> 24) |
           ((value & 0x00FF0000) >> 8)  |
           ((value & 0x0000FF00) << 8)  |
           ((value & 0x000000FF) << 24);
}

static inline uint16_t cns_swap16(uint16_t value) {
    return ((value & 0xFF00) >> 8) | ((value & 0x00FF) << 8);
}

// Conditional byte swapping
#define CNS_TO_LE32(x) (cns_is_big_endian() ? cns_swap32(x) : (x))
#define CNS_TO_LE16(x) (cns_is_big_endian() ? cns_swap16(x) : (x))
#define CNS_FROM_LE32(x) CNS_TO_LE32(x)
#define CNS_FROM_LE16(x) CNS_TO_LE16(x)

// ============================================================================
// Performance Helpers
// ============================================================================

// SIMD operations (when available)
#ifdef __AVX2__
uint32_t cns_checksum_avx2(const uint8_t *data, size_t size);
void cns_memcpy_avx2(void *dst, const void *src, size_t size);
#endif

// Scalar fallbacks
uint32_t cns_checksum_scalar(const uint8_t *data, size_t size);
void cns_memcpy_scalar(void *dst, const void *src, size_t size);

// Select best implementation
static inline uint32_t cns_checksum(const uint8_t *data, size_t size) {
#ifdef __AVX2__
    return cns_checksum_avx2(data, size);
#else
    return cns_checksum_scalar(data, size);
#endif
}

static inline void cns_memcpy(void *dst, const void *src, size_t size) {
#ifdef __AVX2__
    cns_memcpy_avx2(dst, src, size);
#else
    cns_memcpy_scalar(dst, src, size);
#endif
}

// Additional functions from binary_materializer_types.h
// Buffer management
int cns_buffer_cache_init(void);
void cns_buffer_cache_cleanup(void);
int cns_write_buffer_append(cns_write_buffer_t* buf, const void* data, size_t size);
int cns_write_buffer_write_varint(cns_write_buffer_t* buf, uint64_t value);
int cns_read_buffer_read(cns_read_buffer_t* buf, void* data, size_t size);
int cns_read_buffer_read_varint(cns_read_buffer_t* buf, uint64_t* value);

// Graph management
int cns_graph_add_node(cns_graph_t* graph, uint64_t id, uint32_t type, const void* data, size_t data_size);
int cns_graph_add_edge(cns_graph_t* graph, uint64_t source, uint64_t target, uint32_t type, double weight, const void* data, size_t data_size);
cns_node_t* cns_graph_find_node(cns_graph_t* graph, uint64_t id);
int cns_graph_get_neighbors(cns_graph_t* graph, uint64_t node_id, uint64_t** neighbors, size_t* count);
cns_graph_t* cns_graph_clone(const cns_graph_t* graph);
void cns_graph_get_stats(const cns_graph_t* graph, cns_graph_stats_t* stats);

// Serialization/Deserialization
int cns_graph_serialize_to_file(const cns_graph_t* graph, const char* path, uint32_t flags);
int cns_graph_deserialize_from_file(cns_graph_t* graph, const char* path, uint32_t flags);
int cns_graph_serialize_batch(const cns_graph_t** graphs, size_t count, cns_write_buffer_t** buffers, uint32_t flags);

// Zero-copy view
int cns_graph_view_get_node(const cns_graph_view_t* view, uint64_t node_id, cns_node_view_t* node_view);

// Utilities
uint32_t cns_calculate_crc32(const void* data, size_t length);
const char* cns_error_string(int error_code);

#ifdef __cplusplus
}
#endif

#endif // CNS_BINARY_MATERIALIZER_H