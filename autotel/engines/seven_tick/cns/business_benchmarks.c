/*  ─────────────────────────────────────────────────────────────
    business_benchmarks.c  –  Real Business Use Case Benchmarks
    5 practical business analyst scenarios with realistic data
    ───────────────────────────────────────────────────────────── */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <math.h>

#define S7T_MAX_CYCLES 7
#define S7T_NS_PER_CYCLE 0.3
#define DATASET_SIZE 10000

// CPU cycle counter for ARM64
static inline uint64_t get_cycles(void) {
    uint64_t val;
    __asm__ __volatile__("mrs %0, cntvct_el0" : "=r" (val));
    return val;
}

// Business data structures
typedef struct {
    int customer_id;
    int product_id;
    float revenue;
    int quarter;
    int region_id;
    int day_of_year;
} SalesRecord;

typedef struct {
    int employee_id;
    int department_id;
    float salary;
    int performance_score;
    int years_experience;
} Employee;

typedef struct {
    int order_id;
    int customer_id;
    float amount;
    int status; // 0=pending, 1=completed, 2=cancelled
    int days_to_fulfill;
} Order;

// Generate realistic business data
void generate_sales_data(SalesRecord* sales, int count) {
    uint32_t seed = (uint32_t)time(NULL);
    for (int i = 0; i < count; i++) {
        seed = seed * 1664525 + 1013904223;
        sales[i].customer_id = (seed % 5000) + 1;
        
        seed = seed * 1664525 + 1013904223;
        sales[i].product_id = (seed % 500) + 1;
        
        seed = seed * 1664525 + 1013904223;
        sales[i].revenue = ((seed % 50000) + 100) / 100.0f; // $1-$500
        
        seed = seed * 1664525 + 1013904223;
        sales[i].quarter = (seed % 4) + 1;
        
        seed = seed * 1664525 + 1013904223;
        sales[i].region_id = (seed % 10) + 1;
        
        seed = seed * 1664525 + 1013904223;
        sales[i].day_of_year = (seed % 365) + 1;
    }
}

void generate_employee_data(Employee* employees, int count) {
    uint32_t seed = (uint32_t)time(NULL) + 12345;
    for (int i = 0; i < count; i++) {
        employees[i].employee_id = i + 1;
        
        seed = seed * 1664525 + 1013904223;
        employees[i].department_id = (seed % 15) + 1;
        
        seed = seed * 1664525 + 1013904223;
        employees[i].salary = ((seed % 80000) + 40000); // $40k-$120k
        
        seed = seed * 1664525 + 1013904223;
        employees[i].performance_score = (seed % 100) + 1; // 1-100
        
        seed = seed * 1664525 + 1013904223;
        employees[i].years_experience = (seed % 25) + 1; // 1-25 years
    }
}

void generate_order_data(Order* orders, int count) {
    uint32_t seed = (uint32_t)time(NULL) + 54321;
    for (int i = 0; i < count; i++) {
        orders[i].order_id = i + 1;
        
        seed = seed * 1664525 + 1013904223;
        orders[i].customer_id = (seed % 5000) + 1;
        
        seed = seed * 1664525 + 1013904223;
        orders[i].amount = ((seed % 100000) + 500) / 100.0f; // $5-$1000
        
        seed = seed * 1664525 + 1013904223;
        orders[i].status = seed % 3; // 0, 1, or 2
        
        seed = seed * 1664525 + 1013904223;
        orders[i].days_to_fulfill = (seed % 30) + 1; // 1-30 days
    }
}

// Benchmark function template
double run_benchmark(const char* name, void (*test_func)(void*), void* data, int iterations) {
    printf("%s:\n", name);
    
    uint64_t min_cycles = UINT64_MAX;
    uint64_t max_cycles = 0;
    uint64_t total_cycles = 0;
    
    // Warm up
    for (int i = 0; i < 100; i++) {
        test_func(data);
    }
    
    // Actual benchmark
    for (int i = 0; i < iterations; i++) {
        uint64_t start = get_cycles();
        test_func(data);
        uint64_t cycles = get_cycles() - start;
        
        total_cycles += cycles;
        if (cycles < min_cycles) min_cycles = cycles;
        if (cycles > max_cycles) max_cycles = cycles;
    }
    
    double avg_cycles = (double)total_cycles / iterations;
    double cycles_per_row = avg_cycles / DATASET_SIZE;
    
    printf("   Average: %.0f cycles total, %.3f cycles/row\n", avg_cycles, cycles_per_row);
    printf("   Min-Max: %llu - %llu cycles\n", min_cycles, max_cycles);
    printf("   Latency: %.2f μs total, %.3f ns/row\n", 
           avg_cycles * S7T_NS_PER_CYCLE / 1000, cycles_per_row * S7T_NS_PER_CYCLE);
    printf("   7-tick: %s (%.1fx %s budget)\n\n", 
           cycles_per_row <= S7T_MAX_CYCLES ? "PASS ✓" : "FAIL ✗",
           cycles_per_row <= S7T_MAX_CYCLES ? S7T_MAX_CYCLES / cycles_per_row : cycles_per_row / S7T_MAX_CYCLES,
           cycles_per_row <= S7T_MAX_CYCLES ? "under" : "over");
    
    return cycles_per_row;
}

// Use Case 1: Sales Performance Analysis
void test_sales_analysis(void* data) {
    SalesRecord* sales = (SalesRecord*)data;
    
    // Find high-value customers (revenue > $300)
    int high_value_count = 0;
    float total_revenue = 0;
    
    for (int i = 0; i < DATASET_SIZE; i++) {
        if (sales[i].revenue > 300.0f) {
            high_value_count++;
            total_revenue += sales[i].revenue;
        }
    }
    
    // Prevent optimization
    volatile int result = high_value_count;
    volatile float result2 = total_revenue;
    (void)result; (void)result2;
}

// Use Case 2: Employee Performance Review
void test_employee_analysis(void* data) {
    Employee* employees = (Employee*)data;
    
    // Find top performers (score > 85) in high-paying roles (salary > $80k)
    int top_performers = 0;
    float avg_salary = 0;
    
    for (int i = 0; i < DATASET_SIZE; i++) {
        if (employees[i].performance_score > 85 && employees[i].salary > 80000) {
            top_performers++;
            avg_salary += employees[i].salary;
        }
    }
    
    if (top_performers > 0) {
        avg_salary /= top_performers;
    }
    
    volatile int result = top_performers;
    volatile float result2 = avg_salary;
    (void)result; (void)result2;
}

// Use Case 3: Order Fulfillment Analysis
void test_order_analysis(void* data) {
    Order* orders = (Order*)data;
    
    // Analyze order completion rates and average fulfillment time
    int completed_orders = 0;
    int total_fulfillment_days = 0;
    float revenue_at_risk = 0;
    
    for (int i = 0; i < DATASET_SIZE; i++) {
        if (orders[i].status == 1) { // completed
            completed_orders++;
            total_fulfillment_days += orders[i].days_to_fulfill;
        } else if (orders[i].status == 0) { // pending
            revenue_at_risk += orders[i].amount;
        }
    }
    
    volatile int result = completed_orders;
    volatile float result2 = revenue_at_risk;
    (void)result; (void)result2;
}

// Use Case 4: Regional Sales Comparison
void test_regional_comparison(void* data) {
    SalesRecord* sales = (SalesRecord*)data;
    
    // Calculate revenue by region (aggregation with grouping)
    float region_revenue[11] = {0}; // regions 1-10
    int region_counts[11] = {0};
    
    for (int i = 0; i < DATASET_SIZE; i++) {
        if (sales[i].region_id >= 1 && sales[i].region_id <= 10) {
            region_revenue[sales[i].region_id] += sales[i].revenue;
            region_counts[sales[i].region_id]++;
        }
    }
    
    // Find best performing region
    int best_region = 1;
    float max_revenue = region_revenue[1];
    for (int i = 2; i <= 10; i++) {
        if (region_revenue[i] > max_revenue) {
            max_revenue = region_revenue[i];
            best_region = i;
        }
    }
    
    volatile int result = best_region;
    volatile float result2 = max_revenue;
    (void)result; (void)result2;
}

// Use Case 5: Time Series Trend Analysis
void test_trend_analysis(void* data) {
    SalesRecord* sales = (SalesRecord*)data;
    
    // Calculate quarterly growth trends
    float quarterly_revenue[5] = {0}; // quarters 1-4
    int quarterly_counts[5] = {0};
    
    for (int i = 0; i < DATASET_SIZE; i++) {
        if (sales[i].quarter >= 1 && sales[i].quarter <= 4) {
            quarterly_revenue[sales[i].quarter] += sales[i].revenue;
            quarterly_counts[sales[i].quarter]++;
        }
    }
    
    // Calculate quarter-over-quarter growth
    float growth_rates[4] = {0};
    for (int i = 2; i <= 4; i++) {
        if (quarterly_revenue[i-1] > 0) {
            growth_rates[i-1] = (quarterly_revenue[i] - quarterly_revenue[i-1]) / quarterly_revenue[i-1];
        }
    }
    
    volatile float result = growth_rates[3]; // Q4 growth
    (void)result;
}

int main(int argc, char** argv) {
    printf("7T-SQL Business Use Case Benchmarks\n");
    printf("===================================\n\n");
    
    int iterations = (argc > 1) ? atoi(argv[1]) : 1000;
    printf("Dataset: %d records, %d iterations per test\n\n", DATASET_SIZE, iterations);
    
    // Allocate and initialize data
    SalesRecord* sales = malloc(DATASET_SIZE * sizeof(SalesRecord));
    Employee* employees = malloc(DATASET_SIZE * sizeof(Employee));
    Order* orders = malloc(DATASET_SIZE * sizeof(Order));
    
    if (!sales || !employees || !orders) {
        printf("Memory allocation failed\n");
        return 1;
    }
    
    generate_sales_data(sales, DATASET_SIZE);
    generate_employee_data(employees, DATASET_SIZE);
    generate_order_data(orders, DATASET_SIZE);
    
    printf("Generated realistic business data...\n\n");
    
    // Run business use case benchmarks
    double results[5];
    
    results[0] = run_benchmark("1. Sales Performance Analysis (High-Value Customer Filter)", 
                              test_sales_analysis, sales, iterations);
    
    results[1] = run_benchmark("2. Employee Performance Review (Multi-Criteria Filter)", 
                              test_employee_analysis, employees, iterations);
    
    results[2] = run_benchmark("3. Order Fulfillment Analysis (Status Aggregation)", 
                              test_order_analysis, orders, iterations);
    
    results[3] = run_benchmark("4. Regional Sales Comparison (GROUP BY Region)", 
                              test_regional_comparison, sales, iterations);
    
    results[4] = run_benchmark("5. Time Series Trend Analysis (Quarterly Growth)", 
                              test_trend_analysis, sales, iterations);
    
    // Summary
    printf("=====================================\n");
    printf("Business Analytics Summary:\n");
    
    int passed = 0;
    double total_cycles = 0;
    for (int i = 0; i < 5; i++) {
        if (results[i] <= S7T_MAX_CYCLES) passed++;
        total_cycles += results[i];
    }
    
    printf("   Tests passed: %d/5 (%.1f%%)\n", passed, (passed/5.0)*100);
    printf("   Average cycles/row: %.3f\n", total_cycles/5);
    printf("   7-tick compliance: %s\n", passed == 5 ? "FULL ✓" : "PARTIAL ⚠");
    printf("   Dataset size: %d records\n", DATASET_SIZE);
    printf("   Platform: ARM64 (Apple Silicon)\n");
    
    free(sales);
    free(employees);
    free(orders);
    
    return passed == 5 ? 0 : 1;
}