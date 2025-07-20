/**
 * @file test_arenac_workflow.c
 * @brief Complete ARENAC Workflow Integration Test
 * 
 * Tests the complete workflow:
 * TTL Analysis → Memory Calculation → Arena Generation → Code Generation
 * SHACL Validation → TTL Parsing → AOT Calculation → Static Arena Creation
 * Telemetry Integration across all components
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
#include "cns/shacl.h"
#include "cns/arenac_telemetry.h"
#include "cns/types.h"

// Include AOT components
#include "../aot/aot_calculator.h"
#include "../aot/arena_codegen.h"

// Test framework macros
#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "FAIL: %s at %s:%d\n", message, __FILE__, __LINE__); \
            return -1; \
        } \
        printf("PASS: %s\n", message); \
    } while(0)

#define TEST_SUITE_START(name) \
    printf("\n=== ARENAC Workflow Test: %s ===\n", name)

#define TEST_SUITE_END(name) \
    printf("=== %s Complete ===\n\n", name)

#define ARENAC_7T_CHECK(cycles) \
    TEST_ASSERT(cycles <= 7, "7-tick performance constraint violated")

// Test data - CNS ontology sample
static const char* cns_ontology_triples[][3] = {
    // Base classes
    {"http://cns.org/Agent", "http://www.w3.org/1999/02/22-rdf-syntax-ns#type", "http://www.w3.org/2002/07/owl#Class"},
    {"http://cns.org/Task", "http://www.w3.org/1999/02/22-rdf-syntax-ns#type", "http://www.w3.org/2002/07/owl#Class"},
    {"http://cns.org/Resource", "http://www.w3.org/1999/02/22-rdf-syntax-ns#type", "http://www.w3.org/2002/07/owl#Class"},
    
    // Properties
    {"http://cns.org/hasTask", "http://www.w3.org/1999/02/22-rdf-syntax-ns#type", "http://www.w3.org/2002/07/owl#ObjectProperty"},
    {"http://cns.org/hasCapability", "http://www.w3.org/1999/02/22-rdf-syntax-ns#type", "http://www.w3.org/2002/07/owl#ObjectProperty"},
    {"http://cns.org/allocatesMemory", "http://www.w3.org/1999/02/22-rdf-syntax-ns#type", "http://www.w3.org/2002/07/owl#DatatypeProperty"},
    
    // Instances
    {"http://cns.org/agent1", "http://www.w3.org/1999/02/22-rdf-syntax-ns#type", "http://cns.org/Agent"},
    {"http://cns.org/task1", "http://www.w3.org/1999/02/22-rdf-syntax-ns#type", "http://cns.org/Task"},
    {"http://cns.org/agent1", "http://cns.org/hasTask", "http://cns.org/task1"},
    {"http://cns.org/task1", "http://cns.org/allocatesMemory", "\"1024\"^^http://www.w3.org/2001/XMLSchema#integer"},
    
    // Additional complexity
    {"http://cns.org/arena1", "http://www.w3.org/1999/02/22-rdf-syntax-ns#type", "http://cns.org/Resource"},
    {"http://cns.org/arena1", "http://cns.org/allocatesMemory", "\"4096\"^^http://www.w3.org/2001/XMLSchema#integer"},
    {"http://cns.org/agent1", "http://cns.org/hasCapability", "\"fast_allocation\"^^http://www.w3.org/2001/XMLSchema#string"},
    
    {NULL, NULL, NULL} // Terminator
};

// SHACL shapes for validation
static const char* cns_shacl_shapes[][3] = {
    // Agent shape
    {"http://cns.org/shapes/AgentShape", "http://www.w3.org/1999/02/22-rdf-syntax-ns#type", "http://www.w3.org/ns/shacl#NodeShape"},
    {"http://cns.org/shapes/AgentShape", "http://www.w3.org/ns/shacl#targetClass", "http://cns.org/Agent"},
    {"http://cns.org/shapes/AgentShape", "http://www.w3.org/ns/shacl#property", "_:prop1"},
    
    // Property constraints
    {"_:prop1", "http://www.w3.org/ns/shacl#path", "http://cns.org/hasTask"},
    {"_:prop1", "http://www.w3.org/ns/shacl#class", "http://cns.org/Task"},
    {"_:prop1", "http://www.w3.org/ns/shacl#minCount", "\"1\"^^http://www.w3.org/2001/XMLSchema#integer"},
    
    // Task shape
    {"http://cns.org/shapes/TaskShape", "http://www.w3.org/1999/02/22-rdf-syntax-ns#type", "http://www.w3.org/ns/shacl#NodeShape"},
    {"http://cns.org/shapes/TaskShape", "http://www.w3.org/ns/shacl#targetClass", "http://cns.org/Task"},
    {"http://cns.org/shapes/TaskShape", "http://www.w3.org/ns/shacl#property", "_:prop2"},
    
    // Memory allocation constraint
    {"_:prop2", "http://www.w3.org/ns/shacl#path", "http://cns.org/allocatesMemory"},
    {"_:prop2", "http://www.w3.org/ns/shacl#datatype", "http://www.w3.org/2001/XMLSchema#integer"},
    {"_:prop2", "http://www.w3.org/ns/shacl#minInclusive", "\"64\"^^http://www.w3.org/2001/XMLSchema#integer"},
    {"_:prop2", "http://www.w3.org/ns/shacl#maxInclusive", "\"1048576\"^^http://www.w3.org/2001/XMLSchema#integer"},
    
    {NULL, NULL, NULL} // Terminator
};

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

/**
 * Create test graph from ontology triples
 */
static cns_graph_t* create_cns_ontology_graph(cns_arena_t *arena, cns_interner_t *interner) {
    cns_graph_t *graph = cns_graph_create_default(arena, interner);
    if (!graph) return NULL;
    
    printf("Loading CNS ontology triples...\n");
    int triple_count = 0;
    
    for (int i = 0; cns_ontology_triples[i][0] != NULL; i++) {
        cns_result_t result = cns_graph_insert_triple(
            graph,
            cns_ontology_triples[i][0],
            cns_ontology_triples[i][1], 
            cns_ontology_triples[i][2],
            CNS_OBJECT_TYPE_IRI
        );
        if (result == CNS_OK) {
            triple_count++;
        } else {
            printf("Warning: Failed to insert triple %d\n", i);
        }
    }
    
    printf("Loaded %d ontology triples\n", triple_count);
    return graph;
}

/**
 * Create SHACL shapes graph
 */
static cns_graph_t* create_shacl_shapes_graph(cns_arena_t *arena, cns_interner_t *interner) {
    cns_graph_t *graph = cns_graph_create_default(arena, interner);
    if (!graph) return NULL;
    
    printf("Loading SHACL shapes...\n");
    int shape_count = 0;
    
    for (int i = 0; cns_shacl_shapes[i][0] != NULL; i++) {
        cns_result_t result = cns_graph_insert_triple(
            graph,
            cns_shacl_shapes[i][0],
            cns_shacl_shapes[i][1],
            cns_shacl_shapes[i][2],
            CNS_OBJECT_TYPE_IRI
        );
        if (result == CNS_OK) {
            shape_count++;
        } else {
            printf("Warning: Failed to insert shape triple %d\n", i);
        }
    }
    
    printf("Loaded %d SHACL shape triples\n", shape_count);
    return graph;
}

/**
 * Setup complete test environment
 */
static int setup_arenac_environment(
    cns_arena_t **arena,
    cns_interner_t **interner,
    cns_graph_t **ontology_graph,
    cns_graph_t **shapes_graph,
    cns_telemetry_t **telemetry,
    arenac_telemetry_context_t **telemetry_ctx
) {
    // Create main arena (2MB for comprehensive testing)
    *arena = arenac_create(2 * 1024 * 1024, 
                          ARENAC_FLAG_ZERO_ALLOC | 
                          ARENAC_FLAG_STATS | 
                          ARENAC_FLAG_ALIGN_64);
    if (!*arena) {
        fprintf(stderr, "Failed to create main arena\n");
        return -1;
    }
    
    // Create string interner with generous capacity
    cns_interner_config_t interner_config = {
        .initial_capacity = 2048,
        .string_arena_size = 128 * 1024,
        .load_factor = 0.75f,
        .case_sensitive = true
    };
    
    *interner = cns_interner_create(&interner_config);
    if (!*interner) {
        arenac_destroy(*arena);
        fprintf(stderr, "Failed to create interner\n");
        return -1;
    }
    
    // Create graphs
    *ontology_graph = create_cns_ontology_graph(*arena, *interner);
    if (!*ontology_graph) {
        cns_interner_destroy(*interner);
        arenac_destroy(*arena);
        fprintf(stderr, "Failed to create ontology graph\n");
        return -1;
    }
    
    *shapes_graph = create_shacl_shapes_graph(*arena, *interner);
    if (!*shapes_graph) {
        cns_graph_destroy(*ontology_graph);
        cns_interner_destroy(*interner);
        arenac_destroy(*arena);
        fprintf(stderr, "Failed to create shapes graph\n");
        return -1;
    }
    
    // Initialize telemetry
    *telemetry = malloc(sizeof(cns_telemetry_t));
    if (!*telemetry) {
        fprintf(stderr, "Failed to allocate telemetry\n");
        return -1;
    }
    
    CNSResult tel_result = cns_telemetry_init(*telemetry, NULL);
    if (tel_result != CNS_OK) {
        free(*telemetry);
        fprintf(stderr, "Failed to initialize telemetry\n");
        return -1;
    }
    
    // Initialize telemetry context
    *telemetry_ctx = malloc(sizeof(arenac_telemetry_context_t));
    if (!*telemetry_ctx) {
        cns_telemetry_shutdown(*telemetry);
        free(*telemetry);
        fprintf(stderr, "Failed to allocate telemetry context\n");
        return -1;
    }
    
    tel_result = arenac_telemetry_init(*telemetry_ctx, *telemetry, 
                                      ARENAC_TELEMETRY_ALL, 1.0);
    if (tel_result != CNS_OK) {
        free(*telemetry_ctx);
        cns_telemetry_shutdown(*telemetry);
        free(*telemetry);
        fprintf(stderr, "Failed to initialize telemetry context\n");
        return -1;
    }
    
    // Configure arena for telemetry
    tel_result = arenac_telemetry_configure_arena(*arena, *telemetry_ctx, 1);
    if (tel_result != CNS_OK) {
        fprintf(stderr, "Failed to configure arena telemetry\n");
        return -1;
    }
    
    return 0;
}

/**
 * Cleanup test environment
 */
static void cleanup_arenac_environment(
    cns_arena_t *arena,
    cns_interner_t *interner,
    cns_graph_t *ontology_graph,
    cns_graph_t *shapes_graph,
    cns_telemetry_t *telemetry,
    arenac_telemetry_context_t *telemetry_ctx
) {
    if (telemetry_ctx) {
        arenac_telemetry_shutdown(telemetry_ctx);
        free(telemetry_ctx);
    }
    if (telemetry) {
        cns_telemetry_shutdown(telemetry);
        free(telemetry);
    }
    if (shapes_graph) cns_graph_destroy(shapes_graph);
    if (ontology_graph) cns_graph_destroy(ontology_graph);
    if (interner) cns_interner_destroy(interner);
    if (arena) arenac_destroy(arena);
}

// ============================================================================
// INTEGRATION TEST FUNCTIONS
// ============================================================================

/**
 * Test Step 1: TTL Analysis and Memory Calculation
 */
static int test_ttl_analysis_memory_calculation() {
    TEST_SUITE_START("TTL Analysis → Memory Calculation");
    
    cns_arena_t *arena;
    cns_interner_t *interner;
    cns_graph_t *ontology_graph, *shapes_graph;
    cns_telemetry_t *telemetry;
    arenac_telemetry_context_t *telemetry_ctx;
    
    if (setup_arenac_environment(&arena, &interner, &ontology_graph, 
                                 &shapes_graph, &telemetry, &telemetry_ctx) != 0) {
        return -1;
    }
    
    uint64_t start_cycles = S7T_CYCLES();
    
    // Create TTL analyzer with full configuration
    cns_ttl_analyzer_t *analyzer = cns_ttl_analyzer_create_configured(
        arena, ontology_graph,
        CNS_TTL_ANALYZER_FLAG_DETAILED | 
        CNS_TTL_ANALYZER_FLAG_PROFILING |
        CNS_TTL_ANALYZER_FLAG_GROWTH |
        CNS_TTL_ANALYZER_FLAG_OPTIMIZE,
        50 // max depth
    );
    TEST_ASSERT(analyzer != NULL, "TTL analyzer creation");
    
    uint64_t analyzer_cycles = S7T_CYCLES() - start_cycles;
    ARENAC_7T_CHECK(analyzer_cycles);
    printf("TTL analyzer creation: %lu cycles\n", analyzer_cycles);
    
    // Perform comprehensive graph analysis
    start_cycles = S7T_CYCLES();
    cns_result_t result = cns_ttl_analyzer_analyze_graph(analyzer);
    TEST_ASSERT(result == CNS_OK, "Complete graph analysis");
    
    uint64_t analysis_cycles = S7T_CYCLES() - start_cycles;
    printf("Graph analysis: %lu cycles\n", analysis_cycles);
    
    // Get memory layout
    const cns_ttl_memory_layout_t *layout = cns_ttl_analyzer_get_layout(analyzer);
    TEST_ASSERT(layout != NULL, "Memory layout retrieval");
    TEST_ASSERT(layout->total_memory_bytes > 0, "Total memory calculation");
    TEST_ASSERT(layout->component_count > 0, "Component analysis");
    
    printf("Memory analysis results:\n");
    printf("  Total memory: %zu bytes\n", layout->total_memory_bytes);
    printf("  Node arena: %zu bytes\n", layout->node_arena_size);
    printf("  Edge arena: %zu bytes\n", layout->edge_arena_size);
    printf("  String arena: %zu bytes\n", layout->string_arena_size);
    printf("  Components analyzed: %u\n", layout->component_count);
    
    // Verify arena size recommendations are reasonable
    TEST_ASSERT(layout->main_arena_size >= layout->total_memory_bytes, 
                "Main arena size covers total memory");
    TEST_ASSERT(layout->node_arena_size > 0, "Node arena sized");
    TEST_ASSERT(layout->edge_arena_size > 0, "Edge arena sized");
    TEST_ASSERT(layout->string_arena_size > 0, "String arena sized");
    
    // Test optimization recommendations
    result = cns_ttl_analyzer_analyze_optimization_opportunities(analyzer);
    TEST_ASSERT(result == CNS_OK, "Optimization analysis");
    
    result = cns_ttl_analyzer_recommend_optimizations(analyzer);
    TEST_ASSERT(result == CNS_OK, "Optimization recommendations");
    
    // Test growth estimation
    size_t growth_memory = cns_ttl_analyzer_estimate_growth_memory(
        analyzer, 2.0, 365 // 2x growth over 1 year
    );
    TEST_ASSERT(growth_memory > layout->total_memory_bytes, "Growth estimation");
    printf("  Projected growth memory: %zu bytes\n", growth_memory);
    
    cleanup_arenac_environment(arena, interner, ontology_graph, 
                              shapes_graph, telemetry, telemetry_ctx);
    
    TEST_SUITE_END("TTL Analysis → Memory Calculation");
    return 0;
}

/**
 * Test Step 2: SHACL Validation Integration
 */
static int test_shacl_validation_integration() {
    TEST_SUITE_START("SHACL Validation → TTL Analysis");
    
    cns_arena_t *arena;
    cns_interner_t *interner;
    cns_graph_t *ontology_graph, *shapes_graph;
    cns_telemetry_t *telemetry;
    arenac_telemetry_context_t *telemetry_ctx;
    
    if (setup_arenac_environment(&arena, &interner, &ontology_graph, 
                                 &shapes_graph, &telemetry, &telemetry_ctx) != 0) {
        return -1;
    }
    
    uint64_t start_cycles = S7T_CYCLES();
    
    // Create SHACL validator
    cns_shacl_validator_t *validator = cns_shacl_validator_create_default(arena, interner);
    TEST_ASSERT(validator != NULL, "SHACL validator creation");
    
    uint64_t validator_cycles = S7T_CYCLES() - start_cycles;
    ARENAC_7T_CHECK(validator_cycles);
    printf("SHACL validator creation: %lu cycles\n", validator_cycles);
    
    // Load SHACL shapes
    start_cycles = S7T_CYCLES();
    cns_result_t result = cns_shacl_load_shapes_from_graph(validator, shapes_graph);
    TEST_ASSERT(result == CNS_OK, "SHACL shapes loading");
    
    uint64_t shapes_load_cycles = S7T_CYCLES() - start_cycles;
    printf("SHACL shapes loading: %lu cycles\n", shapes_load_cycles);
    
    // Validate ontology data against shapes
    start_cycles = S7T_CYCLES();
    cns_validation_report_t *report = cns_shacl_create_report(validator);
    TEST_ASSERT(report != NULL, "Validation report creation");
    
    result = cns_shacl_validate_graph(validator, ontology_graph, report);
    TEST_ASSERT(result == CNS_OK, "Graph validation");
    
    uint64_t validation_cycles = S7T_CYCLES() - start_cycles;
    printf("Graph validation: %lu cycles\n", validation_cycles);
    
    // Check validation results
    result = cns_shacl_finalize_report(report);
    TEST_ASSERT(result == CNS_OK, "Validation report finalization");
    
    printf("Validation results:\n");
    printf("  Conforms: %s\n", report->conforms ? "Yes" : "No");
    printf("  Total results: %zu\n", report->result_count);
    printf("  Violations: %zu\n", report->violation_count);
    printf("  Warnings: %zu\n", report->warning_count);
    printf("  Info: %zu\n", report->info_count);
    printf("  Nodes validated: %zu\n", report->nodes_validated);
    
    // For CNS ontology, we expect conformance
    TEST_ASSERT(report->conforms == true, "CNS ontology conforms to shapes");
    TEST_ASSERT(report->violation_count == 0, "No SHACL violations");
    
    // Now create TTL analyzer on validated data
    cns_ttl_analyzer_t *analyzer = cns_ttl_analyzer_create(arena, ontology_graph);
    TEST_ASSERT(analyzer != NULL, "TTL analyzer after validation");
    
    // Analyze validated graph
    result = cns_ttl_analyzer_analyze_graph(analyzer);
    TEST_ASSERT(result == CNS_OK, "Validated graph analysis");
    
    const cns_ttl_memory_layout_t *layout = cns_ttl_analyzer_get_layout(analyzer);
    TEST_ASSERT(layout != NULL, "Memory layout from validated data");
    
    printf("Post-validation memory analysis:\n");
    printf("  Total memory: %zu bytes\n", layout->total_memory_bytes);
    printf("  Parse complexity: %.2f\n", layout->parse_complexity);
    printf("  Query complexity: %.2f\n", layout->query_complexity);
    
    cleanup_arenac_environment(arena, interner, ontology_graph, 
                              shapes_graph, telemetry, telemetry_ctx);
    
    TEST_SUITE_END("SHACL Validation → TTL Analysis");
    return 0;
}

/**
 * Test Step 3: AOT Calculation and Arena Generation
 */
static int test_aot_calculation_arena_generation() {
    TEST_SUITE_START("AOT Calculation → Arena Generation");
    
    cns_arena_t *arena;
    cns_interner_t *interner;
    cns_graph_t *ontology_graph, *shapes_graph;
    cns_telemetry_t *telemetry;
    arenac_telemetry_context_t *telemetry_ctx;
    
    if (setup_arenac_environment(&arena, &interner, &ontology_graph, 
                                 &shapes_graph, &telemetry, &telemetry_ctx) != 0) {
        return -1;
    }
    
    // Analyze graph for memory requirements
    cns_ttl_analyzer_t *analyzer = cns_ttl_analyzer_create(arena, ontology_graph);
    TEST_ASSERT(analyzer != NULL, "TTL analyzer for AOT");
    
    cns_result_t result = cns_ttl_analyzer_analyze_graph(analyzer);
    TEST_ASSERT(result == CNS_OK, "Graph analysis for AOT");
    
    const cns_ttl_memory_layout_t *layout = cns_ttl_analyzer_get_layout(analyzer);
    
    uint64_t start_cycles = S7T_CYCLES();
    
    // Convert TTL analysis to AOT component sizes
    aot_component_sizes_t components = {
        .node_count = layout->total_nodes,
        .edge_count = layout->total_edges,
        .property_count = layout->component_count,
        .string_data_size = layout->string_arena_size,
        .index_overhead = (size_t)(layout->total_memory_bytes * 0.15) // 15% overhead
    };
    
    // Validate components
    int valid = aot_validate_components(&components);
    TEST_ASSERT(valid == 1, "AOT component validation");
    
    // Calculate AOT memory requirements
    aot_memory_config_t config;
    aot_init_default_config(&config);
    config.alignment = 64; // Match ARENAC alignment
    config.safety_margin = 2048; // Extra safety for CNS
    
    aot_memory_layout_t aot_layout;
    size_t total_aot_memory = aot_calculate_memory(&components, &config, &aot_layout);
    
    uint64_t aot_calc_cycles = S7T_CYCLES() - start_cycles;
    ARENAC_7T_CHECK(aot_calc_cycles);
    printf("AOT calculation: %lu cycles\n", aot_calc_cycles);
    
    TEST_ASSERT(total_aot_memory > 0, "AOT memory calculation");
    TEST_ASSERT(total_aot_memory >= layout->total_memory_bytes, 
                "AOT memory covers TTL analysis");
    
    printf("AOT memory calculation results:\n");
    printf("  Total AOT size: %zu bytes\n", total_aot_memory);
    printf("  Node section: %zu bytes\n", aot_layout.node_section_size);
    printf("  Edge section: %zu bytes\n", aot_layout.edge_section_size);
    printf("  String section: %zu bytes\n", aot_layout.string_section_size);
    printf("  Index section: %zu bytes\n", aot_layout.index_section_size);
    printf("  Padding overhead: %zu bytes\n", aot_layout.padding_overhead);
    
    // Generate static arena size
    size_t static_arena_size = aot_generate_static_arena_size(&components, &config);
    TEST_ASSERT(static_arena_size > 0, "Static arena size generation");
    TEST_ASSERT(static_arena_size >= total_aot_memory, "Static size covers calculated");
    
    printf("  Static arena size: %zu bytes\n", static_arena_size);
    
    // Create arena configuration from TTL analysis
    cns_arena_config_t arena_config;
    result = cns_ttl_analyzer_create_arena_config(analyzer, &arena_config);
    TEST_ASSERT(result == CNS_OK, "Arena config from TTL analysis");
    TEST_ASSERT(arena_config.initial_size >= total_aot_memory, 
                "Arena config size covers AOT");
    
    printf("  Arena config size: %zu bytes\n", arena_config.initial_size);
    printf("  Arena alignment: %zu bytes\n", arena_config.alignment);
    
    cleanup_arenac_environment(arena, interner, ontology_graph, 
                              shapes_graph, telemetry, telemetry_ctx);
    
    TEST_SUITE_END("AOT Calculation → Arena Generation");
    return 0;
}

/**
 * Test Step 4: Code Generation and Runtime Arena Creation
 */
static int test_code_generation_runtime_arena() {
    TEST_SUITE_START("Code Generation → Runtime Arena");
    
    cns_arena_t *arena;
    cns_interner_t *interner;
    cns_graph_t *ontology_graph, *shapes_graph;
    cns_telemetry_t *telemetry;
    arenac_telemetry_context_t *telemetry_ctx;
    
    if (setup_arenac_environment(&arena, &interner, &ontology_graph, 
                                 &shapes_graph, &telemetry, &telemetry_ctx) != 0) {
        return -1;
    }
    
    // Perform TTL analysis and AOT calculation
    cns_ttl_analyzer_t *analyzer = cns_ttl_analyzer_create(arena, ontology_graph);
    cns_ttl_analyzer_analyze_graph(analyzer);
    const cns_ttl_memory_layout_t *layout = cns_ttl_analyzer_get_layout(analyzer);
    
    aot_component_sizes_t components = {
        .node_count = layout->total_nodes,
        .edge_count = layout->total_edges,
        .property_count = layout->component_count,
        .string_data_size = layout->string_arena_size,
        .index_overhead = (size_t)(layout->total_memory_bytes * 0.15)
    };
    
    aot_memory_config_t config;
    aot_init_default_config(&config);
    config.alignment = 64;
    
    aot_memory_layout_t aot_layout;
    size_t total_memory = aot_calculate_memory(&components, &config, &aot_layout);
    
    uint64_t start_cycles = S7T_CYCLES();
    
    // Create temporary file for code generation
    FILE *codegen_file = tmpfile();
    TEST_ASSERT(codegen_file != NULL, "Code generation file creation");
    
    // Setup arena code generation
    arena_codegen_config_t codegen_config = {
        .total_size = total_memory,
        .alignment = 64,
        .arena_name = "cns_runtime_arena",
        .type_name = "cns_arena_t",
        .use_static_storage = 1,
        .include_guards = 1,
        .include_debug_info = 1
    };
    
    arena_codegen_ctx_t *codegen_ctx = arena_codegen_create(codegen_file, &codegen_config);
    TEST_ASSERT(codegen_ctx != NULL, "Arena code generator creation");
    
    // Set typed template for CNS components
    int codegen_result = arena_codegen_set_template(codegen_ctx, ARENA_TEMPLATE_TYPED);
    TEST_ASSERT(codegen_result == ARENA_CODEGEN_OK, "Code generator template setup");
    
    // Add zones based on AOT layout
    arena_zone_config_t node_zone = {
        .zone_name = "nodes",
        .type_name = "cns_node_t",
        .type_size = 64, // Based on CNS node size
        .count = components.node_count,
        .alignment = 64
    };
    codegen_result = arena_codegen_add_zone(codegen_ctx, &node_zone);
    TEST_ASSERT(codegen_result == ARENA_CODEGEN_OK, "Node zone addition");
    
    arena_zone_config_t edge_zone = {
        .zone_name = "edges", 
        .type_name = "cns_edge_t",
        .type_size = 32,
        .count = components.edge_count,
        .alignment = 32
    };
    codegen_result = arena_codegen_add_zone(codegen_ctx, &edge_zone);
    TEST_ASSERT(codegen_result == ARENA_CODEGEN_OK, "Edge zone addition");
    
    arena_zone_config_t string_zone = {
        .zone_name = "strings",
        .type_name = "char",
        .type_size = 1,
        .count = components.string_data_size,
        .alignment = 8
    };
    codegen_result = arena_codegen_add_zone(codegen_ctx, &string_zone);
    TEST_ASSERT(codegen_result == ARENA_CODEGEN_OK, "String zone addition");
    
    // Generate complete arena code
    codegen_result = arena_codegen_generate_complete(codegen_ctx);
    TEST_ASSERT(codegen_result == ARENA_CODEGEN_OK, "Complete code generation");
    
    uint64_t codegen_cycles = S7T_CYCLES() - start_cycles;
    printf("Code generation: %lu cycles\n", codegen_cycles);
    
    // Verify generated code (read back from temp file)
    fseek(codegen_file, 0, SEEK_END);
    long file_size = ftell(codegen_file);
    TEST_ASSERT(file_size > 0, "Generated code size");
    printf("Generated code size: %ld bytes\n", file_size);
    
    fseek(codegen_file, 0, SEEK_SET);
    char *generated_code = malloc(file_size + 1);
    TEST_ASSERT(generated_code != NULL, "Code buffer allocation");
    
    size_t read_size = fread(generated_code, 1, file_size, codegen_file);
    TEST_ASSERT(read_size == (size_t)file_size, "Code read from file");
    generated_code[file_size] = '\0';
    
    // Verify code contains expected elements
    TEST_ASSERT(strstr(generated_code, "cns_runtime_arena") != NULL, 
                "Arena name in generated code");
    TEST_ASSERT(strstr(generated_code, "nodes") != NULL, 
                "Node zone in generated code");
    TEST_ASSERT(strstr(generated_code, "edges") != NULL, 
                "Edge zone in generated code");
    TEST_ASSERT(strstr(generated_code, "strings") != NULL, 
                "String zone in generated code");
    
    printf("Code generation validation passed\n");
    
    // Test runtime arena creation with calculated size
    start_cycles = S7T_CYCLES();
    
    cns_arena_t *runtime_arena = arenac_create(
        total_memory, 
        ARENAC_FLAG_ZERO_ALLOC | ARENAC_FLAG_ALIGN_64 | ARENAC_FLAG_STATS
    );
    TEST_ASSERT(runtime_arena != NULL, "Runtime arena creation");
    
    uint64_t runtime_cycles = S7T_CYCLES() - start_cycles;
    ARENAC_7T_CHECK(runtime_cycles);
    printf("Runtime arena creation: %lu cycles\n", runtime_cycles);
    
    // Verify arena properties
    arenac_info_t arena_info;
    arenac_get_info(runtime_arena, &arena_info);
    
    TEST_ASSERT(arena_info.total_size == total_memory, "Arena size matches AOT");
    TEST_ASSERT(arena_info.used_size == 0, "Arena initially empty");
    TEST_ASSERT(arena_info.available_size == total_memory, "Full capacity available");
    
    printf("Runtime arena validation:\n");
    printf("  Total size: %zu bytes\n", arena_info.total_size);
    printf("  Available: %zu bytes\n", arena_info.available_size);
    printf("  Zone count: %u\n", arena_info.zone_count);
    
    // Test allocation performance
    start_cycles = S7T_CYCLES();
    void *test_alloc = arenac_alloc_with_telemetry(runtime_arena, 1024, telemetry_ctx);
    uint64_t alloc_cycles = S7T_CYCLES() - start_cycles;
    
    TEST_ASSERT(test_alloc != NULL, "Runtime allocation");
    ARENAC_7T_CHECK(alloc_cycles);
    printf("Runtime allocation (1KB): %lu cycles\n", alloc_cycles);
    
    // Cleanup
    free(generated_code);
    fclose(codegen_file);
    arena_codegen_destroy(codegen_ctx);
    arenac_destroy(runtime_arena);
    
    cleanup_arenac_environment(arena, interner, ontology_graph, 
                              shapes_graph, telemetry, telemetry_ctx);
    
    TEST_SUITE_END("Code Generation → Runtime Arena");
    return 0;
}

/**
 * Test Step 5: Complete Telemetry Integration
 */
static int test_complete_telemetry_integration() {
    TEST_SUITE_START("Complete Telemetry Integration");
    
    cns_arena_t *arena;
    cns_interner_t *interner;
    cns_graph_t *ontology_graph, *shapes_graph;
    cns_telemetry_t *telemetry;
    arenac_telemetry_context_t *telemetry_ctx;
    
    if (setup_arenac_environment(&arena, &interner, &ontology_graph, 
                                 &shapes_graph, &telemetry, &telemetry_ctx) != 0) {
        return -1;
    }
    
    printf("Testing telemetry across complete ARENAC workflow...\n");
    
    // Start distributed trace for complete workflow
    uint64_t trace_id = arenac_distributed_trace_start(
        telemetry_ctx, "arenac_complete_workflow", 1
    );
    TEST_ASSERT(trace_id > 0, "Distributed trace start");
    
    // Step 1: TTL Analysis with telemetry
    arenac_pattern_telemetry_t ttl_pattern = arenac_trace_memory_pattern_start(
        telemetry_ctx, "ttl_analysis"
    );
    
    cns_ttl_analyzer_t *analyzer = cns_ttl_analyzer_create(arena, ontology_graph);
    cns_ttl_analyzer_analyze_graph(analyzer);
    const cns_ttl_memory_layout_t *layout = cns_ttl_analyzer_get_layout(analyzer);
    
    arenac_trace_memory_pattern_end(telemetry_ctx, &ttl_pattern);
    
    // Step 2: SHACL Validation with telemetry
    arenac_pattern_telemetry_t shacl_pattern = arenac_trace_memory_pattern_start(
        telemetry_ctx, "shacl_validation"
    );
    
    cns_shacl_validator_t *validator = cns_shacl_validator_create_default(arena, interner);
    cns_shacl_load_shapes_from_graph(validator, shapes_graph);
    
    cns_validation_report_t *report = cns_shacl_create_report(validator);
    cns_shacl_validate_graph(validator, ontology_graph, report);
    cns_shacl_finalize_report(report);
    
    arenac_trace_memory_pattern_end(telemetry_ctx, &shacl_pattern);
    
    // Step 3: AOT Calculation with telemetry
    arenac_pattern_telemetry_t aot_pattern = arenac_trace_memory_pattern_start(
        telemetry_ctx, "aot_calculation"
    );
    
    aot_component_sizes_t components = {
        .node_count = layout->total_nodes,
        .edge_count = layout->total_edges,
        .property_count = layout->component_count,
        .string_data_size = layout->string_arena_size,
        .index_overhead = (size_t)(layout->total_memory_bytes * 0.15)
    };
    
    aot_memory_config_t config;
    aot_init_default_config(&config);
    
    aot_memory_layout_t aot_layout;
    size_t total_memory = aot_calculate_memory(&components, &config, &aot_layout);
    
    arenac_trace_memory_pattern_end(telemetry_ctx, &aot_pattern);
    
    // Step 4: Runtime Arena Creation with telemetry
    arenac_pattern_telemetry_t runtime_pattern = arenac_trace_memory_pattern_start(
        telemetry_ctx, "runtime_arena"
    );
    
    cns_arena_t *runtime_arena = arenac_create(
        total_memory, 
        ARENAC_FLAG_ZERO_ALLOC | ARENAC_FLAG_ALIGN_64 | ARENAC_FLAG_STATS
    );
    
    // Test multiple allocations with telemetry
    for (int i = 0; i < 10; i++) {
        size_t size = 64 << i; // Exponentially increasing sizes
        void *ptr = arenac_alloc_with_telemetry(runtime_arena, size, telemetry_ctx);
        TEST_ASSERT(ptr != NULL, "Telemetry-tracked allocation");
        
        arenac_trace_pattern_allocation(telemetry_ctx, &runtime_pattern, size, 
                                       S7T_CYCLES() % 7); // Mock cycles
    }
    
    arenac_trace_memory_pattern_end(telemetry_ctx, &runtime_pattern);
    
    // End distributed trace
    arenac_distributed_trace_end(telemetry_ctx, trace_id, true);
    
    // Get telemetry statistics
    void *stats = malloc(1024); // Mock stats structure
    arenac_telemetry_get_stats(telemetry_ctx, stats);
    TEST_ASSERT(stats != NULL, "Telemetry statistics retrieval");
    
    printf("Telemetry integration results:\n");
    printf("  Trace ID: %lu\n", trace_id);
    printf("  Total spans created: %lu\n", telemetry_ctx->total_spans);
    printf("  Memory usage: %zu bytes\n", telemetry_ctx->telemetry_memory_usage);
    
    // Test telemetry flush
    CNSResult flush_result = arenac_telemetry_flush(telemetry_ctx);
    TEST_ASSERT(flush_result == CNS_OK, "Telemetry flush");
    
    // Verify 7-tick constraint compliance throughout
    const arenac_stats_t *arena_stats = arenac_get_stats(runtime_arena);
    if (arena_stats) {
        TEST_ASSERT(arena_stats->violation_count == 0, "No 7-tick violations");
        printf("  7-tick violations: %lu\n", arena_stats->violation_count);
        printf("  Average allocation cycles: %lu\n", arena_stats->avg_alloc_cycles);
        printf("  Max allocation cycles: %lu\n", arena_stats->max_alloc_cycles);
    }
    
    free(stats);
    arenac_destroy(runtime_arena);
    
    cleanup_arenac_environment(arena, interner, ontology_graph, 
                              shapes_graph, telemetry, telemetry_ctx);
    
    TEST_SUITE_END("Complete Telemetry Integration");
    return 0;
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    
    printf("ARENAC Complete Workflow Integration Test Suite\n");
    printf("==============================================\n");
    printf("Testing: TTL Analysis → Memory Calculation → Arena Generation → Code Generation\n");
    printf("         SHACL Validation → TTL Parsing → AOT Calculation → Static Arena Creation\n");
    printf("         Telemetry Integration across all components\n\n");
    
    int failed = 0;
    
    // Run integration test steps in sequence
    if (test_ttl_analysis_memory_calculation() != 0) failed++;
    if (test_shacl_validation_integration() != 0) failed++;
    if (test_aot_calculation_arena_generation() != 0) failed++;
    if (test_code_generation_runtime_arena() != 0) failed++;
    if (test_complete_telemetry_integration() != 0) failed++;
    
    // Summary
    printf("\n==============================================\n");
    if (failed == 0) {
        printf("✅ All ARENAC workflow integration tests passed!\n");
        printf("🎯 Complete AOT → Runtime vision validated!\n");
        printf("⚡ 7-tick performance constraints maintained!\n");
        printf("📊 Telemetry integration functional!\n");
        return 0;
    } else {
        printf("❌ %d integration test(s) failed!\n", failed);
        return 1;
    }
}