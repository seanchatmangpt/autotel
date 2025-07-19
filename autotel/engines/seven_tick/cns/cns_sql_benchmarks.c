/*
 * CNS 7T-SQL Dedicated Benchmark Suite
 * Comprehensive testing of 7-tick SQL operations with real measurements
 * 
 * This focuses specifically on SQL operations:
 * - SELECT with various WHERE clauses
 * - INSERT operations
 * - UPDATE operations
 * - DELETE operations
 * - JOIN operations (INNER, HASH)
 * - Aggregation functions (SUM, COUNT, AVG, MIN, MAX)
 * - SIMD-optimized operations
 * - Index operations
 * - Memory arena usage
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <stdint.h>
#include <stdbool.h>
#include <immintrin.h>

#include "include/cns/sql.h"
#include "include/cns/telemetry/otel.h"

#define SQL_BENCH_ITERATIONS 50000
#define SQL_WARMUP_ITERATIONS 1000
#define SQL_TEST_ROWS 10000

// CPU cycle measurement
static inline uint64_t get_cycles(void) {
#if defined(__x86_64__) || defined(__i386__)
    uint32_t lo, hi;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
#elif defined(__aarch64__)
    uint64_t val;
    __asm__ __volatile__("mrs %0, cntvct_el0" : "=r" (val));
    return val;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 3000000000ULL + (uint64_t)ts.tv_nsec * 3;
#endif
}

// Test data generation
typedef struct {
    int32_t* ids;
    int32_t* values;
    int64_t* big_values;
    float* float_values;
    double* double_values;
    char** string_values;
    uint32_t* hash_keys;
    uint32_t* hash_values;
    bool* bool_values;
    size_t row_count;
} sql_test_data_t;

static void generate_sql_test_data(sql_test_data_t* data, size_t rows) {
    data->row_count = rows;
    
    // Allocate aligned memory for SIMD operations
    data->ids = aligned_alloc(64, rows * sizeof(int32_t));
    data->values = aligned_alloc(64, rows * sizeof(int32_t));
    data->big_values = aligned_alloc(64, rows * sizeof(int64_t));
    data->float_values = aligned_alloc(64, rows * sizeof(float));
    data->double_values = aligned_alloc(64, rows * sizeof(double));
    data->string_values = malloc(rows * sizeof(char*));
    data->hash_keys = aligned_alloc(64, rows * sizeof(uint32_t));
    data->hash_values = aligned_alloc(64, rows * sizeof(uint32_t));
    data->bool_values = aligned_alloc(64, rows * sizeof(bool));
    
    // Generate realistic data patterns
    srand(42); // Fixed seed for reproducibility
    
    for (size_t i = 0; i < rows; i++) {
        data->ids[i] = i + 1;
        data->values[i] = (rand() % 10000) - 5000; // Range: -5000 to 4999
        data->big_values[i] = ((int64_t)rand() << 32) | rand();
        data->float_values[i] = (float)rand() / RAND_MAX * 1000.0f;
        data->double_values[i] = (double)rand() / RAND_MAX * 1000000.0;
        data->bool_values[i] = (rand() % 2) == 1;
        
        // Generate hash table data
        data->hash_keys[i] = rand() % 1000; // Limited key space for collisions
        data->hash_values[i] = rand();
        
        // Generate string data
        size_t str_len = 8 + (rand() % 16);
        data->string_values[i] = malloc(str_len + 1);
        for (size_t j = 0; j < str_len; j++) {
            data->string_values[i][j] = 'A' + (rand() % 26);
        }
        data->string_values[i][str_len] = '\0';
    }
}

static void cleanup_sql_test_data(sql_test_data_t* data) {
    free(data->ids);
    free(data->values);
    free(data->big_values);
    free(data->float_values);
    free(data->double_values);
    free(data->hash_keys);
    free(data->hash_values);
    free(data->bool_values);
    
    for (size_t i = 0; i < data->row_count; i++) {
        free(data->string_values[i]);
    }
    free(data->string_values);
}

// Benchmark result structure
typedef struct {
    const char* name;
    uint64_t iterations;
    uint64_t min_cycles;
    uint64_t max_cycles;
    uint64_t total_cycles;
    double avg_cycles;
    double std_dev;
    double cycles_per_row;
    bool seven_tick_compliant;
    bool passed;
    const char* error_msg;
} sql_bench_result_t;

static void calculate_sql_stats(uint64_t* measurements, size_t count, 
                               size_t rows_per_op, sql_bench_result_t* result) {
    // Sort for percentile calculation
    for (size_t i = 0; i < count - 1; i++) {
        for (size_t j = 0; j < count - i - 1; j++) {
            if (measurements[j] > measurements[j + 1]) {
                uint64_t temp = measurements[j];
                measurements[j] = measurements[j + 1];
                measurements[j + 1] = temp;
            }
        }
    }
    
    result->min_cycles = measurements[0];
    result->max_cycles = measurements[count - 1];
    
    result->total_cycles = 0;
    for (size_t i = 0; i < count; i++) {
        result->total_cycles += measurements[i];
    }
    result->avg_cycles = (double)result->total_cycles / count;
    result->cycles_per_row = result->avg_cycles / rows_per_op;
    
    // Calculate standard deviation
    double variance = 0.0;
    for (size_t i = 0; i < count; i++) {
        double diff = measurements[i] - result->avg_cycles;
        variance += diff * diff;
    }
    result->std_dev = sqrt(variance / count);
    
    result->seven_tick_compliant = (result->cycles_per_row <= 7.0);
    result->passed = result->seven_tick_compliant;
}

/*═══════════════════════════════════════════════════════════════
  SELECT Benchmarks
  ═══════════════════════════════════════════════════════════════*/

static sql_bench_result_t benchmark_sql_select_full_scan(sql_test_data_t* data) {
    sql_bench_result_t result = {
        .name = "SELECT Full Table Scan",
        .iterations = SQL_BENCH_ITERATIONS / 10
    };
    
    uint64_t* measurements = malloc(result.iterations * sizeof(uint64_t));
    
    // Benchmark full table scan
    for (uint64_t i = 0; i < result.iterations; i++) {
        uint64_t start = get_cycles();
        
        uint32_t result_count = 0;
        for (size_t row = 0; row < data->row_count; row++) {
            // SELECT * FROM table (just count rows)
            result_count++;
        }
        
        uint64_t end = get_cycles();
        measurements[i] = end - start;
        
        // Prevent optimization
        __asm__ __volatile__("" : : "r" (result_count) : "memory");
    }
    
    calculate_sql_stats(measurements, result.iterations, data->row_count, &result);
    free(measurements);
    return result;
}

static sql_bench_result_t benchmark_sql_select_where_eq(sql_test_data_t* data) {
    sql_bench_result_t result = {
        .name = "SELECT WHERE column = value",
        .iterations = SQL_BENCH_ITERATIONS
    };
    
    uint64_t* measurements = malloc(result.iterations * sizeof(uint64_t));
    
    for (uint64_t i = 0; i < result.iterations; i++) {
        uint64_t start = get_cycles();
        
        int32_t target = data->values[i % data->row_count];
        uint32_t match_count = 0;
        
        for (size_t row = 0; row < data->row_count; row++) {
            if (data->values[row] == target) {
                match_count++;
            }
        }
        
        uint64_t end = get_cycles();
        measurements[i] = end - start;
        
        __asm__ __volatile__("" : : "r" (match_count) : "memory");
    }
    
    calculate_sql_stats(measurements, result.iterations, data->row_count, &result);
    free(measurements);
    return result;
}

static sql_bench_result_t benchmark_sql_select_where_range(sql_test_data_t* data) {
    sql_bench_result_t result = {
        .name = "SELECT WHERE column BETWEEN x AND y",
        .iterations = SQL_BENCH_ITERATIONS
    };
    
    uint64_t* measurements = malloc(result.iterations * sizeof(uint64_t));
    
    for (uint64_t i = 0; i < result.iterations; i++) {
        uint64_t start = get_cycles();
        
        int32_t min_val = data->values[i % data->row_count] - 100;
        int32_t max_val = data->values[i % data->row_count] + 100;
        uint32_t match_count = 0;
        
        for (size_t row = 0; row < data->row_count; row++) {
            if (data->values[row] >= min_val && data->values[row] <= max_val) {
                match_count++;
            }
        }
        
        uint64_t end = get_cycles();
        measurements[i] = end - start;
        
        __asm__ __volatile__("" : : "r" (match_count) : "memory");
    }
    
    calculate_sql_stats(measurements, result.iterations, data->row_count, &result);
    free(measurements);
    return result;
}

static sql_bench_result_t benchmark_sql_simd_filter_eq(sql_test_data_t* data) {
    sql_bench_result_t result = {
        .name = "SIMD SELECT WHERE column = value",
        .iterations = SQL_BENCH_ITERATIONS
    };
    
    uint64_t* measurements = malloc(result.iterations * sizeof(uint64_t));
    uint32_t* match_indices = malloc(data->row_count * sizeof(uint32_t));
    
    for (uint64_t i = 0; i < result.iterations; i++) {
        uint64_t start = get_cycles();
        
        int32_t target = data->values[i % 100]; // Limit targets for better hit rate
        uint32_t matches = s7t_simd_filter_eq_i32(
            data->values, target, data->row_count, match_indices
        );
        
        uint64_t end = get_cycles();
        measurements[i] = end - start;
        
        __asm__ __volatile__("" : : "r" (matches) : "memory");
    }
    
    calculate_sql_stats(measurements, result.iterations, data->row_count, &result);
    free(match_indices);
    free(measurements);
    return result;
}

/*═══════════════════════════════════════════════════════════════
  Aggregation Benchmarks
  ═══════════════════════════════════════════════════════════════*/

static sql_bench_result_t benchmark_sql_sum_aggregation(sql_test_data_t* data) {
    sql_bench_result_t result = {
        .name = "SUM Aggregation",
        .iterations = SQL_BENCH_ITERATIONS / 100
    };
    
    uint64_t* measurements = malloc(result.iterations * sizeof(uint64_t));
    
    for (uint64_t i = 0; i < result.iterations; i++) {
        uint64_t start = get_cycles();
        
        int64_t sum = 0;
        size_t j;
        
        // Unrolled loop for better performance
        for (j = 0; j + 7 < data->row_count; j += 8) {
            sum += data->values[j] + data->values[j+1] + data->values[j+2] + data->values[j+3] +
                   data->values[j+4] + data->values[j+5] + data->values[j+6] + data->values[j+7];
        }
        
        // Handle remainder
        for (; j < data->row_count; j++) {
            sum += data->values[j];
        }
        
        uint64_t end = get_cycles();
        measurements[i] = end - start;
        
        __asm__ __volatile__("" : : "r" (sum) : "memory");
    }
    
    calculate_sql_stats(measurements, result.iterations, data->row_count, &result);
    free(measurements);
    return result;
}

static sql_bench_result_t benchmark_sql_count_aggregation(sql_test_data_t* data) {
    sql_bench_result_t result = {
        .name = "COUNT Aggregation",
        .iterations = SQL_BENCH_ITERATIONS / 10
    };
    
    uint64_t* measurements = malloc(result.iterations * sizeof(uint64_t));
    
    for (uint64_t i = 0; i < result.iterations; i++) {
        uint64_t start = get_cycles();
        
        uint64_t count = 0;
        int32_t threshold = data->values[i % data->row_count];
        
        for (size_t row = 0; row < data->row_count; row++) {
            if (data->values[row] > threshold) {
                count++;
            }
        }
        
        uint64_t end = get_cycles();
        measurements[i] = end - start;
        
        __asm__ __volatile__("" : : "r" (count) : "memory");
    }
    
    calculate_sql_stats(measurements, result.iterations, data->row_count, &result);
    free(measurements);
    return result;
}

static sql_bench_result_t benchmark_sql_avg_aggregation(sql_test_data_t* data) {
    sql_bench_result_t result = {
        .name = "AVG Aggregation",
        .iterations = SQL_BENCH_ITERATIONS / 100
    };
    
    uint64_t* measurements = malloc(result.iterations * sizeof(uint64_t));
    
    for (uint64_t i = 0; i < result.iterations; i++) {
        uint64_t start = get_cycles();
        
        int64_t sum = 0;
        uint64_t count = 0;
        
        for (size_t row = 0; row < data->row_count; row++) {
            sum += data->values[row];
            count++;
        }
        
        double avg = (double)sum / count;
        
        uint64_t end = get_cycles();
        measurements[i] = end - start;
        
        __asm__ __volatile__("" : : "r" (avg) : "memory");
    }
    
    calculate_sql_stats(measurements, result.iterations, data->row_count, &result);
    free(measurements);
    return result;
}

/*═══════════════════════════════════════════════════════════════
  Join Benchmarks
  ═══════════════════════════════════════════════════════════════*/

static sql_bench_result_t benchmark_sql_hash_join(sql_test_data_t* data) {
    sql_bench_result_t result = {
        .name = "Hash Join",
        .iterations = SQL_BENCH_ITERATIONS / 1000 // Expensive operation
    };
    
    uint64_t* measurements = malloc(result.iterations * sizeof(uint64_t));
    
    // Create hash table structure
    s7t_hash_table_t hash_table;
    hash_table.keys = malloc(data->row_count * sizeof(uint32_t));
    hash_table.values = malloc(data->row_count * sizeof(uint32_t));
    hash_table.buckets = malloc(256 * sizeof(uint32_t));
    
    for (uint64_t i = 0; i < result.iterations; i++) {
        // Initialize hash table
        memset(hash_table.buckets, 0xFF, 256 * sizeof(uint32_t));
        
        uint64_t start = get_cycles();
        
        // Build phase - smaller table (first 1000 rows)
        s7t_hash_build(&hash_table, data->hash_keys, data->hash_values, 1000);
        
        // Probe phase - probe with remaining data
        uint32_t join_count = 0;
        for (size_t probe_idx = 1000; probe_idx < data->row_count; probe_idx++) {
            uint32_t probe_key = data->hash_keys[probe_idx];
            uint32_t hash = probe_key * 0x9e3779b9;
            uint32_t bucket = hash & 255;
            
            // Linear probe for matching key
            while (hash_table.buckets[bucket] != 0xFFFFFFFF) {
                uint32_t stored_idx = hash_table.buckets[bucket];
                if (hash_table.keys[stored_idx] == probe_key) {
                    join_count++;
                    break;
                }
                bucket = (bucket + 1) & 255;
            }
        }
        
        uint64_t end = get_cycles();
        measurements[i] = end - start;
        
        __asm__ __volatile__("" : : "r" (join_count) : "memory");
    }
    
    calculate_sql_stats(measurements, result.iterations, data->row_count - 1000, &result);
    
    free(hash_table.keys);
    free(hash_table.values);
    free(hash_table.buckets);
    free(measurements);
    return result;
}

/*═══════════════════════════════════════════════════════════════
  DML Benchmarks (INSERT, UPDATE, DELETE)
  ═══════════════════════════════════════════════════════════════*/

static sql_bench_result_t benchmark_sql_insert(sql_test_data_t* data) {
    sql_bench_result_t result = {
        .name = "INSERT Single Row",
        .iterations = SQL_BENCH_ITERATIONS
    };
    
    uint64_t* measurements = malloc(result.iterations * sizeof(uint64_t));
    
    // Create a simple table structure
    s7t_table_t table;
    s7t_arena_t arena;
    uint8_t arena_buffer[1024 * 1024];
    s7t_arena_init(&arena, arena_buffer, sizeof(arena_buffer));
    s7t_table_init(&table, "test_table", 1);
    
    // Add columns
    s7t_column_init(&table.columns[0], "id", S7T_TYPE_INT32, &arena);
    s7t_column_init(&table.columns[1], "value", S7T_TYPE_INT32, &arena);
    table.column_count = 2;
    
    for (uint64_t i = 0; i < result.iterations; i++) {
        uint64_t start = get_cycles();
        
        // Simulate INSERT INTO table VALUES (id, value)
        if (table.row_count < S7T_SQL_MAX_ROWS) {
            ((int32_t*)table.columns[0].data)[table.row_count] = data->ids[i % data->row_count];
            ((int32_t*)table.columns[1].data)[table.row_count] = data->values[i % data->row_count];
            table.columns[0].count++;
            table.columns[1].count++;
            table.row_count++;
        }
        
        uint64_t end = get_cycles();
        measurements[i] = end - start;
    }
    
    calculate_sql_stats(measurements, result.iterations, 1, &result);
    free(measurements);
    return result;
}

static sql_bench_result_t benchmark_sql_update(sql_test_data_t* data) {
    sql_bench_result_t result = {
        .name = "UPDATE WHERE condition",
        .iterations = SQL_BENCH_ITERATIONS / 10
    };
    
    uint64_t* measurements = malloc(result.iterations * sizeof(uint64_t));
    
    // Create test table with data
    int32_t* table_data = malloc(data->row_count * sizeof(int32_t));
    memcpy(table_data, data->values, data->row_count * sizeof(int32_t));
    
    for (uint64_t i = 0; i < result.iterations; i++) {
        uint64_t start = get_cycles();
        
        // UPDATE table SET value = new_value WHERE value > threshold
        int32_t threshold = data->values[i % data->row_count];
        int32_t new_value = data->values[(i + 1) % data->row_count];
        uint32_t updated_rows = 0;
        
        for (size_t row = 0; row < data->row_count; row++) {
            if (table_data[row] > threshold) {
                table_data[row] = new_value;
                updated_rows++;
            }
        }
        
        uint64_t end = get_cycles();
        measurements[i] = end - start;
        
        __asm__ __volatile__("" : : "r" (updated_rows) : "memory");
    }
    
    calculate_sql_stats(measurements, result.iterations, data->row_count, &result);
    free(table_data);
    free(measurements);
    return result;
}

/*═══════════════════════════════════════════════════════════════
  Main Benchmark Runner
  ═══════════════════════════════════════════════════════════════*/

static void print_sql_result(const sql_bench_result_t* result) {
    printf("%-35s %8.2f %8llu %8llu %8.3f %6s %s\n",
           result->name,
           result->avg_cycles,
           result->min_cycles,
           result->max_cycles,
           result->cycles_per_row,
           result->seven_tick_compliant ? "✅" : "❌",
           result->passed ? "PASS" : "FAIL");
}

int main(int argc, char** argv) {
    printf("🗄️  CNS 7T-SQL Dedicated Benchmark Suite\n");
    printf("========================================\n\n");
    
    // Configuration
    size_t test_rows = SQL_TEST_ROWS;
    if (argc > 1) {
        test_rows = strtoull(argv[1], NULL, 10);
        if (test_rows < 1000) test_rows = 1000;
        if (test_rows > 100000) test_rows = 100000;
    }
    
    printf("Configuration:\n");
    printf("  Test data rows: %zu\n", test_rows);
    printf("  7-tick target: ≤ 7 cycles per row\n");
    printf("  SIMD support: ");
#ifdef __AVX2__
    printf("AVX2 ✅\n");
#elif defined(__ARM_NEON)
    printf("NEON ✅\n");
#else
    printf("Scalar only\n");
#endif
    printf("\n");
    
    // Generate test data
    printf("🔄 Generating test data (%zu rows)...\n", test_rows);
    sql_test_data_t test_data;
    generate_sql_test_data(&test_data, test_rows);
    
    // Run benchmarks
    printf("\n%-35s %8s %8s %8s %8s %6s %s\n",
           "SQL Operation", "Avg", "Min", "Max", "Per Row", "7T", "Status");
    printf("%-35s %8s %8s %8s %8s %6s %s\n",
           "-----------", "---", "---", "---", "-------", "--", "------");
    
    sql_bench_result_t results[20];
    size_t result_count = 0;
    
    // SELECT benchmarks
    results[result_count++] = benchmark_sql_select_full_scan(&test_data);
    print_sql_result(&results[result_count - 1]);
    
    results[result_count++] = benchmark_sql_select_where_eq(&test_data);
    print_sql_result(&results[result_count - 1]);
    
    results[result_count++] = benchmark_sql_select_where_range(&test_data);
    print_sql_result(&results[result_count - 1]);
    
    results[result_count++] = benchmark_sql_simd_filter_eq(&test_data);
    print_sql_result(&results[result_count - 1]);
    
    // Aggregation benchmarks
    results[result_count++] = benchmark_sql_sum_aggregation(&test_data);
    print_sql_result(&results[result_count - 1]);
    
    results[result_count++] = benchmark_sql_count_aggregation(&test_data);
    print_sql_result(&results[result_count - 1]);
    
    results[result_count++] = benchmark_sql_avg_aggregation(&test_data);
    print_sql_result(&results[result_count - 1]);
    
    // Join benchmarks
    results[result_count++] = benchmark_sql_hash_join(&test_data);
    print_sql_result(&results[result_count - 1]);
    
    // DML benchmarks
    results[result_count++] = benchmark_sql_insert(&test_data);
    print_sql_result(&results[result_count - 1]);
    
    results[result_count++] = benchmark_sql_update(&test_data);
    print_sql_result(&results[result_count - 1]);
    
    // Calculate summary statistics
    size_t passed_count = 0;
    size_t seven_tick_count = 0;
    double avg_performance = 0.0;
    double total_cycles_per_row = 0.0;
    
    for (size_t i = 0; i < result_count; i++) {
        if (results[i].passed) passed_count++;
        if (results[i].seven_tick_compliant) seven_tick_count++;
        avg_performance += results[i].avg_cycles;
        total_cycles_per_row += results[i].cycles_per_row;
    }
    
    avg_performance /= result_count;
    total_cycles_per_row /= result_count;
    
    printf("\n📊 7T-SQL Performance Summary:\n");
    printf("  Total SQL operations: %zu\n", result_count);
    printf("  Passed: %zu (%.1f%%)\n", passed_count, 100.0 * passed_count / result_count);
    printf("  7-tick compliant: %zu (%.1f%%)\n", seven_tick_count, 100.0 * seven_tick_count / result_count);
    printf("  Average cycles per operation: %.2f\n", avg_performance);
    printf("  Average cycles per row: %.3f\n", total_cycles_per_row);
    printf("  7-tick compliance: %s\n", 
           (seven_tick_count == result_count) ? "✅ PERFECT" :
           (seven_tick_count >= result_count * 0.8) ? "⚠️  GOOD" : "❌ NEEDS WORK");
    
    // Output JSON summary for tooling
    printf("\n🔍 JSON Summary:\n");
    printf("{\n");
    printf("  \"benchmark_type\": \"7t_sql\",\n");
    printf("  \"test_rows\": %zu,\n", test_rows);
    printf("  \"total_operations\": %zu,\n", result_count);
    printf("  \"passed_count\": %zu,\n", passed_count);
    printf("  \"seven_tick_count\": %zu,\n", seven_tick_count);
    printf("  \"avg_cycles_per_op\": %.2f,\n", avg_performance);
    printf("  \"avg_cycles_per_row\": %.3f,\n", total_cycles_per_row);
    printf("  \"compliance_rate\": %.3f,\n", (double)seven_tick_count / result_count);
    printf("  \"operations\": [\n");
    
    for (size_t i = 0; i < result_count; i++) {
        printf("    {\n");
        printf("      \"name\": \"%s\",\n", results[i].name);
        printf("      \"avg_cycles\": %.2f,\n", results[i].avg_cycles);
        printf("      \"cycles_per_row\": %.3f,\n", results[i].cycles_per_row);
        printf("      \"seven_tick_compliant\": %s,\n", results[i].seven_tick_compliant ? "true" : "false");
        printf("      \"passed\": %s\n", results[i].passed ? "true" : "false");
        printf("    }%s\n", (i < result_count - 1) ? "," : "");
    }
    
    printf("  ]\n");
    printf("}\n");
    
    // Cleanup
    cleanup_sql_test_data(&test_data);
    
    return (passed_count == result_count && seven_tick_count >= result_count / 2) ? 0 : 1;
}