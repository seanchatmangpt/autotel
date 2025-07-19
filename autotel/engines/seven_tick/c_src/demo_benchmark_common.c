#include "demo_benchmark_common.h"
#include <immintrin.h>

// Common constants
const char *DATASET_TYPE_CLASSIFICATION = "classification";
const char *DATASET_TYPE_REGRESSION = "regression";
const char *DATASET_TYPE_MULTICLASS = "multiclass";

const char *ALGORITHM_CATEGORY_PREPROCESSING = "preprocessing";
const char *ALGORITHM_CATEGORY_FEATURE_SELECTION = "feature_selection";
const char *ALGORITHM_CATEGORY_CLASSIFICATION = "classification";
const char *ALGORITHM_CATEGORY_REGRESSION = "regression";

// Matrix operations
Matrix *matrix_create(size_t rows, size_t cols)
{
  Matrix *matrix = malloc(sizeof(Matrix));
  if (!matrix)
    return NULL;

  matrix->rows = rows;
  matrix->cols = cols;
  matrix->capacity = rows * cols;
  matrix->data = calloc(matrix->capacity, sizeof(double));

  if (!matrix->data)
  {
    free(matrix);
    return NULL;
  }

  return matrix;
}

void matrix_destroy(Matrix *matrix)
{
  if (matrix)
  {
    SAFE_FREE(matrix->data);
    free(matrix);
  }
}

void matrix_set(Matrix *matrix, size_t row, size_t col, double value)
{
  if (matrix && row < matrix->rows && col < matrix->cols)
  {
    matrix->data[row * matrix->cols + col] = value;
  }
}

double matrix_get(Matrix *matrix, size_t row, size_t col)
{
  if (matrix && row < matrix->rows && col < matrix->cols)
  {
    return matrix->data[row * matrix->cols + col];
  }
  return 0.0;
}

void matrix_fill_random(Matrix *matrix, double min_val, double max_val)
{
  if (!matrix)
    return;

  for (size_t i = 0; i < matrix->capacity; i++)
  {
    matrix->data[i] = random_uniform(min_val, max_val);
  }
}

void matrix_fill_normal(Matrix *matrix, double mean, double std_dev)
{
  if (!matrix)
    return;

  for (size_t i = 0; i < matrix->capacity; i++)
  {
    matrix->data[i] = random_normal(mean, std_dev);
  }
}

void matrix_normalize(Matrix *matrix)
{
  if (!matrix)
    return;

  for (size_t col = 0; col < matrix->cols; col++)
  {
    double min_val = INFINITY, max_val = -INFINITY;

    // Find min and max
    for (size_t row = 0; row < matrix->rows; row++)
    {
      double val = matrix_get(matrix, row, col);
      if (val < min_val)
        min_val = val;
      if (val > max_val)
        max_val = val;
    }

    // Normalize
    if (max_val > min_val)
    {
      for (size_t row = 0; row < matrix->rows; row++)
      {
        double val = matrix_get(matrix, row, col);
        double normalized = (val - min_val) / (max_val - min_val);
        matrix_set(matrix, row, col, normalized);
      }
    }
  }
}

void matrix_standardize(Matrix *matrix)
{
  if (!matrix)
    return;

  for (size_t col = 0; col < matrix->cols; col++)
  {
    double mean = matrix_mean(matrix, col);
    double std = matrix_std(matrix, col);

    if (std > 0.0)
    {
      for (size_t row = 0; row < matrix->rows; row++)
      {
        double val = matrix_get(matrix, row, col);
        double standardized = (val - mean) / std;
        matrix_set(matrix, row, col, standardized);
      }
    }
  }
}

double matrix_mean(Matrix *matrix, size_t col)
{
  if (!matrix || col >= matrix->cols)
    return 0.0;

  double sum = 0.0;
  for (size_t row = 0; row < matrix->rows; row++)
  {
    sum += matrix_get(matrix, row, col);
  }
  return sum / matrix->rows;
}

double matrix_std(Matrix *matrix, size_t col)
{
  if (!matrix || col >= matrix->cols)
    return 0.0;

  double mean = matrix_mean(matrix, col);
  double sum_sq = 0.0;

  for (size_t row = 0; row < matrix->rows; row++)
  {
    double val = matrix_get(matrix, row, col);
    sum_sq += (val - mean) * (val - mean);
  }

  return sqrt(sum_sq / matrix->rows);
}

double matrix_correlation(Matrix *matrix, size_t col1, size_t col2)
{
  if (!matrix || col1 >= matrix->cols || col2 >= matrix->cols)
    return 0.0;

  double mean1 = matrix_mean(matrix, col1);
  double mean2 = matrix_mean(matrix, col2);

  double numerator = 0.0;
  double sum_sq1 = 0.0;
  double sum_sq2 = 0.0;

  for (size_t row = 0; row < matrix->rows; row++)
  {
    double val1 = matrix_get(matrix, row, col1) - mean1;
    double val2 = matrix_get(matrix, row, col2) - mean2;

    numerator += val1 * val2;
    sum_sq1 += val1 * val1;
    sum_sq2 += val2 * val2;
  }

  double denominator = sqrt(sum_sq1 * sum_sq2);
  return denominator > 0.0 ? numerator / denominator : 0.0;
}

// Array operations
IntArray *int_array_create(size_t initial_capacity)
{
  IntArray *array = malloc(sizeof(IntArray));
  if (!array)
    return NULL;

  array->capacity = initial_capacity;
  array->size = 0;
  array->data = malloc(initial_capacity * sizeof(int));

  if (!array->data)
  {
    free(array);
    return NULL;
  }

  return array;
}

void int_array_destroy(IntArray *array)
{
  if (array)
  {
    SAFE_FREE(array->data);
    free(array);
  }
}

void int_array_add(IntArray *array, int value)
{
  if (!array)
    return;

  if (array->size >= array->capacity)
  {
    size_t new_capacity = array->capacity * 2;
    int *new_data = realloc(array->data, new_capacity * sizeof(int));
    if (!new_data)
      return;

    array->data = new_data;
    array->capacity = new_capacity;
  }

  array->data[array->size++] = value;
}

int int_array_get(IntArray *array, size_t index)
{
  if (array && index < array->size)
  {
    return array->data[index];
  }
  return 0;
}

void int_array_set(IntArray *array, size_t index, int value)
{
  if (array && index < array->size)
  {
    array->data[index] = value;
  }
}

void int_array_fill_range(IntArray *array, int start, int end)
{
  if (!array)
    return;

  for (int i = start; i < end; i++)
  {
    int_array_add(array, i);
  }
}

void int_array_shuffle(IntArray *array)
{
  if (!array)
    return;

  for (size_t i = array->size - 1; i > 0; i--)
  {
    size_t j = random_int(0, i);
    int temp = array->data[i];
    array->data[i] = array->data[j];
    array->data[j] = temp;
  }
}

IntArray *int_array_sample(IntArray *source, size_t count)
{
  if (!source || count > source->size)
    return NULL;

  IntArray *sample = int_array_create(count);
  if (!sample)
    return NULL;

  // Create a copy for shuffling
  IntArray *temp = int_array_create(source->size);
  if (!temp)
  {
    int_array_destroy(sample);
    return NULL;
  }

  for (size_t i = 0; i < source->size; i++)
  {
    int_array_add(temp, int_array_get(source, i));
  }

  int_array_shuffle(temp);

  for (size_t i = 0; i < count; i++)
  {
    int_array_add(sample, int_array_get(temp, i));
  }

  int_array_destroy(temp);
  return sample;
}

DoubleArray *double_array_create(size_t initial_capacity)
{
  DoubleArray *array = malloc(sizeof(DoubleArray));
  if (!array)
    return NULL;

  array->capacity = initial_capacity;
  array->size = 0;
  array->data = malloc(initial_capacity * sizeof(double));

  if (!array->data)
  {
    free(array);
    return NULL;
  }

  return array;
}

void double_array_destroy(DoubleArray *array)
{
  if (array)
  {
    SAFE_FREE(array->data);
    free(array);
  }
}

void double_array_add(DoubleArray *array, double value)
{
  if (!array)
    return;

  if (array->size >= array->capacity)
  {
    size_t new_capacity = array->capacity * 2;
    double *new_data = realloc(array->data, new_capacity * sizeof(double));
    if (!new_data)
      return;

    array->data = new_data;
    array->capacity = new_capacity;
  }

  array->data[array->size++] = value;
}

double double_array_get(DoubleArray *array, size_t index)
{
  if (array && index < array->size)
  {
    return array->data[index];
  }
  return 0.0;
}

void double_array_set(DoubleArray *array, size_t index, double value)
{
  if (array && index < array->size)
  {
    array->data[index] = value;
  }
}

double double_array_mean(DoubleArray *array)
{
  if (!array || array->size == 0)
    return 0.0;

  double sum = 0.0;
  for (size_t i = 0; i < array->size; i++)
  {
    sum += array->data[i];
  }
  return sum / array->size;
}

double double_array_std(DoubleArray *array)
{
  if (!array || array->size == 0)
    return 0.0;

  double mean = double_array_mean(array);
  double sum_sq = 0.0;

  for (size_t i = 0; i < array->size; i++)
  {
    double diff = array->data[i] - mean;
    sum_sq += diff * diff;
  }

  return sqrt(sum_sq / array->size);
}

double double_array_min(DoubleArray *array)
{
  if (!array || array->size == 0)
    return INFINITY;

  double min_val = array->data[0];
  for (size_t i = 1; i < array->size; i++)
  {
    if (array->data[i] < min_val)
    {
      min_val = array->data[i];
    }
  }
  return min_val;
}

double double_array_max(DoubleArray *array)
{
  if (!array || array->size == 0)
    return -INFINITY;

  double max_val = array->data[0];
  for (size_t i = 1; i < array->size; i++)
  {
    if (array->data[i] > max_val)
    {
      max_val = array->data[i];
    }
  }
  return max_val;
}

// Dataset operations
Dataset *dataset_create(const char *name, size_t sample_count, size_t feature_count, size_t class_count, int is_regression)
{
  Dataset *dataset = malloc(sizeof(Dataset));
  if (!dataset)
    return NULL;

  dataset->name = strdup(name);
  dataset->sample_count = sample_count;
  dataset->feature_count = feature_count;
  dataset->class_count = class_count;
  dataset->is_regression = is_regression;

  dataset->features = *matrix_create(sample_count, feature_count);
  dataset->targets = *int_array_create(sample_count);
  dataset->sample_weights = *double_array_create(sample_count);

  // Initialize sample weights to 1.0
  for (size_t i = 0; i < sample_count; i++)
  {
    double_array_add(&dataset->sample_weights, 1.0);
  }

  return dataset;
}

void dataset_destroy(Dataset *dataset)
{
  if (dataset)
  {
    SAFE_FREE(dataset->name);
    matrix_destroy(&dataset->features);
    int_array_destroy(&dataset->targets);
    double_array_destroy(&dataset->sample_weights);
    free(dataset);
  }
}

void dataset_set_feature(Dataset *dataset, size_t sample, size_t feature, double value)
{
  if (dataset)
  {
    matrix_set(&dataset->features, sample, feature, value);
  }
}

double dataset_get_feature(Dataset *dataset, size_t sample, size_t feature)
{
  if (dataset)
  {
    return matrix_get(&dataset->features, sample, feature);
  }
  return 0.0;
}

void dataset_set_target(Dataset *dataset, size_t sample, int target)
{
  if (dataset && sample < dataset->targets.size)
  {
    int_array_set(&dataset->targets, sample, target);
  }
}

int dataset_get_target(Dataset *dataset, size_t sample)
{
  if (dataset && sample < dataset->targets.size)
  {
    return int_array_get(&dataset->targets, sample);
  }
  return 0;
}

void dataset_set_sample_weight(Dataset *dataset, size_t sample, double weight)
{
  if (dataset && sample < dataset->sample_weights.size)
  {
    double_array_set(&dataset->sample_weights, sample, weight);
  }
}

double dataset_get_sample_weight(Dataset *dataset, size_t sample)
{
  if (dataset && sample < dataset->sample_weights.size)
  {
    return double_array_get(&dataset->sample_weights, sample);
  }
  return 1.0;
}

// Common ML algorithms
double algorithm_normalize_features(Dataset *dataset, Matrix *selected_features, IntArray *selected_samples)
{
  if (!dataset || !selected_features || !selected_samples)
    return 0.0;

  double total_fitness = 0.0;
  int processed_samples = 0;

  for (size_t i = 0; i < selected_samples->size; i++)
  {
    int sample_idx = int_array_get(selected_samples, i);
    if (sample_idx >= 0 && sample_idx < dataset->sample_count)
    {
      double sample_fitness = 0.0;
      int active_features = 0;

      for (size_t j = 0; j < selected_features->cols; j++)
      {
        double value = dataset_get_feature(dataset, sample_idx, j);
        // Simple normalization simulation
        double normalized = value / 16.0; // Assuming 0-16 range
        sample_fitness += normalized;
        active_features++;
      }

      if (active_features > 0)
      {
        total_fitness += sample_fitness / active_features;
        processed_samples++;
      }
    }
  }

  return processed_samples > 0 ? total_fitness / processed_samples : 0.0;
}

double algorithm_standardize_features(Dataset *dataset, Matrix *selected_features, IntArray *selected_samples)
{
  if (!dataset || !selected_features || !selected_samples)
    return 0.0;

  double total_fitness = 0.0;
  int processed_features = 0;

  for (size_t j = 0; j < selected_features->cols; j++)
  {
    double sum = 0.0, sum_sq = 0.0;
    int count = 0;

    // Calculate mean and std
    for (size_t i = 0; i < selected_samples->size; i++)
    {
      int sample_idx = int_array_get(selected_samples, i);
      if (sample_idx >= 0 && sample_idx < dataset->sample_count)
      {
        double value = dataset_get_feature(dataset, sample_idx, j);
        sum += value;
        sum_sq += value * value;
        count++;
      }
    }

    if (count > 1)
    {
      double mean = sum / count;
      double variance = (sum_sq / count) - (mean * mean);
      double std_dev = sqrt(variance);

      if (std_dev > 0.0)
      {
        double standardized_sum = 0.0;
        for (size_t i = 0; i < selected_samples->size; i++)
        {
          int sample_idx = int_array_get(selected_samples, i);
          if (sample_idx >= 0 && sample_idx < dataset->sample_count)
          {
            double value = dataset_get_feature(dataset, sample_idx, j);
            double standardized = (value - mean) / std_dev;
            standardized_sum += fabs(standardized);
          }
        }

        total_fitness += standardized_sum / count;
        processed_features++;
      }
    }
  }

  return processed_features > 0 ? total_fitness / processed_features : 0.0;
}

double algorithm_select_k_best_features(Dataset *dataset, Matrix *selected_features, IntArray *selected_samples, int k)
{
  if (!dataset || !selected_features || !selected_samples)
    return 0.0;

  // Calculate feature importance (simplified)
  IntArray *feature_scores = int_array_create(dataset->feature_count);
  if (!feature_scores)
    return 0.0;

  for (size_t j = 0; j < dataset->feature_count; j++)
  {
    double importance = 0.0;
    int count = 0;

    for (size_t i = 0; i < selected_samples->size; i++)
    {
      int sample_idx = int_array_get(selected_samples, i);
      if (sample_idx >= 0 && sample_idx < dataset->sample_count)
      {
        double value = dataset_get_feature(dataset, sample_idx, j);
        importance += fabs(value);
        count++;
      }
    }

    if (count > 0)
    {
      int_array_set(feature_scores, j, (int)(importance / count * 100));
    }
  }

  // Select top k features
  int selected_count = 0;
  for (size_t j = 0; j < dataset->feature_count && selected_count < k; j++)
  {
    int score = int_array_get(feature_scores, j);
    if (score > 10)
    { // Threshold
      selected_count++;
    }
  }

  double fitness = (double)selected_count / k;
  int_array_destroy(feature_scores);

  return fitness;
}

double algorithm_random_forest_classifier(Dataset *dataset, Matrix *selected_features, IntArray *selected_samples)
{
  if (!dataset || !selected_features || !selected_samples)
    return 0.0;

  int correct_predictions = 0;
  int total_predictions = 0;

  for (size_t i = 0; i < selected_samples->size; i++)
  {
    int sample_idx = int_array_get(selected_samples, i);
    if (sample_idx >= 0 && sample_idx < dataset->sample_count)
    {
      // Simple classification based on feature values
      double decision_value = 0.0;
      int active_features = 0;

      for (size_t j = 0; j < selected_features->cols; j++)
      {
        double value = dataset_get_feature(dataset, sample_idx, j);
        decision_value += value;
        active_features++;
      }

      if (active_features > 0)
      {
        decision_value /= active_features;

        // Simple threshold-based classification
        int predicted_class = (decision_value > 5.0) ? 1 : 0;
        int actual_class = dataset_get_target(dataset, sample_idx);

        if (predicted_class == actual_class)
        {
          correct_predictions++;
        }
        total_predictions++;
      }
    }
  }

  return total_predictions > 0 ? (double)correct_predictions / total_predictions : 0.0;
}

// Common timing utilities
void common_timer_start(CommonTimer *timer, const char *operation_name)
{
  if (timer)
  {
    timer->operation_name = operation_name;
    timer->start_cycles = __builtin_readcyclecounter();

    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    timer->start_time_ns = ts.tv_sec * 1000000000ULL + ts.tv_nsec;
  }
}

void common_timer_end(CommonTimer *timer)
{
  if (timer)
  {
    timer->end_cycles = __builtin_readcyclecounter();

    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    timer->end_time_ns = ts.tv_sec * 1000000000ULL + ts.tv_nsec;
  }
}

uint64_t common_timer_get_cycles(CommonTimer *timer)
{
  return timer ? timer->end_cycles - timer->start_cycles : 0;
}

uint64_t common_timer_get_time_ns(CommonTimer *timer)
{
  return timer ? timer->end_time_ns - timer->start_time_ns : 0;
}

double common_timer_get_time_ms(CommonTimer *timer)
{
  return timer ? (double)(timer->end_time_ns - timer->start_time_ns) / 1000000.0 : 0.0;
}

// Common utilities
void set_random_seed(unsigned int seed)
{
  srand(seed);
}

double random_uniform(double min_val, double max_val)
{
  return min_val + (max_val - min_val) * ((double)rand() / RAND_MAX);
}

double random_normal(double mean, double std_dev)
{
  // Box-Muller transform
  double u1 = random_uniform(0.0, 1.0);
  double u2 = random_uniform(0.0, 1.0);
  double z0 = sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);
  return mean + std_dev * z0;
}

int random_int(int min_val, int max_val)
{
  return min_val + rand() % (max_val - min_val + 1);
}

void shuffle_array(void *array, size_t size, size_t element_size)
{
  char *data = (char *)array;
  for (size_t i = size - 1; i > 0; i--)
  {
    size_t j = random_int(0, i);
    // Swap elements
    for (size_t k = 0; k < element_size; k++)
    {
      char temp = data[i * element_size + k];
      data[i * element_size + k] = data[j * element_size + k];
      data[j * element_size + k] = temp;
    }
  }
}

// Error handling
void set_error(ErrorInfo *error, ErrorCode code, const char *message, const char *function, int line)
{
  if (error)
  {
    error->code = code;
    snprintf(error->message, sizeof(error->message), "%s", message);
    error->function = function;
    error->line = line;
  }
}

void clear_error(ErrorInfo *error)
{
  if (error)
  {
    error->code = ERROR_NONE;
    error->message[0] = '\0';
    error->function = NULL;
    error->line = 0;
  }
}

void print_error(ErrorInfo *error)
{
  if (error && error->code != ERROR_NONE)
  {
    fprintf(stderr, "Error in %s:%d: %s (%d)\n",
            error->function, error->line, error->message, error->code);
  }
}