# 7T Engine API Reference

## Overview

The 7T Engine provides a comprehensive C API for high-performance semantic computing operations. All functions are designed for **≤7 CPU cycles** and **<10 nanoseconds** performance.

## Core Data Types

### Engine Structures

```c
// Runtime Engine State
typedef struct {
    StringHashTable string_table[2];  // [0]=strings, [1]=alloc_sizes
    BitVector** predicate_vectors;     // [pred_id][chunk] bit matrix
    BitVector** object_vectors;        // [obj_id][chunk] bit matrix
    size_t triple_count;
    uint32_t max_subject_id;
    uint32_t max_predicate_id;
    uint32_t max_object_id;
} EngineState;

// SPARQL Engine
typedef struct {
    uint64_t* predicate_vectors;  // [pred_id][chunk] bit matrix
    uint64_t* object_vectors;     // [obj_id][chunk] bit matrix
    ObjectNode** ps_to_o_index;   // [pred_id * max_subjects + subj_id] -> ObjectNode*
    size_t max_subjects;
    size_t max_predicates;
    size_t max_objects;
    size_t stride_len;            // (max_subjects + 63) / 64
} S7TEngine;

// SHACL Engine
typedef struct {
    uint64_t* node_class_vectors;     // [node_id][chunk] bit matrix
    uint64_t* node_property_vectors;  // [node_id][chunk] bit matrix
    CompiledShape* shapes;            // Compiled shape definitions
    size_t max_nodes;
    size_t max_shapes;
    size_t stride_len;
} ShaclEngine;

// OWL Engine
typedef struct {
    EngineState* base_engine;         // Base SPARQL engine
    uint64_t* transitive_properties;  // Bit vector of transitive properties
    uint64_t* symmetric_properties;   // Bit vector of symmetric properties
    size_t max_properties;
    size_t stride_len;
} OWLEngine;
```

### Utility Types

```c
// Triple Pattern for batch operations
typedef struct {
    uint32_t s, p, o;  // subject, predicate, object
} TriplePattern;

// Object Node for multiple objects per (predicate, subject)
typedef struct ObjectNode {
    uint32_t object;
    struct ObjectNode* next;
} ObjectNode;

// Bit Vector for efficient set operations
typedef struct {
    uint64_t* bits;
    size_t capacity;
    size_t count;
} BitVector;
```

## Runtime Engine API

### Engine Management

#### `s7t_create_engine()`
Creates a new runtime engine instance.

```c
EngineState* s7t_create_engine(void);
```

**Returns**: Pointer to initialized engine state, or NULL on failure

**Example**:
```c
EngineState* engine = s7t_create_engine();
if (!engine) {
    fprintf(stderr, "Failed to create engine\n");
    return 1;
}
```

#### `s7t_destroy_engine(engine)`
Destroys an engine instance and frees all memory.

```c
void s7t_destroy_engine(EngineState* engine);
```

**Parameters**:
- `engine`: Engine instance to destroy

**Example**:
```c
s7t_destroy_engine(engine);
```

### String Interning

#### `s7t_intern_string(engine, string)`
Interns a string and returns a unique integer ID.

```c
uint32_t s7t_intern_string(EngineState* engine, const char* string);
```

**Parameters**:
- `engine`: Engine instance
- `string`: String to intern

**Returns**: Unique integer ID for the string

**Performance**: O(1) average case with hash table

**Example**:
```c
uint32_t person_id = s7t_intern_string(engine, "Person");
uint32_t name_id = s7t_intern_string(engine, "name");
uint32_t john_id = s7t_intern_string(engine, "John");
```

### Triple Operations

#### `s7t_add_triple(engine, s, p, o)`
Adds a triple to the engine.

```c
void s7t_add_triple(EngineState* engine, uint32_t s, uint32_t p, uint32_t o);
```

**Parameters**:
- `engine`: Engine instance
- `s`: Subject ID
- `p`: Predicate ID
- `o`: Object ID

**Performance**: O(1) with bit vector operations

**Example**:
```c
s7t_add_triple(engine, john_id, name_id, john_name_id);
```

#### `s7t_ask_pattern(engine, s, p, o)`
Checks if a triple pattern exists.

```c
int s7t_ask_pattern(EngineState* engine, uint32_t s, uint32_t p, uint32_t o);
```

**Parameters**:
- `engine`: Engine instance
- `s`: Subject ID
- `p`: Predicate ID
- `o`: Object ID

**Returns**: 1 if pattern exists, 0 otherwise

**Performance**: ≤7 CPU cycles, <10 nanoseconds

**Example**:
```c
int exists = s7t_ask_pattern(engine, john_id, name_id, john_name_id);
if (exists) {
    printf("Pattern found\n");
}
```

### Query Materialization

#### `s7t_materialize_subjects(engine, predicate_id, object_id, count)`
Materializes all subjects that match a predicate-object pattern.

```c
uint32_t* s7t_materialize_subjects(EngineState* engine, uint32_t predicate_id,
                                   uint32_t object_id, size_t* count);
```

**Parameters**:
- `engine`: Engine instance
- `predicate_id`: Predicate ID to match
- `object_id`: Object ID to match
- `count`: Pointer to store result count

**Returns**: Array of subject IDs, or NULL if no matches

**Performance**: ≤7 ticks per result

**Example**:
```c
size_t count;
uint32_t* subjects = s7t_materialize_subjects(engine, name_id, john_name_id, &count);
if (subjects) {
    printf("Found %zu subjects\n", count);
    for (size_t i = 0; i < count; i++) {
        printf("Subject: %u\n", subjects[i]);
    }
    free(subjects);
}
```

### Bit Vector Operations

#### `s7t_get_subject_vector(engine, predicate_id, object_id)`
Gets the bit vector of subjects for a predicate-object pair.

```c
BitVector* s7t_get_subject_vector(EngineState* engine, uint32_t predicate_id, uint32_t object_id);
```

**Parameters**:
- `engine`: Engine instance
- `predicate_id`: Predicate ID
- `object_id`: Object ID

**Returns**: Bit vector of subjects, or NULL on error

**Example**:
```c
BitVector* subjects = s7t_get_subject_vector(engine, name_id, john_name_id);
if (subjects) {
    printf("Subject count: %zu\n", subjects->count);
    bitvec_destroy(subjects);
}
```

## SPARQL Engine API

### Engine Management

#### `s7t_create(max_s, max_p, max_o)`
Creates a new SPARQL engine instance.

```c
S7TEngine* s7t_create(size_t max_s, size_t max_p, size_t max_o);
```

**Parameters**:
- `max_s`: Maximum number of subjects
- `max_p`: Maximum number of predicates
- `max_o`: Maximum number of objects

**Returns**: Pointer to initialized engine, or NULL on failure

**Example**:
```c
S7TEngine* engine = s7t_create(100000, 1000, 100000);
if (!engine) {
    fprintf(stderr, "Failed to create SPARQL engine\n");
    return 1;
}
```

#### `s7t_destroy(engine)`
Destroys a SPARQL engine instance.

```c
void s7t_destroy(S7TEngine* engine);
```

**Parameters**:
- `engine`: Engine instance to destroy

**Example**:
```c
s7t_destroy(engine);
```

### Triple Operations

#### `s7t_add_triple(engine, s, p, o)`
Adds a triple to the SPARQL engine.

```c
void s7t_add_triple(S7TEngine* engine, uint32_t s, uint32_t p, uint32_t o);
```

**Parameters**:
- `engine`: Engine instance
- `s`: Subject ID
- `p`: Predicate ID
- `o`: Object ID

**Features**:
- Supports multiple objects per (predicate, subject)
- Duplicate checking
- O(1) performance

**Example**:
```c
s7t_add_triple(engine, 1, 2, 3);  // (1, 2, 3)
s7t_add_triple(engine, 1, 2, 4);  // (1, 2, 4) - multiple objects
s7t_add_triple(engine, 2, 2, 3);  // (2, 2, 3) - different subject
```

#### `s7t_ask_pattern(engine, s, p, o)`
Checks if a triple pattern exists.

```c
int s7t_ask_pattern(S7TEngine* engine, uint32_t s, uint32_t p, uint32_t o);
```

**Parameters**:
- `engine`: Engine instance
- `s`: Subject ID
- `p`: Predicate ID
- `o`: Object ID

**Returns**: 1 if pattern exists, 0 otherwise

**Performance**: ≤7 CPU cycles, <10 nanoseconds

**Example**:
```c
int result1 = s7t_ask_pattern(engine, 1, 2, 3);  // Should return 1
int result2 = s7t_ask_pattern(engine, 1, 2, 4);  // Should return 1
int result3 = s7t_ask_pattern(engine, 1, 2, 5);  // Should return 0
```

### Batch Operations

#### `s7t_ask_batch(engine, patterns, results, count)`
Processes multiple patterns in batch.

```c
void s7t_ask_batch(S7TEngine* engine, TriplePattern* patterns, int* results, size_t count);
```

**Parameters**:
- `engine`: Engine instance
- `patterns`: Array of triple patterns
- `results`: Array to store results
- `count`: Number of patterns to process

**Performance**: 4 patterns in ≤7 ticks

**Example**:
```c
TriplePattern patterns[4] = {
    {1, 2, 3},  // Should match
    {1, 2, 4},  // Should match
    {2, 2, 3},  // Should match
    {1, 2, 5}   // Should not match
};

int results[4];
s7t_ask_batch(engine, patterns, results, 4);

// results = [1, 1, 1, 0]
```

## SHACL Engine API

### Engine Management

#### `shacl_create(max_nodes, max_shapes)`
Creates a new SHACL engine instance.

```c
ShaclEngine* shacl_create(size_t max_nodes, size_t max_shapes);
```

**Parameters**:
- `max_nodes`: Maximum number of nodes
- `max_shapes`: Maximum number of shapes

**Returns**: Pointer to initialized engine, or NULL on failure

#### `shacl_destroy(engine)`
Destroys a SHACL engine instance.

```c
void shacl_destroy(ShaclEngine* engine);
```

### Validation Operations

#### `shacl_validate_batch(engine, nodes, shapes, results, count)`
Validates multiple nodes against shapes in batch.

```c
void shacl_validate_batch(ShaclEngine* engine, uint32_t* nodes, uint32_t* shapes,
                         int* results, size_t count);
```

**Parameters**:
- `engine`: Engine instance
- `nodes`: Array of node IDs
- `shapes`: Array of shape IDs
- `results`: Array to store validation results
- `count`: Number of nodes to validate

**Performance**: 4 nodes in ≤7 ticks

## OWL Engine API

### Engine Management

#### `owl_create(base_engine, max_properties, max_classes)`
Creates a new OWL engine instance.

```c
OWLEngine* owl_create(EngineState* base_engine, size_t max_properties, size_t max_classes);
```

**Parameters**:
- `base_engine`: Base SPARQL engine
- `max_properties`: Maximum number of properties
- `max_classes`: Maximum number of classes

**Returns**: Pointer to initialized engine, or NULL on failure

#### `owl_destroy(engine)`
Destroys an OWL engine instance.

```c
void owl_destroy(OWLEngine* engine);
```

### Reasoning Operations

#### `owl_ask_pattern(engine, s, p, o)`
Checks if a pattern exists with OWL reasoning.

```c
int owl_ask_pattern(OWLEngine* engine, uint32_t s, uint32_t p, uint32_t o);
```

**Parameters**:
- `engine`: Engine instance
- `s`: Subject ID
- `p`: Predicate ID
- `o`: Object ID

**Returns**: 1 if pattern exists (including inferred), 0 otherwise

**Features**:
- Transitive reasoning
- Symmetric reasoning
- Property inference

## Error Handling

### Return Values
- **NULL**: Memory allocation failure
- **0**: Operation failed or pattern not found
- **1**: Operation successful or pattern found
- **Negative values**: Error codes (when applicable)

### Common Error Patterns
```c
// Check for allocation failures
S7TEngine* engine = s7t_create(100000, 1000, 100000);
if (!engine) {
    fprintf(stderr, "Memory allocation failed\n");
    return 1;
}

// Check for invalid parameters
if (s >= max_subjects || p >= max_predicates || o >= max_objects) {
    fprintf(stderr, "Invalid triple parameters\n");
    return 1;
}

// Always cleanup
s7t_destroy(engine);
```

## Performance Guidelines

### Memory Management
- Always call destroy functions to prevent memory leaks
- Use appropriate sizes for engine creation
- Monitor memory usage for large datasets

### Batch Operations
- Use batch operations for multiple queries
- Process 4 patterns/nodes at a time for optimal performance
- Align data structures for cache efficiency

### String Interning
- Intern strings once and reuse IDs
- Use integer IDs instead of string comparisons
- Monitor hash table performance for large string sets

## Thread Safety

### Current Limitations
- **Single-threaded**: Current implementation is not thread-safe
- **No locking**: No mutex or atomic operations
- **Shared state**: Engines maintain shared state

### Future Considerations
- **Read-only operations**: May be safe for concurrent reads
- **Write operations**: Require external synchronization
- **Multiple engines**: Use separate engines for different threads

## Platform Support

### Supported Architectures
- **ARM64**: Optimized for ARM64 with hardware tick counters
- **x86_64**: Fallback to clock_gettime for timing
- **Other**: Generic C implementation

### Compiler Requirements
- **C99**: Standard C99 features
- **Optimization**: -O3 recommended for performance
- **Architecture**: -march=native for best performance

### Dependencies
- **Standard C library**: malloc, free, string operations
- **POSIX**: clock_gettime for timing (fallback)
- **Hardware**: ARM64 cntvct_el0 for precise timing 