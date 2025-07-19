#ifndef DEMO_BENCHMARK_TEST_FACTORIES_H
#define DEMO_BENCHMARK_TEST_FACTORIES_H

#include "demo_benchmark_common.h"
#include "demo_benchmark_framework.h"

// Test factory base structure
typedef struct
{
  const char *name;
  const char *description;
  int (*setup_function)(void);
  int (*teardown_function)(void);
  int (*validation_function)(void);
} TestFactory;

// Dataset test factory
typedef struct
{
  TestFactory base;
  const char *dataset_name;
  size_t sample_count;
  size_t feature_count;
  size_t class_count;
  int is_regression;
  double noise_level;
  Dataset *(*create_function)(void);
  void (*destroy_function)(Dataset *);
} DatasetTestFactory;

// Pipeline test factory
typedef struct
{
  TestFactory base;
  const char *pipeline_name;
  const char *preprocessing_type;
  const char *feature_selection_type;
  const char *model_type;
  Pipeline *(*create_function)(void);
  void (*destroy_function)(Pipeline *);
} PipelineTestFactory;

// Algorithm test factory
typedef struct
{
  TestFactory base;
  const char *algorithm_name;
  const char *category;
  double min_fitness;
  double max_fitness;
  double (*evaluate_function)(Dataset *, Matrix *, IntArray *);
  Algorithm *(*create_function)(void);
  void (*destroy_function)(Algorithm *);
} AlgorithmTestFactory;

// Benchmark test factory
typedef struct
{
  TestFactory base;
  const char *benchmark_name;
  DemoUseCase use_case;
  size_t iterations;
  uint64_t target_cycles;
  double min_fitness;
  double max_fitness;
  DemoBenchmarkResult (*create_function)(void);
  int (*validate_function)(DemoBenchmarkResult *);
} BenchmarkTestFactory;

// Test suite factory
typedef struct
{
  TestFactory base;
  const char *suite_name;
  DatasetTestFactory **datasets;
  size_t dataset_count;
  PipelineTestFactory **pipelines;
  size_t pipeline_count;
  AlgorithmTestFactory **algorithms;
  size_t algorithm_count;
  BenchmarkTestFactory **benchmarks;
  size_t benchmark_count;
  DemoBenchmarkSuite *(*create_function)(void);
  void (*destroy_function)(DemoBenchmarkSuite *);
} TestSuiteFactory;

// Factory creation functions
DatasetTestFactory *create_dataset_test_factory(const char *name, const char *dataset_name,
                                                size_t sample_count, size_t feature_count,
                                                size_t class_count, int is_regression,
                                                double noise_level);

PipelineTestFactory *create_pipeline_test_factory(const char *name, const char *pipeline_name,
                                                  const char *preprocessing, const char *feature_selection,
                                                  const char *model);

AlgorithmTestFactory *create_algorithm_test_factory(const char *name, const char *algorithm_name,
                                                    const char *category, double min_fitness,
                                                    double max_fitness,
                                                    double (*evaluate_function)(Dataset *, Matrix *, IntArray *));

BenchmarkTestFactory *create_benchmark_test_factory(const char *name, const char *benchmark_name,
                                                    DemoUseCase use_case, size_t iterations,
                                                    uint64_t target_cycles, double min_fitness,
                                                    double max_fitness);

TestSuiteFactory *create_test_suite_factory(const char *name, const char *suite_name);

// Factory execution functions
Dataset *execute_dataset_factory(DatasetTestFactory *factory);
Pipeline *execute_pipeline_factory(PipelineTestFactory *factory);
Algorithm *execute_algorithm_factory(AlgorithmTestFactory *factory);
DemoBenchmarkResult execute_benchmark_factory(BenchmarkTestFactory *factory);
DemoBenchmarkSuite *execute_test_suite_factory(TestSuiteFactory *factory);

// Factory validation functions
int validate_dataset_factory(DatasetTestFactory *factory, Dataset *dataset);
int validate_pipeline_factory(PipelineTestFactory *factory, Pipeline *pipeline);
int validate_algorithm_factory(AlgorithmTestFactory *factory, Algorithm *algorithm);
int validate_benchmark_factory(BenchmarkTestFactory *factory, DemoBenchmarkResult *result);
int validate_test_suite_factory(TestSuiteFactory *factory, DemoBenchmarkSuite *suite);

// Factory cleanup functions
void destroy_dataset_factory(DatasetTestFactory *factory);
void destroy_pipeline_factory(PipelineTestFactory *factory);
void destroy_algorithm_factory(AlgorithmTestFactory *factory);
void destroy_benchmark_factory(BenchmarkTestFactory *factory);
void destroy_test_suite_factory(TestSuiteFactory *factory);

// Predefined test factories
extern DatasetTestFactory *iris_dataset_factory;
extern DatasetTestFactory *boston_dataset_factory;
extern DatasetTestFactory *digits_dataset_factory;
extern DatasetTestFactory *cancer_dataset_factory;
extern DatasetTestFactory *wine_dataset_factory;

extern PipelineTestFactory *normalize_rf_pipeline_factory;
extern PipelineTestFactory *standardize_lr_pipeline_factory;
extern PipelineTestFactory *selectkbest_rf_pipeline_factory;
extern PipelineTestFactory *full_pipeline_factory;

extern AlgorithmTestFactory *normalize_algorithm_factory;
extern AlgorithmTestFactory *standardize_algorithm_factory;
extern AlgorithmTestFactory *selectkbest_algorithm_factory;
extern AlgorithmTestFactory *randomforest_algorithm_factory;
extern AlgorithmTestFactory *linearregression_algorithm_factory;

extern BenchmarkTestFactory *iris_benchmark_factory;
extern BenchmarkTestFactory *boston_benchmark_factory;
extern BenchmarkTestFactory *digits_benchmark_factory;
extern BenchmarkTestFactory *cancer_benchmark_factory;
extern BenchmarkTestFactory *wine_benchmark_factory;

extern TestSuiteFactory *comprehensive_test_suite_factory;

// Factory initialization
void initialize_test_factories(void);
void cleanup_test_factories(void);

// Factory-based test execution
int run_factory_based_tests(void);
int run_dataset_factory_tests(void);
int run_pipeline_factory_tests(void);
int run_algorithm_factory_tests(void);
int run_benchmark_factory_tests(void);
int run_test_suite_factory_tests(void);

#endif // DEMO_BENCHMARK_TEST_FACTORIES_H