#ifndef CNS_SPARQL_H
#define CNS_SPARQL_H

#include "cns/types.h"
#include <stdint.h>
#include <stdbool.h>

// Forward declarations
typedef struct SPARQLEngine SPARQLEngine;
typedef struct SPARQLResult SPARQLResult;

// SPARQL Result structure (80/20: simplified for performance)
struct SPARQLResult
{
  uint32_t count; // Number of result triples
  struct
  {
    uint32_t subject;
    uint32_t predicate;
    uint32_t object;
  } *triples; // Result triples (NULL for simplified results)
};

// SPARQL Performance Statistics
typedef struct
{
  uint64_t total_queries;      // Total queries executed
  uint64_t cache_hits;         // Number of cache hits
  double cache_hit_rate;       // Cache hit rate (0.0 to 1.0)
  uint32_t total_triples;      // Total triples in store
  double avg_cycles_per_query; // Average cycles per query
} SPARQLStats;

// Core SPARQL Engine API (80/20 optimized)

// Create SPARQL engine (7 cycles target)
// 80/20: Single instance is sufficient for most use cases
SPARQLEngine *cns_sparql_create(size_t initial_capacity);

// Destroy SPARQL engine (3 cycles target)
void cns_sparql_destroy(SPARQLEngine *engine);

// Add triple to store (5 cycles target)
// 80/20: Most operations are simple triple additions
int cns_sparql_add_triple(SPARQLEngine *engine, uint32_t subject, uint32_t predicate, uint32_t object);

// Execute SPARQL query (15 cycles target)
// 80/20: Optimized for common SELECT patterns
SPARQLResult *cns_sparql_execute(SPARQLEngine *engine, const char *query);

// Free SPARQL result (3 cycles target)
void cns_sparql_free_result(SPARQLResult *result);

// Get performance statistics
SPARQLStats cns_sparql_get_stats(SPARQLEngine *engine);

// 80/20 optimized triple lookup (3 cycles target)
// 80/20: Most lookups are simple pattern matches
uint32_t cns_sparql_find_triples(SPARQLEngine *engine, uint32_t subject, uint32_t predicate, uint32_t object);

// 80/20 Optimization Macros

// Check if operation is 7T compliant
#define SPARQL_7T_COMPLIANT(cycles) ((cycles) <= 7)

// Assert 7T compliance
#define SPARQL_ASSERT_7T(cycles) assert(SPARQL_7T_COMPLIANT(cycles))

// Performance tracking
#define SPARQL_TRACK_PERFORMANCE(engine, start_cycles)   \
  do                                                     \
  {                                                      \
    uint64_t cycles = cns_get_cycles() - (start_cycles); \
    if (engine)                                          \
      engine->total_cycles += cycles;                    \
  } while (0)

// Common SPARQL patterns (80/20: most queries follow these patterns)
#define SPARQL_PATTERN_SELECT_ALL "SELECT ?s ?p ?o WHERE { ?s ?p ?o }"
#define SPARQL_PATTERN_SELECT_SUBJECT "SELECT ?s WHERE { ?s ?p ?o }"
#define SPARQL_PATTERN_SELECT_PREDICATE "SELECT ?p WHERE { ?s ?p ?o }"
#define SPARQL_PATTERN_SELECT_OBJECT "SELECT ?o WHERE { ?s ?p ?o }"

// Error codes
#define SPARQL_OK 0
#define SPARQL_ERR_INVALID_QUERY 1
#define SPARQL_ERR_NO_RESULTS 2
#define SPARQL_ERR_MEMORY 3
#define SPARQL_ERR_SYNTAX 4

#endif // CNS_SPARQL_H