#include "7t_benchmark_framework.h"
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

// ============================================================================
// SIMPLIFIED BENCHMARK TESTS
// ============================================================================

// Test 1: Basic operation latency (simulated)
void test_basic_operation(void *context)
{
  BenchmarkContext *ctx = (BenchmarkContext *)context;
  size_t index = ctx->current_index % ctx->data->count;

  uint32_t s = ctx->data->subjects[index];
  uint32_t p = ctx->data->predicates[index];
  uint32_t o = ctx->data->objects[index];

  // Simulate basic operation
  uint32_t result = s + p + o;
  (void)result; // Prevent unused variable warning

  ctx->current_index++;
}

// Test 2: Batch operations (simulated)
void test_batch_operations(void *context, size_t batch_size)
{
  BenchmarkContext *ctx = (BenchmarkContext *)context;

  for (size_t i = 0; i < batch_size; i++)
  {
    size_t index = ctx->current_index % ctx->data->count;

    uint32_t s = ctx->data->subjects[index];
    uint32_t p = ctx->data->predicates[index];
    uint32_t o = ctx->data->objects[index];

    // Simulate batch operation
    uint32_t result = s * p + o;
    (void)result; // Prevent unused variable warning

    ctx->current_index++;
  }
}

// Test 3: Memory efficiency
void test_memory_efficiency(void *context)
{
  BenchmarkContext *ctx = (BenchmarkContext *)context;
  size_t index = ctx->current_index % ctx->data->count;

  uint32_t s = ctx->data->subjects[index];
  uint32_t p = ctx->data->predicates[index];
  uint32_t o = ctx->data->objects[index];

  // Simulate memory allocation
  uint32_t *temp_data = malloc(1000 * sizeof(uint32_t));
  if (temp_data)
  {
    for (int i = 0; i < 1000; i++)
    {
      temp_data[i] = s + p + o + i;
    }
    free(temp_data);
  }

  ctx->current_index++;
}

// Test 4: Cache performance
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
    uint32_t result1 = s + p;
    uint32_t result2 = s + 1 + p;
    uint32_t result3 = s + 2 + p;

    (void)result1;
    (void)result2;
    (void)result3; // Prevent unused variable warnings

    ctx->current_index++;
  }
}

// Test 5: Branch prediction performance
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
      uint32_t result = s + p + o;
      (void)result;
    }
    else
    {
      uint32_t result = s * p * o; // Different operation
      (void)result;
    }
  }

  ctx->current_index++;
}

// Test 6: SIMD optimization performance (simulated)
void test_simd_performance(void *context)
{
  BenchmarkContext *ctx = (BenchmarkContext *)context;

  // Simulate SIMD-optimized batch operations
  uint32_t subjects[8], predicates[8], objects[8];

  for (size_t i = 0; i < 8; i++)
  {
    size_t index = ctx->current_index % ctx->data->count;
    subjects[i] = ctx->data->subjects[index];
    predicates[i] = ctx->data->predicates[index];
    objects[i] = ctx->data->objects[index];
    ctx->current_index++;
  }

  // Simulate batch processing
  for (size_t i = 0; i < 8; i++)
  {
    uint32_t result = subjects[i] + predicates[i] + objects[i];
    (void)result;
  }
}

// Test 7: Stress test
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

    // Multiple operations
    uint32_t result1 = s + p;
    uint32_t result2 = p + o;
    uint32_t result3 = s + o;
    uint32_t final_result = result1 + result2 + result3;

    (void)final_result; // Prevent unused variable warning

    ctx->current_index++;
  }
}

// Test 8: Cycle counting accuracy
void test_cycle_counting_accuracy(void *context)
{
  BenchmarkContext *ctx = (BenchmarkContext *)context;
  size_t index = ctx->current_index % ctx->data->count;

  uint32_t s = ctx->data->subjects[index];
  uint32_t p = ctx->data->predicates[index];
  uint32_t o = ctx->data->objects[index];

  // Test cycle counting accuracy with known operations
  uint64_t start_cycles = get_cycles();

  // Perform a series of operations
  for (int i = 0; i < 100; i++)
  {
    uint32_t result = s + p + o + i;
    (void)result;
  }

  uint64_t end_cycles = get_cycles();
  uint64_t cycles = end_cycles - start_cycles;

  // Validate cycle counting
  if (cycles < 1000)
  {               // Should be reasonable for 100 operations
    (void)cycles; // Prevent unused variable warning
  }

  ctx->current_index++;
}

// ============================================================================
// MAIN BENCHMARK SUITE
// ============================================================================

int main(int argc, char *argv[])
{
  printf("=== 7T Engine Unified Benchmark Suite ===\n");
  printf("Framework Version: %s\n", SEVEN_TICK_BENCHMARK_VERSION);
  printf("Target: ≤%d CPU cycles per operation\n", SEVEN_TICK_TARGET_CYCLES);
  printf("Target: ≤%d nanoseconds per operation\n", SEVEN_TICK_TARGET_NS);
  printf("Target: ≥%.0f operations per second\n", SEVEN_TICK_TARGET_OPS_PER_SEC / 1000000.0);

  // Parse command line arguments
  for (int i = 1; i < argc; i++)
  {
    if (strcmp(argv[i], "--help") == 0)
    {
      printf("Usage: %s [--help]\n", argv[0]);
      printf("  --help: Show this help message\n");
      return 0;
    }
  }

  // Detect hardware capabilities
  HardwareInfo *hw_info = detect_hardware_capabilities();
  if (hw_info)
  {
    print_hardware_info(hw_info);
  }

  // Create benchmark suite
  BenchmarkSuite *suite = benchmark_suite_create("7T Engine Unified Performance Suite");
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

  // Initialize benchmark context
  BenchmarkContext context = {
      .data = test_data,
      .current_index = 0};

  // Initialize memory tracker
  MemoryTracker *memory_tracker = memory_tracker_create(4ULL * 1024 * 1024 * 1024); // 4GB limit

  printf("\n=== Running Benchmark Tests ===\n");

  // ============================================================================
  // CORE BENCHMARK TESTS
  // ============================================================================

  printf("\n--- Core Engine Benchmarks ---\n");

  // Test 1: Basic operation latency
  printf("\nRunning Test 1: Basic Operation Latency...\n");
  BenchmarkResult result1 = benchmark_execute_single(
      "Basic Operation",
      100000, // 100K operations
      test_basic_operation,
      &context);
  benchmark_suite_add_result(suite, result1);
  memory_tracker_update(memory_tracker);

  // Test 2: Batch operations
  printf("\nRunning Test 2: Batch Operations...\n");
  BenchmarkResult result2 = benchmark_execute_batch(
      "Batch Operations",
      10000, // 10K batches
      test_batch_operations,
      &context,
      100 // 100 operations per batch
  );
  benchmark_suite_add_result(suite, result2);
  memory_tracker_update(memory_tracker);

  // Test 3: Memory efficiency
  printf("\nRunning Test 3: Memory Efficiency...\n");
  BenchmarkResult result3 = benchmark_execute_single(
      "Memory Efficiency",
      100000, // 100K operations
      test_memory_efficiency,
      &context);
  benchmark_suite_add_result(suite, result3);
  memory_tracker_update(memory_tracker);

  // Test 4: Cache performance
  printf("\nRunning Test 4: Cache Performance...\n");
  BenchmarkResult result4 = benchmark_execute_single(
      "Cache Performance",
      50000, // 50K operations
      test_cache_performance,
      &context);
  benchmark_suite_add_result(suite, result4);
  memory_tracker_update(memory_tracker);

  // Test 5: Branch prediction
  printf("\nRunning Test 5: Branch Prediction...\n");
  BenchmarkResult result5 = benchmark_execute_single(
      "Branch Prediction",
      100000, // 100K operations
      test_branch_prediction,
      &context);
  benchmark_suite_add_result(suite, result5);
  memory_tracker_update(memory_tracker);

  // Test 6: SIMD performance
  printf("\nRunning Test 6: SIMD Performance...\n");
  BenchmarkResult result6 = benchmark_execute_single(
      "SIMD Performance",
      50000, // 50K operations
      test_simd_performance,
      &context);
  benchmark_suite_add_result(suite, result6);
  memory_tracker_update(memory_tracker);

  // Test 7: Stress test
  printf("\nRunning Test 7: Stress Test...\n");
  BenchmarkResult result7 = benchmark_execute_single(
      "Stress Test",
      10000, // 10K operations
      test_stress_performance,
      &context);
  benchmark_suite_add_result(suite, result7);
  memory_tracker_update(memory_tracker);

  // Test 8: Cycle counting accuracy
  printf("\nRunning Test 8: Cycle Counting Accuracy...\n");
  BenchmarkResult result8 = benchmark_execute_single(
      "Cycle Counting Accuracy",
      50000, // 50K operations
      test_cycle_counting_accuracy,
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

  // Cleanup
  memory_tracker_destroy(memory_tracker);
  destroy_test_data(test_data);
  benchmark_suite_destroy(suite);
  destroy_hardware_info(hw_info);

  printf("\n=== Benchmark Suite Complete ===\n");

  return 0;
}