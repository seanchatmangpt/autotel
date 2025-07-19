/*  ─────────────────────────────────────────────────────────────
    optimized_benchmarks.c  –  80/20 Performance Optimizations
    Highest-impact optimizations for 7T-SQL performance
    ───────────────────────────────────────────────────────────── */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <arm_neon.h>  // ARM NEON SIMD for Apple Silicon

#define L1_CACHE_SIZE (24 * 1024)
#define RECORD_SIZE 32
#define DATASET_SIZE (L1_CACHE_SIZE / RECORD_SIZE)
#define S7T_MAX_CYCLES 7
#define ESTIMATED_CPU_FREQ_GHZ 3.5
#define NS_PER_CYCLE (1.0 / ESTIMATED_CPU_FREQ_GHZ)

// Cache-line aligned record for optimal memory access
typedef struct {
    int32_t customer_id;
    int32_t product_id;
    float revenue;
    int16_t quarter;
    int16_t region_id;
    int32_t order_date;
    int32_t status;       // 0=pending, 1=complete, 2=cancelled
} __attribute__((aligned(32))) SalesRecord;

volatile int global_result = 0;
volatile float global_result_f = 0.0f;

uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

void generate_aligned_sales_data(SalesRecord* sales, int count) {
    uint32_t seed = (uint32_t)time(NULL);
    for (int i = 0; i < count; i++) {
        seed = seed * 1664525 + 1013904223;
        sales[i].customer_id = (seed % 1000) + 1;
        
        seed = seed * 1664525 + 1013904223;
        sales[i].product_id = (seed % 100) + 1;
        
        seed = seed * 1664525 + 1013904223;
        sales[i].revenue = ((seed % 50000) + 100) / 100.0f;
        
        seed = seed * 1664525 + 1013904223;
        sales[i].quarter = (seed % 4) + 1;
        
        seed = seed * 1664525 + 1013904223;
        sales[i].region_id = (seed % 10) + 1;
        
        seed = seed * 1664525 + 1013904223;
        sales[i].order_date = seed % 365;
        
        seed = seed * 1664525 + 1013904223;
        sales[i].status = seed % 3;
    }
}

// 80/20 OPTIMIZATION 1: SIMD Vectorized Filter (biggest performance gain)
void test_simd_filter(SalesRecord* sales, int count) {
    int matches = 0;
    float total = 0;
    const float threshold = 300.0f;
    
    // NEON SIMD processing - 4 floats at a time
    const float32x4_t threshold_vec = vdupq_n_f32(threshold);
    float32x4_t sum_vec = vdupq_n_f32(0.0f);
    uint32x4_t count_vec = vdupq_n_u32(0);
    
    int i;
    for (i = 0; i + 3 < count; i += 4) {
        // Load 4 revenue values
        float32x4_t revenue_vec = vld1q_f32(&sales[i].revenue);
        
        // Compare with threshold
        uint32x4_t mask = vcgtq_f32(revenue_vec, threshold_vec);
        
        // Conditionally add to sum (branchless)
        float32x4_t masked_revenue = vreinterpretq_f32_u32(vandq_u32(vreinterpretq_u32_f32(revenue_vec), mask));
        sum_vec = vaddq_f32(sum_vec, masked_revenue);
        
        // Count matches (branchless)
        count_vec = vaddq_u32(count_vec, vandq_u32(mask, vdupq_n_u32(1)));
        
        // Manual prefetch for next cache line
        __builtin_prefetch(&sales[i + 8], 0, 3);
    }
    
    // Horizontal sum of SIMD results
    float sum_array[4];
    uint32_t count_array[4];
    vst1q_f32(sum_array, sum_vec);
    vst1q_u32(count_array, count_vec);
    
    for (int j = 0; j < 4; j++) {
        total += sum_array[j];
        matches += count_array[j];
    }
    
    // Handle remaining elements
    for (; i < count; i++) {
        if (sales[i].revenue > threshold) {
            matches++;
            total += sales[i].revenue;
        }
    }
    
    global_result = matches;
    global_result_f = total;
}

// 80/20 OPTIMIZATION 2: Branchless Complex Filter
void test_branchless_filter(SalesRecord* sales, int count) {
    int matches = 0;
    float total = 0;
    
    for (int i = 0; i < count; i++) {
        // Branchless conditions using bit operations
        int cond1 = sales[i].revenue > 250.0f;
        int cond2 = sales[i].quarter >= 2;
        int cond3 = sales[i].region_id <= 5;
        int all_conditions = cond1 & cond2 & cond3;
        
        // Branchless accumulation
        matches += all_conditions;
        total += sales[i].revenue * all_conditions;
        
        // Prefetch next cache line
        if (__builtin_expect((i & 15) == 0, 0)) {
            __builtin_prefetch(&sales[i + 16], 0, 3);
        }
    }
    
    global_result = matches;
    global_result_f = total;
}

// 80/20 OPTIMIZATION 3: Unrolled Aggregation with SIMD
void test_unrolled_aggregation(SalesRecord* sales, int count) {
    // Use SIMD-friendly arrays
    float region_totals[16] __attribute__((aligned(16))) = {0}; // Padded for SIMD
    
    // Process 8 records at a time (loop unrolling)
    int i;
    for (i = 0; i + 7 < count; i += 8) {
        // Manual unroll for better pipeline utilization
        region_totals[sales[i].region_id] += sales[i].revenue;
        region_totals[sales[i+1].region_id] += sales[i+1].revenue;
        region_totals[sales[i+2].region_id] += sales[i+2].revenue;
        region_totals[sales[i+3].region_id] += sales[i+3].revenue;
        region_totals[sales[i+4].region_id] += sales[i+4].revenue;
        region_totals[sales[i+5].region_id] += sales[i+5].revenue;
        region_totals[sales[i+6].region_id] += sales[i+6].revenue;
        region_totals[sales[i+7].region_id] += sales[i+7].revenue;
        
        // Prefetch ahead
        __builtin_prefetch(&sales[i + 16], 0, 3);
    }
    
    // Handle remainder
    for (; i < count; i++) {
        region_totals[sales[i].region_id] += sales[i].revenue;
    }
    
    // SIMD find maximum (process 4 at a time)
    float32x4_t max_vec = vld1q_f32(&region_totals[0]);
    uint32x4_t idx_vec = {0, 1, 2, 3};
    uint32x4_t best_idx_vec = idx_vec;
    
    for (int j = 4; j < 12; j += 4) {
        float32x4_t curr_vec = vld1q_f32(&region_totals[j]);
        uint32x4_t curr_idx_vec = vaddq_u32(idx_vec, vdupq_n_u32(j));
        
        uint32x4_t mask = vcgtq_f32(curr_vec, max_vec);
        max_vec = vmaxq_f32(max_vec, curr_vec);
        best_idx_vec = vbslq_u32(mask, curr_idx_vec, best_idx_vec);
    }
    
    // Extract best region (simplified)
    float max_vals[4];
    uint32_t best_indices[4];
    vst1q_f32(max_vals, max_vec);
    vst1q_u32(best_indices, best_idx_vec);
    
    int best_region = best_indices[0];
    float max_total = max_vals[0];
    for (int j = 1; j < 4; j++) {
        if (max_vals[j] > max_total) {
            max_total = max_vals[j];
            best_region = best_indices[j];
        }
    }
    
    global_result = best_region;
    global_result_f = max_total;
}

// 80/20 OPTIMIZATION 4: Cache-Optimized Join with SOA Layout
void test_optimized_join(SalesRecord* sales, int count) {
    // Structure of Arrays for better cache utilization
    static int32_t customer_ids[DATASET_SIZE] __attribute__((aligned(64)));
    static float revenues[DATASET_SIZE] __attribute__((aligned(64)));
    static float customer_totals[1000] __attribute__((aligned(64))) = {0};
    
    // First pass: extract data in cache-friendly order (SOA)
    for (int i = 0; i < count; i++) {
        customer_ids[i] = sales[i].customer_id % 1000;
        revenues[i] = sales[i].revenue;
    }
    
    // Second pass: SIMD aggregation
    for (int i = 0; i + 3 < count; i += 4) {
        // Load 4 customer IDs and revenues
        int32x4_t cust_vec = vld1q_s32(&customer_ids[i]);
        float32x4_t rev_vec = vld1q_f32(&revenues[i]);
        
        // Manual scatter (NEON doesn't have scatter, so unroll)
        int32_t cust_array[4];
        float rev_array[4];
        vst1q_s32(cust_array, cust_vec);
        vst1q_f32(rev_array, rev_vec);
        
        for (int j = 0; j < 4; j++) {
            customer_totals[cust_array[j]] += rev_array[j];
        }
        
        __builtin_prefetch(&customer_ids[i + 8], 0, 3);
        __builtin_prefetch(&revenues[i + 8], 0, 3);
    }
    
    // Handle remainder
    for (int i = count & ~3; i < count; i++) {
        customer_totals[customer_ids[i]] += revenues[i];
    }
    
    // Count high-value customers with branchless operations
    int high_value_count = 0;
    const float threshold = 1000.0f;
    
    for (int i = 0; i < 1000; i++) {
        high_value_count += (customer_totals[i] > threshold);
    }
    
    // Count orders from high-value customers
    int high_value_orders = 0;
    float high_value_revenue = 0;
    
    for (int i = 0; i < count; i++) {
        int is_high_value = (customer_totals[customer_ids[i]] > threshold);
        high_value_orders += is_high_value;
        high_value_revenue += revenues[i] * is_high_value;
    }
    
    global_result = high_value_orders;
    global_result_f = high_value_revenue;
}

// Baseline tests for comparison
void test_baseline_filter(SalesRecord* sales, int count) {
    int matches = 0;
    float total = 0;
    
    for (int i = 0; i < count; i++) {
        if (sales[i].revenue > 300.0f) {
            matches++;
            total += sales[i].revenue;
        }
    }
    
    global_result = matches;
    global_result_f = total;
}

double run_optimization_benchmark(const char* name, void (*test_func)(SalesRecord*, int), 
                                  SalesRecord* data, int count, int iterations) {
    printf("%s:\n", name);
    
    uint64_t min_ns = UINT64_MAX;
    uint64_t max_ns = 0;
    uint64_t total_ns = 0;
    
    // Warm up
    for (int i = 0; i < 100; i++) {
        test_func(data, count);
    }
    
    // Benchmark
    for (int i = 0; i < iterations; i++) {
        uint64_t start_ns = get_time_ns();
        test_func(data, count);
        uint64_t elapsed_ns = get_time_ns() - start_ns;
        
        total_ns += elapsed_ns;
        if (elapsed_ns < min_ns) min_ns = elapsed_ns;
        if (elapsed_ns > max_ns) max_ns = elapsed_ns;
    }
    
    double avg_ns = (double)total_ns / iterations;
    double avg_cycles = avg_ns / NS_PER_CYCLE;
    double cycles_per_row = avg_cycles / count;
    
    printf("   Time: %.0f ns (%.0f - %.0f)\n", avg_ns, (double)min_ns, (double)max_ns);
    printf("   Cycles: %.1f total, %.3f per row\n", avg_cycles, cycles_per_row);
    printf("   7-tick: %s", cycles_per_row <= S7T_MAX_CYCLES ? "PASS ✓" : "FAIL ✗");
    if (cycles_per_row <= S7T_MAX_CYCLES) {
        printf(" (%.1fx under)\n", S7T_MAX_CYCLES / cycles_per_row);
    } else {
        printf(" (%.1fx over)\n", cycles_per_row / S7T_MAX_CYCLES);
    }
    printf("   Result: %d / $%.2f\n\n", global_result, global_result_f);
    
    return cycles_per_row;
}

int main(int argc, char** argv) {
    printf("7T-SQL 80/20 Performance Optimization Benchmark\n");
    printf("==============================================\n\n");
    
    int iterations = (argc > 1) ? atoi(argv[1]) : 1000;
    
    printf("Optimization Strategy:\n");
    printf("   1. SIMD vectorization (NEON)\n");
    printf("   2. Branchless operations\n");
    printf("   3. Loop unrolling\n");
    printf("   4. Memory prefetching\n");
    printf("   5. Structure of Arrays (SOA)\n");
    printf("   6. Cache-line alignment\n\n");
    
    // Allocate cache-aligned data
    SalesRecord* sales = aligned_alloc(64, DATASET_SIZE * sizeof(SalesRecord));
    if (!sales) {
        printf("Memory allocation failed\n");
        return 1;
    }
    
    generate_aligned_sales_data(sales, DATASET_SIZE);
    printf("Dataset: %d records (%.1f KB), %d iterations\n\n", 
           DATASET_SIZE, (DATASET_SIZE * sizeof(SalesRecord)) / 1024.0, iterations);
    
    // Baseline vs Optimized Comparison
    printf("BASELINE PERFORMANCE:\n");
    double baseline = run_optimization_benchmark("Baseline Filter", 
                                                 test_baseline_filter, sales, DATASET_SIZE, iterations);
    
    printf("OPTIMIZED PERFORMANCE:\n");
    double opt1 = run_optimization_benchmark("1. SIMD Vectorized Filter", 
                                             test_simd_filter, sales, DATASET_SIZE, iterations);
    
    double opt2 = run_optimization_benchmark("2. Branchless Complex Filter", 
                                             test_branchless_filter, sales, DATASET_SIZE, iterations);
    
    double opt3 = run_optimization_benchmark("3. Unrolled SIMD Aggregation", 
                                             test_unrolled_aggregation, sales, DATASET_SIZE, iterations);
    
    double opt4 = run_optimization_benchmark("4. Cache-Optimized Join (SOA)", 
                                             test_optimized_join, sales, DATASET_SIZE, iterations);
    
    // Performance Analysis
    printf("=====================================\n");
    printf("80/20 Optimization Results:\n");
    printf("   Baseline: %.3f cycles/row\n", baseline);
    printf("   SIMD Filter: %.3f cycles/row (%.1fx speedup)\n", opt1, baseline/opt1);
    printf("   Branchless: %.3f cycles/row (%.1fx speedup)\n", opt2, baseline/opt2);
    printf("   Unrolled: %.3f cycles/row (%.1fx speedup)\n", opt3, baseline/opt3);
    printf("   SOA Join: %.3f cycles/row (%.1fx speedup)\n", opt4, baseline/opt4);
    
    double best_perf = opt1 < opt2 ? opt1 : opt2;
    best_perf = best_perf < opt3 ? best_perf : opt3;
    best_perf = best_perf < opt4 ? best_perf : opt4;
    
    printf("\n   Best optimization: %.3f cycles/row\n", best_perf);
    printf("   Total speedup: %.1fx\n", baseline/best_perf);
    printf("   7-tick compliance: %s\n", best_perf <= S7T_MAX_CYCLES ? "ACHIEVED ✓" : "NEEDS MORE WORK");
    
    free(sales);
    return best_perf <= S7T_MAX_CYCLES ? 0 : 1;
}