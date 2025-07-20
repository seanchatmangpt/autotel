/**
 * @file test_numerical.c
 * @brief Numerical Precision and SIMD Tests for 8T
 * 
 * Comprehensive testing of numerical operations with SIMD optimization
 * and precision validation for the 8T substrate.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <float.h>
#include <assert.h>
#include <immintrin.h>
#include <time.h>

// SIMD configuration
#define SIMD_WIDTH_AVX2 8    // 8 floats in AVX2
#define SIMD_WIDTH_SSE 4     // 4 floats in SSE
#define ALIGNMENT 32         // AVX2 alignment

// Precision thresholds
#define EPSILON_SINGLE 1e-6f
#define EPSILON_DOUBLE 1e-15
#define MAX_ULP_DIFF 4       // Maximum ULPs for equality

// Test data size
#define TEST_SIZE 8192
#define ITERATIONS 10000

// Performance measurement
static inline uint64_t rdtsc(void) {
    unsigned int lo, hi;
    __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
    return ((uint64_t)hi << 32) | lo;
}

// ULP (Units in the Last Place) comparison
int float_equal_ulp(float a, float b, int max_ulps) {
    if (isnan(a) || isnan(b)) return isnan(a) && isnan(b);
    if (isinf(a) || isinf(b)) return (a == b);
    
    int32_t a_int = *(int32_t*)&a;
    int32_t b_int = *(int32_t*)&b;
    
    // Handle sign bit
    if (a_int < 0) a_int = 0x80000000 - a_int;
    if (b_int < 0) b_int = 0x80000000 - b_int;
    
    return abs(a_int - b_int) <= max_ulps;
}

// Scalar vector operations
void vector_add_scalar(const float *a, const float *b, float *result, size_t n) {
    for (size_t i = 0; i < n; i++) {
        result[i] = a[i] + b[i];
    }
}

void vector_mul_scalar(const float *a, const float *b, float *result, size_t n) {
    for (size_t i = 0; i < n; i++) {
        result[i] = a[i] * b[i];
    }
}

float vector_dot_scalar(const float *a, const float *b, size_t n) {
    float sum = 0.0f;
    for (size_t i = 0; i < n; i++) {
        sum += a[i] * b[i];
    }
    return sum;
}

// AVX2 SIMD operations
void vector_add_avx2(const float *a, const float *b, float *result, size_t n) {
    size_t simd_end = n - (n % SIMD_WIDTH_AVX2);
    
    for (size_t i = 0; i < simd_end; i += SIMD_WIDTH_AVX2) {
        __m256 va = _mm256_load_ps(&a[i]);
        __m256 vb = _mm256_load_ps(&b[i]);
        __m256 vr = _mm256_add_ps(va, vb);
        _mm256_store_ps(&result[i], vr);
    }
    
    // Handle remaining elements
    for (size_t i = simd_end; i < n; i++) {
        result[i] = a[i] + b[i];
    }
}

void vector_mul_avx2(const float *a, const float *b, float *result, size_t n) {
    size_t simd_end = n - (n % SIMD_WIDTH_AVX2);
    
    for (size_t i = 0; i < simd_end; i += SIMD_WIDTH_AVX2) {
        __m256 va = _mm256_load_ps(&a[i]);
        __m256 vb = _mm256_load_ps(&b[i]);
        __m256 vr = _mm256_mul_ps(va, vb);
        _mm256_store_ps(&result[i], vr);
    }
    
    // Handle remaining elements
    for (size_t i = simd_end; i < n; i++) {
        result[i] = a[i] * b[i];
    }
}

float vector_dot_avx2(const float *a, const float *b, size_t n) {
    __m256 sum = _mm256_setzero_ps();
    size_t simd_end = n - (n % SIMD_WIDTH_AVX2);
    
    for (size_t i = 0; i < simd_end; i += SIMD_WIDTH_AVX2) {
        __m256 va = _mm256_load_ps(&a[i]);
        __m256 vb = _mm256_load_ps(&b[i]);
        __m256 prod = _mm256_mul_ps(va, vb);
        sum = _mm256_add_ps(sum, prod);
    }
    
    // Horizontal sum
    __m128 low = _mm256_castps256_ps128(sum);
    __m128 high = _mm256_extractf128_ps(sum, 1);
    __m128 sum128 = _mm_add_ps(low, high);
    
    sum128 = _mm_hadd_ps(sum128, sum128);
    sum128 = _mm_hadd_ps(sum128, sum128);
    
    float result = _mm_cvtss_f32(sum128);
    
    // Handle remaining elements
    for (size_t i = simd_end; i < n; i++) {
        result += a[i] * b[i];
    }
    
    return result;
}

// Test SIMD vs scalar precision
int test_simd_precision(void) {
    printf("Testing SIMD vs scalar precision...\n");
    
    // Aligned memory allocation
    float *a = aligned_alloc(ALIGNMENT, TEST_SIZE * sizeof(float));
    float *b = aligned_alloc(ALIGNMENT, TEST_SIZE * sizeof(float));
    float *result_scalar = aligned_alloc(ALIGNMENT, TEST_SIZE * sizeof(float));
    float *result_simd = aligned_alloc(ALIGNMENT, TEST_SIZE * sizeof(float));
    
    // Initialize test data
    srand(42);
    for (size_t i = 0; i < TEST_SIZE; i++) {
        a[i] = (float)rand() / RAND_MAX * 100.0f - 50.0f;
        b[i] = (float)rand() / RAND_MAX * 100.0f - 50.0f;
    }
    
    // Test vector addition
    vector_add_scalar(a, b, result_scalar, TEST_SIZE);
    vector_add_avx2(a, b, result_simd, TEST_SIZE);
    
    int add_errors = 0;
    for (size_t i = 0; i < TEST_SIZE; i++) {
        if (!float_equal_ulp(result_scalar[i], result_simd[i], MAX_ULP_DIFF)) {
            add_errors++;
        }
    }
    
    // Test vector multiplication
    vector_mul_scalar(a, b, result_scalar, TEST_SIZE);
    vector_mul_avx2(a, b, result_simd, TEST_SIZE);
    
    int mul_errors = 0;
    for (size_t i = 0; i < TEST_SIZE; i++) {
        if (!float_equal_ulp(result_scalar[i], result_simd[i], MAX_ULP_DIFF)) {
            mul_errors++;
        }
    }
    
    // Test dot product
    float dot_scalar = vector_dot_scalar(a, b, TEST_SIZE);
    float dot_simd = vector_dot_avx2(a, b, TEST_SIZE);
    
    int dot_error = !float_equal_ulp(dot_scalar, dot_simd, MAX_ULP_DIFF * 10);
    
    printf("Vector addition precision errors: %d/%zu (%.3f%%)\n", 
           add_errors, TEST_SIZE, (double)add_errors / TEST_SIZE * 100);
    printf("Vector multiplication precision errors: %d/%zu (%.3f%%)\n", 
           mul_errors, TEST_SIZE, (double)mul_errors / TEST_SIZE * 100);
    printf("Dot product precision error: %s\n", dot_error ? "YES" : "NO");
    printf("Dot product scalar: %.6f, SIMD: %.6f\n", dot_scalar, dot_simd);
    
    free(a);
    free(b);
    free(result_scalar);
    free(result_simd);
    
    if (add_errors == 0 && mul_errors == 0 && !dot_error) {
        printf("✓ SIMD precision matches scalar within ULP tolerance\n");
        return 0;
    } else {
        printf("✗ SIMD precision errors detected\n");
        return -1;
    }
}

// Test numerical stability
int test_numerical_stability(void) {
    printf("Testing numerical stability...\n");
    
    // Test with very small numbers
    float small_a = 1e-30f;
    float small_b = 1e-30f;
    float small_result = small_a + small_b;
    
    if (small_result == 0.0f) {
        printf("⚠ Underflow detected with very small numbers\n");
    }
    
    // Test with very large numbers
    float large_a = 1e30f;
    float large_b = 1e30f;
    float large_result = large_a + large_b;
    
    if (isinf(large_result)) {
        printf("⚠ Overflow detected with very large numbers\n");
    }
    
    // Test catastrophic cancellation
    float x = 1.0f;
    float y = 1.0f - 1e-7f;
    float diff = x - y;
    float expected = 1e-7f;
    
    float relative_error = fabsf((diff - expected) / expected);
    printf("Catastrophic cancellation test - relative error: %.2e\n", relative_error);
    
    // Test accumulation precision
    float sum = 0.0f;
    for (int i = 0; i < 1000000; i++) {
        sum += 1e-7f;
    }
    float expected_sum = 1000000.0f * 1e-7f;
    relative_error = fabsf((sum - expected_sum) / expected_sum);
    printf("Accumulation precision test - relative error: %.2e\n", relative_error);
    
    printf("✓ Numerical stability tests completed\n");
    return 0;
}

// Performance benchmark
int benchmark_simd_performance(void) {
    printf("Benchmarking SIMD performance...\n");
    
    float *a = aligned_alloc(ALIGNMENT, TEST_SIZE * sizeof(float));
    float *b = aligned_alloc(ALIGNMENT, TEST_SIZE * sizeof(float));
    float *result = aligned_alloc(ALIGNMENT, TEST_SIZE * sizeof(float));
    
    // Initialize data
    for (size_t i = 0; i < TEST_SIZE; i++) {
        a[i] = (float)i;
        b[i] = (float)(i + 1);
    }
    
    uint64_t scalar_time, simd_time;
    
    // Benchmark scalar addition
    uint64_t start = rdtsc();
    for (int i = 0; i < ITERATIONS; i++) {
        vector_add_scalar(a, b, result, TEST_SIZE);
    }
    uint64_t end = rdtsc();
    scalar_time = end - start;
    
    // Benchmark SIMD addition
    start = rdtsc();
    for (int i = 0; i < ITERATIONS; i++) {
        vector_add_avx2(a, b, result, TEST_SIZE);
    }
    end = rdtsc();
    simd_time = end - start;
    
    printf("Scalar addition time: %lu cycles\n", scalar_time);
    printf("SIMD addition time: %lu cycles\n", simd_time);
    printf("SIMD speedup: %.2fx\n", (double)scalar_time / simd_time);
    
    // Benchmark scalar multiplication
    start = rdtsc();
    for (int i = 0; i < ITERATIONS; i++) {
        vector_mul_scalar(a, b, result, TEST_SIZE);
    }
    end = rdtsc();
    scalar_time = end - start;
    
    // Benchmark SIMD multiplication
    start = rdtsc();
    for (int i = 0; i < ITERATIONS; i++) {
        vector_mul_avx2(a, b, result, TEST_SIZE);
    }
    end = rdtsc();
    simd_time = end - start;
    
    printf("Scalar multiplication time: %lu cycles\n", scalar_time);
    printf("SIMD multiplication time: %lu cycles\n", simd_time);
    printf("SIMD speedup: %.2fx\n", (double)scalar_time / simd_time);
    
    free(a);
    free(b);
    free(result);
    
    printf("✓ SIMD performance benchmarks completed\n");
    return 0;
}

// Test edge cases
int test_edge_cases(void) {
    printf("Testing numerical edge cases...\n");
    
    float test_values[] = {
        0.0f, -0.0f, INFINITY, -INFINITY, NAN,
        FLT_MIN, FLT_MAX, -FLT_MIN, -FLT_MAX,
        1.0f, -1.0f, 0.5f, -0.5f
    };
    int num_values = sizeof(test_values) / sizeof(test_values[0]);
    
    int edge_case_errors = 0;
    
    for (int i = 0; i < num_values; i++) {
        for (int j = 0; j < num_values; j++) {
            float a = test_values[i];
            float b = test_values[j];
            
            // Test addition
            float scalar_add = a + b;
            float simd_array_a[8] __attribute__((aligned(32))) = {a, a, a, a, a, a, a, a};
            float simd_array_b[8] __attribute__((aligned(32))) = {b, b, b, b, b, b, b, b};
            float simd_result[8] __attribute__((aligned(32)));
            
            vector_add_avx2(simd_array_a, simd_array_b, simd_result, 8);
            
            // Compare results (handle NaN specially)
            if (isnan(scalar_add)) {
                if (!isnan(simd_result[0])) edge_case_errors++;
            } else if (isinf(scalar_add)) {
                if (scalar_add != simd_result[0]) edge_case_errors++;
            } else {
                if (!float_equal_ulp(scalar_add, simd_result[0], MAX_ULP_DIFF)) {
                    edge_case_errors++;
                }
            }
        }
    }
    
    printf("Edge case errors: %d\n", edge_case_errors);
    
    if (edge_case_errors == 0) {
        printf("✓ All edge cases handled correctly\n");
        return 0;
    } else {
        printf("✗ Some edge cases failed\n");
        return -1;
    }
}

// Test SIMD capability detection
int test_simd_capabilities(void) {
    printf("Testing SIMD capability detection...\n");
    
    // Check CPUID for SIMD support
    unsigned int eax, ebx, ecx, edx;
    
    // Check for SSE support
    __asm__ ("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    int sse_support = (edx >> 25) & 1;
    int sse2_support = (edx >> 26) & 1;
    int sse3_support = ecx & 1;
    
    // Check for AVX support
    __asm__ ("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    int avx_support = (ecx >> 28) & 1;
    
    // Check for AVX2 support
    __asm__ ("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
    int avx2_support = (ebx >> 5) & 1;
    
    printf("SSE support: %s\n", sse_support ? "YES" : "NO");
    printf("SSE2 support: %s\n", sse2_support ? "YES" : "NO");
    printf("SSE3 support: %s\n", sse3_support ? "YES" : "NO");
    printf("AVX support: %s\n", avx_support ? "YES" : "NO");
    printf("AVX2 support: %s\n", avx2_support ? "YES" : "NO");
    
    if (avx2_support) {
        printf("✓ AVX2 SIMD capabilities available for 8T optimization\n");
        return 0;
    } else {
        printf("⚠ AVX2 not available, falling back to scalar operations\n");
        return 0; // Not an error, just a limitation
    }
}

int main(void) {
    printf("=== 8T Numerical Precision and SIMD Tests ===\n\n");
    
    int result = 0;
    
    result |= test_simd_capabilities();
    printf("\n");
    
    result |= test_simd_precision();
    printf("\n");
    
    result |= test_numerical_stability();
    printf("\n");
    
    result |= test_edge_cases();
    printf("\n");
    
    result |= benchmark_simd_performance();
    printf("\n");
    
    if (result == 0) {
        printf("✅ All numerical and SIMD tests passed!\n");
    } else {
        printf("❌ Some tests failed\n");
    }
    
    return result;
}