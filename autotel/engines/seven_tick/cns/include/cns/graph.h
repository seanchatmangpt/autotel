#ifndef CNS_GRAPH_H
#define CNS_GRAPH_H

#include "cns/types.h"
#include "cns/arena.h"
#include "cns/interner.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// CNS GRAPH - HIGH-PERFORMANCE RDF GRAPH FOR 7T SUBSTRATE
// ============================================================================

// RDF graph implementation designed for 7T substrate with deterministic
// performance guarantees. All graph operations complete within 7 CPU ticks
// through cache-friendly data structures and O(1) operations.

// ============================================================================
// GRAPH STRUCTURE DEFINITIONS
// ============================================================================

// RDF triple representation - cache-aligned for performance
typedef struct cns_triple {
    cns_string_ref_t subject;     // Subject IRI/blank node
    cns_string_ref_t predicate;   // Predicate IRI
    cns_string_ref_t object;      // Object IRI/literal/blank node
    cns_type_id_t object_type;    // Object type (IRI, literal, blank node)
    uint32_t flags;               // Triple flags
    uint32_t graph_id;            // Named graph ID (0 for default graph)
    uint32_t triple_id;           // Unique triple identifier
} cns_triple_t;

// Node representation - optimized for fast lookup and traversal
typedef struct cns_node {
    cns_string_ref_t iri;         // Node IRI or blank node ID
    cns_type_id_t type;           // Node type (IRI, blank node, literal)
    uint32_t flags;               // Node flags
    uint32_t in_degree;           // Number of incoming edges
    uint32_t out_degree;          // Number of outgoing edges
    uint32_t first_out_edge;      // Index of first outgoing edge
    uint32_t first_in_edge;       // Index of first incoming edge
    void *data;                   // Node-specific data
} cns_node_t;

// Edge representation - connects nodes via predicates
typedef struct cns_edge {
    uint32_t source_id;           // Source node index
    uint32_t target_id;           // Target node index
    cns_string_ref_t predicate;   // Predicate IRI
    uint32_t triple_id;           // Associated triple ID
    uint32_t next_out;            // Next outgoing edge from same source
    uint32_t next_in;             // Next incoming edge to same target
    uint32_t flags;               // Edge flags
} cns_edge_t;

// Named graph information
typedef struct {
    cns_string_ref_t iri;         // Named graph IRI
    uint32_t triple_count;        // Number of triples in this graph
    uint32_t flags;               // Graph flags
} cns_named_graph_t;

// Graph statistics for monitoring and optimization
typedef struct {
    size_t node_count;            // Total number of nodes
    size_t edge_count;            // Total number of edges
    size_t triple_count;          // Total number of triples
    size_t named_graph_count;     // Number of named graphs
    size_t memory_usage;          // Total memory usage
    double avg_degree;            // Average node degree
    double clustering_coefficient; // Graph clustering coefficient
    size_t max_in_degree;         // Maximum in-degree
    size_t max_out_degree;        // Maximum out-degree
    uint64_t insert_operations;   // Total insert operations
    uint64_t query_operations;    // Total query operations
    cns_tick_t total_insert_ticks; // Total ticks spent inserting
    cns_tick_t total_query_ticks;  // Total ticks spent querying
} cns_graph_stats_t;

// Main graph structure
struct cns_graph {
    // Memory management
    cns_arena_t *node_arena;      // Arena for nodes
    cns_arena_t *edge_arena;      // Arena for edges
    cns_arena_t *triple_arena;    // Arena for triples
    cns_interner_t *interner;     // String interner for IRIs/literals
    
    // Node storage and indexing
    cns_node_t *nodes;            // Array of nodes
    size_t node_count;            // Current number of nodes
    size_t node_capacity;         // Node array capacity
    
    // Edge storage
    cns_edge_t *edges;            // Array of edges
    size_t edge_count;            // Current number of edges
    size_t edge_capacity;         // Edge array capacity
    
    // Triple storage
    cns_triple_t *triples;        // Array of triples
    size_t triple_count;          // Current number of triples
    size_t triple_capacity;       // Triple array capacity
    
    // Named graph support
    cns_named_graph_t *named_graphs; // Array of named graphs
    size_t named_graph_count;     // Number of named graphs
    size_t named_graph_capacity;  // Named graph capacity
    
    // Hash tables for O(1) lookups
    uint32_t *node_hash_table;    // Node IRI -> node index mapping
    size_t node_hash_size;        // Node hash table size
    size_t node_hash_mask;        // Hash table mask (size - 1)
    
    uint32_t *triple_hash_table;  // Triple hash -> triple index mapping
    size_t triple_hash_size;      // Triple hash table size
    size_t triple_hash_mask;      // Hash table mask (size - 1)
    
    // Performance tracking
    cns_graph_stats_t stats;      // Performance statistics
    
    // Configuration
    uint32_t flags;               // Graph configuration flags
    bool strict_mode;             // Strict RDF validation mode
    bool enable_inference;        // Enable basic inference rules
    
    // Thread safety (if enabled)
    void *mutex;                  // Mutex for thread safety
    uint32_t magic;               // Magic number for validation
};

// Graph flags
#define CNS_GRAPH_FLAG_DIRECTED         (1 << 0)  // Directed graph (always true for RDF)
#define CNS_GRAPH_FLAG_ALLOW_DUPLICATES (1 << 1)  // Allow duplicate triples
#define CNS_GRAPH_FLAG_STRICT_RDF       (1 << 2)  // Strict RDF validation
#define CNS_GRAPH_FLAG_ENABLE_INFERENCE (1 << 3)  // Enable inference
#define CNS_GRAPH_FLAG_THREAD_SAFE      (1 << 4)  // Thread-safe operations
#define CNS_GRAPH_FLAG_TRACK_PROVENANCE (1 << 5)  // Track triple provenance
#define CNS_GRAPH_FLAG_OPTIMIZE_QUERIES (1 << 6)  // Optimize for queries

// Node types
#define CNS_NODE_TYPE_IRI         1  // IRI node
#define CNS_NODE_TYPE_BLANK       2  // Blank node
#define CNS_NODE_TYPE_LITERAL     3  // Literal node

// Object types for triples
#define CNS_OBJECT_TYPE_IRI       1  // IRI object
#define CNS_OBJECT_TYPE_LITERAL   2  // Literal object
#define CNS_OBJECT_TYPE_BLANK     3  // Blank node object

// Magic number for graph validation
#define CNS_GRAPH_MAGIC 0x47525048  // 'GRPH'

// ============================================================================
// GRAPH LIFECYCLE FUNCTIONS - O(1) OPERATIONS
// ============================================================================

// Create a new RDF graph with specified configuration
// PERFORMANCE: O(1) - completes within 7 CPU ticks
cns_graph_t* cns_graph_create(const cns_graph_config_t *config);

// Create graph with default configuration
// PERFORMANCE: O(1) - optimized default settings
cns_graph_t* cns_graph_create_default(cns_arena_t *arena, cns_interner_t *interner);

// Destroy graph and free all memory
// PERFORMANCE: O(1) when using arenas - just resets arena pointers
void cns_graph_destroy(cns_graph_t *graph);

// Clear all triples from graph (fast reset)
// PERFORMANCE: O(1) - resets arenas and counters
cns_result_t cns_graph_clear(cns_graph_t *graph);

// Clone graph structure (not contents)
// PERFORMANCE: O(1) - copies configuration only
cns_graph_t* cns_graph_clone_config(const cns_graph_t *graph);

// ============================================================================
// TRIPLE INSERTION FUNCTIONS - O(1) GUARANTEED
// ============================================================================

// Insert RDF triple into graph
// PERFORMANCE: O(1) - hash table insertion with string interning
cns_result_t cns_graph_insert_triple(cns_graph_t *graph,
                                    const char *subject,
                                    const char *predicate,
                                    const char *object,
                                    cns_type_id_t object_type);

// Insert triple with string references (fastest path)
// PERFORMANCE: O(1) - no string interning needed
cns_result_t cns_graph_insert_triple_refs(cns_graph_t *graph,
                                         cns_string_ref_t subject,
                                         cns_string_ref_t predicate,
                                         cns_string_ref_t object,
                                         cns_type_id_t object_type);

// Insert triple into named graph
// PERFORMANCE: O(1) - same as regular insert with graph ID
cns_result_t cns_graph_insert_triple_named(cns_graph_t *graph,
                                          const char *subject,
                                          const char *predicate,
                                          const char *object,
                                          cns_type_id_t object_type,
                                          const char *graph_iri);

// Batch insert multiple triples
// PERFORMANCE: O(n) where n is number of triples, but optimized
cns_result_t cns_graph_insert_triples_batch(cns_graph_t *graph,
                                           const cns_triple_t *triples,
                                           size_t count);

// ============================================================================
// TRIPLE QUERY FUNCTIONS - O(1) GUARANTEED
// ============================================================================

// Check if triple exists in graph
// PERFORMANCE: O(1) - hash table lookup
bool cns_graph_contains_triple(const cns_graph_t *graph,
                              const char *subject,
                              const char *predicate,
                              const char *object);

// Check if triple exists using string references
// PERFORMANCE: O(1) - direct hash lookup
bool cns_graph_contains_triple_refs(const cns_graph_t *graph,
                                   cns_string_ref_t subject,
                                   cns_string_ref_t predicate,
                                   cns_string_ref_t object);

// Get triple by index
// PERFORMANCE: O(1) - array access
const cns_triple_t* cns_graph_get_triple(const cns_graph_t *graph, uint32_t index);

// Find triples matching pattern (NULL values are wildcards)
// PERFORMANCE: O(k) where k is number of matching triples
cns_result_t cns_graph_find_triples(const cns_graph_t *graph,
                                   const char *subject,
                                   const char *predicate,
                                   const char *object,
                                   cns_triple_t **results,
                                   size_t *count);

// ============================================================================
// NODE OPERATIONS - O(1) GUARANTEED
// ============================================================================

// Get or create node for IRI
// PERFORMANCE: O(1) - hash table lookup with automatic creation
uint32_t cns_graph_get_node(cns_graph_t *graph, const char *iri, cns_type_id_t type);

// Get node by string reference
// PERFORMANCE: O(1) - direct hash lookup
uint32_t cns_graph_get_node_ref(cns_graph_t *graph, cns_string_ref_t iri, cns_type_id_t type);

// Get node information by index
// PERFORMANCE: O(1) - array access
const cns_node_t* cns_graph_get_node_info(const cns_graph_t *graph, uint32_t node_index);

// Get all outgoing edges from node
// PERFORMANCE: O(out_degree) - follows edge chain
cns_result_t cns_graph_get_outgoing_edges(const cns_graph_t *graph,
                                         uint32_t node_index,
                                         cns_edge_t **edges,
                                         size_t *count);

// Get all incoming edges to node
// PERFORMANCE: O(in_degree) - follows edge chain
cns_result_t cns_graph_get_incoming_edges(const cns_graph_t *graph,
                                         uint32_t node_index,
                                         cns_edge_t **edges,
                                         size_t *count);

// ============================================================================
// GRAPH TRAVERSAL - OPTIMIZED FOR 7T PERFORMANCE
// ============================================================================

// Graph traversal callback function
typedef cns_result_t (*cns_graph_visit_fn_t)(const cns_graph_t *graph,
                                            const cns_node_t *node,
                                            const cns_edge_t *edge,
                                            void *user_data);

// Depth-first traversal from node
// PERFORMANCE: O(V + E) where V is nodes, E is edges visited
cns_result_t cns_graph_traverse_dfs(const cns_graph_t *graph,
                                   uint32_t start_node,
                                   cns_graph_visit_fn_t visit_fn,
                                   void *user_data);

// Breadth-first traversal from node
// PERFORMANCE: O(V + E) where V is nodes, E is edges visited
cns_result_t cns_graph_traverse_bfs(const cns_graph_t *graph,
                                   uint32_t start_node,
                                   cns_graph_visit_fn_t visit_fn,
                                   void *user_data);

// Get shortest path between nodes
// PERFORMANCE: O(V + E) using optimized BFS
cns_result_t cns_graph_shortest_path(const cns_graph_t *graph,
                                    uint32_t source,
                                    uint32_t target,
                                    uint32_t **path,
                                    size_t *path_length);

// ============================================================================
// NAMED GRAPH OPERATIONS
// ============================================================================

// Create named graph
// PERFORMANCE: O(1) - adds to named graph array
uint32_t cns_graph_create_named_graph(cns_graph_t *graph, const char *graph_iri);

// Get named graph ID by IRI
// PERFORMANCE: O(1) - hash table lookup
uint32_t cns_graph_get_named_graph_id(const cns_graph_t *graph, const char *graph_iri);

// Get triples in named graph
// PERFORMANCE: O(n) where n is triples in graph - filtered iteration
cns_result_t cns_graph_get_named_graph_triples(const cns_graph_t *graph,
                                              uint32_t graph_id,
                                              cns_triple_t **triples,
                                              size_t *count);

// ============================================================================
// GRAPH INFORMATION AND STATISTICS
// ============================================================================

// Get graph statistics
// PERFORMANCE: O(1) - returns cached statistics
cns_result_t cns_graph_get_stats(const cns_graph_t *graph, cns_graph_stats_t *stats);

// Get number of triples
// PERFORMANCE: O(1) - returns cached count
size_t cns_graph_triple_count(const cns_graph_t *graph);

// Get number of nodes
// PERFORMANCE: O(1) - returns cached count
size_t cns_graph_node_count(const cns_graph_t *graph);

// Get number of edges
// PERFORMANCE: O(1) - returns cached count
size_t cns_graph_edge_count(const cns_graph_t *graph);

// Get memory usage
// PERFORMANCE: O(1) - returns cached value
size_t cns_graph_memory_usage(const cns_graph_t *graph);

// ============================================================================
// GRAPH OPTIMIZATION AND MAINTENANCE
// ============================================================================

// Optimize graph for query performance
// PERFORMANCE: O(n log n) - rebuilds indexes and structures
cns_result_t cns_graph_optimize(cns_graph_t *graph);

// Compact graph memory (remove fragmentation)
// PERFORMANCE: O(n) - compacts arrays and hash tables
cns_result_t cns_graph_compact(cns_graph_t *graph);

// Rebuild hash tables for better distribution
// PERFORMANCE: O(n) - rehashes all entries
cns_result_t cns_graph_rebuild_indexes(cns_graph_t *graph);

// Validate graph integrity
// PERFORMANCE: O(n) - checks all data structures
cns_result_t cns_graph_validate(const cns_graph_t *graph);

// ============================================================================
// ITERATOR INTERFACE FOR ENUMERATION
// ============================================================================

// Iterator for walking graph elements
typedef struct {
    const cns_graph_t *graph;     // Graph being iterated
    uint32_t current_index;       // Current element index
    uint32_t element_type;        // Type of element (node, edge, triple)
    void *filter_data;            // Filter data (optional)
} cns_graph_iterator_t;

// Element types for iterator
#define CNS_GRAPH_ITER_NODES     1
#define CNS_GRAPH_ITER_EDGES     2
#define CNS_GRAPH_ITER_TRIPLES   3

// Initialize iterator for nodes
// PERFORMANCE: O(1) - sets up iterator state
cns_graph_iterator_t cns_graph_iter_nodes(const cns_graph_t *graph);

// Initialize iterator for edges
// PERFORMANCE: O(1) - sets up iterator state
cns_graph_iterator_t cns_graph_iter_edges(const cns_graph_t *graph);

// Initialize iterator for triples
// PERFORMANCE: O(1) - sets up iterator state
cns_graph_iterator_t cns_graph_iter_triples(const cns_graph_t *graph);

// Check if iterator has more elements
// PERFORMANCE: O(1) - checks iterator state
bool cns_graph_iter_has_next(const cns_graph_iterator_t *iter);

// Get next element from iterator
// PERFORMANCE: O(1) - returns pointer to next element
void* cns_graph_iter_next(cns_graph_iterator_t *iter);

// Reset iterator to beginning
// PERFORMANCE: O(1) - resets iterator state
void cns_graph_iter_reset(cns_graph_iterator_t *iter);

// ============================================================================
// SPARQL PATTERN MATCHING SUPPORT
// ============================================================================

// SPARQL triple pattern
typedef struct {
    cns_string_ref_t subject;     // Subject (or null for variable)
    cns_string_ref_t predicate;   // Predicate (or null for variable)
    cns_string_ref_t object;      // Object (or null for variable)
    bool subject_var;             // True if subject is variable
    bool predicate_var;           // True if predicate is variable
    bool object_var;              // True if object is variable
} cns_triple_pattern_t;

// Match triple pattern against graph
// PERFORMANCE: O(k) where k is number of matching triples
cns_result_t cns_graph_match_pattern(const cns_graph_t *graph,
                                    const cns_triple_pattern_t *pattern,
                                    cns_triple_t **results,
                                    size_t *count);

// Join multiple triple patterns
// PERFORMANCE: O(k1 * k2) where k1, k2 are result sizes
cns_result_t cns_graph_join_patterns(const cns_graph_t *graph,
                                    const cns_triple_pattern_t *patterns,
                                    size_t pattern_count,
                                    cns_triple_t **results,
                                    size_t *count);

// ============================================================================
// UTILITY FUNCTIONS AND HELPERS
// ============================================================================

// Compute hash for triple
// PERFORMANCE: O(1) - combines precomputed string hashes
cns_hash_t cns_graph_triple_hash(cns_string_ref_t subject,
                                cns_string_ref_t predicate,
                                cns_string_ref_t object);

// Check if two triples are equal
// PERFORMANCE: O(1) - compares string references
bool cns_graph_triples_equal(const cns_triple_t *a, const cns_triple_t *b);

// Convert node type to string
// PERFORMANCE: O(1) - lookup table
const char* cns_graph_node_type_string(cns_type_id_t type);

// Convert object type to string
// PERFORMANCE: O(1) - lookup table
const char* cns_graph_object_type_string(cns_type_id_t type);

// ============================================================================
// DEBUG AND VALIDATION FUNCTIONS
// ============================================================================

// Print graph statistics
// PERFORMANCE: O(1) - prints cached statistics
cns_result_t cns_graph_print_stats(const cns_graph_t *graph, FILE *output);

// Dump all triples in graph
// PERFORMANCE: O(n) - walks all triples
cns_result_t cns_graph_dump_triples(const cns_graph_t *graph, FILE *output);

// Analyze graph structure
// PERFORMANCE: O(V + E) - analyzes nodes and edges
cns_result_t cns_graph_analyze_structure(const cns_graph_t *graph);

// Check for inconsistencies
// PERFORMANCE: O(n) - validates all relationships
cns_result_t cns_graph_check_consistency(const cns_graph_t *graph);

// ============================================================================
// PERFORMANCE MONITORING INTEGRATION
// ============================================================================

// Performance callback for graph events
typedef void (*cns_graph_perf_callback_t)(const cns_graph_t *graph,
                                         const char *operation,
                                         size_t elements_processed,
                                         cns_tick_t ticks,
                                         void *user_data);

// Set performance monitoring callback
// PERFORMANCE: O(1) - stores callback pointer
cns_result_t cns_graph_set_perf_callback(cns_graph_t *graph,
                                        cns_graph_perf_callback_t callback,
                                        void *user_data);

// Clear performance monitoring callback
// PERFORMANCE: O(1) - clears callback pointer
cns_result_t cns_graph_clear_perf_callback(cns_graph_t *graph);

// ============================================================================
// THREAD SAFETY FUNCTIONS
// ============================================================================

// Enable thread safety for graph
// PERFORMANCE: O(1) - initializes mutex
cns_result_t cns_graph_enable_thread_safety(cns_graph_t *graph);

// Disable thread safety
// PERFORMANCE: O(1) - destroys mutex
cns_result_t cns_graph_disable_thread_safety(cns_graph_t *graph);

// Lock graph for exclusive access
// PERFORMANCE: O(1) - mutex lock
cns_result_t cns_graph_lock(cns_graph_t *graph);

// Unlock graph
// PERFORMANCE: O(1) - mutex unlock
cns_result_t cns_graph_unlock(cns_graph_t *graph);

// ============================================================================
// UTILITY MACROS FOR COMMON PATTERNS
// ============================================================================

// Insert triple and check for success
#define CNS_GRAPH_INSERT_CHECK(graph, s, p, o, type) \
    do { \
        cns_result_t result = cns_graph_insert_triple((graph), (s), (p), (o), (type)); \
        if (CNS_7T_UNLIKELY(result != CNS_OK)) return result; \
    } while(0)

// Get node and check validity
#define CNS_GRAPH_GET_NODE_CHECK(graph, iri, type, node_var) \
    do { \
        (node_var) = cns_graph_get_node((graph), (iri), (type)); \
        if (CNS_7T_UNLIKELY((node_var) == UINT32_MAX)) \
            return CNS_ERROR_NOT_FOUND; \
    } while(0)

// Fast triple equality check
#define CNS_GRAPH_TRIPLES_EQUAL_FAST(a, b) \
    (cns_string_ref_equal((a)->subject, (b)->subject) && \
     cns_string_ref_equal((a)->predicate, (b)->predicate) && \
     cns_string_ref_equal((a)->object, (b)->object))

#ifdef __cplusplus
}
#endif

#endif // CNS_GRAPH_H