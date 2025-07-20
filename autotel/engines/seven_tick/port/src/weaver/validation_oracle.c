/**
 * @file validation_oracle.c
 * @brief Gatekeeper Oracle & Validation - Master Validation Loop
 * 
 * Principle: The ultimate measure of success is the silent, unchanging verdict of the Gatekeeper.
 * Purpose: Implement the master validation loop that compares gatekeeper reports to prove invariance.
 */

#include "cns/cns_weaver.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>

// ============================================================================
// GATEKEEPER ORACLE CORE
// ============================================================================

// Calculate deviation score between two gatekeeper reports
uint64_t cns_weaver_calculate_deviation(const gatekeeper_metrics_t* a,
                                       const gatekeeper_metrics_t* b) {
    assert(a != NULL);
    assert(b != NULL);
    
    uint64_t deviation = 0;
    
    // Calculate absolute differences for each metric
    deviation += (a->total_ticks > b->total_ticks) ? 
                 (a->total_ticks - b->total_ticks) : 
                 (b->total_ticks - a->total_ticks);
    
    deviation += (a->l1_cache_hits > b->l1_cache_hits) ? 
                 (a->l1_cache_hits - b->l1_cache_hits) : 
                 (b->l1_cache_hits - a->l1_cache_hits);
    
    deviation += (a->l1_cache_misses > b->l1_cache_misses) ? 
                 (a->l1_cache_misses - b->l1_cache_misses) : 
                 (b->l1_cache_misses - a->l1_cache_misses);
    
    deviation += (a->memory_allocated > b->memory_allocated) ? 
                 (a->memory_allocated - b->memory_allocated) : 
                 (b->memory_allocated - a->memory_allocated);
    
    deviation += (a->operations_completed > b->operations_completed) ? 
                 (a->operations_completed - b->operations_completed) : 
                 (b->operations_completed - a->operations_completed);
    
    deviation += (a->trinity_hash > b->trinity_hash) ? 
                 (a->trinity_hash - b->trinity_hash) : 
                 (b->trinity_hash - a->trinity_hash);
    
    deviation += (a->cognitive_cycle_count > b->cognitive_cycle_count) ? 
                 (a->cognitive_cycle_count - b->cognitive_cycle_count) : 
                 (b->cognitive_cycle_count - a->cognitive_cycle_count);
    
    deviation += (a->memory_quanta_used > b->memory_quanta_used) ? 
                 (a->memory_quanta_used - b->memory_quanta_used) : 
                 (b->memory_quanta_used - a->memory_quanta_used);
    
    deviation += (a->physics_operations > b->physics_operations) ? 
                 (a->physics_operations - b->physics_operations) : 
                 (b->physics_operations - a->physics_operations);
    
    deviation += (a->shacl_validations > b->shacl_validations) ? 
                 (a->shacl_validations - b->shacl_validations) : 
                 (b->shacl_validations - a->shacl_validations);
    
    deviation += (a->sparql_queries > b->sparql_queries) ? 
                 (a->sparql_queries - b->sparql_queries) : 
                 (b->sparql_queries - a->sparql_queries);
    
    deviation += (a->graph_operations > b->graph_operations) ? 
                 (a->graph_operations - b->graph_operations) : 
                 (b->graph_operations - a->graph_operations);
    
    deviation += (a->entropy_score > b->entropy_score) ? 
                 (a->entropy_score - b->entropy_score) : 
                 (b->entropy_score - a->entropy_score);
    
    deviation += (a->dark_patterns_detected > b->dark_patterns_detected) ? 
                 (a->dark_patterns_detected - b->dark_patterns_detected) : 
                 (b->dark_patterns_detected - a->dark_patterns_detected);
    
    deviation += (a->evolution_counter > b->evolution_counter) ? 
                 (a->evolution_counter - b->evolution_counter) : 
                 (b->evolution_counter - a->evolution_counter);
    
    // Checksum difference is critical - any difference indicates non-determinism
    if (a->checksum != b->checksum) {
        deviation += 0xFFFFFFFFFFFFFFFFULL; // Maximum penalty for checksum mismatch
    }
    
    return deviation;
}

// Validate invariance between two gatekeeper reports
bool cns_weaver_validate_invariance(const gatekeeper_metrics_t* canonical,
                                   const gatekeeper_metrics_t* permuted) {
    assert(canonical != NULL);
    assert(permuted != NULL);
    
    // Perfect invariance requires byte-for-byte match
    if (memcmp(canonical, permuted, sizeof(gatekeeper_metrics_t)) == 0) {
        return true;
    }
    
    // Calculate deviation score for detailed analysis
    uint64_t deviation = cns_weaver_calculate_deviation(canonical, permuted);
    
    // Any deviation is a failure in the Fifth Epoch
    if (deviation > 0) {
        printf("INVARIANCE VIOLATION DETECTED!\n");
        printf("Deviation score: %llu\n", deviation);
        printf("Checksum match: %s\n", canonical->checksum == permuted->checksum ? "YES" : "NO");
        return false;
    }
    
    return true;
}

// ============================================================================
// MASTER VALIDATION LOOP
// ============================================================================

// Run canonical sequence and capture gatekeeper report
int oracle_run_canonical_sequence(const cns_weave_op_t* sequence,
                                 uint32_t op_count,
                                 gatekeeper_metrics_t* canonical_report) {
    assert(sequence != NULL);
    assert(canonical_report != NULL);
    
    // Allocate telemetry buffer
    probe_telemetry_t* telemetry = malloc(op_count * sizeof(probe_telemetry_t));
    if (!telemetry) {
        return CNS_WEAVER_ERROR_MEMORY;
    }
    
    // Execute canonical sequence
    int result = probe_execute_sequence(sequence, op_count, telemetry, NULL);
    if (result != CNS_WEAVER_SUCCESS) {
        free(telemetry);
        return result;
    }
    
    // Collect gatekeeper metrics
    result = probe_collect_gatekeeper_metrics(telemetry, op_count, canonical_report);
    
    free(telemetry);
    return result;
}

// Run permuted sequence and capture gatekeeper report
int oracle_run_permuted_sequence(const cns_weave_op_t* sequence,
                                uint32_t op_count,
                                cns_permutation_config_t* config,
                                gatekeeper_metrics_t* permuted_report) {
    assert(sequence != NULL);
    assert(config != NULL);
    assert(permuted_report != NULL);
    
    // Allocate buffers for permutation
    cns_weave_op_t* permuted_sequence = malloc(op_count * sizeof(cns_weave_op_t));
    uint64_t* temporal_delays = malloc(op_count * sizeof(uint64_t));
    probe_telemetry_t* telemetry = malloc(op_count * sizeof(probe_telemetry_t));
    
    if (!permuted_sequence || !temporal_delays || !telemetry) {
        free(permuted_sequence);
        free(temporal_delays);
        free(telemetry);
        return CNS_WEAVER_ERROR_MEMORY;
    }
    
    // Apply permutation
    int result = permutation_apply_composite_permutation(sequence, op_count, 
                                                        config, permuted_sequence, 
                                                        temporal_delays);
    if (result != CNS_WEAVER_SUCCESS) {
        free(permuted_sequence);
        free(temporal_delays);
        free(telemetry);
        return result;
    }
    
    // Execute permuted sequence
    result = probe_execute_sequence(permuted_sequence, op_count, telemetry, temporal_delays);
    if (result != CNS_WEAVER_SUCCESS) {
        free(permuted_sequence);
        free(temporal_delays);
        free(telemetry);
        return result;
    }
    
    // Collect gatekeeper metrics
    result = probe_collect_gatekeeper_metrics(telemetry, op_count, permuted_report);
    
    free(permuted_sequence);
    free(temporal_delays);
    free(telemetry);
    return result;
}

// Run a single permutation test
int cns_weaver_run_permutation(cns_weave_t* weave, 
                              cns_permutation_config_t* config,
                              cns_permutation_result_t* result) {
    assert(weave != NULL);
    assert(config != NULL);
    assert(result != NULL);
    
    // Initialize result
    memset(result, 0, sizeof(cns_permutation_result_t));
    result->config = *config;
    
    // Record start time
    uint64_t start_time = probe_get_cycles();
    
    // Run canonical sequence
    int canonical_result = oracle_run_canonical_sequence(weave->canonical_sequence,
                                                        weave->op_count,
                                                        &result->canonical_report);
    if (canonical_result != CNS_WEAVER_SUCCESS) {
        return canonical_result;
    }
    
    // Run permuted sequence
    int permuted_result = oracle_run_permuted_sequence(weave->canonical_sequence,
                                                      weave->op_count,
                                                      config,
                                                      &result->permuted_report);
    if (permuted_result != CNS_WEAVER_SUCCESS) {
        return permuted_result;
    }
    
    // Record end time
    uint64_t end_time = probe_get_cycles();
    result->execution_time = end_time - start_time;
    
    // Validate invariance
    result->is_invariant = cns_weaver_validate_invariance(&result->canonical_report,
                                                         &result->permuted_report);
    
    // Calculate deviation score
    result->deviation_score = cns_weaver_calculate_deviation(&result->canonical_report,
                                                            &result->permuted_report);
    
    return CNS_WEAVER_SUCCESS;
}

// ============================================================================
// BATCH VALIDATION
// ============================================================================

// Run multiple permutations and collect results
int oracle_run_batch_validation(cns_weave_t* weave,
                               cns_permutation_config_t* configs,
                               uint32_t config_count,
                               cns_permutation_result_t* results) {
    assert(weave != NULL);
    assert(configs != NULL);
    assert(results != NULL);
    
    uint32_t successful_permutations = 0;
    uint32_t failed_permutations = 0;
    
    printf("Running batch validation: %u permutations\n", config_count);
    
    for (uint32_t i = 0; i < config_count; i++) {
        printf("Permutation %u/%u: ", i + 1, config_count);
        
        int result = cns_weaver_run_permutation(weave, &configs[i], &results[i]);
        if (result != CNS_WEAVER_SUCCESS) {
            printf("FAILED (execution error %d)\n", result);
            failed_permutations++;
            continue;
        }
        
        if (results[i].is_invariant) {
            printf("PASS (invariant)\n");
            successful_permutations++;
        } else {
            printf("FAIL (deviation score: %llu)\n", results[i].deviation_score);
            failed_permutations++;
        }
    }
    
    printf("\nBatch validation complete:\n");
    printf("  Successful: %u\n", successful_permutations);
    printf("  Failed: %u\n", failed_permutations);
    printf("  Success rate: %.2f%%\n", 
           config_count > 0 ? (double)successful_permutations / config_count * 100.0 : 0.0);
    
    return CNS_WEAVER_SUCCESS;
}

// ============================================================================
// DETAILED ANALYSIS
// ============================================================================

// Print detailed comparison of two gatekeeper reports
void oracle_print_comparison(const gatekeeper_metrics_t* canonical,
                            const gatekeeper_metrics_t* permuted) {
    assert(canonical != NULL);
    assert(permuted != NULL);
    
    printf("=== Gatekeeper Report Comparison ===\n");
    printf("Metric                    Canonical    Permuted     Difference\n");
    printf("------------------------- ------------ ------------ ------------\n");
    
    printf("Total ticks               %12llu %12llu %12lld\n",
           canonical->total_ticks, permuted->total_ticks,
           (int64_t)canonical->total_ticks - (int64_t)permuted->total_ticks);
    
    printf("L1 cache hits             %12llu %12llu %12lld\n",
           canonical->l1_cache_hits, permuted->l1_cache_hits,
           (int64_t)canonical->l1_cache_hits - (int64_t)permuted->l1_cache_hits);
    
    printf("L1 cache misses           %12llu %12llu %12lld\n",
           canonical->l1_cache_misses, permuted->l1_cache_misses,
           (int64_t)canonical->l1_cache_misses - (int64_t)permuted->l1_cache_misses);
    
    printf("Memory allocated          %12llu %12llu %12lld\n",
           canonical->memory_allocated, permuted->memory_allocated,
           (int64_t)canonical->memory_allocated - (int64_t)permuted->memory_allocated);
    
    printf("Operations completed       %12llu %12llu %12lld\n",
           canonical->operations_completed, permuted->operations_completed,
           (int64_t)canonical->operations_completed - (int64_t)permuted->operations_completed);
    
    printf("Trinity hash              0x%08llX    0x%08llX    %s\n",
           canonical->trinity_hash, permuted->trinity_hash,
           canonical->trinity_hash == permuted->trinity_hash ? "MATCH" : "DIFFERENT");
    
    printf("Cognitive cycles          %12llu %12llu %12lld\n",
           canonical->cognitive_cycle_count, permuted->cognitive_cycle_count,
           (int64_t)canonical->cognitive_cycle_count - (int64_t)permuted->cognitive_cycle_count);
    
    printf("Memory quanta             %12llu %12llu %12lld\n",
           canonical->memory_quanta_used, permuted->memory_quanta_used,
           (int64_t)canonical->memory_quanta_used - (int64_t)permuted->memory_quanta_used);
    
    printf("Physics operations        %12llu %12llu %12lld\n",
           canonical->physics_operations, permuted->physics_operations,
           (int64_t)canonical->physics_operations - (int64_t)permuted->physics_operations);
    
    printf("SHACL validations         %12llu %12llu %12lld\n",
           canonical->shacl_validations, permuted->shacl_validations,
           (int64_t)canonical->shacl_validations - (int64_t)permuted->shacl_validations);
    
    printf("SPARQL queries            %12llu %12llu %12lld\n",
           canonical->sparql_queries, permuted->sparql_queries,
           (int64_t)canonical->sparql_queries - (int64_t)permuted->sparql_queries);
    
    printf("Graph operations          %12llu %12llu %12lld\n",
           canonical->graph_operations, permuted->graph_operations,
           (int64_t)canonical->graph_operations - (int64_t)permuted->graph_operations);
    
    printf("Entropy score             %12llu %12llu %12lld\n",
           canonical->entropy_score, permuted->entropy_score,
           (int64_t)canonical->entropy_score - (int64_t)permuted->entropy_score);
    
    printf("Dark patterns             %12llu %12llu %12lld\n",
           canonical->dark_patterns_detected, permuted->dark_patterns_detected,
           (int64_t)canonical->dark_patterns_detected - (int64_t)permuted->dark_patterns_detected);
    
    printf("Evolution counter         %12llu %12llu %12lld\n",
           canonical->evolution_counter, permuted->evolution_counter,
           (int64_t)canonical->evolution_counter - (int64_t)permuted->evolution_counter);
    
    printf("Checksum                  0x%016llX 0x%016llX %s\n",
           canonical->checksum, permuted->checksum,
           canonical->checksum == permuted->checksum ? "MATCH" : "CRITICAL FAILURE");
    
    printf("=== End Comparison ===\n");
}

// Analyze permutation results for patterns
int oracle_analyze_results(const cns_permutation_result_t* results,
                          uint32_t result_count,
                          cns_hypothesis_t* hypothesis) {
    assert(results != NULL);
    assert(hypothesis != NULL);
    
    // Find the permutation with the highest deviation
    uint64_t max_deviation = 0;
    uint32_t max_deviation_index = 0;
    
    for (uint32_t i = 0; i < result_count; i++) {
        if (results[i].deviation_score > max_deviation) {
            max_deviation = results[i].deviation_score;
            max_deviation_index = i;
        }
    }
    
    if (max_deviation == 0) {
        // No deviations found - system is invariant
        hypothesis->description = "System appears to be invariant under all tested permutations";
        hypothesis->confidence_score = 100;
        hypothesis->evidence_count = result_count;
        return CNS_WEAVER_SUCCESS;
    }
    
    // Analyze the worst-case permutation
    const cns_permutation_result_t* worst = &results[max_deviation_index];
    
    // Generate hypothesis based on permutation type and deviation
    if (worst->config.type & PERM_TEMPORAL) {
        hypothesis->description = "System shows sensitivity to temporal permutations";
        hypothesis->permutation_type = PERM_TEMPORAL;
    } else if (worst->config.type & PERM_LOGICAL) {
        hypothesis->description = "System shows sensitivity to logical reordering";
        hypothesis->permutation_type = PERM_LOGICAL;
    } else if (worst->config.type & PERM_SPATIAL) {
        hypothesis->description = "System shows sensitivity to memory layout changes";
        hypothesis->permutation_type = PERM_SPATIAL;
    } else {
        hypothesis->description = "System shows sensitivity to composite permutations";
        hypothesis->permutation_type = worst->config.type;
    }
    
    hypothesis->confidence_score = (worst->deviation_score * 100) / 0xFFFFFFFFFFFFFFFFULL;
    hypothesis->evidence_count = 1;
    
    return CNS_WEAVER_SUCCESS;
}

// ============================================================================
// INITIALIZATION
// ============================================================================

// Initialize the validation oracle
int oracle_init(void) {
    printf("Gatekeeper Oracle initialized\n");
    return CNS_WEAVER_SUCCESS;
}

// Clean up oracle resources
void oracle_cleanup(void) {
    printf("Gatekeeper Oracle cleaned up\n");
} 