#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../compiler/src/cjinja.h"

// 80/20 Simple Unit Tests
// Focus: Most critical functionality that must work

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

// Test 1: CJinja 7-tick path (most critical)
int test_cjinja_7tick()
{
  TEST_SECTION("CJinja 7-Tick Path");

  CJinjaEngine *engine = cjinja_create("./templates");
  CJinjaContext *ctx = cjinja_create_context();

  TEST_ASSERT(engine != NULL, "Engine creation");
  TEST_ASSERT(ctx != NULL, "Context creation");

  // Test critical 7-tick functionality
  cjinja_set_var(ctx, "name", "John");
  cjinja_set_var(ctx, "title", "Developer");

  const char *template = "Hello {{name}}, you are a {{title}}!";
  char *result = cjinja_render_string_7tick(template, ctx);

  TEST_ASSERT(result != NULL, "7-tick render result not null");
  TEST_ASSERT(strstr(result, "Hello John") != NULL, "7-tick variable substitution");
  TEST_ASSERT(strstr(result, "you are a Developer") != NULL, "7-tick multiple variables");

  free(result);

  // Test 7-tick conditional
  cjinja_set_bool(ctx, "is_admin", 1);
  const char *conditional_template = "{% if is_admin %}Admin user{% endif %}";
  char *conditional_result = cjinja_render_conditionals_7tick(conditional_template, ctx);

  TEST_ASSERT(conditional_result != NULL, "7-tick conditional result not null");
  TEST_ASSERT(strstr(conditional_result, "Admin user") != NULL, "7-tick conditional rendering");

  free(conditional_result);

  cjinja_destroy_context(ctx);
  cjinja_destroy_engine(engine);

  return 1;
}

// Test 2: CJinja 49-tick path (advanced features)
int test_cjinja_49tick()
{
  TEST_SECTION("CJinja 49-Tick Path");

  CJinjaEngine *engine = cjinja_create("./templates");
  CJinjaContext *ctx = cjinja_create_context();

  TEST_ASSERT(engine != NULL, "Engine creation");
  TEST_ASSERT(ctx != NULL, "Context creation");

  cjinja_set_var(ctx, "user", "Alice");
  cjinja_set_var(ctx, "email", "alice@example.com");

  char *items[] = {"apple", "banana", "cherry"};
  cjinja_set_array(ctx, "fruits", items, 3);

  // Test filters
  const char *filter_template = "User: {{user | upper}}, Email: {{email | lower}}";
  char *result = cjinja_render_string(filter_template, ctx);

  TEST_ASSERT(result != NULL, "49-tick render result not null");
  TEST_ASSERT(strstr(result, "ALICE") != NULL, "49-tick upper filter");
  TEST_ASSERT(strstr(result, "alice@example.com") != NULL, "49-tick lower filter");

  free(result);

  // Test loops
  const char *loop_template =
      "Fruits:\n"
      "{% for fruit in fruits %}"
      "  - {{fruit | capitalize}}\n"
      "{% endfor %}"
      "Total: {{fruits | length}} fruits";

  char *loop_result = cjinja_render_with_loops(loop_template, ctx);

  TEST_ASSERT(loop_result != NULL, "49-tick loop result not null");
  TEST_ASSERT(strstr(loop_result, "Apple") != NULL, "49-tick loop with capitalize");
  TEST_ASSERT(strstr(loop_result, "3 fruits") != NULL, "49-tick length filter");

  free(loop_result);

  cjinja_destroy_context(ctx);
  cjinja_destroy_engine(engine);

  return 1;
}

// Test 3: Performance comparison
int test_performance_comparison()
{
  TEST_SECTION("Performance Comparison (7-Tick vs 49-Tick)");

  CJinjaEngine *engine = cjinja_create("./templates");
  CJinjaContext *ctx = cjinja_create_context();

  TEST_ASSERT(engine != NULL, "Engine creation");
  TEST_ASSERT(ctx != NULL, "Context creation");

  cjinja_set_var(ctx, "name", "Performance");
  cjinja_set_var(ctx, "value", "Test");

  const char *template = "Hello {{name}}, value: {{value}}";

  // Measure 7-tick performance
  clock_t start_7tick = clock();
  for (int i = 0; i < 1000; i++)
  {
    char *result = cjinja_render_string_7tick(template, ctx);
    free(result);
  }
  clock_t end_7tick = clock();
  double time_7tick = ((double)(end_7tick - start_7tick)) / CLOCKS_PER_SEC;

  // Measure 49-tick performance
  clock_t start_49tick = clock();
  for (int i = 0; i < 1000; i++)
  {
    char *result = cjinja_render_string(template, ctx);
    free(result);
  }
  clock_t end_49tick = clock();
  double time_49tick = ((double)(end_49tick - start_49tick)) / CLOCKS_PER_SEC;

  printf("  7-tick time: %.6f seconds\n", time_7tick);
  printf("  49-tick time: %.6f seconds\n", time_49tick);

  TEST_ASSERT(time_7tick > 0, "7-tick performance measurement");
  TEST_ASSERT(time_49tick > 0, "49-tick performance measurement");
  TEST_ASSERT(time_7tick <= time_49tick, "7-tick path is not slower than 49-tick");

  cjinja_destroy_context(ctx);
  cjinja_destroy_engine(engine);

  return 1;
}

// Test 4: Error handling
int test_error_handling()
{
  TEST_SECTION("Error Handling");

  // Test NULL operations
  char *result = cjinja_render_string_7tick(NULL, NULL);
  TEST_ASSERT(result == NULL, "NULL template returns NULL");

  CJinjaEngine *engine = cjinja_create("./templates");
  CJinjaContext *ctx = cjinja_create_context();

  result = cjinja_render_string_7tick("{{name}}", NULL);
  TEST_ASSERT(result == NULL, "NULL context returns NULL");

  result = cjinja_render_string_7tick(NULL, ctx);
  TEST_ASSERT(result == NULL, "NULL template with context returns NULL");

  // Test variable operations
  cjinja_set_var(ctx, "test", "value");
  TEST_ASSERT(strcmp(get_var(ctx, "test"), "value") == 0, "Variable get/set");
  TEST_ASSERT(get_var(ctx, "missing") == NULL, "Missing variable returns NULL");

  // Test boolean operations
  cjinja_set_bool(ctx, "flag", 1);
  TEST_ASSERT(get_bool(ctx, "flag") == 1, "Boolean get/set");
  TEST_ASSERT(get_bool(ctx, "missing") == 0, "Missing boolean returns 0");

  cjinja_destroy_context(ctx);
  cjinja_destroy_engine(engine);

  return 1;
}

// Test 5: Memory safety
int test_memory_safety()
{
  TEST_SECTION("Memory Safety");

  // Test multiple creation/destruction cycles
  for (int i = 0; i < 10; i++)
  {
    CJinjaEngine *engine = cjinja_create("./templates");
    CJinjaContext *ctx = cjinja_create_context();

    TEST_ASSERT(engine != NULL, "Engine creation in loop");
    TEST_ASSERT(ctx != NULL, "Context creation in loop");

    // Use the engine and context
    cjinja_set_var(ctx, "test", "value");
    char *result = cjinja_render_string_7tick("{{test}}", ctx);
    TEST_ASSERT(result != NULL, "Render result in loop");
    free(result);

    cjinja_destroy_context(ctx);
    cjinja_destroy_engine(engine);
  }

  return 1;
}

// Main test runner
int main()
{
  printf("80/20 Simple Unit Tests\n");
  printf("=======================\n");
  printf("Focus: Most critical functionality\n\n");

  int total_tests = 5;
  int passed_tests = 0;

  // Run all tests
  if (test_cjinja_7tick())
    passed_tests++;
  if (test_cjinja_49tick())
    passed_tests++;
  if (test_performance_comparison())
    passed_tests++;
  if (test_error_handling())
    passed_tests++;
  if (test_memory_safety())
    passed_tests++;

  // Summary
  printf("\n=== Test Summary ===\n");
  printf("Total tests: %d\n", total_tests);
  printf("Passed: %d\n", passed_tests);
  printf("Failed: %d\n", total_tests - passed_tests);
  printf("Success rate: %.1f%%\n", (passed_tests * 100.0) / total_tests);

  if (passed_tests == total_tests)
  {
    printf("\n🎉 All tests passed! Critical functionality is working.\n");
    return 0;
  }
  else
  {
    printf("\n❌ Some tests failed. Please review the output above.\n");
    return 1;
  }
}