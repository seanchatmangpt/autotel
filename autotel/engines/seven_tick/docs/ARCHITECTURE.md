# 7T Engine Architecture

## System Overview

The 7T Engine is built around the principle of **≤7 CPU cycles** and **<10 nanoseconds** performance for core operations. The architecture is designed for extreme performance while maintaining proper semantic computing semantics.

## Core Design Principles

### 1. 7-Tick Performance
Every core operation is designed to execute in exactly 7 CPU cycles:
- **Tick 1**: Division/Address calculation
- **Tick 2**: Bit manipulation
- **Tick 3-4**: Memory load
- **Tick 5**: Bitwise operation + branch
- **Tick 6**: Memory load
- **Tick 7**: Comparison/result

### 2. Memory Hierarchy Optimization
- **L1 Cache**: All hot data structures fit in L1 cache
- **Bit Vectors**: Efficient sparse matrix representation
- **Hash Tables**: O(1) string interning
- **SIMD**: Parallel processing of 4 operations

### 3. Zero-Copy Design
- Direct memory access patterns
- Minimal data movement
- Cache-friendly access patterns

## Component Architecture

### Runtime Engine (`runtime/src/`)

#### EngineState Structure
```c
typedef struct {
    // String interning with hash table
    StringHashTable string_table[2];  // [0]=strings, [1]=alloc_sizes
    
    // Bit vector storage
    BitVector** predicate_vectors;     // [pred_id][chunk] bit matrix
    BitVector** object_vectors;        // [obj_id][chunk] bit matrix
    
    // Statistics
    size_t triple_count;
    uint32_t max_subject_id;
    uint32_t max_predicate_id;
    uint32_t max_object_id;
} EngineState;
```

#### Key Features
- **String Interning**: O(1) hash table lookup
- **Bit Vector Operations**: Efficient set operations
- **Memory Management**: Zero leaks with proper cleanup
- **Statistics Tracking**: Real-time performance metrics

### SPARQL Engine (`c_src/sparql7t.c`)

#### S7TEngine Structure
```c
typedef struct {
    uint64_t* predicate_vectors;  // [pred_id][chunk] bit matrix
    uint64_t* object_vectors;     // [obj_id][chunk] bit matrix
    ObjectNode** ps_to_o_index;   // [pred_id * max_subjects + subj_id] -> ObjectNode*
    
    size_t max_subjects;
    size_t max_predicates;
    size_t max_objects;
    size_t stride_len;            // (max_subjects + 63) / 64
} S7TEngine;
```

#### Multiple Objects Support
```c
typedef struct ObjectNode {
    uint32_t object;
    struct ObjectNode* next;
} ObjectNode;
```

#### Key Features
- **7-Tick Pattern Matching**: Exact 7-cycle implementation
- **Batch Operations**: 4 patterns in ≤7 ticks
- **Multiple Objects**: Linked list for (predicate, subject) pairs
- **SIMD Optimization**: Parallel processing

### SHACL Engine (`c_src/shacl7t.c`)

#### ShaclEngine Structure
```c
typedef struct {
    uint64_t* node_class_vectors;     // [node_id][chunk] bit matrix
    uint64_t* node_property_vectors;  // [node_id][chunk] bit matrix
    CompiledShape* shapes;            // Compiled shape definitions
    
    size_t max_nodes;
    size_t max_shapes;
    size_t stride_len;
} ShaclEngine;
```

#### Key Features
- **Shape Validation**: Compiled shape checking
- **Batch Validation**: 4 nodes in ≤7 ticks
- **Constraint Checking**: Property and class validation
- **Performance Optimization**: Bit vector operations

### OWL Engine (`c_src/owl7t.c`)

#### OWLEngine Structure
```c
typedef struct {
    EngineState* base_engine;         // Base SPARQL engine
    uint64_t* transitive_properties;  // Bit vector of transitive properties
    uint64_t* symmetric_properties;   // Bit vector of symmetric properties
    
    size_t max_properties;
    size_t stride_len;
} OWLEngine;
```

#### Key Features
- **Transitive Reasoning**: Depth-limited DFS
- **Property Inference**: Symmetric and transitive properties
- **Closure Computation**: Efficient reasoning
- **Integration**: Built on SPARQL engine

### Compiler (`compiler/src/`)

#### Query Optimization
- **MCTS Algorithm**: Monte Carlo Tree Search for query planning
- **Cost Modeling**: Real engine statistics for cost estimation
- **Code Generation**: Tier-specific optimizations

#### Cost Model
```c
typedef struct {
    size_t total_triples;
    size_t max_predicate_id;
    size_t max_object_id;
    size_t* predicate_cardinalities;
    size_t* object_cardinalities;
    double* predicate_selectivities;
} CostModel;
```

## Data Flow Architecture

### 1. Triple Addition Flow
```
Input Triple (s, p, o)
    ↓
String Interning (O(1) hash table)
    ↓
Bit Vector Update (predicate_vectors[p][chunk] |= bit)
    ↓
Bit Vector Update (object_vectors[o][chunk] |= bit)
    ↓
Object List Update (ps_to_o_index[p*max_s+s] = new_node)
```

### 2. Pattern Matching Flow
```
Input Pattern (s, p, o)
    ↓
Tick 1: Calculate chunk = s / 64
    ↓
Tick 2: Calculate bit = 1ULL << (s % 64)
    ↓
Tick 3-4: Load predicate_word = predicate_vectors[p][chunk]
    ↓
Tick 5: Check if (predicate_word & bit) == 0
    ↓
Tick 6: Load object_list = ps_to_o_index[p*max_s+s]
    ↓
Tick 7: Check if object exists in list
```

### 3. Batch Processing Flow
```
Input Batch (4 patterns)
    ↓
Tick 1: Load 4 subject chunks in parallel
    ↓
Tick 2: Compute 4 bit masks in parallel
    ↓
Tick 3: Load 4 predicate vectors in parallel
    ↓
Tick 4: Check 4 predicate bits in parallel
    ↓
Tick 5: Load 4 object lists in parallel
    ↓
Tick 6: Check 4 object matches in parallel
    ↓
Tick 7: Combine 4 results in parallel
```

## Memory Layout

### Bit Vector Storage
```
predicate_vectors[pred_id][chunk] = 64-bit word
object_vectors[obj_id][chunk] = 64-bit word

Where:
- pred_id = predicate identifier
- obj_id = object identifier  
- chunk = (subject_id / 64)
- bit = 1ULL << (subject_id % 64)
```

### Object List Storage
```
ps_to_o_index[pred_id * max_subjects + subj_id] = ObjectNode*

ObjectNode {
    uint32_t object;
    ObjectNode* next;
}
```

### String Hash Table
```
string_table[0] = StringHashTable {
    StringHashEntry* entries[HASH_TABLE_SIZE];
}

StringHashEntry {
    char* string;
    uint32_t id;
    StringHashEntry* next;
}
```

## Performance Optimizations

### 1. Cache Locality
- **Stride Length**: Optimized for cache line size
- **Memory Layout**: Contiguous arrays for vector operations
- **Access Patterns**: Sequential access where possible

### 2. SIMD Operations
- **4-Way Parallelism**: Process 4 patterns/nodes simultaneously
- **Vector Instructions**: Leverage CPU vector units
- **Memory Bandwidth**: Maximize memory throughput

### 3. Branch Prediction
- **Early Exit**: Fail fast for non-matches
- **Common Case**: Optimize for single object per pattern
- **Predictable Branches**: Structured control flow

### 4. Memory Management
- **Zero Initialization**: Use calloc for clean state
- **Minimal Allocation**: Pre-allocate fixed-size structures
- **Proper Cleanup**: s7t_destroy() for memory safety

## Scalability Considerations

### Horizontal Scaling
- **Sharding**: Partition by predicate or subject ranges
- **Load Balancing**: Distribute queries across engines
- **Consistency**: Eventual consistency for distributed operations

### Vertical Scaling
- **Memory**: Support for larger datasets
- **CPU**: Multi-core parallel processing
- **Storage**: Persistent storage integration

### Performance Scaling
- **Linear Scaling**: O(1) operations scale linearly
- **Memory Scaling**: Bit vectors scale with data size
- **Query Scaling**: Batch operations scale with query count

## Integration Points

### External Systems
- **RDF Parsers**: Turtle, N-Triples, RDF/XML
- **Query Languages**: SPARQL, GraphQL
- **Storage Systems**: PostgreSQL, MongoDB, Redis
- **Message Queues**: Kafka, RabbitMQ

### APIs
- **C API**: Direct function calls
- **Python Bindings**: ctypes integration
- **REST API**: HTTP/JSON interface
- **gRPC**: High-performance RPC

## Security Considerations

### Memory Safety
- **Bounds Checking**: Validate all array accesses
- **Null Pointer**: Check all pointer operations
- **Memory Leaks**: Comprehensive cleanup functions

### Input Validation
- **String Sanitization**: Validate string inputs
- **Size Limits**: Enforce maximum sizes
- **Type Checking**: Validate data types

### Access Control
- **Authentication**: User identity verification
- **Authorization**: Permission checking
- **Audit Logging**: Operation tracking

## Future Architecture

### Planned Enhancements
- **Compression**: Dictionary encoding for strings
- **Indexing**: Secondary indexes for complex queries
- **Caching**: Multi-level caching system
- **Streaming**: Real-time data processing

### Research Directions
- **Quantum Computing**: Quantum algorithm integration
- **Neuromorphic**: Brain-inspired computing
- **Federated**: Distributed semantic computing
- **Edge Computing**: IoT and edge deployment 