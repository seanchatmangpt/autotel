#include "7t_unit_test_framework.h"
#include <sys/time.h>
#include <sys/resource.h>

// Global test configuration
TestConfig test_config = {
    .verbose_output = false,
    .stop_on_failure = false,
    .run_performance_tests = true,
    .run_memory_tests = true,
    .run_stress_tests = false,
    .performance_threshold_cycles = 1000,
    .latency_threshold_ns = 1000000,       // 1ms
    .memory_threshold_bytes = 1024 * 1024, // 1MB
    .max_test_iterations = 1000};

// Test suite management
TestSuite *test_suite_create(const char *suite_name)
{
  TestSuite *suite = malloc(sizeof(TestSuite));
  if (!suite)
    return NULL;

  suite->suite_name = suite_name;
  suite->result_capacity = 100;
  suite->results = malloc(suite->result_capacity * sizeof(TestResult));
  if (!suite->results)
  {
    free(suite);
    return NULL;
  }

  suite->result_count = 0;
  suite->total_time_ns = 0;
  suite->total_memory_bytes = 0;
  suite->passed_count = 0;
  suite->failed_count = 0;
  suite->skipped_count = 0;
  suite->error_count = 0;

  return suite;
}

void test_suite_destroy(TestSuite *suite)
{
  if (suite)
  {
    free(suite->results);
    free(suite);
  }
}

void test_suite_add_result(TestSuite *suite, TestResult result)
{
  if (suite->result_count >= suite->result_capacity)
  {
    size_t new_capacity = suite->result_capacity * 2;
    TestResult *new_results = realloc(suite->results, new_capacity * sizeof(TestResult));
    if (!new_results)
      return;

    suite->results = new_results;
    suite->result_capacity = new_capacity;
  }

  suite->results[suite->result_count++] = result;
  suite->total_time_ns += result.execution_time_ns;
  suite->total_memory_bytes += result.memory_usage_bytes;

  switch (result.status)
  {
  case TEST_PASS:
    suite->passed_count++;
    break;
  case TEST_FAIL:
    suite->failed_count++;
    break;
  case TEST_SKIP:
    suite->skipped_count++;
    break;
  case TEST_ERROR:
    suite->error_count++;
    break;
  }
}

void test_suite_print_summary(TestSuite *suite)
{
  printf("\n=== Test Suite Summary: %s ===\n", suite->suite_name);
  printf("Total tests: %zu\n", suite->result_count);
  printf("Passed: %d\n", suite->passed_count);
  printf("Failed: %d\n", suite->failed_count);
  printf("Skipped: %d\n", suite->skipped_count);
  printf("Errors: %d\n", suite->error_count);
  printf("Total time: %.3f ms\n", suite->total_time_ns / 1000000.0);
  printf("Total memory: %.2f KB\n", suite->total_memory_bytes / 1024.0);

  double success_rate = (double)suite->passed_count / suite->result_count * 100.0;
  printf("Success rate: %.1f%%\n", success_rate);

  if (suite->failed_count == 0 && suite->error_count == 0)
  {
    printf("Status: ✅ PASSED\n");
  }
  else
  {
    printf("Status: ❌ FAILED\n");
  }
}

void test_suite_print_detailed(TestSuite *suite)
{
  printf("\n=== Detailed Test Results: %s ===\n", suite->suite_name);

  for (size_t i = 0; i < suite->result_count; i++)
  {
    TestResult *result = &suite->results[i];

    printf("\nTest: %s\n", result->test_name);
    printf("Status: ");
    switch (result->status)
    {
    case TEST_PASS:
      printf("✅ PASSED");
      break;
    case TEST_FAIL:
      printf("❌ FAILED");
      break;
    case TEST_SKIP:
      printf("⏭️  SKIPPED");
      break;
    case TEST_ERROR:
      printf("💥 ERROR");
      break;
    }
    printf("\n");

    printf("Execution time: %.3f ms\n", result->execution_time_ns / 1000000.0);
    printf("Memory usage: %.2f KB\n", result->memory_usage_bytes / 1024.0);

    if (result->error_message)
    {
      printf("Error: %s\n", result->error_message);
    }

    if (result->file_name && result->line_number > 0)
    {
      printf("Location: %s:%d\n", result->file_name, result->line_number);
    }
  }

  test_suite_print_summary(suite);
}

// Test execution
TestResult test_execute_single(TestCase *test_case, TestContext *context)
{
  TestResult result = {0};
  result.test_name = test_case->test_name;
  result.test_suite = "unknown";
  result.status = TEST_PASS;
  result.error_message = NULL;
  result.execution_time_ns = 0;
  result.memory_usage_bytes = 0;
  result.line_number = 0;
  result.file_name = NULL;

  if (!test_case->enabled)
  {
    result.status = TEST_SKIP;
    result.error_message = "Test disabled";
    return result;
  }

  // Initialize context
  if (context)
  {
    context->test_data = NULL;
    context->setup_data = NULL;
    context->teardown_data = NULL;
    context->setup_called = false;
    context->teardown_called = false;
  }

  // Measure memory before
  size_t memory_before = get_memory_usage();

  // Start timing
  uint64_t start_time = get_nanoseconds();

  // Run setup if provided
  if (test_case->setup_func && context)
  {
    context->setup_called = true;
    test_case->setup_func(context);
  }

  // Run test
  if (test_case->test_func)
  {
    test_case->test_func(context);
  }

  // Run teardown if provided
  if (test_case->teardown_func && context)
  {
    context->teardown_called = true;
    test_case->teardown_func(context);
  }

  // End timing
  uint64_t end_time = get_nanoseconds();
  result.execution_time_ns = end_time - start_time;

  // Measure memory after
  size_t memory_after = get_memory_usage();
  result.memory_usage_bytes = memory_after - memory_before;

  return result;
}

void test_run_suite(TestSuiteRegistration *registration)
{
  if (!registration)
    return;

  TestSuite *suite = test_suite_create(registration->suite_name);
  if (!suite)
    return;

  printf("\n=== Running Test Suite: %s ===\n", registration->suite_name);

  // Run suite setup if provided
  if (registration->suite_setup)
  {
    TestContext context = {0};
    registration->suite_setup(&context);
  }

  // Run all test cases
  for (size_t i = 0; i < registration->test_case_count; i++)
  {
    TestCase *test_case = &registration->test_cases[i];

    if (test_config.verbose_output)
    {
      print_test_header(test_case->test_name);
    }

    TestContext context = {0};
    TestResult result = test_execute_single(test_case, &context);
    result.test_suite = registration->suite_name;

    test_suite_add_result(suite, result);

    if (test_config.verbose_output)
    {
      print_test_footer(test_case->test_name, result.status, result.execution_time_ns);
    }

    // Stop on failure if configured
    if (test_config.stop_on_failure &&
        (result.status == TEST_FAIL || result.status == TEST_ERROR))
    {
      break;
    }
  }

  // Run suite teardown if provided
  if (registration->suite_teardown)
  {
    TestContext context = {0};
    registration->suite_teardown(&context);
  }

  test_suite_print_detailed(suite);
  test_suite_destroy(suite);
}

// Utility functions
uint64_t get_nanoseconds(void)
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

size_t get_memory_usage(void)
{
  struct rusage usage;
  if (getrusage(RUSAGE_SELF, &usage) == 0)
  {
    return usage.ru_maxrss * 1024; // Convert KB to bytes
  }
  return 0;
}

void print_test_header(const char *test_name)
{
  printf("Running test: %s\n", test_name);
}

void print_test_footer(const char *test_name, TestStatus status, uint64_t time_ns)
{
  const char *status_str;
  switch (status)
  {
  case TEST_PASS:
    status_str = "PASSED";
    break;
  case TEST_FAIL:
    status_str = "FAILED";
    break;
  case TEST_SKIP:
    status_str = "SKIPPED";
    break;
  case TEST_ERROR:
    status_str = "ERROR";
    break;
  }

  printf("Test %s: %s (%.3f ms)\n", test_name, status_str, time_ns / 1000000.0);
}

// Test data generation
void *generate_test_data(size_t size)
{
  void *data = malloc(size);
  if (data)
  {
    // Fill with random data
    for (size_t i = 0; i < size; i++)
    {
      ((char *)data)[i] = rand() % 256;
    }
  }
  return data;
}

void destroy_test_data(void *data)
{
  free(data);
}

char *generate_random_string(size_t length)
{
  char *str = malloc(length + 1);
  if (!str)
    return NULL;

  const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  for (size_t i = 0; i < length; i++)
  {
    str[i] = charset[rand() % (sizeof(charset) - 1)];
  }
  str[length] = '\0';

  return str;
}

uint32_t *generate_random_uints(size_t count, uint32_t max_value)
{
  uint32_t *data = malloc(count * sizeof(uint32_t));
  if (!data)
    return NULL;

  for (size_t i = 0; i < count; i++)
  {
    data[i] = rand() % max_value;
  }

  return data;
}

// Mock function implementation
MockFunction *mock_function_create(const char *function_name)
{
  MockFunction *mock = malloc(sizeof(MockFunction));
  if (mock)
  {
    mock->function_name = function_name;
    mock->call_count = 0;
    mock->return_value = NULL;
    mock->should_fail = false;
  }
  return mock;
}

void mock_function_destroy(MockFunction *mock)
{
  free(mock);
}

void mock_function_set_return_value(MockFunction *mock, void *return_value)
{
  if (mock)
  {
    mock->return_value = return_value;
  }
}

void mock_function_set_should_fail(MockFunction *mock, bool should_fail)
{
  if (mock)
  {
    mock->should_fail = should_fail;
  }
}

int mock_function_get_call_count(MockFunction *mock)
{
  return mock ? mock->call_count : 0;
}

void mock_function_reset(MockFunction *mock)
{
  if (mock)
  {
    mock->call_count = 0;
    mock->return_value = NULL;
    mock->should_fail = false;
  }
}

// Test fixture implementation
TestFixture *test_fixture_create(const char *name, void *data, size_t data_size)
{
  TestFixture *fixture = malloc(sizeof(TestFixture));
  if (fixture)
  {
    fixture->name = name;
    fixture->data = data;
    fixture->data_size = data_size;
  }
  return fixture;
}

void test_fixture_destroy(TestFixture *fixture)
{
  if (fixture)
  {
    free(fixture->data);
    free(fixture);
  }
}

void *test_fixture_get_data(TestFixture *fixture)
{
  return fixture ? fixture->data : NULL;
}

// Test configuration functions
void test_config_init(void)
{
  // Initialize with default values (already set in global variable)
  srand(time(NULL)); // Initialize random seed
}

void test_config_set_verbose(bool verbose)
{
  test_config.verbose_output = verbose;
}

void test_config_set_stop_on_failure(bool stop_on_failure)
{
  test_config.stop_on_failure = stop_on_failure;
}

void test_config_set_performance_threshold(uint64_t cycles)
{
  test_config.performance_threshold_cycles = cycles;
}

void test_config_set_latency_threshold(uint64_t ns)
{
  test_config.latency_threshold_ns = ns;
}

void test_config_set_memory_threshold(size_t bytes)
{
  test_config.memory_threshold_bytes = bytes;
}

// Test reporting implementation
TestReport *test_report_create(void)
{
  TestReport *report = malloc(sizeof(TestReport));
  if (report)
  {
    report->total_tests = 0;
    report->passed_tests = 0;
    report->failed_tests = 0;
    report->skipped_tests = 0;
    report->error_tests = 0;
    report->total_time_ns = 0;
    report->total_memory_bytes = 0;
    report->success_rate = 0.0;
  }
  return report;
}

void test_report_destroy(TestReport *report)
{
  free(report);
}

void test_report_add_result(TestReport *report, TestResult result)
{
  if (!report)
    return;

  report->total_tests++;
  report->total_time_ns += result.execution_time_ns;
  report->total_memory_bytes += result.memory_usage_bytes;

  switch (result.status)
  {
  case TEST_PASS:
    report->passed_tests++;
    break;
  case TEST_FAIL:
    report->failed_tests++;
    break;
  case TEST_SKIP:
    report->skipped_tests++;
    break;
  case TEST_ERROR:
    report->error_tests++;
    break;
  }

  report->success_rate = (double)report->passed_tests / report->total_tests * 100.0;
}

void test_report_print_summary(TestReport *report)
{
  if (!report)
    return;

  printf("\n=== Test Report Summary ===\n");
  printf("Total tests: %d\n", report->total_tests);
  printf("Passed: %d\n", report->passed_tests);
  printf("Failed: %d\n", report->failed_tests);
  printf("Skipped: %d\n", report->skipped_tests);
  printf("Errors: %d\n", report->error_tests);
  printf("Success rate: %.1f%%\n", report->success_rate);
  printf("Total time: %.3f ms\n", report->total_time_ns / 1000000.0);
  printf("Total memory: %.2f KB\n", report->total_memory_bytes / 1024.0);
}

void test_report_export_json(TestReport *report, const char *filename)
{
  if (!report || !filename)
    return;

  FILE *file = fopen(filename, "w");
  if (!file)
    return;

  fprintf(file, "{\n");
  fprintf(file, "  \"test_report\": {\n");
  fprintf(file, "    \"total_tests\": %d,\n", report->total_tests);
  fprintf(file, "    \"passed_tests\": %d,\n", report->passed_tests);
  fprintf(file, "    \"failed_tests\": %d,\n", report->failed_tests);
  fprintf(file, "    \"skipped_tests\": %d,\n", report->skipped_tests);
  fprintf(file, "    \"error_tests\": %d,\n", report->error_tests);
  fprintf(file, "    \"success_rate\": %.1f,\n", report->success_rate);
  fprintf(file, "    \"total_time_ns\": %llu,\n", report->total_time_ns);
  fprintf(file, "    \"total_memory_bytes\": %zu\n", report->total_memory_bytes);
  fprintf(file, "  }\n");
  fprintf(file, "}\n");

  fclose(file);
}

void test_report_export_junit_xml(TestReport *report, const char *filename)
{
  if (!report || !filename)
    return;

  FILE *file = fopen(filename, "w");
  if (!file)
    return;

  fprintf(file, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
  fprintf(file, "<testsuites>\n");
  fprintf(file, "  <testsuite name=\"7T Engine Tests\" tests=\"%d\" failures=\"%d\" errors=\"%d\" skipped=\"%d\">\n",
          report->total_tests, report->failed_tests, report->error_tests, report->skipped_tests);

  // Note: Individual test results would be added here in a real implementation

  fprintf(file, "  </testsuite>\n");
  fprintf(file, "</testsuites>\n");

  fclose(file);
}