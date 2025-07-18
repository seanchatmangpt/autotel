#ifndef SPARQL7T_H
#define SPARQL7T_H

#include <stdint.h>
#include <stddef.h>

// Core data structure - everything fits in L1 cache
typedef struct {
    uint64_t* predicate_vectors;  // [pred_id][chunk] bit matrix
    uint64_t* object_vectors;     // [obj_id][chunk] bit matrix
    uint32_t* ps_to_o_index;      // [pred_id * max_subjects + subj_id] -> obj_id
    
    size_t max_subjects;
    size_t max_predicates;
    size_t max_objects;
    size_t stride_len;            // (max_subjects + 63) / 64
} S7TEngine;

// The only three operations that matter
S7TEngine* s7t_create(size_t max_s, size_t max_p, size_t max_o);
void s7t_add_triple(S7TEngine* e, uint32_t s, uint32_t p, uint32_t o);
int s7t_ask_pattern(S7TEngine* e, uint32_t s, uint32_t p, uint32_t o);

// Batch operations for SIMD
typedef struct {
    uint32_t s, p, o;
} TriplePattern;

void s7t_ask_batch(S7TEngine* e, TriplePattern* patterns, int* results, size_t count);

#endif
