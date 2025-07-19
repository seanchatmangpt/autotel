/*  ─────────────────────────────────────────────────────────────
    simple_test.c  –  Simple test of generated function signatures
    ───────────────────────────────────────────────────────────── */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// Mock types for testing
typedef struct {
    int32_t region_id;
    float total_revenue;
    int32_t record_count;
} QuarterlySalesResult_t;

typedef struct {
    int32_t customer_id;
    char customer_name[32];
    float lifetime_value;
    int32_t region_id;
} HighValueCustomerResult_t;

typedef struct {
    int32_t segment;
    int32_t customer_count;
    float avg_ltv;
    float total_ltv;
} CustomerSegmentResult_t;

typedef struct {
    int32_t sale_id;
    int16_t region_id;
    int16_t quarter;
    float revenue;
    int16_t year;
} SalesRecord;

typedef struct {
    int32_t customer_id;
    char customer_name[32];
    char email[64];
    int16_t region_id;
    int16_t segment;
    float lifetime_value;
} Customer;

// Test function signature compilation
static inline int run_query_quarterly_sales_report(const SalesRecord* sales_data, int data_count, int32_t quarter_num, QuarterlySalesResult_t* results) {
    printf("Testing quarterly sales report with %d records, quarter %d\n", data_count, quarter_num);
    return 0;
}

static inline int run_query_high_value_customers(const Customer* customers_data, int data_count, float min_value, HighValueCustomerResult_t* results) {
    printf("Testing high value customers with %d records, min value %.2f\n", data_count, min_value);
    return 0;
}

static inline int run_query_customer_segment_analysis(const Customer* customers_data, int data_count, int32_t region_filter, CustomerSegmentResult_t* results) {
    printf("Testing customer segment analysis with %d records, region %d\n", data_count, region_filter);
    return 0;
}

int main() {
    printf("SQL Compiler Function Signature Test\n");
    printf("=====================================\n\n");
    
    // Test data
    SalesRecord sales[10];
    Customer customers[10];
    
    // Test function calls
    QuarterlySalesResult_t sales_results[10];
    HighValueCustomerResult_t customer_results[10];
    CustomerSegmentResult_t segment_results[10];
    
    run_query_quarterly_sales_report(sales, 10, 1, sales_results);
    run_query_high_value_customers(customers, 10, 5000.0f, customer_results);
    run_query_customer_segment_analysis(customers, 10, 1, segment_results);
    
    printf("\nFunction signature test completed successfully!\n");
    return 0;
}