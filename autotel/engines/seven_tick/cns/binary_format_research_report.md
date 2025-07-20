# Binary Serialization Formats for Graph Data Research Report

## Executive Summary

This report analyzes existing binary serialization formats for graph data structures, evaluating their performance characteristics, design patterns, and suitability for the 7-tick CNS system. Based on comprehensive research of current formats and benchmarks from 2024-2025, we provide recommendations for developing a custom binary format optimized for graph serialization.

## Key Performance Metrics Comparison

### 1. Protocol Buffers (Protobuf)
- **Serialization Speed**: ~1827 ns/op
- **Deserialization Speed**: ~1058 ns/op (Rust), ~496 ns/op (Go)
- **Compression Ratio**: Good, smaller than JSON
- **Memory Footprint**: Moderate (requires deserialization into objects)
- **Graph Suitability**: Moderate - requires schema definition, good for structured nodes/edges

**Strengths:**
- Strong schema validation and evolution
- Wide ecosystem support
- Good compression ratios
- Excellent for RPC/network communication

**Weaknesses:**
- Requires full deserialization before access
- Additional memory allocation for objects
- Overhead for simple graph structures

### 2. FlatBuffers
- **Serialization Speed**: ~856-878 ns/op
- **Deserialization Speed**: ~18.89 ns/op (Go), ~331 ns/op (Rust), overall ~711 ns/op in stress tests
- **Compression Ratio**: Larger files due to alignment requirements (~432 bytes vs 299 for Protobuf)
- **Memory Footprint**: Minimal - zero-copy access
- **Graph Suitability**: Excellent - can access specific nodes without loading entire graph

**Strengths:**
- Zero-copy deserialization - direct buffer access
- Can decode specific graph elements without touching rest of data
- Excellent for mobile/embedded systems
- Superior runtime performance

**Weaknesses:**
- Larger file sizes due to alignment padding
- More complex API
- Less flexible schema evolution

### 3. MessagePack
- **Serialization Speed**: Fast
- **Deserialization Speed**: Very fast, outperforms JSON significantly
- **Compression Ratio**: Excellent - 68% size reduction vs JSON
- **Memory Footprint**: Low
- **Graph Suitability**: Good for smaller graphs

**Strengths:**
- Simple API and ease of use
- Excellent compression for small data
- Fast serialization/deserialization
- Good cross-language support

**Weaknesses:**
- No schema validation
- Less suitable for very large graphs
- No zero-copy capabilities

### 4. Cap'n Proto
- **Serialization Speed**: Near zero (no encoding step)
- **Deserialization Speed**: Zero (direct memory mapping)
- **Compression Ratio**: Similar to Protobuf
- **Memory Footprint**: Minimal - true zero-copy
- **Graph Suitability**: Excellent - "infinity times faster" than Protobuf for direct access

**Strengths:**
- No encoding/decoding required
- True zero-copy with memory mapping
- Excellent for streaming data
- Can be used as both interchange format and in-memory representation

**Weaknesses:**
- Less mature ecosystem
- More complex to implement
- Limited language support compared to Protobuf

### 5. Apache Arrow
- **Serialization Speed**: Not applicable (columnar format)
- **Deserialization Speed**: Zero (shared memory)
- **Compression Ratio**: Efficient columnar storage
- **Memory Footprint**: Optimized for analytics
- **Graph Suitability**: Good for graph analytics, less ideal for traversal

**Strengths:**
- Zero serialization overhead
- 80-90% cost reduction in analytical workloads
- Excellent for columnar graph data (node/edge properties)
- Direct memory sharing between processes

**Weaknesses:**
- Columnar format less suitable for graph traversal
- More complex for simple use cases
- Designed for analytics rather than general serialization

### 6. BSON
- **Performance**: Slower than JSON in most cases
- **Compression**: Larger than JSON
- **Graph Suitability**: Poor - not recommended

## Graph-Specific Formats and Techniques

### CSR (Compressed Sparse Row)
- Optimal for vertex-oriented algorithms and traversal
- Efficient memory usage for sparse graphs
- Standard format in graph processing libraries

### Edge List Format
- Simple representation: source, target, [weight]
- Easy to convert to other formats
- Good for edge-oriented computations

### GraphBLAS Binary Format
- Automatically selects between hypersparse and standard CSR
- Flexible format selection based on graph density
- New 2025 Binary Sparse Format specification for cross-platform compatibility

### DCSR/DCSC (Double Compressed)
- Eliminates storage for empty rows/columns
- Further compression for very sparse graphs
- Requires additional index arrays

## Key Design Patterns to Adopt

1. **Zero-Copy Access** (from FlatBuffers/Cap'n Proto)
   - Direct buffer access without deserialization
   - Critical for large graphs where only portions need access

2. **Variable-Size Encoding** (from GT format)
   - Use smallest byte size (1, 2, 4, or 8 bytes) for indices
   - Significant space savings for smaller graphs

3. **Lazy Evaluation** (from FlatBuffers)
   - Decode only requested nodes/edges
   - Essential for 7-tick performance constraints

4. **Alignment Optimization** (from Arrow)
   - Proper memory alignment for SIMD operations
   - Cache-friendly data layout

5. **Hybrid Storage** (from GraphBLAS)
   - Automatic format selection based on graph characteristics
   - CSR for dense regions, COO for sparse regions

## Design Patterns to Avoid

1. **Full Deserialization Requirement** (JSON/XML pattern)
   - Never require loading entire graph to access one element

2. **Fixed-Size Allocations** (naive approaches)
   - Wastes space for small graphs

3. **Schema Rigidity** (some Protobuf uses)
   - Allow flexibility for graph evolution

4. **Single Format Lock-in**
   - Support multiple internal representations

## Recommendations for Custom 7-Tick Format

### Core Design Principles

1. **Zero-Copy First**
   - Adopt Cap'n Proto's approach: format suitable for both storage and in-memory use
   - Memory-mapped file support for large graphs

2. **Adaptive Compression**
   - Variable-size encoding for indices (1-8 bytes based on graph size)
   - Run-length encoding for sparse regions
   - Optional compression layers (LZ4/Zstd)

3. **Hybrid Representation**
   - CSR for dense subgraphs
   - COO (Coordinate) format for sparse regions
   - Automatic format selection based on density threshold

4. **7-Tick Optimization**
   - Align data structures for SIMD operations
   - Cache-line aware layout
   - Minimize pointer chasing

### Proposed Format Structure

```
Header (64 bytes):
- Magic number (8 bytes)
- Version (4 bytes)
- Graph properties (directed/undirected, weighted, etc.) (4 bytes)
- Node count (8 bytes)
- Edge count (8 bytes)
- Index byte size (1 byte)
- Compression flags (1 byte)
- Reserved (30 bytes)

Node Section:
- Node offsets array (variable size based on index byte size)
- Node data (properties, labels)

Edge Section:
- Format indicator (CSR/COO/Hybrid)
- Edge data based on format

Metadata Section:
- Additional graph properties
- Index structures for fast lookup
```

### Performance Targets

Based on current best-in-class formats:
- Deserialization: <20 ns/op for single node access (matching FlatBuffers)
- Memory overhead: <10% over raw data
- Compression ratio: 50-70% of JSON size
- Zero-copy access for all read operations

## Conclusion

For the 7-tick CNS system, we recommend developing a custom format that combines:
- Zero-copy architecture from Cap'n Proto/FlatBuffers
- Adaptive compression from GraphBLAS
- Variable-size encoding from GT format
- Hybrid storage strategies for different graph densities

This approach will provide optimal performance within the 7-tick constraint while maintaining flexibility for various graph types and sizes.