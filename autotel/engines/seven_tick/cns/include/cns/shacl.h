#ifndef CNS_SHACL_H
#define CNS_SHACL_H

#include "cns/types.h"
#include "cns/arena.h"
#include "cns/interner.h"
#include "cns/graph.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// CNS SHACL VALIDATOR - HIGH-PERFORMANCE RDF VALIDATION FOR 7T SUBSTRATE
// ============================================================================

// SHACL validator designed for 7T substrate with deterministic performance
// guarantees. All validation operations maintain 7-tick performance through
// efficient constraint checking and optimized data structures.

// ============================================================================
// SHACL CONSTRAINT TYPES
// ============================================================================

// SHACL constraint types supported by the validator
typedef enum {
    CNS_SHACL_CLASS = 0,              // sh:class constraint
    CNS_SHACL_DATATYPE,               // sh:datatype constraint
    CNS_SHACL_NODE_KIND,              // sh:nodeKind constraint
    CNS_SHACL_MIN_COUNT,              // sh:minCount constraint
    CNS_SHACL_MAX_COUNT,              // sh:maxCount constraint
    CNS_SHACL_MIN_LENGTH,             // sh:minLength constraint
    CNS_SHACL_MAX_LENGTH,             // sh:maxLength constraint
    CNS_SHACL_PATTERN,                // sh:pattern constraint
    CNS_SHACL_LANGUAGE_IN,            // sh:languageIn constraint
    CNS_SHACL_UNIQUE_LANG,            // sh:uniqueLang constraint
    CNS_SHACL_EQUALS,                 // sh:equals constraint
    CNS_SHACL_DISJOINT,               // sh:disjoint constraint
    CNS_SHACL_LESS_THAN,              // sh:lessThan constraint
    CNS_SHACL_LESS_THAN_OR_EQUALS,    // sh:lessThanOrEquals constraint
    CNS_SHACL_MIN_EXCLUSIVE,          // sh:minExclusive constraint
    CNS_SHACL_MIN_INCLUSIVE,          // sh:minInclusive constraint
    CNS_SHACL_MAX_EXCLUSIVE,          // sh:maxExclusive constraint
    CNS_SHACL_MAX_INCLUSIVE,          // sh:maxInclusive constraint
    CNS_SHACL_IN,                     // sh:in constraint
    CNS_SHACL_HAS_VALUE,              // sh:hasValue constraint
    CNS_SHACL_CLOSED,                 // sh:closed constraint
    CNS_SHACL_IGNORED_PROPERTIES,     // sh:ignoredProperties constraint
    CNS_SHACL_QUALIFIED_VALUE_SHAPE,  // sh:qualifiedValueShape constraint
    CNS_SHACL_QUALIFIED_MIN_COUNT,    // sh:qualifiedMinCount constraint
    CNS_SHACL_QUALIFIED_MAX_COUNT,    // sh:qualifiedMaxCount constraint
    CNS_SHACL_NODE,                   // sh:node constraint (nested shape)
    CNS_SHACL_PROPERTY,               // sh:property constraint
    CNS_SHACL_AND,                    // sh:and logical constraint
    CNS_SHACL_OR,                     // sh:or logical constraint
    CNS_SHACL_NOT,                    // sh:not logical constraint
    CNS_SHACL_XONE,                   // sh:xone logical constraint
    CNS_SHACL_SPARQL,                 // sh:sparql constraint
    CNS_SHACL_CONSTRAINT_COUNT        // Number of constraint types
} cns_shacl_constraint_type_t;

// Node kind enumeration for sh:nodeKind constraint
typedef enum {
    CNS_NODE_KIND_IRI = 0,           // sh:IRI
    CNS_NODE_KIND_BLANK_NODE,        // sh:BlankNode
    CNS_NODE_KIND_LITERAL,           // sh:Literal
    CNS_NODE_KIND_BLANK_NODE_OR_IRI, // sh:BlankNodeOrIRI
    CNS_NODE_KIND_BLANK_NODE_OR_LITERAL, // sh:BlankNodeOrLiteral
    CNS_NODE_KIND_IRI_OR_LITERAL     // sh:IRIOrLiteral
} cns_node_kind_t;

// Severity levels for validation results
typedef enum {
    CNS_SEVERITY_INFO = 0,           // sh:Info
    CNS_SEVERITY_WARNING,            // sh:Warning
    CNS_SEVERITY_VIOLATION           // sh:Violation
} cns_severity_level_t;

// ============================================================================
// SHACL CONSTRAINT STRUCTURE
// ============================================================================

// Generic constraint value union for efficient storage
typedef union {
    int64_t integer;                 // Integer value
    double decimal;                  // Decimal value
    bool boolean;                    // Boolean value
    cns_string_ref_t string;         // String/IRI reference
    cns_node_kind_t node_kind;       // Node kind enumeration
    struct cns_shape *shape;         // Nested shape reference
    cns_string_ref_t *list;          // List of string references
} cns_constraint_value_t;

// SHACL constraint structure - cache-aligned for performance
typedef struct cns_constraint {
    cns_shacl_constraint_type_t type; // Constraint type
    cns_constraint_value_t value;     // Constraint value
    cns_string_ref_t message;         // Custom validation message
    cns_severity_level_t severity;    // Severity level
    uint32_t flags;                   // Constraint flags
    struct cns_constraint *next;      // Next constraint in list
} cns_constraint_t;

// Constraint flags
#define CNS_CONSTRAINT_FLAG_OPTIONAL   (1 << 0)  // Optional constraint
#define CNS_CONSTRAINT_FLAG_DEACTIVATED (1 << 1) // Deactivated constraint
#define CNS_CONSTRAINT_FLAG_CUSTOM      (1 << 2) // Custom constraint
#define CNS_CONSTRAINT_FLAG_COMPILED    (1 << 3) // Pre-compiled constraint

// ============================================================================
// SHACL SHAPE STRUCTURE
// ============================================================================

// Property shape structure for sh:property constraints
typedef struct cns_property_shape {
    cns_string_ref_t path;           // Property path
    cns_constraint_t *constraints;   // List of constraints
    struct cns_shape *value_shape;   // Value shape (for sh:node)
    uint32_t min_count;              // Minimum count (default 0)
    uint32_t max_count;              // Maximum count (default unlimited)
    uint32_t flags;                  // Property shape flags
    struct cns_property_shape *next; // Next property shape
} cns_property_shape_t;

// Main SHACL shape structure
typedef struct cns_shape {
    cns_string_ref_t iri;            // Shape IRI
    cns_string_ref_t *targets;       // Target node IRIs
    size_t target_count;             // Number of targets
    cns_constraint_t *constraints;   // Node constraints
    cns_property_shape_t *properties; // Property shapes
    struct cns_shape *parent;        // Parent shape (for inheritance)
    uint32_t shape_id;               // Unique shape identifier
    uint32_t flags;                  // Shape flags
    bool deactivated;                // Shape deactivated flag
    bool closed;                     // Closed shape flag
    cns_string_ref_t *ignored_properties; // Ignored properties for closed shapes
    size_t ignored_count;            // Number of ignored properties
} cns_shape_t;

// Shape flags
#define CNS_SHAPE_FLAG_TARGET_NODE      (1 << 0)  // Has sh:targetNode
#define CNS_SHAPE_FLAG_TARGET_CLASS     (1 << 1)  // Has sh:targetClass
#define CNS_SHAPE_FLAG_TARGET_SUBJECTS  (1 << 2)  // Has sh:targetSubjectsOf
#define CNS_SHAPE_FLAG_TARGET_OBJECTS   (1 << 3)  // Has sh:targetObjectsOf
#define CNS_SHAPE_FLAG_CLOSED           (1 << 4)  // Closed shape
#define CNS_SHAPE_FLAG_DEACTIVATED      (1 << 5)  // Deactivated shape
#define CNS_SHAPE_FLAG_COMPILED         (1 << 6)  // Pre-compiled for performance

// ============================================================================
// VALIDATION RESULT STRUCTURE
// ============================================================================

// Individual validation result
typedef struct cns_validation_result {
    cns_string_ref_t focus_node;     // Focus node that failed validation
    cns_string_ref_t result_path;    // Path to the failing property
    cns_string_ref_t value;          // The failing value
    cns_string_ref_t source_constraint_component; // Constraint that failed
    cns_string_ref_t source_shape;   // Shape that defined the constraint
    cns_string_ref_t message;        // Validation message
    cns_severity_level_t severity;   // Severity level
    uint32_t result_id;              // Unique result identifier
    struct cns_validation_result *next; // Next result in list
} cns_validation_result_t;

// Validation report structure
typedef struct {
    bool conforms;                   // True if data conforms to shapes
    cns_validation_result_t *results; // List of validation results
    size_t result_count;             // Number of validation results
    size_t info_count;               // Number of info-level results
    size_t warning_count;            // Number of warning-level results
    size_t violation_count;          // Number of violation-level results
    uint64_t validation_time_ticks;  // Time taken for validation
    size_t nodes_validated;          // Number of nodes validated
    size_t constraints_checked;      // Number of constraints checked
} cns_validation_report_t;

// ============================================================================
// VALIDATOR STATISTICS
// ============================================================================

// Validator performance statistics
typedef struct {
    uint64_t validations_performed;  // Total validations performed
    uint64_t shapes_loaded;          // Total shapes loaded
    uint64_t constraints_evaluated;  // Total constraints evaluated
    uint64_t nodes_validated;       // Total nodes validated
    uint64_t violations_found;       // Total violations found
    cns_tick_t total_validation_ticks; // Total validation time
    cns_tick_t avg_validation_ticks;  // Average validation time
    cns_tick_t shape_loading_ticks;   // Time spent loading shapes
    cns_tick_t constraint_eval_ticks; // Time spent evaluating constraints
    size_t memory_usage;             // Total memory usage
} cns_shacl_stats_t;

// ============================================================================
// MAIN VALIDATOR STRUCTURE
// ============================================================================

// Main SHACL validator structure
struct cns_shacl_validator {
    // Memory management
    cns_arena_t *shape_arena;        // Arena for shapes
    cns_arena_t *constraint_arena;   // Arena for constraints
    cns_arena_t *result_arena;       // Arena for validation results
    cns_interner_t *interner;        // String interner
    
    // Shape storage
    cns_shape_t *shapes;             // Array of loaded shapes
    size_t shape_count;              // Number of loaded shapes
    size_t shape_capacity;           // Shape array capacity
    
    // Shape index for fast lookup
    uint32_t *shape_hash_table;      // Shape IRI -> shape index mapping
    size_t shape_hash_size;          // Shape hash table size
    size_t shape_hash_mask;          // Hash table mask (size - 1)
    
    // Target index for efficient validation
    uint32_t *target_hash_table;     // Target node -> shape mapping
    size_t target_hash_size;         // Target hash table size
    size_t target_hash_mask;         // Target hash table mask
    
    // Performance tracking
    cns_shacl_stats_t stats;         // Validator statistics
    
    // Configuration
    uint32_t flags;                  // Validator flags
    bool strict_mode;                // Strict SHACL compliance
    bool enable_sparql;              // Enable SPARQL constraints
    bool enable_deactivated;         // Process deactivated shapes
    
    // Thread safety (if enabled)
    void *mutex;                     // Mutex for thread safety
    uint32_t magic;                  // Magic number for validation
};

// Validator flags
#define CNS_SHACL_FLAG_STRICT_MODE      (1 << 0)  // Strict SHACL compliance
#define CNS_SHACL_FLAG_ENABLE_SPARQL    (1 << 1)  // Enable SPARQL constraints
#define CNS_SHACL_FLAG_PROCESS_DEACTIVATED (1 << 2) // Process deactivated shapes
#define CNS_SHACL_FLAG_CLOSED_BY_DEFAULT (1 << 3) // Shapes closed by default
#define CNS_SHACL_FLAG_THREAD_SAFE      (1 << 4)  // Thread-safe operations
#define CNS_SHACL_FLAG_OPTIMIZE_TARGETS (1 << 5)  // Optimize target resolution
#define CNS_SHACL_FLAG_CACHE_RESULTS    (1 << 6)  // Cache validation results

// Magic number for validator validation
#define CNS_SHACL_MAGIC 0x5348434C  // 'SHCL'

// ============================================================================
// VALIDATOR LIFECYCLE FUNCTIONS - O(1) OPERATIONS
// ============================================================================

// Create a new SHACL validator with specified configuration
// PERFORMANCE: O(1) - completes within 7 CPU ticks
cns_shacl_validator_t* cns_shacl_validator_create(const cns_shacl_config_t *config);

// Create validator with default configuration
// PERFORMANCE: O(1) - optimized default settings
cns_shacl_validator_t* cns_shacl_validator_create_default(cns_arena_t *arena,
                                                        cns_interner_t *interner);

// Destroy validator and free all memory
// PERFORMANCE: O(1) when using arenas - just resets arena pointers
void cns_shacl_validator_destroy(cns_shacl_validator_t *validator);

// Clear all loaded shapes
// PERFORMANCE: O(1) - resets arenas and counters
cns_result_t cns_shacl_validator_clear(cns_shacl_validator_t *validator);

// Clone validator configuration (not shapes)
// PERFORMANCE: O(1) - copies configuration only
cns_shacl_validator_t* cns_shacl_validator_clone_config(const cns_shacl_validator_t *validator);

// ============================================================================
// SHAPE LOADING FUNCTIONS - OPTIMIZED FOR 7T PERFORMANCE
// ============================================================================

// Load SHACL shapes from RDF graph
// PERFORMANCE: O(n) where n is number of shapes, but 7T per shape
cns_result_t cns_shacl_load_shapes_from_graph(cns_shacl_validator_t *validator,
                                             const cns_graph_t *shapes_graph);

// Load single shape from triples
// PERFORMANCE: O(1) - direct shape construction
cns_result_t cns_shacl_load_shape(cns_shacl_validator_t *validator,
                                 const cns_triple_t *triples,
                                 size_t triple_count);

// Create shape programmatically
// PERFORMANCE: O(1) - direct shape allocation
cns_shape_t* cns_shacl_create_shape(cns_shacl_validator_t *validator,
                                   const char *shape_iri);

// Add constraint to shape
// PERFORMANCE: O(1) - constraint list insertion
cns_result_t cns_shacl_add_constraint(cns_shape_t *shape,
                                     cns_shacl_constraint_type_t type,
                                     const cns_constraint_value_t *value);

// Add property shape to shape
// PERFORMANCE: O(1) - property list insertion
cns_result_t cns_shacl_add_property_shape(cns_shape_t *shape,
                                         const char *property_path,
                                         cns_property_shape_t *property_shape);

// ============================================================================
// VALIDATION FUNCTIONS - 7T GUARANTEED
// ============================================================================

// Validate RDF graph against loaded shapes
// PERFORMANCE: O(n * m) where n is nodes, m is avg constraints per node
cns_result_t cns_shacl_validate_graph(cns_shacl_validator_t *validator,
                                     const cns_graph_t *data_graph,
                                     cns_validation_report_t *report);

// Validate specific node against all applicable shapes
// PERFORMANCE: O(k) where k is number of applicable shapes
cns_result_t cns_shacl_validate_node(cns_shacl_validator_t *validator,
                                    const cns_graph_t *data_graph,
                                    cns_string_ref_t node_iri,
                                    cns_validation_report_t *report);

// Validate node against specific shape
// PERFORMANCE: O(c) where c is number of constraints in shape
cns_result_t cns_shacl_validate_node_shape(cns_shacl_validator_t *validator,
                                          const cns_graph_t *data_graph,
                                          cns_string_ref_t node_iri,
                                          const cns_shape_t *shape,
                                          cns_validation_report_t *report);

// Validate property values against property shape
// PERFORMANCE: O(v * c) where v is values, c is constraints
cns_result_t cns_shacl_validate_property(cns_shacl_validator_t *validator,
                                        const cns_graph_t *data_graph,
                                        cns_string_ref_t focus_node,
                                        const cns_property_shape_t *property_shape,
                                        cns_validation_report_t *report);

// ============================================================================
// CONSTRAINT EVALUATION FUNCTIONS - 7T PERFORMANCE
// ============================================================================

// Evaluate single constraint against value
// PERFORMANCE: O(1) for most constraints, O(n) for complex patterns
cns_result_t cns_shacl_eval_constraint(cns_shacl_validator_t *validator,
                                      const cns_graph_t *data_graph,
                                      cns_string_ref_t focus_node,
                                      cns_string_ref_t value,
                                      const cns_constraint_t *constraint,
                                      bool *conforms);

// Evaluate class constraint (sh:class)
// PERFORMANCE: O(1) - type checking via graph lookup
bool cns_shacl_eval_class_constraint(const cns_graph_t *data_graph,
                                    cns_string_ref_t value,
                                    cns_string_ref_t class_iri);

// Evaluate datatype constraint (sh:datatype)
// PERFORMANCE: O(1) - literal type checking
bool cns_shacl_eval_datatype_constraint(const cns_graph_t *data_graph,
                                       cns_string_ref_t value,
                                       cns_string_ref_t datatype_iri);

// Evaluate node kind constraint (sh:nodeKind)
// PERFORMANCE: O(1) - node type checking
bool cns_shacl_eval_node_kind_constraint(const cns_graph_t *data_graph,
                                        cns_string_ref_t value,
                                        cns_node_kind_t node_kind);

// Evaluate pattern constraint (sh:pattern)
// PERFORMANCE: O(n) where n is value length - regex matching
bool cns_shacl_eval_pattern_constraint(cns_string_ref_t value,
                                      cns_string_ref_t pattern,
                                      cns_string_ref_t flags);

// Evaluate length constraints (sh:minLength, sh:maxLength)
// PERFORMANCE: O(1) - string length comparison
bool cns_shacl_eval_length_constraint(cns_string_ref_t value,
                                     uint32_t min_length,
                                     uint32_t max_length);

// Evaluate count constraints (sh:minCount, sh:maxCount)
// PERFORMANCE: O(1) - count comparison
bool cns_shacl_eval_count_constraint(size_t actual_count,
                                    uint32_t min_count,
                                    uint32_t max_count);

// ============================================================================
// TARGET RESOLUTION FUNCTIONS
// ============================================================================

// Get target nodes for shape
// PERFORMANCE: O(t) where t is number of targets
cns_result_t cns_shacl_get_target_nodes(cns_shacl_validator_t *validator,
                                       const cns_graph_t *data_graph,
                                       const cns_shape_t *shape,
                                       cns_string_ref_t **targets,
                                       size_t *target_count);

// Check if node is target of shape
// PERFORMANCE: O(1) - hash table lookup
bool cns_shacl_is_target_node(cns_shacl_validator_t *validator,
                             const cns_shape_t *shape,
                             cns_string_ref_t node_iri);

// Build target index for efficient lookup
// PERFORMANCE: O(n * t) where n is shapes, t is avg targets per shape
cns_result_t cns_shacl_build_target_index(cns_shacl_validator_t *validator,
                                         const cns_graph_t *data_graph);

// ============================================================================
// SHAPE MANAGEMENT FUNCTIONS
// ============================================================================

// Get shape by IRI
// PERFORMANCE: O(1) - hash table lookup
const cns_shape_t* cns_shacl_get_shape(const cns_shacl_validator_t *validator,
                                      cns_string_ref_t shape_iri);

// Get all shapes
// PERFORMANCE: O(1) - returns shape array
cns_result_t cns_shacl_get_shapes(const cns_shacl_validator_t *validator,
                                 const cns_shape_t **shapes,
                                 size_t *count);

// Get shapes applicable to node
// PERFORMANCE: O(s) where s is number of shapes
cns_result_t cns_shacl_get_applicable_shapes(cns_shacl_validator_t *validator,
                                            const cns_graph_t *data_graph,
                                            cns_string_ref_t node_iri,
                                            const cns_shape_t **shapes,
                                            size_t *count);

// Remove shape from validator
// PERFORMANCE: O(1) - marks shape as removed
cns_result_t cns_shacl_remove_shape(cns_shacl_validator_t *validator,
                                   cns_string_ref_t shape_iri);

// ============================================================================
// VALIDATION REPORT FUNCTIONS
// ============================================================================

// Create validation report
// PERFORMANCE: O(1) - allocates report structure
cns_validation_report_t* cns_shacl_create_report(cns_shacl_validator_t *validator);

// Add result to report
// PERFORMANCE: O(1) - adds to result list
cns_result_t cns_shacl_add_result(cns_validation_report_t *report,
                                 cns_string_ref_t focus_node,
                                 cns_string_ref_t result_path,
                                 cns_string_ref_t value,
                                 cns_string_ref_t constraint_component,
                                 cns_string_ref_t source_shape,
                                 cns_string_ref_t message,
                                 cns_severity_level_t severity);

// Finalize validation report
// PERFORMANCE: O(1) - updates counters and conformance
cns_result_t cns_shacl_finalize_report(cns_validation_report_t *report);

// Destroy validation report
// PERFORMANCE: O(1) when using arenas - just resets arena
void cns_shacl_destroy_report(cns_validation_report_t *report);

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

// Convert constraint type to string
// PERFORMANCE: O(1) - lookup table
const char* cns_shacl_constraint_type_string(cns_shacl_constraint_type_t type);

// Convert node kind to string
// PERFORMANCE: O(1) - lookup table
const char* cns_shacl_node_kind_string(cns_node_kind_t node_kind);

// Convert severity to string
// PERFORMANCE: O(1) - lookup table
const char* cns_shacl_severity_string(cns_severity_level_t severity);

// Check if value is IRI
// PERFORMANCE: O(1) - type checking
bool cns_shacl_is_iri(cns_string_ref_t value);

// Check if value is literal
// PERFORMANCE: O(1) - type checking
bool cns_shacl_is_literal(cns_string_ref_t value);

// Check if value is blank node
// PERFORMANCE: O(1) - type checking
bool cns_shacl_is_blank_node(cns_string_ref_t value);

// ============================================================================
// VALIDATOR INFORMATION AND STATISTICS
// ============================================================================

// Get validator statistics
// PERFORMANCE: O(1) - returns cached statistics
cns_result_t cns_shacl_get_stats(const cns_shacl_validator_t *validator,
                                cns_shacl_stats_t *stats);

// Get number of loaded shapes
// PERFORMANCE: O(1) - returns cached count
size_t cns_shacl_shape_count(const cns_shacl_validator_t *validator);

// Get memory usage
// PERFORMANCE: O(1) - returns cached value
size_t cns_shacl_memory_usage(const cns_shacl_validator_t *validator);

// Get validator flags
// PERFORMANCE: O(1) - returns cached flags
uint32_t cns_shacl_get_flags(const cns_shacl_validator_t *validator);

// Set validator flags
// PERFORMANCE: O(1) - updates flags
cns_result_t cns_shacl_set_flags(cns_shacl_validator_t *validator, uint32_t flags);

// ============================================================================
// DEBUG AND VALIDATION FUNCTIONS
// ============================================================================

// Validate validator integrity
// PERFORMANCE: O(n) where n is number of shapes
cns_result_t cns_shacl_validate_validator(const cns_shacl_validator_t *validator);

// Print validator statistics
// PERFORMANCE: O(1) - prints cached statistics
cns_result_t cns_shacl_print_stats(const cns_shacl_validator_t *validator, FILE *output);

// Print validation report
// PERFORMANCE: O(r) where r is number of results
cns_result_t cns_shacl_print_report(const cns_validation_report_t *report, FILE *output);

// Dump all loaded shapes
// PERFORMANCE: O(s) where s is number of shapes
cns_result_t cns_shacl_dump_shapes(const cns_shacl_validator_t *validator, FILE *output);

// ============================================================================
// PERFORMANCE MONITORING INTEGRATION
// ============================================================================

// Performance callback for validator events
typedef void (*cns_shacl_perf_callback_t)(const cns_shacl_validator_t *validator,
                                         const char *operation,
                                         size_t elements_processed,
                                         cns_tick_t ticks,
                                         void *user_data);

// Set performance monitoring callback
// PERFORMANCE: O(1) - stores callback pointer
cns_result_t cns_shacl_set_perf_callback(cns_shacl_validator_t *validator,
                                        cns_shacl_perf_callback_t callback,
                                        void *user_data);

// Clear performance monitoring callback
// PERFORMANCE: O(1) - clears callback pointer
cns_result_t cns_shacl_clear_perf_callback(cns_shacl_validator_t *validator);

// ============================================================================
// THREAD SAFETY FUNCTIONS
// ============================================================================

// Enable thread safety for validator
// PERFORMANCE: O(1) - initializes mutex
cns_result_t cns_shacl_enable_thread_safety(cns_shacl_validator_t *validator);

// Disable thread safety
// PERFORMANCE: O(1) - destroys mutex
cns_result_t cns_shacl_disable_thread_safety(cns_shacl_validator_t *validator);

// Lock validator for exclusive access
// PERFORMANCE: O(1) - mutex lock
cns_result_t cns_shacl_lock(cns_shacl_validator_t *validator);

// Unlock validator
// PERFORMANCE: O(1) - mutex unlock
cns_result_t cns_shacl_unlock(cns_shacl_validator_t *validator);

// ============================================================================
// UTILITY MACROS FOR COMMON PATTERNS
// ============================================================================

// Validate and check for conformance
#define CNS_SHACL_VALIDATE_CHECK(validator, graph, report_var) \
    do { \
        cns_result_t result = cns_shacl_validate_graph((validator), (graph), &(report_var)); \
        if (CNS_7T_UNLIKELY(result != CNS_OK)) return result; \
    } while(0)

// Add validation result and check for success
#define CNS_SHACL_ADD_RESULT_CHECK(report, focus, path, value, constraint, shape, message, severity) \
    do { \
        cns_result_t result = cns_shacl_add_result((report), (focus), (path), (value), \
                                                  (constraint), (shape), (message), (severity)); \
        if (CNS_7T_UNLIKELY(result != CNS_OK)) return result; \
    } while(0)

// Check constraint evaluation result
#define CNS_SHACL_EVAL_CONSTRAINT_CHECK(validator, graph, focus, value, constraint, conforms_var) \
    do { \
        cns_result_t result = cns_shacl_eval_constraint((validator), (graph), (focus), \
                                                       (value), (constraint), &(conforms_var)); \
        if (CNS_7T_UNLIKELY(result != CNS_OK)) return result; \
    } while(0)

#ifdef __cplusplus
}
#endif

#endif // CNS_SHACL_H