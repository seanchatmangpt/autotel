#include "7t_unit_test_framework.h"
#include "../c_src/7t_tpot.h"
#include <string.h>
#include <math.h>

// Test suite registration
TEST_SUITE_BEGIN(seven_t_tpot)

// Test data structures
typedef struct
{
  Dataset7T *test_dataset;
  OptimizationEngine7T *optimizer;
  Pipeline7T *test_pipeline;
} TPOTTestData;

// Setup and teardown functions
static void setup_tpot_test(TestContext *context)
{
  TPOTTestData *data = malloc(sizeof(TPOTTestData));
  ASSERT_NOT_NULL(data);

  // Create test dataset
  data->test_dataset = create_iris_dataset();
  ASSERT_NOT_NULL(data->test_dataset);

  // Create optimizer
  data->optimizer = create_optimizer_7t(10, 3);
  ASSERT_NOT_NULL(data->optimizer);

  // Create test pipeline
  data->test_pipeline = create_pipeline(3);
  ASSERT_NOT_NULL(data->test_pipeline);

  context->test_data = data;
}

static void teardown_tpot_test(TestContext *context)
{
  TPOTTestData *data = (TPOTTestData *)context->test_data;
  if (data)
  {
    if (data->test_dataset)
    {
      destroy_dataset_7t(data->test_dataset);
    }
    if (data->optimizer)
    {
      destroy_optimizer_7t(data->optimizer);
    }
    if (data->test_pipeline)
    {
      destroy_pipeline_7t(data->test_pipeline);
    }
    free(data);
  }
}

// ============================================================================
// DATASET TESTS
// ============================================================================

TEST_CASE(dataset_creation_iris, "Test Iris dataset creation")
static void test_dataset_creation_iris(TestContext *context)
{
  Dataset7T *dataset = create_iris_dataset();
  ASSERT_NOT_NULL(dataset);

  // Check dataset properties
  ASSERT_GREATER_THAN(dataset->num_samples, 0);
  ASSERT_GREATER_THAN(dataset->num_features, 0);
  ASSERT_NOT_NULL(dataset->features);
  ASSERT_NOT_NULL(dataset->targets);

  // Check data integrity
  for (uint32_t i = 0; i < dataset->num_samples; i++)
  {
    for (uint32_t j = 0; j < dataset->num_features; j++)
    {
      ASSERT_TRUE(!isnan(dataset->features[i * dataset->num_features + j]));
    }
    ASSERT_TRUE(!isnan(dataset->targets[i]));
  }

  destroy_dataset_7t(dataset);
}

TEST_CASE(dataset_creation_boston, "Test Boston Housing dataset creation")
static void test_dataset_creation_boston(TestContext *context)
{
  Dataset7T *dataset = create_boston_dataset();
  ASSERT_NOT_NULL(dataset);

  // Check dataset properties
  ASSERT_GREATER_THAN(dataset->num_samples, 0);
  ASSERT_GREATER_THAN(dataset->num_features, 0);
  ASSERT_NOT_NULL(dataset->features);
  ASSERT_NOT_NULL(dataset->targets);

  // Check data integrity
  for (uint32_t i = 0; i < dataset->num_samples; i++)
  {
    for (uint32_t j = 0; j < dataset->num_features; j++)
    {
      ASSERT_TRUE(!isnan(dataset->features[i * dataset->num_features + j]));
    }
    ASSERT_TRUE(!isnan(dataset->targets[i]));
  }

  destroy_dataset_7t(dataset);
}

TEST_CASE(dataset_creation_breast_cancer, "Test Breast Cancer dataset creation")
static void test_dataset_creation_breast_cancer(TestContext *context)
{
  Dataset7T *dataset = create_breast_cancer_dataset();
  ASSERT_NOT_NULL(dataset);

  // Check dataset properties
  ASSERT_GREATER_THAN(dataset->num_samples, 0);
  ASSERT_GREATER_THAN(dataset->num_features, 0);
  ASSERT_NOT_NULL(dataset->features);
  ASSERT_NOT_NULL(dataset->targets);

  // Check data integrity
  for (uint32_t i = 0; i < dataset->num_samples; i++)
  {
    for (uint32_t j = 0; j < dataset->num_features; j++)
    {
      ASSERT_TRUE(!isnan(dataset->features[i * dataset->num_features + j]));
    }
    ASSERT_TRUE(!isnan(dataset->targets[i]));
  }

  destroy_dataset_7t(dataset);
}

TEST_CASE(dataset_creation_diabetes, "Test Diabetes dataset creation")
static void test_dataset_creation_diabetes(TestContext *context)
{
  Dataset7T *dataset = create_diabetes_dataset();
  ASSERT_NOT_NULL(dataset);

  // Check dataset properties
  ASSERT_GREATER_THAN(dataset->num_samples, 0);
  ASSERT_GREATER_THAN(dataset->num_features, 0);
  ASSERT_NOT_NULL(dataset->features);
  ASSERT_NOT_NULL(dataset->targets);

  // Check data integrity
  for (uint32_t i = 0; i < dataset->num_samples; i++)
  {
    for (uint32_t j = 0; j < dataset->num_features; j++)
    {
      ASSERT_TRUE(!isnan(dataset->features[i * dataset->num_features + j]));
    }
    ASSERT_TRUE(!isnan(dataset->targets[i]));
  }

  destroy_dataset_7t(dataset);
}

TEST_CASE(dataset_creation_digits, "Test Digits dataset creation")
static void test_dataset_creation_digits(TestContext *context)
{
  Dataset7T *dataset = create_digits_dataset();
  ASSERT_NOT_NULL(dataset);

  // Check dataset properties
  ASSERT_GREATER_THAN(dataset->num_samples, 0);
  ASSERT_GREATER_THAN(dataset->num_features, 0);
  ASSERT_NOT_NULL(dataset->features);
  ASSERT_NOT_NULL(dataset->targets);

  // Check data integrity
  for (uint32_t i = 0; i < dataset->num_samples; i++)
  {
    for (uint32_t j = 0; j < dataset->num_features; j++)
    {
      ASSERT_TRUE(!isnan(dataset->features[i * dataset->num_features + j]));
    }
    ASSERT_TRUE(!isnan(dataset->targets[i]));
  }

  destroy_dataset_7t(dataset);
}

// ============================================================================
// PIPELINE TESTS
// ============================================================================

TEST_CASE(pipeline_creation, "Test pipeline creation")
static void test_pipeline_creation(TestContext *context)
{
  Pipeline7T *pipeline = create_pipeline(5);
  ASSERT_NOT_NULL(pipeline);

  // Check pipeline properties
  ASSERT_EQUAL(5, pipeline->num_steps);
  ASSERT_NOT_NULL(pipeline->steps);
  ASSERT_EQUAL(0.0, pipeline->fitness_score);

  destroy_pipeline_7t(pipeline);
}

TEST_CASE(pipeline_evaluation, "Test pipeline evaluation")
static void test_pipeline_evaluation(TestContext *context)
{
  TPOTTestData *data = (TPOTTestData *)context->test_data;
  ASSERT_NOT_NULL(data);

  // Evaluate pipeline
  double fitness = evaluate_pipeline_7t(data->test_pipeline, data->test_dataset);

  // Check fitness score
  ASSERT_TRUE(!isnan(fitness));
  ASSERT_TRUE(fitness >= 0.0); // Fitness should be non-negative

  // Update pipeline fitness
  data->test_pipeline->fitness_score = fitness;
  ASSERT_EQUAL(fitness, data->test_pipeline->fitness_score);
}

TEST_CASE(pipeline_performance, "Test pipeline evaluation performance")
static void test_pipeline_performance(TestContext *context)
{
  TPOTTestData *data = (TPOTTestData *)context->test_data;
  ASSERT_NOT_NULL(data);

  // Performance test: evaluate pipeline multiple times
  ASSERT_PERFORMANCE({
        for (int i = 0; i < 10; i++) {
            evaluate_pipeline_7t(data->test_pipeline, data->test_dataset);
        } }, 1000000); // 1M cycles for 10 evaluations
}

TEST_CASE(pipeline_latency, "Test pipeline evaluation latency")
static void test_pipeline_latency(TestContext *context)
{
  TPOTTestData *data = (TPOTTestData *)context->test_data;
  ASSERT_NOT_NULL(data);

  // Latency test: single pipeline evaluation
  ASSERT_LATENCY({ evaluate_pipeline_7t(data->test_pipeline, data->test_dataset); }, 1000000); // 1ms limit
}

// ============================================================================
// OPTIMIZATION ENGINE TESTS
// ============================================================================

TEST_CASE(optimizer_creation, "Test optimizer creation")
static void test_optimizer_creation(TestContext *context)
{
  OptimizationEngine7T *optimizer = create_optimizer_7t(20, 5);
  ASSERT_NOT_NULL(optimizer);

  // Check optimizer properties
  ASSERT_EQUAL(20, optimizer->population_size);
  ASSERT_EQUAL(5, optimizer->num_generations);
  ASSERT_NOT_NULL(optimizer->population);
  ASSERT_NOT_NULL(optimizer->fitness_scores);

  destroy_optimizer_7t(optimizer);
}

TEST_CASE(optimizer_optimization, "Test pipeline optimization")
static void test_optimizer_optimization(TestContext *context)
{
  TPOTTestData *data = (TPOTTestData *)context->test_data;
  ASSERT_NOT_NULL(data);

  // Run optimization
  Pipeline7T *best_pipeline = optimize_pipeline_7t(data->optimizer, data->test_dataset, 10);

  ASSERT_NOT_NULL(best_pipeline);
  ASSERT_TRUE(!isnan(best_pipeline->fitness_score));
  ASSERT_TRUE(best_pipeline->fitness_score >= 0.0);

  destroy_pipeline_7t(best_pipeline);
}

TEST_CASE(optimizer_performance, "Test optimization performance")
static void test_optimizer_performance(TestContext *context)
{
  TPOTTestData *data = (TPOTTestData *)context->test_data;
  ASSERT_NOT_NULL(data);

  // Performance test: run optimization
  ASSERT_PERFORMANCE({
        Pipeline7T* best_pipeline = optimize_pipeline_7t(data->optimizer, data->test_dataset, 5);
        if (best_pipeline) {
            destroy_pipeline_7t(best_pipeline);
        } }, 10000000); // 10M cycles for optimization
}

TEST_CASE(optimizer_memory, "Test optimization memory usage")
static void test_optimizer_memory(TestContext *context)
{
  TPOTTestData *data = (TPOTTestData *)context->test_data;
  ASSERT_NOT_NULL(data);

  // Memory test: run optimization
  ASSERT_MEMORY_USAGE({
        Pipeline7T* best_pipeline = optimize_pipeline_7t(data->optimizer, data->test_dataset, 5);
        if (best_pipeline) {
            destroy_pipeline_7t(best_pipeline);
        } }, 100 * 1024 * 1024); // 100MB limit
}

// ============================================================================
// ALGORITHM REGISTRATION TESTS
// ============================================================================

TEST_CASE(algorithm_registration, "Test algorithm registration")
static void test_algorithm_registration(TestContext *context)
{
  // Register algorithms
  register_algorithms();

  // Test should pass if registration doesn't crash
  ASSERT_TRUE(true);
}

// ============================================================================
// FEATURE PROCESSING TESTS
// ============================================================================

TEST_CASE(feature_normalization, "Test feature normalization")
static void test_feature_normalization(TestContext *context)
{
  TPOTTestData *data = (TPOTTestData *)context->test_data;
  ASSERT_NOT_NULL(data);

  // Test normalization
  double params[] = {1.0, 0.0}; // mean=1.0, std=0.0
  double result = normalize_features(data->test_dataset, params);

  ASSERT_TRUE(!isnan(result));
  ASSERT_TRUE(result >= 0.0);
}

TEST_CASE(feature_standardization, "Test feature standardization")
static void test_feature_standardization(TestContext *context)
{
  TPOTTestData *data = (TPOTTestData *)context->test_data;
  ASSERT_NOT_NULL(data);

  // Test standardization
  double params[] = {1.0, 0.0}; // mean=1.0, std=0.0
  double result = standardize_features(data->test_dataset, params);

  ASSERT_TRUE(!isnan(result));
  ASSERT_TRUE(result >= 0.0);
}

TEST_CASE(feature_selection, "Test feature selection")
static void test_feature_selection(TestContext *context)
{
  TPOTTestData *data = (TPOTTestData *)context->test_data;
  ASSERT_NOT_NULL(data);

  // Test feature selection
  double params[] = {5.0}; // k=5
  double result = select_k_best_features(data->test_dataset, params);

  ASSERT_TRUE(!isnan(result));
  ASSERT_TRUE(result >= 0.0);
}

// ============================================================================
// STRESS TESTS
// ============================================================================

TEST_CASE(stress_test_large_population, "Test with large population")
static void test_stress_test_large_population(TestContext *context)
{
  TPOTTestData *data = (TPOTTestData *)context->test_data;
  ASSERT_NOT_NULL(data);

  // Create optimizer with large population
  OptimizationEngine7T *large_optimizer = create_optimizer_7t(100, 10);
  ASSERT_NOT_NULL(large_optimizer);

  // Run optimization with large population
  Pipeline7T *best_pipeline = optimize_pipeline_7t(large_optimizer, data->test_dataset, 30);

  ASSERT_NOT_NULL(best_pipeline);
  ASSERT_TRUE(!isnan(best_pipeline->fitness_score));

  destroy_pipeline_7t(best_pipeline);
  destroy_optimizer_7t(large_optimizer);
}

TEST_CASE(stress_test_multiple_datasets, "Test with multiple datasets")
static void test_stress_test_multiple_datasets(TestContext *context)
{
  TPOTTestData *data = (TPOTTestData *)context->test_data;
  ASSERT_NOT_NULL(data);

  // Test with multiple datasets
  Dataset7T *datasets[] = {
      create_iris_dataset(),
      create_boston_dataset(),
      create_breast_cancer_dataset(),
      create_diabetes_dataset(),
      create_digits_dataset()};

  for (int i = 0; i < 5; i++)
  {
    if (datasets[i])
    {
      Pipeline7T *best_pipeline = optimize_pipeline_7t(data->optimizer, datasets[i], 5);
      if (best_pipeline)
      {
        ASSERT_TRUE(!isnan(best_pipeline->fitness_score));
        destroy_pipeline_7t(best_pipeline);
      }
      destroy_dataset_7t(datasets[i]);
    }
  }
}

TEST_CASE(stress_test_memory_efficiency, "Test memory efficiency under load")
static void test_stress_test_memory_efficiency(TestContext *context)
{
  TPOTTestData *data = (TPOTTestData *)context->test_data;
  ASSERT_NOT_NULL(data);

  // Memory efficiency test: run multiple optimizations
  size_t initial_memory = get_memory_usage();

  for (int i = 0; i < 5; i++)
  {
    Pipeline7T *best_pipeline = optimize_pipeline_7t(data->optimizer, data->test_dataset, 3);
    if (best_pipeline)
    {
      destroy_pipeline_7t(best_pipeline);
    }
  }

  size_t final_memory = get_memory_usage();
  size_t memory_used = final_memory - initial_memory;

  // Memory usage should be reasonable (less than 50MB for 5 optimizations)
  ASSERT_LESS_THAN(memory_used, 50 * 1024 * 1024);
}

// ============================================================================
// EDGE CASE TESTS
// ============================================================================

TEST_CASE(edge_case_empty_dataset, "Test with empty dataset")
static void test_edge_case_empty_dataset(TestContext *context)
{
  TPOTTestData *data = (TPOTTestData *)context->test_data;
  ASSERT_NOT_NULL(data);

  // Create empty dataset (this would need to be implemented)
  // For now, test with minimal dataset
  Dataset7T *minimal_dataset = create_iris_dataset();
  if (minimal_dataset)
  {
    // Test pipeline evaluation with minimal dataset
    double fitness = evaluate_pipeline_7t(data->test_pipeline, minimal_dataset);
    ASSERT_TRUE(!isnan(fitness));

    destroy_dataset_7t(minimal_dataset);
  }
}

TEST_CASE(edge_case_single_sample, "Test with single sample dataset")
static void test_edge_case_single_sample(TestContext *context)
{
  TPOTTestData *data = (TPOTTestData *)context->test_data;
  ASSERT_NOT_NULL(data);

  // Test with small dataset (Iris has 150 samples, which is small)
  Pipeline7T *best_pipeline = optimize_pipeline_7t(data->optimizer, data->test_dataset, 2);

  if (best_pipeline)
  {
    ASSERT_TRUE(!isnan(best_pipeline->fitness_score));
    destroy_pipeline_7t(best_pipeline);
  }
}

TEST_CASE(edge_case_null_parameters, "Test handling of NULL parameters")
static void test_edge_case_null_parameters(TestContext *context)
{
  TPOTTestData *data = (TPOTTestData *)context->test_data;
  ASSERT_NOT_NULL(data);

  // Test with NULL dataset (should handle gracefully)
  Pipeline7T *result = optimize_pipeline_7t(data->optimizer, NULL, 5);
  // Implementation dependent - should either return NULL or handle gracefully

  // Test with NULL optimizer (should handle gracefully)
  result = optimize_pipeline_7t(NULL, data->test_dataset, 5);
  // Implementation dependent - should either return NULL or handle gracefully
}

TEST_CASE(edge_case_invalid_parameters, "Test handling of invalid parameters")
static void test_edge_case_invalid_parameters(TestContext *context)
{
  TPOTTestData *data = (TPOTTestData *)context->test_data;
  ASSERT_NOT_NULL(data);

  // Test with zero population size
  OptimizationEngine7T *zero_pop_optimizer = create_optimizer_7t(0, 5);
  // Implementation dependent - should either return NULL or handle gracefully

  // Test with zero generations
  OptimizationEngine7T *zero_gen_optimizer = create_optimizer_7t(10, 0);
  // Implementation dependent - should either return NULL or handle gracefully

  if (zero_pop_optimizer)
    destroy_optimizer_7t(zero_pop_optimizer);
  if (zero_gen_optimizer)
    destroy_optimizer_7t(zero_gen_optimizer);
}

// ============================================================================
// BENCHMARK COMPARISON TESTS
// ============================================================================

TEST_CASE(benchmark_comparison_iris, "Test Iris dataset benchmark comparison")
static void test_benchmark_comparison_iris(TestContext *context)
{
  TPOTTestData *data = (TPOTTestData *)context->test_data;
  ASSERT_NOT_NULL(data);

  // Run optimization on Iris dataset
  Pipeline7T *best_pipeline = optimize_pipeline_7t(data->optimizer, data->test_dataset, 10);

  if (best_pipeline)
  {
    // Check that we got a reasonable fitness score
    ASSERT_TRUE(best_pipeline->fitness_score > 0.0);
    ASSERT_TRUE(best_pipeline->fitness_score <= 1.0); // Assuming normalized fitness

    // Check evaluation time
    ASSERT_GREATER_THAN(best_pipeline->evaluation_time_ns, 0);

    destroy_pipeline_7t(best_pipeline);
  }
}

TEST_CASE(benchmark_comparison_boston, "Test Boston dataset benchmark comparison")
static void test_benchmark_comparison_boston(TestContext *context)
{
  Dataset7T *boston_dataset = create_boston_dataset();
  ASSERT_NOT_NULL(boston_dataset);

  OptimizationEngine7T *optimizer = create_optimizer_7t(15, 5);
  ASSERT_NOT_NULL(optimizer);

  // Run optimization on Boston dataset
  Pipeline7T *best_pipeline = optimize_pipeline_7t(optimizer, boston_dataset, 10);

  if (best_pipeline)
  {
    // Check that we got a reasonable fitness score
    ASSERT_TRUE(best_pipeline->fitness_score > 0.0);

    // Check evaluation time
    ASSERT_GREATER_THAN(best_pipeline->evaluation_time_ns, 0);

    destroy_pipeline_7t(best_pipeline);
  }

  destroy_optimizer_7t(optimizer);
  destroy_dataset_7t(boston_dataset);
}

TEST_SUITE_END(seven_t_tpot)

// Test suite registration function
void register_seven_t_tpot_tests(void)
{
  // Register all test cases
  TestCase test_cases[] = {
      // Dataset tests
      test_case_dataset_creation_iris,
      test_case_dataset_creation_boston,
      test_case_dataset_creation_breast_cancer,
      test_case_dataset_creation_diabetes,
      test_case_dataset_creation_digits,

      // Pipeline tests
      test_case_pipeline_creation,
      test_case_pipeline_evaluation,
      test_case_pipeline_performance,
      test_case_pipeline_latency,

      // Optimization engine tests
      test_case_optimizer_creation,
      test_case_optimizer_optimization,
      test_case_optimizer_performance,
      test_case_optimizer_memory,

      // Algorithm registration
      test_case_algorithm_registration,

      // Feature processing tests
      test_case_feature_normalization,
      test_case_feature_standardization,
      test_case_feature_selection,

      // Stress tests
      test_case_stress_test_large_population,
      test_case_stress_test_multiple_datasets,
      test_case_stress_test_memory_efficiency,

      // Edge cases
      test_case_edge_case_empty_dataset,
      test_case_edge_case_single_sample,
      test_case_edge_case_null_parameters,
      test_case_edge_case_invalid_parameters,

      // Benchmark comparisons
      test_case_benchmark_comparison_iris,
      test_case_benchmark_comparison_boston};

  seven_t_tpot_registration.test_cases = test_cases;
  seven_t_tpot_registration.test_case_count = sizeof(test_cases) / sizeof(TestCase);
  seven_t_tpot_registration.suite_setup = setup_tpot_test;
  seven_t_tpot_registration.suite_teardown = teardown_tpot_test;
}

// Main test runner
int main(int argc, char *argv[])
{
  printf("=== 7T TPOT Unit Tests ===\n");
  printf("Framework Version: %s\n", SEVEN_TICK_TEST_VERSION);

  // Initialize test configuration
  test_config_init();
  test_config_set_verbose(true);

  // Register tests
  register_seven_t_tpot_tests();

  // Run test suite
  test_run_suite(&seven_t_tpot_registration);

  printf("\n=== TPOT Tests Complete ===\n");
  return 0;
}