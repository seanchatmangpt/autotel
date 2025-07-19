#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include "../compiler/src/cjinja.h"
#include "benchmark_framework.h"

// 80/20 Critical Unit Tests
// Focus: Most critical functionality that must work for the system to function

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

// Simplified SPARQL simulation
typedef struct
{
  int subject;
  int predicate;
  int object;
} Triple;

typedef struct
{
  Triple *triples;
  size_t count;
  size_t capacity;
} S7TEngine;

S7TEngine *s7t_create(size_t max_triples)
{
  S7TEngine *engine = malloc(sizeof(S7TEngine));
  engine->triples = malloc(max_triples * sizeof(Triple));
  engine->count = 0;
  engine->capacity = max_triples;
  return engine;
}

void s7t_add_triple(S7TEngine *engine, int s, int p, int o)
{
  if (engine->count < engine->capacity)
  {
    engine->triples[engine->count].subject = s;
    engine->triples[engine->count].predicate = p;
    engine->triples[engine->count].object = o;
    engine->count++;
  }
}

int s7t_ask_pattern(S7TEngine *engine, int s, int p, int o)
{
  for (size_t i = 0; i < engine->count; i++)
  {
    if (engine->triples[i].subject == s &&
        engine->triples[i].predicate == p &&
        engine->triples[i].object == o)
    {
      return 1;
    }
  }
  return 0;
}

void s7t_destroy(S7TEngine *engine)
{
  free(engine->triples);
  free(engine);
}

// Test 1: CJinja 7-tick path (most critical)
int test_cjinja_7tick_critical()
{
  TEST_SECTION("CJinja 7-Tick Path (Critical)");

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
int test_cjinja_49tick_critical()
{
  TEST_SECTION("CJinja 49-Tick Path (Critical)");

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

// Test 3: SPARQL critical functionality
int test_sparql_critical()
{
  TEST_SECTION("SPARQL Critical Functionality");

  S7TEngine *engine = s7t_create(1000);
  TEST_ASSERT(engine != NULL, "SPARQL engine creation");

  // Add critical test data
  s7t_add_triple(engine, 1, 1, 2); // (Alice, knows, Bob)
  s7t_add_triple(engine, 1, 2, 4); // (Alice, worksAt, TechCorp)
  s7t_add_triple(engine, 2, 2, 5); // (Bob, worksAt, StartupInc)

  TEST_ASSERT(engine->count == 3, "Triple count after addition");

  // Test critical pattern matching
  int alice_knows_bob = s7t_ask_pattern(engine, 1, 1, 2);
  TEST_ASSERT(alice_knows_bob == 1, "Alice knows Bob pattern");

  int alice_works_techcorp = s7t_ask_pattern(engine, 1, 2, 4);
  TEST_ASSERT(alice_works_techcorp == 1, "Alice works at TechCorp pattern");

  int bob_works_startup = s7t_ask_pattern(engine, 2, 2, 5);
  TEST_ASSERT(bob_works_startup == 1, "Bob works at StartupInc pattern");

  // Test non-existent pattern
  int alice_knows_dave = s7t_ask_pattern(engine, 1, 1, 6);
  TEST_ASSERT(alice_knows_dave == 0, "Non-existent pattern returns false");

  s7t_destroy(engine);
  return 1;
}

// Test 4: Integration (SPARQL + CJinja)
int test_integration_critical()
{
  TEST_SECTION("Integration (SPARQL + CJinja)");

  S7TEngine *sparql = s7t_create(1000);
  CJinjaEngine *cjinja = cjinja_create("./templates");
  CJinjaContext *ctx = cjinja_create_context();

  TEST_ASSERT(sparql != NULL, "SPARQL engine creation");
  TEST_ASSERT(cjinja != NULL, "CJinja engine creation");
  TEST_ASSERT(ctx != NULL, "CJinja context creation");

  // Add SPARQL data
  s7t_add_triple(sparql, 1, 1, 2); // (Alice, knows, Bob)
  s7t_add_triple(sparql, 1, 2, 4); // (Alice, worksAt, TechCorp)

  // Execute SPARQL query
  int alice_knows_bob = s7t_ask_pattern(sparql, 1, 1, 2);
  int alice_works_techcorp = s7t_ask_pattern(sparql, 1, 2, 4);

  // Format with CJinja (7-tick path)
  const char *template = "Alice knows Bob: {{knows_bob}}, Alice works at TechCorp: {{works_techcorp}}";
  cjinja_set_var(ctx, "knows_bob", alice_knows_bob ? "Yes" : "No");
  cjinja_set_var(ctx, "works_techcorp", alice_works_techcorp ? "Yes" : "No");

  char *result = cjinja_render_string_7tick(template, ctx);

  TEST_ASSERT(result != NULL, "Integration result not null");
  TEST_ASSERT(strstr(result, "Alice knows Bob: Yes") != NULL, "Integration Bob result");
  TEST_ASSERT(strstr(result, "Alice works at TechCorp: Yes") != NULL, "Integration TechCorp result");

  free(result);

  // Test 49-tick integration
  char *techcorp_employees[] = {"Alice"};
  cjinja_set_array(ctx, "employees", techcorp_employees, 1);

  const char *complex_template =
      "TechCorp Analysis\n"
      "=================\n"
      "Company: {{company_name | upper}}\n"
      "Employee Count: {{employees | length}}\n"
      "{% for employee in employees %}\n"
      "  - {{employee | capitalize}}\n"
      "{% endfor %}";

  cjinja_set_var(ctx, "company_name", "TechCorp");

  char *complex_result = cjinja_render_with_loops(complex_template, ctx);

  TEST_ASSERT(complex_result != NULL, "Complex integration result not null");
  TEST_ASSERT(strstr(complex_result, "TECHCORP") != NULL, "Complex integration company name");
  TEST_ASSERT(strstr(complex_result, "1") != NULL, "Complex integration employee count");
  TEST_ASSERT(strstr(complex_result, "Alice") != NULL, "Complex integration employee name");

  free(complex_result);

  cjinja_destroy_context(ctx);
  cjinja_destroy_engine(cjinja);
  s7t_destroy(sparql);

  return 1;
}

// Test 5: Benchmark framework critical functionality
int test_benchmark_framework_critical()
{
  TEST_SECTION("Benchmark Framework Critical Functionality");

  // Test suite creation
  BenchmarkSuite *suite = benchmark_suite_create("Critical Test Suite");
  TEST_ASSERT(suite != NULL, "Suite creation");
  TEST_ASSERT(suite->result_count == 0, "Initial result count");

  // Test data
  int test_value = 0;

  // Test benchmark execution
  BenchmarkResult result = benchmark_execute_single(
      "Critical Test",
      1000,
      [](void *data)
      { (*(int *)data)++; },
      &test_value);

  TEST_ASSERT(result.test_name != NULL, "Result test name not null");
  TEST_ASSERT(result.operations == 1000, "Result operations count");
  TEST_ASSERT(result.total_cycles > 0, "Result total cycles");
  TEST_ASSERT(result.total_time_ns > 0, "Result total time");
  TEST_ASSERT(result.avg_cycles_per_op > 0, "Result average cycles");
  TEST_ASSERT(result.avg_time_ns_per_op > 0, "Result average time");
  TEST_ASSERT(result.ops_per_sec > 0, "Result operations per second");

  // Test suite result addition
  benchmark_suite_add_result(suite, result);
  TEST_ASSERT(suite->result_count == 1, "Suite result count after addition");

  // Test statistics calculation
  benchmark_suite_calculate_stats(suite);
  TEST_ASSERT(suite->overall_score >= 0, "Suite overall score");

  // Test export functionality
  benchmark_suite_export_json(suite, "critical_test.json");
  benchmark_suite_export_csv(suite, "critical_test.csv");

  // Verify files were created
  FILE *json_file = fopen("critical_test.json", "r");
  TEST_ASSERT(json_file != NULL, "JSON file creation");
  if (json_file)
    fclose(json_file);

  FILE *csv_file = fopen("critical_test.csv", "r");
  TEST_ASSERT(csv_file != NULL, "CSV file creation");
  if (csv_file)
    fclose(csv_file);

  // Cleanup
  remove("critical_test.json");
  remove("critical_test.csv");
  benchmark_suite_destroy(suite);

  return 1;
}

// Test 6: Performance comparison (7-tick vs 49-tick)
int test_performance_comparison_critical()
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

// Test 7: Error handling critical
int test_error_handling_critical()
{
  TEST_SECTION("Error Handling Critical");

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

// Test 8: Memory safety critical
int test_memory_safety_critical()
{
  TEST_SECTION("Memory Safety Critical");

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

  // Test SPARQL memory safety
  for (int i = 0; i < 10; i++)
  {
    S7TEngine *engine = s7t_create(100);
    TEST_ASSERT(engine != NULL, "SPARQL engine creation in loop");

    s7t_add_triple(engine, i, i, i);
    int result = s7t_ask_pattern(engine, i, i, i);
    TEST_ASSERT(result == 1, "SPARQL usage in loop");

    s7t_destroy(engine);
  }

  return 1;
}

// Main test runner
int main()
{
  printf("80/20 Critical Unit Tests\n");
  printf("=========================\n");
  printf("Focus: Most critical functionality that must work\n\n");

  int total_tests = 8;
  int passed_tests = 0;

  // Run all critical tests
  if (test_cjinja_7tick_critical())
    passed_tests++;
  if (test_cjinja_49tick_critical())
    passed_tests++;
  if (test_sparql_critical())
    passed_tests++;
  if (test_integration_critical())
    passed_tests++;
  if (test_benchmark_framework_critical())
    passed_tests++;
  if (test_performance_comparison_critical())
    passed_tests++;
  if (test_error_handling_critical())
    passed_tests++;
  if (test_memory_safety_critical())
    passed_tests++;

  // Summary
  printf("\n=== Critical Test Summary ===\n");
  printf("Total tests: %d\n", total_tests);
  printf("Passed: %d\n", passed_tests);
  printf("Failed: %d\n", total_tests - passed_tests);
  printf("Success rate: %.1f%%\n", (passed_tests * 100.0) / total_tests);

  if (passed_tests == total_tests)
  {
    printf("\n🎉 All critical tests passed! System is ready for production.\n");
    return 0;
  }
  else
  {
    printf("\n❌ Critical tests failed. System needs attention.\n");
    return 1;
  }
}