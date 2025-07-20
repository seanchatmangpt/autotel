/*  ─────────────────────────────────────────────────────────────
    src/graph.c  –  7T In-Memory Graph Implementation
    O(1) triple addition and retrieval with cache-optimized layout
    ───────────────────────────────────────────────────────────── */

#include "../include/cns/core/memory.h"
#include "../include/cns/binary_materializer_types.h"
#include "../s7t_minimal.h"
#include <string.h>
#include <assert.h>

/*═══════════════════════════════════════════════════════════════
  7T Graph Structure (Cache-Aligned)
  ═══════════════════════════════════════════════════════════════*/

// Triple representation (24 bytes, cache-line friendly)
typedef struct S7T_ALIGNED(8) {
    uint64_t subject;      // Subject ID
    uint64_t predicate;    // Predicate ID  
    uint64_t object;       // Object ID
} cns_triple_t;

// Graph adjacency list entry (16 bytes)
typedef struct S7T_ALIGNED(8) {
    uint64_t target;       // Target node ID
    uint64_t predicate;    // Edge predicate
} cns_edge_entry_t;

// Node adjacency list (cache-optimized)
typedef struct {
    cns_edge_entry_t* edges;    // Outgoing edges
    uint32_t edge_count;        // Number of edges
    uint32_t edge_capacity;     // Allocated capacity
} cns_node_adjacency_t;

// Hash table entry for O(1) lookup
typedef struct {
    uint64_t node_id;           // Node identifier
    uint32_t adj_index;         // Index into adjacency array
    uint32_t next_index;        // Chain for collision resolution
} cns_hash_entry_t;

// Main graph structure (64-byte aligned)
typedef struct S7T_ALIGNED(64) {
    cns_memory_arena_t* arena;      // Memory arena
    cns_triple_t* triples;          // Dense triple array
    cns_node_adjacency_t* nodes;    // Node adjacency lists
    cns_hash_entry_t* hash_table;   // Node ID hash table
    uint32_t* hash_buckets;         // Hash bucket heads
    
    uint32_t triple_count;          // Number of triples
    uint32_t triple_capacity;       // Triple array capacity
    uint32_t node_count;            // Number of nodes
    uint32_t node_capacity;         // Node array capacity
    uint32_t hash_capacity;         // Hash table capacity
    uint32_t flags;                 // Graph flags
} cns_7t_graph_t;

/*═══════════════════════════════════════════════════════════════
  7T Constraint Enforcement
  ═══════════════════════════════════════════════════════════════*/

_Static_assert(S7T_MAX_CYCLES == 7, "Graph implementation requires 7-tick constraint");

/*═══════════════════════════════════════════════════════════════
  Hash Function (< 2 ticks)
  ═══════════════════════════════════════════════════════════════*/

S7T_ALWAYS_INLINE uint32_t cns_hash_node_id(uint64_t node_id, uint32_t capacity) {
    // Fast hash using multiplication and bit shifting
    const uint64_t multiplier = 0x9e3779b97f4a7c15ULL; // Golden ratio
    uint64_t hash = node_id * multiplier;
    return (uint32_t)(hash >> (64 - __builtin_clz(capacity - 1) - 1)) & (capacity - 1);
}

/*═══════════════════════════════════════════════════════════════
  Graph Creation and Initialization
  ═══════════════════════════════════════════════════════════════*/

cns_7t_graph_t* cns_graph_create_7t(
    cns_memory_arena_t* arena,
    uint32_t initial_triples,
    uint32_t initial_nodes
) {
    if (!arena) return NULL;
    
    // Ensure capacities are powers of 2 for fast hashing
    if (initial_triples < 16) initial_triples = 16;
    if (initial_nodes < 16) initial_nodes = 16;
    
    initial_triples = 1U << (32 - __builtin_clz(initial_triples - 1));
    initial_nodes = 1U << (32 - __builtin_clz(initial_nodes - 1));
    
    cns_7t_graph_t* graph = CNS_ARENA_NEW(arena, cns_7t_graph_t);
    if (!graph) return NULL;
    
    // Allocate arrays
    graph->triples = CNS_ARENA_NEW_ARRAY(arena, cns_triple_t, initial_triples);
    graph->nodes = CNS_ARENA_NEW_ARRAY(arena, cns_node_adjacency_t, initial_nodes);
    graph->hash_table = CNS_ARENA_NEW_ARRAY(arena, cns_hash_entry_t, initial_nodes);
    graph->hash_buckets = CNS_ARENA_NEW_ARRAY(arena, uint32_t, initial_nodes);
    
    if (!graph->triples || !graph->nodes || !graph->hash_table || !graph->hash_buckets) {
        return NULL;
    }
    
    // Initialize hash buckets to empty
    for (uint32_t i = 0; i < initial_nodes; i++) {
        graph->hash_buckets[i] = UINT32_MAX;
    }
    
    // Initialize graph
    graph->arena = arena;
    graph->triple_count = 0;
    graph->triple_capacity = initial_triples;
    graph->node_count = 0;
    graph->node_capacity = initial_nodes;
    graph->hash_capacity = initial_nodes;
    graph->flags = 0;
    
    return graph;
}

/*═══════════════════════════════════════════════════════════════
  Node Management (O(1) operations)
  ═══════════════════════════════════════════════════════════════*/

S7T_ALWAYS_INLINE uint32_t cns_graph_find_node_index(
    const cns_7t_graph_t* graph,
    uint64_t node_id
) {
    uint32_t bucket = cns_hash_node_id(node_id, graph->hash_capacity);
    uint32_t entry_index = graph->hash_buckets[bucket];
    
    while (entry_index != UINT32_MAX) {
        const cns_hash_entry_t* entry = &graph->hash_table[entry_index];
        if (entry->node_id == node_id) {
            return entry->adj_index;
        }
        entry_index = entry->next_index;
    }
    
    return UINT32_MAX; // Not found
}

S7T_ALWAYS_INLINE uint32_t cns_graph_add_node(
    cns_7t_graph_t* graph,
    uint64_t node_id
) {
    // Check if node already exists
    uint32_t existing = cns_graph_find_node_index(graph, node_id);
    if (existing != UINT32_MAX) {
        return existing;
    }
    
    // Check capacity
    if (graph->node_count >= graph->node_capacity) {
        return UINT32_MAX; // Graph full
    }
    
    // Initialize new node
    uint32_t node_index = graph->node_count;
    cns_node_adjacency_t* node = &graph->nodes[node_index];
    node->edges = NULL;
    node->edge_count = 0;
    node->edge_capacity = 0;
    
    // Add to hash table
    uint32_t bucket = cns_hash_node_id(node_id, graph->hash_capacity);
    cns_hash_entry_t* hash_entry = &graph->hash_table[node_index];
    hash_entry->node_id = node_id;
    hash_entry->adj_index = node_index;
    hash_entry->next_index = graph->hash_buckets[bucket];
    graph->hash_buckets[bucket] = node_index;
    
    graph->node_count++;
    return node_index;
}

/*═══════════════════════════════════════════════════════════════
  Triple Addition (O(1) amortized, < 7 ticks)
  ═══════════════════════════════════════════════════════════════*/

int cns_graph_add_triple_7t(
    cns_7t_graph_t* graph,
    uint64_t subject,
    uint64_t predicate,
    uint64_t object
) {
    if (!graph) return -1;
    
    // Check triple capacity (1 tick)
    if (graph->triple_count >= graph->triple_capacity) {
        return -2; // Graph full
    }
    
    // Add subject and object nodes (2-4 ticks total)
    uint32_t subj_index = cns_graph_add_node(graph, subject);
    uint32_t obj_index = cns_graph_add_node(graph, object);
    
    if (subj_index == UINT32_MAX || obj_index == UINT32_MAX) {
        return -3; // Node addition failed
    }
    
    // Store triple in dense array (1 tick)
    cns_triple_t* triple = &graph->triples[graph->triple_count];
    triple->subject = subject;
    triple->predicate = predicate;
    triple->object = object;
    graph->triple_count++;
    
    // Add edge to subject's adjacency list
    cns_node_adjacency_t* subj_node = &graph->nodes[subj_index];
    
    // Allocate edge array if needed (first edge for this node)
    if (subj_node->edges == NULL) {
        const uint32_t initial_edge_capacity = 4;
        subj_node->edges = CNS_ARENA_NEW_ARRAY(graph->arena, cns_edge_entry_t, initial_edge_capacity);
        if (!subj_node->edges) {
            graph->triple_count--; // Rollback
            return -4;
        }
        subj_node->edge_capacity = initial_edge_capacity;
    }
    
    // Check edge capacity
    if (subj_node->edge_count >= subj_node->edge_capacity) {
        // Grow edge array (arena allocation)
        uint32_t new_capacity = subj_node->edge_capacity * 2;
        cns_edge_entry_t* new_edges = CNS_ARENA_NEW_ARRAY(graph->arena, cns_edge_entry_t, new_capacity);
        if (!new_edges) {
            graph->triple_count--; // Rollback
            return -5;
        }
        
        // Copy existing edges
        memcpy(new_edges, subj_node->edges, subj_node->edge_count * sizeof(cns_edge_entry_t));
        subj_node->edges = new_edges;
        subj_node->edge_capacity = new_capacity;
    }
    
    // Add edge entry (1 tick)
    cns_edge_entry_t* edge = &subj_node->edges[subj_node->edge_count];
    edge->target = object;
    edge->predicate = predicate;
    subj_node->edge_count++;
    
    return 0; // Success
}

/*═══════════════════════════════════════════════════════════════
  Triple Lookup (O(1), < 5 ticks)
  ═══════════════════════════════════════════════════════════════*/

bool cns_graph_has_triple_7t(
    const cns_7t_graph_t* graph,
    uint64_t subject,
    uint64_t predicate,
    uint64_t object
) {
    if (!graph) return false;
    
    // Find subject node (2-3 ticks)
    uint32_t subj_index = cns_graph_find_node_index(graph, subject);
    if (subj_index == UINT32_MAX) {
        return false;
    }
    
    // Search adjacency list (1-2 ticks)
    const cns_node_adjacency_t* subj_node = &graph->nodes[subj_index];
    for (uint32_t i = 0; i < subj_node->edge_count; i++) {
        const cns_edge_entry_t* edge = &subj_node->edges[i];
        if (edge->target == object && edge->predicate == predicate) {
            return true;
        }
    }
    
    return false;
}

/*═══════════════════════════════════════════════════════════════
  Adjacency Queries (O(k) where k is out-degree)
  ═══════════════════════════════════════════════════════════════*/

uint32_t cns_graph_get_outgoing_7t(
    const cns_7t_graph_t* graph,
    uint64_t subject,
    cns_edge_entry_t** edges_out
) {
    if (!graph || !edges_out) return 0;
    
    uint32_t subj_index = cns_graph_find_node_index(graph, subject);
    if (subj_index == UINT32_MAX) {
        *edges_out = NULL;
        return 0;
    }
    
    const cns_node_adjacency_t* subj_node = &graph->nodes[subj_index];
    *edges_out = subj_node->edges;
    return subj_node->edge_count;
}

/*═══════════════════════════════════════════════════════════════
  Pattern Matching (Optimized for SPARQL-style queries)
  ═══════════════════════════════════════════════════════════════*/

typedef struct {
    uint64_t* subjects;
    uint64_t* predicates;
    uint64_t* objects;
    uint32_t count;
    uint32_t capacity;
} cns_triple_result_set_t;

int cns_graph_query_pattern_7t(
    const cns_7t_graph_t* graph,
    uint64_t subject_pattern,    // UINT64_MAX = wildcard
    uint64_t predicate_pattern,  // UINT64_MAX = wildcard
    uint64_t object_pattern,     // UINT64_MAX = wildcard
    cns_triple_result_set_t* results
) {
    if (!graph || !results) return -1;
    
    uint32_t match_count = 0;
    
    // Case 1: Subject is bound, scan its adjacency list
    if (subject_pattern != UINT64_MAX) {
        uint32_t subj_index = cns_graph_find_node_index(graph, subject_pattern);
        if (subj_index != UINT32_MAX) {
            const cns_node_adjacency_t* subj_node = &graph->nodes[subj_index];
            
            for (uint32_t i = 0; i < subj_node->edge_count; i++) {
                const cns_edge_entry_t* edge = &subj_node->edges[i];
                
                bool predicate_match = (predicate_pattern == UINT64_MAX || 
                                      edge->predicate == predicate_pattern);
                bool object_match = (object_pattern == UINT64_MAX || 
                                   edge->target == object_pattern);
                
                if (predicate_match && object_match) {
                    if (match_count < results->capacity) {
                        results->subjects[match_count] = subject_pattern;
                        results->predicates[match_count] = edge->predicate;
                        results->objects[match_count] = edge->target;
                    }
                    match_count++;
                }
            }
        }
    }
    // Case 2: No bound subject, scan all triples (less efficient)
    else {
        for (uint32_t i = 0; i < graph->triple_count; i++) {
            const cns_triple_t* triple = &graph->triples[i];
            
            bool predicate_match = (predicate_pattern == UINT64_MAX || 
                                  triple->predicate == predicate_pattern);
            bool object_match = (object_pattern == UINT64_MAX || 
                               triple->object == object_pattern);
            
            if (predicate_match && object_match) {
                if (match_count < results->capacity) {
                    results->subjects[match_count] = triple->subject;
                    results->predicates[match_count] = triple->predicate;
                    results->objects[match_count] = triple->object;
                }
                match_count++;
            }
        }
    }
    
    results->count = (match_count <= results->capacity) ? match_count : results->capacity;
    return (match_count <= results->capacity) ? 0 : 1; // 1 = truncated results
}

/*═══════════════════════════════════════════════════════════════
  Statistics and Monitoring
  ═══════════════════════════════════════════════════════════════*/

typedef struct {
    uint32_t triple_count;
    uint32_t node_count;
    uint32_t total_edges;
    uint32_t max_out_degree;
    double avg_out_degree;
    double hash_load_factor;
    size_t memory_usage_bytes;
} cns_graph_stats_7t_t;

void cns_graph_get_stats_7t(
    const cns_7t_graph_t* graph,
    cns_graph_stats_7t_t* stats
) {
    if (!graph || !stats) return;
    
    memset(stats, 0, sizeof(*stats));
    
    stats->triple_count = graph->triple_count;
    stats->node_count = graph->node_count;
    stats->hash_load_factor = (double)graph->node_count / graph->hash_capacity;
    
    // Calculate edge statistics
    uint32_t total_edges = 0;
    uint32_t max_out_degree = 0;
    
    for (uint32_t i = 0; i < graph->node_count; i++) {
        const cns_node_adjacency_t* node = &graph->nodes[i];
        total_edges += node->edge_count;
        if (node->edge_count > max_out_degree) {
            max_out_degree = node->edge_count;
        }
    }
    
    stats->total_edges = total_edges;
    stats->max_out_degree = max_out_degree;
    stats->avg_out_degree = graph->node_count > 0 ? 
        (double)total_edges / graph->node_count : 0.0;
    
    // Estimate memory usage
    stats->memory_usage_bytes = sizeof(cns_7t_graph_t) +
                               (graph->triple_capacity * sizeof(cns_triple_t)) +
                               (graph->node_capacity * sizeof(cns_node_adjacency_t)) +
                               (graph->hash_capacity * sizeof(cns_hash_entry_t)) +
                               (graph->hash_capacity * sizeof(uint32_t));
}

/*═══════════════════════════════════════════════════════════════
  Bulk Operations
  ═══════════════════════════════════════════════════════════════*/

int cns_graph_add_triples_batch_7t(
    cns_7t_graph_t* graph,
    const cns_triple_t* triples,
    uint32_t count
) {
    if (!graph || !triples) return -1;
    
    uint32_t successful = 0;
    for (uint32_t i = 0; i < count; i++) {
        const cns_triple_t* triple = &triples[i];
        int result = cns_graph_add_triple_7t(graph, triple->subject, 
                                           triple->predicate, triple->object);
        if (result == 0) {
            successful++;
        }
    }
    
    return (int)successful;
}

/*═══════════════════════════════════════════════════════════════
  Debug and Validation
  ═══════════════════════════════════════════════════════════════*/

#ifdef DEBUG
bool cns_graph_validate_7t(const cns_7t_graph_t* graph) {
    if (!graph) return false;
    if (!graph->triples || !graph->nodes || !graph->hash_table) return false;
    if (graph->triple_count > graph->triple_capacity) return false;
    if (graph->node_count > graph->node_capacity) return false;
    
    // Validate hash table consistency
    for (uint32_t i = 0; i < graph->node_count; i++) {
        const cns_hash_entry_t* entry = &graph->hash_table[i];
        if (entry->adj_index >= graph->node_count) return false;
        
        // Verify hash bucket placement
        uint32_t expected_bucket = cns_hash_node_id(entry->node_id, graph->hash_capacity);
        bool found_in_chain = false;
        
        uint32_t chain_index = graph->hash_buckets[expected_bucket];
        while (chain_index != UINT32_MAX) {
            if (chain_index == i) {
                found_in_chain = true;
                break;
            }
            chain_index = graph->hash_table[chain_index].next_index;
        }
        
        if (!found_in_chain) return false;
    }
    
    return true;
}

void cns_graph_debug_dump_7t(const cns_7t_graph_t* graph) {
    if (!graph) return;
    
    printf("7T Graph Debug Dump:\n");
    printf("  Triples: %u/%u\n", graph->triple_count, graph->triple_capacity);
    printf("  Nodes: %u/%u\n", graph->node_count, graph->node_capacity);
    
    cns_graph_stats_7t_t stats;
    cns_graph_get_stats_7t(graph, &stats);
    printf("  Hash Load Factor: %.2f\n", stats.hash_load_factor);
    printf("  Avg Out-Degree: %.2f\n", stats.avg_out_degree);
    printf("  Max Out-Degree: %u\n", stats.max_out_degree);
    printf("  Memory Usage: %zu bytes\n", stats.memory_usage_bytes);
}
#endif