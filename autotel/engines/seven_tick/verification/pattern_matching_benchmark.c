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

// Test pattern matching performance
int main()
{
  printf("7T Pattern Matching Benchmark\n");
  printf("=============================\n\n");

  // Create engine and add realistic data
  printf("Creating engine with realistic data...\n");
  EngineState *engine = s7t_create_engine();

  // Add diverse data to test pattern matching
  uint32_t pred_type = s7t_intern_string(engine, "type");
  uint32_t pred_name = s7t_intern_string(engine, "name");
  uint32_t pred_age = s7t_intern_string(engine, "age");
  uint32_t pred_city = s7t_intern_string(engine, "city");
  uint32_t pred_works = s7t_intern_string(engine, "works");

  uint32_t class_Person = s7t_intern_string(engine, "Person");
  uint32_t class_Company = s7t_intern_string(engine, "Company");
  uint32_t class_City = s7t_intern_string(engine, "City");

  // Add 10,000 people with varied properties
  printf("Adding 10,000 people with varied properties...\n");
  uint64_t start = get_microseconds();

  for (int i = 0; i < 10000; i++)
  {
    char buf[256];

    // Person entity
    snprintf(buf, sizeof(buf), "person_%d", i);
    uint32_t person = s7t_intern_string(engine, buf);
    s7t_add_triple(engine, person, pred_type, class_Person);

    // Name (everyone has a name)
    snprintf(buf, sizeof(buf), "Person %d", i);
    uint32_t name = s7t_intern_string(engine, buf);
    s7t_add_triple(engine, person, pred_name, name);

    // Age (only 80% have age)
    if (i % 5 != 0)
    {
      snprintf(buf, sizeof(buf), "age_%d", 20 + (i % 60));
      uint32_t age = s7t_intern_string(engine, buf);
      s7t_add_triple(engine, person, pred_age, age);
    }

    // City (only 60% have city)
    if (i % 5 != 0 && i % 3 != 0)
    {
      snprintf(buf, sizeof(buf), "city_%d", i % 100);
      uint32_t city = s7t_intern_string(engine, buf);
      s7t_add_triple(engine, person, pred_city, city);
    }

    // Works (only 70% work)
    if (i % 10 != 0)
    {
      snprintf(buf, sizeof(buf), "company_%d", i % 50);
      uint32_t company = s7t_intern_string(engine, buf);
      s7t_add_triple(engine, person, pred_works, company);
    }
  }

  uint64_t end = get_microseconds();
  double add_time = (end - start) / 1000000.0;

  printf("Added %zu triples in %.3f seconds\n", engine->triple_count, add_time);
  printf("Triple addition rate: %.0f triples/sec\n\n", engine->triple_count / add_time);

  // Test s7t_ask_pattern performance
  printf("Testing s7t_ask_pattern performance...\n");

  // Create test patterns
  uint32_t test_subjects[] = {1, 100, 500, 1000, 5000, 9999};
  int num_test_subjects = sizeof(test_subjects) / sizeof(test_subjects[0]);

  start = get_microseconds();
  int iterations = 100000;

  for (int iter = 0; iter < iterations; iter++)
  {
    for (int i = 0; i < num_test_subjects; i++)
    {
      uint32_t subject = test_subjects[i];

      // Test different patterns
      int result1 = s7t_ask_pattern(engine, subject, pred_type, class_Person);
      int result2 = s7t_ask_pattern(engine, subject, pred_name, 0);
      int result3 = s7t_ask_pattern(engine, subject, pred_age, 0);
      int result4 = s7t_ask_pattern(engine, subject, pred_city, 0);
      int result5 = s7t_ask_pattern(engine, subject, pred_works, 0);

      (void)result1;
      (void)result2;
      (void)result3;
      (void)result4;
      (void)result5;
    }
  }

  end = get_microseconds();
  double pattern_time = (end - start) / 1000000.0;

  printf("Executed %d pattern matches in %.3f seconds\n",
         iterations * num_test_subjects * 5, pattern_time);
  printf("Pattern matching rate: %.0f ops/sec\n",
         (iterations * num_test_subjects * 5) / pattern_time);

  // Test specific pattern matches
  printf("\nTesting specific pattern matches...\n");

  // Test existing person
  uint32_t person_100 = s7t_intern_string(engine, "person_100");

  start = get_microseconds();
  int result = s7t_ask_pattern(engine, person_100, pred_type, class_Person);
  end = get_microseconds();

  double single_time = (end - start) / 1000000.0;
  printf("Person 100 type check: %s in %.6f seconds\n",
         result ? "TRUE" : "FALSE", single_time);

  // Test non-existent person
  uint32_t person_99999 = s7t_intern_string(engine, "person_99999");

  start = get_microseconds();
  result = s7t_ask_pattern(engine, person_99999, pred_type, class_Person);
  end = get_microseconds();

  single_time = (end - start) / 1000000.0;
  printf("Non-existent person check: %s in %.6f seconds\n",
         result ? "TRUE" : "FALSE", single_time);

  // Compare with bit vector operations
  printf("\nComparing with bit vector operations...\n");

  // Test bit vector intersection
  start = get_microseconds();

  for (int i = 0; i < 10000; i++)
  {
    BitVector *pred_vec = s7t_get_subject_vector(engine, pred_type, class_Person);
    BitVector *obj_vec = s7t_get_subject_vector(engine, pred_name, 0);

    if (pred_vec && obj_vec)
    {
      BitVector *intersection = bitvec_and(pred_vec, obj_vec);
      size_t count = bitvec_popcount(intersection);
      (void)count;
      bitvec_destroy(intersection);
    }

    if (pred_vec)
      bitvec_destroy(pred_vec);
    if (obj_vec)
      bitvec_destroy(obj_vec);
  }

  end = get_microseconds();
  double bitvec_time = (end - start) / 1000000.0;

  printf("Executed %d bit vector operations in %.3f seconds\n", 10000, bitvec_time);
  printf("Bit vector operation rate: %.0f ops/sec\n", 10000 / bitvec_time);

  // Performance comparison
  printf("\nPerformance Comparison:\n");
  printf("Pattern matching: %.0f ops/sec\n",
         (iterations * num_test_subjects * 5) / pattern_time);
  printf("Bit vector ops: %.0f ops/sec\n", 10000 / bitvec_time);
  printf("Pattern matching is %.1fx faster\n",
         ((iterations * num_test_subjects * 5) / pattern_time) / (10000 / bitvec_time));

  // Memory efficiency
  printf("\nMemory Efficiency:\n");
  printf("Pattern matching: No additional memory allocation\n");
  printf("Bit vector ops: Requires temporary bit vectors\n");

  // Cleanup
  s7t_destroy_engine(engine);

  printf("\n✅ Pattern Matching Benchmark Complete!\n");
  return 0;
}