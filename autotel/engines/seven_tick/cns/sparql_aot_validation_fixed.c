/*
 * SPARQL AOT Validation Suite - Fixed Implementation
 * ==================================================
 * 
 * This validation suite tests the SPARQL AOT implementation with:
 * 1. Correctness validation against reference implementation
 * 2. Performance measurement with proper 7-tick targeting
 * 3. Recommendations for achieving 7-tick compliance
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <math.h>

// Include SPARQL engine and queries
#include "cns/engines/sparql.h"
#include "sparql_simple_queries.h"

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
#define ITERATIONS 1000
#define WARMUP 100
#define DATASET_SIZE 10000

// Validation result structure
typedef struct {
    const char* query_name;
    bool correctness_passed;
    bool performance_passed;
    double avg_cycles;
    double target_cycles;
    int result_count;
    char failure_reason[256];
} ValidationResult;

// Create test dataset
void create_test_dataset(CNSSparqlEngine* engine) {
    printf("Creating test dataset...\n");
    
    // Add persons (1000-1999)
    for (int i = 1000; i < 2000; i++) {
        cns_sparql_add_triple(engine, i, RDF_TYPE, PERSON_CLASS);
        cns_sparql_add_triple(engine, i, FOAF_NAME, 5000 + i);
        
        // Add social connections
        if (i % 7 == 0 && i < 1990) {
            cns_sparql_add_triple(engine, i, FOAF_KNOWS, i + (i % 10) + 1);
        }
    }
    
    // Add documents (2000-2499)
    for (int i = 2000; i < 2500; i++) {
        cns_sparql_add_triple(engine, i, RDF_TYPE, DOCUMENT_CLASS);
        cns_sparql_add_triple(engine, i, DC_TITLE, 6000 + i);
        cns_sparql_add_triple(engine, i, DC_CREATOR, 1000 + (i % 1000));
    }
    
    // Add organizations (3000-3099)
    for (int i = 3000; i < 3100; i++) {
        cns_sparql_add_triple(engine, i, RDF_TYPE, ORGANIZATION_CLASS);
        cns_sparql_add_triple(engine, i, RDFS_LABEL, 7000 + i);
    }
    
    // Add customers (subset of persons)
    for (int i = 1000; i < 1200; i += 5) {
        cns_sparql_add_triple(engine, i, RDF_TYPE, CUSTOMER_CLASS);
    }
    
    printf("Dataset created: ~%d triples\n", 2000 + 500 + 100 + 40);
}

// Reference implementations for validation
int ref_type_query_person(CNSSparqlEngine* engine, uint32_t* results, int max_results) {
    int count = 0;
    
    // Linear scan through reasonable ID space
    for (uint32_t s = 1000; s < 4000 && count < max_results; s++) {
        if (cns_sparql_ask_pattern(engine, s, RDF_TYPE, PERSON_CLASS)) {
            results[count++] = s;
        }
    }
    
    return count;
}

int ref_type_query_document(CNSSparqlEngine* engine, uint32_t* results, int max_results) {
    int count = 0;
    
    for (uint32_t s = 1000; s < 4000 && count < max_results; s++) {
        if (cns_sparql_ask_pattern(engine, s, RDF_TYPE, DOCUMENT_CLASS)) {
            results[count++] = s;
        }
    }
    
    return count;
}

int ref_predicate_scan_foaf_name(CNSSparqlEngine* engine, uint32_t* results, int max_results) {
    int count = 0;
    
    for (uint32_t s = 1000; s < 4000 && count < max_results; s++) {
        for (uint32_t o = 5000; o < 7000; o++) {
            if (cns_sparql_ask_pattern(engine, s, FOAF_NAME, o)) {
                results[count++] = s;
                if (count >= max_results) return count;
                break;
            }
        }
    }
    
    return count;
}

// Validate correctness
bool validate_correctness(CNSSparqlEngine* engine, 
                         int (*aot_func)(CNSSparqlEngine*, QueryResult*, int),
                         int (*ref_func)(CNSSparqlEngine*, uint32_t*, int),
                         ValidationResult* result) {
    
    uint32_t expected[1000];
    uint32_t actual[1000];
    QueryResult aot_results[1000];
    
    // Get reference results
    int expected_count = ref_func(engine, expected, 1000);
    
    // Get AOT results
    int actual_count = aot_func(engine, aot_results, 1000);
    
    // Extract subject IDs from QueryResult
    for (int i = 0; i < actual_count; i++) {
        actual[i] = aot_results[i].subject_id;
    }
    
    result->result_count = actual_count;
    
    // Check count
    if (actual_count != expected_count) {
        snprintf(result->failure_reason, sizeof(result->failure_reason),
                "Count mismatch: expected %d, got %d", expected_count, actual_count);
        return false;
    }
    
    // Sort for comparison
    qsort(expected, expected_count, sizeof(uint32_t), 
          (int (*)(const void*, const void*))memcmp);
    qsort(actual, actual_count, sizeof(uint32_t),
          (int (*)(const void*, const void*))memcmp);
    
    // Compare results
    for (int i = 0; i < expected_count; i++) {
        if (expected[i] != actual[i]) {
            snprintf(result->failure_reason, sizeof(result->failure_reason),
                    "Result mismatch at %d: expected %u, got %u", 
                    i, expected[i], actual[i]);
            return false;
        }
    }
    
    return true;
}

// Measure performance
bool measure_performance(CNSSparqlEngine* engine,
                        int (*aot_func)(CNSSparqlEngine*, QueryResult*, int),
                        double target_cycles,
                        ValidationResult* result) {
    
    QueryResult results[1000];
    uint64_t total_cycles = 0;
    
    // Warmup
    for (int i = 0; i < WARMUP; i++) {
        int count = aot_func(engine, results, 1000);
        FORCE_EVAL(count);
    }
    
    // Measure
    for (int i = 0; i < ITERATIONS; i++) {
        uint64_t start = get_cycles();
        int count = aot_func(engine, results, 1000);
        uint64_t end = get_cycles();
        
        total_cycles += (end - start);
        FORCE_EVAL(count);
        FORCE_EVAL(results[0].subject_id);
    }
    
    result->avg_cycles = (double)total_cycles / ITERATIONS;
    result->target_cycles = target_cycles;
    
    // Check if meets target (with 20% tolerance)
    if (result->avg_cycles > target_cycles * 1.2) {
        snprintf(result->failure_reason, sizeof(result->failure_reason),
                "Performance target missed: %.1f cycles (target: %.1f)",
                result->avg_cycles, target_cycles);
        return false;
    }
    
    return true;
}

// Test cases
typedef struct {
    const char* name;
    int (*aot_func)(CNSSparqlEngine*, QueryResult*, int);
    int (*ref_func)(CNSSparqlEngine*, uint32_t*, int);
    double target_cycles;
} TestCase;

TestCase test_cases[] = {
    {
        "Type Query (Person)",
        compiled_type_query_person,
        ref_type_query_person,
        5.0  // Target: ≤5 cycles
    },
    {
        "Type Query (Document)",
        compiled_type_query_document,
        ref_type_query_document,
        5.0  // Target: ≤5 cycles
    },
    {
        "Predicate Scan (foaf:name)",
        compiled_predicate_scan_foaf_name,
        ref_predicate_scan_foaf_name,
        7.0  // Target: ≤7 cycles
    }
};

// Print validation report
void print_validation_report(ValidationResult* results, int count) {
    printf("\n");
    printf("================================================================================\n");
    printf("                      SPARQL AOT VALIDATION REPORT                              \n");
    printf("================================================================================\n\n");
    
    printf("Executive Summary\n");
    printf("-----------------\n");
    
    int correct = 0, performant = 0, seven_tick = 0;
    double total_cycles = 0;
    
    for (int i = 0; i < count; i++) {
        if (results[i].correctness_passed) correct++;
        if (results[i].performance_passed) performant++;
        if (results[i].avg_cycles <= 7.0) seven_tick++;
        total_cycles += results[i].avg_cycles;
    }
    
    printf("✓ Correctness: %d/%d tests passed (%.0f%%)\n", 
           correct, count, 100.0 * correct / count);
    printf("✓ Performance: %d/%d tests passed (%.0f%%)\n",
           performant, count, 100.0 * performant / count);
    printf("✓ 7-tick compliance: %d/%d patterns (%.0f%%)\n",
           seven_tick, count, 100.0 * seven_tick / count);
    printf("✓ Average cycles: %.1f\n\n", total_cycles / count);
    
    // Detailed results
    printf("Detailed Results\n");
    printf("----------------\n");
    printf("%-30s %10s %10s %12s %8s %8s\n",
           "Query Pattern", "Correct", "Target", "Actual", "7-Tick", "Status");
    printf("%-30s %10s %10s %12s %8s %8s\n",
           "-------------", "-------", "------", "------", "------", "------");
    
    for (int i = 0; i < count; i++) {
        ValidationResult* r = &results[i];
        printf("%-30s %10s %10.1f %12.1f %8s %8s\n",
               r->query_name,
               r->correctness_passed ? "✅" : "❌",
               r->target_cycles,
               r->avg_cycles,
               r->avg_cycles <= 7.0 ? "✅" : "❌",
               r->performance_passed ? "PASS" : "FAIL");
        
        if (!r->correctness_passed || !r->performance_passed) {
            printf("  └─ %s\n", r->failure_reason);
        }
    }
    
    printf("\nPerformance Analysis\n");
    printf("--------------------\n");
    
    // Current vs Target comparison
    printf("\nCurrent Performance vs 7-Tick Target:\n");
    for (int i = 0; i < count; i++) {
        double gap = results[i].avg_cycles / 7.0;
        printf("  %s: %.1fx over target\n", results[i].query_name, gap);
    }
    
    printf("\nPath to 7-Tick Compliance\n");
    printf("-------------------------\n");
    
    double avg_gap = total_cycles / count / 7.0;
    
    if (avg_gap > 1000) {
        printf("❌ CRITICAL: Performance is %.0fx over target\n\n", avg_gap);
        
        printf("Required Optimizations (in priority order):\n\n");
        
        printf("1. INDEXING INFRASTRUCTURE (1000x improvement)\n");
        printf("   ├─ Implement hash-based triple indexes\n");
        printf("   ├─ Add specialized type indexes\n");
        printf("   ├─ Create predicate-based indexes\n");
        printf("   └─ Use perfect hashing for common patterns\n\n");
        
        printf("2. AOT COMPILATION ENHANCEMENTS (10x improvement)\n");
        printf("   ├─ Generate index-aware code\n");
        printf("   ├─ Inline all function calls\n");
        printf("   ├─ Eliminate bounds checking in hot paths\n");
        printf("   └─ Use compile-time constants\n\n");
        
        printf("3. SIMD VECTORIZATION (10x improvement)\n");
        printf("   ├─ Batch ID comparisons with AVX2/NEON\n");
        printf("   ├─ Parallel result gathering\n");
        printf("   ├─ Vector-based filtering\n");
        printf("   └─ Cache-line aligned data structures\n\n");
        
        printf("4. MEMORY OPTIMIZATION (5x improvement)\n");
        printf("   ├─ Prefetch triple data\n");
        printf("   ├─ Optimize cache usage\n");
        printf("   ├─ Reduce memory bandwidth\n");
        printf("   └─ Use compact data representations\n\n");
        
        printf("Expected Timeline:\n");
        printf("  Phase 1 (Indexing): 2-3 weeks → ~300 cycles\n");
        printf("  Phase 2 (AOT Enhanced): 1-2 weeks → ~30 cycles\n");
        printf("  Phase 3 (SIMD): 1 week → ~3 cycles\n");
        printf("  Phase 4 (Fine-tuning): 1 week → <1 cycle\n");
        printf("  Total: 5-7 weeks to achieve 7-tick compliance\n");
        
    } else if (avg_gap > 10) {
        printf("⚠️ MODERATE: Performance is %.1fx over target\n\n", avg_gap);
        printf("Focus on AOT compilation improvements and SIMD optimization.\n");
    } else if (avg_gap > 1) {
        printf("✅ CLOSE: Performance is %.1fx over target\n\n", avg_gap);
        printf("Minor optimizations needed. Focus on hot path optimization.\n");
    } else {
        printf("🎉 EXCELLENT: 7-tick compliance achieved!\n\n");
        printf("Consider expanding to more complex query patterns.\n");
    }
    
    printf("\nRecommended Next Steps\n");
    printf("----------------------\n");
    
    if (correct < count) {
        printf("1. ❌ Fix correctness issues first - %d queries produce wrong results\n",
               count - correct);
    } else {
        printf("1. ✅ All queries produce correct results\n");
    }
    
    if (avg_gap > 1000) {
        printf("2. 🔧 Implement indexing infrastructure immediately\n");
        printf("3. 📊 Create performance tracking dashboard\n");
        printf("4. 🧪 Set up continuous benchmarking\n");
    } else {
        printf("2. 🚀 Focus on final optimizations\n");
        printf("3. 📈 Expand query pattern coverage\n");
    }
}

int main() {
    printf("🔬 SPARQL AOT Validation Suite\n");
    printf("==============================\n\n");
    
    // Create SPARQL engine with proper parameters
    CNSSparqlEngine* engine = cns_sparql_create(10000, 100, 10000);
    if (!engine) {
        printf("❌ Failed to create SPARQL engine\n");
        return 1;
    }
    
    // Create test dataset
    create_test_dataset(engine);
    
    // Run validation tests
    int test_count = sizeof(test_cases) / sizeof(test_cases[0]);
    ValidationResult* results = calloc(test_count, sizeof(ValidationResult));
    
    for (int i = 0; i < test_count; i++) {
        TestCase* test = &test_cases[i];
        ValidationResult* result = &results[i];
        
        result->query_name = test->name;
        
        printf("Testing %s...\n", test->name);
        
        // Validate correctness
        printf("  Validating correctness...");
        result->correctness_passed = validate_correctness(engine, 
                                                         test->aot_func,
                                                         test->ref_func,
                                                         result);
        if (result->correctness_passed) {
            printf(" ✅ PASS (%d results)\n", result->result_count);
        } else {
            printf(" ❌ FAIL\n");
            printf("    Error: %s\n", result->failure_reason);
            continue;
        }
        
        // Measure performance
        printf("  Measuring performance...");
        result->performance_passed = measure_performance(engine,
                                                        test->aot_func,
                                                        test->target_cycles,
                                                        result);
        if (result->performance_passed) {
            printf(" ✅ PASS (%.1f cycles)\n", result->avg_cycles);
        } else {
            printf(" ❌ FAIL (%.1f cycles)\n", result->avg_cycles);
        }
        
        printf("\n");
    }
    
    // Print validation report
    print_validation_report(results, test_count);
    
    // Cleanup
    free(results);
    cns_sparql_destroy(engine);
    
    return 0;
}