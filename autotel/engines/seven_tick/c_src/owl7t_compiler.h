#ifndef OWL7T_COMPILER_H
#define OWL7T_COMPILER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Axiom types that fit in 7 ticks
typedef enum {
    OWL7T_SUBCLASS = 1,
    OWL7T_EQUIVALENT_CLASS = 2,
    OWL7T_DOMAIN = 4,
    OWL7T_RANGE = 8,
    OWL7T_FUNCTIONAL = 16,
    OWL7T_INVERSE_FUNCTIONAL = 32,
    OWL7T_TRANSITIVE = 64,
    OWL7T_SYMMETRIC = 128
} OWL7T_AxiomType;

// Compiled axiom representation
typedef struct {
    OWL7T_AxiomType type;
    uint32_t subject_id;
    uint32_t object_id;
    uint64_t mask;           // Pre-computed bit mask
    uint8_t tick_cost;       // Estimated CPU cycles
} CompiledAxiom;

// Compilation result
typedef struct {
    CompiledAxiom* axioms;
    size_t axiom_count;
    
    uint64_t* class_masks;   // [class_id] -> parent classes mask
    uint64_t* property_masks; // [prop_id] -> characteristics mask
    
    size_t class_count;
    size_t property_count;
    
    // Statistics
    uint32_t tick_compliant_count;
    uint32_t materialized_count;
    uint32_t rejected_count;
} OWL7T_CompileResult;

// Core API
OWL7T_CompileResult* owl7t_compile(const char* ontology_path, const char* output_dir);
void owl7t_free_result(OWL7T_CompileResult* result);

// Code generation
int owl7t_generate_c_code(const OWL7T_CompileResult* result, const char* output_dir);

// Runtime checking (7 ticks guaranteed)
bool owl7t_check_subclass(const OWL7T_CompileResult* result, uint32_t child, uint32_t parent);
bool owl7t_check_property(const OWL7T_CompileResult* result, uint32_t prop, uint32_t characteristic);

#endif
