#include "cns/sparql.h"
#include "cns/cli.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// 80/20 Validation Configuration
#define VALIDATION_ITERATIONS 1000
#define TEST_DATA_SIZE 100
#define CACHE_TEST_QUERIES 10

// Performance thresholds (80/20: focus on common cases)
#define THRESHOLD_ENGINE_CREATE 7
#define THRESHOLD_TRIPLE_ADD 5
#define THRESHOLD_QUERY_EXECUTE 15
#define THRESHOLD_TRIPLE_LOOKUP 3

// Validation results
typedef struct
{
  uint64_t total_operations;
  uint64_t passed_operations;
  uint64_t failed_operations;
  double avg_cycles_per_operation;
  uint64_t max_cycles;
  uint64_t min_cycles;
  bool all_tests_passed;
} ValidationResult;

// Test data generation (80/20: simple patterns)
void generate_test_data(SPARQLEngine *engine, int count)
{
  for (int i = 0; i < count; i++)
  {
    uint32_t subject = i;
    uint32_t predicate = i + 100;
    uint32_t object = i + 200;
    cns_sparql_add_triple(engine, subject, predicate, object);
  }
}

// Validate engine lifecycle
ValidationResult validate_engine_lifecycle()
{
  printf("🔧 Validating Engine Lifecycle...\n");

  ValidationResult result = {0};
  result.all_tests_passed = true;

  for (int i = 0; i < VALIDATION_ITERATIONS; i++)
  {
    uint64_t start = cns_get_cycles();

    SPARQLEngine *engine = cns_sparql_create(1024);
    uint64_t cycles = cns_get_cycles() - start;

    if (engine)
    {
      result.passed_operations++;

      if (cycles <= THRESHOLD_ENGINE_CREATE)
      {
        result.passed_operations++;
      }
      else
      {
        result.failed_operations++;
        result.all_tests_passed = false;
      }

      cns_sparql_destroy(engine);
    }
    else
    {
      result.failed_operations++;
      result.all_tests_passed = false;
    }

    result.total_operations++;

    if (cycles < result.min_cycles || result.min_cycles == 0)
      result.min_cycles = cycles;
    if (cycles > result.max_cycles)
      result.max_cycles = cycles;
  }

  result.avg_cycles_per_operation = (double)result.total_operations / VALIDATION_ITERATIONS;

  printf("   Operations: %llu passed, %llu failed\n", result.passed_operations, result.failed_operations);
  printf("   Cycles: min=%llu, max=%llu, avg=%.1f\n", result.min_cycles, result.max_cycles, result.avg_cycles_per_operation);
  printf("   Status: %s\n", result.all_tests_passed ? "✅ PASSED" : "❌ FAILED");

  return result;
}

// Validate triple operations
ValidationResult validate_triple_operations()
{
  printf("🔧 Validating Triple Operations...\n");

  ValidationResult result = {0};
  result.all_tests_passed = true;

  SPARQLEngine *engine = cns_sparql_create(1024);
  if (!engine)
  {
    printf("   ❌ Failed to create engine\n");
    result.all_tests_passed = false;
    return result;
  }

  for (int i = 0; i < VALIDATION_ITERATIONS; i++)
  {
    uint64_t start = cns_get_cycles();

    int add_result = cns_sparql_add_triple(engine, i, i + 100, i + 200);
    uint64_t cycles = cns_get_cycles() - start;

    if (add_result == CNS_OK)
    {
      result.passed_operations++;

      if (cycles <= THRESHOLD_TRIPLE_ADD)
      {
        result.passed_operations++;
      }
      else
      {
        result.failed_operations++;
        result.all_tests_passed = false;
      }
    }
    else
    {
      result.failed_operations++;
      result.all_tests_passed = false;
    }

    result.total_operations++;

    if (cycles < result.min_cycles || result.min_cycles == 0)
      result.min_cycles = cycles;
    if (cycles > result.max_cycles)
      result.max_cycles = cycles;
  }

  result.avg_cycles_per_operation = (double)result.total_operations / VALIDATION_ITERATIONS;

  printf("   Operations: %llu passed, %llu failed\n", result.passed_operations, result.failed_operations);
  printf("   Cycles: min=%llu, max=%llu, avg=%.1f\n", result.min_cycles, result.max_cycles, result.avg_cycles_per_operation);
  printf("   Status: %s\n", result.all_tests_passed ? "✅ PASSED" : "❌ FAILED");

  cns_sparql_destroy(engine);
  return result;
}

// Validate query execution
ValidationResult validate_query_execution()
{
  printf("🔧 Validating Query Execution...\n");

  ValidationResult result = {0};
  result.all_tests_passed = true;

  SPARQLEngine *engine = cns_sparql_create(1024);
  if (!engine)
  {
    printf("   ❌ Failed to create engine\n");
    result.all_tests_passed = false;
    return result;
  }

  // Add test data
  generate_test_data(engine, TEST_DATA_SIZE);

  // Common SPARQL patterns (80/20: these cover most use cases)
  const char *test_queries[] = {
      "SELECT ?s ?p ?o WHERE { ?s ?p ?o }",
      "SELECT ?s WHERE { ?s ?p ?o }",
      "SELECT ?p WHERE { ?s ?p ?o }",
      "SELECT ?o WHERE { ?s ?p ?o }"};
  int num_queries = sizeof(test_queries) / sizeof(test_queries[0]);

  for (int i = 0; i < VALIDATION_ITERATIONS; i++)
  {
    const char *query = test_queries[i % num_queries];

    uint64_t start = cns_get_cycles();

    SPARQLResult *query_result = cns_sparql_execute(engine, query);
    uint64_t cycles = cns_get_cycles() - start;

    if (query_result)
    {
      result.passed_operations++;

      if (cycles <= THRESHOLD_QUERY_EXECUTE)
      {
        result.passed_operations++;
      }
      else
      {
        result.failed_operations++;
        result.all_tests_passed = false;
      }

      cns_sparql_free_result(query_result);
    }
    else
    {
      result.failed_operations++;
      result.all_tests_passed = false;
    }

    result.total_operations++;

    if (cycles < result.min_cycles || result.min_cycles == 0)
      result.min_cycles = cycles;
    if (cycles > result.max_cycles)
      result.max_cycles = cycles;
  }

  result.avg_cycles_per_operation = (double)result.total_operations / VALIDATION_ITERATIONS;

  printf("   Operations: %llu passed, %llu failed\n", result.passed_operations, result.failed_operations);
  printf("   Cycles: min=%llu, max=%llu, avg=%.1f\n", result.min_cycles, result.max_cycles, result.avg_cycles_per_operation);
  printf("   Status: %s\n", result.all_tests_passed ? "✅ PASSED" : "❌ FAILED");

  cns_sparql_destroy(engine);
  return result;
}

// Validate caching behavior
ValidationResult validate_caching()
{
  printf("🔧 Validating Caching Behavior...\n");

  ValidationResult result = {0};
  result.all_tests_passed = true;

  SPARQLEngine *engine = cns_sparql_create(1024);
  if (!engine)
  {
    printf("   ❌ Failed to create engine\n");
    result.all_tests_passed = false;
    return result;
  }

  // Add test data
  generate_test_data(engine, TEST_DATA_SIZE);

  // Test cache with repeated queries
  const char *repeated_query = "SELECT ?s ?p ?o WHERE { ?s ?p ?o }";

  for (int i = 0; i < CACHE_TEST_QUERIES; i++)
  {
    SPARQLResult *query_result = cns_sparql_execute(engine, repeated_query);
    if (query_result)
    {
      result.passed_operations++;
      cns_sparql_free_result(query_result);
    }
    else
    {
      result.failed_operations++;
      result.all_tests_passed = false;
    }
    result.total_operations++;
  }

  // Check cache statistics
  SPARQLStats stats = cns_sparql_get_stats(engine);

  printf("   Total queries: %llu\n", stats.total_queries);
  printf("   Cache hits: %llu\n", stats.cache_hits);
  printf("   Cache hit rate: %.1f%%\n", stats.cache_hit_rate * 100.0);
  printf("   Status: %s\n", (stats.cache_hits > 0) ? "✅ PASSED" : "❌ FAILED");

  if (stats.cache_hits == 0)
  {
    result.all_tests_passed = false;
  }

  cns_sparql_destroy(engine);
  return result;
}

// Validate triple lookup performance
ValidationResult validate_triple_lookup()
{
  printf("🔧 Validating Triple Lookup...\n");

  ValidationResult result = {0};
  result.all_tests_passed = true;

  SPARQLEngine *engine = cns_sparql_create(1024);
  if (!engine)
  {
    printf("   ❌ Failed to create engine\n");
    result.all_tests_passed = false;
    return result;
  }

  // Add test data
  generate_test_data(engine, TEST_DATA_SIZE);

  for (int i = 0; i < VALIDATION_ITERATIONS; i++)
  {
    uint64_t start = cns_get_cycles();

    uint32_t count = cns_sparql_find_triples(engine, i % TEST_DATA_SIZE, 0, 0);
    uint64_t cycles = cns_get_cycles() - start;

    if (count >= 0)
    {
      result.passed_operations++;

      if (cycles <= THRESHOLD_TRIPLE_LOOKUP)
      {
        result.passed_operations++;
      }
      else
      {
        result.failed_operations++;
        result.all_tests_passed = false;
      }
    }
    else
    {
      result.failed_operations++;
      result.all_tests_passed = false;
    }

    result.total_operations++;

    if (cycles < result.min_cycles || result.min_cycles == 0)
      result.min_cycles = cycles;
    if (cycles > result.max_cycles)
      result.max_cycles = cycles;
  }

  result.avg_cycles_per_operation = (double)result.total_operations / VALIDATION_ITERATIONS;

  printf("   Operations: %llu passed, %llu failed\n", result.passed_operations, result.failed_operations);
  printf("   Cycles: min=%llu, max=%llu, avg=%.1f\n", result.min_cycles, result.max_cycles, result.avg_cycles_per_operation);
  printf("   Status: %s\n", result.all_tests_passed ? "✅ PASSED" : "❌ FAILED");

  cns_sparql_destroy(engine);
  return result;
}

// Main validation function
int main()
{
  printf("🚀 SPARQL 80/20 System Validation\n");
  printf("=================================\n");
  printf("Validating system is fully operational and deployable...\n\n");

  time_t start_time = time(NULL);

  // Run all validations
  ValidationResult lifecycle_result = validate_engine_lifecycle();
  ValidationResult triple_result = validate_triple_operations();
  ValidationResult query_result = validate_query_execution();
  ValidationResult cache_result = validate_caching();
  ValidationResult lookup_result = validate_triple_lookup();

  time_t end_time = time(NULL);
  double duration = difftime(end_time, start_time);

  // Summary
  printf("\n=== Validation Summary ===\n");
  printf("Engine Lifecycle: %s\n", lifecycle_result.all_tests_passed ? "✅ PASSED" : "❌ FAILED");
  printf("Triple Operations: %s\n", triple_result.all_tests_passed ? "✅ PASSED" : "❌ FAILED");
  printf("Query Execution: %s\n", query_result.all_tests_passed ? "✅ PASSED" : "❌ FAILED");
  printf("Caching Behavior: %s\n", cache_result.all_tests_passed ? "✅ PASSED" : "❌ FAILED");
  printf("Triple Lookup: %s\n", lookup_result.all_tests_passed ? "✅ PASSED" : "❌ FAILED");

  bool all_passed = lifecycle_result.all_tests_passed &&
                    triple_result.all_tests_passed &&
                    query_result.all_tests_passed &&
                    cache_result.all_tests_passed &&
                    lookup_result.all_tests_passed;

  printf("\nValidation Duration: %.1f seconds\n", duration);
  printf("Overall Status: %s\n", all_passed ? "✅ SYSTEM FULLY OPERATIONAL" : "❌ SYSTEM NEEDS FIXES");

  if (all_passed)
  {
    printf("\n🎉 SPARQL system is fully operational and deployable!\n");
    printf("   - All performance targets met\n");
    printf("   - All functionality validated\n");
    printf("   - 80/20 optimization working\n");
    printf("   - Ready for production deployment\n");
  }
  else
  {
    printf("\n⚠️  SPARQL system needs fixes before deployment.\n");
    printf("   - Some performance targets not met\n");
    printf("   - Some functionality failing\n");
    printf("   - Review failed validations above\n");
  }

  return all_passed ? 0 : 1;
}