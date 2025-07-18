#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "../runtime/src/seven_t_runtime.h"

// Performance test with 200 triples
int main()
{
  printf("7T Performance Test (200 Triples)\n");
  printf("=================================\n\n");

  // Step 1: Create engine
  printf("Creating engine...\n");
  EngineState *engine = s7t_create_engine();

  // Step 2: Add test data
  printf("Adding 200 triples...\n");

  // Define predicates
  uint32_t pred_type = s7t_intern_string(engine, "type");
  uint32_t pred_name = s7t_intern_string(engine, "name");
  uint32_t pred_has = s7t_intern_string(engine, "has");

  // Define classes
  uint32_t class_Person = s7t_intern_string(engine, "Person");
  uint32_t class_Item = s7t_intern_string(engine, "Item");

  clock_t start = clock();

  // Create 50 people with items
  for (int i = 0; i < 50; i++)
  {
    char buf[256];

    // Person
    snprintf(buf, sizeof(buf), "person_%d", i);
    uint32_t person = s7t_intern_string(engine, buf);
    s7t_add_triple(engine, person, pred_type, class_Person);

    // Person name
    snprintf(buf, sizeof(buf), "Person %d", i);
    uint32_t name = s7t_intern_string(engine, buf);
    s7t_add_triple(engine, person, pred_name, name);

    // Items (4 per person)
    for (int j = 0; j < 4; j++)
    {
      snprintf(buf, sizeof(buf), "item_%d_%d", i, j);
      uint32_t item = s7t_intern_string(engine, buf);
      s7t_add_triple(engine, item, pred_type, class_Item);
      s7t_add_triple(engine, person, pred_has, item);
    }
  }

  clock_t end = clock();
  double add_time = ((double)(end - start)) / CLOCKS_PER_SEC;

  printf("Added %zu triples in %.3f seconds\n", engine->triple_count, add_time);
  printf("Triple addition rate: %.0f triples/sec\n", engine->triple_count / add_time);

  // Step 3: Test query performance
  printf("\nTesting query performance...\n");

  // Warm up
  for (int i = 0; i < 10; i++)
  {
    BitVector *people = s7t_get_subject_vector(engine, pred_type, class_Person);
    bitvec_destroy(people);
  }

  // Benchmark
  start = clock();
  int iterations = 10000;

  for (int i = 0; i < iterations; i++)
  {
    BitVector *people = s7t_get_subject_vector(engine, pred_type, class_Person);

    // Count results
    size_t count = bitvec_popcount(people);

    bitvec_destroy(people);
  }

  end = clock();
  double query_time = ((double)(end - start)) / CLOCKS_PER_SEC;

  printf("Executed %d queries in %.3f seconds\n", iterations, query_time);
  printf("Query throughput: %.2f KQPS\n", (iterations / query_time) / 1e3);
  printf("Average query latency: %.2f microseconds\n", (query_time * 1e6) / iterations);

  // Test object lookup
  printf("\nTesting object lookup...\n");
  start = clock();

  for (int i = 0; i < 1000; i++)
  {
    uint32_t person = s7t_intern_string(engine, "person_0");
    BitVector *objects = s7t_get_object_vector(engine, pred_has, person);
    bitvec_destroy(objects);
  }

  end = clock();
  double lookup_time = ((double)(end - start)) / CLOCKS_PER_SEC;

  printf("Executed 1000 object lookups in %.3f seconds\n", lookup_time);
  printf("Object lookup latency: %.2f microseconds\n", (lookup_time * 1e6) / 1000);

  // Success criteria check
  double avg_query_latency_us = (query_time * 1e6) / iterations;
  double query_throughput_kqps = (iterations / query_time) / 1e3;

  printf("\nPerformance Results:\n");
  printf("-------------------\n");

  if (avg_query_latency_us < 100)
  { // < 100 microseconds
    printf("✅ PASS: Query latency %.0f μs meets requirement (<100 μs)\n", avg_query_latency_us);
  }
  else
  {
    printf("❌ FAIL: Query latency %.0f μs exceeds requirement\n", avg_query_latency_us);
  }

  if (query_throughput_kqps > 1.0)
  {
    printf("✅ PASS: Query throughput %.1f KQPS exceeds requirement (>1 KQPS)\n", query_throughput_kqps);
  }
  else
  {
    printf("❌ FAIL: Query throughput %.1f KQPS below requirement\n", query_throughput_kqps);
  }

  // Cleanup
  s7t_destroy_engine(engine);

  printf("\n🎉 7T Performance Test Complete!\n");

  return 0;
}