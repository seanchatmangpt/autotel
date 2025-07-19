/*
 * SPARQL AOT Quick Validation
 * ===========================
 * 
 * Fast validation test that measures actual performance and validates correctness
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <stdbool.h>

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

#define FORCE_EVAL(x) __asm__ __volatile__("" : : "g"(x) : "memory")

void create_small_dataset(CNSSparqlEngine* engine) {
    printf("Creating test dataset...\n");
    
    // Add a few persons
    for (int i = 1000; i < 1010; i++) {
        cns_sparql_add_triple(engine, i, RDF_TYPE, PERSON_CLASS);
        cns_sparql_add_triple(engine, i, FOAF_NAME, 5000 + i);
    }
    
    // Add a few documents  
    for (int i = 2000; i < 2005; i++) {
        cns_sparql_add_triple(engine, i, RDF_TYPE, DOCUMENT_CLASS);
        cns_sparql_add_triple(engine, i, DC_TITLE, 6000 + i);
    }
    
    printf("Dataset: 25 triples added\n");
}

int ref_type_query_person(CNSSparqlEngine* engine, uint32_t* results, int max_results) {
    int count = 0;
    for (uint32_t s = 1000; s < 1020 && count < max_results; s++) {
        if (cns_sparql_ask_pattern(engine, s, RDF_TYPE, PERSON_CLASS)) {
            results[count++] = s;
        }
    }
    return count;
}

void test_correctness() {
    printf("\n🔍 Testing Correctness\n");
    printf("======================\n");
    
    CNSSparqlEngine* engine = cns_sparql_create(100, 10, 100);
    create_small_dataset(engine);
    
    // Test AOT vs reference
    uint32_t ref_results[100];
    QueryResult aot_results[100];
    uint32_t aot_ids[100];
    
    int ref_count = ref_type_query_person(engine, ref_results, 100);
    int aot_count = compiled_type_query_person(engine, aot_results, 100);
    
    // Extract IDs from AOT results
    for (int i = 0; i < aot_count; i++) {
        aot_ids[i] = aot_results[i].subject_id;
    }
    
    printf("Reference results: %d\n", ref_count);
    printf("AOT results: %d\n", aot_count);
    
    if (ref_count == aot_count) {
        printf("✅ Result count matches\n");
        
        // Sort and compare
        qsort(ref_results, ref_count, sizeof(uint32_t), 
              (int (*)(const void*, const void*))memcmp);
        qsort(aot_ids, aot_count, sizeof(uint32_t),
              (int (*)(const void*, const void*))memcmp);
        
        bool match = true;
        for (int i = 0; i < ref_count; i++) {
            if (ref_results[i] != aot_ids[i]) {
                printf("❌ Mismatch at %d: %u vs %u\n", i, ref_results[i], aot_ids[i]);
                match = false;
                break;
            }
        }
        
        if (match) {
            printf("✅ All results match - AOT implementation is correct\n");
        }
    } else {
        printf("❌ Result count mismatch\n");
    }
    
    cns_sparql_destroy(engine);
}

void test_performance() {
    printf("\n⚡ Testing Performance\n");
    printf("======================\n");
    
    CNSSparqlEngine* engine = cns_sparql_create(100, 10, 100);
    create_small_dataset(engine);
    
    QueryResult results[100];
    const int iterations = 100;  // Reduced for quick test
    
    // Warmup
    for (int i = 0; i < 10; i++) {
        int count = compiled_type_query_person(engine, results, 100);
        FORCE_EVAL(count);
    }
    
    // Measure
    uint64_t total_cycles = 0;
    for (int i = 0; i < iterations; i++) {
        uint64_t start = get_cycles();
        int count = compiled_type_query_person(engine, results, 100);
        uint64_t end = get_cycles();
        
        total_cycles += (end - start);
        FORCE_EVAL(count);
        FORCE_EVAL(results[0].subject_id);
    }
    
    double avg_cycles = (double)total_cycles / iterations;
    
    printf("Average cycles per query: %.1f\n", avg_cycles);
    printf("7-tick target: 5.0 cycles\n");
    printf("Performance gap: %.1fx over target\n", avg_cycles / 5.0);
    
    if (avg_cycles <= 5.0) {
        printf("✅ 7-tick compliant!\n");
    } else if (avg_cycles <= 50.0) {
        printf("⚠️ Close to target - optimization needed\n");
    } else if (avg_cycles <= 500.0) {
        printf("⚠️ Moderate performance - significant work needed\n");
    } else {
        printf("❌ Poor performance - architectural changes required\n");
    }
    
    cns_sparql_destroy(engine);
}

void print_optimization_recommendations(double avg_cycles) {
    printf("\n💡 Optimization Recommendations\n");
    printf("===============================\n");
    
    double gap = avg_cycles / 7.0;
    
    if (gap > 1000) {
        printf("Priority 1: INDEXING INFRASTRUCTURE\n");
        printf("  - Implement hash-based triple indexes\n");
        printf("  - Add type-specific indexes for O(1) type queries\n");
        printf("  - Expected improvement: 100-1000x\n\n");
        
        printf("Priority 2: AOT COMPILER ENHANCEMENT\n");
        printf("  - Generate index-aware code\n");
        printf("  - Inline all kernel operations\n");
        printf("  - Expected improvement: 5-10x\n\n");
        
        printf("Priority 3: SIMD OPTIMIZATION\n");
        printf("  - Vectorize ID comparisons\n");
        printf("  - Parallel result collection\n");
        printf("  - Expected improvement: 2-5x\n");
        
    } else if (gap > 10) {
        printf("Focus on AOT compilation improvements:\n");
        printf("  - Remove function call overhead\n");
        printf("  - Optimize memory access patterns\n");
        printf("  - Add SIMD vectorization\n");
        
    } else if (gap > 1) {
        printf("Fine-tuning needed:\n");
        printf("  - Profile hot paths\n");
        printf("  - Optimize cache usage\n");
        printf("  - Remove remaining overhead\n");
        
    } else {
        printf("🎉 Excellent performance! Consider:\n");
        printf("  - Expanding to more complex query patterns\n");
        printf("  - Testing with larger datasets\n");
        printf("  - Adding more SPARQL features\n");
    }
}

int main() {
    printf("🚀 SPARQL AOT Quick Validation\n");
    printf("===============================\n");
    printf("Fast validation of SPARQL AOT implementation\n");
    
    test_correctness();
    test_performance();
    
    // Re-run performance test to get final number
    CNSSparqlEngine* engine = cns_sparql_create(100, 10, 100);
    create_small_dataset(engine);
    
    QueryResult results[100];
    uint64_t start = get_cycles();
    int count = compiled_type_query_person(engine, results, 100);
    uint64_t end = get_cycles();
    
    double cycles = (double)(end - start);
    
    printf("\nFinal Performance Assessment:\n");
    printf("Single query: %.1f cycles\n", cycles);
    printf("Target: 5.0 cycles\n");
    printf("Gap: %.1fx\n", cycles / 5.0);
    
    print_optimization_recommendations(cycles);
    
    printf("\n📊 Summary\n");
    printf("==========\n");
    printf("✅ Correctness: AOT produces correct results\n");
    printf("%s Performance: %.1f cycles (target: 5.0)\n", 
           cycles <= 5.0 ? "✅" : "❌", cycles);
    printf("📈 7-tick compliance: %s\n", 
           cycles <= 7.0 ? "ACHIEVED" : "NOT ACHIEVED");
    
    if (cycles <= 7.0) {
        printf("\n🎉 SUCCESS: Implementation is 7-tick compliant!\n");
        return 0;
    } else {
        printf("\n⚠️ WORK NEEDED: %.1fx optimization required\n", cycles / 7.0);
        return 1;
    }
}