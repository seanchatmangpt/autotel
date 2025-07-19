/*  ─────────────────────────────────────────────────────────────
    benchmark_7t_sql.c  –  7T-SQL Benchmark Results Demo
    Shows real performance measurements
    ───────────────────────────────────────────────────────────── */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <immintrin.h>

#define S7T_MAX_CYCLES 7
#define S7T_NS_PER_CYCLE 0.3
#define S7T_SQL_MAX_ROWS 1024

// CPU cycle counter
static inline uint64_t get_cycles(void) {
    uint32_t lo, hi;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

// SIMD filter implementation
static uint32_t simd_filter_eq_i32(const int32_t* data, int32_t value, 
                                   uint32_t count, uint32_t* out_indices) {
    uint32_t matches = 0;
    
#ifdef __AVX2__
    __m256i vval = _mm256_set1_epi32(value);
    
    uint32_t i;
    for (i = 0; i + 7 < count; i += 8) {
        __m256i vdata = _mm256_loadu_si256((__m256i*)(data + i));
        __m256i vcmp = _mm256_cmpeq_epi32(vdata, vval);
        uint32_t mask = _mm256_movemask_ps(_mm256_castsi256_ps(vcmp));
        
        while (mask) {
            uint32_t idx = __builtin_ctz(mask);
            out_indices[matches++] = i + idx;
            mask &= mask - 1;
        }
    }
    
    // Handle remaining elements
    for (; i < count; i++) {
        if (data[i] == value) {
            out_indices[matches++] = i;
        }
    }
#else
    // Scalar fallback
    for (uint32_t i = 0; i < count; i++) {
        if (data[i] == value) {
            out_indices[matches++] = i;
        }
    }
#endif
    
    return matches;
}

int main(int argc, char** argv) {
    printf("7T-SQL Real Benchmark Results\n");
    printf("=============================\n\n");
    
    int iterations = (argc > 1) ? atoi(argv[1]) : 10000;
    printf("Running with %d iterations...\n\n", iterations);
    
    // Allocate test data
    int32_t* test_data = (int32_t*)aligned_alloc(64, 1000 * sizeof(int32_t));
    uint32_t* match_indices = (uint32_t*)aligned_alloc(64, 1000 * sizeof(uint32_t));
    
    // Generate realistic test data
    uint32_t seed = (uint32_t)time(NULL);
    for (int i = 0; i < 1000; i++) {
        seed = seed * 1664525 + 1013904223;  // LCG
        test_data[i] = seed % 100;
    }
    
    // Warm up CPU
    volatile int64_t warmup = 0;
    for (int i = 0; i < 1000; i++) {
        for (int j = 0; j < 1000; j++) {
            warmup += test_data[j];
        }
    }
    
    // Benchmark 1: Sequential Scan
    printf("1. Sequential Scan (WHERE value > 50):\n");
    {
        uint64_t min_cycles = UINT64_MAX;
        uint64_t max_cycles = 0;
        uint64_t total_cycles = 0;
        
        for (int iter = 0; iter < iterations; iter++) {
            uint64_t start = get_cycles();
            
            uint32_t count = 0;
            for (int i = 0; i < 1000; i++) {
                if (test_data[i] > 50) {
                    match_indices[count++] = i;
                }
            }
            
            uint64_t cycles = get_cycles() - start;
            total_cycles += cycles;
            if (cycles < min_cycles) min_cycles = cycles;
            if (cycles > max_cycles) max_cycles = cycles;
        }
        
        double avg_cycles = (double)total_cycles / iterations;
        double cycles_per_row = avg_cycles / 1000;
        
        printf("   Average: %.2f cycles total, %.3f cycles/row\n", avg_cycles, cycles_per_row);
        printf("   Min-Max: %llu - %llu cycles\n", min_cycles, max_cycles);
        printf("   Latency: %.2f ns total, %.3f ns/row\n", 
               avg_cycles * S7T_NS_PER_CYCLE, cycles_per_row * S7T_NS_PER_CYCLE);
        printf("   7-tick: %s (%.1fx %s budget)\n\n", 
               cycles_per_row <= S7T_MAX_CYCLES ? "PASS ✓" : "FAIL ✗",
               cycles_per_row <= S7T_MAX_CYCLES ? S7T_MAX_CYCLES / cycles_per_row : cycles_per_row / S7T_MAX_CYCLES,
               cycles_per_row <= S7T_MAX_CYCLES ? "under" : "over");
    }
    
    // Benchmark 2: SIMD Filter
    printf("2. SIMD Filter (WHERE value = 42):\n");
    {
        uint64_t min_cycles = UINT64_MAX;
        uint64_t max_cycles = 0;
        uint64_t total_cycles = 0;
        uint32_t total_matches = 0;
        
        for (int iter = 0; iter < iterations; iter++) {
            uint64_t start = get_cycles();
            
            uint32_t matches = simd_filter_eq_i32(test_data, 42, 1000, match_indices);
            
            uint64_t cycles = get_cycles() - start;
            total_cycles += cycles;
            total_matches += matches;
            if (cycles < min_cycles) min_cycles = cycles;
            if (cycles > max_cycles) max_cycles = cycles;
        }
        
        double avg_cycles = (double)total_cycles / iterations;
        double cycles_per_row = avg_cycles / 1000;
        double avg_matches = (double)total_matches / iterations;
        
        printf("   Average: %.2f cycles total, %.3f cycles/row\n", avg_cycles, cycles_per_row);
        printf("   Min-Max: %llu - %llu cycles\n", min_cycles, max_cycles);
        printf("   Latency: %.2f ns total, %.3f ns/row\n", 
               avg_cycles * S7T_NS_PER_CYCLE, cycles_per_row * S7T_NS_PER_CYCLE);
        printf("   Matches: %.1f average per query\n", avg_matches);
        printf("   7-tick: %s (%.1fx %s budget)\n\n", 
               cycles_per_row <= S7T_MAX_CYCLES ? "PASS ✓" : "FAIL ✗",
               cycles_per_row <= S7T_MAX_CYCLES ? S7T_MAX_CYCLES / cycles_per_row : cycles_per_row / S7T_MAX_CYCLES,
               cycles_per_row <= S7T_MAX_CYCLES ? "under" : "over");
    }
    
    // Benchmark 3: Aggregation
    printf("3. Sum Aggregation (1000 rows):\n");
    {
        uint64_t min_cycles = UINT64_MAX;
        uint64_t max_cycles = 0;
        uint64_t total_cycles = 0;
        
        for (int iter = 0; iter < iterations; iter++) {
            uint64_t start = get_cycles();
            
            int64_t sum = 0;
            int i;
            
            // Unrolled loop for better performance
            for (i = 0; i + 7 < 1000; i += 8) {
                sum += test_data[i] + test_data[i+1] + test_data[i+2] + test_data[i+3] +
                       test_data[i+4] + test_data[i+5] + test_data[i+6] + test_data[i+7];
            }
            
            // Handle remainder
            for (; i < 1000; i++) {
                sum += test_data[i];
            }
            
            uint64_t cycles = get_cycles() - start;
            total_cycles += cycles;
            if (cycles < min_cycles) min_cycles = cycles;
            if (cycles > max_cycles) max_cycles = cycles;
        }
        
        double avg_cycles = (double)total_cycles / iterations;
        double cycles_per_row = avg_cycles / 1000;
        
        printf("   Average: %.2f cycles total, %.3f cycles/row\n", avg_cycles, cycles_per_row);
        printf("   Min-Max: %llu - %llu cycles\n", min_cycles, max_cycles);
        printf("   Latency: %.2f ns total, %.3f ns/row\n", 
               avg_cycles * S7T_NS_PER_CYCLE, cycles_per_row * S7T_NS_PER_CYCLE);
        printf("   7-tick: %s (%.1fx %s budget)\n\n", 
               cycles_per_row <= S7T_MAX_CYCLES ? "PASS ✓" : "FAIL ✗",
               cycles_per_row <= S7T_MAX_CYCLES ? S7T_MAX_CYCLES / cycles_per_row : cycles_per_row / S7T_MAX_CYCLES,
               cycles_per_row <= S7T_MAX_CYCLES ? "under" : "over");
    }
    
    // Benchmark 4: Single Row Insert
    printf("4. Insert Operation (single row):\n");
    {
        uint64_t min_cycles = UINT64_MAX;
        uint64_t max_cycles = 0;
        uint64_t total_cycles = 0;
        
        int32_t* insert_buffer = (int32_t*)aligned_alloc(64, 1000 * sizeof(int32_t));
        int row_count = 0;
        
        int insert_iterations = (iterations < 1000) ? iterations : 1000;
        
        for (int iter = 0; iter < insert_iterations; iter++) {
            uint64_t start = get_cycles();
            
            // Simulate insert: just store value at next position
            insert_buffer[row_count++] = iter;
            
            uint64_t cycles = get_cycles() - start;
            total_cycles += cycles;
            if (cycles < min_cycles) min_cycles = cycles;
            if (cycles > max_cycles) max_cycles = cycles;
        }
        
        double avg_cycles = (double)total_cycles / insert_iterations;
        
        printf("   Average: %.2f cycles per insert\n", avg_cycles);
        printf("   Min-Max: %llu - %llu cycles\n", min_cycles, max_cycles);
        printf("   Latency: %.2f ns per insert\n", avg_cycles * S7T_NS_PER_CYCLE);
        printf("   7-tick: %s (%.1fx %s budget)\n\n", 
               avg_cycles <= S7T_MAX_CYCLES ? "PASS ✓" : "FAIL ✗",
               avg_cycles <= S7T_MAX_CYCLES ? S7T_MAX_CYCLES / avg_cycles : avg_cycles / S7T_MAX_CYCLES,
               avg_cycles <= S7T_MAX_CYCLES ? "under" : "over");
        
        free(insert_buffer);
    }
    
    // Summary
    printf("=======================================\n");
    printf("System Information:\n");
    printf("   CPU Frequency (estimated): %.2f GHz\n", 1.0 / S7T_NS_PER_CYCLE);
    printf("   7-tick budget: %d cycles (%.1f ns)\n", S7T_MAX_CYCLES, S7T_MAX_CYCLES * S7T_NS_PER_CYCLE);
#ifdef __AVX2__
    printf("   SIMD: AVX2 enabled ✓\n");
#else
    printf("   SIMD: Scalar fallback\n");
#endif
    
    free(test_data);
    free(match_indices);
    
    return 0;
}