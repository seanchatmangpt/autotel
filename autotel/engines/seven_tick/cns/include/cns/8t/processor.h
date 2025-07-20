#ifndef CNS_8T_PROCESSOR_H
#define CNS_8T_PROCESSOR_H

#include "cns/8t/core.h"
#include "cns/graph.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// 8T PROCESSING PIPELINE - MODULAR COMPUTATION ENGINE
// ============================================================================

// Processing stage types
typedef enum {
    CNS_8T_STAGE_INPUT,        // Input data ingestion
    CNS_8T_STAGE_VALIDATE,     // Data validation
    CNS_8T_STAGE_TRANSFORM,    // Data transformation
    CNS_8T_STAGE_COMPUTE,      // Core computation
    CNS_8T_STAGE_OPTIMIZE,     // Result optimization
    CNS_8T_STAGE_OUTPUT,       // Output generation
    CNS_8T_STAGE_CUSTOM        // Custom user-defined stage
} cns_8t_stage_type_t;

// Processing stage function signature
typedef cns_8t_result_t (*cns_8t_stage_fn_t)(
    cns_8t_context_t* ctx,
    const void* input,
    void* output,
    const cns_8t_numeric_context_t* num_ctx
);

// Processing stage descriptor
typedef struct {
    cns_8t_stage_type_t type;
    const char* name;
    cns_8t_stage_fn_t function;
    uint32_t input_size;
    uint32_t output_size;
    uint32_t scratch_size;      // Temporary memory needed
    uint32_t simd_requirements; // SIMD instruction requirements
    cns_8t_precision_mode_t precision;
    uint32_t max_ticks;         // Maximum ticks allowed
    uint32_t dependencies;      // Bitmask of stage dependencies
} cns_8t_stage_descriptor_t __attribute__((aligned(64)));

// Processing pipeline configuration
typedef struct {
    cns_8t_stage_descriptor_t* stages;
    uint32_t stage_count;
    uint32_t max_parallel_stages;
    uint32_t total_scratch_size;
    cns_8t_memory_pool_t* memory_pool;
    bool enable_pipelining;
    bool enable_vectorization;
    uint32_t cache_line_prefetch; // Lines to prefetch ahead
} cns_8t_pipeline_config_t;

// Processing context with pipeline state
struct cns_8t_processor {
    cns_8t_context_t* base_context;
    cns_8t_pipeline_config_t config;
    
    // Pipeline execution state
    uint32_t current_stage;
    uint32_t completed_stages;   // Bitmask of completed stages
    void** stage_buffers;        // Input/output buffers for each stage
    void* scratch_memory;        // Shared scratch space
    
    // Performance tracking per stage
    cns_8t_perf_metrics_t* stage_metrics;
    cns_8t_perf_metrics_t total_metrics;
    
    // SIMD optimization state
    cns_8t_simd_ops_t simd_ops;
    uint32_t simd_flags;
    
    // Error handling
    cns_8t_error_context_t last_error;
    uint32_t error_recovery_mode;
} __attribute__((aligned(64)));

// ============================================================================
// 8T NUMERICAL ALGORITHMS
// ============================================================================

// Algorithm types for 8T processing
typedef enum {
    CNS_8T_ALGO_LINEAR_ALGEBRA,  // Matrix operations, solving systems
    CNS_8T_ALGO_OPTIMIZATION,    // Optimization algorithms
    CNS_8T_ALGO_STATISTICS,      // Statistical computations
    CNS_8T_ALGO_SIGNAL_PROCESSING, // DSP algorithms
    CNS_8T_ALGO_GRAPH_THEORY,    // Graph algorithms
    CNS_8T_ALGO_NUMERICAL_INTEGRATION, // Integration methods
    CNS_8T_ALGO_ROOT_FINDING,    // Root finding algorithms
    CNS_8T_ALGO_INTERPOLATION    // Interpolation methods
} cns_8t_algorithm_type_t;

// Algorithm descriptor
typedef struct {
    cns_8t_algorithm_type_t type;
    const char* name;
    const char* description;
    cns_8t_precision_mode_t min_precision;
    cns_8t_precision_mode_t max_precision;
    uint32_t simd_requirements;
    uint32_t memory_requirements;
    uint32_t typical_iterations;
    cns_8t_stage_fn_t implementation;
} cns_8t_algorithm_descriptor_t;

// Matrix operations for SIMD processing
typedef struct {
    double* data;
    uint32_t rows;
    uint32_t cols;
    uint32_t stride;  // For memory layout optimization
    uint32_t flags;   // Symmetric, triangular, etc.
} cns_8t_matrix_t __attribute__((aligned(32)));

// Vector operations for SIMD processing
typedef struct {
    double* data;
    uint32_t size;
    uint32_t stride;
    uint32_t flags;
} cns_8t_vector_t __attribute__((aligned(32)));

// ============================================================================
// 8T SPECIALIZED NUMERICAL FUNCTIONS
// ============================================================================

// High-precision arithmetic operations
cns_8t_result_t cns_8t_quad_add(const cns_8t_quad_precision_t* a,
                                 const cns_8t_quad_precision_t* b,
                                 cns_8t_quad_precision_t* result);

cns_8t_result_t cns_8t_quad_mul(const cns_8t_quad_precision_t* a,
                                 const cns_8t_quad_precision_t* b,
                                 cns_8t_quad_precision_t* result);

cns_8t_result_t cns_8t_quad_div(const cns_8t_quad_precision_t* a,
                                 const cns_8t_quad_precision_t* b,
                                 cns_8t_quad_precision_t* result);

// SIMD matrix operations
cns_8t_result_t cns_8t_matrix_multiply_simd(const cns_8t_matrix_t* a,
                                             const cns_8t_matrix_t* b,
                                             cns_8t_matrix_t* result,
                                             const cns_8t_numeric_context_t* ctx);

cns_8t_result_t cns_8t_matrix_transpose_simd(const cns_8t_matrix_t* input,
                                              cns_8t_matrix_t* output);

cns_8t_result_t cns_8t_matrix_invert_simd(const cns_8t_matrix_t* input,
                                           cns_8t_matrix_t* output,
                                           const cns_8t_numeric_context_t* ctx);

// SIMD vector operations
cns_8t_result_t cns_8t_vector_dot_simd(const cns_8t_vector_t* a,
                                        const cns_8t_vector_t* b,
                                        double* result);

cns_8t_result_t cns_8t_vector_norm_simd(const cns_8t_vector_t* input,
                                         double* result);

cns_8t_result_t cns_8t_vector_cross_simd(const cns_8t_vector_t* a,
                                          const cns_8t_vector_t* b,
                                          cns_8t_vector_t* result);

// Statistical functions with SIMD optimization
cns_8t_result_t cns_8t_stats_mean_simd(const double* data, size_t count, double* result);
cns_8t_result_t cns_8t_stats_variance_simd(const double* data, size_t count, double* result);
cns_8t_result_t cns_8t_stats_correlation_simd(const double* x, const double* y, 
                                               size_t count, double* result);

// ============================================================================
// 8T PROCESSOR API
// ============================================================================

// Processor lifecycle
cns_8t_result_t cns_8t_processor_create(const cns_8t_pipeline_config_t* config,
                                         cns_8t_processor_t** processor);

cns_8t_result_t cns_8t_processor_destroy(cns_8t_processor_t* processor);

// Pipeline management
cns_8t_result_t cns_8t_processor_add_stage(cns_8t_processor_t* processor,
                                            const cns_8t_stage_descriptor_t* stage);

cns_8t_result_t cns_8t_processor_remove_stage(cns_8t_processor_t* processor,
                                               uint32_t stage_index);

cns_8t_result_t cns_8t_processor_optimize_pipeline(cns_8t_processor_t* processor);

// Execution control
cns_8t_result_t cns_8t_processor_execute(cns_8t_processor_t* processor,
                                          const void* input,
                                          void* output);

cns_8t_result_t cns_8t_processor_execute_async(cns_8t_processor_t* processor,
                                                const void* input,
                                                void* output,
                                                uint32_t* completion_token);

cns_8t_result_t cns_8t_processor_wait(cns_8t_processor_t* processor,
                                       uint32_t completion_token);

// Performance analysis
cns_8t_result_t cns_8t_processor_get_metrics(cns_8t_processor_t* processor,
                                              cns_8t_perf_metrics_t* metrics);

cns_8t_result_t cns_8t_processor_reset_metrics(cns_8t_processor_t* processor);

// Algorithm registration
cns_8t_result_t cns_8t_processor_register_algorithm(cns_8t_processor_t* processor,
                                                     const cns_8t_algorithm_descriptor_t* algo);

cns_8t_result_t cns_8t_processor_execute_algorithm(cns_8t_processor_t* processor,
                                                    const char* algorithm_name,
                                                    const void* input,
                                                    void* output,
                                                    const cns_8t_numeric_context_t* num_ctx);

// ============================================================================
// 8T BUILT-IN PROCESSING STAGES
// ============================================================================

// Standard validation stage
cns_8t_result_t cns_8t_stage_validate_input(cns_8t_context_t* ctx,
                                             const void* input,
                                             void* output,
                                             const cns_8t_numeric_context_t* num_ctx);

// SIMD-optimized transformation stage
cns_8t_result_t cns_8t_stage_simd_transform(cns_8t_context_t* ctx,
                                             const void* input,
                                             void* output,
                                             const cns_8t_numeric_context_t* num_ctx);

// High-precision computation stage
cns_8t_result_t cns_8t_stage_precise_compute(cns_8t_context_t* ctx,
                                              const void* input,
                                              void* output,
                                              const cns_8t_numeric_context_t* num_ctx);

// Result optimization stage
cns_8t_result_t cns_8t_stage_optimize_result(cns_8t_context_t* ctx,
                                              const void* input,
                                              void* output,
                                              const cns_8t_numeric_context_t* num_ctx);

#ifdef __cplusplus
}
#endif

#endif // CNS_8T_PROCESSOR_H