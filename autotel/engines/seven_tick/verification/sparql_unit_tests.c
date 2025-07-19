#include "7t_unit_test_framework.h"
#include "../runtime/src/seven_t_runtime.h"

// SPARQL test context
typedef struct
{
  EngineState *engine;
  uint32_t alice, bob, charlie, diana;
  uint32_t knows, likes, works_for, lives_in;
  uint32_t company, city, pizza, music;
} SPARQLTestContext;

// Test setup and teardown
SPARQLTestContext *setup_sparql_test_context(void)
{
  SPARQLTestContext *ctx = malloc(sizeof(SPARQLTestContext));
  if (!ctx)
    return NULL;

  ctx->engine = s7t_create_engine();
  if (!ctx->engine)
  {
    free(ctx);
    return NULL;
  }

  // Intern test strings
  ctx->alice = s7t_intern_string(ctx->engine, "ex:alice");
  ctx->bob = s7t_intern_string(ctx->engine, "ex:bob");
  ctx->charlie = s7t_intern_string(ctx->engine, "ex:charlie");
  ctx->diana = s7t_intern_string(ctx->engine, "ex:diana");

  ctx->knows = s7t_intern_string(ctx->engine, "ex:knows");
  ctx->likes = s7t_intern_string(ctx->engine, "ex:likes");
  ctx->works_for = s7t_intern_string(ctx->engine, "ex:worksFor");
  ctx->lives_in = s7t_intern_string(ctx->engine, "ex:livesIn");

  ctx->company = s7t_intern_string(ctx->engine, "ex:company");
  ctx->city = s7t_intern_string(ctx->engine, "ex:city");
  ctx->pizza = s7t_intern_string(ctx->engine, "ex:pizza");
  ctx->music = s7t_intern_string(ctx->engine, "ex:music");

  // Add test triples
  s7t_add_triple(ctx->engine, ctx->alice, ctx->knows, ctx->bob);
  s7t_add_triple(ctx->engine, ctx->alice, ctx->knows, ctx->charlie);
  s7t_add_triple(ctx->engine, ctx->bob, ctx->knows, ctx->diana);
  s7t_add_triple(ctx->engine, ctx->alice, ctx->likes, ctx->pizza);
  s7t_add_triple(ctx->engine, ctx->bob, ctx->likes, ctx->music);
  s7t_add_triple(ctx->engine, ctx->alice, ctx->works_for, ctx->company);
  s7t_add_triple(ctx->engine, ctx->bob, ctx->works_for, ctx->company);
  s7t_add_triple(ctx->engine, ctx->alice, ctx->lives_in, ctx->city);
  s7t_add_triple(ctx->engine, ctx->bob, ctx->lives_in, ctx->city);

  return ctx;
}

void teardown_sparql_test_context(SPARQLTestContext *ctx)
{
  if (ctx)
  {
    s7t_destroy_engine(ctx->engine);
    free(ctx);
  }
}

// Individual test functions
void test_engine_creation(void)
{
  EngineState *engine = s7t_create_engine();
  ASSERT_NOT_NULL(engine);
  ASSERT_EQUAL(0, engine->triple_count);
  ASSERT_EQUAL(0, engine->string_count);
  s7t_destroy_engine(engine);
}

void test_string_interning(void)
{
  EngineState *engine = s7t_create_engine();

  uint32_t id1 = s7t_intern_string(engine, "test");
  uint32_t id2 = s7t_intern_string(engine, "test");
  uint32_t id3 = s7t_intern_string(engine, "test2");

  ASSERT_EQUAL(id1, id2);                // Same string should get same ID
  ASSERT_NOT_EQUAL(id1, id3);            // Different strings should get different IDs
  ASSERT_EQUAL(2, engine->string_count); // Two unique strings

  s7t_destroy_engine(engine);
}

void test_triple_addition(void)
{
  EngineState *engine = s7t_create_engine();

  uint32_t s = s7t_intern_string(engine, "subject");
  uint32_t p = s7t_intern_string(engine, "predicate");
  uint32_t o = s7t_intern_string(engine, "object");

  s7t_add_triple(engine, s, p, o);
  ASSERT_EQUAL(1, engine->triple_count);

  s7t_add_triple(engine, s, p, o);       // Duplicate triple
  ASSERT_EQUAL(1, engine->triple_count); // Should not add duplicate

  s7t_destroy_engine(engine);
}

void test_pattern_matching_basic(void)
{
  SPARQLTestContext *ctx = setup_sparql_test_context();
  ASSERT_NOT_NULL(ctx);

  // Test existing patterns
  ASSERT_TRUE(s7t_ask_pattern(ctx->engine, ctx->alice, ctx->knows, ctx->bob));
  ASSERT_TRUE(s7t_ask_pattern(ctx->engine, ctx->alice, ctx->likes, ctx->pizza));
  ASSERT_TRUE(s7t_ask_pattern(ctx->engine, ctx->bob, ctx->knows, ctx->diana));

  // Test non-existing patterns
  ASSERT_FALSE(s7t_ask_pattern(ctx->engine, ctx->alice, ctx->knows, ctx->diana));
  ASSERT_FALSE(s7t_ask_pattern(ctx->engine, ctx->bob, ctx->likes, ctx->pizza));
  ASSERT_FALSE(s7t_ask_pattern(ctx->engine, ctx->charlie, ctx->knows, ctx->bob));

  teardown_sparql_test_context(ctx);
}

void test_pattern_matching_wildcards(void)
{
  SPARQLTestContext *ctx = setup_sparql_test_context();
  ASSERT_NOT_NULL(ctx);

  // Test wildcard queries (object = 0)
  ASSERT_TRUE(s7t_ask_pattern(ctx->engine, ctx->alice, ctx->knows, 0));
  ASSERT_TRUE(s7t_ask_pattern(ctx->engine, ctx->alice, ctx->likes, 0));
  ASSERT_TRUE(s7t_ask_pattern(ctx->engine, ctx->bob, ctx->knows, 0));

  // Test non-existing wildcard queries
  ASSERT_FALSE(s7t_ask_pattern(ctx->engine, ctx->charlie, ctx->knows, 0));
  ASSERT_FALSE(s7t_ask_pattern(ctx->engine, ctx->diana, ctx->likes, 0));

  teardown_sparql_test_context(ctx);
}

void test_bit_vector_operations(void)
{
  SPARQLTestContext *ctx = setup_sparql_test_context();
  ASSERT_NOT_NULL(ctx);

  // Test getting subject vectors
  BitVector *alice_knows = s7t_get_subject_vector(ctx->engine, ctx->knows, ctx->bob);
  ASSERT_NOT_NULL(alice_knows);
  ASSERT_TRUE(bitvec_test(alice_knows, ctx->alice));
  ASSERT_FALSE(bitvec_test(alice_knows, ctx->bob));
  bitvec_destroy(alice_knows);

  // Test getting object vectors
  BitVector *knows_bob = s7t_get_subject_vector(ctx->engine, ctx->knows, ctx->bob);
  ASSERT_NOT_NULL(knows_bob);
  ASSERT_TRUE(bitvec_test(knows_bob, ctx->alice));
  bitvec_destroy(knows_bob);

  teardown_sparql_test_context(ctx);
}

void test_materialization(void)
{
  SPARQLTestContext *ctx = setup_sparql_test_context();
  ASSERT_NOT_NULL(ctx);

  // Test materializing subjects
  size_t count;
  uint32_t *results = s7t_materialize_subjects(ctx->engine, ctx->knows, ctx->bob, &count);
  ASSERT_NOT_NULL(results);
  ASSERT_EQUAL(1, count);
  ASSERT_EQUAL(ctx->alice, results[0]);
  free(results);

  // Test materializing multiple results
  results = s7t_materialize_subjects(ctx->engine, ctx->works_for, ctx->company, &count);
  ASSERT_NOT_NULL(results);
  ASSERT_EQUAL(2, count);
  bool found_alice = false, found_bob = false;
  for (size_t i = 0; i < count; i++)
  {
    if (results[i] == ctx->alice)
      found_alice = true;
    if (results[i] == ctx->bob)
      found_bob = true;
  }
  ASSERT_TRUE(found_alice);
  ASSERT_TRUE(found_bob);
  free(results);

  teardown_sparql_test_context(ctx);
}

void test_large_scale_operations(void)
{
  EngineState *engine = s7t_create_engine();
  ASSERT_NOT_NULL(engine);

  // Add many triples
  for (int i = 0; i < 1000; i++)
  {
    char subj[32], obj[32];
    snprintf(subj, sizeof(subj), "subject_%d", i);
    snprintf(obj, sizeof(obj), "object_%d", i);

    uint32_t s = s7t_intern_string(engine, subj);
    uint32_t p = s7t_intern_string(engine, "predicate");
    uint32_t o = s7t_intern_string(engine, obj);

    s7t_add_triple(engine, s, p, o);
  }

  ASSERT_EQUAL(1000, engine->triple_count);
  ASSERT_EQUAL(1001, engine->string_count); // 1000 unique + 1 predicate

  // Test pattern matching on large dataset
  uint32_t test_subj = s7t_intern_string(engine, "subject_500");
  uint32_t test_pred = s7t_intern_string(engine, "predicate");
  uint32_t test_obj = s7t_intern_string(engine, "object_500");

  ASSERT_TRUE(s7t_ask_pattern(engine, test_subj, test_pred, test_obj));
  ASSERT_FALSE(s7t_ask_pattern(engine, test_subj, test_pred, test_obj + 1));

  s7t_destroy_engine(engine);
}

void test_performance_7tick_pattern_matching(void)
{
  SPARQLTestContext *ctx = setup_sparql_test_context();
  ASSERT_NOT_NULL(ctx);

  // Test 7-tick performance for pattern matching
  ASSERT_PERFORMANCE_7TICK(
      s7t_ask_pattern(ctx->engine, ctx->alice, ctx->knows, ctx->bob),
      100000);

  teardown_sparql_test_context(ctx);
}

void test_performance_bit_vector_operations(void)
{
  SPARQLTestContext *ctx = setup_sparql_test_context();
  ASSERT_NOT_NULL(ctx);

  // Test 7-tick performance for bit vector operations
  ASSERT_PERFORMANCE_7TICK(
      {
        BitVector *result = s7t_get_subject_vector(ctx->engine, ctx->knows, ctx->bob);
        if (result)
          bitvec_destroy(result);
      },
      10000);

  teardown_sparql_test_context(ctx);
}

void test_edge_cases(void)
{
  EngineState *engine = s7t_create_engine();
  ASSERT_NOT_NULL(engine);

  // Test with zero IDs
  ASSERT_FALSE(s7t_ask_pattern(engine, 0, 0, 0));

  // Test with non-existent IDs
  ASSERT_FALSE(s7t_ask_pattern(engine, 999, 999, 999));

  // Test empty engine
  BitVector *result = s7t_get_subject_vector(engine, 1, 1);
  ASSERT_NULL(result);

  s7t_destroy_engine(engine);
}

void test_memory_management(void)
{
  EngineState *engine = s7t_create_engine();
  ASSERT_NOT_NULL(engine);

  // Add and remove many triples
  for (int i = 0; i < 100; i++)
  {
    char subj[32], obj[32];
    snprintf(subj, sizeof(subj), "subject_%d", i);
    snprintf(obj, sizeof(obj), "object_%d", i);

    uint32_t s = s7t_intern_string(engine, subj);
    uint32_t p = s7t_intern_string(engine, "predicate");
    uint32_t o = s7t_intern_string(engine, obj);

    s7t_add_triple(engine, s, p, o);
  }

  // Test that engine still works after many operations
  uint32_t test_s = s7t_intern_string(engine, "subject_50");
  uint32_t test_p = s7t_intern_string(engine, "predicate");
  uint32_t test_o = s7t_intern_string(engine, "object_50");

  ASSERT_TRUE(s7t_ask_pattern(engine, test_s, test_p, test_o));

  s7t_destroy_engine(engine);
}

// Test suite runner
void run_sparql_tests(TestSuite *suite)
{
  printf("\n🔍 Running SPARQL Engine Unit Tests\n");
  printf("===================================\n");

  run_test(suite, "Engine Creation", test_engine_creation);
  run_test(suite, "String Interning", test_string_interning);
  run_test(suite, "Triple Addition", test_triple_addition);
  run_test(suite, "Pattern Matching Basic", test_pattern_matching_basic);
  run_test(suite, "Pattern Matching Wildcards", test_pattern_matching_wildcards);
  run_test(suite, "Bit Vector Operations", test_bit_vector_operations);
  run_test(suite, "Materialization", test_materialization);
  run_test(suite, "Large Scale Operations", test_large_scale_operations);
  run_test(suite, "7-Tick Pattern Matching Performance", test_performance_7tick_pattern_matching);
  run_test(suite, "7-Tick Bit Vector Performance", test_performance_bit_vector_operations);
  run_test(suite, "Edge Cases", test_edge_cases);
  run_test(suite, "Memory Management", test_memory_management);
}

// Main test runner
int main()
{
  TestSuite *suite = create_test_suite("SPARQL Engine");
  if (!suite)
  {
    printf("❌ Failed to create test suite\n");
    return 1;
  }

  run_sparql_tests(suite);

  print_test_report(suite);

  int exit_code = suite->failed_tests > 0 ? 1 : 0;
  destroy_test_suite(suite);

  return exit_code;
}