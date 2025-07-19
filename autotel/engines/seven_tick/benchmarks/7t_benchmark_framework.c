#include "7t_benchmark_framework.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/sysctl.h>
#if defined(__x86_64__) || defined(__i386__)
#include <cpuid.h>
#endif

// Cycle distribution implementation
CycleDistribution *cycle_distribution_create(size_t capacity)
{
  CycleDistribution *dist = malloc(sizeof(CycleDistribution));
  if (!dist)
    return NULL;

  dist->cycle_counts = malloc(capacity * sizeof(uint64_t));
  if (!dist->cycle_counts)
  {
    free(dist);
    return NULL;
  }

  dist->capacity = capacity;
  dist->count = 0;
  dist->total_cycles = 0;
  dist->min_cycles = UINT64_MAX;
  dist->max_cycles = 0;

  return dist;
}

void cycle_distribution_add(CycleDistribution *dist, uint64_t cycles)
{
  if (dist->count < dist->capacity)
  {
    dist->cycle_counts[dist->count++] = cycles;
    dist->total_cycles += cycles;

    if (cycles < dist->min_cycles)
      dist->min_cycles = cycles;
    if (cycles > dist->max_cycles)
      dist->max_cycles = cycles;
  }
}

// Simple percentile calculation (for production, use a more sophisticated algorithm)
double cycle_distribution_percentile(CycleDistribution *dist, double percentile)
{
  if (dist->count == 0)
    return 0.0;

  // Sort the cycle counts (simple bubble sort for small datasets)
  for (size_t i = 0; i < dist->count - 1; i++)
  {
    for (size_t j = 0; j < dist->count - i - 1; j++)
    {
      if (dist->cycle_counts[j] > dist->cycle_counts[j + 1])
      {
        uint64_t temp = dist->cycle_counts[j];
        dist->cycle_counts[j] = dist->cycle_counts[j + 1];
        dist->cycle_counts[j + 1] = temp;
      }
    }
  }

  size_t index = (size_t)(percentile * dist->count / 100.0);
  if (index >= dist->count)
    index = dist->count - 1;

  return (double)dist->cycle_counts[index];
}

void cycle_distribution_destroy(CycleDistribution *dist)
{
  if (dist)
  {
    free(dist->cycle_counts);
    free(dist);
  }
}

// Benchmark suite implementation
BenchmarkSuite *benchmark_suite_create(const char *suite_name)
{
  BenchmarkSuite *suite = malloc(sizeof(BenchmarkSuite));
  if (!suite)
    return NULL;

  suite->suite_name = suite_name;
  suite->result_capacity = 100;
  suite->results = malloc(suite->result_capacity * sizeof(BenchmarkResult));
  if (!suite->results)
  {
    free(suite);
    return NULL;
  }

  suite->result_count = 0;
  suite->total_suite_time_ns = 0;
  suite->overall_score = 0.0;

  return suite;
}

void benchmark_suite_add_result(BenchmarkSuite *suite, BenchmarkResult result)
{
  if (suite->result_count >= suite->result_capacity)
  {
    size_t new_capacity = suite->result_capacity * 2;
    BenchmarkResult *new_results = realloc(suite->results, new_capacity * sizeof(BenchmarkResult));
    if (!new_results)
      return;

    suite->results = new_results;
    suite->result_capacity = new_capacity;
  }

  suite->results[suite->result_count++] = result;
  suite->total_suite_time_ns += result.total_time_ns;
}

void benchmark_suite_calculate_stats(BenchmarkSuite *suite)
{
  if (suite->result_count == 0)
    return;

  double total_score = 0.0;
  for (size_t i = 0; i < suite->result_count; i++)
  {
    total_score += suite->results[i].target_achievement_percent;
  }

  suite->overall_score = total_score / suite->result_count;
}

void benchmark_suite_destroy(BenchmarkSuite *suite)
{
  if (suite)
  {
    free(suite->results);
    free(suite);
  }
}

// Individual benchmark execution
BenchmarkResult benchmark_execute_single(
    const char *test_name,
    size_t iterations,
    void (*test_function)(void *),
    void *test_data)
{
  BenchmarkResult result = {0};
  result.test_name = test_name;
  result.operations = iterations;
  result.passed = 1;

  CycleDistribution *dist = cycle_distribution_create(iterations);
  if (!dist)
  {
    result.passed = 0;
    return result;
  }

  BenchmarkTimer timer;
  benchmark_timer_start(&timer, test_name);

  // Warm-up run
  for (size_t i = 0; i < iterations / 10; i++)
  {
    test_function(test_data);
  }

  // Actual benchmark runs
  for (size_t i = 0; i < iterations; i++)
  {
    uint64_t start_cycles = get_cycles();
    test_function(test_data);
    uint64_t end_cycles = get_cycles();

    uint64_t cycles = end_cycles - start_cycles;
    cycle_distribution_add(dist, cycles);

    if (cycles <= SEVEN_TICK_TARGET_CYCLES)
    {
      result.operations_within_target++;
    }
  }

  benchmark_timer_end(&timer);

  // Calculate statistics
  result.total_cycles = benchmark_timer_get_cycles(&timer);
  result.total_time_ns = benchmark_timer_get_time_ns(&timer);
  result.avg_cycles_per_op = (double)result.total_cycles / iterations;
  result.avg_time_ns_per_op = (double)result.total_time_ns / iterations;
  result.ops_per_sec = iterations / (result.total_time_ns / 1000000000.0);

  result.min_cycles = dist->min_cycles;
  result.max_cycles = dist->max_cycles;
  result.p50_cycles = cycle_distribution_percentile(dist, 50.0);
  result.p95_cycles = cycle_distribution_percentile(dist, 95.0);
  result.p99_cycles = cycle_distribution_percentile(dist, 99.0);
  result.p99_9_cycles = cycle_distribution_percentile(dist, 99.9);

  result.target_achievement_percent = (double)result.operations_within_target / iterations * 100.0;

  cycle_distribution_destroy(dist);
  return result;
}

// Batch benchmark execution
BenchmarkResult benchmark_execute_batch(
    const char *test_name,
    size_t iterations,
    void (*test_function)(void *, size_t),
    void *test_data,
    size_t batch_size)
{
  BenchmarkResult result = {0};
  result.test_name = test_name;
  result.operations = iterations;
  result.passed = 1;

  CycleDistribution *dist = cycle_distribution_create(iterations);
  if (!dist)
  {
    result.passed = 0;
    return result;
  }

  BenchmarkTimer timer;
  benchmark_timer_start(&timer, test_name);

  // Warm-up run
  test_function(test_data, batch_size);

  // Actual benchmark runs
  for (size_t i = 0; i < iterations; i++)
  {
    uint64_t start_cycles = get_cycles();
    test_function(test_data, batch_size);
    uint64_t end_cycles = get_cycles();

    uint64_t cycles = end_cycles - start_cycles;
    cycle_distribution_add(dist, cycles);

    if (cycles <= SEVEN_TICK_TARGET_CYCLES * batch_size)
    {
      result.operations_within_target++;
    }
  }

  benchmark_timer_end(&timer);

  // Calculate statistics
  result.total_cycles = benchmark_timer_get_cycles(&timer);
  result.total_time_ns = benchmark_timer_get_time_ns(&timer);
  result.avg_cycles_per_op = (double)result.total_cycles / iterations;
  result.avg_time_ns_per_op = (double)result.total_time_ns / iterations;
  result.ops_per_sec = iterations / (result.total_time_ns / 1000000000.0);

  result.min_cycles = dist->min_cycles;
  result.max_cycles = dist->max_cycles;
  result.p50_cycles = cycle_distribution_percentile(dist, 50.0);
  result.p95_cycles = cycle_distribution_percentile(dist, 95.0);
  result.p99_cycles = cycle_distribution_percentile(dist, 99.0);
  result.p99_9_cycles = cycle_distribution_percentile(dist, 99.9);

  result.target_achievement_percent = (double)result.operations_within_target / iterations * 100.0;

  cycle_distribution_destroy(dist);
  return result;
}

// Performance validation
int benchmark_validate_target(BenchmarkResult *result)
{
  return result->target_achievement_percent >= 95.0 &&
         result->avg_cycles_per_op <= SEVEN_TICK_TARGET_CYCLES &&
         result->avg_time_ns_per_op <= SEVEN_TICK_TARGET_NS;
}

// Result formatting and reporting
void benchmark_result_print(BenchmarkResult *result)
{
  printf("\n=== Benchmark Result: %s ===\n", result->test_name);
  printf("Operations: %zu\n", result->operations);
  printf("Total time: %.3f ms (%.0f ns)\n", result->total_time_ns / 1000000.0, (double)result->total_time_ns);
  printf("Total cycles: %llu\n", result->total_cycles);
  printf("Average cycles per operation: %.2f\n", result->avg_cycles_per_op);
  printf("Average time per operation: %.2f ns\n", result->avg_time_ns_per_op);
  printf("Throughput: %.0f ops/sec\n", result->ops_per_sec);
  printf("\nCycle Distribution:\n");
  printf("  Min: %llu cycles\n", result->min_cycles);
  printf("  P50: %.1f cycles\n", result->p50_cycles);
  printf("  P95: %.1f cycles\n", result->p95_cycles);
  printf("  P99: %.1f cycles\n", result->p99_cycles);
  printf("  P99.9: %.1f cycles\n", result->p99_9_cycles);
  printf("  Max: %llu cycles\n", result->max_cycles);
  printf("\nTarget Achievement:\n");
  printf("  Operations within ≤%d cycles: %zu/%zu (%.1f%%)\n",
         SEVEN_TICK_TARGET_CYCLES, result->operations_within_target, result->operations,
         result->target_achievement_percent);

  if (benchmark_validate_target(result))
  {
    printf("  Status: ✅ PASSED\n");
  }
  else
  {
    printf("  Status: ❌ FAILED\n");
  }
}

void benchmark_suite_print_summary(BenchmarkSuite *suite)
{
  printf("\n=== Benchmark Suite Summary: %s ===\n", suite->suite_name);
  printf("Total tests: %zu\n", suite->result_count);
  printf("Total suite time: %.3f ms\n", suite->total_suite_time_ns / 1000000.0);
  printf("Overall score: %.1f%%\n", suite->overall_score);

  int passed_tests = 0;
  for (size_t i = 0; i < suite->result_count; i++)
  {
    if (suite->results[i].passed)
      passed_tests++;
  }

  printf("Passed tests: %d/%zu\n", passed_tests, suite->result_count);

  if (suite->overall_score >= 95.0)
  {
    printf("Suite Status: ✅ PASSED\n");
  }
  else
  {
    printf("Suite Status: ❌ FAILED\n");
  }
}

void benchmark_suite_print_detailed(BenchmarkSuite *suite)
{
  printf("\n=== Detailed Benchmark Suite Results: %s ===\n", suite->suite_name);

  for (size_t i = 0; i < suite->result_count; i++)
  {
    benchmark_result_print(&suite->results[i]);
  }

  benchmark_suite_print_summary(suite);
}

// Hardware detection
double detect_cpu_frequency(void)
{
  // This is a simplified implementation
  // In production, use more sophisticated CPU frequency detection
  return 3.0; // Assume 3GHz as default
}

size_t detect_cache_line_size(void)
{
  // This is a simplified implementation
  // In production, use CPUID or sysfs to get actual cache line size
  return 64; // Assume 64 bytes as default
}

HardwareInfo *detect_hardware_capabilities(void)
{
  HardwareInfo *info = malloc(sizeof(HardwareInfo));
  if (!info)
    return NULL;

  // Simplified hardware detection
  info->avx2_support = 0;
  info->avx512_support = 0;
  info->sse4_2_support = 0;
  info->l1_cache_size = 32 * 1024;       // Assume 32KB
  info->l2_cache_size = 256 * 1024;      // Assume 256KB
  info->l3_cache_size = 8 * 1024 * 1024; // Assume 8MB
  info->num_cores = 8;                   // Assume 8 cores
  info->cpu_frequency_ghz = detect_cpu_frequency();

  return info;
}

void print_hardware_info(HardwareInfo *info)
{
  printf("\n=== Hardware Information ===\n");
  printf("CPU Frequency: %.1f GHz\n", info->cpu_frequency_ghz);
  printf("Number of Cores: %d\n", info->num_cores);
  printf("L1 Cache Size: %zu KB\n", info->l1_cache_size / 1024);
  printf("L2 Cache Size: %zu KB\n", info->l2_cache_size / 1024);
  printf("L3 Cache Size: %zu MB\n", info->l3_cache_size / (1024 * 1024));
  printf("AVX2 Support: %s\n", info->avx2_support ? "Yes" : "No");
  printf("AVX-512 Support: %s\n", info->avx512_support ? "Yes" : "No");
  printf("SSE4.2 Support: %s\n", info->sse4_2_support ? "Yes" : "No");
}

void destroy_hardware_info(HardwareInfo *info)
{
  free(info);
}

// Memory tracking
MemoryTracker *memory_tracker_create(size_t memory_limit)
{
  MemoryTracker *tracker = malloc(sizeof(MemoryTracker));
  if (!tracker)
    return NULL;

  tracker->memory_limit = memory_limit;
  tracker->initial_memory = 0;
  tracker->peak_memory = 0;
  tracker->final_memory = 0;

  return tracker;
}

void memory_tracker_update(MemoryTracker *tracker)
{
  // Simplified memory tracking
  // In production, use getrusage() or /proc/self/status
  size_t current_memory = 0; // Placeholder

  if (tracker->initial_memory == 0)
  {
    tracker->initial_memory = current_memory;
  }

  if (current_memory > tracker->peak_memory)
  {
    tracker->peak_memory = current_memory;
  }

  tracker->final_memory = current_memory;
}

size_t memory_tracker_get_peak(MemoryTracker *tracker)
{
  return tracker->peak_memory;
}

size_t memory_tracker_get_current(MemoryTracker *tracker)
{
  return tracker->final_memory;
}

void memory_tracker_destroy(MemoryTracker *tracker)
{
  free(tracker);
}

// Regression detection
RegressionDetector *regression_detector_create(BenchmarkResult *baseline, double threshold_percent)
{
  RegressionDetector *detector = malloc(sizeof(RegressionDetector));
  if (!detector)
    return NULL;

  detector->baseline = baseline;
  detector->threshold_percent = threshold_percent;
  detector->current = NULL;
  detector->regression_detected = 0;
  detector->performance_change_percent = 0.0;

  return detector;
}

int regression_detector_check(RegressionDetector *detector, BenchmarkResult *current)
{
  detector->current = current;

  double baseline_avg = detector->baseline->avg_cycles_per_op;
  double current_avg = current->avg_cycles_per_op;

  detector->performance_change_percent = ((current_avg - baseline_avg) / baseline_avg) * 100.0;

  detector->regression_detected = detector->performance_change_percent > detector->threshold_percent;

  return detector->regression_detected;
}

void regression_detector_destroy(RegressionDetector *detector)
{
  free(detector);
}

// Continuous monitoring
ContinuousMonitor *continuous_monitor_create(size_t max_samples, size_t sample_interval)
{
  ContinuousMonitor *monitor = malloc(sizeof(ContinuousMonitor));
  if (!monitor)
    return NULL;

  monitor->samples = malloc(max_samples * sizeof(BenchmarkResult));
  if (!monitor->samples)
  {
    free(monitor);
    return NULL;
  }

  monitor->max_samples = max_samples;
  monitor->sample_interval = sample_interval;
  monitor->sample_count = 0;
  monitor->trend_slope = 0.0;

  return monitor;
}

void continuous_monitor_add_sample(ContinuousMonitor *monitor, BenchmarkResult *result)
{
  if (monitor->sample_count < monitor->max_samples)
  {
    monitor->samples[monitor->sample_count++] = *result;
  }
}

void continuous_monitor_calculate_trend(ContinuousMonitor *monitor)
{
  if (monitor->sample_count < 2)
    return;

  // Simple linear regression
  double sum_x = 0.0, sum_y = 0.0, sum_xy = 0.0, sum_x2 = 0.0;

  for (size_t i = 0; i < monitor->sample_count; i++)
  {
    double x = (double)i;
    double y = monitor->samples[i].avg_cycles_per_op;

    sum_x += x;
    sum_y += y;
    sum_xy += x * y;
    sum_x2 += x * x;
  }

  double n = (double)monitor->sample_count;
  monitor->trend_slope = (n * sum_xy - sum_x * sum_y) / (n * sum_x2 - sum_x * sum_x);
}

void continuous_monitor_destroy(ContinuousMonitor *monitor)
{
  if (monitor)
  {
    free(monitor->samples);
    free(monitor);
  }
}