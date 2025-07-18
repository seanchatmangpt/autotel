#ifndef SEVEN_T_RUNTIME_H
#define SEVEN_T_RUNTIME_H

#include <stdint.h>
#include <stddef.h>
#include "seven_t_config.h"

// Bit vector operations
typedef struct
{
    uint64_t *bits;
    size_t capacity;
    size_t count;
} BitVector;

// Triple store index structures
typedef struct
{
    uint32_t subject_id;
    uint32_t predicate_id;
    uint32_t object_id;
} Triple;

// Main engine state
typedef struct
{
    // Core bit vectors
    BitVector **predicate_vectors; // Array of bit vectors per predicate
    BitVector **object_vectors;    // Array of bit vectors per object

    // Hash table for PS->O lookups
    void *ps_to_o_index;  // PSOHashTable pointer
    void *ps_to_o_counts; // Not used with hash table

    // Cardinality tracking
    uint32_t *node_property_counts; // Per-node property count

    // Type system
    uint32_t *object_type_ids; // Type ID for each object

    // String interning
    char **string_table;
    size_t string_count;
    size_t string_capacity;

    // Statistics
    size_t triple_count;
    size_t max_subject_id;
    size_t max_predicate_id;
    size_t max_object_id;
} EngineState;

// Public API
EngineState *s7t_create_engine(void);
void s7t_destroy_engine(EngineState *engine);
uint32_t s7t_intern_string(EngineState *engine, const char *str);
void s7t_add_triple(EngineState *engine, uint32_t s, uint32_t p, uint32_t o);

// Query primitives
BitVector *s7t_get_subject_vector(EngineState *engine, uint32_t predicate_id, uint32_t object_id);
BitVector *s7t_get_object_vector(EngineState *engine, uint32_t predicate_id, uint32_t subject_id);
uint32_t *s7t_get_objects(EngineState *engine, uint32_t predicate_id, uint32_t subject_id, size_t *count);

// Pattern matching
int s7t_ask_pattern(EngineState *engine, uint32_t s, uint32_t p, uint32_t o);

// Query result materialization
uint32_t *s7t_materialize_subjects(EngineState *engine, uint32_t predicate_id,
                                   uint32_t object_id, size_t *count);

// SHACL validation primitives
int shacl_check_min_count(EngineState *engine, uint32_t subject_id, uint32_t predicate_id, uint32_t min_count);
int shacl_check_max_count(EngineState *engine, uint32_t subject_id, uint32_t predicate_id, uint32_t max_count);
int shacl_check_class(EngineState *engine, uint32_t subject_id, uint32_t class_id);

// Bit vector operations
BitVector *bitvec_create(size_t capacity);
void bitvec_destroy(BitVector *bv);
void bitvec_set(BitVector *bv, size_t index);
int bitvec_test(BitVector *bv, size_t index);
BitVector *bitvec_and(BitVector *a, BitVector *b);
BitVector *bitvec_or(BitVector *a, BitVector *b);
size_t bitvec_popcount(BitVector *bv);

#endif // SEVEN_T_RUNTIME_H
