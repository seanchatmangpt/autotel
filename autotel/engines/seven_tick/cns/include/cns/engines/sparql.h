#ifndef CNS_SPARQL_H
#define CNS_SPARQL_H

#include <stdint.h>
#include <stddef.h>

// Conditional SIMD includes for x86/x64 only
#ifdef __x86_64__
#include <immintrin.h>
#endif

// Branch prediction hints for optimal performance
#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

// CNS SPARQL engine structure with cache-friendly layout
typedef struct
{
  uint64_t *data; // [predicate][subject/64][type] - contiguous memory
  size_t max_subjects;
  size_t max_predicates;
  size_t max_objects;
  size_t cache_lines_per_predicate; // Optimized for cache line access
  size_t memory_usage;              // Track memory usage
} CNSSparqlEngine;

// Triple pattern structure
typedef struct
{
  uint32_t s; // subject
  uint32_t p; // predicate
  uint32_t o; // object
} CNSTriplePattern;

// Function declarations

// Engine creation and destruction
CNSSparqlEngine *cns_sparql_create(size_t max_s, size_t max_p, size_t max_o);
void cns_sparql_destroy(CNSSparqlEngine *engine);

// Core operations with optimized implementations
void cns_sparql_add_triple(CNSSparqlEngine *engine, uint32_t s, uint32_t p, uint32_t o);
int cns_sparql_ask_pattern(CNSSparqlEngine *engine, uint32_t s, uint32_t p, uint32_t o);

// Batch operations with SIMD optimization
void cns_sparql_ask_batch(CNSSparqlEngine *engine, CNSTriplePattern *patterns, int *results, size_t count);
void cns_sparql_simd_ask_batch(CNSSparqlEngine *engine, CNSTriplePattern *patterns, int *results, size_t count);

// Memory management
size_t cns_sparql_get_memory_usage(CNSSparqlEngine *engine);
void cns_sparql_set_memory_limit(size_t limit);

// Performance monitoring
uint64_t cns_sparql_get_cycles(void);
void cns_sparql_measure_operation_cycles(CNSSparqlEngine *engine, uint32_t s, uint32_t p, uint32_t o);

#endif // CNS_SPARQL_H