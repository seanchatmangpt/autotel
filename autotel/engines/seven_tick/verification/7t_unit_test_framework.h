#ifndef SEVEN_T_UNIT_TEST_FRAMEWORK_H
#define SEVEN_T_UNIT_TEST_FRAMEWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <stdbool.h>
#include <stdint.h>

// Test result tracking
typedef struct
{
  const char *test_name;
  const char *test_suite;
  bool passed;
  const char *failure_message;
  double execution_time_ms;
} TestResult;

// Test suite structure
typedef struct
{
  const char *suite_name;
  TestResult *results;
  int result_count;
  int result_capacity;
  int total_tests;
  int passed_tests;
  int failed_tests;
} TestSuite;

// Test function type
typedef void (*TestFunction)(void);

// Test context for sharing data between tests
typedef struct
{
  void *engine_state;
  void *test_data;
  int test_id;
  const char *test_name;
} TestContext;

// Framework functions
TestSuite *create_test_suite(const char *name);
void destroy_test_suite(TestSuite *suite);
void add_test_result(TestSuite *suite, TestResult result);
void print_test_suite(TestSuite *suite);
void print_test_summary(TestSuite *suite);

// Test execution
void run_test(TestSuite *suite, const char *test_name, TestFunction test_func);
void run_test_with_context(TestSuite *suite, const char *test_name,
                           void (*test_func)(TestContext *), TestContext *context);

// Assertion macros
#define ASSERT_TRUE(condition)                           \
  do                                                     \
  {                                                      \
    if (!(condition))                                    \
    {                                                    \
      printf("❌ ASSERT_TRUE failed: %s\n", #condition); \
      return;                                            \
    }                                                    \
  } while (0)

#define ASSERT_FALSE(condition)                           \
  do                                                      \
  {                                                       \
    if ((condition))                                      \
    {                                                     \
      printf("❌ ASSERT_FALSE failed: %s\n", #condition); \
      return;                                             \
    }                                                     \
  } while (0)

#define ASSERT_EQUAL(expected, actual)                                             \
  do                                                                               \
  {                                                                                \
    if ((expected) != (actual))                                                    \
    {                                                                              \
      printf("❌ ASSERT_EQUAL failed: expected %s, got %s\n", #expected, #actual); \
      return;                                                                      \
    }                                                                              \
  } while (0)

#define ASSERT_NOT_EQUAL(expected, actual)                              \
  do                                                                    \
  {                                                                     \
    if ((expected) == (actual))                                         \
    {                                                                   \
      printf("❌ ASSERT_NOT_EQUAL failed: both equal %s\n", #expected); \
      return;                                                           \
    }                                                                   \
  } while (0)

#define ASSERT_NULL(ptr)                                      \
  do                                                          \
  {                                                           \
    if ((ptr) != NULL)                                        \
    {                                                         \
      printf("❌ ASSERT_NULL failed: pointer is not NULL\n"); \
      return;                                                 \
    }                                                         \
  } while (0)

#define ASSERT_NOT_NULL(ptr)                                  \
  do                                                          \
  {                                                           \
    if ((ptr) == NULL)                                        \
    {                                                         \
      printf("❌ ASSERT_NOT_NULL failed: pointer is NULL\n"); \
      return;                                                 \
    }                                                         \
  } while (0)

#define ASSERT_STRING_EQUAL(expected, actual)                                                   \
  do                                                                                            \
  {                                                                                             \
    if (strcmp((expected), (actual)) != 0)                                                      \
    {                                                                                           \
      printf("❌ ASSERT_STRING_EQUAL failed: expected '%s', got '%s'\n", (expected), (actual)); \
      return;                                                                                   \
    }                                                                                           \
  } while (0)

#define ASSERT_STRING_NOT_EQUAL(expected, actual)                                 \
  do                                                                              \
  {                                                                               \
    if (strcmp((expected), (actual)) == 0)                                        \
    {                                                                             \
      printf("❌ ASSERT_STRING_NOT_EQUAL failed: both equal '%s'\n", (expected)); \
      return;                                                                     \
    }                                                                             \
  } while (0)

#define ASSERT_GREATER_THAN(expected, actual)                                  \
  do                                                                           \
  {                                                                            \
    if ((actual) <= (expected))                                                \
    {                                                                          \
      printf("❌ ASSERT_GREATER_THAN failed: %s <= %s\n", #actual, #expected); \
      return;                                                                  \
    }                                                                          \
  } while (0)

#define ASSERT_LESS_THAN(expected, actual)                                  \
  do                                                                        \
  {                                                                         \
    if ((actual) >= (expected))                                             \
    {                                                                       \
      printf("❌ ASSERT_LESS_THAN failed: %s >= %s\n", #actual, #expected); \
      return;                                                               \
    }                                                                       \
  } while (0)

// Performance assertions
#define ASSERT_PERFORMANCE_7TICK(operation, iterations)                                     \
  do                                                                                        \
  {                                                                                         \
    clock_t start = clock();                                                                \
    for (int i = 0; i < (iterations); i++)                                                  \
    {                                                                                       \
      operation;                                                                            \
    }                                                                                       \
    clock_t end = clock();                                                                  \
    double avg_ns = ((double)(end - start) / CLOCKS_PER_SEC) * 1000000000.0 / (iterations); \
    if (avg_ns >= 10.0)                                                                     \
    {                                                                                       \
      printf("❌ ASSERT_PERFORMANCE_7TICK failed: %.1f ns (target: <10ns)\n", avg_ns);      \
      return;                                                                               \
    }                                                                                       \
    printf("✅ 7-TICK PERFORMANCE: %.1f ns\n", avg_ns);                                     \
  } while (0)

// Test utilities
TestResult create_test_result(const char *test_name, const char *test_suite,
                              bool passed, const char *failure_message, double execution_time_ms);
double get_execution_time_ms(clock_t start, clock_t end);

// Test data utilities
void *create_test_engine(void);
void destroy_test_engine(void *engine);
void *create_test_data(void);
void destroy_test_data(void *data);

// Test suite runners
void run_sparql_tests(TestSuite *suite);
void run_shacl_tests(TestSuite *suite);
void run_cjinja_tests(TestSuite *suite);
void run_memory_tests(TestSuite *suite);
void run_integration_tests(TestSuite *suite);

// Test reporting
void export_test_results_csv(TestSuite *suite, const char *filename);
void export_test_results_json(TestSuite *suite, const char *filename);
void print_test_report(TestSuite *suite);

// Test configuration
typedef struct
{
  bool verbose;
  bool stop_on_failure;
  bool run_performance_tests;
  int performance_iterations;
  const char *output_format;
} TestConfig;

TestConfig get_default_test_config(void);
TestConfig get_quick_test_config(void);
TestConfig get_thorough_test_config(void);

#endif // SEVEN_T_UNIT_TEST_FRAMEWORK_H