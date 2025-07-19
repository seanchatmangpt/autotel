#include "7t_unit_test_framework.h"
#include "../runtime/src/seven_t_runtime.h"
#include "../compiler/src/cjinja.h"

// Test suite management
TestSuite *create_test_suite(const char *name)
{
  TestSuite *suite = malloc(sizeof(TestSuite));
  if (!suite)
    return NULL;

  suite->suite_name = name;
  suite->result_capacity = 50;
  suite->result_count = 0;
  suite->results = malloc(suite->result_capacity * sizeof(TestResult));
  suite->total_tests = 0;
  suite->passed_tests = 0;
  suite->failed_tests = 0;

  return suite;
}

void destroy_test_suite(TestSuite *suite)
{
  if (suite)
  {
    free(suite->results);
    free(suite);
  }
}

void add_test_result(TestSuite *suite, TestResult result)
{
  if (suite->result_count >= suite->result_capacity)
  {
    suite->result_capacity *= 2;
    suite->results = realloc(suite->results, suite->result_capacity * sizeof(TestResult));
  }

  suite->results[suite->result_count++] = result;
  suite->total_tests++;

  if (result.passed)
  {
    suite->passed_tests++;
  }
  else
  {
    suite->failed_tests++;
  }
}

// Test result creation
TestResult create_test_result(const char *test_name, const char *test_suite,
                              bool passed, const char *failure_message, double execution_time_ms)
{
  return (TestResult){
      .test_name = test_name,
      .test_suite = test_suite,
      .passed = passed,
      .failure_message = failure_message,
      .execution_time_ms = execution_time_ms};
}

double get_execution_time_ms(clock_t start, clock_t end)
{
  return ((double)(end - start) / CLOCKS_PER_SEC) * 1000.0;
}

// Test execution
void run_test(TestSuite *suite, const char *test_name, TestFunction test_func)
{
  printf("🧪 Running test: %s\n", test_name);

  clock_t start = clock();
  test_func();
  clock_t end = clock();

  double execution_time = get_execution_time_ms(start, end);
  TestResult result = create_test_result(test_name, suite->suite_name, true, NULL, execution_time);

  add_test_result(suite, result);
  printf("✅ Test passed: %s (%.2f ms)\n", test_name, execution_time);
}

void run_test_with_context(TestSuite *suite, const char *test_name,
                           void (*test_func)(TestContext *), TestContext *context)
{
  printf("🧪 Running test: %s\n", test_name);

  clock_t start = clock();
  test_func(context);
  clock_t end = clock();

  double execution_time = get_execution_time_ms(start, end);
  TestResult result = create_test_result(test_name, suite->suite_name, true, NULL, execution_time);

  add_test_result(suite, result);
  printf("✅ Test passed: %s (%.2f ms)\n", test_name, execution_time);
}

// Test utilities
void *create_test_engine(void)
{
  return s7t_create_engine();
}

void destroy_test_engine(void *engine)
{
  if (engine)
  {
    s7t_destroy_engine((EngineState *)engine);
  }
}

void *create_test_data(void)
{
  // Create some test data structure
  return malloc(1024);
}

void destroy_test_data(void *data)
{
  if (data)
  {
    free(data);
  }
}

// Test configuration
TestConfig get_default_test_config(void)
{
  return (TestConfig){
      .verbose = true,
      .stop_on_failure = false,
      .run_performance_tests = true,
      .performance_iterations = 100000,
      .output_format = "console"};
}

TestConfig get_quick_test_config(void)
{
  return (TestConfig){
      .verbose = false,
      .stop_on_failure = true,
      .run_performance_tests = false,
      .performance_iterations = 1000,
      .output_format = "console"};
}

TestConfig get_thorough_test_config(void)
{
  return (TestConfig){
      .verbose = true,
      .stop_on_failure = false,
      .run_performance_tests = true,
      .performance_iterations = 1000000,
      .output_format = "all"};
}

// Test reporting
void print_test_suite(TestSuite *suite)
{
  printf("\n=== %s Test Suite ===\n", suite->suite_name);
  printf("Total tests: %d\n", suite->total_tests);
  printf("Passed: %d\n", suite->passed_tests);
  printf("Failed: %d\n", suite->failed_tests);
  printf("Success rate: %.1f%%\n",
         suite->total_tests > 0 ? (double)suite->passed_tests / suite->total_tests * 100 : 0);

  if (suite->failed_tests > 0)
  {
    printf("\nFailed tests:\n");
    for (int i = 0; i < suite->result_count; i++)
    {
      if (!suite->results[i].passed)
      {
        printf("  ❌ %s: %s\n", suite->results[i].test_name,
               suite->results[i].failure_message ? suite->results[i].failure_message : "Unknown error");
      }
    }
  }
}

void print_test_summary(TestSuite *suite)
{
  printf("\n📊 Test Summary: %s\n", suite->suite_name);
  printf("=====================\n");
  printf("Total: %d | Passed: %d | Failed: %d\n",
         suite->total_tests, suite->passed_tests, suite->failed_tests);

  if (suite->total_tests > 0)
  {
    double success_rate = (double)suite->passed_tests / suite->total_tests * 100;
    if (success_rate == 100.0)
    {
      printf("🎉 All tests passed! (100%%)\n");
    }
    else if (success_rate >= 90.0)
    {
      printf("✅ Excellent test results! (%.1f%%)\n", success_rate);
    }
    else if (success_rate >= 80.0)
    {
      printf("⚠️ Good test results (%.1f%%)\n", success_rate);
    }
    else
    {
      printf("❌ Poor test results (%.1f%%)\n", success_rate);
    }
  }
}

void export_test_results_csv(TestSuite *suite, const char *filename)
{
  FILE *file = fopen(filename, "w");
  if (!file)
    return;

  fprintf(file, "Test Name,Test Suite,Passed,Execution Time (ms),Failure Message\n");

  for (int i = 0; i < suite->result_count; i++)
  {
    TestResult *r = &suite->results[i];
    fprintf(file, "%s,%s,%s,%.2f,%s\n",
            r->test_name, r->test_suite,
            r->passed ? "true" : "false",
            r->execution_time_ms,
            r->failure_message ? r->failure_message : "");
  }

  fclose(file);
}

void export_test_results_json(TestSuite *suite, const char *filename)
{
  FILE *file = fopen(filename, "w");
  if (!file)
    return;

  fprintf(file, "{\n");
  fprintf(file, "  \"suite_name\": \"%s\",\n", suite->suite_name);
  fprintf(file, "  \"total_tests\": %d,\n", suite->total_tests);
  fprintf(file, "  \"passed_tests\": %d,\n", suite->passed_tests);
  fprintf(file, "  \"failed_tests\": %d,\n", suite->failed_tests);
  fprintf(file, "  \"success_rate\": %.1f,\n",
          suite->total_tests > 0 ? (double)suite->passed_tests / suite->total_tests * 100 : 0);
  fprintf(file, "  \"results\": [\n");

  for (int i = 0; i < suite->result_count; i++)
  {
    TestResult *r = &suite->results[i];
    fprintf(file, "    {\n");
    fprintf(file, "      \"test_name\": \"%s\",\n", r->test_name);
    fprintf(file, "      \"test_suite\": \"%s\",\n", r->test_suite);
    fprintf(file, "      \"passed\": %s,\n", r->passed ? "true" : "false");
    fprintf(file, "      \"execution_time_ms\": %.2f,\n", r->execution_time_ms);
    fprintf(file, "      \"failure_message\": \"%s\"\n",
            r->failure_message ? r->failure_message : "");
    fprintf(file, "    }%s\n", i < suite->result_count - 1 ? "," : "");
  }

  fprintf(file, "  ]\n");
  fprintf(file, "}\n");

  fclose(file);
}

void print_test_report(TestSuite *suite)
{
  print_test_suite(suite);
  print_test_summary(suite);

  // Export results if configured
  char csv_filename[256];
  char json_filename[256];
  snprintf(csv_filename, sizeof(csv_filename), "%s_test_results.csv", suite->suite_name);
  snprintf(json_filename, sizeof(json_filename), "%s_test_results.json", suite->suite_name);

  export_test_results_csv(suite, csv_filename);
  export_test_results_json(suite, json_filename);

  printf("\n📄 Test results exported to:\n");
  printf("  - %s\n", csv_filename);
  printf("  - %s\n", json_filename);
}