/*  ─────────────────────────────────────────────────────────────
    sql_functions.c  –  80/20 Core SQL Function Implementation
    Optimized implementations of the 5 most common SQL operations
    ───────────────────────────────────────────────────────────── */

#include "cns/sql_functions.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <arm_neon.h>

// ═══════════════════════════════════════════════════════════════
// UTILITY FUNCTIONS
// ═══════════════════════════════════════════════════════════════

size_t sql_type_size(sql_data_type_t type) {
    switch (type) {
        case SQL_TYPE_INT32: return sizeof(int32_t);
        case SQL_TYPE_INT64: return sizeof(int64_t);
        case SQL_TYPE_FLOAT32: return sizeof(float);
        case SQL_TYPE_FLOAT64: return sizeof(double);
        case SQL_TYPE_STRING: return sizeof(char*);
        case SQL_TYPE_BOOL: return sizeof(bool);
        default: return 0;
    }
}

sql_column_t* sql_find_column(const sql_table_t* table, const char* name) {
    if (!table || !name) return NULL;
    
    for (uint32_t i = 0; i < table->column_count; i++) {
        if (strcmp(table->columns[i].name, name) == 0) {
            return &table->columns[i];
        }
    }
    return NULL;
}

int sql_get_column_index(const sql_table_t* table, const char* name) {
    if (!table || !name) return -1;
    
    for (uint32_t i = 0; i < table->column_count; i++) {
        if (strcmp(table->columns[i].name, name) == 0) {
            return (int)i;
        }
    }
    return -1;
}

int sql_compare_values(const void* a, const void* b, sql_data_type_t type) {
    switch (type) {
        case SQL_TYPE_INT32: {
            int32_t va = *(const int32_t*)a;
            int32_t vb = *(const int32_t*)b;
            return (va > vb) - (va < vb);
        }
        case SQL_TYPE_INT64: {
            int64_t va = *(const int64_t*)a;
            int64_t vb = *(const int64_t*)b;
            return (va > vb) - (va < vb);
        }
        case SQL_TYPE_FLOAT32: {
            float va = *(const float*)a;
            float vb = *(const float*)b;
            return (va > vb) - (va < vb);
        }
        case SQL_TYPE_FLOAT64: {
            double va = *(const double*)a;
            double vb = *(const double*)b;
            return (va > vb) - (va < vb);
        }
        case SQL_TYPE_STRING:
            return strcmp(*(const char**)a, *(const char**)b);
        case SQL_TYPE_BOOL: {
            bool va = *(const bool*)a;
            bool vb = *(const bool*)b;
            return (va > vb) - (va < vb);
        }
        default:
            return 0;
    }
}

// ═══════════════════════════════════════════════════════════════
// 1. SQL SELECT WITH WHERE (40% of queries)
// ═══════════════════════════════════════════════════════════════

static bool evaluate_condition(const sql_table_t* table, uint32_t row_idx, 
                              const sql_condition_t* condition) {
    sql_column_t* column = sql_find_column(table, condition->column_name);
    if (!column) return false;
    
    size_t type_size = sql_type_size(column->type);
    const void* row_value = (const char*)column->data + (row_idx * type_size);
    
    int cmp = sql_compare_values(row_value, condition->value, column->type);
    
    switch (condition->op) {
        case SQL_OP_EQ: return cmp == 0;
        case SQL_OP_NE: return cmp != 0;
        case SQL_OP_LT: return cmp < 0;
        case SQL_OP_LE: return cmp <= 0;
        case SQL_OP_GT: return cmp > 0;
        case SQL_OP_GE: return cmp >= 0;
        case SQL_OP_LIKE:
            if (column->type == SQL_TYPE_STRING) {
                const char* str = *(const char**)row_value;
                const char* pattern = *(const char**)condition->value;
                return strstr(str, pattern) != NULL;
            }
            return false;
        default:
            return false;
    }
}

int sql_select(const sql_table_t* table, const sql_condition_t* conditions,
               uint32_t condition_count, sql_result_t* result) {
    if (!table || !result) return SQL_ERROR_NULL_POINTER;
    
    // Allocate result indices array
    result->row_indices = malloc(table->row_count * sizeof(uint32_t));
    if (!result->row_indices) return SQL_ERROR_OUT_OF_MEMORY;
    
    result->table = (sql_table_t*)table;
    result->result_count = 0;
    
    // Scan all rows and evaluate conditions
    for (uint32_t i = 0; i < table->row_count; i++) {
        bool matches = true;
        
        // All conditions must be true (AND logic)
        for (uint32_t j = 0; j < condition_count; j++) {
            if (!evaluate_condition(table, i, &conditions[j])) {
                matches = false;
                break;
            }
        }
        
        if (matches) {
            result->row_indices[result->result_count++] = i;
        }
    }
    
    return SQL_OK;
}

// ═══════════════════════════════════════════════════════════════
// 2. SQL GROUP BY WITH AGGREGATIONS (25% of queries)
// ═══════════════════════════════════════════════════════════════

int sql_group_by(const sql_table_t* table, const char** group_columns,
                 uint32_t group_column_count, const char* aggregate_column,
                 sql_aggregate_t* results, uint32_t max_groups, uint32_t* result_count) {
    if (!table || !group_columns || !aggregate_column || !results || !result_count) {
        return SQL_ERROR_NULL_POINTER;
    }
    
    sql_column_t* agg_col = sql_find_column(table, aggregate_column);
    if (!agg_col) return SQL_ERROR_COLUMN_NOT_FOUND;
    
    *result_count = 0;
    
    // Simple implementation: scan all rows and build groups
    for (uint32_t i = 0; i < table->row_count; i++) {
        // Build group key
        char group_key[64] = {0};
        for (uint32_t g = 0; g < group_column_count; g++) {
            sql_column_t* group_col = sql_find_column(table, group_columns[g]);
            if (!group_col) continue;
            
            size_t type_size = sql_type_size(group_col->type);
            const void* value = (const char*)group_col->data + (i * type_size);
            
            if (group_col->type == SQL_TYPE_STRING) {
                strncat(group_key, *(const char**)value, 60);
            } else if (group_col->type == SQL_TYPE_INT32) {
                char temp[16];
                snprintf(temp, sizeof(temp), "%d", *(const int32_t*)value);
                strncat(group_key, temp, 60);
            }
            
            if (g < group_column_count - 1) {
                strncat(group_key, "_", 63);
            }
        }
        
        // Find existing group or create new one
        int group_idx = -1;
        for (uint32_t g = 0; g < *result_count; g++) {
            if (strcmp(results[g].group_key, group_key) == 0) {
                group_idx = g;
                break;
            }
        }
        
        if (group_idx == -1 && *result_count < max_groups) {
            group_idx = *result_count;
            strcpy(results[group_idx].group_key, group_key);
            results[group_idx].sum = 0;
            results[group_idx].count = 0;
            results[group_idx].min = INFINITY;
            results[group_idx].max = -INFINITY;
            (*result_count)++;
        }
        
        if (group_idx >= 0) {
            // Add to aggregate
            double value = 0;
            size_t type_size = sql_type_size(agg_col->type);
            const void* agg_value = (const char*)agg_col->data + (i * type_size);
            
            if (agg_col->type == SQL_TYPE_FLOAT32) {
                value = *(const float*)agg_value;
            } else if (agg_col->type == SQL_TYPE_FLOAT64) {
                value = *(const double*)agg_value;
            } else if (agg_col->type == SQL_TYPE_INT32) {
                value = *(const int32_t*)agg_value;
            } else if (agg_col->type == SQL_TYPE_INT64) {
                value = *(const int64_t*)agg_value;
            }
            
            results[group_idx].sum += value;
            results[group_idx].count++;
            if (value < results[group_idx].min) results[group_idx].min = value;
            if (value > results[group_idx].max) results[group_idx].max = value;
        }
    }
    
    // Calculate averages
    for (uint32_t i = 0; i < *result_count; i++) {
        results[i].avg = results[i].count > 0 ? results[i].sum / results[i].count : 0;
    }
    
    return SQL_OK;
}

// ═══════════════════════════════════════════════════════════════
// 3. SQL JOIN OPERATIONS (20% of queries)
// ═══════════════════════════════════════════════════════════════

int sql_join(const sql_table_t* left_table, const sql_table_t* right_table,
             const char* left_key_column, const char* right_key_column,
             sql_join_type_t join_type, sql_table_t* result) {
    if (!left_table || !right_table || !left_key_column || !right_key_column || !result) {
        return SQL_ERROR_NULL_POINTER;
    }
    
    sql_column_t* left_key = sql_find_column(left_table, left_key_column);
    sql_column_t* right_key = sql_find_column(right_table, right_key_column);
    
    if (!left_key || !right_key) return SQL_ERROR_COLUMN_NOT_FOUND;
    if (left_key->type != right_key->type) return SQL_ERROR_TYPE_MISMATCH;
    
    // For simplicity, implement only INNER JOIN
    if (join_type != SQL_JOIN_INNER) return SQL_ERROR_INVALID_OPERATION;
    
    // Allocate result table
    result->column_count = left_table->column_count + right_table->column_count;
    result->columns = malloc(result->column_count * sizeof(sql_column_t));
    if (!result->columns) return SQL_ERROR_OUT_OF_MEMORY;
    
    // Copy column definitions (simplified)
    memcpy(result->columns, left_table->columns, 
           left_table->column_count * sizeof(sql_column_t));
    memcpy(result->columns + left_table->column_count, right_table->columns,
           right_table->column_count * sizeof(sql_column_t));
    
    // Nested loop join (simple but functional)
    size_t left_key_size = sql_type_size(left_key->type);
    size_t right_key_size = sql_type_size(right_key->type);
    
    result->row_count = 0;
    uint32_t max_result_rows = left_table->row_count * right_table->row_count;
    
    // Allocate data arrays for result columns
    for (uint32_t c = 0; c < result->column_count; c++) {
        result->columns[c].capacity = max_result_rows;
        result->columns[c].count = 0;
        result->columns[c].data = malloc(max_result_rows * sql_type_size(result->columns[c].type));
        if (!result->columns[c].data) return SQL_ERROR_OUT_OF_MEMORY;
    }
    
    // Perform join
    for (uint32_t i = 0; i < left_table->row_count; i++) {
        const void* left_value = (const char*)left_key->data + (i * left_key_size);
        
        for (uint32_t j = 0; j < right_table->row_count; j++) {
            const void* right_value = (const char*)right_key->data + (j * right_key_size);
            
            if (sql_compare_values(left_value, right_value, left_key->type) == 0) {
                // Match found - copy row data
                for (uint32_t c = 0; c < left_table->column_count; c++) {
                    size_t type_size = sql_type_size(left_table->columns[c].type);
                    const void* src = (const char*)left_table->columns[c].data + (i * type_size);
                    void* dst = (char*)result->columns[c].data + (result->row_count * type_size);
                    memcpy(dst, src, type_size);
                }
                
                for (uint32_t c = 0; c < right_table->column_count; c++) {
                    size_t type_size = sql_type_size(right_table->columns[c].type);
                    const void* src = (const char*)right_table->columns[c].data + (j * type_size);
                    void* dst = (char*)result->columns[left_table->column_count + c].data + 
                               (result->row_count * type_size);
                    memcpy(dst, src, type_size);
                }
                
                result->row_count++;
            }
        }
    }
    
    // Update column counts
    for (uint32_t c = 0; c < result->column_count; c++) {
        result->columns[c].count = result->row_count;
    }
    
    return SQL_OK;
}

// ═══════════════════════════════════════════════════════════════
// 4. SQL ORDER BY WITH LIMIT (10% of queries)
// ═══════════════════════════════════════════════════════════════

typedef struct {
    uint32_t row_index;
    void* sort_value;
    sql_data_type_t type;
} sort_item_t;

static int compare_sort_items(const void* a, const void* b) {
    const sort_item_t* item_a = (const sort_item_t*)a;
    const sort_item_t* item_b = (const sort_item_t*)b;
    return sql_compare_values(item_a->sort_value, item_b->sort_value, item_a->type);
}

int sql_order_by(const sql_table_t* table, const char* order_column,
                 bool ascending, uint32_t limit, sql_result_t* result) {
    if (!table || !order_column || !result) return SQL_ERROR_NULL_POINTER;
    
    sql_column_t* sort_col = sql_find_column(table, order_column);
    if (!sort_col) return SQL_ERROR_COLUMN_NOT_FOUND;
    
    // Create sort items
    sort_item_t* items = malloc(table->row_count * sizeof(sort_item_t));
    if (!items) return SQL_ERROR_OUT_OF_MEMORY;
    
    size_t type_size = sql_type_size(sort_col->type);
    
    for (uint32_t i = 0; i < table->row_count; i++) {
        items[i].row_index = i;
        items[i].type = sort_col->type;
        items[i].sort_value = malloc(type_size);
        memcpy(items[i].sort_value, (const char*)sort_col->data + (i * type_size), type_size);
    }
    
    // Sort
    qsort(items, table->row_count, sizeof(sort_item_t), compare_sort_items);
    
    // Create result
    uint32_t result_count = (limit > 0 && limit < table->row_count) ? limit : table->row_count;
    result->row_indices = malloc(result_count * sizeof(uint32_t));
    if (!result->row_indices) {
        free(items);
        return SQL_ERROR_OUT_OF_MEMORY;
    }
    
    result->table = (sql_table_t*)table;
    result->result_count = result_count;
    
    // Copy indices in correct order
    for (uint32_t i = 0; i < result_count; i++) {
        uint32_t idx = ascending ? i : (table->row_count - 1 - i);
        result->row_indices[i] = items[idx].row_index;
    }
    
    // Cleanup
    for (uint32_t i = 0; i < table->row_count; i++) {
        free(items[i].sort_value);
    }
    free(items);
    
    return SQL_OK;
}

// ═══════════════════════════════════════════════════════════════
// 5. SQL AGGREGATE FUNCTIONS (5% of queries)
// ═══════════════════════════════════════════════════════════════

double sql_aggregate(const sql_table_t* table, const char* column_name, const char* operation) {
    if (!table || !column_name || !operation) return 0.0;
    
    sql_column_t* column = sql_find_column(table, column_name);
    if (!column) return 0.0;
    
    double result = 0.0;
    uint32_t count = 0;
    double min_val = INFINITY;
    double max_val = -INFINITY;
    
    size_t type_size = sql_type_size(column->type);
    
    for (uint32_t i = 0; i < table->row_count; i++) {
        double value = 0.0;
        const void* data = (const char*)column->data + (i * type_size);
        
        switch (column->type) {
            case SQL_TYPE_INT32:
                value = *(const int32_t*)data;
                break;
            case SQL_TYPE_INT64:
                value = *(const int64_t*)data;
                break;
            case SQL_TYPE_FLOAT32:
                value = *(const float*)data;
                break;
            case SQL_TYPE_FLOAT64:
                value = *(const double*)data;
                break;
            default:
                continue;
        }
        
        result += value;
        count++;
        if (value < min_val) min_val = value;
        if (value > max_val) max_val = value;
    }
    
    if (strcmp(operation, "COUNT") == 0) {
        return count;
    } else if (strcmp(operation, "SUM") == 0) {
        return result;
    } else if (strcmp(operation, "AVG") == 0) {
        return count > 0 ? result / count : 0.0;
    } else if (strcmp(operation, "MIN") == 0) {
        return min_val;
    } else if (strcmp(operation, "MAX") == 0) {
        return max_val;
    }
    
    return 0.0;
}

// ═══════════════════════════════════════════════════════════════
// TABLE MANAGEMENT FUNCTIONS
// ═══════════════════════════════════════════════════════════════

sql_table_t* sql_create_table(const char* name, uint32_t initial_capacity) {
    sql_table_t* table = malloc(sizeof(sql_table_t));
    if (!table) return NULL;
    
    strncpy(table->name, name, sizeof(table->name) - 1);
    table->name[sizeof(table->name) - 1] = '\0';
    table->columns = NULL;
    table->column_count = 0;
    table->row_count = 0;
    
    return table;
}

int sql_add_column(sql_table_t* table, const char* name, sql_data_type_t type) {
    if (!table || !name) return SQL_ERROR_NULL_POINTER;
    
    table->columns = realloc(table->columns, (table->column_count + 1) * sizeof(sql_column_t));
    if (!table->columns) return SQL_ERROR_OUT_OF_MEMORY;
    
    sql_column_t* col = &table->columns[table->column_count];
    strncpy(col->name, name, sizeof(col->name) - 1);
    col->name[sizeof(col->name) - 1] = '\0';
    col->type = type;
    col->count = 0;
    col->capacity = 1000;  // Default capacity
    col->data = malloc(col->capacity * sql_type_size(type));
    
    if (!col->data) return SQL_ERROR_OUT_OF_MEMORY;
    
    table->column_count++;
    return SQL_OK;
}

void sql_free_table(sql_table_t* table) {
    if (!table) return;
    
    for (uint32_t i = 0; i < table->column_count; i++) {
        free(table->columns[i].data);
    }
    free(table->columns);
    free(table);
}

sql_result_t* sql_create_result(void) {
    sql_result_t* result = malloc(sizeof(sql_result_t));
    if (result) {
        result->table = NULL;
        result->row_indices = NULL;
        result->result_count = 0;
    }
    return result;
}

void sql_free_result(sql_result_t* result) {
    if (!result) return;
    free(result->row_indices);
    free(result);
}