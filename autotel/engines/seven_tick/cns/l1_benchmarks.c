/*  ─────────────────────────────────────────────────────────────
    l1_benchmarks.c  –  L1-Cache Friendly Benchmark
    Fits data in L1 cache (~32KB) for realistic 7T measurements
    ───────────────────────────────────────────────────────────── */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

// L1 cache on Apple Silicon is typically 32KB instruction + 32KB data
// Let's use ~24KB to stay well within L1 data cache
#define L1_CACHE_SIZE (24 * 1024)
#define RECORD_SIZE 32  // Keep records small for L1 efficiency
#define DATASET_SIZE (L1_CACHE_SIZE / RECORD_SIZE)  // ~768 records

#define S7T_MAX_CYCLES 7
#define ESTIMATED_CPU_FREQ_GHZ 3.5
#define NS_PER_CYCLE (1.0 / ESTIMATED_CPU_FREQ_GHZ)

// Compact business record optimized for L1 cache
typedef struct {
    int32_t customer_id;
    int32_t product_id;
    float revenue;
    int16_t quarter;
    int16_t region_id;
    int32_t order_date;
    int32_t padding;  // Align to 32 bytes
} __attribute__((packed)) SalesRecord;

// Prevent optimization
volatile int global_result = 0;
volatile float global_result_f = 0.0f;

uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

void generate_l1_sales_data(SalesRecord* sales, int count) {
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
        
        sales[i].padding = seed;
    }
}

// Test 1: Simple filter (mimics WHERE clause)
void test_simple_filter(SalesRecord* sales, int count) {
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

// Test 2: Multi-condition filter (mimics complex WHERE)
void test_complex_filter(SalesRecord* sales, int count) {
    int matches = 0;
    float total = 0;
    
    for (int i = 0; i < count; i++) {
        if (sales[i].revenue > 250.0f && 
            sales[i].quarter >= 2 && 
            sales[i].region_id <= 5) {
            matches++;
            total += sales[i].revenue;
        }
    }
    
    global_result = matches;
    global_result_f = total;
}

// Test 3: Aggregation (mimics GROUP BY)
void test_aggregation(SalesRecord* sales, int count) {
    float region_totals[11] = {0}; // regions 1-10
    int region_counts[11] = {0};
    
    for (int i = 0; i < count; i++) {
        int region = sales[i].region_id;
        if (region >= 1 && region <= 10) {
            region_totals[region] += sales[i].revenue;
            region_counts[region]++;
        }
    }
    
    // Find max region
    int best_region = 1;
    float max_total = region_totals[1];
    for (int i = 2; i <= 10; i++) {
        if (region_totals[i] > max_total) {
            max_total = region_totals[i];
            best_region = i;
        }
    }
    
    global_result = best_region;
    global_result_f = max_total;
}

// Test 4: Join-like operation (mimics table join)
void test_join_operation(SalesRecord* sales, int count) {
    int high_value_customers[1000] = {0}; // Track customers with >$1000 total
    float customer_totals[1000] = {0};
    
    // First pass: calculate customer totals
    for (int i = 0; i < count; i++) {
        int cust_id = sales[i].customer_id % 1000;
        customer_totals[cust_id] += sales[i].revenue;
    }
    
    // Second pass: mark high-value customers
    int high_value_count = 0;
    for (int i = 0; i < 1000; i++) {
        if (customer_totals[i] > 1000.0f) {
            high_value_customers[i] = 1;
            high_value_count++;
        }
    }
    
    // Third pass: count orders from high-value customers
    int high_value_orders = 0;
    float high_value_revenue = 0;
    for (int i = 0; i < count; i++) {
        int cust_id = sales[i].customer_id % 1000;
        if (high_value_customers[cust_id]) {
            high_value_orders++;
            high_value_revenue += sales[i].revenue;
        }
    }
    
    global_result = high_value_orders;
    global_result_f = high_value_revenue;
}

double run_l1_benchmark(const char* name, void (*test_func)(SalesRecord*, int), 
                        SalesRecord* data, int count, int iterations) {
    printf("%s:\n", name);
    
    uint64_t min_ns = UINT64_MAX;
    uint64_t max_ns = 0;
    uint64_t total_ns = 0;
    
    // Warm up
    for (int i = 0; i < 50; i++) {
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
    
    printf("   Time: %.0f ns avg (%.0f - %.0f ns range)\n", avg_ns, (double)min_ns, (double)max_ns);
    printf("   Cycles: %.1f total, %.3f per row\n", avg_cycles, cycles_per_row);
    printf("   7-tick: %s", cycles_per_row <= S7T_MAX_CYCLES ? "PASS ✓" : "FAIL ✗");
    if (cycles_per_row <= S7T_MAX_CYCLES) {
        printf(" (%.1fx under budget)\n", S7T_MAX_CYCLES / cycles_per_row);
    } else {
        printf(" (%.1fx over budget)\n", cycles_per_row / S7T_MAX_CYCLES);
    }
    printf("   Result: %d / $%.2f\n\n", global_result, global_result_f);
    
    return cycles_per_row;
}

int main(int argc, char** argv) {
    printf("7T-SQL L1-Cache Optimized Benchmark\n");
    printf("===================================\n\n");
    
    int iterations = (argc > 1) ? atoi(argv[1]) : 1000;
    
    printf("L1 Cache Configuration:\n");
    printf("   Target cache size: %d KB\n", L1_CACHE_SIZE / 1024);
    printf("   Record size: %zu bytes\n", sizeof(SalesRecord));
    printf("   Dataset size: %d records (%.1f KB)\n", DATASET_SIZE, 
           (DATASET_SIZE * sizeof(SalesRecord)) / 1024.0);
    printf("   Iterations: %d\n", iterations);
    printf("   CPU estimate: %.1f GHz\n\n", ESTIMATED_CPU_FREQ_GHZ);
    
    // Verify we fit in L1
    size_t total_size = DATASET_SIZE * sizeof(SalesRecord);
    if (total_size > L1_CACHE_SIZE) {
        printf("ERROR: Dataset too large for L1 cache!\n");
        return 1;
    }
    
    // Allocate and initialize data
    SalesRecord* sales = malloc(DATASET_SIZE * sizeof(SalesRecord));
    if (!sales) {
        printf("Memory allocation failed\n");
        return 1;
    }
    
    generate_l1_sales_data(sales, DATASET_SIZE);
    printf("Generated L1-optimized business data...\n\n");
    
    // Run L1-friendly benchmarks
    double results[4];
    
    results[0] = run_l1_benchmark("1. Simple Filter (WHERE revenue > 300)", 
                                  test_simple_filter, sales, DATASET_SIZE, iterations);
    
    results[1] = run_l1_benchmark("2. Complex Filter (Multi-condition WHERE)", 
                                  test_complex_filter, sales, DATASET_SIZE, iterations);
    
    results[2] = run_l1_benchmark("3. Aggregation (GROUP BY region)", 
                                  test_aggregation, sales, DATASET_SIZE, iterations);
    
    results[3] = run_l1_benchmark("4. Join Operation (Customer analysis)", 
                                  test_join_operation, sales, DATASET_SIZE, iterations);
    
    // Summary
    printf("=====================================\n");
    printf("L1-Cache Performance Summary:\n");
    
    int passed = 0;
    double total_cycles = 0;
    double min_cycles = results[0];
    double max_cycles = results[0];
    
    for (int i = 0; i < 4; i++) {
        if (results[i] <= S7T_MAX_CYCLES) passed++;
        total_cycles += results[i];
        if (results[i] < min_cycles) min_cycles = results[i];
        if (results[i] > max_cycles) max_cycles = results[i];
    }
    
    printf("   Tests passed: %d/4 (%.0f%%)\n", passed, (passed/4.0)*100);
    printf("   Cycles/row range: %.3f - %.3f\n", min_cycles, max_cycles);
    printf("   Average cycles/row: %.3f\n", total_cycles/4);
    printf("   7-tick compliance: %s\n", passed == 4 ? "FULL ✓" : "PARTIAL");
    printf("   Cache efficiency: L1-optimized ✓\n");
    
    free(sales);
    return passed == 4 ? 0 : 1;
}