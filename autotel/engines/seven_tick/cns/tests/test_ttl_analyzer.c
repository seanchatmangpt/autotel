/**
 * @file test_ttl_analyzer.c
 * @brief Test suite for TTL Graph Analyzer
 * 
 * Comprehensive tests for memory extraction and analysis functionality
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>

// Include CNS headers
#include "cns/ttl_analyzer.h"
#include "cns/graph.h"
#include "cns/arena.h"
#include "cns/interner.h"
#include "cns/types.h"

// Test utilities
#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "FAIL: %s at %s:%d\n", message, __FILE__, __LINE__); \
            return -1; \
        } \
        printf("PASS: %s\n", message); \
    } while(0)

#define TEST_SUITE_START(name) \
    printf("\n=== Testing %s ===\n", name)

#define TEST_SUITE_END(name) \
    printf("=== %s Complete ===\n\n", name)

// Test data
static const char* test_subjects[] = {
    "http://example.org/person1",
    "http://example.org/person2", 
    "http://example.org/organization1",
    "_:blank1",
    "_:blank2"
};

static const char* test_predicates[] = {
    "http://www.w3.org/1999/02/22-rdf-syntax-ns#type",
    "http://xmlns.com/foaf/0.1/name",
    "http://xmlns.com/foaf/0.1/knows",
    "http://example.org/worksFor",
    "http://example.org/hasAge"
};

static const char* test_objects[] = {
    "http://xmlns.com/foaf/0.1/Person",
    "http://example.org/Organization",
    "\"John Doe\"",
    "\"Jane Smith\"",
    "\"25\"^^http://www.w3.org/2001/XMLSchema#integer"
};

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

/**
 * Create a test graph with sample data
 */
static cns_graph_t* create_test_graph(cns_arena_t *arena, cns_interner_t *interner) {
    cns_graph_t *graph = cns_graph_create_default(arena, interner);
    if (!graph) return NULL;
    
    // Insert test triples
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 3; j++) {
            cns_result_t result = cns_graph_insert_triple(
                graph,
                test_subjects[i % 5],
                test_predicates[j],
                test_objects[(i + j) % 5],
                CNS_OBJECT_TYPE_IRI
            );
            if (result != CNS_OK) {
                printf("Warning: Failed to insert triple %d,%d\n", i, j);
            }
        }
    }
    
    return graph;
}

/**
 * Setup test environment
 */
static int setup_test_environment(
    cns_arena_t **arena,
    cns_interner_t **interner,
    cns_graph_t **graph
) {
    // Create arena
    *arena = arenac_create(1024 * 1024, ARENAC_FLAG_ZERO_ALLOC); // 1MB arena
    if (!*arena) {
        fprintf(stderr, "Failed to create arena\n");
        return -1;
    }
    
    // Create string interner
    cns_interner_config_t interner_config = {
        .initial_capacity = 1024,
        .string_arena_size = 64 * 1024,
        .load_factor = 0.75f,
        .case_sensitive = true
    };
    
    *interner = cns_interner_create(&interner_config);
    if (!*interner) {
        arenac_destroy(*arena);
        fprintf(stderr, "Failed to create interner\n");
        return -1;
    }
    
    // Create test graph
    *graph = create_test_graph(*arena, *interner);
    if (!*graph) {
        cns_interner_destroy(*interner);
        arenac_destroy(*arena);
        fprintf(stderr, "Failed to create test graph\n");
        return -1;
    }
    
    return 0;
}

/**
 * Cleanup test environment
 */
static void cleanup_test_environment(
    cns_arena_t *arena,
    cns_interner_t *interner,
    cns_graph_t *graph
) {
    if (graph) cns_graph_destroy(graph);
    if (interner) cns_interner_destroy(interner);
    if (arena) arenac_destroy(arena);
}

// ============================================================================
// TEST FUNCTIONS
// ============================================================================

/**
 * Test analyzer creation and destruction
 */
static int test_analyzer_lifecycle() {
    TEST_SUITE_START("Analyzer Lifecycle");
    
    cns_arena_t *arena;
    cns_interner_t *interner;
    cns_graph_t *graph;
    
    if (setup_test_environment(&arena, &interner, &graph) != 0) {
        return -1;
    }
    
    // Test basic creation
    cns_ttl_analyzer_t *analyzer = cns_ttl_analyzer_create(arena, graph);
    TEST_ASSERT(analyzer != NULL, "Analyzer creation");
    TEST_ASSERT(analyzer->graph == graph, "Graph assignment");
    TEST_ASSERT(analyzer->arena == arena, "Arena assignment");
    
    // Test configured creation
    cns_ttl_analyzer_t *analyzer2 = cns_ttl_analyzer_create_configured(
        arena, graph, CNS_TTL_ANALYZER_FLAG_DETAILED, 20
    );
    TEST_ASSERT(analyzer2 != NULL, "Configured analyzer creation");
    TEST_ASSERT(analyzer2->enable_detailed_analysis == true, "Detailed analysis flag");
    TEST_ASSERT(analyzer2->max_analysis_depth == 20, "Max depth setting");
    
    // Test reset
    cns_result_t result = cns_ttl_analyzer_reset(analyzer);
    TEST_ASSERT(result == CNS_OK, "Analyzer reset");
    
    // Test validation
    result = cns_ttl_analyzer_validate(analyzer);
    TEST_ASSERT(result == CNS_OK, "Analyzer validation");
    
    cleanup_test_environment(arena, interner, graph);
    
    TEST_SUITE_END("Analyzer Lifecycle");
    return 0;
}

/**
 * Test graph analysis functionality
 */
static int test_graph_analysis() {
    TEST_SUITE_START("Graph Analysis");
    
    cns_arena_t *arena;
    cns_interner_t *interner;
    cns_graph_t *graph;
    
    if (setup_test_environment(&arena, &interner, &graph) != 0) {
        return -1;
    }
    
    cns_ttl_analyzer_t *analyzer = cns_ttl_analyzer_create(arena, graph);
    TEST_ASSERT(analyzer != NULL, "Analyzer creation for analysis");
    
    // Test full graph analysis
    cns_result_t result = cns_ttl_analyzer_analyze_graph(analyzer);
    TEST_ASSERT(result == CNS_OK, "Full graph analysis");
    
    // Check that analysis populated results
    const cns_ttl_memory_layout_t *layout = cns_ttl_analyzer_get_layout(analyzer);
    TEST_ASSERT(layout != NULL, "Layout retrieval");
    TEST_ASSERT(layout->total_nodes > 0, "Node count analysis");
    TEST_ASSERT(layout->total_memory_bytes > 0, "Memory calculation");
    TEST_ASSERT(layout->component_count > 0, "Component analysis");
    
    // Test component-specific analysis
    result = cns_ttl_analyzer_analyze_nodes(analyzer);
    TEST_ASSERT(result == CNS_OK, "Node analysis");
    
    result = cns_ttl_analyzer_analyze_edges(analyzer);
    TEST_ASSERT(result == CNS_OK, "Edge analysis");
    
    result = cns_ttl_analyzer_analyze_literals(analyzer);
    TEST_ASSERT(result == CNS_OK, "Literal analysis");
    
    result = cns_ttl_analyzer_analyze_strings(analyzer);
    TEST_ASSERT(result == CNS_OK, "String analysis");
    
    cleanup_test_environment(arena, interner, graph);
    
    TEST_SUITE_END("Graph Analysis");
    return 0;
}

/**
 * Test memory estimation functionality
 */
static int test_memory_estimation() {
    TEST_SUITE_START("Memory Estimation");
    
    cns_arena_t *arena;
    cns_interner_t *interner;
    cns_graph_t *graph;
    
    if (setup_test_environment(&arena, &interner, &graph) != 0) {
        return -1;
    }
    
    cns_ttl_analyzer_t *analyzer = cns_ttl_analyzer_create(arena, graph);
    TEST_ASSERT(analyzer != NULL, "Analyzer creation for estimation");
    
    // Test fast estimation (80/20 analysis)
    cns_result_t result = cns_ttl_analyzer_estimate_memory(analyzer, 0.8);
    TEST_ASSERT(result == CNS_OK, "Fast memory estimation");
    
    const cns_ttl_memory_layout_t *layout = cns_ttl_analyzer_get_layout(analyzer);
    TEST_ASSERT(layout->total_memory_bytes > 0, "Estimated memory calculation");
    TEST_ASSERT(layout->main_arena_size > 0, "Arena size recommendation");
    
    // Test memory calculation functions
    size_t node_memory = cns_ttl_analyzer_calculate_component_memory(
        analyzer, CNS_TTL_COMPONENT_NODE
    );
    TEST_ASSERT(node_memory > 0, "Node memory calculation");
    
    size_t arena_size = cns_ttl_analyzer_calculate_arena_size(analyzer, 1.5);
    TEST_ASSERT(arena_size > layout->total_memory_bytes, "Arena size with safety factor");
    
    size_t overhead = cns_ttl_analyzer_calculate_overhead(analyzer);
    TEST_ASSERT(overhead > 0, "Memory overhead calculation");
    
    cleanup_test_environment(arena, interner, graph);
    
    TEST_SUITE_END("Memory Estimation");
    return 0;
}

/**
 * Test layout calculation
 */
static int test_layout_calculation() {
    TEST_SUITE_START("Layout Calculation");
    
    cns_arena_t *arena;
    cns_interner_t *interner;
    cns_graph_t *graph;
    
    if (setup_test_environment(&arena, &interner, &graph) != 0) {
        return -1;
    }
    
    cns_ttl_analyzer_t *analyzer = cns_ttl_analyzer_create(arena, graph);
    TEST_ASSERT(analyzer != NULL, "Analyzer creation for layout");
    
    // Analyze graph first
    cns_result_t result = cns_ttl_analyzer_analyze_graph(analyzer);
    TEST_ASSERT(result == CNS_OK, "Graph analysis for layout");
    
    // Test layout calculation
    result = cns_ttl_analyzer_calculate_layout(analyzer);
    TEST_ASSERT(result == CNS_OK, "Layout calculation");
    
    const cns_ttl_memory_layout_t *layout = cns_ttl_analyzer_get_layout(analyzer);
    
    // Verify layout components
    TEST_ASSERT(layout->main_arena_size > 0, "Main arena size");
    TEST_ASSERT(layout->node_arena_size > 0, "Node arena size");
    TEST_ASSERT(layout->edge_arena_size > 0, "Edge arena size");
    TEST_ASSERT(layout->string_arena_size > 0, "String arena size");
    TEST_ASSERT(layout->temp_arena_size > 0, "Temp arena size");
    
    // Check that sub-arenas sum to reasonable total
    size_t sub_total = layout->node_arena_size + layout->edge_arena_size + 
                      layout->string_arena_size + layout->temp_arena_size;
    TEST_ASSERT(sub_total <= layout->main_arena_size * 1.2, "Arena size consistency");
    
    cleanup_test_environment(arena, interner, graph);
    
    TEST_SUITE_END("Layout Calculation");
    return 0;
}

/**
 * Test reporting functionality
 */
static int test_reporting() {
    TEST_SUITE_START("Reporting");
    
    cns_arena_t *arena;
    cns_interner_t *interner;
    cns_graph_t *graph;
    
    if (setup_test_environment(&arena, &interner, &graph) != 0) {
        return -1;
    }
    
    cns_ttl_analyzer_t *analyzer = cns_ttl_analyzer_create(arena, graph);
    TEST_ASSERT(analyzer != NULL, "Analyzer creation for reporting");
    
    // Analyze graph
    cns_result_t result = cns_ttl_analyzer_analyze_graph(analyzer);
    TEST_ASSERT(result == CNS_OK, "Graph analysis for reporting");
    
    // Test report generation
    char report_buffer[4096];
    result = cns_ttl_analyzer_generate_report(analyzer, report_buffer, sizeof(report_buffer));
    TEST_ASSERT(result == CNS_OK, "Report generation");
    TEST_ASSERT(strlen(report_buffer) > 0, "Report content");
    
    // Test summary printing (to stdout)
    printf("--- Sample Analysis Summary ---\n");
    result = cns_ttl_analyzer_print_summary(analyzer, stdout);
    TEST_ASSERT(result == CNS_OK, "Summary printing");
    printf("--- End Summary ---\n");
    
    // Test debug printing
    result = cns_ttl_analyzer_debug_print(analyzer, stdout);
    TEST_ASSERT(result == CNS_OK, "Debug printing");
    
    cleanup_test_environment(arena, interner, graph);
    
    TEST_SUITE_END("Reporting");
    return 0;
}

/**
 * Test utility functions
 */
static int test_utilities() {
    TEST_SUITE_START("Utilities");
    
    // Test component type name lookup
    const char *name = cns_ttl_analyzer_component_type_name(CNS_TTL_COMPONENT_NODE);
    TEST_ASSERT(strcmp(name, "node") == 0, "Component type name lookup");
    
    // Test memory estimation from count
    size_t memory = cns_ttl_analyzer_estimate_component_memory_from_count(
        CNS_TTL_COMPONENT_NODE, 100
    );
    TEST_ASSERT(memory > 0, "Memory estimation from count");
    
    // Test alignment calculation
    size_t padding = cns_ttl_analyzer_calculate_alignment_padding(33, 64);
    TEST_ASSERT(padding == 31, "Alignment padding calculation");
    
    TEST_SUITE_END("Utilities");
    return 0;
}

/**
 * Test integration with AOT calculator
 */
static int test_aot_integration() {
    TEST_SUITE_START("AOT Integration");
    
    cns_arena_t *arena;
    cns_interner_t *interner;
    cns_graph_t *graph;
    
    if (setup_test_environment(&arena, &interner, &graph) != 0) {
        return -1;
    }
    
    cns_ttl_analyzer_t *analyzer = cns_ttl_analyzer_create(arena, graph);
    TEST_ASSERT(analyzer != NULL, "Analyzer creation for AOT");
    
    // Analyze graph
    cns_result_t result = cns_ttl_analyzer_analyze_graph(analyzer);
    TEST_ASSERT(result == CNS_OK, "Graph analysis for AOT");
    
    // Test arena config creation
    cns_arena_config_t config;
    result = cns_ttl_analyzer_create_arena_config(analyzer, &config);
    TEST_ASSERT(result == CNS_OK, "Arena config creation");
    TEST_ASSERT(config.initial_size > 0, "Arena config initial size");
    TEST_ASSERT(config.alignment == CNS_7T_ALIGNMENT, "Arena config alignment");
    
    // Test estimate validation
    result = cns_ttl_analyzer_validate_estimates(analyzer, graph);
    TEST_ASSERT(result == CNS_OK, "Estimate validation");
    
    cleanup_test_environment(arena, interner, graph);
    
    TEST_SUITE_END("AOT Integration");
    return 0;
}

/**
 * Performance benchmark test
 */
static int test_performance() {
    TEST_SUITE_START("Performance");
    
    cns_arena_t *arena;
    cns_interner_t *interner;
    cns_graph_t *graph;
    
    if (setup_test_environment(&arena, &interner, &graph) != 0) {
        return -1;
    }
    
    cns_ttl_analyzer_t *analyzer = cns_ttl_analyzer_create(arena, graph);
    TEST_ASSERT(analyzer != NULL, "Analyzer creation for performance");
    
    // Measure analysis time
    clock_t start = clock();
    
    for (int i = 0; i < 100; i++) {
        cns_ttl_analyzer_reset(analyzer);
        cns_result_t result = cns_ttl_analyzer_analyze_graph(analyzer);
        if (result != CNS_OK) {
            TEST_ASSERT(false, "Performance test analysis failure");
            break;
        }
    }
    
    clock_t end = clock();
    double cpu_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    printf("Performance: 100 analyses in %.3f seconds (%.3f ms/analysis)\n", 
           cpu_time, cpu_time * 10.0);
    
    // Test should complete reasonably quickly
    TEST_ASSERT(cpu_time < 1.0, "Performance within 1 second for 100 runs");
    
    cleanup_test_environment(arena, interner, graph);
    
    TEST_SUITE_END("Performance");
    return 0;
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

int main(int argc, char **argv) {
    (void)argc; (void)argv; // Suppress unused parameter warnings
    
    printf("TTL Graph Analyzer Test Suite\n");
    printf("=============================\n");
    
    int failed = 0;
    
    // Run all test suites
    if (test_analyzer_lifecycle() != 0) failed++;
    if (test_graph_analysis() != 0) failed++;
    if (test_memory_estimation() != 0) failed++;
    if (test_layout_calculation() != 0) failed++;
    if (test_reporting() != 0) failed++;
    if (test_utilities() != 0) failed++;
    if (test_aot_integration() != 0) failed++;
    if (test_performance() != 0) failed++;
    
    // Summary
    printf("\n=============================\n");
    if (failed == 0) {
        printf("✅ All tests passed!\n");
        return 0;
    } else {
        printf("❌ %d test suite(s) failed!\n", failed);
        return 1;
    }
}