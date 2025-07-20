#ifndef CNS_8T_GRAPH_L1_RDF_H
#define CNS_8T_GRAPH_L1_RDF_H

/*
 * CNS 8T L1-OPTIMIZED RDF GRAPH - CACHE-FRIENDLY RDF TRIPLE STORAGE
 * 
 * High-performance RDF graph structure optimized for L1 cache efficiency
 * with 16-byte aligned triples and SIMD operations. Designed to meet
 * 8-tick performance constraints through:
 * 
 * - 16-byte aligned triple structure (fits exactly in L1 cache line)
 * - SIMD-friendly data layout for vectorized operations
 * - Branchless algorithms for predictable performance
 * - Robin Hood hashing for O(1) lookups
 * - Cache-conscious memory access patterns
 * - Zero-copy operations where possible
 * 
 * This implementation focuses specifically on RDF triple storage with
 * minimal overhead and maximum performance.
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// CORE TYPE DEFINITIONS
// ============================================================================

// Forward declaration
typedef struct cns_8t_graph_rdf cns_8t_graph_rdf_t;

// Result codes
typedef enum {
    CNS_8T_RDF_OK = 0,
    CNS_8T_RDF_ERROR_MEMORY = -1,
    CNS_8T_RDF_ERROR_CAPACITY = -2,
    CNS_8T_RDF_ERROR_EXISTS = -3,
    CNS_8T_RDF_ERROR_NOT_FOUND = -4,
    CNS_8T_RDF_ERROR_INVALID = -5
} cns_8t_rdf_result_t;

// Object types for RDF triples
#define CNS_8T_RDF_TYPE_IRI      0x0000
#define CNS_8T_RDF_TYPE_LITERAL  0x0001
#define CNS_8T_RDF_TYPE_BLANK    0x0002

// Triple flags
#define CNS_8T_RDF_FLAG_INFERRED 0x0004
#define CNS_8T_RDF_FLAG_DELETED  0x0008
#define CNS_8T_RDF_FLAG_INDEXED  0x0010

// Wildcard value for pattern matching
#define CNS_8T_RDF_WILDCARD      0xFFFFFFFF

// ============================================================================
// GRAPH LIFECYCLE FUNCTIONS
// ============================================================================

/**
 * Create a new L1-optimized RDF graph
 * 
 * @param initial_capacity Initial capacity for triples (will be rounded up to multiple of 8)
 * @param enable_simd Enable SIMD optimizations (requires AVX2 on x86-64)
 * @return Pointer to new graph or NULL on failure
 * 
 * PERFORMANCE: O(1) - allocation only
 */
cns_8t_graph_rdf_t* cns_8t_graph_rdf_create(size_t initial_capacity, bool enable_simd);

/**
 * Destroy an RDF graph and free all resources
 * 
 * @param graph Graph to destroy
 * 
 * PERFORMANCE: O(1) - deallocation only
 */
void cns_8t_graph_rdf_destroy(cns_8t_graph_rdf_t* graph);

/**
 * Clear all triples from the graph
 * 
 * @param graph Graph to clear
 * 
 * PERFORMANCE: O(1) - resets counters only
 */
void cns_8t_graph_rdf_clear(cns_8t_graph_rdf_t* graph);

// ============================================================================
// TRIPLE INSERTION - 8-TICK GUARANTEED
// ============================================================================

/**
 * Insert a single RDF triple into the graph
 * 
 * @param graph Target graph
 * @param subject Interned subject ID
 * @param predicate Interned predicate ID
 * @param object Interned object ID
 * @param type Object type (IRI, LITERAL, or BLANK)
 * @param graph_id Named graph ID (0 for default graph)
 * @return true if inserted, false if already exists or error
 * 
 * PERFORMANCE: O(1) - completes within 8 CPU ticks
 */
bool cns_8t_graph_rdf_insert_triple(cns_8t_graph_rdf_t* graph,
                                   uint32_t subject,
                                   uint32_t predicate,
                                   uint32_t object,
                                   uint8_t type,
                                   uint16_t graph_id);

/**
 * Batch insert multiple triples using SIMD operations
 * 
 * @param graph Target graph
 * @param subjects Array of subject IDs
 * @param predicates Array of predicate IDs
 * @param objects Array of object IDs
 * @param type_flags Array of type and flag values
 * @param graph_ids Array of graph IDs
 * @param count Number of triples to insert
 * @return Number of triples successfully inserted
 * 
 * PERFORMANCE: O(n) but with SIMD acceleration (processes 8 at a time)
 */
size_t cns_8t_graph_rdf_insert_batch_simd(cns_8t_graph_rdf_t* graph,
                                          const uint32_t* subjects,
                                          const uint32_t* predicates,
                                          const uint32_t* objects,
                                          const uint16_t* type_flags,
                                          const uint16_t* graph_ids,
                                          size_t count);

// ============================================================================
// TRIPLE QUERY - SIMD ACCELERATED
// ============================================================================

/**
 * Check if a specific triple exists in the graph
 * 
 * @param graph Graph to search
 * @param subject Subject ID
 * @param predicate Predicate ID
 * @param object Object ID
 * @return true if triple exists, false otherwise
 * 
 * PERFORMANCE: O(1) - hash table lookup
 */
bool cns_8t_graph_rdf_contains_triple(const cns_8t_graph_rdf_t* graph,
                                      uint32_t subject,
                                      uint32_t predicate,
                                      uint32_t object);

/**
 * Find all triples matching a pattern (SIMD accelerated)
 * Use CNS_8T_RDF_WILDCARD for any component to match all values
 * 
 * @param graph Graph to search
 * @param subject Subject pattern (or WILDCARD)
 * @param predicate Predicate pattern (or WILDCARD)
 * @param object Object pattern (or WILDCARD)
 * @param results Array to store matching triple indices
 * @param max_results Maximum number of results to return
 * @return Number of matching triples found
 * 
 * PERFORMANCE: O(n) but with SIMD acceleration
 */
size_t cns_8t_graph_rdf_find_pattern_simd(const cns_8t_graph_rdf_t* graph,
                                          uint32_t subject,
                                          uint32_t predicate,
                                          uint32_t object,
                                          uint32_t* results,
                                          size_t max_results);

/**
 * Get triple by index
 * 
 * @param graph Graph containing the triple
 * @param index Triple index
 * @param[out] subject Subject ID
 * @param[out] predicate Predicate ID
 * @param[out] object Object ID
 * @param[out] type_flags Type and flags
 * @param[out] graph_id Graph ID
 * @return true if index valid, false otherwise
 * 
 * PERFORMANCE: O(1) - direct array access
 */
bool cns_8t_graph_rdf_get_triple(const cns_8t_graph_rdf_t* graph,
                                 uint32_t index,
                                 uint32_t* subject,
                                 uint32_t* predicate,
                                 uint32_t* object,
                                 uint16_t* type_flags,
                                 uint16_t* graph_id);

// ============================================================================
// PERFORMANCE OPTIMIZATION
// ============================================================================

/**
 * Prefetch a range of triples into L1 cache
 * 
 * @param graph Graph containing triples
 * @param start Starting index
 * @param count Number of triples to prefetch
 * 
 * PERFORMANCE: O(1) - issues prefetch instructions only
 */
void cns_8t_graph_rdf_prefetch_range(const cns_8t_graph_rdf_t* graph,
                                     size_t start,
                                     size_t count);

/**
 * Optimize graph layout for better cache utilization
 * 
 * @param graph Graph to optimize
 * @return Result code
 * 
 * PERFORMANCE: O(n log n) - sorts triples for locality
 */
cns_8t_rdf_result_t cns_8t_graph_rdf_optimize_layout(cns_8t_graph_rdf_t* graph);

// ============================================================================
// STATISTICS AND MONITORING
// ============================================================================

/**
 * Get graph statistics
 * 
 * @param graph Graph to query
 * @param[out] triple_count Number of triples
 * @param[out] capacity Current capacity
 * @param[out] load_factor Hash table load factor
 */
void cns_8t_graph_rdf_get_info(const cns_8t_graph_rdf_t* graph,
                               size_t* triple_count,
                               size_t* capacity,
                               double* load_factor);

/**
 * Get performance statistics
 * 
 * @param graph Graph to query
 * @param[out] cache_hits Number of cache hits
 * @param[out] cache_misses Number of cache misses
 * @param[out] simd_ops Number of SIMD operations performed
 */
void cns_8t_graph_rdf_get_stats(const cns_8t_graph_rdf_t* graph,
                                uint64_t* cache_hits,
                                uint64_t* cache_misses,
                                uint64_t* simd_ops);

/**
 * Reset performance counters
 * 
 * @param graph Graph to reset
 */
void cns_8t_graph_rdf_reset_stats(cns_8t_graph_rdf_t* graph);

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

/**
 * Extract object type from type_flags
 * 
 * @param type_flags Type and flags value
 * @return Object type (IRI, LITERAL, or BLANK)
 */
static inline uint8_t cns_8t_rdf_get_type(uint16_t type_flags) {
    return type_flags & 0x0003;
}

/**
 * Check if a specific flag is set
 * 
 * @param type_flags Type and flags value
 * @param flag Flag to check
 * @return true if flag is set
 */
static inline bool cns_8t_rdf_has_flag(uint16_t type_flags, uint16_t flag) {
    return (type_flags & flag) != 0;
}

// ============================================================================
// TESTING AND VALIDATION
// ============================================================================

#ifdef CNS_8T_TEST

/**
 * Run performance tests on the RDF graph implementation
 */
void test_graph_rdf_performance(void);

/**
 * Validate graph integrity
 * 
 * @param graph Graph to validate
 * @return true if valid, false if corruption detected
 */
bool cns_8t_graph_rdf_validate(const cns_8t_graph_rdf_t* graph);

#endif // CNS_8T_TEST

#ifdef __cplusplus
}
#endif

#endif // CNS_8T_GRAPH_L1_RDF_H