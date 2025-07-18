#include "owl7t.h"
#include <stdlib.h>
#include <string.h>

// Create OWL reasoning engine
OWLEngine* owl_create(S7TEngine* base, size_t max_classes, size_t max_properties) {
    OWLEngine* e = calloc(1, sizeof(OWLEngine));
    e->base_engine = base;
    e->max_classes = max_classes;
    e->max_properties = max_properties;
    
    // Allocate axiom storage
    e->axiom_capacity = 10000;
    e->axioms = calloc(e->axiom_capacity, sizeof(OWLAxiom));
    
    // Allocate closure matrices (bit-matrices for fast transitive closure)
    size_t class_chunks = (max_classes + 63) / 64;
    size_t prop_chunks = (max_properties + 63) / 64;
    
    e->subclass_closure = aligned_alloc(64, max_classes * class_chunks * sizeof(uint64_t));
    e->subproperty_closure = aligned_alloc(64, max_properties * prop_chunks * sizeof(uint64_t));
    e->equivalent_classes = aligned_alloc(64, class_chunks * sizeof(uint64_t));
    
    // Property characteristic vectors
    e->transitive_properties = calloc(prop_chunks, sizeof(uint64_t));
    e->symmetric_properties = calloc(prop_chunks, sizeof(uint64_t));
    e->functional_properties = calloc(prop_chunks, sizeof(uint64_t));
    e->inverse_functional_properties = calloc(prop_chunks, sizeof(uint64_t));
    
    // Initialize diagonal elements (every class is subclass of itself)
    for (size_t i = 0; i < max_classes; i++) {
        size_t chunk = i / 64;
        uint64_t bit = 1ULL << (i % 64);
        e->subclass_closure[i * class_chunks + chunk] |= bit;
    }
    
    return e;
}

// Add subclass axiom
void owl_add_subclass(OWLEngine* e, uint32_t subclass, uint32_t superclass) {
    if (e->axiom_count >= e->axiom_capacity) {
        e->axiom_capacity *= 2;
        e->axioms = realloc(e->axioms, e->axiom_capacity * sizeof(OWLAxiom));
    }
    
    OWLAxiom* axiom = &e->axioms[e->axiom_count++];
    axiom->subject = subclass;
    axiom->object = superclass;
    axiom->axiom_flags = OWL_SUBCLASS_OF;
    
    // Direct update in closure matrix
    size_t class_chunks = (e->max_classes + 63) / 64;
    size_t chunk = superclass / 64;
    uint64_t bit = 1ULL << (superclass % 64);
    e->subclass_closure[subclass * class_chunks + chunk] |= bit;
}
// Set property as transitive
void owl_set_transitive(OWLEngine* e, uint32_t property) {
    size_t chunk = property / 64;
    uint64_t bit = 1ULL << (property % 64);
    e->transitive_properties[chunk] |= bit;
}

// Set property as symmetric
void owl_set_symmetric(OWLEngine* e, uint32_t property) {
    size_t chunk = property / 64;
    uint64_t bit = 1ULL << (property % 64);
    e->symmetric_properties[chunk] |= bit;
}

// Set property as functional
void owl_set_functional(OWLEngine* e, uint32_t property) {
    size_t chunk = property / 64;
    uint64_t bit = 1ULL << (property % 64);
    e->functional_properties[chunk] |= bit;
}

// Compute transitive closure using Warshall's algorithm (bit-parallel version)
void owl_compute_closures(OWLEngine* e) {
    size_t class_chunks = (e->max_classes + 63) / 64;
    
    // Floyd-Warshall for subclass closure
    for (size_t k = 0; k < e->max_classes; k++) {
        for (size_t i = 0; i < e->max_classes; i++) {
            // Check if i is subclass of k
            size_t k_chunk = k / 64;
            uint64_t k_bit = 1ULL << (k % 64);
            
            if (e->subclass_closure[i * class_chunks + k_chunk] & k_bit) {
                // i is subclass of k, so add all superclasses of k to i
                for (size_t chunk = 0; chunk < class_chunks; chunk++) {
                    e->subclass_closure[i * class_chunks + chunk] |= 
                        e->subclass_closure[k * class_chunks + chunk];
                }
            }
        }
    }
    
    // Similar for property closure
    size_t prop_chunks = (e->max_properties + 63) / 64;
    for (size_t k = 0; k < e->max_properties; k++) {
        for (size_t i = 0; i < e->max_properties; i++) {
            size_t k_chunk = k / 64;
            uint64_t k_bit = 1ULL << (k % 64);
            
            if (e->subproperty_closure[i * prop_chunks + k_chunk] & k_bit) {
                for (size_t chunk = 0; chunk < prop_chunks; chunk++) {
                    e->subproperty_closure[i * prop_chunks + chunk] |= 
                        e->subproperty_closure[k * prop_chunks + chunk];
                }
            }
        }
    }
}
// Materialize inferences based on OWL axioms
void owl_materialize_inferences(OWLEngine* e) {
    // First compute closures
    owl_compute_closures(e);
    
    // Process each axiom type
    for (size_t i = 0; i < e->axiom_count; i++) {
        OWLAxiom* axiom = &e->axioms[i];
        
        if (axiom->axiom_flags & OWL_DOMAIN) {
            // If property has domain, add type assertions
            // For all (s,p,o) where p has domain D, assert (s,rdf:type,D)
            uint32_t property = axiom->subject;
            uint32_t domain = axiom->object;
            
            // Scan all subjects that have this property
            size_t stride = e->base_engine->stride_len;
            uint64_t* p_vec = e->base_engine->predicate_vectors + (property * stride);
            
            for (size_t chunk = 0; chunk < stride; chunk++) {
                uint64_t subjects_with_prop = p_vec[chunk];
                while (subjects_with_prop) {
                    uint32_t bit_idx = __builtin_ctzll(subjects_with_prop);
                    uint32_t subject = (chunk * 64) + bit_idx;
                    
                    // Add type assertion
                    s7t_add_triple(e->base_engine, subject, 0, domain); // 0 = rdf:type
                    
                    subjects_with_prop &= subjects_with_prop - 1;
                }
            }
        }
        
        if (axiom->axiom_flags & OWL_RANGE) {
            // Similar for range restrictions
            uint32_t property = axiom->subject;
            uint32_t range = axiom->object;
            
            // For all objects of this property, add type assertions
            for (size_t s = 0; s < e->base_engine->max_subjects; s++) {
                uint32_t o = e->base_engine->ps_to_o_index[property * e->base_engine->max_subjects + s];
                if (o < e->base_engine->max_objects) {
                    s7t_add_triple(e->base_engine, o, 0, range); // Assert object has type range
                }
            }
        }
    }
    
    // Handle transitive properties
    size_t prop_chunks = (e->max_properties + 63) / 64;
    for (size_t p = 0; p < e->max_properties; p++) {
        size_t chunk = p / 64;
        uint64_t bit = 1ULL << (p % 64);
        
        if (e->transitive_properties[chunk] & bit) {
            // Materialize transitive closure for this property
            // This is complex and would require a separate algorithm
            // For now, we mark it as requiring special handling during query
        }
    }
}
// Query with OWL reasoning
int owl_ask_with_reasoning(OWLEngine* e, uint32_t s, uint32_t p, uint32_t o) {
    // First check base facts
    if (s7t_ask_pattern(e->base_engine, s, p, o)) {
        return 1;
    }
    
    // Check if we can infer this through subclass/subproperty reasoning
    size_t class_chunks = (e->max_classes + 63) / 64;
    size_t prop_chunks = (e->max_properties + 63) / 64;
    
    // If querying for rdf:type (property 0), check subclass closure
    if (p == 0) {
        // Get all types of subject s
        for (uint32_t type = 0; type < e->max_classes; type++) {
            if (s7t_ask_pattern(e->base_engine, s, 0, type)) {
                // Check if type is subclass of o
                size_t o_chunk = o / 64;
                uint64_t o_bit = 1ULL << (o % 64);
                if (e->subclass_closure[type * class_chunks + o_chunk] & o_bit) {
                    return 1;
                }
            }
        }
    }
    
    // Check subproperty reasoning
    for (uint32_t subprop = 0; subprop < e->max_properties; subprop++) {
        // Check if subprop is subproperty of p
        size_t p_chunk = p / 64;
        uint64_t p_bit = 1ULL << (p % 64);
        
        if (e->subproperty_closure[subprop * prop_chunks + p_chunk] & p_bit) {
            if (s7t_ask_pattern(e->base_engine, s, subprop, o)) {
                return 1;
            }
        }
    }
    
    // Check transitive properties
    size_t t_chunk = p / 64;
    uint64_t t_bit = 1ULL << (p % 64);
    if (e->transitive_properties[t_chunk] & t_bit) {
        // Need to check transitive path from s to o via property p
        // This would require a graph traversal - simplified here
        return 0; // TODO: Implement transitive reasoning
    }
    
    return 0;
}

// Get all superclasses of a class
void owl_get_all_superclasses(OWLEngine* e, uint32_t class, uint64_t* result_vector) {
    size_t class_chunks = (e->max_classes + 63) / 64;
    memcpy(result_vector, 
           e->subclass_closure + (class * class_chunks), 
           class_chunks * sizeof(uint64_t));
}

// Get all subclasses of a class
void owl_get_all_subclasses(OWLEngine* e, uint32_t class, uint64_t* result_vector) {
    size_t class_chunks = (e->max_classes + 63) / 64;
    memset(result_vector, 0, class_chunks * sizeof(uint64_t));
    
    // Check each class to see if it's a subclass of the given class
    size_t target_chunk = class / 64;
    uint64_t target_bit = 1ULL << (class % 64);
    
    for (uint32_t c = 0; c < e->max_classes; c++) {
        if (e->subclass_closure[c * class_chunks + target_chunk] & target_bit) {
            size_t c_chunk = c / 64;
            uint64_t c_bit = 1ULL << (c % 64);
            result_vector[c_chunk] |= c_bit;
        }
    }
}

// Clean up
void owl_destroy(OWLEngine* e) {
    if (!e) return;
    
    free(e->axioms);
    free(e->subclass_closure);
    free(e->subproperty_closure);
    free(e->equivalent_classes);
    free(e->transitive_properties);
    free(e->symmetric_properties);
    free(e->functional_properties);
    free(e->inverse_functional_properties);
    free(e);
}