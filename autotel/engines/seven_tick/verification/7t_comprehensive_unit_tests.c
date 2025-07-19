#include "7t_unit_test_framework.h"
#include "../runtime/src/seven_t_runtime.h"
#include "../compiler/src/cjinja.h"

// Comprehensive test context
typedef struct
{
  EngineState *engine;
  CJinjaEngine *cjinja_engine;
  CJinjaContext *cjinja_ctx;
  uint32_t test_data[100];
  uint32_t test_strings[50];
} ComprehensiveTestContext;

// Test setup and teardown
ComprehensiveTestContext *setup_comprehensive_test_context(void)
{
  ComprehensiveTestContext *ctx = malloc(sizeof(ComprehensiveTestContext));
  if (!ctx)
    return NULL;

  // Setup 7T engine
  ctx->engine = s7t_create_engine();
  if (!ctx->engine)
  {
    free(ctx);
    return NULL;
  }

  // Setup CJinja engine
  ctx->cjinja_engine = cjinja_create(NULL);
  if (!ctx->cjinja_engine)
  {
    s7t_destroy_engine(ctx->engine);
    free(ctx);
    return NULL;
  }

  // Setup CJinja context
  ctx->cjinja_ctx = cjinja_create_context();
  if (!ctx->cjinja_ctx)
  {
    cjinja_destroy(ctx->cjinja_engine);
    s7t_destroy_engine(ctx->engine);
    free(ctx);
    return NULL;
  }

  // Add CJinja variables
  cjinja_set_var(ctx->cjinja_ctx, "name", "7T Engine");
  cjinja_set_bool(ctx->cjinja_ctx, "enabled", 1);
  char *items[] = {"SPARQL", "SHACL", "CJinja"};
  cjinja_set_array(ctx->cjinja_ctx, "components", items, 3);

  // Setup test data for SPARQL/SHACL
  ctx->test_strings[0] = s7t_intern_string(ctx->engine, "ex:alice");
  ctx->test_strings[1] = s7t_intern_string(ctx->engine, "ex:bob");
  ctx->test_strings[2] = s7t_intern_string(ctx->engine, "ex:charlie");
  ctx->test_strings[3] = s7t_intern_string(ctx->engine, "ex:knows");
  ctx->test_strings[4] = s7t_intern_string(ctx->engine, "ex:likes");
  ctx->test_strings[5] = s7t_intern_string(ctx->engine, "ex:Person");
  ctx->test_strings[6] = s7t_intern_string(ctx->engine, "ex:hasName");
  ctx->test_strings[7] = s7t_intern_string(ctx->engine, "ex:hasEmail");

  // Add test triples
  s7t_add_triple(ctx->engine, ctx->test_strings[0], ctx->test_strings[3], ctx->test_strings[2]);
  s7t_add_triple(ctx->engine, ctx->test_strings[0], ctx->test_strings[4], ctx->test_strings[1]);
  s7t_add_triple(ctx->engine, ctx->test_strings[0], 0, ctx->test_strings[5]);
  s7t_add_triple(ctx->engine, ctx->test_strings[0], ctx->test_strings[6], ctx->test_strings[1]);
  s7t_add_triple(ctx->engine, ctx->test_strings[0], ctx->test_strings[7], ctx->test_strings[2]);

  // Setup test data array
  ctx->test_data[0] = ctx->test_strings[0]; // alice
  ctx->test_data[1] = ctx->test_strings[3]; // knows
  ctx->test_data[2] = ctx->test_strings[2]; // charlie
  ctx->test_data[3] = ctx->test_strings[5]; // Person
  ctx->test_data[4] = ctx->test_strings[6]; // hasName

  return ctx;
}

void teardown_comprehensive_test_context(ComprehensiveTestContext *ctx)
{
  if (ctx)
  {
    if (ctx->cjinja_ctx)
    {
      cjinja_destroy_context(ctx->cjinja_ctx);
    }
    if (ctx->cjinja_engine)
    {
      cjinja_destroy(ctx->cjinja_engine);
    }
    if (ctx->engine)
    {
      s7t_destroy_engine(ctx->engine);
    }
    free(ctx);
  }
}

// Integration test functions
void test_sparql_shacl_integration(void)
{
  ComprehensiveTestContext *ctx = setup_comprehensive_test_context();
  ASSERT_NOT_NULL(ctx);

  // Test SPARQL pattern matching
  ASSERT_TRUE(s7t_ask_pattern(ctx->engine, ctx->test_strings[0], ctx->test_strings[3], ctx->test_strings[2]));

  // Test SHACL class membership
  ASSERT_TRUE(shacl_check_class(ctx->engine, ctx->test_strings[0], ctx->test_strings[5]));

  // Test SHACL property existence
  ASSERT_TRUE(shacl_check_min_count(ctx->engine, ctx->test_strings[0], ctx->test_strings[6], 1));

  teardown_comprehensive_test_context(ctx);
}

void test_sparql_cjinja_integration(void)
{
  ComprehensiveTestContext *ctx = setup_comprehensive_test_context();
  ASSERT_NOT_NULL(ctx);

  // Test SPARQL query
  bool sparql_result = s7t_ask_pattern(ctx->engine, ctx->test_strings[0], ctx->test_strings[3], ctx->test_strings[2]);

  // Use result in CJinja template
  cjinja_set_bool(ctx->cjinja_ctx, "query_result", sparql_result);

  const char *template = "SPARQL query result: {% if query_result %}Found{% else %}Not found{% endif %}";
  char *result = cjinja_render_string(template, ctx->cjinja_ctx);

  ASSERT_NOT_NULL(result);
  ASSERT_STRING_EQUAL("SPARQL query result: Found", result);

  free(result);
  teardown_comprehensive_test_context(ctx);
}

void test_shacl_cjinja_integration(void)
{
  ComprehensiveTestContext *ctx = setup_comprehensive_test_context();
  ASSERT_NOT_NULL(ctx);

  // Test SHACL validation
  bool class_valid = shacl_check_class(ctx->engine, ctx->test_strings[0], ctx->test_strings[5]);
  bool property_valid = shacl_check_min_count(ctx->engine, ctx->test_strings[0], ctx->test_strings[6], 1);

  // Use validation results in CJinja template
  cjinja_set_bool(ctx->cjinja_ctx, "class_valid", class_valid);
  cjinja_set_bool(ctx->cjinja_ctx, "property_valid", property_valid);

  const char *template =
      "Validation Results:\n"
      "- Class membership: {% if class_valid %}✅{% else %}❌{% endif %}\n"
      "- Property existence: {% if property_valid %}✅{% else %}❌{% endif %}";

  char *result = cjinja_render_string(template, ctx->cjinja_ctx);

  ASSERT_NOT_NULL(result);
  const char *expected =
      "Validation Results:\n"
      "- Class membership: ✅\n"
      "- Property existence: ✅";

  ASSERT_STRING_EQUAL(expected, result);

  free(result);
  teardown_comprehensive_test_context(ctx);
}

void test_full_workflow_integration(void)
{
  ComprehensiveTestContext *ctx = setup_comprehensive_test_context();
  ASSERT_NOT_NULL(ctx);

  // Step 1: SPARQL query to find entities
  BitVector *subjects = s7t_get_subject_vector(ctx->engine, ctx->test_strings[3], ctx->test_strings[2]);
  ASSERT_NOT_NULL(subjects);
  ASSERT_TRUE(bitvec_test(subjects, ctx->test_strings[0]));

  // Step 2: SHACL validation of found entities
  bool entity_valid = shacl_check_class(ctx->engine, ctx->test_strings[0], ctx->test_strings[5]);
  ASSERT_TRUE(entity_valid);

  // Step 3: CJinja report generation
  cjinja_set_bool(ctx->cjinja_ctx, "entity_found", true);
  cjinja_set_bool(ctx->cjinja_ctx, "entity_valid", entity_valid);
  cjinja_set_var(ctx->cjinja_ctx, "entity_count", "1");

  const char *template =
      "# Workflow Report\n\n"
      "## Query Results\n"
      "{% if entity_found %}"
      "✅ Found {{ entity_count }} entity(ies)\n"
      "{% else %}"
      "❌ No entities found\n"
      "{% endif %}\n\n"
      "## Validation Results\n"
      "{% if entity_valid %}"
      "✅ All entities pass validation\n"
      "{% else %}"
      "❌ Some entities fail validation\n"
      "{% endif %}";

  char *result = cjinja_render_string(template, ctx->cjinja_ctx);

  ASSERT_NOT_NULL(result);
  const char *expected =
      "# Workflow Report\n\n"
      "## Query Results\n"
      "✅ Found 1 entity(ies)\n\n"
      "## Validation Results\n"
      "✅ All entities pass validation";

  ASSERT_STRING_EQUAL(expected, result);

  bitvec_destroy(subjects);
  free(result);
  teardown_comprehensive_test_context(ctx);
}

void test_performance_integration(void)
{
  ComprehensiveTestContext *ctx = setup_comprehensive_test_context();
  ASSERT_NOT_NULL(ctx);

  // Test integrated performance: SPARQL + SHACL + CJinja
  ASSERT_PERFORMANCE_7TICK(
      {
        // SPARQL query
        bool sparql_result = s7t_ask_pattern(ctx->engine, ctx->test_strings[0], ctx->test_strings[3], ctx->test_strings[2]);

        // SHACL validation
        bool shacl_result = shacl_check_class(ctx->engine, ctx->test_strings[0], ctx->test_strings[5]);

        // CJinja rendering
        cjinja_set_bool(ctx->cjinja_ctx, "valid", sparql_result && shacl_result);
        char *result = cjinja_render_string("{{ valid }}", ctx->cjinja_ctx);
        if (result)
          free(result);
      },
      10000);

  teardown_comprehensive_test_context(ctx);
}

void test_error_handling_integration(void)
{
  ComprehensiveTestContext *ctx = setup_comprehensive_test_context();
  ASSERT_NOT_NULL(ctx);

  // Test with non-existent data
  bool sparql_result = s7t_ask_pattern(ctx->engine, 999, 999, 999);
  bool shacl_result = shacl_check_class(ctx->engine, 999, 999);

  // Should handle gracefully
  ASSERT_FALSE(sparql_result);
  ASSERT_FALSE(shacl_result);

  // CJinja should handle the results
  cjinja_set_bool(ctx->cjinja_ctx, "sparql_ok", sparql_result);
  cjinja_set_bool(ctx->cjinja_ctx, "shacl_ok", shacl_result);

  const char *template = "SPARQL: {% if sparql_ok %}OK{% else %}FAIL{% endif %}, SHACL: {% if shacl_ok %}OK{% else %}FAIL{% endif %}";
  char *result = cjinja_render_string(template, ctx->cjinja_ctx);

  ASSERT_NOT_NULL(result);
  ASSERT_STRING_EQUAL("SPARQL: FAIL, SHACL: FAIL", result);

  free(result);
  teardown_comprehensive_test_context(ctx);
}

void test_memory_integration(void)
{
  ComprehensiveTestContext *ctx = setup_comprehensive_test_context();
  ASSERT_NOT_NULL(ctx);

  // Perform many integrated operations
  for (int i = 0; i < 1000; i++)
  {
    // SPARQL
    s7t_ask_pattern(ctx->engine, ctx->test_strings[0], ctx->test_strings[3], ctx->test_strings[2]);

    // SHACL
    shacl_check_class(ctx->engine, ctx->test_strings[0], ctx->test_strings[5]);

    // CJinja
    char *result = cjinja_render_string("{{ name }}", ctx->cjinja_ctx);
    if (result)
      free(result);
  }

  // Verify all components still work
  ASSERT_TRUE(s7t_ask_pattern(ctx->engine, ctx->test_strings[0], ctx->test_strings[3], ctx->test_strings[2]));
  ASSERT_TRUE(shacl_check_class(ctx->engine, ctx->test_strings[0], ctx->test_strings[5]));

  char *result = cjinja_render_string("{{ name }}", ctx->cjinja_ctx);
  ASSERT_NOT_NULL(result);
  ASSERT_STRING_EQUAL("7T Engine", result);
  free(result);

  teardown_comprehensive_test_context(ctx);
}

// Integration test suite runner
void run_integration_tests(TestSuite *suite)
{
  printf("\n🔗 Running 7T Engine Integration Tests\n");
  printf("=====================================\n");

  run_test(suite, "SPARQL-SHACL Integration", test_sparql_shacl_integration);
  run_test(suite, "SPARQL-CJinja Integration", test_sparql_cjinja_integration);
  run_test(suite, "SHACL-CJinja Integration", test_shacl_cjinja_integration);
  run_test(suite, "Full Workflow Integration", test_full_workflow_integration);
  run_test(suite, "Performance Integration", test_performance_integration);
  run_test(suite, "Error Handling Integration", test_error_handling_integration);
  run_test(suite, "Memory Integration", test_memory_integration);
}

// Main comprehensive test runner
int main()
{
  TestSuite *suite = create_test_suite("7T Engine Comprehensive");
  if (!suite)
  {
    printf("❌ Failed to create test suite\n");
    return 1;
  }

  run_integration_tests(suite);

  print_test_report(suite);

  int exit_code = suite->failed_tests > 0 ? 1 : 0;
  destroy_test_suite(suite);

  return exit_code;
}