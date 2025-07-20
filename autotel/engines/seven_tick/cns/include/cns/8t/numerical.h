#ifndef CNS_8T_NUMERICAL_H
#define CNS_8T_NUMERICAL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <float.h>
#include <math.h>
#ifdef __x86_64__
#include <immintrin.h>
#elif defined(__aarch64__)
#include <arm_neon.h>
#endif
#include "cns/types.h"
#include "cns/8t/core.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// 8T NUMERICAL PRECISION ENGINE - ERROR BOUND TRACKING
// ============================================================================

// Machine epsilon values for different precisions
#define CNS_8T_EPSILON_F32    FLT_EPSILON     // ~1.19e-7
#define CNS_8T_EPSILON_F64    DBL_EPSILON     // ~2.22e-16
#define CNS_8T_EPSILON_F128   1.93e-34L       // Quad precision epsilon

// Condition number thresholds
#define CNS_8T_COND_WELL     10.0
#define CNS_8T_COND_MODERATE 1000.0
#define CNS_8T_COND_ILL      1e10

// FMA availability flags
#ifdef __FMA__
#define CNS_8T_HAS_FMA 1
#else
#define CNS_8T_HAS_FMA 0
#endif

// ============================================================================
// ERROR BOUND TRACKING STRUCTURES
// ============================================================================

// Forward error bound structure
typedef struct {
    double absolute_error;     // Absolute error bound
    double relative_error;     // Relative error bound
    double condition_number;   // Condition number of operation
    uint32_t ulp_error;       // Units in Last Place error
    uint32_t operation_count; // Number of operations performed
    bool overflow_risk;       // Risk of overflow
    bool underflow_risk;      // Risk of underflow
    bool cancellation_risk;   // Risk of catastrophic cancellation
} cns_8t_error_bound_t;

// Numerical stability classification
typedef enum {
    CNS_8T_STABLE_FORWARD,      // Forward stable algorithm
    CNS_8T_STABLE_BACKWARD,     // Backward stable algorithm
    CNS_8T_STABLE_MIXED,        // Mixed stability
    CNS_8T_UNSTABLE,            // Numerically unstable
    CNS_8T_CONDITIONALLY_STABLE // Stable under certain conditions
} cns_8t_stability_t;

// Precision-tracked scalar type
typedef struct {
    double value;                    // The actual value
    cns_8t_error_bound_t bounds;    // Error bounds
    cns_8t_stability_t stability;   // Numerical stability
} cns_8t_tracked_scalar_t;

// Precision-tracked vector type
typedef struct {
    double* values;                  // Vector values
    size_t size;                     // Vector size
    cns_8t_error_bound_t* bounds;   // Per-element bounds
    double norm_error;               // Norm error bound
    double condition_number;         // Vector condition number
} cns_8t_tracked_vector_t;

// Precision-tracked matrix type
typedef struct {
    double* values;                  // Matrix values (row-major)
    size_t rows;
    size_t cols;
    cns_8t_error_bound_t* bounds;   // Per-element bounds
    double frobenius_error;          // Frobenius norm error
    double spectral_radius;          // Spectral radius
    double condition_number;         // Matrix condition number
    bool is_symmetric;               // Symmetry flag for optimizations
} cns_8t_tracked_matrix_t;

// ============================================================================
// SIMD NUMERICAL OPERATIONS WITH ERROR TRACKING
// ============================================================================

// SIMD operation with error bound propagation
typedef struct {
    cns_8t_simd_f64_t result;       // SIMD result
    cns_8t_error_bound_t bounds[4]; // Error bounds per lane
    uint32_t simd_flags;            // SIMD status flags
} cns_8t_simd_tracked_f64_t;

// Compensated summation state (Kahan summation)
typedef struct {
    double sum;                      // Running sum
    double compensation;             // Compensation term
    cns_8t_error_bound_t bounds;    // Accumulated error bounds
    uint64_t count;                  // Number of values summed
} cns_8t_compensated_sum_t;

// Interval arithmetic type
typedef struct {
    double lower;                    // Lower bound
    double upper;                    // Upper bound
    bool is_exact;                   // Exact value flag
} cns_8t_interval_t;

// ============================================================================
// CONDITION NUMBER ANALYSIS
// ============================================================================

// Condition number computation context
typedef struct {
    double (*scalar_condition)(double x, void* params);
    double (*vector_condition)(const double* x, size_t n, void* params);
    double (*matrix_condition)(const double* A, size_t m, size_t n, void* params);
    void* user_params;
} cns_8t_condition_context_t;

// Sensitivity analysis result
typedef struct {
    double* sensitivities;           // Sensitivity coefficients
    double max_sensitivity;          // Maximum sensitivity
    double mean_sensitivity;         // Mean sensitivity
    size_t dimension;                // Problem dimension
} cns_8t_sensitivity_t;

// ============================================================================
// FMA AND VECTORIZED OPERATIONS
// ============================================================================

// FMA operation types
typedef enum {
    CNS_8T_FMA_MULTIPLY_ADD,        // a * b + c
    CNS_8T_FMA_MULTIPLY_SUB,        // a * b - c
    CNS_8T_FMA_NEG_MULTIPLY_ADD,    // -(a * b) + c
    CNS_8T_FMA_NEG_MULTIPLY_SUB     // -(a * b) - c
} cns_8t_fma_op_t;

// Vectorized operation descriptor
typedef struct {
    size_t vector_length;            // Length of vectors
    size_t simd_width;               // SIMD width in elements
    bool use_fma;                    // Use FMA instructions
    bool track_errors;               // Track error bounds
    cns_8t_precision_mode_t mode;   // Precision mode
} cns_8t_vector_op_desc_t;

// ============================================================================
// NUMERICAL ALGORITHMS WITH PROVEN BOUNDS
// ============================================================================

// Iterative refinement context
typedef struct {
    uint32_t max_iterations;         // Maximum refinement iterations
    double tolerance;                // Convergence tolerance
    bool use_mixed_precision;        // Use mixed precision refinement
    cns_8t_error_bound_t* history;  // Error bound history
} cns_8t_refinement_context_t;

// Adaptive precision context
typedef struct {
    double target_accuracy;          // Target accuracy
    cns_8t_precision_mode_t initial_mode;
    bool auto_upgrade_precision;     // Automatically upgrade precision
    uint32_t precision_switches;     // Number of precision changes
} cns_8t_adaptive_context_t;

// ============================================================================
// API FUNCTIONS - BASIC OPERATIONS WITH ERROR TRACKING
// ============================================================================

// Initialize numerical engine
cns_8t_result_t cns_8t_numerical_init(cns_8t_context_t* ctx,
                                       const cns_8t_numeric_context_t* config);

// Tracked scalar operations
cns_8t_result_t cns_8t_add_tracked(const cns_8t_tracked_scalar_t* a,
                                    const cns_8t_tracked_scalar_t* b,
                                    cns_8t_tracked_scalar_t* result);

cns_8t_result_t cns_8t_mul_tracked(const cns_8t_tracked_scalar_t* a,
                                    const cns_8t_tracked_scalar_t* b,
                                    cns_8t_tracked_scalar_t* result);

cns_8t_result_t cns_8t_div_tracked(const cns_8t_tracked_scalar_t* a,
                                    const cns_8t_tracked_scalar_t* b,
                                    cns_8t_tracked_scalar_t* result);

// FMA operations with error tracking
cns_8t_result_t cns_8t_fma_tracked(const cns_8t_tracked_scalar_t* a,
                                    const cns_8t_tracked_scalar_t* b,
                                    const cns_8t_tracked_scalar_t* c,
                                    cns_8t_fma_op_t op,
                                    cns_8t_tracked_scalar_t* result);

// ============================================================================
// SIMD VECTORIZED OPERATIONS
// ============================================================================

// SIMD vector operations with error bounds
cns_8t_result_t cns_8t_simd_add_f64(const double* a, const double* b,
                                     double* result, size_t count,
                                     cns_8t_error_bound_t* bounds);

cns_8t_result_t cns_8t_simd_mul_f64(const double* a, const double* b,
                                     double* result, size_t count,
                                     cns_8t_error_bound_t* bounds);

cns_8t_result_t cns_8t_simd_fma_f64(const double* a, const double* b,
                                     const double* c, double* result,
                                     size_t count, cns_8t_fma_op_t op,
                                     cns_8t_error_bound_t* bounds);

// Compensated summation
cns_8t_result_t cns_8t_compensated_sum_init(cns_8t_compensated_sum_t* sum);

cns_8t_result_t cns_8t_compensated_sum_add(cns_8t_compensated_sum_t* sum,
                                            double value);

cns_8t_result_t cns_8t_compensated_sum_add_array(cns_8t_compensated_sum_t* sum,
                                                  const double* values,
                                                  size_t count);

cns_8t_result_t cns_8t_compensated_sum_finalize(cns_8t_compensated_sum_t* sum,
                                                 cns_8t_tracked_scalar_t* result);

// ============================================================================
// CONDITION NUMBER ANALYSIS
// ============================================================================

// Compute condition numbers
cns_8t_result_t cns_8t_condition_scalar(double value,
                                         const cns_8t_condition_context_t* ctx,
                                         double* condition);

cns_8t_result_t cns_8t_condition_vector(const cns_8t_tracked_vector_t* vec,
                                         const cns_8t_condition_context_t* ctx,
                                         double* condition);

cns_8t_result_t cns_8t_condition_matrix(const cns_8t_tracked_matrix_t* mat,
                                         double* condition);

// Sensitivity analysis
cns_8t_result_t cns_8t_sensitivity_analyze(const double* parameters,
                                            size_t param_count,
                                            double (*function)(const double*, size_t),
                                            cns_8t_sensitivity_t* result);

// ============================================================================
// INTERVAL ARITHMETIC
// ============================================================================

// Interval operations
cns_8t_result_t cns_8t_interval_add(const cns_8t_interval_t* a,
                                     const cns_8t_interval_t* b,
                                     cns_8t_interval_t* result);

cns_8t_result_t cns_8t_interval_mul(const cns_8t_interval_t* a,
                                     const cns_8t_interval_t* b,
                                     cns_8t_interval_t* result);

cns_8t_result_t cns_8t_interval_contains(const cns_8t_interval_t* interval,
                                          double value,
                                          bool* contains);

// ============================================================================
// ERROR BOUND PROPAGATION
// ============================================================================

// Propagate error bounds through operations
cns_8t_result_t cns_8t_propagate_add(const cns_8t_error_bound_t* a,
                                      const cns_8t_error_bound_t* b,
                                      cns_8t_error_bound_t* result);

cns_8t_result_t cns_8t_propagate_mul(const cns_8t_error_bound_t* a,
                                      const cns_8t_error_bound_t* b,
                                      double val_a, double val_b,
                                      cns_8t_error_bound_t* result);

cns_8t_result_t cns_8t_propagate_function(const cns_8t_error_bound_t* input,
                                           double input_value,
                                           double derivative,
                                           cns_8t_error_bound_t* result);

// ============================================================================
// ADAPTIVE PRECISION ALGORITHMS
// ============================================================================

// Matrix multiply with adaptive precision
cns_8t_result_t cns_8t_matrix_mul_adaptive(const cns_8t_tracked_matrix_t* A,
                                            const cns_8t_tracked_matrix_t* B,
                                            cns_8t_tracked_matrix_t* C,
                                            const cns_8t_adaptive_context_t* ctx);

// Linear solve with iterative refinement
cns_8t_result_t cns_8t_solve_refined(const cns_8t_tracked_matrix_t* A,
                                      const cns_8t_tracked_vector_t* b,
                                      cns_8t_tracked_vector_t* x,
                                      const cns_8t_refinement_context_t* ctx);

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

// Create tracked types
cns_8t_result_t cns_8t_tracked_scalar_create(double value,
                                              cns_8t_tracked_scalar_t* scalar);

cns_8t_result_t cns_8t_tracked_vector_create(size_t size,
                                              cns_8t_tracked_vector_t* vector);

cns_8t_result_t cns_8t_tracked_matrix_create(size_t rows, size_t cols,
                                              cns_8t_tracked_matrix_t* matrix);

// Destroy tracked types
void cns_8t_tracked_vector_destroy(cns_8t_tracked_vector_t* vector);
void cns_8t_tracked_matrix_destroy(cns_8t_tracked_matrix_t* matrix);

// Error bound queries
bool cns_8t_is_error_acceptable(const cns_8t_error_bound_t* bounds,
                                 double tolerance);

double cns_8t_get_relative_error(const cns_8t_error_bound_t* bounds);

// Stability analysis
cns_8t_stability_t cns_8t_analyze_stability(const cns_8t_error_bound_t* bounds,
                                             uint32_t operation_count);

// ============================================================================
// PERFORMANCE MACROS
// ============================================================================

// Check if FMA is available and beneficial
#define CNS_8T_USE_FMA(op_count) \
    (CNS_8T_HAS_FMA && (op_count) >= 2)

// SIMD alignment check
#define CNS_8T_IS_SIMD_ALIGNED(ptr) \
    (((uintptr_t)(ptr) & (CNS_8T_SIMD_WIDTH - 1)) == 0)

// Error bound initialization
#define CNS_8T_INIT_ERROR_BOUND(bound) \
    do { \
        (bound)->absolute_error = 0.0; \
        (bound)->relative_error = 0.0; \
        (bound)->condition_number = 1.0; \
        (bound)->ulp_error = 0; \
        (bound)->operation_count = 0; \
        (bound)->overflow_risk = false; \
        (bound)->underflow_risk = false; \
        (bound)->cancellation_risk = false; \
    } while(0)

#ifdef __cplusplus
}
#endif

#endif // CNS_8T_NUMERICAL_H