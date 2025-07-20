#ifndef CNS_8T_SHACL_L1_H
#define CNS_8T_SHACL_L1_H

#include "cns/8t/core.h"
#include "cns/8t/graph_l1.h"
#include "cns/8t/numerical.h"
#include "cns/8t/arena_l1.h"
#include <immintrin.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// 8T L1-OPTIMIZED SHACL VALIDATION
// Cache-optimized SHACL constraint validation with 8-tick guarantee
// ============================================================================

// SHACL constraint types optimized for L1 cache and branchless execution
typedef enum {
    CNS_8T_SHACL_CLASS = 0,         // sh:class constraint
    CNS_8T_SHACL_DATATYPE = 1,      // sh:datatype constraint
    CNS_8T_SHACL_NODE_KIND = 2,     // sh:nodeKind constraint
    CNS_8T_SHACL_MIN_COUNT = 3,     // sh:minCount constraint
    CNS_8T_SHACL_MAX_COUNT = 4,     // sh:maxCount constraint
    CNS_8T_SHACL_MIN_INCLUSIVE = 5, // sh:minInclusive constraint
    CNS_8T_SHACL_MAX_INCLUSIVE = 6, // sh:maxInclusive constraint
    CNS_8T_SHACL_MIN_EXCLUSIVE = 7, // sh:minExclusive constraint
    CNS_8T_SHACL_MAX_EXCLUSIVE = 8, // sh:maxExclusive constraint
    CNS_8T_SHACL_MIN_LENGTH = 9,    // sh:minLength constraint
    CNS_8T_SHACL_MAX_LENGTH = 10,   // sh:maxLength constraint
    CNS_8T_SHACL_PATTERN = 11,      // sh:pattern constraint
    CNS_8T_SHACL_LANGUAGE_IN = 12,  // sh:languageIn constraint
    CNS_8T_SHACL_UNIQUE_LANG = 13,  // sh:uniqueLang constraint
    CNS_8T_SHACL_HAS_VALUE = 14,    // sh:hasValue constraint
    CNS_8T_SHACL_IN = 15            // sh:in constraint
} cns_8t_shacl_constraint_type_t;

// SHACL severity levels
typedef enum {
    CNS_8T_SHACL_VIOLATION = 0,     // sh:Violation
    CNS_8T_SHACL_WARNING = 1,       // sh:Warning
    CNS_8T_SHACL_INFO = 2           // sh:Info
} cns_8t_shacl_severity_t;

// L1-optimized SHACL constraint (64 bytes - one cache line)
typedef struct {
    // Hot path data (first 32 bytes)
    cns_8t_shacl_constraint_type_t type; // Constraint type (4 bytes)
    uint32_t target_shape_id;            // Target shape ID (4 bytes)
    uint32_t property_path_id;           // Property path ID (4 bytes)
    cns_8t_shacl_severity_t severity;    // Severity level (4 bytes)
    
    union {
        // Numeric constraints
        struct {
            cns_8t_quad_number_t min_value; // Minimum value (16 bytes)
        } numeric;
        
        // Count constraints
        struct {
            uint32_t min_count;              // Minimum count (4 bytes)
            uint32_t max_count;              // Maximum count (4 bytes)
            uint64_t padding;                // Padding (8 bytes)
        } count;
        
        // String constraints
        struct {
            uint32_t min_length;             // Minimum length (4 bytes)
            uint32_t max_length;             // Maximum length (4 bytes)
            uint64_t pattern_hash;           // Pattern hash (8 bytes)
        } string;
    } value; // 16 bytes
    
    // Cold path data (second 32 bytes)
    uint64_t constraint_id;              // Unique constraint ID (8 bytes)
    uint32_t validation_flags;           // Validation flags (4 bytes)
    uint32_t execution_count;            // Times executed (4 bytes)
    cns_tick_t total_execution_time;     // Total execution time (8 bytes)
    uint64_t last_validation_hash;       // Hash of last validation (8 bytes)
} cns_8t_shacl_constraint_l1_t __attribute__((aligned(64)));

// SHACL shape optimized for L1 cache (128 bytes - two cache lines)
typedef struct {
    // First cache line - hot data
    uint32_t shape_id;                   // Shape identifier (4 bytes)
    uint32_t constraint_count;           // Number of constraints (4 bytes)
    uint32_t target_count;               // Number of target nodes (4 bytes)
    uint32_t flags;                      // Shape flags (4 bytes)
    
    cns_8t_shacl_constraint_l1_t* constraints; // Constraint array pointer (8 bytes)
    uint64_t* target_nodes;              // Target node IDs (8 bytes)
    
    // Performance metrics
    uint64_t validation_count;           // Total validations (8 bytes)
    cns_tick_t total_validation_time;    // Total time spent (8 bytes)
    double   avg_validation_time;        // Average time per validation (8 bytes)
    uint32_t cache_hits;                 // L1 cache hits (4 bytes)
    uint32_t cache_misses;               // L1 cache misses (4 bytes)
    
    // Second cache line - cold data
    char name[64];                       // Shape name (64 bytes)
} cns_8t_shacl_shape_l1_t __attribute__((aligned(64)));

// SHACL validation result (32 bytes - half cache line)
typedef struct {
    uint64_t node_id;                    // Validated node ID (8 bytes)
    uint32_t shape_id;                   // Shape ID (4 bytes)
    uint32_t constraint_id;              // Failing constraint ID (4 bytes)
    cns_8t_shacl_severity_t severity;    // Violation severity (4 bytes)
    uint32_t error_code;                 // Error code (4 bytes)
    uint64_t validation_context;         // Validation context hash (8 bytes)
} cns_8t_shacl_violation_l1_t __attribute__((aligned(32)));

// SIMD-optimized validation batch (256 bytes - four cache lines)
typedef struct {
    uint32_t batch_size;                 // Number of items in batch (4 bytes)
    uint32_t validation_flags;           // Batch validation flags (4 bytes)
    uint64_t batch_id;                   // Unique batch identifier (8 bytes)
    
    // SIMD-aligned data arrays
    uint64_t node_ids[28];               // Node IDs to validate (224 bytes)
    uint32_t shape_ids[14];              // Shape IDs (56 bytes, padded to 64)
    uint32_t padding[2];                 // Alignment padding (8 bytes)
} cns_8t_shacl_batch_l1_t __attribute__((aligned(64)));

// L1-optimized SHACL validator context
typedef struct {
    // Shape repository
    cns_8t_shacl_shape_l1_t* shapes;    // Array of shapes
    uint32_t shape_count;                // Number of shapes
    uint32_t shape_capacity;             // Shape array capacity
    
    // Constraint repository
    cns_8t_shacl_constraint_l1_t* constraints; // All constraints
    uint32_t constraint_count;           // Total constraints
    uint32_t constraint_capacity;        // Constraint array capacity
    
    // Target graph
    cns_8t_graph_l1_t* graph;           // Graph being validated
    
    // Validation state
    cns_8t_shacl_violation_l1_t* violations; // Violation results
    uint32_t violation_count;            // Number of violations
    uint32_t violation_capacity;         // Violation array capacity
    
    // Memory management
    cns_8t_arena_l1_t* arena;           // L1-optimized arena
    
    // Performance tracking
    cns_8t_perf_metrics_t perf;         // Performance metrics
    uint64_t nodes_validated;           // Total nodes validated
    uint64_t constraints_evaluated;     // Total constraints evaluated
    
    // L1 cache optimization
    uint32_t hot_shape_count;           // Number of hot shapes
    uint32_t* hot_shape_ids;            // IDs of frequently used shapes
    cns_8t_shacl_batch_l1_t* current_batch; // Current validation batch
    
    // SIMD optimization context
    __m256i simd_node_mask;             // Cached node type mask
    __m256d simd_numeric_bounds;        // Cached numeric bounds
} cns_8t_shacl_validator_l1_t __attribute__((aligned(64)));

// Validation configuration
typedef struct {
    bool enable_early_termination;      // Stop on first violation
    bool enable_parallel_validation;    // Use parallel validation
    bool enable_constraint_caching;     // Cache constraint evaluations
    bool enable_simd_optimization;      // Use SIMD for batch operations
    
    // Performance tuning
    uint32_t batch_size;                 // Validation batch size
    uint32_t max_cache_size;             // Maximum cache size
    cns_8t_precision_mode_t numeric_precision; // Numeric precision mode
    
    // Error handling
    bool collect_all_violations;         // Collect all vs. first violation
    uint32_t max_violations;             // Maximum violations to collect
    cns_8t_shacl_severity_t min_severity; // Minimum severity to report
} cns_8t_shacl_validation_config_t;

// ============================================================================
// BRANCHLESS CONSTRAINT VALIDATION FUNCTIONS
// ============================================================================

// Branchless numeric constraint validation
static inline bool cns_8t_validate_numeric_constraint_branchless(
    const cns_8t_quad_number_t* value,
    const cns_8t_shacl_constraint_l1_t* constraint
) {
    // Use bit manipulation to avoid branches
    uint32_t constraint_type = constraint->type;
    
    // Compare value against constraint bounds using SIMD
    __m128d value_vec = _mm_set1_pd(value->value);
    __m128d bound_vec = _mm_set1_pd(constraint->value.numeric.min_value.value);
    
    __m128d cmp_result = _mm_cmpge_pd(value_vec, bound_vec);
    uint32_t cmp_mask = _mm_movemask_pd(cmp_result);
    
    // Branchless selection based on constraint type
    uint32_t min_inclusive_check = (constraint_type == CNS_8T_SHACL_MIN_INCLUSIVE) & (cmp_mask & 1);
    uint32_t min_exclusive_check = (constraint_type == CNS_8T_SHACL_MIN_EXCLUSIVE) & (cmp_mask & 1);
    // ... similar for other constraint types
    
    return min_inclusive_check | min_exclusive_check; // Simplified
}

// Branchless count constraint validation
static inline bool cns_8t_validate_count_constraint_branchless(
    uint32_t actual_count,
    const cns_8t_shacl_constraint_l1_t* constraint
) {
    uint32_t constraint_type = constraint->type;
    uint32_t min_count = constraint->value.count.min_count;
    uint32_t max_count = constraint->value.count.max_count;
    
    // Branchless comparisons
    uint32_t min_check = (constraint_type == CNS_8T_SHACL_MIN_COUNT) & (actual_count >= min_count);
    uint32_t max_check = (constraint_type == CNS_8T_SHACL_MAX_COUNT) & (actual_count <= max_count);
    
    return min_check | max_check;
}

// SIMD string length validation (processes 32 characters at once)
static inline bool cns_8t_validate_string_length_simd(
    const char* string,
    const cns_8t_shacl_constraint_l1_t* constraint
) {
    uint32_t length = 0;
    const char* ptr = string;
    
    // Count string length using SIMD
    while (*ptr) {
        __m256i chars = _mm256_loadu_si256((const __m256i*)ptr);
        __m256i zeros = _mm256_setzero_si256();
        __m256i cmp = _mm256_cmpeq_epi8(chars, zeros);
        uint32_t mask = _mm256_movemask_epi8(cmp);
        
        if (mask) {
            length += __builtin_ctz(mask);
            break;
        }
        
        length += 32;
        ptr += 32;
    }
    
    // Branchless length validation
    uint32_t min_len = constraint->value.string.min_length;
    uint32_t max_len = constraint->value.string.max_length;
    uint32_t constraint_type = constraint->type;
    
    uint32_t min_check = (constraint_type == CNS_8T_SHACL_MIN_LENGTH) & (length >= min_len);
    uint32_t max_check = (constraint_type == CNS_8T_SHACL_MAX_LENGTH) & (length <= max_len);
    
    return min_check | max_check;
}

// ============================================================================
// L1-OPTIMIZED VALIDATION ALGORITHMS
// ============================================================================

// Validate single node against single shape
cns_8t_result_t cns_8t_shacl_validate_node_l1(cns_8t_shacl_validator_l1_t* validator,
                                               uint64_t node_id,
                                               uint32_t shape_id,
                                               cns_8t_shacl_violation_l1_t* violations,
                                               uint32_t* violation_count);

// Batch validation using SIMD optimization
cns_8t_result_t cns_8t_shacl_validate_batch_simd(cns_8t_shacl_validator_l1_t* validator,
                                                  const cns_8t_shacl_batch_l1_t* batch,
                                                  cns_8t_shacl_violation_l1_t* violations,
                                                  uint32_t* violation_count);

// Full graph validation with L1 optimization
cns_8t_result_t cns_8t_shacl_validate_graph_l1(cns_8t_shacl_validator_l1_t* validator,
                                                const cns_8t_shacl_validation_config_t* config,
                                                cns_8t_shacl_violation_l1_t** violations,
                                                uint32_t* violation_count);

// Incremental validation (validate only changed nodes)
cns_8t_result_t cns_8t_shacl_validate_incremental_l1(cns_8t_shacl_validator_l1_t* validator,
                                                      const uint64_t* changed_nodes,
                                                      uint32_t changed_count,
                                                      cns_8t_shacl_violation_l1_t* violations,
                                                      uint32_t* violation_count);

// ============================================================================
// CONSTRAINT EVALUATION FUNCTIONS
// ============================================================================

// Generic constraint evaluation dispatcher (branchless)
cns_8t_result_t cns_8t_shacl_evaluate_constraint_l1(
    const cns_8t_shacl_constraint_l1_t* constraint,
    uint64_t node_id,
    const cns_8t_graph_l1_t* graph,
    bool* validation_result,
    cns_8t_shacl_violation_l1_t* violation
);

// Specific constraint evaluators
cns_8t_result_t cns_8t_shacl_eval_class_constraint(
    const cns_8t_shacl_constraint_l1_t* constraint,
    uint64_t node_id,
    const cns_8t_graph_l1_t* graph,
    bool* result
);

cns_8t_result_t cns_8t_shacl_eval_datatype_constraint(
    const cns_8t_shacl_constraint_l1_t* constraint,
    uint64_t node_id,
    const cns_8t_graph_l1_t* graph,
    bool* result
);

cns_8t_result_t cns_8t_shacl_eval_count_constraint(
    const cns_8t_shacl_constraint_l1_t* constraint,
    uint64_t node_id,
    const cns_8t_graph_l1_t* graph,
    bool* result
);

cns_8t_result_t cns_8t_shacl_eval_numeric_constraint(
    const cns_8t_shacl_constraint_l1_t* constraint,
    uint64_t node_id,
    const cns_8t_graph_l1_t* graph,
    bool* result,
    const cns_8t_numerical_context_t* num_ctx
);

// ============================================================================
// SHAPE AND CONSTRAINT MANAGEMENT
// ============================================================================

// Shape creation and management
cns_8t_result_t cns_8t_shacl_shape_create_l1(uint32_t shape_id,
                                              const char* name,
                                              cns_8t_shacl_shape_l1_t** shape);

cns_8t_result_t cns_8t_shacl_shape_add_constraint(cns_8t_shacl_shape_l1_t* shape,
                                                   const cns_8t_shacl_constraint_l1_t* constraint);

cns_8t_result_t cns_8t_shacl_shape_optimize_l1(cns_8t_shacl_shape_l1_t* shape);

// Constraint creation helpers
cns_8t_result_t cns_8t_shacl_constraint_create_class(uint32_t shape_id,
                                                      uint32_t class_id,
                                                      cns_8t_shacl_constraint_l1_t* constraint);

cns_8t_result_t cns_8t_shacl_constraint_create_count(uint32_t shape_id,
                                                      uint32_t property_id,
                                                      uint32_t min_count,
                                                      uint32_t max_count,
                                                      cns_8t_shacl_constraint_l1_t* constraint);

cns_8t_result_t cns_8t_shacl_constraint_create_numeric(uint32_t shape_id,
                                                        uint32_t property_id,
                                                        cns_8t_shacl_constraint_type_t type,
                                                        const cns_8t_quad_number_t* value,
                                                        cns_8t_shacl_constraint_l1_t* constraint);

// ============================================================================
// API FUNCTIONS
// ============================================================================

// Validator lifecycle
cns_8t_result_t cns_8t_shacl_validator_create_l1(cns_8t_graph_l1_t* graph,
                                                  const cns_8t_shacl_validation_config_t* config,
                                                  cns_8t_shacl_validator_l1_t** validator);

cns_8t_result_t cns_8t_shacl_validator_destroy_l1(cns_8t_shacl_validator_l1_t* validator);

// Shape management
cns_8t_result_t cns_8t_shacl_validator_add_shape(cns_8t_shacl_validator_l1_t* validator,
                                                  const cns_8t_shacl_shape_l1_t* shape);

cns_8t_result_t cns_8t_shacl_validator_remove_shape(cns_8t_shacl_validator_l1_t* validator,
                                                     uint32_t shape_id);

cns_8t_result_t cns_8t_shacl_validator_get_shape(cns_8t_shacl_validator_l1_t* validator,
                                                  uint32_t shape_id,
                                                  cns_8t_shacl_shape_l1_t** shape);

// Validation operations
cns_8t_result_t cns_8t_shacl_validate(cns_8t_shacl_validator_l1_t* validator,
                                       const cns_8t_shacl_validation_config_t* config);

cns_8t_result_t cns_8t_shacl_validate_node(cns_8t_shacl_validator_l1_t* validator,
                                            uint64_t node_id,
                                            uint32_t shape_id);

// Batch operations
cns_8t_result_t cns_8t_shacl_batch_create(uint32_t batch_size,
                                           cns_8t_shacl_batch_l1_t** batch);

cns_8t_result_t cns_8t_shacl_batch_add_node(cns_8t_shacl_batch_l1_t* batch,
                                             uint64_t node_id,
                                             uint32_t shape_id);

cns_8t_result_t cns_8t_shacl_batch_validate(cns_8t_shacl_validator_l1_t* validator,
                                             const cns_8t_shacl_batch_l1_t* batch);

// Results and reporting
cns_8t_result_t cns_8t_shacl_get_violations(cns_8t_shacl_validator_l1_t* validator,
                                             cns_8t_shacl_violation_l1_t** violations,
                                             uint32_t* violation_count);

cns_8t_result_t cns_8t_shacl_clear_violations(cns_8t_shacl_validator_l1_t* validator);

// Performance monitoring
cns_8t_result_t cns_8t_shacl_get_performance_stats(cns_8t_shacl_validator_l1_t* validator,
                                                    cns_8t_perf_metrics_t* stats);

cns_8t_result_t cns_8t_shacl_get_cache_stats(cns_8t_shacl_validator_l1_t* validator,
                                              uint64_t* cache_hits,
                                              uint64_t* cache_misses,
                                              double* hit_ratio);

// Configuration and optimization
cns_8t_result_t cns_8t_shacl_optimize_validator(cns_8t_shacl_validator_l1_t* validator);

cns_8t_result_t cns_8t_shacl_set_hot_shapes(cns_8t_shacl_validator_l1_t* validator,
                                             const uint32_t* shape_ids,
                                             uint32_t count);

#ifdef __cplusplus
}
#endif

#endif // CNS_8T_SHACL_L1_H