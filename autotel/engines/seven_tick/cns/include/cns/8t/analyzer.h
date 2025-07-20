#ifndef CNS_8T_ANALYZER_H
#define CNS_8T_ANALYZER_H

#include "cns/8t/core.h"
#include "cns/8t/processor.h"
#include "cns/8t/scheduler.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// 8T PERFORMANCE ANALYZER - ADVANCED PROFILING AND OPTIMIZATION
// ============================================================================

// Analysis types
typedef enum {
    CNS_8T_ANALYSIS_PERFORMANCE,   // Performance profiling
    CNS_8T_ANALYSIS_MEMORY,        // Memory usage analysis
    CNS_8T_ANALYSIS_CACHE,         // Cache behavior analysis
    CNS_8T_ANALYSIS_SIMD,          // SIMD utilization analysis
    CNS_8T_ANALYSIS_SCHEDULING,    // Scheduling efficiency analysis
    CNS_8T_ANALYSIS_NUMERICAL,     // Numerical accuracy analysis
    CNS_8T_ANALYSIS_BOTTLENECK,    // Bottleneck identification
    CNS_8T_ANALYSIS_PREDICTION     // Performance prediction
} cns_8t_analysis_type_t;

// Performance profile data
typedef struct {
    // Timing information
    cns_tick_t total_execution_ticks;
    cns_tick_t average_tick_per_operation;
    cns_tick_t min_execution_ticks;
    cns_tick_t max_execution_ticks;
    double     standard_deviation;
    
    // Throughput metrics
    double operations_per_second;
    double megabytes_per_second;
    double instructions_per_cycle;
    
    // Cache performance
    uint64_t l1_cache_hits;
    uint64_t l1_cache_misses;
    uint64_t l2_cache_hits;
    uint64_t l2_cache_misses;
    uint64_t l3_cache_hits;
    uint64_t l3_cache_misses;
    double   cache_hit_ratio;
    
    // SIMD utilization
    uint64_t simd_operations;
    uint64_t scalar_operations;
    double   simd_utilization_ratio;
    uint64_t vectorization_opportunities;
    
    // Branch prediction
    uint64_t branch_instructions;
    uint64_t branch_mispredictions;
    double   branch_prediction_accuracy;
    
    // Memory usage
    size_t peak_memory_usage;
    size_t average_memory_usage;
    size_t memory_allocations;
    size_t memory_deallocations;
    double memory_fragmentation_ratio;
} cns_8t_performance_profile_t __attribute__((aligned(64)));

// Memory analysis data
typedef struct {
    // Allocation patterns
    size_t total_allocations;
    size_t total_deallocations;
    size_t active_allocations;
    size_t peak_allocations;
    
    // Size distribution
    size_t small_allocations;    // < 1KB
    size_t medium_allocations;   // 1KB - 1MB
    size_t large_allocations;    // > 1MB
    
    // Timing
    cns_tick_t total_alloc_time;
    cns_tick_t average_alloc_time;
    cns_tick_t total_free_time;
    cns_tick_t average_free_time;
    
    // NUMA analysis
    uint32_t numa_local_allocations;
    uint32_t numa_remote_allocations;
    double   numa_locality_ratio;
    
    // Fragmentation
    size_t internal_fragmentation;
    size_t external_fragmentation;
    double fragmentation_ratio;
    
    // Pool efficiency
    double pool_utilization;
    uint32_t pool_overflows;
    uint32_t pool_underflows;
} cns_8t_memory_analysis_t __attribute__((aligned(64)));

// Cache behavior analysis
typedef struct {
    // Hit rates by level
    double l1_hit_rate;
    double l2_hit_rate;
    double l3_hit_rate;
    double tlb_hit_rate;
    
    // Miss penalties
    cns_tick_t l1_miss_penalty;
    cns_tick_t l2_miss_penalty;
    cns_tick_t l3_miss_penalty;
    cns_tick_t memory_access_penalty;
    
    // Access patterns
    uint64_t sequential_accesses;
    uint64_t random_accesses;
    uint64_t stride_accesses;
    double   spatial_locality_score;
    double   temporal_locality_score;
    
    // Prefetch effectiveness
    uint64_t prefetch_hits;
    uint64_t prefetch_misses;
    double   prefetch_accuracy;
    
    // Cache line utilization
    double cache_line_utilization;
    uint32_t false_sharing_events;
    uint32_t true_sharing_events;
} cns_8t_cache_analysis_t __attribute__((aligned(64)));

// SIMD analysis data
typedef struct {
    // Instruction counts
    uint64_t scalar_instructions;
    uint64_t vector_instructions;
    uint64_t mixed_instructions;
    
    // Utilization by width
    uint64_t simd_128_ops;       // SSE operations
    uint64_t simd_256_ops;       // AVX operations
    uint64_t simd_512_ops;       // AVX-512 operations
    
    // Data types
    uint64_t float32_ops;
    uint64_t float64_ops;
    uint64_t int32_ops;
    uint64_t int64_ops;
    
    // Efficiency metrics
    double vectorization_ratio;
    double lane_utilization;
    double memory_bandwidth_utilization;
    
    // Missed opportunities
    uint32_t vectorizable_loops;
    uint32_t non_vectorized_loops;
    uint32_t alignment_issues;
    uint32_t dependency_chains;
} cns_8t_simd_analysis_t __attribute__((aligned(64)));

// Scheduling analysis
typedef struct {
    // Task statistics
    uint64_t total_tasks;
    uint64_t completed_tasks;
    uint64_t failed_tasks;
    uint64_t cancelled_tasks;
    
    // Timing
    cns_tick_t average_queue_time;
    cns_tick_t average_execution_time;
    cns_tick_t average_turnaround_time;
    cns_tick_t scheduling_overhead;
    
    // Load balancing
    double load_balance_efficiency;
    uint32_t work_stealing_events;
    uint32_t idle_worker_cycles;
    
    // Priority analysis
    cns_tick_t priority_inversion_time;
    uint32_t starved_tasks;
    double fairness_index;
    
    // Resource utilization
    double cpu_utilization;
    double memory_utilization;
    double numa_efficiency;
} cns_8t_scheduling_analysis_t __attribute__((aligned(64)));

// Numerical accuracy analysis
typedef struct {
    // Precision tracking
    double relative_error;
    double absolute_error;
    double max_error;
    double rms_error;
    
    // Condition number analysis
    double condition_number;
    double stability_measure;
    uint32_t ill_conditioned_operations;
    
    // Convergence analysis
    uint32_t iterations_to_convergence;
    double convergence_rate;
    uint32_t divergent_operations;
    
    // Error propagation
    double input_error_amplification;
    double cumulative_error;
    uint32_t catastrophic_cancellations;
    
    // Precision mode effectiveness
    uint32_t precision_downgrades;
    uint32_t precision_upgrades;
    double precision_efficiency;
} cns_8t_numerical_analysis_t __attribute__((aligned(64)));

// Bottleneck identification
typedef struct {
    // Primary bottleneck
    cns_8t_analysis_type_t primary_bottleneck;
    double bottleneck_severity;  // 0.0 to 1.0
    const char* bottleneck_description;
    
    // CPU bottlenecks
    double cpu_bound_ratio;
    uint32_t cpu_intensive_stages;
    
    // Memory bottlenecks
    double memory_bound_ratio;
    uint32_t memory_intensive_stages;
    
    // I/O bottlenecks
    double io_bound_ratio;
    uint32_t io_intensive_stages;
    
    // Cache bottlenecks
    double cache_bound_ratio;
    uint32_t cache_sensitive_stages;
    
    // Synchronization bottlenecks
    uint32_t lock_contention_events;
    cns_tick_t synchronization_overhead;
    
    // Recommendations
    const char* optimization_suggestions[8];
    uint32_t suggestion_count;
} cns_8t_bottleneck_analysis_t __attribute__((aligned(64)));

// Performance prediction
typedef struct {
    // Scaling predictions
    double predicted_single_thread_performance;
    double predicted_multi_thread_performance;
    double predicted_simd_performance;
    
    // Resource requirement predictions
    size_t predicted_memory_usage;
    cns_tick_t predicted_execution_time;
    uint32_t predicted_cache_misses;
    
    // Confidence intervals
    double prediction_confidence;
    double lower_bound_performance;
    double upper_bound_performance;
    
    // Model parameters
    const char* prediction_model;
    double model_accuracy;
    uint32_t training_samples;
} cns_8t_prediction_analysis_t __attribute__((aligned(64)));

// Comprehensive analysis report
typedef struct {
    cns_8t_analysis_type_t analysis_types;  // Bitmask of performed analyses
    cns_tick_t analysis_timestamp;
    cns_tick_t analysis_duration;
    
    // Analysis results
    cns_8t_performance_profile_t* performance;
    cns_8t_memory_analysis_t* memory;
    cns_8t_cache_analysis_t* cache;
    cns_8t_simd_analysis_t* simd;
    cns_8t_scheduling_analysis_t* scheduling;
    cns_8t_numerical_analysis_t* numerical;
    cns_8t_bottleneck_analysis_t* bottleneck;
    cns_8t_prediction_analysis_t* prediction;
    
    // Overall score
    double overall_performance_score;  // 0.0 to 100.0
    const char* performance_grade;     // A, B, C, D, F
    
    // Recommendations
    const char* recommendations[16];
    uint32_t recommendation_count;
} cns_8t_analysis_report_t;

// Analyzer configuration
typedef struct {
    // Analysis scope
    uint32_t analysis_types;        // Bitmask of cns_8t_analysis_type_t
    bool enable_real_time_analysis; // Real-time vs post-mortem
    uint32_t sampling_frequency;    // For real-time analysis
    
    // Performance counters
    bool enable_hardware_counters;  // Use hardware performance counters
    bool enable_software_counters;  // Use software counters
    uint32_t counter_granularity;   // Counter sampling granularity
    
    // Memory tracking
    bool track_all_allocations;     // Track every allocation
    bool track_call_stacks;         // Capture allocation call stacks
    uint32_t max_call_stack_depth;  // Maximum call stack depth
    
    // Cache simulation
    bool enable_cache_simulation;   // Simulate cache behavior
    uint32_t cache_line_size;       // Cache line size for simulation
    uint32_t cache_associativity;   // Cache associativity
    
    // Output configuration
    bool generate_detailed_report;  // Generate detailed report
    bool export_raw_data;          // Export raw measurement data
    const char* output_directory;   // Directory for output files
} cns_8t_analyzer_config_t;

// Main analyzer structure
struct cns_8t_analyzer {
    cns_8t_analyzer_config_t config;
    
    // Target objects
    cns_8t_context_t* context;
    cns_8t_processor_t* processor;
    cns_8t_scheduler_t* scheduler;
    
    // Data collection
    cns_8t_performance_profile_t* profiles;
    uint32_t profile_count;
    uint32_t profile_capacity;
    
    // Real-time monitoring
    bool monitoring_active;
    cns_tick_t monitoring_start_tick;
    uint32_t sample_count;
    
    // Analysis state
    cns_8t_analysis_report_t* current_report;
    cns_8t_analysis_report_t** historical_reports;
    uint32_t report_count;
    
    // Performance counters (simplified for C interface)
    uint64_t hardware_counters[32];
    uint64_t software_counters[32];
    const char* counter_names[64];
} __attribute__((aligned(64)));

// ============================================================================
// 8T ANALYZER API
// ============================================================================

// Analyzer lifecycle
cns_8t_result_t cns_8t_analyzer_create(const cns_8t_analyzer_config_t* config,
                                        cns_8t_analyzer_t** analyzer);

cns_8t_result_t cns_8t_analyzer_destroy(cns_8t_analyzer_t* analyzer);

// Target object association
cns_8t_result_t cns_8t_analyzer_set_context(cns_8t_analyzer_t* analyzer,
                                             cns_8t_context_t* context);

cns_8t_result_t cns_8t_analyzer_set_processor(cns_8t_analyzer_t* analyzer,
                                               cns_8t_processor_t* processor);

cns_8t_result_t cns_8t_analyzer_set_scheduler(cns_8t_analyzer_t* analyzer,
                                               cns_8t_scheduler_t* scheduler);

// Analysis control
cns_8t_result_t cns_8t_analyzer_start_monitoring(cns_8t_analyzer_t* analyzer);

cns_8t_result_t cns_8t_analyzer_stop_monitoring(cns_8t_analyzer_t* analyzer);

cns_8t_result_t cns_8t_analyzer_run_analysis(cns_8t_analyzer_t* analyzer,
                                              uint32_t analysis_types,
                                              cns_8t_analysis_report_t** report);

// Specific analysis functions
cns_8t_result_t cns_8t_analyzer_profile_performance(cns_8t_analyzer_t* analyzer,
                                                     cns_8t_performance_profile_t* profile);

cns_8t_result_t cns_8t_analyzer_analyze_memory(cns_8t_analyzer_t* analyzer,
                                                cns_8t_memory_analysis_t* analysis);

cns_8t_result_t cns_8t_analyzer_analyze_cache(cns_8t_analyzer_t* analyzer,
                                               cns_8t_cache_analysis_t* analysis);

cns_8t_result_t cns_8t_analyzer_analyze_simd(cns_8t_analyzer_t* analyzer,
                                              cns_8t_simd_analysis_t* analysis);

cns_8t_result_t cns_8t_analyzer_analyze_scheduling(cns_8t_analyzer_t* analyzer,
                                                    cns_8t_scheduling_analysis_t* analysis);

cns_8t_result_t cns_8t_analyzer_analyze_numerical(cns_8t_analyzer_t* analyzer,
                                                   cns_8t_numerical_analysis_t* analysis);

cns_8t_result_t cns_8t_analyzer_identify_bottlenecks(cns_8t_analyzer_t* analyzer,
                                                      cns_8t_bottleneck_analysis_t* analysis);

cns_8t_result_t cns_8t_analyzer_predict_performance(cns_8t_analyzer_t* analyzer,
                                                     const void* workload_description,
                                                     cns_8t_prediction_analysis_t* prediction);

// Report management
cns_8t_result_t cns_8t_analyzer_generate_report(cns_8t_analyzer_t* analyzer,
                                                 cns_8t_analysis_report_t** report);

cns_8t_result_t cns_8t_analyzer_export_report(cns_8t_analyzer_t* analyzer,
                                               const cns_8t_analysis_report_t* report,
                                               const char* filename);

cns_8t_result_t cns_8t_analyzer_compare_reports(cns_8t_analyzer_t* analyzer,
                                                 const cns_8t_analysis_report_t* report1,
                                                 const cns_8t_analysis_report_t* report2,
                                                 cns_8t_analysis_report_t** comparison);

// Optimization suggestions
cns_8t_result_t cns_8t_analyzer_get_recommendations(cns_8t_analyzer_t* analyzer,
                                                     const cns_8t_analysis_report_t* report,
                                                     const char*** recommendations,
                                                     uint32_t* count);

cns_8t_result_t cns_8t_analyzer_apply_optimization(cns_8t_analyzer_t* analyzer,
                                                    const char* optimization_name);

// Historical analysis
cns_8t_result_t cns_8t_analyzer_get_historical_trend(cns_8t_analyzer_t* analyzer,
                                                      const char* metric_name,
                                                      double** values,
                                                      cns_tick_t** timestamps,
                                                      uint32_t* count);

// Utility functions
double cns_8t_analyzer_calculate_efficiency(const cns_8t_performance_profile_t* profile);
double cns_8t_analyzer_calculate_scalability(const cns_8t_analysis_report_t* report);
const char* cns_8t_analyzer_get_performance_grade(double score);

#ifdef __cplusplus
}
#endif

#endif // CNS_8T_ANALYZER_H