#ifndef CNS_TTL_ANALYZER_H
#define CNS_TTL_ANALYZER_H

#include "cns/types.h"
#include "cns/graph.h"
#include "cns/arena.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// TTL GRAPH ANALYZER - MEMORY EXTRACTION FOR 7T SUBSTRATE
// ============================================================================

// TTL graph analyzer designed for fast memory requirement extraction from
// RDF graphs. Focuses on component identification and memory layout calculation
// for arena sizing in the 7T substrate system.

// ============================================================================
// MEMORY COMPONENT TYPES
// ============================================================================

// Component types found in TTL graphs
typedef enum {
    CNS_TTL_COMPONENT_UNKNOWN = 0,
    CNS_TTL_COMPONENT_NODE = 1,        // RDF nodes (subjects/objects)
    CNS_TTL_COMPONENT_EDGE = 2,        // RDF predicates/edges
    CNS_TTL_COMPONENT_LITERAL = 3,     // Literal values
    CNS_TTL_COMPONENT_BLANK = 4,       // Blank nodes
    CNS_TTL_COMPONENT_COLLECTION = 5,  // RDF collections/lists
    CNS_TTL_COMPONENT_NAMESPACE = 6,   // Namespace declarations
    CNS_TTL_COMPONENT_GRAPH = 7        // Named graphs
} cns_ttl_component_type_t;

// Memory requirement specification for a component
typedef struct {
    cns_ttl_component_type_t type;     // Component type
    size_t base_memory;                // Base memory requirement (bytes)
    size_t per_element_memory;         // Memory per element (bytes)
    size_t alignment_requirement;      // Required memory alignment
    uint32_t estimated_count;          // Estimated number of instances
    uint32_t max_count;                // Maximum possible instances
    double growth_factor;              // Expected growth multiplier
    char description[64];              // Human-readable description
} cns_ttl_memory_requirement_t;

// Memory layout specification for entire graph
typedef struct {
    // Total memory requirements
    size_t total_nodes;                // Total node count
    size_t total_edges;                // Total edge count
    size_t total_literals;             // Total literal count
    size_t total_memory_bytes;         // Total memory needed
    
    // Arena sizing recommendations
    size_t main_arena_size;            // Recommended main arena size
    size_t node_arena_size;            // Node storage arena
    size_t edge_arena_size;            // Edge storage arena
    size_t string_arena_size;          // String/literal storage arena
    size_t temp_arena_size;            // Temporary processing arena
    
    // Memory breakdown by component
    cns_ttl_memory_requirement_t components[16]; // Component requirements
    uint32_t component_count;          // Number of components
    
    // Performance metrics
    double parse_complexity;           // Estimated parse complexity
    double query_complexity;           // Estimated query complexity
    uint32_t max_depth;                // Maximum graph depth
    uint32_t avg_degree;               // Average node degree
    
    // Memory optimization hints
    uint32_t enable_compression;       // Recommend compression
    uint32_t enable_interning;         // Recommend string interning
    uint32_t enable_indexing;          // Recommend indexing
} cns_ttl_memory_layout_t;

// Graph traversal statistics for analysis
typedef struct {
    uint32_t nodes_visited;            // Nodes traversed
    uint32_t edges_traversed;          // Edges traversed
    uint32_t max_depth_reached;        // Maximum depth reached
    uint32_t cycles_detected;          // Graph cycles found
    size_t total_string_length;        // Total string content length
    size_t unique_predicates;          // Unique predicate count
    size_t unique_namespaces;          // Unique namespace count
    cns_tick_t analysis_time;          // Time spent analyzing
} cns_ttl_analysis_stats_t;

// ============================================================================
// TTL ANALYZER STRUCTURE
// ============================================================================

// Main TTL analyzer structure
typedef struct cns_ttl_analyzer {
    // Memory management
    cns_arena_t *arena;                // Arena for analyzer data
    cns_graph_t *graph;                // Graph being analyzed
    
    // Analysis configuration
    bool enable_detailed_analysis;     // Enable detailed component analysis
    bool enable_memory_profiling;      // Enable memory usage profiling
    bool estimate_growth;              // Estimate future growth
    uint32_t max_analysis_depth;       // Maximum traversal depth
    
    // Current analysis state
    cns_ttl_memory_layout_t layout;    // Current memory layout
    cns_ttl_analysis_stats_t stats;    // Analysis statistics
    
    // Temporary analysis data
    void *temp_data;                   // Temporary analysis structures
    size_t temp_data_size;             // Size of temporary data
    
    // Performance tracking
    cns_tick_t start_time;             // Analysis start time
    uint32_t flags;                    // Analyzer flags
} cns_ttl_analyzer_t;

// Analyzer flags
#define CNS_TTL_ANALYZER_FLAG_DETAILED     (1 << 0)  // Detailed analysis
#define CNS_TTL_ANALYZER_FLAG_PROFILING    (1 << 1)  // Memory profiling
#define CNS_TTL_ANALYZER_FLAG_GROWTH       (1 << 2)  // Growth estimation
#define CNS_TTL_ANALYZER_FLAG_OPTIMIZE     (1 << 3)  // Optimization hints
#define CNS_TTL_ANALYZER_FLAG_VALIDATE     (1 << 4)  // Validate results

// ============================================================================
// ANALYZER LIFECYCLE FUNCTIONS
// ============================================================================

// Create TTL analyzer for graph analysis
// PERFORMANCE: O(1) - completes within 7 CPU ticks
cns_ttl_analyzer_t* cns_ttl_analyzer_create(cns_arena_t *arena, cns_graph_t *graph);

// Create analyzer with configuration
// PERFORMANCE: O(1) - sets up analyzer with options
cns_ttl_analyzer_t* cns_ttl_analyzer_create_configured(
    cns_arena_t *arena,
    cns_graph_t *graph,
    uint32_t flags,
    uint32_t max_depth
);

// Destroy analyzer and free resources
// PERFORMANCE: O(1) - when using arenas
void cns_ttl_analyzer_destroy(cns_ttl_analyzer_t *analyzer);

// Reset analyzer for new analysis
// PERFORMANCE: O(1) - resets state
cns_result_t cns_ttl_analyzer_reset(cns_ttl_analyzer_t *analyzer);

// ============================================================================
// GRAPH ANALYSIS FUNCTIONS - CORE ANALYSIS
// ============================================================================

// Analyze graph for memory requirements
// PERFORMANCE: O(V + E) where V=nodes, E=edges
cns_result_t cns_ttl_analyzer_analyze_graph(cns_ttl_analyzer_t *analyzer);

// Analyze specific component type
// PERFORMANCE: O(k) where k=components of that type
cns_result_t cns_ttl_analyzer_analyze_component(
    cns_ttl_analyzer_t *analyzer,
    cns_ttl_component_type_t component_type
);

// Fast memory estimation (80/20 analysis)
// PERFORMANCE: O(V) - samples graph for quick estimation
cns_result_t cns_ttl_analyzer_estimate_memory(
    cns_ttl_analyzer_t *analyzer,
    double sample_ratio
);

// Analyze memory layout for arena sizing
// PERFORMANCE: O(1) - calculates from existing analysis
cns_result_t cns_ttl_analyzer_calculate_layout(cns_ttl_analyzer_t *analyzer);

// ============================================================================
// COMPONENT ANALYSIS FUNCTIONS
// ============================================================================

// Analyze node memory requirements
// PERFORMANCE: O(V) - analyzes all nodes
cns_result_t cns_ttl_analyzer_analyze_nodes(cns_ttl_analyzer_t *analyzer);

// Analyze edge memory requirements
// PERFORMANCE: O(E) - analyzes all edges
cns_result_t cns_ttl_analyzer_analyze_edges(cns_ttl_analyzer_t *analyzer);

// Analyze literal memory requirements
// PERFORMANCE: O(L) - analyzes all literals
cns_result_t cns_ttl_analyzer_analyze_literals(cns_ttl_analyzer_t *analyzer);

// Analyze string memory requirements (for interning)
// PERFORMANCE: O(S) - analyzes all strings
cns_result_t cns_ttl_analyzer_analyze_strings(cns_ttl_analyzer_t *analyzer);

// Analyze namespace declarations
// PERFORMANCE: O(N) - analyzes all namespaces
cns_result_t cns_ttl_analyzer_analyze_namespaces(cns_ttl_analyzer_t *analyzer);

// ============================================================================
// MEMORY CALCULATION FUNCTIONS
// ============================================================================

// Calculate total memory for component type
// PERFORMANCE: O(1) - uses cached analysis
size_t cns_ttl_analyzer_calculate_component_memory(
    const cns_ttl_analyzer_t *analyzer,
    cns_ttl_component_type_t component_type
);

// Calculate arena size recommendation
// PERFORMANCE: O(1) - uses cached analysis
size_t cns_ttl_analyzer_calculate_arena_size(
    const cns_ttl_analyzer_t *analyzer,
    double safety_factor
);

// Calculate memory overhead (alignment, metadata)
// PERFORMANCE: O(1) - calculates overhead
size_t cns_ttl_analyzer_calculate_overhead(
    const cns_ttl_analyzer_t *analyzer
);

// Estimate growth-based memory requirements
// PERFORMANCE: O(1) - projects from current analysis
size_t cns_ttl_analyzer_estimate_growth_memory(
    const cns_ttl_analyzer_t *analyzer,
    double growth_factor,
    uint32_t time_horizon
);

// ============================================================================
// OPTIMIZATION ANALYSIS
// ============================================================================

// Analyze graph structure for optimization opportunities
// PERFORMANCE: O(V + E) - analyzes structure
cns_result_t cns_ttl_analyzer_analyze_optimization_opportunities(
    cns_ttl_analyzer_t *analyzer
);

// Recommend memory layout optimizations
// PERFORMANCE: O(1) - generates recommendations
cns_result_t cns_ttl_analyzer_recommend_optimizations(
    cns_ttl_analyzer_t *analyzer
);

// Analyze query patterns for index recommendations
// PERFORMANCE: O(V + E) - analyzes access patterns
cns_result_t cns_ttl_analyzer_analyze_query_patterns(
    cns_ttl_analyzer_t *analyzer
);

// ============================================================================
// RESULT ACCESS FUNCTIONS
// ============================================================================

// Get memory layout results
// PERFORMANCE: O(1) - returns cached layout
const cns_ttl_memory_layout_t* cns_ttl_analyzer_get_layout(
    const cns_ttl_analyzer_t *analyzer
);

// Get analysis statistics
// PERFORMANCE: O(1) - returns cached stats
const cns_ttl_analysis_stats_t* cns_ttl_analyzer_get_stats(
    const cns_ttl_analyzer_t *analyzer
);

// Get specific component requirements
// PERFORMANCE: O(1) - lookup in component array
const cns_ttl_memory_requirement_t* cns_ttl_analyzer_get_component_requirement(
    const cns_ttl_analyzer_t *analyzer,
    cns_ttl_component_type_t component_type
);

// ============================================================================
// REPORTING FUNCTIONS
// ============================================================================

// Generate memory layout report
// PERFORMANCE: O(n) - formats output
cns_result_t cns_ttl_analyzer_generate_report(
    const cns_ttl_analyzer_t *analyzer,
    char *buffer,
    size_t buffer_size
);

// Print analysis summary
// PERFORMANCE: O(1) - prints summary
cns_result_t cns_ttl_analyzer_print_summary(
    const cns_ttl_analyzer_t *analyzer,
    FILE *output
);

// Export analysis to JSON
// PERFORMANCE: O(n) - serializes to JSON
cns_result_t cns_ttl_analyzer_export_json(
    const cns_ttl_analyzer_t *analyzer,
    char *buffer,
    size_t buffer_size
);

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

// Get component type name
// PERFORMANCE: O(1) - lookup table
const char* cns_ttl_analyzer_component_type_name(cns_ttl_component_type_t type);

// Estimate component memory from count
// PERFORMANCE: O(1) - calculation
size_t cns_ttl_analyzer_estimate_component_memory_from_count(
    cns_ttl_component_type_t type,
    uint32_t count
);

// Calculate memory alignment padding
// PERFORMANCE: O(1) - alignment calculation
size_t cns_ttl_analyzer_calculate_alignment_padding(
    size_t current_size,
    size_t alignment
);

// ============================================================================
// INTEGRATION WITH AOT CALCULATOR
// ============================================================================

// Create arena configuration from analysis
// PERFORMANCE: O(1) - converts layout to config
cns_result_t cns_ttl_analyzer_create_arena_config(
    const cns_ttl_analyzer_t *analyzer,
    cns_arena_config_t *config
);

// Validate analysis against actual memory usage
// PERFORMANCE: O(1) - compares estimates with actual
cns_result_t cns_ttl_analyzer_validate_estimates(
    const cns_ttl_analyzer_t *analyzer,
    const cns_graph_t *actual_graph
);

// Update analysis with runtime feedback
// PERFORMANCE: O(1) - updates estimates
cns_result_t cns_ttl_analyzer_update_with_feedback(
    cns_ttl_analyzer_t *analyzer,
    const cns_graph_stats_t *runtime_stats
);

// ============================================================================
// BATCH ANALYSIS FUNCTIONS
// ============================================================================

// Analyze multiple graphs for batch sizing
// PERFORMANCE: O(k * (V + E)) where k=number of graphs
cns_result_t cns_ttl_analyzer_batch_analyze(
    cns_ttl_analyzer_t *analyzer,
    cns_graph_t **graphs,
    uint32_t graph_count,
    cns_ttl_memory_layout_t *batch_layout
);

// Calculate consolidated memory requirements
// PERFORMANCE: O(k) where k=number of graphs
cns_result_t cns_ttl_analyzer_consolidate_requirements(
    const cns_ttl_memory_layout_t *layouts,
    uint32_t layout_count,
    cns_ttl_memory_layout_t *consolidated
);

// ============================================================================
// DEBUG AND VALIDATION
// ============================================================================

// Validate analyzer state
// PERFORMANCE: O(1) - checks internal consistency
cns_result_t cns_ttl_analyzer_validate(const cns_ttl_analyzer_t *analyzer);

// Debug print internal state
// PERFORMANCE: O(n) - prints debug info
cns_result_t cns_ttl_analyzer_debug_print(
    const cns_ttl_analyzer_t *analyzer,
    FILE *output
);

// Check analysis accuracy
// PERFORMANCE: O(V + E) - validates against graph
cns_result_t cns_ttl_analyzer_check_accuracy(
    const cns_ttl_analyzer_t *analyzer,
    double *accuracy_score
);

// ============================================================================
// UTILITY MACROS
// ============================================================================

// Check analyzer result and return on error
#define CNS_TTL_ANALYZER_CHECK(result) \
    do { \
        if (CNS_7T_UNLIKELY((result) != CNS_OK)) return (result); \
    } while(0)

// Get component memory with validation
#define CNS_TTL_ANALYZER_GET_COMPONENT_MEMORY(analyzer, type, result) \
    do { \
        const cns_ttl_memory_requirement_t *req = \
            cns_ttl_analyzer_get_component_requirement((analyzer), (type)); \
        (result) = req ? (req->base_memory + \
                         req->per_element_memory * req->estimated_count) : 0; \
    } while(0)

// Calculate aligned size
#define CNS_TTL_ANALYZER_ALIGN_SIZE(size, alignment) \
    (((size) + (alignment) - 1) & ~((alignment) - 1))

#ifdef __cplusplus
}
#endif

#endif // CNS_TTL_ANALYZER_H