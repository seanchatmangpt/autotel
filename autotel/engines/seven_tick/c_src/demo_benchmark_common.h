#ifndef DEMO_BENCHMARK_COMMON_H
#define DEMO_BENCHMARK_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <stdint.h>
#include <stddef.h>

// Common constants
#define MAX_FEATURES 100
#define MAX_SAMPLES 10000
#define MAX_CLASSES 20
#define MAX_PIPELINE_STEPS 10

// Common data structures
typedef struct
{
  double *data;
  size_t rows;
  size_t cols;
  size_t capacity;
} Matrix;

typedef struct
{
  int *data;
  size_t size;
  size_t capacity;
} IntArray;

typedef struct
{
  double *data;
  size_t size;
  size_t capacity;
} DoubleArray;

typedef struct
{
  char *name;
  size_t sample_count;
  size_t feature_count;
  size_t class_count;
  Matrix features;
  IntArray targets;
  DoubleArray sample_weights;
  int is_regression;
} Dataset;

typedef struct
{
  char *name;
  char *category;
  double (*evaluate_function)(Dataset *dataset, Matrix *selected_features, IntArray *selected_samples);
  double default_fitness;
  double min_fitness;
  double max_fitness;
} Algorithm;

typedef struct
{
  char *name;
  Algorithm *algorithms;
  size_t algorithm_count;
  size_t algorithm_capacity;
  double (*pipeline_function)(Dataset *dataset, Algorithm *algorithms, size_t count);
} Pipeline;

// Common timing utilities
typedef struct
{
  uint64_t start_cycles;
  uint64_t end_cycles;
  uint64_t start_time_ns;
  uint64_t end_time_ns;
  const char *operation_name;
} CommonTimer;

// Common memory utilities
typedef struct
{
  size_t initial_memory;
  size_t peak_memory;
  size_t final_memory;
  size_t allocations;
  size_t deallocations;
} MemoryTracker;

// Matrix operations
Matrix *matrix_create(size_t rows, size_t cols);
void matrix_destroy(Matrix *matrix);
void matrix_set(Matrix *matrix, size_t row, size_t col, double value);
double matrix_get(Matrix *matrix, size_t row, size_t col);
void matrix_fill_random(Matrix *matrix, double min_val, double max_val);
void matrix_fill_normal(Matrix *matrix, double mean, double std_dev);
void matrix_normalize(Matrix *matrix);
void matrix_standardize(Matrix *matrix);
void matrix_scale(Matrix *matrix, double min_val, double max_val);
double matrix_mean(Matrix *matrix, size_t col);
double matrix_std(Matrix *matrix, size_t col);
double matrix_correlation(Matrix *matrix, size_t col1, size_t col2);
void matrix_select_columns(Matrix *source, Matrix *target, IntArray *columns);

// Array operations
IntArray *int_array_create(size_t initial_capacity);
void int_array_destroy(IntArray *array);
void int_array_add(IntArray *array, int value);
int int_array_get(IntArray *array, size_t index);
void int_array_set(IntArray *array, size_t index, int value);
void int_array_fill_range(IntArray *array, int start, int end);
void int_array_shuffle(IntArray *array);
IntArray *int_array_sample(IntArray *source, size_t count);

DoubleArray *double_array_create(size_t initial_capacity);
void double_array_destroy(DoubleArray *array);
void double_array_add(DoubleArray *array, double value);
double double_array_get(DoubleArray *array, size_t index);
void double_array_set(DoubleArray *array, size_t index, double value);
double double_array_mean(DoubleArray *array);
double double_array_std(DoubleArray *array);
double double_array_min(DoubleArray *array);
double double_array_max(DoubleArray *array);

// Dataset operations
Dataset *dataset_create(const char *name, size_t sample_count, size_t feature_count, size_t class_count, int is_regression);
void dataset_destroy(Dataset *dataset);
void dataset_set_feature(Dataset *dataset, size_t sample, size_t feature, double value);
double dataset_get_feature(Dataset *dataset, size_t sample, size_t feature);
void dataset_set_target(Dataset *dataset, size_t sample, int target);
int dataset_get_target(Dataset *dataset, size_t sample);
void dataset_set_sample_weight(Dataset *dataset, size_t sample, double weight);
double dataset_get_sample_weight(Dataset *dataset, size_t sample);
void dataset_fill_random(Dataset *dataset, double feature_min, double feature_max);
void dataset_fill_classification_data(Dataset *dataset, double noise_level);
void dataset_fill_regression_data(Dataset *dataset, double noise_level);
void dataset_split(Dataset *source, Dataset *train, Dataset *test, double train_ratio);
Dataset *dataset_subset(Dataset *source, IntArray *sample_indices);

// Algorithm operations
Algorithm *algorithm_create(const char *name, const char *category,
                            double (*evaluate_function)(Dataset *dataset, Matrix *selected_features, IntArray *selected_samples),
                            double min_fitness, double max_fitness);
void algorithm_destroy(Algorithm *algorithm);
double algorithm_evaluate(Algorithm *algorithm, Dataset *dataset, Matrix *selected_features, IntArray *selected_samples);

// Pipeline operations
Pipeline *pipeline_create(const char *name);
void pipeline_destroy(Pipeline *pipeline);
void pipeline_add_algorithm(Pipeline *pipeline, Algorithm *algorithm);
double pipeline_execute(Pipeline *pipeline, Dataset *dataset);

// Common ML algorithms
double algorithm_normalize_features(Dataset *dataset, Matrix *selected_features, IntArray *selected_samples);
double algorithm_standardize_features(Dataset *dataset, Matrix *selected_features, IntArray *selected_samples);
double algorithm_scale_features(Dataset *dataset, Matrix *selected_features, IntArray *selected_samples);
double algorithm_select_k_best_features(Dataset *dataset, Matrix *selected_features, IntArray *selected_samples, int k);
double algorithm_random_forest_classifier(Dataset *dataset, Matrix *selected_features, IntArray *selected_samples);
double algorithm_linear_regression(Dataset *dataset, Matrix *selected_features, IntArray *selected_samples);
double algorithm_logistic_regression(Dataset *dataset, Matrix *selected_features, IntArray *selected_samples);
double algorithm_svm_classifier(Dataset *dataset, Matrix *selected_features, IntArray *selected_samples);
double algorithm_extract_hog_features(Dataset *dataset, Matrix *selected_features, IntArray *selected_samples);

// Common evaluation metrics
double calculate_accuracy(IntArray *predictions, IntArray *targets);
double calculate_precision(IntArray *predictions, IntArray *targets, int class_label);
double calculate_recall(IntArray *predictions, IntArray *targets, int class_label);
double calculate_f1_score(IntArray *predictions, IntArray *targets, int class_label);
double calculate_r_squared(DoubleArray *predictions, DoubleArray *targets);
double calculate_mean_squared_error(DoubleArray *predictions, DoubleArray *targets);
double calculate_mean_absolute_error(DoubleArray *predictions, DoubleArray *targets);

// Common feature selection
IntArray *select_features_by_variance(Dataset *dataset, int k);
IntArray *select_features_by_correlation(Dataset *dataset, int k);
IntArray *select_features_by_mutual_information(Dataset *dataset, int k);
IntArray *select_features_by_anova(Dataset *dataset, int k);

// Common sampling
IntArray *sample_random(Dataset *dataset, size_t count);
IntArray *sample_stratified(Dataset *dataset, size_t count);
IntArray *sample_weighted(Dataset *dataset, size_t count);

// Common validation
double cross_validate(Dataset *dataset, Pipeline *pipeline, int folds);
double bootstrap_validate(Dataset *dataset, Pipeline *pipeline, int iterations);

// Common timing utilities
void common_timer_start(CommonTimer *timer, const char *operation_name);
void common_timer_end(CommonTimer *timer);
uint64_t common_timer_get_cycles(CommonTimer *timer);
uint64_t common_timer_get_time_ns(CommonTimer *timer);
double common_timer_get_time_ms(CommonTimer *timer);

// Common memory utilities
MemoryTracker *memory_tracker_create(void);
void memory_tracker_start(MemoryTracker *tracker);
void memory_tracker_update(MemoryTracker *tracker);
void memory_tracker_end(MemoryTracker *tracker);
size_t memory_tracker_get_peak(MemoryTracker *tracker);
size_t memory_tracker_get_current(MemoryTracker *tracker);
void memory_tracker_destroy(MemoryTracker *tracker);

// Common statistics
double calculate_percentile(DoubleArray *data, double percentile);
double calculate_confidence_interval(DoubleArray *data, double confidence_level);
double calculate_outlier_threshold(DoubleArray *data, double multiplier);

// Common utilities
void set_random_seed(unsigned int seed);
double random_uniform(double min_val, double max_val);
double random_normal(double mean, double std_dev);
int random_int(int min_val, int max_val);
void shuffle_array(void *array, size_t size, size_t element_size);

// Common validation
int validate_dataset(Dataset *dataset);
int validate_matrix(Matrix *matrix);
int validate_array(IntArray *array);
int validate_double_array(DoubleArray *array);

// Common error handling
typedef enum
{
  ERROR_NONE = 0,
  ERROR_MEMORY_ALLOCATION,
  ERROR_INVALID_PARAMETER,
  ERROR_DATASET_EMPTY,
  ERROR_MATRIX_DIMENSION_MISMATCH,
  ERROR_ALGORITHM_NOT_FOUND,
  ERROR_PIPELINE_EMPTY
} ErrorCode;

typedef struct
{
  ErrorCode code;
  char message[256];
  const char *function;
  int line;
} ErrorInfo;

void set_error(ErrorInfo *error, ErrorCode code, const char *message, const char *function, int line);
void clear_error(ErrorInfo *error);
void print_error(ErrorInfo *error);

// Common macros
#define SET_ERROR(error, code, message) set_error(error, code, message, __func__, __LINE__)
#define RETURN_IF_ERROR(error)   \
  if (error->code != ERROR_NONE) \
  return
#define SAFE_FREE(ptr) \
  do                   \
  {                    \
    if (ptr)           \
    {                  \
      free(ptr);       \
      ptr = NULL;      \
    }                  \
  } while (0)

// Common constants for different dataset types
extern const char *DATASET_TYPE_CLASSIFICATION;
extern const char *DATASET_TYPE_REGRESSION;
extern const char *DATASET_TYPE_MULTICLASS;

extern const char *ALGORITHM_CATEGORY_PREPROCESSING;
extern const char *ALGORITHM_CATEGORY_FEATURE_SELECTION;
extern const char *ALGORITHM_CATEGORY_CLASSIFICATION;
extern const char *ALGORITHM_CATEGORY_REGRESSION;

// Common dataset templates
Dataset *create_iris_dataset(void);
Dataset *create_boston_dataset(void);
Dataset *create_digits_dataset(void);
Dataset *create_breast_cancer_dataset(void);
Dataset *create_wine_dataset(void);

// Common pipeline templates
Pipeline *create_iris_pipeline(void);
Pipeline *create_boston_pipeline(void);
Pipeline *create_digits_pipeline(void);
Pipeline *create_breast_cancer_pipeline(void);
Pipeline *create_wine_pipeline(void);

#endif // DEMO_BENCHMARK_COMMON_H