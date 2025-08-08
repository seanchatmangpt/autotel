/*
 * BitActor TTL Compiler - Header
 * 8-tick TTL-to-bytecode compilation for BitActor
 */

#ifndef BITACTOR_TTL_COMPILER_H
#define BITACTOR_TTL_COMPILER_H

#include <stdint.h>
#include <stddef.h>

// Forward declarations to avoid header conflicts
struct CompiledOps;
struct TripleSpace;

// Compiled triple representation
typedef struct {
    uint32_t subject_hash;
    uint32_t predicate_hash;
    uint32_t object_hash;
} compiled_triple_t;

// Main 8-tick compiler function
uint32_t compile_ttl_8tick(const char* ttl_input, size_t input_len, 
                          uint8_t* bytecode_output, size_t max_output);

// Triple compilation in 8 ticks
compiled_triple_t compile_triple_8tick(const char* s, const char* p, const char* o);

// Stream processing integration
int process_turtle_stream_8tick(const char* turtle_data, size_t size,
                               void (*callback)(compiled_triple_t*, void*),
                               void* context);

// Benchmark utilities
void benchmark_ttl_compiler(void);

// Integration with BitActor
int compile_semantic_operations(void* arena, const char* ttl_spec, 
                               struct CompiledOps* ops, struct TripleSpace* space);

#endif // BITACTOR_TTL_COMPILER_H