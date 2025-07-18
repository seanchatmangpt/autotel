#ifndef SHACL_ADVANCED_H
#define SHACL_ADVANCED_H

#include "shacl7t.h"
#include "sparql7t.h"
#include <stdint.h>

// Advanced SHACL constraint types
typedef enum {
    SHACL_NODE_KIND      = 1 << 20,  // sh:nodeKind (IRI, Literal, BlankNode)
    SHACL_MIN_COUNT      = 1 << 21,  // sh:minCount
    SHACL_MAX_COUNT      = 1 << 22,  // sh:maxCount
    SHACL_MIN_LENGTH     = 1 << 23,  // sh:minLength
    SHACL_MAX_LENGTH     = 1 << 24,  // sh:maxLength
    SHACL_PATTERN        = 1 << 25,  // sh:pattern (regex)
    SHACL_MIN_INCLUSIVE  = 1 << 26,  // sh:minInclusive
    SHACL_MAX_INCLUSIVE  = 1 << 27,  // sh:maxInclusive
    SHACL_MIN_EXCLUSIVE  = 1 << 28,  // sh:minExclusive
    SHACL_MAX_EXCLUSIVE  = 1 << 29,  // sh:maxExclusive
    SHACL_IN             = 1 << 30,  // sh:in (value in list)
    SHACL_CLOSED         = 1ULL << 31, // sh:closed
    SHACL_QUALIFIED_SHAPE = 1ULL << 32, // sh:qualifiedValueShape
    SHACL_SPARQL         = 1ULL << 33, // sh:sparql constraint
    SHACL_AND            = 1ULL << 34, // sh:and
    SHACL_OR             = 1ULL << 35, // sh:or
    SHACL_NOT            = 1ULL << 36, // sh:not
    SHACL_XONE           = 1ULL << 37  // sh:xone (exactly one)
} ShaclAdvancedConstraint;

// Property path types for complex navigation
typedef enum {
    PATH_PREDICATE,      // Simple predicate
    PATH_INVERSE,        // Inverse path (^p)
    PATH_SEQUENCE,       // Sequence path (p1/p2)
    PATH_ALTERNATIVE,    // Alternative path (p1|p2)
    PATH_ZERO_OR_MORE,   // Zero or more (p*)
    PATH_ONE_OR_MORE,    // One or more (p+)
    PATH_ZERO_OR_ONE     // Zero or one (p?)
} PropertyPathType;

// Property path representation
typedef struct PropertyPath {
    PropertyPathType type;
    union {
        uint32_t predicate;           // For simple paths
        struct PropertyPath* inverse; // For inverse paths
        struct {
            struct PropertyPath* left;
            struct PropertyPath* right;
        } binary;                     // For sequence/alternative
        struct PropertyPath* repeated; // For */+/? paths
    };
} PropertyPath;

// Extended shape with advanced constraints
typedef struct {
    CompiledShape base;
    
    // Cardinality constraints
    uint16_t min_count;
    uint16_t max_count;
    
    // Value constraints
    uint32_t* in_values;
    size_t in_values_count;
    
    // Numeric constraints
    int64_t min_inclusive;
    int64_t max_inclusive;
    int64_t min_exclusive;
    int64_t max_exclusive;
    
    // String constraints
    uint16_t min_length;
    uint16_t max_length;
    uint32_t pattern_id;  // Reference to compiled regex
    
    // Logical constraints
    uint32_t* and_shapes;
    uint32_t* or_shapes;
    uint32_t* not_shapes;
    uint32_t* xone_shapes;
    size_t and_count, or_count, not_count, xone_count;
    
    // Property paths
    PropertyPath* property_path;
    
    // SPARQL constraints
    uint32_t sparql_constraint_id;
} ExtendedShape;

// Advanced SHACL engine
typedef struct {
    ShaclEngine* base_engine;
    S7TEngine* sparql_engine;
    
    ExtendedShape* extended_shapes;
    size_t extended_shape_count;
    size_t extended_shape_capacity;
    
    // Compiled regex patterns
    void** compiled_patterns;  // Platform-specific regex
    size_t pattern_count;
    
    // SPARQL constraint storage
    char** sparql_constraints;
    size_t sparql_count;
    
    // Validation report accumulator
    uint32_t* violation_nodes;
    uint32_t* violation_shapes;
    size_t violation_count;
    size_t violation_capacity;
} AdvancedShaclEngine;

// Advanced validation functions
AdvancedShaclEngine* shacl_create_advanced(ShaclEngine* base, S7TEngine* sparql);
void shacl_destroy_advanced(AdvancedShaclEngine* e);

// Add extended shapes
uint32_t shacl_add_extended_shape(AdvancedShaclEngine* e, ExtendedShape* shape);
void shacl_set_min_count(AdvancedShaclEngine* e, uint32_t shape_id, uint16_t min);
void shacl_set_max_count(AdvancedShaclEngine* e, uint32_t shape_id, uint16_t max);
void shacl_set_in_values(AdvancedShaclEngine* e, uint32_t shape_id, 
                        uint32_t* values, size_t count);

// Property path operations
PropertyPath* shacl_create_predicate_path(uint32_t predicate);
PropertyPath* shacl_create_inverse_path(PropertyPath* path);
PropertyPath* shacl_create_sequence_path(PropertyPath* left, PropertyPath* right);
PropertyPath* shacl_create_alternative_path(PropertyPath* left, PropertyPath* right);

// Advanced validation
int shacl_validate_advanced(AdvancedShaclEngine* e, uint32_t node, uint32_t shape_id);
void shacl_validate_graph(AdvancedShaclEngine* e);
void shacl_get_validation_report(AdvancedShaclEngine* e, 
                                uint32_t** nodes, uint32_t** shapes, size_t* count);

// Path evaluation
void shacl_evaluate_path(AdvancedShaclEngine* e, uint32_t start_node, 
                        PropertyPath* path, uint64_t* result_nodes);

#endif