#include "demo_benchmark_test_factories.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global factory instances
DatasetTestFactory *iris_dataset_factory = NULL;
DatasetTestFactory *boston_dataset_factory = NULL;
DatasetTestFactory *digits_dataset_factory = NULL;
DatasetTestFactory *cancer_dataset_factory = NULL;
DatasetTestFactory *wine_dataset_factory = NULL;

PipelineTestFactory *normalize_rf_pipeline_factory = NULL;
PipelineTestFactory *standardize_lr_pipeline_factory = NULL;
PipelineTestFactory *selectkbest_rf_pipeline_factory = NULL;
PipelineTestFactory *full_pipeline_factory = NULL;

AlgorithmTestFactory *normalize_algorithm_factory = NULL;
AlgorithmTestFactory *standardize_algorithm_factory = NULL;
AlgorithmTestFactory *selectkbest_algorithm_factory = NULL;
AlgorithmTestFactory *randomforest_algorithm_factory = NULL;
AlgorithmTestFactory *linearregression_algorithm_factory = NULL;

BenchmarkTestFactory *iris_benchmark_factory = NULL;
BenchmarkTestFactory *boston_benchmark_factory = NULL;
BenchmarkTestFactory *digits_benchmark_factory = NULL;
BenchmarkTestFactory *cancer_benchmark_factory = NULL;
BenchmarkTestFactory *wine_benchmark_factory = NULL;

TestSuiteFactory *comprehensive_test_suite_factory = NULL;

// Factory creation functions
DatasetTestFactory *create_dataset_test_factory(const char *name, const char *dataset_name,
                                                size_t sample_count, size_t feature_count,
                                                size_t class_count, int is_regression,
                                                double noise_level)
{
  DatasetTestFactory *factory = malloc(sizeof(DatasetTestFactory));
  if (!factory)
    return NULL;

  factory->base.name = strdup(name);
  factory->base.description = "Dataset test factory";
  factory->base.setup_function = NULL;
  factory->base.teardown_function = NULL;
  factory->base.validation_function = NULL;

  factory->dataset_name = strdup(dataset_name);
  factory->sample_count = sample_count;
  factory->feature_count = feature_count;
  factory->class_count = class_count;
  factory->is_regression = is_regression;
  factory->noise_level = noise_level;
  factory->create_function = NULL;
  factory->destroy_function = dataset_destroy;

  return factory;
}

PipelineTestFactory *create_pipeline_test_factory(const char *name, const char *pipeline_name,
                                                  const char *preprocessing, const char *feature_selection,
                                                  const char *model)
{
  PipelineTestFactory *factory = malloc(sizeof(PipelineTestFactory));
  if (!factory)
    return NULL;

  factory->base.name = strdup(name);
  factory->base.description = "Pipeline test factory";
  factory->base.setup_function = NULL;
  factory->base.teardown_function = NULL;
  factory->base.validation_function = NULL;

  factory->pipeline_name = strdup(pipeline_name);
  factory->preprocessing_type = strdup(preprocessing);
  factory->feature_selection_type = strdup(feature_selection);
  factory->model_type = strdup(model);
  factory->create_function = NULL;
  factory->destroy_function = pipeline_destroy;

  return factory;
}

AlgorithmTestFactory *create_algorithm_test_factory(const char *name, const char *algorithm_name,
                                                    const char *category, double min_fitness,
                                                    double max_fitness,
                                                    double (*evaluate_function)(Dataset *, Matrix *, IntArray *))
{
  AlgorithmTestFactory *factory = malloc(sizeof(AlgorithmTestFactory));
  if (!factory)
    return NULL;

  factory->base.name = strdup(name);
  factory->base.description = "Algorithm test factory";
  factory->base.setup_function = NULL;
  factory->base.teardown_function = NULL;
  factory->base.validation_function = NULL;

  factory->algorithm_name = strdup(algorithm_name);
  factory->category = strdup(category);
  factory->min_fitness = min_fitness;
  factory->max_fitness = max_fitness;
  factory->evaluate_function = evaluate_function;
  factory->create_function = NULL;
  factory->destroy_function = algorithm_destroy;

  return factory;
}

BenchmarkTestFactory *create_benchmark_test_factory(const char *name, const char *benchmark_name,
                                                    DemoUseCase use_case, size_t iterations,
                                                    uint64_t target_cycles, double min_fitness,
                                                    double max_fitness)
{
  BenchmarkTestFactory *factory = malloc(sizeof(BenchmarkTestFactory));
  if (!factory)
    return NULL;

  factory->base.name = strdup(name);
  factory->base.description = "Benchmark test factory";
  factory->base.setup_function = NULL;
  factory->base.teardown_function = NULL;
  factory->base.validation_function = NULL;

  factory->benchmark_name = strdup(benchmark_name);
  factory->use_case = use_case;
  factory->iterations = iterations;
  factory->target_cycles = target_cycles;
  factory->min_fitness = min_fitness;
  factory->max_fitness = max_fitness;
  factory->create_function = NULL;
  factory->validate_function = NULL;

  return factory;
}

TestSuiteFactory *create_test_suite_factory(const char *name, const char *suite_name)
{
  TestSuiteFactory *factory = malloc(sizeof(TestSuiteFactory));
  if (!factory)
    return NULL;

  factory->base.name = strdup(name);
  factory->base.description = "Test suite factory";
  factory->base.setup_function = NULL;
  factory->base.teardown_function = NULL;
  factory->base.validation_function = NULL;

  factory->suite_name = strdup(suite_name);
  factory->datasets = NULL;
  factory->dataset_count = 0;
  factory->pipelines = NULL;
  factory->pipeline_count = 0;
  factory->algorithms = NULL;
  factory->algorithm_count = 0;
  factory->benchmarks = NULL;
  factory->benchmark_count = 0;
  factory->create_function = NULL;
  factory->destroy_function = demo_benchmark_suite_destroy;

  return factory;
}

// Factory execution functions
Dataset *execute_dataset_factory(DatasetTestFactory *factory)
{
  if (!factory)
    return NULL;

  printf("🏭 Executing dataset factory: %s\n", factory->base.name);

  Dataset *dataset = create_standardized_dataset(factory->dataset_name, factory->sample_count,
                                                 factory->feature_count, factory->class_count,
                                                 factory->is_regression, factory->noise_level);

  if (dataset)
  {
    printf("✅ Dataset factory %s created dataset successfully\n", factory->base.name);
  }
  else
  {
    printf("❌ Dataset factory %s failed to create dataset\n", factory->base.name);
  }

  return dataset;
}

Pipeline *execute_pipeline_factory(PipelineTestFactory *factory)
{
  if (!factory)
    return NULL;

  printf("🏭 Executing pipeline factory: %s\n", factory->base.name);

  Pipeline *pipeline = create_standardized_pipeline(factory->pipeline_name,
                                                    factory->preprocessing_type,
                                                    factory->feature_selection_type,
                                                    factory->model_type);

  if (pipeline)
  {
    printf("✅ Pipeline factory %s created pipeline successfully\n", factory->base.name);
  }
  else
  {
    printf("❌ Pipeline factory %s failed to create pipeline\n", factory->base.name);
  }

  return pipeline;
}

Algorithm *execute_algorithm_factory(AlgorithmTestFactory *factory)
{
  if (!factory)
    return NULL;

  printf("🏭 Executing algorithm factory: %s\n", factory->base.name);

  Algorithm *algorithm = algorithm_create(factory->algorithm_name, factory->category,
                                          factory->evaluate_function,
                                          factory->min_fitness, factory->max_fitness);

  if (algorithm)
  {
    printf("✅ Algorithm factory %s created algorithm successfully\n", factory->base.name);
  }
  else
  {
    printf("❌ Algorithm factory %s failed to create algorithm\n", factory->base.name);
  }

  return algorithm;
}

DemoBenchmarkResult execute_benchmark_factory(BenchmarkTestFactory *factory)
{
  DemoBenchmarkResult result = {0};

  if (!factory)
    return result;

  printf("🏭 Executing benchmark factory: %s\n", factory->base.name);

  // Create a test dataset for the benchmark
  Dataset *dataset = NULL;
  switch (factory->use_case)
  {
  case DEMO_IRIS_CLASSIFICATION:
    dataset = create_standardized_dataset("Iris", 150, 4, 3, 0, 0.1);
    break;
  case DEMO_BOSTON_REGRESSION:
    dataset = create_standardized_dataset("Boston", 506, 13, 1, 1, 0.1);
    break;
  case DEMO_DIGITS_CLASSIFICATION:
    dataset = create_standardized_dataset("Digits", 1797, 64, 10, 0, 0.1);
    break;
  case DEMO_BREAST_CANCER_CLASSIFICATION:
    dataset = create_standardized_dataset("Cancer", 569, 30, 2, 0, 0.1);
    break;
  case DEMO_WINE_QUALITY_CLASSIFICATION:
    dataset = create_standardized_dataset("Wine", 1599, 11, 7, 0, 0.1);
    break;
  }

  if (!dataset)
  {
    printf("❌ Benchmark factory %s failed to create test dataset\n", factory->base.name);
    return result;
  }

  // Create a test pipeline
  Pipeline *pipeline = create_standardized_pipeline("TestPipeline", "normalize", "", "random_forest");
  if (!pipeline)
  {
    printf("❌ Benchmark factory %s failed to create test pipeline\n", factory->base.name);
    dataset_destroy(dataset);
    return result;
  }

  // Execute benchmark
  result = execute_standardized_benchmark(factory->benchmark_name, factory->use_case,
                                          dataset, pipeline, factory->iterations);

  printf("✅ Benchmark factory %s executed successfully\n", factory->base.name);

  // Cleanup
  pipeline_destroy(pipeline);
  dataset_destroy(dataset);

  return result;
}

DemoBenchmarkSuite *execute_test_suite_factory(TestSuiteFactory *factory)
{
  if (!factory)
    return NULL;

  printf("🏭 Executing test suite factory: %s\n", factory->base.name);

  DemoBenchmarkSuite *suite = demo_benchmark_suite_create(factory->suite_name);
  if (!suite)
  {
    printf("❌ Test suite factory %s failed to create suite\n", factory->base.name);
    return NULL;
  }

  // Add results from all benchmarks
  for (size_t i = 0; i < factory->benchmark_count; i++)
  {
    DemoBenchmarkResult result = execute_benchmark_factory(factory->benchmarks[i]);
    demo_benchmark_suite_add_result(suite, result);
  }

  demo_benchmark_suite_calculate_stats(suite);

  printf("✅ Test suite factory %s executed successfully\n", factory->base.name);

  return suite;
}

// Factory validation functions
int validate_dataset_factory(DatasetTestFactory *factory, Dataset *dataset)
{
  if (!factory || !dataset)
    return 0;

  printf("🔍 Validating dataset factory: %s\n", factory->base.name);

  int valid = 1;

  // Validate dataset properties
  if (dataset->sample_count != factory->sample_count)
  {
    printf("❌ Dataset sample count mismatch: expected %zu, got %zu\n",
           factory->sample_count, dataset->sample_count);
    valid = 0;
  }

  if (dataset->feature_count != factory->feature_count)
  {
    printf("❌ Dataset feature count mismatch: expected %zu, got %zu\n",
           factory->feature_count, dataset->feature_count);
    valid = 0;
  }

  if (dataset->class_count != factory->class_count)
  {
    printf("❌ Dataset class count mismatch: expected %zu, got %zu\n",
           factory->class_count, dataset->class_count);
    valid = 0;
  }

  if (dataset->is_regression != factory->is_regression)
  {
    printf("❌ Dataset regression flag mismatch: expected %d, got %d\n",
           factory->is_regression, dataset->is_regression);
    valid = 0;
  }

  if (valid)
  {
    printf("✅ Dataset factory %s validation passed\n", factory->base.name);
  }
  else
  {
    printf("❌ Dataset factory %s validation failed\n", factory->base.name);
  }

  return valid;
}

int validate_pipeline_factory(PipelineTestFactory *factory, Pipeline *pipeline)
{
  if (!factory || !pipeline)
    return 0;

  printf("🔍 Validating pipeline factory: %s\n", factory->base.name);

  int valid = 1;

  // Validate pipeline has algorithms
  if (pipeline->algorithm_count == 0)
  {
    printf("❌ Pipeline has no algorithms\n");
    valid = 0;
  }

  if (valid)
  {
    printf("✅ Pipeline factory %s validation passed\n", factory->base.name);
  }
  else
  {
    printf("❌ Pipeline factory %s validation failed\n", factory->base.name);
  }

  return valid;
}

int validate_algorithm_factory(AlgorithmTestFactory *factory, Algorithm *algorithm)
{
  if (!factory || !algorithm)
    return 0;

  printf("🔍 Validating algorithm factory: %s\n", factory->base.name);

  int valid = 1;

  // Validate algorithm properties
  if (strcmp(algorithm->name, factory->algorithm_name) != 0)
  {
    printf("❌ Algorithm name mismatch: expected %s, got %s\n",
           factory->algorithm_name, algorithm->name);
    valid = 0;
  }

  if (strcmp(algorithm->category, factory->category) != 0)
  {
    printf("❌ Algorithm category mismatch: expected %s, got %s\n",
           factory->category, algorithm->category);
    valid = 0;
  }

  if (algorithm->min_fitness != factory->min_fitness)
  {
    printf("❌ Algorithm min fitness mismatch: expected %f, got %f\n",
           factory->min_fitness, algorithm->min_fitness);
    valid = 0;
  }

  if (algorithm->max_fitness != factory->max_fitness)
  {
    printf("❌ Algorithm max fitness mismatch: expected %f, got %f\n",
           factory->max_fitness, algorithm->max_fitness);
    valid = 0;
  }

  if (valid)
  {
    printf("✅ Algorithm factory %s validation passed\n", factory->base.name);
  }
  else
  {
    printf("❌ Algorithm factory %s validation failed\n", factory->base.name);
  }

  return valid;
}

int validate_benchmark_factory(BenchmarkTestFactory *factory, DemoBenchmarkResult *result)
{
  if (!factory || !result)
    return 0;

  printf("🔍 Validating benchmark factory: %s\n", factory->base.name);

  int valid = 1;

  // Validate benchmark properties
  if (result->iterations != factory->iterations)
  {
    printf("❌ Benchmark iterations mismatch: expected %zu, got %zu\n",
           factory->iterations, result->iterations);
    valid = 0;
  }

  if (result->avg_cycles_per_iteration > factory->target_cycles)
  {
    printf("❌ Benchmark performance target not met: %llu > %llu\n",
           result->avg_cycles_per_iteration, factory->target_cycles);
    valid = 0;
  }

  if (result->avg_fitness_score < factory->min_fitness ||
      result->avg_fitness_score > factory->max_fitness)
  {
    printf("❌ Benchmark fitness out of range: %f not in [%f, %f]\n",
           result->avg_fitness_score, factory->min_fitness, factory->max_fitness);
    valid = 0;
  }

  if (valid)
  {
    printf("✅ Benchmark factory %s validation passed\n", factory->base.name);
  }
  else
  {
    printf("❌ Benchmark factory %s validation failed\n", factory->base.name);
  }

  return valid;
}

int validate_test_suite_factory(TestSuiteFactory *factory, DemoBenchmarkSuite *suite)
{
  if (!factory || !suite)
    return 0;

  printf("🔍 Validating test suite factory: %s\n", factory->base.name);

  int valid = 1;

  // Validate suite properties
  if (suite->result_count == 0)
  {
    printf("❌ Test suite has no results\n");
    valid = 0;
  }

  if (valid)
  {
    printf("✅ Test suite factory %s validation passed\n", factory->base.name);
  }
  else
  {
    printf("❌ Test suite factory %s validation failed\n", factory->base.name);
  }

  return valid;
}

// Factory cleanup functions
void destroy_dataset_factory(DatasetTestFactory *factory)
{
  if (factory)
  {
    SAFE_FREE(factory->base.name);
    SAFE_FREE(factory->dataset_name);
    free(factory);
  }
}

void destroy_pipeline_factory(PipelineTestFactory *factory)
{
  if (factory)
  {
    SAFE_FREE(factory->base.name);
    SAFE_FREE(factory->pipeline_name);
    SAFE_FREE(factory->preprocessing_type);
    SAFE_FREE(factory->feature_selection_type);
    SAFE_FREE(factory->model_type);
    free(factory);
  }
}

void destroy_algorithm_factory(AlgorithmTestFactory *factory)
{
  if (factory)
  {
    SAFE_FREE(factory->base.name);
    SAFE_FREE(factory->algorithm_name);
    SAFE_FREE(factory->category);
    free(factory);
  }
}

void destroy_benchmark_factory(BenchmarkTestFactory *factory)
{
  if (factory)
  {
    SAFE_FREE(factory->base.name);
    SAFE_FREE(factory->benchmark_name);
    free(factory);
  }
}

void destroy_test_suite_factory(TestSuiteFactory *factory)
{
  if (factory)
  {
    SAFE_FREE(factory->base.name);
    SAFE_FREE(factory->suite_name);
    free(factory);
  }
}

// Factory initialization
void initialize_test_factories(void)
{
  printf("🏭 Initializing test factories...\n");

  // Initialize dataset factories
  iris_dataset_factory = create_dataset_test_factory("IrisDatasetFactory", "Iris", 150, 4, 3, 0, 0.1);
  boston_dataset_factory = create_dataset_test_factory("BostonDatasetFactory", "Boston", 506, 13, 1, 1, 0.1);
  digits_dataset_factory = create_dataset_test_factory("DigitsDatasetFactory", "Digits", 1797, 64, 10, 0, 0.1);
  cancer_dataset_factory = create_dataset_test_factory("CancerDatasetFactory", "Cancer", 569, 30, 2, 0, 0.1);
  wine_dataset_factory = create_dataset_test_factory("WineDatasetFactory", "Wine", 1599, 11, 7, 0, 0.1);

  // Initialize pipeline factories
  normalize_rf_pipeline_factory = create_pipeline_test_factory("NormalizeRFPipelineFactory",
                                                               "NormalizeRF", "normalize", "", "random_forest");
  standardize_lr_pipeline_factory = create_pipeline_test_factory("StandardizeLRPipelineFactory",
                                                                 "StandardizeLR", "standardize", "", "linear_regression");
  selectkbest_rf_pipeline_factory = create_pipeline_test_factory("SelectKBestRFPipelineFactory",
                                                                 "SelectKBestRF", "", "select_k_best", "random_forest");
  full_pipeline_factory = create_pipeline_test_factory("FullPipelineFactory",
                                                       "FullPipeline", "normalize", "select_k_best", "random_forest");

  // Initialize algorithm factories
  normalize_algorithm_factory = create_algorithm_test_factory("NormalizeAlgorithmFactory",
                                                              "Normalize", ALGORITHM_CATEGORY_PREPROCESSING,
                                                              0.7, 0.95, algorithm_normalize_features);
  standardize_algorithm_factory = create_algorithm_test_factory("StandardizeAlgorithmFactory",
                                                                "Standardize", ALGORITHM_CATEGORY_PREPROCESSING,
                                                                0.7, 0.95, algorithm_standardize_features);
  selectkbest_algorithm_factory = create_algorithm_test_factory("SelectKBestAlgorithmFactory",
                                                                "SelectKBest", ALGORITHM_CATEGORY_FEATURE_SELECTION,
                                                                0.7, 0.95, algorithm_select_k_best_features);
  randomforest_algorithm_factory = create_algorithm_test_factory("RandomForestAlgorithmFactory",
                                                                 "RandomForest", ALGORITHM_CATEGORY_CLASSIFICATION,
                                                                 0.7, 0.95, algorithm_random_forest_classifier);
  linearregression_algorithm_factory = create_algorithm_test_factory("LinearRegressionAlgorithmFactory",
                                                                     "LinearRegression", ALGORITHM_CATEGORY_REGRESSION,
                                                                     0.7, 0.95, algorithm_linear_regression);

  // Initialize benchmark factories
  iris_benchmark_factory = create_benchmark_test_factory("IrisBenchmarkFactory", "IrisBenchmark",
                                                         DEMO_IRIS_CLASSIFICATION, 1000, 49, 0.7, 0.95);
  boston_benchmark_factory = create_benchmark_test_factory("BostonBenchmarkFactory", "BostonBenchmark",
                                                           DEMO_BOSTON_REGRESSION, 1000, 49, 0.7, 0.95);
  digits_benchmark_factory = create_benchmark_test_factory("DigitsBenchmarkFactory", "DigitsBenchmark",
                                                           DEMO_DIGITS_CLASSIFICATION, 1000, 49, 0.7, 0.95);
  cancer_benchmark_factory = create_benchmark_test_factory("CancerBenchmarkFactory", "CancerBenchmark",
                                                           DEMO_BREAST_CANCER_CLASSIFICATION, 1000, 49, 0.7, 0.95);
  wine_benchmark_factory = create_benchmark_test_factory("WineBenchmarkFactory", "WineBenchmark",
                                                         DEMO_WINE_QUALITY_CLASSIFICATION, 1000, 49, 0.7, 0.95);

  // Initialize test suite factory
  comprehensive_test_suite_factory = create_test_suite_factory("ComprehensiveTestSuiteFactory", "ComprehensiveTestSuite");

  printf("✅ Test factories initialized successfully\n");
}

void cleanup_test_factories(void)
{
  printf("🧹 Cleaning up test factories...\n");

  // Cleanup dataset factories
  destroy_dataset_factory(iris_dataset_factory);
  destroy_dataset_factory(boston_dataset_factory);
  destroy_dataset_factory(digits_dataset_factory);
  destroy_dataset_factory(cancer_dataset_factory);
  destroy_dataset_factory(wine_dataset_factory);

  // Cleanup pipeline factories
  destroy_pipeline_factory(normalize_rf_pipeline_factory);
  destroy_pipeline_factory(standardize_lr_pipeline_factory);
  destroy_pipeline_factory(selectkbest_rf_pipeline_factory);
  destroy_pipeline_factory(full_pipeline_factory);

  // Cleanup algorithm factories
  destroy_algorithm_factory(normalize_algorithm_factory);
  destroy_algorithm_factory(standardize_algorithm_factory);
  destroy_algorithm_factory(selectkbest_algorithm_factory);
  destroy_algorithm_factory(randomforest_algorithm_factory);
  destroy_algorithm_factory(linearregression_algorithm_factory);

  // Cleanup benchmark factories
  destroy_benchmark_factory(iris_benchmark_factory);
  destroy_benchmark_factory(boston_benchmark_factory);
  destroy_benchmark_factory(digits_benchmark_factory);
  destroy_benchmark_factory(cancer_benchmark_factory);
  destroy_benchmark_factory(wine_benchmark_factory);

  // Cleanup test suite factory
  destroy_test_suite_factory(comprehensive_test_suite_factory);

  printf("✅ Test factories cleaned up successfully\n");
}

// Factory-based test execution
int run_factory_based_tests(void)
{
  printf("🏭 Running factory-based tests...\n");

  initialize_test_factories();

  int total_tests = 0;
  int passed_tests = 0;

  // Test dataset factories
  total_tests += 5;
  if (run_dataset_factory_tests())
    passed_tests += 5;

  // Test pipeline factories
  total_tests += 4;
  if (run_pipeline_factory_tests())
    passed_tests += 4;

  // Test algorithm factories
  total_tests += 5;
  if (run_algorithm_factory_tests())
    passed_tests += 5;

  // Test benchmark factories
  total_tests += 5;
  if (run_benchmark_factory_tests())
    passed_tests += 5;

  // Test test suite factory
  total_tests += 1;
  if (run_test_suite_factory_tests())
    passed_tests += 1;

  cleanup_test_factories();

  printf("🏭 Factory-based test results: %d/%d passed\n", passed_tests, total_tests);

  return passed_tests == total_tests ? 0 : 1;
}

int run_dataset_factory_tests(void)
{
  printf("🏭 Testing dataset factories...\n");

  Dataset *datasets[] = {
      execute_dataset_factory(iris_dataset_factory),
      execute_dataset_factory(boston_dataset_factory),
      execute_dataset_factory(digits_dataset_factory),
      execute_dataset_factory(cancer_dataset_factory),
      execute_dataset_factory(wine_dataset_factory)};

  int all_valid = 1;
  for (int i = 0; i < 5; i++)
  {
    if (!validate_dataset_factory(
            (i == 0) ? iris_dataset_factory : (i == 1) ? boston_dataset_factory
                                          : (i == 2)   ? digits_dataset_factory
                                          : (i == 3)   ? cancer_dataset_factory
                                                       : wine_dataset_factory,
            datasets[i]))
    {
      all_valid = 0;
    }
    if (datasets[i])
      dataset_destroy(datasets[i]);
  }

  return all_valid;
}

int run_pipeline_factory_tests(void)
{
  printf("🏭 Testing pipeline factories...\n");

  Pipeline *pipelines[] = {
      execute_pipeline_factory(normalize_rf_pipeline_factory),
      execute_pipeline_factory(standardize_lr_pipeline_factory),
      execute_pipeline_factory(selectkbest_rf_pipeline_factory),
      execute_pipeline_factory(full_pipeline_factory)};

  int all_valid = 1;
  for (int i = 0; i < 4; i++)
  {
    if (!validate_pipeline_factory(
            (i == 0) ? normalize_rf_pipeline_factory : (i == 1) ? standardize_lr_pipeline_factory
                                                   : (i == 2)   ? selectkbest_rf_pipeline_factory
                                                                : full_pipeline_factory,
            pipelines[i]))
    {
      all_valid = 0;
    }
    if (pipelines[i])
      pipeline_destroy(pipelines[i]);
  }

  return all_valid;
}

int run_algorithm_factory_tests(void)
{
  printf("🏭 Testing algorithm factories...\n");

  Algorithm *algorithms[] = {
      execute_algorithm_factory(normalize_algorithm_factory),
      execute_algorithm_factory(standardize_algorithm_factory),
      execute_algorithm_factory(selectkbest_algorithm_factory),
      execute_algorithm_factory(randomforest_algorithm_factory),
      execute_algorithm_factory(linearregression_algorithm_factory)};

  int all_valid = 1;
  for (int i = 0; i < 5; i++)
  {
    if (!validate_algorithm_factory(
            (i == 0) ? normalize_algorithm_factory : (i == 1) ? standardize_algorithm_factory
                                                 : (i == 2)   ? selectkbest_algorithm_factory
                                                 : (i == 3)   ? randomforest_algorithm_factory
                                                              : linearregression_algorithm_factory,
            algorithms[i]))
    {
      all_valid = 0;
    }
    if (algorithms[i])
      algorithm_destroy(algorithms[i]);
  }

  return all_valid;
}

int run_benchmark_factory_tests(void)
{
  printf("🏭 Testing benchmark factories...\n");

  DemoBenchmarkResult results[] = {
      execute_benchmark_factory(iris_benchmark_factory),
      execute_benchmark_factory(boston_benchmark_factory),
      execute_benchmark_factory(digits_benchmark_factory),
      execute_benchmark_factory(cancer_benchmark_factory),
      execute_benchmark_factory(wine_benchmark_factory)};

  int all_valid = 1;
  for (int i = 0; i < 5; i++)
  {
    if (!validate_benchmark_factory(
            (i == 0) ? iris_benchmark_factory : (i == 1) ? boston_benchmark_factory
                                            : (i == 2)   ? digits_benchmark_factory
                                            : (i == 3)   ? cancer_benchmark_factory
                                                         : wine_benchmark_factory,
            &results[i]))
    {
      all_valid = 0;
    }
  }

  return all_valid;
}

int run_test_suite_factory_tests(void)
{
  printf("🏭 Testing test suite factory...\n");

  DemoBenchmarkSuite *suite = execute_test_suite_factory(comprehensive_test_suite_factory);

  int valid = validate_test_suite_factory(comprehensive_test_suite_factory, suite);

  if (suite)
    demo_benchmark_suite_destroy(suite);

  return valid;
}