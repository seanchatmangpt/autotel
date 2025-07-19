#include "7t_benchmark_framework.h"
#include <math.h>

// Default benchmark configuration
BenchmarkConfig get_default_config(void)
{
  return (BenchmarkConfig){
      .benchmark_name = "Default Benchmark",
      .iterations = DEFAULT_ITERATIONS,
      .warmup_iterations = DEFAULT_WARMUP_ITERATIONS,
      .batch_size = DEFAULT_BATCH_SIZE,
      .verbose = false,
      .validate_correctness = true};
}

BenchmarkConfig get_quick_config(void)
{
  return (BenchmarkConfig){
      .benchmark_name = "Quick Benchmark",
      .iterations = 100000,
      .warmup_iterations = 1000,
      .batch_size = 100,
      .verbose = false,
      .validate_correctness = false};
}

BenchmarkConfig get_thorough_config(void)
{
  return (BenchmarkConfig){
      .benchmark_name = "Thorough Benchmark",
      .iterations = 10000000,
      .warmup_iterations = 100000,
      .batch_size = 10000,
      .verbose = true,
      .validate_correctness = true};
}

// Performance tier assessment
const char *get_performance_tier(double avg_time_ns)
{
  if (avg_time_ns < SEVEN_TICK_TARGET_NS)
  {
    return "L1 (7-TICK)";
  }
  else if (avg_time_ns < L2_TIER_TARGET_NS)
  {
    return "L2 (Sub-100ns)";
  }
  else if (avg_time_ns < L3_TIER_TARGET_NS)
  {
    return "L3 (Sub-1μs)";
  }
  else
  {
    return "Above L3";
  }
}

bool is_target_achieved(double avg_time_ns)
{
  return avg_time_ns < SEVEN_TICK_TARGET_NS;
}

// Performance validation functions
bool validate_7tick_performance(double avg_time_ns)
{
  return avg_time_ns < SEVEN_TICK_TARGET_NS;
}

bool validate_l2_tier_performance(double avg_time_ns)
{
  return avg_time_ns < L2_TIER_TARGET_NS;
}

bool validate_l3_tier_performance(double avg_time_ns)
{
  return avg_time_ns < L3_TIER_TARGET_NS;
}

// Utility functions
void benchmark_warmup(BenchmarkOperation operation, void *context, uint64_t iterations)
{
  for (uint64_t i = 0; i < iterations; i++)
  {
    operation(context);
  }
}

// Simple bubble sort for small arrays
void sort_times(uint64_t *times, uint64_t count)
{
  for (uint64_t i = 0; i < count - 1; i++)
  {
    for (uint64_t j = 0; j < count - i - 1; j++)
    {
      if (times[j] > times[j + 1])
      {
        uint64_t temp = times[j];
        times[j] = times[j + 1];
        times[j + 1] = temp;
      }
    }
  }
}

double calculate_percentile(uint64_t *times, uint64_t count, double percentile)
{
  if (count == 0)
    return 0.0;

  // Create a copy for sorting
  uint64_t *sorted_times = malloc(count * sizeof(uint64_t));
  if (!sorted_times)
    return 0.0;

  memcpy(sorted_times, times, count * sizeof(uint64_t));
  sort_times(sorted_times, count);

  double index = (percentile / 100.0) * (count - 1);
  uint64_t lower_index = (uint64_t)index;
  uint64_t upper_index = lower_index + 1;

  double result;
  if (upper_index >= count)
  {
    result = sorted_times[lower_index];
  }
  else
  {
    double weight = index - lower_index;
    result = sorted_times[lower_index] * (1 - weight) + sorted_times[upper_index] * weight;
  }

  free(sorted_times);
  return result;
}

// Main benchmark runner
BenchmarkResult run_benchmark(const char *operation_name,
                              const char *component_name,
                              BenchmarkConfig *config,
                              BenchmarkOperation operation,
                              void *context)
{
  BenchmarkResult result = {0};
  result.operation_name = operation_name;
  result.component_name = component_name;
  result.total_operations = config->iterations;
  result.min_time_ns = UINT64_MAX;
  result.max_time_ns = 0;

  if (config->verbose)
  {
    printf("Running benchmark: %s (%s)\n", operation_name, component_name);
    printf("Iterations: %llu, Warmup: %llu\n", config->iterations, config->warmup_iterations);
  }

  // Warmup phase
  if (config->warmup_iterations > 0)
  {
    benchmark_warmup(operation, context, config->warmup_iterations);
  }

  // Allocate array for individual timing measurements
  uint64_t *individual_times = malloc(config->iterations * sizeof(uint64_t));
  if (!individual_times)
  {
    printf("❌ Failed to allocate memory for timing measurements\n");
    return result;
  }

  // Actual benchmark
  uint64_t total_start = get_nanoseconds();

  for (uint64_t i = 0; i < config->iterations; i++)
  {
    uint64_t start = get_nanoseconds();
    operation(context);
    uint64_t end = get_nanoseconds();

    uint64_t duration = end - start;
    individual_times[i] = duration;
    result.total_time_ns += duration;

    if (duration < result.min_time_ns)
      result.min_time_ns = duration;
    if (duration > result.max_time_ns)
      result.max_time_ns = duration;
  }

  uint64_t total_end = get_nanoseconds();

  // Calculate statistics
  result.avg_time_ns = (double)result.total_time_ns / config->iterations;
  result.throughput_ops_per_sec = (double)config->iterations / (result.total_time_ns / 1e9);
  result.p50_time_ns = calculate_percentile(individual_times, config->iterations, 50.0);
  result.p95_time_ns = calculate_percentile(individual_times, config->iterations, 95.0);
  result.p99_time_ns = calculate_percentile(individual_times, config->iterations, 99.0);
  result.target_achieved = is_target_achieved(result.avg_time_ns);
  result.performance_tier = get_performance_tier(result.avg_time_ns);

  free(individual_times);

  return result;
}

// Result printing functions
void print_benchmark_result(BenchmarkResult *result)
{
  printf("=== %s Benchmark (%s) ===\n", result->operation_name, result->component_name);
  printf("Operations: %llu\n", result->total_operations);
  printf("Total time: %.3f ms\n", result->total_time_ns / 1e6);
  printf("Average: %.1f ns\n", result->avg_time_ns);
  printf("Min: %llu ns\n", result->min_time_ns);
  printf("Max: %llu ns\n", result->max_time_ns);
  printf("P50: %.1f ns\n", result->p50_time_ns);
  printf("P95: %.1f ns\n", result->p95_time_ns);
  printf("P99: %.1f ns\n", result->p99_time_ns);
  printf("Throughput: %.0f ops/sec\n", result->throughput_ops_per_sec);

  // Performance tier assessment
  if (result->target_achieved)
  {
    printf("🎉 %s: 7-TICK PERFORMANCE ACHIEVED! (%.1f ns)\n",
           result->performance_tier, result->avg_time_ns);
  }
  else if (result->avg_time_ns < L2_TIER_TARGET_NS)
  {
    printf("✅ %s: Sub-100ns performance! (%.1f ns)\n",
           result->performance_tier, result->avg_time_ns);
  }
  else if (result->avg_time_ns < L3_TIER_TARGET_NS)
  {
    printf("✅ %s: Sub-1μs performance! (%.1f ns)\n",
           result->performance_tier, result->avg_time_ns);
  }
  else
  {
    printf("⚠️ %s: Performance above 1μs (%.1f ns)\n",
           result->performance_tier, result->avg_time_ns);
  }
  printf("\n");
}

void print_benchmark_summary(BenchmarkResult *results, int count)
{
  printf("=== Benchmark Summary ===\n");
  printf("Total benchmarks: %d\n\n", count);

  int seven_tick_count = 0;
  int l2_tier_count = 0;
  int l3_tier_count = 0;
  int above_l3_count = 0;

  for (int i = 0; i < count; i++)
  {
    if (results[i].target_achieved)
    {
      seven_tick_count++;
    }
    else if (results[i].avg_time_ns < L2_TIER_TARGET_NS)
    {
      l2_tier_count++;
    }
    else if (results[i].avg_time_ns < L3_TIER_TARGET_NS)
    {
      l3_tier_count++;
    }
    else
    {
      above_l3_count++;
    }
  }

  printf("Performance Distribution:\n");
  printf("  🎉 7-Tick Performance: %d/%d (%.1f%%)\n",
         seven_tick_count, count, (double)seven_tick_count / count * 100);
  printf("  ✅ L2 Tier (Sub-100ns): %d/%d (%.1f%%)\n",
         l2_tier_count, count, (double)l2_tier_count / count * 100);
  printf("  ✅ L3 Tier (Sub-1μs): %d/%d (%.1f%%)\n",
         l3_tier_count, count, (double)l3_tier_count / count * 100);
  printf("  ⚠️ Above L3: %d/%d (%.1f%%)\n",
         above_l3_count, count, (double)above_l3_count / count * 100);
  printf("\n");
}

// Hardware information (simplified)
HardwareInfo get_hardware_info(void)
{
  return (HardwareInfo){
      .cpu_model = "Unknown",
      .cpu_frequency_ghz = 3.0,
      .l1_cache_size_kb = 32,
      .l2_cache_size_kb = 256,
      .l3_cache_size_kb = 8192,
      .memory_size_gb = 16};
}

void print_hardware_info(HardwareInfo *info)
{
  printf("=== Hardware Information ===\n");
  printf("CPU Model: %s\n", info->cpu_model);
  printf("CPU Frequency: %.1f GHz\n", info->cpu_frequency_ghz);
  printf("L1 Cache: %zu KB\n", info->l1_cache_size_kb);
  printf("L2 Cache: %zu KB\n", info->l2_cache_size_kb);
  printf("L3 Cache: %zu KB\n", info->l3_cache_size_kb);
  printf("Memory: %zu GB\n", info->memory_size_gb);
  printf("\n");
}

// Memory statistics (simplified)
MemoryStats get_memory_stats(void)
{
  return (MemoryStats){
      .peak_memory_bytes = 0,
      .current_memory_bytes = 0,
      .allocations_count = 0,
      .deallocations_count = 0};
}

void print_memory_stats(MemoryStats *stats)
{
  printf("=== Memory Statistics ===\n");
  printf("Peak Memory: %.2f MB\n", stats->peak_memory_bytes / (1024.0 * 1024.0));
  printf("Current Memory: %.2f MB\n", stats->current_memory_bytes / (1024.0 * 1024.0));
  printf("Allocations: %zu\n", stats->allocations_count);
  printf("Deallocations: %zu\n", stats->deallocations_count);
  printf("\n");
}

// Benchmark suite management
BenchmarkSuite *create_benchmark_suite(const char *name)
{
  BenchmarkSuite *suite = malloc(sizeof(BenchmarkSuite));
  if (!suite)
    return NULL;

  suite->suite_name = name;
  suite->result_capacity = 10;
  suite->result_count = 0;
  suite->results = malloc(suite->result_capacity * sizeof(BenchmarkResult));
  suite->hardware = get_hardware_info();
  suite->memory = get_memory_stats();

  return suite;
}

void add_benchmark_result(BenchmarkSuite *suite, BenchmarkResult result)
{
  if (suite->result_count >= suite->result_capacity)
  {
    suite->result_capacity *= 2;
    suite->results = realloc(suite->results, suite->result_capacity * sizeof(BenchmarkResult));
  }

  suite->results[suite->result_count++] = result;
}

void print_benchmark_suite(BenchmarkSuite *suite)
{
  printf("=== %s Benchmark Suite ===\n", suite->suite_name);
  print_hardware_info(&suite->hardware);

  for (int i = 0; i < suite->result_count; i++)
  {
    print_benchmark_result(&suite->results[i]);
  }

  print_benchmark_summary(suite->results, suite->result_count);
  print_memory_stats(&suite->memory);
}

void destroy_benchmark_suite(BenchmarkSuite *suite)
{
  if (suite)
  {
    free(suite->results);
    free(suite);
  }
}

// Export functions (simplified)
void export_benchmark_results_csv(BenchmarkSuite *suite, const char *filename)
{
  FILE *file = fopen(filename, "w");
  if (!file)
    return;

  fprintf(file, "Operation,Component,Operations,Avg_Time_ns,Min_Time_ns,Max_Time_ns,P50_ns,P95_ns,P99_ns,Throughput_ops_per_sec,Target_Achieved,Performance_Tier\n");

  for (int i = 0; i < suite->result_count; i++)
  {
    BenchmarkResult *r = &suite->results[i];
    fprintf(file, "%s,%s,%llu,%.1f,%llu,%llu,%.1f,%.1f,%.1f,%.0f,%s,%s\n",
            r->operation_name, r->component_name, r->total_operations,
            r->avg_time_ns, r->min_time_ns, r->max_time_ns,
            r->p50_time_ns, r->p95_time_ns, r->p99_time_ns,
            r->throughput_ops_per_sec,
            r->target_achieved ? "true" : "false",
            r->performance_tier);
  }

  fclose(file);
}

void export_benchmark_results_json(BenchmarkSuite *suite, const char *filename)
{
  FILE *file = fopen(filename, "w");
  if (!file)
    return;

  fprintf(file, "{\n");
  fprintf(file, "  \"suite_name\": \"%s\",\n", suite->suite_name);
  fprintf(file, "  \"results\": [\n");

  for (int i = 0; i < suite->result_count; i++)
  {
    BenchmarkResult *r = &suite->results[i];
    fprintf(file, "    {\n");
    fprintf(file, "      \"operation\": \"%s\",\n", r->operation_name);
    fprintf(file, "      \"component\": \"%s\",\n", r->component_name);
    fprintf(file, "      \"avg_time_ns\": %.1f,\n", r->avg_time_ns);
    fprintf(file, "      \"throughput_ops_per_sec\": %.0f,\n", r->throughput_ops_per_sec);
    fprintf(file, "      \"target_achieved\": %s,\n", r->target_achieved ? "true" : "false");
    fprintf(file, "      \"performance_tier\": \"%s\"\n", r->performance_tier);
    fprintf(file, "    }%s\n", i < suite->result_count - 1 ? "," : "");
  }

  fprintf(file, "  ]\n");
  fprintf(file, "}\n");

  fclose(file);
}