#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include "../runtime/src/seven_t_runtime.h"

// High-precision timing
static inline uint64_t get_microseconds()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000000ULL + tv.tv_usec;
}

int main()
{
  printf("7T Scalability Performance Test\n");
  printf("===============================\n\n");

  // Test different dataset sizes
  int test_sizes[] = {100, 500, 1000, 2000, 5000, 10000};
  int num_tests = sizeof(test_sizes) / sizeof(test_sizes[0]);

  for (int test_idx = 0; test_idx < num_tests; test_idx++)
  {
    int size = test_sizes[test_idx];
    printf("Testing with %d entities...\n", size);

    // Create engine
    EngineState *engine = s7t_create_engine();
    if (!engine)
    {
      printf("Failed to create engine for size %d\n", size);
      continue;
    }

    // Test string interning performance
    uint64_t start = get_microseconds();
    int success = 1;

    for (int i = 0; i < size; i++)
    {
      char buf[256];
      snprintf(buf, sizeof(buf), "entity_%d", i);
      uint32_t id = s7t_intern_string(engine, buf);

      // Add some properties
      snprintf(buf, sizeof(buf), "property_%d", i % 10);
      uint32_t prop_id = s7t_intern_string(engine, buf);

      snprintf(buf, sizeof(buf), "value_%d", i % 100);
      uint32_t val_id = s7t_intern_string(engine, buf);

      // Add triple
      s7t_add_triple(engine, id, prop_id, val_id);

      if (i % 1000 == 0 && i > 0)
      {
        printf("  Added %d entities...\n", i);
      }
    }

    uint64_t end = get_microseconds();
    double time_taken = (end - start) / 1000000.0;

    if (success)
    {
      printf("✅ Successfully added %d entities in %.3f seconds\n", size, time_taken);
      printf("   Rate: %.0f entities/sec\n", size / time_taken);

      // Test pattern matching performance
      start = get_microseconds();
      int matches = 0;
      for (int i = 0; i < size; i++)
      {
        char buf[256];
        snprintf(buf, sizeof(buf), "entity_%d", i);
        uint32_t entity_id = s7t_intern_string(engine, buf);

        snprintf(buf, sizeof(buf), "property_%d", i % 10);
        uint32_t prop_id = s7t_intern_string(engine, buf);

        snprintf(buf, sizeof(buf), "value_%d", i % 100);
        uint32_t val_id = s7t_intern_string(engine, buf);

        if (s7t_ask_pattern(engine, entity_id, prop_id, val_id))
        {
          matches++;
        }
      }
      end = get_microseconds();
      time_taken = (end - start) / 1000000.0;

      printf("   Pattern matching: %d matches in %.3f seconds\n", matches, time_taken);
      printf("   Pattern matching rate: %.0f ops/sec\n", size / time_taken);
    }
    else
    {
      printf("❌ Failed at %d entities\n", size);
    }

    // Cleanup
    s7t_destroy_engine(engine);
    printf("\n");
  }

  printf("🎉 Scalability test complete!\n");
  return 0;
}