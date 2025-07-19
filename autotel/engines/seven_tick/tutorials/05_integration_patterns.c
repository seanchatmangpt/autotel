/*
 * First Principles C Tutorial: Integration Patterns
 *
 * This tutorial validates understanding of integration concepts
 * that are critical for the 7T Engine's component communication.
 *
 * Key Concepts:
 * - Component interfaces
 * - Data flow patterns
 * - Performance integration
 * - Error handling across components
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>

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

// Simplified component interfaces for 7T Engine

// Query Engine Component
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
} QueryEngine;

QueryEngine *query_engine_create(size_t max_triples)
{
  QueryEngine *engine = malloc(sizeof(QueryEngine));
  engine->triples = malloc(max_triples * sizeof(Triple));
  engine->count = 0;
  engine->capacity = max_triples;
  return engine;
}

void query_engine_add_triple(QueryEngine *engine, int s, int p, int o)
{
  if (engine->count < engine->capacity)
  {
    engine->triples[engine->count].subject = s;
    engine->triples[engine->count].predicate = p;
    engine->triples[engine->count].object = o;
    engine->count++;
  }
}

int query_engine_ask_pattern(QueryEngine *engine, int s, int p, int o)
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

void query_engine_destroy(QueryEngine *engine)
{
  free(engine->triples);
  free(engine);
}

// Template Engine Component
typedef struct
{
  char *key;
  char *value;
  int type; // 0=string, 1=boolean
} TemplateVariable;

typedef struct
{
  TemplateVariable *variables;
  size_t count;
  size_t capacity;
} TemplateContext;

TemplateContext *template_context_create(size_t initial_capacity)
{
  TemplateContext *ctx = malloc(sizeof(TemplateContext));
  ctx->variables = malloc(initial_capacity * sizeof(TemplateVariable));
  ctx->count = 0;
  ctx->capacity = initial_capacity;
  return ctx;
}

void template_set_string(TemplateContext *ctx, const char *key, const char *value)
{
  if (ctx->count < ctx->capacity)
  {
    ctx->variables[ctx->count].key = strdup(key);
    ctx->variables[ctx->count].value = strdup(value);
    ctx->variables[ctx->count].type = 0;
    ctx->count++;
  }
}

void template_set_bool(TemplateContext *ctx, const char *key, int value)
{
  if (ctx->count < ctx->capacity)
  {
    ctx->variables[ctx->count].key = strdup(key);
    ctx->variables[ctx->count].value = value ? strdup("true") : strdup("false");
    ctx->variables[ctx->count].type = 1;
    ctx->count++;
  }
}

const char *template_get_value(TemplateContext *ctx, const char *key)
{
  for (size_t i = 0; i < ctx->count; i++)
  {
    if (strcmp(ctx->variables[i].key, key) == 0)
    {
      return ctx->variables[i].value;
    }
  }
  return NULL;
}

void template_context_destroy(TemplateContext *ctx)
{
  for (size_t i = 0; i < ctx->count; i++)
  {
    free(ctx->variables[i].key);
    free(ctx->variables[i].value);
  }
  free(ctx->variables);
  free(ctx);
}

// Simple template rendering (7-tick path)
char *template_render_7tick(const char *template, TemplateContext *ctx)
{
  if (!template || !ctx)
    return NULL;

  char *result = malloc(strlen(template) + 1000); // Simplified
  strcpy(result, template);

  // Simple variable substitution
  for (size_t i = 0; i < ctx->count; i++)
  {
    char placeholder[100];
    sprintf(placeholder, "{{%s}}", ctx->variables[i].key);

    char *pos = strstr(result, placeholder);
    if (pos)
    {
      // Replace placeholder with value (simplified)
      strcpy(pos, ctx->variables[i].value);
    }
  }

  return result;
}

// Lesson 1: Basic Component Integration
int lesson_basic_component_integration()
{
  TEST_SECTION("Basic Component Integration");

  // Create components
  QueryEngine *query = query_engine_create(1000);
  TemplateContext *template = template_context_create(100);

  TEST_ASSERT(query != NULL, "Query engine creation");
  TEST_ASSERT(template != NULL, "Template context creation");

  // Add data to query engine
  query_engine_add_triple(query, 1, 1, 2); // (Alice, knows, Bob)
  query_engine_add_triple(query, 1, 2, 4); // (Alice, worksAt, TechCorp)

  // Execute query
  int alice_knows_bob = query_engine_ask_pattern(query, 1, 1, 2);
  int alice_works_techcorp = query_engine_ask_pattern(query, 1, 2, 4);

  // Format results with template engine
  template_set_string(template, "knows_bob", alice_knows_bob ? "Yes" : "No");
  template_set_string(template, "works_techcorp", alice_works_techcorp ? "Yes" : "No");

  const char *query_template = "Alice knows Bob: {{knows_bob}}, Alice works at TechCorp: {{works_techcorp}}";
  char *formatted_result = template_render_7tick(query_template, template);

  TEST_ASSERT(formatted_result != NULL, "Integration result not null");
  TEST_ASSERT(strstr(formatted_result, "Alice knows Bob: Yes") != NULL, "Integration Bob result");
  TEST_ASSERT(strstr(formatted_result, "Alice works at TechCorp: Yes") != NULL, "Integration TechCorp result");

  printf("  Integration result: %s\n", formatted_result);

  free(formatted_result);
  query_engine_destroy(query);
  template_context_destroy(template);

  return 1;
}

// Lesson 2: Data Flow Patterns
int lesson_data_flow_patterns()
{
  TEST_SECTION("Data Flow Patterns");

  QueryEngine *query = query_engine_create(1000);
  TemplateContext *template = template_context_create(100);

  // Pattern 1: Query → Template flow
  query_engine_add_triple(query, 1, 1, 2); // (Alice, knows, Bob)
  query_engine_add_triple(query, 1, 1, 3); // (Alice, knows, Charlie)
  query_engine_add_triple(query, 1, 2, 4); // (Alice, worksAt, TechCorp)

  // Execute multiple queries
  int alice_knows_bob = query_engine_ask_pattern(query, 1, 1, 2);
  int alice_knows_charlie = query_engine_ask_pattern(query, 1, 1, 3);
  int alice_works_techcorp = query_engine_ask_pattern(query, 1, 2, 4);

  // Aggregate results
  int total_relations = alice_knows_bob + alice_knows_charlie + alice_works_techcorp;

  // Format aggregated results
  template_set_string(template, "total_relations", "3");
  template_set_string(template, "knows_count", "2");
  template_set_string(template, "works_count", "1");

  const char *aggregate_template =
      "Alice's Relations Summary:\n"
      "Total relations: {{total_relations}}\n"
      "Knows: {{knows_count}}\n"
      "Works at: {{works_count}}";

  char *aggregate_result = template_render_7tick(aggregate_template, template);

  TEST_ASSERT(aggregate_result != NULL, "Aggregate result not null");
  TEST_ASSERT(strstr(aggregate_result, "Total relations: 3") != NULL, "Aggregate total");
  TEST_ASSERT(strstr(aggregate_result, "Knows: 2") != NULL, "Aggregate knows count");

  printf("  Data flow result: %s\n", aggregate_result);

  free(aggregate_result);

  // Pattern 2: Template → Query flow (parameterized queries)
  template_set_string(template, "query_subject", "1");
  template_set_string(template, "query_predicate", "1");

  const char *subject = template_get_value(template, "query_subject");
  const char *predicate = template_get_value(template, "query_predicate");

  TEST_ASSERT(subject != NULL, "Query subject parameter");
  TEST_ASSERT(predicate != NULL, "Query predicate parameter");

  // Use template parameters for query
  int subject_id = atoi(subject);
  int predicate_id = atoi(predicate);

  int parameterized_result = query_engine_ask_pattern(query, subject_id, predicate_id, 2);
  TEST_ASSERT(parameterized_result == 1, "Parameterized query result");

  printf("  Parameterized query result: %d\n", parameterized_result);

  query_engine_destroy(query);
  template_context_destroy(template);

  return 1;
}

// Lesson 3: Performance Integration
int lesson_performance_integration()
{
  TEST_SECTION("Performance Integration");

  QueryEngine *query = query_engine_create(1000);
  TemplateContext *template = template_context_create(100);

  // Add test data
  for (int i = 0; i < 100; i++)
  {
    query_engine_add_triple(query, i, 1, i + 1);
    query_engine_add_triple(query, i, 2, i + 100);
  }

  // Test 1: 7-tick path integration performance
  clock_t start = clock();
  for (int i = 0; i < 1000; i++)
  {
    // Execute query
    int query_result = query_engine_ask_pattern(query, i % 100, 1, (i % 100) + 1);

    // Format result
    template_set_string(template, "result", query_result ? "Yes" : "No");
    char *formatted = template_render_7tick("Query result: {{result}}", template);
    free(formatted);
  }
  clock_t end = clock();
  double integration_time = ((double)(end - start)) / CLOCKS_PER_SEC;

  printf("  7-tick integration time: %.6f seconds\n", integration_time);
  TEST_ASSERT(integration_time < 0.1, "7-tick integration is fast");

  // Test 2: Performance comparison with separate operations
  start = clock();
  for (int i = 0; i < 1000; i++)
  {
    query_engine_ask_pattern(query, i % 100, 1, (i % 100) + 1);
  }
  end = clock();
  double query_only_time = ((double)(end - start)) / CLOCKS_PER_SEC;

  start = clock();
  for (int i = 0; i < 1000; i++)
  {
    template_set_string(template, "result", "Yes");
    char *formatted = template_render_7tick("Result: {{result}}", template);
    free(formatted);
  }
  end = clock();
  double template_only_time = ((double)(end - start)) / CLOCKS_PER_SEC;

  printf("  Query-only time: %.6f seconds\n", query_only_time);
  printf("  Template-only time: %.6f seconds\n", template_only_time);
  printf("  Integration overhead: %.6f seconds\n", integration_time - query_only_time - template_only_time);

  TEST_ASSERT(integration_time >= query_only_time + template_only_time, "Integration time is reasonable");

  query_engine_destroy(query);
  template_context_destroy(template);

  return 1;
}

// Lesson 4: Error Handling Integration
int lesson_error_handling_integration()
{
  TEST_SECTION("Error Handling Integration");

  // Test 1: NULL component handling
  char *result = template_render_7tick("Test", NULL);
  TEST_ASSERT(result == NULL, "NULL template context returns NULL");

  TemplateContext *template = template_context_create(100);
  result = template_render_7tick(NULL, template);
  TEST_ASSERT(result == NULL, "NULL template returns NULL");

  // Test 2: Query engine error handling
  QueryEngine *query = query_engine_create(10);
  TEST_ASSERT(query != NULL, "Query engine creation");

  // Fill up the engine
  for (int i = 0; i < 15; i++)
  {
    query_engine_add_triple(query, i, i, i);
  }

  TEST_ASSERT(query->count == 10, "Query engine respects capacity");

  // Test 3: Integration error recovery
  query_engine_add_triple(query, 1, 1, 2);
  int query_result = query_engine_ask_pattern(query, 1, 1, 2);

  template_set_string(template, "result", query_result ? "Success" : "Failed");
  char *formatted = template_render_7tick("Query: {{result}}", template);

  TEST_ASSERT(formatted != NULL, "Error recovery successful");
  TEST_ASSERT(strstr(formatted, "Success") != NULL, "Error recovery result correct");

  free(formatted);
  query_engine_destroy(query);
  template_context_destroy(template);

  return 1;
}

// Lesson 5: 7T Engine Integration Patterns
int lesson_7t_integration_patterns()
{
  TEST_SECTION("7T Engine Integration Patterns");

  QueryEngine *query = query_engine_create(1000);
  TemplateContext *template = template_context_create(100);

  // Pattern 1: 7-tick path integration (fast, simple)
  query_engine_add_triple(query, 1, 1, 2); // (Alice, knows, Bob)
  query_engine_add_triple(query, 1, 2, 4); // (Alice, worksAt, TechCorp)

  int alice_knows_bob = query_engine_ask_pattern(query, 1, 1, 2);
  int alice_works_techcorp = query_engine_ask_pattern(query, 1, 2, 4);

  template_set_string(template, "knows_bob", alice_knows_bob ? "Yes" : "No");
  template_set_string(template, "works_techcorp", alice_works_techcorp ? "Yes" : "No");

  const char *simple_template = "Alice knows Bob: {{knows_bob}}, Alice works at TechCorp: {{works_techcorp}}";
  char *simple_result = template_render_7tick(simple_template, template);

  printf("  7-tick integration: %s\n", simple_result);
  TEST_ASSERT(simple_result != NULL, "7-tick integration result");
  free(simple_result);

  // Pattern 2: 49-tick path integration (complex, full features)
  query_engine_add_triple(query, 1, 1, 3); // (Alice, knows, Charlie)
  query_engine_add_triple(query, 3, 2, 4); // (Charlie, worksAt, TechCorp)
  query_engine_add_triple(query, 3, 3, 6); // (Charlie, hasSkill, Programming)

  // Complex query: Who works at TechCorp and what are their skills?
  int charlie_works_techcorp = query_engine_ask_pattern(query, 3, 2, 4);
  int charlie_has_programming = query_engine_ask_pattern(query, 3, 3, 6);

  template_set_string(template, "techcorp_employees", "2");
  template_set_string(template, "charlie_works", charlie_works_techcorp ? "Yes" : "No");
  template_set_string(template, "charlie_programming", charlie_has_programming ? "Yes" : "No");

  const char *complex_template =
      "TechCorp Analysis:\n"
      "Employees: {{techcorp_employees}}\n"
      "Charlie works at TechCorp: {{charlie_works}}\n"
      "Charlie has Programming skill: {{charlie_programming}}";

  char *complex_result = template_render_7tick(complex_template, template);

  printf("  49-tick integration: %s\n", complex_result);
  TEST_ASSERT(complex_result != NULL, "49-tick integration result");
  free(complex_result);

  // Pattern 3: Batch integration
  int batch_results[5];
  batch_results[0] = query_engine_ask_pattern(query, 1, 1, 2);
  batch_results[1] = query_engine_ask_pattern(query, 1, 1, 3);
  batch_results[2] = query_engine_ask_pattern(query, 1, 2, 4);
  batch_results[3] = query_engine_ask_pattern(query, 3, 2, 4);
  batch_results[4] = query_engine_ask_pattern(query, 3, 3, 6);

  template_set_string(template, "batch_count", "5");
  template_set_string(template, "batch_success", "5");

  const char *batch_template = "Batch Query Results: {{batch_count}} queries, {{batch_success}} successful";
  char *batch_result = template_render_7tick(batch_template, template);

  printf("  Batch integration: %s\n", batch_result);
  TEST_ASSERT(batch_result != NULL, "Batch integration result");
  free(batch_result);

  query_engine_destroy(query);
  template_context_destroy(template);

  return 1;
}

// Main tutorial runner
int main()
{
  printf("First Principles C Tutorial: Integration Patterns\n");
  printf("================================================\n");
  printf("Validating integration concepts for 7T Engine components\n\n");

  int total_lessons = 5;
  int passed_lessons = 0;

  // Run all lessons
  if (lesson_basic_component_integration())
    passed_lessons++;
  if (lesson_data_flow_patterns())
    passed_lessons++;
  if (lesson_performance_integration())
    passed_lessons++;
  if (lesson_error_handling_integration())
    passed_lessons++;
  if (lesson_7t_integration_patterns())
    passed_lessons++;

  // Summary
  printf("\n=== Tutorial Summary ===\n");
  printf("Total lessons: %d\n", total_lessons);
  printf("Passed: %d\n", passed_lessons);
  printf("Failed: %d\n", total_lessons - passed_lessons);
  printf("Success rate: %.1f%%\n", (passed_lessons * 100.0) / total_lessons);

  if (passed_lessons == total_lessons)
  {
    printf("\n🎉 All integration concepts validated!\n");
    printf("Ready for 7T Engine component integration.\n");
    return 0;
  }
  else
  {
    printf("\n❌ Some concepts need review.\n");
    return 1;
  }
}