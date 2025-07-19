#ifndef SEVEN_T_TPOT_H
#define SEVEN_T_TPOT_H

#include <stdint.h>
#include <time.h>

// Bit-vector structure for efficient set operations
typedef struct
{
  uint64_t *data;
  uint32_t size;
  uint32_t capacity;
} BitVector;

// 7T TPOT Core Structures
typedef struct
{
  uint32_t num_samples;
  uint32_t num_features;
  double *data;            // Row-major layout for cache efficiency
  uint32_t *labels;        // Integer labels
  BitVector *feature_mask; // Active features
  BitVector *sample_mask;  // Active samples
} Dataset7T;

typedef struct
{
  uint32_t step_type;    // PREPROCESSING, FEATURE_SELECTION, MODEL
  uint32_t algorithm_id; // Algorithm identifier
  double *parameters;    // Algorithm parameters
  uint32_t num_parameters;
  BitVector *input_features;
  BitVector *output_features;
} PipelineStep;

typedef struct
{
  uint32_t pipeline_id;
  uint32_t num_steps;
  PipelineStep *steps;
  double fitness_score;
  uint64_t evaluation_time_ns;
  uint32_t num_correct;
  uint32_t num_total;
} Pipeline7T;

typedef struct
{
  Pipeline7T **population;
  uint32_t population_size;
  uint32_t generation;
  uint32_t best_pipeline_id;
  double best_fitness;
} OptimizationEngine7T;

// Algorithm Registry
typedef struct
{
  uint32_t algorithm_id;
  const char *name;
  uint32_t category;
  double (*evaluate)(Dataset7T *data, double *params);
  void (*fit)(Dataset7T *data, double *params, void *model);
  double (*predict)(void *model, Dataset7T *data);
} Algorithm7T;

// Algorithm Categories
#define PREPROCESSING 1
#define FEATURE_SELECTION 2
#define MODEL 3

// Algorithm IDs
#define NORMALIZE 1
#define STANDARDIZE 2
#define SELECT_K_BEST 3
#define RANDOM_FOREST 4
#define LINEAR_REGRESSION 5
#define LOGISTIC_REGRESSION 6
#define SVM 7
#define KNN 8

// Performance measurement macros
#define START_TIMER() clock_gettime(CLOCK_MONOTONIC, &start_time)
#define END_TIMER() clock_gettime(CLOCK_MONOTONIC, &end_time)
#define GET_ELAPSED_NS() ((end_time.tv_sec - start_time.tv_sec) * 1000000000ULL + (end_time.tv_nsec - start_time.tv_nsec))

// Function declarations
Dataset7T *create_iris_dataset();
Dataset7T *create_boston_dataset();
Dataset7T *create_breast_cancer_dataset();
Dataset7T *create_diabetes_dataset();
Dataset7T *create_digits_dataset();

Pipeline7T *create_pipeline(uint32_t num_steps);
double evaluate_pipeline_7t(Pipeline7T *pipeline, Dataset7T *data);
OptimizationEngine7T *create_optimizer_7t(uint32_t population_size, uint32_t num_generations);
Pipeline7T *optimize_pipeline_7t(OptimizationEngine7T *optimizer, Dataset7T *data, uint32_t timeout_seconds);

void register_algorithms();
void benchmark_7t_tpot();

// Algorithm evaluation functions
double normalize_features(Dataset7T *data, double *params);
double standardize_features(Dataset7T *data, double *params);
double select_k_best_features(Dataset7T *data, double *params);
double evaluate_random_forest(Dataset7T *data, double *params);
double evaluate_linear_regression(Dataset7T *data, double *params);

#endif // SEVEN_T_TPOT_H