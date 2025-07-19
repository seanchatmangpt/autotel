/*  ─────────────────────────────────────────────────────────────
    cmd_benchmark.c  –  CNS Benchmark Command Implementation
    Ports 7T benchmark framework to CNS command structure
    ───────────────────────────────────────────────────────────── */

#include "../include/cns.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

/*═══════════════════════════════════════════════════════════════
  Performance Targets (from 7T framework)
  ═══════════════════════════════════════════════════════════════*/

#define SEVEN_TICK_TARGET_NS 10.0
#define L2_TIER_TARGET_NS 100.0
#define L3_TIER_TARGET_NS 1000.0

#define DEFAULT_ITERATIONS 1000000
#define DEFAULT_WARMUP_ITERATIONS 10000
#define DEFAULT_BATCH_SIZE 1000

/*═══════════════════════════════════════════════════════════════
  Benchmark Structures
  ═══════════════════════════════════════════════════════════════*/

typedef struct {
    const char* operation_name;
    const char* component_name;
    uint64_t total_operations;
    uint64_t total_time_ns;
    uint64_t min_time_ns;
    uint64_t max_time_ns;
    double avg_time_ns;
    double throughput_ops_per_sec;
    double p50_time_ns;
    double p95_time_ns;
    double p99_time_ns;
    bool target_achieved;
    const char* performance_tier;
} cns_benchmark_result_t;

typedef struct {
    const char* benchmark_name;
    uint64_t iterations;
    uint64_t warmup_iterations;
    uint64_t batch_size;
    bool verbose;
    bool validate_correctness;
} cns_benchmark_config_t;

// Function pointer for benchmark operations
typedef void (*cns_benchmark_op_t)(void* context);

/*═══════════════════════════════════════════════════════════════
  High-Precision Timing Functions
  ═══════════════════════════════════════════════════════════════*/

static inline uint64_t get_nanoseconds(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

/*═══════════════════════════════════════════════════════════════
  Benchmark Utility Functions
  ═══════════════════════════════════════════════════════════════*/

static const char* get_performance_tier(double avg_time_ns) {
    if (avg_time_ns < SEVEN_TICK_TARGET_NS) {
        return "L1 (7-TICK)";
    } else if (avg_time_ns < L2_TIER_TARGET_NS) {
        return "L2 (Sub-100ns)";
    } else if (avg_time_ns < L3_TIER_TARGET_NS) {
        return "L3 (Sub-1μs)";
    } else {
        return "Above L3";
    }
}

static void sort_times(uint64_t* times, uint64_t count) {
    for (uint64_t i = 0; i < count - 1; i++) {
        for (uint64_t j = 0; j < count - i - 1; j++) {
            if (times[j] > times[j + 1]) {
                uint64_t temp = times[j];
                times[j] = times[j + 1];
                times[j + 1] = temp;
            }
        }
    }
}

static double calculate_percentile(uint64_t* times, uint64_t count, double percentile) {
    if (count == 0) return 0.0;
    
    uint64_t* sorted_times = malloc(count * sizeof(uint64_t));
    if (!sorted_times) return 0.0;
    
    memcpy(sorted_times, times, count * sizeof(uint64_t));
    sort_times(sorted_times, count);
    
    double index = (percentile / 100.0) * (count - 1);
    uint64_t lower_index = (uint64_t)index;
    uint64_t upper_index = lower_index + 1;
    
    double result;
    if (upper_index >= count) {
        result = sorted_times[lower_index];
    } else {
        double weight = index - lower_index;
        result = sorted_times[lower_index] * (1 - weight) + sorted_times[upper_index] * weight;
    }
    
    free(sorted_times);
    return result;
}

/*═══════════════════════════════════════════════════════════════
  Built-in Benchmark Operations
  ═══════════════════════════════════════════════════════════════*/

// Context for hash benchmark
typedef struct {
    const char* test_string;
    uint32_t result;
} hash_bench_ctx_t;

// 7-tick hash benchmark operation
static void bench_hash_op(void* ctx) {
    hash_bench_ctx_t* hctx = (hash_bench_ctx_t*)ctx;
    hctx->result = s7t_hash_string(hctx->test_string, strlen(hctx->test_string));
}

// Context for command lookup benchmark
typedef struct {
    cns_engine_t* engine;
    uint32_t hash;
    cns_cmd_entry_t* result;
} lookup_bench_ctx_t;

// Command lookup benchmark operation
static void bench_lookup_op(void* ctx) {
    lookup_bench_ctx_t* lctx = (lookup_bench_ctx_t*)ctx;
    lctx->result = cns_lookup(lctx->engine, lctx->hash);
}

// Context for parse benchmark
typedef struct {
    const char* cmdline;
    cns_command_t cmd;
} parse_bench_ctx_t;

// Parse benchmark operation
static void bench_parse_op(void* ctx) {
    parse_bench_ctx_t* pctx = (parse_bench_ctx_t*)ctx;
    cns_parse(pctx->cmdline, &pctx->cmd);
}

/*═══════════════════════════════════════════════════════════════
  Main Benchmark Runner
  ═══════════════════════════════════════════════════════════════*/

static cns_benchmark_result_t run_benchmark(
    const char* operation_name,
    const char* component_name,
    cns_benchmark_config_t* config,
    cns_benchmark_op_t operation,
    void* context
) {
    cns_benchmark_result_t result = {0};
    result.operation_name = operation_name;
    result.component_name = component_name;
    result.total_operations = config->iterations;
    result.min_time_ns = UINT64_MAX;
    result.max_time_ns = 0;
    
    if (config->verbose) {
        printf("Running benchmark: %s (%s)\n", operation_name, component_name);
        printf("Iterations: %llu, Warmup: %llu\n", config->iterations, config->warmup_iterations);
    }
    
    // Warmup phase
    for (uint64_t i = 0; i < config->warmup_iterations; i++) {
        operation(context);
    }
    
    // Allocate array for individual timing measurements
    uint64_t* individual_times = malloc(config->iterations * sizeof(uint64_t));
    if (!individual_times) {
        printf("❌ Failed to allocate memory for timing measurements\n");
        return result;
    }
    
    // Actual benchmark
    for (uint64_t i = 0; i < config->iterations; i++) {
        uint64_t start = get_nanoseconds();
        operation(context);
        uint64_t end = get_nanoseconds();
        
        uint64_t duration = end - start;
        individual_times[i] = duration;
        result.total_time_ns += duration;
        
        if (duration < result.min_time_ns) result.min_time_ns = duration;
        if (duration > result.max_time_ns) result.max_time_ns = duration;
    }
    
    // Calculate statistics
    result.avg_time_ns = (double)result.total_time_ns / config->iterations;
    result.throughput_ops_per_sec = (double)config->iterations / (result.total_time_ns / 1e9);
    result.p50_time_ns = calculate_percentile(individual_times, config->iterations, 50.0);
    result.p95_time_ns = calculate_percentile(individual_times, config->iterations, 95.0);
    result.p99_time_ns = calculate_percentile(individual_times, config->iterations, 99.0);
    result.target_achieved = result.avg_time_ns < SEVEN_TICK_TARGET_NS;
    result.performance_tier = get_performance_tier(result.avg_time_ns);
    
    free(individual_times);
    return result;
}

/*═══════════════════════════════════════════════════════════════
  Result Printing Functions
  ═══════════════════════════════════════════════════════════════*/

static void print_benchmark_result(cns_benchmark_result_t* result) {
    printf("=== %s Benchmark (%s) ===\n", result->operation_name, result->component_name);
    printf("Operations: %llu\n", result->total_operations);
    printf("Total time: %.3f ms\n", result->total_time_ns / 1e6);
    printf("Average: %.1f ns\n", result->avg_time_ns);
    printf("Min: %llu ns\n", result->min_time_ns);
    printf("Max: %llu ns\n", result->max_time_ns);
    printf("P50: %.1f ns\n", result->p50_time_ns);
    printf("P95: %.1f ns\n", result->p95_time_ns);
    printf("P99: %.1f ns\n", result->p99_time_ns);
    printf("Throughput: %.0f ops/sec\n", result->throughput_ops_per_sec);
    
    // Performance tier assessment
    if (result->target_achieved) {
        printf("🎉 %s: 7-TICK PERFORMANCE ACHIEVED! (%.1f ns)\n",
               result->performance_tier, result->avg_time_ns);
    } else if (result->avg_time_ns < L2_TIER_TARGET_NS) {
        printf("✅ %s: Sub-100ns performance! (%.1f ns)\n",
               result->performance_tier, result->avg_time_ns);
    } else if (result->avg_time_ns < L3_TIER_TARGET_NS) {
        printf("✅ %s: Sub-1μs performance! (%.1f ns)\n",
               result->performance_tier, result->avg_time_ns);
    } else {
        printf("⚠️ %s: Performance above 1μs (%.1f ns)\n",
               result->performance_tier, result->avg_time_ns);
    }
    printf("\n");
}

/*═══════════════════════════════════════════════════════════════
  CNS Benchmark Command Implementation
  ═══════════════════════════════════════════════════════════════*/

int cmd_benchmark(int argc, char** argv) {
    // Default configuration
    cns_benchmark_config_t config = {
        .benchmark_name = "CNS Benchmark",
        .iterations = DEFAULT_ITERATIONS,
        .warmup_iterations = DEFAULT_WARMUP_ITERATIONS,
        .batch_size = DEFAULT_BATCH_SIZE,
        .verbose = false,
        .validate_correctness = true
    };
    
    // What to benchmark
    bool bench_hash = false;
    bool bench_lookup = false;
    bool bench_parse = false;
    bool bench_all = false;
    
    // Parse arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
            config.iterations = strtoull(argv[++i], NULL, 10);
        } else if (strcmp(argv[i], "-w") == 0 && i + 1 < argc) {
            config.warmup_iterations = strtoull(argv[++i], NULL, 10);
        } else if (strcmp(argv[i], "-v") == 0) {
            config.verbose = true;
        } else if (strcmp(argv[i], "-q") == 0) {
            // Quick benchmark mode
            config.iterations = 100000;
            config.warmup_iterations = 1000;
        } else if (strcmp(argv[i], "-t") == 0) {
            // Thorough benchmark mode
            config.iterations = 10000000;
            config.warmup_iterations = 100000;
            config.verbose = true;
        } else if (strcmp(argv[i], "hash") == 0) {
            bench_hash = true;
        } else if (strcmp(argv[i], "lookup") == 0) {
            bench_lookup = true;
        } else if (strcmp(argv[i], "parse") == 0) {
            bench_parse = true;
        } else if (strcmp(argv[i], "all") == 0) {
            bench_all = true;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printf("Usage: cns benchmark [options] [operations]\n");
            printf("\nOperations:\n");
            printf("  hash    - Benchmark 7-tick hash function\n");
            printf("  lookup  - Benchmark command lookup\n");
            printf("  parse   - Benchmark command parsing\n");
            printf("  all     - Run all benchmarks\n");
            printf("\nOptions:\n");
            printf("  -i N    - Number of iterations (default: %d)\n", DEFAULT_ITERATIONS);
            printf("  -w N    - Warmup iterations (default: %d)\n", DEFAULT_WARMUP_ITERATIONS);
            printf("  -v      - Verbose output\n");
            printf("  -q      - Quick benchmark (100k iterations)\n");
            printf("  -t      - Thorough benchmark (10M iterations)\n");
            printf("  -h      - Show this help\n");
            return 0;
        }
    }
    
    // Default to all if nothing specified
    if (!bench_hash && !bench_lookup && !bench_parse && !bench_all) {
        bench_all = true;
    }
    
    if (bench_all) {
        bench_hash = bench_lookup = bench_parse = true;
    }
    
    printf("=== CNS 7-Tick Benchmark Suite ===\n");
    printf("Configuration:\n");
    printf("  Iterations: %llu\n", config.iterations);
    printf("  Warmup: %llu\n", config.warmup_iterations);
    printf("\n");
    
    int result_count = 0;
    cns_benchmark_result_t results[10];
    
    // Run hash benchmark
    if (bench_hash) {
        hash_bench_ctx_t hash_ctx = {
            .test_string = "benchmark_test_string_12345",
            .result = 0
        };
        
        results[result_count++] = run_benchmark(
            "String Hash", "7T Core",
            &config, bench_hash_op, &hash_ctx
        );
    }
    
    // Run lookup benchmark (requires engine context)
    if (bench_lookup) {
        // Create a dummy engine for benchmarking
        cns_engine_t* engine = cns_create(64);
        if (engine) {
            // Register some test commands
            CNS_REGISTER_CMD(engine, "test1", NULL, 0, 0, "Test command 1");
            CNS_REGISTER_CMD(engine, "test2", NULL, 0, 0, "Test command 2");
            CNS_REGISTER_CMD(engine, "benchmark", NULL, 0, 0, "Benchmark command");
            
            lookup_bench_ctx_t lookup_ctx = {
                .engine = engine,
                .hash = s7t_hash_string("benchmark", 9),
                .result = NULL
            };
            
            results[result_count++] = run_benchmark(
                "Command Lookup", "CNS Engine",
                &config, bench_lookup_op, &lookup_ctx
            );
            
            cns_destroy(engine);
        }
    }
    
    // Run parse benchmark
    if (bench_parse) {
        parse_bench_ctx_t parse_ctx = {
            .cmdline = "benchmark -i 1000000 -v all",
            .cmd = {0}
        };
        
        results[result_count++] = run_benchmark(
            "Command Parse", "CNS Parser",
            &config, bench_parse_op, &parse_ctx
        );
    }
    
    // Print all results
    for (int i = 0; i < result_count; i++) {
        print_benchmark_result(&results[i]);
    }
    
    // Print summary
    printf("=== Benchmark Summary ===\n");
    printf("Total benchmarks: %d\n\n", result_count);
    
    int seven_tick_count = 0;
    int l2_tier_count = 0;
    int l3_tier_count = 0;
    int above_l3_count = 0;
    
    for (int i = 0; i < result_count; i++) {
        if (results[i].target_achieved) {
            seven_tick_count++;
        } else if (results[i].avg_time_ns < L2_TIER_TARGET_NS) {
            l2_tier_count++;
        } else if (results[i].avg_time_ns < L3_TIER_TARGET_NS) {
            l3_tier_count++;
        } else {
            above_l3_count++;
        }
    }
    
    printf("Performance Distribution:\n");
    if (result_count > 0) {
        printf("  🎉 7-Tick Performance: %d/%d (%.1f%%)\n",
               seven_tick_count, result_count, (double)seven_tick_count / result_count * 100);
        printf("  ✅ L2 Tier (Sub-100ns): %d/%d (%.1f%%)\n",
               l2_tier_count, result_count, (double)l2_tier_count / result_count * 100);
        printf("  ✅ L3 Tier (Sub-1μs): %d/%d (%.1f%%)\n",
               l3_tier_count, result_count, (double)l3_tier_count / result_count * 100);
        printf("  ⚠️ Above L3: %d/%d (%.1f%%)\n",
               above_l3_count, result_count, (double)above_l3_count / result_count * 100);
    }
    printf("\n");
    
    return 0;
}