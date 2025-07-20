#include "cns/8t/8t.h"
#include <string.h>
#include <assert.h>

// ============================================================================
// L1-OPTIMIZED GRAPH STRUCTURE IMPLEMENTATION
// ============================================================================

void cns_8t_graph_init(cns_8t_graph_t* graph, cns_8t_arena_t* arena, size_t initial_capacity) {
    assert(graph != NULL);
    assert(arena != NULL);
    assert(initial_capacity > 0);
    
    // Allocate cache-aligned triple storage
    graph->triples = (cns_8t_triple_t*)cns_8t_arena_alloc(
        arena, 
        sizeof(cns_8t_triple_t) * initial_capacity, 
        CNS_8T_ALIGNMENT
    );
    
    assert(graph->triples != NULL);
    assert(((uintptr_t)graph->triples & (CNS_8T_ALIGNMENT - 1)) == 0);
    
    graph->count = 0;
    graph->capacity = initial_capacity;
    
    // Calculate cache lines used
    graph->cache_line_count = (sizeof(cns_8t_triple_t) * initial_capacity + 
                              CNS_8T_CACHE_LINE_SIZE - 1) / CNS_8T_CACHE_LINE_SIZE;
    
    graph->l1_utilization = 0;
    graph->access_pattern_hash = 0;
}

void cns_8t_graph_add_triple(cns_8t_graph_t* graph, cns_8t_triple_t triple) {
    assert(graph != NULL);
    assert(graph->count < graph->capacity);
    
    // Prefetch the target cache line before writing
    __builtin_prefetch(&graph->triples[graph->count], 1, 3); // Write, high locality
    
    // Platform-optimized copy
#ifdef CNS_8T_X86_64
    // SIMD-optimized copy using 128-bit operations
    // Each triple is 16 bytes (4 × uint32_t), perfect for SSE
    __m128i triple_data = _mm_set_epi32(triple.metadata, triple.object, 
                                       triple.predicate, triple.subject);
    _mm_store_si128((__m128i*)&graph->triples[graph->count], triple_data);
#elif defined(CNS_8T_ARM64)
    // ARM NEON optimized copy
    uint32x4_t triple_data = {triple.subject, triple.predicate, triple.object, triple.metadata};
    vst1q_u32((uint32_t*)&graph->triples[graph->count], triple_data);
#else
    // Scalar fallback
    graph->triples[graph->count] = triple;
#endif
    
    graph->count++;
    
    // Update access pattern hash for cache analysis
    graph->access_pattern_hash = graph->access_pattern_hash * 31 + 
                                (triple.subject ^ triple.predicate ^ triple.object);
    
    // Update L1 utilization estimate
    size_t used_bytes = graph->count * sizeof(cns_8t_triple_t);
    graph->l1_utilization = (uint32_t)((used_bytes * 100) / CNS_8T_L1_CACHE_SIZE);
}

void cns_8t_graph_prefetch_region(cns_8t_graph_t* graph, size_t start_index, size_t count) {
    assert(graph != NULL);
    assert(start_index + count <= graph->count);
    
    // Prefetch cache lines covering the specified region
    char* start_addr = (char*)&graph->triples[start_index];
    char* end_addr = (char*)&graph->triples[start_index + count - 1] + sizeof(cns_8t_triple_t);
    
    for (char* addr = start_addr; addr < end_addr; addr += CNS_8T_CACHE_LINE_SIZE) {
        __builtin_prefetch(addr, 0, 3); // Read, high locality
    }
}

// ============================================================================
// SIMD-OPTIMIZED GRAPH OPERATIONS
// ============================================================================

void cns_8t_graph_find_pattern_simd(cns_8t_graph_t* graph, cns_8t_triple_t pattern, 
                                   uint32_t* results, size_t* result_count) {
    assert(graph != NULL);
    assert(results != NULL);
    assert(result_count != NULL);
    
    *result_count = 0;
    
    // Create SIMD pattern vectors for comparison
    __m128i pattern_s = _mm_set1_epi32(pattern.subject);
    __m128i pattern_p = _mm_set1_epi32(pattern.predicate);
    __m128i pattern_o = _mm_set1_epi32(pattern.object);
    
    // Use wildcards (0xFFFFFFFF) to match any value
    __m128i wildcard = _mm_set1_epi32(0xFFFFFFFF);
    
    // Process 4 triples at a time
    for (size_t i = 0; i < graph->count; i += 4) {
        size_t batch_size = (graph->count - i >= 4) ? 4 : graph->count - i;
        
        // Prefetch next batch
        if (i + 8 < graph->count) {
            __builtin_prefetch(&graph->triples[i + 8], 0, 3);
        }
        
        for (size_t j = 0; j < batch_size; j++) {
            cns_8t_triple_t* triple = &graph->triples[i + j];
            
            // Scalar pattern matching (works on all platforms)
            bool match_s = (pattern.subject == 0xFFFFFFFF) || (pattern.subject == triple->subject);
            bool match_p = (pattern.predicate == 0xFFFFFFFF) || (pattern.predicate == triple->predicate);
            bool match_o = (pattern.object == 0xFFFFFFFF) || (pattern.object == triple->object);
            
            if (match_s && match_p && match_o) {
                results[*result_count] = (uint32_t)(i + j);
                (*result_count)++;
            }
        }
    }
}

// ============================================================================
// CACHE-OPTIMIZED GRAPH TRAVERSAL
// ============================================================================

void cns_8t_graph_traverse_cache_optimized(cns_8t_graph_t* graph, 
                                          uint32_t start_subject,
                                          void (*visit_fn)(cns_8t_triple_t*, void*),
                                          void* user_data) {
    assert(graph != NULL);
    assert(visit_fn != NULL);
    
    // Build a simple index for cache-friendly traversal
    // This is a simplified version - production code would use more sophisticated indexing
    
    // Group triples by subject for better cache locality
    for (size_t i = 0; i < graph->count; i++) {
        if (graph->triples[i].subject == start_subject) {
            // Prefetch next few triples
            if (i + 1 < graph->count) {
                __builtin_prefetch(&graph->triples[i + 1], 0, 3);
            }
            
            visit_fn(&graph->triples[i], user_data);
        }
    }
}

// ============================================================================
// GRAPH ANALYSIS AND OPTIMIZATION
// ============================================================================

typedef struct {
    uint32_t hot_subjects[64];    // Most frequently accessed subjects
    uint32_t hot_predicates[64];  // Most frequently accessed predicates
    uint32_t access_counts[64];   // Access counts for hot data
    size_t hot_count;
} cns_8t_graph_hotness_info_t;

void cns_8t_graph_analyze_hotness(cns_8t_graph_t* graph, 
                                 cns_8t_graph_hotness_info_t* hotness) {
    assert(graph != NULL);
    assert(hotness != NULL);
    
    memset(hotness, 0, sizeof(*hotness));
    
    // Simple frequency analysis
    // In production, this would use more sophisticated algorithms
    
    for (size_t i = 0; i < graph->count && hotness->hot_count < 64; i++) {
        cns_8t_triple_t* triple = &graph->triples[i];
        
        // Look for existing entries
        bool found = false;
        for (size_t j = 0; j < hotness->hot_count; j++) {
            if (hotness->hot_subjects[j] == triple->subject) {
                hotness->access_counts[j]++;
                found = true;
                break;
            }
        }
        
        if (!found && hotness->hot_count < 64) {
            hotness->hot_subjects[hotness->hot_count] = triple->subject;
            hotness->hot_predicates[hotness->hot_count] = triple->predicate;
            hotness->access_counts[hotness->hot_count] = 1;
            hotness->hot_count++;
        }
    }
}

// Calculate cache efficiency for the graph
double cns_8t_graph_cache_efficiency(const cns_8t_graph_t* graph) {
    assert(graph != NULL);
    
    if (graph->count == 0) {
        return 1.0;
    }
    
    // Estimate based on L1 utilization and access patterns
    double utilization_factor = fmin(1.0, (double)graph->l1_utilization / 100.0);
    double pattern_factor = 1.0; // Simplified - would analyze access patterns
    
    return utilization_factor * pattern_factor;
}

// Optimize graph layout for cache efficiency
void cns_8t_graph_optimize_layout(cns_8t_graph_t* graph, cns_8t_arena_t* temp_arena) {
    assert(graph != NULL);
    assert(temp_arena != NULL);
    
    if (graph->count < 2) {
        return; // Nothing to optimize
    }
    
    // Create temporary storage for reorganized triples
    cns_8t_triple_t* temp_triples = (cns_8t_triple_t*)cns_8t_arena_alloc(
        temp_arena,
        sizeof(cns_8t_triple_t) * graph->count,
        CNS_8T_ALIGNMENT
    );
    
    assert(temp_triples != NULL);
    
    // Simple subject-based sorting for better cache locality
    // Copy triples grouped by subject
    size_t write_pos = 0;
    
    for (size_t pass = 0; pass < graph->count && write_pos < graph->count; pass++) {
        uint32_t current_subject = graph->triples[pass].subject;
        
        // Copy all triples with this subject
        for (size_t i = 0; i < graph->count; i++) {
            if (graph->triples[i].subject == current_subject) {
                // Check if already copied
                bool already_copied = false;
                for (size_t j = 0; j < write_pos; j++) {
                    if (temp_triples[j].subject == current_subject &&
                        temp_triples[j].predicate == graph->triples[i].predicate &&
                        temp_triples[j].object == graph->triples[i].object) {
                        already_copied = true;
                        break;
                    }
                }
                
                if (!already_copied && write_pos < graph->count) {
                    temp_triples[write_pos++] = graph->triples[i];
                }
            }
        }
    }
    
    // Copy back optimized layout
    memcpy(graph->triples, temp_triples, sizeof(cns_8t_triple_t) * graph->count);
}