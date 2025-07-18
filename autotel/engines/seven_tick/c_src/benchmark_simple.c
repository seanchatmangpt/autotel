#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include "sparql7t.h"
#include "shacl7t.h"

// Simple timing function using nanoseconds
static double get_ns_per_op(void (*test_func)(void* engine), void* engine, int iterations) {
    struct timespec start, end;
    
    // Warmup
    for (int i = 0; i < 1000; i++) {
        test_func(engine);
    }
    
    // Measure
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < iterations; i++) {
        test_func(engine);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    double elapsed_ns = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
    return elapsed_ns / iterations;
}

// Test functions
static S7TEngine* g_sparql_engine;
static ShaclEngine* g_shacl_engine;

void test_sparql_query(void* engine) {
    S7TEngine* e = (S7TEngine*)engine;
    s7t_ask_pattern(e, 42, 1, 100);
}

void test_shacl_validate(void* engine) {
    ShaclEngine* e = (ShaclEngine*)engine;
    shacl_validate_node(e, 100, 0);
}

int main() {
    printf("=== Seven Tick Performance Benchmark ===\n\n");
    
    // Setup SPARQL-7T
    printf("Setting up SPARQL-7T...\n");
    g_sparql_engine = s7t_create(10000, 100, 1000);
    
    // Add test data
    for (int i = 0; i < 1000; i++) {
        s7t_add_triple(g_sparql_engine, i, i % 10, i * 2);
    }
    
    // Setup SHACL-7T
    printf("Setting up SHACL-7T...\n");
    g_shacl_engine = shacl_create(10000, 10);
    
    CompiledShape shape = {
        .target_class_mask = 1ULL << 1,
        .property_mask = 1ULL << 7,
        .constraint_flags = SHACL_TARGET_CLASS | SHACL_PROPERTY
    };
    shacl_add_shape(g_shacl_engine, 0, &shape);
    
    for (int i = 0; i < 1000; i++) {
        shacl_set_node_class(g_shacl_engine, i, 1);
        shacl_set_node_property(g_shacl_engine, i, 7);
    }
    
    // Benchmark
    const int iterations = 10000000;
    
    printf("\nRunning benchmarks (%d iterations each)...\n", iterations);
    
    double sparql_ns = get_ns_per_op(test_sparql_query, g_sparql_engine, iterations);
    printf("\nSPARQL-7T:\n");
    printf("  Nanoseconds per query: %.2f\n", sparql_ns);
    printf("  Queries per second: %.0f\n", 1e9 / sparql_ns);
    
    double shacl_ns = get_ns_per_op(test_shacl_validate, g_shacl_engine, iterations);
    printf("\nSHACL-7T:\n");
    printf("  Nanoseconds per validation: %.2f\n", shacl_ns);
    printf("  Validations per second: %.0f\n", 1e9 / shacl_ns);
    
    // Combined operation
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    for (int i = 0; i < iterations; i++) {
        if (shacl_validate_node(g_shacl_engine, i % 1000, 0)) {
            s7t_ask_pattern(g_sparql_engine, i % 1000, 1, (i % 1000) * 2);
        }
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    double combined_ns = ((end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec)) / iterations;
    
    printf("\nCombined (validate + query):\n");
    printf("  Nanoseconds per operation: %.2f\n", combined_ns);
    printf("  Operations per second: %.0f\n", 1e9 / combined_ns);
    
    // Summary
    printf("\n=== Summary ===\n");
    if (sparql_ns < 10) {
        printf("✅ SPARQL-7T: Sub-10ns performance achieved!\n");
    } else {
        printf("⚠️  SPARQL-7T: %.2f ns (target: <10ns)\n", sparql_ns);
    }
    
    if (shacl_ns < 10) {
        printf("✅ SHACL-7T: Sub-10ns performance achieved!\n");
    } else {
        printf("⚠️  SHACL-7T: %.2f ns (target: <10ns)\n", shacl_ns);
    }
    
    if (combined_ns < 20) {
        printf("✅ Combined: Sub-20ns performance achieved!\n");
    } else {
        printf("⚠️  Combined: %.2f ns (target: <20ns)\n", combined_ns);
    }
    
    // Cleanup
    free(g_sparql_engine->predicate_vectors);
    free(g_sparql_engine->object_vectors);
    free(g_sparql_engine->ps_to_o_index);
    free(g_sparql_engine);
    
    free(g_shacl_engine->node_class_vectors);
    free(g_shacl_engine->node_property_vectors);
    free(g_shacl_engine->property_value_vectors);
    free(g_shacl_engine->node_datatype_index);
    free(g_shacl_engine->shapes);
    free(g_shacl_engine);
    
    return 0;
}
