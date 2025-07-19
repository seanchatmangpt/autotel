#include <stdio.h>
#include <stdlib.h>
#include "../include/s7t.h"

/*
 * Example: High-Performance Triple Store using s7t.h
 * 
 * This demonstrates how to build a 7-tick triple store using
 * the physics-compliant primitives from s7t.h
 */

// Triple structure using interned IDs
typedef struct {
    s7t_id_t subject;
    s7t_id_t predicate;
    s7t_id_t object;
} Triple;

// Index structure using bit vectors
typedef struct {
    s7t_bitvec_t **predicate_index;  // For each predicate, which subjects
    s7t_bitvec_t **object_index;     // For each object, which subjects
    uint32_t max_predicates;
    uint32_t max_objects;
} TripleIndex;

// Main triple store
typedef struct {
    s7t_intern_table_t strings;
    s7t_arena_t arena;
    Triple *triples;
    uint32_t triple_count;
    uint32_t triple_capacity;
    TripleIndex index;
} TripleStore;

// Initialize triple store
TripleStore* create_triple_store(size_t arena_size) {
    // Allocate main structure
    TripleStore *store = malloc(sizeof(TripleStore));
    
    // Initialize string interning
    s7t_intern_init(&store->strings, 4096);
    
    // Initialize memory arena
    void *arena_buffer = aligned_alloc(S7T_CACHE_LINE, arena_size);
    s7t_arena_init(&store->arena, arena_buffer, arena_size);
    
    // Allocate triple storage from arena
    store->triple_capacity = 10000;
    store->triples = s7t_arena_alloc(&store->arena, 
                                     sizeof(Triple) * store->triple_capacity);
    store->triple_count = 0;
    
    // Initialize indexes
    store->index.max_predicates = 1000;
    store->index.max_objects = 10000;
    store->index.predicate_index = s7t_arena_alloc(&store->arena,
                                   sizeof(s7t_bitvec_t*) * store->index.max_predicates);
    store->index.object_index = s7t_arena_alloc(&store->arena,
                                sizeof(s7t_bitvec_t*) * store->index.max_objects);
    
    // Initialize bit vectors
    for (uint32_t i = 0; i < store->index.max_predicates; i++) {
        store->index.predicate_index[i] = s7t_arena_alloc(&store->arena, sizeof(s7t_bitvec_t));
        store->index.predicate_index[i]->words = s7t_arena_alloc(&store->arena, 
                                                 sizeof(s7t_mask_t) * 1000);
        store->index.predicate_index[i]->num_words = 1000;
    }
    
    return store;
}

// Add triple with automatic indexing
void add_triple(TripleStore *store, const char *s, const char *p, const char *o) {
    // Intern strings to get IDs (4 cycles each)
    s7t_id_t s_id = s7t_intern(&store->strings, s);
    s7t_id_t p_id = s7t_intern(&store->strings, p);
    s7t_id_t o_id = s7t_intern(&store->strings, o);
    
    // Store triple
    uint32_t idx = store->triple_count++;
    store->triples[idx].subject = s_id;
    store->triples[idx].predicate = p_id;
    store->triples[idx].object = o_id;
    
    // Update indexes using bit operations (1 cycle)
    if (p_id < store->index.max_predicates) {
        s7t_bitvec_set(store->index.predicate_index[p_id], s_id);
    }
    if (o_id < store->index.max_objects && store->index.object_index[o_id]) {
        s7t_bitvec_set(store->index.object_index[o_id], s_id);
    }
}

// Query using bit-vector operations
uint32_t* query_subjects_by_predicate(TripleStore *store, const char *predicate, 
                                      uint32_t *count) {
    // Intern predicate (4 cycles)
    s7t_id_t p_id = s7t_intern(&store->strings, predicate);
    
    if (p_id >= store->index.max_predicates) {
        *count = 0;
        return NULL;
    }
    
    // Get bit vector for this predicate
    s7t_bitvec_t *subjects = store->index.predicate_index[p_id];
    
    // Count results using popcount (0.5 cycles per word)
    *count = 0;
    for (uint32_t i = 0; i < subjects->num_words; i++) {
        *count += s7t_popcount(subjects->words[i]);
    }
    
    // Allocate result array from arena
    uint32_t *results = s7t_arena_alloc(&store->arena, sizeof(uint32_t) * (*count));
    
    // Extract subject IDs using bit manipulation
    uint32_t result_idx = 0;
    for (uint32_t word_idx = 0; word_idx < subjects->num_words; word_idx++) {
        s7t_mask_t word = subjects->words[word_idx];
        while (word) {
            uint32_t bit = s7t_ctz(word);  // Find next set bit (1 cycle)
            results[result_idx++] = word_idx * 64 + bit;
            word = s7t_clear_lowest(word);  // Clear processed bit (1 cycle)
        }
    }
    
    return results;
}

// Benchmark the implementation
void benchmark_triple_store() {
    printf("\n=== Triple Store Benchmark ===\n");
    
    // Create store with 16MB arena
    TripleStore *store = create_triple_store(16 * 1024 * 1024);
    
    // Measure triple insertion
    s7t_cycle_t cycles;
    char subj[32], obj[32];
    S7T_MEASURE_CYCLES(cycles, 
        for (int i = 0; i < 1000; i++) {
            snprintf(subj, sizeof(subj), "subject_%d", i);
            snprintf(obj, sizeof(obj), "object_%d", i % 100);
            add_triple(store, subj, "hasValue", obj);
        }
    );
    
    printf("Added 1000 triples: %llu cycles (%.2f cycles/triple)\n",
           (unsigned long long)cycles, cycles / 1000.0);
    
    // Measure query performance
    uint32_t count;
    uint32_t *results;
    S7T_MEASURE_CYCLES(cycles, 
        results = query_subjects_by_predicate(store, "hasValue", &count);
        (void)results; // Suppress unused warning
    );
    
    printf("Query found %u subjects: %llu cycles (%.2f cycles/result)\n",
           count, (unsigned long long)cycles, cycles / (double)count);
    
    // Test branch-free filtering
    uint32_t filtered = 0;
    S7T_MEASURE_CYCLES(cycles, 
        for (uint32_t i = 0; i < store->triple_count; i++) {
            // Branch-free counting of triples with object < 50
            bool matches = store->triples[i].object < 50;
            filtered += s7t_select(matches, 1, 0);
        }
    );
    printf("Branch-free filtered: %u triples\n", filtered);
    
    printf("Branch-free filtering: %llu cycles (%.2f cycles/triple)\n",
           (unsigned long long)cycles, cycles / (double)store->triple_count);
}

int main() {
    printf("S7T Example: High-Performance Triple Store\n");
    printf("==========================================\n");
    
    // Show physics constants
    printf("\nPhysics-Compliant Operations:\n");
    printf("- String interning: ~4 cycles\n");
    printf("- Bit operations: 0.5-1 cycles\n");
    printf("- Branch-free select: 1-2 cycles\n");
    printf("- Arena allocation: <1 cycle\n");
    
    // Run benchmark
    benchmark_triple_store();
    
    // Demonstrate SIMD capabilities
    #ifdef __SSE4_2__
    printf("\n=== SIMD Operations Available ===\n");
    printf("SSE4.2 supported - 16-byte parallel operations\n");
    #endif
    
    #ifdef __AVX2__
    printf("AVX2 supported - 32-byte parallel operations\n");
    #endif
    
    return 0;
}