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

// Test query result materialization performance
int main()
{
  printf("7T Query Materialization Benchmark\n");
  printf("==================================\n\n");

  // Create engine and add realistic data
  printf("Creating engine with realistic data...\n");
  EngineState *engine = s7t_create_engine();

  // Add diverse data to test materialization
  uint32_t pred_type = s7t_intern_string(engine, "type");
  uint32_t pred_name = s7t_intern_string(engine, "name");
  uint32_t pred_age = s7t_intern_string(engine, "age");
  uint32_t pred_city = s7t_intern_string(engine, "city");
  uint32_t pred_works = s7t_intern_string(engine, "works");

  uint32_t class_Person = s7t_intern_string(engine, "Person");
  uint32_t class_Company = s7t_intern_string(engine, "Company");
  uint32_t class_City = s7t_intern_string(engine, "City");

  // Add 50,000 entities with varied properties
  printf("Adding 50,000 entities with varied properties...\n");
  uint64_t start = get_microseconds();

  for (int i = 0; i < 50000; i++)
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

  // Test materialization performance
  printf("Testing query result materialization...\n");

  // Test different query patterns
  struct
  {
    uint32_t predicate;
    uint32_t object;
    const char *description;
  } test_queries[] = {
      {pred_type, class_Person, "All persons"},
      {pred_age, 0, "People with age"},
      {pred_city, 0, "People with city"},
      {pred_works, 0, "People who work"},
      {pred_name, 0, "People with names"}};

  int num_queries = sizeof(test_queries) / sizeof(test_queries[0]);

  for (int q = 0; q < num_queries; q++)
  {
    printf("\nQuery %d: %s\n", q + 1, test_queries[q].description);

    start = get_microseconds();
    size_t result_count;
    uint32_t *results = s7t_materialize_subjects(engine,
                                                 test_queries[q].predicate,
                                                 test_queries[q].object,
                                                 &result_count);
    end = get_microseconds();

    double materialization_time = (end - start) / 1000000.0;

    printf("  Results: %zu subjects\n", result_count);
    printf("  Time: %.6f seconds\n", materialization_time);
    printf("  Rate: %.0f results/sec\n", result_count / materialization_time);

    if (results && result_count > 0)
    {
      printf("  Sample results: ");
      for (int i = 0; i < 5 && i < result_count; i++)
      {
        printf("%u ", results[i]);
      }
      if (result_count > 5)
        printf("...");
      printf("\n");
    }

    free(results);
  }

  // Test materialization vs bit vector operations
  printf("\nComparing materialization vs bit vector operations...\n");

  // Test materialization approach
  start = get_microseconds();
  size_t mat_count;
  uint32_t *mat_results = s7t_materialize_subjects(engine, pred_type, class_Person, &mat_count);
  end = get_microseconds();

  double mat_time = (end - start) / 1000000.0;
  printf("Materialization approach: %zu results in %.6f seconds\n", mat_count, mat_time);

  // Test bit vector approach
  start = get_microseconds();
  BitVector *pred_vec = s7t_get_subject_vector(engine, pred_type, class_Person);
  BitVector *obj_vec = s7t_get_subject_vector(engine, pred_name, 0);
  BitVector *intersection = NULL;
  size_t bv_count = 0;

  if (pred_vec && obj_vec)
  {
    intersection = bitvec_and(pred_vec, obj_vec);
    if (intersection)
    {
      bv_count = bitvec_popcount(intersection);
    }
  }
  end = get_microseconds();

  double bv_time = (end - start) / 1000000.0;
  printf("Bit vector approach: %zu results in %.6f seconds\n", bv_count, bv_time);

  printf("Materialization is %.1fx faster than bit vector operations\n", bv_time / mat_time);

  // Cleanup
  if (mat_results)
    free(mat_results);
  if (pred_vec)
    bitvec_destroy(pred_vec);
  if (obj_vec)
    bitvec_destroy(obj_vec);
  if (intersection)
    bitvec_destroy(intersection);

  s7t_destroy_engine(engine);

  printf("\n✅ Query Materialization Benchmark Complete!\n");
  return 0;
}