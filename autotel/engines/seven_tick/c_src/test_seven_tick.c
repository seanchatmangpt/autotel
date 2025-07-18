#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include "sparql7t.h"
#include "shacl7t.h"

// ANSI colors for output
#define GREEN "\033[0;32m"
#define YELLOW "\033[0;33m"
#define BLUE "\033[0;34m"
#define RESET "\033[0m"

// Get hardware ticks
static inline uint64_t get_ticks(void) {
#if defined(__aarch64__)
    uint64_t val;
    __asm__ __volatile__ ("mrs %0, cntvct_el0" : "=r" (val));
    return val;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000ULL + ts.tv_nsec;
#endif
}

// Get CPU frequency (approximate)
static double get_cpu_freq(void) {
#if defined(__aarch64__)
    // ARM64 timer frequency (typically 24MHz on Apple Silicon)
    uint64_t freq;
    __asm__ __volatile__ ("mrs %0, cntfrq_el0" : "=r" (freq));
    return (double)freq;
#else
    uint64_t start = get_ticks();
    struct timespec ts = {0, 100000000}; // 100ms
    nanosleep(&ts, NULL);
    uint64_t end = get_ticks();
    return (end - start) / 100000000.0; // Hz
#endif
}

// Test SPARQL-7T
void test_sparql7t(void) {
    printf(BLUE "\n=== SPARQL-7T Tests ===\n" RESET);
    
    // Create engine
    S7TEngine* engine = s7t_create(1000000, 1000, 10000);
    assert(engine != NULL);
    
    // Add test data
    s7t_add_triple(engine, 42, 1, 100);
    s7t_add_triple(engine, 42, 2, 200);
    s7t_add_triple(engine, 99, 1, 100);
    
    // Test queries
    assert(s7t_ask_pattern(engine, 42, 1, 100) == 1);
    assert(s7t_ask_pattern(engine, 42, 1, 200) == 0);
    assert(s7t_ask_pattern(engine, 99, 1, 100) == 1);
    
    printf(GREEN "✓ SPARQL-7T basic tests passed\n" RESET);
    
    // Cleanup
    free(engine->predicate_vectors);
    free(engine->object_vectors);
    free(engine->ps_to_o_index);
    free(engine);
}

// Test SHACL-7T
void test_shacl7t(void) {
    printf(BLUE "\n=== SHACL-7T Tests ===\n" RESET);
    
    // Create engine
    ShaclEngine* engine = shacl_create(10000, 100);
    assert(engine != NULL);
    
    // Define shape
    CompiledShape user_shape = {
        .target_class_mask = 1ULL << 1,  // Class 1
        .property_mask = 1ULL << 7,      // Property 7
        .constraint_flags = SHACL_TARGET_CLASS | SHACL_PROPERTY
    };
    shacl_add_shape(engine, 0, &user_shape);
    
    // Test validation
    shacl_set_node_class(engine, 100, 1);
    shacl_set_node_property(engine, 100, 7);
    assert(shacl_validate_node(engine, 100, 0) == 1);
    
    shacl_set_node_class(engine, 200, 1);
    // No property set
    assert(shacl_validate_node(engine, 200, 0) == 0);
    
    printf(GREEN "✓ SHACL-7T validation tests passed\n" RESET);
    
    // Cleanup
    free(engine->node_class_vectors);
    free(engine->node_property_vectors);
    free(engine->property_value_vectors);
    free(engine->node_datatype_index);
    free(engine->shapes);
    free(engine);
}

// Benchmark SPARQL-7T
void benchmark_sparql7t(double cpu_freq) {
    printf(BLUE "\n=== SPARQL-7T Benchmark ===\n" RESET);
    
    S7TEngine* engine = s7t_create(1000000, 1000, 10000);
    
    // Populate with test data
    for (int i = 0; i < 10000; i++) {
        s7t_add_triple(engine, i, i % 100, i * 2);
    }
    
    // Warmup
    for (int i = 0; i < 10000; i++) {
        s7t_ask_pattern(engine, i % 1000, i % 100, (i * 2) % 10000);
    }
    
    // Benchmark single queries
    const int iterations = 1000000;
    uint64_t min_cycles = UINT64_MAX;
    uint64_t max_cycles = 0;
    uint64_t total_cycles = 0;
    uint64_t cycles_array[1000];
    
    printf("Running %d single pattern queries...\n", iterations);
    
    for (int i = 0; i < iterations; i++) {
        uint32_t s = i % 1000;
        uint32_t p = i % 100;
        uint32_t o = (i * 2) % 10000;
        
        uint64_t start = get_ticks();
        s7t_ask_pattern(engine, s, p, o);
        uint64_t end = get_ticks();
        
        uint64_t cycles = end - start;
        total_cycles += cycles;
        
        if (i < 1000) cycles_array[i] = cycles;
        if (cycles < min_cycles) min_cycles = cycles;
        if (cycles > max_cycles && cycles < 1000) max_cycles = cycles; // Filter outliers
    }
    
    // Calculate P95
    for (int i = 0; i < 950; i++) {
        for (int j = i + 1; j < 1000; j++) {
            if (cycles_array[i] > cycles_array[j]) {
                uint64_t temp = cycles_array[i];
                cycles_array[i] = cycles_array[j];
                cycles_array[j] = temp;
            }
        }
    }
    uint64_t p95_cycles = cycles_array[950];
    
    double avg_cycles = (double)total_cycles / iterations;
    double ns_per_query = avg_cycles / (cpu_freq / 1e9);
    
    printf(YELLOW "Results:\n" RESET);
    printf("  Min cycles: %llu\n", min_cycles);
    printf("  Avg cycles: %.2f\n", avg_cycles);
    printf("  P95 cycles: %llu\n", p95_cycles);
    printf("  Max cycles: %llu\n", max_cycles);
    printf("  Avg ns/query: %.2f\n", ns_per_query);
    printf("  Queries/sec: %.0f\n", 1e9 / ns_per_query);
    
    // Note: On ARM, timer ticks != CPU cycles
    printf(YELLOW "  Note: ARM timer ticks, not CPU cycles\n" RESET);
    
    // Benchmark batch queries
    printf("\nRunning batch queries...\n");
    
    TriplePattern patterns[4000];
    int results[4000];
    
    for (int i = 0; i < 4000; i++) {
        patterns[i].s = i % 1000;
        patterns[i].p = i % 100;
        patterns[i].o = (i * 2) % 10000;
    }
    
    uint64_t batch_start = get_ticks();
    s7t_ask_batch(engine, patterns, results, 4000);
    uint64_t batch_end = get_ticks();
    
    double batch_cycles_per_query = (batch_end - batch_start) / 4000.0;
    printf("  Batch cycles/query: %.2f\n", batch_cycles_per_query);
    printf("  Batch speedup: %.2fx\n", avg_cycles / batch_cycles_per_query);
    
    // Cleanup
    free(engine->predicate_vectors);
    free(engine->object_vectors);
    free(engine->ps_to_o_index);
    free(engine);
}

// Benchmark SHACL-7T
void benchmark_shacl7t(double cpu_freq) {
    printf(BLUE "\n=== SHACL-7T Benchmark ===\n" RESET);
    
    ShaclEngine* engine = shacl_create(100000, 10);
    
    // Create shapes
    for (int i = 0; i < 10; i++) {
        CompiledShape shape = {
            .target_class_mask = 1ULL << i,
            .property_mask = 1ULL << (i + 10),
            .constraint_flags = SHACL_TARGET_CLASS | SHACL_PROPERTY
        };
        shacl_add_shape(engine, i, &shape);
    }
    
    // Populate nodes
    for (int i = 0; i < 10000; i++) {
        shacl_set_node_class(engine, i, i % 10);
        shacl_set_node_property(engine, i, (i % 10) + 10);
    }
    
    // Benchmark
    const int iterations = 1000000;
    
    printf("Running %d shape validations...\n", iterations);
    
    uint64_t bench_start = get_ticks();
    for (int i = 0; i < iterations; i++) {
        uint32_t node = i % 10000;
        uint32_t shape = i % 10;
        
        shacl_validate_node(engine, node, shape);
    }
    uint64_t bench_end = get_ticks();
    
    double avg_cycles = (double)(bench_end - bench_start) / iterations;
    double ns_per_validation = avg_cycles / (cpu_freq / 1e9);
    
    printf(YELLOW "Results:\n" RESET);
    printf("  Avg cycles: %.2f\n", avg_cycles);
    printf("  Avg ns/validation: %.2f\n", ns_per_validation);
    printf("  Validations/sec: %.0f\n", 1e9 / ns_per_validation);
    
    // Cleanup
    free(engine->node_class_vectors);
    free(engine->node_property_vectors);
    free(engine->property_value_vectors);
    free(engine->node_datatype_index);
    free(engine->shapes);
    free(engine);
}

// Combined stack benchmark
void benchmark_combined(double cpu_freq) {
    printf(BLUE "\n=== Combined Seven-Tick Stack Benchmark ===\n" RESET);
    
    S7TEngine* sparql = s7t_create(100000, 100, 1000);
    ShaclEngine* shacl = shacl_create(100000, 10);
    
    // Setup
    CompiledShape shape = {
        .target_class_mask = 1ULL << 1,
        .property_mask = 1ULL << 2,
        .constraint_flags = SHACL_TARGET_CLASS | SHACL_PROPERTY
    };
    shacl_add_shape(shacl, 0, &shape);
    
    // Add data
    for (int i = 0; i < 1000; i++) {
        s7t_add_triple(sparql, i, 1, i * 2);
        shacl_set_node_class(shacl, i, 1);
        shacl_set_node_property(shacl, i, 2);
    }
    
    // Benchmark combined operations
    printf("Running combined validate + query operations...\n");
    
    const int iterations = 100000;
    uint64_t start = get_ticks();
    
    int valid_count = 0;
    int match_count = 0;
    
    for (int i = 0; i < iterations; i++) {
        uint32_t node = i % 1000;
        
        // First validate with SHACL
        if (shacl_validate_node(shacl, node, 0)) {
            valid_count++;
            
            // Then query with SPARQL
            if (s7t_ask_pattern(sparql, node, 1, node * 2)) {
                match_count++;
            }
        }
    }
    
    uint64_t end = get_ticks();
    
    double avg_cycles = (double)(end - start) / iterations;
    double ns_per_op = avg_cycles / (cpu_freq / 1e9);
    
    printf(YELLOW "Results:\n" RESET);
    printf("  Combined cycles/op: %.2f\n", avg_cycles);
    printf("  Combined ns/op: %.2f\n", ns_per_op);
    printf("  Operations/sec: %.0f\n", 1e9 / ns_per_op);
    printf("  Valid nodes: %d/%d (%.1f%%)\n", valid_count, iterations, 
           100.0 * valid_count / iterations);
    printf("  Matches: %d/%d (%.1f%%)\n", match_count, valid_count,
           100.0 * match_count / valid_count);
    
    // Cleanup
    free(sparql->predicate_vectors);
    free(sparql->object_vectors);
    free(sparql->ps_to_o_index);
    free(sparql);
    
    free(shacl->node_class_vectors);
    free(shacl->node_property_vectors);
    free(shacl->property_value_vectors);
    free(shacl->node_datatype_index);
    free(shacl->shapes);
    free(shacl);
}

int main(int argc, char* argv[]) {
    printf(GREEN "\n╔══════════════════════════════════════╗\n");
    printf("║      Seven Tick (7T) Test Suite      ║\n");
    printf("╚══════════════════════════════════════╝\n" RESET);
    
    // Get CPU frequency
    printf("\nCalibrating timer frequency...\n");
    double cpu_freq = get_cpu_freq();
    printf("Detected timer frequency: %.2f MHz\n", cpu_freq / 1e6);
    
    if (argc > 1 && strcmp(argv[1], "benchmark") == 0) {
        // Run benchmarks
        benchmark_sparql7t(cpu_freq);
        benchmark_shacl7t(cpu_freq);
        benchmark_combined(cpu_freq);
        
        printf(GREEN "\n=== All Benchmarks Complete ===\n" RESET);
    } else {
        // Run tests
        test_sparql7t();
        test_shacl7t();
        
        printf(GREEN "\n=== All Tests Passed ===\n" RESET);
    }
    
    return 0;
}
