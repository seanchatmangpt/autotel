/**
 * @file ttl_analyzer_usage.c
 * @brief Example usage of TTL graph analyzer for memory extraction
 * 
 * Demonstrates how to use the TTL analyzer to extract memory requirements
 * from RDF graphs and integrate with the AOT calculator for arena sizing.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cns/ttl_analyzer.h"
#include "cns/graph.h"
#include "cns/arena.h"
#include "cns/interner.h"

// ============================================================================
// EXAMPLE GRAPH DATA
// ============================================================================

// Sample TTL-like data (in C arrays for simplicity)
static const char* sample_triples[][3] = {
    // Person data
    {"http://example.org/john", "http://www.w3.org/1999/02/22-rdf-syntax-ns#type", "http://xmlns.com/foaf/0.1/Person"},
    {"http://example.org/john", "http://xmlns.com/foaf/0.1/name", "\"John Doe\""},
    {"http://example.org/john", "http://xmlns.com/foaf/0.1/age", "\"30\"^^http://www.w3.org/2001/XMLSchema#integer"},
    {"http://example.org/john", "http://xmlns.com/foaf/0.1/email", "\"john@example.org\""},
    
    {"http://example.org/jane", "http://www.w3.org/1999/02/22-rdf-syntax-ns#type", "http://xmlns.com/foaf/0.1/Person"},
    {"http://example.org/jane", "http://xmlns.com/foaf/0.1/name", "\"Jane Smith\""},
    {"http://example.org/jane", "http://xmlns.com/foaf/0.1/age", "\"28\"^^http://www.w3.org/2001/XMLSchema#integer"},
    {"http://example.org/jane", "http://xmlns.com/foaf/0.1/knows", "http://example.org/john"},
    
    // Organization data
    {"http://example.org/acme", "http://www.w3.org/1999/02/22-rdf-syntax-ns#type", "http://example.org/Organization"},
    {"http://example.org/acme", "http://xmlns.com/foaf/0.1/name", "\"Acme Corporation\""},
    {"http://example.org/acme", "http://example.org/hasEmployee", "http://example.org/john"},
    {"http://example.org/acme", "http://example.org/hasEmployee", "http://example.org/jane"},
    
    // Project data
    {"http://example.org/project1", "http://www.w3.org/1999/02/22-rdf-syntax-ns#type", "http://example.org/Project"},
    {"http://example.org/project1", "http://xmlns.com/foaf/0.1/name", "\"Knowledge Graph Project\""},
    {"http://example.org/project1", "http://example.org/assignedTo", "http://example.org/john"},
    {"http://example.org/project1", "http://example.org/assignedTo", "http://example.org/jane"},
    
    // Blank node examples
    {"_:meeting1", "http://www.w3.org/1999/02/22-rdf-syntax-ns#type", "http://example.org/Meeting"},
    {"_:meeting1", "http://example.org/hasAttendee", "http://example.org/john"},
    {"_:meeting1", "http://example.org/hasAttendee", "http://example.org/jane"},
    {"_:meeting1", "http://example.org/date", "\"2024-01-15\"^^http://www.w3.org/2001/XMLSchema#date"},
};

#define SAMPLE_TRIPLE_COUNT (sizeof(sample_triples) / sizeof(sample_triples[0]))

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

/**
 * Setup the CNS environment with arena and string interner
 */
static int setup_cns_environment(
    cns_arena_t **arena,
    cns_interner_t **interner,
    size_t arena_size
) {
    // Create main arena
    *arena = arenac_create(arena_size, ARENAC_FLAG_ZERO_ALLOC | ARENAC_FLAG_STATS);
    if (!*arena) {
        fprintf(stderr, "ERROR: Failed to create arena\n");
        return -1;
    }
    
    // Configure string interner
    cns_interner_config_t interner_config = {
        .initial_capacity = 1024,
        .string_arena_size = 128 * 1024,  // 128KB for strings
        .load_factor = 0.75f,
        .case_sensitive = true
    };
    
    *interner = cns_interner_create(&interner_config);
    if (!*interner) {
        arenac_destroy(*arena);
        fprintf(stderr, "ERROR: Failed to create string interner\n");
        return -1;
    }
    
    printf("✅ CNS environment initialized (Arena: %zu bytes)\n", arena_size);
    return 0;
}

/**
 * Load sample data into the graph
 */
static int load_sample_data(cns_graph_t *graph) {
    printf("📥 Loading sample data (%zu triples)...\n", SAMPLE_TRIPLE_COUNT);
    
    int success_count = 0;
    for (size_t i = 0; i < SAMPLE_TRIPLE_COUNT; i++) {
        cns_result_t result = cns_graph_insert_triple(
            graph,
            sample_triples[i][0],  // subject
            sample_triples[i][1],  // predicate
            sample_triples[i][2],  // object
            CNS_OBJECT_TYPE_IRI    // assume IRI objects for simplicity
        );
        
        if (result == CNS_OK) {
            success_count++;
        } else {
            printf("⚠️  Failed to insert triple %zu: %s -> %s -> %s\n", 
                   i, sample_triples[i][0], sample_triples[i][1], sample_triples[i][2]);
        }
    }
    
    printf("✅ Loaded %d/%zu triples successfully\n", success_count, SAMPLE_TRIPLE_COUNT);
    return success_count;
}

/**
 * Demonstrate fast memory estimation (80/20 approach)
 */
static void demonstrate_fast_estimation(cns_ttl_analyzer_t *analyzer) {
    printf("\n🚀 Fast Memory Estimation (80/20 Analysis)\n");
    printf("==========================================\n");
    
    // Perform fast estimation using 80% sampling
    cns_result_t result = cns_ttl_analyzer_estimate_memory(analyzer, 0.8);
    if (result != CNS_OK) {
        printf("❌ Fast estimation failed\n");
        return;
    }
    
    const cns_ttl_memory_layout_t *layout = cns_ttl_analyzer_get_layout(analyzer);
    
    printf("📊 Quick Analysis Results:\n");
    printf("   Nodes: %zu\n", layout->total_nodes);
    printf("   Edges: %zu\n", layout->total_edges);
    printf("   Literals: %zu\n", layout->total_literals);
    printf("   Total Memory: %zu bytes (%.2f KB)\n", 
           layout->total_memory_bytes, layout->total_memory_bytes / 1024.0);
    
    printf("\n🏗️  Arena Size Recommendations:\n");
    printf("   Main Arena: %zu bytes (%.2f KB)\n", 
           layout->main_arena_size, layout->main_arena_size / 1024.0);
    printf("   Node Arena: %zu bytes (%.2f KB)\n", 
           layout->node_arena_size, layout->node_arena_size / 1024.0);
    printf("   Edge Arena: %zu bytes (%.2f KB)\n", 
           layout->edge_arena_size, layout->edge_arena_size / 1024.0);
    printf("   String Arena: %zu bytes (%.2f KB)\n", 
           layout->string_arena_size, layout->string_arena_size / 1024.0);
}

/**
 * Demonstrate detailed graph analysis
 */
static void demonstrate_detailed_analysis(cns_ttl_analyzer_t *analyzer) {
    printf("\n🔍 Detailed Graph Analysis\n");
    printf("==========================\n");
    
    // Perform comprehensive analysis
    cns_result_t result = cns_ttl_analyzer_analyze_graph(analyzer);
    if (result != CNS_OK) {
        printf("❌ Detailed analysis failed\n");
        return;
    }
    
    const cns_ttl_memory_layout_t *layout = cns_ttl_analyzer_get_layout(analyzer);
    const cns_ttl_analysis_stats_t *stats = cns_ttl_analyzer_get_stats(analyzer);
    
    printf("📈 Analysis Statistics:\n");
    printf("   Nodes Visited: %u\n", stats->nodes_visited);
    printf("   Edges Traversed: %u\n", stats->edges_traversed);
    printf("   String Content: %zu bytes\n", stats->total_string_length);
    printf("   Analysis Time: %lu ticks\n", stats->analysis_time);
    
    printf("\n💾 Component Memory Breakdown:\n");
    for (uint32_t i = 0; i < layout->component_count; i++) {
        const cns_ttl_memory_requirement_t *req = &layout->components[i];
        size_t total_mem = req->base_memory + (req->per_element_memory * req->estimated_count);
        
        printf("   %s: %u items, %zu bytes\n",
               cns_ttl_analyzer_component_type_name(req->type),
               req->estimated_count, total_mem);
    }
    
    printf("\n⚙️  Optimization Recommendations:\n");
    printf("   Compression: %s\n", layout->enable_compression ? "✅ Recommended" : "❌ Not needed");
    printf("   String Interning: %s\n", layout->enable_interning ? "✅ Recommended" : "❌ Not needed");
    printf("   Indexing: %s\n", layout->enable_indexing ? "✅ Recommended" : "❌ Not needed");
}

/**
 * Demonstrate AOT integration
 */
static void demonstrate_aot_integration(cns_ttl_analyzer_t *analyzer) {
    printf("\n🏭 AOT Calculator Integration\n");
    printf("=============================\n");
    
    // Create arena configuration from analysis
    cns_arena_config_t config;
    cns_result_t result = cns_ttl_analyzer_create_arena_config(analyzer, &config);
    if (result != CNS_OK) {
        printf("❌ Failed to create arena config\n");
        return;
    }
    
    printf("🔧 Generated Arena Configuration:\n");
    printf("   Initial Size: %zu bytes (%.2f KB)\n", 
           config.initial_size, config.initial_size / 1024.0);
    printf("   Maximum Size: %zu bytes (%.2f KB)\n", 
           config.max_size, config.max_size / 1024.0);
    printf("   Alignment: %zu bytes\n", config.alignment);
    printf("   Guard Pages: %s\n", config.enable_guard ? "Enabled" : "Disabled");
    
    // Calculate growth projections
    const cns_ttl_memory_layout_t *layout = cns_ttl_analyzer_get_layout(analyzer);
    
    printf("\n📈 Growth Projections:\n");
    for (int horizon = 1; horizon <= 5; horizon++) {
        size_t projected_memory = cns_ttl_analyzer_estimate_growth_memory(
            analyzer, 1.5, horizon  // 50% growth per time period
        );
        printf("   %d periods: %zu bytes (%.2f KB)\n", 
               horizon, projected_memory, projected_memory / 1024.0);
    }
    
    // Demonstrate arena size calculation with safety factors
    printf("\n🛡️  Safety Factor Analysis:\n");
    double safety_factors[] = {1.0, 1.3, 1.5, 2.0};
    for (size_t i = 0; i < sizeof(safety_factors) / sizeof(safety_factors[0]); i++) {
        size_t safe_size = cns_ttl_analyzer_calculate_arena_size(analyzer, safety_factors[i]);
        printf("   %.1fx safety: %zu bytes (%.2f KB)\n", 
               safety_factors[i], safe_size, safe_size / 1024.0);
    }
}

/**
 * Generate and display comprehensive report
 */
static void generate_comprehensive_report(cns_ttl_analyzer_t *analyzer) {
    printf("\n📋 Comprehensive Memory Analysis Report\n");
    printf("=======================================\n");
    
    char report_buffer[4096];
    cns_result_t result = cns_ttl_analyzer_generate_report(
        analyzer, report_buffer, sizeof(report_buffer)
    );
    
    if (result == CNS_OK) {
        printf("%s\n", report_buffer);
    } else {
        printf("❌ Failed to generate report\n");
    }
}

// ============================================================================
// MAIN EXAMPLE PROGRAM
// ============================================================================

int main(int argc, char **argv) {
    (void)argc; (void)argv; // Suppress unused parameter warnings
    
    printf("TTL Graph Analyzer Usage Example\n");
    printf("=================================\n\n");
    
    // Initialize CNS environment
    cns_arena_t *arena = NULL;
    cns_interner_t *interner = NULL;
    
    if (setup_cns_environment(&arena, &interner, 1024 * 1024) != 0) {
        return 1;
    }
    
    // Create graph
    cns_graph_t *graph = cns_graph_create_default(arena, interner);
    if (!graph) {
        fprintf(stderr, "ERROR: Failed to create graph\n");
        arenac_destroy(arena);
        cns_interner_destroy(interner);
        return 1;
    }
    
    // Load sample data
    if (load_sample_data(graph) == 0) {
        fprintf(stderr, "ERROR: Failed to load any sample data\n");
        cns_graph_destroy(graph);
        arenac_destroy(arena);
        cns_interner_destroy(interner);
        return 1;
    }
    
    // Create TTL analyzer with detailed analysis enabled
    cns_ttl_analyzer_t *analyzer = cns_ttl_analyzer_create_configured(
        arena, graph,
        CNS_TTL_ANALYZER_FLAG_DETAILED | CNS_TTL_ANALYZER_FLAG_OPTIMIZE,
        50  // max depth
    );
    
    if (!analyzer) {
        fprintf(stderr, "ERROR: Failed to create TTL analyzer\n");
        cns_graph_destroy(graph);
        arenac_destroy(arena);
        cns_interner_destroy(interner);
        return 1;
    }
    
    // Demonstrate different analysis approaches
    demonstrate_fast_estimation(analyzer);
    demonstrate_detailed_analysis(analyzer);
    demonstrate_aot_integration(analyzer);
    generate_comprehensive_report(analyzer);
    
    // Performance validation
    printf("\n⚡ Performance Validation\n");
    printf("========================\n");
    
    cns_result_t validation = cns_ttl_analyzer_validate(analyzer);
    printf("Analyzer Validation: %s\n", 
           validation == CNS_OK ? "✅ Passed" : "❌ Failed");
    
    // Cleanup
    cns_graph_destroy(graph);
    cns_interner_destroy(interner);
    arenac_destroy(arena);
    
    printf("\n🎉 Example completed successfully!\n");
    printf("\n💡 Key Takeaways:\n");
    printf("   • TTL analyzer provides fast memory estimation for RDF graphs\n");
    printf("   • 80/20 approach gives quick results for arena sizing\n");
    printf("   • Detailed analysis provides optimization recommendations\n");
    printf("   • Results integrate seamlessly with AOT calculator\n");
    printf("   • Memory projections help with capacity planning\n");
    
    return 0;
}