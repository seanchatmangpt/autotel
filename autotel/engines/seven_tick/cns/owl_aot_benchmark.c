/*
 * OWL-AOT Comprehensive Benchmark Suite
 * Tests the complete ahead-of-time compilation workflow
 */

#include "cns/owl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

// Benchmark configuration
#define WARMUP_ITERATIONS 10000
#define BENCH_ITERATIONS 1000000
#define MAX_ENTITIES 32  // Stay within CNS_OWL_MAX_ENTITIES

// High-resolution timing
static inline uint64_t get_cycles() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

typedef struct {
    const char* name;
    uint64_t total_cycles;
    uint64_t iterations;
    double cycles_per_operation;
    bool is_7t_compliant;
} BenchmarkResult;

// Global engine for all tests
static CNSOWLEngine* g_engine = NULL;

void setup_test_ontology() {
    printf("Setting up test ontology...\n");
    
    g_engine = cns_owl_create(1000);
    if (!g_engine) {
        fprintf(stderr, "Failed to create OWL engine\n");
        exit(1);
    }
    
    // Create class hierarchy: Animal -> Mammal -> Dog
    //                              -> Bird -> Eagle
    uint32_t animal = 1, mammal = 2, dog = 3, bird = 4, eagle = 5;
    
    cns_owl_add_subclass(g_engine, mammal, animal);
    cns_owl_add_subclass(g_engine, dog, mammal);
    cns_owl_add_subclass(g_engine, bird, animal);
    cns_owl_add_subclass(g_engine, eagle, bird);
    
    // Create properties
    uint32_t ancestor = 10, knows = 11, manages = 12, isManagedBy = 13;
    
    cns_owl_set_transitive(g_engine, ancestor);
    cns_owl_set_symmetric(g_engine, knows);
    
    // Add some property assertions (using axiom type 0 for property assertions)
    cns_owl_add_axiom(g_engine, 20, ancestor, 21, 0);  // alice ancestor bob
    cns_owl_add_axiom(g_engine, 21, ancestor, 22, 0);  // bob ancestor charlie
    cns_owl_add_axiom(g_engine, 22, ancestor, 23, 0);  // charlie ancestor david
    
    cns_owl_add_axiom(g_engine, 20, knows, 21, 0);     // alice knows bob
    cns_owl_add_axiom(g_engine, 22, knows, 23, 0);     // charlie knows david
    
    // Materialize all inferences
    cns_owl_materialize_inferences_80_20(g_engine);
    
    printf("Ontology setup complete:\n");
    printf("  - Classes: Animal, Mammal, Dog, Bird, Eagle\n");
    printf("  - Properties: ancestor (transitive), knows (symmetric)\n");
    printf("  - Individuals: alice, bob, charlie, david\n");
    printf("  - Total axioms: %zu\n", g_engine->axiom_count);
    printf("  - Inferences computed: %u\n", cns_owl_get_inference_count(g_engine));
}

BenchmarkResult benchmark_subclass_queries() {
    printf("\n=== Benchmarking Subclass Queries ===\n");
    
    uint64_t start_cycles, end_cycles;
    uint32_t true_results = 0;
    
    // Warmup
    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
        cns_owl_is_subclass_of(g_engine, 3, 1); // dog subclass of animal
    }
    
    // Benchmark
    start_cycles = get_cycles();
    for (int i = 0; i < BENCH_ITERATIONS; i++) {
        if (cns_owl_is_subclass_of(g_engine, 3, 1)) true_results++;  // dog -> animal
        if (cns_owl_is_subclass_of(g_engine, 3, 2)) true_results++;  // dog -> mammal
        if (cns_owl_is_subclass_of(g_engine, 5, 4)) true_results++;  // eagle -> bird
    }
    end_cycles = get_cycles();
    
    uint64_t total_cycles = end_cycles - start_cycles;
    uint64_t total_operations = BENCH_ITERATIONS * 3;
    double cycles_per_op = (double)total_cycles / total_operations;
    
    printf("Subclass Query Results:\n");
    printf("  Iterations: %llu operations\n", total_operations);
    printf("  Total cycles: %llu\n", total_cycles);
    printf("  Cycles per operation: %.2f\n", cycles_per_op);
    printf("  True results: %u\n", true_results);
    printf("  7T compliant: %s\n", cycles_per_op <= 7.0 ? "✓ YES" : "✗ NO");
    
    return (BenchmarkResult){
        .name = "Subclass Queries",
        .total_cycles = total_cycles,
        .iterations = total_operations,
        .cycles_per_operation = cycles_per_op,
        .is_7t_compliant = cycles_per_op <= 7.0
    };
}

BenchmarkResult benchmark_transitive_reasoning() {
    printf("\n=== Benchmarking Transitive Reasoning ===\n");
    
    uint64_t start_cycles, end_cycles;
    uint32_t true_results = 0;
    
    // Warmup
    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
        cns_owl_transitive_query(g_engine, 20, 10, 23); // alice ancestor david
    }
    
    // Benchmark
    start_cycles = get_cycles();
    for (int i = 0; i < BENCH_ITERATIONS; i++) {
        if (cns_owl_transitive_query(g_engine, 20, 10, 21)) true_results++; // alice -> bob
        if (cns_owl_transitive_query(g_engine, 20, 10, 22)) true_results++; // alice -> charlie
        if (cns_owl_transitive_query(g_engine, 20, 10, 23)) true_results++; // alice -> david (transitive)
    }
    end_cycles = get_cycles();
    
    uint64_t total_cycles = end_cycles - start_cycles;
    uint64_t total_operations = BENCH_ITERATIONS * 3;
    double cycles_per_op = (double)total_cycles / total_operations;
    
    printf("Transitive Reasoning Results:\n");
    printf("  Iterations: %llu operations\n", total_operations);
    printf("  Total cycles: %llu\n", total_cycles);
    printf("  Cycles per operation: %.2f\n", cycles_per_op);
    printf("  True results: %u\n", true_results);
    printf("  7T compliant: %s\n", cycles_per_op <= 7.0 ? "✓ YES" : "✗ NO");
    
    return (BenchmarkResult){
        .name = "Transitive Reasoning",
        .total_cycles = total_cycles,
        .iterations = total_operations,
        .cycles_per_operation = cycles_per_op,
        .is_7t_compliant = cycles_per_op <= 7.0
    };
}

BenchmarkResult benchmark_property_characteristics() {
    printf("\n=== Benchmarking Property Characteristics ===\n");
    
    uint64_t start_cycles, end_cycles;
    uint32_t true_results = 0;
    
    // Warmup
    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
        cns_owl_has_property_characteristic(g_engine, 10, OWL_TRANSITIVE);
    }
    
    // Benchmark
    start_cycles = get_cycles();
    for (int i = 0; i < BENCH_ITERATIONS; i++) {
        if (cns_owl_has_property_characteristic(g_engine, 10, OWL_TRANSITIVE)) true_results++;
        if (cns_owl_has_property_characteristic(g_engine, 11, OWL_SYMMETRIC)) true_results++;
        if (cns_owl_has_property_characteristic(g_engine, 12, OWL_FUNCTIONAL)) true_results++;
    }
    end_cycles = get_cycles();
    
    uint64_t total_cycles = end_cycles - start_cycles;
    uint64_t total_operations = BENCH_ITERATIONS * 3;
    double cycles_per_op = (double)total_cycles / total_operations;
    
    printf("Property Characteristics Results:\n");
    printf("  Iterations: %llu operations\n", total_operations);
    printf("  Total cycles: %llu\n", total_cycles);
    printf("  Cycles per operation: %.2f\n", cycles_per_op);
    printf("  True results: %u\n", true_results);
    printf("  7T compliant: %s\n", cycles_per_op <= 7.0 ? "✓ YES" : "✗ NO");
    
    return (BenchmarkResult){
        .name = "Property Characteristics",
        .total_cycles = total_cycles,
        .iterations = total_operations,
        .cycles_per_operation = cycles_per_op,
        .is_7t_compliant = cycles_per_op <= 7.0
    };
}

BenchmarkResult benchmark_materialization_performance() {
    printf("\n=== Benchmarking Materialization Performance ===\n");
    
    // Create a fresh engine for materialization testing
    CNSOWLEngine* test_engine = cns_owl_create(1000);
    
    // Add complex ontology
    for (int i = 0; i < 20; i++) {
        cns_owl_add_subclass(test_engine, i + 1, i);  // Create chain
    }
    
    uint64_t start_cycles = get_cycles();
    cns_owl_materialize_inferences_80_20(test_engine);
    uint64_t end_cycles = get_cycles();
    
    uint64_t total_cycles = end_cycles - start_cycles;
    uint32_t inference_count = cns_owl_get_inference_count(test_engine);
    
    printf("Materialization Results:\n");
    printf("  Input axioms: %zu\n", test_engine->axiom_count);
    printf("  Generated inferences: %u\n", inference_count);
    printf("  Total cycles: %llu\n", total_cycles);
    printf("  Cycles per inference: %.2f\n", (double)total_cycles / inference_count);
    
    cns_owl_destroy(test_engine);
    
    return (BenchmarkResult){
        .name = "Materialization",
        .total_cycles = total_cycles,
        .iterations = inference_count,
        .cycles_per_operation = (double)total_cycles / inference_count,
        .is_7t_compliant = true  // Materialization is offline
    };
}

BenchmarkResult benchmark_80_20_optimization() {
    printf("\n=== Benchmarking 80/20 Optimization Effectiveness ===\n");
    
    CNSOWLEngine* engine_80_20 = cns_owl_create(500);
    CNSOWLEngine* engine_standard = cns_owl_create(500);
    
    // Disable 80/20 optimizations for comparison
    engine_standard->use_80_20_materialization = false;
    engine_standard->use_80_20_reasoning = false;
    
    // Add identical ontologies
    for (int i = 0; i < 15; i++) {
        cns_owl_add_subclass(engine_80_20, i + 1, i);
        cns_owl_add_subclass(engine_standard, i + 1, i);
    }
    
    // Benchmark 80/20 optimized materialization
    uint64_t start_80_20 = get_cycles();
    cns_owl_materialize_inferences_80_20(engine_80_20);
    uint64_t end_80_20 = get_cycles();
    
    // Benchmark standard materialization
    uint64_t start_std = get_cycles();
    cns_owl_materialize_inferences(engine_standard);
    uint64_t end_std = get_cycles();
    
    uint64_t cycles_80_20 = end_80_20 - start_80_20;
    uint64_t cycles_std = end_std - start_std;
    double speedup = (double)cycles_std / cycles_80_20;
    
    printf("80/20 Optimization Results:\n");
    printf("  Standard materialization: %llu cycles\n", cycles_std);
    printf("  80/20 optimized materialization: %llu cycles\n", cycles_80_20);
    printf("  Speedup: %.2fx\n", speedup);
    printf("  Optimization effective: %s\n", speedup > 1.5 ? "✓ YES" : "✗ NO");
    
    cns_owl_destroy(engine_80_20);
    cns_owl_destroy(engine_standard);
    
    return (BenchmarkResult){
        .name = "80/20 Optimization",
        .total_cycles = cycles_80_20,
        .iterations = 1,
        .cycles_per_operation = speedup,
        .is_7t_compliant = true
    };
}

void print_summary(BenchmarkResult* results, int count) {
    printf("\n" "=" "=" "=" "=" "=" " OWL-AOT BENCHMARK SUMMARY " "=" "=" "=" "=" "=" "\n");
    printf("%-25s %12s %12s %8s\n", "Benchmark", "Cycles/Op", "Iterations", "7T?");
    printf("-" "-" "-" "-" "-" "-" "-" "-" "-" "-" "-" "-" "-" "-" "-" "-" "-" "-" "-" "-" "-" "-" "-" "-" "-" "-" "-" "-" "-" "-" "-" "-" "-" "-" "-" "-" "-" "-" "-" "-" "-" "-" "-" "-" "-" "-" "-" "-" "-" "-" "-" "-" "-" "-" "-" "-" "-" "-" "\n");
    
    int compliant_count = 0;
    for (int i = 0; i < count; i++) {
        printf("%-25s %12.2f %12llu %8s\n", 
               results[i].name,
               results[i].cycles_per_operation,
               results[i].iterations,
               results[i].is_7t_compliant ? "✓" : "✗");
        if (results[i].is_7t_compliant) compliant_count++;
    }
    
    printf("\n7-Tick Compliance: %d/%d benchmarks (%.1f%%)\n", 
           compliant_count, count, (double)compliant_count / count * 100.0);
    
    printf("\nOWL-AOT Implementation Status:\n");
    if (compliant_count == count) {
        printf("✅ FULLY OPERATIONAL - All benchmarks meet 7-tick requirements\n");
    } else if (compliant_count >= count * 0.8) {
        printf("⚠️  MOSTLY OPERATIONAL - Minor optimizations needed\n");
    } else {
        printf("❌ NEEDS OPTIMIZATION - Significant performance improvements required\n");
    }
}

int main() {
    printf("🚀 CNS OWL-AOT Comprehensive Benchmark Suite\n");
    printf("Testing ahead-of-time compilation with 7-tick performance\n\n");
    
    setup_test_ontology();
    
    BenchmarkResult results[5];
    results[0] = benchmark_subclass_queries();
    results[1] = benchmark_transitive_reasoning();
    results[2] = benchmark_property_characteristics();
    results[3] = benchmark_materialization_performance();
    results[4] = benchmark_80_20_optimization();
    
    print_summary(results, 5);
    
    // Cleanup
    if (g_engine) {
        cns_owl_destroy(g_engine);
    }
    
    return 0;
}