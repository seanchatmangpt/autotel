#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../runtime/src/seven_t_runtime.h"

int main()
{
  printf("7T String Interning Performance Test\n");
  printf("====================================\n\n");

  // Create engine
  printf("Creating engine...\n");
  EngineState *engine = s7t_create_engine();
  if (!engine)
  {
    printf("Failed to create engine\n");
    return 1;
  }

  // Test string interning performance
  printf("Testing string interning with 1,000 strings...\n");
  uint64_t start = clock();

  for (int i = 0; i < 1000; i++)
  {
    char buf[256];
    snprintf(buf, sizeof(buf), "test_string_%d", i);
    uint32_t id = s7t_intern_string(engine, buf);

    if (i % 100 == 0)
    {
      printf("Added string %d with ID %u\n", i, id);
    }
  }

  uint64_t end = clock();
  double time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;

  printf("Added 1,000 strings in %.3f seconds\n", time_taken);
  printf("String interning rate: %.0f strings/sec\n\n", 1000.0 / time_taken);

  // Test pattern matching
  printf("Testing pattern matching...\n");
  uint32_t test_pred = s7t_intern_string(engine, "test_predicate");
  uint32_t test_obj = s7t_intern_string(engine, "test_object");

  start = clock();
  int matches = 0;
  for (int i = 0; i < 1000; i++)
  {
    char buf[256];
    snprintf(buf, sizeof(buf), "test_string_%d", i);
    uint32_t test_subj = s7t_intern_string(engine, buf);

    // Add some triples
    s7t_add_triple(engine, test_subj, test_pred, test_obj);

    // Test pattern matching
    if (s7t_ask_pattern(engine, test_subj, test_pred, test_obj))
    {
      matches++;
    }
  }
  end = clock();
  time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;

  printf("Pattern matching: %d matches in %.3f seconds\n", matches, time_taken);
  printf("Pattern matching rate: %.0f ops/sec\n", 1000.0 / time_taken);

  // Cleanup
  s7t_destroy_engine(engine);
  printf("\n✅ String interning test complete!\n");
  return 0;
}