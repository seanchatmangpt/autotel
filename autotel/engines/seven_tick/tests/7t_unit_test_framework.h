#ifndef SEVEN_TICK_UNIT_TEST_FRAMEWORK_H
#define SEVEN_TICK_UNIT_TEST_FRAMEWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <stdint.h>
#include <stdbool.h>

// Test framework version
#define SEVEN_TICK_TEST_VERSION "1.0.0"

// Test result status
typedef enum
{
  TEST_PASS,
  TEST_FAIL,
  TEST_SKIP,
  TEST_ERROR
} TestStatus;

// Test result structure
typedef struct
{
  const char *test_name;
  const char *test_suite;
  TestStatus status;
  const char *error_message;
  uint64_t execution_time_ns;
  size_t memory_usage_bytes;
  int line_number;
  const char *file_name;
} TestResult;

// Test suite structure
typedef struct
{
  const char *suite_name;
  TestResult *results;
  size_t result_count;
  size_t result_capacity;
  uint64_t total_time_ns;
  size_t total_memory_bytes;
  int passed_count;
  int failed_count;
  int skipped_count;
  int error_count;
} TestSuite;

// Test context for setup/teardown
typedef struct
{
  void *test_data;
  void *setup_data;
  void *teardown_data;
  bool setup_called;
  bool teardown_called;
} TestContext;

// Test function types
typedef void (*TestFunction)(TestContext *context);
typedef void (*SetupFunction)(TestContext *context);
typedef void (*TeardownFunction)(TestContext *context);

// Test case structure
typedef struct
{
  const char *test_name;
  TestFunction test_func;
  SetupFunction setup_func;
  TeardownFunction teardown_func;
  const char *description;
  bool enabled;
} TestCase;

// Test suite registration
typedef struct
{
  const char *suite_name;
  TestCase *test_cases;
  size_t test_case_count;
  SetupFunction suite_setup;
  TeardownFunction suite_teardown;
} TestSuiteRegistration;

// Assertion macros for void functions
#define ASSERT_TRUE(condition)                                                     \
  do                                                                               \
  {                                                                                \
    if (!(condition))                                                              \
    {                                                                              \
      printf("ASSERT_TRUE failed at %s:%d: %s\n", __FILE__, __LINE__, #condition); \
    }                                                                              \
  } while (0)

#define ASSERT_FALSE(condition)                                                     \
  do                                                                                \
  {                                                                                 \
    if ((condition))                                                                \
    {                                                                               \
      printf("ASSERT_FALSE failed at %s:%d: %s\n", __FILE__, __LINE__, #condition); \
    }                                                                               \
  } while (0)

#define ASSERT_EQUAL(expected, actual)                                        \
  do                                                                          \
  {                                                                           \
    if ((expected) != (actual))                                               \
    {                                                                         \
      printf("ASSERT_EQUAL failed at %s:%d: expected %lld, got %lld\n",       \
             __FILE__, __LINE__, (long long)(expected), (long long)(actual)); \
    }                                                                         \
  } while (0)

#define ASSERT_NOT_EQUAL(expected, actual)                               \
  do                                                                     \
  {                                                                      \
    if ((expected) == (actual))                                          \
    {                                                                    \
      printf("ASSERT_NOT_EQUAL failed at %s:%d: both values are %lld\n", \
             __FILE__, __LINE__, (long long)(expected));                 \
    }                                                                    \
  } while (0)

#define ASSERT_STRING_EQUAL(expected, actual)                                  \
  do                                                                           \
  {                                                                            \
    if (strcmp((expected), (actual)) != 0)                                     \
    {                                                                          \
      printf("ASSERT_STRING_EQUAL failed at %s:%d: expected '%s', got '%s'\n", \
             __FILE__, __LINE__, (expected), (actual));                        \
    }                                                                          \
  } while (0)

#define ASSERT_NULL(ptr)                                           \
  do                                                               \
  {                                                                \
    if ((ptr) != NULL)                                             \
    {                                                              \
      printf("ASSERT_NULL failed at %s:%d: pointer is not NULL\n", \
             __FILE__, __LINE__);                                  \
    }                                                              \
  } while (0)

#define ASSERT_NOT_NULL(ptr)                                       \
  do                                                               \
  {                                                                \
    if ((ptr) == NULL)                                             \
    {                                                              \
      printf("ASSERT_NOT_NULL failed at %s:%d: pointer is NULL\n", \
             __FILE__, __LINE__);                                  \
    }                                                              \
  } while (0)

#define ASSERT_GREATER_THAN(value, threshold)                                        \
  do                                                                                 \
  {                                                                                  \
    if ((value) <= (threshold))                                                      \
    {                                                                                \
      printf("ASSERT_GREATER_THAN failed at %s:%d: %lld is not greater than %lld\n", \
             __FILE__, __LINE__, (long long)(value), (long long)(threshold));        \
    }                                                                                \
  } while (0)

#define ASSERT_LESS_THAN(value, threshold)                                     \
  do                                                                           \
  {                                                                            \
    if ((value) >= (threshold))                                                \
    {                                                                          \
      printf("ASSERT_LESS_THAN failed at %s:%d: %lld is not less than %lld\n", \
             __FILE__, __LINE__, (long long)(value), (long long)(threshold));  \
    }                                                                          \
  } while (0)

#define ASSERT_IN_RANGE(value, min, max)                                                  \
  do                                                                                      \
  {                                                                                       \
    if ((value) < (min) || (value) > (max))                                               \
    {                                                                                     \
      printf("ASSERT_IN_RANGE failed at %s:%d: %lld is not in range [%lld, %lld]\n",      \
             __FILE__, __LINE__, (long long)(value), (long long)(min), (long long)(max)); \
    }                                                                                     \
  } while (0)

// Performance assertion macros
#define ASSERT_PERFORMANCE(operation, max_cycles)                                       \
  do                                                                                    \
  {                                                                                     \
    uint64_t start_cycles = __builtin_readcyclecounter();                               \
    operation;                                                                          \
    uint64_t end_cycles = __builtin_readcyclecounter();                                 \
    uint64_t cycles = end_cycles - start_cycles;                                        \
    if (cycles > (max_cycles))                                                          \
    {                                                                                   \
      printf("ASSERT_PERFORMANCE failed at %s:%d: %llu cycles exceeds limit of %llu\n", \
             __FILE__, __LINE__, cycles, (uint64_t)(max_cycles));                       \
    }                                                                                   \
  } while (0)

#define ASSERT_LATENCY(operation, max_ns)                                                      \
  do                                                                                           \
  {                                                                                            \
    struct timespec start, end;                                                                \
    clock_gettime(CLOCK_MONOTONIC, &start);                                                    \
    operation;                                                                                 \
    clock_gettime(CLOCK_MONOTONIC, &end);                                                      \
    uint64_t ns = (end.tv_sec - start.tv_sec) * 1000000000ULL + (end.tv_nsec - start.tv_nsec); \
    if (ns > (max_ns))                                                                         \
    {                                                                                          \
      printf("ASSERT_LATENCY failed at %s:%d: %llu ns exceeds limit of %llu\n",                \
             __FILE__, __LINE__, ns, (uint64_t)(max_ns));                                      \
    }                                                                                          \
  } while (0)

// Memory assertion macros
#define ASSERT_MEMORY_USAGE(operation, max_bytes)                                     \
  do                                                                                  \
  {                                                                                   \
    size_t start_memory = get_memory_usage();                                         \
    operation;                                                                        \
    size_t end_memory = get_memory_usage();                                           \
    size_t memory_used = end_memory - start_memory;                                   \
    if (memory_used > (max_bytes))                                                    \
    {                                                                                 \
      printf("ASSERT_MEMORY_USAGE failed at %s:%d: %zu bytes exceeds limit of %zu\n", \
             __FILE__, __LINE__, memory_used, (size_t)(max_bytes));                   \
    }                                                                                 \
  } while (0)

// Test framework functions
TestSuite *test_suite_create(const char *suite_name);
void test_suite_destroy(TestSuite *suite);
void test_suite_add_result(TestSuite *suite, TestResult result);
void test_suite_print_summary(TestSuite *suite);
void test_suite_print_detailed(TestSuite *suite);

// Test execution functions
TestResult test_execute_single(TestCase *test_case, TestContext *context);
void test_run_suite(TestSuiteRegistration *registration);

// Utility functions
uint64_t get_nanoseconds(void);
size_t get_memory_usage(void);
void print_test_header(const char *test_name);
void print_test_footer(const char *test_name, TestStatus status, uint64_t time_ns);

// Test registration macros
#define TEST_SUITE_BEGIN(suite_name)                  \
  TestSuiteRegistration suite_name##_registration = { \
      .suite_name = #suite_name,                      \
      .test_cases = NULL,                             \
      .test_case_count = 0,                           \
      .suite_setup = NULL,                            \
      .suite_teardown = NULL};

#define TEST_CASE(test_name, description)             \
  static void test_##test_name(TestContext *context); \
  static TestCase test_case_##test_name = {           \
      .test_name = #test_name,                        \
      .test_func = test_##test_name,                  \
      .setup_func = NULL,                             \
      .teardown_func = NULL,                          \
      .description = description,                     \
      .enabled = true};                               \
  static void test_##test_name(TestContext *context)

#define TEST_CASE_WITH_SETUP(test_name, setup_func, teardown_func, description) \
  static void test_##test_name(TestContext *context);                           \
  static TestCase test_case_##test_name = {                                     \
      .test_name = #test_name,                                                  \
      .test_func = test_##test_name,                                            \
      .setup_func = setup_func,                                                 \
      .teardown_func = teardown_func,                                           \
      .description = description,                                               \
      .enabled = true};                                                         \
  static void test_##test_name(TestContext *context)

#define TEST_SUITE_END(suite_name)         \
  void register_##suite_name##_tests(void) \
  {                                        \
    /* Register test cases here */         \
  }

// Test runner
int run_all_tests(void);
int run_test_suite(const char *suite_name);
int run_specific_test(const char *suite_name, const char *test_name);

// Test data generation
void *generate_test_data(size_t size);
void destroy_test_data(void *data);
char *generate_random_string(size_t length);
uint32_t *generate_random_uints(size_t count, uint32_t max_value);

// Mock and stub functions
typedef struct
{
  const char *function_name;
  int call_count;
  void *return_value;
  bool should_fail;
} MockFunction;

MockFunction *mock_function_create(const char *function_name);
void mock_function_destroy(MockFunction *mock);
void mock_function_set_return_value(MockFunction *mock, void *return_value);
void mock_function_set_should_fail(MockFunction *mock, bool should_fail);
int mock_function_get_call_count(MockFunction *mock);
void mock_function_reset(MockFunction *mock);

// Test fixtures
typedef struct
{
  void *data;
  size_t data_size;
  const char *name;
} TestFixture;

TestFixture *test_fixture_create(const char *name, void *data, size_t data_size);
void test_fixture_destroy(TestFixture *fixture);
void *test_fixture_get_data(TestFixture *fixture);

// Test categories
typedef enum
{
  TEST_CATEGORY_UNIT,
  TEST_CATEGORY_INTEGRATION,
  TEST_CATEGORY_PERFORMANCE,
  TEST_CATEGORY_MEMORY,
  TEST_CATEGORY_STRESS,
  TEST_CATEGORY_REGRESSION
} TestCategory;

// Test configuration
typedef struct
{
  bool verbose_output;
  bool stop_on_failure;
  bool run_performance_tests;
  bool run_memory_tests;
  bool run_stress_tests;
  uint64_t performance_threshold_cycles;
  uint64_t latency_threshold_ns;
  size_t memory_threshold_bytes;
  int max_test_iterations;
} TestConfig;

extern TestConfig test_config;

// Initialize test configuration
void test_config_init(void);
void test_config_set_verbose(bool verbose);
void test_config_set_stop_on_failure(bool stop_on_failure);
void test_config_set_performance_threshold(uint64_t cycles);
void test_config_set_latency_threshold(uint64_t ns);
void test_config_set_memory_threshold(size_t bytes);

// Test reporting
typedef struct
{
  int total_tests;
  int passed_tests;
  int failed_tests;
  int skipped_tests;
  int error_tests;
  uint64_t total_time_ns;
  size_t total_memory_bytes;
  double success_rate;
} TestReport;

TestReport *test_report_create(void);
void test_report_destroy(TestReport *report);
void test_report_add_result(TestReport *report, TestResult result);
void test_report_print_summary(TestReport *report);
void test_report_export_json(TestReport *report, const char *filename);
void test_report_export_junit_xml(TestReport *report, const char *filename);

#endif // SEVEN_TICK_UNIT_TEST_FRAMEWORK_H