/*
 * CNS Binary Materializer Types and Constants
 * Complete type definitions for graph serialization
 */

#ifndef CNS_BINARY_MATERIALIZER_TYPES_H
#define CNS_BINARY_MATERIALIZER_TYPES_H

#include <stdint.h>
#include <stddef.h>

// Buffer cache size
#define CNS_BUFFER_CACHE_SIZE 16

// Default buffer size
#define CNS_DEFAULT_BUFFER_SIZE (64 * 1024)

// Error codes
typedef enum {
    CNS_SUCCESS = 0,
    CNS_ERROR_MEMORY = -1,
    CNS_ERROR_INVALID_ARGUMENT = -2,
    CNS_ERROR_INVALID_FORMAT = -3,
    CNS_ERROR_UNSUPPORTED_VERSION = -4,
    CNS_ERROR_CHECKSUM_MISMATCH = -5,
    CNS_ERROR_EOF = -6,
    CNS_ERROR_IO = -7,
    CNS_ERROR_NOT_FOUND = -8,
    CNS_ERROR_OVERFLOW = -9
} cns_error_t;

// Graph flags
#define CNS_GRAPH_FLAG_DIRECTED     (1 << 0)
#define CNS_GRAPH_FLAG_WEIGHTED     (1 << 1)
#define CNS_GRAPH_FLAG_COMPRESSED   (1 << 2)

// Serialization flags
#define CNS_FLAG_BUILD_INDEX        (1 << 0)
#define CNS_FLAG_COMPRESS_VARINTS   (1 << 1)
#define CNS_FLAG_SKIP_CHECKSUM      (1 << 2)
#define CNS_FLAG_WEIGHTED_EDGES     (1 << 3)

// Binary format constants
#define CNS_BINARY_MAGIC    0x434E5342  // 'CNSB'
#define CNS_BINARY_VERSION  0x00010000  // 1.0

// Write buffer
typedef struct {
    uint8_t* data;
    size_t size;
    size_t capacity;
} cns_write_buffer_t;

// Read buffer
typedef struct {
    const void* data;
    size_t size;
    size_t position;
} cns_read_buffer_t;

// Buffer cache
typedef struct {
    cns_write_buffer_t** write_buffers;
    cns_read_buffer_t** read_buffers;
    size_t write_count;
    size_t read_count;
    size_t hits;
    size_t misses;
} cns_buffer_cache_t;

// Binary header
typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t flags;
    uint64_t timestamp;
    uint32_t graph_flags;
    uint64_t node_count;
    uint64_t edge_count;
    uint64_t metadata_offset;
    uint32_t checksum;
    uint8_t reserved[12];
} cns_binary_header_t;

// Binary metadata
typedef struct {
    uint64_t node_index_offset;
    uint64_t node_data_offset;
    uint64_t edge_data_offset;
    uint64_t property_pool_offset;
    uint32_t extension_count;
    uint32_t reserved;
} cns_binary_metadata_t;

// Node structure
typedef struct {
    uint64_t id;
    uint32_t type;
    uint32_t flags;
    void* data;
    size_t data_size;
} cns_node_t;

// Edge structure
typedef struct {
    uint64_t source;
    uint64_t target;
    uint32_t type;
    double weight;
    uint32_t flags;
    void* data;
    size_t data_size;
} cns_edge_t;

// Graph structure
typedef struct {
    cns_node_t* nodes;
    cns_edge_t* edges;
    size_t node_count;
    size_t edge_count;
    size_t node_capacity;
    size_t edge_capacity;
    uint32_t flags;
} cns_graph_t;

// Graph view for zero-copy access
typedef struct {
    const void* data;
    size_t size;
    const cns_binary_header_t* header;
    const cns_binary_metadata_t* metadata;
    const uint64_t* node_index;
    const uint8_t* node_data;
    const uint8_t* edge_data;
} cns_graph_view_t;

// Node view
typedef struct {
    const uint8_t* data;
    uint64_t node_id;
} cns_node_view_t;

// Graph statistics
typedef struct {
    size_t node_count;
    size_t edge_count;
    size_t memory_usage;
    double avg_degree;
} cns_graph_stats_t;

#endif /* CNS_BINARY_MATERIALIZER_TYPES_H */