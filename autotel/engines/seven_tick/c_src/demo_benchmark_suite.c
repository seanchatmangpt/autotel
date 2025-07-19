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

// Include the real pipeline implementations from demo_benchmark_connectors.c
// These connect the actual demo use cases to the benchmark framework
#include "demo_benchmark_connectors.c"