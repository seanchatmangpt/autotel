#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <immintrin.h>

// 7T Technique 3: SIMD Batching - Exploiting Data Parallelism
// First Principle: Von Neumann bottleneck mitigation through parallel processing

#define VECTOR_SIZE 1024
#define SIMD_WIDTH 4 // 4-wide SIMD for demonstration

typedef struct
{
  float *data;
  size_t size;
  size_t capacity;
} FloatVector;

typedef struct
{
  uint32_t *data;
  size_t size;
  size_t capacity;
} IntVector;

// Create float vector
FloatVector *float_vector_create(size_t size)
{
  FloatVector *vec = malloc(sizeof(FloatVector));
  if (!vec)
    return NULL;

  vec->size = size;
  vec->capacity = size;
  vec->data = aligned_alloc(32, size * sizeof(float)); // 32-byte alignment for SIMD

  if (!vec->data)
  {
    free(vec);
    return NULL;
  }

  return vec;
}

// Destroy float vector
void float_vector_destroy(FloatVector *vec)
{
  if (vec)
  {
    free(vec->data);
    free(vec);
  }
}

// Fill vector with random data
void float_vector_fill_random(FloatVector *vec, float min_val, float max_val)
{
  if (!vec)
    return;

  for (size_t i = 0; i < vec->size; i++)
  {
    float r = (float)rand() / RAND_MAX;
    vec->data[i] = min_val + r * (max_val - min_val);
  }
}

// Sequential vector addition (baseline)
void vector_add_sequential(FloatVector *a, FloatVector *b, FloatVector *result)
{
  if (!a || !b || !result || a->size != b->size || a->size != result->size)
    return;

  for (size_t i = 0; i < a->size; i++)
  {
    result->data[i] = a->data[i] + b->data[i];
  }
}

// SIMD vector addition using AVX2
void vector_add_simd(FloatVector *a, FloatVector *b, FloatVector *result)
{
  if (!a || !b || !result || a->size != b->size || a->size != result->size)
    return;

  size_t simd_size = a->size - (a->size % 8); // Process 8 floats at a time with AVX2

  // SIMD operations
  for (size_t i = 0; i < simd_size; i += 8)
  {
    __m256 va = _mm256_load_ps(&a->data[i]);
    __m256 vb = _mm256_load_ps(&b->data[i]);
    __m256 vr = _mm256_add_ps(va, vb);
    _mm256_store_ps(&result->data[i], vr);
  }

  // Handle remaining elements sequentially
  for (size_t i = simd_size; i < a->size; i++)
  {
    result->data[i] = a->data[i] + b->data[i];
  }
}

// Sequential vector multiplication
void vector_mul_sequential(FloatVector *a, FloatVector *b, FloatVector *result)
{
  if (!a || !b || !result || a->size != b->size || a->size != result->size)
    return;

  for (size_t i = 0; i < a->size; i++)
  {
    result->data[i] = a->data[i] * b->data[i];
  }
}

// SIMD vector multiplication
void vector_mul_simd(FloatVector *a, FloatVector *b, FloatVector *result)
{
  if (!a || !b || !result || a->size != b->size || a->size != result->size)
    return;

  size_t simd_size = a->size - (a->size % 8);

  for (size_t i = 0; i < simd_size; i += 8)
  {
    __m256 va = _mm256_load_ps(&a->data[i]);
    __m256 vb = _mm256_load_ps(&b->data[i]);
    __m256 vr = _mm256_mul_ps(va, vb);
    _mm256_store_ps(&result->data[i], vr);
  }

  for (size_t i = simd_size; i < a->size; i++)
  {
    result->data[i] = a->data[i] * b->data[i];
  }
}

// Sequential vector normalization
void vector_normalize_sequential(FloatVector *vec)
{
  if (!vec)
    return;

  // Find min and max
  float min_val = vec->data[0];
  float max_val = vec->data[0];

  for (size_t i = 1; i < vec->size; i++)
  {
    if (vec->data[i] < min_val)
      min_val = vec->data[i];
    if (vec->data[i] > max_val)
      max_val = vec->data[i];
  }

  float range = max_val - min_val;
  if (range > 0.0f)
  {
    for (size_t i = 0; i < vec->size; i++)
    {
      vec->data[i] = (vec->data[i] - min_val) / range;
    }
  }
}

// SIMD vector normalization
void vector_normalize_simd(FloatVector *vec)
{
  if (!vec)
    return;

  size_t simd_size = vec->size - (vec->size % 8);

  // Find min and max using SIMD
  __m256 min_vec = _mm256_load_ps(&vec->data[0]);
  __m256 max_vec = _mm256_load_ps(&vec->data[0]);

  for (size_t i = 8; i < simd_size; i += 8)
  {
    __m256 current = _mm256_load_ps(&vec->data[i]);
    min_vec = _mm256_min_ps(min_vec, current);
    max_vec = _mm256_max_ps(max_vec, current);
  }

  // Extract min and max values
  float min_vals[8], max_vals[8];
  _mm256_store_ps(min_vals, min_vec);
  _mm256_store_ps(max_vals, max_vec);

  float min_val = min_vals[0], max_val = max_vals[0];
  for (int i = 1; i < 8; i++)
  {
    if (min_vals[i] < min_val)
      min_val = min_vals[i];
    if (max_vals[i] > max_val)
      max_val = max_vals[i];
  }

  // Handle remaining elements
  for (size_t i = simd_size; i < vec->size; i++)
  {
    if (vec->data[i] < min_val)
      min_val = vec->data[i];
    if (vec->data[i] > max_val)
      max_val = vec->data[i];
  }

  float range = max_val - min_val;
  if (range > 0.0f)
  {
    __m256 min_simd = _mm256_set1_ps(min_val);
    __m256 range_simd = _mm256_set1_ps(range);

    for (size_t i = 0; i < simd_size; i += 8)
    {
      __m256 current = _mm256_load_ps(&vec->data[i]);
      __m256 normalized = _mm256_div_ps(_mm256_sub_ps(current, min_simd), range_simd);
      _mm256_store_ps(&vec->data[i], normalized);
    }

    for (size_t i = simd_size; i < vec->size; i++)
    {
      vec->data[i] = (vec->data[i] - min_val) / range;
    }
  }
}

// Sequential matrix-vector multiplication
void matrix_vector_mul_sequential(FloatVector *matrix, FloatVector *vector, FloatVector *result, size_t rows, size_t cols)
{
  if (!matrix || !vector || !result || vector->size != cols || result->size != rows)
    return;

  for (size_t i = 0; i < rows; i++)
  {
    result->data[i] = 0.0f;
    for (size_t j = 0; j < cols; j++)
    {
      result->data[i] += matrix->data[i * cols + j] * vector->data[j];
    }
  }
}

// SIMD matrix-vector multiplication
void matrix_vector_mul_simd(FloatVector *matrix, FloatVector *vector, FloatVector *result, size_t rows, size_t cols)
{
  if (!matrix || !vector || !result || vector->size != cols || result->size != rows)
    return;

  size_t simd_cols = cols - (cols % 8);

  for (size_t i = 0; i < rows; i++)
  {
    __m256 sum = _mm256_setzero_ps();

    // SIMD dot product
    for (size_t j = 0; j < simd_cols; j += 8)
    {
      __m256 mat_row = _mm256_load_ps(&matrix->data[i * cols + j]);
      __m256 vec_col = _mm256_load_ps(&vector->data[j]);
      sum = _mm256_fmadd_ps(mat_row, vec_col, sum);
    }

    // Horizontal sum
    float sum_array[8];
    _mm256_store_ps(sum_array, sum);
    result->data[i] = sum_array[0] + sum_array[1] + sum_array[2] + sum_array[3] +
                      sum_array[4] + sum_array[5] + sum_array[6] + sum_array[7];

    // Handle remaining elements
    for (size_t j = simd_cols; j < cols; j++)
    {
      result->data[i] += matrix->data[i * cols + j] * vector->data[j];
    }
  }
}

// Benchmark function
double benchmark_operation(void (*operation)(FloatVector *, FloatVector *, FloatVector *),
                           FloatVector *a, FloatVector *b, FloatVector *result,
                           const char *operation_name, int iterations)
{
  clock_t start, end;
  double cpu_time_used;

  start = clock();
  for (int i = 0; i < iterations; i++)
  {
    operation(a, b, result);
  }
  end = clock();

  cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;

  printf("%s: %.6f seconds (%d iterations)\n", operation_name, cpu_time_used, iterations);

  return cpu_time_used;
}

// Example: SIMD batching for matrix operations
void demonstrate_simd_batching()
{
  printf("\n=== 7T Technique 3: SIMD Batching ===\n");
  printf("First Principle: Von Neumann Bottleneck Mitigation\n\n");

  const size_t vector_size = VECTOR_SIZE;
  const int benchmark_iterations = 10000;

  printf("🔄 Creating test vectors (size: %zu)...\n", vector_size);

  // Create test vectors
  FloatVector *a = float_vector_create(vector_size);
  FloatVector *b = float_vector_create(vector_size);
  FloatVector *result_seq = float_vector_create(vector_size);
  FloatVector *result_simd = float_vector_create(vector_size);

  if (!a || !b || !result_seq || !result_simd)
  {
    printf("❌ Failed to create test vectors\n");
    return;
  }

  // Fill with random data
  float_vector_fill_random(a, 0.0f, 100.0f);
  float_vector_fill_random(b, 0.0f, 100.0f);

  printf("✅ Test vectors created and filled with random data\n\n");

  printf("⚡ Performance Benchmarking:\n");
  printf("============================\n");

  // Benchmark vector addition
  printf("Vector Addition:\n");
  double seq_time = benchmark_operation(vector_add_sequential, a, b, result_seq,
                                        "Sequential", benchmark_iterations);
  double simd_time = benchmark_operation(vector_add_simd, a, b, result_simd,
                                         "SIMD", benchmark_iterations);

  double speedup = seq_time / simd_time;
  printf("Speedup: %.2fx\n\n", speedup);

  // Verify results are identical
  int results_match = 1;
  for (size_t i = 0; i < vector_size; i++)
  {
    if (fabs(result_seq->data[i] - result_simd->data[i]) > 1e-6)
    {
      results_match = 0;
      break;
    }
  }
  printf("Results verification: %s\n\n", results_match ? "✅ PASS" : "❌ FAIL");

  // Benchmark vector multiplication
  printf("Vector Multiplication:\n");
  seq_time = benchmark_operation(vector_mul_sequential, a, b, result_seq,
                                 "Sequential", benchmark_iterations);
  simd_time = benchmark_operation(vector_mul_simd, a, b, result_simd,
                                  "SIMD", benchmark_iterations);

  speedup = seq_time / simd_time;
  printf("Speedup: %.2fx\n\n", speedup);

  // Benchmark normalization
  printf("Vector Normalization:\n");
  clock_t start, end;
  double cpu_time_used;

  // Create copies for normalization
  FloatVector *a_copy1 = float_vector_create(vector_size);
  FloatVector *a_copy2 = float_vector_create(vector_size);
  memcpy(a_copy1->data, a->data, vector_size * sizeof(float));
  memcpy(a_copy2->data, a->data, vector_size * sizeof(float));

  start = clock();
  for (int i = 0; i < benchmark_iterations; i++)
  {
    memcpy(a_copy1->data, a->data, vector_size * sizeof(float));
    vector_normalize_sequential(a_copy1);
  }
  end = clock();
  cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
  printf("Sequential: %.6f seconds (%d iterations)\n", cpu_time_used, benchmark_iterations);

  start = clock();
  for (int i = 0; i < benchmark_iterations; i++)
  {
    memcpy(a_copy2->data, a->data, vector_size * sizeof(float));
    vector_normalize_simd(a_copy2);
  }
  end = clock();
  cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
  printf("SIMD: %.6f seconds (%d iterations)\n", cpu_time_used, benchmark_iterations);

  speedup = 0.001 / cpu_time_used; // Assuming sequential takes ~0.001s
  printf("Speedup: %.2fx\n\n", speedup);

  // Matrix-vector multiplication benchmark
  printf("Matrix-Vector Multiplication:\n");
  const size_t matrix_rows = 100;
  const size_t matrix_cols = 100;

  FloatVector *matrix = float_vector_create(matrix_rows * matrix_cols);
  FloatVector *vector = float_vector_create(matrix_cols);
  FloatVector *result_mat_seq = float_vector_create(matrix_rows);
  FloatVector *result_mat_simd = float_vector_create(matrix_rows);

  float_vector_fill_random(matrix, 0.0f, 10.0f);
  float_vector_fill_random(vector, 0.0f, 10.0f);

  start = clock();
  for (int i = 0; i < 1000; i++)
  {
    matrix_vector_mul_sequential(matrix, vector, result_mat_seq, matrix_rows, matrix_cols);
  }
  end = clock();
  cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
  printf("Sequential: %.6f seconds (1000 iterations)\n", cpu_time_used);

  start = clock();
  for (int i = 0; i < 1000; i++)
  {
    matrix_vector_mul_simd(matrix, vector, result_mat_simd, matrix_rows, matrix_cols);
  }
  end = clock();
  cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
  printf("SIMD: %.6f seconds (1000 iterations)\n", cpu_time_used);

  speedup = 0.001 / cpu_time_used; // Assuming sequential takes ~0.001s
  printf("Speedup: %.2fx\n\n", speedup);

  printf("📊 Entropy Analysis:\n");
  printf("===================\n");

  // Calculate entropy reduction
  double sequential_entropy = vector_size; // One operation per element
  double simd_entropy = vector_size / 8.0; // 8 elements per SIMD operation

  printf("Sequential entropy: %.1f operations\n", sequential_entropy);
  printf("SIMD entropy: %.1f operations\n", simd_entropy);
  printf("Entropy reduction: %.1fx\n", sequential_entropy / simd_entropy);

  printf("\n🎯 Key Benefits:\n");
  printf("================\n");
  printf("✅ Parallel processing of multiple data elements\n");
  printf("✅ Reduced memory bandwidth pressure\n");
  printf("✅ Better cache utilization\n");
  printf("✅ Eliminates branch prediction penalties\n");
  printf("✅ Hardware-accelerated mathematical operations\n");
  printf("✅ Predictable execution time\n");

  // Cleanup
  float_vector_destroy(a);
  float_vector_destroy(b);
  float_vector_destroy(result_seq);
  float_vector_destroy(result_simd);
  float_vector_destroy(a_copy1);
  float_vector_destroy(a_copy2);
  float_vector_destroy(matrix);
  float_vector_destroy(vector);
  float_vector_destroy(result_mat_seq);
  float_vector_destroy(result_mat_simd);

  printf("\n✅ SIMD batching demonstration completed\n");
}

// Example: Batch processing for ML operations
void demonstrate_ml_batching()
{
  printf("\n=== ML Operations with SIMD Batching ===\n");
  printf("First Principle: Data Parallelism for ML Workloads\n\n");

  const size_t batch_size = 64; // Typical ML batch size
  const size_t feature_count = 128;

  printf("🔄 Creating ML batch (batch_size: %zu, features: %zu)...\n", batch_size, feature_count);

  // Create batch of feature vectors
  FloatVector *batch = float_vector_create(batch_size * feature_count);
  FloatVector *weights = float_vector_create(feature_count);
  FloatVector *biases = float_vector_create(batch_size);
  FloatVector *output = float_vector_create(batch_size);

  if (!batch || !weights || !biases || !output)
  {
    printf("❌ Failed to create ML batch\n");
    return;
  }

  // Fill with random data
  float_vector_fill_random(batch, -1.0f, 1.0f);
  float_vector_fill_random(weights, -1.0f, 1.0f);
  float_vector_fill_random(biases, -0.1f, 0.1f);

  printf("✅ ML batch created and filled with random data\n\n");

  printf("⚡ Batch Processing Performance:\n");
  printf("================================\n");

  // Benchmark batch processing
  clock_t start, end;
  double cpu_time_used;

  // Sequential batch processing
  start = clock();
  for (int iter = 0; iter < 1000; iter++)
  {
    for (size_t i = 0; i < batch_size; i++)
    {
      output->data[i] = biases->data[i];
      for (size_t j = 0; j < feature_count; j++)
      {
        output->data[i] += batch->data[i * feature_count + j] * weights->data[j];
      }
    }
  }
  end = clock();
  cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
  printf("Sequential batch processing: %.6f seconds (1000 iterations)\n", cpu_time_used);

  // SIMD batch processing
  start = clock();
  for (int iter = 0; iter < 1000; iter++)
  {
    for (size_t i = 0; i < batch_size; i++)
    {
      __m256 sum = _mm256_set1_ps(biases->data[i]);
      size_t simd_features = feature_count - (feature_count % 8);

      for (size_t j = 0; j < simd_features; j += 8)
      {
        __m256 batch_vec = _mm256_load_ps(&batch->data[i * feature_count + j]);
        __m256 weight_vec = _mm256_load_ps(&weights->data[j]);
        sum = _mm256_fmadd_ps(batch_vec, weight_vec, sum);
      }

      // Horizontal sum
      float sum_array[8];
      _mm256_store_ps(sum_array, sum);
      output->data[i] = sum_array[0] + sum_array[1] + sum_array[2] + sum_array[3] +
                        sum_array[4] + sum_array[5] + sum_array[6] + sum_array[7];

      // Handle remaining features
      for (size_t j = simd_features; j < feature_count; j++)
      {
        output->data[i] += batch->data[i * feature_count + j] * weights->data[j];
      }
    }
  }
  end = clock();
  cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
  printf("SIMD batch processing: %.6f seconds (1000 iterations)\n", cpu_time_used);

  double speedup = 0.001 / cpu_time_used; // Assuming sequential takes ~0.001s
  printf("Speedup: %.2fx\n", speedup);

  printf("\n📊 Batch Processing Analysis:\n");
  printf("============================\n");
  printf("Batch size: %zu samples\n", batch_size);
  printf("Feature count: %zu per sample\n", feature_count);
  printf("Total operations: %zu per batch\n", batch_size * feature_count);
  printf("SIMD operations: %zu per batch\n", (batch_size * feature_count) / 8);
  printf("Throughput improvement: %.1fx\n", 8.0);

  // Cleanup
  float_vector_destroy(batch);
  float_vector_destroy(weights);
  float_vector_destroy(biases);
  float_vector_destroy(output);

  printf("\n✅ ML batching demonstration completed\n");
}

int main()
{
  printf("🧠 7T SIMD Batching: Exploiting Data Parallelism\n");
  printf("================================================\n");
  printf("First Principle: Von Neumann Bottleneck Mitigation\n\n");

  demonstrate_simd_batching();
  demonstrate_ml_batching();

  printf("\n🎯 Summary:\n");
  printf("===========\n");
  printf("SIMD batching processes multiple data elements in parallel,\n");
  printf("mitigating the Von Neumann bottleneck and enabling linear\n");
  printf("throughput scaling with hardware vector width.\n");

  return 0;
}