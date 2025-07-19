/*
 * SPARQL AOT Validation Suite
 * ===========================
 * 
 * Comprehensive validation of SPARQL AOT implementation:
 * 1. Correctness validation - AOT results match interpreted results
 * 2. Performance validation - Each pattern achieves cycle targets
 * 3. Robustness validation - Handle edge cases and large datasets
 * 4. 7-tick compliance validation - 80% of patterns must be compliant
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <math.h>
#include <assert.h>

// Include both interpreted and AOT implementations
#include "cns/engines/sparql.h"
#include "sparql_queries.h"

// Performance measurement
static inline uint64_t get_cycles(void) {
#if defined(__x86_64__) || defined(__i386__)
    uint32_t lo, hi;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
#elif defined(__aarch64__)
    uint64_t val;
    __asm__ __volatile__("mrs %0, cntvct_el0" : "=r" (val));
    return val;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 2400000000ULL + (uint64_t)ts.tv_nsec * 2.4;
#endif
}

// Prevent compiler optimizations
#define FORCE_EVAL(x) __asm__ __volatile__("" : : "g"(x) : "memory")

// Test configuration
#define MAX_RESULTS 10000
#define TEST_ITERATIONS 1000
#define WARMUP_ITERATIONS 100
#define DATASET_SMALL 1000
#define DATASET_MEDIUM 10000  
#define DATASET_LARGE 100000

// Validation results
typedef struct {
    const char* test_name;
    bool correctness_passed;
    bool performance_passed;
    bool robustness_passed;
    double avg_cycles;
    double target_cycles;
    int result_count;
    char failure_reason[256];
} ValidationResult;

typedef struct {
    CNSSparqlEngine* engine;
    size_t dataset_size;
    uint32_t* expected_results;
    uint32_t* actual_results;
    int expected_count;
    int actual_count;
} ValidationContext;

// Create test dataset with known structure
void create_validation_dataset(CNSSparqlEngine* engine, size_t size) {
    printf("  Creating validation dataset (size=%zu)...\n", size);
    
    // Clear existing data
    cns_sparql_clear(engine);
    
    // Add persons (IDs 1000-1999 or scaled)
    size_t person_count = size / 4;
    for (size_t i = 0; i < person_count; i++) {
        uint32_t person_id = 1000 + i;
        cns_sparql_add_pattern(engine, person_id, RDF_TYPE, PERSON_CLASS);
        cns_sparql_add_pattern(engine, person_id, FOAF_NAME, 5000 + i);
        
        // Add social connections (sparse graph)
        if (i % 7 == 0 && i < person_count - 10) {
            cns_sparql_add_pattern(engine, person_id, FOAF_KNOWS, person_id + (i % 10) + 1);
        }
    }
    
    // Add documents (IDs 2000-2999 or scaled)
    size_t doc_count = size / 4;
    for (size_t i = 0; i < doc_count; i++) {
        uint32_t doc_id = 2000 + i;
        cns_sparql_add_pattern(engine, doc_id, RDF_TYPE, DOCUMENT_CLASS);
        cns_sparql_add_pattern(engine, doc_id, DC_TITLE, 6000 + i);
        cns_sparql_add_pattern(engine, doc_id, DC_CREATOR, 1000 + (i % person_count));
    }
    
    // Add organizations
    size_t org_count = size / 8;
    for (size_t i = 0; i < org_count; i++) {
        uint32_t org_id = 3000 + i;
        cns_sparql_add_pattern(engine, org_id, RDF_TYPE, ORGANIZATION_CLASS);
        cns_sparql_add_pattern(engine, org_id, RDFS_LABEL, 7000 + i);
    }
    
    // Add customers (subset of persons)
    for (size_t i = 0; i < person_count / 5; i++) {
        uint32_t person_id = 1000 + (i * 5);
        cns_sparql_add_pattern(engine, person_id, RDF_TYPE, CUSTOMER_CLASS);
    }
    
    printf("  Dataset created: %zu triples\n", cns_sparql_triple_count(engine));
}

// Validate correctness by comparing AOT vs interpreted results
bool validate_correctness(ValidationContext* ctx, const char* query_name,
                         int (*aot_func)(CNSSparqlEngine*, QueryResult*, int),
                         int (*ref_func)(CNSSparqlEngine*, uint32_t*, int)) {
    
    // Run reference (interpreted) implementation
    ctx->expected_count = ref_func(ctx->engine, ctx->expected_results, MAX_RESULTS);
    
    // Run AOT implementation
    QueryResult aot_results[MAX_RESULTS];
    ctx->actual_count = aot_func(ctx->engine, aot_results, MAX_RESULTS);
    
    // Extract subject IDs from QueryResult for comparison
    for (int i = 0; i < ctx->actual_count; i++) {
        ctx->actual_results[i] = aot_results[i].subject_id;
    }
    
    // Check count matches
    if (ctx->actual_count != ctx->expected_count) {
        snprintf(ctx->failure_reason, sizeof(ctx->failure_reason),
                "Result count mismatch: expected %d, got %d",
                ctx->expected_count, ctx->actual_count);
        return false;
    }
    
    // Sort results for comparison (order may differ)
    qsort(ctx->expected_results, ctx->expected_count, sizeof(uint32_t),
          (int (*)(const void*, const void*))strcmp);
    qsort(ctx->actual_results, ctx->actual_count, sizeof(uint32_t),
          (int (*)(const void*, const void*))strcmp);
    
    // Compare sorted results
    for (int i = 0; i < ctx->expected_count; i++) {
        if (ctx->expected_results[i] != ctx->actual_results[i]) {
            snprintf(ctx->failure_reason, sizeof(ctx->failure_reason),
                    "Result mismatch at index %d: expected %u, got %u",
                    i, ctx->expected_results[i], ctx->actual_results[i]);
            return false;
        }
    }
    
    return true;
}

// Validate performance meets cycle targets
bool validate_performance(ValidationContext* ctx, const char* query_name,
                         int (*aot_func)(CNSSparqlEngine*, QueryResult*, int),
                         double target_cycles, double* avg_cycles) {
    
    QueryResult results[MAX_RESULTS];
    uint64_t total_cycles = 0;
    
    // Warmup
    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
        int count = aot_func(ctx->engine, results, MAX_RESULTS);
        FORCE_EVAL(count);
        FORCE_EVAL(results[0].subject_id);
    }
    
    // Measure
    for (int i = 0; i < TEST_ITERATIONS; i++) {
        uint64_t start = get_cycles();
        int count = aot_func(ctx->engine, results, MAX_RESULTS);
        uint64_t end = get_cycles();
        
        total_cycles += (end - start);
        FORCE_EVAL(count);
        FORCE_EVAL(results[0].subject_id);
    }
    
    *avg_cycles = (double)total_cycles / TEST_ITERATIONS;
    
    // Check if meets target (with 20% margin for measurement variance)
    if (*avg_cycles > target_cycles * 1.2) {
        snprintf(ctx->failure_reason, sizeof(ctx->failure_reason),
                "Performance target missed: %.1f cycles (target: %.1f)",
                *avg_cycles, target_cycles);
        return false;
    }
    
    return true;
}

// Validate robustness with edge cases
bool validate_robustness(ValidationContext* ctx, const char* query_name,
                        int (*aot_func)(CNSSparqlEngine*, QueryResult*, int)) {
    
    // Test 1: Empty dataset
    cns_sparql_clear(ctx->engine);
    QueryResult results[10];
    int count = aot_func(ctx->engine, results, 10);
    if (count != 0) {
        snprintf(ctx->failure_reason, sizeof(ctx->failure_reason),
                "Failed empty dataset test: returned %d results", count);
        return false;
    }
    
    // Test 2: Single result
    cns_sparql_add_pattern(ctx->engine, 1, RDF_TYPE, PERSON_CLASS);
    count = aot_func(ctx->engine, results, 10);
    if (count > 1) {
        snprintf(ctx->failure_reason, sizeof(ctx->failure_reason),
                "Failed single result test: returned %d results", count);
        return false;
    }
    
    // Test 3: Result limit enforcement
    create_validation_dataset(ctx->engine, 1000);
    count = aot_func(ctx->engine, results, 5);
    if (count > 5) {
        snprintf(ctx->failure_reason, sizeof(ctx->failure_reason),
                "Failed result limit test: returned %d results (limit 5)", count);
        return false;
    }
    
    // Test 4: Large dataset handling
    create_validation_dataset(ctx->engine, ctx->dataset_size);
    count = aot_func(ctx->engine, results, 10);
    if (count < 0) {
        snprintf(ctx->failure_reason, sizeof(ctx->failure_reason),
                "Failed large dataset test: error code %d", count);
        return false;
    }
    
    return true;
}

// Reference implementations for comparison
int ref_type_query_person(CNSSparqlEngine* engine, uint32_t* results, int max_results) {
    int count = 0;
    for (uint32_t s = 0; s < 10000 && count < max_results; s++) {
        if (cns_sparql_ask_pattern(engine, s, RDF_TYPE, PERSON_CLASS)) {
            results[count++] = s;
        }
    }
    return count;
}

int ref_type_query_document(CNSSparqlEngine* engine, uint32_t* results, int max_results) {
    int count = 0;
    for (uint32_t s = 0; s < 10000 && count < max_results; s++) {
        if (cns_sparql_ask_pattern(engine, s, RDF_TYPE, DOCUMENT_CLASS)) {
            results[count++] = s;
        }
    }
    return count;
}

int ref_predicate_scan_foaf_name(CNSSparqlEngine* engine, uint32_t* results, int max_results) {
    int count = 0;
    for (uint32_t s = 0; s < 10000 && count < max_results; s++) {
        for (uint32_t o = 0; o < 10000; o++) {
            if (cns_sparql_ask_pattern(engine, s, FOAF_NAME, o)) {
                results[count++] = s;
                if (count >= max_results) return count;
                break;
            }
        }
    }
    return count;
}

// Validation test suite
typedef struct {
    const char* name;
    int (*aot_func)(CNSSparqlEngine*, QueryResult*, int);
    int (*ref_func)(CNSSparqlEngine*, uint32_t*, int);
    double target_cycles;
} ValidationTest;

ValidationTest validation_tests[] = {
    {
        "type_query_person",
        compiled_type_query_person,
        ref_type_query_person,
        5.0  // Target: ≤5 cycles for type queries
    },
    {
        "type_query_document", 
        compiled_type_query_document,
        ref_type_query_document,
        5.0
    },
    {
        "predicate_scan_foaf_name",
        compiled_predicate_scan_foaf_name,
        ref_predicate_scan_foaf_name,
        7.0  // Target: ≤7 cycles for simple scans
    },
    // Add more tests as AOT functions are implemented
};

void print_validation_report(ValidationResult* results, int count) {
    printf("\n");
    printf("================================================================================\n");
    printf("                         SPARQL AOT VALIDATION REPORT                           \n");
    printf("================================================================================\n\n");
    
    printf("Test Summary\n");
    printf("------------\n");
    
    int correctness_passed = 0;
    int performance_passed = 0;
    int robustness_passed = 0;
    int seven_tick_compliant = 0;
    
    for (int i = 0; i < count; i++) {
        if (results[i].correctness_passed) correctness_passed++;
        if (results[i].performance_passed) performance_passed++;
        if (results[i].robustness_passed) robustness_passed++;
        if (results[i].avg_cycles <= 7.0) seven_tick_compliant++;
    }
    
    printf("Correctness: %d/%d tests passed (%.1f%%)\n",
           correctness_passed, count, 100.0 * correctness_passed / count);
    printf("Performance: %d/%d tests passed (%.1f%%)\n", 
           performance_passed, count, 100.0 * performance_passed / count);
    printf("Robustness:  %d/%d tests passed (%.1f%%)\n",
           robustness_passed, count, 100.0 * robustness_passed / count);
    printf("7-tick compliance: %d/%d patterns (%.1f%%)\n\n",
           seven_tick_compliant, count, 100.0 * seven_tick_compliant / count);
    
    // Detailed results
    printf("Detailed Test Results\n");
    printf("-------------------\n");
    printf("%-30s %10s %10s %10s %12s %8s\n",
           "Query Pattern", "Correct", "Perf", "Robust", "Avg Cycles", "7-Tick");
    printf("%-30s %10s %10s %10s %12s %8s\n",
           "-------------", "-------", "----", "------", "----------", "------");
    
    for (int i = 0; i < count; i++) {
        ValidationResult* r = &results[i];
        printf("%-30s %10s %10s %10s %12.1f %8s\n",
               r->test_name,
               r->correctness_passed ? "✅ PASS" : "❌ FAIL",
               r->performance_passed ? "✅ PASS" : "❌ FAIL", 
               r->robustness_passed ? "✅ PASS" : "❌ FAIL",
               r->avg_cycles,
               r->avg_cycles <= 7.0 ? "✅" : "❌");
        
        if (!r->correctness_passed || !r->performance_passed || !r->robustness_passed) {
            printf("      └─ Failure: %s\n", r->failure_reason);
        }
    }
    
    printf("\nPerformance Analysis\n");
    printf("-------------------\n");
    
    // Calculate statistics
    double total_cycles = 0;
    double min_cycles = 1e9;
    double max_cycles = 0;
    
    for (int i = 0; i < count; i++) {
        total_cycles += results[i].avg_cycles;
        if (results[i].avg_cycles < min_cycles) min_cycles = results[i].avg_cycles;
        if (results[i].avg_cycles > max_cycles) max_cycles = results[i].avg_cycles;
    }
    
    printf("Average cycles across all patterns: %.1f\n", total_cycles / count);
    printf("Best performing pattern: %.1f cycles\n", min_cycles);
    printf("Worst performing pattern: %.1f cycles\n", max_cycles);
    
    // Overall assessment
    printf("\nOverall Assessment\n");
    printf("-----------------\n");
    
    bool all_correct = (correctness_passed == count);
    bool meets_perf = (seven_tick_compliant >= (count * 0.8)); // 80% compliance
    bool all_robust = (robustness_passed == count);
    
    if (all_correct && meets_perf && all_robust) {
        printf("✅ VALIDATION PASSED - AOT implementation is production ready\n");
        printf("   - All queries produce correct results\n");
        printf("   - %.1f%% of patterns achieve 7-tick compliance (target: 80%%)\n",
               100.0 * seven_tick_compliant / count);
        printf("   - All robustness tests passed\n");
    } else {
        printf("❌ VALIDATION FAILED - Issues found:\n");
        if (!all_correct) {
            printf("   - Correctness failures: %d patterns produce incorrect results\n",
                   count - correctness_passed);
        }
        if (!meets_perf) {
            printf("   - Performance issues: Only %.1f%% achieve 7-tick compliance (need 80%%)\n",
                   100.0 * seven_tick_compliant / count);
        }
        if (!all_robust) {
            printf("   - Robustness issues: %d patterns fail edge case tests\n",
                   count - robustness_passed);
        }
    }
    
    printf("\nRecommendations\n");
    printf("---------------\n");
    
    if (!all_correct) {
        printf("1. Fix correctness issues first - results must match reference implementation\n");
    }
    
    if (!meets_perf) {
        printf("2. Optimize performance bottlenecks:\n");
        printf("   - Implement hash-based indexing for O(1) lookups\n");
        printf("   - Use SIMD instructions for parallel operations\n");
        printf("   - Add specialized kernels for common patterns\n");
    }
    
    if (!all_robust) {
        printf("3. Improve edge case handling:\n");
        printf("   - Add bounds checking for all array accesses\n");
        printf("   - Handle empty dataset gracefully\n");
        printf("   - Enforce result limits properly\n");
    }
}

int main(int argc, char* argv[]) {
    printf("🔬 SPARQL AOT Validation Suite\n");
    printf("==============================\n\n");
    
    // Parse command line arguments
    size_t dataset_size = DATASET_MEDIUM;
    if (argc > 1) {
        if (strcmp(argv[1], "--small") == 0) {
            dataset_size = DATASET_SMALL;
        } else if (strcmp(argv[1], "--large") == 0) {
            dataset_size = DATASET_LARGE;
        }
    }
    
    printf("Configuration:\n");
    printf("  Dataset size: %zu\n", dataset_size);
    printf("  Test iterations: %d\n", TEST_ITERATIONS);
    printf("  Target: 80%% of patterns ≤7 cycles\n\n");
    
    // Initialize SPARQL engine
    CNSSparqlEngine* engine = cns_sparql_create(dataset_size * 10);
    if (!engine) {
        printf("❌ Failed to create SPARQL engine\n");
        return 1;
    }
    
    // Allocate result buffers
    uint32_t* expected_results = malloc(MAX_RESULTS * sizeof(uint32_t));
    uint32_t* actual_results = malloc(MAX_RESULTS * sizeof(uint32_t));
    
    ValidationContext ctx = {
        .engine = engine,
        .dataset_size = dataset_size,
        .expected_results = expected_results,
        .actual_results = actual_results
    };
    
    // Run validation tests
    int test_count = sizeof(validation_tests) / sizeof(validation_tests[0]);
    ValidationResult* results = calloc(test_count, sizeof(ValidationResult));
    
    for (int i = 0; i < test_count; i++) {
        ValidationTest* test = &validation_tests[i];
        ValidationResult* result = &results[i];
        
        result->test_name = test->name;
        result->target_cycles = test->target_cycles;
        
        printf("Testing %s...\n", test->name);
        
        // Create fresh dataset for each test
        create_validation_dataset(engine, dataset_size);
        
        // 1. Validate correctness
        printf("  ✓ Validating correctness...");
        result->correctness_passed = validate_correctness(&ctx, test->name,
                                                         test->aot_func, test->ref_func);
        if (result->correctness_passed) {
            printf(" PASS\n");
            result->result_count = ctx.actual_count;
        } else {
            printf(" FAIL\n");
            strncpy(result->failure_reason, ctx.failure_reason, sizeof(result->failure_reason));
            continue; // Skip other tests if correctness fails
        }
        
        // 2. Validate performance
        printf("  ✓ Validating performance...");
        result->performance_passed = validate_performance(&ctx, test->name,
                                                         test->aot_func, test->target_cycles,
                                                         &result->avg_cycles);
        if (result->performance_passed) {
            printf(" PASS (%.1f cycles)\n", result->avg_cycles);
        } else {
            printf(" FAIL\n");
            strncpy(result->failure_reason, ctx.failure_reason, sizeof(result->failure_reason));
        }
        
        // 3. Validate robustness
        printf("  ✓ Validating robustness...");
        result->robustness_passed = validate_robustness(&ctx, test->name, test->aot_func);
        if (result->robustness_passed) {
            printf(" PASS\n");
        } else {
            printf(" FAIL\n");
            strncpy(result->failure_reason, ctx.failure_reason, sizeof(result->failure_reason));
        }
        
        printf("\n");
    }
    
    // Generate validation report
    print_validation_report(results, test_count);
    
    // Cleanup
    free(expected_results);
    free(actual_results);
    free(results);
    cns_sparql_destroy(engine);
    
    // Return success only if validation passed
    int correctness_passed = 0;
    int seven_tick_compliant = 0;
    for (int i = 0; i < test_count; i++) {
        if (results[i].correctness_passed) correctness_passed++;
        if (results[i].avg_cycles <= 7.0) seven_tick_compliant++;
    }
    
    bool validation_passed = (correctness_passed == test_count) &&
                            (seven_tick_compliant >= test_count * 0.8);
    
    return validation_passed ? 0 : 1;
}