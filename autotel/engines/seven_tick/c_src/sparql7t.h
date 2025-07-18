#ifndef SPARQL7T_H
#define SPARQL7T_H

#include <stdint.h>
#include <stddef.h>

// Forward declaration for ObjectNode
typedef struct ObjectNode ObjectNode;

// Core data structure - everything fits in L1 cache
typedef struct
{
    uint64_t *predicate_vectors; // [pred_id][chunk] bit matrix
    uint64_t *object_vectors;    // [obj_id][chunk] bit matrix
    ObjectNode **ps_to_o_index;  // [pred_id * max_subjects + subj_id] -> ObjectNode*

    size_t max_subjects;
    size_t max_predicates;
    size_t max_objects;
    size_t stride_len; // (max_subjects + 63) / 64
} S7TEngine;

// The core operations
S7TEngine *s7t_create(size_t max_s, size_t max_p, size_t max_o);
void s7t_add_triple(S7TEngine *e, uint32_t s, uint32_t p, uint32_t o);
int s7t_ask_pattern(S7TEngine *e, uint32_t s, uint32_t p, uint32_t o);
void s7t_destroy(S7TEngine *e);

// Batch operations for SIMD
typedef struct
{
    uint32_t s, p, o;
} TriplePattern;

void s7t_ask_batch(S7TEngine *e, TriplePattern *patterns, int *results, size_t count);

#endif
