#include "demo_benchmark_common.h"
#include "demo_benchmark_framework.h"
#include "../runtime/src/seven_t_runtime.h"

// Template for creating standardized datasets
Dataset *create_standardized_dataset(const char *name, size_t sample_count, size_t feature_count,
                                     size_t class_count, int is_regression, double noise_level)
{
  Dataset *dataset = dataset_create(name, sample_count, feature_count, class_count, is_regression);
  if (!dataset)
    return NULL;

  if (is_regression)
  {
    dataset_fill_regression_data(dataset, noise_level);
  }
  else
  {
    dataset_fill_classification_data(dataset, noise_level);
  }

  return dataset;
}

// Template for creating standardized pipelines
Pipeline *create_standardized_pipeline(const char *name, const char *preprocessing_type,
                                       const char *feature_selection_type, const char *model_type)
{
  Pipeline *pipeline = pipeline_create(name);
  if (!pipeline)
    return NULL;

  // Add preprocessing algorithm
  if (strcmp(preprocessing_type, "normalize") == 0)
  {
    Algorithm *norm_alg = algorithm_create("Normalize", ALGORITHM_CATEGORY_PREPROCESSING,
                                           algorithm_normalize_features, 0.7, 0.95);
    pipeline_add_algorithm(pipeline, norm_alg);
  }
  else if (strcmp(preprocessing_type, "standardize") == 0)
  {
    Algorithm *std_alg = algorithm_create("Standardize", ALGORITHM_CATEGORY_PREPROCESSING,
                                          algorithm_standardize_features, 0.7, 0.95);
    pipeline_add_algorithm(pipeline, std_alg);
  }

  // Add feature selection algorithm
  if (strcmp(feature_selection_type, "select_k_best") == 0)
  {
    Algorithm *select_alg = algorithm_create("SelectKBest", ALGORITHM_CATEGORY_FEATURE_SELECTION,
                                             algorithm_select_k_best_features, 0.7, 0.95);
    pipeline_add_algorithm(pipeline, select_alg);
  }

  // Add model algorithm
  if (strcmp(model_type, "random_forest") == 0)
  {
    Algorithm *rf_alg = algorithm_create("RandomForest", ALGORITHM_CATEGORY_CLASSIFICATION,
                                         algorithm_random_forest_classifier, 0.7, 0.95);
    pipeline_add_algorithm(pipeline, rf_alg);
  }
  else if (strcmp(model_type, "linear_regression") == 0)
  {
    Algorithm *lr_alg = algorithm_create("LinearRegression", ALGORITHM_CATEGORY_REGRESSION,
                                         algorithm_linear_regression, 0.7, 0.95);
    pipeline_add_algorithm(pipeline, lr_alg);
  }

  return pipeline;
}

// Template for standardized benchmark execution
DemoBenchmarkResult execute_standardized_benchmark(const char *test_name, DemoUseCase use_case,
                                                   Dataset *dataset, Pipeline *pipeline,
                                                   size_t iterations)
{
  // Create pipeline result function
  PipelineResult (*pipeline_function)(void) = NULL;

  // Set up pipeline function based on use case
  switch (use_case)
  {
  case DEMO_IRIS_CLASSIFICATION:
    pipeline_function = create_iris_pipeline_function(dataset, pipeline);
    break;
  case DEMO_BOSTON_REGRESSION:
    pipeline_function = create_boston_pipeline_function(dataset, pipeline);
    break;
  case DEMO_DIGITS_CLASSIFICATION:
    pipeline_function = create_digits_pipeline_function(dataset, pipeline);
    break;
  case DEMO_BREAST_CANCER_CLASSIFICATION:
    pipeline_function = create_cancer_pipeline_function(dataset, pipeline);
    break;
  case DEMO_WINE_QUALITY_CLASSIFICATION:
    pipeline_function = create_wine_pipeline_function(dataset, pipeline);
    break;
  }

  if (!pipeline_function)
  {
    DemoBenchmarkResult empty_result = {0};
    return empty_result;
  }

  // Execute benchmark
  DemoBenchmarkResult result = demo_benchmark_execute_single(
      test_name, use_case, iterations, pipeline_function, NULL);

  // Cleanup
  free(pipeline_function);

  return result;
}

// Template for creating pipeline functions
PipelineResult (*create_iris_pipeline_function(Dataset *dataset, Pipeline *pipeline))(void)
{
  return create_generic_pipeline_function(dataset, pipeline, "Iris");
}

PipelineResult (*create_boston_pipeline_function(Dataset *dataset, Pipeline *pipeline))(void)
{
  return create_generic_pipeline_function(dataset, pipeline, "Boston");
}

PipelineResult (*create_digits_pipeline_function(Dataset *dataset, Pipeline *pipeline))(void)
{
  return create_generic_pipeline_function(dataset, pipeline, "Digits");
}

PipelineResult (*create_cancer_pipeline_function(Dataset *dataset, Pipeline *pipeline))(void)
{
  return create_generic_pipeline_function(dataset, pipeline, "Cancer");
}

PipelineResult (*create_wine_pipeline_function(Dataset *dataset, Pipeline *pipeline))(void)
{
  return create_generic_pipeline_function(dataset, pipeline, "Wine");
}

// Generic pipeline function creator
PipelineResult (*create_generic_pipeline_function(Dataset *dataset, Pipeline *pipeline, const char *type))(void)
{
  // This is a simplified version - in practice, you'd create a closure-like structure
  // For now, we'll return a function that uses global variables
  static Dataset *global_dataset = NULL;
  static Pipeline *global_pipeline = NULL;

  global_dataset = dataset;
  global_pipeline = pipeline;

  return &generic_pipeline_executor;
}

// Generic pipeline executor
PipelineResult generic_pipeline_executor(void)
{
  static Dataset *global_dataset = NULL;
  static Pipeline *global_pipeline = NULL;

  if (!global_dataset || !global_pipeline)
  {
    PipelineResult empty_result = {0};
    return empty_result;
  }

  // Create pipeline result
  PipelineResult *pipeline = pipeline_result_create(DEMO_IRIS_CLASSIFICATION); // Default
  if (!pipeline)
  {
    PipelineResult empty_result = {0};
    return empty_result;
  }

  // Execute pipeline steps
  for (size_t i = 0; i < global_pipeline->algorithm_count; i++)
  {
    Algorithm *algorithm = &global_pipeline->algorithms[i];

    // Create step result
    PipelineStepResult *step = pipeline_step_result_create(
        get_step_type_from_category(algorithm->category), algorithm->name);

    if (step)
    {
      // Execute algorithm
      CommonTimer timer;
      common_timer_start(&timer, algorithm->name);

      // Create dummy matrices for algorithm execution
      Matrix selected_features = {0};
      IntArray selected_samples = {0};

      double fitness = algorithm->evaluate_function(global_dataset, &selected_features, &selected_samples);

      common_timer_end(&timer);

      // Fill step result
      step->execution_time_ns = common_timer_get_time_ns(&timer);
      step->execution_cycles = common_timer_get_cycles(&timer);
      step->fitness_score = fitness;
      step->samples_processed = global_dataset->sample_count;
      step->features_processed = global_dataset->feature_count;
      step->success = 1;

      pipeline_result_add_step(pipeline, step);
      pipeline_step_result_destroy(step);
    }
  }

  pipeline_result_calculate_stats(pipeline);
  PipelineResult result = *pipeline;
  pipeline_result_destroy(pipeline);

  return result;
}

// Helper function to convert algorithm category to step type
PipelineStepType get_step_type_from_category(const char *category)
{
  if (strcmp(category, ALGORITHM_CATEGORY_PREPROCESSING) == 0)
  {
    return PIPELINE_STEP_PREPROCESSING;
  }
  else if (strcmp(category, ALGORITHM_CATEGORY_FEATURE_SELECTION) == 0)
  {
    return PIPELINE_STEP_FEATURE_SELECTION;
  }
  else if (strcmp(category, ALGORITHM_CATEGORY_CLASSIFICATION) == 0 ||
           strcmp(category, ALGORITHM_CATEGORY_REGRESSION) == 0)
  {
    return PIPELINE_STEP_MODEL_TRAINING;
  }
  else
  {
    return PIPELINE_STEP_EVALUATION;
  }
}

// Template for creating standardized benchmark suites
DemoBenchmarkSuite *create_standardized_benchmark_suite(const char *suite_name,
                                                        Dataset **datasets, size_t dataset_count,
                                                        Pipeline **pipelines, size_t pipeline_count)
{
  DemoBenchmarkSuite *suite = demo_benchmark_suite_create(suite_name);
  if (!suite)
    return NULL;

  // Create benchmark tests for each dataset-pipeline combination
  for (size_t i = 0; i < dataset_count; i++)
  {
    for (size_t j = 0; j < pipeline_count; j++)
    {
      char test_name[256];
      snprintf(test_name, sizeof(test_name), "%s_%s_Pipeline",
               datasets[i]->name, pipelines[j]->name);

      DemoUseCase use_case = get_use_case_from_dataset_name(datasets[i]->name);

      DemoBenchmarkResult result = execute_standardized_benchmark(
          test_name, use_case, datasets[i], pipelines[j], 1000);

      demo_benchmark_suite_add_result(suite, result);
    }
  }

  demo_benchmark_suite_calculate_stats(suite);
  return suite;
}

// Helper function to get use case from dataset name
DemoUseCase get_use_case_from_dataset_name(const char *name)
{
  if (strstr(name, "iris") || strstr(name, "Iris"))
  {
    return DEMO_IRIS_CLASSIFICATION;
  }
  else if (strstr(name, "boston") || strstr(name, "Boston"))
  {
    return DEMO_BOSTON_REGRESSION;
  }
  else if (strstr(name, "digits") || strstr(name, "Digits"))
  {
    return DEMO_DIGITS_CLASSIFICATION;
  }
  else if (strstr(name, "cancer") || strstr(name, "Cancer"))
  {
    return DEMO_BREAST_CANCER_CLASSIFICATION;
  }
  else if (strstr(name, "wine") || strstr(name, "Wine"))
  {
    return DEMO_WINE_QUALITY_CLASSIFICATION;
  }
  else
  {
    return DEMO_IRIS_CLASSIFICATION; // Default
  }
}

// Template for creating all standard datasets
Dataset **create_all_standard_datasets(size_t *count)
{
  *count = 5;
  Dataset **datasets = malloc(*count * sizeof(Dataset *));
  if (!datasets)
    return NULL;

  datasets[0] = create_standardized_dataset("Iris", 150, 4, 3, 0, 0.1);
  datasets[1] = create_standardized_dataset("Boston", 506, 13, 1, 1, 0.1);
  datasets[2] = create_standardized_dataset("Digits", 1797, 64, 10, 0, 0.1);
  datasets[3] = create_standardized_dataset("Cancer", 569, 30, 2, 0, 0.1);
  datasets[4] = create_standardized_dataset("Wine", 1599, 11, 7, 0, 0.1);

  return datasets;
}

// Template for creating all standard pipelines
Pipeline **create_all_standard_pipelines(size_t *count)
{
  *count = 6;
  Pipeline **pipelines = malloc(*count * sizeof(Pipeline *));
  if (!pipelines)
    return NULL;

  pipelines[0] = create_standardized_pipeline("Normalize_SelectKBest_RandomForest",
                                              "normalize", "select_k_best", "random_forest");
  pipelines[1] = create_standardized_pipeline("Standardize_SelectKBest_LinearRegression",
                                              "standardize", "select_k_best", "linear_regression");
  pipelines[2] = create_standardized_pipeline("Normalize_RandomForest",
                                              "normalize", "", "random_forest");
  pipelines[3] = create_standardized_pipeline("Standardize_LinearRegression",
                                              "standardize", "", "linear_regression");
  pipelines[4] = create_standardized_pipeline("SelectKBest_RandomForest",
                                              "", "select_k_best", "random_forest");
  pipelines[5] = create_standardized_pipeline("RandomForest_Only",
                                              "", "", "random_forest");

  return pipelines;
}

// Template for running comprehensive benchmark suite
int run_comprehensive_benchmark_suite(const char *suite_name)
{
  printf("=== Running Comprehensive Benchmark Suite: %s ===\n", suite_name);

  // Create all datasets
  size_t dataset_count;
  Dataset **datasets = create_all_standard_datasets(&dataset_count);
  if (!datasets)
  {
    printf("❌ Failed to create datasets\n");
    return 1;
  }

  // Create all pipelines
  size_t pipeline_count;
  Pipeline **pipelines = create_all_standard_pipelines(&pipeline_count);
  if (!pipelines)
  {
    printf("❌ Failed to create pipelines\n");
    // Cleanup datasets
    for (size_t i = 0; i < dataset_count; i++)
    {
      dataset_destroy(datasets[i]);
    }
    free(datasets);
    return 1;
  }

  // Create and run benchmark suite
  DemoBenchmarkSuite *suite = create_standardized_benchmark_suite(
      suite_name, datasets, dataset_count, pipelines, pipeline_count);

  if (suite)
  {
    // Print results
    demo_benchmark_suite_print_summary(suite);

    // Save results
    demo_benchmark_suite_export_json(suite, "comprehensive_benchmark_results.json");
    demo_benchmark_suite_export_csv(suite, "comprehensive_benchmark_results.csv");

    // Cleanup
    demo_benchmark_suite_destroy(suite);
  }

  // Cleanup
  for (size_t i = 0; i < dataset_count; i++)
  {
    dataset_destroy(datasets[i]);
  }
  free(datasets);

  for (size_t i = 0; i < pipeline_count; i++)
  {
    pipeline_destroy(pipelines[i]);
  }
  free(pipelines);

  printf("✅ Comprehensive benchmark suite completed\n");
  return 0;
}

// Template for performance regression testing
int run_performance_regression_test(const char *baseline_file, const char *current_file)
{
  printf("=== Running Performance Regression Test ===\n");

  // Load baseline results
  DemoBenchmarkSuite *baseline_suite = load_benchmark_suite_from_file(baseline_file);
  if (!baseline_suite)
  {
    printf("❌ Failed to load baseline results\n");
    return 1;
  }

  // Run current benchmark
  DemoBenchmarkSuite *current_suite = demo_benchmark_suite_create("Current");
  if (!current_suite)
  {
    printf("❌ Failed to create current suite\n");
    demo_benchmark_suite_destroy(baseline_suite);
    return 1;
  }

  // Run current benchmarks and add to suite
  // (This would be implemented based on your specific needs)

  // Compare results
  int regression_detected = 0;
  double total_regression = 0.0;

  for (size_t i = 0; i < baseline_suite->result_count && i < current_suite->result_count; i++)
  {
    DemoBenchmarkResult *baseline = &baseline_suite->results[i];
    DemoBenchmarkResult *current = &current_suite->results[i];

    double performance_change = ((current->avg_cycles_per_iteration - baseline->avg_cycles_per_iteration) /
                                 baseline->avg_cycles_per_iteration) *
                                100.0;

    if (performance_change > 10.0)
    { // 10% regression threshold
      printf("⚠️  Performance regression in %s: %.1f%% increase\n",
             current->test_name, performance_change);
      regression_detected = 1;
      total_regression += performance_change;
    }
  }

  if (regression_detected)
  {
    printf("❌ Performance regression detected: %.1f%% average increase\n",
           total_regression / baseline_suite->result_count);
  }
  else
  {
    printf("✅ No performance regression detected\n");
  }

  // Cleanup
  demo_benchmark_suite_destroy(baseline_suite);
  demo_benchmark_suite_destroy(current_suite);

  return regression_detected;
}

// Helper function to load benchmark suite from file (placeholder)
DemoBenchmarkSuite *load_benchmark_suite_from_file(const char *filename)
{
  // This would implement loading from JSON/CSV file
  // For now, return NULL
  return NULL;
}