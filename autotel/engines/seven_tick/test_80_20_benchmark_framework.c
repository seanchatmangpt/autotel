#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "benchmark_framework.h"

// 80/20 Unit Tests for Benchmark Framework
// Focus: Critical functionality, timing accuracy, result tracking

// Test utilities
#define TEST_ASSERT(condition, message) \
  do                                    \
  {                                     \
    if (!(condition))                   \
    {                                   \
      printf("❌ FAIL: %s\n", message); \
      return 0;                         \
    }                                   \
    else                                \
    {                                   \
      printf("✅ PASS: %s\n", message); \
    }                                   \
  } while (0)

#define TEST_SECTION(name) printf("\n=== %s ===\n", name)

// Test data structure
typedef struct
{
  int value;
  int iterations;
} TestData;

// Test functions for benchmarking
void benchmark_simple_operation(void *data)
{
  TestData *test_data = (TestData *)data;
  test_data->value++; // Simple increment operation
}

void benchmark_memory_operation(void *data)
{
  TestData *test_data = (TestData *)data;
  char *buffer = malloc(100);
  memset(buffer, 'A', 100);
  test_data->value += buffer[50];
  free(buffer);
}

void benchmark_string_operation(void *data)
{
  TestData *test_data = (TestData *)data;
  char str[100];
  sprintf(str, "Test value: %d", test_data->value);
  test_data->value += strlen(str);
}

// Test 1: Basic framework functionality
int test_basic_framework()
{
  TEST_SECTION("Basic Framework Functionality");

  // Test suite creation
  BenchmarkSuite *suite = benchmark_suite_create("Test Suite");
  TEST_ASSERT(suite != NULL, "Suite creation");
  TEST_ASSERT(suite->result_count == 0, "Initial result count");
  TEST_ASSERT(suite->result_capacity >= 20, "Initial capacity");
  TEST_ASSERT(strcmp(suite->suite_name, "Test Suite") == 0, "Suite name");

  // Test data setup
  TestData test_data = {0, 1000};

  // Test single benchmark execution
  BenchmarkResult result = benchmark_execute_single(
      "Simple Test",
      1000,
      benchmark_simple_operation,
      &test_data);

  TEST_ASSERT(result.test_name != NULL, "Result test name not null");
  TEST_ASSERT(strcmp(result.test_name, "Simple Test") == 0, "Result test name");
  TEST_ASSERT(result.operations == 1000, "Result operations count");
  TEST_ASSERT(result.total_cycles > 0, "Result total cycles");
  TEST_ASSERT(result.total_time_ns > 0, "Result total time");
  TEST_ASSERT(result.avg_cycles_per_op > 0, "Result average cycles");
  TEST_ASSERT(result.avg_time_ns_per_op > 0, "Result average time");
  TEST_ASSERT(result.ops_per_sec > 0, "Result operations per second");

  // Test suite result addition
  benchmark_suite_add_result(suite, result);
  TEST_ASSERT(suite->result_count == 1, "Suite result count after addition");
  TEST_ASSERT(suite->total_suite_time_ns > 0, "Suite total time");

  // Test suite statistics calculation
  benchmark_suite_calculate_stats(suite);
  TEST_ASSERT(suite->overall_score >= 0, "Suite overall score");

  // Cleanup
  benchmark_suite_destroy(suite);

  return 1;
}

// Test 2: Timing accuracy
int test_timing_accuracy()
{
  TEST_SECTION("Timing Accuracy");

  // Test timer functionality
  BenchmarkTimer timer;
  benchmark_timer_start(&timer, "Test Timer");

  // Perform some work
  volatile int sum = 0;
  for (int i = 0; i < 1000; i++)
  {
    sum += i;
  }

  benchmark_timer_end(&timer);

  uint64_t cycles = benchmark_timer_get_cycles(&timer);
  uint64_t time_ns = benchmark_timer_get_time_ns(&timer);

  TEST_ASSERT(cycles > 0, "Timer cycles measurement");
  TEST_ASSERT(time_ns > 0, "Timer time measurement");
  TEST_ASSERT(cycles >= time_ns / 1000, "Cycles reasonable compared to time");

  // Test multiple timing operations
  uint64_t total_cycles = 0;
  uint64_t total_time = 0;

  for (int i = 0; i < 10; i++)
  {
    benchmark_timer_start(&timer, "Multiple Timer");

    // Different work loads
    volatile int work = 0;
    for (int j = 0; j < 100 * (i + 1); j++)
    {
      work += j;
    }

    benchmark_timer_end(&timer);
    total_cycles += benchmark_timer_get_cycles(&timer);
    total_time += benchmark_timer_get_time_ns(&timer);
  }

  TEST_ASSERT(total_cycles > 0, "Multiple timer cycles");
  TEST_ASSERT(total_time > 0, "Multiple timer time");

  return 1;
}

// Test 3: Result validation
int test_result_validation()
{
  TEST_SECTION("Result Validation");

  // Create test results
  BenchmarkResult fast_result = {
      .test_name = "Fast Test",
      .total_cycles = 5000,
      .total_time_ns = 5000,
      .operations = 1000,
      .avg_cycles_per_op = 5.0,
      .avg_time_ns_per_op = 5.0,
      .ops_per_sec = 200000000.0,
      .operations_within_target = 1000,
      .target_achievement_percent = 100.0,
      .passed = 1};

  BenchmarkResult slow_result = {
      .test_name = "Slow Test",
      .total_cycles = 50000,
      .total_time_ns = 50000,
      .operations = 1000,
      .avg_cycles_per_op = 50.0,
      .avg_time_ns_per_op = 50.0,
      .ops_per_sec = 20000000.0,
      .operations_within_target = 0,
      .target_achievement_percent = 0.0,
      .passed = 0};

  // Test target validation
  int fast_valid = benchmark_validate_target(&fast_result);
  int slow_valid = benchmark_validate_target(&slow_result);

  TEST_ASSERT(fast_valid == 1, "Fast result passes validation");
  TEST_ASSERT(slow_valid == 0, "Slow result fails validation");
  TEST_ASSERT(fast_result.passed == 1, "Fast result marked as passed");
  TEST_ASSERT(slow_result.passed == 0, "Slow result marked as failed");

  return 1;
}

// Test 4: Suite management
int test_suite_management()
{
  TEST_SECTION("Suite Management");

  BenchmarkSuite *suite = benchmark_suite_create("Management Test");
  TEST_ASSERT(suite != NULL, "Suite creation");

  // Add multiple results
  for (int i = 0; i < 25; i++)
  {
    BenchmarkResult result = {
        .test_name = "Test",
        .total_cycles = 1000 + i,
        .total_time_ns = 1000 + i,
        .operations = 100,
        .avg_cycles_per_op = 10.0 + i,
        .avg_time_ns_per_op = 10.0 + i,
        .ops_per_sec = 100000000.0 / (10.0 + i),
        .operations_within_target = 95,
        .target_achievement_percent = 95.0,
        .passed = 1};

    benchmark_suite_add_result(suite, result);
  }

  TEST_ASSERT(suite->result_count == 25, "Suite result count");
  TEST_ASSERT(suite->result_capacity >= 25, "Suite capacity expansion");

  // Test statistics calculation
  benchmark_suite_calculate_stats(suite);
  TEST_ASSERT(suite->overall_score > 0, "Suite overall score calculation");
  TEST_ASSERT(suite->total_suite_time_ns > 0, "Suite total time calculation");

  benchmark_suite_destroy(suite);

  return 1;
}

// Test 5: Export functionality
int test_export_functionality()
{
  TEST_SECTION("Export Functionality");

  BenchmarkSuite *suite = benchmark_suite_create("Export Test");
  TEST_ASSERT(suite != NULL, "Suite creation");

  // Add test results
  BenchmarkResult result = {
      .test_name = "Export Test",
      .total_cycles = 10000,
      .total_time_ns = 10000,
      .operations = 1000,
      .avg_cycles_per_op = 10.0,
      .avg_time_ns_per_op = 10.0,
      .ops_per_sec = 100000000.0,
      .operations_within_target = 950,
      .target_achievement_percent = 95.0,
      .passed = 1};

  benchmark_suite_add_result(suite, result);
  benchmark_suite_calculate_stats(suite);

  // Test JSON export
  const char *json_filename = "test_export.json";
  benchmark_suite_export_json(suite, json_filename);

  // Verify file was created
  FILE *json_file = fopen(json_filename, "r");
  TEST_ASSERT(json_file != NULL, "JSON file creation");

  if (json_file)
  {
    char buffer[1024];
    size_t bytes_read = fread(buffer, 1, sizeof(buffer) - 1, json_file);
    buffer[bytes_read] = '\0';
    fclose(json_file);

    TEST_ASSERT(bytes_read > 0, "JSON file content");
    TEST_ASSERT(strstr(buffer, "Export Test") != NULL, "JSON contains test name");
    TEST_ASSERT(strstr(buffer, "95.0") != NULL, "JSON contains achievement percent");
    TEST_ASSERT(strstr(buffer, "true") != NULL, "JSON contains passed status");
  }

  // Test CSV export
  const char *csv_filename = "test_export.csv";
  benchmark_suite_export_csv(suite, csv_filename);

  // Verify file was created
  FILE *csv_file = fopen(csv_filename, "r");
  TEST_ASSERT(csv_file != NULL, "CSV file creation");

  if (csv_file)
  {
    char buffer[1024];
    size_t bytes_read = fread(buffer, 1, sizeof(buffer) - 1, csv_file);
    buffer[bytes_read] = '\0';
    fclose(csv_file);

    TEST_ASSERT(bytes_read > 0, "CSV file content");
    TEST_ASSERT(strstr(buffer, "Export Test") != NULL, "CSV contains test name");
    TEST_ASSERT(strstr(buffer, "95.0") != NULL, "CSV contains achievement percent");
    TEST_ASSERT(strstr(buffer, "true") != NULL, "CSV contains passed status");
  }

  // Cleanup test files
  remove(json_filename);
  remove(csv_filename);

  benchmark_suite_destroy(suite);

  return 1;
}

// Test 6: Hardware detection
int test_hardware_detection()
{
  TEST_SECTION("Hardware Detection");

  HardwareInfo *info = detect_hardware_capabilities();
  TEST_ASSERT(info != NULL, "Hardware info creation");

  TEST_ASSERT(info->num_cores > 0, "CPU cores detection");
  TEST_ASSERT(info->cpu_frequency_ghz > 0, "CPU frequency detection");

  // Test hardware info printing (should not crash)
  print_hardware_info(info);

  destroy_hardware_info(info);

  return 1;
}

// Test 7: Performance comparison
int test_performance_comparison()
{
  TEST_SECTION("Performance Comparison");

  BenchmarkSuite *suite = benchmark_suite_create("Performance Test");
  TEST_ASSERT(suite != NULL, "Suite creation");

  TestData test_data = {0, 1000};

  // Test different operation types
  BenchmarkResult simple_result = benchmark_execute_single(
      "Simple Operation",
      1000,
      benchmark_simple_operation,
      &test_data);

  BenchmarkResult memory_result = benchmark_execute_single(
      "Memory Operation",
      1000,
      benchmark_memory_operation,
      &test_data);

  BenchmarkResult string_result = benchmark_execute_single(
      "String Operation",
      1000,
      benchmark_string_operation,
      &test_data);

  // Add results to suite
  benchmark_suite_add_result(suite, simple_result);
  benchmark_suite_add_result(suite, memory_result);
  benchmark_suite_add_result(suite, string_result);

  // Calculate statistics
  benchmark_suite_calculate_stats(suite);

  TEST_ASSERT(suite->result_count == 3, "Suite result count");
  TEST_ASSERT(suite->overall_score >= 0, "Suite overall score");

  // Verify performance characteristics
  TEST_ASSERT(simple_result.avg_cycles_per_op <= memory_result.avg_cycles_per_op,
              "Simple operation faster than memory operation");
  TEST_ASSERT(memory_result.avg_cycles_per_op <= string_result.avg_cycles_per_op,
              "Memory operation faster than string operation");

  benchmark_suite_destroy(suite);

  return 1;
}

// Test 8: Error handling and edge cases
int test_error_handling()
{
  TEST_SECTION("Error Handling and Edge Cases");

  // Test NULL suite operations
  benchmark_suite_add_result(NULL, (BenchmarkResult){0});
  benchmark_suite_calculate_stats(NULL);
  benchmark_suite_destroy(NULL);

  // Test NULL export operations
  benchmark_suite_export_json(NULL, "test.json");
  benchmark_suite_export_csv(NULL, "test.csv");

  // Test NULL hardware operations
  print_hardware_info(NULL);
  destroy_hardware_info(NULL);

  // Test empty suite
  BenchmarkSuite *suite = benchmark_suite_create("Empty Test");
  TEST_ASSERT(suite != NULL, "Empty suite creation");

  benchmark_suite_calculate_stats(suite);
  TEST_ASSERT(suite->overall_score == 0.0, "Empty suite score");

  benchmark_suite_destroy(suite);

  // Test single operation benchmark
  TestData test_data = {0, 1};
  BenchmarkResult result = benchmark_execute_single(
      "Single Operation",
      1,
      benchmark_simple_operation,
      &test_data);

  TEST_ASSERT(result.operations == 1, "Single operation count");
  TEST_ASSERT(result.total_cycles > 0, "Single operation cycles");
  TEST_ASSERT(result.avg_cycles_per_op > 0, "Single operation average cycles");

  return 1;
}

// Main test runner
int main()
{
  printf("Benchmark Framework 80/20 Unit Tests\n");
  printf("====================================\n");
  printf("Focus: Critical functionality, timing accuracy, result tracking\n\n");

  int total_tests = 8;
  int passed_tests = 0;

  // Run all tests
  if (test_basic_framework())
    passed_tests++;
  if (test_timing_accuracy())
    passed_tests++;
  if (test_result_validation())
    passed_tests++;
  if (test_suite_management())
    passed_tests++;
  if (test_export_functionality())
    passed_tests++;
  if (test_hardware_detection())
    passed_tests++;
  if (test_performance_comparison())
    passed_tests++;
  if (test_error_handling())
    passed_tests++;

  // Summary
  printf("\n=== Test Summary ===\n");
  printf("Total tests: %d\n", total_tests);
  printf("Passed: %d\n", passed_tests);
  printf("Failed: %d\n", total_tests - passed_tests);
  printf("Success rate: %.1f%%\n", (passed_tests * 100.0) / total_tests);

  if (passed_tests == total_tests)
  {
    printf("\n🎉 All tests passed! Benchmark framework is working correctly.\n");
    return 0;
  }
  else
  {
    printf("\n❌ Some tests failed. Please review the output above.\n");
    return 1;
  }
}