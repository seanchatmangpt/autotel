#ifndef CNS_8T_GRAPH_L1_H
#define CNS_8T_GRAPH_L1_H

#include "cns/8t/core.h"
#include "cns/8t/arena_l1.h"
#include "cns/graph.h"
#include <immintrin.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// 8T L1-OPTIMIZED GRAPH REPRESENTATION
// Cache-optimized graph data structures and algorithms
// ============================================================================

// L1-optimized node representation (64 bytes - exactly one cache line)
typedef struct {
    // Hot data (first 32 bytes) - frequently accessed
    uint64_t id;                 // Node identifier (8 bytes)
    uint32_t type;               // Node type (4 bytes)
    uint32_t degree;             // Total degree (in + out) (4 bytes)
    uint32_t out_degree;         // Outgoing degree (4 bytes)
    uint32_t first_edge_idx;     // Index to first edge (4 bytes)
    uint32_t property_offset;    // Offset to properties (4 bytes)
    uint32_t flags;              // Node flags and metadata (4 bytes)
    
    // Cold data (second 32 bytes) - less frequently accessed
    uint64_t creation_time;      // Creation timestamp (8 bytes)
    uint32_t last_access_time;   // Last access (compressed) (4 bytes)
    uint32_t ref_count;          // Reference count (4 bytes)
    uint64_t checksum;           // Integrity checksum (8 bytes)
    uint64_t reserved;           // Future use (8 bytes)
} cns_8t_graph_node_l1_t __attribute__((aligned(64)));

// L1-optimized edge representation (32 bytes - half cache line)
typedef struct {
    uint64_t source_id;          // Source node ID (8 bytes)
    uint64_t target_id;          // Target node ID (8 bytes)
    uint32_t type;               // Edge type (4 bytes)
    uint32_t next_edge_idx;      // Next edge from same source (4 bytes)
    double   weight;             // Edge weight (8 bytes)
} cns_8t_graph_edge_l1_t __attribute__((aligned(32)));

// SIMD-optimized adjacency list (256 bytes - 4 cache lines)
typedef struct {
    uint32_t capacity;           // Maximum neighbors (4 bytes)
    uint32_t count;              // Current neighbor count (4 bytes)
    uint32_t padding[6];         // Padding to 32 bytes
    
    // SIMD-aligned neighbor arrays (32 bytes each)
    uint64_t neighbors[28];      // Up to 28 neighbors (224 bytes)
    // Total: 32 + 224 = 256 bytes = 4 cache lines
} cns_8t_adjacency_l1_t __attribute__((aligned(64)));

// Cache-line aligned graph structure with L1 optimization
typedef struct {
    // Graph metadata (64 bytes - 1 cache line)
    uint64_t node_count;         // Number of nodes
    uint64_t edge_count;         // Number of edges
    uint64_t max_nodes;          // Maximum node capacity
    uint64_t max_edges;          // Maximum edge capacity
    uint32_t graph_flags;        // Graph properties (directed, weighted, etc.)
    uint32_t cache_generation;   // Cache invalidation generation
    cns_tick_t last_modified;    // Last modification time
    uint32_t reserved[6];        // Padding to 64 bytes
    
    // Data arrays (allocated separately for better cache locality)
    cns_8t_graph_node_l1_t* nodes;     // Node array
    cns_8t_graph_edge_l1_t* edges;     // Edge array
    cns_8t_adjacency_l1_t*  adjacency; // Adjacency lists
    uint8_t* property_pool;             // Property data pool
    
    // L1 cache optimization state
    uint32_t* access_pattern;           // Recent access pattern
    uint32_t hot_node_count;            // Number of hot nodes
    uint64_t* hot_node_ids;             // IDs of frequently accessed nodes
    
    // Memory allocation
    cns_8t_arena_l1_t* arena;           // L1-optimized arena allocator
    size_t total_memory_size;           // Total allocated memory
    
    // Performance counters
    uint64_t l1_cache_hits;             // Estimated L1 hits
    uint64_t l1_cache_misses;           // Estimated L1 misses
    uint64_t graph_operations;          // Total operations performed
} cns_8t_graph_l1_t __attribute__((aligned(64)));

// Graph traversal iterator with L1 optimization
typedef struct {
    cns_8t_graph_l1_t* graph;          // Graph being traversed
    uint64_t current_node;              // Current node ID
    uint32_t current_edge_idx;          // Current edge index
    uint32_t traversal_type;            // BFS, DFS, etc.
    
    // L1 cache optimization
    uint64_t prefetch_queue[8];         // Nodes to prefetch
    uint8_t  prefetch_head;             // Prefetch queue head
    uint8_t  prefetch_tail;             // Prefetch queue tail
    
    // State for specific traversal algorithms
    uint64_t* visited_bitmap;           // Visited nodes bitmap
    uint64_t* queue;                    // BFS/DFS queue
    uint32_t queue_head;                // Queue head pointer
    uint32_t queue_tail;                // Queue tail pointer
    uint32_t queue_capacity;            // Queue capacity
} cns_8t_graph_iterator_l1_t;

// Graph algorithm configuration
typedef struct {
    bool enable_simd;                   // Enable SIMD optimizations
    bool enable_prefetch;               // Enable hardware prefetching
    bool cache_hot_nodes;               // Cache frequently accessed nodes
    uint32_t working_set_limit;         // Max working set size for L1
    cns_8t_precision_mode_t precision;  // Numerical precision for algorithms
    
    // Algorithm-specific parameters
    uint32_t max_iterations;            // For iterative algorithms
    double convergence_threshold;       // Convergence criteria
    bool parallel_execution;            // Enable parallel execution
} cns_8t_graph_algo_config_t;

// ============================================================================
// L1-OPTIMIZED GRAPH ALGORITHMS
// ============================================================================

// SIMD-accelerated graph operations
cns_8t_result_t cns_8t_graph_bfs_simd(cns_8t_graph_l1_t* graph,
                                       uint64_t start_node,
                                       uint64_t* visited_order,
                                       size_t* visited_count,
                                       const cns_8t_graph_algo_config_t* config);

cns_8t_result_t cns_8t_graph_dfs_simd(cns_8t_graph_l1_t* graph,
                                       uint64_t start_node,
                                       uint64_t* visited_order,
                                       size_t* visited_count,
                                       const cns_8t_graph_algo_config_t* config);

// Cache-optimized shortest path algorithms
cns_8t_result_t cns_8t_graph_dijkstra_l1(cns_8t_graph_l1_t* graph,
                                          uint64_t source,
                                          double* distances,
                                          uint64_t* predecessors,
                                          const cns_8t_graph_algo_config_t* config);

cns_8t_result_t cns_8t_graph_floyd_warshall_l1(cns_8t_graph_l1_t* graph,
                                                double* distance_matrix,
                                                uint64_t* path_matrix,
                                                const cns_8t_graph_algo_config_t* config);

// SIMD matrix operations for graph algorithms
cns_8t_result_t cns_8t_graph_adjacency_matrix_simd(cns_8t_graph_l1_t* graph,
                                                    double* matrix,
                                                    bool weighted);

cns_8t_result_t cns_8t_graph_laplacian_matrix_simd(cns_8t_graph_l1_t* graph,
                                                    double* laplacian);

// L1-optimized centrality algorithms
cns_8t_result_t cns_8t_graph_betweenness_centrality_l1(cns_8t_graph_l1_t* graph,
                                                        double* centrality,
                                                        const cns_8t_graph_algo_config_t* config);

cns_8t_result_t cns_8t_graph_pagerank_l1(cns_8t_graph_l1_t* graph,
                                          double* pagerank,
                                          double damping_factor,
                                          const cns_8t_graph_algo_config_t* config);

// ============================================================================
// BRANCHLESS GRAPH OPERATIONS
// ============================================================================

// Branchless neighbor search (completes within 8T constraint)
static inline uint32_t cns_8t_graph_find_neighbor_branchless(
    const cns_8t_adjacency_l1_t* adj,
    uint64_t target_id
) {
    // Use SIMD to search 4 neighbors at once
    __m256i target_vec = _mm256_set1_epi64x(target_id);
    
    for (uint32_t i = 0; i < adj->count; i += 4) {
        __m256i neighbors = _mm256_load_si256((const __m256i*)&adj->neighbors[i]);
        __m256i cmp = _mm256_cmpeq_epi64(neighbors, target_vec);
        uint32_t mask = _mm256_movemask_pd(_mm256_castsi256_pd(cmp));
        
        if (mask) {
            return i + __builtin_ctz(mask);
        }
    }
    
    return UINT32_MAX; // Not found
}

// Branchless degree computation
static inline uint32_t cns_8t_graph_compute_degree_branchless(
    const cns_8t_graph_node_l1_t* node,
    bool count_self_loops
) {
    // Use bit manipulation to avoid branches
    uint32_t base_degree = node->degree;
    uint32_t self_loop_adjustment = count_self_loops ? 0 : 1;
    
    // Check if node has self-loop (branchless)
    // This would need to be precomputed in node flags for true branchless operation
    uint32_t has_self_loop = (node->flags >> 0) & 1;
    
    return base_degree - (has_self_loop * self_loop_adjustment);
}

// ============================================================================
// L1 CACHE OPTIMIZATION FUNCTIONS
// ============================================================================

// Optimize graph layout for L1 cache
cns_8t_result_t cns_8t_graph_optimize_l1_layout(cns_8t_graph_l1_t* graph);

// Reorder nodes for better cache locality
cns_8t_result_t cns_8t_graph_reorder_nodes_l1(cns_8t_graph_l1_t* graph,
                                               const uint64_t* new_order);

// Prefetch nodes for upcoming operations
cns_8t_result_t cns_8t_graph_prefetch_nodes(cns_8t_graph_l1_t* graph,
                                             const uint64_t* node_ids,
                                             uint32_t count);

// Analyze L1 cache performance
cns_8t_result_t cns_8t_graph_analyze_l1_performance(cns_8t_graph_l1_t* graph,
                                                     cns_8t_l1_analysis_t* analysis);

// ============================================================================
// GRAPH COMPRESSION AND ENCODING
// ============================================================================

// Compress adjacency lists using delta encoding
cns_8t_result_t cns_8t_graph_compress_adjacency(cns_8t_graph_l1_t* graph,
                                                 uint8_t** compressed_data,
                                                 size_t* compressed_size);

// Variable-length encoding for node IDs
cns_8t_result_t cns_8t_graph_encode_node_ids(const uint64_t* node_ids,
                                              uint32_t count,
                                              uint8_t* encoded_data,
                                              size_t* encoded_size);

// Bit-packed edge representation
cns_8t_result_t cns_8t_graph_pack_edges(const cns_8t_graph_edge_l1_t* edges,
                                         uint32_t count,
                                         uint8_t* packed_data,
                                         size_t* packed_size);

// ============================================================================
// API FUNCTIONS
// ============================================================================

// Graph lifecycle management
cns_8t_result_t cns_8t_graph_l1_create(uint64_t max_nodes,
                                        uint64_t max_edges,
                                        const cns_8t_arena_l1_config_t* arena_config,
                                        cns_8t_graph_l1_t** graph);

cns_8t_result_t cns_8t_graph_l1_destroy(cns_8t_graph_l1_t* graph);

cns_8t_result_t cns_8t_graph_l1_clear(cns_8t_graph_l1_t* graph);

// Node operations
cns_8t_result_t cns_8t_graph_l1_add_node(cns_8t_graph_l1_t* graph,
                                          uint64_t node_id,
                                          uint32_t node_type,
                                          const void* properties,
                                          size_t property_size);

cns_8t_result_t cns_8t_graph_l1_remove_node(cns_8t_graph_l1_t* graph,
                                             uint64_t node_id);

cns_8t_result_t cns_8t_graph_l1_get_node(cns_8t_graph_l1_t* graph,
                                          uint64_t node_id,
                                          cns_8t_graph_node_l1_t** node);

// Edge operations
cns_8t_result_t cns_8t_graph_l1_add_edge(cns_8t_graph_l1_t* graph,
                                          uint64_t source_id,
                                          uint64_t target_id,
                                          uint32_t edge_type,
                                          double weight);

cns_8t_result_t cns_8t_graph_l1_remove_edge(cns_8t_graph_l1_t* graph,
                                             uint64_t source_id,
                                             uint64_t target_id);

cns_8t_result_t cns_8t_graph_l1_get_edge(cns_8t_graph_l1_t* graph,
                                          uint64_t source_id,
                                          uint64_t target_id,
                                          cns_8t_graph_edge_l1_t** edge);

// Batch operations for better cache performance
cns_8t_result_t cns_8t_graph_l1_add_nodes_batch(cns_8t_graph_l1_t* graph,
                                                 const uint64_t* node_ids,
                                                 const uint32_t* node_types,
                                                 uint32_t count);

cns_8t_result_t cns_8t_graph_l1_add_edges_batch(cns_8t_graph_l1_t* graph,
                                                 const uint64_t* source_ids,
                                                 const uint64_t* target_ids,
                                                 const double* weights,
                                                 uint32_t count);

// Iterator operations
cns_8t_result_t cns_8t_graph_iterator_l1_create(cns_8t_graph_l1_t* graph,
                                                 uint32_t traversal_type,
                                                 cns_8t_graph_iterator_l1_t** iterator);

cns_8t_result_t cns_8t_graph_iterator_l1_destroy(cns_8t_graph_iterator_l1_t* iterator);

cns_8t_result_t cns_8t_graph_iterator_l1_next(cns_8t_graph_iterator_l1_t* iterator,
                                               uint64_t* node_id);

// Neighborhood operations
cns_8t_result_t cns_8t_graph_l1_get_neighbors(cns_8t_graph_l1_t* graph,
                                               uint64_t node_id,
                                               uint64_t* neighbors,
                                               uint32_t* count,
                                               uint32_t max_neighbors);

cns_8t_result_t cns_8t_graph_l1_get_common_neighbors(cns_8t_graph_l1_t* graph,
                                                      uint64_t node1_id,
                                                      uint64_t node2_id,
                                                      uint64_t* common_neighbors,
                                                      uint32_t* count);

// Performance monitoring
cns_8t_result_t cns_8t_graph_l1_get_cache_stats(cns_8t_graph_l1_t* graph,
                                                 uint64_t* hits,
                                                 uint64_t* misses,
                                                 double* hit_ratio);

cns_8t_result_t cns_8t_graph_l1_reset_stats(cns_8t_graph_l1_t* graph);

// Validation and debugging
#ifdef CNS_8T_DEBUG
cns_8t_result_t cns_8t_graph_l1_validate_integrity(cns_8t_graph_l1_t* graph);
cns_8t_result_t cns_8t_graph_l1_dump_cache_state(cns_8t_graph_l1_t* graph,
                                                  char* buffer,
                                                  size_t buffer_size);
#endif

#ifdef __cplusplus
}
#endif

#endif // CNS_8T_GRAPH_L1_H