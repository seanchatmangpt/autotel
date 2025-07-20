#ifndef CNS_8T_BENCHMARK_H
#define CNS_8T_BENCHMARK_H

#include "cns/8t/core.h"
#include "cns/8t/processor.h"
#include "cns/8t/scheduler.h"
#include "cns/8t/analyzer.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// 8T BENCHMARK FRAMEWORK - COMPREHENSIVE PERFORMANCE TESTING
// ============================================================================

// Benchmark types
typedef enum {
    CNS_8T_BENCH_MATRIX_OPS,      // Matrix operations
    CNS_8T_BENCH_SIMD_OPS,        // SIMD operations
    CNS_8T_BENCH_SCHEDULER,       // Scheduler performance
    CNS_8T_BENCH_MEMORY,          // Memory operations
    CNS_8T_BENCH_CACHE,           // Cache performance
    CNS_8T_BENCH_NUMERICAL,       // Numerical accuracy
    CNS_8T_BENCH_INTEGRATION,     // Integration with 7T
    CNS_8T_BENCH_SCALABILITY,     // Scalability testing
    CNS_8T_BENCH_COMPREHENSIVE    // All benchmarks
} cns_8t_benchmark_type_t;

// Benchmark configuration
typedef struct {
    cns_8t_benchmark_type_t type;
    const char* name;
    const char* description;
    
    // Test parameters
    uint32_t iterations;
    uint32_t warmup_iterations;
    size_t data_size;
    uint32_t thread_count;
    
    // Validation parameters
    bool validate_results;
    double tolerance;
    
    // Performance targets
    cns_tick_t target_ticks;
    double target_throughput;
    double target_efficiency;
    
    // SIMD configuration
    uint32_t simd_width;
    bool test_scalar_fallback;
    
    // Memory configuration
    bool test_numa_effects;
    uint32_t memory_pattern;    // Sequential, random, stride
} cns_8t_benchmark_config_t;

// Benchmark result
typedef struct {
    // Basic timing
    cns_tick_t min_ticks;
    cns_tick_t max_ticks;
    cns_tick_t avg_ticks;
    cns_tick_t median_ticks;
    double std_deviation;
    
    // Performance metrics
    double operations_per_second;
    double megabytes_per_second;
    double gflops;              // Giga-FLOPS for numerical operations
    double efficiency;          // 0.0 to 1.0
    
    // Resource utilization
    double cpu_utilization;
    double memory_bandwidth_utilization;
    double cache_hit_ratio;
    double simd_utilization;
    
    // Accuracy metrics (for numerical benchmarks)
    double relative_error;
    double absolute_error;
    bool passed_validation;
    
    // Scalability metrics
    double parallel_efficiency;
    double speedup_ratio;
    
    // Pass/fail status
    bool passed_performance_target;
    bool passed_accuracy_target;
    bool overall_pass;
    
    // Additional measurements
    cns_8t_perf_metrics_t detailed_metrics;
} cns_8t_benchmark_result_t;

// Benchmark suite
typedef struct {
    cns_8t_benchmark_config_t* benchmarks;
    uint32_t benchmark_count;
    cns_8t_benchmark_result_t* results;
    
    // Suite configuration
    bool run_parallel;
    bool generate_report;
    const char* output_directory;
    
    // System under test
    cns_8t_system_t* system;
    
    // Overall results
    uint32_t passed_benchmarks;
    uint32_t failed_benchmarks;
    double overall_score;       // 0.0 to 100.0
} cns_8t_benchmark_suite_t;

// ============================================================================
// 8T SPECIFIC BENCHMARK OPERATIONS
// ============================================================================

// Matrix operation benchmarks
typedef struct {
    size_t matrix_size;
    cns_8t_precision_mode_t precision;
    bool use_simd;
    bool test_transposition;
    bool test_inversion;
    bool test_eigenvalues;
} cns_8t_matrix_bench_config_t;

cns_8t_result_t cns_8t_benchmark_matrix_multiply(const cns_8t_matrix_bench_config_t* config,
                                                  cns_8t_benchmark_result_t* result);

cns_8t_result_t cns_8t_benchmark_matrix_transpose(const cns_8t_matrix_bench_config_t* config,
                                                   cns_8t_benchmark_result_t* result);

cns_8t_result_t cns_8t_benchmark_matrix_invert(const cns_8t_matrix_bench_config_t* config,
                                                cns_8t_benchmark_result_t* result);

// SIMD operation benchmarks
typedef struct {
    size_t vector_size;
    uint32_t simd_width;        // 128, 256, 512
    bool test_arithmetic;
    bool test_transcendental;
    bool test_comparison;
    bool test_memory_ops;
} cns_8t_simd_bench_config_t;

cns_8t_result_t cns_8t_benchmark_simd_arithmetic(const cns_8t_simd_bench_config_t* config,
                                                  cns_8t_benchmark_result_t* result);

cns_8t_result_t cns_8t_benchmark_simd_transcendental(const cns_8t_simd_bench_config_t* config,
                                                      cns_8t_benchmark_result_t* result);

cns_8t_result_t cns_8t_benchmark_simd_memory(const cns_8t_simd_bench_config_t* config,
                                              cns_8t_benchmark_result_t* result);

// Scheduler benchmarks
typedef struct {
    uint32_t task_count;
    uint32_t worker_count;
    cns_8t_execution_mode_t exec_mode;
    bool test_load_balancing;
    bool test_priority_handling;
    bool test_numa_awareness;
} cns_8t_scheduler_bench_config_t;

cns_8t_result_t cns_8t_benchmark_task_throughput(const cns_8t_scheduler_bench_config_t* config,
                                                  cns_8t_benchmark_result_t* result);

cns_8t_result_t cns_8t_benchmark_load_balancing(const cns_8t_scheduler_bench_config_t* config,
                                                 cns_8t_benchmark_result_t* result);

cns_8t_result_t cns_8t_benchmark_priority_scheduling(const cns_8t_scheduler_bench_config_t* config,
                                                      cns_8t_benchmark_result_t* result);

// Memory benchmarks
typedef struct {
    size_t allocation_size;
    uint32_t allocation_count;
    uint32_t access_pattern;    // Sequential, random, stride
    bool test_numa_locality;
    bool test_fragmentation;
    bool test_pool_efficiency;
} cns_8t_memory_bench_config_t;

cns_8t_result_t cns_8t_benchmark_allocation_speed(const cns_8t_memory_bench_config_t* config,
                                                   cns_8t_benchmark_result_t* result);

cns_8t_result_t cns_8t_benchmark_memory_bandwidth(const cns_8t_memory_bench_config_t* config,
                                                   cns_8t_benchmark_result_t* result);

cns_8t_result_t cns_8t_benchmark_numa_locality(const cns_8t_memory_bench_config_t* config,
                                                cns_8t_benchmark_result_t* result);

// Numerical accuracy benchmarks
typedef struct {
    cns_8t_precision_mode_t precision;
    const char* algorithm_name;
    size_t problem_size;
    double* reference_solution;
    bool test_convergence;
    bool test_stability;
} cns_8t_numerical_bench_config_t;

cns_8t_result_t cns_8t_benchmark_numerical_accuracy(const cns_8t_numerical_bench_config_t* config,
                                                     cns_8t_benchmark_result_t* result);

cns_8t_result_t cns_8t_benchmark_convergence_rate(const cns_8t_numerical_bench_config_t* config,
                                                   cns_8t_benchmark_result_t* result);

// ============================================================================
// 8T BENCHMARK SUITE API
// ============================================================================

// Suite management
cns_8t_result_t cns_8t_benchmark_suite_create(cns_8t_benchmark_suite_t** suite);

cns_8t_result_t cns_8t_benchmark_suite_destroy(cns_8t_benchmark_suite_t* suite);

cns_8t_result_t cns_8t_benchmark_suite_add(cns_8t_benchmark_suite_t* suite,
                                            const cns_8t_benchmark_config_t* config);

cns_8t_result_t cns_8t_benchmark_suite_set_system(cns_8t_benchmark_suite_t* suite,
                                                   cns_8t_system_t* system);

// Execution
cns_8t_result_t cns_8t_benchmark_suite_run(cns_8t_benchmark_suite_t* suite);

cns_8t_result_t cns_8t_benchmark_run_single(const cns_8t_benchmark_config_t* config,
                                             cns_8t_system_t* system,
                                             cns_8t_benchmark_result_t* result);

// Results and reporting
cns_8t_result_t cns_8t_benchmark_suite_get_results(cns_8t_benchmark_suite_t* suite,
                                                    cns_8t_benchmark_result_t** results,
                                                    uint32_t* count);

cns_8t_result_t cns_8t_benchmark_generate_report(cns_8t_benchmark_suite_t* suite,
                                                  const char* filename);

cns_8t_result_t cns_8t_benchmark_export_csv(cns_8t_benchmark_suite_t* suite,
                                             const char* filename);

cns_8t_result_t cns_8t_benchmark_export_json(cns_8t_benchmark_suite_t* suite,
                                              const char* filename);

// ============================================================================
// 8T BUILT-IN BENCHMARK SUITES
// ============================================================================

// Create standard benchmark suites
cns_8t_result_t cns_8t_create_matrix_benchmark_suite(cns_8t_benchmark_suite_t** suite);

cns_8t_result_t cns_8t_create_simd_benchmark_suite(cns_8t_benchmark_suite_t** suite);

cns_8t_result_t cns_8t_create_scheduler_benchmark_suite(cns_8t_benchmark_suite_t** suite);

cns_8t_result_t cns_8t_create_memory_benchmark_suite(cns_8t_benchmark_suite_t** suite);

cns_8t_result_t cns_8t_create_numerical_benchmark_suite(cns_8t_benchmark_suite_t** suite);

cns_8t_result_t cns_8t_create_comprehensive_benchmark_suite(cns_8t_benchmark_suite_t** suite);

// Integration benchmarks
cns_8t_result_t cns_8t_create_7t_integration_benchmark_suite(cns_context_t* base_context,
                                                             cns_8t_benchmark_suite_t** suite);

// ============================================================================
// 8T BENCHMARK UTILITIES
// ============================================================================

// Statistical functions
double cns_8t_benchmark_calculate_mean(const cns_tick_t* values, uint32_t count);
double cns_8t_benchmark_calculate_median(cns_tick_t* values, uint32_t count);
double cns_8t_benchmark_calculate_stddev(const cns_tick_t* values, uint32_t count, double mean);
double cns_8t_benchmark_calculate_percentile(cns_tick_t* values, uint32_t count, double percentile);

// Performance calculations
double cns_8t_benchmark_calculate_gflops(uint64_t operations, cns_tick_t ticks, uint64_t frequency);
double cns_8t_benchmark_calculate_bandwidth(size_t bytes, cns_tick_t ticks, uint64_t frequency);
double cns_8t_benchmark_calculate_efficiency(cns_tick_t actual_ticks, cns_tick_t theoretical_ticks);

// Validation functions
bool cns_8t_benchmark_validate_matrix_result(const cns_8t_matrix_t* result,
                                              const cns_8t_matrix_t* reference,
                                              double tolerance);

bool cns_8t_benchmark_validate_vector_result(const cns_8t_vector_t* result,
                                              const cns_8t_vector_t* reference,
                                              double tolerance);

bool cns_8t_benchmark_validate_scalar_result(double result, double reference, double tolerance);

// System information
void cns_8t_benchmark_get_system_info(char* buffer, size_t buffer_size);
uint32_t cns_8t_benchmark_detect_simd_capabilities(void);
uint32_t cns_8t_benchmark_get_cache_sizes(uint32_t* l1_size, uint32_t* l2_size, uint32_t* l3_size);

// ============================================================================
// 8T BENCHMARK CONFIGURATION PRESETS
// ============================================================================

// Standard configurations for different use cases
extern const cns_8t_benchmark_config_t CNS_8T_BENCH_CONFIG_QUICK;
extern const cns_8t_benchmark_config_t CNS_8T_BENCH_CONFIG_STANDARD;
extern const cns_8t_benchmark_config_t CNS_8T_BENCH_CONFIG_COMPREHENSIVE;
extern const cns_8t_benchmark_config_t CNS_8T_BENCH_CONFIG_STRESS;

// Matrix-specific presets
extern const cns_8t_matrix_bench_config_t CNS_8T_MATRIX_BENCH_SMALL;
extern const cns_8t_matrix_bench_config_t CNS_8T_MATRIX_BENCH_MEDIUM;
extern const cns_8t_matrix_bench_config_t CNS_8T_MATRIX_BENCH_LARGE;

// SIMD-specific presets
extern const cns_8t_simd_bench_config_t CNS_8T_SIMD_BENCH_AVX2;
extern const cns_8t_simd_bench_config_t CNS_8T_SIMD_BENCH_AVX512;

#ifdef __cplusplus
}
#endif

#endif // CNS_8T_BENCHMARK_H