#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include "../runtime/src/seven_t_runtime.h"
#include "../c_src/sparql7t.h"
#include "../c_src/shacl7t.h"

// High-precision timing
static inline uint64_t get_microseconds()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000000ULL + tv.tv_usec;
}

// Test batch operations performance
int main()
{
  printf("7T Batch Operations Benchmark\n");
  printf("=============================\n\n");

  // 1. Test Batch Pattern Matching
  printf("1. BATCH PATTERN MATCHING\n");
  printf("   Testing optimized s7t_ask_batch() vs individual calls\n\n");

  // Create SPARQL engine
  S7TEngine *sparql = s7t_create(10000, 100, 1000);

  // Add test data
  for (int i = 0; i < 5000; i++)
  {
    s7t_add_triple(sparql, i, i % 50, i % 200);
  }

  // Create test patterns
  const int num_patterns = 10000;
  TriplePattern *patterns = malloc(num_patterns * sizeof(TriplePattern));
  int *results = malloc(num_patterns * sizeof(int));

  for (int i = 0; i < num_patterns; i++)
  {
    patterns[i].s = i % 5000;
    patterns[i].p = i % 50;
    patterns[i].o = i % 200;
  }

  // Test batch performance
  uint64_t start = get_microseconds();
  s7t_ask_batch(sparql, patterns, results, num_patterns);
  uint64_t end = get_microseconds();

  double batch_time = (end - start) / 1000000.0;
  printf("Batch pattern matching: %d patterns in %.3f seconds\n", num_patterns, batch_time);
  printf("Batch throughput: %.0f patterns/sec\n", num_patterns / batch_time);

  // Test individual performance for comparison
  start = get_microseconds();
  for (int i = 0; i < num_patterns; i++)
  {
    results[i] = s7t_ask_pattern(sparql, patterns[i].s, patterns[i].p, patterns[i].o);
  }
  end = get_microseconds();

  double individual_time = (end - start) / 1000000.0;
  printf("Individual pattern matching: %d patterns in %.3f seconds\n", num_patterns, individual_time);
  printf("Individual throughput: %.0f patterns/sec\n", num_patterns / individual_time);

  printf("Batch is %.1fx faster than individual calls\n\n", individual_time / batch_time);

  // 2. Test Batch SHACL Validation
  printf("2. BATCH SHACL VALIDATION\n");
  printf("   Testing optimized shacl_validate_batch() vs individual calls\n\n");

  // Create SHACL engine
  ShaclEngine *shacl = shacl_create(10000, 10);

  // Add test shapes
  CompiledShape shape1 = {0};
  shape1.target_class_mask = 1; // Target class 0
  shape1.property_mask = 3;     // Require properties 0 and 1
  shacl_add_shape(shacl, 0, &shape1);

  CompiledShape shape2 = {0};
  shape2.target_class_mask = 2; // Target class 1
  shape2.property_mask = 5;     // Require properties 0 and 2
  shacl_add_shape(shacl, 1, &shape2);

  // Set up test nodes
  for (int i = 0; i < 5000; i++)
  {
    // Set class membership
    shacl_set_node_class(shacl, i, i % 2);

    // Set properties (some nodes have required properties, some don't)
    if (i % 3 != 0)
    {
      shacl_set_node_property(shacl, i, 0); // Property 0
    }
    if (i % 4 != 0)
    {
      shacl_set_node_property(shacl, i, 1); // Property 1
    }
    if (i % 5 != 0)
    {
      shacl_set_node_property(shacl, i, 2); // Property 2
    }
  }

  // Create test validation requests
  uint32_t *nodes = malloc(num_patterns * sizeof(uint32_t));
  uint32_t *shapes = malloc(num_patterns * sizeof(uint32_t));
  int *validation_results = malloc(num_patterns * sizeof(int));

  for (int i = 0; i < num_patterns; i++)
  {
    nodes[i] = i % 5000;
    shapes[i] = i % 2; // Alternate between shape 0 and 1
  }

  // Test batch validation performance
  start = get_microseconds();
  shacl_validate_batch(shacl, nodes, shapes, validation_results, num_patterns);
  end = get_microseconds();

  double batch_validation_time = (end - start) / 1000000.0;
  printf("Batch SHACL validation: %d nodes in %.3f seconds\n", num_patterns, batch_validation_time);
  printf("Batch validation throughput: %.0f nodes/sec\n", num_patterns / batch_validation_time);

  // Test individual validation for comparison
  start = get_microseconds();
  for (int i = 0; i < num_patterns; i++)
  {
    validation_results[i] = shacl_validate_node(shacl, nodes[i], shapes[i]);
  }
  end = get_microseconds();

  double individual_validation_time = (end - start) / 1000000.0;
  printf("Individual SHACL validation: %d nodes in %.3f seconds\n", num_patterns, individual_validation_time);
  printf("Individual validation throughput: %.0f nodes/sec\n", num_patterns / individual_validation_time);

  printf("Batch validation is %.1fx faster than individual calls\n\n", individual_validation_time / batch_validation_time);

  // 3. Test Query Result Materialization
  printf("3. QUERY RESULT MATERIALIZATION\n");
  printf("   Testing new s7t_materialize_subjects() function\n\n");

  // Create runtime engine for materialization test
  EngineState *engine = s7t_create_engine();

  // Add test data
  uint32_t pred_type = s7t_intern_string(engine, "type");
  uint32_t class_Person = s7t_intern_string(engine, "Person");

  for (int i = 0; i < 10000; i++)
  {
    char buf[256];
    snprintf(buf, sizeof(buf), "person_%d", i);
    uint32_t person = s7t_intern_string(engine, buf);
    s7t_add_triple(engine, person, pred_type, class_Person);
  }

  // Test materialization performance
  start = get_microseconds();
  size_t result_count;
  uint32_t *materialized_results = s7t_materialize_subjects(engine, pred_type, class_Person, &result_count);
  end = get_microseconds();

  double materialization_time = (end - start) / 1000000.0;
  printf("Query materialization: %zu results in %.6f seconds\n", result_count, materialization_time);
  printf("Materialization rate: %.0f results/sec\n", result_count / materialization_time);

  if (materialized_results)
  {
    printf("First 5 results: ");
    for (int i = 0; i < 5 && i < result_count; i++)
    {
      printf("%u ", materialized_results[i]);
    }
    printf("\n");
    free(materialized_results);
  }

  // Performance summary
  printf("\nPERFORMANCE SUMMARY:\n");
  printf("===================\n");
  printf("Batch Pattern Matching:    %.0f patterns/sec\n", num_patterns / batch_time);
  printf("Batch SHACL Validation:    %.0f nodes/sec\n", num_patterns / batch_validation_time);
  printf("Query Materialization:     %.0f results/sec\n", result_count / materialization_time);
  printf("Batch Speedup (Pattern):   %.1fx\n", individual_time / batch_time);
  printf("Batch Speedup (SHACL):     %.1fx\n", individual_validation_time / batch_validation_time);

  // Cleanup
  free(patterns);
  free(results);
  free(nodes);
  free(shapes);
  free(validation_results);

  // Note: These engines don't have proper destroy functions in the headers
  // In production, we'd properly free the memory

  printf("\n✅ Batch Operations Benchmark Complete!\n");
  return 0;
}