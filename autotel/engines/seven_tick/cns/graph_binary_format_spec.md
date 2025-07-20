# Graph Binary Format Specification v1.0

## Overview
This specification defines a custom binary format optimized for serializing and deserializing graph structures with minimal size and maximum performance.

## Design Goals
- **Compact Size**: Minimize storage requirements through efficient encoding
- **Fast Random Access**: Support O(1) access to nodes and edges
- **Extensibility**: Version-aware format allowing future enhancements
- **Cache-Friendly**: Aligned data structures for optimal CPU cache usage
- **Type Safety**: Strong typing with explicit data sizes

## File Structure Overview

```
[Header]
[Metadata Section]
[Node Index]
[Node Data Section]
[Edge Index]
[Edge Data Section]
[Property Data Pool]
[Extension Sections] (optional)
```

## Detailed Specification

### 1. Header (32 bytes)

```c
struct Header {
    uint32_t magic;           // 0x47524150 ('GRAP' in ASCII)
    uint16_t version_major;   // 1
    uint16_t version_minor;   // 0
    uint64_t file_size;       // Total file size in bytes
    uint32_t flags;           // Bit flags (see below)
    uint32_t checksum;        // CRC32 of entire file (excluding this field)
    uint64_t metadata_offset; // Offset to metadata section
};
```

**Flags** (32-bit field):
- Bit 0: Graph type (0 = undirected, 1 = directed)
- Bit 1: Weighted edges (0 = unweighted, 1 = weighted)
- Bit 2: Node properties present (0 = no, 1 = yes)
- Bit 3: Edge properties present (0 = no, 1 = yes)
- Bit 4: Compressed (0 = no, 1 = yes)
- Bit 5: Sparse representation (0 = dense, 1 = sparse)
- Bits 6-31: Reserved for future use

### 2. Metadata Section (Variable size)

```c
struct Metadata {
    uint64_t node_count;      // Total number of nodes
    uint64_t edge_count;      // Total number of edges
    uint64_t node_index_offset; // Offset to node index
    uint64_t node_data_offset;  // Offset to node data section
    uint64_t edge_index_offset; // Offset to edge index
    uint64_t edge_data_offset;  // Offset to edge data section
    uint64_t property_pool_offset; // Offset to property data pool
    uint32_t node_id_size;    // Size of node ID (1, 2, 4, or 8 bytes)
    uint32_t edge_id_size;    // Size of edge ID (1, 2, 4, or 8 bytes)
    uint16_t extension_count; // Number of extension sections
    uint16_t padding;         // Alignment padding
    // Extension descriptors follow (if extension_count > 0)
};
```

### 3. Node Index (Fixed size per entry)

For fast O(1) node lookup:

```c
struct NodeIndexEntry {
    uint64_t data_offset;     // Offset to node data in Node Data Section
    uint32_t out_degree;      // Number of outgoing edges
    uint32_t in_degree;       // Number of incoming edges (0 for undirected)
};
```

### 4. Node Data Section

Each node's data is stored contiguously:

```c
struct NodeData {
    // Node ID (size determined by node_id_size in metadata)
    // For node_id_size = 4:
    uint32_t node_id;
    
    // Properties (if node properties flag is set)
    uint16_t property_count;
    PropertyRef properties[property_count]; // See PropertyRef below
    
    // Adjacency data
    EdgeRef out_edges[out_degree]; // Outgoing edges
    EdgeRef in_edges[in_degree];   // Incoming edges (directed graphs only)
};

struct PropertyRef {
    uint32_t key_offset;      // Offset in property pool to key string
    uint32_t value_offset;    // Offset in property pool to value
    uint8_t value_type;       // Type of value (see Property Types)
    uint8_t padding[3];       // Alignment
};
```

### 5. Edge Index (Optional, for edge-centric access)

```c
struct EdgeIndexEntry {
    uint64_t source_node_id;  // Source node ID
    uint64_t target_node_id;  // Target node ID
    uint64_t data_offset;     // Offset to edge data
};
```

### 6. Edge Data Section

```c
struct EdgeData {
    // Edge weight (if weighted flag is set)
    float weight;             // Or double for higher precision
    
    // Properties (if edge properties flag is set)
    uint16_t property_count;
    PropertyRef properties[property_count];
};

struct EdgeRef {
    // Target node ID (size determined by node_id_size)
    uint32_t target_node_id;  // Example for 4-byte IDs
    uint32_t edge_data_offset; // Offset to EdgeData (0 if no data)
};
```

### 7. Property Data Pool

A contiguous pool of property data:

```c
// Strings are stored as:
struct StringData {
    uint32_t length;
    char data[length];        // UTF-8 encoded, no null terminator
};

// Property values are stored based on type
```

**Property Types** (uint8_t):
- 0x01: INT32
- 0x02: INT64
- 0x03: FLOAT32
- 0x04: FLOAT64
- 0x05: STRING
- 0x06: BINARY
- 0x07: BOOLEAN
- 0x08: ARRAY
- 0x09: MAP
- 0x10-0xFF: Reserved

### 8. Extension Sections

For future extensibility:

```c
struct ExtensionDescriptor {
    uint32_t extension_id;    // Unique extension identifier
    uint64_t offset;          // Offset to extension data
    uint64_t size;            // Size of extension data
};
```

## Encoding Optimizations

### 1. Variable-Length Integer Encoding (VarInt)

For sparse graphs or when node/edge IDs have small values:

```
0xxxxxxx                    = 0-127 (1 byte)
10xxxxxx xxxxxxxx           = 128-16383 (2 bytes)
110xxxxx xxxxxxxx xxxxxxxx  = 16384-2097151 (3 bytes)
...
```

### 2. Delta Encoding

For sorted adjacency lists, store deltas instead of absolute IDs:

```c
// Instead of: [100, 105, 150, 200]
// Store: [100, 5, 45, 50]
```

### 3. Bit-Packed Boolean Arrays

For boolean node/edge properties:

```c
// 8 boolean properties in 1 byte
uint8_t bool_props = 0b10110101;
```

### 4. Run-Length Encoding

For repeated values in properties:

```c
struct RLEValue {
    uint32_t count;
    // Value follows
};
```

## Memory Alignment

All sections start at 8-byte boundaries for optimal memory access:

```c
#define ALIGN_TO_8(x) (((x) + 7) & ~7)
```

## Example Usage

### Writing a Simple Graph

```c
// Graph: A -> B -> C
//        A -> C

Header header = {
    .magic = 0x47524150,
    .version_major = 1,
    .version_minor = 0,
    .flags = 0x1, // Directed graph
    // ... other fields
};

// Node data for A (ID=0)
NodeData node_a = {
    .node_id = 0,
    .property_count = 0,
    .out_edges = {
        {.target_node_id = 1, .edge_data_offset = 0},
        {.target_node_id = 2, .edge_data_offset = 0}
    }
};
```

## Performance Characteristics

### Time Complexity
- Node lookup: O(1) using node index
- Edge traversal: O(degree) for adjacency list
- Property access: O(1) with offset lookup

### Space Complexity
- Overhead per node: 16 bytes (index entry)
- Overhead per edge: 8 bytes (EdgeRef)
- Property overhead: 12 bytes per property

## Compression Options

When compression flag is set:
1. **Dictionary Compression**: For property keys
2. **LZ4**: For property values and string data
3. **Bit Compression**: For sparse boolean matrices

## Version Compatibility

- **Forward Compatible**: Older readers skip unknown extension sections
- **Backward Compatible**: Newer readers handle missing sections gracefully
- **Version Negotiation**: Magic number and version fields enable detection

## Implementation Notes

1. **Endianness**: All multi-byte values are stored in little-endian format
2. **String Encoding**: All strings are UTF-8 encoded
3. **Null Values**: Offset of 0 indicates null/missing data
4. **Checksums**: CRC32 for data integrity
5. **Streaming**: Format supports incremental reading/writing

## Example File Layout

```
Offset  | Size    | Content
--------|---------|------------------
0x0000  | 32      | Header
0x0020  | 64      | Metadata
0x0060  | 16*N    | Node Index (N nodes)
0x1000  | Var     | Node Data Section
0x4000  | 16*E    | Edge Index (E edges)
0x8000  | Var     | Edge Data Section
0xC000  | Var     | Property Pool
0xF000  | Var     | Extensions
```

## Future Extensions

Reserved extension IDs:
- 0x1000: Spatial indexing (R-tree)
- 0x1001: Temporal data (timestamps)
- 0x1002: Graph algorithms cache
- 0x1003: Distributed graph sharding info
- 0x1004: Machine learning embeddings

## Reference Implementation

See accompanying C implementation for:
- Binary reader/writer
- In-memory graph representation
- Conversion utilities
- Validation tools