#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include "../runtime/src/seven_t_runtime.h"

// High-precision timing
static inline uint64_t get_microseconds()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000000ULL + tv.tv_usec;
}

// Compressed Sparse Row (CSR) structure
typedef struct
{
  uint32_t *row_ptr;
  uint32_t *col_ind;
  uint32_t *values;
  size_t num_rows;
  size_t num_cols;
  size_t nnz;
} CSRMatrix;

// Run-length encoded bit vector
typedef struct
{
  uint32_t run_length;
  uint8_t value;
} RLEEntry;

typedef struct
{
  RLEEntry *entries;
  size_t count;
  size_t capacity;
} RLEBitVector;

// Dictionary encoding
typedef struct
{
  uint32_t *values;
  uint32_t *map;
  size_t count;
  size_t capacity;
} Dictionary;

// Create CSR matrix from sparse data
CSRMatrix *create_csr_matrix(size_t num_rows, size_t num_cols)
{
  CSRMatrix *matrix = malloc(sizeof(CSRMatrix));
  matrix->num_rows = num_rows;
  matrix->num_cols = num_cols;
  matrix->nnz = 0;

  // Estimate non-zeros (10% sparsity)
  size_t estimated_nnz = (num_rows * num_cols) / 10;
  matrix->row_ptr = calloc(num_rows + 1, sizeof(uint32_t));
  matrix->col_ind = malloc(estimated_nnz * sizeof(uint32_t));
  matrix->values = malloc(estimated_nnz * sizeof(uint32_t));

  return matrix;
}

// Add entry to CSR matrix
void csr_add_entry(CSRMatrix *matrix, uint32_t row, uint32_t col, uint32_t value)
{
  if (matrix->nnz >= matrix->num_rows * matrix->num_cols / 10)
  {
    // Reallocate if needed
    size_t new_size = matrix->nnz * 2;
    matrix->col_ind = realloc(matrix->col_ind, new_size * sizeof(uint32_t));
    matrix->values = realloc(matrix->values, new_size * sizeof(uint32_t));
  }

  matrix->col_ind[matrix->nnz] = col;
  matrix->values[matrix->nnz] = value;
  matrix->row_ptr[row + 1]++;
  matrix->nnz++;
}

// Finalize CSR matrix (convert row_ptr to cumulative)
void csr_finalize(CSRMatrix *matrix)
{
  for (size_t i = 1; i <= matrix->num_rows; i++)
  {
    matrix->row_ptr[i] += matrix->row_ptr[i - 1];
  }
}

// Lookup in CSR matrix
int csr_lookup(CSRMatrix *matrix, uint32_t row, uint32_t col)
{
  uint32_t start = matrix->row_ptr[row];
  uint32_t end = matrix->row_ptr[row + 1];

  // Binary search
  while (start < end)
  {
    uint32_t mid = (start + end) / 2;
    if (matrix->col_ind[mid] == col)
    {
      return matrix->values[mid];
    }
    else if (matrix->col_ind[mid] < col)
    {
      start = mid + 1;
    }
    else
    {
      end = mid;
    }
  }
  return -1; // Not found
}

// Create RLE bit vector
RLEBitVector *create_rle_bitvector()
{
  RLEBitVector *rle = malloc(sizeof(RLEBitVector));
  rle->capacity = 1000;
  rle->count = 0;
  rle->entries = malloc(rle->capacity * sizeof(RLEEntry));
  return rle;
}

// Add run to RLE
void rle_add_run(RLEBitVector *rle, uint32_t length, uint8_t value)
{
  if (rle->count >= rle->capacity)
  {
    rle->capacity *= 2;
    rle->entries = realloc(rle->entries, rle->capacity * sizeof(RLEEntry));
  }

  rle->entries[rle->count].run_length = length;
  rle->entries[rle->count].value = value;
  rle->count++;
}

// Test bit in RLE
int rle_test_bit(RLEBitVector *rle, uint32_t index)
{
  uint32_t current_pos = 0;
  for (size_t i = 0; i < rle->count; i++)
  {
    if (index < current_pos + rle->entries[i].run_length)
    {
      return rle->entries[i].value;
    }
    current_pos += rle->entries[i].run_length;
  }
  return 0;
}

// Create dictionary
Dictionary *create_dictionary()
{
  Dictionary *dict = malloc(sizeof(Dictionary));
  dict->capacity = 1000;
  dict->count = 0;
  dict->values = malloc(dict->capacity * sizeof(uint32_t));
  dict->map = malloc(dict->capacity * sizeof(uint32_t));
  return dict;
}

// Add value to dictionary
uint32_t dict_add_value(Dictionary *dict, uint32_t value)
{
  // Check if value already exists
  for (size_t i = 0; i < dict->count; i++)
  {
    if (dict->values[i] == value)
    {
      return i;
    }
  }

  // Add new value
  if (dict->count >= dict->capacity)
  {
    dict->capacity *= 2;
    dict->values = realloc(dict->values, dict->capacity * sizeof(uint32_t));
    dict->map = realloc(dict->map, dict->capacity * sizeof(uint32_t));
  }

  dict->values[dict->count] = value;
  dict->map[dict->count] = dict->count;
  return dict->count++;
}

// Lookup in dictionary
uint32_t dict_lookup(Dictionary *dict, uint32_t encoded_value)
{
  if (encoded_value < dict->count)
  {
    return dict->values[encoded_value];
  }
  return encoded_value; // Fallback
}

// Test compressed data structures
int main()
{
  printf("7T Compression Benchmark\n");
  printf("========================\n\n");

  // Test CSR matrix compression
  printf("Testing CSR Matrix Compression...\n");

  size_t num_rows = 10000;
  size_t num_cols = 10000;

  CSRMatrix *csr = create_csr_matrix(num_rows, num_cols);

  // Add sparse data (10% density)
  uint64_t start = get_microseconds();

  for (size_t i = 0; i < num_rows; i++)
  {
    for (size_t j = 0; j < num_cols; j++)
    {
      if ((i + j) % 10 == 0)
      { // 10% sparsity
        csr_add_entry(csr, i, j, (i * num_cols + j) % 1000);
      }
    }
  }

  csr_finalize(csr);

  uint64_t end = get_microseconds();
  double csr_build_time = (end - start) / 1000000.0;

  printf("CSR matrix built in %.3f seconds\n", csr_build_time);
  printf("Matrix size: %zux%zu, Non-zeros: %zu\n", num_rows, num_cols, csr->nnz);
  printf("Compression ratio: %.2f%%\n",
         (double)csr->nnz / (num_rows * num_cols) * 100);

  // Test CSR lookup performance
  printf("\nTesting CSR lookup performance...\n");

  start = get_microseconds();
  int iterations = 100000;

  for (int i = 0; i < iterations; i++)
  {
    uint32_t row = i % num_rows;
    uint32_t col = i % num_cols;
    int result = csr_lookup(csr, row, col);
    (void)result; // Prevent unused variable warning
  }

  end = get_microseconds();
  double csr_lookup_time = (end - start) / 1000000.0;

  printf("Executed %d CSR lookups in %.3f seconds\n", iterations, csr_lookup_time);
  printf("CSR lookup rate: %.0f ops/sec\n", iterations / csr_lookup_time);

  // Test RLE bit vector compression
  printf("\nTesting RLE Bit Vector Compression...\n");

  RLEBitVector *rle = create_rle_bitvector();

  // Create sparse bit vector with runs
  start = get_microseconds();

  uint32_t total_bits = 1000000;
  uint32_t current_pos = 0;

  while (current_pos < total_bits)
  {
    uint32_t run_length = 1 + (rand() % 1000);  // Random run length
    uint8_t value = (rand() % 10 == 0) ? 1 : 0; // 10% ones

    if (current_pos + run_length > total_bits)
    {
      run_length = total_bits - current_pos;
    }

    rle_add_run(rle, run_length, value);
    current_pos += run_length;
  }

  end = get_microseconds();
  double rle_build_time = (end - start) / 1000000.0;

  printf("RLE bit vector built in %.3f seconds\n", rle_build_time);
  printf("Total bits: %u, RLE entries: %zu\n", total_bits, rle->count);
  printf("Compression ratio: %.2f%%\n",
         (double)rle->count / (total_bits / 8) * 100);

  // Test RLE lookup performance
  printf("\nTesting RLE lookup performance...\n");

  start = get_microseconds();

  for (int i = 0; i < iterations; i++)
  {
    uint32_t bit_index = i % total_bits;
    int result = rle_test_bit(rle, bit_index);
    (void)result; // Prevent unused variable warning
  }

  end = get_microseconds();
  double rle_lookup_time = (end - start) / 1000000.0;

  printf("Executed %d RLE lookups in %.3f seconds\n", iterations, rle_lookup_time);
  printf("RLE lookup rate: %.0f ops/sec\n", iterations / rle_lookup_time);

  // Test dictionary encoding
  printf("\nTesting Dictionary Encoding...\n");

  Dictionary *dict = create_dictionary();

  // Add repeated values
  start = get_microseconds();

  for (int i = 0; i < 100000; i++)
  {
    uint32_t value = i % 1000; // Many repeated values
    uint32_t encoded = dict_add_value(dict, value);
    (void)encoded; // Prevent unused variable warning
  }

  end = get_microseconds();
  double dict_build_time = (end - start) / 1000000.0;

  printf("Dictionary built in %.3f seconds\n", dict_build_time);
  printf("Unique values: %zu out of 100,000\n", dict->count);
  printf("Compression ratio: %.2f%%\n",
         (double)dict->count / 100000 * 100);

  // Test dictionary lookup performance
  printf("\nTesting dictionary lookup performance...\n");

  start = get_microseconds();

  for (int i = 0; i < iterations; i++)
  {
    uint32_t encoded_value = i % dict->count;
    uint32_t result = dict_lookup(dict, encoded_value);
    (void)result; // Prevent unused variable warning
  }

  end = get_microseconds();
  double dict_lookup_time = (end - start) / 1000000.0;

  printf("Executed %d dictionary lookups in %.3f seconds\n", iterations, dict_lookup_time);
  printf("Dictionary lookup rate: %.0f ops/sec\n", iterations / dict_lookup_time);

  // Memory usage comparison
  printf("\nMemory Usage Comparison:\n");
  printf("CSR Matrix: %zu bytes (%.1f MB)\n",
         csr->nnz * sizeof(uint32_t) * 2 + (num_rows + 1) * sizeof(uint32_t),
         (csr->nnz * sizeof(uint32_t) * 2 + (num_rows + 1) * sizeof(uint32_t)) / (1024.0 * 1024.0));
  printf("RLE Bit Vector: %zu bytes (%.1f MB)\n",
         rle->count * sizeof(RLEEntry),
         (rle->count * sizeof(RLEEntry)) / (1024.0 * 1024.0));
  printf("Dictionary: %zu bytes (%.1f MB)\n",
         dict->count * sizeof(uint32_t) * 2,
         (dict->count * sizeof(uint32_t) * 2) / (1024.0 * 1024.0));

  // Cleanup
  free(csr->row_ptr);
  free(csr->col_ind);
  free(csr->values);
  free(csr);

  free(rle->entries);
  free(rle);

  free(dict->values);
  free(dict->map);
  free(dict);

  printf("\n✅ Compression Benchmark Complete!\n");
  return 0;
}