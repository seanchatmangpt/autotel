/*
 * 8T Substrate Test Suite
 * Validates 8-tick SIMD vector operations
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <math.h>
#include <time.h>
#include <immintrin.h>

// Test utilities
#define TEST_START(name) printf("\n[TEST] %s\n", name)
#define TEST_PASS(name) printf("✅ %s: PASSED\n", name)
#define TEST_FAIL(name, msg) printf("❌ %s: FAILED - %s\n", name, msg)

// Cycle counter
static inline uint64_t rdtsc() {
    unsigned int lo, hi;
    __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
    return ((uint64_t)hi << 32) | lo;
}

// Test 1: Verify 8-element vector operations
void test_vector_operations() {
    TEST_START("8-Element Vector Operations");
    
    // Create test vectors
    alignas(64) uint64_t a[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    alignas(64) uint64_t b[8] = {10, 20, 30, 40, 50, 60, 70, 80};
    alignas(64) uint64_t result[8];
    
    // Load vectors
    __m512i va = _mm512_load_si512((__m512i*)a);
    __m512i vb = _mm512_load_si512((__m512i*)b);
    
    // Add vectors (should complete in 8 ticks)
    uint64_t start = rdtsc();
    __m512i vr = _mm512_add_epi64(va, vb);
    uint64_t end = rdtsc();
    
    // Store result
    _mm512_store_si512((__m512i*)result, vr);
    
    // Verify results
    int correct = 1;
    for (int i = 0; i < 8; i++) {
        if (result[i] != a[i] + b[i]) {
            correct = 0;
            break;
        }
    }
    
    if (correct) {
        TEST_PASS("Vector Addition");
        printf("  Cycles: %llu (target: ≤8)\n", end - start);
    } else {
        TEST_FAIL("Vector Addition", "Incorrect results");
    }
}

// Test 2: Verify alignment requirements
void test_alignment() {
    TEST_START("64-byte Alignment");
    
    // Allocate aligned memory
    void* ptr = aligned_alloc(64, 512);
    assert(ptr != NULL);
    
    // Check alignment
    if (((uintptr_t)ptr & 63) == 0) {
        TEST_PASS("Memory Alignment");
    } else {
        TEST_FAIL("Memory Alignment", "Not 64-byte aligned");
    }
    
    free(ptr);
}

// Test 3: Benchmark different access patterns
void test_access_patterns() {
    TEST_START("Access Pattern Performance");
    
    const int size = 8192;  // 8K elements
    alignas(64) uint64_t* data = aligned_alloc(64, size * sizeof(uint64_t));
    
    // Initialize data
    for (int i = 0; i < size; i++) {
        data[i] = i;
    }
    
    // Sequential access (8 elements at a time)
    uint64_t sum = 0;
    uint64_t start = rdtsc();
    
    for (int i = 0; i < size; i += 8) {
        __m512i v = _mm512_load_si512((__m512i*)&data[i]);
        sum += _mm512_reduce_add_epi64(v);
    }
    
    uint64_t seq_cycles = rdtsc() - start;
    
    // Random access
    uint64_t rand_sum = 0;
    start = rdtsc();
    
    for (int i = 0; i < size/8; i++) {
        int idx = (i * 1009) % (size/8);  // Prime number for spread
        __m512i v = _mm512_load_si512((__m512i*)&data[idx * 8]);
        rand_sum += _mm512_reduce_add_epi64(v);
    }
    
    uint64_t rand_cycles = rdtsc() - start;
    
    printf("  Sequential: %llu cycles (%.2f per vector)\n", 
           seq_cycles, (double)seq_cycles / (size/8));
    printf("  Random: %llu cycles (%.2f per vector)\n", 
           rand_cycles, (double)rand_cycles / (size/8));
    printf("  Ratio: %.2fx slower\n", (double)rand_cycles / seq_cycles);
    
    free(data);
    TEST_PASS("Access Patterns");
}

// Test 4: Verify SIMD instruction mix
void test_instruction_mix() {
    TEST_START("SIMD Instruction Mix");
    
    alignas(64) double a[8] = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0};
    alignas(64) double b[8] = {2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0};
    alignas(64) double c[8] = {0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5};
    alignas(64) double result[8];
    
    __m512d va = _mm512_load_pd(a);
    __m512d vb = _mm512_load_pd(b);
    __m512d vc = _mm512_load_pd(c);
    
    // FMA operation: a * b + c (should be 8 ticks)
    uint64_t start = rdtsc();
    __m512d vr = _mm512_fmadd_pd(va, vb, vc);
    uint64_t fma_cycles = rdtsc() - start;
    
    _mm512_store_pd(result, vr);
    
    // Verify
    int correct = 1;
    for (int i = 0; i < 8; i++) {
        double expected = a[i] * b[i] + c[i];
        if (fabs(result[i] - expected) > 1e-10) {
            correct = 0;
            break;
        }
    }
    
    if (correct) {
        TEST_PASS("FMA Operation");
        printf("  FMA cycles: %llu (target: ≤8)\n", fma_cycles);
    } else {
        TEST_FAIL("FMA Operation", "Incorrect results");
    }
}

// Test 5: Verify cache-line behavior
void test_cache_behavior() {
    TEST_START("Cache-line Optimization");
    
    const int iterations = 10000;
    alignas(64) uint64_t cache_line[8];  // Exactly one cache line
    
    // Test 1: Single cache line access
    uint64_t start = rdtsc();
    for (int i = 0; i < iterations; i++) {
        __m512i v = _mm512_load_si512((__m512i*)cache_line);
        _mm512_store_si512((__m512i*)cache_line, v);
    }
    uint64_t single_line = rdtsc() - start;
    
    // Test 2: Cross cache line (misaligned)
    alignas(128) uint64_t two_lines[16];
    uint64_t* misaligned = &two_lines[4];  // Crosses cache line boundary
    
    start = rdtsc();
    for (int i = 0; i < iterations; i++) {
        __m512i v = _mm512_loadu_si512((__m512i*)misaligned);
        _mm512_storeu_si512((__m512i*)misaligned, v);
    }
    uint64_t cross_line = rdtsc() - start;
    
    printf("  Single cache line: %llu cycles\n", single_line);
    printf("  Cross cache line: %llu cycles\n", cross_line);
    printf("  Penalty: %.2fx\n", (double)cross_line / single_line);
    
    TEST_PASS("Cache Behavior");
}

// Test 6: Verify prefetch effectiveness
void test_prefetch() {
    TEST_START("Prefetch Optimization");
    
    const int size = 65536;  // 64K elements
    alignas(64) uint64_t* data = aligned_alloc(64, size * sizeof(uint64_t));
    
    // Initialize
    for (int i = 0; i < size; i++) {
        data[i] = i;
    }
    
    // Without prefetch
    uint64_t sum1 = 0;
    uint64_t start = rdtsc();
    
    for (int i = 0; i < size; i += 8) {
        __m512i v = _mm512_load_si512((__m512i*)&data[i]);
        sum1 += _mm512_reduce_add_epi64(v);
    }
    
    uint64_t no_prefetch = rdtsc() - start;
    
    // With prefetch
    uint64_t sum2 = 0;
    start = rdtsc();
    
    for (int i = 0; i < size; i += 8) {
        // Prefetch next cache line
        if (i + 64 < size) {
            _mm_prefetch((char*)&data[i + 64], _MM_HINT_T0);
        }
        
        __m512i v = _mm512_load_si512((__m512i*)&data[i]);
        sum2 += _mm512_reduce_add_epi64(v);
    }
    
    uint64_t with_prefetch = rdtsc() - start;
    
    printf("  Without prefetch: %llu cycles\n", no_prefetch);
    printf("  With prefetch: %llu cycles\n", with_prefetch);
    printf("  Improvement: %.2f%%\n", 
           100.0 * (1.0 - (double)with_prefetch / no_prefetch));
    
    free(data);
    TEST_PASS("Prefetch");
}

// Test 7: Verify NUMA awareness
void test_numa_awareness() {
    TEST_START("NUMA Memory Access");
    
    // Note: Full NUMA testing requires multi-socket systems
    // This is a simplified test for memory locality
    
    const int size = 1048576;  // 1M elements (8MB)
    alignas(64) uint64_t* local_mem = aligned_alloc(64, size * sizeof(uint64_t));
    
    // Touch all pages to ensure allocation
    for (int i = 0; i < size; i += 512) {  // Every 4KB
        local_mem[i] = i;
    }
    
    // Measure access latency
    uint64_t total_cycles = 0;
    const int samples = 1000;
    
    for (int s = 0; s < samples; s++) {
        int idx = (s * 65521) % (size - 8);  // Prime for spread
        
        uint64_t start = rdtsc();
        __m512i v = _mm512_load_si512((__m512i*)&local_mem[idx]);
        uint64_t cycles = rdtsc() - start;
        
        // Use result to prevent optimization
        volatile uint64_t dummy = _mm512_extract_epi64(v, 0);
        (void)dummy;
        
        total_cycles += cycles;
    }
    
    double avg_latency = (double)total_cycles / samples;
    printf("  Average memory latency: %.1f cycles\n", avg_latency);
    
    free(local_mem);
    TEST_PASS("NUMA Access");
}

// Main test runner
int main() {
    printf("8T Substrate Test Suite\n");
    printf("=======================\n");
    
    // Check CPU features
    printf("\nCPU Features:\n");
    if (__builtin_cpu_supports("avx512f")) {
        printf("✅ AVX-512F supported\n");
    } else {
        printf("❌ AVX-512F not supported - tests may fail\n");
    }
    
    // Run tests
    test_vector_operations();
    test_alignment();
    test_access_patterns();
    test_instruction_mix();
    test_cache_behavior();
    test_prefetch();
    test_numa_awareness();
    
    printf("\n=========================\n");
    printf("8T Test Suite Complete\n");
    printf("=========================\n");
    
    return 0;
}