/*  ─────────────────────────────────────────────────────────────
    cns/bench/framework.h  –  Benchmark Framework (v2.0)
    Comprehensive benchmarking with 7-tick validation
    ───────────────────────────────────────────────────────────── */
#ifndef CNS_BENCH_FRAMEWORK_H
#define CNS_BENCH_FRAMEWORK_H

#include "../../../../include/s7t.h"
#include "../core/perf.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

/*═══════════════════════════════════════════════════════════════
  Benchmark Types
  ═══════════════════════════════════════════════════════════════*/

typedef enum {
    CNS_BENCH_MICRO,            // Micro-benchmark
    CNS_BENCH_MACRO,            // Macro-benchmark
    CNS_BENCH_STRESS,           // Stress test
    CNS_BENCH_REGRESSION,       // Regression test
    CNS_BENCH_COMPARATIVE,      // Comparative benchmark
} cns_bench_type_t;

/*═══════════════════════════════════════════════════════════════
  Benchmark Configuration
  ═══════════════════════════════════════════════════════════════*/

typedef struct {
    uint64_t iterations;        // Number of iterations
    uint64_t warmup_iterations; // Warmup iterations
    uint64_t timeout_ms;        // Timeout in milliseconds
    bool verify_results;        // Verify correctness
    bool measure_memory;        // Track memory usage
    bool export_json;           // Export JSON results
    bool export_csv;            // Export CSV results
    const char* output_dir;     // Output directory
} cns_bench_config_t;

/*═══════════════════════════════════════════════════════════════
  Benchmark Function
  ═══════════════════════════════════════════════════════════════*/

// Benchmark function type
typedef void (*cns_bench_fn)(void* context);

// Benchmark setup/teardown
typedef void* (*cns_bench_setup_fn)(void);
typedef void (*cns_bench_teardown_fn)(void* context);

/*═══════════════════════════════════════════════════════════════
  Benchmark Definition
  ═══════════════════════════════════════════════════════════════*/

typedef struct {
    const char* name;           // Benchmark name
    const char* description;    // Description
    cns_bench_type_t type;      // Benchmark type
    
    // Functions
    cns_bench_fn function;      // Benchmark function
    cns_bench_setup_fn setup;   // Setup function
    cns_bench_teardown_fn teardown; // Teardown function
    
    // Constraints
    uint64_t max_cycles;        // Maximum allowed cycles
    bool requires_7tick;        // Must complete in 7 ticks
    
    // Tags
    const char** tags;          // Benchmark tags
    size_t tag_count;           // Number of tags
} cns_bench_def_t;

/*═══════════════════════════════════════════════════════════════
  Benchmark Results
  ═══════════════════════════════════════════════════════════════*/

typedef struct {
    // Identification
    const char* name;           // Benchmark name
    cns_bench_type_t type;      // Benchmark type
    
    // Timing results
    uint64_t iterations;        // Iterations performed
    uint64_t total_cycles;      // Total CPU cycles
    uint64_t avg_cycles;        // Average cycles
    uint64_t min_cycles;        // Minimum cycles
    uint64_t max_cycles;        // Maximum cycles
    uint64_t median_cycles;     // Median cycles
    
    // Statistical measures
    double std_dev;             // Standard deviation
    double variance;            // Variance
    double cv;                  // Coefficient of variation
    
    // Percentiles
    uint64_t p50;               // 50th percentile
    uint64_t p90;               // 90th percentile
    uint64_t p95;               // 95th percentile
    uint64_t p99;               // 99th percentile
    
    // Performance
    double ops_per_sec;         // Operations per second
    double ns_per_op;           // Nanoseconds per operation
    
    // Validation
    bool passed;                // Passed constraints
    uint64_t violations;        // Constraint violations
    
    // Memory (if measured)
    size_t memory_used;         // Memory used
    size_t peak_memory;         // Peak memory usage
} cns_bench_result_t;

/*═══════════════════════════════════════════════════════════════
  Benchmark Suite
  ═══════════════════════════════════════════════════════════════*/

typedef struct {
    const char* name;           // Suite name
    const char* description;    // Suite description
    cns_bench_def_t* benchmarks; // Benchmark definitions
    size_t benchmark_count;     // Number of benchmarks
    cns_bench_config_t config;  // Suite configuration
} cns_bench_suite_t;

/*═══════════════════════════════════════════════════════════════
  Benchmark Runner
  ═══════════════════════════════════════════════════════════════*/

// Run single benchmark
cns_bench_result_t cns_bench_run(
    const cns_bench_def_t* bench,
    const cns_bench_config_t* config
);

// Run benchmark suite
void cns_bench_run_suite(
    const cns_bench_suite_t* suite,
    cns_bench_result_t* results
);

// Compare two results
typedef struct {
    double speedup;             // Speedup factor
    double percent_change;      // Percentage change
    bool regression;            // Is regression?
    double confidence;          // Confidence level
} cns_bench_comparison_t;

cns_bench_comparison_t cns_bench_compare(
    const cns_bench_result_t* baseline,
    const cns_bench_result_t* current
);

/*═══════════════════════════════════════════════════════════════
  Benchmark Reporting
  ═══════════════════════════════════════════════════════════════*/

// Print result summary
void cns_bench_print_result(
    const cns_bench_result_t* result,
    FILE* output
);

// Export results to JSON
void cns_bench_export_json(
    const cns_bench_result_t* results,
    size_t count,
    const char* filename
);

// Export results to CSV
void cns_bench_export_csv(
    const cns_bench_result_t* results,
    size_t count,
    const char* filename
);

// Generate HTML report
void cns_bench_generate_report(
    const cns_bench_suite_t* suite,
    const cns_bench_result_t* results,
    const char* output_dir
);

/*═══════════════════════════════════════════════════════════════
  Benchmark Macros
  ═══════════════════════════════════════════════════════════════*/

// Define a benchmark
#define CNS_BENCHMARK_DEF(name, fn, desc) { \
    .name = #name, \
    .description = desc, \
    .type = CNS_BENCH_MICRO, \
    .function = fn, \
    .setup = NULL, \
    .teardown = NULL, \
    .max_cycles = 0, \
    .requires_7tick = false, \
    .tags = NULL, \
    .tag_count = 0 \
}

// Define a 7-tick benchmark
#define CNS_7TICK_BENCHMARK_DEF(name, fn, desc) { \
    .name = #name, \
    .description = desc, \
    .type = CNS_BENCH_MICRO, \
    .function = fn, \
    .setup = NULL, \
    .teardown = NULL, \
    .max_cycles = 7 * S7T_CYCLES_PER_TICK, \
    .requires_7tick = true, \
    .tags = NULL, \
    .tag_count = 0 \
}

// Quick benchmark execution
#define CNS_QUICK_BENCHMARK(name, iterations, code) \
    do { \
        printf("Benchmark: %s\n", name); \
        uint64_t __total = 0, __min = UINT64_MAX, __max = 0; \
        for (uint64_t __i = 0; __i < iterations; __i++) { \
            uint64_t __start = s7t_cycles(); \
            { code; } \
            uint64_t __cycles = s7t_cycles() - __start; \
            __total += __cycles; \
            if (__cycles < __min) __min = __cycles; \
            if (__cycles > __max) __max = __cycles; \
        } \
        printf("  Iterations: %lu\n", iterations); \
        printf("  Avg cycles: %lu\n", __total / iterations); \
        printf("  Min cycles: %lu\n", __min); \
        printf("  Max cycles: %lu\n", __max); \
        printf("  7-tick: %s\n", \
            (__total / iterations <= 7 * S7T_CYCLES_PER_TICK) ? "PASS" : "FAIL"); \
    } while(0)

/*═══════════════════════════════════════════════════════════════
  Benchmark Harness
  ═══════════════════════════════════════════════════════════════*/

// Benchmark harness state
typedef struct {
    uint64_t* samples;          // Cycle samples
    size_t sample_count;        // Number of samples
    size_t sample_capacity;     // Sample array capacity
    bool outlier_removal;       // Remove outliers
    double outlier_threshold;   // Outlier threshold (IQR multiplier)
} cns_bench_harness_t;

// Initialize harness
void cns_bench_harness_init(
    cns_bench_harness_t* harness,
    size_t capacity,
    bool outlier_removal
);

// Cleanup harness
void cns_bench_harness_cleanup(cns_bench_harness_t* harness);

// Add sample
void cns_bench_harness_add_sample(
    cns_bench_harness_t* harness,
    uint64_t cycles
);

// Calculate statistics
void cns_bench_harness_calculate(
    cns_bench_harness_t* harness,
    cns_bench_result_t* result
);

/*═══════════════════════════════════════════════════════════════
  Default Configuration
  ═══════════════════════════════════════════════════════════════*/

#define CNS_DEFAULT_BENCH_CONFIG { \
    .iterations = 10000, \
    .warmup_iterations = 1000, \
    .timeout_ms = 60000, \
    .verify_results = true, \
    .measure_memory = false, \
    .export_json = true, \
    .export_csv = false, \
    .output_dir = "./bench_results" \
}

#endif /* CNS_BENCH_FRAMEWORK_H */