#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "../runtime/src/seven_t_runtime.h"

#define ITERATIONS 1000000
#define WARMUP_ITERATIONS 10000

// Helper for nanosecond timing
static inline uint64_t get_nanoseconds()
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

void benchmark_sparql_pattern_matching()
{
  printf("SPARQL 7-Tick Performance Benchmark\n");
  printf("===================================\n\n");

  // Create engine
  EngineState *engine = s7t_create_engine();
  if (!engine)
  {
    printf("❌ Failed to create engine\n");
    return;
  }

  // Add test data
  uint32_t s1 = s7t_intern_string(engine, "ex:alice");
  uint32_t s2 = s7t_intern_string(engine, "ex:bob");
  uint32_t p1 = s7t_intern_string(engine, "ex:knows");
  uint32_t p2 = s7t_intern_string(engine, "ex:likes");
  uint32_t o1 = s7t_intern_string(engine, "ex:charlie");
  uint32_t o2 = s7t_intern_string(engine, "ex:pizza");

  s7t_add_triple(engine, s1, p1, o1); // Alice knows Charlie
  s7t_add_triple(engine, s1, p2, o2); // Alice likes Pizza
  s7t_add_triple(engine, s2, p1, o1); // Bob knows Charlie

  printf("✅ Test data loaded\n");

  // Warmup
  for (int i = 0; i < WARMUP_ITERATIONS; i++)
  {
    s7t_ask_pattern(engine, s1, p1, o1);
    s7t_ask_pattern(engine, s1, p2, o2);
    s7t_ask_pattern(engine, s2, p1, o1);
  }

  // Benchmark pattern matching
  printf("\nBenchmarking SPARQL pattern matching...\n");

  uint64_t start = get_nanoseconds();
  for (int i = 0; i < ITERATIONS; i++)
  {
    s7t_ask_pattern(engine, s1, p1, o1); // Should be true
  }
  uint64_t end = get_nanoseconds();

  double avg_ns = (double)(end - start) / ITERATIONS;
  double throughput = (double)ITERATIONS * 1000000000.0 / (end - start);

  printf("Pattern matching performance:\n");
  printf("  • Average latency: %.1f ns\n", avg_ns);
  printf("  • Throughput: %.1f patterns/sec\n", throughput);

  if (avg_ns < 10.0)
  {
    printf("  🎉 7-TICK PERFORMANCE ACHIEVED! (< 10ns)\n");
  }
  else if (avg_ns < 100.0)
  {
    printf("  ✅ Sub-100ns performance\n");
  }
  else
  {
    printf("  ⚠️ Performance above 100ns\n");
  }

  // Test multiple patterns
  printf("\nTesting multiple patterns...\n");

  start = get_nanoseconds();
  for (int i = 0; i < ITERATIONS; i++)
  {
    s7t_ask_pattern(engine, s1, p1, o1); // True
    s7t_ask_pattern(engine, s1, p2, o2); // True
    s7t_ask_pattern(engine, s2, p1, o1); // True
    s7t_ask_pattern(engine, s1, p1, o2); // False
  }
  end = get_nanoseconds();

  avg_ns = (double)(end - start) / (ITERATIONS * 4);
  throughput = (double)(ITERATIONS * 4) * 1000000000.0 / (end - start);

  printf("Multiple pattern performance:\n");
  printf("  • Average latency: %.1f ns per pattern\n", avg_ns);
  printf("  • Throughput: %.1f patterns/sec\n", throughput);

  // Test wildcard queries (o == 0)
  printf("\nTesting wildcard queries...\n");

  start = get_nanoseconds();
  for (int i = 0; i < ITERATIONS; i++)
  {
    s7t_ask_pattern(engine, s1, p1, 0); // Check if Alice has 'knows' property
    s7t_ask_pattern(engine, s1, p2, 0); // Check if Alice has 'likes' property
  }
  end = get_nanoseconds();

  avg_ns = (double)(end - start) / (ITERATIONS * 2);
  throughput = (double)(ITERATIONS * 2) * 1000000000.0 / (end - start);

  printf("Wildcard query performance:\n");
  printf("  • Average latency: %.1f ns per query\n", avg_ns);
  printf("  • Throughput: %.1f queries/sec\n", throughput);

  // Verify correctness
  printf("\nVerifying correctness...\n");
  int result1 = s7t_ask_pattern(engine, s1, p1, o1); // Should be true
  int result2 = s7t_ask_pattern(engine, s1, p2, o2); // Should be true
  int result3 = s7t_ask_pattern(engine, s2, p1, o1); // Should be true
  int result4 = s7t_ask_pattern(engine, s1, p1, o2); // Should be false

  if (result1 && result2 && result3 && !result4)
  {
    printf("  ✅ All pattern matches correct\n");
  }
  else
  {
    printf("  ❌ Pattern matching incorrect\n");
  }

  // Cleanup
  s7t_destroy_engine(engine);

  printf("\nSPARQL 7-Tick Benchmark Complete\n");
  printf("================================\n");
}

int main()
{
  benchmark_sparql_pattern_matching();
  return 0;
}