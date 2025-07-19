#ifndef DEMO_BENCHMARK_MACROS_H
#define DEMO_BENCHMARK_MACROS_H

#include "demo_benchmark_common.h"
#include "demo_benchmark_framework.h"

// Performance measurement macros
#define BENCHMARK_START(name)         \
  CommonTimer benchmark_timer_##name; \
  common_timer_start(&benchmark_timer_##name, #name)

#define BENCHMARK_END(name) \
  common_timer_end(&benchmark_timer_##name)

#define BENCHMARK_GET_CYCLES(name) \
  common_timer_get_cycles(&benchmark_timer_##name)

#define BENCHMARK_GET_TIME_NS(name) \
  common_timer_get_time_ns(&benchmark_timer_##name)

#define BENCHMARK_GET_TIME_MS(name) \
  common_timer_get_time_ms(&benchmark_timer_##name)

// Pipeline step creation macros
#define CREATE_PIPELINE_STEP(step_type, step_name) \
  PipelineStepResult *step = pipeline_step_result_create(step_type, step_name)

#define FILL_PIPELINE_STEP(step, time_ns, cycles, fitness, samples, features, success) \
  do                                                                                   \
  {                                                                                    \
    step->execution_time_ns = time_ns;                                                 \
    step->execution_cycles = cycles;                                                   \
    step->fitness_score = fitness;                                                     \
    step->samples_processed = samples;                                                 \
    step->features_processed = features;                                               \
    step->success = success;                                                           \
  } while (0)

#define ADD_PIPELINE_STEP(pipeline, step)   \
  pipeline_result_add_step(pipeline, step); \
  pipeline_step_result_destroy(step)

// Dataset operation macros
#define DATASET_SET_FEATURE(dataset, sample, feature, value) \
  dataset_set_feature(dataset, sample, feature, value)

#define DATASET_GET_FEATURE(dataset, sample, feature) \
  dataset_get_feature(dataset, sample, feature)

#define DATASET_SET_TARGET(dataset, sample, target) \
  dataset_set_target(dataset, sample, target)

#define DATASET_GET_TARGET(dataset, sample) \
  dataset_get_target(dataset, sample)

// Matrix operation macros
#define MATRIX_SET(matrix, row, col, value) \
  matrix_set(matrix, row, col, value)

#define MATRIX_GET(matrix, row, col) \
  matrix_get(matrix, row, col)

// Array operation macros
#define ARRAY_ADD(array, value) \
  int_array_add(array, value)

#define ARRAY_GET(array, index) \
  int_array_get(array, index)

#define ARRAY_SET(array, index, value) \
  int_array_set(array, index, value)

#define DOUBLE_ARRAY_ADD(array, value) \
  double_array_add(array, value)

#define DOUBLE_ARRAY_GET(array, index) \
  double_array_get(array, index)

#define DOUBLE_ARRAY_SET(array, index, value) \
  double_array_set(array, index, value)

// Memory management macros
#define SAFE_CREATE(type, ...)                             \
  (type *)malloc(sizeof(type));                            \
  if (!type##_ptr)                                         \
  {                                                        \
    printf("❌ Memory allocation failed for " #type "\n"); \
    return NULL;                                           \
  }

#define SAFE_DESTROY(ptr, destroy_func) \
  do                                    \
  {                                     \
    if (ptr)                            \
    {                                   \
      destroy_func(ptr);                \
      ptr = NULL;                       \
    }                                   \
  } while (0)

// Error handling macros
#define CHECK_NULL(ptr, error_msg)              \
  if (!ptr)                                     \
  {                                             \
    printf("❌ " error_msg ": NULL pointer\n"); \
    return NULL;                                \
  }

#define CHECK_CONDITION(condition, error_msg) \
  if (!(condition))                           \
  {                                           \
    printf("❌ " error_msg "\n");             \
    return NULL;                              \
  }

// Benchmark result creation macros
#define CREATE_BENCHMARK_RESULT(test_name, use_case, iterations) \
  DemoBenchmarkResult result = {0};                              \
  result.test_name = test_name;                                  \
  result.use_case = use_case;                                    \
  result.iterations = iterations

#define FILL_BENCHMARK_RESULT(result, total_cycles, total_time_ns, avg_cycles, avg_time_ns, ops_per_sec) \
  do                                                                                                     \
  {                                                                                                      \
    result.total_cycles = total_cycles;                                                                  \
    result.total_time_ns = total_time_ns;                                                                \
    result.avg_cycles_per_iteration = avg_cycles;                                                        \
    result.avg_time_ns_per_iteration = avg_time_ns;                                                      \
    result.ops_per_sec = ops_per_sec;                                                                    \
  } while (0)

#define FILL_BENCHMARK_PERCENTILES(result, p50, p95, p99, min_cycles, max_cycles) \
  do                                                                              \
  {                                                                               \
    result.p50_cycles = p50;                                                      \
    result.p95_cycles = p95;                                                      \
    result.p99_cycles = p99;                                                      \
    result.min_cycles = min_cycles;                                               \
    result.max_cycles = max_cycles;                                               \
  } while (0)

#define FILL_BENCHMARK_TARGETS(result, within_target, target_percent, fitness_percent) \
  do                                                                                   \
  {                                                                                    \
    result.iterations_within_target = within_target;                                   \
    result.target_achievement_percent = target_percent;                                \
    result.fitness_achievement_percent = fitness_percent;                              \
  } while (0)

// Pipeline creation macros
#define CREATE_STANDARD_PIPELINE(name, preprocessing, feature_selection, model) \
  Pipeline *pipeline = create_standardized_pipeline(name, preprocessing, feature_selection, model)

#define CREATE_STANDARD_DATASET(name, samples, features, classes, is_regression, noise) \
  Dataset *dataset = create_standardized_dataset(name, samples, features, classes, is_regression, noise)

// Algorithm execution macros
#define EXECUTE_ALGORITHM(algorithm, dataset, selected_features, selected_samples) \
  algorithm->evaluate_function(dataset, selected_features, selected_samples)

#define EXECUTE_PIPELINE(pipeline, dataset) \
  pipeline_execute(pipeline, dataset)

// Validation macros
#define VALIDATE_TARGET(result) \
  demo_benchmark_validate_target(&result)

#define VALIDATE_FITNESS(result) \
  demo_benchmark_validate_fitness(&result)

#define VALIDATE_BOTH(result) \
  (VALIDATE_TARGET(result) && VALIDATE_FITNESS(result))

// Result printing macros
#define PRINT_BENCHMARK_RESULT(result) \
  demo_benchmark_result_print(&result)

#define PRINT_BENCHMARK_SUITE(suite) \
  demo_benchmark_suite_print_summary(suite)

// Export macros
#define EXPORT_RESULTS(suite, base_name)                        \
  do                                                            \
  {                                                             \
    demo_benchmark_suite_export_json(suite, base_name ".json"); \
    demo_benchmark_suite_export_csv(suite, base_name ".csv");   \
  } while (0)

// Loop macros for common patterns
#define FOR_EACH_SAMPLE(dataset, sample_var) \
  for (size_t sample_var = 0; sample_var < dataset->sample_count; sample_var++)

#define FOR_EACH_FEATURE(dataset, feature_var) \
  for (size_t feature_var = 0; feature_var < dataset->feature_count; feature_var++)

#define FOR_EACH_CLASS(dataset, class_var) \
  for (size_t class_var = 0; class_var < dataset->class_count; class_var++)

// Performance target macros
#define CHECK_PERFORMANCE_TARGET(cycles, target_cycles) \
  (cycles <= target_cycles)

#define CHECK_FITNESS_TARGET(fitness, min_fitness, max_fitness) \
  (fitness >= min_fitness && fitness <= max_fitness)

// Common algorithm patterns
#define NORMALIZE_FEATURES(dataset, selected_features, selected_samples) \
  algorithm_normalize_features(dataset, selected_features, selected_samples)

#define STANDARDIZE_FEATURES(dataset, selected_features, selected_samples) \
  algorithm_standardize_features(dataset, selected_features, selected_samples)

#define SELECT_K_BEST_FEATURES(dataset, selected_features, selected_samples, k) \
  algorithm_select_k_best_features(dataset, selected_features, selected_samples, k)

#define RANDOM_FOREST_CLASSIFIER(dataset, selected_features, selected_samples) \
  algorithm_random_forest_classifier(dataset, selected_features, selected_samples)

// Common evaluation patterns
#define CALCULATE_ACCURACY(predictions, targets) \
  calculate_accuracy(predictions, targets)

#define CALCULATE_PRECISION(predictions, targets, class_label) \
  calculate_precision(predictions, targets, class_label)

#define CALCULATE_RECALL(predictions, targets, class_label) \
  calculate_recall(predictions, targets, class_label)

#define CALCULATE_F1_SCORE(predictions, targets, class_label) \
  calculate_f1_score(predictions, targets, class_label)

#define CALCULATE_R_SQUARED(predictions, targets) \
  calculate_r_squared(predictions, targets)

// Common sampling patterns
#define SAMPLE_RANDOM(dataset, count) \
  sample_random(dataset, count)

#define SAMPLE_STRATIFIED(dataset, count) \
  sample_stratified(dataset, count)

#define SAMPLE_WEIGHTED(dataset, count) \
  sample_weighted(dataset, count)

// Common validation patterns
#define CROSS_VALIDATE(dataset, pipeline, folds) \
  cross_validate(dataset, pipeline, folds)

#define BOOTSTRAP_VALIDATE(dataset, pipeline, iterations) \
  bootstrap_validate(dataset, pipeline, iterations)

// Debug macros
#define DEBUG_PRINT(fmt, ...) \
  printf("[DEBUG] " fmt "\n", ##__VA_ARGS__)

#define DEBUG_PRINT_MATRIX(matrix, name)                                          \
  do                                                                              \
  {                                                                               \
    printf("[DEBUG] Matrix %s (%zu x %zu):\n", name, matrix->rows, matrix->cols); \
    for (size_t i = 0; i < matrix->rows && i < 5; i++)                            \
    {                                                                             \
      printf("  Row %zu: ", i);                                                   \
      for (size_t j = 0; j < matrix->cols && j < 5; j++)                          \
      {                                                                           \
        printf("%.3f ", matrix_get(matrix, i, j));                                \
      }                                                                           \
      if (matrix->cols > 5)                                                       \
        printf("...");                                                            \
      printf("\n");                                                               \
    }                                                                             \
    if (matrix->rows > 5)                                                         \
      printf("  ... (%zu more rows)\n", matrix->rows - 5);                        \
  } while (0)

#define DEBUG_PRINT_ARRAY(array, name)                              \
  do                                                                \
  {                                                                 \
    printf("[DEBUG] Array %s (%zu elements): ", name, array->size); \
    for (size_t i = 0; i < array->size && i < 10; i++)              \
    {                                                               \
      printf("%d ", int_array_get(array, i));                       \
    }                                                               \
    if (array->size > 10)                                           \
      printf("... (%zu more)", array->size - 10);                   \
    printf("\n");                                                   \
  } while (0)

// Performance measurement macros with automatic cleanup
#define BENCHMARK_SCOPE(name, code)                                                                         \
  do                                                                                                        \
  {                                                                                                         \
    BENCHMARK_START(name);                                                                                  \
    code;                                                                                                   \
    BENCHMARK_END(name);                                                                                    \
    printf("  %s: %llu ns, %llu cycles\n", #name, BENCHMARK_GET_TIME_NS(name), BENCHMARK_GET_CYCLES(name)); \
  } while (0)

// Memory tracking macros
#define MEMORY_TRACK_START(tracker) \
  memory_tracker_start(tracker)

#define MEMORY_TRACK_UPDATE(tracker) \
  memory_tracker_update(tracker)

#define MEMORY_TRACK_END(tracker) \
  memory_tracker_end(tracker)

#define MEMORY_TRACK_PRINT(tracker) \
  printf("  Memory: peak=%zu, current=%zu\n", memory_tracker_get_peak(tracker), memory_tracker_get_current(tracker))

// Complete pipeline execution macro
#define EXECUTE_COMPLETE_PIPELINE(pipeline_name, dataset, preprocessing, feature_selection, model) \
  do                                                                                               \
  {                                                                                                \
    printf("Executing %s pipeline...\n", pipeline_name);                                           \
    CREATE_STANDARD_PIPELINE(pipeline_name, preprocessing, feature_selection, model);              \
    CHECK_NULL(pipeline, "Failed to create pipeline");                                             \
    double fitness = EXECUTE_PIPELINE(pipeline, dataset);                                          \
    printf("  %s fitness: %.4f\n", pipeline_name, fitness);                                        \
    pipeline_destroy(pipeline);                                                                    \
  } while (0)

// Complete benchmark execution macro
#define EXECUTE_COMPLETE_BENCHMARK(test_name, use_case, dataset, pipeline, iterations)                               \
  do                                                                                                                 \
  {                                                                                                                  \
    printf("Running %s benchmark...\n", test_name);                                                                  \
    DemoBenchmarkResult result = execute_standardized_benchmark(test_name, use_case, dataset, pipeline, iterations); \
    PRINT_BENCHMARK_RESULT(result);                                                                                  \
    printf("  %s status: %s\n", test_name, result.passed ? "PASS" : "FAIL");                                         \
  } while (0)

#endif // DEMO_BENCHMARK_MACROS_H