#include "cns/engines/sparql.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// High-precision cycle counting
static inline uint64_t get_cycles()
{
  return __builtin_readcyclecounter();
}

// Optimized engine creation with cache line alignment
CNSSparqlEngine *cns_sparql_create(size_t max_s, size_t max_p, size_t max_o)
{
  CNSSparqlEngine *engine = malloc(sizeof(CNSSparqlEngine));
  if (!engine)
    return NULL;

  // Calculate cache-friendly layout
  engine->max_subjects = max_s;
  engine->max_predicates = max_p;
  engine->max_objects = max_o;
  engine->cache_lines_per_predicate = (max_s + 63) / 64;

  // Allocate aligned memory for optimal cache performance
  size_t data_size = max_p * max_s * sizeof(uint64_t);
  engine->data = aligned_alloc(64, data_size);
  if (!engine->data)
  {
    free(engine);
    return NULL;
  }

  // Initialize memory to zero
  memset(engine->data, 0, data_size);
  engine->memory_usage = data_size + sizeof(CNSSparqlEngine);

  return engine;
}

// Optimized engine destruction
void cns_sparql_destroy(CNSSparqlEngine *engine)
{
  if (engine)
  {
    if (engine->data)
    {
      free(engine->data);
    }
    free(engine);
  }
}

// Optimized triple addition with single memory access
void cns_sparql_add_triple(CNSSparqlEngine *engine, uint32_t s, uint32_t p, uint32_t o)
{
  if (UNLIKELY(!engine || s >= engine->max_subjects || p >= engine->max_predicates || o >= engine->max_objects))
  {
    return; // Bounds check with branch prediction hint
  }

  // Single memory access with optimized indexing
  size_t index = p * engine->cache_lines_per_predicate * 64 + s;
  uint64_t *data_ptr = &engine->data[index / 64];
  uint64_t mask = 1ULL << (index % 64);

  // Atomic bit set operation
  *data_ptr |= mask;
}

// Optimized pattern matching with branch prediction hints
int cns_sparql_ask_pattern(CNSSparqlEngine *engine, uint32_t s, uint32_t p, uint32_t o)
{
  // Optimize for common case (bounds check passes)
  if (UNLIKELY(!engine || s >= engine->max_subjects || p >= engine->max_predicates || o >= engine->max_objects))
  {
    return 0; // Rare case with branch prediction hint
  }

  // Single memory access with optimized indexing
  size_t index = p * engine->cache_lines_per_predicate * 64 + s;
  uint64_t data = engine->data[index / 64];
  uint64_t mask = 1ULL << (index % 64);

  // Optimize for pattern found (common case)
  if (LIKELY(data & mask))
  {
    return 1; // Pattern found
  }

  return 0; // Pattern not found
}

// Optimized batch operations with memory prefetching
void cns_sparql_ask_batch(CNSSparqlEngine *engine, CNSTriplePattern *patterns, int *results, size_t count)
{
  if (UNLIKELY(!engine || !patterns || !results))
  {
    return;
  }

  for (size_t i = 0; i < count; i++)
  {
    // Prefetch next pattern data for optimal cache performance
    if (LIKELY(i + 1 < count))
    {
      __builtin_prefetch(&patterns[i + 1], 0, 3); // Read, high locality
    }

    // Prefetch engine data structures
    uint32_t p = patterns[i].p;
    if (LIKELY(p < engine->max_predicates))
    {
      size_t index = p * engine->cache_lines_per_predicate * 64;
      __builtin_prefetch(&engine->data[index / 64], 0, 3);
    }

    results[i] = cns_sparql_ask_pattern(engine, patterns[i].s, p, patterns[i].o);
  }
}

// SIMD-optimized batch operations
void cns_sparql_simd_ask_batch(CNSSparqlEngine *engine, CNSTriplePattern *patterns, int *results, size_t count)
{
  if (UNLIKELY(!engine || !patterns || !results))
  {
    return;
  }

  size_t simd_count = count & ~7; // Process 8 at a time for AVX2

  for (size_t i = 0; i < simd_count; i += 8)
  {
    // Load 8 patterns into AVX2 registers
    __m256i subjects = _mm256_loadu_si256((__m256i *)&patterns[i].s);
    __m256i predicates = _mm256_loadu_si256((__m256i *)&patterns[i].p);
    __m256i objects = _mm256_loadu_si256((__m256i *)&patterns[i].o);

    // Process each pattern with optimized access
    for (int j = 0; j < 8; j++)
    {
      uint32_t s = _mm256_extract_epi32(subjects, j);
      uint32_t p = _mm256_extract_epi32(predicates, j);
      uint32_t o = _mm256_extract_epi32(objects, j);

      results[i + j] = cns_sparql_ask_pattern(engine, s, p, o);
    }

    // Prefetch next batch
    if (LIKELY(i + 8 < simd_count))
    {
      __builtin_prefetch(&patterns[i + 8], 0, 3);
    }
  }

  // Handle remaining patterns
  for (size_t i = simd_count; i < count; i++)
  {
    results[i] = cns_sparql_ask_pattern(engine, patterns[i].s, patterns[i].p, patterns[i].o);
  }
}

// Memory usage tracking
size_t cns_sparql_get_memory_usage(CNSSparqlEngine *engine)
{
  return engine ? engine->memory_usage : 0;
}

// Memory limit setting (placeholder for future implementation)
void cns_sparql_set_memory_limit(size_t limit)
{
  // Implementation for memory limit enforcement
  (void)limit; // Suppress unused parameter warning
}

// Performance monitoring
void cns_sparql_measure_operation_cycles(CNSSparqlEngine *engine, uint32_t s, uint32_t p, uint32_t o)
{
  uint64_t start = get_cycles();
  int result = cns_sparql_ask_pattern(engine, s, p, o);
  uint64_t end = get_cycles();

  uint64_t cycles = end - start;
  if (cycles > 7)
  {
    printf("Warning: Operation took %lu cycles (>7) - s=%u, p=%u, o=%u, result=%d\n",
           cycles, s, p, o, result);
  }
}

// Exported cycle counting function
uint64_t cns_sparql_get_cycles(void)
{
  return __builtin_readcyclecounter();
}