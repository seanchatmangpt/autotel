#include "sparql_joins.h"
#include <stdlib.h>
#include <string.h>

// Get subject bit-vector for all subjects with predicate p and object o
JoinResult* s7t_get_subject_vector(S7TEngine* e, uint32_t p, uint32_t o) {
    JoinResult* result = calloc(1, sizeof(JoinResult));
    result->result_vector = aligned_alloc(64, e->stride_len * sizeof(uint64_t));
    
    uint64_t* p_vec = e->predicate_vectors + (p * e->stride_len);
    uint64_t* o_vec = e->object_vectors + (o * e->stride_len);
    
    // Vectorized AND operation
    size_t cardinality = 0;
    for (size_t i = 0; i < e->stride_len; i++) {
        result->result_vector[i] = p_vec[i] & o_vec[i];
        cardinality += __builtin_popcountll(result->result_vector[i]);
    }
    
    result->cardinality = cardinality;
    return result;
}

// Get object bit-vector for subject s with predicate p
JoinResult* s7t_get_object_vector(S7TEngine* e, uint32_t s, uint32_t p) {
    JoinResult* result = calloc(1, sizeof(JoinResult));
    result->result_vector = aligned_alloc(64, (e->max_objects + 63) / 64 * sizeof(uint64_t));
    memset(result->result_vector, 0, (e->max_objects + 63) / 64 * sizeof(uint64_t));
    
    // Check if subject has predicate
    size_t chunk = s / 64;
    uint64_t bit = 1ULL << (s % 64);
    
    if (e->predicate_vectors[p * e->stride_len + chunk] & bit) {
        uint32_t o = e->ps_to_o_index[p * e->max_subjects + s];
        if (o < e->max_objects) {
            size_t o_chunk = o / 64;
            uint64_t o_bit = 1ULL << (o % 64);
            result->result_vector[o_chunk] |= o_bit;
            result->cardinality = 1;
        }
    }
    
    return result;
}
// Perform multi-way join with optimal ordering
size_t s7t_join_patterns(S7TEngine* e, JoinPattern* patterns, size_t count,
                        uint32_t* output_subjects, size_t max_output) {
    if (count == 0) return 0;
    
    // Start with first pattern
    JoinResult* current = NULL;
    
    // Process each pattern
    for (size_t i = 0; i < count; i++) {
        JoinPattern* pat = &patterns[i];
        JoinResult* pattern_result = NULL;
        
        // Determine pattern type and get appropriate vector
        if (pat->bindings[0].var_id != 0) {
            // (?s, p, o) pattern
            pattern_result = s7t_get_subject_vector(e, pat->pattern.p, pat->pattern.o);
        } else if (pat->bindings[2].var_id != 0) {
            // (s, p, ?o) pattern
            pattern_result = s7t_get_object_vector(e, pat->pattern.s, pat->pattern.p);
        }
        
        if (i == 0) {
            current = pattern_result;
        } else {
            // Intersect with previous results
            JoinResult* intersected = s7t_intersect(current, pattern_result);
            s7t_free_result(current);
            s7t_free_result(pattern_result);
            current = intersected;
        }
        
        // Early termination if no results
        if (current->cardinality == 0) break;
    }
    
    // Decode bit-vector to subject IDs
    size_t match_count = 0;
    if (current) {
        for (size_t i = 0; i < e->stride_len && match_count < max_output; i++) {
            uint64_t chunk = current->result_vector[i];
            while (chunk != 0 && match_count < max_output) {
                uint32_t bit_idx = __builtin_ctzll(chunk);
                output_subjects[match_count++] = (i * 64) + bit_idx;
                chunk &= chunk - 1;  // Clear lowest set bit
            }
        }
        s7t_free_result(current);
    }
    
    return match_count;
}
// Set operations for SPARQL algebra
JoinResult* s7t_union(JoinResult* a, JoinResult* b) {
    JoinResult* result = calloc(1, sizeof(JoinResult));
    size_t vec_size = a->cardinality > 0 ? 
                     (a->cardinality + 63) / 64 * sizeof(uint64_t) : 
                     sizeof(uint64_t);
    result->result_vector = aligned_alloc(64, vec_size);
    
    size_t cardinality = 0;
    size_t max_chunks = vec_size / sizeof(uint64_t);
    
    for (size_t i = 0; i < max_chunks; i++) {
        result->result_vector[i] = a->result_vector[i] | b->result_vector[i];
        cardinality += __builtin_popcountll(result->result_vector[i]);
    }
    
    result->cardinality = cardinality;
    return result;
}

JoinResult* s7t_intersect(JoinResult* a, JoinResult* b) {
    JoinResult* result = calloc(1, sizeof(JoinResult));
    size_t vec_size = a->cardinality > 0 ? 
                     (a->cardinality + 63) / 64 * sizeof(uint64_t) : 
                     sizeof(uint64_t);
    result->result_vector = aligned_alloc(64, vec_size);
    
    size_t cardinality = 0;
    size_t max_chunks = vec_size / sizeof(uint64_t);
    
    for (size_t i = 0; i < max_chunks; i++) {
        result->result_vector[i] = a->result_vector[i] & b->result_vector[i];
        cardinality += __builtin_popcountll(result->result_vector[i]);
    }
    
    result->cardinality = cardinality;
    return result;
}

JoinResult* s7t_difference(JoinResult* a, JoinResult* b) {
    JoinResult* result = calloc(1, sizeof(JoinResult));
    size_t vec_size = a->cardinality > 0 ? 
                     (a->cardinality + 63) / 64 * sizeof(uint64_t) : 
                     sizeof(uint64_t);
    result->result_vector = aligned_alloc(64, vec_size);
    
    size_t cardinality = 0;
    size_t max_chunks = vec_size / sizeof(uint64_t);
    
    for (size_t i = 0; i < max_chunks; i++) {
        result->result_vector[i] = a->result_vector[i] & ~b->result_vector[i];
        cardinality += __builtin_popcountll(result->result_vector[i]);
    }
    
    result->cardinality = cardinality;
    return result;
}

void s7t_free_result(JoinResult* result) {
    if (result) {
        free(result->result_vector);
        free(result);
    }
}