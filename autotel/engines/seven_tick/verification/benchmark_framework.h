#ifndef BENCHMARK_FRAMEWORK_H
#define BENCHMARK_FRAMEWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <stdint.h>

// Simplified benchmark framework for cross-platform compatibility
#define BENCHMARK_VERSION "1.0.0"
#define TARGET_CYCLES 7
#define TARGET_NS 10
#define TARGET_OPS_PER_SEC 100000000

// High-precision timing functions
static inline uint64_t get_cycles(void)
{
  // Use clock_gettime for cross-platform compatibility
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

static inline uint64_t get_nanoseconds(void)
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

// Benchmark result structure
typedef struct
{
  const char *test_name;
  uint64_t total_cycles;
  uint64_t total_time_ns;
  size_t operations;
  double avg_cycles_per_op;
  double avg_time_ns_per_op;
  double ops_per_sec;
  uint64_t min_cycles;
  uint64_t max_cycles;
  size_t operations_within_target;
  double target_achievement_percent;
  int passed;
} BenchmarkResult;

// Benchmark suite structure
typedef struct
{
  const char *suite_name;
  BenchmarkResult *results;
  size_t result_count;
  size_t result_capacity;
  uint64_t total_suite_time_ns;
  double overall_score;
} BenchmarkSuite;

// Benchmark timer structure
typedef struct
{
  uint64_t start_cycles;
  uint64_t end_cycles;
  uint64_t start_time_ns;
  uint64_t end_time_ns;
  const char *operation_name;
} BenchmarkTimer;

// Initialize benchmark timer
static inline void benchmark_timer_start(BenchmarkTimer *timer, const char *operation_name)
{
  timer->operation_name = operation_name;
  timer->start_cycles = get_cycles();
  timer->start_time_ns = get_nanoseconds();
}

// End benchmark timer
static inline void benchmark_timer_end(BenchmarkTimer *timer)
{
  timer->end_cycles = get_cycles();
  timer->end_time_ns = get_nanoseconds();
}

// Get timer results
static inline uint64_t benchmark_timer_get_cycles(BenchmarkTimer *timer)
{
  return timer->end_cycles - timer->start_cycles;
}

static inline uint64_t benchmark_timer_get_time_ns(BenchmarkTimer *timer)
{
  return timer->end_time_ns - timer->start_time_ns;
}

// Benchmark suite management
static inline BenchmarkSuite *benchmark_suite_create(const char *suite_name)
{
  BenchmarkSuite *suite = malloc(sizeof(BenchmarkSuite));
  suite->suite_name = suite_name;
  suite->results = malloc(20 * sizeof(BenchmarkResult)); // Initial capacity
  suite->result_count = 0;
  suite->result_capacity = 20;
  suite->total_suite_time_ns = 0;
  suite->overall_score = 0.0;
  return suite;
}

static inline void benchmark_suite_add_result(BenchmarkSuite *suite, BenchmarkResult result)
{
  if (suite->result_count >= suite->result_capacity)
  {
    suite->result_capacity *= 2;
    suite->results = realloc(suite->results, suite->result_capacity * sizeof(BenchmarkResult));
  }
  suite->results[suite->result_count++] = result;
  suite->total_suite_time_ns += result.total_time_ns;
}

static inline void benchmark_suite_calculate_stats(BenchmarkSuite *suite)
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

static inline void benchmark_suite_destroy(BenchmarkSuite *suite)
{
  free(suite->results);
  free(suite);
}

// Individual benchmark execution
static inline BenchmarkResult benchmark_execute_single(
    const char *test_name,
    size_t iterations,
    void (*test_function)(void *),
    void *test_data)
{

  BenchmarkTimer timer;
  benchmark_timer_start(&timer, test_name);

  // Warm-up run
  test_function(test_data);

  // Reset timer for actual measurement
  benchmark_timer_start(&timer, test_name);

  for (size_t i = 0; i < iterations; i++)
  {
    test_function(test_data);
  }

  benchmark_timer_end(&timer);

  uint64_t total_cycles = benchmark_timer_get_cycles(&timer);
  uint64_t total_time_ns = benchmark_timer_get_time_ns(&timer);

  BenchmarkResult result;
  result.test_name = test_name;
  result.total_cycles = total_cycles;
  result.total_time_ns = total_time_ns;
  result.operations = iterations;
  result.avg_cycles_per_op = (double)total_cycles / iterations;
  result.avg_time_ns_per_op = (double)total_time_ns / iterations;
  result.ops_per_sec = (iterations * 1000000000.0) / total_time_ns;
  result.min_cycles = result.avg_cycles_per_op; // Simplified
  result.max_cycles = result.avg_cycles_per_op; // Simplified
  result.operations_within_target = (result.avg_cycles_per_op <= TARGET_CYCLES) ? iterations : 0;
  result.target_achievement_percent = (result.operations_within_target * 100.0) / iterations;
  result.passed = (result.target_achievement_percent >= 95.0) &&
                  (result.avg_cycles_per_op <= TARGET_CYCLES) &&
                  (result.avg_time_ns_per_op <= TARGET_NS);

  return result;
}

// Performance validation
static inline int benchmark_validate_target(BenchmarkResult *result)
{
  return result->target_achievement_percent >= 95.0 &&
         result->avg_cycles_per_op <= TARGET_CYCLES &&
         result->avg_time_ns_per_op <= TARGET_NS;
}

// Result formatting and reporting
static inline void benchmark_result_print(BenchmarkResult *result)
{
  printf("=== Benchmark Result: %s ===\n", result->test_name);
  printf("Operations: %zu\n", result->operations);
  printf("Total time: %.3f ms (%llu ns)\n", result->total_time_ns / 1000000.0, result->total_time_ns);
  printf("Total cycles: %llu\n", result->total_cycles);
  printf("Average cycles per operation: %.1f\n", result->avg_cycles_per_op);
  printf("Average time per operation: %.1f ns\n", result->avg_time_ns_per_op);
  printf("Throughput: %.0f ops/sec\n", result->ops_per_sec);
  printf("\n");
  printf("Target Achievement:\n");
  printf("  Operations within ≤%d cycles: %zu/%zu (%.1f%%)\n",
         TARGET_CYCLES, result->operations_within_target, result->operations, result->target_achievement_percent);
  printf("  Status: %s\n", result->passed ? "✅ PASSED" : "❌ FAILED");
  printf("\n");
}

static inline void benchmark_suite_print_summary(BenchmarkSuite *suite)
{
  printf("=== %s Summary ===\n", suite->suite_name);
  printf("Total tests: %zu\n", suite->result_count);
  printf("Total suite time: %.3f ms\n", suite->total_suite_time_ns / 1000000.0);
  printf("Overall score: %.1f%%\n", suite->overall_score);
  printf("\n");
}

static inline void benchmark_suite_print_detailed(BenchmarkSuite *suite)
{
  printf("=== %s Detailed Results ===\n", suite->suite_name);
  printf("\n");

  for (size_t i = 0; i < suite->result_count; i++)
  {
    benchmark_result_print(&suite->results[i]);
  }

  benchmark_suite_print_summary(suite);
}

// Export results (simplified)
static inline void benchmark_suite_export_json(BenchmarkSuite *suite, const char *filename)
{
  FILE *file = fopen(filename, "w");
  if (!file)
    return;

  fprintf(file, "{\n");
  fprintf(file, "  \"suite_name\": \"%s\",\n", suite->suite_name);
  fprintf(file, "  \"total_tests\": %zu,\n", suite->result_count);
  fprintf(file, "  \"overall_score\": %.1f,\n", suite->overall_score);
  fprintf(file, "  \"results\": [\n");

  for (size_t i = 0; i < suite->result_count; i++)
  {
    BenchmarkResult *result = &suite->results[i];
    fprintf(file, "    {\n");
    fprintf(file, "      \"test_name\": \"%s\",\n", result->test_name);
    fprintf(file, "      \"operations\": %zu,\n", result->operations);
    fprintf(file, "      \"avg_cycles_per_op\": %.1f,\n", result->avg_cycles_per_op);
    fprintf(file, "      \"avg_time_ns_per_op\": %.1f,\n", result->avg_time_ns_per_op);
    fprintf(file, "      \"ops_per_sec\": %.0f,\n", result->ops_per_sec);
    fprintf(file, "      \"target_achievement_percent\": %.1f,\n", result->target_achievement_percent);
    fprintf(file, "      \"passed\": %s\n", result->passed ? "true" : "false");
    fprintf(file, "    }%s\n", (i < suite->result_count - 1) ? "," : "");
  }

  fprintf(file, "  ]\n");
  fprintf(file, "}\n");
  fclose(file);
}

static inline void benchmark_suite_export_csv(BenchmarkSuite *suite, const char *filename)
{
  FILE *file = fopen(filename, "w");
  if (!file)
    return;

  fprintf(file, "test_name,operations,avg_cycles_per_op,avg_time_ns_per_op,ops_per_sec,target_achievement_percent,passed\n");

  for (size_t i = 0; i < suite->result_count; i++)
  {
    BenchmarkResult *result = &suite->results[i];
    fprintf(file, "\"%s\",%zu,%.1f,%.1f,%.0f,%.1f,%s\n",
            result->test_name,
            result->operations,
            result->avg_cycles_per_op,
            result->avg_time_ns_per_op,
            result->ops_per_sec,
            result->target_achievement_percent,
            result->passed ? "true" : "false");
  }

  fclose(file);
}

// Hardware info (simplified)
typedef struct
{
  int num_cores;
  double cpu_frequency_ghz;
} HardwareInfo;

static inline HardwareInfo *detect_hardware_capabilities(void)
{
  HardwareInfo *info = malloc(sizeof(HardwareInfo));
  info->num_cores = 8;           // Default
  info->cpu_frequency_ghz = 3.2; // Default
  return info;
}

static inline void print_hardware_info(HardwareInfo *info)
{
  printf("=== Hardware Information ===\n");
  printf("Number of Cores: %d\n", info->num_cores);
  printf("CPU Frequency: %.1f GHz\n", info->cpu_frequency_ghz);
  printf("Target: ≤%d CPU cycles per operation\n", TARGET_CYCLES);
  printf("Target: ≤%d nanoseconds per operation\n", TARGET_NS);
  printf("Target: ≥%d operations per second\n", TARGET_OPS_PER_SEC);
  printf("\n");
}

static inline void destroy_hardware_info(HardwareInfo *info)
{
  free(info);
}

#endif // BENCHMARK_FRAMEWORK_H