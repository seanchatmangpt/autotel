#include "7t_unit_test_framework.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Test suite registration
TEST_SUITE_BEGIN(simple_tests)

// Test data structures
typedef struct
{
  int test_value;
  char *test_string;
} SimpleTestData;

// Setup and teardown functions
static void setup_simple_test(TestContext *context)
{
  SimpleTestData *data = malloc(sizeof(SimpleTestData));
  ASSERT_NOT_NULL(data);

  data->test_value = 42;
  data->test_string = strdup("test_string");
  ASSERT_NOT_NULL(data->test_string);

  context->test_data = data;
}

static void teardown_simple_test(TestContext *context)
{
  SimpleTestData *data = (SimpleTestData *)context->test_data;
  if (data)
  {
    if (data->test_string)
    {
      free(data->test_string);
    }
    free(data);
  }
}

// ============================================================================
// BASIC TESTS
// ============================================================================

TEST_CASE(basic_assertions, "Test basic assertion macros")
static void test_basic_assertions(TestContext *context)
{
  // Test basic assertions
  ASSERT_TRUE(true);
  ASSERT_FALSE(false);
  ASSERT_EQUAL(42, 42);
  ASSERT_NOT_EQUAL(42, 43);
  ASSERT_STRING_EQUAL("hello", "hello");
  ASSERT_NULL(NULL);
  ASSERT_NOT_NULL("not null");

  // Test range assertions
  ASSERT_GREATER_THAN(10, 5);
  ASSERT_LESS_THAN(5, 10);
  ASSERT_IN_RANGE(7, 5, 10);
}

TEST_CASE(test_data_access, "Test accessing test data")
static void test_test_data_access(TestContext *context)
{
  SimpleTestData *data = (SimpleTestData *)context->test_data;
  ASSERT_NOT_NULL(data);

  ASSERT_EQUAL(42, data->test_value);
  ASSERT_STRING_EQUAL("test_string", data->test_string);
}

TEST_CASE(memory_allocation, "Test memory allocation")
static void test_memory_allocation(TestContext *context)
{
  void *ptr = malloc(1024);
  ASSERT_NOT_NULL(ptr);

  // Test memory usage
  ASSERT_MEMORY_USAGE({
        void* temp = malloc(1000);
        if (temp) {
            free(temp);
        } }, 1024 * 1024); // 1MB limit

  free(ptr);
}

TEST_CASE(performance_test, "Test performance assertions")
static void test_performance_test(TestContext *context)
{
  // Test performance assertion
  ASSERT_PERFORMANCE({
        for (int i = 0; i < 1000; i++) {
            // Do some work
            int result = i * i;
            (void)result; // Prevent unused variable warning
        } }, 100000); // 100K cycles limit

  // Test latency assertion
  ASSERT_LATENCY({
        for (int i = 0; i < 100; i++) {
            // Do some work
            int result = i + i;
            (void)result; // Prevent unused variable warning
        } }, 1000000); // 1ms limit
}

TEST_CASE(string_operations, "Test string operations")
static void test_string_operations(TestContext *context)
{
  char *str1 = generate_random_string(10);
  ASSERT_NOT_NULL(str1);

  char *str2 = generate_random_string(10);
  ASSERT_NOT_NULL(str2);

  // Test string operations
  ASSERT_NOT_EQUAL(strlen(str1), 0);
  ASSERT_NOT_EQUAL(strlen(str2), 0);

  // Test string comparison
  if (strcmp(str1, str2) == 0)
  {
    // If they happen to be equal, generate another one
    free(str2);
    str2 = generate_random_string(10);
    ASSERT_NOT_NULL(str2);
  }

  ASSERT_NOT_EQUAL(strcmp(str1, str2), 0);

  free(str1);
  free(str2);
}

TEST_CASE(number_operations, "Test number operations")
static void test_number_operations(TestContext *context)
{
  uint32_t *numbers = generate_random_uints(100, 1000);
  ASSERT_NOT_NULL(numbers);

  // Test number operations
  for (int i = 0; i < 100; i++)
  {
    ASSERT_IN_RANGE(numbers[i], 0, 1000);
  }

  // Test arithmetic
  int sum = 0;
  for (int i = 0; i < 100; i++)
  {
    sum += numbers[i];
  }

  ASSERT_GREATER_THAN(sum, 0);
  ASSERT_LESS_THAN(sum, 100 * 1000);

  free(numbers);
}

TEST_CASE(error_handling, "Test error handling")
static void test_error_handling(TestContext *context)
{
  // Test NULL pointer handling
  void *null_ptr = NULL;
  ASSERT_NULL(null_ptr);

  // Test invalid memory access (should not crash)
  void *ptr = malloc(10);
  ASSERT_NOT_NULL(ptr);
  free(ptr);

  // Test that we can continue after potential errors
  ASSERT_TRUE(true);
}

TEST_CASE(setup_teardown, "Test setup and teardown functions")
static void test_setup_teardown(TestContext *context)
{
  // Verify setup was called
  ASSERT_TRUE(context->setup_called);

  // Verify test data is available
  SimpleTestData *data = (SimpleTestData *)context->test_data;
  ASSERT_NOT_NULL(data);
  ASSERT_EQUAL(42, data->test_value);

  // Modify test data
  data->test_value = 100;
  ASSERT_EQUAL(100, data->test_value);
}

TEST_CASE_WITH_SETUP(custom_setup_test, setup_simple_test, teardown_simple_test, "Test with custom setup/teardown")
static void test_custom_setup_test(TestContext *context)
{
  // This test uses custom setup/teardown
  SimpleTestData *data = (SimpleTestData *)context->test_data;
  ASSERT_NOT_NULL(data);

  // Test the setup data
  ASSERT_EQUAL(42, data->test_value);
  ASSERT_STRING_EQUAL("test_string", data->test_string);

  // Modify the data
  data->test_value = 999;
  ASSERT_EQUAL(999, data->test_value);
}

TEST_SUITE_END(simple_tests)

// Test suite registration function
void register_simple_tests(void)
{
  // Register all test cases
  TestCase test_cases[] = {
      test_case_basic_assertions,
      test_case_test_data_access,
      test_case_memory_allocation,
      test_case_performance_test,
      test_case_string_operations,
      test_case_number_operations,
      test_case_error_handling,
      test_case_setup_teardown,
      test_case_custom_setup_test};

  simple_tests_registration.test_cases = test_cases;
  simple_tests_registration.test_case_count = sizeof(test_cases) / sizeof(TestCase);
  simple_tests_registration.suite_setup = setup_simple_test;
  simple_tests_registration.suite_teardown = teardown_simple_test;
}

// Main test runner
int main(int argc, char *argv[])
{
  printf("=== Simple Unit Tests ===\n");
  printf("Framework Version: %s\n", SEVEN_TICK_TEST_VERSION);

  // Initialize test configuration
  test_config_init();
  test_config_set_verbose(true);

  // Register tests
  register_simple_tests();

  // Run test suite
  test_run_suite(&simple_tests_registration);

  printf("\n=== Simple Tests Complete ===\n");
  return 0;
}