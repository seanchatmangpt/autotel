/**
 * SPARQL AOT Integration Header
 * 
 * Provides AOT-enhanced SPARQL query execution with 7-tick compliance
 */

#ifndef CNS_SPARQL_AOT_H
#define CNS_SPARQL_AOT_H

#include <stdint.h>
#include <stddef.h>
#include "sparql.h"

#ifdef __cplusplus
extern "C" {
#endif

// Forward declaration
typedef struct CNSSparqlEngineAOT CNSSparqlEngineAOT;

// Customer result structure for demo queries
typedef struct {
    uint32_t customer_id;
    float lifetime_value;
    char* name;
} CustomerResult;

/**
 * Create AOT-enhanced SPARQL engine
 * @param max_triples Maximum number of triples to store
 * @param buffer_size Size of working buffers
 * @return Pointer to engine or NULL on failure
 */
CNSSparqlEngineAOT* cns_sparql_create_aot(size_t max_triples, size_t buffer_size);

/**
 * Destroy AOT-enhanced SPARQL engine
 * @param engine Engine to destroy
 */
void cns_sparql_destroy_aot(CNSSparqlEngineAOT* engine);

/**
 * Add triple to AOT engine
 * @param engine Target engine
 * @param subject Subject ID
 * @param predicate Predicate ID
 * @param object Object ID
 * @param type_id Type ID for the subject
 */
void cns_sparql_add_triple_aot(CNSSparqlEngineAOT* engine, 
                               uint32_t subject, uint32_t predicate, 
                               uint32_t object, uint32_t type_id);

/**
 * Execute type query (most common 80/20 pattern)
 * Finds all triples with matching type_id
 * @param engine Query engine
 * @param type_id Type to search for
 * @param results Output buffer for matching triple indices
 * @return Number of matches found
 */
uint32_t cns_sparql_query_by_type_aot(CNSSparqlEngineAOT* engine, 
                                      uint32_t type_id,
                                      uint32_t* results);

/**
 * Execute predicate query
 * Finds all triples with matching predicate
 * @param engine Query engine
 * @param predicate_id Predicate to search for
 * @param results Output buffer for matching triple indices
 * @return Number of matches found
 */
uint32_t cns_sparql_query_by_predicate_aot(CNSSparqlEngineAOT* engine,
                                           uint32_t predicate_id,
                                           uint32_t* results);

/**
 * Execute hash join (second most common pattern)
 * @param engine Query engine
 * @param left_keys Keys from left relation
 * @param left_values Values from left relation
 * @param left_count Number of left tuples
 * @param right_keys Keys from right relation
 * @param right_values Values from right relation
 * @param right_count Number of right tuples
 * @param result_left Output buffer for matched left values
 * @param result_right Output buffer for matched right values
 * @return Number of join results
 */
uint32_t cns_sparql_join_aot(CNSSparqlEngineAOT* engine,
                             uint32_t* left_keys, uint32_t* left_values, uint32_t left_count,
                             uint32_t* right_keys, uint32_t* right_values, uint32_t right_count,
                             uint32_t* result_left, uint32_t* result_right);

/**
 * Execute filter query with SIMD
 * Filters values greater than threshold
 * @param engine Query engine
 * @param values Array of float values to filter
 * @param count Number of values
 * @param threshold Filter threshold
 * @param indices Original indices of values
 * @param results Output buffer for indices passing filter
 * @return Number of values passing filter
 */
uint32_t cns_sparql_filter_gt_aot(CNSSparqlEngineAOT* engine,
                                  float* values, uint32_t count,
                                  float threshold,
                                  uint32_t* indices,
                                  uint32_t* results);

/**
 * Project results
 * Copies selected columns to output based on indices
 * @param engine Query engine
 * @param columns Source column arrays
 * @param num_columns Number of columns
 * @param indices Indices to project
 * @param count Number of indices
 * @param output Output arrays for projected data
 */
void cns_sparql_project_aot(CNSSparqlEngineAOT* engine,
                           void** columns, uint32_t num_columns,
                           uint32_t* indices, uint32_t count,
                           void** output);

/**
 * Execute complex customer query example
 * Demonstrates combining multiple AOT kernels
 * @param engine Query engine
 * @param min_value Minimum lifetime value threshold
 * @param results Output buffer for customer results
 * @param max_results Maximum number of results to return
 * @return Number of results found
 */
uint32_t cns_sparql_execute_customer_query_aot(CNSSparqlEngineAOT* engine,
                                               float min_value,
                                               CustomerResult* results,
                                               uint32_t max_results);

/**
 * Benchmark AOT kernels
 * Runs performance tests and prints results
 * @param engine Query engine to benchmark
 */
void cns_sparql_benchmark_aot_kernels(CNSSparqlEngineAOT* engine);

#ifdef __cplusplus
}
#endif

#endif // CNS_SPARQL_AOT_H