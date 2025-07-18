#include "seven_t_runtime.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define INITIAL_CAPACITY 1024
#define BITVEC_WORD_BITS 64

// Track allocated sizes
typedef struct {
    size_t predicate_vectors_size;
    size_t object_vectors_size;
    size_t node_counts_size;
    size_t ps_index_size;
} AllocSizes;

// Helper to ensure array capacity
static void ensure_capacity(void** array, size_t* current_size, size_t required_size, size_t elem_size) {
    if (required_size > *current_size) {
        size_t new_size = required_size * 2; // Double for amortized growth
        void* new_array = realloc(*array, new_size * elem_size);
        if (!new_array) abort(); // Out of memory
        
        // Zero new memory
        memset((char*)new_array + (*current_size * elem_size), 0, 
               (new_size - *current_size) * elem_size);
        
        *array = new_array;
        *current_size = new_size;
    }
}

// Bit vector implementation
BitVector* bitvec_create(size_t capacity) {
    BitVector* bv = malloc(sizeof(BitVector));
    bv->capacity = (capacity + BITVEC_WORD_BITS - 1) / BITVEC_WORD_BITS;
    bv->bits = calloc(bv->capacity, sizeof(uint64_t));
    bv->count = 0;
    return bv;
}

void bitvec_destroy(BitVector* bv) {
    free(bv->bits);
    free(bv);
}

S7T_HOT void bitvec_set(BitVector* bv, size_t index) {
    size_t word = index / BITVEC_WORD_BITS;
    size_t bit = index % BITVEC_WORD_BITS;
    
    if (S7T_UNLIKELY(word >= bv->capacity)) {
        size_t new_capacity = word * 2 + 1;
        bv->bits = realloc(bv->bits, new_capacity * sizeof(uint64_t));
        memset(&bv->bits[bv->capacity], 0, (new_capacity - bv->capacity) * sizeof(uint64_t));
        bv->capacity = new_capacity;
    }
    
    uint64_t mask = 1ULL << bit;
    if (!(bv->bits[word] & mask)) {
        bv->bits[word] |= mask;
        bv->count++;
    }
}

S7T_HOT S7T_PURE int bitvec_test(BitVector* bv, size_t index) {
    size_t word = index / BITVEC_WORD_BITS;
    size_t bit = index % BITVEC_WORD_BITS;
    
    if (S7T_UNLIKELY(word >= bv->capacity)) return 0;
    return (bv->bits[word] & (1ULL << bit)) != 0;
}

S7T_HOT BitVector* bitvec_and(BitVector* S7T_RESTRICT a, BitVector* S7T_RESTRICT b) {
    size_t min_capacity = a->capacity < b->capacity ? a->capacity : b->capacity;
    BitVector* result = bitvec_create(min_capacity * BITVEC_WORD_BITS);
    
    result->count = 0;
    for (size_t i = 0; i < min_capacity; i++) {
        result->bits[i] = a->bits[i] & b->bits[i];
        result->count += __builtin_popcountll(result->bits[i]);
    }
    
    return result;
}

BitVector* bitvec_or(BitVector* S7T_RESTRICT a, BitVector* S7T_RESTRICT b) {
    size_t max_capacity = a->capacity > b->capacity ? a->capacity : b->capacity;
    BitVector* result = bitvec_create(max_capacity * BITVEC_WORD_BITS);
    
    result->count = 0;
    for (size_t i = 0; i < a->capacity && i < max_capacity; i++) {
        result->bits[i] |= a->bits[i];
    }
    for (size_t i = 0; i < b->capacity && i < max_capacity; i++) {
        result->bits[i] |= b->bits[i];
    }
    
    for (size_t i = 0; i < max_capacity; i++) {
        result->count += __builtin_popcountll(result->bits[i]);
    }
    
    return result;
}

S7T_PURE size_t bitvec_popcount(BitVector* bv) {
    return bv->count;
}

// Engine creation and management
EngineState* s7t_create_engine(void) {
    EngineState* engine = calloc(1, sizeof(EngineState));
    
    // Allocate extension tracking
    AllocSizes* sizes = calloc(1, sizeof(AllocSizes));
    sizes->predicate_vectors_size = INITIAL_CAPACITY;
    sizes->object_vectors_size = INITIAL_CAPACITY;
    sizes->node_counts_size = INITIAL_CAPACITY;
    sizes->ps_index_size = INITIAL_CAPACITY;
    
    // Store sizes in first slot of string table for now (hack for MVP)
    engine->string_table = calloc(INITIAL_CAPACITY, sizeof(char*));
    engine->string_table[0] = (char*)sizes;
    engine->string_capacity = INITIAL_CAPACITY;
    engine->string_count = 1; // Start at 1 to skip sizes
    
    // Pre-allocate arrays
    engine->predicate_vectors = calloc(INITIAL_CAPACITY, sizeof(BitVector*));
    engine->object_vectors = calloc(INITIAL_CAPACITY, sizeof(BitVector*));
    engine->ps_to_o_index = calloc(INITIAL_CAPACITY, sizeof(uint32_t**));
    engine->ps_to_o_counts = calloc(INITIAL_CAPACITY, sizeof(size_t*));
    engine->node_property_counts = calloc(INITIAL_CAPACITY, sizeof(uint32_t));
    engine->object_type_ids = calloc(INITIAL_CAPACITY, sizeof(uint32_t));
    
    return engine;
}

void s7t_destroy_engine(EngineState* engine) {
    // Get sizes
    AllocSizes* sizes = (AllocSizes*)engine->string_table[0];
    
    // Free string table (skip first entry)
    for (size_t i = 1; i < engine->string_count; i++) {
        free(engine->string_table[i]);
    }
    free(sizes);
    free(engine->string_table);
    
    // Free bit vectors
    for (size_t i = 0; i <= engine->max_predicate_id && i < sizes->predicate_vectors_size; i++) {
        if (engine->predicate_vectors[i]) {
            bitvec_destroy(engine->predicate_vectors[i]);
        }
    }
    free(engine->predicate_vectors);
    
    for (size_t i = 0; i <= engine->max_object_id && i < sizes->object_vectors_size; i++) {
        if (engine->object_vectors[i]) {
            bitvec_destroy(engine->object_vectors[i]);
        }
    }
    free(engine->object_vectors);
    
    // Free PS->O index
    for (size_t p = 0; p <= engine->max_predicate_id && p < sizes->ps_index_size; p++) {
        if (engine->ps_to_o_index[p]) {
            for (size_t s = 0; s <= engine->max_subject_id; s++) {
                free(engine->ps_to_o_index[p][s]);
            }
            free(engine->ps_to_o_index[p]);
            free(engine->ps_to_o_counts[p]);
        }
    }
    free(engine->ps_to_o_index);
    free(engine->ps_to_o_counts);
    
    free(engine->node_property_counts);
    free(engine->object_type_ids);
    free(engine);
}

// String interning
S7T_HOT uint32_t s7t_intern_string(EngineState* engine, const char* str) {
    // Linear search starting from 1 (0 is reserved for sizes)
    for (size_t i = 1; i < engine->string_count; i++) {
        if (strcmp(engine->string_table[i], str) == 0) {
            return i;
        }
    }
    
    // Add new string
    if (S7T_UNLIKELY(engine->string_count >= engine->string_capacity)) {
        engine->string_capacity *= 2;
        engine->string_table = realloc(engine->string_table, 
                                     engine->string_capacity * sizeof(char*));
    }
    
    engine->string_table[engine->string_count] = strdup(str);
    return engine->string_count++;
}

// Triple addition - hot path optimized
S7T_HOT void s7t_add_triple(EngineState* engine, uint32_t s, uint32_t p, uint32_t o) {
    AllocSizes* sizes = (AllocSizes*)engine->string_table[0];
    
    // Update max IDs
    if (S7T_UNLIKELY(s > engine->max_subject_id)) engine->max_subject_id = s;
    if (S7T_UNLIKELY(p > engine->max_predicate_id)) engine->max_predicate_id = p;
    if (S7T_UNLIKELY(o > engine->max_object_id)) engine->max_object_id = o;
    
    // Ensure arrays are large enough
    ensure_capacity((void**)&engine->predicate_vectors, &sizes->predicate_vectors_size, 
                   p + 1, sizeof(BitVector*));
    ensure_capacity((void**)&engine->object_vectors, &sizes->object_vectors_size,
                   o + 1, sizeof(BitVector*));
    ensure_capacity((void**)&engine->ps_to_o_index, &sizes->ps_index_size,
                   p + 1, sizeof(uint32_t**));
    ensure_capacity((void**)&engine->ps_to_o_counts, &sizes->ps_index_size,
                   p + 1, sizeof(size_t*));
    ensure_capacity((void**)&engine->node_property_counts, &sizes->node_counts_size,
                   s + 1, sizeof(uint32_t));
    ensure_capacity((void**)&engine->object_type_ids, &sizes->object_vectors_size,
                   o + 1, sizeof(uint32_t));
    
    // Ensure predicate vector exists
    if (S7T_UNLIKELY(!engine->predicate_vectors[p])) {
        engine->predicate_vectors[p] = bitvec_create(INITIAL_CAPACITY);
    }
    bitvec_set(engine->predicate_vectors[p], s);
    
    // Ensure object vector exists
    if (S7T_UNLIKELY(!engine->object_vectors[o])) {
        engine->object_vectors[o] = bitvec_create(INITIAL_CAPACITY);
    }
    bitvec_set(engine->object_vectors[o], s);
    
    // Update PS->O index
    if (S7T_UNLIKELY(!engine->ps_to_o_index[p])) {
        size_t initial_size = engine->max_subject_id + 100;
        engine->ps_to_o_index[p] = calloc(initial_size, sizeof(uint32_t*));
        engine->ps_to_o_counts[p] = calloc(initial_size, sizeof(size_t));
    }
    
    // Check if subject array needs expansion
    if (S7T_UNLIKELY(s >= engine->max_subject_id + 100)) {
        size_t new_size = s + 100;
        engine->ps_to_o_index[p] = realloc(engine->ps_to_o_index[p], new_size * sizeof(uint32_t*));
        engine->ps_to_o_counts[p] = realloc(engine->ps_to_o_counts[p], new_size * sizeof(size_t));
        
        size_t old_size = engine->max_subject_id + 100;
        memset(&engine->ps_to_o_index[p][old_size], 0, (new_size - old_size) * sizeof(uint32_t*));
        memset(&engine->ps_to_o_counts[p][old_size], 0, (new_size - old_size) * sizeof(size_t));
    }
    
    // Add object to PS->O index
    size_t current_count = engine->ps_to_o_counts[p][s];
    if (current_count == 0) {
        engine->ps_to_o_index[p][s] = malloc(sizeof(uint32_t));
    } else {
        engine->ps_to_o_index[p][s] = realloc(engine->ps_to_o_index[p][s], 
                                              (current_count + 1) * sizeof(uint32_t));
    }
    
    engine->ps_to_o_index[p][s][current_count] = o;
    engine->ps_to_o_counts[p][s]++;
    
    // Update node property count
    engine->node_property_counts[s]++;
    
    engine->triple_count++;
}
// Query primitives - optimized for L1 cache
S7T_HOT BitVector* s7t_get_subject_vector(EngineState* engine, uint32_t predicate_id, uint32_t object_id) {
    AllocSizes* sizes = (AllocSizes*)engine->string_table[0];
    
    if (S7T_UNLIKELY(predicate_id >= sizes->predicate_vectors_size || 
                     object_id >= sizes->object_vectors_size)) {
        return bitvec_create(0);
    }
    
    BitVector* pred_vec = engine->predicate_vectors[predicate_id];
    BitVector* obj_vec = engine->object_vectors[object_id];
    
    if (S7T_UNLIKELY(!pred_vec || !obj_vec)) {
        return bitvec_create(0);
    }
    
    return bitvec_and(pred_vec, obj_vec);
}

S7T_HOT BitVector* s7t_get_object_vector(EngineState* engine, uint32_t predicate_id, uint32_t subject_id) {
    AllocSizes* sizes = (AllocSizes*)engine->string_table[0];
    
    if (S7T_UNLIKELY(predicate_id >= sizes->ps_index_size || 
                     subject_id > engine->max_subject_id)) {
        return bitvec_create(0);
    }
    
    if (S7T_UNLIKELY(!engine->ps_to_o_index[predicate_id])) {
        return bitvec_create(0);
    }
    
    BitVector* result = bitvec_create(engine->max_object_id + 1);
    
    size_t count = engine->ps_to_o_counts[predicate_id][subject_id];
    uint32_t* objects = engine->ps_to_o_index[predicate_id][subject_id];
    
    for (size_t i = 0; i < count; i++) {
        bitvec_set(result, objects[i]);
    }
    
    return result;
}

S7T_HOT S7T_PURE uint32_t* s7t_get_objects(EngineState* engine, uint32_t predicate_id, 
                                           uint32_t subject_id, size_t* count) {
    AllocSizes* sizes = (AllocSizes*)engine->string_table[0];
    
    if (S7T_UNLIKELY(predicate_id >= sizes->ps_index_size || 
                     subject_id > engine->max_subject_id)) {
        *count = 0;
        return NULL;
    }
    
    if (S7T_UNLIKELY(!engine->ps_to_o_index[predicate_id])) {
        *count = 0;
        return NULL;
    }
    
    *count = engine->ps_to_o_counts[predicate_id][subject_id];
    return engine->ps_to_o_index[predicate_id][subject_id];
}

// SHACL validation primitives
S7T_HOT S7T_PURE int shacl_check_min_count(EngineState* engine, uint32_t subject_id, 
                                           uint32_t predicate_id, uint32_t min_count) {
    AllocSizes* sizes = (AllocSizes*)engine->string_table[0];
    
    if (S7T_UNLIKELY(predicate_id >= sizes->ps_index_size || 
                     subject_id > engine->max_subject_id)) {
        return min_count == 0;
    }
    
    if (S7T_UNLIKELY(!engine->ps_to_o_counts[predicate_id])) {
        return min_count == 0;
    }
    
    return engine->ps_to_o_counts[predicate_id][subject_id] >= min_count;
}

S7T_HOT S7T_PURE int shacl_check_max_count(EngineState* engine, uint32_t subject_id, 
                                           uint32_t predicate_id, uint32_t max_count) {
    AllocSizes* sizes = (AllocSizes*)engine->string_table[0];
    
    if (S7T_UNLIKELY(predicate_id >= sizes->ps_index_size || 
                     subject_id > engine->max_subject_id)) {
        return 1;
    }
    
    if (S7T_UNLIKELY(!engine->ps_to_o_counts[predicate_id])) {
        return 1;
    }
    
    return engine->ps_to_o_counts[predicate_id][subject_id] <= max_count;
}

S7T_HOT S7T_PURE int shacl_check_class(EngineState* engine, uint32_t subject_id, uint32_t class_id) {
    AllocSizes* sizes = (AllocSizes*)engine->string_table[0];
    
    if (S7T_UNLIKELY(subject_id >= sizes->node_counts_size)) {
        return 0;
    }
    
    return engine->object_type_ids[subject_id] == class_id;
}
