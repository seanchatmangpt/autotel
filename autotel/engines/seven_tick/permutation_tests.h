/**
 * @file permutation_tests.h
 * @brief Comprehensive Permutation Test Framework for CJinja
 * @version 1.0.0
 * 
 * Tests all possible permutations of:
 * - Variable names (short, long, special chars, edge cases)
 * - Template syntax (valid, malformed, nested, edge cases)
 * - Implementation versions (final, ultra-portable, blazing-fast)
 * - Performance scenarios (small/large templates, few/many variables)
 * 
 * @author 7T Engine Team
 * @date 2024-01-15
 */

#ifndef PERMUTATION_TESTS_H
#define PERMUTATION_TESTS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// Include all CJinja implementations for comparison
#include "cjinja_final.h"
#include "cjinja_ultra_portable.h"
#include "cjinja_blazing_fast.h"

// Resolve macro conflicts
#ifdef MAX_VARIABLES
#undef MAX_VARIABLES
#endif
#define MAX_VARIABLES 32  // Use blazing fast limit for compatibility

#ifdef INITIAL_BUFFER_SIZE  
#undef INITIAL_BUFFER_SIZE
#endif
#define INITIAL_BUFFER_SIZE 512  // Use blazing fast size

// =============================================================================
// PERMUTATION TEST CONFIGURATION
// =============================================================================

#define MAX_TEST_CASES 10000
#define MAX_TEMPLATE_LEN 2048
#define MAX_RESULT_LEN 4096
#define MAX_VAR_COUNT 64

// Test case types
typedef enum {
    TEST_BASIC_VARIABLES,
    TEST_EDGE_CASES,
    TEST_MALFORMED_SYNTAX,
    TEST_PERFORMANCE_STRESS,
    TEST_MEMORY_LIMITS,
    TEST_UNICODE_SUPPORT,
    TEST_NESTED_TEMPLATES,
    TEST_EMPTY_VALUES,
    TEST_LONG_VARIABLES,
    TEST_SPECIAL_CHARS
} PermutationTestType;

// Implementation types to test
typedef enum {
    IMPL_FINAL,
    IMPL_ULTRA_PORTABLE,
    IMPL_BLAZING_FAST,
    IMPL_COUNT
} ImplementationType;

// Test case structure
typedef struct {
    char template[MAX_TEMPLATE_LEN];
    char variables[MAX_VAR_COUNT][2][64]; // [index][key/value][content]
    int var_count;
    char expected_result[MAX_RESULT_LEN];
    PermutationTestType type;
    bool should_pass;
    char description[128];
} PermutationTestCase;

// Test result structure
typedef struct {
    bool passed;
    char actual_result[MAX_RESULT_LEN];
    uint64_t execution_time_ns;
    ImplementationType impl_type;
    char error_message[256];
} PermutationTestResult;

// Test suite statistics
typedef struct {
    uint32_t total_tests;
    uint32_t passed_tests;
    uint32_t failed_tests;
    uint32_t skipped_tests;
    uint64_t total_time_ns;
    uint32_t impl_results[IMPL_COUNT][2]; // [impl][pass/fail]
} PermutationTestStats;

// =============================================================================
// PERMUTATION TEST GENERATORS
// =============================================================================

/**
 * @brief Generate all variable name permutations
 */
void generate_variable_name_permutations(PermutationTestCase* cases, int* count);

/**
 * @brief Generate all template syntax permutations
 */
void generate_template_syntax_permutations(PermutationTestCase* cases, int* count);

/**
 * @brief Generate edge case permutations
 */
void generate_edge_case_permutations(PermutationTestCase* cases, int* count);

/**
 * @brief Generate performance stress permutations
 */
void generate_performance_permutations(PermutationTestCase* cases, int* count);

/**
 * @brief Generate malformed syntax permutations
 */
void generate_malformed_permutations(PermutationTestCase* cases, int* count);

/**
 * @brief Generate unicode and special character permutations
 */
void generate_unicode_permutations(PermutationTestCase* cases, int* count);

// =============================================================================
// IMPLEMENTATION WRAPPERS
// =============================================================================

/**
 * @brief Execute test case on Final implementation
 */
PermutationTestResult execute_final_impl(const PermutationTestCase* test);

/**
 * @brief Execute test case on Ultra-Portable implementation
 */
PermutationTestResult execute_ultra_impl(const PermutationTestCase* test);

/**
 * @brief Execute test case on Blazing-Fast implementation
 */
PermutationTestResult execute_blazing_impl(const PermutationTestCase* test);

// =============================================================================
// TEST EXECUTION AND VALIDATION
// =============================================================================

/**
 * @brief Run single test case on all implementations
 */
void run_permutation_test(const PermutationTestCase* test, PermutationTestResult* results);

/**
 * @brief Run all permutation tests
 */
PermutationTestStats run_all_permutation_tests(void);

/**
 * @brief Validate consistency across implementations
 */
bool validate_implementation_consistency(const PermutationTestResult* results);

/**
 * @brief Compare performance across implementations
 */
void compare_implementation_performance(const PermutationTestResult* results);

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

/**
 * @brief Get high-resolution timestamp
 */
static inline uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

/**
 * @brief Safe string comparison with NULL handling
 */
static inline bool safe_string_equals(const char* a, const char* b) {
    if (a == NULL && b == NULL) return true;
    if (a == NULL || b == NULL) return false;
    return strcmp(a, b) == 0;
}

/**
 * @brief Generate random string for testing
 */
void generate_random_string(char* buffer, int length, bool alphanumeric_only);

/**
 * @brief Print test case details
 */
void print_test_case(const PermutationTestCase* test);

/**
 * @brief Print test results
 */
void print_test_results(const PermutationTestResult* results, int count);

/**
 * @brief Print comprehensive statistics
 */
void print_permutation_stats(const PermutationTestStats* stats);

#ifdef __cplusplus
}
#endif

#endif // PERMUTATION_TESTS_H