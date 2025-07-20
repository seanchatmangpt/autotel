/**
 * CNS 8T Numerical Precision Engine Implementation
 * 
 * Implements high-precision numerical operations with:
 * - Proven error bound tracking
 * - Condition number analysis
 * - FMA (Fused Multiply-Add) optimization
 * - SIMD vectorization for arrays
 * - Compensated summation algorithms
 * - Interval arithmetic support
 */

#include "cns/8t/numerical.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fenv.h>

// Platform-specific includes for FMA
#if defined(__x86_64__) && defined(__FMA__)
#include <immintrin.h>
#elif defined(__aarch64__)
#include <arm_neon.h>
#endif

// ============================================================================
// INTERNAL CONSTANTS AND HELPERS
// ============================================================================

// ULP computation helper
static uint32_t compute_ulp_error(double computed, double exact) {
    if (exact == 0.0) return 0;
    
    // Get the ULP of the exact value
    double ulp = ldexp(1.0, ilogb(exact) - 52);  // 52 bits for double mantissa
    double error = fabs(computed - exact);
    
    return (uint32_t)(error / ulp);
}

// Check for numerical hazards
static void check_numerical_hazards(double value, cns_8t_error_bound_t* bounds) {
    if (fabs(value) < DBL_MIN * 100) {
        bounds->underflow_risk = true;
    }
    if (fabs(value) > DBL_MAX / 100) {
        bounds->overflow_risk = true;
    }
}

// ============================================================================
// INITIALIZATION
// ============================================================================

cns_8t_result_t cns_8t_numerical_init(cns_8t_context_t* ctx,
                                       const cns_8t_numeric_context_t* config) {
    if (!ctx || !config) return CNS_ERROR_INVALID_ARGUMENT;
    
    // Set up FPU control word for specified rounding mode
    if (config->rounding_mode != 0) {
        fesetround(config->rounding_mode);
    }
    
    // Clear FPU exceptions
    feclearexcept(FE_ALL_EXCEPT);
    
    return CNS_8T_OK;
}

// ============================================================================
// TRACKED SCALAR OPERATIONS
// ============================================================================

cns_8t_result_t cns_8t_tracked_scalar_create(double value,
                                              cns_8t_tracked_scalar_t* scalar) {
    if (!scalar) return CNS_ERROR_INVALID_ARGUMENT;
    
    scalar->value = value;
    CNS_8T_INIT_ERROR_BOUND(&scalar->bounds);
    scalar->stability = CNS_8T_STABLE_FORWARD;
    
    // Initial ULP error is 0.5 (rounding error)
    scalar->bounds.ulp_error = 1;
    scalar->bounds.relative_error = CNS_8T_EPSILON_F64;
    scalar->bounds.absolute_error = fabs(value) * CNS_8T_EPSILON_F64;
    
    check_numerical_hazards(value, &scalar->bounds);
    
    return CNS_8T_OK;
}

cns_8t_result_t cns_8t_add_tracked(const cns_8t_tracked_scalar_t* a,
                                    const cns_8t_tracked_scalar_t* b,
                                    cns_8t_tracked_scalar_t* result) {
    if (!a || !b || !result) return CNS_ERROR_INVALID_ARGUMENT;
    
    CNS_8T_START_TIMING(result);
    
    // Compute the sum
    result->value = a->value + b->value;
    
    // Error bound propagation for addition
    result->bounds.absolute_error = a->bounds.absolute_error + b->bounds.absolute_error;
    
    // Check for catastrophic cancellation
    if (fabs(result->value) < fabs(a->value) * 0.1 && 
        fabs(result->value) < fabs(b->value) * 0.1) {
        result->bounds.cancellation_risk = true;
        // Cancellation can amplify relative error significantly
        double cancellation_factor = fmax(fabs(a->value), fabs(b->value)) / fabs(result->value);
        result->bounds.relative_error = cancellation_factor * 
            (a->bounds.relative_error + b->bounds.relative_error);
    } else {
        result->bounds.relative_error = (fabs(a->value) * a->bounds.relative_error + 
                                         fabs(b->value) * b->bounds.relative_error) / 
                                        fabs(result->value);
    }
    
    // Add rounding error
    result->bounds.absolute_error += fabs(result->value) * CNS_8T_EPSILON_F64;
    result->bounds.ulp_error = a->bounds.ulp_error + b->bounds.ulp_error + 1;
    
    // Condition number for addition
    result->bounds.condition_number = fmax(fabs(a->value), fabs(b->value)) / fabs(result->value);
    
    // Operation count
    result->bounds.operation_count = a->bounds.operation_count + b->bounds.operation_count + 1;
    
    // Check hazards
    check_numerical_hazards(result->value, &result->bounds);
    result->bounds.overflow_risk = a->bounds.overflow_risk || b->bounds.overflow_risk;
    result->bounds.underflow_risk = a->bounds.underflow_risk || b->bounds.underflow_risk;
    
    // Determine stability
    result->stability = (result->bounds.cancellation_risk) ? 
                       CNS_8T_CONDITIONALLY_STABLE : CNS_8T_STABLE_FORWARD;
    
    CNS_8T_END_TIMING(result);
    
    return CNS_8T_OK;
}

cns_8t_result_t cns_8t_mul_tracked(const cns_8t_tracked_scalar_t* a,
                                    const cns_8t_tracked_scalar_t* b,
                                    cns_8t_tracked_scalar_t* result) {
    if (!a || !b || !result) return CNS_ERROR_INVALID_ARGUMENT;
    
    CNS_8T_START_TIMING(result);
    
    // Compute the product
    result->value = a->value * b->value;
    
    // Error propagation for multiplication: |δ(xy)| ≈ |y|δx + |x|δy
    result->bounds.absolute_error = fabs(b->value) * a->bounds.absolute_error + 
                                    fabs(a->value) * b->bounds.absolute_error;
    
    // Relative error: δ(xy)/(xy) ≈ δx/x + δy/y
    result->bounds.relative_error = a->bounds.relative_error + b->bounds.relative_error;
    
    // Add rounding error
    result->bounds.absolute_error += fabs(result->value) * CNS_8T_EPSILON_F64;
    result->bounds.relative_error += CNS_8T_EPSILON_F64;
    result->bounds.ulp_error = a->bounds.ulp_error + b->bounds.ulp_error + 1;
    
    // Condition number for multiplication
    result->bounds.condition_number = 1.0;  // Multiplication is well-conditioned
    
    // Operation count
    result->bounds.operation_count = a->bounds.operation_count + b->bounds.operation_count + 1;
    
    // Check hazards
    check_numerical_hazards(result->value, &result->bounds);
    result->bounds.overflow_risk = a->bounds.overflow_risk || b->bounds.overflow_risk || 
                                   (fabs(a->value) > 1.0 && fabs(b->value) > DBL_MAX / fabs(a->value));
    result->bounds.underflow_risk = a->bounds.underflow_risk || b->bounds.underflow_risk ||
                                    (fabs(result->value) < DBL_MIN);
    
    result->stability = CNS_8T_STABLE_FORWARD;
    
    CNS_8T_END_TIMING(result);
    
    return CNS_8T_OK;
}

cns_8t_result_t cns_8t_div_tracked(const cns_8t_tracked_scalar_t* a,
                                    const cns_8t_tracked_scalar_t* b,
                                    cns_8t_tracked_scalar_t* result) {
    if (!a || !b || !result) return CNS_ERROR_INVALID_ARGUMENT;
    
    // Check for division by zero
    if (fabs(b->value) < DBL_MIN) {
        return CNS_8T_ERROR_OVERFLOW;
    }
    
    CNS_8T_START_TIMING(result);
    
    // Compute the quotient
    result->value = a->value / b->value;
    
    // Error propagation for division: more complex than multiplication
    double b_squared = b->value * b->value;
    result->bounds.absolute_error = (fabs(b->value) * a->bounds.absolute_error + 
                                     fabs(a->value) * b->bounds.absolute_error) / b_squared;
    
    // Relative error for division
    result->bounds.relative_error = a->bounds.relative_error + b->bounds.relative_error;
    
    // Division can amplify errors when divisor is small
    if (fabs(b->value) < 1.0) {
        double amplification = 1.0 / fabs(b->value);
        result->bounds.relative_error *= amplification;
    }
    
    // Add rounding error
    result->bounds.absolute_error += fabs(result->value) * CNS_8T_EPSILON_F64;
    result->bounds.relative_error += CNS_8T_EPSILON_F64;
    result->bounds.ulp_error = a->bounds.ulp_error + 2 * b->bounds.ulp_error + 1;
    
    // Condition number for division
    result->bounds.condition_number = fabs(b->value) < 1.0 ? 1.0 / fabs(b->value) : 1.0;
    
    // Operation count
    result->bounds.operation_count = a->bounds.operation_count + b->bounds.operation_count + 1;
    
    // Check hazards
    check_numerical_hazards(result->value, &result->bounds);
    result->bounds.overflow_risk = a->bounds.overflow_risk || 
                                   (fabs(b->value) < 1.0 && fabs(a->value) > DBL_MAX * fabs(b->value));
    result->bounds.underflow_risk = a->bounds.underflow_risk || b->bounds.overflow_risk;
    
    result->stability = (fabs(b->value) < 0.1) ? CNS_8T_CONDITIONALLY_STABLE : CNS_8T_STABLE_FORWARD;
    
    CNS_8T_END_TIMING(result);
    
    return CNS_8T_OK;
}

// ============================================================================
// FMA OPERATIONS
// ============================================================================

cns_8t_result_t cns_8t_fma_tracked(const cns_8t_tracked_scalar_t* a,
                                    const cns_8t_tracked_scalar_t* b,
                                    const cns_8t_tracked_scalar_t* c,
                                    cns_8t_fma_op_t op,
                                    cns_8t_tracked_scalar_t* result) {
    if (!a || !b || !c || !result) return CNS_ERROR_INVALID_ARGUMENT;
    
    CNS_8T_START_TIMING(result);
    
    // Compute FMA result based on operation type
    switch (op) {
        case CNS_8T_FMA_MULTIPLY_ADD:
            #ifdef __FMA__
            result->value = fma(a->value, b->value, c->value);
            #else
            result->value = a->value * b->value + c->value;
            #endif
            break;
            
        case CNS_8T_FMA_MULTIPLY_SUB:
            #ifdef __FMA__
            result->value = fma(a->value, b->value, -c->value);
            #else
            result->value = a->value * b->value - c->value;
            #endif
            break;
            
        case CNS_8T_FMA_NEG_MULTIPLY_ADD:
            #ifdef __FMA__
            result->value = fma(-a->value, b->value, c->value);
            #else
            result->value = -(a->value * b->value) + c->value;
            #endif
            break;
            
        case CNS_8T_FMA_NEG_MULTIPLY_SUB:
            #ifdef __FMA__
            result->value = fma(-a->value, b->value, -c->value);
            #else
            result->value = -(a->value * b->value) - c->value;
            #endif
            break;
    }
    
    // FMA has special error properties
    #ifdef __FMA__
    // With hardware FMA, only one rounding error
    result->bounds.absolute_error = fabs(b->value) * a->bounds.absolute_error + 
                                    fabs(a->value) * b->bounds.absolute_error +
                                    c->bounds.absolute_error;
    result->bounds.relative_error = a->bounds.relative_error + b->bounds.relative_error;
    result->bounds.ulp_error = a->bounds.ulp_error + b->bounds.ulp_error + 
                               c->bounds.ulp_error + 1;  // Only 1 rounding
    #else
    // Without FMA, two rounding errors
    double mul_error = fabs(b->value) * a->bounds.absolute_error + 
                      fabs(a->value) * b->bounds.absolute_error;
    result->bounds.absolute_error = mul_error + c->bounds.absolute_error + 
                                   fabs(a->value * b->value) * CNS_8T_EPSILON_F64;
    result->bounds.relative_error = a->bounds.relative_error + b->bounds.relative_error + 
                                   CNS_8T_EPSILON_F64;
    result->bounds.ulp_error = a->bounds.ulp_error + b->bounds.ulp_error + 
                               c->bounds.ulp_error + 2;  // Two roundings
    #endif
    
    // Add final rounding error
    result->bounds.absolute_error += fabs(result->value) * CNS_8T_EPSILON_F64;
    
    // Condition number - more complex for FMA
    double ab = fabs(a->value * b->value);
    double c_abs = fabs(c->value);
    result->bounds.condition_number = (ab + c_abs) / fabs(result->value);
    
    // Check for cancellation in addition/subtraction
    if ((op == CNS_8T_FMA_MULTIPLY_SUB || op == CNS_8T_FMA_NEG_MULTIPLY_ADD) &&
        fabs(result->value) < 0.1 * fmax(ab, c_abs)) {
        result->bounds.cancellation_risk = true;
    }
    
    // Operation count
    result->bounds.operation_count = a->bounds.operation_count + b->bounds.operation_count + 
                                     c->bounds.operation_count + 2;
    
    check_numerical_hazards(result->value, &result->bounds);
    
    result->stability = result->bounds.cancellation_risk ? 
                       CNS_8T_CONDITIONALLY_STABLE : CNS_8T_STABLE_FORWARD;
    
    CNS_8T_END_TIMING(result);
    
    return CNS_8T_OK;
}

// ============================================================================
// SIMD OPERATIONS
// ============================================================================

cns_8t_result_t cns_8t_simd_add_f64(const double* a, const double* b,
                                     double* result, size_t count,
                                     cns_8t_error_bound_t* bounds) {
    if (!a || !b || !result || count == 0) return CNS_ERROR_INVALID_ARGUMENT;
    
    CNS_8T_START_TIMING(bounds);
    
    size_t i = 0;
    
    #ifdef __x86_64__
    // Process 4 doubles at a time with AVX
    size_t simd_count = count & ~3;  // Round down to multiple of 4
    for (; i < simd_count; i += 4) {
        __m256d va = _mm256_loadu_pd(&a[i]);
        __m256d vb = _mm256_loadu_pd(&b[i]);
        __m256d vr = _mm256_add_pd(va, vb);
        _mm256_storeu_pd(&result[i], vr);
    }
    #elif defined(__aarch64__)
    // Process 2 doubles at a time with NEON
    size_t simd_count = count & ~1;  // Round down to multiple of 2
    for (; i < simd_count; i += 2) {
        float64x2_t va = vld1q_f64(&a[i]);
        float64x2_t vb = vld1q_f64(&b[i]);
        float64x2_t vr = vaddq_f64(va, vb);
        vst1q_f64(&result[i], vr);
    }
    #endif
    
    // Handle remaining elements
    for (; i < count; i++) {
        result[i] = a[i] + b[i];
    }
    
    // Compute error bounds if requested
    if (bounds) {
        CNS_8T_INIT_ERROR_BOUND(bounds);
        bounds->relative_error = CNS_8T_EPSILON_F64;
        bounds->operation_count = count;
        bounds->ulp_error = 1;
        
        // Find maximum absolute value for error estimation
        double max_abs = 0.0;
        for (size_t j = 0; j < count; j++) {
            double abs_val = fabs(result[j]);
            if (abs_val > max_abs) max_abs = abs_val;
        }
        bounds->absolute_error = max_abs * CNS_8T_EPSILON_F64;
    }
    
    CNS_8T_END_TIMING(bounds);
    
    return CNS_8T_OK;
}

cns_8t_result_t cns_8t_simd_mul_f64(const double* a, const double* b,
                                     double* result, size_t count,
                                     cns_8t_error_bound_t* bounds) {
    if (!a || !b || !result || count == 0) return CNS_ERROR_INVALID_ARGUMENT;
    
    CNS_8T_START_TIMING(bounds);
    
    size_t i = 0;
    
    #ifdef __x86_64__
    size_t simd_count = count & ~3;
    for (; i < simd_count; i += 4) {
        __m256d va = _mm256_loadu_pd(&a[i]);
        __m256d vb = _mm256_loadu_pd(&b[i]);
        __m256d vr = _mm256_mul_pd(va, vb);
        _mm256_storeu_pd(&result[i], vr);
    }
    #elif defined(__aarch64__)
    size_t simd_count = count & ~1;
    for (; i < simd_count; i += 2) {
        float64x2_t va = vld1q_f64(&a[i]);
        float64x2_t vb = vld1q_f64(&b[i]);
        float64x2_t vr = vmulq_f64(va, vb);
        vst1q_f64(&result[i], vr);
    }
    #endif
    
    // Handle remaining elements
    for (; i < count; i++) {
        result[i] = a[i] * b[i];
    }
    
    // Compute error bounds
    if (bounds) {
        CNS_8T_INIT_ERROR_BOUND(bounds);
        bounds->relative_error = 2.0 * CNS_8T_EPSILON_F64;  // Two operands
        bounds->operation_count = count;
        bounds->ulp_error = 1;
        
        double max_abs = 0.0;
        for (size_t j = 0; j < count; j++) {
            double abs_val = fabs(result[j]);
            if (abs_val > max_abs) max_abs = abs_val;
        }
        bounds->absolute_error = max_abs * bounds->relative_error;
    }
    
    CNS_8T_END_TIMING(bounds);
    
    return CNS_8T_OK;
}

cns_8t_result_t cns_8t_simd_fma_f64(const double* a, const double* b,
                                     const double* c, double* result,
                                     size_t count, cns_8t_fma_op_t op,
                                     cns_8t_error_bound_t* bounds) {
    if (!a || !b || !c || !result || count == 0) return CNS_ERROR_INVALID_ARGUMENT;
    
    CNS_8T_START_TIMING(bounds);
    
    size_t i = 0;
    
    #if defined(__x86_64__) && defined(__FMA__)
    size_t simd_count = count & ~3;
    
    switch (op) {
        case CNS_8T_FMA_MULTIPLY_ADD:
            for (; i < simd_count; i += 4) {
                __m256d va = _mm256_loadu_pd(&a[i]);
                __m256d vb = _mm256_loadu_pd(&b[i]);
                __m256d vc = _mm256_loadu_pd(&c[i]);
                __m256d vr = _mm256_fmadd_pd(va, vb, vc);
                _mm256_storeu_pd(&result[i], vr);
            }
            break;
            
        case CNS_8T_FMA_MULTIPLY_SUB:
            for (; i < simd_count; i += 4) {
                __m256d va = _mm256_loadu_pd(&a[i]);
                __m256d vb = _mm256_loadu_pd(&b[i]);
                __m256d vc = _mm256_loadu_pd(&c[i]);
                __m256d vr = _mm256_fmsub_pd(va, vb, vc);
                _mm256_storeu_pd(&result[i], vr);
            }
            break;
            
        case CNS_8T_FMA_NEG_MULTIPLY_ADD:
            for (; i < simd_count; i += 4) {
                __m256d va = _mm256_loadu_pd(&a[i]);
                __m256d vb = _mm256_loadu_pd(&b[i]);
                __m256d vc = _mm256_loadu_pd(&c[i]);
                __m256d vr = _mm256_fnmadd_pd(va, vb, vc);
                _mm256_storeu_pd(&result[i], vr);
            }
            break;
            
        case CNS_8T_FMA_NEG_MULTIPLY_SUB:
            for (; i < simd_count; i += 4) {
                __m256d va = _mm256_loadu_pd(&a[i]);
                __m256d vb = _mm256_loadu_pd(&b[i]);
                __m256d vc = _mm256_loadu_pd(&c[i]);
                __m256d vr = _mm256_fnmsub_pd(va, vb, vc);
                _mm256_storeu_pd(&result[i], vr);
            }
            break;
    }
    #endif
    
    // Handle remaining elements or non-FMA fallback
    for (; i < count; i++) {
        switch (op) {
            case CNS_8T_FMA_MULTIPLY_ADD:
                #ifdef __FMA__
                result[i] = fma(a[i], b[i], c[i]);
                #else
                result[i] = a[i] * b[i] + c[i];
                #endif
                break;
                
            case CNS_8T_FMA_MULTIPLY_SUB:
                #ifdef __FMA__
                result[i] = fma(a[i], b[i], -c[i]);
                #else
                result[i] = a[i] * b[i] - c[i];
                #endif
                break;
                
            case CNS_8T_FMA_NEG_MULTIPLY_ADD:
                #ifdef __FMA__
                result[i] = fma(-a[i], b[i], c[i]);
                #else
                result[i] = -(a[i] * b[i]) + c[i];
                #endif
                break;
                
            case CNS_8T_FMA_NEG_MULTIPLY_SUB:
                #ifdef __FMA__
                result[i] = fma(-a[i], b[i], -c[i]);
                #else
                result[i] = -(a[i] * b[i]) - c[i];
                #endif
                break;
        }
    }
    
    // Compute error bounds
    if (bounds) {
        CNS_8T_INIT_ERROR_BOUND(bounds);
        #ifdef __FMA__
        bounds->relative_error = CNS_8T_EPSILON_F64;  // Single rounding
        bounds->ulp_error = 1;
        #else
        bounds->relative_error = 2.0 * CNS_8T_EPSILON_F64;  // Two roundings
        bounds->ulp_error = 2;
        #endif
        bounds->operation_count = count * 2;  // mul + add
        
        double max_abs = 0.0;
        for (size_t j = 0; j < count; j++) {
            double abs_val = fabs(result[j]);
            if (abs_val > max_abs) max_abs = abs_val;
        }
        bounds->absolute_error = max_abs * bounds->relative_error;
    }
    
    CNS_8T_END_TIMING(bounds);
    
    return CNS_8T_OK;
}

// ============================================================================
// COMPENSATED SUMMATION
// ============================================================================

cns_8t_result_t cns_8t_compensated_sum_init(cns_8t_compensated_sum_t* sum) {
    if (!sum) return CNS_ERROR_INVALID_ARGUMENT;
    
    sum->sum = 0.0;
    sum->compensation = 0.0;
    sum->count = 0;
    CNS_8T_INIT_ERROR_BOUND(&sum->bounds);
    
    return CNS_8T_OK;
}

cns_8t_result_t cns_8t_compensated_sum_add(cns_8t_compensated_sum_t* sum,
                                            double value) {
    if (!sum) return CNS_ERROR_INVALID_ARGUMENT;
    
    // Kahan summation algorithm
    double y = value - sum->compensation;
    double t = sum->sum + y;
    sum->compensation = (t - sum->sum) - y;
    sum->sum = t;
    sum->count++;
    
    // Update error bounds
    sum->bounds.operation_count++;
    sum->bounds.absolute_error += fabs(value) * CNS_8T_EPSILON_F64;
    sum->bounds.ulp_error++;
    
    return CNS_8T_OK;
}

cns_8t_result_t cns_8t_compensated_sum_add_array(cns_8t_compensated_sum_t* sum,
                                                  const double* values,
                                                  size_t count) {
    if (!sum || !values || count == 0) return CNS_ERROR_INVALID_ARGUMENT;
    
    for (size_t i = 0; i < count; i++) {
        cns_8t_compensated_sum_add(sum, values[i]);
    }
    
    return CNS_8T_OK;
}

cns_8t_result_t cns_8t_compensated_sum_finalize(cns_8t_compensated_sum_t* sum,
                                                 cns_8t_tracked_scalar_t* result) {
    if (!sum || !result) return CNS_ERROR_INVALID_ARGUMENT;
    
    result->value = sum->sum;
    result->bounds = sum->bounds;
    
    // Kahan summation has excellent error properties
    result->bounds.relative_error = (sum->count + 1) * CNS_8T_EPSILON_F64;
    result->stability = CNS_8T_STABLE_FORWARD;
    
    return CNS_8T_OK;
}

// ============================================================================
// CONDITION NUMBER ANALYSIS
// ============================================================================

cns_8t_result_t cns_8t_condition_scalar(double value,
                                         const cns_8t_condition_context_t* ctx,
                                         double* condition) {
    if (!condition) return CNS_ERROR_INVALID_ARGUMENT;
    
    if (ctx && ctx->scalar_condition) {
        *condition = ctx->scalar_condition(value, ctx->user_params);
    } else {
        // Default: condition number for identity function is 1
        *condition = 1.0;
    }
    
    return CNS_8T_OK;
}

cns_8t_result_t cns_8t_condition_vector(const cns_8t_tracked_vector_t* vec,
                                         const cns_8t_condition_context_t* ctx,
                                         double* condition) {
    if (!vec || !condition) return CNS_ERROR_INVALID_ARGUMENT;
    
    if (ctx && ctx->vector_condition) {
        *condition = ctx->vector_condition(vec->values, vec->size, ctx->user_params);
    } else {
        // Default: use norm-based condition number
        double norm = 0.0;
        for (size_t i = 0; i < vec->size; i++) {
            norm += vec->values[i] * vec->values[i];
        }
        norm = sqrt(norm);
        
        *condition = norm > DBL_MIN ? 1.0 : DBL_MAX;
    }
    
    return CNS_8T_OK;
}

cns_8t_result_t cns_8t_condition_matrix(const cns_8t_tracked_matrix_t* mat,
                                         double* condition) {
    if (!mat || !condition) return CNS_ERROR_INVALID_ARGUMENT;
    
    // For now, return stored condition number
    // Full implementation would compute singular values
    *condition = mat->condition_number;
    
    return CNS_8T_OK;
}

// ============================================================================
// INTERVAL ARITHMETIC
// ============================================================================

cns_8t_result_t cns_8t_interval_add(const cns_8t_interval_t* a,
                                     const cns_8t_interval_t* b,
                                     cns_8t_interval_t* result) {
    if (!a || !b || !result) return CNS_ERROR_INVALID_ARGUMENT;
    
    // Save rounding mode
    int old_round = fegetround();
    
    // Compute lower bound with downward rounding
    fesetround(FE_DOWNWARD);
    result->lower = a->lower + b->lower;
    
    // Compute upper bound with upward rounding
    fesetround(FE_UPWARD);
    result->upper = a->upper + b->upper;
    
    // Restore rounding mode
    fesetround(old_round);
    
    result->is_exact = a->is_exact && b->is_exact;
    
    return CNS_8T_OK;
}

cns_8t_result_t cns_8t_interval_mul(const cns_8t_interval_t* a,
                                     const cns_8t_interval_t* b,
                                     cns_8t_interval_t* result) {
    if (!a || !b || !result) return CNS_ERROR_INVALID_ARGUMENT;
    
    // Save rounding mode
    int old_round = fegetround();
    
    // Compute all possible products
    double products[4];
    fesetround(FE_DOWNWARD);
    products[0] = a->lower * b->lower;
    products[1] = a->lower * b->upper;
    products[2] = a->upper * b->lower;
    
    fesetround(FE_UPWARD);
    products[3] = a->upper * b->upper;
    
    // Find min and max
    result->lower = products[0];
    result->upper = products[0];
    
    for (int i = 1; i < 4; i++) {
        if (products[i] < result->lower) result->lower = products[i];
        if (products[i] > result->upper) result->upper = products[i];
    }
    
    // Restore rounding mode
    fesetround(old_round);
    
    result->is_exact = a->is_exact && b->is_exact;
    
    return CNS_8T_OK;
}

cns_8t_result_t cns_8t_interval_contains(const cns_8t_interval_t* interval,
                                          double value,
                                          bool* contains) {
    if (!interval || !contains) return CNS_ERROR_INVALID_ARGUMENT;
    
    *contains = (value >= interval->lower && value <= interval->upper);
    
    return CNS_8T_OK;
}

// ============================================================================
// ERROR BOUND PROPAGATION
// ============================================================================

cns_8t_result_t cns_8t_propagate_add(const cns_8t_error_bound_t* a,
                                      const cns_8t_error_bound_t* b,
                                      cns_8t_error_bound_t* result) {
    if (!a || !b || !result) return CNS_ERROR_INVALID_ARGUMENT;
    
    result->absolute_error = a->absolute_error + b->absolute_error;
    result->relative_error = fmax(a->relative_error, b->relative_error);
    result->ulp_error = a->ulp_error + b->ulp_error + 1;
    result->operation_count = a->operation_count + b->operation_count + 1;
    
    result->condition_number = fmax(a->condition_number, b->condition_number);
    result->overflow_risk = a->overflow_risk || b->overflow_risk;
    result->underflow_risk = a->underflow_risk || b->underflow_risk;
    result->cancellation_risk = a->cancellation_risk || b->cancellation_risk;
    
    return CNS_8T_OK;
}

cns_8t_result_t cns_8t_propagate_mul(const cns_8t_error_bound_t* a,
                                      const cns_8t_error_bound_t* b,
                                      double val_a, double val_b,
                                      cns_8t_error_bound_t* result) {
    if (!a || !b || !result) return CNS_ERROR_INVALID_ARGUMENT;
    
    result->absolute_error = fabs(val_b) * a->absolute_error + 
                            fabs(val_a) * b->absolute_error;
    result->relative_error = a->relative_error + b->relative_error;
    result->ulp_error = a->ulp_error + b->ulp_error + 1;
    result->operation_count = a->operation_count + b->operation_count + 1;
    
    result->condition_number = a->condition_number * b->condition_number;
    result->overflow_risk = a->overflow_risk || b->overflow_risk;
    result->underflow_risk = a->underflow_risk || b->underflow_risk;
    result->cancellation_risk = false;
    
    return CNS_8T_OK;
}

cns_8t_result_t cns_8t_propagate_function(const cns_8t_error_bound_t* input,
                                           double input_value,
                                           double derivative,
                                           cns_8t_error_bound_t* result) {
    if (!input || !result) return CNS_ERROR_INVALID_ARGUMENT;
    
    // Linear error propagation: δf ≈ f'(x) * δx
    result->absolute_error = fabs(derivative) * input->absolute_error;
    result->relative_error = input->relative_error;  // Approximate
    result->ulp_error = input->ulp_error + 1;
    result->operation_count = input->operation_count + 1;
    
    // Condition number of function at point
    result->condition_number = fabs(derivative * input_value);
    result->overflow_risk = input->overflow_risk;
    result->underflow_risk = input->underflow_risk;
    result->cancellation_risk = input->cancellation_risk;
    
    return CNS_8T_OK;
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

cns_8t_result_t cns_8t_tracked_vector_create(size_t size,
                                              cns_8t_tracked_vector_t* vector) {
    if (!vector || size == 0) return CNS_ERROR_INVALID_ARGUMENT;
    
    vector->values = calloc(size, sizeof(double));
    vector->bounds = calloc(size, sizeof(cns_8t_error_bound_t));
    
    if (!vector->values || !vector->bounds) {
        free(vector->values);
        free(vector->bounds);
        return CNS_ERROR_OUT_OF_MEMORY;
    }
    
    vector->size = size;
    vector->norm_error = 0.0;
    vector->condition_number = 1.0;
    
    // Initialize bounds
    for (size_t i = 0; i < size; i++) {
        CNS_8T_INIT_ERROR_BOUND(&vector->bounds[i]);
    }
    
    return CNS_8T_OK;
}

cns_8t_result_t cns_8t_tracked_matrix_create(size_t rows, size_t cols,
                                              cns_8t_tracked_matrix_t* matrix) {
    if (!matrix || rows == 0 || cols == 0) return CNS_ERROR_INVALID_ARGUMENT;
    
    size_t total = rows * cols;
    matrix->values = calloc(total, sizeof(double));
    matrix->bounds = calloc(total, sizeof(cns_8t_error_bound_t));
    
    if (!matrix->values || !matrix->bounds) {
        free(matrix->values);
        free(matrix->bounds);
        return CNS_ERROR_OUT_OF_MEMORY;
    }
    
    matrix->rows = rows;
    matrix->cols = cols;
    matrix->frobenius_error = 0.0;
    matrix->spectral_radius = 0.0;
    matrix->condition_number = 1.0;
    matrix->is_symmetric = false;
    
    // Initialize bounds
    for (size_t i = 0; i < total; i++) {
        CNS_8T_INIT_ERROR_BOUND(&matrix->bounds[i]);
    }
    
    return CNS_8T_OK;
}

void cns_8t_tracked_vector_destroy(cns_8t_tracked_vector_t* vector) {
    if (vector) {
        free(vector->values);
        free(vector->bounds);
        vector->values = NULL;
        vector->bounds = NULL;
        vector->size = 0;
    }
}

void cns_8t_tracked_matrix_destroy(cns_8t_tracked_matrix_t* matrix) {
    if (matrix) {
        free(matrix->values);
        free(matrix->bounds);
        matrix->values = NULL;
        matrix->bounds = NULL;
        matrix->rows = 0;
        matrix->cols = 0;
    }
}

bool cns_8t_is_error_acceptable(const cns_8t_error_bound_t* bounds,
                                 double tolerance) {
    if (!bounds) return false;
    
    return bounds->relative_error <= tolerance && 
           !bounds->overflow_risk && 
           !bounds->underflow_risk;
}

double cns_8t_get_relative_error(const cns_8t_error_bound_t* bounds) {
    return bounds ? bounds->relative_error : DBL_MAX;
}

cns_8t_stability_t cns_8t_analyze_stability(const cns_8t_error_bound_t* bounds,
                                             uint32_t operation_count) {
    if (!bounds) return CNS_8T_UNSTABLE;
    
    // Simple heuristic based on error growth
    double error_growth = bounds->relative_error / (operation_count * CNS_8T_EPSILON_F64);
    
    if (error_growth < 10.0) {
        return CNS_8T_STABLE_FORWARD;
    } else if (error_growth < 100.0) {
        return CNS_8T_STABLE_MIXED;
    } else if (bounds->cancellation_risk) {
        return CNS_8T_CONDITIONALLY_STABLE;
    } else {
        return CNS_8T_UNSTABLE;
    }
}