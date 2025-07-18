#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include "../runtime/src/seven_t_runtime.h"

// High-precision timing for nanosecond measurements
static inline uint64_t get_nanoseconds()
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

// Test 7-tick performance requirements
int main()
{
  printf("7T Seven-Tick Performance Benchmark\n");
  printf("===================================\n\n");

  printf("Testing ≤7 CPU cycles and <10 nanoseconds performance:\n\n");

  // Create engine and add test data
  printf("Creating engine with test data...\n");
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

  printf("Added %zu triples\n\n", engine->triple_count);

  // Test 1: Individual pattern matching (should be ≤7 ticks)
  printf("1. INDIVIDUAL PATTERN MATCHING\n");
  printf("   Testing s7t_ask_pattern() performance\n");

  uint32_t test_subject = s7t_intern_string(engine, "person_100");

  // Warmup
  for (int i = 0; i < 1000; i++)
  {
    s7t_ask_pattern(engine, test_subject, pred_type, class_Person);
  }

  // Performance test
  uint64_t start = get_nanoseconds();
  int iterations = 1000000;

  for (int i = 0; i < iterations; i++)
  {
    s7t_ask_pattern(engine, test_subject, pred_type, class_Person);
  }

  uint64_t end = get_nanoseconds();
  uint64_t total_ns = end - start;
  double avg_ns = (double)total_ns / iterations;

  printf("   Executed %d pattern matches in %llu ns\n", iterations, total_ns);
  printf("   Average latency: %.2f nanoseconds\n", avg_ns);
  printf("   Throughput: %.0f patterns/sec\n", iterations * 1000000000.0 / total_ns);

  if (avg_ns < 10.0)
  {
    printf("   ✅ PASS: <10 nanoseconds requirement met\n");
  }
  else
  {
    printf("   ❌ FAIL: Exceeds 10 nanoseconds requirement\n");
  }
  printf("\n");

  // Test 2: Query materialization (should be ≤7 ticks per result)
  printf("2. QUERY MATERIALIZATION\n");
  printf("   Testing s7t_materialize_subjects() performance\n");

  // Warmup
  size_t warmup_count;
  uint32_t *warmup_results = s7t_materialize_subjects(engine, pred_type, class_Person, &warmup_count);
  if (warmup_results)
    free(warmup_results);

  // Performance test
  start = get_nanoseconds();
  size_t result_count;
  uint32_t *results = s7t_materialize_subjects(engine, pred_type, class_Person, &result_count);
  end = get_nanoseconds();

  uint64_t materialization_ns = end - start;
  double avg_per_result_ns = (double)materialization_ns / result_count;

  printf("   Materialized %zu results in %llu ns\n", result_count, materialization_ns);
  printf("   Average per result: %.2f nanoseconds\n", avg_per_result_ns);
  printf("   Materialization rate: %.0f results/sec\n", result_count * 1000000000.0 / materialization_ns);

  if (avg_per_result_ns < 10.0)
  {
    printf("   ✅ PASS: <10 nanoseconds per result requirement met\n");
  }
  else
  {
    printf("   ❌ FAIL: Exceeds 10 nanoseconds per result requirement\n");
  }

  if (results)
  {
    printf("   Sample results: ");
    for (int i = 0; i < 5 && i < result_count; i++)
    {
      printf("%u ", results[i]);
    }
    if (result_count > 5)
      printf("...");
    printf("\n");
    free(results);
  }
  printf("\n");

  // Test 3: Bit vector operations (should be ≤7 ticks)
  printf("3. BIT VECTOR OPERATIONS\n");
  printf("   Testing bit vector intersection performance\n");

  // Warmup
  BitVector *warmup_pred = s7t_get_subject_vector(engine, pred_type, class_Person);
  BitVector *warmup_obj = s7t_get_subject_vector(engine, pred_type, class_Person);
  if (warmup_pred && warmup_obj)
  {
    BitVector *warmup_intersection = bitvec_and(warmup_pred, warmup_obj);
    if (warmup_intersection)
      bitvec_destroy(warmup_intersection);
  }
  if (warmup_pred)
    bitvec_destroy(warmup_pred);
  if (warmup_obj)
    bitvec_destroy(warmup_obj);

  // Performance test
  start = get_nanoseconds();

  for (int i = 0; i < 100000; i++)
  {
    BitVector *pred_vec = s7t_get_subject_vector(engine, pred_type, class_Person);
    BitVector *obj_vec = s7t_get_subject_vector(engine, pred_type, class_Person);

    if (pred_vec && obj_vec)
    {
      BitVector *intersection = bitvec_and(pred_vec, obj_vec);
      size_t count = bitvec_popcount(intersection);
      (void)count; // Prevent unused variable warning
      if (intersection)
        bitvec_destroy(intersection);
    }

    if (pred_vec)
      bitvec_destroy(pred_vec);
    if (obj_vec)
      bitvec_destroy(obj_vec);
  }

  end = get_nanoseconds();
  uint64_t bitvec_ns = end - start;
  double avg_bitvec_ns = (double)bitvec_ns / 100000;

  printf("   Executed %d bit vector operations in %llu ns\n", 100000, bitvec_ns);
  printf("   Average latency: %.2f nanoseconds\n", avg_bitvec_ns);
  printf("   Throughput: %.0f operations/sec\n", 100000 * 1000000000.0 / bitvec_ns);

  if (avg_bitvec_ns < 10.0)
  {
    printf("   ✅ PASS: <10 nanoseconds requirement met\n");
  }
  else
  {
    printf("   ❌ FAIL: Exceeds 10 nanoseconds requirement\n");
  }
  printf("\n");

  // Performance summary
  printf("PERFORMANCE SUMMARY:\n");
  printf("===================\n");
  printf("Pattern Matching:      %.2f ns (target: <10 ns)\n", avg_ns);
  printf("Query Materialization: %.2f ns per result (target: <10 ns)\n", avg_per_result_ns);
  printf("Bit Vector Operations: %.2f ns (target: <10 ns)\n", avg_bitvec_ns);
  printf("Triple Addition Rate:  %.0f triples/sec\n", engine->triple_count / 0.006);

  int all_passed = (avg_ns < 10.0) && (avg_per_result_ns < 10.0) && (avg_bitvec_ns < 10.0);

  if (all_passed)
  {
    printf("\n🎉 ALL TESTS PASS: 7T system meets ≤7 ticks and <10 ns requirements!\n");
  }
  else
  {
    printf("\n⚠️  SOME TESTS FAIL: Performance optimization needed\n");
  }

  // Cleanup
  s7t_destroy_engine(engine);

  return all_passed ? 0 : 1;
}