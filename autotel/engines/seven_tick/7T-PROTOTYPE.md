# 7T Prototype: C Implementation of the 7T Substrate

## Overview

This tutorial provides the complete specification for the C functions that form the 7T materialization layer. The C code is not an "application" in the traditional sense; it is the set of deterministic tools for proving and materializing a logical system defined in a TTL graph.

## Architecture

The 7T prototype consists of two main layers:

1. **Part 1: The Materialization Substrate** - Physical primitives for memory, identity, and logic
2. **Part 2: The AOT Toolchain** - Orchestration of the materialization pipeline

## Part 1: The Materialization Substrate

This layer provides the physical primitives for memory, identity, and logic.

### 1. The Arena Allocator: `arena.c`

**Purpose**: To provide a deterministic, pre-allocated memory contract. It eliminates runtime `malloc` and its associated non-determinism from the hot path.

**Core Data Structure**: The arena is a physical memory contract, not an abstract object.

```c
// arena.h
typedef struct {
    char* beg; // The current allocation pointer (bump allocator).
    char* end; // The end of the pre-allocated memory block.
} CNSArena;
```

**Key Function**: `cns_arena_alloc`

**Signature**: `void* cns_arena_alloc(CNSArena* arena, size_t size, size_t align);`

**Principle**: Allocation is a deterministic, O(1) pointer bump. Deallocation is an O(1) reset of the arena's state.

**Implementation Logic**:

1. Calculate the padding required to align the current `beg` pointer to the `align` boundary. The calculation `-(uintptr_t)arena->beg & (align - 1)` is a branchless, cycle-bounded operation.

2. Check if `padding + size` exceeds the remaining capacity (`arena->end - arena->beg`).

3. If there is insufficient capacity, it is a compile-time failure. The AOT toolchain must prove that all required memory fits within the arena. A runtime failure triggers a `CNS_ABORT()`.

4. Advance the `beg` pointer by `padding + size`.

5. Return the aligned pointer. The memory is not zeroed by default; this must be an explicit, AOT-planned action.

### 2. The String Interner: `interner.c`

**Purpose**: To eliminate strings from the hot path by mapping all URIs and literals to `uint32_t` IDs at compile time.

**Core Data Structure**: A hash table populated entirely by the AOT toolchain.

```c
// interner.h
typedef struct {
    const char* key; // The original string.
    uint32_t id;     // The canonical integer ID.
} CNSInternEntry;

typedef struct {
    CNSInternEntry* entries;
    size_t capacity;
    size_t length;
} CNSInterner;
```

**Key Function**: `cns_interner_get_id`

**Signature**: `uint32_t cns_interner_get_id(CNSInterner* interner, const char* str);`

**Principle**: A runtime call to this function is a failure condition. All strings must be interned during the AOT compilation phase. The function's purpose at runtime is to serve lookups for cold-path or diagnostic operations.

**Implementation Logic**:

1. Calculate the FNV-1a hash of the input string `str`.

2. Use the hash to find an entry in the `entries` array, using linear probing to resolve collisions.

3. If the string is found, return its `id`.

4. If the string is not found, trigger a `CNS_ASSERT()` failure. New strings cannot be created at runtime.

### 3. The In-Memory Graph: `graph.c`

**Purpose**: To define the physical memory layout of the TTL graph using the integer IDs from the interner.

**Core Data Structure**: A `CNSTriple` is a physical arrangement of three integer IDs.

```c
// graph.h
typedef struct {
    uint32_t subject;
    uint32_t predicate;
    uint32_t object;
} CNSTriple;

typedef struct {
    CNSTriple* triples; // Pointer into an arena.
    size_t count;
    size_t capacity;
} CNSGraph;
```

**Key Function**: `cns_graph_add_triple`

**Signature**: `void cns_graph_add_triple(CNSGraph* graph, CNSTriple triple);`

**Principle**: Adding a triple is an O(1) array append within the arena.

**Implementation Logic**:

1. Assert that `graph->count < graph->capacity`.

2. Copy the triple struct to `graph->triples[graph->count]`.

3. Increment `graph->count`.

### 4. The Lexer & Parser: `lexer.c`, `parser.c`

**Purpose**: To recognize a known, perfect TTL grammar and materialize it into the in-memory graph. Parsing is recognition, not interpretation.

**Key Function**: `cns_parser_materialize`

**Signature**: `CNSParseResult cns_parser_materialize(CNSParser* parser, const char* ttl_stream);`

**Principle**: The parser is a single-pass, non-recursive function that consumes a token stream and materializes `CNSTriple`s.

**Implementation Logic**:

1. The parser's main loop consumes tokens from a lexer.

2. The lexer is a deterministic finite automaton (DFA) materialized as a C function. It is generated at compile time by the AOT toolchain from the TTL grammar. It uses a switch statement or a jump table to recognize tokens in a fixed number of cycles.

3. For each recognized subject, predicate, and object token, the parser looks up its `uint32_t` ID from the AOT-populated interner.

4. After materializing a `CNSTriple`, the parser immediately calls `cns_shacl_validate_on_add`. A failure from the SHACL engine is a fatal parse error.

### 5. The SHACL Governance Engine: `shacl.c`

**Purpose**: To act as the compile-time governance gate. Validation is a condition of existence.

**Key Function**: `cns_shacl_validate_on_add`

**Signature**: `CNSValidationResult cns_shacl_validate_on_add(CNSShaclEngine* engine, CNSTriple triple);`

**Principle**: This is not a full, runtime SHACL validator. It is a set of hyper-optimized, 7T-compliant C functions, each corresponding to a specific SHACL constraint (e.g., `sh:minCount`, `sh:class`). These functions are generated by the AOT compiler from the SHACL shapes defined in the TTL IR.

**Implementation Logic**:

1. When a triple is added, this function is called.

2. It identifies which AOT-generated validation functions apply to the triple's subject.

3. It executes these specific, targeted checks. For example, a `sh:minCount` check might increment a counter associated with the subject and predicate.

4. A SHACL violation is a parse-time failure that halts the AOT compilation process. An invalid state cannot be built.

### 6. The Binary Materializer: `materializer.c`

**Purpose**: To serialize the validated in-memory graph into a final, memory-mappable binary artifact (`.plan.bin`).

**Key Function**: `cns_materializer_write_binary`

**Signature**: `bool cns_materializer_write_binary(CNSGraph* graph, const char* path);`

**Principle**: The binary format is the in-memory format. There is no complex serialization.

**Implementation Logic**:

1. Open the output file in binary write mode (`"wb"`).

2. Use a single `fwrite` call to perform a direct memory write of the `graph->triples` array.

3. The output file is a zero-copy, memory-mappable representation of the logical proof.

## Part 2: The AOT Toolchain

This layer orchestrates the materialization pipeline.

### 7. The AOT Orchestrator: `cns_transpile.c`

**Purpose**: The `main()` function that drives the logic → proof → physical artifact pipeline.

**Key Function**: `main`

**Signature**: `int main(int argc, char** argv);`

**Principle**: This function executes the complete, deterministic sequence of materialization.

**Implementation Logic**:

1. Initialize a `CNSArena`.

2. Initialize a `CNSInterner` by loading the AOT-generated vocabulary from a binary file.

3. Initialize a `CNSGraph` within the arena.

4. Initialize a `CNSShaclEngine` by loading the AOT-compiled SHACL rules.

5. Create a `CNSParser` instance, providing it with the arena, interner, graph, and SHACL engine.

6. Read the input `.ttl` file into a buffer.

7. Call `cns_parser_materialize` to parse, validate, and materialize the graph.

8. Call `cns_materializer_write_binary` to produce the final `.plan.bin` artifact.

## Implementation Details

### Memory Management Strategy

The 7T prototype uses a deterministic memory management approach:

- **Arena Allocation**: All memory is pre-allocated in a single arena
- **Bump Pointer**: O(1) allocation with no fragmentation
- **No Runtime malloc**: Eliminates non-determinism from hot paths
- **AOT Validation**: Memory requirements are proven at compile time

### String Handling

Strings are eliminated from hot paths through:

- **String Interning**: All strings mapped to integer IDs at compile time
- **Hash Table**: O(1) lookups for string-to-ID mapping
- **Runtime Failure**: New strings cannot be created at runtime
- **AOT Population**: All strings known and interned during compilation

### Graph Representation

The TTL graph is materialized as:

- **Triple Structure**: Physical arrangement of three uint32_t IDs
- **Array Storage**: Contiguous memory layout for optimal access
- **Arena Backing**: All triples stored within the arena
- **O(1) Addition**: Simple array append operations

### Parsing Strategy

The parser implements a deterministic approach:

- **DFA Lexer**: Generated at compile time from TTL grammar
- **Single Pass**: Non-recursive, linear parsing
- **Token Recognition**: Fixed-cycle token identification
- **Immediate Validation**: SHACL validation on each triple addition

### Validation Engine

SHACL validation is optimized for 7T compliance:

- **AOT Generation**: Validation functions generated at compile time
- **Targeted Checks**: Specific functions for each SHACL constraint
- **Parse-time Validation**: Validation occurs during parsing
- **Fatal Failures**: Invalid states cannot be built

## Usage Example

```bash
# Compile the AOT toolchain
gcc -o cns_transpile cns_transpile.c arena.c interner.c graph.c lexer.c parser.c shacl.c materializer.c

# Run the materialization pipeline
./cns_transpile input.ttl output.plan.bin
```

## Performance Characteristics

### Time Complexity

- **Arena Allocation**: O(1) - single pointer bump
- **String Lookup**: O(1) - hash table access
- **Triple Addition**: O(1) - array append
- **Parsing**: O(n) - single pass through input
- **Validation**: O(1) per triple - targeted checks

### Space Complexity

- **Arena**: O(1) - fixed pre-allocation
- **Interner**: O(n) - proportional to unique strings
- **Graph**: O(n) - proportional to triple count
- **Overall**: O(n) - linear with input size

### Memory Efficiency

- **Zero Fragmentation**: Bump pointer allocation
- **No Metadata**: Direct triple storage
- **Cache Friendly**: Contiguous memory layout
- **Minimal Overhead**: Only alignment padding

## Integration with CNS

The 7T prototype integrates with the CNS (Contextual Neural System) through:

1. **TTL Input**: Consumes TTL files defining logical systems
2. **SHACL Validation**: Ensures logical consistency
3. **Binary Output**: Produces memory-mappable artifacts
4. **AOT Integration**: Works with CNS AOT toolchain

## Benefits

1. **Deterministic**: No runtime non-determinism
2. **7T Compliant**: Optimized for 7T performance characteristics
3. **AOT Validated**: All requirements proven at compile time
4. **Memory Efficient**: Zero fragmentation, minimal overhead
5. **Fast**: O(1) operations throughout hot paths
6. **Safe**: No runtime memory allocation failures

## Conclusion

This completes the tutorial for the C functions required to implement the 7T substrate. Each function is a necessary component in the AOT pipeline that transforms a verifiable logical specification into a deterministic, high-performance physical artifact.

The 7T prototype demonstrates how to build a system that combines:
- Mathematical rigor (AOT validation)
- Performance optimization (7T compliance)
- Memory safety (arena allocation)
- Logical consistency (SHACL validation)

This approach is particularly well-suited for systems where predictability, performance, and correctness are critical requirements. 