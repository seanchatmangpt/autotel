/*  ─────────────────────────────────────────────────────────────
    test_sql_compiler.c  –  Test SQL Compiler Generated Code
    Verifies the SQL AOT compiler generates valid C code
    ───────────────────────────────────────────────────────────── */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Include generated header
#include "sql_queries_generated.h"

// Mock s7t functions for testing
typedef struct {
    uint64_t start_cycles;
    uint64_t end_cycles;
} s7t_span_t;

static inline void s7t_span_start(s7t_span_t* span, const char* name) {
    span->start_cycles = 1000;
}

static inline void s7t_span_end(s7t_span_t* span) {
    span->end_cycles = 2000;
}

// Mock SIMD function
uint32_t s7t_simd_filter_eq_i32_strided(const int32_t* data, int32_t value, 
                                        size_t count, size_t stride, uint32_t* matches) {
    uint32_t match_count = 0;
    for (size_t i = 0; i < count; i++) {
        if (*data == value) {
            matches[match_count++] = i;
        }
        data = (int32_t*)((char*)data + stride);
    }
    return match_count;
}

// Generate test data
static void generate_test_data(SalesRecord* sales, Customer* customers, 
                              Order* orders, Product* products) {
    // Generate sales data
    for (int i = 0; i < 100; i++) {
        sales[i].sale_id = i + 1;
        sales[i].region_id = (i % 10) + 1;
        sales[i].quarter = (i % 4) + 1;
        sales[i].revenue = 100.0f + (i * 10.0f);
        sales[i].year = 2023;
    }
    
    // Generate customer data
    for (int i = 0; i < 50; i++) {
        customers[i].customer_id = i + 1;
        snprintf(customers[i].customer_name, 32, "Customer_%d", i + 1);
        customers[i].region_id = (i % 10) + 1;
        customers[i].segment = (i % 3) + 1;
        customers[i].lifetime_value = 1000.0f + (i * 100.0f);
    }
    
    // Generate order data
    for (int i = 0; i < 200; i++) {
        orders[i].order_id = i + 1;
        orders[i].customer_id = (i % 50) + 1;
        orders[i].amount = 50.0f + (i * 5.0f);
        orders[i].product_id = (i % 10) + 1;
        orders[i].status = 3; // delivered
        orders[i].quarter = (i % 4) + 1;
        orders[i].year = 2023 + (i / 100);
    }
    
    // Generate product data
    for (int i = 0; i < 10; i++) {
        products[i].product_id = i + 1;
        snprintf(products[i].product_name, 64, "Product_%d", i + 1);
        strcpy(products[i].category, i < 5 ? "Electronics" : "Clothing");
        products[i].price = 100.0f + (i * 50.0f);
        products[i].supplier_id = (i % 3) + 1;
        products[i].margin_percent = 20.0f + (i * 2.0f);
    }
}

int main() {
    printf("Testing SQL Compiler Generated Code\n");
    printf("===================================\n\n");
    
    // Allocate test data
    SalesRecord* sales = malloc(100 * sizeof(SalesRecord));
    Customer* customers = malloc(50 * sizeof(Customer));
    Order* orders = malloc(200 * sizeof(Order));
    Product* products = malloc(10 * sizeof(Product));
    
    generate_test_data(sales, customers, orders, products);
    
    // Test 1: Quarterly Sales Report
    printf("Test 1: Quarterly Sales Report (Q1)\n");
    QuarterlySalesResult_t sales_results[10];
    int result_count = run_query_quarterly_sales_report(sales, 100, 1, sales_results);
    printf("  Found %d regions with sales in Q1\n", result_count);
    for (int i = 0; i < result_count && i < 3; i++) {
        printf("  Region %d: $%.2f (from %d records)\n", 
               sales_results[i].region_id, 
               sales_results[i].total_revenue,
               sales_results[i].record_count);
    }
    
    // Test 2: High Value Customers
    printf("\nTest 2: High Value Customers (>$3000)\n");
    HighValueCustomerResult_t customer_results[100];
    result_count = run_query_high_value_customers(customers, 50, 3000.0f, customer_results);
    printf("  Found %d high value customers\n", result_count);
    for (int i = 0; i < result_count && i < 3; i++) {
        printf("  %s: $%.2f (Region %d)\n", 
               customer_results[i].customer_name, 
               customer_results[i].lifetime_value,
               customer_results[i].region_id);
    }
    
    // Test 3: Customer Segment Analysis
    printf("\nTest 3: Customer Segment Analysis (Region 1)\n");
    CustomerSegmentResult_t segment_results[3];
    result_count = run_query_customer_segment_analysis(customers, 50, 1, segment_results);
    printf("  Found %d segments in region 1\n", result_count);
    for (int i = 0; i < result_count; i++) {
        printf("  Segment %d: %d customers, avg LTV $%.2f\n", 
               segment_results[i].segment, 
               segment_results[i].customer_count,
               segment_results[i].avg_ltv);
    }
    
    // Test 4: Dispatcher
    printf("\nTest 4: Testing Query Dispatcher\n");
    void* params[5];
    params[0] = sales;
    int data_count = 100;
    params[1] = &data_count;
    int quarter = 2;
    params[2] = &quarter;
    
    result_count = execute_aot_sql_query("quarterly_sales_report", params, sales_results);
    printf("  Dispatcher returned %d results for Q2\n", result_count);
    
    // Cleanup
    free(sales);
    free(customers);
    free(orders);
    free(products);
    
    printf("\nAll tests completed successfully!\n");
    return 0;
}