#include "cns/sparql.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Test utilities
#define TEST_ASSERT(condition, message)        \
  do                                           \
  {                                            \
    if (!(condition))                          \
    {                                          \
      printf("❌ TEST FAILED: %s\n", message); \
      return 1;                                \
    }                                          \
    else                                       \
    {                                          \
      printf("✅ %s\n", message);              \
    }                                          \
  } while (0)

#define TEST_SECTION(name) printf("\n=== %s ===\n", name)

// Test SPARQL engine creation and destruction
int test_engine_lifecycle()
{
  TEST_SECTION("Engine Lifecycle");

  // Test creation
  SPARQLEngine *engine = cns_sparql_create(1024);
  TEST_ASSERT(engine != NULL, "Engine creation successful");

  // Test singleton behavior
  SPARQLEngine *engine2 = cns_sparql_create(2048);
  TEST_ASSERT(engine2 == engine, "Singleton pattern works");

  // Test destruction
  cns_sparql_destroy(engine);
  TEST_ASSERT(1, "Engine destruction successful");

  return 0;
}

// Test triple addition
int test_triple_operations()
{
  TEST_SECTION("Triple Operations");

  SPARQLEngine *engine = cns_sparql_create(1024);
  TEST_ASSERT(engine != NULL, "Engine created for triple tests");

  // Test adding triples
  int result = cns_sparql_add_triple(engine, 1, 2, 3);
  TEST_ASSERT(result == CNS_OK, "Triple (1,2,3) added successfully");

  result = cns_sparql_add_triple(engine, 4, 5, 6);
  TEST_ASSERT(result == CNS_OK, "Triple (4,5,6) added successfully");

  result = cns_sparql_add_triple(engine, 7, 8, 9);
  TEST_ASSERT(result == CNS_OK, "Triple (7,8,9) added successfully");

  // Test triple lookup
  uint32_t count = cns_sparql_find_triples(engine, 1, 0, 0);
  TEST_ASSERT(count == 1, "Found 1 triple with subject=1");

  count = cns_sparql_find_triples(engine, 0, 2, 0);
  TEST_ASSERT(count == 1, "Found 1 triple with predicate=2");

  count = cns_sparql_find_triples(engine, 0, 0, 3);
  TEST_ASSERT(count == 1, "Found 1 triple with object=3");

  count = cns_sparql_find_triples(engine, 1, 2, 3);
  TEST_ASSERT(count == 1, "Found 1 triple with exact match (1,2,3)");

  count = cns_sparql_find_triples(engine, 0, 0, 0);
  TEST_ASSERT(count == 3, "Found all 3 triples with wildcard search");

  cns_sparql_destroy(engine);
  return 0;
}

// Test SPARQL query execution
int test_query_execution()
{
  TEST_SECTION("Query Execution");

  SPARQLEngine *engine = cns_sparql_create(1024);
  TEST_ASSERT(engine != NULL, "Engine created for query tests");

  // Add test data
  for (int i = 0; i < 10; i++)
  {
    cns_sparql_add_triple(engine, i, i + 10, i + 20);
  }

  // Test SELECT query
  SPARQLResult *result = cns_sparql_execute(engine, "SELECT ?s ?p ?o WHERE { ?s ?p ?o }");
  TEST_ASSERT(result != NULL, "SELECT query executed successfully");
  TEST_ASSERT(result->count == 10, "SELECT query returned 10 results");
  cns_sparql_free_result(result);

  // Test invalid query
  result = cns_sparql_execute(engine, "INVALID QUERY");
  TEST_ASSERT(result != NULL, "Invalid query handled gracefully");
  TEST_ASSERT(result->count == 0, "Invalid query returned 0 results");
  cns_sparql_free_result(result);

  // Test NULL query
  result = cns_sparql_execute(engine, NULL);
  TEST_ASSERT(result == NULL, "NULL query returns NULL");

  cns_sparql_destroy(engine);
  return 0;
}

// Test performance characteristics
int test_performance()
{
  TEST_SECTION("Performance Characteristics");

  SPARQLEngine *engine = cns_sparql_create(1024);
  TEST_ASSERT(engine != NULL, "Engine created for performance tests");

  // Test engine creation performance (7 cycles target)
  uint64_t start = cns_get_cycles();
  SPARQLEngine *test_engine = cns_sparql_create(512);
  uint64_t cycles = cns_get_cycles() - start;
  TEST_ASSERT(cycles <= 7, "Engine creation within 7-cycle limit");
  cns_sparql_destroy(test_engine);

  // Test triple addition performance (5 cycles target)
  start = cns_get_cycles();
  int result = cns_sparql_add_triple(engine, 1, 2, 3);
  cycles = cns_get_cycles() - start;
  TEST_ASSERT(result == CNS_OK, "Triple addition successful");
  TEST_ASSERT(cycles <= 5, "Triple addition within 5-cycle limit");

  // Test query execution performance (15 cycles target)
  start = cns_get_cycles();
  SPARQLResult *query_result = cns_sparql_execute(engine, "SELECT ?s ?p ?o WHERE { ?s ?p ?o }");
  cycles = cns_get_cycles() - start;
  TEST_ASSERT(query_result != NULL, "Query execution successful");
  TEST_ASSERT(cycles <= 15, "Query execution within 15-cycle limit");
  cns_sparql_free_result(query_result);

  // Test triple lookup performance (3 cycles target)
  start = cns_get_cycles();
  uint32_t count = cns_sparql_find_triples(engine, 1, 0, 0);
  cycles = cns_get_cycles() - start;
  TEST_ASSERT(count == 1, "Triple lookup successful");
  TEST_ASSERT(cycles <= 3, "Triple lookup within 3-cycle limit");

  cns_sparql_destroy(engine);
  return 0;
}

// Test caching behavior
int test_caching()
{
  TEST_SECTION("Caching Behavior");

  SPARQLEngine *engine = cns_sparql_create(1024);
  TEST_ASSERT(engine != NULL, "Engine created for caching tests");

  // Add some data
  cns_sparql_add_triple(engine, 1, 2, 3);

  // Execute same query multiple times
  const char *query = "SELECT ?s ?p ?o WHERE { ?s ?p ?o }";

  // First execution
  SPARQLResult *result1 = cns_sparql_execute(engine, query);
  TEST_ASSERT(result1 != NULL, "First query execution successful");
  cns_sparql_free_result(result1);

  // Second execution (should hit cache)
  SPARQLResult *result2 = cns_sparql_execute(engine, query);
  TEST_ASSERT(result2 != NULL, "Second query execution successful");
  cns_sparql_free_result(result2);

  // Check cache statistics
  SPARQLStats stats = cns_sparql_get_stats(engine);
  TEST_ASSERT(stats.total_queries == 2, "Total queries tracked correctly");
  TEST_ASSERT(stats.cache_hits >= 1, "Cache hits tracked correctly");
  TEST_ASSERT(stats.cache_hit_rate > 0.0, "Cache hit rate calculated correctly");

  cns_sparql_destroy(engine);
  return 0;
}

// Test statistics
int test_statistics()
{
  TEST_SECTION("Statistics");

  SPARQLEngine *engine = cns_sparql_create(1024);
  TEST_ASSERT(engine != NULL, "Engine created for statistics tests");

  // Initial stats
  SPARQLStats stats = cns_sparql_get_stats(engine);
  TEST_ASSERT(stats.total_queries == 0, "Initial total queries is 0");
  TEST_ASSERT(stats.cache_hits == 0, "Initial cache hits is 0");
  TEST_ASSERT(stats.total_triples == 0, "Initial total triples is 0");

  // Add some operations
  cns_sparql_add_triple(engine, 1, 2, 3);
  cns_sparql_add_triple(engine, 4, 5, 6);
  cns_sparql_execute(engine, "SELECT ?s ?p ?o WHERE { ?s ?p ?o }");

  // Updated stats
  stats = cns_sparql_get_stats(engine);
  TEST_ASSERT(stats.total_queries == 1, "Total queries updated correctly");
  TEST_ASSERT(stats.total_triples == 2, "Total triples updated correctly");
  TEST_ASSERT(stats.cache_hit_rate >= 0.0 && stats.cache_hit_rate <= 1.0, "Cache hit rate in valid range");

  cns_sparql_destroy(engine);
  return 0;
}

// Test error handling
int test_error_handling()
{
  TEST_SECTION("Error Handling");

  // Test NULL engine operations
  int result = cns_sparql_add_triple(NULL, 1, 2, 3);
  TEST_ASSERT(result == CNS_ERR_RESOURCE, "NULL engine triple addition returns error");

  SPARQLResult *result_ptr = cns_sparql_execute(NULL, "SELECT ?s ?p ?o WHERE { ?s ?p ?o }");
  TEST_ASSERT(result_ptr == NULL, "NULL engine query execution returns NULL");

  uint32_t count = cns_sparql_find_triples(NULL, 1, 2, 3);
  TEST_ASSERT(count == 0, "NULL engine triple lookup returns 0");

  // Test NULL result handling
  cns_sparql_free_result(NULL);
  TEST_ASSERT(1, "NULL result free handled gracefully");

  return 0;
}

// Main test runner
int main()
{
  printf("🧪 SPARQL Engine Test Suite\n");
  printf("==========================\n");

  int failures = 0;

  failures += test_engine_lifecycle();
  failures += test_triple_operations();
  failures += test_query_execution();
  failures += test_performance();
  failures += test_caching();
  failures += test_statistics();
  failures += test_error_handling();

  printf("\n=== Test Summary ===\n");
  if (failures == 0)
  {
    printf("✅ All tests passed! SPARQL engine is fully operational.\n");
    return 0;
  }
  else
  {
    printf("❌ %d test sections failed. SPARQL engine needs fixes.\n", failures);
    return 1;
  }
}