/*  ─────────────────────────────────────────────────────────────
    sql_aot_types.h  –  SQL AOT Data Structures for CNS
    Type definitions for ahead-of-time compiled SQL queries
    ───────────────────────────────────────────────────────────── */

#ifndef CNS_SQL_AOT_TYPES_H
#define CNS_SQL_AOT_TYPES_H

#include "../../../include/s7t.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*═══════════════════════════════════════════════════════════════
  Business Entity Structures (Cache-Aligned)
  ═══════════════════════════════════════════════════════════════*/

typedef struct S7T_ALIGNED(64) {
    int32_t customer_id;
    char customer_name[32];
    char email[64];
    int16_t region_id;
    int16_t segment;  // 1=Enterprise, 2=SMB, 3=Consumer
    float lifetime_value;
    uint32_t padding;  // Align to 64 bytes
} Customer;

typedef struct S7T_ALIGNED(32) {
    int32_t order_id;
    int32_t customer_id;
    float amount;
    int32_t product_id;
    char order_date[11];  // YYYY-MM-DD
    int16_t status;       // 1=pending, 2=shipped, 3=delivered, 4=cancelled
    int16_t quarter;
    int16_t year;
    uint16_t padding;
} Order;

typedef struct S7T_ALIGNED(64) {
    int32_t product_id;
    char product_name[64];
    char category[32];
    float price;
    int16_t supplier_id;
    float margin_percent;
    uint32_t padding[3];  // Align to 64 bytes
} Product;

typedef struct S7T_ALIGNED(32) {
    int32_t sale_id;
    int16_t region_id;
    int16_t quarter;
    float revenue;
    int16_t year;
    uint16_t padding;
} SalesRecord;

/*═══════════════════════════════════════════════════════════════
  AOT Query Result Structures
  ═══════════════════════════════════════════════════════════════*/

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
    int32_t product_id;
    char product_name[64];
    float total_sales;
    int32_t order_count;
} ProductPerformanceResult_t;

typedef struct {
    int16_t year;
    int16_t month;
    float monthly_revenue;
    int32_t order_count;
} MonthlyRevenueResult_t;

typedef struct {
    int32_t segment;
    int32_t customer_count;
    float avg_ltv;
    float total_ltv;
} CustomerSegmentResult_t;

/*═══════════════════════════════════════════════════════════════
  AOT Query Parameter Structures
  ═══════════════════════════════════════════════════════════════*/

typedef struct {
    int32_t quarter_num;
} QuarterlySalesParams_t;

typedef struct {
    float min_value;
} HighValueCustomersParams_t;

typedef struct {
    char category_name[32];
} ProductPerformanceParams_t;

typedef struct {
    int32_t start_year;
    int32_t start_month;
} MonthlyRevenueParams_t;

typedef struct {
    int32_t region_filter;
} CustomerSegmentParams_t;

/*═══════════════════════════════════════════════════════════════
  AOT Query Execution Context
  ═══════════════════════════════════════════════════════════════*/

typedef struct S7T_ALIGNED(64) {
    // Data pointers
    const Customer* customers;
    const Order* orders;
    const Product* products;
    const SalesRecord* sales;
    
    // Row counts
    uint32_t customer_count;
    uint32_t order_count;
    uint32_t product_count;
    uint32_t sales_count;
    
    // Performance tracking
    uint64_t total_cycles;
    uint32_t queries_executed;
    uint32_t cache_hits;
    
    // Memory arena for results
    void* result_arena;
    size_t arena_size;
    size_t arena_used;
} AOTQueryContext_t;

/*═══════════════════════════════════════════════════════════════
  AOT Query Function Signatures
  ═══════════════════════════════════════════════════════════════*/

typedef int (*aot_query_func_t)(const AOTQueryContext_t* ctx, const void* params, void* results);

typedef struct {
    const char* name;
    aot_query_func_t function;
    size_t param_size;
    size_t result_size;
    uint32_t estimated_cycles;
} AOTQueryRegistry_t;

/*═══════════════════════════════════════════════════════════════
  Performance Metrics
  ═══════════════════════════════════════════════════════════════*/

typedef struct {
    const char* query_name;
    uint64_t total_cycles;
    uint64_t min_cycles;
    uint64_t max_cycles;
    uint32_t execution_count;
    double cycles_per_row;
    bool meets_7tick_budget;
} AOTQueryMetrics_t;

typedef struct {
    AOTQueryMetrics_t metrics[16];
    uint32_t metric_count;
    uint64_t total_executions;
    double weighted_average_cycles;
} AOTBenchmarkReport_t;

#ifdef __cplusplus
}
#endif

#endif /* CNS_SQL_AOT_TYPES_H */