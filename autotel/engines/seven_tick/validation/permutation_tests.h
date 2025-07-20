/**
 * @file permutation_tests.h
 * @brief Comprehensive Permutation Testing Framework for CNS v8 Architecture
 *
 * This header provides a complete testing framework with 10,000+ test cases
 * covering all possible input combinations for the CNS v8 architecture.
 *
 * Test Categories:
 * 1. OWL Class Hierarchy Permutations
 * 2. SHACL Constraint Permutations
 * 3. SPARQL Pattern Permutations
 * 4. CJinja Template Permutations
 * 5. Memory Allocation Permutations
 * 6. Contract Enforcement Permutations
 * 7. Performance Regression Detection
 */

#ifndef PERMUTATION_TESTS_H
#define PERMUTATION_TESTS_H

#include "../include/cns/cns_core.h"
#include "../include/cns/cns_contracts.h"
#include "../include/cns/engines/cjinja.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <stdint.h>
#include <math.h>

// ============================================================================
// TEST CONFIGURATION
// ============================================================================

// Comprehensive test configuration
#define MAX_PERMUTATION_SIZE 16 // 2^16 = 65,536 permutations per test
#define MAX_CLASSES_PERM 16
#define MAX_PROPERTIES_PERM 16
#define MAX_TRIPLES_PERM 16
#define MAX_SHAPES_PERM 16
#define MAX_NODES_PERM 16
#define MAX_ALLOCATIONS_PERM 32
#define MAX_TEMPLATES_PERM 16
#define MAX_VARIABLES_PERM 16
#define PERFORMANCE_SAMPLES 10000

// Test result tracking
typedef struct
{
  size_t total_tests;
  size_t passed_tests;
  size_t failed_tests;
  size_t performance_regressions;
  size_t cjinja_tests;
  size_t owl_tests;
  size_t shacl_tests;
  size_t sparql_tests;
  size_t memory_tests;
  size_t contract_tests;
  double total_time_ns;
  double avg_time_ns;
  double min_time_ns;
  double max_time_ns;
} permutation_test_stats_t;

// Performance baseline
typedef struct
{
  double owl_subclass_ns;
  double owl_property_ns;
  double shacl_validation_ns;
  double sparql_match_ns;
  double arena_alloc_ns;
  double cjinja_render_ns;
  double cjinja_compile_ns;
} permutation_performance_baseline_t;

// ============================================================================
// TEST DATA STRUCTURES
// ============================================================================

// OWL test structures
typedef struct CNS_ALIGN_64
{
  cns_id_t class_id;
  cns_bitmask_t superclasses;
  cns_bitmask_t properties;
  cns_bitmask_t instances;
} owl_class_perm_t;

// SHACL test structures
typedef struct CNS_ALIGN_64
{
  cns_id_t shape_id;
  cns_bitmask_t required_properties;
  cns_bitmask_t forbidden_properties;
  cns_bitmask_t value_constraints;
  uint32_t min_count;
  uint32_t max_count;
} shacl_shape_perm_t;

typedef struct CNS_ALIGN_64
{
  cns_id_t node_id;
  cns_bitmask_t properties;
  cns_bitmask_t values;
  uint32_t property_count;
} shacl_node_perm_t;

// SPARQL test structures
typedef struct CNS_ALIGN_64
{
  cns_bitmask_t subject_slab[MAX_TRIPLES_PERM];
  cns_bitmask_t predicate_slab[MAX_TRIPLES_PERM];
  cns_bitmask_t object_slab[MAX_TRIPLES_PERM];
  cns_id_t triple_ids[MAX_TRIPLES_PERM];
  size_t num_triples;
} sparql_bitslab_perm_t;

// CJinja test structures
typedef struct CNS_ALIGN_64
{
  char *template_string;
  char *expected_output;
  char *variables[MAX_VARIABLES_PERM];
  char *variable_names[MAX_VARIABLES_PERM];
  size_t variable_count;
  cns_bool_t should_succeed;
} cjinja_template_perm_t;

// Arena test structures
typedef struct CNS_ALIGN_64
{
  uint8_t *base;
  size_t size;
  size_t used;
  size_t peak;
  uint64_t magic;
} arena_perm_t;

// ============================================================================
// GLOBAL TEST DATA
// ============================================================================

extern owl_class_perm_t owl_classes_perm[MAX_CLASSES_PERM];
extern shacl_shape_perm_t shacl_shapes_perm[MAX_SHAPES_PERM];
extern shacl_node_perm_t shacl_nodes_perm[MAX_NODES_PERM];
extern sparql_bitslab_perm_t sparql_slabs_perm[8];
extern cjinja_template_perm_t cjinja_templates_perm[MAX_TEMPLATES_PERM];
extern arena_perm_t test_arena_perm;
extern uint8_t arena_buffer_perm[1024 * 1024]; // 1MB arena

// Global test statistics
extern permutation_test_stats_t global_perm_stats;
extern permutation_performance_baseline_t perm_baseline;

// ============================================================================
// PERFORMANCE MEASUREMENT
// ============================================================================

// High-precision timing
CNS_INLINE double get_time_ns()
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (double)ts.tv_sec * 1000000000.0 + (double)ts.tv_nsec;
}

// Cycle counting for 8T compliance
CNS_INLINE uint64_t get_cycles()
{
  return __builtin_readcyclecounter();
}

// ============================================================================
// TEST FUNCTION DECLARATIONS
// ============================================================================

// Core test functions
void test_owl_comprehensive_permutations();
void test_shacl_comprehensive_permutations();
void test_sparql_comprehensive_permutations();
void test_cjinja_comprehensive_permutations();
void test_arena_comprehensive_permutations();
void test_contract_comprehensive_permutations();
void test_performance_comprehensive_permutations();

// CJinja specific test functions
void test_cjinja_template_permutations();
void test_cjinja_variable_permutations();
void test_cjinja_conditionals_permutations();
void test_cjinja_loops_permutations();
void test_cjinja_filters_permutations();
void test_cjinja_escape_permutations();
void test_cjinja_performance_permutations();

// Validation functions
void validate_owl_permutation(cns_bitmask_t class_hierarchy, cns_bitmask_t expected);
void validate_shacl_permutation(cns_bitmask_t shape_constraints, cns_bitmask_t node_properties, cns_bool_t expected);
void validate_sparql_permutation(cns_bitmask_t subject_pattern, cns_bitmask_t predicate_pattern, cns_bitmask_t object_pattern, cns_bitmask_t expected);
void validate_cjinja_permutation(const char *template_str, const char *expected_output, cns_bool_t should_succeed);
void validate_arena_permutation(size_t *sizes, size_t count, cns_bool_t expected_success);
void validate_contract_permutation(const char *test_name, cns_bool_t condition, const char *failure_msg);

// Statistics functions
void update_permutation_stats(cns_bool_t passed, double time_ns, const char *test_category);
void print_comprehensive_results();
void print_cjinja_specific_results();

// ============================================================================
// CJINJA TEST TEMPLATES
// ============================================================================

// Basic variable substitution templates
#define CJINJA_BASIC_TEMPLATES_COUNT 100
extern const char *cjinja_basic_templates[CJINJA_BASIC_TEMPLATES_COUNT];

// Conditional templates
#define CJINJA_CONDITIONAL_TEMPLATES_COUNT 50
extern const char *cjinja_conditional_templates[CJINJA_CONDITIONAL_TEMPLATES_COUNT];

// Loop templates
#define CJINJA_LOOP_TEMPLATES_COUNT 50
extern const char *cjinja_loop_templates[CJINJA_LOOP_TEMPLATES_COUNT];

// Filter templates
#define CJINJA_FILTER_TEMPLATES_COUNT 30
extern const char *cjinja_filter_templates[CJINJA_FILTER_TEMPLATES_COUNT];

// Complex templates
#define CJINJA_COMPLEX_TEMPLATES_COUNT 20
extern const char *cjinja_complex_templates[CJINJA_COMPLEX_TEMPLATES_COUNT];

// ============================================================================
// TEST UTILITIES
// ============================================================================

// Template generation utilities
char *generate_random_template(size_t complexity);
char *generate_template_with_variables(size_t variable_count);
char *generate_conditional_template(size_t condition_count);
char *generate_loop_template(size_t loop_count);
char *generate_filter_template(size_t filter_count);

// Variable generation utilities
void generate_random_variables(CNSCjinjaContext *ctx, size_t count);
void generate_edge_case_variables(CNSCjinjaContext *ctx);
void generate_performance_variables(CNSCjinjaContext *ctx);

// Test data generation utilities
void generate_owl_test_data();
void generate_shacl_test_data();
void generate_sparql_test_data();
void generate_cjinja_test_data();
void generate_arena_test_data();

// ============================================================================
// PERFORMANCE MONITORING
// ============================================================================

// Performance regression detection
cns_bool_t detect_performance_regression(double time_ns, double baseline_ns, double threshold);
void record_performance_metric(const char *operation, double time_ns);
void analyze_performance_trends();

// Memory usage monitoring
size_t get_memory_usage();
void track_memory_allocation(size_t size);
void track_memory_deallocation(size_t size);

// ============================================================================
// TEST EXECUTION MACROS
// ============================================================================

// Test execution with timing
#define EXECUTE_TEST(test_name, test_func)                                     \
  do                                                                           \
  {                                                                            \
    double start_time = get_time_ns();                                         \
    test_func();                                                               \
    double end_time = get_time_ns();                                           \
    printf("  %s: %.2f ms\n", test_name, (end_time - start_time) / 1000000.0); \
  } while (0)

// Performance assertion
#define ASSERT_PERFORMANCE(operation, time_ns, baseline_ns, threshold)           \
  do                                                                             \
  {                                                                              \
    if (time_ns > baseline_ns * threshold)                                       \
    {                                                                            \
      printf("  PERFORMANCE REGRESSION: %s took %.2f ns (expected < %.2f ns)\n", \
             operation, time_ns, baseline_ns * threshold);                       \
      global_perm_stats.performance_regressions++;                               \
    }                                                                            \
  } while (0)

// Contract assertion
#define ASSERT_CONTRACT(condition, message)          \
  do                                                 \
  {                                                  \
    if (!(condition))                                \
    {                                                \
      printf("  CONTRACT VIOLATION: %s\n", message); \
      global_perm_stats.failed_tests++;              \
    }                                                \
    else                                             \
    {                                                \
      global_perm_stats.passed_tests++;              \
    }                                                \
    global_perm_stats.total_tests++;                 \
  } while (0)

// ============================================================================
// TEST CATEGORIES
// ============================================================================

// Test category enumeration
typedef enum
{
  TEST_CATEGORY_OWL = 0,
  TEST_CATEGORY_SHACL,
  TEST_CATEGORY_SPARQL,
  TEST_CATEGORY_CJINJA,
  TEST_CATEGORY_MEMORY,
  TEST_CATEGORY_CONTRACT,
  TEST_CATEGORY_PERFORMANCE,
  TEST_CATEGORY_COUNT
} test_category_t;

// Test category names
extern const char *test_category_names[TEST_CATEGORY_COUNT];

// Test category statistics
typedef struct
{
  size_t total_tests[TEST_CATEGORY_COUNT];
  size_t passed_tests[TEST_CATEGORY_COUNT];
  size_t failed_tests[TEST_CATEGORY_COUNT];
  double total_time_ns[TEST_CATEGORY_COUNT];
  double avg_time_ns[TEST_CATEGORY_COUNT];
} test_category_stats_t;

extern test_category_stats_t category_stats;

// ============================================================================
// COMPREHENSIVE TEST SUITE
// ============================================================================

// Main test execution function
int run_comprehensive_permutation_tests();

// Individual test suite functions
int run_owl_permutation_suite();
int run_shacl_permutation_suite();
int run_sparql_permutation_suite();
int run_cjinja_permutation_suite();
int run_memory_permutation_suite();
int run_contract_permutation_suite();
int run_performance_permutation_suite();

// Quick test suite (80/20 focus)
int run_quick_permutation_tests();

// Precision benchmark suite
int run_precision_benchmark_tests();

#endif // PERMUTATION_TESTS_H