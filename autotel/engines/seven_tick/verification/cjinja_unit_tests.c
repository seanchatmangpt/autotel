#include "7t_unit_test_framework.h"
#include "../compiler/src/cjinja.h"
#include <string.h>

// Custom filter function for testing
char *highlight_filter(const char *input, const char *args)
{
  char *result = malloc(strlen(input) + 20);
  sprintf(result, "**%s**", input);
  return result;
}

// CJinja test context
typedef struct
{
  CJinjaEngine *engine;
  CJinjaContext *ctx;
  const char *test_template;
  const char *expected_result;
} CJinjaTestContext;

// Test setup and teardown
CJinjaTestContext *setup_cjinja_test_context(void)
{
  CJinjaTestContext *test_ctx = malloc(sizeof(CJinjaTestContext));
  if (!test_ctx)
    return NULL;

  test_ctx->engine = cjinja_create(NULL);
  if (!test_ctx->engine)
  {
    free(test_ctx);
    return NULL;
  }

  test_ctx->ctx = cjinja_create_context();
  if (!test_ctx->ctx)
  {
    cjinja_destroy(test_ctx->engine);
    free(test_ctx);
    return NULL;
  }

  return test_ctx;
}

void teardown_cjinja_test_context(CJinjaTestContext *test_ctx)
{
  if (test_ctx)
  {
    if (test_ctx->ctx)
    {
      cjinja_destroy_context(test_ctx->ctx);
    }
    if (test_ctx->engine)
    {
      cjinja_destroy(test_ctx->engine);
    }
    free(test_ctx);
  }
}

// Individual test functions
void test_engine_creation(void)
{
  CJinjaEngine *engine = cjinja_create(NULL);
  ASSERT_NOT_NULL(engine);
  cjinja_destroy(engine);
}

void test_context_creation(void)
{
  CJinjaContext *ctx = cjinja_create_context();
  ASSERT_NOT_NULL(ctx);
  ASSERT_EQUAL(0, ctx->count);
  cjinja_destroy_context(ctx);
}

void test_variable_setting(void)
{
  CJinjaContext *ctx = cjinja_create_context();
  ASSERT_NOT_NULL(ctx);

  cjinja_set_var(ctx, "name", "7T Engine");
  cjinja_set_var(ctx, "version", "1.0");

  char *name_val = get_var(ctx, "name");
  char *version_val = get_var(ctx, "version");

  ASSERT_NOT_NULL(name_val);
  ASSERT_NOT_NULL(version_val);
  ASSERT_STRING_EQUAL("7T Engine", name_val);
  ASSERT_STRING_EQUAL("1.0", version_val);

  cjinja_destroy_context(ctx);
}

void test_simple_variable_substitution(void)
{
  CJinjaTestContext *test_ctx = setup_cjinja_test_context();
  ASSERT_NOT_NULL(test_ctx);

  cjinja_set_var(test_ctx->ctx, "name", "7T Engine");
  cjinja_set_var(test_ctx->ctx, "version", "1.0");

  const char *template = "Hello {{ name }} version {{ version }}!";
  char *result = cjinja_render_string(template, test_ctx->ctx);

  ASSERT_NOT_NULL(result);
  ASSERT_STRING_EQUAL("Hello 7T Engine version 1.0!", result);

  free(result);
  teardown_cjinja_test_context(test_ctx);
}

void test_boolean_variables(void)
{
  CJinjaTestContext *test_ctx = setup_cjinja_test_context();
  ASSERT_NOT_NULL(test_ctx);

  cjinja_set_bool(test_ctx->ctx, "enabled", 1);
  cjinja_set_bool(test_ctx->ctx, "debug", 0);

  const char *template = "Enabled: {{ enabled }}, Debug: {{ debug }}";
  char *result = cjinja_render_string(template, test_ctx->ctx);

  ASSERT_NOT_NULL(result);
  ASSERT_STRING_EQUAL("Enabled: 1, Debug: 0", result);

  free(result);
  teardown_cjinja_test_context(test_ctx);
}

void test_array_variables(void)
{
  CJinjaTestContext *test_ctx = setup_cjinja_test_context();
  ASSERT_NOT_NULL(test_ctx);

  char *items[] = {"item1", "item2", "item3"};
  cjinja_set_array(test_ctx->ctx, "items", items, 3);

  const char *template = "Items: {{ items[0] }}, {{ items[1] }}, {{ items[2] }}";
  char *result = cjinja_render_string(template, test_ctx->ctx);

  ASSERT_NOT_NULL(result);
  ASSERT_STRING_EQUAL("Items: item1, item2, item3", result);

  free(result);
  teardown_cjinja_test_context(test_ctx);
}

void test_conditional_rendering(void)
{
  CJinjaTestContext *test_ctx = setup_cjinja_test_context();
  ASSERT_NOT_NULL(test_ctx);

  cjinja_set_bool(test_ctx->ctx, "enabled", 1);
  cjinja_set_bool(test_ctx->ctx, "debug", 0);

  const char *template = "{% if enabled %}Feature is enabled{% else %}Feature is disabled{% endif %}\n"
                         "{% if debug %}Debug mode{% else %}Production mode{% endif %}";

  char *result = cjinja_render_string(template, test_ctx->ctx);

  ASSERT_NOT_NULL(result);
  ASSERT_STRING_EQUAL("Feature is enabled\nProduction mode", result);

  free(result);
  teardown_cjinja_test_context(test_ctx);
}

void test_loop_rendering(void)
{
  CJinjaTestContext *test_ctx = setup_cjinja_test_context();
  ASSERT_NOT_NULL(test_ctx);

  char *features[] = {"SPARQL", "SHACL", "CJinja"};
  cjinja_set_array(test_ctx->ctx, "features", features, 3);

  const char *template = "Features:\n{% for feature in features %}- {{ feature }}\n{% endfor %}";
  char *result = cjinja_render_string(template, test_ctx->ctx);

  ASSERT_NOT_NULL(result);
  ASSERT_STRING_EQUAL("Features:\n- SPARQL\n- SHACL\n- CJinja\n", result);

  free(result);
  teardown_cjinja_test_context(test_ctx);
}

void test_filter_operations(void)
{
  CJinjaTestContext *test_ctx = setup_cjinja_test_context();
  ASSERT_NOT_NULL(test_ctx);

  cjinja_set_var(test_ctx->ctx, "text", "hello world");
  cjinja_set_var(test_ctx->ctx, "number", "42");

  // Test built-in filters
  const char *template = "{{ text | upper }}\n{{ text | capitalize }}\n{{ number | length }}";
  char *result = cjinja_render_string(template, test_ctx->ctx);

  ASSERT_NOT_NULL(result);
  ASSERT_STRING_EQUAL("HELLO WORLD\nHello world\n2", result);

  free(result);
  teardown_cjinja_test_context(test_ctx);
}

void test_custom_filter(void)
{
  CJinjaTestContext *test_ctx = setup_cjinja_test_context();
  ASSERT_NOT_NULL(test_ctx);

  // Register custom filter
  cjinja_register_filter("highlight", highlight_filter);

  cjinja_set_var(test_ctx->ctx, "title", "7T Engine");

  const char *template = "{{ title | highlight }}";
  char *result = cjinja_render_string(template, test_ctx->ctx);

  ASSERT_NOT_NULL(result);
  ASSERT_STRING_EQUAL("**7T Engine**", result);

  free(result);
  teardown_cjinja_test_context(test_ctx);
}

void test_template_caching(void)
{
  CJinjaTestContext *test_ctx = setup_cjinja_test_context();
  ASSERT_NOT_NULL(test_ctx);

  cjinja_enable_cache(test_ctx->engine, 1);
  cjinja_set_var(test_ctx->ctx, "name", "7T Engine");

  const char *template = "Hello {{ name }}!";

  // First render (should cache)
  char *result1 = cjinja_render_string(template, test_ctx->ctx);
  ASSERT_NOT_NULL(result1);
  ASSERT_STRING_EQUAL("Hello 7T Engine!", result1);

  // Second render (should use cache)
  char *result2 = cjinja_render_string(template, test_ctx->ctx);
  ASSERT_NOT_NULL(result2);
  ASSERT_STRING_EQUAL("Hello 7T Engine!", result2);

  free(result1);
  free(result2);
  teardown_cjinja_test_context(test_ctx);
}

void test_complex_template(void)
{
  CJinjaTestContext *test_ctx = setup_cjinja_test_context();
  ASSERT_NOT_NULL(test_ctx);

  cjinja_set_var(test_ctx->ctx, "title", "7T Engine Report");
  cjinja_set_var(test_ctx->ctx, "company", "Autotel Systems");
  cjinja_set_bool(test_ctx->ctx, "performance_target_met", 1);

  char *metrics[] = {"SPARQL: 1.4ns", "SHACL: 1.5ns", "CJinja: 206ns"};
  cjinja_set_array(test_ctx->ctx, "metrics", metrics, 3);

  const char *template =
      "# {{ title }}\n\n"
      "**Company:** {{ company }}\n\n"
      "## Performance Summary\n"
      "{% if performance_target_met %}"
      "✅ All performance targets met!\n"
      "{% else %}"
      "❌ Performance targets not met\n"
      "{% endif %}\n\n"
      "## Metrics\n"
      "{% for metric in metrics %}"
      "- {{ metric }}\n"
      "{% endfor %}";

  char *result = cjinja_render_string(template, test_ctx->ctx);

  ASSERT_NOT_NULL(result);
  const char *expected =
      "# 7T Engine Report\n\n"
      "**Company:** Autotel Systems\n\n"
      "## Performance Summary\n"
      "✅ All performance targets met!\n\n"
      "## Metrics\n"
      "- SPARQL: 1.4ns\n"
      "- SHACL: 1.5ns\n"
      "- CJinja: 206ns\n";

  ASSERT_STRING_EQUAL(expected, result);

  free(result);
  teardown_cjinja_test_context(test_ctx);
}

void test_error_handling(void)
{
  CJinjaTestContext *test_ctx = setup_cjinja_test_context();
  ASSERT_NOT_NULL(test_ctx);

  // Test with non-existent variable
  const char *template = "Hello {{ nonexistent }}!";
  char *result = cjinja_render_string(template, test_ctx->ctx);

  // Should handle gracefully (empty string or variable name)
  ASSERT_NOT_NULL(result);

  free(result);
  teardown_cjinja_test_context(test_ctx);
}

void test_performance_7tick_variable_substitution(void)
{
  CJinjaTestContext *test_ctx = setup_cjinja_test_context();
  ASSERT_NOT_NULL(test_ctx);

  cjinja_set_var(test_ctx->ctx, "name", "7T Engine");

  // Test 7-tick performance for variable substitution
  ASSERT_PERFORMANCE_7TICK(
      {
        char *result = cjinja_render_string("{{ name }}", test_ctx->ctx);
        if (result)
          free(result);
      },
      10000);

  teardown_cjinja_test_context(test_ctx);
}

void test_performance_7tick_conditional(void)
{
  CJinjaTestContext *test_ctx = setup_cjinja_test_context();
  ASSERT_NOT_NULL(test_ctx);

  cjinja_set_bool(test_ctx->ctx, "enabled", 1);

  // Test 7-tick performance for conditional rendering
  ASSERT_PERFORMANCE_7TICK(
      {
        char *result = cjinja_render_string("{% if enabled %}Yes{% else %}No{% endif %}", test_ctx->ctx);
        if (result)
          free(result);
      },
      10000);

  teardown_cjinja_test_context(test_ctx);
}

void test_performance_7tick_loop(void)
{
  CJinjaTestContext *test_ctx = setup_cjinja_test_context();
  ASSERT_NOT_NULL(test_ctx);

  char *items[] = {"a", "b", "c"};
  cjinja_set_array(test_ctx->ctx, "items", items, 3);

  // Test 7-tick performance for loop rendering
  ASSERT_PERFORMANCE_7TICK(
      {
        char *result = cjinja_render_string("{% for item in items %}{{ item }}{% endfor %}", test_ctx->ctx);
        if (result)
          free(result);
      },
      1000);

  teardown_cjinja_test_context(test_ctx);
}

void test_large_scale_rendering(void)
{
  CJinjaTestContext *test_ctx = setup_cjinja_test_context();
  ASSERT_NOT_NULL(test_ctx);

  // Create large template with many variables
  cjinja_set_var(test_ctx->ctx, "title", "Large Scale Test");

  char *items[100];
  for (int i = 0; i < 100; i++)
  {
    char item_str[32];
    snprintf(item_str, sizeof(item_str), "item_%d", i);
    items[i] = strdup(item_str);
  }
  cjinja_set_array(test_ctx->ctx, "items", items, 100);

  const char *template = "{{ title }}\n{% for item in items %}- {{ item }}\n{% endfor %}";
  char *result = cjinja_render_string(template, test_ctx->ctx);

  ASSERT_NOT_NULL(result);
  ASSERT_GREATER_THAN(1000, strlen(result)); // Should have substantial output

  // Cleanup
  for (int i = 0; i < 100; i++)
  {
    free(items[i]);
  }
  free(result);
  teardown_cjinja_test_context(test_ctx);
}

void test_memory_management(void)
{
  CJinjaTestContext *test_ctx = setup_cjinja_test_context();
  ASSERT_NOT_NULL(test_ctx);

  // Perform many rendering operations
  for (int i = 0; i < 1000; i++)
  {
    char template_str[64];
    snprintf(template_str, sizeof(template_str), "Test %d", i);

    char var_name[32];
    snprintf(var_name, sizeof(var_name), "number");

    cjinja_set_var(test_ctx->ctx, var_name, "42");

    char *result = cjinja_render_string(template_str, test_ctx->ctx);
    if (result)
    {
      free(result);
    }
  }

  // Verify context still works after many operations
  cjinja_set_var(test_ctx->ctx, "name", "7T Engine");
  char *result = cjinja_render_string("{{ name }}", test_ctx->ctx);
  ASSERT_NOT_NULL(result);
  ASSERT_STRING_EQUAL("7T Engine", result);
  free(result);

  teardown_cjinja_test_context(test_ctx);
}

// Test suite runner
void run_cjinja_tests(TestSuite *suite)
{
  printf("\n📝 Running CJinja Engine Unit Tests\n");
  printf("===================================\n");

  run_test(suite, "Engine Creation", test_engine_creation);
  run_test(suite, "Context Creation", test_context_creation);
  run_test(suite, "Variable Setting", test_variable_setting);
  run_test(suite, "Simple Variable Substitution", test_simple_variable_substitution);
  run_test(suite, "Boolean Variables", test_boolean_variables);
  run_test(suite, "Array Variables", test_array_variables);
  run_test(suite, "Conditional Rendering", test_conditional_rendering);
  run_test(suite, "Loop Rendering", test_loop_rendering);
  run_test(suite, "Filter Operations", test_filter_operations);
  run_test(suite, "Custom Filter", test_custom_filter);
  run_test(suite, "Template Caching", test_template_caching);
  run_test(suite, "Complex Template", test_complex_template);
  run_test(suite, "Error Handling", test_error_handling);
  run_test(suite, "7-Tick Variable Substitution Performance", test_performance_7tick_variable_substitution);
  run_test(suite, "7-Tick Conditional Performance", test_performance_7tick_conditional);
  run_test(suite, "7-Tick Loop Performance", test_performance_7tick_loop);
  run_test(suite, "Large Scale Rendering", test_large_scale_rendering);
  run_test(suite, "Memory Management", test_memory_management);
}

// Main test runner
int main()
{
  TestSuite *suite = create_test_suite("CJinja Engine");
  if (!suite)
  {
    printf("❌ Failed to create test suite\n");
    return 1;
  }

  run_cjinja_tests(suite);

  print_test_report(suite);

  int exit_code = suite->failed_tests > 0 ? 1 : 0;
  destroy_test_suite(suite);

  return exit_code;
}