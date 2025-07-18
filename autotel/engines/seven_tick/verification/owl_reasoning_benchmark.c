#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include "../runtime/src/seven_t_runtime.h"
#include "../c_src/owl7t.h"

// High-precision timing
static inline uint64_t get_microseconds()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000000ULL + tv.tv_usec;
}

// Test OWL reasoning with transitive properties
int main()
{
  printf("7T OWL Reasoning Benchmark\n");
  printf("==========================\n\n");

  // Create base engine
  printf("Creating base engine...\n");
  EngineState *base_engine = s7t_create_engine();

  // Create OWL reasoning engine
  printf("Creating OWL reasoning engine...\n");
  OWLEngine *owl = owl_create(base_engine, 1000, 100);

  // Define transitive properties
  uint32_t pred_ancestor = s7t_intern_string(base_engine, "ancestor");
  uint32_t pred_part_of = s7t_intern_string(base_engine, "part_of");
  uint32_t pred_contains = s7t_intern_string(base_engine, "contains");

  owl_set_transitive(owl, pred_ancestor);
  owl_set_transitive(owl, pred_part_of);
  owl_set_transitive(owl, pred_contains);

  printf("Defined transitive properties: ancestor, part_of, contains\n\n");

  // Create family tree data (transitive ancestor relationships)
  printf("Creating family tree with transitive relationships...\n");
  uint64_t start = get_microseconds();

  // Create 1000 people in a family tree
  for (int i = 0; i < 1000; i++)
  {
    char buf[256];
    snprintf(buf, sizeof(buf), "person_%d", i);
    uint32_t person = s7t_intern_string(base_engine, buf);

    // Each person has 2 parents (except root)
    if (i > 0)
    {
      int parent1 = (i - 1) / 2;
      int parent2 = (i - 1) / 2 + 500; // Different branch

      snprintf(buf, sizeof(buf), "person_%d", parent1);
      uint32_t parent1_id = s7t_intern_string(base_engine, buf);
      s7t_add_triple(base_engine, person, pred_ancestor, parent1_id);

      if (parent2 < 1000)
      {
        snprintf(buf, sizeof(buf), "person_%d", parent2);
        uint32_t parent2_id = s7t_intern_string(base_engine, buf);
        s7t_add_triple(base_engine, person, pred_ancestor, parent2_id);
      }
    }
  }

  uint64_t end = get_microseconds();
  double add_time = (end - start) / 1000000.0;

  printf("Added %zu triples in %.3f seconds\n", base_engine->triple_count, add_time);
  printf("Triple addition rate: %.0f triples/sec\n\n", base_engine->triple_count / add_time);

  // Test transitive reasoning queries
  printf("Testing transitive reasoning queries...\n");

  // Test ancestor queries at different depths
  int test_cases[] = {1, 2, 3, 4, 5, 10, 20, 50, 100};
  int num_tests = sizeof(test_cases) / sizeof(test_cases[0]);

  start = get_microseconds();
  int iterations = 1000;

  for (int iter = 0; iter < iterations; iter++)
  {
    for (int i = 0; i < num_tests; i++)
    {
      int depth = test_cases[i];
      int person_id = 999 - depth; // Start from bottom of tree

      char buf[256];
      snprintf(buf, sizeof(buf), "person_%d", person_id);
      uint32_t person = s7t_intern_string(base_engine, buf);

      snprintf(buf, sizeof(buf), "person_%d", 0); // Root ancestor
      uint32_t root = s7t_intern_string(base_engine, buf);

      // Query: Is person an ancestor of root? (should be false)
      int result = owl_ask_with_reasoning(owl, person, pred_ancestor, root);
      (void)result; // Prevent unused variable warning
    }
  }

  end = get_microseconds();
  double reasoning_time = (end - start) / 1000000.0;

  printf("Executed %d transitive reasoning queries in %.3f seconds\n",
         iterations * num_tests, reasoning_time);
  printf("Transitive reasoning rate: %.0f queries/sec\n",
         (iterations * num_tests) / reasoning_time);

  // Test specific transitive paths
  printf("\nTesting specific transitive paths...\n");

  // Test a long chain: person_999 -> person_0 (should be true)
  uint32_t person_999 = s7t_intern_string(base_engine, "person_999");
  uint32_t person_0 = s7t_intern_string(base_engine, "person_0");

  start = get_microseconds();
  int result = owl_ask_with_reasoning(owl, person_999, pred_ancestor, person_0);
  end = get_microseconds();

  double path_time = (end - start) / 1000000.0;
  printf("Long chain query (999->0): %s in %.6f seconds\n",
         result ? "TRUE" : "FALSE", path_time);

  // Test a short chain: person_1 -> person_0 (should be true)
  uint32_t person_1 = s7t_intern_string(base_engine, "person_1");

  start = get_microseconds();
  result = owl_ask_with_reasoning(owl, person_1, pred_ancestor, person_0);
  end = get_microseconds();

  path_time = (end - start) / 1000000.0;
  printf("Short chain query (1->0): %s in %.6f seconds\n",
         result ? "TRUE" : "FALSE", path_time);

  // Test non-existent path: person_0 -> person_999 (should be false)
  start = get_microseconds();
  result = owl_ask_with_reasoning(owl, person_0, pred_ancestor, person_999);
  end = get_microseconds();

  path_time = (end - start) / 1000000.0;
  printf("Non-existent path (0->999): %s in %.6f seconds\n",
         result ? "TRUE" : "FALSE", path_time);

  // Test closure computation
  printf("\nTesting closure computation...\n");
  start = get_microseconds();

  owl_compute_closures(owl);

  end = get_microseconds();
  double closure_time = (end - start) / 1000000.0;

  printf("Closure computation completed in %.3f seconds\n", closure_time);

  // Cleanup
  owl_destroy(owl);
  s7t_destroy_engine(base_engine);

  printf("\n✅ OWL Reasoning Benchmark Complete!\n");
  return 0;
}