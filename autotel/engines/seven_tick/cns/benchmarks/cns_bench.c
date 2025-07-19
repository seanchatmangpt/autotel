/*  ─────────────────────────────────────────────────────────────
    cns_bench.c  –  CNS Comprehensive Benchmarks
    Performance validation and regression testing
    ───────────────────────────────────────────────────────────── */

#include "cns/core/engine.h"
#include "cns/core/registry.h"
#include "cns/core/memory.h"
#include "cns/commands/base.h"
#include "cns/bench/framework.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*═══════════════════════════════════════════════════════════════
  Benchmark Context
  ═══════════════════════════════════════════════════════════════*/

typedef struct {
    cns_engine_t* engine;
    cns_registry_t* registry;
    cns_memory_arena_t* arena;
    char** test_commands;
    size_t command_count;
} bench_context_t;

/*═══════════════════════════════════════════════════════════════
  Parser Benchmarks
  ═══════════════════════════════════════════════════════════════*/

// Simple command parsing
void bench_parse_simple(void* context) {
    bench_context_t* ctx = (bench_context_t*)context;
    cns_command_t cmd;
    
    const char* input = "echo hello";
    cns_parse(input, &cmd);
}

// Complex command parsing
void bench_parse_complex(void* context) {
    bench_context_t* ctx = (bench_context_t*)context;
    cns_command_t cmd;
    
    const char* input = "build --target=release --jobs=8 --features=\"opt1,opt2\" src/main.c";
    cns_parse(input, &cmd);
}

// Quoted string parsing
void bench_parse_quoted(void* context) {
    bench_context_t* ctx = (bench_context_t*)context;
    cns_command_t cmd;
    
    const char* input = "exec \"command with spaces\" \"another argument\" --flag";
    cns_parse(input, &cmd);
}

/*═══════════════════════════════════════════════════════════════
  Registry Benchmarks
  ═══════════════════════════════════════════════════════════════*/

// Command lookup
void bench_registry_lookup(void* context) {
    bench_context_t* ctx = (bench_context_t*)context;
    
    uint32_t hash = s7t_hash_string("build", 5);
    cns_registry_entry_t* entry = cns_registry_lookup_hash(ctx->registry, hash);
}

// Failed lookup (worst case)
void bench_registry_miss(void* context) {
    bench_context_t* ctx = (bench_context_t*)context;
    
    uint32_t hash = s7t_hash_string("nonexistent", 11);
    cns_registry_entry_t* entry = cns_registry_lookup_hash(ctx->registry, hash);
}

// Prefix search
void bench_registry_prefix(void* context) {
    bench_context_t* ctx = (bench_context_t*)context;
    cns_registry_entry_t* results[10];
    
    size_t count = cns_registry_find_prefix(ctx->registry, "test", results, 10);
}

/*═══════════════════════════════════════════════════════════════
  Memory Benchmarks
  ═══════════════════════════════════════════════════════════════*/

// Small allocation
void bench_arena_alloc_small(void* context) {
    bench_context_t* ctx = (bench_context_t*)context;
    
    void* ptr = cns_arena_alloc(ctx->arena, 64);
}

// Large allocation
void bench_arena_alloc_large(void* context) {
    bench_context_t* ctx = (bench_context_t*)context;
    
    void* ptr = cns_arena_alloc(ctx->arena, 4096);
}

// String duplication
void bench_arena_strdup(void* context) {
    bench_context_t* ctx = (bench_context_t*)context;
    
    char* str = cns_arena_strdup(ctx->arena, "benchmark test string");
}

/*═══════════════════════════════════════════════════════════════
  End-to-End Benchmarks
  ═══════════════════════════════════════════════════════════════*/

// Full command execution
void bench_execute_command(void* context) {
    bench_context_t* ctx = (bench_context_t*)context;
    
    cns_command_t cmd = {
        .name = "echo",
        .hash = s7t_hash_string("echo", 4),
        .args = (char*[]){"hello", "world"},
        .argc = 2,
    };
    
    cns_context_t exec_ctx = {
        .arena = ctx->arena,
    };
    
    cns_engine_execute(ctx->engine, &cmd, &exec_ctx);
}

// Batch command execution
void bench_execute_batch(void* context) {
    bench_context_t* ctx = (bench_context_t*)context;
    
    const char* commands[] = {
        "echo test1",
        "echo test2",
        "echo test3",
    };
    
    cns_context_t exec_ctx = {
        .arena = ctx->arena,
    };
    
    cns_engine_execute_batch(ctx->engine, commands, 3, &exec_ctx);
}

/*═══════════════════════════════════════════════════════════════
  7-Tick Critical Path Benchmarks
  ═══════════════════════════════════════════════════════════════*/

// Critical path: parse + lookup + dispatch
void bench_critical_path(void* context) {
    bench_context_t* ctx = (bench_context_t*)context;
    
    // This entire sequence must complete in 7 ticks
    cns_command_t cmd;
    const char* input = "help";
    
    // Tick 1-2: Parse
    cns_parse_simple(input, &cmd);
    
    // Tick 3-4: Lookup
    cns_registry_entry_t* entry = cns_registry_lookup_hash(
        ctx->registry, cmd.hash);
    
    // Tick 5-6: Validate
    if (entry && cmd.argc >= entry->command->min_args) {
        // Tick 7: Dispatch (just setup, not execution)
        cns_context_t exec_ctx = {
            .arena = ctx->arena,
            .start_cycles = s7t_cycles(),
        };
    }
}

/*═══════════════════════════════════════════════════════════════
  Benchmark Setup
  ═══════════════════════════════════════════════════════════════*/

void* bench_setup(void) {
    bench_context_t* ctx = calloc(1, sizeof(bench_context_t));
    
    // Initialize engine
    cns_engine_config_t config = CNS_DEFAULT_ENGINE_CONFIG;
    ctx->engine = calloc(1, sizeof(cns_engine_t));
    cns_engine_init(ctx->engine, &config);
    
    // Initialize registry
    cns_registry_config_t reg_config = CNS_DEFAULT_REGISTRY_CONFIG;
    ctx->registry = calloc(1, sizeof(cns_registry_t));
    cns_registry_init(ctx->registry, &reg_config);
    
    // Initialize memory arena
    void* arena_mem = malloc(65536);
    ctx->arena = calloc(1, sizeof(cns_memory_arena_t));
    cns_arena_init(ctx->arena, arena_mem, 65536, CNS_ARENA_FLAG_NONE);
    
    // Register test commands
    static cns_command_def_t test_commands[] = {
        CNS_COMMAND_DEF("echo", NULL, 0, 10, "Echo command"),
        CNS_COMMAND_DEF("help", NULL, 0, 1, "Help command"),
        CNS_COMMAND_DEF("build", NULL, 0, 5, "Build command"),
        CNS_COMMAND_DEF("test", NULL, 0, 3, "Test command"),
        CNS_COMMAND_DEF("bench", NULL, 0, 2, "Bench command"),
    };
    
    cns_registry_register_batch(ctx->registry, test_commands, 5, NULL);
    
    return ctx;
}

void bench_teardown(void* context) {
    bench_context_t* ctx = (bench_context_t*)context;
    
    if (ctx) {
        if (ctx->engine) {
            cns_engine_shutdown(ctx->engine);
            free(ctx->engine);
        }
        if (ctx->registry) {
            cns_registry_cleanup(ctx->registry);
            free(ctx->registry);
        }
        if (ctx->arena) {
            free(ctx->arena->base);
            free(ctx->arena);
        }
        free(ctx);
    }
}

/*═══════════════════════════════════════════════════════════════
  Benchmark Definitions
  ═══════════════════════════════════════════════════════════════*/

static cns_bench_def_t parser_benchmarks[] = {
    {
        .name = "parse_simple",
        .description = "Parse simple command",
        .type = CNS_BENCH_MICRO,
        .function = bench_parse_simple,
        .setup = bench_setup,
        .teardown = bench_teardown,
        .requires_7tick = true,
        .max_cycles = 7 * S7T_CYCLES_PER_TICK,
    },
    {
        .name = "parse_complex",
        .description = "Parse complex command with arguments",
        .type = CNS_BENCH_MICRO,
        .function = bench_parse_complex,
        .setup = bench_setup,
        .teardown = bench_teardown,
        .requires_7tick = true,
        .max_cycles = 7 * S7T_CYCLES_PER_TICK,
    },
    {
        .name = "parse_quoted",
        .description = "Parse command with quoted strings",
        .type = CNS_BENCH_MICRO,
        .function = bench_parse_quoted,
        .setup = bench_setup,
        .teardown = bench_teardown,
        .requires_7tick = true,
        .max_cycles = 7 * S7T_CYCLES_PER_TICK,
    },
};

static cns_bench_def_t registry_benchmarks[] = {
    {
        .name = "registry_lookup",
        .description = "Hash table lookup (hit)",
        .type = CNS_BENCH_MICRO,
        .function = bench_registry_lookup,
        .setup = bench_setup,
        .teardown = bench_teardown,
        .requires_7tick = true,
        .max_cycles = 7 * S7T_CYCLES_PER_TICK,
    },
    {
        .name = "registry_miss",
        .description = "Hash table lookup (miss)",
        .type = CNS_BENCH_MICRO,
        .function = bench_registry_miss,
        .setup = bench_setup,
        .teardown = bench_teardown,
        .requires_7tick = true,
        .max_cycles = 7 * S7T_CYCLES_PER_TICK,
    },
};

static cns_bench_def_t memory_benchmarks[] = {
    {
        .name = "arena_alloc_small",
        .description = "Small arena allocation (64 bytes)",
        .type = CNS_BENCH_MICRO,
        .function = bench_arena_alloc_small,
        .setup = bench_setup,
        .teardown = bench_teardown,
        .requires_7tick = true,
        .max_cycles = 7 * S7T_CYCLES_PER_TICK,
    },
    {
        .name = "arena_strdup",
        .description = "String duplication in arena",
        .type = CNS_BENCH_MICRO,
        .function = bench_arena_strdup,
        .setup = bench_setup,
        .teardown = bench_teardown,
        .requires_7tick = true,
        .max_cycles = 7 * S7T_CYCLES_PER_TICK,
    },
};

static cns_bench_def_t critical_benchmarks[] = {
    {
        .name = "critical_path",
        .description = "Full critical path (parse + lookup + dispatch)",
        .type = CNS_BENCH_MICRO,
        .function = bench_critical_path,
        .setup = bench_setup,
        .teardown = bench_teardown,
        .requires_7tick = true,
        .max_cycles = 7 * S7T_CYCLES_PER_TICK,
    },
};

/*═══════════════════════════════════════════════════════════════
  Benchmark Suites
  ═══════════════════════════════════════════════════════════════*/

static cns_bench_suite_t benchmark_suites[] = {
    {
        .name = "Parser Performance",
        .description = "Command parsing benchmarks",
        .benchmarks = parser_benchmarks,
        .benchmark_count = sizeof(parser_benchmarks) / sizeof(parser_benchmarks[0]),
    },
    {
        .name = "Registry Performance",
        .description = "Command registry benchmarks",
        .benchmarks = registry_benchmarks,
        .benchmark_count = sizeof(registry_benchmarks) / sizeof(registry_benchmarks[0]),
    },
    {
        .name = "Memory Performance",
        .description = "Memory allocation benchmarks",
        .benchmarks = memory_benchmarks,
        .benchmark_count = sizeof(memory_benchmarks) / sizeof(memory_benchmarks[0]),
    },
    {
        .name = "Critical Path",
        .description = "7-tick critical path validation",
        .benchmarks = critical_benchmarks,
        .benchmark_count = sizeof(critical_benchmarks) / sizeof(critical_benchmarks[0]),
    },
};

/*═══════════════════════════════════════════════════════════════
  Main Benchmark Runner
  ═══════════════════════════════════════════════════════════════*/

int main(int argc, char** argv) {
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("  CNS Performance Benchmarks\n");
    printf("  Platform: %s\n", S7T_PLATFORM_NAME);
    printf("  CPU: %s\n", S7T_CPU_NAME);
    printf("  Cycles per tick: %d\n", S7T_CYCLES_PER_TICK);
    printf("═══════════════════════════════════════════════════════════════\n\n");
    
    // Configuration
    cns_bench_config_t config = {
        .iterations = 10000,
        .warmup_iterations = 1000,
        .timeout_ms = 60000,
        .verify_results = true,
        .measure_memory = false,
        .export_json = true,
        .export_csv = true,
        .output_dir = "./bench_results",
    };
    
    // Run all suites
    size_t total_benchmarks = 0;
    size_t passed_benchmarks = 0;
    
    for (size_t i = 0; i < sizeof(benchmark_suites) / sizeof(benchmark_suites[0]); i++) {
        cns_bench_suite_t* suite = &benchmark_suites[i];
        
        printf("\n┌─ %s ─┐\n", suite->name);
        printf("│ %s\n", suite->description);
        printf("└%.*s┘\n", (int)strlen(suite->name) + 4, 
               "─────────────────────────────────────────");
        
        // Allocate results
        cns_bench_result_t* results = calloc(suite->benchmark_count, 
                                           sizeof(cns_bench_result_t));
        
        // Run benchmarks
        for (size_t j = 0; j < suite->benchmark_count; j++) {
            printf("\n▶ %s\n", suite->benchmarks[j].name);
            printf("  %s\n", suite->benchmarks[j].description);
            
            results[j] = cns_bench_run(&suite->benchmarks[j], &config);
            total_benchmarks++;
            
            // Print summary
            printf("  Results:\n");
            printf("    Iterations: %lu\n", results[j].iterations);
            printf("    Avg cycles: %lu (%.2f ns)\n", 
                   results[j].avg_cycles,
                   results[j].ns_per_op);
            printf("    Min/Max:    %lu / %lu cycles\n",
                   results[j].min_cycles,
                   results[j].max_cycles);
            printf("    Std dev:    %.2f cycles\n", results[j].std_dev);
            printf("    P50/P99:    %lu / %lu cycles\n",
                   results[j].p50, results[j].p99);
            
            if (suite->benchmarks[j].requires_7tick) {
                printf("    7-tick:     %s", 
                       results[j].passed ? "✓ PASS" : "✗ FAIL");
                if (!results[j].passed) {
                    printf(" (%.1fx over limit)", 
                           (double)results[j].avg_cycles / (7 * S7T_CYCLES_PER_TICK));
                }
                printf("\n");
            }
            
            if (results[j].passed) {
                passed_benchmarks++;
            }
        }
        
        // Export results
        if (config.export_json || config.export_csv) {
            char filename[256];
            
            if (config.export_json) {
                snprintf(filename, sizeof(filename), 
                        "%s/%s.json", config.output_dir, suite->name);
                cns_bench_export_json(results, suite->benchmark_count, filename);
            }
            
            if (config.export_csv) {
                snprintf(filename, sizeof(filename),
                        "%s/%s.csv", config.output_dir, suite->name);
                cns_bench_export_csv(results, suite->benchmark_count, filename);
            }
        }
        
        free(results);
    }
    
    // Summary
    printf("\n═══════════════════════════════════════════════════════════════\n");
    printf("  Benchmark Summary\n");
    printf("  Total:  %zu benchmarks\n", total_benchmarks);
    printf("  Passed: %zu (%.1f%%)\n", passed_benchmarks,
           100.0 * passed_benchmarks / total_benchmarks);
    printf("  Failed: %zu\n", total_benchmarks - passed_benchmarks);
    printf("═══════════════════════════════════════════════════════════════\n");
    
    return (passed_benchmarks == total_benchmarks) ? 0 : 1;
}