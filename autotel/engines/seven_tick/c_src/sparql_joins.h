#ifndef SPARQL_JOINS_H
#define SPARQL_JOINS_H

#include "sparql7t.h"
#include <stdint.h>

// Join types for SPARQL patterns
typedef enum {
    JOIN_INNER,      // Default SPARQL join
    JOIN_LEFT,       // OPTIONAL pattern
    JOIN_FILTER,     // FILTER clause
    JOIN_UNION       // UNION operator
} JoinType;

// Variable binding in join
typedef struct {
    uint32_t var_id;     // Variable identifier
    uint32_t position;   // Position in triple (0=s, 1=p, 2=o)
} VarBinding;

// Join pattern representation
typedef struct {
    TriplePattern pattern;
    VarBinding bindings[3];  // Which positions are variables
    uint8_t num_vars;
} JoinPattern;

// Join operations with bit-vectors
typedef struct {
    uint64_t* result_vector;   // Bit-vector of matching subjects
    size_t cardinality;        // Number of set bits
} JoinResult;

// Core join operations
JoinResult* s7t_get_subject_vector(S7TEngine* e, uint32_t p, uint32_t o);
JoinResult* s7t_get_object_vector(S7TEngine* e, uint32_t s, uint32_t p);
JoinResult* s7t_get_predicate_vector(S7TEngine* e, uint32_t s, uint32_t o);

// High-performance join execution
size_t s7t_join_patterns(S7TEngine* e, JoinPattern* patterns, size_t count,
                        uint32_t* output_subjects, size_t max_output);

// Optimized join with query planner integration
size_t s7t_join_optimized(S7TEngine* e, JoinPattern* patterns, size_t count,
                         uint32_t* pattern_order, uint32_t* output_subjects);

// SPARQL operators
JoinResult* s7t_union(JoinResult* a, JoinResult* b);
JoinResult* s7t_intersect(JoinResult* a, JoinResult* b);
JoinResult* s7t_difference(JoinResult* a, JoinResult* b);

// Memory management
void s7t_free_result(JoinResult* result);

#endif