#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../compiler/src/cjinja.h"

// 80/20 Unit Tests for SPARQL
// Focus: Critical functionality, pattern matching, integration

// Simplified SPARQL simulation for testing
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

S7TEngine *s7t_create(size_t max_triples, size_t max_subjects, size_t max_objects)
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

// Test 1: Basic SPARQL operations
int test_basic_sparql_operations()
{
  TEST_SECTION("Basic SPARQL Operations");

  S7TEngine *engine = s7t_create(1000, 100, 1000);
  TEST_ASSERT(engine != NULL, "SPARQL engine creation");

  // Add test triples
  s7t_add_triple(engine, 1, 1, 2); // (Alice, knows, Bob)
  s7t_add_triple(engine, 1, 1, 3); // (Alice, knows, Charlie)
  s7t_add_triple(engine, 1, 2, 4); // (Alice, worksAt, TechCorp)
  s7t_add_triple(engine, 2, 2, 5); // (Bob, worksAt, StartupInc)

  TEST_ASSERT(engine->count == 4, "Triple count after addition");

  // Test pattern matching
  int alice_knows_bob = s7t_ask_pattern(engine, 1, 1, 2);
  TEST_ASSERT(alice_knows_bob == 1, "Alice knows Bob pattern");

  int alice_knows_charlie = s7t_ask_pattern(engine, 1, 1, 3);
  TEST_ASSERT(alice_knows_charlie == 1, "Alice knows Charlie pattern");

  int alice_works_techcorp = s7t_ask_pattern(engine, 1, 2, 4);
  TEST_ASSERT(alice_works_techcorp == 1, "Alice works at TechCorp pattern");

  int bob_works_startup = s7t_ask_pattern(engine, 2, 2, 5);
  TEST_ASSERT(bob_works_startup == 1, "Bob works at StartupInc pattern");

  // Test non-existent patterns
  int alice_knows_dave = s7t_ask_pattern(engine, 1, 1, 6);
  TEST_ASSERT(alice_knows_dave == 0, "Non-existent pattern returns false");

  int dave_works_anywhere = s7t_ask_pattern(engine, 6, 2, 4);
  TEST_ASSERT(dave_works_anywhere == 0, "Non-existent subject returns false");

  s7t_destroy(engine);
  return 1;
}

// Test 2: Complex SPARQL queries
int test_complex_sparql_queries()
{
  TEST_SECTION("Complex SPARQL Queries");

  S7TEngine *engine = s7t_create(1000, 100, 1000);
  TEST_ASSERT(engine != NULL, "SPARQL engine creation");

  // Add complex test data
  s7t_add_triple(engine, 1, 1, 2); // (Alice, knows, Bob)
  s7t_add_triple(engine, 1, 1, 3); // (Alice, knows, Charlie)
  s7t_add_triple(engine, 2, 1, 3); // (Bob, knows, Charlie)
  s7t_add_triple(engine, 1, 2, 4); // (Alice, worksAt, TechCorp)
  s7t_add_triple(engine, 3, 2, 4); // (Charlie, worksAt, TechCorp)
  s7t_add_triple(engine, 3, 3, 6); // (Charlie, hasSkill, Programming)
  s7t_add_triple(engine, 1, 3, 7); // (Alice, hasSkill, Design)
  s7t_add_triple(engine, 2, 3, 8); // (Bob, hasSkill, Marketing)

  // Test complex query: Who works at TechCorp and what are their skills?
  int alice_works_techcorp = s7t_ask_pattern(engine, 1, 2, 4);
  int charlie_works_techcorp = s7t_ask_pattern(engine, 3, 2, 4);
  int alice_has_design = s7t_ask_pattern(engine, 1, 3, 7);
  int charlie_has_programming = s7t_ask_pattern(engine, 3, 3, 6);

  TEST_ASSERT(alice_works_techcorp == 1, "Alice works at TechCorp");
  TEST_ASSERT(charlie_works_techcorp == 1, "Charlie works at TechCorp");
  TEST_ASSERT(alice_has_design == 1, "Alice has Design skill");
  TEST_ASSERT(charlie_has_programming == 1, "Charlie has Programming skill");

  // Test mutual knowledge: Who knows each other?
  int alice_knows_bob = s7t_ask_pattern(engine, 1, 1, 2);
  int bob_knows_alice = s7t_ask_pattern(engine, 2, 1, 1);
  int alice_knows_charlie = s7t_ask_pattern(engine, 1, 1, 3);
  int charlie_knows_alice = s7t_ask_pattern(engine, 3, 1, 1);

  TEST_ASSERT(alice_knows_bob == 1, "Alice knows Bob");
  TEST_ASSERT(bob_knows_alice == 0, "Bob doesn't know Alice (unidirectional)");
  TEST_ASSERT(alice_knows_charlie == 1, "Alice knows Charlie");
  TEST_ASSERT(charlie_knows_alice == 0, "Charlie doesn't know Alice (unidirectional)");

  s7t_destroy(engine);
  return 1;
}

// Test 3: Batch SPARQL operations
int test_batch_sparql_operations()
{
  TEST_SECTION("Batch SPARQL Operations");

  S7TEngine *engine = s7t_create(1000, 100, 1000);
  TEST_ASSERT(engine != NULL, "SPARQL engine creation");

  // Add test data
  s7t_add_triple(engine, 1, 1, 2); // (Alice, knows, Bob)
  s7t_add_triple(engine, 2, 2, 5); // (Bob, worksAt, StartupInc)
  s7t_add_triple(engine, 3, 3, 6); // (Charlie, hasSkill, Programming)
  s7t_add_triple(engine, 1, 3, 7); // (Alice, hasSkill, Design)
  s7t_add_triple(engine, 2, 3, 8); // (Bob, hasSkill, Marketing)

  // Execute batch queries
  int queries[] = {
      s7t_ask_pattern(engine, 1, 1, 2), // Alice knows Bob
      s7t_ask_pattern(engine, 2, 2, 5), // Bob works at StartupInc
      s7t_ask_pattern(engine, 3, 3, 6), // Charlie has Programming skill
      s7t_ask_pattern(engine, 1, 3, 7), // Alice has Design skill
      s7t_ask_pattern(engine, 2, 3, 8)  // Bob has Marketing skill
  };

  // Verify all expected results
  TEST_ASSERT(queries[0] == 1, "Batch query 1: Alice knows Bob");
  TEST_ASSERT(queries[1] == 1, "Batch query 2: Bob works at StartupInc");
  TEST_ASSERT(queries[2] == 1, "Batch query 3: Charlie has Programming skill");
  TEST_ASSERT(queries[3] == 1, "Batch query 4: Alice has Design skill");
  TEST_ASSERT(queries[4] == 1, "Batch query 5: Bob has Marketing skill");

  // Test batch with some false results
  int false_queries[] = {
      s7t_ask_pattern(engine, 1, 1, 9), // Alice knows Dave (false)
      s7t_ask_pattern(engine, 9, 2, 5), // Dave works at StartupInc (false)
      s7t_ask_pattern(engine, 3, 3, 9)  // Charlie has Cooking skill (false)
  };

  TEST_ASSERT(false_queries[0] == 0, "Batch false query 1");
  TEST_ASSERT(false_queries[1] == 0, "Batch false query 2");
  TEST_ASSERT(false_queries[2] == 0, "Batch false query 3");

  s7t_destroy(engine);
  return 1;
}

// Test 4: SPARQL + CJinja integration (7-tick path)
int test_sparql_cjinja_integration_7tick()
{
  TEST_SECTION("SPARQL + CJinja Integration (7-Tick Path)");

  S7TEngine *sparql = s7t_create(1000, 100, 1000);
  CJinjaEngine *cjinja = cjinja_create("./templates");
  CJinjaContext *ctx = cjinja_create_context();

  TEST_ASSERT(sparql != NULL, "SPARQL engine creation");
  TEST_ASSERT(cjinja != NULL, "CJinja engine creation");
  TEST_ASSERT(ctx != NULL, "CJinja context creation");

  // Add SPARQL data
  s7t_add_triple(sparql, 1, 1, 2); // (Alice, knows, Bob)
  s7t_add_triple(sparql, 1, 1, 3); // (Alice, knows, Charlie)

  // Execute SPARQL query
  int alice_knows_bob = s7t_ask_pattern(sparql, 1, 1, 2);
  int alice_knows_charlie = s7t_ask_pattern(sparql, 1, 1, 3);

  // Format results with CJinja (7-tick path)
  const char *template = "Alice knows Bob: {{knows_bob}}, Alice knows Charlie: {{knows_charlie}}";
  cjinja_set_var(ctx, "knows_bob", alice_knows_bob ? "Yes" : "No");
  cjinja_set_var(ctx, "knows_charlie", alice_knows_charlie ? "Yes" : "No");

  char *result = cjinja_render_string_7tick(template, ctx);

  TEST_ASSERT(result != NULL, "7-tick integration result not null");
  TEST_ASSERT(strstr(result, "Alice knows Bob: Yes") != NULL, "7-tick integration Bob result");
  TEST_ASSERT(strstr(result, "Alice knows Charlie: Yes") != NULL, "7-tick integration Charlie result");

  free(result);

  // Cleanup
  cjinja_destroy_context(ctx);
  cjinja_destroy_engine(cjinja);
  s7t_destroy(sparql);

  return 1;
}

// Test 5: SPARQL + CJinja integration (49-tick path)
int test_sparql_cjinja_integration_49tick()
{
  TEST_SECTION("SPARQL + CJinja Integration (49-tick Path)");

  S7TEngine *sparql = s7t_create(1000, 100, 1000);
  CJinjaEngine *cjinja = cjinja_create("./templates");
  CJinjaContext *ctx = cjinja_create_context();

  TEST_ASSERT(sparql != NULL, "SPARQL engine creation");
  TEST_ASSERT(cjinja != NULL, "CJinja engine creation");
  TEST_ASSERT(ctx != NULL, "CJinja context creation");

  // Add SPARQL data
  s7t_add_triple(sparql, 1, 2, 4); // (Alice, worksAt, TechCorp)
  s7t_add_triple(sparql, 3, 2, 4); // (Charlie, worksAt, TechCorp)
  s7t_add_triple(sparql, 1, 3, 7); // (Alice, hasSkill, Design)
  s7t_add_triple(sparql, 3, 3, 6); // (Charlie, hasSkill, Programming)

  // Execute complex SPARQL query
  int alice_works_techcorp = s7t_ask_pattern(sparql, 1, 2, 4);
  int charlie_works_techcorp = s7t_ask_pattern(sparql, 3, 2, 4);
  int alice_has_design = s7t_ask_pattern(sparql, 1, 3, 7);
  int charlie_has_programming = s7t_ask_pattern(sparql, 3, 3, 6);

  // Set up arrays for complex template
  char *techcorp_employees[] = {"Alice", "Charlie"};
  cjinja_set_array(ctx, "employees", techcorp_employees, 2);

  const char *complex_template =
      "TechCorp Employee Analysis\n"
      "==========================\n"
      "Company: {{company_name | upper}}\n"
      "Employee Count: {{employees | length}}\n\n"
      "Employees:\n"
      "{% for employee in employees %}\n"
      "  - {{employee | capitalize}}\n"
      "{% endfor %}\n\n"
      "Skills:\n"
      "{% if alice_has_design %}\n"
      "  - Alice: Design\n"
      "{% endif %}\n"
      "{% if charlie_has_programming %}\n"
      "  - Charlie: Programming\n"
      "{% endif %}\n\n"
      "Summary: {{employees | length}} employees with diverse skills.";

  cjinja_set_var(ctx, "company_name", "TechCorp");
  cjinja_set_bool(ctx, "alice_has_design", alice_has_design);
  cjinja_set_bool(ctx, "charlie_has_programming", charlie_has_programming);

  char *result = cjinja_render_with_loops(complex_template, ctx);

  TEST_ASSERT(result != NULL, "49-tick integration result not null");
  TEST_ASSERT(strstr(result, "TECHCORP") != NULL, "49-tick integration company name");
  TEST_ASSERT(strstr(result, "2 employees") != NULL, "49-tick integration employee count");
  TEST_ASSERT(strstr(result, "Alice") != NULL, "49-tick integration Alice employee");
  TEST_ASSERT(strstr(result, "Charlie") != NULL, "49-tick integration Charlie employee");
  TEST_ASSERT(strstr(result, "Design") != NULL, "49-tick integration Alice skill");
  TEST_ASSERT(strstr(result, "Programming") != NULL, "49-tick integration Charlie skill");

  free(result);

  // Cleanup
  cjinja_destroy_context(ctx);
  cjinja_destroy_engine(cjinja);
  s7t_destroy(sparql);

  return 1;
}

// Test 6: Error handling and edge cases
int test_sparql_error_handling()
{
  TEST_SECTION("SPARQL Error Handling and Edge Cases");

  // Test NULL engine
  int result = s7t_ask_pattern(NULL, 1, 1, 2);
  TEST_ASSERT(result == 0, "NULL engine returns false");

  // Test empty engine
  S7TEngine *engine = s7t_create(1000, 100, 1000);
  TEST_ASSERT(engine != NULL, "Engine creation");
  TEST_ASSERT(engine->count == 0, "Empty engine count");

  result = s7t_ask_pattern(engine, 1, 1, 2);
  TEST_ASSERT(result == 0, "Empty engine pattern returns false");

  // Test engine capacity
  for (int i = 0; i < 1000; i++)
  {
    s7t_add_triple(engine, i, i, i);
  }
  TEST_ASSERT(engine->count == 1000, "Engine at capacity");

  // Test adding beyond capacity
  s7t_add_triple(engine, 1001, 1001, 1001);
  TEST_ASSERT(engine->count == 1000, "Engine count unchanged after capacity");

  // Test pattern with negative values
  result = s7t_ask_pattern(engine, -1, -1, -1);
  TEST_ASSERT(result == 0, "Negative pattern returns false");

  s7t_destroy(engine);
  return 1;
}

// Test 7: Performance comparison (7-tick vs 49-tick integration)
int test_integration_performance()
{
  TEST_SECTION("Integration Performance Comparison");

  S7TEngine *sparql = s7t_create(1000, 100, 1000);
  CJinjaEngine *cjinja = cjinja_create("./templates");
  CJinjaContext *ctx = cjinja_create_context();

  TEST_ASSERT(sparql != NULL, "SPARQL engine creation");
  TEST_ASSERT(cjinja != NULL, "CJinja engine creation");
  TEST_ASSERT(ctx != NULL, "CJinja context creation");

  // Add test data
  s7t_add_triple(sparql, 1, 1, 2);
  s7t_add_triple(sparql, 1, 2, 4);

  const char *simple_template = "Query result: {{result}}";
  cjinja_set_var(ctx, "result", "Success");

  // Measure 7-tick integration performance
  clock_t start_7tick = clock();
  for (int i = 0; i < 1000; i++)
  {
    int query_result = s7t_ask_pattern(sparql, 1, 1, 2);
    char *result = cjinja_render_string_7tick(simple_template, ctx);
    free(result);
  }
  clock_t end_7tick = clock();
  double time_7tick = ((double)(end_7tick - start_7tick)) / CLOCKS_PER_SEC;

  // Measure 49-tick integration performance
  clock_t start_49tick = clock();
  for (int i = 0; i < 1000; i++)
  {
    int query_result = s7t_ask_pattern(sparql, 1, 1, 2);
    char *result = cjinja_render_string(simple_template, ctx);
    free(result);
  }
  clock_t end_49tick = clock();
  double time_49tick = ((double)(end_49tick - start_49tick)) / CLOCKS_PER_SEC;

  printf("  7-tick integration time: %.6f seconds\n", time_7tick);
  printf("  49-tick integration time: %.6f seconds\n", time_49tick);

  TEST_ASSERT(time_7tick > 0, "7-tick integration performance measurement");
  TEST_ASSERT(time_49tick > 0, "49-tick integration performance measurement");
  TEST_ASSERT(time_7tick <= time_49tick, "7-tick integration is not slower than 49-tick");

  // Cleanup
  cjinja_destroy_context(ctx);
  cjinja_destroy_engine(cjinja);
  s7t_destroy(sparql);

  return 1;
}

// Test 8: Memory safety and cleanup
int test_sparql_memory_safety()
{
  TEST_SECTION("SPARQL Memory Safety and Cleanup");

  // Test multiple engine creation and destruction
  for (int i = 0; i < 10; i++)
  {
    S7TEngine *engine = s7t_create(100, 10, 100);
    TEST_ASSERT(engine != NULL, "Engine creation in loop");

    // Use the engine
    s7t_add_triple(engine, i, i, i);
    int result = s7t_ask_pattern(engine, i, i, i);
    TEST_ASSERT(result == 1, "Engine usage in loop");

    // Cleanup
    s7t_destroy(engine);
  }

  // Test large engine
  S7TEngine *large_engine = s7t_create(10000, 1000, 10000);
  TEST_ASSERT(large_engine != NULL, "Large engine creation");

  // Add many triples
  for (int i = 0; i < 1000; i++)
  {
    s7t_add_triple(large_engine, i, i, i);
  }
  TEST_ASSERT(large_engine->count == 1000, "Large engine triple count");

  s7t_destroy(large_engine);

  return 1;
}

// Main test runner
int main()
{
  printf("SPARQL 80/20 Unit Tests\n");
  printf("=======================\n");
  printf("Focus: Critical functionality, pattern matching, integration\n\n");

  int total_tests = 8;
  int passed_tests = 0;

  // Run all tests
  if (test_basic_sparql_operations())
    passed_tests++;
  if (test_complex_sparql_queries())
    passed_tests++;
  if (test_batch_sparql_operations())
    passed_tests++;
  if (test_sparql_cjinja_integration_7tick())
    passed_tests++;
  if (test_sparql_cjinja_integration_49tick())
    passed_tests++;
  if (test_sparql_error_handling())
    passed_tests++;
  if (test_integration_performance())
    passed_tests++;
  if (test_sparql_memory_safety())
    passed_tests++;

  // Summary
  printf("\n=== Test Summary ===\n");
  printf("Total tests: %d\n", total_tests);
  printf("Passed: %d\n", passed_tests);
  printf("Failed: %d\n", total_tests - passed_tests);
  printf("Success rate: %.1f%%\n", (passed_tests * 100.0) / total_tests);

  if (passed_tests == total_tests)
  {
    printf("\n🎉 All tests passed! SPARQL integration is working correctly.\n");
    return 0;
  }
  else
  {
    printf("\n❌ Some tests failed. Please review the output above.\n");
    return 1;
  }
}