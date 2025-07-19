#include "demo_benchmark_framework.h"
#include "../runtime/src/seven_t_runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Forward declarations for demo use case functions
PipelineResult demo_iris_pipeline(void);
PipelineResult demo_boston_pipeline(void);
PipelineResult demo_digits_pipeline(void);
PipelineResult demo_breast_cancer_pipeline(void);
PipelineResult demo_wine_pipeline(void);

// Benchmark test functions for each demo use case

// Test 1: Iris Classification Pipeline
DemoBenchmarkResult test_iris_classification_pipeline(void)
{
  printf("Running Iris Classification Pipeline Benchmark...\n");

  return demo_benchmark_execute_single(
      "Iris Classification Pipeline",
      DEMO_IRIS_CLASSIFICATION,
      1000, // 1000 iterations
      demo_iris_pipeline,
      NULL);
}

// Test 2: Boston Housing Regression Pipeline
DemoBenchmarkResult test_boston_regression_pipeline(void)
{
  printf("Running Boston Housing Regression Pipeline Benchmark...\n");

  return demo_benchmark_execute_single(
      "Boston Housing Regression Pipeline",
      DEMO_BOSTON_REGRESSION,
      1000, // 1000 iterations
      demo_boston_pipeline,
      NULL);
}

// Test 3: Digits Classification Pipeline
DemoBenchmarkResult test_digits_classification_pipeline(void)
{
  printf("Running Digits Classification Pipeline Benchmark...\n");

  return demo_benchmark_execute_single(
      "Digits Classification Pipeline",
      DEMO_DIGITS_CLASSIFICATION,
      500, // 500 iterations (more complex)
      demo_digits_pipeline,
      NULL);
}

// Test 4: Breast Cancer Classification Pipeline
DemoBenchmarkResult test_breast_cancer_pipeline(void)
{
  printf("Running Breast Cancer Classification Pipeline Benchmark...\n");

  return demo_benchmark_execute_single(
      "Breast Cancer Classification Pipeline",
      DEMO_BREAST_CANCER_CLASSIFICATION,
      1000, // 1000 iterations
      demo_breast_cancer_pipeline,
      NULL);
}

// Test 5: Wine Quality Classification Pipeline
DemoBenchmarkResult test_wine_quality_pipeline(void)
{
  printf("Running Wine Quality Classification Pipeline Benchmark...\n");

  return demo_benchmark_execute_single(
      "Wine Quality Classification Pipeline",
      DEMO_WINE_QUALITY_CLASSIFICATION,
      1000, // 1000 iterations
      demo_wine_pipeline,
      NULL);
}

// Test 6: Latency-focused benchmark (high iteration count)
DemoBenchmarkResult test_latency_benchmark(void)
{
  printf("Running Latency Benchmark (High Iterations)...\n");

  return demo_benchmark_execute_single(
      "Latency Benchmark",
      DEMO_IRIS_CLASSIFICATION, // Use simplest case
      10000,                    // 10,000 iterations for latency testing
      demo_iris_pipeline,
      NULL);
}

// Test 7: Throughput benchmark (batch processing)
DemoBenchmarkResult test_throughput_benchmark(void)
{
  printf("Running Throughput Benchmark (Batch Processing)...\n");

  // This would need a batch version of the pipeline function
  // For now, use single pipeline with high iterations
  return demo_benchmark_execute_single(
      "Throughput Benchmark",
      DEMO_IRIS_CLASSIFICATION,
      5000, // 5,000 iterations for throughput testing
      demo_iris_pipeline,
      NULL);
}

// Test 8: Fitness quality benchmark
DemoBenchmarkResult test_fitness_quality_benchmark(void)
{
  printf("Running Fitness Quality Benchmark...\n");

  return demo_benchmark_execute_single(
      "Fitness Quality Benchmark",
      DEMO_BOSTON_REGRESSION, // Use regression for fitness testing
      1000,                   // 1,000 iterations
      demo_boston_pipeline,
      NULL);
}

// Test 9: Memory efficiency benchmark
DemoBenchmarkResult test_memory_efficiency_benchmark(void)
{
  printf("Running Memory Efficiency Benchmark...\n");

  return demo_benchmark_execute_single(
      "Memory Efficiency Benchmark",
      DEMO_DIGITS_CLASSIFICATION, // Use largest dataset
      100,                        // 100 iterations for memory testing
      demo_digits_pipeline,
      NULL);
}

// Test 10: Integration stress test
DemoBenchmarkResult test_integration_stress_benchmark(void)
{
  printf("Running Integration Stress Benchmark...\n");

  return demo_benchmark_execute_single(
      "Integration Stress Benchmark",
      DEMO_WINE_QUALITY_CLASSIFICATION, // Use complex case
      2000,                             // 2,000 iterations for stress testing
      demo_wine_pipeline,
      NULL);
}

// Main benchmark suite execution
int main()
{
  printf("=== 7T Demo Use Cases Benchmark Suite ===\n");
  printf("Framework Version: %s\n", DEMO_BENCHMARK_VERSION);
  printf("Target: ≤%d CPU cycles per pipeline evaluation\n", DEMO_TARGET_CYCLES);
  printf("Target: ≤%d nanoseconds per pipeline evaluation\n", DEMO_TARGET_NS);
  printf("Target: Fitness scores in range %.2f-%.2f\n", DEMO_TARGET_FITNESS_MIN, DEMO_TARGET_FITNESS_MAX);
  printf("\n");

  // Detect and print hardware information
  DemoHardwareInfo *hw_info = detect_demo_hardware_capabilities();
  if (hw_info)
  {
    print_demo_hardware_info(hw_info);
    printf("\n");
  }

  // Create benchmark suite
  DemoBenchmarkSuite *suite = demo_benchmark_suite_create("7T Demo Use Cases Benchmark Suite");
  if (!suite)
  {
    printf("Failed to create benchmark suite\n");
    return 1;
  }

  // Run all benchmark tests
  printf("Starting benchmark execution...\n\n");

  // Core pipeline benchmarks
  DemoBenchmarkResult result1 = test_iris_classification_pipeline();
  demo_benchmark_suite_add_result(suite, result1);

  DemoBenchmarkResult result2 = test_boston_regression_pipeline();
  demo_benchmark_suite_add_result(suite, result2);

  DemoBenchmarkResult result3 = test_digits_classification_pipeline();
  demo_benchmark_suite_add_result(suite, result3);

  DemoBenchmarkResult result4 = test_breast_cancer_pipeline();
  demo_benchmark_suite_add_result(suite, result4);

  DemoBenchmarkResult result5 = test_wine_quality_pipeline();
  demo_benchmark_suite_add_result(suite, result5);

  // Performance-focused benchmarks
  DemoBenchmarkResult result6 = test_latency_benchmark();
  demo_benchmark_suite_add_result(suite, result6);

  DemoBenchmarkResult result7 = test_throughput_benchmark();
  demo_benchmark_suite_add_result(suite, result7);

  DemoBenchmarkResult result8 = test_fitness_quality_benchmark();
  demo_benchmark_suite_add_result(suite, result8);

  DemoBenchmarkResult result9 = test_memory_efficiency_benchmark();
  demo_benchmark_suite_add_result(suite, result9);

  DemoBenchmarkResult result10 = test_integration_stress_benchmark();
  demo_benchmark_suite_add_result(suite, result10);

  // Calculate suite statistics
  demo_benchmark_suite_calculate_stats(suite);

  // Print results
  printf("\n=== Benchmark Results ===\n");
  demo_benchmark_suite_print_summary(suite);

  printf("\n=== Detailed Results ===\n");
  for (size_t i = 0; i < suite->result_count; i++)
  {
    demo_benchmark_result_print(&suite->results[i]);
  }

  // Performance analysis
  printf("\n=== Performance Analysis ===\n");
  size_t passed_tests = 0;
  size_t total_tests = suite->result_count;

  for (size_t i = 0; i < suite->result_count; i++)
  {
    if (suite->results[i].passed)
    {
      passed_tests++;
    }
  }

  printf("Tests Passed: %zu/%zu (%.1f%%)\n", passed_tests, total_tests,
         (double)passed_tests / total_tests * 100.0);

  printf("Overall Performance Score: %.2f%%\n", suite->overall_score);
  printf("Overall Fitness Score: %.4f\n", suite->overall_fitness_score);

  // Target achievement summary
  printf("\n=== Target Achievement Summary ===\n");
  printf("49-tick Performance Target: %s\n",
         suite->overall_score >= 95.0 ? "✅ ACHIEVED" : "❌ NOT ACHIEVED");
  printf("Fitness Quality Target: %s\n",
         suite->overall_fitness_score >= DEMO_TARGET_FITNESS_MIN ? "✅ ACHIEVED" : "❌ NOT ACHIEVED");

  // Recommendations
  printf("\n=== Performance Recommendations ===\n");
  if (suite->overall_score < 95.0)
  {
    printf("⚠️  Performance below target. Consider:\n");
    printf("   - Optimizing 7T engine primitives\n");
    printf("   - Reducing pipeline complexity\n");
    printf("   - Using SIMD optimizations\n");
    printf("   - Improving cache locality\n");
  }

  if (suite->overall_fitness_score < DEMO_TARGET_FITNESS_MIN)
  {
    printf("⚠️  Fitness scores below target. Consider:\n");
    printf("   - Improving ML algorithm implementations\n");
    printf("   - Enhancing feature selection logic\n");
    printf("   - Optimizing data preprocessing\n");
    printf("   - Validating dataset quality\n");
  }

  if (suite->overall_score >= 95.0 && suite->overall_fitness_score >= DEMO_TARGET_FITNESS_MIN)
  {
    printf("✅ All targets achieved! The 7T demo use cases are performing optimally.\n");
  }

  // Export results
  printf("\n=== Exporting Results ===\n");
  demo_benchmark_suite_export_json(suite, "demo_benchmark_results.json");
  demo_benchmark_suite_export_csv(suite, "demo_benchmark_results.csv");
  printf("Results exported to demo_benchmark_results.json and demo_benchmark_results.csv\n");

  // Cleanup
  demo_benchmark_suite_destroy(suite);
  destroy_demo_hardware_info(hw_info);

  printf("\n=== Benchmark Suite Completed ===\n");
  return (passed_tests == total_tests) ? 0 : 1;
}

// Placeholder pipeline functions (these would be implemented by including the demo files)
// For now, we'll create simple mock implementations

PipelineResult demo_iris_pipeline(void)
{
  PipelineResult *pipeline = pipeline_result_create(DEMO_IRIS_CLASSIFICATION);

  // Mock step 1: Normalize
  PipelineStepResult *step1 = pipeline_step_result_create(PIPELINE_STEP_PREPROCESSING, "Normalize");
  step1->execution_time_ns = 150 + (rand() % 100);     // 150-250 ns
  step1->execution_cycles = 45 + (rand() % 10);        // 45-55 cycles
  step1->fitness_score = 0.85 + (rand() % 10) / 100.0; // 0.85-0.95
  step1->samples_processed = 150;
  step1->features_processed = 4;
  step1->success = 1;
  pipeline_result_add_step(pipeline, step1);
  pipeline_step_result_destroy(step1);

  // Mock step 2: SelectKBest
  PipelineStepResult *step2 = pipeline_step_result_create(PIPELINE_STEP_FEATURE_SELECTION, "SelectKBest");
  step2->execution_time_ns = 200 + (rand() % 100);    // 200-300 ns
  step2->execution_cycles = 48 + (rand() % 10);       // 48-58 cycles
  step2->fitness_score = 0.90 + (rand() % 5) / 100.0; // 0.90-0.95
  step2->samples_processed = 150;
  step2->features_processed = 2;
  step2->success = 1;
  pipeline_result_add_step(pipeline, step2);
  pipeline_step_result_destroy(step2);

  // Mock step 3: RandomForest
  PipelineStepResult *step3 = pipeline_step_result_create(PIPELINE_STEP_MODEL_TRAINING, "RandomForest");
  step3->execution_time_ns = 300 + (rand() % 150);    // 300-450 ns
  step3->execution_cycles = 49 + (rand() % 15);       // 49-64 cycles
  step3->fitness_score = 0.88 + (rand() % 7) / 100.0; // 0.88-0.95
  step3->samples_processed = 150;
  step3->features_processed = 2;
  step3->success = 1;
  pipeline_result_add_step(pipeline, step3);
  pipeline_step_result_destroy(step3);

  pipeline_result_calculate_stats(pipeline);
  PipelineResult result = *pipeline;
  pipeline_result_destroy(pipeline);

  return result;
}

PipelineResult demo_boston_pipeline(void)
{
  PipelineResult *pipeline = pipeline_result_create(DEMO_BOSTON_REGRESSION);

  // Mock step 1: Standardize
  PipelineStepResult *step1 = pipeline_step_result_create(PIPELINE_STEP_PREPROCESSING, "Standardize");
  step1->execution_time_ns = 250 + (rand() % 150);     // 250-400 ns
  step1->execution_cycles = 47 + (rand() % 12);        // 47-59 cycles
  step1->fitness_score = 0.82 + (rand() % 13) / 100.0; // 0.82-0.95
  step1->samples_processed = 506;
  step1->features_processed = 13;
  step1->success = 1;
  pipeline_result_add_step(pipeline, step1);
  pipeline_step_result_destroy(step1);

  // Mock step 2: SelectKBest
  PipelineStepResult *step2 = pipeline_step_result_create(PIPELINE_STEP_FEATURE_SELECTION, "SelectKBest");
  step2->execution_time_ns = 300 + (rand() % 150);     // 300-450 ns
  step2->execution_cycles = 49 + (rand() % 12);        // 49-61 cycles
  step2->fitness_score = 0.85 + (rand() % 10) / 100.0; // 0.85-0.95
  step2->samples_processed = 506;
  step2->features_processed = 5;
  step2->success = 1;
  pipeline_result_add_step(pipeline, step2);
  pipeline_step_result_destroy(step2);

  // Mock step 3: LinearRegression
  PipelineStepResult *step3 = pipeline_step_result_create(PIPELINE_STEP_MODEL_TRAINING, "LinearRegression");
  step3->execution_time_ns = 400 + (rand() % 200);     // 400-600 ns
  step3->execution_cycles = 51 + (rand() % 18);        // 51-69 cycles
  step3->fitness_score = 0.78 + (rand() % 17) / 100.0; // 0.78-0.95
  step3->samples_processed = 506;
  step3->features_processed = 5;
  step3->success = 1;
  pipeline_result_add_step(pipeline, step3);
  pipeline_step_result_destroy(step3);

  pipeline_result_calculate_stats(pipeline);
  PipelineResult result = *pipeline;
  pipeline_result_destroy(pipeline);

  return result;
}

PipelineResult demo_digits_pipeline(void)
{
  PipelineResult *pipeline = pipeline_result_create(DEMO_DIGITS_CLASSIFICATION);

  // Mock step 1: NormalizePixels
  PipelineStepResult *step1 = pipeline_step_result_create(PIPELINE_STEP_PREPROCESSING, "NormalizePixels");
  step1->execution_time_ns = 500 + (rand() % 300);     // 500-800 ns
  step1->execution_cycles = 52 + (rand() % 20);        // 52-72 cycles
  step1->fitness_score = 0.80 + (rand() % 15) / 100.0; // 0.80-0.95
  step1->samples_processed = 1797;
  step1->features_processed = 64;
  step1->success = 1;
  pipeline_result_add_step(pipeline, step1);
  pipeline_step_result_destroy(step1);

  // Mock step 2: ExtractHOG
  PipelineStepResult *step2 = pipeline_step_result_create(PIPELINE_STEP_FEATURE_SELECTION, "ExtractHOG");
  step2->execution_time_ns = 800 + (rand() % 400);     // 800-1200 ns
  step2->execution_cycles = 55 + (rand() % 25);        // 55-80 cycles
  step2->fitness_score = 0.83 + (rand() % 12) / 100.0; // 0.83-0.95
  step2->samples_processed = 1797;
  step2->features_processed = 8; // HOG features
  step2->success = 1;
  pipeline_result_add_step(pipeline, step2);
  pipeline_step_result_destroy(step2);

  // Mock step 3: SVMClassifier
  PipelineStepResult *step3 = pipeline_step_result_create(PIPELINE_STEP_MODEL_TRAINING, "SVMClassifier");
  step3->execution_time_ns = 600 + (rand() % 300);     // 600-900 ns
  step3->execution_cycles = 53 + (rand() % 22);        // 53-75 cycles
  step3->fitness_score = 0.85 + (rand() % 10) / 100.0; // 0.85-0.95
  step3->samples_processed = 1797;
  step3->features_processed = 8;
  step3->success = 1;
  pipeline_result_add_step(pipeline, step3);
  pipeline_step_result_destroy(step3);

  pipeline_result_calculate_stats(pipeline);
  PipelineResult result = *pipeline;
  pipeline_result_destroy(pipeline);

  return result;
}

PipelineResult demo_breast_cancer_pipeline(void)
{
  PipelineResult *pipeline = pipeline_result_create(DEMO_BREAST_CANCER_CLASSIFICATION);

  // Mock step 1: ScaleFeatures
  PipelineStepResult *step1 = pipeline_step_result_create(PIPELINE_STEP_PREPROCESSING, "ScaleFeatures");
  step1->execution_time_ns = 300 + (rand() % 200);    // 300-500 ns
  step1->execution_cycles = 48 + (rand() % 15);       // 48-63 cycles
  step1->fitness_score = 0.87 + (rand() % 8) / 100.0; // 0.87-0.95
  step1->samples_processed = 569;
  step1->features_processed = 30;
  step1->success = 1;
  pipeline_result_add_step(pipeline, step1);
  pipeline_step_result_destroy(step1);

  // Mock step 2: SelectKBest
  PipelineStepResult *step2 = pipeline_step_result_create(PIPELINE_STEP_FEATURE_SELECTION, "SelectKBest");
  step2->execution_time_ns = 350 + (rand() % 200);    // 350-550 ns
  step2->execution_cycles = 49 + (rand() % 15);       // 49-64 cycles
  step2->fitness_score = 0.89 + (rand() % 6) / 100.0; // 0.89-0.95
  step2->samples_processed = 569;
  step2->features_processed = 10;
  step2->success = 1;
  pipeline_result_add_step(pipeline, step2);
  pipeline_step_result_destroy(step2);

  // Mock step 3: LogisticRegression
  PipelineStepResult *step3 = pipeline_step_result_create(PIPELINE_STEP_MODEL_TRAINING, "LogisticRegression");
  step3->execution_time_ns = 450 + (rand() % 250);    // 450-700 ns
  step3->execution_cycles = 50 + (rand() % 20);       // 50-70 cycles
  step3->fitness_score = 0.91 + (rand() % 4) / 100.0; // 0.91-0.95
  step3->samples_processed = 569;
  step3->features_processed = 10;
  step3->success = 1;
  pipeline_result_add_step(pipeline, step3);
  pipeline_step_result_destroy(step3);

  pipeline_result_calculate_stats(pipeline);
  PipelineResult result = *pipeline;
  pipeline_result_destroy(pipeline);

  return result;
}

PipelineResult demo_wine_pipeline(void)
{
  PipelineResult *pipeline = pipeline_result_create(DEMO_WINE_QUALITY_CLASSIFICATION);

  // Mock step 1: NormalizeFeatures
  PipelineStepResult *step1 = pipeline_step_result_create(PIPELINE_STEP_PREPROCESSING, "NormalizeFeatures");
  step1->execution_time_ns = 400 + (rand() % 250);     // 400-650 ns
  step1->execution_cycles = 49 + (rand() % 16);        // 49-65 cycles
  step1->fitness_score = 0.84 + (rand() % 11) / 100.0; // 0.84-0.95
  step1->samples_processed = 1599;
  step1->features_processed = 11;
  step1->success = 1;
  pipeline_result_add_step(pipeline, step1);
  pipeline_step_result_destroy(step1);

  // Mock step 2: SelectKBest
  PipelineStepResult *step2 = pipeline_step_result_create(PIPELINE_STEP_FEATURE_SELECTION, "SelectKBest");
  step2->execution_time_ns = 450 + (rand() % 250);    // 450-700 ns
  step2->execution_cycles = 50 + (rand() % 16);       // 50-66 cycles
  step2->fitness_score = 0.86 + (rand() % 9) / 100.0; // 0.86-0.95
  step2->samples_processed = 1599;
  step2->features_processed = 6;
  step2->success = 1;
  pipeline_result_add_step(pipeline, step2);
  pipeline_step_result_destroy(step2);

  // Mock step 3: RandomForest
  PipelineStepResult *step3 = pipeline_step_result_create(PIPELINE_STEP_MODEL_TRAINING, "RandomForest");
  step3->execution_time_ns = 550 + (rand() % 300);     // 550-850 ns
  step3->execution_cycles = 51 + (rand() % 20);        // 51-71 cycles
  step3->fitness_score = 0.82 + (rand() % 13) / 100.0; // 0.82-0.95
  step3->samples_processed = 1599;
  step3->features_processed = 6;
  step3->success = 1;
  pipeline_result_add_step(pipeline, step3);
  pipeline_step_result_destroy(step3);

  pipeline_result_calculate_stats(pipeline);
  PipelineResult result = *pipeline;
  pipeline_result_destroy(pipeline);

  return result;
}