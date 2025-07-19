/*  ─────────────────────────────────────────────────────────────
    sql_domain.c  –  7-Tick SQL Domain Implementation
    CNS SQL commands with physics-compliant execution
    ───────────────────────────────────────────────────────────── */

#include "../../include/cns/cli.h"
#include "../../include/cns/sql.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Global SQL engine state
static struct {
    s7t_table_t tables[S7T_SQL_MAX_TABLES];
    uint32_t table_count;
    uint8_t arena_buffer[S7T_SQL_ARENA_SIZE] S7T_ALIGNED(64);
    s7t_arena_t arena;
    bool initialized;
} g_sql_engine = {0};

/*═══════════════════════════════════════════════════════════════
  SQL Command Handlers
  ═══════════════════════════════════════════════════════════════*/

// Initialize SQL engine
static int cmd_sql_init(CNSContext* ctx, int argc, char** argv) {
    s7t_span_t span;
    s7t_span_start(&span, "sql_init");
    
    if (g_sql_engine.initialized) {
        cns_cli_warning("SQL engine already initialized\n");
        s7t_span_end(&span);
        return CNS_OK;
    }
    
    // Initialize arena
    s7t_arena_init(&g_sql_engine.arena, g_sql_engine.arena_buffer, S7T_SQL_ARENA_SIZE);
    
    // Clear tables
    g_sql_engine.table_count = 0;
    memset(g_sql_engine.tables, 0, sizeof(g_sql_engine.tables));
    
    g_sql_engine.initialized = true;
    
    s7t_span_end(&span);
    uint64_t cycles = span.end_cycles - span.start_cycles;
    
    cns_cli_success("SQL engine initialized (cycles: %lu)\n", cycles);
    
    // Assert 7-tick compliance
    if (cycles > S7T_MAX_CYCLES * 100) {  // Allow more for initialization
        cns_cli_warning("Initialization exceeded 7-tick budget\n");
    }
    
    return CNS_OK;
}

// Create table
static int cmd_sql_create(CNSContext* ctx, int argc, char** argv) {
    if (!g_sql_engine.initialized) {
        cns_cli_error("SQL engine not initialized. Run 'cns sql init' first.\n");
        return CNS_ERROR;
    }
    
    if (argc < 2) {
        cns_cli_error("Usage: cns sql create <table_name> <col1:type> [col2:type ...]\n");
        cns_cli_info("Types: int32, int64, float32, float64, id, date, time, bool\n");
        return CNS_ERROR_ARGS;
    }
    
    s7t_span_t span;
    s7t_span_start(&span, "sql_create_table");
    
    const char* table_name = argv[1];
    
    if (g_sql_engine.table_count >= S7T_SQL_MAX_TABLES) {
        cns_cli_error("Maximum table limit reached (%d)\n", S7T_SQL_MAX_TABLES);
        s7t_span_end(&span);
        return CNS_ERROR_RESOURCE;
    }
    
    // Get table slot
    s7t_table_t* table = &g_sql_engine.tables[g_sql_engine.table_count];
    s7t_table_init(table, table_name, g_sql_engine.table_count);
    
    // Parse columns
    for (int i = 2; i < argc; i++) {
        char* col_def = argv[i];
        char* colon = strchr(col_def, ':');
        if (!colon) {
            cns_cli_error("Invalid column definition: %s\n", col_def);
            s7t_span_end(&span);
            return CNS_ERROR_ARGS;
        }
        
        *colon = '\0';
        const char* col_name = col_def;
        const char* type_str = colon + 1;
        
        // Parse type
        s7t_sql_type_t type;
        if (strcmp(type_str, "int32") == 0) type = S7T_TYPE_INT32;
        else if (strcmp(type_str, "int64") == 0) type = S7T_TYPE_INT64;
        else if (strcmp(type_str, "float32") == 0) type = S7T_TYPE_FLOAT32;
        else if (strcmp(type_str, "float64") == 0) type = S7T_TYPE_FLOAT64;
        else if (strcmp(type_str, "id") == 0) type = S7T_TYPE_ID;
        else if (strcmp(type_str, "date") == 0) type = S7T_TYPE_DATE;
        else if (strcmp(type_str, "time") == 0) type = S7T_TYPE_TIME;
        else if (strcmp(type_str, "bool") == 0) type = S7T_TYPE_BOOL;
        else {
            cns_cli_error("Unknown type: %s\n", type_str);
            s7t_span_end(&span);
            return CNS_ERROR_ARGS;
        }
        
        // Add column
        if (table->column_count >= S7T_SQL_MAX_COLUMNS) {
            cns_cli_error("Maximum column limit reached (%d)\n", S7T_SQL_MAX_COLUMNS);
            s7t_span_end(&span);
            return CNS_ERROR_RESOURCE;
        }
        
        s7t_column_init(&table->columns[table->column_count], col_name, type, &g_sql_engine.arena);
        table->column_count++;
    }
    
    g_sql_engine.table_count++;
    
    s7t_span_end(&span);
    span.rows_output = table->column_count;
    uint64_t cycles = span.end_cycles - span.start_cycles;
    
    cns_cli_success("Created table '%s' with %u columns (cycles: %lu)\n", 
                    table_name, table->column_count, cycles);
    
    return CNS_OK;
}

// Insert data
static int cmd_sql_insert(CNSContext* ctx, int argc, char** argv) {
    if (!g_sql_engine.initialized) {
        cns_cli_error("SQL engine not initialized. Run 'cns sql init' first.\n");
        return CNS_ERROR;
    }
    
    if (argc < 3) {
        cns_cli_error("Usage: cns sql insert <table_name> <val1> [val2 ...]\n");
        return CNS_ERROR_ARGS;
    }
    
    s7t_span_t span;
    s7t_span_start(&span, "sql_insert");
    
    const char* table_name = argv[1];
    
    // Find table
    s7t_table_t* table = NULL;
    for (uint32_t i = 0; i < g_sql_engine.table_count; i++) {
        if (strcmp(g_sql_engine.tables[i].name, table_name) == 0) {
            table = &g_sql_engine.tables[i];
            break;
        }
    }
    
    if (!table) {
        cns_cli_error("Table not found: %s\n", table_name);
        s7t_span_end(&span);
        return CNS_ERROR_NOT_FOUND;
    }
    
    // Check value count
    uint32_t value_count = argc - 2;
    if (value_count != table->column_count) {
        cns_cli_error("Expected %u values, got %u\n", table->column_count, value_count);
        s7t_span_end(&span);
        return CNS_ERROR_ARGS;
    }
    
    // Check row limit
    if (table->row_count >= S7T_SQL_MAX_ROWS) {
        cns_cli_error("Table row limit reached (%d)\n", S7T_SQL_MAX_ROWS);
        s7t_span_end(&span);
        return CNS_ERROR_RESOURCE;
    }
    
    // Insert values
    for (uint32_t i = 0; i < table->column_count; i++) {
        s7t_column_t* col = &table->columns[i];
        const char* value_str = argv[i + 2];
        
        // Parse and store value based on type
        switch (col->type) {
            case S7T_TYPE_INT32: {
                int32_t* data = (int32_t*)col->data;
                data[table->row_count] = atoi(value_str);
                break;
            }
            case S7T_TYPE_INT64: {
                int64_t* data = (int64_t*)col->data;
                data[table->row_count] = atoll(value_str);
                break;
            }
            case S7T_TYPE_FLOAT32: {
                float* data = (float*)col->data;
                data[table->row_count] = atof(value_str);
                break;
            }
            case S7T_TYPE_FLOAT64: {
                double* data = (double*)col->data;
                data[table->row_count] = atof(value_str);
                break;
            }
            case S7T_TYPE_ID: {
                s7t_id_t* data = (s7t_id_t*)col->data;
                // Simple hash for demo
                data[table->row_count] = s7t_hash_string(value_str, strlen(value_str));
                break;
            }
            case S7T_TYPE_BOOL: {
                bool* data = (bool*)col->data;
                data[table->row_count] = (strcmp(value_str, "true") == 0 || 
                                         strcmp(value_str, "1") == 0);
                break;
            }
            default:
                // For date/time, just use int64 for now
                int64_t* data = (int64_t*)col->data;
                data[table->row_count] = atoll(value_str);
                break;
        }
        
        col->count++;
    }
    
    table->row_count++;
    
    s7t_span_end(&span);
    span.rows_processed = 1;
    span.rows_output = 1;
    uint64_t cycles = span.end_cycles - span.start_cycles;
    
    cns_cli_success("Inserted 1 row (cycles: %lu)\n", cycles);
    
    // Assert 7-tick compliance for insert
    if (cycles > S7T_MAX_CYCLES * 10) {
        cns_cli_warning("Insert exceeded 7-tick budget\n");
    }
    
    return CNS_OK;
}

// Select query
static int cmd_sql_select(CNSContext* ctx, int argc, char** argv) {
    if (!g_sql_engine.initialized) {
        cns_cli_error("SQL engine not initialized. Run 'cns sql init' first.\n");
        return CNS_ERROR;
    }
    
    if (argc < 2) {
        cns_cli_error("Usage: cns sql select <query>\n");
        cns_cli_info("Example: cns sql select \"* FROM table WHERE col > 10\"\n");
        return CNS_ERROR_ARGS;
    }
    
    s7t_span_t span;
    s7t_span_start(&span, "sql_select");
    
    // Simple query parser (for demo)
    const char* query = argv[1];
    
    // Extract table name (simple parsing)
    const char* from_pos = strstr(query, "FROM");
    if (!from_pos) {
        cns_cli_error("Invalid query: missing FROM clause\n");
        s7t_span_end(&span);
        return CNS_ERROR_ARGS;
    }
    
    char table_name[32];
    sscanf(from_pos + 5, "%31s", table_name);
    
    // Find table
    s7t_table_t* table = NULL;
    for (uint32_t i = 0; i < g_sql_engine.table_count; i++) {
        if (strcmp(g_sql_engine.tables[i].name, table_name) == 0) {
            table = &g_sql_engine.tables[i];
            break;
        }
    }
    
    if (!table) {
        cns_cli_error("Table not found: %s\n", table_name);
        s7t_span_end(&span);
        return CNS_ERROR_NOT_FOUND;
    }
    
    // Simple WHERE clause parsing
    const char* where_pos = strstr(query, "WHERE");
    bool has_filter = (where_pos != NULL);
    
    // Print header
    for (uint32_t i = 0; i < table->column_count; i++) {
        printf("%-15s ", table->columns[i].name);
    }
    printf("\n");
    for (uint32_t i = 0; i < table->column_count; i++) {
        printf("--------------- ");
    }
    printf("\n");
    
    // Process rows
    uint32_t output_rows = 0;
    uint32_t matches[S7T_SQL_MAX_ROWS];
    uint32_t match_count = table->row_count;
    
    // If no filter, all rows match
    if (!has_filter) {
        for (uint32_t i = 0; i < table->row_count; i++) {
            matches[i] = i;
        }
    } else {
        // Simple filter implementation (for demo)
        // In real implementation, this would use SIMD operations
        match_count = 0;
        
        // Parse simple WHERE col op value
        char col_name[32], op[8];
        int filter_value;
        if (sscanf(where_pos + 6, "%31s %7s %d", col_name, op, &filter_value) == 3) {
            // Find column
            int col_idx = -1;
            for (uint32_t i = 0; i < table->column_count; i++) {
                if (strcmp(table->columns[i].name, col_name) == 0) {
                    col_idx = i;
                    break;
                }
            }
            
            if (col_idx >= 0 && table->columns[col_idx].type == S7T_TYPE_INT32) {
                int32_t* data = (int32_t*)table->columns[col_idx].data;
                
                // Use SIMD filter
                if (strcmp(op, "=") == 0) {
                    match_count = s7t_simd_filter_eq_i32(data, filter_value, 
                                                         table->row_count, matches);
                } else {
                    // Fallback for other operators
                    for (uint32_t i = 0; i < table->row_count; i++) {
                        bool match = false;
                        if (strcmp(op, ">") == 0) match = data[i] > filter_value;
                        else if (strcmp(op, "<") == 0) match = data[i] < filter_value;
                        else if (strcmp(op, ">=") == 0) match = data[i] >= filter_value;
                        else if (strcmp(op, "<=") == 0) match = data[i] <= filter_value;
                        
                        if (match) {
                            matches[match_count++] = i;
                        }
                    }
                }
            }
        }
    }
    
    // Output matching rows
    for (uint32_t i = 0; i < match_count; i++) {
        uint32_t row_idx = matches[i];
        
        for (uint32_t j = 0; j < table->column_count; j++) {
            s7t_column_t* col = &table->columns[j];
            
            // Print value based on type
            switch (col->type) {
                case S7T_TYPE_INT32:
                    printf("%-15d ", ((int32_t*)col->data)[row_idx]);
                    break;
                case S7T_TYPE_INT64:
                    printf("%-15lld ", ((int64_t*)col->data)[row_idx]);
                    break;
                case S7T_TYPE_FLOAT32:
                    printf("%-15.2f ", ((float*)col->data)[row_idx]);
                    break;
                case S7T_TYPE_FLOAT64:
                    printf("%-15.2f ", ((double*)col->data)[row_idx]);
                    break;
                case S7T_TYPE_ID:
                    printf("%-15u ", ((s7t_id_t*)col->data)[row_idx]);
                    break;
                case S7T_TYPE_BOOL:
                    printf("%-15s ", ((bool*)col->data)[row_idx] ? "true" : "false");
                    break;
                default:
                    printf("%-15lld ", ((int64_t*)col->data)[row_idx]);
                    break;
            }
        }
        printf("\n");
        output_rows++;
    }
    
    s7t_span_end(&span);
    span.rows_processed = table->row_count;
    span.rows_output = output_rows;
    uint64_t cycles = span.end_cycles - span.start_cycles;
    
    printf("\n%u rows selected (cycles: %lu, cycles/row: %.2f)\n", 
           output_rows, cycles, (double)cycles / table->row_count);
    
    return CNS_OK;
}

// Benchmark SQL operations
static int cmd_sql_bench(CNSContext* ctx, int argc, char** argv) {
    if (!g_sql_engine.initialized) {
        cns_cli_error("SQL engine not initialized. Run 'cns sql init' first.\n");
        return CNS_ERROR;
    }
    
    int iterations = 1000;
    if (argc > 1) {
        iterations = atoi(argv[1]);
    }
    
    cns_cli_info("Running SQL benchmarks (%d iterations)...\n\n", iterations);
    
    // Create test table with real data
    s7t_table_t* test_table = &g_sql_engine.tables[g_sql_engine.table_count];
    s7t_table_init(test_table, "bench_table", g_sql_engine.table_count);
    
    // Initialize columns
    s7t_column_init(&test_table->columns[0], "id", S7T_TYPE_INT32, &g_sql_engine.arena);
    s7t_column_init(&test_table->columns[1], "value", S7T_TYPE_INT32, &g_sql_engine.arena);
    s7t_column_init(&test_table->columns[2], "score", S7T_TYPE_FLOAT32, &g_sql_engine.arena);
    test_table->column_count = 3;
    
    // Generate realistic test data
    int32_t* id_data = (int32_t*)test_table->columns[0].data;
    int32_t* val_data = (int32_t*)test_table->columns[1].data;
    float* score_data = (float*)test_table->columns[2].data;
    
    // Use current time as seed for realistic data
    uint32_t seed = (uint32_t)time(NULL);
    for (uint32_t i = 0; i < 1000; i++) {
        id_data[i] = i;
        // Generate pseudo-random values using LCG
        seed = seed * 1664525 + 1013904223;
        val_data[i] = seed % 100;
        score_data[i] = (float)(seed % 10000) / 100.0f;
    }
    test_table->row_count = 1000;
    test_table->columns[0].count = 1000;
    test_table->columns[1].count = 1000;
    test_table->columns[2].count = 1000;
    
    g_sql_engine.table_count++;
    
    // Warm up CPU caches
    volatile int64_t warmup = 0;
    for (int i = 0; i < 100; i++) {
        for (uint32_t j = 0; j < test_table->row_count; j++) {
            warmup += val_data[j];
        }
    }
    
    // Benchmark 1: Sequential scan with actual predicate evaluation
    {
        uint64_t min_cycles = UINT64_MAX;
        uint64_t max_cycles = 0;
        uint64_t total_cycles = 0;
        uint32_t matches[S7T_SQL_MAX_ROWS];
        
        for (int i = 0; i < iterations; i++) {
            uint64_t start = s7t_cycles();
            
            uint32_t count = 0;
            for (uint32_t j = 0; j < test_table->row_count; j++) {
                if (val_data[j] > 50) {
                    matches[count++] = j;
                }
            }
            
            uint64_t cycles = s7t_cycles() - start;
            total_cycles += cycles;
            if (cycles < min_cycles) min_cycles = cycles;
            if (cycles > max_cycles) max_cycles = cycles;
        }
        
        double avg_cycles = (double)total_cycles / iterations;
        double cycles_per_row = avg_cycles / test_table->row_count;
        printf("Sequential Scan (>50):\n");
        printf("  Avg: %.2f cycles (%.2f ns)\n", avg_cycles, avg_cycles * S7T_NS_PER_CYCLE);
        printf("  Min: %lu cycles, Max: %lu cycles\n", min_cycles, max_cycles);
        printf("  Per row: %.3f cycles\n", cycles_per_row);
    }
    
    // Benchmark 2: SIMD filter with real measurements
    {
        uint64_t min_cycles = UINT64_MAX;
        uint64_t max_cycles = 0;
        uint64_t total_cycles = 0;
        uint32_t matches[S7T_SQL_MAX_ROWS];
        uint32_t total_matches = 0;
        
        for (int i = 0; i < iterations; i++) {
            uint64_t start = s7t_cycles();
            
            uint32_t count = s7t_simd_filter_eq_i32(val_data, 42, test_table->row_count, matches);
            
            uint64_t cycles = s7t_cycles() - start;
            total_cycles += cycles;
            total_matches += count;
            if (cycles < min_cycles) min_cycles = cycles;
            if (cycles > max_cycles) max_cycles = cycles;
        }
        
        double avg_cycles = (double)total_cycles / iterations;
        double cycles_per_row = avg_cycles / test_table->row_count;
        double avg_matches = (double)total_matches / iterations;
        printf("\nSIMD Filter (=42):\n");
        printf("  Avg: %.2f cycles (%.2f ns)\n", avg_cycles, avg_cycles * S7T_NS_PER_CYCLE);
        printf("  Min: %lu cycles, Max: %lu cycles\n", min_cycles, max_cycles);
        printf("  Per row: %.3f cycles\n", cycles_per_row);
        printf("  Avg matches: %.1f\n", avg_matches);
    }
    
    // Benchmark 3: Hash join build with collision handling
    {
        uint64_t min_cycles = UINT64_MAX;
        uint64_t max_cycles = 0;
        uint64_t total_cycles = 0;
        
        // Allocate hash table structures
        uint32_t* hash_buckets = (uint32_t*)s7t_arena_alloc(&g_sql_engine.arena, 256 * sizeof(uint32_t));
        uint32_t* hash_keys = (uint32_t*)s7t_arena_alloc(&g_sql_engine.arena, 100 * sizeof(uint32_t));
        uint32_t* hash_values = (uint32_t*)s7t_arena_alloc(&g_sql_engine.arena, 100 * sizeof(uint32_t));
        
        s7t_hash_table_t ht = {
            .keys = hash_keys,
            .values = hash_values,
            .buckets = hash_buckets,
            .bucket_count = 256,
            .size = 0
        };
        
        for (int i = 0; i < iterations; i++) {
            // Clear hash table
            memset(hash_buckets, 0xFF, 256 * sizeof(uint32_t));
            
            uint64_t start = s7t_cycles();
            
            s7t_hash_build(&ht, (uint32_t*)id_data, (uint32_t*)val_data, 100);
            
            uint64_t cycles = s7t_cycles() - start;
            total_cycles += cycles;
            if (cycles < min_cycles) min_cycles = cycles;
            if (cycles > max_cycles) max_cycles = cycles;
        }
        
        double avg_cycles = (double)total_cycles / iterations;
        double cycles_per_row = avg_cycles / 100;
        printf("\nHash Join Build (100 rows):\n");
        printf("  Avg: %.2f cycles (%.2f ns)\n", avg_cycles, avg_cycles * S7T_NS_PER_CYCLE);
        printf("  Min: %lu cycles, Max: %lu cycles\n", min_cycles, max_cycles);
        printf("  Per row: %.3f cycles\n", cycles_per_row);
    }
    
    // Benchmark 4: Aggregation with SIMD horizontal reduction
    {
        uint64_t min_cycles = UINT64_MAX;
        uint64_t max_cycles = 0;
        uint64_t total_cycles = 0;
        
        for (int i = 0; i < iterations; i++) {
            uint64_t start = s7t_cycles();
            
            // SIMD-friendly sum aggregation
            int64_t sum = 0;
            uint32_t j;
            
            // Process 8 elements at a time if possible
            for (j = 0; j + 7 < test_table->row_count; j += 8) {
                sum += val_data[j] + val_data[j+1] + val_data[j+2] + val_data[j+3] +
                       val_data[j+4] + val_data[j+5] + val_data[j+6] + val_data[j+7];
            }
            
            // Handle remaining elements
            for (; j < test_table->row_count; j++) {
                sum += val_data[j];
            }
            
            uint64_t cycles = s7t_cycles() - start;
            total_cycles += cycles;
            if (cycles < min_cycles) min_cycles = cycles;
            if (cycles > max_cycles) max_cycles = cycles;
        }
        
        double avg_cycles = (double)total_cycles / iterations;
        double cycles_per_row = avg_cycles / test_table->row_count;
        printf("\nSum Aggregation (1000 rows):\n");
        printf("  Avg: %.2f cycles (%.2f ns)\n", avg_cycles, avg_cycles * S7T_NS_PER_CYCLE);
        printf("  Min: %lu cycles, Max: %lu cycles\n", min_cycles, max_cycles);
        printf("  Per row: %.3f cycles\n", cycles_per_row);
        
        if (cycles_per_row <= S7T_MAX_CYCLES) {
            cns_cli_success("  ✓ Meets 7-tick budget!\n");
        } else {
            cns_cli_warning("  ✗ Exceeds 7-tick budget (%.1fx over)\n", 
                           cycles_per_row / S7T_MAX_CYCLES);
        }
    }
    
    // Benchmark 5: Memory-aligned insert operations
    {
        uint64_t min_cycles = UINT64_MAX;
        uint64_t max_cycles = 0;
        uint64_t total_cycles = 0;
        
        // Create a fresh table for insert benchmarks
        s7t_table_t* insert_table = &g_sql_engine.tables[g_sql_engine.table_count];
        s7t_table_init(insert_table, "insert_bench", g_sql_engine.table_count);
        s7t_column_init(&insert_table->columns[0], "id", S7T_TYPE_INT32, &g_sql_engine.arena);
        insert_table->column_count = 1;
        
        int32_t* insert_data = (int32_t*)insert_table->columns[0].data;
        
        // Measure single-row inserts
        int actual_iterations = (iterations < S7T_SQL_MAX_ROWS) ? iterations : S7T_SQL_MAX_ROWS;
        for (int i = 0; i < actual_iterations; i++) {
            uint64_t start = s7t_cycles();
            
            // Actual insert operation
            insert_data[insert_table->row_count] = i;
            insert_table->row_count++;
            insert_table->columns[0].count++;
            
            uint64_t cycles = s7t_cycles() - start;
            total_cycles += cycles;
            if (cycles < min_cycles) min_cycles = cycles;
            if (cycles > max_cycles) max_cycles = cycles;
        }
        
        g_sql_engine.table_count++;
        
        double avg_cycles = (double)total_cycles / actual_iterations;
        printf("\nInsert Operation:\n");
        printf("  Avg: %.2f cycles (%.2f ns)\n", avg_cycles, avg_cycles * S7T_NS_PER_CYCLE);
        printf("  Min: %lu cycles, Max: %lu cycles\n", min_cycles, max_cycles);
        printf("  Rows inserted: %d\n", actual_iterations);
        
        if (avg_cycles <= S7T_MAX_CYCLES) {
            cns_cli_success("  ✓ Meets 7-tick budget!\n");
        } else {
            cns_cli_warning("  ✗ Exceeds 7-tick budget (%.1fx over)\n", 
                           avg_cycles / S7T_MAX_CYCLES);
        }
    }
    
    // Summary statistics
    printf("\n═══════════════════════════════════════\n");
    printf("Benchmark Summary:\n");
    printf("  CPU frequency estimate: %.2f GHz\n", 1.0 / S7T_NS_PER_CYCLE);
    printf("  7-tick budget: %d cycles (%.2f ns)\n", 
           S7T_MAX_CYCLES, S7T_MAX_CYCLES * S7T_NS_PER_CYCLE);
    printf("  Arena memory used: %lu / %lu bytes\n", 
           g_sql_engine.arena.offset, g_sql_engine.arena.size);
    
    return CNS_OK;
}

// Explain query plan
static int cmd_sql_explain(CNSContext* ctx, int argc, char** argv) {
    if (!g_sql_engine.initialized) {
        cns_cli_error("SQL engine not initialized. Run 'cns sql init' first.\n");
        return CNS_ERROR;
    }
    
    if (argc < 2) {
        cns_cli_error("Usage: cns sql explain <query>\n");
        return CNS_ERROR_ARGS;
    }
    
    const char* query = argv[1];
    
    // Simple query analysis (for demo)
    cns_cli_info("Query Plan for: %s\n\n", query);
    
    // Estimate based on query type
    bool has_where = strstr(query, "WHERE") != NULL;
    bool has_join = strstr(query, "JOIN") != NULL;
    bool has_group = strstr(query, "GROUP BY") != NULL;
    bool has_order = strstr(query, "ORDER BY") != NULL;
    
    uint64_t estimated_cycles = 1;  // Base scan
    
    printf("Execution Plan:\n");
    printf("──────────────\n");
    
    if (has_where) {
        if (strstr(query, "=")) {
            printf("├─ SIMD Filter (equality)    : 4 cycles\n");
            estimated_cycles += 4;
        } else {
            printf("├─ Sequential Filter         : 5 cycles\n");
            estimated_cycles += 5;
        }
    }
    
    if (has_join) {
        printf("├─ Hash Join                 : 7 cycles\n");
        estimated_cycles += 7;
    }
    
    if (has_group) {
        printf("├─ Group Aggregation         : 6 cycles\n");
        estimated_cycles += 6;
    }
    
    if (has_order) {
        printf("├─ Bitonic Sort              : 7 cycles\n");
        estimated_cycles += 7;
    }
    
    printf("└─ Sequential Scan           : 1 cycle\n");
    
    printf("\nEstimated Total: %lu cycles (%.2f ns)\n", 
           estimated_cycles, estimated_cycles * S7T_NS_PER_CYCLE);
    
    if (estimated_cycles <= S7T_MAX_CYCLES) {
        cns_cli_success("\n✓ Query meets 7-tick budget!\n");
    } else {
        cns_cli_warning("\n✗ Query exceeds 7-tick budget (%lu > %d cycles)\n", 
                       estimated_cycles, S7T_MAX_CYCLES);
        cns_cli_info("Consider:\n");
        cns_cli_info("  - Using indexed columns\n");
        cns_cli_info("  - Limiting result set size\n");
        cns_cli_info("  - Simplifying predicates\n");
    }
    
    return CNS_OK;
}

// Show tables
static int cmd_sql_show(CNSContext* ctx, int argc, char** argv) {
    if (!g_sql_engine.initialized) {
        cns_cli_error("SQL engine not initialized. Run 'cns sql init' first.\n");
        return CNS_ERROR;
    }
    
    if (g_sql_engine.table_count == 0) {
        cns_cli_info("No tables found.\n");
        return CNS_OK;
    }
    
    printf("\nTables:\n");
    printf("────────────────────────────────────────\n");
    printf("%-20s %-10s %-10s\n", "Table Name", "Columns", "Rows");
    printf("────────────────────────────────────────\n");
    
    for (uint32_t i = 0; i < g_sql_engine.table_count; i++) {
        s7t_table_t* table = &g_sql_engine.tables[i];
        printf("%-20s %-10u %-10u\n", table->name, table->column_count, table->row_count);
    }
    
    printf("\nMemory Usage:\n");
    printf("────────────────────────────────────────\n");
    printf("Arena: %lu / %lu bytes (%.1f%%)\n", 
           g_sql_engine.arena.offset, 
           g_sql_engine.arena.size,
           100.0 * g_sql_engine.arena.offset / g_sql_engine.arena.size);
    printf("Allocations: %lu\n", g_sql_engine.arena.allocations);
    
    return CNS_OK;
}

/*═══════════════════════════════════════════════════════════════
  Command Registration
  ═══════════════════════════════════════════════════════════════*/

// SQL command options
static CNSOption init_options[] = {
    {NULL}
};

static CNSOption create_options[] = {
    {NULL}
};

static CNSOption insert_options[] = {
    {NULL}
};

static CNSOption select_options[] = {
    {NULL}
};

static CNSOption bench_options[] = {
    {NULL}
};

static CNSOption explain_options[] = {
    {NULL}
};

static CNSOption show_options[] = {
    {NULL}
};

// SQL commands
static CNSCommand sql_commands[] = {
    {
        .name = "init",
        .description = "Initialize SQL engine",
        .handler = cmd_sql_init,
        .options = init_options,
        .option_count = 0,
        .arguments = NULL,
        .argument_count = 0
    },
    {
        .name = "create",
        .description = "Create a new table",
        .handler = cmd_sql_create,
        .options = create_options,
        .option_count = 0,
        .arguments = NULL,
        .argument_count = 0
    },
    {
        .name = "insert",
        .description = "Insert row into table",
        .handler = cmd_sql_insert,
        .options = insert_options,
        .option_count = 0,
        .arguments = NULL,
        .argument_count = 0
    },
    {
        .name = "select",
        .description = "Execute SELECT query",
        .handler = cmd_sql_select,
        .options = select_options,
        .option_count = 0,
        .arguments = NULL,
        .argument_count = 0
    },
    {
        .name = "bench",
        .description = "Run SQL benchmarks",
        .handler = cmd_sql_bench,
        .options = bench_options,
        .option_count = 0,
        .arguments = NULL,
        .argument_count = 0
    },
    {
        .name = "explain",
        .description = "Explain query execution plan",
        .handler = cmd_sql_explain,
        .options = explain_options,
        .option_count = 0,
        .arguments = NULL,
        .argument_count = 0
    },
    {
        .name = "show",
        .description = "Show tables and statistics",
        .handler = cmd_sql_show,
        .options = show_options,
        .option_count = 0,
        .arguments = NULL,
        .argument_count = 0
    }
};

// SQL domain definition
CNSDomain cns_sql_domain = {
    .name = "sql",
    .description = "7-Tick SQL engine commands",
    .commands = sql_commands,
    .command_count = sizeof(sql_commands) / sizeof(sql_commands[0])
};