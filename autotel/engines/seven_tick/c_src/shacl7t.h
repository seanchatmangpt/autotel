#ifndef SHACL7T_H
#define SHACL7T_H

#include <stdint.h>
#include <stddef.h>

// Shape constraint types that fit in 7 ticks
typedef enum {
    SHACL_TARGET_CLASS = 1,
    SHACL_PROPERTY = 2,
    SHACL_DATATYPE = 4,
    SHACL_MIN_COUNT = 8,
    SHACL_MAX_COUNT = 16,
    SHACL_IN_SET = 32,
    SHACL_PATTERN = 64
} ShapeConstraintType;

// Compiled shape - everything is pre-computed masks
typedef struct {
    uint64_t target_class_mask;    // Which nodes this shape applies to
    uint64_t property_mask;        // Required properties
    uint64_t datatype_mask;        // Expected datatypes
    uint64_t cardinality_mask;     // Min/max count constraints
    uint64_t literal_set_mask;     // sh:in validation
    uint8_t  pattern_dfa[256];     // Pre-compiled regex DFA
    uint32_t constraint_flags;     // Which constraints are active
} CompiledShape;

// Validation engine
typedef struct {
    uint64_t* node_class_vectors;  // [node_id][chunk] class membership
    uint64_t* node_property_vectors; // [node_id][chunk] property existence  
    uint64_t* property_value_vectors; // [prop_id][chunk] value constraints
    uint32_t* node_datatype_index; // [node_id] -> datatype_id
    
    CompiledShape* shapes;         // Array of compiled shapes
    size_t shape_count;
    size_t max_nodes;
    size_t stride_len;
} ShaclEngine;

// Core API - all operations ≤7 ticks
ShaclEngine* shacl_create(size_t max_nodes, size_t max_shapes);
void shacl_add_shape(ShaclEngine* e, size_t shape_id, CompiledShape* shape);
void shacl_set_node_class(ShaclEngine* e, uint32_t node_id, uint32_t class_id);
void shacl_set_node_property(ShaclEngine* e, uint32_t node_id, uint32_t prop_id);
int shacl_validate_node(ShaclEngine* e, uint32_t node_id, uint32_t shape_id);

// Batch validation
void shacl_validate_batch(ShaclEngine* e, uint32_t* nodes, uint32_t* shapes, 
                         int* results, size_t count);

#endif
