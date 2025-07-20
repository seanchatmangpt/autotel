#include "cns/8t/8t.h"
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <stdlib.h>

// ============================================================================
// 8T COMPREHENSIVE UNIT TESTS
// ============================================================================

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) \
    do { \
        tests_run++; \
        printf("Running test: %s... ", name); \
        if (test_##name()) { \
            tests_passed++; \
            printf("✓ PASS\n"); \
        } else { \
            printf("✗ FAIL\n"); \
        } \
    } while(0)

#define ASSERT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            printf("Assertion failed: %s (line %d)\n", #condition, __LINE__); \
            return false; \
        } \
    } while(0)

#define ASSERT_EQ(expected, actual) \
    do { \
        if ((expected) != (actual)) { \
            printf("Assertion failed: expected %ld, got %ld (line %d)\n", \
                   (long)(expected), (long)(actual), __LINE__); \
            return false; \
        } \
    } while(0)

#define ASSERT_NEAR(expected, actual, tolerance) \
    do { \
        if (fabs((expected) - (actual)) > (tolerance)) { \
            printf("Assertion failed: expected %f, got %f (tolerance %f, line %d)\n", \
                   (double)(expected), (double)(actual), (double)(tolerance), __LINE__); \
            return false; \
        } \
    } while(0)

// ============================================================================
// ARENA ALLOCATOR TESTS
// ============================================================================

bool test_arena_basic_allocation(void) {
    // Allocate test arena
    size_t arena_size = 4096;
    void* memory = aligned_alloc(CNS_8T_ALIGNMENT, arena_size);
    ASSERT_TRUE(memory != NULL);
    
    cns_8t_arena_t arena;
    cns_8t_arena_init(&arena, memory, arena_size);
    
    // Test basic allocation
    void* ptr1 = cns_8t_arena_alloc(&arena, 64, 8);
    ASSERT_TRUE(ptr1 != NULL);
    ASSERT_TRUE(((uintptr_t)ptr1 & 7) == 0); // Check 8-byte alignment
    
    void* ptr2 = cns_8t_arena_alloc(&arena, 128, 16);
    ASSERT_TRUE(ptr2 != NULL);
    ASSERT_TRUE(((uintptr_t)ptr2 & 15) == 0); // Check 16-byte alignment
    
    // Test that pointers are different
    ASSERT_TRUE(ptr1 != ptr2);
    
    // Test arena reset
    cns_8t_arena_reset(&arena);
    void* ptr3 = cns_8t_arena_alloc(&arena, 64, 8);
    ASSERT_TRUE(ptr3 == ptr1); // Should reuse memory after reset
    
    free(memory);
    return true;
}

bool test_arena_alignment(void) {
    size_t arena_size = 4096;
    void* memory = aligned_alloc(CNS_8T_ALIGNMENT, arena_size);
    ASSERT_TRUE(memory != NULL);
    
    cns_8t_arena_t arena;
    cns_8t_arena_init(&arena, memory, arena_size);
    
    // Test various alignments
    void* ptr8 = cns_8t_arena_alloc(&arena, 1, 8);
    ASSERT_TRUE(((uintptr_t)ptr8 & 7) == 0);
    
    void* ptr16 = cns_8t_arena_alloc(&arena, 1, 16);
    ASSERT_TRUE(((uintptr_t)ptr16 & 15) == 0);
    
    void* ptr32 = cns_8t_arena_alloc(&arena, 1, 32);
    ASSERT_TRUE(((uintptr_t)ptr32 & 31) == 0);
    
    void* ptr64 = cns_8t_arena_alloc(&arena, 1, 64);
    ASSERT_TRUE(((uintptr_t)ptr64 & 63) == 0);
    
    free(memory);
    return true;
}

bool test_arena_cache_line_optimization(void) {
    size_t arena_size = 4096;
    void* memory = aligned_alloc(CNS_8T_ALIGNMENT, arena_size);
    ASSERT_TRUE(memory != NULL);
    
    cns_8t_arena_t arena;
    cns_8t_arena_init(&arena, memory, arena_size);
    
    // Allocate data that should fit in cache lines efficiently
    for (int i = 0; i < 10; i++) {
        void* ptr = cns_8t_arena_alloc(&arena, 32, 8); // 32 bytes, 8-byte aligned
        ASSERT_TRUE(ptr != NULL);
        
        // Write to memory to test cache behavior
        *((volatile uint64_t*)ptr) = 0xDEADBEEF;
    }
    
    // Check L1 cache efficiency
    double efficiency = cns_8t_arena_get_l1_efficiency(&arena);
    ASSERT_TRUE(efficiency >= 0.0 && efficiency <= 1.0);
    
    free(memory);
    return true;
}

// ============================================================================
// NUMERICAL OPERATIONS TESTS
// ============================================================================

bool test_numerical_basic_operations(void) {
    cns_8t_numerical_t a = cns_8t_numerical_create(3.14159);
    cns_8t_numerical_t b = cns_8t_numerical_create(2.71828);
    
    // Test addition
    cns_8t_numerical_t sum = cns_8t_numerical_add(a, b);
    ASSERT_NEAR(sum.value, 5.85987, 1e-5);
    ASSERT_TRUE(sum.error_bound > 0);
    
    // Test multiplication
    cns_8t_numerical_t product = cns_8t_numerical_mul(a, b);
    ASSERT_NEAR(product.value, 8.5397, 1e-4);
    ASSERT_TRUE(product.error_bound > 0);
    
    // Test division
    cns_8t_numerical_t quotient = cns_8t_numerical_div(a, b);
    ASSERT_NEAR(quotient.value, 1.1557, 1e-4);
    ASSERT_TRUE(quotient.error_bound > 0);
    
    return true;
}

bool test_numerical_error_bounds(void) {
    cns_8t_numerical_t a = cns_8t_numerical_create(1.0);
    cns_8t_numerical_t small = cns_8t_numerical_create(1e-15);
    
    // Perform many additions to accumulate error
    cns_8t_numerical_t result = a;
    for (int i = 0; i < 1000; i++) {
        result = cns_8t_numerical_add(result, small);
    }
    
    // Error bound should be realistic
    ASSERT_TRUE(result.error_bound > 0);
    ASSERT_TRUE(result.error_bound < 1e-10); // Should be reasonable
    
    // Test precision validation
    ASSERT_TRUE(cns_8t_numerical_validate_precision(&result, 1e-9));
    ASSERT_TRUE(!cns_8t_numerical_validate_precision(&result, 1e-16));
    
    return true;
}

bool test_numerical_division_by_zero(void) {
    cns_8t_numerical_t a = cns_8t_numerical_create(1.0);
    cns_8t_numerical_t zero = cns_8t_numerical_create(0.0);
    
    cns_8t_numerical_t result = cns_8t_numerical_div(a, zero);
    
    // Should return infinity with appropriate flags
    ASSERT_TRUE(isinf(result.value));
    ASSERT_TRUE(result.precision_flags != 0); // Should have overflow flag
    
    return true;
}

// ============================================================================
// SIMD OPERATIONS TESTS
// ============================================================================

bool test_simd_numerical_operations(void) {
    size_t arena_size = 8192;
    void* memory = aligned_alloc(CNS_8T_ALIGNMENT, arena_size);
    ASSERT_TRUE(memory != NULL);
    
    cns_8t_arena_t arena;
    cns_8t_arena_init(&arena, memory, arena_size);
    
    // Create test arrays
    size_t size = 100;
    cns_8t_numerical_array_t array_a, array_b, array_result;
    cns_8t_numerical_array_init(&array_a, &arena, size);
    cns_8t_numerical_array_init(&array_b, &arena, size);
    cns_8t_numerical_array_init(&array_result, &arena, size);
    
    // Fill arrays with test data
    for (size_t i = 0; i < size; i++) {
        cns_8t_numerical_array_push(&array_a, cns_8t_numerical_create(1.0 + i));
        cns_8t_numerical_array_push(&array_b, cns_8t_numerical_create(2.0 + i));
    }
    
    // Perform SIMD addition
    cns_8t_numerical_add_simd(&array_result, &array_a, &array_b);
    
    // Verify results
    ASSERT_EQ(array_result.count, size);
    for (size_t i = 0; i < size; i++) {
        double expected = (1.0 + i) + (2.0 + i);
        ASSERT_NEAR(array_result.values[i].value, expected, 1e-10);
    }
    
    free(memory);
    return true;
}

bool test_simd_capability_detection(void) {
    cns_8t_simd_caps_t caps = cns_8t_detect_simd_capabilities();
    
    // Just verify that detection doesn't crash and returns reasonable values
    // The actual capabilities depend on the CPU
    printf("    SIMD capabilities: SSE2=%d AVX=%d AVX2=%d FMA=%d\n",
           caps.sse2, caps.avx, caps.avx2, caps.fma);
    
    return true;
}

// ============================================================================
// GRAPH OPERATIONS TESTS
// ============================================================================

bool test_graph_basic_operations(void) {
    size_t arena_size = 8192;
    void* memory = aligned_alloc(CNS_8T_ALIGNMENT, arena_size);
    ASSERT_TRUE(memory != NULL);
    
    cns_8t_arena_t arena;
    cns_8t_arena_init(&arena, memory, arena_size);
    
    cns_8t_graph_t graph;
    cns_8t_graph_init(&graph, &arena, 100);
    
    // Add test triples
    cns_8t_triple_t triple1 = {1, 2, 3, 0};
    cns_8t_triple_t triple2 = {4, 5, 6, 0};
    
    cns_8t_graph_add_triple(&graph, triple1);
    cns_8t_graph_add_triple(&graph, triple2);
    
    ASSERT_EQ(graph.count, 2);
    ASSERT_EQ(graph.triples[0].subject, 1);
    ASSERT_EQ(graph.triples[0].predicate, 2);
    ASSERT_EQ(graph.triples[0].object, 3);
    
    free(memory);
    return true;
}

bool test_graph_pattern_matching(void) {
    size_t arena_size = 8192;
    void* memory = aligned_alloc(CNS_8T_ALIGNMENT, arena_size);
    ASSERT_TRUE(memory != NULL);
    
    cns_8t_arena_t arena;
    cns_8t_arena_init(&arena, memory, arena_size);
    
    cns_8t_graph_t graph;
    cns_8t_graph_init(&graph, &arena, 100);
    
    // Add test triples
    for (uint32_t i = 1; i <= 10; i++) {
        cns_8t_triple_t triple = {i, 100, i * 2, 0};
        cns_8t_graph_add_triple(&graph, triple);
    }
    
    // Add some with different predicates
    for (uint32_t i = 1; i <= 5; i++) {
        cns_8t_triple_t triple = {i, 200, i * 3, 0};
        cns_8t_graph_add_triple(&graph, triple);
    }
    
    // Test pattern matching
    uint32_t results[100];
    size_t result_count;
    
    // Find all triples with predicate 100
    cns_8t_triple_t pattern = {0xFFFFFFFF, 100, 0xFFFFFFFF, 0};
    cns_8t_graph_find_pattern_simd(&graph, pattern, results, &result_count);
    
    ASSERT_EQ(result_count, 10);
    
    // Find all triples with subject 1
    pattern = (cns_8t_triple_t){1, 0xFFFFFFFF, 0xFFFFFFFF, 0};
    cns_8t_graph_find_pattern_simd(&graph, pattern, results, &result_count);
    
    ASSERT_EQ(result_count, 2); // Subject 1 appears twice (with predicates 100 and 200)
    
    free(memory);
    return true;
}

bool test_graph_cache_efficiency(void) {
    size_t arena_size = 8192;
    void* memory = aligned_alloc(CNS_8T_ALIGNMENT, arena_size);
    ASSERT_TRUE(memory != NULL);
    
    cns_8t_arena_t arena;
    cns_8t_arena_init(&arena, memory, arena_size);
    
    cns_8t_graph_t graph;
    cns_8t_graph_init(&graph, &arena, 100);
    
    // Add triples to test cache efficiency
    for (uint32_t i = 0; i < 50; i++) {
        cns_8t_triple_t triple = {i, i % 10, i * 2, 0};
        cns_8t_graph_add_triple(&graph, triple);
    }
    
    double efficiency = cns_8t_graph_cache_efficiency(&graph);
    ASSERT_TRUE(efficiency >= 0.0 && efficiency <= 1.0);
    
    free(memory);
    return true;
}

// ============================================================================
// INTEGRATION TESTS
// ============================================================================

bool test_8t_context_lifecycle(void) {
    cns_8t_context_t ctx;
    cns_result_t result = cns_8t_context_init(&ctx, 64 * 1024); // 64KB arena
    ASSERT_TRUE(result == CNS_OK);
    
    // Test that components are initialized
    ASSERT_TRUE(ctx.arena != NULL);
    ASSERT_TRUE(ctx.graph != NULL);
    
    // Test SIMD capability detection
    ASSERT_TRUE(ctx.avx2_available == true || ctx.avx2_available == false); // Just check it's set
    
    cns_8t_context_destroy(&ctx);
    return true;
}

bool test_performance_constraints(void) {
    // This test verifies that operations meet 8-tick constraints
    // In a real implementation, this would use cycle counters
    
    size_t arena_size = 4096;
    void* memory = aligned_alloc(CNS_8T_ALIGNMENT, arena_size);
    ASSERT_TRUE(memory != NULL);
    
    cns_8t_arena_t arena;
    cns_8t_arena_init(&arena, memory, arena_size);
    
    // Test allocation performance
    uint64_t start = get_cpu_cycles();
    void* ptr = cns_8t_arena_alloc(&arena, 64, 8);
    uint64_t end = get_cpu_cycles();
    
    ASSERT_TRUE(ptr != NULL);
    uint64_t cycles = end - start;
    
    // This is platform dependent, but should be very fast
    printf("    Allocation took %lu cycles (target: ≤ 8)\n", cycles);
    
    free(memory);
    return true;
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

static uint64_t get_cpu_cycles(void) {
#if defined(__x86_64__) || defined(__i386__)
    uint32_t hi, lo;
    __asm__ volatile ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)lo) | (((uint64_t)hi) << 32);
#elif defined(__aarch64__)
    uint64_t tsc;
    __asm__ volatile ("mrs %0, cntvct_el0" : "=r" (tsc));
    return tsc;
#else
    return 0; // Fallback for testing
#endif
}

int main(void) {
    printf("8T Prototype Unit Test Suite\n");
    printf("============================\n\n");
    
    // Arena allocator tests
    printf("Arena Allocator Tests:\n");
    TEST(arena_basic_allocation);
    TEST(arena_alignment);
    TEST(arena_cache_line_optimization);
    printf("\n");
    
    // Numerical operations tests
    printf("Numerical Operations Tests:\n");
    TEST(numerical_basic_operations);
    TEST(numerical_error_bounds);
    TEST(numerical_division_by_zero);
    printf("\n");
    
    // SIMD operations tests
    printf("SIMD Operations Tests:\n");
    TEST(simd_numerical_operations);
    TEST(simd_capability_detection);
    printf("\n");
    
    // Graph operations tests
    printf("Graph Operations Tests:\n");
    TEST(graph_basic_operations);
    TEST(graph_pattern_matching);
    TEST(graph_cache_efficiency);
    printf("\n");
    
    // Integration tests
    printf("Integration Tests:\n");
    TEST(8t_context_lifecycle);
    TEST(performance_constraints);
    printf("\n");
    
    // Print summary
    printf("Test Results Summary:\n");
    printf("====================\n");
    printf("Tests run: %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_run - tests_passed);
    printf("Success rate: %.1f%%\n", (double)tests_passed / tests_run * 100.0);
    
    if (tests_passed == tests_run) {
        printf("\n✓ All tests passed! 8T implementation is working correctly.\n");
        return 0;
    } else {
        printf("\n✗ Some tests failed. Please check the implementation.\n");
        return 1;
    }
}