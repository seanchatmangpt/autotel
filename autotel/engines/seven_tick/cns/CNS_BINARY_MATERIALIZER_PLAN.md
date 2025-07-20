# CNS Binary Materializer - C Implementation Plan

## Executive Summary

This document outlines the C API and implementation strategy for a high-performance binary materializer for the CNS (Core Nervous System) graph database. The design prioritizes:

- **Zero-copy deserialization** for maximum performance
- **7-tick compliance** for all core operations  
- **Platform portability** with endianness handling
- **Memory efficiency** through compact binary formats
- **SIMD optimization** where available

## Core Data Structures

### 1. Graph Representation

```c
// In-memory graph structure optimized for cache locality
typedef struct {
    uint32_t id;          // Node/edge identifier
    uint16_t type;        // Type identifier (class, property, etc.)
    uint16_t flags;       // Bit flags for characteristics
    uint32_t data_offset; // Offset to variable data
} cns_graph_element_t;

// Compact node representation (16 bytes aligned)
typedef struct {
    cns_graph_element_t base;
    uint32_t first_out_edge;  // First outgoing edge index
    uint32_t first_in_edge;   // First incoming edge index
} cns_node_t;

// Compact edge representation (24 bytes aligned)
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
```

### 2. Serialization Buffers

```c
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

// Serialization context for state tracking
typedef struct {
    cns_graph_t *graph;
    void *buffer;            // Either read or write buffer
    uint32_t *id_map;        // ID remapping table
    size_t id_map_size;
    uint64_t start_cycles;
    uint64_t end_cycles;
    char error_msg[256];
} cns_serialize_ctx_t;
```

### 3. Binary Format Header

```c
// File format header (64 bytes)
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
```

## Key Functions

### 1. Core Serialization API

```c
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
```

### 2. Element Serialization

```c
// Node serialization
int cns_node_write(cns_write_buffer_t *buffer,
                  const cns_node_t *node,
                  const uint8_t *data_pool);

int cns_node_read(cns_read_buffer_t *buffer,
                 cns_node_t *node,
                 cns_serialize_ctx_t *ctx);

// Edge serialization
int cns_edge_write(cns_write_buffer_t *buffer,
                  const cns_edge_t *edge,
                  const uint8_t *data_pool);

int cns_edge_read(cns_read_buffer_t *buffer,
                 cns_edge_t *edge,
                 cns_serialize_ctx_t *ctx);

// Batch operations for performance
int cns_nodes_write_batch(cns_write_buffer_t *buffer,
                         const cns_node_t *nodes,
                         uint32_t count,
                         const uint8_t *data_pool);

int cns_edges_write_batch(cns_write_buffer_t *buffer,
                         const cns_edge_t *edges,
                         uint32_t count,
                         const uint8_t *data_pool);
```

### 3. Buffer Management

```c
// Write buffer operations
cns_write_buffer_t* cns_write_buffer_create(size_t initial_size);
void cns_write_buffer_destroy(cns_write_buffer_t *buffer);
int cns_write_buffer_ensure_capacity(cns_write_buffer_t *buffer, 
                                   size_t required);
int cns_write_buffer_write_u32(cns_write_buffer_t *buffer, uint32_t value);
int cns_write_buffer_write_bytes(cns_write_buffer_t *buffer, 
                               const void *data, size_t size);

// Read buffer operations
cns_read_buffer_t* cns_read_buffer_create(const void *data, size_t size);
void cns_read_buffer_destroy(cns_read_buffer_t *buffer);
int cns_read_buffer_read_u32(cns_read_buffer_t *buffer, uint32_t *value);
int cns_read_buffer_read_bytes(cns_read_buffer_t *buffer, 
                              void *data, size_t size);
```

### 4. Platform Portability

```c
// Endianness detection and conversion
static inline bool cns_is_big_endian(void) {
    union { uint32_t i; char c[4]; } test = {0x01020304};
    return test.c[0] == 1;
}

static inline uint32_t cns_swap32(uint32_t value) {
    return ((value & 0xFF000000) >> 24) |
           ((value & 0x00FF0000) >> 8)  |
           ((value & 0x0000FF00) << 8)  |
           ((value & 0x000000FF) << 24);
}

static inline uint16_t cns_swap16(uint16_t value) {
    return ((value & 0xFF00) >> 8) | ((value & 0x00FF) << 8);
}

// Conditional byte swapping based on platform
#define CNS_TO_LE32(x) (cns_is_big_endian() ? cns_swap32(x) : (x))
#define CNS_TO_LE16(x) (cns_is_big_endian() ? cns_swap16(x) : (x))
#define CNS_FROM_LE32(x) CNS_TO_LE32(x)
#define CNS_FROM_LE16(x) CNS_TO_LE16(x)
```

### 5. Error Handling

```c
// Error codes
typedef enum {
    CNS_SERIALIZE_OK = 0,
    CNS_SERIALIZE_ERROR_MEMORY = -1,
    CNS_SERIALIZE_ERROR_IO = -2,
    CNS_SERIALIZE_ERROR_FORMAT = -3,
    CNS_SERIALIZE_ERROR_VERSION = -4,
    CNS_SERIALIZE_ERROR_CHECKSUM = -5,
    CNS_SERIALIZE_ERROR_BOUNDS = -6,
} cns_serialize_error_t;

// Error reporting
const char* cns_serialize_error_string(int error_code);
int cns_serialize_get_last_error(cns_serialize_ctx_t *ctx);
```

## Memory Management Strategy

### 1. Allocation Patterns

```c
// Memory pool for reduced fragmentation
typedef struct {
    uint8_t *memory;
    size_t size;
    size_t used;
    size_t alignment;
} cns_memory_pool_t;

// Pool-based allocation
void* cns_pool_alloc(cns_memory_pool_t *pool, size_t size);
void cns_pool_reset(cns_memory_pool_t *pool);
void cns_pool_destroy(cns_memory_pool_t *pool);
```

### 2. Buffer Reuse

```c
// Reusable buffer cache
typedef struct {
    cns_write_buffer_t **buffers;
    size_t count;
    size_t capacity;
    pthread_mutex_t lock;
} cns_buffer_cache_t;

cns_write_buffer_t* cns_buffer_cache_acquire(cns_buffer_cache_t *cache);
void cns_buffer_cache_release(cns_buffer_cache_t *cache, 
                             cns_write_buffer_t *buffer);
```

## Performance Optimizations

### 1. Zero-Copy Deserialization

```c
// Direct memory mapping
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

int cns_graph_view_open(cns_graph_view_t *view, const char *path);
void cns_graph_view_close(cns_graph_view_t *view);
```

### 2. SIMD Operations

```c
#ifdef __AVX2__
// Vectorized checksum computation
uint32_t cns_checksum_avx2(const uint8_t *data, size_t size);

// Vectorized memory copy
void cns_memcpy_avx2(void *dst, const void *src, size_t size);
#endif

// Fallback implementations
uint32_t cns_checksum_scalar(const uint8_t *data, size_t size);
void cns_memcpy_scalar(void *dst, const void *src, size_t size);
```

### 3. Compression Support

```c
// Optional compression (80/20 rule - compress large data only)
typedef enum {
    CNS_COMPRESS_NONE = 0,
    CNS_COMPRESS_LZ4 = 1,
    CNS_COMPRESS_ZSTD = 2,
} cns_compress_type_t;

int cns_compress_data(const uint8_t *src, size_t src_size,
                     uint8_t *dst, size_t *dst_size,
                     cns_compress_type_t type);

int cns_decompress_data(const uint8_t *src, size_t src_size,
                       uint8_t *dst, size_t *dst_size,
                       cns_compress_type_t type);
```

## Implementation Phases

### Phase 1: Core Infrastructure (Week 1)
- Basic data structures
- Write/read buffer implementation
- Simple node/edge serialization
- Unit tests for core functions

### Phase 2: Binary Format (Week 2)
- Header structure and validation
- Platform portability (endianness)
- Checksum computation
- Error handling framework

### Phase 3: Performance Features (Week 3)
- Memory-mapped I/O support
- Zero-copy deserialization
- SIMD optimizations
- Buffer caching and reuse

### Phase 4: Advanced Features (Week 4)
- Compression support
- Incremental serialization
- Streaming API
- Performance benchmarks

## Testing Strategy

### 1. Unit Tests
- Buffer operations
- Endianness conversion
- Individual element serialization
- Error conditions

### 2. Integration Tests
- Full graph round-trip
- Cross-platform compatibility
- Large graph handling
- Memory leak detection

### 3. Performance Tests
- Serialization throughput
- Deserialization speed
- Memory usage
- 7-tick compliance verification

## Benchmarking Goals

- **Serialization**: > 1GB/s throughput
- **Deserialization**: > 2GB/s throughput (zero-copy)
- **Memory overhead**: < 10% of graph size
- **7-tick operations**: 100% compliance for lookups

## Integration Points

1. **CNS Types**: Reuse existing type system
2. **OWL Engine**: Serialize OWL axioms efficiently
3. **SPARQL**: Fast graph loading for queries
4. **Telemetry**: Performance tracking hooks

## Example Usage

```c
// Serialize a graph
cns_graph_t *graph = create_test_graph();
cns_write_buffer_t *buffer = cns_write_buffer_create(1024 * 1024);

if (cns_graph_serialize(graph, buffer, 0) == CNS_SERIALIZE_OK) {
    // Write to file
    FILE *f = fopen("graph.cnsb", "wb");
    fwrite(buffer->data, 1, buffer->size, f);
    fclose(f);
}

// Deserialize with zero-copy
cns_graph_view_t view;
if (cns_graph_view_open(&view, "graph.cnsb") == CNS_SERIALIZE_OK) {
    // Direct access to graph data
    printf("Nodes: %u, Edges: %u\n", 
           view.header->node_count, 
           view.header->edge_count);
    
    // Query operations work directly on mapped memory
    const cns_node_t *node = &view.nodes[0];
    // ...
    
    cns_graph_view_close(&view);
}
```

## Success Criteria

1. **Correctness**: 100% round-trip fidelity
2. **Performance**: Meet all throughput goals
3. **Portability**: Works on x86_64, ARM64, big/little endian
4. **Integration**: Seamless with existing CNS components
5. **7-tick**: All lookup operations under 7 CPU cycles