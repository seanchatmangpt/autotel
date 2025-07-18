#ifndef OWL7T_H
#define OWL7T_H

#include "sparql7t.h"
#include <stdint.h>

// OWL 2 RL profile axioms as bit flags
typedef enum {
    OWL_SUBCLASS_OF      = 1 << 0,
    OWL_EQUIVALENT_CLASS = 1 << 1,
    OWL_DISJOINT_WITH    = 1 << 2,
    OWL_SUBPROPERTY_OF   = 1 << 3,
    OWL_INVERSE_OF       = 1 << 4,
    OWL_FUNCTIONAL       = 1 << 5,
    OWL_INVERSE_FUNC     = 1 << 6,
    OWL_TRANSITIVE       = 1 << 7,
    OWL_SYMMETRIC        = 1 << 8,
    OWL_ASYMMETRIC       = 1 << 9,
    OWL_REFLEXIVE        = 1 << 10,
    OWL_IRREFLEXIVE      = 1 << 11,
    OWL_DOMAIN           = 1 << 12,
    OWL_RANGE            = 1 << 13,
    OWL_HAS_VALUE        = 1 << 14,
    OWL_ALL_VALUES_FROM  = 1 << 15,
    OWL_SOME_VALUES_FROM = 1 << 16,
    OWL_MIN_CARDINALITY  = 1 << 17,
    OWL_MAX_CARDINALITY  = 1 << 18,
    OWL_CARDINALITY      = 1 << 19
} OWLAxiomType;

// Compact OWL axiom representation
typedef struct {
    uint32_t subject;
    uint32_t predicate;
    uint32_t object;
    uint32_t axiom_flags;
    uint16_t cardinality;  // For cardinality restrictions
} OWLAxiom;

// OWL reasoning engine extension
typedef struct {
    S7TEngine* base_engine;
    
    // Axiom storage
    OWLAxiom* axioms;
    size_t axiom_count;
    size_t axiom_capacity;
    
    // Precomputed inference closures
    uint64_t* subclass_closure;     // Transitive closure of subClassOf
    uint64_t* subproperty_closure;  // Transitive closure of subPropertyOf
    uint64_t* equivalent_classes;   // Equivalence class representatives
    
    // Property characteristics bit-vectors
    uint64_t* transitive_properties;
    uint64_t* symmetric_properties;
    uint64_t* functional_properties;
    uint64_t* inverse_functional_properties;
    
    size_t max_classes;
    size_t max_properties;
} OWLEngine;

// OWL engine creation and management
OWLEngine* owl_create(S7TEngine* base, size_t max_classes, size_t max_properties);
void owl_destroy(OWLEngine* e);

// Add OWL axioms
void owl_add_subclass(OWLEngine* e, uint32_t subclass, uint32_t superclass);
void owl_add_equivalent_class(OWLEngine* e, uint32_t class1, uint32_t class2);
void owl_add_disjoint_classes(OWLEngine* e, uint32_t class1, uint32_t class2);
void owl_add_subproperty(OWLEngine* e, uint32_t subprop, uint32_t superprop);
void owl_add_property_domain(OWLEngine* e, uint32_t property, uint32_t domain);
void owl_add_property_range(OWLEngine* e, uint32_t property, uint32_t range);

// Property characteristics
void owl_set_transitive(OWLEngine* e, uint32_t property);
void owl_set_symmetric(OWLEngine* e, uint32_t property);
void owl_set_functional(OWLEngine* e, uint32_t property);
void owl_set_inverse_functional(OWLEngine* e, uint32_t property);

// Reasoning operations
void owl_compute_closures(OWLEngine* e);
void owl_materialize_inferences(OWLEngine* e);

// Query with reasoning
int owl_ask_with_reasoning(OWLEngine* e, uint32_t s, uint32_t p, uint32_t o);
void owl_get_all_superclasses(OWLEngine* e, uint32_t class, uint64_t* result_vector);
void owl_get_all_subclasses(OWLEngine* e, uint32_t class, uint64_t* result_vector);

// Consistency checking
int owl_check_consistency(OWLEngine* e);
void owl_find_inconsistent_classes(OWLEngine* e, uint64_t* result_vector);

#endif