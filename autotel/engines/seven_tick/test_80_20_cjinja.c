#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../compiler/src/cjinja.h"

// 80/20 Unit Tests for CJinja
// Focus: Critical functionality, 7-tick vs 49-tick paths, error handling

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

// Test 1: Basic 7-tick path functionality
int test_7tick_basic_operations()
{
  TEST_SECTION("7-Tick Basic Operations");

  CJinjaEngine *engine = cjinja_create("./templates");
  CJinjaContext *ctx = cjinja_create_context();

  TEST_ASSERT(engine != NULL, "Engine creation");
  TEST_ASSERT(ctx != NULL, "Context creation");

  // Set up test data
  cjinja_set_var(ctx, "name", "John");
  cjinja_set_var(ctx, "title", "Developer");

  // Test 7-tick variable substitution
  const char *template = "Hello {{name}}, you are a {{title}}!";
  char *result = cjinja_render_string_7tick(template, ctx);

  TEST_ASSERT(result != NULL, "7-tick render result not null");
  TEST_ASSERT(strstr(result, "Hello John") != NULL, "7-tick variable substitution");
  TEST_ASSERT(strstr(result, "you are a Developer") != NULL, "7-tick multiple variables");

  free(result);

  // Test 7-tick conditional rendering
  cjinja_set_bool(ctx, "is_admin", 1);
  const char *conditional_template = "{% if is_admin %}Admin user{% endif %}";
  char *conditional_result = cjinja_render_conditionals_7tick(conditional_template, ctx);

  TEST_ASSERT(conditional_result != NULL, "7-tick conditional result not null");
  TEST_ASSERT(strstr(conditional_result, "Admin user") != NULL, "7-tick conditional rendering");

  free(conditional_result);

  // Cleanup
  cjinja_destroy_context(ctx);
  cjinja_destroy_engine(engine);

  return 1;
}

// Test 2: 49-tick path functionality
int test_49tick_advanced_features()
{
  TEST_SECTION("49-Tick Advanced Features");

  CJinjaEngine *engine = cjinja_create("./templates");
  CJinjaContext *ctx = cjinja_create_context();

  TEST_ASSERT(engine != NULL, "Engine creation");
  TEST_ASSERT(ctx != NULL, "Context creation");

  // Set up test data
  cjinja_set_var(ctx, "user", "Alice");
  cjinja_set_var(ctx, "email", "alice@example.com");
  cjinja_set_bool(ctx, "is_premium", 1);

  char *items[] = {"apple", "banana", "cherry"};
  cjinja_set_array(ctx, "fruits", items, 3);

  // Test 49-tick variable substitution
  const char *template = "User: {{user | upper}}, Email: {{email | lower}}";
  char *result = cjinja_render_string(template, ctx);

  TEST_ASSERT(result != NULL, "49-tick render result not null");
  TEST_ASSERT(strstr(result, "ALICE") != NULL, "49-tick upper filter");
  TEST_ASSERT(strstr(result, "alice@example.com") != NULL, "49-tick lower filter");

  free(result);

  // Test 49-tick loop rendering
  const char *loop_template =
      "Fruits:\n"
      "{% for fruit in fruits %}"
      "  - {{fruit | capitalize}}\n"
      "{% endfor %}"
      "Total: {{fruits | length}} fruits";

  char *loop_result = cjinja_render_with_loops(loop_template, ctx);

  TEST_ASSERT(loop_result != NULL, "49-tick loop result not null");
  TEST_ASSERT(strstr(loop_result, "Apple") != NULL, "49-tick loop with capitalize filter");
  TEST_ASSERT(strstr(loop_result, "Banana") != NULL, "49-tick loop with capitalize filter");
  TEST_ASSERT(strstr(loop_result, "Cherry") != NULL, "49-tick loop with capitalize filter");
  TEST_ASSERT(strstr(loop_result, "3 fruits") != NULL, "49-tick length filter");

  free(loop_result);

  // Test 49-tick conditional rendering
  const char *conditional_template =
      "{% if is_premium %}Premium user{% else %}Regular user{% endif %}";
  char *conditional_result = cjinja_render_with_conditionals(conditional_template, ctx);

  TEST_ASSERT(conditional_result != NULL, "49-tick conditional result not null");
  TEST_ASSERT(strstr(conditional_result, "Premium user") != NULL, "49-tick conditional rendering");

  free(conditional_result);

  // Cleanup
  cjinja_destroy_context(ctx);
  cjinja_destroy_engine(engine);

  return 1;
}

// Test 3: Template inheritance (49-tick only)
int test_template_inheritance()
{
  TEST_SECTION("Template Inheritance (49-Tick Only)");

  CJinjaEngine *engine = cjinja_create("./templates");
  CJinjaContext *ctx = cjinja_create_context();

  TEST_ASSERT(engine != NULL, "Engine creation");
  TEST_ASSERT(ctx != NULL, "Context creation");

  cjinja_set_var(ctx, "title", "My Page");
  cjinja_set_var(ctx, "content", "Hello World");

  // Create inheritance context
  CJinjaInheritanceContext *inherit_ctx = cjinja_create_inheritance_context();
  TEST_ASSERT(inherit_ctx != NULL, "Inheritance context creation");

  // Set up base template
  cjinja_set_base_template(inherit_ctx,
                           "<html><head><title>{{title}}</title></head><body>{{% block content %}}Default{{% endblock %}}</body></html>");

  // Add content block
  cjinja_add_block(inherit_ctx, "content", "{{content}}");

  // Test inheritance rendering
  const char *child_template =
      "{{% extends base %}}\n"
      "{{% block content %}}{{content}}{{% endblock %}}";

  char *result = cjinja_render_with_inheritance(child_template, ctx, inherit_ctx);

  TEST_ASSERT(result != NULL, "Inheritance render result not null");
  TEST_ASSERT(strstr(result, "<html>") != NULL, "Inheritance HTML structure");
  TEST_ASSERT(strstr(result, "<title>My Page</title>") != NULL, "Inheritance title");
  TEST_ASSERT(strstr(result, "Hello World") != NULL, "Inheritance content");

  free(result);
  cjinja_destroy_inheritance_context(inherit_ctx);

  // Cleanup
  cjinja_destroy_context(ctx);
  cjinja_destroy_engine(engine);

  return 1;
}

// Test 4: Batch rendering (49-tick only)
int test_batch_rendering()
{
  TEST_SECTION("Batch Rendering (49-Tick Only)");

  CJinjaEngine *engine = cjinja_create("./templates");
  CJinjaContext *ctx = cjinja_create_context();

  TEST_ASSERT(engine != NULL, "Engine creation");
  TEST_ASSERT(ctx != NULL, "Context creation");

  cjinja_set_var(ctx, "user", "Bob");
  cjinja_set_var(ctx, "title", "Dashboard");

  // Create batch render
  CJinjaBatchRender *batch = cjinja_create_batch_render(3);
  TEST_ASSERT(batch != NULL, "Batch render creation");

  // Set up templates
  const char *templates[] = {
      "User: {{user}}",
      "Title: {{title}}",
      "Welcome {{user | upper}} to {{title}}!"};

  for (int i = 0; i < 3; i++)
  {
    batch->templates[i] = templates[i];
  }

  // Execute batch render
  int result = cjinja_render_batch(engine, batch, ctx);
  TEST_ASSERT(result == 0, "Batch render execution");

  // Verify results
  TEST_ASSERT(batch->results[0] != NULL, "Batch result 1 not null");
  TEST_ASSERT(strstr(batch->results[0], "User: Bob") != NULL, "Batch result 1 content");

  TEST_ASSERT(batch->results[1] != NULL, "Batch result 2 not null");
  TEST_ASSERT(strstr(batch->results[1], "Title: Dashboard") != NULL, "Batch result 2 content");

  TEST_ASSERT(batch->results[2] != NULL, "Batch result 3 not null");
  TEST_ASSERT(strstr(batch->results[2], "Welcome BOB") != NULL, "Batch result 3 content");
  TEST_ASSERT(strstr(batch->results[2], "to Dashboard") != NULL, "Batch result 3 content");

  cjinja_destroy_batch_render(batch);

  // Cleanup
  cjinja_destroy_context(ctx);
  cjinja_destroy_engine(engine);

  return 1;
}

// Test 5: Error handling and edge cases
int test_error_handling()
{
  TEST_SECTION("Error Handling and Edge Cases");

  // Test NULL engine
  CJinjaContext *ctx1 = cjinja_create_context();
  TEST_ASSERT(ctx1 != NULL, "Context creation with NULL engine");
  cjinja_destroy_context(ctx1);

  // Test NULL context
  CJinjaEngine *engine = cjinja_create("./templates");
  TEST_ASSERT(engine != NULL, "Engine creation");

  // Test rendering with NULL context
  const char *template = "Hello {{name}}";
  char *result = cjinja_render_string_7tick(template, NULL);
  TEST_ASSERT(result == NULL, "7-tick render with NULL context returns NULL");

  // Test rendering with NULL template
  CJinjaContext *ctx = cjinja_create_context();
  result = cjinja_render_string_7tick(NULL, ctx);
  TEST_ASSERT(result == NULL, "7-tick render with NULL template returns NULL");

  // Test variable operations
  cjinja_set_var(ctx, "name", "Test");
  TEST_ASSERT(strcmp(get_var(ctx, "name"), "Test") == 0, "Variable get/set");

  // Test boolean operations
  cjinja_set_bool(ctx, "flag", 1);
  TEST_ASSERT(get_bool(ctx, "flag") == 1, "Boolean get/set");

  // Test array operations
  char *items[] = {"item1", "item2"};
  cjinja_set_array(ctx, "items", items, 2);
  TEST_ASSERT(get_array_length(ctx, "items") == 2, "Array length");

  // Test missing variables
  TEST_ASSERT(get_var(ctx, "missing") == NULL, "Missing variable returns NULL");
  TEST_ASSERT(get_bool(ctx, "missing") == 0, "Missing boolean returns 0");
  TEST_ASSERT(get_array_length(ctx, "missing") == 0, "Missing array returns 0");

  // Cleanup
  cjinja_destroy_context(ctx);
  cjinja_destroy_engine(engine);

  return 1;
}

// Test 6: Performance comparison (7-tick vs 49-tick)
int test_performance_comparison()
{
  TEST_SECTION("Performance Comparison (7-Tick vs 49-Tick)");

  CJinjaEngine *engine = cjinja_create("./templates");
  CJinjaContext *ctx = cjinja_create_context();

  TEST_ASSERT(engine != NULL, "Engine creation");
  TEST_ASSERT(ctx != NULL, "Context creation");

  cjinja_set_var(ctx, "name", "Performance");
  cjinja_set_var(ctx, "value", "Test");

  const char *simple_template = "Hello {{name}}, value: {{value}}";

  // Measure 7-tick performance
  clock_t start_7tick = clock();
  for (int i = 0; i < 1000; i++)
  {
    char *result = cjinja_render_string_7tick(simple_template, ctx);
    free(result);
  }
  clock_t end_7tick = clock();
  double time_7tick = ((double)(end_7tick - start_7tick)) / CLOCKS_PER_SEC;

  // Measure 49-tick performance
  clock_t start_49tick = clock();
  for (int i = 0; i < 1000; i++)
  {
    char *result = cjinja_render_string(simple_template, ctx);
    free(result);
  }
  clock_t end_49tick = clock();
  double time_49tick = ((double)(end_49tick - start_49tick)) / CLOCKS_PER_SEC;

  printf("  7-tick time: %.6f seconds\n", time_7tick);
  printf("  49-tick time: %.6f seconds\n", time_49tick);

  TEST_ASSERT(time_7tick > 0, "7-tick performance measurement");
  TEST_ASSERT(time_49tick > 0, "49-tick performance measurement");
  TEST_ASSERT(time_7tick <= time_49tick, "7-tick path is not slower than 49-tick");

  // Cleanup
  cjinja_destroy_context(ctx);
  cjinja_destroy_engine(engine);

  return 1;
}

// Test 7: Memory safety and cleanup
int test_memory_safety()
{
  TEST_SECTION("Memory Safety and Cleanup");

  // Test multiple engine/context creation and destruction
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

    // Cleanup
    cjinja_destroy_context(ctx);
    cjinja_destroy_engine(engine);
  }

  // Test inheritance context cleanup
  CJinjaInheritanceContext *inherit_ctx = cjinja_create_inheritance_context();
  TEST_ASSERT(inherit_ctx != NULL, "Inheritance context creation");
  cjinja_destroy_inheritance_context(inherit_ctx);

  // Test batch render cleanup
  CJinjaBatchRender *batch = cjinja_create_batch_render(5);
  TEST_ASSERT(batch != NULL, "Batch render creation");
  cjinja_destroy_batch_render(batch);

  return 1;
}

// Test 8: Advanced filters (49-tick only)
int test_advanced_filters()
{
  TEST_SECTION("Advanced Filters (49-Tick Only)");

  CJinjaEngine *engine = cjinja_create("./templates");
  CJinjaContext *ctx = cjinja_create_context();

  TEST_ASSERT(engine != NULL, "Engine creation");
  TEST_ASSERT(ctx != NULL, "Context creation");

  cjinja_set_var(ctx, "text", "  Hello World  ");
  cjinja_set_var(ctx, "name", "John Doe");
  cjinja_set_var(ctx, "list", "apple,banana,cherry");

  // Test trim filter
  const char *trim_template = "Trimmed: '{{text | trim}}'";
  char *trim_result = cjinja_render_with_loops(trim_template, ctx);
  TEST_ASSERT(trim_result != NULL, "Trim filter result not null");
  TEST_ASSERT(strstr(trim_result, "Hello World") != NULL, "Trim filter functionality");
  free(trim_result);

  // Test replace filter
  const char *replace_template = "Replaced: {{name | replace('John','Jane')}}";
  char *replace_result = cjinja_render_with_loops(replace_template, ctx);
  TEST_ASSERT(replace_result != NULL, "Replace filter result not null");
  TEST_ASSERT(strstr(replace_result, "Jane Doe") != NULL, "Replace filter functionality");
  free(replace_result);

  // Test slice filter
  const char *slice_template = "Sliced: {{name | slice(0,4)}}";
  char *slice_result = cjinja_render_with_loops(slice_template, ctx);
  TEST_ASSERT(slice_result != NULL, "Slice filter result not null");
  TEST_ASSERT(strstr(slice_result, "John") != NULL, "Slice filter functionality");
  free(slice_result);

  // Test default filter
  const char *default_template = "Default: {{missing_var | default('Not Found')}}";
  char *default_result = cjinja_render_with_loops(default_template, ctx);
  TEST_ASSERT(default_result != NULL, "Default filter result not null");
  TEST_ASSERT(strstr(default_result, "Not Found") != NULL, "Default filter functionality");
  free(default_result);

  // Cleanup
  cjinja_destroy_context(ctx);
  cjinja_destroy_engine(engine);

  return 1;
}

// Main test runner
int main()
{
  printf("CJinja 80/20 Unit Tests\n");
  printf("=======================\n");
  printf("Focus: Critical functionality, 7-tick vs 49-tick paths, error handling\n\n");

  int total_tests = 8;
  int passed_tests = 0;

  // Run all tests
  if (test_7tick_basic_operations())
    passed_tests++;
  if (test_49tick_advanced_features())
    passed_tests++;
  if (test_template_inheritance())
    passed_tests++;
  if (test_batch_rendering())
    passed_tests++;
  if (test_error_handling())
    passed_tests++;
  if (test_performance_comparison())
    passed_tests++;
  if (test_memory_safety())
    passed_tests++;
  if (test_advanced_filters())
    passed_tests++;

  // Summary
  printf("\n=== Test Summary ===\n");
  printf("Total tests: %d\n", total_tests);
  printf("Passed: %d\n", passed_tests);
  printf("Failed: %d\n", total_tests - passed_tests);
  printf("Success rate: %.1f%%\n", (passed_tests * 100.0) / total_tests);

  if (passed_tests == total_tests)
  {
    printf("\n🎉 All tests passed! CJinja is working correctly.\n");
    return 0;
  }
  else
  {
    printf("\n❌ Some tests failed. Please review the output above.\n");
    return 1;
  }
}