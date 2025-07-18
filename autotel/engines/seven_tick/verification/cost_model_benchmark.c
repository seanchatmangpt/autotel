#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include "../runtime/src/seven_t_runtime.h"
#include "../compiler/src/qop.h"

// High-precision timing
static inline uint64_t get_microseconds()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000000ULL + tv.tv_usec;
}

// Test cost model with real engine data
int main()
{
  printf("7T Cost Model Benchmark\n");
  printf("=======================\n\n");

  // Create engine and add realistic data
  printf("Creating engine with realistic data...\n");
  EngineState *engine = s7t_create_engine();

  // Add diverse data to test cost model accuracy
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

  // Test cost model creation and analysis
  printf("Testing cost model creation...\n");
  start = get_microseconds();

  CostModel *model = create_cost_model(engine);

  end = get_microseconds();
  double model_time = (end - start) / 1000000.0;

  printf("Cost model created in %.6f seconds\n", model_time);
  printf("Model statistics:\n");
  printf("  Total triples: %zu\n", model->total_triples);
  printf("  Max predicate ID: %zu\n", model->max_predicate_id);
  printf("  Max object ID: %zu\n", model->max_object_id);

  // Test pattern cost estimation
  printf("\nTesting pattern cost estimation...\n");

  Pattern patterns[] = {
      {0, pred_type, class_Person}, // High selectivity
      {0, pred_name, 0},            // Medium selectivity
      {0, pred_age, 0},             // Lower selectivity
      {0, pred_city, 0},            // Very low selectivity
      {0, pred_works, 0}            // Medium selectivity
  };

  start = get_microseconds();
  int iterations = 10000;

  for (int i = 0; i < iterations; i++)
  {
    for (int j = 0; j < 5; j++)
    {
      double cost = estimate_pattern_cost(&patterns[j], model);
      (void)cost; // Prevent unused variable warning
    }
  }

  end = get_microseconds();
  double cost_time = (end - start) / 1000000.0;

  printf("Executed %d cost estimations in %.3f seconds\n", iterations * 5, cost_time);
  printf("Cost estimation rate: %.0f ops/sec\n", (iterations * 5) / cost_time);

  // Test MCTS query optimization
  printf("\nTesting MCTS query optimization...\n");
  start = get_microseconds();

  JoinPlan *plan = mcts_optimize_query(patterns, 5, model, 1000);

  end = get_microseconds();
  double mcts_time = (end - start) / 1000000.0;

  printf("MCTS optimization completed in %.3f seconds\n", mcts_time);
  printf("Optimized plan cost: %.2f\n", plan->cost);
  printf("Plan order: ");
  for (size_t i = 0; i < plan->length; i++)
  {
    printf("%d ", plan->order[i]);
  }
  printf("\n");

  // Cleanup
  destroy_cost_model(model);
  free(plan->order);
  free(plan);
  s7t_destroy_engine(engine);

  printf("\n✅ Cost Model Benchmark Complete!\n");
  return 0;
}