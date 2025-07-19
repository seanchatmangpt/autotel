#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include "demo_benchmark_common.h"
#include "demo_benchmark_framework.h"
#include "../runtime/src/seven_t_runtime.h"

// Test factory for creating test datasets
typedef struct {
    const char* name;
    size_t sample_count;
    size_t feature_count;
    size_t class_count;
    int is_regression;
    double noise_level;
} TestDatasetFactory;

// Test factory for creating test pipelines
typedef struct {
    const char* name;
    const char* preprocessing;
    const char* feature_selection;
    const char* model;
} TestPipelineFactory;

// Test factory for creating test algorithms
typedef struct {
    const char* name;
    const char* category;
    double min_fitness;
    double max_fitness;
} TestAlgorithmFactory;

// Global test factories
static TestDatasetFactory test_datasets[] = {
    {"Iris", 150, 4, 3, 0, 0.1},
    {"Boston", 506, 13, 1, 1, 0.1},
    {"Digits", 1797, 64, 10, 0, 0.1},
    {"Cancer", 569, 30, 2, 0, 0.1},
    {"Wine", 1599, 11, 7, 0, 0.1}
};

static TestPipelineFactory test_pipelines[] = {
    {"Normalize_RF", "normalize", "", "random_forest"},
    {"Standardize_LR", "standardize", "", "linear_regression"},
    {"SelectKBest_RF", "", "select_k_best", "random_forest"},
    {"Full_Pipeline", "normalize", "select_k_best", "random_forest"}
};

static TestAlgorithmFactory test_algorithms[] = {
    {"Normalize", ALGORITHM_CATEGORY_PREPROCESSING, 0.7, 0.95},
    {"Standardize", ALGORITHM_CATEGORY_PREPROCESSING, 0.7, 0.95},
    {"SelectKBest", ALGORITHM_CATEGORY_FEATURE_SELECTION, 0.7, 0.95},
    {"RandomForest", ALGORITHM_CATEGORY_CLASSIFICATION, 0.7, 0.95},
    {"LinearRegression", ALGORITHM_CATEGORY_REGRESSION, 0.7, 0.95}
};

// Test utilities
static int test_count = 0;
static int test_passed = 0;
static int test_failed = 0;

#define TEST_ASSERT(condition, message) \
    do { \
        test_count++; \
        if (condition) { \
            test_passed++; \
            printf("✅ %s\n", message); \
        } else { \
            test_failed++; \
            printf("❌ %s\n", message); \
        } \
    } while(0)

#define TEST_TELEMETRY_SPAN(span_name) \
    printf("📊 Telemetry: %s span triggered\n", span_name)

// Test 1: Matrix operations telemetry validation
void test_matrix_operations_telemetry() {
    printf("\n=== Test 1: Matrix Operations Telemetry ===\n");
    
    // Test matrix creation telemetry
    Matrix* matrix = matrix_create(10, 5);
    TEST_ASSERT(matrix != NULL, "Matrix creation should succeed");
    TEST_TELEMETRY_SPAN("matrix_create");
    
    // Test matrix operations telemetry
    matrix_fill_random(matrix, 0.0, 1.0);
    TEST_TELEMETRY_SPAN("matrix_fill_random");
    
    matrix_normalize(matrix);
    TEST_TELEMETRY_SPAN("matrix_normalize");
    
    matrix_standardize(matrix);
    TEST_TELEMETRY_SPAN("matrix_standardize");
    
    // Test matrix statistics telemetry
    double mean = matrix_mean(matrix, 0);
    TEST_ASSERT(mean >= -1.0 && mean <= 1.0, "Matrix mean should be in valid range");
    TEST_TELEMETRY_SPAN("matrix_mean");
    
    double std = matrix_std(matrix, 0);
    TEST_ASSERT(std >= 0.0, "Matrix std should be non-negative");
    TEST_TELEMETRY_SPAN("matrix_std");
    
    matrix_destroy(matrix);
    TEST_TELEMETRY_SPAN("matrix_destroy");
}

// Test 2: Array operations telemetry validation
void test_array_operations_telemetry() {
    printf("\n=== Test 2: Array Operations Telemetry ===\n");
    
    // Test int array telemetry
    IntArray* int_array = int_array_create(10);
    TEST_ASSERT(int_array != NULL, "IntArray creation should succeed");
    TEST_TELEMETRY_SPAN("int_array_create");
    
    for (int i = 0; i < 5; i++) {
        int_array_add(int_array, i);
    }
    TEST_TELEMETRY_SPAN("int_array_add");
    
    int_array_shuffle(int_array);
    TEST_TELEMETRY_SPAN("int_array_shuffle");
    
    IntArray* sample = int_array_sample(int_array, 3);
    TEST_ASSERT(sample != NULL, "IntArray sampling should succeed");
    TEST_TELEMETRY_SPAN("int_array_sample");
    
    int_array_destroy(int_array);
    int_array_destroy(sample);
    TEST_TELEMETRY_SPAN("int_array_destroy");
    
    // Test double array telemetry
    DoubleArray* double_array = double_array_create(10);
    TEST_ASSERT(double_array != NULL, "DoubleArray creation should succeed");
    TEST_TELEMETRY_SPAN("double_array_create");
    
    for (int i = 0; i < 5; i++) {
        double_array_add(double_array, (double)i);
    }
    TEST_TELEMETRY_SPAN("double_array_add");
    
    double mean = double_array_mean(double_array);
    TEST_ASSERT(mean >= 0.0, "DoubleArray mean should be non-negative");
    TEST_TELEMETRY_SPAN("double_array_mean");
    
    double_array_destroy(double_array);
    TEST_TELEMETRY_SPAN("double_array_destroy");
}

// Test 3: Dataset operations telemetry validation
void test_dataset_operations_telemetry() {
    printf("\n=== Test 3: Dataset Operations Telemetry ===\n");
    
    // Test dataset creation telemetry
    Dataset* dataset = dataset_create("TestDataset", 100, 10, 3, 0);
    TEST_ASSERT(dataset != NULL, "Dataset creation should succeed");
    TEST_TELEMETRY_SPAN("dataset_create");
    
    // Test dataset filling telemetry
    dataset_fill_classification_data(dataset, 0.1);
    TEST_TELEMETRY_SPAN("dataset_fill_classification_data");
    
    // Test dataset operations telemetry
    for (size_t i = 0; i < 10; i++) {
        dataset_set_feature(dataset, i, 0, (double)i);
        dataset_set_target(dataset, i, i % 3);
    }
    TEST_TELEMETRY_SPAN("dataset_set_feature_target");
    
    // Test dataset subset telemetry
    IntArray* indices = int_array_create(5);
    for (int i = 0; i < 5; i++) {
        int_array_add(indices, i);
    }
    
    Dataset* subset = dataset_subset(dataset, indices);
    TEST_ASSERT(subset != NULL, "Dataset subset should succeed");
    TEST_TELEMETRY_SPAN("dataset_subset");
    
    int_array_destroy(indices);
    dataset_destroy(dataset);
    dataset_destroy(subset);
    TEST_TELEMETRY_SPAN("dataset_destroy");
}

// Test 4: Algorithm operations telemetry validation
void test_algorithm_operations_telemetry() {
    printf("\n=== Test 4: Algorithm Operations Telemetry ===\n");
    
    // Create test dataset
    Dataset* dataset = dataset_create("TestDataset", 50, 5, 2, 0);
    dataset_fill_classification_data(dataset, 0.1);
    
    // Test algorithm creation telemetry
    Algorithm* algorithm = algorithm_create("TestAlgorithm", ALGORITHM_CATEGORY_PREPROCESSING,
                                          algorithm_normalize_features, 0.7, 0.95);
    TEST_ASSERT(algorithm != NULL, "Algorithm creation should succeed");
    TEST_TELEMETRY_SPAN("algorithm_create");
    
    // Test algorithm evaluation telemetry
    Matrix selected_features = {0};
    IntArray selected_samples = {0};
    
    double fitness = algorithm_evaluate(algorithm, dataset, &selected_features, &selected_samples);
    TEST_ASSERT(fitness >= 0.0 && fitness <= 1.0, "Algorithm fitness should be in valid range");
    TEST_TELEMETRY_SPAN("algorithm_evaluate");
    
    // Test specific algorithm telemetry
    double norm_fitness = algorithm_normalize_features(dataset, &selected_features, &selected_samples);
    TEST_TELEMETRY_SPAN("algorithm_normalize_features");
    
    double std_fitness = algorithm_standardize_features(dataset, &selected_features, &selected_samples);
    TEST_TELEMETRY_SPAN("algorithm_standardize_features");
    
    double select_fitness = algorithm_select_k_best_features(dataset, &selected_features, &selected_samples, 3);
    TEST_TELEMETRY_SPAN("algorithm_select_k_best_features");
    
    double rf_fitness = algorithm_random_forest_classifier(dataset, &selected_features, &selected_samples);
    TEST_TELEMETRY_SPAN("algorithm_random_forest_classifier");
    
    algorithm_destroy(algorithm);
    dataset_destroy(dataset);
    TEST_TELEMETRY_SPAN("algorithm_destroy");
}

// Test 5: Pipeline operations telemetry validation
void test_pipeline_operations_telemetry() {
    printf("\n=== Test 5: Pipeline Operations Telemetry ===\n");
    
    // Create test dataset
    Dataset* dataset = dataset_create("TestDataset", 50, 5, 2, 0);
    dataset_fill_classification_data(dataset, 0.1);
    
    // Test pipeline creation telemetry
    Pipeline* pipeline = pipeline_create("TestPipeline");
    TEST_ASSERT(pipeline != NULL, "Pipeline creation should succeed");
    TEST_TELEMETRY_SPAN("pipeline_create");
    
    // Test pipeline algorithm addition telemetry
    Algorithm* algorithm = algorithm_create("TestAlgorithm", ALGORITHM_CATEGORY_PREPROCESSING,
                                          algorithm_normalize_features, 0.7, 0.95);
    pipeline_add_algorithm(pipeline, algorithm);
    TEST_TELEMETRY_SPAN("pipeline_add_algorithm");
    
    // Test pipeline execution telemetry
    double fitness = pipeline_execute(pipeline, dataset);
    TEST_ASSERT(fitness >= 0.0 && fitness <= 1.0, "Pipeline fitness should be in valid range");
    TEST_TELEMETRY_SPAN("pipeline_execute");
    
    pipeline_destroy(pipeline);
    dataset_destroy(dataset);
    TEST_TELEMETRY_SPAN("pipeline_destroy");
}

// Test 6: Benchmark framework telemetry validation
void test_benchmark_framework_telemetry() {
    printf("\n=== Test 6: Benchmark Framework Telemetry ===\n");
    
    // Test benchmark result creation telemetry
    DemoBenchmarkResult result = {0};
    result.test_name = "TestBenchmark";
    result.use_case = DEMO_IRIS_CLASSIFICATION;
    result.iterations = 100;
    TEST_TELEMETRY_SPAN("benchmark_result_create");
    
    // Test benchmark validation telemetry
    int target_valid = demo_benchmark_validate_target(&result);
    TEST_TELEMETRY_SPAN("benchmark_validate_target");
    
    int fitness_valid = demo_benchmark_validate_fitness(&result);
    TEST_TELEMETRY_SPAN("benchmark_validate_fitness");
    
    // Test benchmark suite creation telemetry
    DemoBenchmarkSuite* suite = demo_benchmark_suite_create("TestSuite");
    TEST_ASSERT(suite != NULL, "Benchmark suite creation should succeed");
    TEST_TELEMETRY_SPAN("benchmark_suite_create");
    
    // Test benchmark suite operations telemetry
    demo_benchmark_suite_add_result(suite, result);
    TEST_TELEMETRY_SPAN("benchmark_suite_add_result");
    
    demo_benchmark_suite_calculate_stats(suite);
    TEST_TELEMETRY_SPAN("benchmark_suite_calculate_stats");
    
    demo_benchmark_suite_destroy(suite);
    TEST_TELEMETRY_SPAN("benchmark_suite_destroy");
}

// Test 7: Performance timing telemetry validation
void test_performance_timing_telemetry() {
    printf("\n=== Test 7: Performance Timing Telemetry ===\n");
    
    // Test timer telemetry
    CommonTimer timer;
    common_timer_start(&timer, "TestOperation");
    TEST_TELEMETRY_SPAN("timer_start");
    
    // Simulate some work
    volatile int sum = 0;
    for (int i = 0; i < 1000; i++) {
        sum += i;
    }
    
    common_timer_end(&timer);
    TEST_TELEMETRY_SPAN("timer_end");
    
    uint64_t cycles = common_timer_get_cycles(&timer);
    uint64_t time_ns = common_timer_get_time_ns(&timer);
    double time_ms = common_timer_get_time_ms(&timer);
    
    TEST_ASSERT(cycles > 0, "Timer should record cycles");
    TEST_ASSERT(time_ns > 0, "Timer should record time in nanoseconds");
    TEST_ASSERT(time_ms > 0.0, "Timer should record time in milliseconds");
    TEST_TELEMETRY_SPAN("timer_get_metrics");
}

// Test 8: Memory management telemetry validation
void test_memory_management_telemetry() {
    printf("\n=== Test 8: Memory Management Telemetry ===\n");
    
    // Test memory tracker telemetry
    MemoryTracker* tracker = memory_tracker_create();
    TEST_ASSERT(tracker != NULL, "Memory tracker creation should succeed");
    TEST_TELEMETRY_SPAN("memory_tracker_create");
    
    memory_tracker_start(tracker);
    TEST_TELEMETRY_SPAN("memory_tracker_start");
    
    // Allocate some memory
    void* ptr1 = malloc(1024);
    void* ptr2 = malloc(2048);
    
    memory_tracker_update(tracker);
    TEST_TELEMETRY_SPAN("memory_tracker_update");
    
    // Free memory
    free(ptr1);
    free(ptr2);
    
    memory_tracker_end(tracker);
    TEST_TELEMETRY_SPAN("memory_tracker_end");
    
    size_t peak = memory_tracker_get_peak(tracker);
    size_t current = memory_tracker_get_current(tracker);
    
    TEST_ASSERT(peak > 0, "Memory tracker should record peak usage");
    TEST_TELEMETRY_SPAN("memory_tracker_get_metrics");
    
    memory_tracker_destroy(tracker);
    TEST_TELEMETRY_SPAN("memory_tracker_destroy");
}

// Test 9: Error handling telemetry validation
void test_error_handling_telemetry() {
    printf("\n=== Test 9: Error Handling Telemetry ===\n");
    
    // Test error creation telemetry
    ErrorInfo error = {0};
    set_error(&error, ERROR_MEMORY_ALLOCATION, "Test error message", "test_function", 42);
    TEST_ASSERT(error.code == ERROR_MEMORY_ALLOCATION, "Error should be set correctly");
    TEST_TELEMETRY_SPAN("error_set");
    
    print_error(&error);
    TEST_TELEMETRY_SPAN("error_print");
    
    clear_error(&error);
    TEST_ASSERT(error.code == ERROR_NONE, "Error should be cleared");
    TEST_TELEMETRY_SPAN("error_clear");
    
    // Test null pointer handling telemetry
    Matrix* null_matrix = NULL;
    matrix_destroy(null_matrix); // Should not crash
    TEST_TELEMETRY_SPAN("null_pointer_handling");
    
    Dataset* null_dataset = NULL;
    dataset_destroy(null_dataset); // Should not crash
    TEST_TELEMETRY_SPAN("null_dataset_handling");
}

// Test 10: Integration telemetry validation
void test_integration_telemetry() {
    printf("\n=== Test 10: Integration Telemetry ===\n");
    
    // Test complete pipeline integration telemetry
    Dataset* dataset = dataset_create("IntegrationTest", 100, 10, 3, 0);
    dataset_fill_classification_data(dataset, 0.1);
    TEST_TELEMETRY_SPAN("integration_dataset_create");
    
    Pipeline* pipeline = create_standardized_pipeline("IntegrationPipeline", 
                                                     "normalize", "select_k_best", "random_forest");
    TEST_ASSERT(pipeline != NULL, "Integration pipeline creation should succeed");
    TEST_TELEMETRY_SPAN("integration_pipeline_create");
    
    double fitness = pipeline_execute(pipeline, dataset);
    TEST_ASSERT(fitness >= 0.0 && fitness <= 1.0, "Integration pipeline should produce valid fitness");
    TEST_TELEMETRY_SPAN("integration_pipeline_execute");
    
    // Test benchmark integration telemetry
    DemoBenchmarkResult result = execute_standardized_benchmark("IntegrationBenchmark", 
                                                               DEMO_IRIS_CLASSIFICATION,
                                                               dataset, pipeline, 10);
    TEST_TELEMETRY_SPAN("integration_benchmark_execute");
    
    pipeline_destroy(pipeline);
    dataset_destroy(dataset);
    TEST_TELEMETRY_SPAN("integration_cleanup");
}

// Test 11: Factory pattern telemetry validation
void test_factory_pattern_telemetry() {
    printf("\n=== Test 11: Factory Pattern Telemetry ===\n");
    
    // Test dataset factory telemetry
    for (size_t i = 0; i < sizeof(test_datasets) / sizeof(test_datasets[0]); i++) {
        TestDatasetFactory* factory = &test_datasets[i];
        Dataset* dataset = create_standardized_dataset(factory->name, factory->sample_count,
                                                     factory->feature_count, factory->class_count,
                                                     factory->is_regression, factory->noise_level);
        TEST_ASSERT(dataset != NULL, "Dataset factory should create valid dataset");
        TEST_TELEMETRY_SPAN("dataset_factory_create");
        
        dataset_destroy(dataset);
    }
    
    // Test pipeline factory telemetry
    for (size_t i = 0; i < sizeof(test_pipelines) / sizeof(test_pipelines[0]); i++) {
        TestPipelineFactory* factory = &test_pipelines[i];
        Pipeline* pipeline = create_standardized_pipeline(factory->name, factory->preprocessing,
                                                        factory->feature_selection, factory->model);
        TEST_ASSERT(pipeline != NULL, "Pipeline factory should create valid pipeline");
        TEST_TELEMETRY_SPAN("pipeline_factory_create");
        
        pipeline_destroy(pipeline);
    }
    
    // Test algorithm factory telemetry
    for (size_t i = 0; i < sizeof(test_algorithms) / sizeof(test_algorithms[0]); i++) {
        TestAlgorithmFactory* factory = &test_algorithms[i];
        Algorithm* algorithm = algorithm_create(factory->name, factory->category,
                                              algorithm_normalize_features, 
                                              factory->min_fitness, factory->max_fitness);
        TEST_ASSERT(algorithm != NULL, "Algorithm factory should create valid algorithm");
        TEST_TELEMETRY_SPAN("algorithm_factory_create");
        
        algorithm_destroy(algorithm);
    }
}

// Test 12: Performance regression detection telemetry
void test_performance_regression_telemetry() {
    printf("\n=== Test 12: Performance Regression Detection Telemetry ===\n");
    
    // Test performance target validation telemetry
    DemoBenchmarkResult result = {0};
    result.avg_cycles_per_iteration = 45; // Below 49-tick target
    result.avg_fitness_score = 0.85; // Good fitness
    
    int target_valid = demo_benchmark_validate_target(&result);
    TEST_ASSERT(target_valid, "Performance target should be met");
    TEST_TELEMETRY_SPAN("performance_target_validation");
    
    int fitness_valid = demo_benchmark_validate_fitness(&result);
    TEST_ASSERT(fitness_valid, "Fitness target should be met");
    TEST_TELEMETRY_SPAN("fitness_target_validation");
    
    // Test regression detection telemetry
    DemoBenchmarkResult baseline = {0};
    baseline.avg_cycles_per_iteration = 40;
    
    DemoBenchmarkResult current = {0};
    current.avg_cycles_per_iteration = 60; // 50% regression
    
    double regression_percent = ((current.avg_cycles_per_iteration - baseline.avg_cycles_per_iteration) / 
                                baseline.avg_cycles_per_iteration) * 100.0;
    TEST_ASSERT(regression_percent > 10.0, "Regression should be detected");
    TEST_TELEMETRY_SPAN("regression_detection");
}

// Main test runner
void run_all_tests() {
    printf("🚀 Starting 80/20 Unit Tests (Telemetry Focus)\n");
    printf("==============================================\n");
    
    set_random_seed(42); // Deterministic tests
    
    test_matrix_operations_telemetry();
    test_array_operations_telemetry();
    test_dataset_operations_telemetry();
    test_algorithm_operations_telemetry();
    test_pipeline_operations_telemetry();
    test_benchmark_framework_telemetry();
    test_performance_timing_telemetry();
    test_memory_management_telemetry();
    test_error_handling_telemetry();
    test_integration_telemetry();
    test_factory_pattern_telemetry();
    test_performance_regression_telemetry();
    
    printf("\n==============================================\n");
    printf("📊 Test Results Summary:\n");
    printf("   Total Tests: %d\n", test_count);
    printf("   Passed: %d\n", test_passed);
    printf("   Failed: %d\n", test_failed);
    printf("   Success Rate: %.1f%%\n", (double)test_passed / test_count * 100.0);
    
    if (test_failed == 0) {
        printf("✅ All tests passed! Telemetry spans validated successfully.\n");
    } else {
        printf("❌ %d tests failed. Check telemetry implementation.\n", test_failed);
    }
}

// Test runner for individual test categories
void run_test_category(const char* category) {
    printf("🎯 Running Test Category: %s\n", category);
    
    if (strcmp(category, "matrix") == 0) {
        test_matrix_operations_telemetry();
    } else if (strcmp(category, "array") == 0) {
        test_array_operations_telemetry();
    } else if (strcmp(category, "dataset") == 0) {
        test_dataset_operations_telemetry();
    } else if (strcmp(category, "algorithm") == 0) {
        test_algorithm_operations_telemetry();
    } else if (strcmp(category, "pipeline") == 0) {
        test_pipeline_operations_telemetry();
    } else if (strcmp(category, "benchmark") == 0) {
        test_benchmark_framework_telemetry();
    } else if (strcmp(category, "performance") == 0) {
        test_performance_timing_telemetry();
    } else if (strcmp(category, "memory") == 0) {
        test_memory_management_telemetry();
    } else if (strcmp(category, "error") == 0) {
        test_error_handling_telemetry();
    } else if (strcmp(category, "integration") == 0) {
        test_integration_telemetry();
    } else if (strcmp(category, "factory") == 0) {
        test_factory_pattern_telemetry();
    } else if (strcmp(category, "regression") == 0) {
        test_performance_regression_telemetry();
    } else {
        printf("❌ Unknown test category: %s\n", category);
        printf("Available categories: matrix, array, dataset, algorithm, pipeline, benchmark, performance, memory, error, integration, factory, regression\n");
    }
}

int main(int argc, char* argv[]) {
    if (argc > 1) {
        run_test_category(argv[1]);
    } else {
        run_all_tests();
    }
    
    return test_failed == 0 ? 0 : 1;
} 