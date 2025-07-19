/*  ─────────────────────────────────────────────────────────────
    sql_functions.h  –  80/20 Core SQL Function Library
    5 essential functions that handle 80% of SQL use cases
    ───────────────────────────────────────────────────────────── */

#ifndef CNS_SQL_FUNCTIONS_H
#define CNS_SQL_FUNCTIONS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Core data types
typedef enum {
    SQL_TYPE_INT32,
    SQL_TYPE_INT64,
    SQL_TYPE_FLOAT32,
    SQL_TYPE_FLOAT64,
    SQL_TYPE_STRING,
    SQL_TYPE_BOOL
} sql_data_type_t;

// Column definition
typedef struct {
    void* data;
    sql_data_type_t type;
    uint32_t count;
    uint32_t capacity;
    char name[32];
} sql_column_t;

// Table structure
typedef struct {
    sql_column_t* columns;
    uint32_t column_count;
    uint32_t row_count;
    char name[64];
} sql_table_t;

// Condition for WHERE clauses
typedef enum {
    SQL_OP_EQ,    // =
    SQL_OP_NE,    // !=
    SQL_OP_LT,    // <
    SQL_OP_LE,    // <=
    SQL_OP_GT,    // >
    SQL_OP_GE,    // >=
    SQL_OP_LIKE,  // LIKE
    SQL_OP_IN     // IN
} sql_operator_t;

typedef struct {
    char column_name[32];
    sql_operator_t op;
    void* value;
    sql_data_type_t value_type;
} sql_condition_t;

// Result set
typedef struct {
    sql_table_t* table;
    uint32_t* row_indices;  // Which rows from source table
    uint32_t result_count;
} sql_result_t;

// Aggregate result
typedef struct {
    char group_key[64];
    double sum;
    double avg;
    uint32_t count;
    double min;
    double max;
} sql_aggregate_t;

// Join types
typedef enum {
    SQL_JOIN_INNER,
    SQL_JOIN_LEFT,
    SQL_JOIN_RIGHT,
    SQL_JOIN_FULL
} sql_join_type_t;

// ═══════════════════════════════════════════════════════════════
// 80/20 CORE SQL FUNCTIONS (Handle 80% of business use cases)
// ═══════════════════════════════════════════════════════════════

// 1. SELECT with WHERE (40% of all SQL queries)
// Usage: sql_select(table, conditions, condition_count, result)
int sql_select(
    const sql_table_t* table,
    const sql_condition_t* conditions,
    uint32_t condition_count,
    sql_result_t* result
);

// 2. GROUP BY with aggregations (25% of all SQL queries)  
// Usage: sql_group_by(table, group_columns, agg_column, results, max_groups)
int sql_group_by(
    const sql_table_t* table,
    const char** group_columns,
    uint32_t group_column_count,
    const char* aggregate_column,
    sql_aggregate_t* results,
    uint32_t max_groups,
    uint32_t* result_count
);

// 3. JOIN operations (20% of all SQL queries)
// Usage: sql_join(left_table, right_table, left_key, right_key, join_type, result)
int sql_join(
    const sql_table_t* left_table,
    const sql_table_t* right_table,
    const char* left_key_column,
    const char* right_key_column,
    sql_join_type_t join_type,
    sql_table_t* result
);

// 4. ORDER BY with LIMIT (10% of all SQL queries)
// Usage: sql_order_by(table, column_name, ascending, limit, result)
int sql_order_by(
    const sql_table_t* table,
    const char* order_column,
    bool ascending,
    uint32_t limit,
    sql_result_t* result
);

// 5. Aggregate functions - COUNT/SUM/AVG (5% of all SQL queries)
// Usage: sql_aggregate(table, column_name, operation)
double sql_aggregate(
    const sql_table_t* table,
    const char* column_name,
    const char* operation  // "COUNT", "SUM", "AVG", "MIN", "MAX"
);

// ═══════════════════════════════════════════════════════════════
// UTILITY FUNCTIONS
// ═══════════════════════════════════════════════════════════════

// Table creation and management
sql_table_t* sql_create_table(const char* name, uint32_t initial_capacity);
int sql_add_column(sql_table_t* table, const char* name, sql_data_type_t type);
int sql_insert_row(sql_table_t* table, void** values);
void sql_free_table(sql_table_t* table);

// Column utilities
sql_column_t* sql_find_column(const sql_table_t* table, const char* name);
int sql_get_column_index(const sql_table_t* table, const char* name);

// Result management
sql_result_t* sql_create_result(void);
void sql_free_result(sql_result_t* result);

// Data type utilities
size_t sql_type_size(sql_data_type_t type);
int sql_compare_values(const void* a, const void* b, sql_data_type_t type);

// ═══════════════════════════════════════════════════════════════
// PERFORMANCE OPTIMIZED VERSIONS (7T-compliant)
// ═══════════════════════════════════════════════════════════════

// SIMD-optimized SELECT for numeric columns
int sql_select_simd(
    const sql_table_t* table,
    const sql_condition_t* conditions,
    uint32_t condition_count,
    sql_result_t* result
);

// Cache-optimized GROUP BY for hot-path aggregations
int sql_group_by_fast(
    const sql_table_t* table,
    const char* group_column,
    const char* aggregate_column,
    sql_aggregate_t* results,
    uint32_t max_groups,
    uint32_t* result_count
);

// Hash join for large datasets
int sql_hash_join(
    const sql_table_t* left_table,
    const sql_table_t* right_table,
    const char* left_key_column,
    const char* right_key_column,
    sql_table_t* result
);

// ═══════════════════════════════════════════════════════════════
// ERROR CODES
// ═══════════════════════════════════════════════════════════════

#define SQL_OK                    0
#define SQL_ERROR_NULL_POINTER   -1
#define SQL_ERROR_COLUMN_NOT_FOUND -2
#define SQL_ERROR_TYPE_MISMATCH   -3
#define SQL_ERROR_OUT_OF_MEMORY   -4
#define SQL_ERROR_INVALID_OPERATION -5
#define SQL_ERROR_BUFFER_TOO_SMALL -6

#ifdef __cplusplus
}
#endif

#endif // CNS_SQL_FUNCTIONS_H