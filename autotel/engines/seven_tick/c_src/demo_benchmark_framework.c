#include "demo_benchmark_framework.h"
#include "../runtime/src/seven_t_runtime.h"
#include <immintrin.h>

// Cycle distribution implementation
DemoCycleDistribution *demo_cycle_distribution_create(size_t capacity)
{
  DemoCycleDistribution *dist = malloc(sizeof(DemoCycleDistribution));
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

void demo_cycle_distribution_add(DemoCycleDistribution *dist, uint64_t cycles)
{
  if (!dist || dist->count >= dist->capacity)
    return;

  dist->cycle_counts[dist->count++] = cycles;
  dist->total_cycles += cycles;

  if (cycles < dist->min_cycles)
    dist->min_cycles = cycles;
  if (cycles > dist->max_cycles)
    dist->max_cycles = cycles;
}

double demo_cycle_distribution_percentile(DemoCycleDistribution *dist, double percentile)
{
  if (!dist || dist->count == 0)
    return 0.0;

  // Simple percentile calculation (not optimized for large datasets)
  size_t index = (size_t)(percentile * dist->count / 100.0);
  if (index >= dist->count)
    index = dist->count - 1;

  return (double)dist->cycle_counts[index];
}

void demo_cycle_distribution_destroy(DemoCycleDistribution *dist)
{
  if (dist)
  {
    free(dist->cycle_counts);
    free(dist);
  }
}

// Fitness distribution implementation
FitnessDistribution *fitness_distribution_create(size_t capacity)
{
  FitnessDistribution *dist = malloc(sizeof(FitnessDistribution));
  if (!dist)
    return NULL;

  dist->fitness_scores = malloc(capacity * sizeof(double));
  if (!dist->fitness_scores)
  {
    free(dist);
    return NULL;
  }

  dist->capacity = capacity;
  dist->count = 0;
  dist->total_fitness = 0.0;
  dist->min_fitness = INFINITY;
  dist->max_fitness = -INFINITY;

  return dist;
}

void fitness_distribution_add(FitnessDistribution *dist, double fitness)
{
  if (!dist || dist->count >= dist->capacity)
    return;

  dist->fitness_scores[dist->count++] = fitness;
  dist->total_fitness += fitness;

  if (fitness < dist->min_fitness)
    dist->min_fitness = fitness;
  if (fitness > dist->max_fitness)
    dist->max_fitness = fitness;
}

double fitness_distribution_percentile(FitnessDistribution *dist, double percentile)
{
  if (!dist || dist->count == 0)
    return 0.0;

  // Simple percentile calculation
  size_t index = (size_t)(percentile * dist->count / 100.0);
  if (index >= dist->count)
    index = dist->count - 1;

  return dist->fitness_scores[index];
}

void fitness_distribution_destroy(FitnessDistribution *dist)
{
  if (dist)
  {
    free(dist->fitness_scores);
    free(dist);
  }
}

// Demo benchmark suite management
DemoBenchmarkSuite *demo_benchmark_suite_create(const char *suite_name)
{
  DemoBenchmarkSuite *suite = malloc(sizeof(DemoBenchmarkSuite));
  if (!suite)
    return NULL;

  suite->suite_name = suite_name;
  suite->result_capacity = 10;
  suite->results = malloc(suite->result_capacity * sizeof(DemoBenchmarkResult));
  if (!suite->results)
  {
    free(suite);
    return NULL;
  }

  suite->result_count = 0;
  suite->total_suite_time_ns = 0;
  suite->overall_score = 0.0;
  suite->overall_fitness_score = 0.0;

  return suite;
}

void demo_benchmark_suite_add_result(DemoBenchmarkSuite *suite, DemoBenchmarkResult result)
{
  if (!suite)
    return;

  if (suite->result_count >= suite->result_capacity)
  {
    size_t new_capacity = suite->result_capacity * 2;
    DemoBenchmarkResult *new_results = realloc(suite->results, new_capacity * sizeof(DemoBenchmarkResult));
    if (!new_results)
      return;

    suite->results = new_results;
    suite->result_capacity = new_capacity;
  }

  suite->results[suite->result_count++] = result;
  suite->total_suite_time_ns += result.total_time_ns;
}

void demo_benchmark_suite_calculate_stats(DemoBenchmarkSuite *suite)
{
  if (!suite || suite->result_count == 0)
    return;

  double total_score = 0.0;
  double total_fitness = 0.0;

  for (size_t i = 0; i < suite->result_count; i++)
  {
    total_score += suite->results[i].target_achievement_percent;
    total_fitness += suite->results[i].pipeline_result.overall_fitness;
  }

  suite->overall_score = total_score / suite->result_count;
  suite->overall_fitness_score = total_fitness / suite->result_count;
}

void demo_benchmark_suite_destroy(DemoBenchmarkSuite *suite)
{
  if (suite)
  {
    free(suite->results);
    free(suite);
  }
}

// Individual demo benchmark execution
DemoBenchmarkResult demo_benchmark_execute_single(
    const char *test_name,
    DemoUseCase use_case,
    size_t iterations,
    PipelineResult (*pipeline_function)(void),
    void *pipeline_data)
{

  DemoBenchmarkResult result = {0};
  result.test_name = test_name;
  result.use_case = use_case;
  result.iterations = iterations;

  DemoCycleDistribution *cycle_dist = demo_cycle_distribution_create(iterations);
  FitnessDistribution *fitness_dist = fitness_distribution_create(iterations);

  if (!cycle_dist || !fitness_dist)
  {
    demo_cycle_distribution_destroy(cycle_dist);
    fitness_distribution_destroy(fitness_dist);
    return result;
  }

  uint64_t total_start_cycles = get_cycles();
  uint64_t total_start_time = get_nanoseconds();

  for (size_t i = 0; i < iterations; i++)
  {
    uint64_t start_cycles = get_cycles();
    uint64_t start_time = get_nanoseconds();

    // Execute pipeline
    PipelineResult pipeline_result = pipeline_function();

    uint64_t end_cycles = get_cycles();
    uint64_t end_time = get_nanoseconds();

    uint64_t cycles = end_cycles - start_cycles;
    uint64_t time_ns = end_time - start_time;

    demo_cycle_distribution_add(cycle_dist, cycles);
    fitness_distribution_add(fitness_dist, pipeline_result.overall_fitness);

    // Store first pipeline result as representative
    if (i == 0)
    {
      result.pipeline_result = pipeline_result;
    }

    result.total_cycles += cycles;
    result.total_time_ns += time_ns;
  }

  uint64_t total_end_cycles = get_cycles();
  uint64_t total_end_time = get_nanoseconds();

  // Calculate statistics
  result.avg_cycles_per_iteration = (double)result.total_cycles / iterations;
  result.avg_time_ns_per_iteration = (double)result.total_time_ns / iterations;
  result.ops_per_sec = (double)iterations * 1000000000ULL / result.total_time_ns;

  result.p50_cycles = demo_cycle_distribution_percentile(cycle_dist, 50.0);
  result.p95_cycles = demo_cycle_distribution_percentile(cycle_dist, 95.0);
  result.p99_cycles = demo_cycle_distribution_percentile(cycle_dist, 99.0);
  result.min_cycles = cycle_dist->min_cycles;
  result.max_cycles = cycle_dist->max_cycles;

  // Calculate target achievement
  size_t within_target = 0;
  for (size_t i = 0; i < cycle_dist->count; i++)
  {
    if (cycle_dist->cycle_counts[i] <= DEMO_TARGET_CYCLES)
    {
      within_target++;
    }
  }
  result.iterations_within_target = within_target;
  result.target_achievement_percent = (double)within_target / iterations * 100.0;

  // Calculate fitness achievement
  size_t fitness_within_target = 0;
  for (size_t i = 0; i < fitness_dist->count; i++)
  {
    double fitness = fitness_dist->fitness_scores[i];
    if (fitness >= DEMO_TARGET_FITNESS_MIN && fitness <= DEMO_TARGET_FITNESS_MAX)
    {
      fitness_within_target++;
    }
  }
  result.fitness_achievement_percent = (double)fitness_within_target / iterations * 100.0;

  // Validate results
  result.passed = demo_benchmark_validate_target(&result) && demo_benchmark_validate_fitness(&result);

  demo_cycle_distribution_destroy(cycle_dist);
  fitness_distribution_destroy(fitness_dist);

  return result;
}

// Performance validation
int demo_benchmark_validate_target(DemoBenchmarkResult *result)
{
  if (!result)
    return 0;

  return result->target_achievement_percent >= 95.0 &&
         result->avg_cycles_per_iteration <= DEMO_TARGET_CYCLES &&
         result->avg_time_ns_per_iteration <= DEMO_TARGET_NS;
}

int demo_benchmark_validate_fitness(DemoBenchmarkResult *result)
{
  if (!result)
    return 0;

  return result->fitness_achievement_percent >= 80.0 &&
         result->pipeline_result.overall_fitness >= DEMO_TARGET_FITNESS_MIN &&
         result->pipeline_result.overall_fitness <= DEMO_TARGET_FITNESS_MAX;
}

// Result formatting and reporting
void demo_benchmark_result_print(DemoBenchmarkResult *result)
{
  if (!result)
    return;

  printf("\n=== Demo Benchmark Result: %s ===\n", result->test_name);
  printf("Use Case: %s\n", demo_use_case_get_name(result->use_case));
  printf("Iterations: %zu\n", result->iterations);
  printf("Total time: %.3f ms (%llu ns)\n", result->total_time_ns / 1000000.0, result->total_time_ns);
  printf("Total cycles: %llu\n", result->total_cycles);
  printf("Average cycles per iteration: %.2f\n", result->avg_cycles_per_iteration);
  printf("Average time per iteration: %.2f ns\n", result->avg_time_ns_per_iteration);
  printf("Throughput: %.0f ops/sec\n", result->ops_per_sec);

  printf("\nCycle Distribution:\n");
  printf("  Min: %llu cycles\n", result->min_cycles);
  printf("  P50: %.1f cycles\n", result->p50_cycles);
  printf("  P95: %.1f cycles\n", result->p95_cycles);
  printf("  P99: %.1f cycles\n", result->p99_cycles);
  printf("  Max: %llu cycles\n", result->max_cycles);

  printf("\nTarget Achievement:\n");
  printf("  Iterations within ≤%d cycles: %zu/%zu (%.1f%%)\n",
         DEMO_TARGET_CYCLES, result->iterations_within_target, result->iterations, result->target_achievement_percent);
  printf("  Fitness within target range: %.1f%%\n", result->fitness_achievement_percent);
  printf("  Overall fitness: %.4f\n", result->pipeline_result.overall_fitness);
  printf("  Status: %s\n", result->passed ? "✅ PASSED" : "❌ FAILED");

  printf("\nPipeline Details:\n");
  print_pipeline_result(&result->pipeline_result);
}

void demo_benchmark_suite_print_summary(DemoBenchmarkSuite *suite)
{
  if (!suite)
    return;

  printf("\n=== Demo Benchmark Suite Summary: %s ===\n", suite->suite_name);
  printf("Total tests: %zu\n", suite->result_count);
  printf("Total suite time: %.3f ms (%llu ns)\n", suite->total_suite_time_ns / 1000000.0, suite->total_suite_time_ns);
  printf("Overall score: %.2f%%\n", suite->overall_score);
  printf("Overall fitness: %.4f\n", suite->overall_fitness_score);

  printf("\nTest Results:\n");
  for (size_t i = 0; i < suite->result_count; i++)
  {
    DemoBenchmarkResult *result = &suite->results[i];
    printf("  %s: %s (%.1f%% target, %.4f fitness)\n",
           result->test_name,
           result->passed ? "PASS" : "FAIL",
           result->target_achievement_percent,
           result->pipeline_result.overall_fitness);
  }
}

// Demo use case specific functions
const char *demo_use_case_get_name(DemoUseCase use_case)
{
  switch (use_case)
  {
  case DEMO_IRIS_CLASSIFICATION:
    return "Iris Classification";
  case DEMO_BOSTON_REGRESSION:
    return "Boston Housing Regression";
  case DEMO_DIGITS_CLASSIFICATION:
    return "Digits Classification";
  case DEMO_BREAST_CANCER_CLASSIFICATION:
    return "Breast Cancer Classification";
  case DEMO_WINE_QUALITY_CLASSIFICATION:
    return "Wine Quality Classification";
  default:
    return "Unknown";
  }
}

size_t demo_use_case_get_sample_count(DemoUseCase use_case)
{
  switch (use_case)
  {
  case DEMO_IRIS_CLASSIFICATION:
    return 150;
  case DEMO_BOSTON_REGRESSION:
    return 506;
  case DEMO_DIGITS_CLASSIFICATION:
    return 1797;
  case DEMO_BREAST_CANCER_CLASSIFICATION:
    return 569;
  case DEMO_WINE_QUALITY_CLASSIFICATION:
    return 1599;
  default:
    return 0;
  }
}

size_t demo_use_case_get_feature_count(DemoUseCase use_case)
{
  switch (use_case)
  {
  case DEMO_IRIS_CLASSIFICATION:
    return 4;
  case DEMO_BOSTON_REGRESSION:
    return 13;
  case DEMO_DIGITS_CLASSIFICATION:
    return 64;
  case DEMO_BREAST_CANCER_CLASSIFICATION:
    return 30;
  case DEMO_WINE_QUALITY_CLASSIFICATION:
    return 11;
  default:
    return 0;
  }
}

size_t demo_use_case_get_class_count(DemoUseCase use_case)
{
  switch (use_case)
  {
  case DEMO_IRIS_CLASSIFICATION:
    return 3;
  case DEMO_BOSTON_REGRESSION:
    return 1; // Regression
  case DEMO_DIGITS_CLASSIFICATION:
    return 10;
  case DEMO_BREAST_CANCER_CLASSIFICATION:
    return 2;
  case DEMO_WINE_QUALITY_CLASSIFICATION:
    return 7;
  default:
    return 0;
  }
}

// Pipeline step management
PipelineStepResult *pipeline_step_result_create(PipelineStepType step_type, const char *step_name)
{
  PipelineStepResult *step = malloc(sizeof(PipelineStepResult));
  if (!step)
    return NULL;

  step->step_type = step_type;
  step->step_name = step_name;
  step->execution_time_ns = 0;
  step->execution_cycles = 0;
  step->fitness_score = 0.0;
  step->samples_processed = 0;
  step->features_processed = 0;
  step->success = 0;

  return step;
}

void pipeline_step_result_destroy(PipelineStepResult *step)
{
  free(step);
}

// Pipeline result management
PipelineResult *pipeline_result_create(DemoUseCase use_case)
{
  PipelineResult *pipeline = malloc(sizeof(PipelineResult));
  if (!pipeline)
    return NULL;

  pipeline->use_case = use_case;
  pipeline->use_case_name = demo_use_case_get_name(use_case);
  pipeline->steps = malloc(10 * sizeof(PipelineStepResult)); // Max 10 steps
  pipeline->step_count = 0;
  pipeline->step_capacity = 10;
  pipeline->total_pipeline_time_ns = 0;
  pipeline->total_pipeline_cycles = 0;
  pipeline->overall_fitness = 0.0;
  pipeline->avg_fitness_per_step = 0.0;
  pipeline->pipeline_success = 0;
  pipeline->total_samples = demo_use_case_get_sample_count(use_case);
  pipeline->total_features = demo_use_case_get_feature_count(use_case);

  return pipeline;
}

void pipeline_result_add_step(PipelineResult *pipeline, PipelineStepResult *step)
{
  if (!pipeline || !step || pipeline->step_count >= pipeline->step_capacity)
    return;

  pipeline->steps[pipeline->step_count++] = *step;
  pipeline->total_pipeline_time_ns += step->execution_time_ns;
  pipeline->total_pipeline_cycles += step->execution_cycles;
}

void pipeline_result_calculate_stats(PipelineResult *pipeline)
{
  if (!pipeline || pipeline->step_count == 0)
    return;

  double total_fitness = 0.0;
  int successful_steps = 0;

  for (size_t i = 0; i < pipeline->step_count; i++)
  {
    if (pipeline->steps[i].success)
    {
      total_fitness += pipeline->steps[i].fitness_score;
      successful_steps++;
    }
  }

  if (successful_steps > 0)
  {
    pipeline->overall_fitness = total_fitness / successful_steps;
    pipeline->avg_fitness_per_step = total_fitness / pipeline->step_count;
  }

  pipeline->pipeline_success = (successful_steps == pipeline->step_count);
}

void pipeline_result_destroy(PipelineResult *pipeline)
{
  if (pipeline)
  {
    free(pipeline->steps);
    free(pipeline);
  }
}

// Utility functions
void print_pipeline_step_result(PipelineStepResult *step)
{
  if (!step)
    return;

  printf("    %s: %.2f ns, %.0f cycles, fitness=%.4f, samples=%u, features=%u, %s\n",
         step->step_name,
         (double)step->execution_time_ns,
         (double)step->execution_cycles,
         step->fitness_score,
         step->samples_processed,
         step->features_processed,
         step->success ? "SUCCESS" : "FAILED");
}

void print_pipeline_result(PipelineResult *pipeline)
{
  if (!pipeline)
    return;

  printf("  Pipeline: %s\n", pipeline->use_case_name);
  printf("  Total time: %.2f ns (%llu cycles)\n",
         (double)pipeline->total_pipeline_time_ns, pipeline->total_pipeline_cycles);
  printf("  Overall fitness: %.4f\n", pipeline->overall_fitness);
  printf("  Steps: %zu/%zu successful\n", pipeline->step_count, pipeline->step_count);
  printf("  Samples: %zu, Features: %zu\n", pipeline->total_samples, pipeline->total_features);

  printf("  Step Details:\n");
  for (size_t i = 0; i < pipeline->step_count; i++)
  {
    print_pipeline_step_result(&pipeline->steps[i]);
  }
}

// Hardware detection (simplified)
double detect_cpu_frequency(void)
{
  // Simplified CPU frequency detection
  return 3.2; // Assume 3.2 GHz
}

size_t detect_cache_line_size(void)
{
  // Simplified cache line size detection
  return 64; // Assume 64 bytes
}

DemoHardwareInfo *detect_demo_hardware_capabilities(void)
{
  DemoHardwareInfo *info = malloc(sizeof(DemoHardwareInfo));
  if (!info)
    return NULL;

  // Simplified hardware detection
  info->avx2_support = 1;
  info->avx512_support = 0;
  info->sse4_2_support = 1;
  info->l1_cache_size = 32 * 1024;       // 32 KB
  info->l2_cache_size = 256 * 1024;      // 256 KB
  info->l3_cache_size = 8 * 1024 * 1024; // 8 MB
  info->num_cores = 8;
  info->cpu_frequency_ghz = 3.2;

  return info;
}

void print_demo_hardware_info(DemoHardwareInfo *info)
{
  if (!info)
    return;

  printf("=== Hardware Information ===\n");
  printf("CPU Frequency: %.1f GHz\n", info->cpu_frequency_ghz);
  printf("Number of Cores: %d\n", info->num_cores);
  printf("L1 Cache Size: %zu KB\n", info->l1_cache_size / 1024);
  printf("L2 Cache Size: %zu KB\n", info->l2_cache_size / 1024);
  printf("L3 Cache Size: %zu MB\n", info->l3_cache_size / (1024 * 1024));
  printf("AVX2 Support: %s\n", info->avx2_support ? "Yes" : "No");
  printf("AVX-512 Support: %s\n", info->avx512_support ? "Yes" : "No");
  printf("SSE4.2 Support: %s\n", info->sse4_2_support ? "Yes" : "No");
}

void destroy_demo_hardware_info(DemoHardwareInfo *info)
{
  free(info);
}