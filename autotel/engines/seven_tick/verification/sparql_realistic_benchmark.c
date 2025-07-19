#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "../runtime/src/seven_t_runtime.h"

#define ITERATIONS 100000
#define DATASET_SIZE 10000
#define WARMUP_ITERATIONS 1000

// Helper for nanosecond timing
static inline uint64_t get_nanoseconds()
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

void benchmark_realistic_sparql()
{
  printf("SPARQL Realistic Performance Benchmark\n");
  printf("=====================================\n\n");

  // Create engine
  EngineState *engine = s7t_create_engine();
  if (!engine)
  {
    printf("❌ Failed to create engine\n");
    return;
  }

  printf("Loading realistic dataset (%d triples)...\n", DATASET_SIZE);

  // Load realistic dataset
  for (int i = 0; i < DATASET_SIZE; i++)
  {
    char s_str[64], p_str[64], o_str[64];
    snprintf(s_str, sizeof(s_str), "ex:subject_%d", i);
    snprintf(p_str, sizeof(p_str), "ex:predicate_%d", i % 100); // 100 different predicates
    snprintf(o_str, sizeof(o_str), "ex:object_%d", i % 1000);   // 1000 different objects

    uint32_t s = s7t_intern_string(engine, s_str);
    uint32_t p = s7t_intern_string(engine, p_str);
    uint32_t o = s7t_intern_string(engine, o_str);

    s7t_add_triple(engine, s, p, o);
  }

  printf("✅ Dataset loaded\n");

  // Warmup with random queries
  printf("Warming up with random queries...\n");
  for (int i = 0; i < WARMUP_ITERATIONS; i++)
  {
    int idx = rand() % DATASET_SIZE;
    char s_str[64], p_str[64], o_str[64];
    snprintf(s_str, sizeof(s_str), "ex:subject_%d", idx);
    snprintf(p_str, sizeof(p_str), "ex:predicate_%d", idx % 100);
    snprintf(o_str, sizeof(o_str), "ex:object_%d", idx % 1000);

    uint32_t s = s7t_intern_string(engine, s_str);
    uint32_t p = s7t_intern_string(engine, p_str);
    uint32_t o = s7t_intern_string(engine, o_str);

    s7t_ask_pattern(engine, s, p, o);
  }

  // Benchmark with random queries (cache misses)
  printf("\nBenchmarking with random queries (cache misses)...\n");

  uint64_t start = get_nanoseconds();
  for (int i = 0; i < ITERATIONS; i++)
  {
    int idx = rand() % DATASET_SIZE;
    char s_str[64], p_str[64], o_str[64];
    snprintf(s_str, sizeof(s_str), "ex:subject_%d", idx);
    snprintf(p_str, sizeof(p_str), "ex:predicate_%d", idx % 100);
    snprintf(o_str, sizeof(o_str), "ex:object_%d", idx % 1000);

    uint32_t s = s7t_intern_string(engine, s_str);
    uint32_t p = s7t_intern_string(engine, p_str);
    uint32_t o = s7t_intern_string(engine, o_str);

    s7t_ask_pattern(engine, s, p, o);
  }
  uint64_t end = get_nanoseconds();

  double avg_ns = (double)(end - start) / ITERATIONS;
  double throughput = (double)ITERATIONS * 1000000000.0 / (end - start);

  printf("Random query performance:\n");
  printf("  • Average latency: %.1f ns\n", avg_ns);
  printf("  • Throughput: %.1f queries/sec\n", throughput);

  if (avg_ns < 10.0)
  {
    printf("  🎉 7-TICK PERFORMANCE ACHIEVED! (< 10ns)\n");
  }
  else if (avg_ns < 100.0)
  {
    printf("  ✅ Sub-100ns performance\n");
  }
  else if (avg_ns < 1000.0)
  {
    printf("  ✅ Sub-μs performance\n");
  }
  else
  {
    printf("  ⚠️ Performance above 1μs\n");
  }

  // Benchmark with repeated queries (cache hits)
  printf("\nBenchmarking with repeated queries (cache hits)...\n");

  // Use fixed IDs for repeated queries
  uint32_t fixed_s = s7t_intern_string(engine, "ex:subject_1000");
  uint32_t fixed_p = s7t_intern_string(engine, "ex:predicate_50");
  uint32_t fixed_o = s7t_intern_string(engine, "ex:object_500");

  start = get_nanoseconds();
  for (int i = 0; i < ITERATIONS; i++)
  {
    s7t_ask_pattern(engine, fixed_s, fixed_p, fixed_o);
  }
  end = get_nanoseconds();

  avg_ns = (double)(end - start) / ITERATIONS;
  throughput = (double)ITERATIONS * 1000000000.0 / (end - start);

  printf("Repeated query performance:\n");
  printf("  • Average latency: %.1f ns\n", avg_ns);
  printf("  • Throughput: %.1f queries/sec\n", throughput);

  // Benchmark with non-existent queries
  printf("\nBenchmarking with non-existent queries...\n");

  uint32_t nonexistent_s = s7t_intern_string(engine, "ex:nonexistent_subject");
  uint32_t nonexistent_p = s7t_intern_string(engine, "ex:nonexistent_predicate");
  uint32_t nonexistent_o = s7t_intern_string(engine, "ex:nonexistent_object");

  start = get_nanoseconds();
  for (int i = 0; i < ITERATIONS; i++)
  {
    s7t_ask_pattern(engine, nonexistent_s, nonexistent_p, nonexistent_o);
  }
  end = get_nanoseconds();

  avg_ns = (double)(end - start) / ITERATIONS;
  throughput = (double)ITERATIONS * 1000000000.0 / (end - start);

  printf("Non-existent query performance:\n");
  printf("  • Average latency: %.1f ns\n", avg_ns);
  printf("  • Throughput: %.1f queries/sec\n", throughput);

  // Test hash table collision scenarios
  printf("\nTesting hash table collision scenarios...\n");

  // Create many entries with same hash
  for (int i = 0; i < 100; i++)
  {
    char s_str[64], p_str[64], o_str[64];
    snprintf(s_str, sizeof(s_str), "ex:collision_s_%d", i);
    snprintf(p_str, sizeof(p_str), "ex:collision_p_%d", i);
    snprintf(o_str, sizeof(o_str), "ex:collision_o_%d", i);

    uint32_t s = s7t_intern_string(engine, s_str);
    uint32_t p = s7t_intern_string(engine, p_str);
    uint32_t o = s7t_intern_string(engine, o_str);

    s7t_add_triple(engine, s, p, o);
  }

  // Query with potential collisions
  start = get_nanoseconds();
  for (int i = 0; i < ITERATIONS; i++)
  {
    int idx = i % 100;
    char s_str[64], p_str[64], o_str[64];
    snprintf(s_str, sizeof(s_str), "ex:collision_s_%d", idx);
    snprintf(p_str, sizeof(p_str), "ex:collision_p_%d", idx);
    snprintf(o_str, sizeof(o_str), "ex:collision_o_%d", idx);

    uint32_t s = s7t_intern_string(engine, s_str);
    uint32_t p = s7t_intern_string(engine, p_str);
    uint32_t o = s7t_intern_string(engine, o_str);

    s7t_ask_pattern(engine, s, p, o);
  }
  end = get_nanoseconds();

  avg_ns = (double)(end - start) / ITERATIONS;
  throughput = (double)ITERATIONS * 1000000000.0 / (end - start);

  printf("Collision scenario performance:\n");
  printf("  • Average latency: %.1f ns\n", avg_ns);
  printf("  • Throughput: %.1f queries/sec\n", throughput);

  // Cleanup
  s7t_destroy_engine(engine);

  printf("\nRealistic SPARQL Benchmark Complete\n");
  printf("===================================\n");
}

int main()
{
  srand(42); // Fixed seed for reproducible results
  benchmark_realistic_sparql();
  return 0;
}