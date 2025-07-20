/*  ─────────────────────────────────────────────────────────────
    shacl_validator_standalone.h  –  Standalone SHACL Validator
    
    Self-contained SHACL validator for memory bound validation.
    Designed to work independently without complex dependencies.
    ───────────────────────────────────────────────────────────── */

#ifndef SHACL_VALIDATOR_STANDALONE_H
#define SHACL_VALIDATOR_STANDALONE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// BASIC TYPES AND CONSTANTS
// ============================================================================

#define SHACL_VALIDATOR_VERSION_MAJOR 1
#define SHACL_VALIDATOR_VERSION_MINOR 0

// Memory bound limits for ARENAC compliance
#define SHACL_MAX_MEMORY_PER_NODE    (1024 * 1024)     // 1MB per node
#define SHACL_MAX_MEMORY_PER_GRAPH   (64 * 1024 * 1024) // 64MB per graph
#define SHACL_MAX_CONSTRAINTS        1024               // Max constraints per shape

// 7T performance limits
#define SHACL_MAX_TICKS_PER_NODE     7
#define SHACL_MAX_TICKS_PER_CONSTRAINT 2

// Magic number for validator integrity
#define SHACL_VALIDATOR_MAGIC 0x53484143  // 'SHAC'

// Result codes
typedef enum {
    SHACL_OK = 0,
    SHACL_ERROR_MEMORY = 1,
    SHACL_ERROR_INVALID_ARG = 2,
    SHACL_ERROR_NOT_FOUND = 3,
    SHACL_ERROR_CAPACITY = 4,
    SHACL_ERROR_CORRUPTION = 5
} shacl_result_t;

// Tick counter type
typedef uint64_t shacl_tick_t;

// ============================================================================
// CONSTRAINT TYPES - 80/20 SUBSET
// ============================================================================

typedef enum {
    SHACL_CONSTRAINT_MEMORY_BOUND = 0,    // Custom: memory bound check
    SHACL_CONSTRAINT_MIN_COUNT,           // sh:minCount
    SHACL_CONSTRAINT_MAX_COUNT,           // sh:maxCount  
    SHACL_CONSTRAINT_CLASS,               // sh:class
    SHACL_CONSTRAINT_DATATYPE,            // sh:datatype
    SHACL_CONSTRAINT_NODE_KIND,           // sh:nodeKind
    SHACL_CONSTRAINT_COUNT                // Number of constraint types
} shacl_constraint_type_t;

// Node kinds for validation
typedef enum {
    SHACL_NODE_KIND_IRI = 0,
    SHACL_NODE_KIND_BLANK_NODE,
    SHACL_NODE_KIND_LITERAL,
    SHACL_NODE_KIND_ANY
} shacl_node_kind_t;

// Result levels
typedef enum {
    SHACL_RESULT_INFO = 0,
    SHACL_RESULT_WARNING,
    SHACL_RESULT_VIOLATION,
    SHACL_RESULT_MEMORY_VIOLATION
} shacl_result_level_t;

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
} shacl_memory_footprint_t;

// Constraint value union - memory efficient
typedef union {
    uint32_t count_value;        // For min/max count
    size_t memory_limit;         // For memory bound constraint
    shacl_node_kind_t node_kind; // For node kind
    char *string_value;          // For pattern, datatype, class
} shacl_constraint_value_t;

// Individual constraint
typedef struct shacl_constraint {
    shacl_constraint_type_t type;
    shacl_constraint_value_t value;
    char *property_path;         // Property this constraint applies to
    char *message;               // Custom error message
    shacl_memory_footprint_t memory; // Memory requirements
    struct shacl_constraint *next;
} shacl_constraint_t;

// Shape definition
typedef struct shacl_shape {
    char *shape_iri;             // Shape identifier
    char *target_class;          // Target class (simplified targeting)
    shacl_constraint_t *constraints; // Linked list of constraints
    uint32_t constraint_count;   // Number of constraints
    shacl_memory_footprint_t memory; // Memory footprint of this shape
    bool active;                 // Shape is active for validation
    uint32_t shape_id;           // Numeric ID for fast lookup
} shacl_shape_t;

// Validation result
typedef struct shacl_validation_result {
    char *focus_node;            // Node that failed validation
    char *property_path;         // Property path (if applicable)
    char *value;                 // Failing value
    shacl_constraint_type_t constraint_type; // Constraint that failed
    shacl_result_level_t level;  // Result severity level
    char *message;               // Validation message
    shacl_memory_footprint_t memory; // Memory usage details
    struct shacl_validation_result *next;   // Next result in list
} shacl_validation_result_t;

// Validation report
typedef struct {
    bool conforms;               // True if data conforms to shapes
    shacl_validation_result_t *results;     // List of validation results
    uint32_t result_count;       // Number of results
    uint32_t violation_count;    // Number of violations
    uint32_t memory_violation_count; // Number of memory violations
    shacl_memory_footprint_t total_memory; // Total memory usage
    shacl_tick_t validation_ticks; // Time taken for validation
    uint32_t nodes_validated;    // Number of nodes validated
} shacl_validation_report_t;

// Main SHACL validator - simplified
typedef struct {
    // Shape storage
    shacl_shape_t *shapes;       // Array of shapes
    uint32_t shape_count;        // Number of loaded shapes
    uint32_t shape_capacity;     // Shape array capacity
    
    // Configuration
    size_t max_memory_per_node;  // Memory limit per node
    size_t max_memory_per_graph; // Memory limit per graph
    bool strict_memory_mode;     // Strict memory enforcement
    
    // Statistics
    uint64_t validations_performed; // Total validations
    uint64_t memory_violations;  // Memory violations found
    shacl_tick_t total_ticks;    // Total validation time
    
    // Memory tracking
    shacl_memory_footprint_t memory;
    
    // Integrity
    uint32_t magic;              // Magic number for validation
} shacl_validator_t;

// Simple graph type for testing
typedef struct {
    void *data;
    size_t size;
} shacl_graph_t;

// ============================================================================
// FUNCTION DECLARATIONS
// ============================================================================

// Validator lifecycle
shacl_validator_t* shacl_validator_create(size_t max_memory_per_node, size_t max_memory_per_graph);
shacl_validator_t* shacl_validator_create_default(void);
void shacl_validator_destroy(shacl_validator_t *validator);
shacl_result_t shacl_validator_clear(shacl_validator_t *validator);

// Shape management
shacl_result_t shacl_load_shape(shacl_validator_t *validator, const char *shape_iri, const char *target_class);
shacl_result_t shacl_add_constraint(shacl_validator_t *validator, const char *shape_iri, 
                                   shacl_constraint_type_t type, const char *property_path,
                                   const shacl_constraint_value_t *value);
shacl_result_t shacl_add_memory_constraint(shacl_validator_t *validator, const char *shape_iri,
                                          const char *property_path, size_t memory_limit);
const shacl_shape_t* shacl_get_shape(const shacl_validator_t *validator, const char *shape_iri);

// Validation functions
shacl_result_t shacl_validate_node(shacl_validator_t *validator, const shacl_graph_t *graph,
                                  const char *node_iri, shacl_validation_report_t *report);
shacl_result_t shacl_validate_node_shape(shacl_validator_t *validator, const shacl_graph_t *graph,
                                         const char *node_iri, const shacl_shape_t *shape,
                                         shacl_validation_report_t *report);

// Constraint evaluation
shacl_result_t shacl_eval_constraint(shacl_validator_t *validator, const shacl_graph_t *graph,
                                     const char *focus_node, const char *property_path,
                                     const shacl_constraint_t *constraint, bool *conforms,
                                     shacl_memory_footprint_t *memory);
bool shacl_eval_memory_bound(const shacl_graph_t *graph, const char *focus_node,
                            size_t memory_limit, shacl_memory_footprint_t *footprint);
bool shacl_eval_count_constraint(const shacl_graph_t *graph, const char *focus_node,
                                const char *property_path, uint32_t min_count, uint32_t max_count,
                                uint32_t *actual_count);
bool shacl_eval_node_kind_constraint(const shacl_graph_t *graph, const char *value,
                                    shacl_node_kind_t node_kind);

// Validation reporting
shacl_validation_report_t* shacl_create_report(shacl_validator_t *validator);
shacl_result_t shacl_add_result(shacl_validation_report_t *report, const char *focus_node,
                               const char *property_path, const char *value,
                               shacl_constraint_type_t constraint_type, shacl_result_level_t level,
                               const char *message, const shacl_memory_footprint_t *memory);
shacl_result_t shacl_finalize_report(shacl_validation_report_t *report);
void shacl_destroy_report(shacl_validation_report_t *report);

// Utility functions
const char* shacl_constraint_type_string(shacl_constraint_type_t type);
const char* shacl_result_level_string(shacl_result_level_t level);
const char* shacl_node_kind_string(shacl_node_kind_t node_kind);
shacl_result_t shacl_print_report(const shacl_validation_report_t *report, FILE *output);

// Performance and integrity
size_t shacl_validator_memory_usage(const shacl_validator_t *validator);
bool shacl_validator_valid(const shacl_validator_t *validator);
shacl_result_t shacl_validate_validator(const shacl_validator_t *validator);

#ifdef __cplusplus
}
#endif

#endif // SHACL_VALIDATOR_STANDALONE_H