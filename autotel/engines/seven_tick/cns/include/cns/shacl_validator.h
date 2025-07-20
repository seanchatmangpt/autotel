/*  ─────────────────────────────────────────────────────────────
    cns/shacl_validator.h  –  Memory-Constrained SHACL Validator
    
    80/20 SHACL validator focused on memory bound validation for ARENAC.
    Ensures TTL graphs have bounded memory footprints with 7T operations.
    
    Key Features:
    - Memory bound checking for all components
    - 7T-compliant validation operations  
    - Validation reports with memory requirements
    - Focus on constraint validation vs full SHACL spec
    
    Design Philosophy:
    - Simple and efficient validation (80/20 approach)
    - Memory constraints are first-class citizens
    - Performance over feature completeness
    ───────────────────────────────────────────────────────────── */

#ifndef CNS_SHACL_VALIDATOR_H
#define CNS_SHACL_VALIDATOR_H

#include "cns/arena.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

// Forward declarations to avoid conflicts
typedef enum {
    CNS_SHACL_OK = 0,
    CNS_SHACL_ERROR_MEMORY = 1,
    CNS_SHACL_ERROR_INVALID_ARG = 2,
    CNS_SHACL_ERROR_NOT_FOUND = 3,
    CNS_SHACL_ERROR_CAPACITY = 4,
    CNS_SHACL_ERROR_CORRUPTION = 5
} cns_shacl_result_t;

// Simple graph type for this implementation
typedef struct {
    void *data;
    size_t size;
} cns_shacl_graph_t;

// String reference type for this implementation
typedef struct {
    uint32_t hash;        // Precomputed hash
    uint32_t offset;      // Offset in string arena
    uint16_t length;      // String length
    uint16_t ref_count;   // Reference counting
} cns_string_ref_t;

// Tick counter type
typedef uint64_t cns_tick_t;

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// MEMORY CONSTRAINT VALIDATION CONSTANTS
// ============================================================================

#define CNS_SHACL_VALIDATOR_VERSION_MAJOR 1
#define CNS_SHACL_VALIDATOR_VERSION_MINOR 0

// Memory bound limits for ARENAC compliance
#define CNS_SHACL_MAX_MEMORY_PER_NODE    (1024 * 1024)     // 1MB per node
#define CNS_SHACL_MAX_MEMORY_PER_GRAPH   (64 * 1024 * 1024) // 64MB per graph
#define CNS_SHACL_MAX_VALIDATION_DEPTH   32                 // Max constraint depth
#define CNS_SHACL_MAX_CONSTRAINTS        1024               // Max constraints per shape

// 7T performance limits
#define CNS_SHACL_MAX_TICKS_PER_NODE     7
#define CNS_SHACL_MAX_TICKS_PER_CONSTRAINT 2

// Magic number for validator integrity
#define CNS_SHACL_VALIDATOR_MAGIC 0x53484143  // 'SHAC'

// ============================================================================
// CONSTRAINT TYPES - 80/20 SUBSET
// ============================================================================

// Focus on most commonly used constraints
typedef enum {
    CNS_SHACL_CONSTRAINT_MEMORY_BOUND = 0,    // Custom: memory bound check
    CNS_SHACL_CONSTRAINT_MIN_COUNT,           // sh:minCount
    CNS_SHACL_CONSTRAINT_MAX_COUNT,           // sh:maxCount  
    CNS_SHACL_CONSTRAINT_CLASS,               // sh:class
    CNS_SHACL_CONSTRAINT_DATATYPE,            // sh:datatype
    CNS_SHACL_CONSTRAINT_NODE_KIND,           // sh:nodeKind
    CNS_SHACL_CONSTRAINT_MIN_LENGTH,          // sh:minLength
    CNS_SHACL_CONSTRAINT_MAX_LENGTH,          // sh:maxLength
    CNS_SHACL_CONSTRAINT_PATTERN,             // sh:pattern (basic regex)
    CNS_SHACL_CONSTRAINT_IN,                  // sh:in (value enumeration)
    CNS_SHACL_CONSTRAINT_COUNT                // Number of constraint types
} cns_shacl_constraint_type_t;

// Node kinds for validation
typedef enum {
    CNS_SHACL_NODE_KIND_IRI = 0,
    CNS_SHACL_NODE_KIND_BLANK_NODE,
    CNS_SHACL_NODE_KIND_LITERAL,
    CNS_SHACL_NODE_KIND_ANY
} cns_shacl_node_kind_t;

// ============================================================================
// MEMORY BOUND STRUCTURES
// ============================================================================

// Memory footprint descriptor for validation
typedef struct {
    size_t current_usage;        // Current memory usage
    size_t peak_usage;           // Peak memory usage
    size_t max_allowed;          // Maximum allowed memory
    uint32_t violation_count;    // Number of memory violations
    bool bounded;                // True if within bounds
} cns_memory_footprint_t;

// Constraint value union - memory efficient
typedef union {
    uint32_t count_value;        // For min/max count
    cns_string_ref_t string_ref; // For pattern, datatype, class
    cns_shacl_node_kind_t node_kind; // For node kind
    size_t memory_limit;         // For memory bound constraint
    cns_string_ref_t *enum_values; // For sh:in constraint
} cns_constraint_value_t;

// Individual constraint - cache-aligned
typedef struct cns_shacl_constraint {
    cns_shacl_constraint_type_t type;
    cns_constraint_value_t value;
    cns_string_ref_t property_path;  // Property this constraint applies to
    cns_string_ref_t message;        // Custom error message
    cns_memory_footprint_t memory;   // Memory requirements for this constraint
    struct cns_shacl_constraint *next;
} cns_shacl_constraint_t;

// Shape definition - optimized for 7T
typedef struct cns_shacl_shape {
    cns_string_ref_t shape_iri;      // Shape identifier
    cns_string_ref_t target_class;   // Target class (simplified targeting)
    cns_shacl_constraint_t *constraints; // Linked list of constraints
    uint32_t constraint_count;        // Number of constraints
    cns_memory_footprint_t memory;    // Memory footprint of this shape
    bool active;                      // Shape is active for validation
    uint32_t shape_id;               // Numeric ID for fast lookup
} cns_shacl_shape_t;

// ============================================================================
// VALIDATION RESULTS
// ============================================================================

// Validation result levels
typedef enum {
    CNS_SHACL_RESULT_INFO = 0,
    CNS_SHACL_RESULT_WARNING,
    CNS_SHACL_RESULT_VIOLATION,
    CNS_SHACL_RESULT_MEMORY_VIOLATION  // Custom: memory bound violation
} cns_shacl_result_level_t;

// Individual validation result
typedef struct cns_shacl_result {
    cns_string_ref_t focus_node;      // Node that failed validation
    cns_string_ref_t property_path;   // Property path (if applicable)
    cns_string_ref_t value;           // Failing value
    cns_shacl_constraint_type_t constraint_type; // Constraint that failed
    cns_shacl_result_level_t level;   // Result severity level
    cns_string_ref_t message;         // Validation message
    cns_memory_footprint_t memory;    // Memory usage details
    struct cns_shacl_result *next;    // Next result in list
} cns_shacl_result_t;

// Validation report
typedef struct {
    bool conforms;                    // True if data conforms to shapes
    cns_shacl_result_t *results;      // List of validation results
    uint32_t result_count;            // Number of results
    uint32_t violation_count;         // Number of violations
    uint32_t memory_violation_count;  // Number of memory violations
    cns_memory_footprint_t total_memory; // Total memory usage
    cns_tick_t validation_ticks;      // Time taken for validation
    uint32_t nodes_validated;         // Number of nodes validated
} cns_shacl_validation_report_t;

// ============================================================================
// VALIDATOR STRUCTURE
// ============================================================================

// Main SHACL validator - 7T optimized
typedef struct {
    // Memory management
    cns_arena_t *arena;              // Arena for all allocations
    cns_memory_footprint_t memory;   // Validator memory footprint
    
    // Shape storage
    cns_shacl_shape_t *shapes;       // Array of shapes
    uint32_t shape_count;            // Number of loaded shapes
    uint32_t shape_capacity;         // Shape array capacity
    
    // Fast lookup tables
    uint32_t *shape_hash_table;      // Shape IRI hash -> index
    uint32_t hash_table_size;        // Hash table size
    
    // Configuration
    size_t max_memory_per_node;      // Memory limit per node
    size_t max_memory_per_graph;     // Memory limit per graph
    uint32_t max_validation_depth;   // Maximum constraint depth
    bool strict_memory_mode;         // Strict memory enforcement
    
    // Statistics
    uint64_t validations_performed;  // Total validations
    uint64_t memory_violations;      // Memory violations found
    cns_tick_t total_ticks;          // Total validation time
    
    // Integrity
    uint32_t magic;                  // Magic number for validation
} cns_shacl_validator_t;

// ============================================================================
// VALIDATOR LIFECYCLE FUNCTIONS
// ============================================================================

// Create validator with memory constraints
cns_shacl_validator_t* cns_shacl_validator_create(cns_arena_t *arena,
                                                  size_t max_memory_per_node,
                                                  size_t max_memory_per_graph);

// Create validator with default memory limits
cns_shacl_validator_t* cns_shacl_validator_create_default(cns_arena_t *arena);

// Destroy validator (O(1) with arena)
void cns_shacl_validator_destroy(cns_shacl_validator_t *validator);

// Clear all shapes (O(1) arena reset)
cns_shacl_result_t cns_shacl_validator_clear(cns_shacl_validator_t *validator);

// ============================================================================
// SHAPE MANAGEMENT FUNCTIONS
// ============================================================================

// Load shape from simple definition
cns_shacl_result_t cns_shacl_load_shape(cns_shacl_validator_t *validator,
                                        const char *shape_iri,
                                        const char *target_class);

// Add constraint to shape
cns_shacl_result_t cns_shacl_add_constraint(cns_shacl_validator_t *validator,
                                            const char *shape_iri,
                                            cns_shacl_constraint_type_t type,
                                            const char *property_path,
                                            const cns_constraint_value_t *value);

// Add memory bound constraint (ARENAC-specific)
cns_shacl_result_t cns_shacl_add_memory_constraint(cns_shacl_validator_t *validator,
                                                   const char *shape_iri,
                                                   const char *property_path,
                                                   size_t memory_limit);

// Get shape by IRI (O(1) hash lookup)
const cns_shacl_shape_t* cns_shacl_get_shape(const cns_shacl_validator_t *validator,
                                              const char *shape_iri);

// ============================================================================
// VALIDATION FUNCTIONS - 7T GUARANTEED
// ============================================================================

// Validate entire graph with memory bounds
cns_shacl_result_t cns_shacl_validate_graph(cns_shacl_validator_t *validator,
                                            const cns_shacl_graph_t *graph,
                                            cns_shacl_validation_report_t *report);

// Validate single node with memory tracking
cns_shacl_result_t cns_shacl_validate_node(cns_shacl_validator_t *validator,
                                           const cns_shacl_graph_t *graph,
                                           const char *node_iri,
                                           cns_shacl_validation_report_t *report);

// Validate node against specific shape
cns_shacl_result_t cns_shacl_validate_node_shape(cns_shacl_validator_t *validator,
                                                 const cns_shacl_graph_t *graph,
                                                 const char *node_iri,
                                                 const cns_shacl_shape_t *shape,
                                                 cns_shacl_validation_report_t *report);

// Check memory bounds for node (ARENAC-specific)
cns_shacl_result_t cns_shacl_check_memory_bounds(cns_shacl_validator_t *validator,
                                                 const cns_shacl_graph_t *graph,
                                                 const char *node_iri,
                                                 cns_memory_footprint_t *footprint);

// ============================================================================
// CONSTRAINT EVALUATION FUNCTIONS
// ============================================================================

// Evaluate single constraint (7T performance)
cns_shacl_result_t cns_shacl_eval_constraint(cns_shacl_validator_t *validator,
                                             const cns_shacl_graph_t *graph,
                                             const char *focus_node,
                                             const char *property_path,
                                             const cns_shacl_constraint_t *constraint,
                                             bool *conforms,
                                             cns_memory_footprint_t *memory);

// Memory bound constraint evaluation
bool cns_shacl_eval_memory_bound(const cns_shacl_graph_t *graph,
                                const char *focus_node,
                                size_t memory_limit,
                                cns_memory_footprint_t *footprint);

// Count constraint evaluation (min/max count)
bool cns_shacl_eval_count_constraint(const cns_shacl_graph_t *graph,
                                     const char *focus_node,
                                     const char *property_path,
                                     uint32_t min_count,
                                     uint32_t max_count,
                                     uint32_t *actual_count);

// Class constraint evaluation
bool cns_shacl_eval_class_constraint(const cns_shacl_graph_t *graph,
                                     const char *focus_node,
                                     const char *target_class);

// Datatype constraint evaluation
bool cns_shacl_eval_datatype_constraint(const cns_shacl_graph_t *graph,
                                        const char *value,
                                        const char *datatype);

// Node kind constraint evaluation
bool cns_shacl_eval_node_kind_constraint(const cns_shacl_graph_t *graph,
                                         const char *value,
                                         cns_shacl_node_kind_t node_kind);

// ============================================================================
// VALIDATION REPORT FUNCTIONS
// ============================================================================

// Create validation report
cns_shacl_validation_report_t* cns_shacl_create_report(cns_shacl_validator_t *validator);

// Add result to report
cns_shacl_result_t cns_shacl_add_result(cns_shacl_validation_report_t *report,
                                        const char *focus_node,
                                        const char *property_path,
                                        const char *value,
                                        cns_shacl_constraint_type_t constraint_type,
                                        cns_shacl_result_level_t level,
                                        const char *message,
                                        const cns_memory_footprint_t *memory);

// Finalize report (calculate conformance)
cns_shacl_result_t cns_shacl_finalize_report(cns_shacl_validation_report_t *report);

// Destroy report (O(1) with arena)
void cns_shacl_destroy_report(cns_shacl_validation_report_t *report);

// ============================================================================
// MEMORY ANALYSIS FUNCTIONS
// ============================================================================

// Calculate memory footprint of graph
cns_shacl_result_t cns_shacl_calculate_graph_memory(const cns_shacl_graph_t *graph,
                                                    cns_memory_footprint_t *footprint);

// Calculate memory footprint of node
cns_shacl_result_t cns_shacl_calculate_node_memory(const cns_shacl_graph_t *graph,
                                                   const char *node_iri,
                                                   cns_memory_footprint_t *footprint);

// Get validator memory usage
size_t cns_shacl_validator_memory_usage(const cns_shacl_validator_t *validator);

// Check if memory bounds are satisfied
bool cns_shacl_memory_bounds_satisfied(const cns_memory_footprint_t *footprint);

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

// Convert constraint type to string
const char* cns_shacl_constraint_type_string(cns_shacl_constraint_type_t type);

// Convert result level to string
const char* cns_shacl_result_level_string(cns_shacl_result_level_t level);

// Convert node kind to string
const char* cns_shacl_node_kind_string(cns_shacl_node_kind_t node_kind);

// Print validation report
cns_shacl_result_t cns_shacl_print_report(const cns_shacl_validation_report_t *report, FILE *output);

// Print memory footprint
cns_shacl_result_t cns_shacl_print_memory_footprint(const cns_memory_footprint_t *footprint, FILE *output);

// ============================================================================
// PERFORMANCE MONITORING
// ============================================================================

// Get validator statistics
cns_shacl_result_t cns_shacl_get_stats(const cns_shacl_validator_t *validator,
                                       uint64_t *validations,
                                       uint64_t *memory_violations,
                                       cns_tick_t *total_ticks,
                                       size_t *memory_usage);

// Print validator statistics
cns_shacl_result_t cns_shacl_print_stats(const cns_shacl_validator_t *validator, FILE *output);

// ============================================================================
// INTEGRITY CHECKS
// ============================================================================

// Validate validator integrity
cns_shacl_result_t cns_shacl_validate_validator(const cns_shacl_validator_t *validator);

// Check magic number
bool cns_shacl_validator_valid(const cns_shacl_validator_t *validator);

#ifdef __cplusplus
}
#endif

#endif // CNS_SHACL_VALIDATOR_H