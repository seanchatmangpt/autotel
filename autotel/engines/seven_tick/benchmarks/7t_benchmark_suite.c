#include "7t_benchmark_framework.h"
#include "../c_src/7t_tpot.h"
#include "../c_src/telemetry7t.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Test data structures
typedef struct
{
  uint32_t *subjects;
  uint32_t *predicates;
  uint32_t *objects;
  size_t count;
} TestData;

typedef struct
{
  TPOTEngine *engine;
  TestData *data;
  size_t current_index;
} BenchmarkContext;

// Test data generation
TestData *generate_test_data(size_t count)
{
  TestData *data = malloc(sizeof(TestData));
  if (!data)
    return NULL;

  data->subjects = malloc(count * sizeof(uint32_t));
  data->predicates = malloc(count * sizeof(uint32_t));
  data->objects = malloc(count * sizeof(uint32_t));

  if (!data->subjects || !data->predicates || !data->objects)
  {
    free(data->subjects);
    free(data->predicates);
    free(data->objects);
    free(data);
    return NULL;
  }

  data->count = count;

  // Generate realistic test data
  for (size_t i = 0; i < count; i++)
  {
    data->subjects[i] = i % 100000; // 100K unique subjects
    data->predicates[i] = i % 1000; // 1K unique predicates
    data->objects[i] = i % 100000;  // 100K unique objects
  }

  return data;
}

void destroy_test_data(TestData *data)
{
  if (data)
  {
    free(data->subjects);
    free(data->predicates);
    free(data->objects);
    free(data);
  }
}

// Benchmark test functions

// Test 1: Basic TPOT operation latency
void test_basic_tpot_operation(void *context)
{
  BenchmarkContext *ctx = (BenchmarkContext *)context;
  size_t index = ctx->current_index % ctx->data->count;

  uint32_t s = ctx->data->subjects[index];
  uint32_t p = ctx->data->predicates[index];
  uint32_t o = ctx->data->objects[index];

  tpot_add_triple(ctx->engine, s, p, o);
  tpot_ask_pattern(ctx->engine, s, p, o);

  ctx->current_index++;
}

// Test 2: TPOT batch operations
void test_tpot_batch_operations(void *context, size_t batch_size)
{
  BenchmarkContext *ctx = (BenchmarkContext *)context;

  for (size_t i = 0; i < batch_size; i++)
  {
    size_t index = ctx->current_index % ctx->data->count;

    uint32_t s = ctx->data->subjects[index];
    uint32_t p = ctx->data->predicates[index];
    uint32_t o = ctx->data->objects[index];

    tpot_add_triple(ctx->engine, s, p, o);
    tpot_ask_pattern(ctx->engine, s, p, o);

    ctx->current_index++;
  }
}

// Test 3: Telemetry overhead measurement
void test_telemetry_overhead(void *context)
{
  BenchmarkContext *ctx = (BenchmarkContext *)context;
  size_t index = ctx->current_index % ctx->data->count;

  uint32_t s = ctx->data->subjects[index];
  uint32_t p = ctx->data->predicates[index];
  uint32_t o = ctx->data->objects[index];

  // Start telemetry span
  TelemetrySpan *span = telemetry7t_start_span("tpot_operation");

  tpot_add_triple(ctx->engine, s, p, o);
  int result = tpot_ask_pattern(ctx->engine, s, p, o);

  // Add telemetry attributes
  telemetry7t_add_attribute(span, "subject", s);
  telemetry7t_add_attribute(span, "predicate", p);
  telemetry7t_add_attribute(span, "object", o);
  telemetry7t_add_attribute(span, "result", result);

  // End telemetry span
  telemetry7t_end_span(span);

  ctx->current_index++;
}

// Test 4: Memory efficiency
void test_memory_efficiency(void *context)
{
  BenchmarkContext *ctx = (BenchmarkContext *)context;
  size_t index = ctx->current_index % ctx->data->count;

  uint32_t s = ctx->data->subjects[index];
  uint32_t p = ctx->data->predicates[index];
  uint32_t o = ctx->data->objects[index];

  // Measure memory before
  size_t memory_before = tpot_get_memory_usage(ctx->engine);

  tpot_add_triple(ctx->engine, s, p, o);

  // Measure memory after
  size_t memory_after = tpot_get_memory_usage(ctx->engine);

  // Validate memory efficiency
  if (memory_after - memory_before > 1000)
  { // More than 1KB per triple
    printf("Warning: High memory usage detected\n");
  }

  ctx->current_index++;
}

// Test 5: Cache performance
void test_cache_performance(void *context)
{
  BenchmarkContext *ctx = (BenchmarkContext *)context;

  // Access patterns designed to test cache performance
  for (size_t i = 0; i < 100; i++)
  {
    size_t index = ctx->current_index % ctx->data->count;

    uint32_t s = ctx->data->subjects[index];
    uint32_t p = ctx->data->predicates[index];
    uint32_t o = ctx->data->objects[index];

    // Sequential access pattern
    tpot_ask_pattern(ctx->engine, s, p, o);
    tpot_ask_pattern(ctx->engine, s + 1, p, o);
    tpot_ask_pattern(ctx->engine, s + 2, p, o);

    ctx->current_index++;
  }
}

// Test 6: Branch prediction performance
void test_branch_prediction(void *context)
{
  BenchmarkContext *ctx = (BenchmarkContext *)context;
  size_t index = ctx->current_index % ctx->data->count;

  uint32_t s = ctx->data->subjects[index];
  uint32_t p = ctx->data->predicates[index];
  uint32_t o = ctx->data->objects[index];

  // Test predictable branch patterns
  for (int i = 0; i < 10; i++)
  {
    if (i < 8)
    { // Predictable branch (80% taken)
      tpot_ask_pattern(ctx->engine, s, p, o);
    }
    else
    {
      tpot_ask_pattern(ctx->engine, 999999, p, o); // Not found
    }
  }

  ctx->current_index++;
}

// Test 7: SIMD optimization performance
void test_simd_performance(void *context)
{
  BenchmarkContext *ctx = (BenchmarkContext *)context;

  // Test SIMD-optimized batch operations
  uint32_t subjects[8], predicates[8], objects[8];

  for (size_t i = 0; i < 8; i++)
  {
    size_t index = ctx->current_index % ctx->data->count;
    subjects[i] = ctx->data->subjects[index];
    predicates[i] = ctx->data->predicates[index];
    objects[i] = ctx->data->objects[index];
    ctx->current_index++;
  }

  // Batch add triples
  for (size_t i = 0; i < 8; i++)
  {
    tpot_add_triple(ctx->engine, subjects[i], predicates[i], objects[i]);
  }

  // Batch query triples
  for (size_t i = 0; i < 8; i++)
  {
    tpot_ask_pattern(ctx->engine, subjects[i], predicates[i], objects[i]);
  }
}

// Test 8: Stress test
void test_stress_performance(void *context)
{
  BenchmarkContext *ctx = (BenchmarkContext *)context;

  // High-frequency operations
  for (size_t i = 0; i < 1000; i++)
  {
    size_t index = ctx->current_index % ctx->data->count;

    uint32_t s = ctx->data->subjects[index];
    uint32_t p = ctx->data->predicates[index];
    uint32_t o = ctx->data->objects[index];

    tpot_add_triple(ctx->engine, s, p, o);
    tpot_ask_pattern(ctx->engine, s, p, o);

    ctx->current_index++;
  }
}

// Main benchmark suite
int main()
{
  printf("=== 7T Engine Benchmark Suite ===\n");
  printf("Framework Version: %s\n", SEVEN_TICK_BENCHMARK_VERSION);
  printf("Target: ≤%d CPU cycles per operation\n", SEVEN_TICK_TARGET_CYCLES);
  printf("Target: ≤%d nanoseconds per operation\n", SEVEN_TICK_TARGET_NS);
  printf("Target: ≥%.0f operations per second\n", SEVEN_TICK_TARGET_OPS_PER_SEC / 1000000.0);

  // Detect hardware capabilities
  HardwareInfo *hw_info = detect_hardware_capabilities();
  if (hw_info)
  {
    print_hardware_info(hw_info);
  }

  // Create benchmark suite
  BenchmarkSuite *suite = benchmark_suite_create("7T Engine Performance Suite");
  if (!suite)
  {
    fprintf(stderr, "Failed to create benchmark suite\n");
    return 1;
  }

  // Initialize test data
  TestData *test_data = generate_test_data(1000000); // 1M test triples
  if (!test_data)
  {
    fprintf(stderr, "Failed to generate test data\n");
    benchmark_suite_destroy(suite);
    return 1;
  }

  // Initialize TPOT engine
  TPOTEngine *engine = tpot_create(1000000, 10000, 1000000); // 1M subjects, 10K predicates, 1M objects
  if (!engine)
  {
    fprintf(stderr, "Failed to create TPOT engine\n");
    destroy_test_data(test_data);
    benchmark_suite_destroy(suite);
    return 1;
  }

  // Initialize benchmark context
  BenchmarkContext context = {
      .engine = engine,
      .data = test_data,
      .current_index = 0};

  // Initialize telemetry
  telemetry7t_init();

  // Initialize memory tracker
  MemoryTracker *memory_tracker = memory_tracker_create(4ULL * 1024 * 1024 * 1024); // 4GB limit

  printf("\n=== Running Benchmark Tests ===\n");

  // Test 1: Basic TPOT operation latency
  printf("\nRunning Test 1: Basic TPOT Operation Latency...\n");
  BenchmarkResult result1 = benchmark_execute_single(
      "Basic TPOT Operation",
      100000, // 100K operations
      test_basic_tpot_operation,
      &context);
  benchmark_suite_add_result(suite, result1);
  memory_tracker_update(memory_tracker);

  // Test 2: TPOT batch operations
  printf("\nRunning Test 2: TPOT Batch Operations...\n");
  BenchmarkResult result2 = benchmark_execute_batch(
      "TPOT Batch Operations",
      10000, // 10K batches
      test_tpot_batch_operations,
      &context,
      100 // 100 operations per batch
  );
  benchmark_suite_add_result(suite, result2);
  memory_tracker_update(memory_tracker);

  // Test 3: Telemetry overhead
  printf("\nRunning Test 3: Telemetry Overhead...\n");
  BenchmarkResult result3 = benchmark_execute_single(
      "Telemetry Overhead",
      50000, // 50K operations
      test_telemetry_overhead,
      &context);
  benchmark_suite_add_result(suite, result3);
  memory_tracker_update(memory_tracker);

  // Test 4: Memory efficiency
  printf("\nRunning Test 4: Memory Efficiency...\n");
  BenchmarkResult result4 = benchmark_execute_single(
      "Memory Efficiency",
      100000, // 100K operations
      test_memory_efficiency,
      &context);
  benchmark_suite_add_result(suite, result4);
  memory_tracker_update(memory_tracker);

  // Test 5: Cache performance
  printf("\nRunning Test 5: Cache Performance...\n");
  BenchmarkResult result5 = benchmark_execute_single(
      "Cache Performance",
      50000, // 50K operations
      test_cache_performance,
      &context);
  benchmark_suite_add_result(suite, result5);
  memory_tracker_update(memory_tracker);

  // Test 6: Branch prediction
  printf("\nRunning Test 6: Branch Prediction...\n");
  BenchmarkResult result6 = benchmark_execute_single(
      "Branch Prediction",
      100000, // 100K operations
      test_branch_prediction,
      &context);
  benchmark_suite_add_result(suite, result6);
  memory_tracker_update(memory_tracker);

  // Test 7: SIMD performance
  printf("\nRunning Test 7: SIMD Performance...\n");
  BenchmarkResult result7 = benchmark_execute_single(
      "SIMD Performance",
      50000, // 50K operations
      test_simd_performance,
      &context);
  benchmark_suite_add_result(suite, result7);
  memory_tracker_update(memory_tracker);

  // Test 8: Stress test
  printf("\nRunning Test 8: Stress Test...\n");
  BenchmarkResult result8 = benchmark_execute_single(
      "Stress Test",
      10000, // 10K operations
      test_stress_performance,
      &context);
  benchmark_suite_add_result(suite, result8);
  memory_tracker_update(memory_tracker);

  // Calculate suite statistics
  benchmark_suite_calculate_stats(suite);

  // Print detailed results
  benchmark_suite_print_detailed(suite);

  // Print memory statistics
  printf("\n=== Memory Statistics ===\n");
  printf("Peak memory usage: %.2f MB\n", memory_tracker_get_peak(memory_tracker) / (1024.0 * 1024.0));
  printf("Final memory usage: %.2f MB\n", memory_tracker_get_current(memory_tracker) / (1024.0 * 1024.0));
  printf("TPOT engine memory: %.2f MB\n", tpot_get_memory_usage(engine) / (1024.0 * 1024.0));

  // Print telemetry statistics
  printf("\n=== Telemetry Statistics ===\n");
  telemetry7t_print_stats();

  // Cleanup
  telemetry7t_shutdown();
  memory_tracker_destroy(memory_tracker);
  tpot_destroy(engine);
  destroy_test_data(test_data);
  benchmark_suite_destroy(suite);
  destroy_hardware_info(hw_info);

  printf("\n=== Benchmark Suite Complete ===\n");

  return 0;
}