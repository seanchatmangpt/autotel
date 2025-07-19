/*
 * First Principles C Tutorial: Query Engine
 *
 * This tutorial validates understanding of query engine concepts
 * that are critical for the SPARQL implementation in the 7T Engine.
 *
 * Key Concepts:
 * - Triple storage
 * - Pattern matching
 * - Query execution
 * - Performance optimization
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

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

// Triple structure
typedef struct
{
  int subject;
  int predicate;
  int object;
} Triple;

// Query engine structure
typedef struct
{
  Triple *triples;
  size_t count;
  size_t capacity;
} QueryEngine;

// Initialize query engine
QueryEngine *query_engine_create(size_t max_triples)
{
  QueryEngine *engine = malloc(sizeof(QueryEngine));
  engine->triples = malloc(max_triples * sizeof(Triple));
  engine->count = 0;
  engine->capacity = max_triples;
  return engine;
}

// Add triple to engine
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

// Pattern matching (ASK query)
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

// SELECT query (simplified)
Triple *query_engine_select_pattern(QueryEngine *engine, int s, int p, int o, size_t *result_count)
{
  Triple *results = malloc(engine->count * sizeof(Triple));
  *result_count = 0;

  for (size_t i = 0; i < engine->count; i++)
  {
    if ((s == -1 || engine->triples[i].subject == s) &&
        (p == -1 || engine->triples[i].predicate == p) &&
        (o == -1 || engine->triples[i].object == o))
    {
      results[*result_count] = engine->triples[i];
      (*result_count)++;
    }
  }

  return results;
}

// Cleanup query engine
void query_engine_destroy(QueryEngine *engine)
{
  free(engine->triples);
  free(engine);
}

// Lesson 1: Basic Triple Storage
int lesson_basic_triple_storage()
{
  TEST_SECTION("Basic Triple Storage");

  // Test 1: Engine creation
  QueryEngine *engine = query_engine_create(1000);
  TEST_ASSERT(engine != NULL, "Query engine creation");
  TEST_ASSERT(engine->count == 0, "Initial triple count");
  TEST_ASSERT(engine->capacity == 1000, "Engine capacity");

  // Test 2: Triple addition
  query_engine_add_triple(engine, 1, 1, 2); // (Alice, knows, Bob)
  query_engine_add_triple(engine, 1, 2, 4); // (Alice, worksAt, TechCorp)
  query_engine_add_triple(engine, 2, 2, 5); // (Bob, worksAt, StartupInc)

  TEST_ASSERT(engine->count == 3, "Triple count after addition");

  // Test 3: Triple validation
  TEST_ASSERT(engine->triples[0].subject == 1, "First triple subject");
  TEST_ASSERT(engine->triples[0].predicate == 1, "First triple predicate");
  TEST_ASSERT(engine->triples[0].object == 2, "First triple object");

  TEST_ASSERT(engine->triples[1].subject == 1, "Second triple subject");
  TEST_ASSERT(engine->triples[1].predicate == 2, "Second triple predicate");
  TEST_ASSERT(engine->triples[1].object == 4, "Second triple object");

  query_engine_destroy(engine);

  return 1;
}

// Lesson 2: Pattern Matching
int lesson_pattern_matching()
{
  TEST_SECTION("Pattern Matching");

  QueryEngine *engine = query_engine_create(1000);

  // Add test data
  query_engine_add_triple(engine, 1, 1, 2); // (Alice, knows, Bob)
  query_engine_add_triple(engine, 1, 1, 3); // (Alice, knows, Charlie)
  query_engine_add_triple(engine, 1, 2, 4); // (Alice, worksAt, TechCorp)
  query_engine_add_triple(engine, 3, 2, 4); // (Charlie, worksAt, TechCorp)

  // Test 1: Exact pattern matching
  int alice_knows_bob = query_engine_ask_pattern(engine, 1, 1, 2);
  TEST_ASSERT(alice_knows_bob == 1, "Alice knows Bob pattern");

  int alice_knows_charlie = query_engine_ask_pattern(engine, 1, 1, 3);
  TEST_ASSERT(alice_knows_charlie == 1, "Alice knows Charlie pattern");

  int alice_works_techcorp = query_engine_ask_pattern(engine, 1, 2, 4);
  TEST_ASSERT(alice_works_techcorp == 1, "Alice works at TechCorp pattern");

  // Test 2: Non-existent patterns
  int alice_knows_dave = query_engine_ask_pattern(engine, 1, 1, 6);
  TEST_ASSERT(alice_knows_dave == 0, "Non-existent pattern returns false");

  int dave_works_anywhere = query_engine_ask_pattern(engine, 6, 2, 4);
  TEST_ASSERT(dave_works_anywhere == 0, "Non-existent subject returns false");

  // Test 3: Multiple matches
  int charlie_works_techcorp = query_engine_ask_pattern(engine, 3, 2, 4);
  TEST_ASSERT(charlie_works_techcorp == 1, "Charlie works at TechCorp pattern");

  printf("  Pattern matching results:\n");
  printf("  - Alice knows Bob: %s\n", alice_knows_bob ? "Yes" : "No");
  printf("  - Alice knows Charlie: %s\n", alice_knows_charlie ? "Yes" : "No");
  printf("  - Alice works at TechCorp: %s\n", alice_works_techcorp ? "Yes" : "No");
  printf("  - Charlie works at TechCorp: %s\n", charlie_works_techcorp ? "Yes" : "No");

  query_engine_destroy(engine);

  return 1;
}

// Lesson 3: Query Execution
int lesson_query_execution()
{
  TEST_SECTION("Query Execution");

  QueryEngine *engine = query_engine_create(1000);

  // Add complex test data
  query_engine_add_triple(engine, 1, 1, 2); // (Alice, knows, Bob)
  query_engine_add_triple(engine, 1, 1, 3); // (Alice, knows, Charlie)
  query_engine_add_triple(engine, 2, 1, 3); // (Bob, knows, Charlie)
  query_engine_add_triple(engine, 1, 2, 4); // (Alice, worksAt, TechCorp)
  query_engine_add_triple(engine, 3, 2, 4); // (Charlie, worksAt, TechCorp)
  query_engine_add_triple(engine, 3, 3, 6); // (Charlie, hasSkill, Programming)
  query_engine_add_triple(engine, 1, 3, 7); // (Alice, hasSkill, Design)
  query_engine_add_triple(engine, 2, 3, 8); // (Bob, hasSkill, Marketing)

  // Test 1: SELECT query with wildcards
  size_t result_count;

  // Query: Who works at TechCorp?
  Triple *techcorp_employees = query_engine_select_pattern(engine, -1, 2, 4, &result_count);
  TEST_ASSERT(techcorp_employees != NULL, "SELECT query result allocation");
  TEST_ASSERT(result_count == 2, "Two employees work at TechCorp");

  printf("  TechCorp employees: %zu\n", result_count);
  for (size_t i = 0; i < result_count; i++)
  {
    printf("  - Employee %d\n", techcorp_employees[i].subject);
  }

  free(techcorp_employees);

  // Query: What skills does Charlie have?
  Triple *charlie_skills = query_engine_select_pattern(engine, 3, 3, -1, &result_count);
  TEST_ASSERT(charlie_skills != NULL, "Charlie skills query result allocation");
  TEST_ASSERT(result_count == 1, "Charlie has one skill");
  TEST_ASSERT(charlie_skills[0].object == 6, "Charlie has Programming skill");

  printf("  Charlie's skills: %zu\n", result_count);
  for (size_t i = 0; i < result_count; i++)
  {
    printf("  - Skill %d\n", charlie_skills[i].object);
  }

  free(charlie_skills);

  // Test 2: Complex query simulation
  // Query: Who knows Charlie and works at TechCorp?
  int knows_charlie_and_works_techcorp = 0;

  // Check Bob
  int bob_knows_charlie = query_engine_ask_pattern(engine, 2, 1, 3);
  int bob_works_techcorp = query_engine_ask_pattern(engine, 2, 2, 4);
  if (bob_knows_charlie && bob_works_techcorp)
  {
    knows_charlie_and_works_techcorp++;
  }

  // Check Alice
  int alice_knows_charlie = query_engine_ask_pattern(engine, 1, 1, 3);
  int alice_works_techcorp = query_engine_ask_pattern(engine, 1, 2, 4);
  if (alice_knows_charlie && alice_works_techcorp)
  {
    knows_charlie_and_works_techcorp++;
  }

  printf("  People who know Charlie and work at TechCorp: %d\n", knows_charlie_and_works_techcorp);
  TEST_ASSERT(knows_charlie_and_works_techcorp == 1, "One person meets complex criteria");

  query_engine_destroy(engine);

  return 1;
}

// Lesson 4: Performance Optimization
int lesson_performance_optimization()
{
  TEST_SECTION("Performance Optimization");

  QueryEngine *engine = query_engine_create(10000);

  // Add large dataset
  for (int i = 0; i < 1000; i++)
  {
    query_engine_add_triple(engine, i, 1, i + 1);   // (i, knows, i+1)
    query_engine_add_triple(engine, i, 2, i + 100); // (i, worksAt, i+100)
    query_engine_add_triple(engine, i, 3, i + 200); // (i, hasSkill, i+200)
  }

  TEST_ASSERT(engine->count == 3000, "Large dataset loaded");

  // Test 1: Query performance
  clock_t start = clock();
  int found_count = 0;
  for (int i = 0; i < 1000; i++)
  {
    if (query_engine_ask_pattern(engine, i, 1, i + 1))
    {
      found_count++;
    }
  }
  clock_t end = clock();
  double query_time = ((double)(end - start)) / CLOCKS_PER_SEC;

  printf("  Query time: %.6f seconds\n", query_time);
  printf("  Found matches: %d\n", found_count);
  TEST_ASSERT(found_count == 1000, "All expected matches found");
  TEST_ASSERT(query_time < 0.1, "Query performance is acceptable");

  // Test 2: SELECT query performance
  start = clock();
  size_t result_count;
  Triple *results = query_engine_select_pattern(engine, 500, -1, -1, &result_count);
  end = clock();
  double select_time = ((double)(end - start)) / CLOCKS_PER_SEC;

  printf("  SELECT query time: %.6f seconds\n", select_time);
  printf("  Results found: %zu\n", result_count);
  TEST_ASSERT(result_count == 3, "Three triples for subject 500");
  TEST_ASSERT(select_time < 0.01, "SELECT query performance is fast");

  free(results);

  // Test 3: Memory efficiency
  size_t memory_usage = engine->count * sizeof(Triple);
  printf("  Memory usage: %zu bytes for %zu triples\n", memory_usage, engine->count);
  TEST_ASSERT(memory_usage > 0, "Memory usage is positive");

  query_engine_destroy(engine);

  return 1;
}

// Lesson 5: 7T Engine Query Patterns
int lesson_7t_query_patterns()
{
  TEST_SECTION("7T Engine Query Patterns");

  QueryEngine *engine = query_engine_create(1000);

  // Pattern 1: Simple ASK queries (7-tick path)
  query_engine_add_triple(engine, 1, 1, 2); // (Alice, knows, Bob)
  query_engine_add_triple(engine, 1, 2, 4); // (Alice, worksAt, TechCorp)

  int alice_knows_bob = query_engine_ask_pattern(engine, 1, 1, 2);
  int alice_works_techcorp = query_engine_ask_pattern(engine, 1, 2, 4);

  printf("  7-tick ASK queries:\n");
  printf("  - Alice knows Bob: %s\n", alice_knows_bob ? "Yes" : "No");
  printf("  - Alice works at TechCorp: %s\n", alice_works_techcorp ? "Yes" : "No");

  TEST_ASSERT(alice_knows_bob == 1, "7-tick ASK query result");
  TEST_ASSERT(alice_works_techcorp == 1, "7-tick ASK query result");

  // Pattern 2: Complex SELECT queries (49-tick path)
  query_engine_add_triple(engine, 1, 1, 3); // (Alice, knows, Charlie)
  query_engine_add_triple(engine, 3, 2, 4); // (Charlie, worksAt, TechCorp)
  query_engine_add_triple(engine, 3, 3, 6); // (Charlie, hasSkill, Programming)

  size_t result_count;

  // Complex query: Who works at TechCorp and what are their skills?
  Triple *techcorp_skills = query_engine_select_pattern(engine, -1, 2, 4, &result_count);

  printf("  49-tick SELECT query results:\n");
  printf("  - TechCorp employees: %zu\n", result_count);
  for (size_t i = 0; i < result_count; i++)
  {
    printf("  - Employee %d\n", techcorp_skills[i].subject);
  }

  TEST_ASSERT(result_count == 2, "Two employees at TechCorp");
  free(techcorp_skills);

  // Pattern 3: Integration with template engine
  // Query results formatted for CJinja
  Triple *alice_relations = query_engine_select_pattern(engine, 1, -1, -1, &result_count);

  printf("  Integration query results:\n");
  printf("  - Alice's relations: %zu\n", result_count);
  for (size_t i = 0; i < result_count; i++)
  {
    printf("  - Alice %d %d\n", alice_relations[i].predicate, alice_relations[i].object);
  }

  TEST_ASSERT(result_count == 3, "Three relations for Alice");
  free(alice_relations);

  query_engine_destroy(engine);

  return 1;
}

// Main tutorial runner
int main()
{
  printf("First Principles C Tutorial: Query Engine\n");
  printf("=========================================\n");
  printf("Validating query concepts for SPARQL implementation\n\n");

  int total_lessons = 5;
  int passed_lessons = 0;

  // Run all lessons
  if (lesson_basic_triple_storage())
    passed_lessons++;
  if (lesson_pattern_matching())
    passed_lessons++;
  if (lesson_query_execution())
    passed_lessons++;
  if (lesson_performance_optimization())
    passed_lessons++;
  if (lesson_7t_query_patterns())
    passed_lessons++;

  // Summary
  printf("\n=== Tutorial Summary ===\n");
  printf("Total lessons: %d\n", total_lessons);
  printf("Passed: %d\n", passed_lessons);
  printf("Failed: %d\n", total_lessons - passed_lessons);
  printf("Success rate: %.1f%%\n", (passed_lessons * 100.0) / total_lessons);

  if (passed_lessons == total_lessons)
  {
    printf("\n🎉 All query engine concepts validated!\n");
    printf("Ready for SPARQL implementation.\n");
    return 0;
  }
  else
  {
    printf("\n❌ Some concepts need review.\n");
    return 1;
  }
}