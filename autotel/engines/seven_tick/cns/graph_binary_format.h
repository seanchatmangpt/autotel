#ifndef GRAPH_BINARY_FORMAT_H
#define GRAPH_BINARY_FORMAT_H

#include <stdint.h>
#include <stdbool.h>

#define GRAPH_MAGIC 0x47524150  // 'GRAP' in ASCII
#define GRAPH_VERSION_MAJOR 1
#define GRAPH_VERSION_MINOR 0

// Alignment macro
#define ALIGN_TO_8(x) (((x) + 7) & ~7)

// Graph flags
#define GRAPH_FLAG_DIRECTED     (1 << 0)
#define GRAPH_FLAG_WEIGHTED     (1 << 1)
#define GRAPH_FLAG_NODE_PROPS   (1 << 2)
#define GRAPH_FLAG_EDGE_PROPS   (1 << 3)
#define GRAPH_FLAG_COMPRESSED   (1 << 4)
#define GRAPH_FLAG_SPARSE       (1 << 5)

// Property types
typedef enum {
    PROP_TYPE_INT32   = 0x01,
    PROP_TYPE_INT64   = 0x02,
    PROP_TYPE_FLOAT32 = 0x03,
    PROP_TYPE_FLOAT64 = 0x04,
    PROP_TYPE_STRING  = 0x05,
    PROP_TYPE_BINARY  = 0x06,
    PROP_TYPE_BOOLEAN = 0x07,
    PROP_TYPE_ARRAY   = 0x08,
    PROP_TYPE_MAP     = 0x09
} PropertyType;

// File header structure (32 bytes)
typedef struct {
    uint32_t magic;           // Magic number for format identification
    uint16_t version_major;   // Major version number
    uint16_t version_minor;   // Minor version number
    uint64_t file_size;       // Total file size in bytes
    uint32_t flags;           // Bit flags for graph properties
    uint32_t checksum;        // CRC32 checksum of file
    uint64_t metadata_offset; // Offset to metadata section
} GraphHeader;

// Metadata section
typedef struct {
    uint64_t node_count;          // Total number of nodes
    uint64_t edge_count;          // Total number of edges
    uint64_t node_index_offset;   // Offset to node index
    uint64_t node_data_offset;    // Offset to node data section
    uint64_t edge_index_offset;   // Offset to edge index
    uint64_t edge_data_offset;    // Offset to edge data section
    uint64_t property_pool_offset;// Offset to property data pool
    uint32_t node_id_size;        // Size of node ID (1, 2, 4, or 8)
    uint32_t edge_id_size;        // Size of edge ID (1, 2, 4, or 8)
    uint16_t extension_count;     // Number of extension sections
    uint16_t padding;             // Alignment padding
} GraphMetadata;

// Node index entry (16 bytes)
typedef struct {
    uint64_t data_offset;     // Offset to node data
    uint32_t out_degree;      // Number of outgoing edges
    uint32_t in_degree;       // Number of incoming edges
} NodeIndexEntry;

// Property reference (12 bytes)
typedef struct {
    uint32_t key_offset;      // Offset to property key in pool
    uint32_t value_offset;    // Offset to property value in pool
    uint8_t  value_type;      // Type of the property value
    uint8_t  padding[3];      // Alignment padding
} PropertyRef;

// Edge reference (8 bytes for 32-bit node IDs)
typedef struct {
    uint32_t target_node_id;  // Target node ID
    uint32_t edge_data_offset;// Offset to edge data (0 if no data)
} EdgeRef;

// Extension descriptor
typedef struct {
    uint32_t extension_id;    // Unique extension identifier
    uint64_t offset;          // Offset to extension data
    uint64_t size;            // Size of extension data
} ExtensionDescriptor;

// String data in property pool
typedef struct {
    uint32_t length;          // String length (excluding this field)
    // char data[] follows
} StringData;

// Variable integer encoding functions
static inline int write_varint(uint8_t* buf, uint64_t value) {
    int len = 0;
    while (value >= 0x80) {
        buf[len++] = (value & 0x7F) | 0x80;
        value >>= 7;
    }
    buf[len++] = value & 0x7F;
    return len;
}

static inline int read_varint(const uint8_t* buf, uint64_t* value) {
    *value = 0;
    int shift = 0;
    int len = 0;
    uint8_t byte;
    do {
        byte = buf[len++];
        *value |= (uint64_t)(byte & 0x7F) << shift;
        shift += 7;
    } while (byte & 0x80);
    return len;
}

// Helper macros for different node ID sizes
#define READ_NODE_ID(ptr, size) \
    ((size) == 1 ? *(uint8_t*)(ptr) : \
     (size) == 2 ? *(uint16_t*)(ptr) : \
     (size) == 4 ? *(uint32_t*)(ptr) : \
     *(uint64_t*)(ptr))

#define WRITE_NODE_ID(ptr, value, size) \
    do { \
        if ((size) == 1) *(uint8_t*)(ptr) = (uint8_t)(value); \
        else if ((size) == 2) *(uint16_t*)(ptr) = (uint16_t)(value); \
        else if ((size) == 4) *(uint32_t*)(ptr) = (uint32_t)(value); \
        else *(uint64_t*)(ptr) = (uint64_t)(value); \
    } while(0)

// CRC32 calculation (simplified, actual implementation would use lookup table)
uint32_t calculate_crc32(const void* data, size_t length);

// Validation functions
bool validate_header(const GraphHeader* header);
bool validate_metadata(const GraphMetadata* metadata);

// Extension IDs
#define EXT_ID_SPATIAL_INDEX    0x1000
#define EXT_ID_TEMPORAL_DATA    0x1001
#define EXT_ID_ALGO_CACHE       0x1002
#define EXT_ID_SHARDING_INFO    0x1003
#define EXT_ID_ML_EMBEDDINGS    0x1004

#endif // GRAPH_BINARY_FORMAT_H