/**
 * @file ttl_analyzer.c
 * @brief TTL Graph Analyzer Implementation - Memory extraction for 7T substrate
 * 
 * Fast graph analysis for memory requirement extraction. Focuses on 80/20
 * approach for quick memory sizing rather than full semantic reasoning.
 */

#include "cns/ttl_analyzer.h"
#include "cns/graph.h"
#include "cns/arena.h"
#include "cns/interner.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// ============================================================================
// INTERNAL CONSTANTS AND HELPERS
// ============================================================================

// Memory size constants for different component types
#define CNS_TTL_NODE_BASE_SIZE       64   // Base size per node
#define CNS_TTL_EDGE_BASE_SIZE       32   // Base size per edge
#define CNS_TTL_LITERAL_BASE_SIZE    48   // Base size per literal
#define CNS_TTL_STRING_OVERHEAD      16   // String interning overhead
#define CNS_TTL_INDEX_OVERHEAD       24   // Index structure overhead

// Default safety factors
#define CNS_TTL_SAFETY_FACTOR        1.3  // 30% safety margin
#define CNS_TTL_GROWTH_FACTOR        2.0  // 2x growth assumption
#define CNS_TTL_ALIGNMENT_PADDING    0.15 // 15% alignment overhead

// Component type names for reporting
static const char* component_type_names[] = {
    "unknown",
    "node",
    "edge", 
    "literal",
    "blank_node",
    "collection",
    "namespace",
    "graph"
};

// ============================================================================
// INTERNAL HELPER FUNCTIONS
// ============================================================================

/**
 * Initialize memory requirement structure
 */
static void init_memory_requirement(
    cns_ttl_memory_requirement_t *req,
    cns_ttl_component_type_t type,
    size_t base_memory,
    size_t per_element_memory
) {
    memset(req, 0, sizeof(*req));
    req->type = type;
    req->base_memory = base_memory;
    req->per_element_memory = per_element_memory;
    req->alignment_requirement = CNS_7T_ALIGNMENT;
    req->growth_factor = CNS_TTL_GROWTH_FACTOR;
    strncpy(req->description, component_type_names[type], sizeof(req->description) - 1);
}

/**
 * Estimate string memory requirements from graph
 */
static size_t estimate_string_memory(const cns_graph_t *graph) {
    if (!graph) return 0;
    
    // Estimate based on graph size and typical string lengths
    size_t node_count = cns_graph_node_count(graph);
    size_t triple_count = cns_graph_triple_count(graph);
    
    // Rough estimates: IRIs ~50 chars, literals ~100 chars average
    size_t estimated_iri_memory = node_count * 50;
    size_t estimated_literal_memory = triple_count * 30; // Not all objects are literals
    
    return estimated_iri_memory + estimated_literal_memory + 
           (node_count + triple_count) * CNS_TTL_STRING_OVERHEAD;
}

/**
 * Calculate memory alignment padding
 */
static size_t calculate_alignment_padding(size_t size, size_t alignment) {
    if (alignment == 0) return 0;
    size_t remainder = size % alignment;
    return remainder ? (alignment - remainder) : 0;
}

/**
 * Estimate average node degree from graph
 */
static double estimate_avg_degree(const cns_graph_t *graph) {
    if (!graph) return 0.0;
    
    size_t node_count = cns_graph_node_count(graph);
    size_t edge_count = cns_graph_edge_count(graph);
    
    if (node_count == 0) return 0.0;
    return (double)(edge_count * 2) / node_count; // Undirected degree approximation
}

// ============================================================================
// ANALYZER LIFECYCLE FUNCTIONS
// ============================================================================

cns_ttl_analyzer_t* cns_ttl_analyzer_create(cns_arena_t *arena, cns_graph_t *graph) {
    return cns_ttl_analyzer_create_configured(arena, graph, 0, 10);
}

cns_ttl_analyzer_t* cns_ttl_analyzer_create_configured(
    cns_arena_t *arena,
    cns_graph_t *graph,
    uint32_t flags,
    uint32_t max_depth
) {
    if (!arena || !graph) return NULL;
    
    CNS_7T_START_TIMING(arena);
    
    cns_ttl_analyzer_t *analyzer = ARENAC_NEW_ZERO(arena, cns_ttl_analyzer_t);
    if (!analyzer) return NULL;
    
    // Initialize basic fields
    analyzer->arena = arena;
    analyzer->graph = graph;
    analyzer->flags = flags;
    analyzer->max_analysis_depth = max_depth;
    analyzer->enable_detailed_analysis = (flags & CNS_TTL_ANALYZER_FLAG_DETAILED) != 0;
    analyzer->enable_memory_profiling = (flags & CNS_TTL_ANALYZER_FLAG_PROFILING) != 0;
    analyzer->estimate_growth = (flags & CNS_TTL_ANALYZER_FLAG_GROWTH) != 0;
    
    // Initialize layout structure
    memset(&analyzer->layout, 0, sizeof(analyzer->layout));
    memset(&analyzer->stats, 0, sizeof(analyzer->stats));
    
    analyzer->start_time = cns_get_tick_count();
    
    return analyzer;
}

void cns_ttl_analyzer_destroy(cns_ttl_analyzer_t *analyzer) {
    // With arena allocation, destruction is handled by arena reset
    (void)analyzer; // Unused when using arena
}

cns_result_t cns_ttl_analyzer_reset(cns_ttl_analyzer_t *analyzer) {
    if (!analyzer) return CNS_ERROR_INVALID_ARG;
    
    // Reset analysis state
    memset(&analyzer->layout, 0, sizeof(analyzer->layout));
    memset(&analyzer->stats, 0, sizeof(analyzer->stats));
    analyzer->temp_data = NULL;
    analyzer->temp_data_size = 0;
    analyzer->start_time = cns_get_tick_count();
    
    return CNS_OK;
}

// ============================================================================
// CORE ANALYSIS FUNCTIONS
// ============================================================================

cns_result_t cns_ttl_analyzer_analyze_graph(cns_ttl_analyzer_t *analyzer) {
    if (!analyzer || !analyzer->graph) return CNS_ERROR_INVALID_ARG;
    
    CNS_7T_START_TIMING(analyzer->arena);
    
    cns_result_t result;
    
    // Reset analysis state
    CNS_TTL_ANALYZER_CHECK(cns_ttl_analyzer_reset(analyzer));
    
    // Analyze each component type
    CNS_TTL_ANALYZER_CHECK(cns_ttl_analyzer_analyze_nodes(analyzer));
    CNS_TTL_ANALYZER_CHECK(cns_ttl_analyzer_analyze_edges(analyzer));
    CNS_TTL_ANALYZER_CHECK(cns_ttl_analyzer_analyze_literals(analyzer));
    CNS_TTL_ANALYZER_CHECK(cns_ttl_analyzer_analyze_strings(analyzer));
    
    // Calculate memory layout
    CNS_TTL_ANALYZER_CHECK(cns_ttl_analyzer_calculate_layout(analyzer));
    
    // Analyze optimization opportunities if requested
    if (analyzer->flags & CNS_TTL_ANALYZER_FLAG_OPTIMIZE) {
        result = cns_ttl_analyzer_analyze_optimization_opportunities(analyzer);
        if (result != CNS_OK) return result;
    }
    
    // Record analysis completion time
    analyzer->stats.analysis_time = cns_get_tick_count() - analyzer->start_time;
    
    return CNS_OK;
}

cns_result_t cns_ttl_analyzer_estimate_memory(
    cns_ttl_analyzer_t *analyzer,
    double sample_ratio
) {
    if (!analyzer || !analyzer->graph || sample_ratio <= 0.0 || sample_ratio > 1.0) {
        return CNS_ERROR_INVALID_ARG;
    }
    
    CNS_7T_START_TIMING(analyzer->arena);
    
    // Get graph statistics
    size_t total_nodes = cns_graph_node_count(analyzer->graph);
    size_t total_edges = cns_graph_edge_count(analyzer->graph);
    size_t total_triples = cns_graph_triple_count(analyzer->graph);
    
    // Fast estimation based on sample ratio
    analyzer->layout.total_nodes = total_nodes;
    analyzer->layout.total_edges = total_edges;
    
    // Estimate literals (assume 30% of triples have literal objects)
    analyzer->layout.total_literals = (size_t)(total_triples * 0.3);
    
    // Calculate component requirements
    cns_ttl_memory_requirement_t *node_req = &analyzer->layout.components[0];
    init_memory_requirement(node_req, CNS_TTL_COMPONENT_NODE, 
                           CNS_TTL_NODE_BASE_SIZE, CNS_TTL_NODE_BASE_SIZE);
    node_req->estimated_count = total_nodes;
    node_req->max_count = (uint32_t)(total_nodes * CNS_TTL_GROWTH_FACTOR);
    
    cns_ttl_memory_requirement_t *edge_req = &analyzer->layout.components[1];
    init_memory_requirement(edge_req, CNS_TTL_COMPONENT_EDGE,
                           CNS_TTL_EDGE_BASE_SIZE, CNS_TTL_EDGE_BASE_SIZE);
    edge_req->estimated_count = total_edges;
    edge_req->max_count = (uint32_t)(total_edges * CNS_TTL_GROWTH_FACTOR);
    
    cns_ttl_memory_requirement_t *literal_req = &analyzer->layout.components[2];
    init_memory_requirement(literal_req, CNS_TTL_COMPONENT_LITERAL,
                           CNS_TTL_LITERAL_BASE_SIZE, CNS_TTL_LITERAL_BASE_SIZE);
    literal_req->estimated_count = analyzer->layout.total_literals;
    literal_req->max_count = (uint32_t)(analyzer->layout.total_literals * CNS_TTL_GROWTH_FACTOR);
    
    analyzer->layout.component_count = 3;
    
    // Calculate total memory
    CNS_TTL_ANALYZER_CHECK(cns_ttl_analyzer_calculate_layout(analyzer));
    
    return CNS_OK;
}

cns_result_t cns_ttl_analyzer_calculate_layout(cns_ttl_analyzer_t *analyzer) {
    if (!analyzer) return CNS_ERROR_INVALID_ARG;
    
    size_t total_memory = 0;
    
    // Sum up all component memory requirements
    for (uint32_t i = 0; i < analyzer->layout.component_count; i++) {
        const cns_ttl_memory_requirement_t *req = &analyzer->layout.components[i];
        size_t component_memory = req->base_memory + 
                                 (req->per_element_memory * req->estimated_count);
        
        // Add alignment padding
        size_t padding = calculate_alignment_padding(component_memory, req->alignment_requirement);
        component_memory += padding;
        
        total_memory += component_memory;
    }
    
    // Apply safety factor
    total_memory = (size_t)(total_memory * CNS_TTL_SAFETY_FACTOR);
    
    // Calculate arena sizes with reasonable distribution
    analyzer->layout.total_memory_bytes = total_memory;
    analyzer->layout.main_arena_size = total_memory;
    
    // Distribute memory across specialized arenas
    analyzer->layout.node_arena_size = total_memory * 0.4;     // 40% for nodes
    analyzer->layout.edge_arena_size = total_memory * 0.3;     // 30% for edges
    analyzer->layout.string_arena_size = total_memory * 0.2;   // 20% for strings
    analyzer->layout.temp_arena_size = total_memory * 0.1;     // 10% for temp
    
    // Estimate performance characteristics
    analyzer->layout.avg_degree = (uint32_t)estimate_avg_degree(analyzer->graph);
    analyzer->layout.parse_complexity = log10(analyzer->layout.total_nodes + 1.0);
    analyzer->layout.query_complexity = log10(analyzer->layout.total_edges + 1.0);
    
    // Set optimization recommendations
    analyzer->layout.enable_compression = (analyzer->layout.total_memory_bytes > 1024 * 1024); // > 1MB
    analyzer->layout.enable_interning = 1; // Always recommend string interning
    analyzer->layout.enable_indexing = (analyzer->layout.total_nodes > 1000); // > 1K nodes
    
    return CNS_OK;
}

// ============================================================================
// COMPONENT ANALYSIS FUNCTIONS
// ============================================================================

cns_result_t cns_ttl_analyzer_analyze_nodes(cns_ttl_analyzer_t *analyzer) {
    if (!analyzer || !analyzer->graph) return CNS_ERROR_INVALID_ARG;
    
    size_t node_count = cns_graph_node_count(analyzer->graph);
    analyzer->layout.total_nodes = node_count;
    analyzer->stats.nodes_visited = node_count;
    
    // Find or create node component requirement
    cns_ttl_memory_requirement_t *req = NULL;
    for (uint32_t i = 0; i < analyzer->layout.component_count; i++) {
        if (analyzer->layout.components[i].type == CNS_TTL_COMPONENT_NODE) {
            req = &analyzer->layout.components[i];
            break;
        }
    }
    
    if (!req && analyzer->layout.component_count < 16) {
        req = &analyzer->layout.components[analyzer->layout.component_count++];
        init_memory_requirement(req, CNS_TTL_COMPONENT_NODE,
                               CNS_TTL_NODE_BASE_SIZE, CNS_TTL_NODE_BASE_SIZE);
    }
    
    if (req) {
        req->estimated_count = node_count;
        req->max_count = (uint32_t)(node_count * CNS_TTL_GROWTH_FACTOR);
    }
    
    return CNS_OK;
}

cns_result_t cns_ttl_analyzer_analyze_edges(cns_ttl_analyzer_t *analyzer) {
    if (!analyzer || !analyzer->graph) return CNS_ERROR_INVALID_ARG;
    
    size_t edge_count = cns_graph_edge_count(analyzer->graph);
    analyzer->layout.total_edges = edge_count;
    analyzer->stats.edges_traversed = edge_count;
    
    // Find or create edge component requirement
    cns_ttl_memory_requirement_t *req = NULL;
    for (uint32_t i = 0; i < analyzer->layout.component_count; i++) {
        if (analyzer->layout.components[i].type == CNS_TTL_COMPONENT_EDGE) {
            req = &analyzer->layout.components[i];
            break;
        }
    }
    
    if (!req && analyzer->layout.component_count < 16) {
        req = &analyzer->layout.components[analyzer->layout.component_count++];
        init_memory_requirement(req, CNS_TTL_COMPONENT_EDGE,
                               CNS_TTL_EDGE_BASE_SIZE, CNS_TTL_EDGE_BASE_SIZE);
    }
    
    if (req) {
        req->estimated_count = edge_count;
        req->max_count = (uint32_t)(edge_count * CNS_TTL_GROWTH_FACTOR);
    }
    
    return CNS_OK;
}

cns_result_t cns_ttl_analyzer_analyze_literals(cns_ttl_analyzer_t *analyzer) {
    if (!analyzer || !analyzer->graph) return CNS_ERROR_INVALID_ARG;
    
    // Estimate literals as ~30% of all triples (rough approximation)
    size_t triple_count = cns_graph_triple_count(analyzer->graph);
    size_t literal_count = (size_t)(triple_count * 0.3);
    
    analyzer->layout.total_literals = literal_count;
    
    // Find or create literal component requirement
    cns_ttl_memory_requirement_t *req = NULL;
    for (uint32_t i = 0; i < analyzer->layout.component_count; i++) {
        if (analyzer->layout.components[i].type == CNS_TTL_COMPONENT_LITERAL) {
            req = &analyzer->layout.components[i];
            break;
        }
    }
    
    if (!req && analyzer->layout.component_count < 16) {
        req = &analyzer->layout.components[analyzer->layout.component_count++];
        init_memory_requirement(req, CNS_TTL_COMPONENT_LITERAL,
                               CNS_TTL_LITERAL_BASE_SIZE, CNS_TTL_LITERAL_BASE_SIZE);
    }
    
    if (req) {
        req->estimated_count = literal_count;
        req->max_count = (uint32_t)(literal_count * CNS_TTL_GROWTH_FACTOR);
    }
    
    return CNS_OK;
}

cns_result_t cns_ttl_analyzer_analyze_strings(cns_ttl_analyzer_t *analyzer) {
    if (!analyzer || !analyzer->graph) return CNS_ERROR_INVALID_ARG;
    
    // Estimate string memory requirements
    size_t estimated_string_memory = estimate_string_memory(analyzer->graph);
    analyzer->stats.total_string_length = estimated_string_memory;
    
    // Update string arena size recommendation
    analyzer->layout.string_arena_size = (size_t)(estimated_string_memory * CNS_TTL_SAFETY_FACTOR);
    
    return CNS_OK;
}

cns_result_t cns_ttl_analyzer_analyze_namespaces(cns_ttl_analyzer_t *analyzer) {
    if (!analyzer || !analyzer->graph) return CNS_ERROR_INVALID_ARG;
    
    // Simple estimation: ~10 namespaces per 1000 triples
    size_t triple_count = cns_graph_triple_count(analyzer->graph);
    size_t namespace_count = (triple_count / 100) + 1; // At least 1
    
    analyzer->stats.unique_namespaces = namespace_count;
    
    return CNS_OK;
}

// ============================================================================
// MEMORY CALCULATION FUNCTIONS
// ============================================================================

size_t cns_ttl_analyzer_calculate_component_memory(
    const cns_ttl_analyzer_t *analyzer,
    cns_ttl_component_type_t component_type
) {
    if (!analyzer) return 0;
    
    for (uint32_t i = 0; i < analyzer->layout.component_count; i++) {
        const cns_ttl_memory_requirement_t *req = &analyzer->layout.components[i];
        if (req->type == component_type) {
            return req->base_memory + (req->per_element_memory * req->estimated_count);
        }
    }
    
    return 0;
}

size_t cns_ttl_analyzer_calculate_arena_size(
    const cns_ttl_analyzer_t *analyzer,
    double safety_factor
) {
    if (!analyzer) return 0;
    
    return (size_t)(analyzer->layout.total_memory_bytes * safety_factor);
}

size_t cns_ttl_analyzer_calculate_overhead(const cns_ttl_analyzer_t *analyzer) {
    if (!analyzer) return 0;
    
    // Calculate overhead as 15% of total memory for alignment and metadata
    return (size_t)(analyzer->layout.total_memory_bytes * CNS_TTL_ALIGNMENT_PADDING);
}

size_t cns_ttl_analyzer_estimate_growth_memory(
    const cns_ttl_analyzer_t *analyzer,
    double growth_factor,
    uint32_t time_horizon
) {
    if (!analyzer) return 0;
    
    // Simple exponential growth model
    double growth_multiplier = pow(growth_factor, time_horizon);
    return (size_t)(analyzer->layout.total_memory_bytes * growth_multiplier);
}

// ============================================================================
// OPTIMIZATION ANALYSIS
// ============================================================================

cns_result_t cns_ttl_analyzer_analyze_optimization_opportunities(
    cns_ttl_analyzer_t *analyzer
) {
    if (!analyzer) return CNS_ERROR_INVALID_ARG;
    
    // Analyze graph characteristics for optimization hints
    size_t total_nodes = analyzer->layout.total_nodes;
    size_t total_memory = analyzer->layout.total_memory_bytes;
    
    // Set optimization flags based on thresholds
    analyzer->layout.enable_compression = (total_memory > 1024 * 1024); // > 1MB
    analyzer->layout.enable_interning = 1; // Always beneficial
    analyzer->layout.enable_indexing = (total_nodes > 1000); // > 1K nodes
    
    return CNS_OK;
}

cns_result_t cns_ttl_analyzer_recommend_optimizations(cns_ttl_analyzer_t *analyzer) {
    return cns_ttl_analyzer_analyze_optimization_opportunities(analyzer);
}

cns_result_t cns_ttl_analyzer_analyze_query_patterns(cns_ttl_analyzer_t *analyzer) {
    if (!analyzer) return CNS_ERROR_INVALID_ARG;
    
    // Simple heuristic: if high degree variance, recommend indexing
    if (analyzer->layout.avg_degree > 10) {
        analyzer->layout.enable_indexing = 1;
    }
    
    return CNS_OK;
}

// ============================================================================
// RESULT ACCESS FUNCTIONS
// ============================================================================

const cns_ttl_memory_layout_t* cns_ttl_analyzer_get_layout(
    const cns_ttl_analyzer_t *analyzer
) {
    return analyzer ? &analyzer->layout : NULL;
}

const cns_ttl_analysis_stats_t* cns_ttl_analyzer_get_stats(
    const cns_ttl_analyzer_t *analyzer
) {
    return analyzer ? &analyzer->stats : NULL;
}

const cns_ttl_memory_requirement_t* cns_ttl_analyzer_get_component_requirement(
    const cns_ttl_analyzer_t *analyzer,
    cns_ttl_component_type_t component_type
) {
    if (!analyzer) return NULL;
    
    for (uint32_t i = 0; i < analyzer->layout.component_count; i++) {
        if (analyzer->layout.components[i].type == component_type) {
            return &analyzer->layout.components[i];
        }
    }
    
    return NULL;
}

// ============================================================================
// REPORTING FUNCTIONS
// ============================================================================

cns_result_t cns_ttl_analyzer_generate_report(
    const cns_ttl_analyzer_t *analyzer,
    char *buffer,
    size_t buffer_size
) {
    if (!analyzer || !buffer || buffer_size == 0) return CNS_ERROR_INVALID_ARG;
    
    const cns_ttl_memory_layout_t *layout = &analyzer->layout;
    
    int written = snprintf(buffer, buffer_size,
        "TTL Graph Memory Analysis Report\n"
        "================================\n"
        "Total Nodes: %zu\n"
        "Total Edges: %zu\n"
        "Total Literals: %zu\n"
        "Total Memory: %zu bytes (%.2f MB)\n"
        "\n"
        "Arena Recommendations:\n"
        "- Main Arena: %zu bytes\n"
        "- Node Arena: %zu bytes\n"
        "- Edge Arena: %zu bytes\n"
        "- String Arena: %zu bytes\n"
        "- Temp Arena: %zu bytes\n"
        "\n"
        "Optimizations:\n"
        "- Compression: %s\n"
        "- String Interning: %s\n"
        "- Indexing: %s\n",
        layout->total_nodes,
        layout->total_edges,
        layout->total_literals,
        layout->total_memory_bytes,
        layout->total_memory_bytes / (1024.0 * 1024.0),
        layout->main_arena_size,
        layout->node_arena_size,
        layout->edge_arena_size,
        layout->string_arena_size,
        layout->temp_arena_size,
        layout->enable_compression ? "Recommended" : "Not needed",
        layout->enable_interning ? "Recommended" : "Not needed",
        layout->enable_indexing ? "Recommended" : "Not needed"
    );
    
    return (written > 0 && (size_t)written < buffer_size) ? CNS_OK : CNS_ERROR_CAPACITY;
}

cns_result_t cns_ttl_analyzer_print_summary(
    const cns_ttl_analyzer_t *analyzer,
    FILE *output
) {
    if (!analyzer || !output) return CNS_ERROR_INVALID_ARG;
    
    const cns_ttl_memory_layout_t *layout = &analyzer->layout;
    
    fprintf(output, "TTL Memory Analysis Summary:\n");
    fprintf(output, "  Nodes: %zu, Edges: %zu, Total Memory: %.2f MB\n",
            layout->total_nodes, layout->total_edges,
            layout->total_memory_bytes / (1024.0 * 1024.0));
    fprintf(output, "  Recommended Arena Size: %.2f MB\n",
            layout->main_arena_size / (1024.0 * 1024.0));
    
    return CNS_OK;
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

const char* cns_ttl_analyzer_component_type_name(cns_ttl_component_type_t type) {
    if (type < sizeof(component_type_names) / sizeof(component_type_names[0])) {
        return component_type_names[type];
    }
    return "unknown";
}

size_t cns_ttl_analyzer_estimate_component_memory_from_count(
    cns_ttl_component_type_t type,
    uint32_t count
) {
    size_t base_size = 0;
    
    switch (type) {
        case CNS_TTL_COMPONENT_NODE:
            base_size = CNS_TTL_NODE_BASE_SIZE;
            break;
        case CNS_TTL_COMPONENT_EDGE:
            base_size = CNS_TTL_EDGE_BASE_SIZE;
            break;
        case CNS_TTL_COMPONENT_LITERAL:
            base_size = CNS_TTL_LITERAL_BASE_SIZE;
            break;
        default:
            base_size = 32; // Default estimate
            break;
    }
    
    return base_size * count;
}

size_t cns_ttl_analyzer_calculate_alignment_padding(
    size_t current_size,
    size_t alignment
) {
    return calculate_alignment_padding(current_size, alignment);
}

// ============================================================================
// AOT INTEGRATION
// ============================================================================

cns_result_t cns_ttl_analyzer_create_arena_config(
    const cns_ttl_analyzer_t *analyzer,
    cns_arena_config_t *config
) {
    if (!analyzer || !config) return CNS_ERROR_INVALID_ARG;
    
    const cns_ttl_memory_layout_t *layout = &analyzer->layout;
    
    // Set up arena configuration based on analysis
    config->initial_size = layout->main_arena_size;
    config->max_size = layout->main_arena_size * 2; // Allow 2x growth
    config->alignment = CNS_7T_ALIGNMENT;
    config->enable_guard = false; // Disabled for performance
    config->alloc = NULL; // Use default allocator
    config->free = NULL;  // Use default deallocator
    config->user_data = NULL;
    
    return CNS_OK;
}

cns_result_t cns_ttl_analyzer_validate_estimates(
    const cns_ttl_analyzer_t *analyzer,
    const cns_graph_t *actual_graph
) {
    if (!analyzer || !actual_graph) return CNS_ERROR_INVALID_ARG;
    
    // Compare estimates with actual usage
    size_t actual_memory = cns_graph_memory_usage(actual_graph);
    size_t estimated_memory = analyzer->layout.total_memory_bytes;
    
    // Check if estimate is within reasonable bounds (50% to 200% of actual)
    if (estimated_memory < actual_memory / 2 || estimated_memory > actual_memory * 2) {
        return CNS_ERROR_VALIDATION;
    }
    
    return CNS_OK;
}

// ============================================================================
// DEBUG AND VALIDATION
// ============================================================================

cns_result_t cns_ttl_analyzer_validate(const cns_ttl_analyzer_t *analyzer) {
    if (!analyzer) return CNS_ERROR_INVALID_ARG;
    if (!analyzer->graph) return CNS_ERROR_INVALID_ARG;
    if (!analyzer->arena) return CNS_ERROR_INVALID_ARG;
    
    // Validate layout consistency
    const cns_ttl_memory_layout_t *layout = &analyzer->layout;
    if (layout->total_memory_bytes == 0) return CNS_ERROR_VALIDATION;
    if (layout->component_count > 16) return CNS_ERROR_VALIDATION;
    
    return CNS_OK;
}

cns_result_t cns_ttl_analyzer_debug_print(
    const cns_ttl_analyzer_t *analyzer,
    FILE *output
) {
    if (!analyzer || !output) return CNS_ERROR_INVALID_ARG;
    
    fprintf(output, "TTL Analyzer Debug Info:\n");
    fprintf(output, "  Graph: %p\n", (void*)analyzer->graph);
    fprintf(output, "  Arena: %p\n", (void*)analyzer->arena);
    fprintf(output, "  Flags: 0x%x\n", analyzer->flags);
    fprintf(output, "  Components: %u\n", analyzer->layout.component_count);
    
    for (uint32_t i = 0; i < analyzer->layout.component_count; i++) {
        const cns_ttl_memory_requirement_t *req = &analyzer->layout.components[i];
        fprintf(output, "    %s: %u items, %zu bytes\n",
                component_type_names[req->type],
                req->estimated_count,
                req->base_memory + req->per_element_memory * req->estimated_count);
    }
    
    return CNS_OK;
}