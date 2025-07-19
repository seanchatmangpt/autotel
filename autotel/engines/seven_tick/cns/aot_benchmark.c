// ============================================================================
// CNS OWL AOT COMPREHENSIVE BENCHMARK SUITE
// ============================================================================
// This benchmark validates the Ahead-of-Time compilation approach described
// in OWL-AOT.md and ensures 7T compliance for all reasoning operations.

#include "cns/owl.h"
#include "ontology_ids.h"
#include "ontology_rules.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

// ============================================================================
// BENCHMARK FRAMEWORK
// ============================================================================

typedef struct {
    const char* name;
    uint64_t total_cycles;
    uint64_t min_cycles;
    uint64_t max_cycles;
    int iterations;
    bool compliant_7t;
} BenchmarkResult;

static BenchmarkResult results[20];
static int result_count = 0;

#define BENCHMARK_START(name, iters) \
    const char* bench_name = name; \
    int bench_iterations = iters; \
    uint64_t bench_start = cns_get_cycles(); \
    uint64_t bench_min = UINT64_MAX; \
    uint64_t bench_max = 0;

#define BENCHMARK_OPERATION(op) \
    do { \
        uint64_t op_start = cns_get_cycles(); \
        op; \
        uint64_t op_end = cns_get_cycles(); \
        uint64_t op_cycles = op_end - op_start; \
        if (op_cycles < bench_min) bench_min = op_cycles; \
        if (op_cycles > bench_max) bench_max = op_cycles; \
    } while(0)

#define BENCHMARK_END() \
    do { \
        uint64_t bench_end = cns_get_cycles(); \
        uint64_t bench_total = bench_end - bench_start; \
        results[result_count].name = bench_name; \
        results[result_count].total_cycles = bench_total; \
        results[result_count].min_cycles = bench_min; \
        results[result_count].max_cycles = bench_max; \
        results[result_count].iterations = bench_iterations; \
        results[result_count].compliant_7t = (bench_max <= 7); \
        result_count++; \
    } while(0)

// ============================================================================
// AOT ONTOLOGY SETUP
// ============================================================================

static CNSOWLEngine* setup_aot_ontology(void) {
    CNSOWLEngine* engine = cns_owl_create(1000);
    if (!engine) return NULL;

    // Setup employee hierarchy as described in OWL-AOT.md
    cns_owl_add_subclass(engine, ID_Manager, ID_Employee);
    cns_owl_add_subclass(engine, ID_IndividualContributor, ID_Employee);
    
    // Setup animal hierarchy for testing
    cns_owl_add_subclass(engine, ID_Mammal, ID_Animal);
    cns_owl_add_subclass(engine, ID_Dog, ID_Mammal);
    
    // Setup equivalent classes
    cns_owl_add_equivalent_class(engine, ID_Person, ID_Human);
    
    // Setup property characteristics
    cns_owl_set_symmetric(engine, ID_worksWith);
    cns_owl_set_functional(engine, ID_hasName);
    cns_owl_set_transitive(engine, ID_ancestor);
    
    // Add test hierarchy for benchmarks
    for (int i = 0; i < 10; i++) {
        cns_owl_add_subclass(engine, ID_TestClass_1 + i + 1, ID_TestClass_1 + i);
    }
    
    // Materialize all inferences using 80/20 optimization
    cns_owl_materialize_inferences_80_20(engine);
    
    return engine;
}

// ============================================================================
// AOT REASONING BENCHMARKS
// ============================================================================

static void benchmark_aot_subclass_reasoning(CNSOWLEngine* engine) {
    printf("\n=== AOT Subclass Reasoning Benchmark ===\n");
    
    BENCHMARK_START("AOT Employee Type Check", 100000);
    
    for (int i = 0; i < 100000; i++) {
        uint32_t entity = ID_Manager + (i % 3); // Rotate through Manager, IC, Contractor
        BENCHMARK_OPERATION(
            volatile bool result = is_Employee(engine, entity); (void)result
        );
    }
    
    BENCHMARK_END();
    
    printf("Employee type checking: %s\n", 
           results[result_count-1].compliant_7t ? "✓ 7T COMPLIANT" : "✗ 7T VIOLATION");
    printf("  Max cycles per operation: %llu\n", results[result_count-1].max_cycles);
    printf("  Min cycles per operation: %llu\n", results[result_count-1].min_cycles);
}

static void benchmark_aot_property_reasoning(CNSOWLEngine* engine) {
    printf("\n=== AOT Property Reasoning Benchmark ===\n");
    
    BENCHMARK_START("AOT Management Relationship", 50000);
    
    for (int i = 0; i < 50000; i++) {
        uint32_t manager = ID_Manager;
        uint32_t employee = ID_IndividualContributor + (i % 10);
        BENCHMARK_OPERATION(
            volatile bool result = check_manages_relationship(engine, manager, employee); (void)result
        );
    }
    
    BENCHMARK_END();
    
    printf("Management relationship checking: %s\n", 
           results[result_count-1].compliant_7t ? "✓ 7T COMPLIANT" : "✗ 7T VIOLATION");
    printf("  Max cycles per operation: %llu\n", results[result_count-1].max_cycles);
}

static void benchmark_aot_transitive_reasoning(CNSOWLEngine* engine) {
    printf("\n=== AOT Transitive Reasoning Benchmark ===\n");
    
    BENCHMARK_START("AOT Transitive Reports-To", 25000);
    
    for (int i = 0; i < 25000; i++) {
        uint32_t employee = ID_IndividualContributor + (i % 10);
        uint32_t ceo = ID_Manager;
        BENCHMARK_OPERATION(
            volatile bool result = check_reports_to_transitively(engine, employee, ceo); (void)result
        );
    }
    
    BENCHMARK_END();
    
    printf("Transitive reporting checking: %s\n", 
           results[result_count-1].compliant_7t ? "✓ 7T COMPLIANT" : "✗ 7T VIOLATION");
    printf("  Max cycles per operation: %llu\n", results[result_count-1].max_cycles);
}

static void benchmark_aot_equivalence_reasoning(CNSOWLEngine* engine) {
    printf("\n=== AOT Equivalence Reasoning Benchmark ===\n");
    
    BENCHMARK_START("AOT Person-Human Equivalence", 75000);
    
    for (int i = 0; i < 75000; i++) {
        uint32_t entity1 = (i % 2 == 0) ? ID_Person : ID_Human;
        uint32_t entity2 = (i % 2 == 0) ? ID_Human : ID_Person;
        BENCHMARK_OPERATION(
            volatile bool result = check_person_human_equivalence(engine, entity1, entity2); (void)result
        );
    }
    
    BENCHMARK_END();
    
    printf("Equivalence checking: %s\n", 
           results[result_count-1].compliant_7t ? "✓ 7T COMPLIANT" : "✗ 7T VIOLATION");
    printf("  Max cycles per operation: %llu\n", results[result_count-1].max_cycles);
}

static void benchmark_aot_permission_levels(CNSOWLEngine* engine) {
    printf("\n=== AOT Permission Level Benchmark ===\n");
    
    BENCHMARK_START("AOT Employee Permissions", 80000);
    
    for (int i = 0; i < 80000; i++) {
        uint32_t employee = ID_Employee + (i % 4); // Rotate through employee types
        BENCHMARK_OPERATION(
            volatile int level = get_employee_permission_level(engine, employee); (void)level
        );
    }
    
    BENCHMARK_END();
    
    printf("Permission level calculation: %s\n", 
           results[result_count-1].compliant_7t ? "✓ 7T COMPLIANT" : "✗ 7T VIOLATION");
    printf("  Max cycles per operation: %llu\n", results[result_count-1].max_cycles);
}

// ============================================================================
// BATCH REASONING BENCHMARKS
// ============================================================================

static void benchmark_aot_batch_operations(CNSOWLEngine* engine) {
    printf("\n=== AOT Batch Operations Benchmark ===\n");
    
    uint32_t entities[] = {ID_TestClass_1, ID_TestClass_2, ID_TestClass_3, ID_TestClass_4};
    uint32_t types[] = {ID_TestClass_1, ID_TestClass_1, ID_TestClass_1, ID_TestClass_1};
    
    BENCHMARK_START("AOT Batch Reasoning", 20000);
    
    for (int i = 0; i < 20000; i++) {
        BENCHMARK_OPERATION(
            volatile int matches = batch_reasoning_check(engine, entities, types, 4); (void)matches
        );
    }
    
    BENCHMARK_END();
    
    printf("Batch reasoning (4 entities): %s\n", 
           results[result_count-1].compliant_7t ? "✓ 7T COMPLIANT" : "✗ 7T VIOLATION");
    printf("  Max cycles per operation: %llu\n", results[result_count-1].max_cycles);
}

// ============================================================================
// MATERIALIZATION BENCHMARKS
// ============================================================================

static void benchmark_aot_materialization_performance(void) {
    printf("\n=== AOT Materialization Performance ===\n");
    
    // Test standard materialization
    CNSOWLEngine* engine1 = cns_owl_create(1000);
    for (int i = 0; i < 100; i++) {
        cns_owl_add_subclass(engine1, i + 1001, i + 1000);
    }
    
    {
        BENCHMARK_START("Standard Materialization", 1);
        BENCHMARK_OPERATION(
            cns_owl_materialize_inferences(engine1)
        );
        BENCHMARK_END();
    }
    
    uint64_t standard_cycles = results[result_count-1].max_cycles;
    
    // Test 80/20 optimized materialization
    CNSOWLEngine* engine2 = cns_owl_create(1000);
    for (int i = 0; i < 100; i++) {
        cns_owl_add_subclass(engine2, i + 1001, i + 1000);
    }
    
    {
        BENCHMARK_START("80/20 Optimized Materialization", 1);
        BENCHMARK_OPERATION(
            cns_owl_materialize_inferences_80_20(engine2)
        );
        BENCHMARK_END();
    }
    
    uint64_t optimized_cycles = results[result_count-1].max_cycles;
    
    printf("Standard materialization: %llu cycles\n", standard_cycles);
    printf("80/20 optimized materialization: %llu cycles\n", optimized_cycles);
    
    if (standard_cycles > 0) {
        double improvement = (double)standard_cycles / optimized_cycles;
        printf("Improvement factor: %.2fx\n", improvement);
    }
    
    cns_owl_destroy(engine1);
    cns_owl_destroy(engine2);
}

// ============================================================================
// MEMORY EFFICIENCY BENCHMARKS
// ============================================================================

static void benchmark_aot_memory_efficiency(CNSOWLEngine* engine) {
    printf("\n=== AOT Memory Efficiency ===\n");
    
    // Memory usage analysis
    size_t base_memory = sizeof(CNSOWLEngine);
    size_t axiom_memory = engine->axiom_count * sizeof(OWLAxiom);
    size_t matrix_memory = ((64 + 63) / 64) * sizeof(uint64_t) * 3; // 3 matrices  
    size_t entity_memory = (64 * 2) * sizeof(uint32_t);
    
    size_t total_memory = base_memory + axiom_memory + matrix_memory + entity_memory;
    
    printf("Memory usage analysis:\n");
    printf("  Base engine: %zu bytes\n", base_memory);
    printf("  Axioms: %zu bytes (%zu axioms)\n", axiom_memory, engine->axiom_count);
    printf("  Bit matrices: %zu bytes\n", matrix_memory);
    printf("  Entity mappings: %zu bytes\n", entity_memory);
    printf("  Total: %zu bytes (%.2f KB)\n", total_memory, total_memory / 1024.0);
    
    // Memory efficiency per reasoning operation
    double bytes_per_axiom = (double)total_memory / engine->axiom_count;
    printf("  Memory per axiom: %.2f bytes\n", bytes_per_axiom);
    printf("  Memory efficiency: %s\n", 
           total_memory < 8192 ? "✓ EXCELLENT" : "⚠ COULD BE IMPROVED");
}

// ============================================================================
// MAIN BENCHMARK RUNNER
// ============================================================================

int main(void) {
    printf("=================================================================\n");
    printf("CNS OWL AOT COMPREHENSIVE BENCHMARK SUITE\n");
    printf("=================================================================\n");
    printf("Testing Ahead-of-Time compilation for OWL reasoning\n");
    printf("7T Compliance Target: ≤7 CPU cycles per reasoning operation\n");
    printf("=================================================================\n");
    
    // Setup AOT ontology
    CNSOWLEngine* engine = setup_aot_ontology();
    if (!engine) {
        printf("✗ Failed to create OWL engine\n");
        return 1;
    }
    
    printf("✓ AOT ontology setup complete\n");
    printf("  Axioms loaded: %zu\n", engine->axiom_count);
    printf("  Inferences computed: %u\n", cns_owl_get_inference_count(engine));
    printf("  Materialization cycles: %llu\n", cns_owl_get_materialization_cycles(engine));
    
    // Run all benchmarks
    benchmark_aot_subclass_reasoning(engine);
    benchmark_aot_property_reasoning(engine);
    benchmark_aot_transitive_reasoning(engine);
    benchmark_aot_equivalence_reasoning(engine);
    benchmark_aot_permission_levels(engine);
    benchmark_aot_batch_operations(engine);
    benchmark_aot_materialization_performance();
    benchmark_aot_memory_efficiency(engine);
    
    // Print comprehensive results
    printf("\n=================================================================\n");
    printf("COMPREHENSIVE BENCHMARK RESULTS\n");
    printf("=================================================================\n");
    
    int compliant_count = 0;
    uint64_t total_operations = 0;
    
    for (int i = 0; i < result_count; i++) {
        printf("%-30s: ", results[i].name);
        printf("%s ", results[i].compliant_7t ? "✓ 7T" : "✗ SLOW");
        printf("(max: %2llu cycles, min: %2llu cycles, %d ops)\n", 
               results[i].max_cycles, results[i].min_cycles, results[i].iterations);
        
        if (results[i].compliant_7t) compliant_count++;
        total_operations += results[i].iterations;
    }
    
    // Final assessment
    printf("\n=== FINAL ASSESSMENT ===\n");
    printf("Total benchmarks: %d\n", result_count);
    printf("7T compliant: %d\n", compliant_count);
    printf("7T violations: %d\n", result_count - compliant_count);
    printf("Compliance rate: %.1f%%\n", (double)compliant_count / result_count * 100.0);
    printf("Total operations tested: %llu\n", total_operations);
    
    if (compliant_count == result_count) {
        printf("\n🎉 SUCCESS: AOT OWL ENGINE IS FULLY 7T COMPLIANT\n");
        printf("✓ All reasoning operations complete in ≤7 CPU cycles\n");
        printf("✓ AOT compilation strategy is working perfectly\n");
        printf("✓ Ready for production deployment\n");
    } else {
        printf("\n⚠ PARTIAL SUCCESS: Some operations need optimization\n");
        printf("✓ %d/%d operations are 7T compliant\n", compliant_count, result_count);
        printf("⚠ %d operations need further optimization\n", result_count - compliant_count);
    }
    
    cns_owl_destroy(engine);
    return (compliant_count == result_count) ? 0 : 1;
}