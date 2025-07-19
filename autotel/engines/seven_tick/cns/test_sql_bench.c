/*  ─────────────────────────────────────────────────────────────
    test_sql_bench.c  –  Standalone 7T-SQL Benchmark Test
    Demonstrates real benchmark results
    ───────────────────────────────────────────────────────────── */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "include/cns/sql.h"

// Simple benchmark driver
int main(int argc, char** argv) {
    printf("7T-SQL Benchmark Test\n");
    printf("====================\n\n");
    
    // Initialize arena
    uint8_t arena_buffer[S7T_SQL_ARENA_SIZE] __attribute__((aligned(64)));
    s7t_arena_t arena;
    s7t_arena_init(&arena, arena_buffer, S7T_SQL_ARENA_SIZE);
    
    // Create test table
    s7t_table_t table;
    s7t_table_init(&table, "benchmark", 1);
    
    // Add columns
    s7t_column_init(&table.columns[0], "id", S7T_TYPE_INT32, &arena);
    s7t_column_init(&table.columns[1], "value", S7T_TYPE_INT32, &arena);
    s7t_column_init(&table.columns[2], "score", S7T_TYPE_FLOAT32, &arena);
    table.column_count = 3;
    
    // Generate test data
    int32_t* id_data = (int32_t*)table.columns[0].data;
    int32_t* val_data = (int32_t*)table.columns[1].data;
    float* score_data = (float*)table.columns[2].data;
    
    uint32_t seed = (uint32_t)time(NULL);
    for (uint32_t i = 0; i < 1000; i++) {
        id_data[i] = i;
        seed = seed * 1664525 + 1013904223;
        val_data[i] = seed % 100;
        score_data[i] = (float)(seed % 10000) / 100.0f;
    }
    table.row_count = 1000;
    for (int i = 0; i < 3; i++) {
        table.columns[i].count = 1000;
    }
    
    int iterations = (argc > 1) ? atoi(argv[1]) : 10000;
    printf("Running benchmarks with %d iterations...\n\n", iterations);
    
    // Warm up caches
    volatile int64_t warmup = 0;
    for (int i = 0; i < 100; i++) {
        for (uint32_t j = 0; j < table.row_count; j++) {
            warmup += val_data[j];
        }
    }
    
    // Benchmark 1: Sequential scan
    {
        uint64_t min_cycles = UINT64_MAX;
        uint64_t max_cycles = 0;
        uint64_t total_cycles = 0;
        uint32_t matches[S7T_SQL_MAX_ROWS];
        
        for (int i = 0; i < iterations; i++) {
            uint64_t start = s7t_cycles();
            
            uint32_t count = 0;
            for (uint32_t j = 0; j < table.row_count; j++) {
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
        double cycles_per_row = avg_cycles / table.row_count;
        printf("Sequential Scan (>50):\n");
        printf("  Avg: %.2f cycles (%.2f ns)\n", avg_cycles, avg_cycles * S7T_NS_PER_CYCLE);
        printf("  Min: %lu cycles, Max: %lu cycles\n", min_cycles, max_cycles);
        printf("  Per row: %.3f cycles\n", cycles_per_row);
        printf("  7-tick compliance: %s (%.1fx)\n\n", 
               cycles_per_row <= S7T_MAX_CYCLES ? "PASS" : "FAIL",
               cycles_per_row / S7T_MAX_CYCLES);
    }
    
    // Benchmark 2: SIMD filter
    {
        uint64_t min_cycles = UINT64_MAX;
        uint64_t max_cycles = 0;
        uint64_t total_cycles = 0;
        uint32_t matches[S7T_SQL_MAX_ROWS];
        uint32_t total_matches = 0;
        
        for (int i = 0; i < iterations; i++) {
            uint64_t start = s7t_cycles();
            
            uint32_t count = s7t_simd_filter_eq_i32(val_data, 42, table.row_count, matches);
            
            uint64_t cycles = s7t_cycles() - start;
            total_cycles += cycles;
            total_matches += count;
            if (cycles < min_cycles) min_cycles = cycles;
            if (cycles > max_cycles) max_cycles = cycles;
        }
        
        double avg_cycles = (double)total_cycles / iterations;
        double cycles_per_row = avg_cycles / table.row_count;
        double avg_matches = (double)total_matches / iterations;
        printf("SIMD Filter (=42):\n");
        printf("  Avg: %.2f cycles (%.2f ns)\n", avg_cycles, avg_cycles * S7T_NS_PER_CYCLE);
        printf("  Min: %lu cycles, Max: %lu cycles\n", min_cycles, max_cycles);
        printf("  Per row: %.3f cycles\n", cycles_per_row);
        printf("  Avg matches: %.1f\n", avg_matches);
        printf("  7-tick compliance: %s (%.1fx)\n\n", 
               cycles_per_row <= S7T_MAX_CYCLES ? "PASS" : "FAIL",
               cycles_per_row / S7T_MAX_CYCLES);
    }
    
    // Benchmark 3: Aggregation
    {
        uint64_t min_cycles = UINT64_MAX;
        uint64_t max_cycles = 0;
        uint64_t total_cycles = 0;
        
        for (int i = 0; i < iterations; i++) {
            uint64_t start = s7t_cycles();
            
            int64_t sum = 0;
            uint32_t j;
            
            // Process 8 elements at a time
            for (j = 0; j + 7 < table.row_count; j += 8) {
                sum += val_data[j] + val_data[j+1] + val_data[j+2] + val_data[j+3] +
                       val_data[j+4] + val_data[j+5] + val_data[j+6] + val_data[j+7];
            }
            
            // Handle remaining
            for (; j < table.row_count; j++) {
                sum += val_data[j];
            }
            
            uint64_t cycles = s7t_cycles() - start;
            total_cycles += cycles;
            if (cycles < min_cycles) min_cycles = cycles;
            if (cycles > max_cycles) max_cycles = cycles;
        }
        
        double avg_cycles = (double)total_cycles / iterations;
        double cycles_per_row = avg_cycles / table.row_count;
        printf("Sum Aggregation (1000 rows):\n");
        printf("  Avg: %.2f cycles (%.2f ns)\n", avg_cycles, avg_cycles * S7T_NS_PER_CYCLE);
        printf("  Min: %lu cycles, Max: %lu cycles\n", min_cycles, max_cycles);
        printf("  Per row: %.3f cycles\n", cycles_per_row);
        printf("  7-tick compliance: %s (%.1fx)\n\n", 
               cycles_per_row <= S7T_MAX_CYCLES ? "PASS" : "FAIL",
               cycles_per_row / S7T_MAX_CYCLES);
    }
    
    // Benchmark 4: Insert operation
    {
        uint64_t min_cycles = UINT64_MAX;
        uint64_t max_cycles = 0;
        uint64_t total_cycles = 0;
        
        // Create fresh table for inserts
        s7t_table_t insert_table;
        s7t_table_init(&insert_table, "insert_test", 2);
        s7t_column_init(&insert_table.columns[0], "id", S7T_TYPE_INT32, &arena);
        insert_table.column_count = 1;
        
        int32_t* insert_data = (int32_t*)insert_table.columns[0].data;
        
        int insert_count = (iterations < 1000) ? iterations : 1000;
        for (int i = 0; i < insert_count; i++) {
            uint64_t start = s7t_cycles();
            
            // Single row insert
            insert_data[insert_table.row_count] = i;
            insert_table.row_count++;
            insert_table.columns[0].count++;
            
            uint64_t cycles = s7t_cycles() - start;
            total_cycles += cycles;
            if (cycles < min_cycles) min_cycles = cycles;
            if (cycles > max_cycles) max_cycles = cycles;
        }
        
        double avg_cycles = (double)total_cycles / insert_count;
        printf("Insert Operation:\n");
        printf("  Avg: %.2f cycles (%.2f ns)\n", avg_cycles, avg_cycles * S7T_NS_PER_CYCLE);
        printf("  Min: %lu cycles, Max: %lu cycles\n", min_cycles, max_cycles);
        printf("  7-tick compliance: %s (%.1fx)\n\n", 
               avg_cycles <= S7T_MAX_CYCLES ? "PASS" : "FAIL",
               avg_cycles / S7T_MAX_CYCLES);
    }
    
    // Summary
    printf("=====================================\n");
    printf("Summary:\n");
    printf("  CPU frequency estimate: %.2f GHz\n", 1.0 / S7T_NS_PER_CYCLE);
    printf("  7-tick budget: %d cycles (%.2f ns)\n", S7T_MAX_CYCLES, S7T_MAX_CYCLES * S7T_NS_PER_CYCLE);
    printf("  Arena memory used: %lu bytes\n", arena.used);
    
    return 0;
}