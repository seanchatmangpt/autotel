#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "benchmark_framework.h"
#include "../compiler/src/cjinja.h"

// Simplified SPARQL simulation for benchmarking
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

// Test data structure for SPARQL benchmarks
typedef struct
{
  S7TEngine *sparql;
  CJinjaEngine *cjinja;
  CJinjaContext *ctx;
  const char *test_name;
} SPARQLTestData;

// Benchmark test functions
void benchmark_simple_query(void *data)
{
  SPARQLTestData *test_data = (SPARQLTestData *)data;

  // Simple SPARQL query: Who does Alice know?
  int alice_knows_bob = s7t_ask_pattern(test_data->sparql, 1, 1, 2);
  int alice_knows_charlie = s7t_ask_pattern(test_data->sparql, 1, 1, 3);

  // Use result (prevent optimization)
  if (alice_knows_bob && alice_knows_charlie)
  {
    // Result: Alice knows both Bob and Charlie
  }
}

void benchmark_complex_query(void *data)
{
  SPARQLTestData *test_data = (SPARQLTestData *)data;

  // Complex SPARQL query: Who works at TechCorp and what are their skills?
  int alice_works_techcorp = s7t_ask_pattern(test_data->sparql, 1, 2, 4);
  int charlie_works_techcorp = s7t_ask_pattern(test_data->sparql, 3, 2, 4);
  int alice_has_design = s7t_ask_pattern(test_data->sparql, 1, 3, 7);
  int charlie_has_programming = s7t_ask_pattern(test_data->sparql, 3, 3, 6);

  // Use results (prevent optimization)
  if (alice_works_techcorp && charlie_works_techcorp)
  {
    // Result: Both Alice and Charlie work at TechCorp
  }
}

void benchmark_batch_queries(void *data)
{
  SPARQLTestData *test_data = (SPARQLTestData *)data;

  // Execute multiple SPARQL queries
  int queries[] = {
      s7t_ask_pattern(test_data->sparql, 1, 1, 2), // Alice knows Bob
      s7t_ask_pattern(test_data->sparql, 2, 2, 5), // Bob works at StartupInc
      s7t_ask_pattern(test_data->sparql, 3, 3, 6), // Charlie has Programming skill
      s7t_ask_pattern(test_data->sparql, 1, 3, 7), // Alice has Design skill
      s7t_ask_pattern(test_data->sparql, 2, 3, 8)  // Bob has Marketing skill
  };

  // Use results (prevent optimization)
  int total_true = 0;
  for (int i = 0; i < 5; i++)
  {
    total_true += queries[i];
  }
}

void benchmark_sparql_with_cjinja_formatting(void *data)
{
  SPARQLTestData *test_data = (SPARQLTestData *)data;

  // Execute SPARQL query
  int alice_knows_bob = s7t_ask_pattern(test_data->sparql, 1, 1, 2);

  // Format result with CJinja
  const char *template = "Alice knows Bob: {{result}}";
  cjinja_set_var(test_data->ctx, "result", alice_knows_bob ? "Yes" : "No");

  char *formatted_result = cjinja_render_string_7tick(template, test_data->ctx);
  free(formatted_result);
}

void benchmark_sparql_with_complex_formatting(void *data)
{
  SPARQLTestData *test_data = (SPARQLTestData *)data;

  // Execute complex SPARQL query
  int alice_works_techcorp = s7t_ask_pattern(test_data->sparql, 1, 2, 4);
  int charlie_works_techcorp = s7t_ask_pattern(test_data->sparql, 3, 2, 4);

  // Set up arrays for complex template
  char *techcorp_employees[] = {"Alice", "Charlie"};
  cjinja_set_array(test_data->ctx, "employees", techcorp_employees, 2);

  const char *complex_template =
      "TechCorp Employee Analysis\n"
      "==========================\n"
      "Company: {{company_name | upper}}\n"
      "Employee Count: {{employees | length}}\n\n"
      "Employees:\n"
      "{% for employee in employees %}\n"
      "  - {{employee | capitalize}}\n"
      "{% endfor %}\n\n"
      "Summary: {{employees | length}} employees.";

  cjinja_set_var(test_data->ctx, "company_name", "TechCorp");

  char *formatted_result = cjinja_render_with_loops(complex_template, test_data->ctx);
  free(formatted_result);
}

// Setup test data
SPARQLTestData *setup_sparql_test_data(void)
{
  SPARQLTestData *data = malloc(sizeof(SPARQLTestData));

  // Create SPARQL engine
  data->sparql = s7t_create(10000, 100, 10000);

  // Create CJinja engine and context
  data->cjinja = cjinja_create("./templates");
  data->ctx = cjinja_create_context();

  // Add sample RDF data
  s7t_add_triple(data->sparql, 1, 1, 2); // (Alice, knows, Bob)
  s7t_add_triple(data->sparql, 1, 1, 3); // (Alice, knows, Charlie)
  s7t_add_triple(data->sparql, 1, 2, 4); // (Alice, worksAt, TechCorp)
  s7t_add_triple(data->sparql, 2, 1, 3); // (Bob, knows, Charlie)
  s7t_add_triple(data->sparql, 2, 2, 5); // (Bob, worksAt, StartupInc)
  s7t_add_triple(data->sparql, 3, 2, 4); // (Charlie, worksAt, TechCorp)
  s7t_add_triple(data->sparql, 3, 3, 6); // (Charlie, hasSkill, Programming)
  s7t_add_triple(data->sparql, 1, 3, 7); // (Alice, hasSkill, Design)
  s7t_add_triple(data->sparql, 2, 3, 8); // (Bob, hasSkill, Marketing)

  // Set up CJinja context variables
  cjinja_set_var(data->ctx, "entity_1", "Alice");
  cjinja_set_var(data->ctx, "entity_2", "Bob");
  cjinja_set_var(data->ctx, "entity_3", "Charlie");
  cjinja_set_var(data->ctx, "entity_4", "TechCorp");
  cjinja_set_var(data->ctx, "entity_5", "StartupInc");
  cjinja_set_var(data->ctx, "entity_6", "Programming");
  cjinja_set_var(data->ctx, "entity_7", "Design");
  cjinja_set_var(data->ctx, "entity_8", "Marketing");

  return data;
}

void cleanup_sparql_test_data(SPARQLTestData *data)
{
  s7t_destroy(data->sparql);
  cjinja_destroy_context(data->ctx);
  cjinja_destroy_engine(data->cjinja);
  free(data);
}

int main()
{
  printf("SPARQL Benchmark Suite (Refactored with Simple Framework)\n");
  printf("=========================================================\n\n");

  // Detect hardware capabilities
  HardwareInfo *hw_info = detect_hardware_capabilities();
  print_hardware_info(hw_info);

  // Create benchmark suite
  BenchmarkSuite *suite = benchmark_suite_create("SPARQL Performance Suite");

  // Setup test data
  SPARQLTestData *test_data = setup_sparql_test_data();

  printf("Running SPARQL benchmarks with simple framework...\n\n");

  // Test 1: Simple SPARQL Query
  BenchmarkResult result1 = benchmark_execute_single(
      "Simple SPARQL Query",
      10000,
      benchmark_simple_query,
      test_data);
  benchmark_suite_add_result(suite, result1);

  // Test 2: Complex SPARQL Query
  BenchmarkResult result2 = benchmark_execute_single(
      "Complex SPARQL Query",
      10000,
      benchmark_complex_query,
      test_data);
  benchmark_suite_add_result(suite, result2);

  // Test 3: Batch SPARQL Queries
  BenchmarkResult result3 = benchmark_execute_single(
      "Batch SPARQL Queries",
      10000,
      benchmark_batch_queries,
      test_data);
  benchmark_suite_add_result(suite, result3);

  // Test 4: SPARQL with CJinja Formatting (7-tick)
  BenchmarkResult result4 = benchmark_execute_single(
      "SPARQL + CJinja Formatting (7-tick)",
      10000,
      benchmark_sparql_with_cjinja_formatting,
      test_data);
  benchmark_suite_add_result(suite, result4);

  // Test 5: SPARQL with Complex CJinja Formatting (49-tick)
  BenchmarkResult result5 = benchmark_execute_single(
      "SPARQL + Complex CJinja Formatting (49-tick)",
      1000,
      benchmark_sparql_with_complex_formatting,
      test_data);
  benchmark_suite_add_result(suite, result5);

  // Calculate suite statistics
  benchmark_suite_calculate_stats(suite);

  // Print results
  printf("=== SPARQL Benchmark Results ===\n\n");
  benchmark_suite_print_detailed(suite);

  // Performance analysis
  printf("\n=== SPARQL Performance Analysis ===\n");
  BenchmarkResult *results = suite->results;

  printf("Query Performance Summary:\n");
  for (size_t i = 0; i < suite->result_count; i++)
  {
    printf("  %s:\n", results[i].test_name);
    printf("    Average time: %.1f ns\n", results[i].avg_time_ns_per_op);
    printf("    Throughput: %.0f ops/sec\n", results[i].ops_per_sec);
    printf("    Target achievement: %.1f%%\n", results[i].target_achievement_percent);
    printf("    Status: %s\n", results[i].passed ? "✅ PASSED" : "❌ FAILED");
    printf("\n");
  }

  // Integration performance comparison
  printf("Integration Performance Comparison:\n");
  printf("  Simple formatting (7-tick): %.1f ns\n", results[3].avg_time_ns_per_op);
  printf("  Complex formatting (49-tick): %.1f ns\n", results[4].avg_time_ns_per_op);
  double speedup = results[4].avg_time_ns_per_op / results[3].avg_time_ns_per_op;
  printf("  Speedup: %.1fx faster with simple formatting\n", speedup);

  // Export results
  char timestamp[64];
  time_t now = time(NULL);
  strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", localtime(&now));

  char json_filename[128];
  snprintf(json_filename, sizeof(json_filename), "sparql_benchmark_results_%s.json", timestamp);
  benchmark_suite_export_json(suite, json_filename);

  char csv_filename[128];
  snprintf(csv_filename, sizeof(csv_filename), "sparql_benchmark_results_%s.csv", timestamp);
  benchmark_suite_export_csv(suite, csv_filename);

  printf("Results exported to:\n");
  printf("  JSON: %s\n", json_filename);
  printf("  CSV: %s\n", csv_filename);

  // Cleanup
  cleanup_sparql_test_data(test_data);
  benchmark_suite_destroy(suite);
  destroy_hardware_info(hw_info);

  printf("\nSPARQL benchmark suite completed!\n");
  return 0;
}