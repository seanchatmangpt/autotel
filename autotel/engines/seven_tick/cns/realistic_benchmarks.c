/*  ─────────────────────────────────────────────────────────────
    realistic_benchmarks.c  –  Proper CPU Cycle Measurement
    Addresses ARM64 timer vs cycle counter issues
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

// Use performance counter that maps to actual cycles on Apple Silicon
static inline uint64_t get_cpu_cycles(void) {
    uint64_t val;
    // On Apple Silicon, pmccntr_el0 gives actual CPU cycles
    // But requires special permissions, so we'll use a frequency-adjusted timer
    __asm__ __volatile__("mrs %0, cntvct_el0" : "=r" (val));
    return val;
}

// Get CPU frequency to convert timer ticks to cycles
double get_cpu_frequency_ghz(void) {
    // Apple M1/M2 typically run at ~3.2-3.8 GHz for P-cores
    // We'll estimate based on timer frequency
    struct timespec start, end;
    uint64_t cycle_start, cycle_end;
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    cycle_start = get_cpu_cycles();
    
    usleep(10000); // 10ms
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    cycle_end = get_cpu_cycles();
    
    double elapsed_ns = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
    uint64_t cycle_diff = cycle_end - cycle_start;
    
    double timer_freq_ghz = cycle_diff / elapsed_ns;
    
    // Apple Silicon timer typically runs at 24MHz, CPU at ~3.5GHz
    // So actual CPU cycles = timer_ticks * (cpu_freq / timer_freq)
    double cpu_freq_ghz = timer_freq_ghz * (3500.0 / 24.0); // Approximate ratio
    
    return cpu_freq_ghz;
}

// Prevent compiler optimization by making data globally visible
volatile int global_result = 0;
volatile float global_result_f = 0.0f;

// Business data with realistic sizes that stress cache
typedef struct {
    int customer_id;
    int product_id;
    float revenue;
    int quarter;
    int region_id;
    char customer_name[64];    // Add padding to stress cache
    char product_desc[128];    // More realistic data sizes
    int padding[16];           // Force cache misses
} SalesRecord;

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
        
        // Fill padding to prevent optimization
        for (int j = 0; j < 16; j++) {
            sales[i].padding[j] = seed + j;
        }
        
        // Generate names to make data access realistic
        snprintf(sales[i].customer_name, 64, "Customer_%d_%x", sales[i].customer_id, seed);
        snprintf(sales[i].product_desc, 128, "Product_%d_Description_%x", sales[i].product_id, seed);
    }
}

// More realistic test that prevents optimization
void test_sales_filter_no_opt(SalesRecord* sales, int count) {
    int high_value_count = 0;
    float total_revenue = 0;
    int checked_records = 0;
    
    for (int i = 0; i < count; i++) {
        // Force memory access to all fields to prevent optimization
        volatile int customer = sales[i].customer_id;
        volatile int product = sales[i].product_id;
        volatile float revenue = sales[i].revenue;
        volatile int quarter = sales[i].quarter;
        volatile int region = sales[i].region_id;
        
        // Use padding data to force cache misses
        volatile int pad_sum = 0;
        for (int j = 0; j < 16; j++) {
            pad_sum += sales[i].padding[j];
        }
        
        checked_records++;
        
        // Actual business logic
        if (revenue > 300.0f && quarter >= 2 && region <= 5) {
            high_value_count++;
            total_revenue += revenue;
            
            // Touch string data to force realistic memory access
            volatile int name_len = strlen(sales[i].customer_name);
            volatile int desc_len = strlen(sales[i].product_desc);
            (void)name_len; (void)desc_len;
        }
        
        (void)customer; (void)product; (void)pad_sum;
    }
    
    // Store results globally to prevent optimization
    global_result = high_value_count;
    global_result_f = total_revenue;
}

double run_realistic_benchmark(const char* name, SalesRecord* data, int count, int iterations) {
    printf("%s:\n", name);
    
    uint64_t min_timer = UINT64_MAX;
    uint64_t max_timer = 0;
    uint64_t total_timer = 0;
    
    // Warm up - force data into cache, then force it out
    for (int i = 0; i < 10; i++) {
        test_sales_filter_no_opt(data, count);
    }
    
    // Force cache flush by accessing large amount of other data
    volatile char* flush_buffer = malloc(8 * 1024 * 1024); // 8MB
    for (int i = 0; i < 8 * 1024 * 1024; i += 64) {
        flush_buffer[i] = i & 0xFF;
    }
    free((void*)flush_buffer);
    
    // Actual benchmark
    for (int i = 0; i < iterations; i++) {
        uint64_t start = get_cpu_cycles();
        test_sales_filter_no_opt(data, count);
        uint64_t timer_ticks = get_cpu_cycles() - start;
        
        total_timer += timer_ticks;
        if (timer_ticks < min_timer) min_timer = timer_ticks;
        if (timer_ticks > max_timer) max_timer = timer_ticks;
    }
    
    double cpu_freq = get_cpu_frequency_ghz();
    double timer_freq = 0.024; // 24 MHz typical for Apple Silicon
    double cycle_multiplier = cpu_freq / timer_freq;
    
    double avg_timer = (double)total_timer / iterations;
    double avg_cycles = avg_timer * cycle_multiplier;
    double cycles_per_row = avg_cycles / count;
    
    printf("   CPU Frequency: %.2f GHz (estimated)\n", cpu_freq);
    printf("   Timer ticks: %.0f avg, %llu min, %llu max\n", avg_timer, min_timer, max_timer);
    printf("   Estimated CPU cycles: %.0f total, %.3f per row\n", avg_cycles, cycles_per_row);
    printf("   Latency: %.2f μs total, %.0f ns per row\n", 
           avg_timer / (timer_freq * 1000), cycles_per_row * (1000.0 / cpu_freq));
    printf("   7-tick: %s (%.1fx %s budget)\n", 
           cycles_per_row <= S7T_MAX_CYCLES ? "PASS ✓" : "FAIL ✗",
           cycles_per_row <= S7T_MAX_CYCLES ? S7T_MAX_CYCLES / cycles_per_row : cycles_per_row / S7T_MAX_CYCLES,
           cycles_per_row <= S7T_MAX_CYCLES ? "under" : "over");
    printf("   Result verification: found %d records, total $%.2f\n\n", 
           global_result, global_result_f);
    
    return cycles_per_row;
}

int main(int argc, char** argv) {
    printf("Realistic 7T-SQL Benchmark (Corrected Cycle Measurement)\n");
    printf("=======================================================\n\n");
    
    int iterations = (argc > 1) ? atoi(argv[1]) : 100;
    printf("Dataset: %d records, %d iterations\n", DATASET_SIZE, iterations);
    printf("Record size: %zu bytes (includes realistic padding)\n\n", sizeof(SalesRecord));
    
    // Allocate data - force it to not be cache-friendly
    SalesRecord* sales = malloc(DATASET_SIZE * sizeof(SalesRecord));
    if (!sales) {
        printf("Memory allocation failed\n");
        return 1;
    }
    
    generate_sales_data(sales, DATASET_SIZE);
    printf("Generated realistic business data with cache-unfriendly layout...\n\n");
    
    // Run realistic benchmark
    double cycles_per_row = run_realistic_benchmark(
        "Business Analytics Query (Multi-field filter with string access)", 
        sales, DATASET_SIZE, iterations);
    
    printf("=====================================\n");
    printf("Realistic Performance Analysis:\n");
    printf("   Cycles per row: %.3f\n", cycles_per_row);
    printf("   7-tick compliance: %s\n", cycles_per_row <= S7T_MAX_CYCLES ? "PASS ✓" : "FAIL ✗");
    printf("   Memory access pattern: Cache-unfriendly (realistic)\n");
    printf("   Data verification: %d high-value records found\n", global_result);
    
    // Analysis of why results might be unrealistic
    if (cycles_per_row < 1.0) {
        printf("\n⚠️  WARNING: Suspiciously low cycle count!\n");
        printf("   Possible causes:\n");
        printf("   • ARM64 timer frequency estimation incorrect\n");
        printf("   • Compiler optimization despite volatile keywords\n");
        printf("   • Data still fits in L1 cache (32KB typical)\n");
        printf("   • Timer resolution insufficient for measurement\n");
    }
    
    free(sales);
    return cycles_per_row <= S7T_MAX_CYCLES ? 0 : 1;
}