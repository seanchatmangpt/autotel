#ifndef SPARQL7T_OPTIMIZED_H
#define SPARQL7T_OPTIMIZED_H

#include <stdint.h>
#include <stddef.h>
#include <immintrin.h>

// Branch prediction hints for optimal performance
#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

// Optimized engine structure with cache-friendly layout
typedef struct
{
  uint64_t *data; // [predicate][subject/64][type] - contiguous memory
  size_t max_subjects;
  size_t max_predicates;
  size_t max_objects;
  size_t cache_lines_per_predicate; // Optimized for cache line access
  size_t memory_usage;              // Track memory usage
} OptimizedEngine;

// Triple pattern structure
typedef struct
{
  uint32_t s; // subject
  uint32_t p; // predicate
  uint32_t o; // object
} TriplePattern;

// Function declarations

// Engine creation and destruction
OptimizedEngine *optimized_create(size_t max_s, size_t max_p, size_t max_o);
void optimized_destroy(OptimizedEngine *engine);

// Core operations with optimized implementations
void optimized_add_triple(OptimizedEngine *engine, uint32_t s, uint32_t p, uint32_t o);
int optimized_ask_pattern(OptimizedEngine *engine, uint32_t s, uint32_t p, uint32_t o);

// Batch operations with SIMD optimization
void optimized_ask_batch(OptimizedEngine *engine, TriplePattern *patterns, int *results, size_t count);
void simd_ask_batch(OptimizedEngine *engine, TriplePattern *patterns, int *results, size_t count);

// Memory management
size_t optimized_get_memory_usage(OptimizedEngine *engine);
void optimized_set_memory_limit(size_t limit);

// Performance monitoring
uint64_t get_cycles(void);
void measure_operation_cycles(OptimizedEngine *engine, uint32_t s, uint32_t p, uint32_t o);

#endif // SPARQL7T_OPTIMIZED_H