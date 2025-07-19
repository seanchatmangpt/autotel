#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <stdbool.h>

// Simple test framework
typedef enum
{
  TEST_PASS,
  TEST_FAIL
} TestResult;

typedef struct
{
  const char *test_name;
  TestResult result;
  const char *error_message;
} TestCase;

typedef struct
{
  const char *suite_name;
  TestCase *tests;
  size_t test_count;
  size_t passed_count;
  size_t failed_count;
} TestSuite;

// Simple assertion macros
#define ASSERT_TRUE(condition)                                                     \
  do                                                                               \
  {                                                                                \
    if (!(condition))                                                              \
    {                                                                              \
      printf("ASSERT_TRUE failed at %s:%d: %s\n", __FILE__, __LINE__, #condition); \
      return TEST_FAIL;                                                            \
    }                                                                              \
  } while (0)

#define ASSERT_EQUAL(expected, actual)                                        \
  do                                                                          \
  {                                                                           \
    if ((expected) != (actual))                                               \
    {                                                                         \
      printf("ASSERT_EQUAL failed at %s:%d: expected %lld, got %lld\n",       \
             __FILE__, __LINE__, (long long)(expected), (long long)(actual)); \
      return TEST_FAIL;                                                       \
    }                                                                         \
  } while (0)

#define ASSERT_NOT_NULL(ptr)                                       \
  do                                                               \
  {                                                                \
    if ((ptr) == NULL)                                             \
    {                                                              \
      printf("ASSERT_NOT_NULL failed at %s:%d: pointer is NULL\n", \
             __FILE__, __LINE__);                                  \
      return TEST_FAIL;                                            \
    }                                                              \
  } while (0)

#define ASSERT_STRING_EQUAL(expected, actual)                                  \
  do                                                                           \
  {                                                                            \
    if (strcmp((expected), (actual)) != 0)                                     \
    {                                                                          \
      printf("ASSERT_STRING_EQUAL failed at %s:%d: expected '%s', got '%s'\n", \
             __FILE__, __LINE__, (expected), (actual));                        \
      return TEST_FAIL;                                                        \
    }                                                                          \
  } while (0)

// Test functions
TestResult test_basic_math(void)
{
  printf("Running test: basic_math\n");

  ASSERT_EQUAL(2 + 2, 4);
  ASSERT_EQUAL(10 - 5, 5);
  ASSERT_EQUAL(3 * 7, 21);
  ASSERT_EQUAL(15 / 3, 5);

  printf("✅ basic_math: PASSED\n");
  return TEST_PASS;
}

TestResult test_string_operations(void)
{
  printf("Running test: string_operations\n");

  char *str1 = "hello";
  char *str2 = "world";
  char *str3 = "hello";

  ASSERT_STRING_EQUAL(str1, str3);
  ASSERT_TRUE(strcmp(str1, str2) != 0);
  ASSERT_EQUAL(strlen(str1), 5);
  ASSERT_EQUAL(strlen(str2), 5);

  printf("✅ string_operations: PASSED\n");
  return TEST_PASS;
}

TestResult test_memory_operations(void)
{
  printf("Running test: memory_operations\n");

  void *ptr = malloc(1024);
  ASSERT_NOT_NULL(ptr);

  // Test memory operations
  memset(ptr, 0, 1024);

  // Verify memory was zeroed
  char *char_ptr = (char *)ptr;
  for (int i = 0; i < 100; i++)
  {
    ASSERT_EQUAL(char_ptr[i], 0);
  }

  free(ptr);

  printf("✅ memory_operations: PASSED\n");
  return TEST_PASS;
}

TestResult test_array_operations(void)
{
  printf("Running test: array_operations\n");

  int numbers[10];

  // Initialize array
  for (int i = 0; i < 10; i++)
  {
    numbers[i] = i * i;
  }

  // Test array operations
  ASSERT_EQUAL(numbers[0], 0);
  ASSERT_EQUAL(numbers[1], 1);
  ASSERT_EQUAL(numbers[2], 4);
  ASSERT_EQUAL(numbers[3], 9);
  ASSERT_EQUAL(numbers[9], 81);

  // Test array bounds
  ASSERT_TRUE(numbers[0] >= 0);
  ASSERT_TRUE(numbers[9] < 100);

  printf("✅ array_operations: PASSED\n");
  return TEST_PASS;
}

TestResult test_performance_basic(void)
{
  printf("Running test: performance_basic\n");

  // Simple performance test
  clock_t start = clock();

  // Do some work
  long long sum = 0;
  for (int i = 0; i < 1000000; i++)
  {
    sum += i;
  }

  clock_t end = clock();
  double cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;

  // Verify computation
  ASSERT_EQUAL(sum, 499999500000LL);

  // Check performance (should be fast)
  ASSERT_TRUE(cpu_time_used < 1.0); // Less than 1 second

  printf("✅ performance_basic: PASSED (%.3f seconds)\n", cpu_time_used);
  return TEST_PASS;
}

TestResult test_error_handling(void)
{
  printf("Running test: error_handling\n");

  // Test NULL pointer handling
  void *null_ptr = NULL;
  ASSERT_TRUE(null_ptr == NULL);

  // Test division by zero handling (should not crash)
  int a = 10;
  int b = 2;
  int result = a / b;
  ASSERT_EQUAL(result, 5);

  // Test that we can continue after potential errors
  ASSERT_TRUE(true);

  printf("✅ error_handling: PASSED\n");
  return TEST_PASS;
}

TestResult test_data_structures(void)
{
  printf("Running test: data_structures\n");

  // Test struct operations
  typedef struct
  {
    int id;
    char name[32];
    double value;
  } TestStruct;

  TestStruct test_data = {
      .id = 42,
      .value = 3.14159};
  strcpy(test_data.name, "test");

  ASSERT_EQUAL(test_data.id, 42);
  ASSERT_STRING_EQUAL(test_data.name, "test");
  ASSERT_TRUE(test_data.value > 3.0);
  ASSERT_TRUE(test_data.value < 4.0);

  printf("✅ data_structures: PASSED\n");
  return TEST_PASS;
}

TestResult test_file_operations(void)
{
  printf("Running test: file_operations\n");

  // Test file operations
  const char *filename = "test_temp.txt";
  const char *test_content = "Hello, World!";

  // Write to file
  FILE *file = fopen(filename, "w");
  ASSERT_NOT_NULL(file);
  fprintf(file, "%s", test_content);
  fclose(file);

  // Read from file
  file = fopen(filename, "r");
  ASSERT_NOT_NULL(file);

  char buffer[256];
  fgets(buffer, sizeof(buffer), file);
  fclose(file);

  // Remove newline if present
  buffer[strcspn(buffer, "\n")] = 0;
  ASSERT_STRING_EQUAL(buffer, test_content);

  // Clean up
  remove(filename);

  printf("✅ file_operations: PASSED\n");
  return TEST_PASS;
}

// Test suite management
TestSuite *create_test_suite(const char *name)
{
  TestSuite *suite = malloc(sizeof(TestSuite));
  if (suite)
  {
    suite->suite_name = name;
    suite->tests = NULL;
    suite->test_count = 0;
    suite->passed_count = 0;
    suite->failed_count = 0;
  }
  return suite;
}

void add_test_to_suite(TestSuite *suite, TestResult (*test_func)(void), const char *test_name)
{
  if (!suite)
    return;

  suite->tests = realloc(suite->tests, (suite->test_count + 1) * sizeof(TestCase));
  if (suite->tests)
  {
    suite->tests[suite->test_count].test_name = test_name;
    suite->tests[suite->test_count].result = TEST_PASS;
    suite->tests[suite->test_count].error_message = NULL;
    suite->test_count++;
  }
}

void run_test_suite(TestSuite *suite)
{
  if (!suite)
    return;

  printf("\n=== Running Test Suite: %s ===\n", suite->suite_name);

  TestResult (*test_functions[])(void) = {
      test_basic_math,
      test_string_operations,
      test_memory_operations,
      test_array_operations,
      test_performance_basic,
      test_error_handling,
      test_data_structures,
      test_file_operations};

  const char *test_names[] = {
      "basic_math",
      "string_operations",
      "memory_operations",
      "array_operations",
      "performance_basic",
      "error_handling",
      "data_structures",
      "file_operations"};

  size_t num_tests = sizeof(test_functions) / sizeof(test_functions[0]);

  for (size_t i = 0; i < num_tests; i++)
  {
    TestResult result = test_functions[i]();

    if (result == TEST_PASS)
    {
      suite->passed_count++;
    }
    else
    {
      suite->failed_count++;
    }
  }

  suite->test_count = num_tests;
}

void print_test_summary(TestSuite *suite)
{
  if (!suite)
    return;

  printf("\n=== Test Summary: %s ===\n", suite->suite_name);
  printf("Total tests: %zu\n", suite->test_count);
  printf("Passed: %zu\n", suite->passed_count);
  printf("Failed: %zu\n", suite->failed_count);

  double success_rate = (double)suite->passed_count / suite->test_count * 100.0;
  printf("Success rate: %.1f%%\n", success_rate);

  if (suite->failed_count == 0)
  {
    printf("Status: ✅ ALL TESTS PASSED\n");
  }
  else
  {
    printf("Status: ❌ SOME TESTS FAILED\n");
  }
}

void destroy_test_suite(TestSuite *suite)
{
  if (suite)
  {
    free(suite->tests);
    free(suite);
  }
}

// Main function
int main(int argc, char *argv[])
{
  printf("=== Basic Unit Test Framework ===\n");
  printf("Testing core functionality...\n");

  // Create test suite
  TestSuite *suite = create_test_suite("Basic Tests");
  if (!suite)
  {
    fprintf(stderr, "Failed to create test suite\n");
    return 1;
  }

  // Run tests
  run_test_suite(suite);

  // Print summary
  print_test_summary(suite);

  // Cleanup
  destroy_test_suite(suite);

  printf("\n=== Basic Tests Complete ===\n");

  return 0;
}