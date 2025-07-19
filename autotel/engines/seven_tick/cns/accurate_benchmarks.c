/*  ─────────────────────────────────────────────────────────────
    accurate_benchmarks.c  –  Time-based Performance Measurement
    Uses nanosecond timing and estimates cycles from known CPU frequency
    ───────────────────────────────────────────────────────────── */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/time.h>

#define S7T_MAX_CYCLES 7
#define DATASET_SIZE 1000

// Apple Silicon M1/M2 performance cores typically run at ~3.2-3.8 GHz
// We'll use 3.5 GHz as a reasonable estimate
#define ESTIMATED_CPU_FREQ_GHZ 3.5
#define NS_PER_CYCLE (1.0 / ESTIMATED_CPU_FREQ_GHZ)

// Prevent compiler optimization
volatile int global_result = 0;
volatile float global_result_f = 0.0f;

// More realistic business record with cache-unfriendly layout
typedef struct {
    int customer_id;
    int product_id;
    float revenue;
    int quarter;
    int region_id;
    char customer_name[64];
    char product_desc[128];
    int padding[16];  // 64 bytes of padding to stress memory system
} SalesRecord;

// Get high-resolution time in nanoseconds
uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

void generate_sales_data(SalesRecord* sales, int count) {
    uint32_t seed = (uint32_t)time(NULL);
    for (int i = 0; i < count; i++) {
        seed = seed * 1664525 + 1013904223;
        sales[i].customer_id = (seed % 5000) + 1;
        
        seed = seed * 1664525 + 1013904223;
        sales[i].product_id = (seed % 500) + 1;
        
        seed = seed * 1664525 + 1013904223;
        sales[i].revenue = ((seed % 50000) + 100) / 100.0f;
        
        seed = seed * 1664525 + 1013904223;
        sales[i].quarter = (seed % 4) + 1;
        
        seed = seed * 1664525 + 1013904223;
        sales[i].region_id = (seed % 10) + 1;
        
        // Fill padding with random data
        for (int j = 0; j < 16; j++) {
            sales[i].padding[j] = seed + j;
        }
        
        // Generate realistic string data
        snprintf(sales[i].customer_name, 64, "Customer_%d_%08x", sales[i].customer_id, seed);
        snprintf(sales[i].product_desc, 128, "Product_%d_Description_%08x_with_longer_text", sales[i].product_id, seed);
    }
}

// Realistic business query that can't be optimized away
void test_business_query(SalesRecord* sales, int count) {
    int high_value_count = 0;
    float total_revenue = 0;
    int total_name_length = 0;
    
    for (int i = 0; i < count; i++) {
        // Force memory reads of all fields
        volatile int customer = sales[i].customer_id;
        volatile int product = sales[i].product_id;
        volatile float revenue = sales[i].revenue;
        volatile int quarter = sales[i].quarter;
        volatile int region = sales[i].region_id;
        
        // Access padding to ensure realistic memory access pattern
        volatile int pad_checksum = 0;
        for (int j = 0; j < 16; j++) {
            pad_checksum ^= sales[i].padding[j];
        }
        
        // Access string data (forces cache misses)
        int name_len = strlen(sales[i].customer_name);
        int desc_len = strlen(sales[i].product_desc);
        total_name_length += name_len + desc_len;
        
        // Business logic: find high-value Q2+ customers in regions 1-5
        if (revenue > 300.0f && quarter >= 2 && region <= 5) {
            high_value_count++;
            total_revenue += revenue;
            
            // Additional processing to make work realistic
            volatile float margin = revenue * 0.15f;
            volatile int days_in_quarter = quarter * 90;
            (void)margin; (void)days_in_quarter;
        }
        
        // Prevent optimization
        (void)customer; (void)product; (void)pad_checksum;
    }
    
    // Store results globally
    global_result = high_value_count;
    global_result_f = total_revenue + (float)total_name_length;
}

double run_business_benchmark(const char* name, SalesRecord* data, int count, int iterations) {
    printf("%s:\n", name);
    
    uint64_t min_ns = UINT64_MAX;
    uint64_t max_ns = 0;
    uint64_t total_ns = 0;
    
    // Warm up CPU and cache
    for (int i = 0; i < 10; i++) {
        test_business_query(data, count);
    }
    
    // Clear cache by accessing unrelated memory
    volatile char* flush_data = malloc(4 * 1024 * 1024); // 4MB
    for (int i = 0; i < 4 * 1024 * 1024; i += 64) {
        flush_data[i] = (char)(i & 0xFF);
    }
    volatile int checksum = 0;
    for (int i = 0; i < 4 * 1024 * 1024; i += 4096) {
        checksum += flush_data[i];
    }
    free((void*)flush_data);
    (void)checksum;
    
    // Run actual benchmark
    for (int i = 0; i < iterations; i++) {
        uint64_t start_ns = get_time_ns();
        test_business_query(data, count);
        uint64_t elapsed_ns = get_time_ns() - start_ns;
        
        total_ns += elapsed_ns;
        if (elapsed_ns < min_ns) min_ns = elapsed_ns;
        if (elapsed_ns > max_ns) max_ns = elapsed_ns;
    }
    
    double avg_ns = (double)total_ns / iterations;
    double avg_cycles = avg_ns / NS_PER_CYCLE;
    double cycles_per_row = avg_cycles / count;
    double ns_per_row = avg_ns / count;
    
    printf("   CPU Frequency: %.1f GHz (estimated)\n", ESTIMATED_CPU_FREQ_GHZ);
    printf("   Time: %.0f ns avg, %llu ns min, %llu ns max\n", avg_ns, min_ns, max_ns);
    printf("   Estimated cycles: %.0f total, %.3f per row\n", avg_cycles, cycles_per_row);
    printf("   Latency: %.2f μs total, %.0f ns per row\n", avg_ns / 1000.0, ns_per_row);
    printf("   7-tick: %s (%.1fx %s budget)\n", 
           cycles_per_row <= S7T_MAX_CYCLES ? "PASS ✓" : "FAIL ✗",
           cycles_per_row <= S7T_MAX_CYCLES ? S7T_MAX_CYCLES / cycles_per_row : cycles_per_row / S7T_MAX_CYCLES,
           cycles_per_row <= S7T_MAX_CYCLES ? "under" : "over");
    printf("   Business result: %d high-value customers, $%.2f total\n\n", 
           global_result, global_result_f);
    
    return cycles_per_row;
}

int main(int argc, char** argv) {
    printf("Accurate 7T-SQL Business Benchmark\n");
    printf("=================================\n\n");
    
    int iterations = (argc > 1) ? atoi(argv[1]) : 100;
    printf("Dataset: %d records, %d iterations\n", DATASET_SIZE, iterations);
    printf("Record size: %zu bytes (realistic with padding)\n", sizeof(SalesRecord));
    printf("Total dataset size: %.1f KB\n", (DATASET_SIZE * sizeof(SalesRecord)) / 1024.0);
    printf("Estimated CPU: %.1f GHz (Apple Silicon)\n\n", ESTIMATED_CPU_FREQ_GHZ);
    
    // Allocate realistic business data
    SalesRecord* sales = malloc(DATASET_SIZE * sizeof(SalesRecord));
    if (!sales) {
        printf("Memory allocation failed\n");
        return 1;
    }
    
    generate_sales_data(sales, DATASET_SIZE);
    printf("Generated realistic business data...\n\n");
    
    // Run the benchmark
    double cycles_per_row = run_business_benchmark(
        "Business Analytics: Multi-criteria filter with string processing", 
        sales, DATASET_SIZE, iterations);
    
    printf("=====================================\n");
    printf("Performance Analysis:\n");
    printf("   Cycles per row: %.3f\n", cycles_per_row);
    printf("   7-tick compliance: %s\n", cycles_per_row <= S7T_MAX_CYCLES ? "PASS ✓" : "FAIL ✗");
    printf("   Performance class: ");
    
    if (cycles_per_row <= 7) {
        printf("7-Tick compliant ✓\n");
    } else if (cycles_per_row <= 50) {
        printf("Fast (sub-50 cycle)\n");
    } else if (cycles_per_row <= 200) {
        printf("Good (sub-200 cycle)\n");
    } else {
        printf("Needs optimization\n");
    }
    
    printf("\nRealistic Expectations:\n");
    printf("   • Memory-bound workload: 20-100 cycles/row typical\n");
    printf("   • String processing overhead: +10-30 cycles\n");
    printf("   • Cache misses: +50-200 cycles per miss\n");
    printf("   • 7-tick target: Very aggressive for complex queries\n");
    
    free(sales);
    return cycles_per_row <= S7T_MAX_CYCLES ? 0 : 1;
}