/**
 * @file test_8t_simple.c
 * @brief Unit tests for 8T components - L1 cache optimization and numerical precision
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <time.h>
#include <stdint.h>

// Include 8T headers
#include "cns/8t/arena_l1.h"
#include "cns/8t/numerical.h"
#include "cns/8t/graph_l1_rdf.h"

// Test results tracking
typedef struct {
    int total;
    int passed;
    int failed;
    double total_time;
} test_results_t;

static test_results_t results = {0, 0, 0, 0.0};

// Test macros
#define TEST_START(name) \
    printf("\n[TEST] %s\n", name); \
    clock_t start = clock(); \
    results.total++;

#define TEST_PASS() \
    results.passed++; \
    double elapsed = ((double)(clock() - start)) / CLOCKS_PER_SEC; \
    results.total_time += elapsed; \
    printf("  ✓ PASSED (%.6f seconds)\n", elapsed);

#define TEST_FAIL(msg) \
    results.failed++; \
    printf("  ✗ FAILED: %s\n", msg); \
    return;

#define ASSERT_TRUE(cond, msg) \
    if (!(cond)) { TEST_FAIL(msg); }

#define ASSERT_EQ(a, b, msg) \
    if ((a) != (b)) { \
        printf("  Expected: %ld, Got: %ld\n", (long)(b), (long)(a)); \
        TEST_FAIL(msg); \
    }

#define ASSERT_NEAR(a, b, tol, msg) \
    if (fabs((a) - (b)) > (tol)) { \
        printf("  Expected: %f, Got: %f (tolerance: %f)\n", (double)(b), (double)(a), (double)(tol)); \
        TEST_FAIL(msg); \
    }

// Test L1-optimized arena allocator
void test_arena_l1_basic() {
    TEST_START("Arena L1: Basic allocation");
    
    cns_8t_arena_l1_config_t config = {
        .l1_working_set_size = 16 * 1024,  // 16KB
        .prefetch_distance = 2,
        .alignment_requirement = 64,
        .enable_numa_locality = false,
        .enable_cache_coloring = true,
        .use_huge_pages = false,
        .enable_prefetch = true,
        .optimize_for_streaming = false,
    };
    
    cns_8t_arena_l1_t* arena = NULL;
    cns_8t_result_t result = cns_8t_arena_l1_create(&config, &arena);
    ASSERT_EQ(result, CNS_8T_SUCCESS, "Arena creation failed");
    ASSERT_TRUE(arena != NULL, "Arena is NULL");
    
    // Test aligned allocation
    void* ptr1 = NULL;
    result = cns_8t_arena_l1_alloc(arena, 128, &ptr1);
    ASSERT_EQ(result, CNS_8T_SUCCESS, "Allocation failed");
    ASSERT_TRUE(ptr1 != NULL, "Allocated pointer is NULL");
    ASSERT_EQ((uintptr_t)ptr1 % 64, 0, "Pointer not cache-line aligned");
    
    // Test multiple allocations
    void* ptr2 = NULL;
    result = cns_8t_arena_l1_alloc(arena, 256, &ptr2);
    ASSERT_EQ(result, CNS_8T_SUCCESS, "Second allocation failed");
    ASSERT_TRUE(ptr2 != NULL, "Second pointer is NULL");
    ASSERT_TRUE(ptr2 > ptr1, "Second pointer not after first");
    
    // Test stats
    cns_8t_arena_l1_stats_t stats;
    cns_8t_arena_l1_get_stats(arena, &stats);
    ASSERT_TRUE(stats.l1_hit_rate >= 0.0, "Invalid L1 hit rate");
    ASSERT_TRUE(stats.total_allocated >= 384, "Invalid allocation count");
    
    cns_8t_arena_l1_destroy(arena);
    TEST_PASS();
}

void test_arena_l1_batch() {
    TEST_START("Arena L1: Batch allocation");
    
    cns_8t_arena_l1_config_t config = {
        .l1_working_set_size = 32 * 1024,
        .prefetch_distance = 3,
        .alignment_requirement = 64,
        .enable_numa_locality = false,
        .enable_cache_coloring = true,
        .use_huge_pages = false,
        .enable_prefetch = true,
        .optimize_for_streaming = true,
    };
    
    cns_8t_arena_l1_t* arena = NULL;
    cns_8t_arena_l1_create(&config, &arena);
    
    // Batch allocation test
    size_t sizes[] = {64, 128, 256, 512, 1024};
    void* ptrs[5] = {NULL};
    
    cns_8t_result_t result = cns_8t_arena_l1_alloc_batch(arena, sizes, 5, ptrs);
    ASSERT_EQ(result, CNS_8T_SUCCESS, "Batch allocation failed");
    
    // Verify all pointers are valid and aligned
    for (int i = 0; i < 5; i++) {
        ASSERT_TRUE(ptrs[i] != NULL, "Batch pointer is NULL");
        ASSERT_EQ((uintptr_t)ptrs[i] % 64, 0, "Batch pointer not aligned");
        if (i > 0) {
            ASSERT_TRUE(ptrs[i] > ptrs[i-1], "Pointers not sequential");
        }
    }
    
    cns_8t_arena_l1_destroy(arena);
    TEST_PASS();
}

// Test numerical precision engine
void test_numerical_basic() {
    TEST_START("Numerical: Basic operations with error bounds");
    
    cns_8t_numerical_config_t config = {
        .enable_error_tracking = true,
        .enable_fma = true,
        .enable_simd = true,
        .enable_interval_arithmetic = true,
    };
    
    cns_8t_numerical_t* numerical = NULL;
    cns_8t_result_t result = cns_8t_numerical_create(&config, &numerical);
    ASSERT_EQ(result, CNS_8T_SUCCESS, "Numerical engine creation failed");
    
    // Test addition with error bounds
    cns_8t_tracked_value_t a = {.value = 1.0, .absolute_error = 0.0, .relative_error = 0.0};
    cns_8t_tracked_value_t b = {.value = 2.0, .absolute_error = 0.0, .relative_error = 0.0};
    cns_8t_tracked_value_t sum;
    
    result = cns_8t_numerical_add(numerical, &a, &b, &sum);
    ASSERT_EQ(result, CNS_8T_SUCCESS, "Addition failed");
    ASSERT_NEAR(sum.value, 3.0, 1e-15, "Addition result incorrect");
    ASSERT_TRUE(sum.absolute_error < 1e-14, "Error bound too large");
    
    // Test multiplication
    cns_8t_tracked_value_t product;
    result = cns_8t_numerical_multiply(numerical, &a, &b, &product);
    ASSERT_EQ(result, CNS_8T_SUCCESS, "Multiplication failed");
    ASSERT_NEAR(product.value, 2.0, 1e-15, "Multiplication result incorrect");
    
    // Test condition number
    double condition = cns_8t_numerical_condition_number(numerical, &a, &b, CNS_8T_OP_ADD);
    ASSERT_TRUE(condition >= 0.0, "Invalid condition number");
    
    cns_8t_numerical_destroy(numerical);
    TEST_PASS();
}

void test_numerical_compensated() {
    TEST_START("Numerical: Compensated summation");
    
    cns_8t_numerical_config_t config = {
        .enable_error_tracking = true,
        .enable_fma = true,
        .enable_simd = true,
        .enable_interval_arithmetic = false,
    };
    
    cns_8t_numerical_t* numerical = NULL;
    cns_8t_numerical_create(&config, &numerical);
    
    // Test Kahan summation
    cns_8t_compensated_sum_t kahan_sum;
    cns_8t_numerical_kahan_init(numerical, &kahan_sum);
    
    // Add many small values
    for (int i = 0; i < 1000000; i++) {
        cns_8t_numerical_kahan_add(numerical, &kahan_sum, 0.1);
    }
    
    double result = cns_8t_numerical_kahan_value(numerical, &kahan_sum);
    double expected = 100000.0;
    double naive_error = fabs(expected - (0.1 * 1000000));
    double kahan_error = fabs(expected - result);
    
    ASSERT_TRUE(kahan_error < naive_error, "Kahan summation not more accurate");
    ASSERT_NEAR(result, expected, 1e-10, "Kahan sum incorrect");
    
    cns_8t_numerical_destroy(numerical);
    TEST_PASS();
}

// Test cache-optimized graph
void test_graph_l1_basic() {
    TEST_START("Graph L1: Basic triple operations");
    
    cns_8t_graph_l1_rdf_t* graph = cns_8t_graph_l1_rdf_create(1024);
    ASSERT_TRUE(graph != NULL, "Graph creation failed");
    
    // Add triples
    cns_8t_triple_rdf_t triple1 = {
        .subject_id = 1,
        .predicate_id = 2,
        .object_id = 3,
        .type_flags = CNS_8T_TRIPLE_TYPE_RESOURCE,
        .graph_id = 0
    };
    
    cns_8t_result_t result = cns_8t_graph_l1_rdf_add(graph, &triple1);
    ASSERT_EQ(result, CNS_8T_SUCCESS, "Triple addition failed");
    
    // Add more triples
    for (uint32_t i = 0; i < 100; i++) {
        cns_8t_triple_rdf_t triple = {
            .subject_id = i,
            .predicate_id = i + 1,
            .object_id = i + 2,
            .type_flags = CNS_8T_TRIPLE_TYPE_RESOURCE,
            .graph_id = 0
        };
        result = cns_8t_graph_l1_rdf_add(graph, &triple);
        ASSERT_EQ(result, CNS_8T_SUCCESS, "Bulk triple addition failed");
    }
    
    // Test pattern matching
    cns_8t_pattern_t pattern = {
        .subject_id = 1,
        .predicate_id = CNS_8T_WILDCARD,
        .object_id = CNS_8T_WILDCARD
    };
    
    cns_8t_triple_rdf_t matches[10];
    size_t match_count = 0;
    result = cns_8t_graph_l1_rdf_match(graph, &pattern, matches, 10, &match_count);
    ASSERT_EQ(result, CNS_8T_SUCCESS, "Pattern matching failed");
    ASSERT_TRUE(match_count > 0, "No matches found");
    
    // Verify stats
    size_t count = cns_8t_graph_l1_rdf_count(graph);
    ASSERT_EQ(count, 101, "Incorrect triple count");
    
    cns_8t_graph_l1_rdf_destroy(graph);
    TEST_PASS();
}

void test_graph_l1_simd() {
    TEST_START("Graph L1: SIMD batch operations");
    
    cns_8t_graph_l1_rdf_t* graph = cns_8t_graph_l1_rdf_create(8192);
    ASSERT_TRUE(graph != NULL, "Graph creation failed");
    
    // Prepare batch of triples
    cns_8t_triple_rdf_t batch[8];
    for (int i = 0; i < 8; i++) {
        batch[i].subject_id = i * 10;
        batch[i].predicate_id = i * 10 + 1;
        batch[i].object_id = i * 10 + 2;
        batch[i].type_flags = CNS_8T_TRIPLE_TYPE_RESOURCE;
        batch[i].graph_id = 0;
    }
    
    // Add batch using SIMD
    cns_8t_result_t result = cns_8t_graph_l1_rdf_add_batch(graph, batch, 8);
    ASSERT_EQ(result, CNS_8T_SUCCESS, "SIMD batch addition failed");
    
    // Verify all were added
    size_t count = cns_8t_graph_l1_rdf_count(graph);
    ASSERT_EQ(count, 8, "Incorrect batch count");
    
    // Test SIMD pattern matching
    cns_8t_pattern_t pattern = {
        .subject_id = CNS_8T_WILDCARD,
        .predicate_id = 11,  // Should match second triple
        .object_id = CNS_8T_WILDCARD
    };
    
    cns_8t_triple_rdf_t matches[8];
    size_t match_count = 0;
    result = cns_8t_graph_l1_rdf_match(graph, &pattern, matches, 8, &match_count);
    ASSERT_EQ(result, CNS_8T_SUCCESS, "SIMD pattern matching failed");
    ASSERT_EQ(match_count, 1, "Incorrect SIMD match count");
    
    cns_8t_graph_l1_rdf_destroy(graph);
    TEST_PASS();
}

// Integration test
void test_8t_integration() {
    TEST_START("8T Integration: Combined components");
    
    // Create all components
    cns_8t_arena_l1_config_t arena_config = {
        .l1_working_set_size = 64 * 1024,
        .prefetch_distance = 3,
        .alignment_requirement = 64,
        .enable_cache_coloring = true,
        .enable_prefetch = true,
    };
    
    cns_8t_arena_l1_t* arena = NULL;
    cns_8t_arena_l1_create(&arena_config, &arena);
    
    cns_8t_numerical_config_t num_config = {
        .enable_error_tracking = true,
        .enable_fma = true,
        .enable_simd = true,
    };
    
    cns_8t_numerical_t* numerical = NULL;
    cns_8t_numerical_create(&num_config, &numerical);
    
    cns_8t_graph_l1_rdf_t* graph = cns_8t_graph_l1_rdf_create(4096);
    
    // Use arena for graph allocations
    void* buffer = NULL;
    cns_8t_arena_l1_alloc(arena, 1024 * sizeof(cns_8t_triple_rdf_t), &buffer);
    
    // Add triples and track numerical properties
    for (int i = 0; i < 1000; i++) {
        cns_8t_triple_rdf_t triple = {
            .subject_id = i,
            .predicate_id = i % 10,
            .object_id = i * 2,
            .type_flags = CNS_8T_TRIPLE_TYPE_LITERAL,
            .graph_id = 0
        };
        
        cns_8t_graph_l1_rdf_add(graph, &triple);
        
        // Track some numerical property
        cns_8t_tracked_value_t val = {
            .value = (double)i,
            .absolute_error = 0.0,
            .relative_error = 0.0
        };
        cns_8t_tracked_value_t squared;
        cns_8t_numerical_multiply(numerical, &val, &val, &squared);
    }
    
    // Check results
    size_t count = cns_8t_graph_l1_rdf_count(graph);
    ASSERT_EQ(count, 1000, "Integration test triple count incorrect");
    
    cns_8t_arena_l1_stats_t stats;
    cns_8t_arena_l1_get_stats(arena, &stats);
    ASSERT_TRUE(stats.l1_hit_rate > 0.7, "L1 hit rate too low");
    
    // Cleanup
    cns_8t_graph_l1_rdf_destroy(graph);
    cns_8t_numerical_destroy(numerical);
    cns_8t_arena_l1_destroy(arena);
    
    TEST_PASS();
}

// Main test runner
int main(int argc, char* argv[]) {
    printf("=== 8T Component Unit Tests ===\n");
    printf("Testing L1 cache optimization and numerical precision\n");
    
    // Run tests
    test_arena_l1_basic();
    test_arena_l1_batch();
    test_numerical_basic();
    test_numerical_compensated();
    test_graph_l1_basic();
    test_graph_l1_simd();
    test_8t_integration();
    
    // Print summary
    printf("\n=== Test Summary ===\n");
    printf("Total tests: %d\n", results.total);
    printf("Passed: %d (%.1f%%)\n", results.passed, 
           results.total > 0 ? (100.0 * results.passed / results.total) : 0);
    printf("Failed: %d\n", results.failed);
    printf("Total time: %.3f seconds\n", results.total_time);
    
    if (results.failed == 0) {
        printf("\n✓ All tests passed!\n");
        return 0;
    } else {
        printf("\n✗ Some tests failed.\n");
        return 1;
    }
}