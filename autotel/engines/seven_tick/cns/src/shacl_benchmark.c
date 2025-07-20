#include "cns/shacl.h"
#include "cns/graph.h"
#include "cns/arena.h"
#include "cns/interner.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

// SHACL Validation Engine Benchmark - 7T Performance Validation
// =============================================================

#define BENCHMARK_ITERATIONS 100000
#define WARMUP_ITERATIONS 1000

// High-precision timing
static inline uint64_t get_cycle_count(void) {
#if defined(__x86_64__) || defined(__i386__)
    uint64_t tsc;
    __asm__ volatile ("rdtsc" : "=A" (tsc));
    return tsc;
#elif defined(__aarch64__)
    uint64_t tsc;
    __asm__ volatile ("mrs %0, cntvct_el0" : "=r" (tsc));
    return tsc;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
#endif
}

// Performance measurement structure
typedef struct {
    const char *name;
    uint64_t min_cycles;
    uint64_t max_cycles;
    uint64_t total_cycles;
    uint64_t iterations;
    double avg_cycles;
    bool passes_7t;
} benchmark_result_t;

// Initialize benchmark result
static void init_benchmark(benchmark_result_t *result, const char *name) {
    result->name = name;
    result->min_cycles = UINT64_MAX;
    result->max_cycles = 0;
    result->total_cycles = 0;
    result->iterations = 0;
    result->avg_cycles = 0.0;
    result->passes_7t = false;
}

// Record measurement
static void record_measurement(benchmark_result_t *result, uint64_t cycles) {
    if (cycles < result->min_cycles) result->min_cycles = cycles;
    if (cycles > result->max_cycles) result->max_cycles = cycles;
    result->total_cycles += cycles;
    result->iterations++;
}

// Finalize benchmark
static void finalize_benchmark(benchmark_result_t *result) {
    if (result->iterations > 0) {
        result->avg_cycles = (double)result->total_cycles / result->iterations;
        result->passes_7t = (result->avg_cycles <= 7.0);
    }
}

// Print benchmark results
static void print_results(const benchmark_result_t *result) {
    printf("📊 %s\n", result->name);
    printf("   Min cycles: %lu\n", result->min_cycles);
    printf("   Max cycles: %lu\n", result->max_cycles);
    printf("   Avg cycles: %.2f\n", result->avg_cycles);
    printf("   Total iterations: %lu\n", result->iterations);
    printf("   7T compliant: %s %s\n", 
           result->passes_7t ? "✅" : "❌",
           result->passes_7t ? "PASS" : "FAIL");
    printf("\n");
}

int main(void) {
    printf("⚡ SHACL Validation Engine Performance Benchmark\n");
    printf("================================================\n");
    printf("Target: 7 CPU ticks maximum per validation\n");
    printf("Iterations: %d (after %d warmup)\n\n", BENCHMARK_ITERATIONS, WARMUP_ITERATIONS);
    
    // Initialize 7T substrate
    arena_t arena;
    uint8_t arena_memory[4 * 1024 * 1024]; // 4MB arena
    int result = arenac_init(&arena, arena_memory, sizeof(arena_memory), 
                           ARENAC_FLAG_STATS | ARENAC_FLAG_ALIGN_64);
    if (result != 0) {
        fprintf(stderr, "❌ Failed to initialize arena\n");
        return 1;
    }
    
    cns_interner_t *interner = cns_interner_create_default(&arena);
    if (!interner) {
        fprintf(stderr, "❌ Failed to create interner\n");
        return 1;
    }
    
    cns_graph_t *graph = cns_graph_create_default(&arena, interner);
    if (!graph) {
        fprintf(stderr, "❌ Failed to create graph\n");
        return 1;
    }
    
    cns_shacl_validator_t *validator = cns_shacl_validator_create_default(&arena, interner);
    if (!validator) {
        fprintf(stderr, "❌ Failed to create SHACL validator\n");
        return 1;
    }
    
    printf("✅ 7T substrate initialized successfully\n\n");
    
    // Setup test data
    printf("📋 Setting up test data...\n");
    
    // Create shape with class constraint
    cns_shape_t *person_shape = cns_shacl_create_shape(validator, "http://example.org/PersonShape");
    
    cns_constraint_value_t class_value = {
        .string = cns_interner_intern(interner, "http://example.org/Person")
    };
    cns_shacl_add_constraint(person_shape, CNS_SHACL_CLASS, &class_value);
    
    // Add test triples
    cns_graph_insert_triple(graph,
        "http://example.org/john",
        "http://www.w3.org/1999/02/22-rdf-syntax-ns#type",
        "http://example.org/Person",
        CNS_OBJECT_TYPE_IRI);
    
    cns_graph_insert_triple(graph,
        "http://example.org/mary", 
        "http://www.w3.org/1999/02/22-rdf-syntax-ns#type",
        "http://example.org/Person",
        CNS_OBJECT_TYPE_IRI);
    
    // Get string references for benchmarking
    cns_string_ref_t john_ref = cns_interner_intern(interner, "http://example.org/john");
    cns_string_ref_t mary_ref = cns_interner_intern(interner, "http://example.org/mary");
    
    const cns_constraint_t *constraint = person_shape->constraints;
    
    printf("✅ Test data setup complete\n\n");
    
    // Benchmark 1: Constraint Evaluation
    printf("🧪 Benchmark 1: Individual Constraint Evaluation\n");
    
    benchmark_result_t constraint_bench;
    init_benchmark(&constraint_bench, "Constraint Evaluation (sh:class)");
    
    // Warmup
    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
        bool conforms;
        cns_shacl_eval_constraint(validator, graph, john_ref, john_ref, constraint, &conforms);
    }
    
    // Benchmark constraint evaluation
    for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
        uint64_t start = get_cycle_count();
        
        bool conforms;
        cns_shacl_eval_constraint(validator, graph, john_ref, john_ref, constraint, &conforms);
        
        uint64_t end = get_cycle_count();
        record_measurement(&constraint_bench, end - start);
    }
    
    finalize_benchmark(&constraint_bench);
    print_results(&constraint_bench);
    
    // Benchmark 2: Node Validation
    printf("🧪 Benchmark 2: Complete Node Validation\n");
    
    benchmark_result_t node_bench;
    init_benchmark(&node_bench, "Node Validation (full shape)");
    
    // Warmup
    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
        cns_validation_report_t report = {0};
        cns_shacl_validate_node(validator, graph, john_ref, &report);
    }
    
    // Benchmark node validation
    for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
        uint64_t start = get_cycle_count();
        
        cns_validation_report_t report = {0};
        cns_shacl_validate_node(validator, graph, john_ref, &report);
        
        uint64_t end = get_cycle_count();
        record_measurement(&node_bench, end - start);
    }
    
    finalize_benchmark(&node_bench);
    print_results(&node_bench);
    
    // Benchmark 3: Type Checking Utilities
    printf("🧪 Benchmark 3: Type Checking Utilities\n");
    
    benchmark_result_t type_bench;
    init_benchmark(&type_bench, "Type Checking (is_iri, is_literal, is_blank)");
    
    cns_string_ref_t test_iri = john_ref;
    test_iri.type_flags = CNS_NODE_TYPE_IRI;
    
    // Warmup
    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
        cns_shacl_is_iri(test_iri);
        cns_shacl_is_literal(test_iri);
        cns_shacl_is_blank_node(test_iri);
    }
    
    // Benchmark type checking
    for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
        uint64_t start = get_cycle_count();
        
        cns_shacl_is_iri(test_iri);
        cns_shacl_is_literal(test_iri);
        cns_shacl_is_blank_node(test_iri);
        
        uint64_t end = get_cycle_count();
        record_measurement(&type_bench, end - start);
    }
    
    finalize_benchmark(&type_bench);
    print_results(&type_bench);
    
    // Benchmark 4: String Conversion
    printf("🧪 Benchmark 4: String Conversion Utilities\n");
    
    benchmark_result_t string_bench;
    init_benchmark(&string_bench, "String Conversion (constraint/severity/node_kind)");
    
    // Warmup
    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
        cns_shacl_constraint_type_string(CNS_SHACL_CLASS);
        cns_shacl_severity_string(CNS_SEVERITY_VIOLATION);
        cns_shacl_node_kind_string(CNS_NODE_KIND_IRI);
    }
    
    // Benchmark string conversion
    for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
        uint64_t start = get_cycle_count();
        
        cns_shacl_constraint_type_string(CNS_SHACL_CLASS);
        cns_shacl_severity_string(CNS_SEVERITY_VIOLATION);
        cns_shacl_node_kind_string(CNS_NODE_KIND_IRI);
        
        uint64_t end = get_cycle_count();
        record_measurement(&string_bench, end - start);
    }
    
    finalize_benchmark(&string_bench);
    print_results(&string_bench);
    
    // Summary
    printf("📈 PERFORMANCE SUMMARY\n");
    printf("=====================\n");
    
    bool all_pass_7t = constraint_bench.passes_7t && 
                       node_bench.passes_7t && 
                       type_bench.passes_7t && 
                       string_bench.passes_7t;
    
    printf("Overall 7T Compliance: %s %s\n\n", 
           all_pass_7t ? "✅" : "❌",
           all_pass_7t ? "PASS" : "FAIL");
    
    // Memory usage analysis
    printf("💾 MEMORY USAGE ANALYSIS\n");
    printf("========================\n");
    
    arenac_info_t arena_info;
    arenac_get_info(&arena, &arena_info);
    
    printf("Arena utilization: %.1f%% (%zu / %zu bytes)\n",
           arena_info.utilization, arena_info.used_size, arena_info.total_size);
    printf("Total allocations: %lu\n", arena_info.allocation_count);
    printf("SHACL validator memory: %zu bytes\n", cns_shacl_memory_usage(validator));
    printf("Shape count: %zu\n", cns_shacl_shape_count(validator));
    
    // Validator statistics
    cns_shacl_stats_t stats;
    cns_shacl_get_stats(validator, &stats);
    
    printf("\n📊 VALIDATOR STATISTICS\n");
    printf("=======================\n");
    printf("Total validations: %lu\n", stats.validations_performed);
    printf("Constraints evaluated: %lu\n", stats.constraints_evaluated);
    printf("Nodes validated: %lu\n", stats.nodes_validated);
    printf("Violations found: %lu\n", stats.violations_found);
    
    if (all_pass_7t) {
        printf("\n🎉 SUCCESS: SHACL validation engine meets 7T performance requirements!\n");
        printf("🚀 AOT optimization is working effectively\n");
        printf("💪 Ready for production use in 7T substrate\n");
        return 0;
    } else {
        printf("\n⚠️  WARNING: Some operations exceed 7T limit\n");
        printf("🔧 Consider additional optimization\n");
        return 1;
    }
}