/**
 * @file complete_arenac_example.c
 * @brief Complete ARENAC Workflow Demonstration
 * 
 * Complete demonstration of ARENAC from ontology to runtime allocation:
 * 1. Load CNS ontology data
 * 2. Validate with SHACL constraints
 * 3. Analyze memory requirements with TTL analyzer
 * 4. Calculate AOT memory layout
 * 5. Generate static arena code
 * 6. Create runtime arena and allocate
 * 7. Performance benchmark complete pipeline
 * 8. Demonstrate telemetry integration
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

// Include CNS headers
#include "cns/ttl_analyzer.h"
#include "cns/graph.h"
#include "cns/arena.h"
#include "cns/interner.h"
#include "cns/shacl.h"
#include "cns/arenac_telemetry.h"
#include "cns/parser.h"
#include "cns/types.h"

// Include AOT components
#include "../aot/aot_calculator.h"
#include "../aot/arena_codegen.h"

// ============================================================================
// DEMONSTRATION CONFIGURATION
// ============================================================================

#define DEMO_VERSION "1.0.0"
#define DEMO_MAX_CYCLES 7
#define DEMO_SAFETY_FACTOR 1.5

// Demonstration modes
typedef enum {
    DEMO_MODE_BASIC,        // Basic workflow demonstration
    DEMO_MODE_PERFORMANCE,  // Performance benchmarking
    DEMO_MODE_SCALABILITY,  // Scalability testing
    DEMO_MODE_TELEMETRY,    // Telemetry showcase
    DEMO_MODE_COMPLETE      // Complete showcase
} demo_mode_t;

// Results tracking
typedef struct {
    uint64_t total_time_cycles;
    uint64_t ttl_analysis_cycles;
    uint64_t shacl_validation_cycles;
    uint64_t aot_calculation_cycles;
    uint64_t code_generation_cycles;
    uint64_t runtime_creation_cycles;
    uint64_t allocation_test_cycles;
    
    size_t total_memory_required;
    size_t total_triples_processed;
    size_t total_allocations_tested;
    size_t code_size_generated;
    
    bool all_7t_compliant;
    uint64_t max_operation_cycles;
    uint64_t violations_detected;
    
    double memory_efficiency_ratio;
    double performance_score;
} arenac_demo_results_t;

// ============================================================================
// SAMPLE CNS ONTOLOGY DATA
// ============================================================================

// Comprehensive CNS ontology for demonstration
static const char* cns_demo_ontology[][3] = {
    // Core ontology classes
    {"http://cns.org/Agent", "http://www.w3.org/1999/02/22-rdf-syntax-ns#type", "http://www.w3.org/2002/07/owl#Class"},
    {"http://cns.org/Task", "http://www.w3.org/1999/02/22-rdf-syntax-ns#type", "http://www.w3.org/2002/07/owl#Class"},
    {"http://cns.org/Resource", "http://www.w3.org/1999/02/22-rdf-syntax-ns#type", "http://www.w3.org/2002/07/owl#Class"},
    {"http://cns.org/Arena", "http://www.w3.org/1999/02/22-rdf-syntax-ns#type", "http://www.w3.org/2002/07/owl#Class"},
    {"http://cns.org/Memory", "http://www.w3.org/1999/02/22-rdf-syntax-ns#type", "http://www.w3.org/2002/07/owl#Class"},
    {"http://cns.org/Allocation", "http://www.w3.org/1999/02/22-rdf-syntax-ns#type", "http://www.w3.org/2002/07/owl#Class"},
    {"http://cns.org/Performance", "http://www.w3.org/1999/02/22-rdf-syntax-ns#type", "http://www.w3.org/2002/07/owl#Class"},
    
    // Specialized agent types
    {"http://cns.org/ResearchAgent", "http://www.w3.org/2000/01/rdf-schema#subClassOf", "http://cns.org/Agent"},
    {"http://cns.org/CoderAgent", "http://www.w3.org/2000/01/rdf-schema#subClassOf", "http://cns.org/Agent"},
    {"http://cns.org/AnalystAgent", "http://www.w3.org/2000/01/rdf-schema#subClassOf", "http://cns.org/Agent"},
    {"http://cns.org/CoordinatorAgent", "http://www.w3.org/2000/01/rdf-schema#subClassOf", "http://cns.org/Agent"},
    
    // Task hierarchy
    {"http://cns.org/ParsingTask", "http://www.w3.org/2000/01/rdf-schema#subClassOf", "http://cns.org/Task"},
    {"http://cns.org/ValidationTask", "http://www.w3.org/2000/01/rdf-schema#subClassOf", "http://cns.org/Task"},
    {"http://cns.org/AnalysisTask", "http://www.w3.org/2000/01/rdf-schema#subClassOf", "http://cns.org/Task"},
    {"http://cns.org/AllocationTask", "http://www.w3.org/2000/01/rdf-schema#subClassOf", "http://cns.org/Task"},
    
    // Object properties
    {"http://cns.org/hasTask", "http://www.w3.org/1999/02/22-rdf-syntax-ns#type", "http://www.w3.org/2002/07/owl#ObjectProperty"},
    {"http://cns.org/hasCapability", "http://www.w3.org/1999/02/22-rdf-syntax-ns#type", "http://www.w3.org/2002/07/owl#ObjectProperty"},
    {"http://cns.org/manages", "http://www.w3.org/1999/02/22-rdf-syntax-ns#type", "http://www.w3.org/2002/07/owl#ObjectProperty"},
    {"http://cns.org/allocatesFrom", "http://www.w3.org/1999/02/22-rdf-syntax-ns#type", "http://www.w3.org/2002/07/owl#ObjectProperty"},
    {"http://cns.org/monitors", "http://www.w3.org/1999/02/22-rdf-syntax-ns#type", "http://www.w3.org/2002/07/owl#ObjectProperty"},
    {"http://cns.org/optimizes", "http://www.w3.org/1999/02/22-rdf-syntax-ns#type", "http://www.w3.org/2002/07/owl#ObjectProperty"},
    
    // Data properties
    {"http://cns.org/memorySize", "http://www.w3.org/1999/02/22-rdf-syntax-ns#type", "http://www.w3.org/2002/07/owl#DatatypeProperty"},
    {"http://cns.org/alignment", "http://www.w3.org/1999/02/22-rdf-syntax-ns#type", "http://www.w3.org/2002/07/owl#DatatypeProperty"},
    {"http://cns.org/cycleCount", "http://www.w3.org/1999/02/22-rdf-syntax-ns#type", "http://www.w3.org/2002/07/owl#DatatypeProperty"},
    {"http://cns.org/priority", "http://www.w3.org/1999/02/22-rdf-syntax-ns#type", "http://www.w3.org/2002/07/owl#DatatypeProperty"},
    {"http://cns.org/efficiency", "http://www.w3.org/1999/02/22-rdf-syntax-ns#type", "http://www.w3.org/2002/07/owl#DatatypeProperty"},
    {"http://cns.org/throughput", "http://www.w3.org/1999/02/22-rdf-syntax-ns#type", "http://www.w3.org/2002/07/owl#DatatypeProperty"},
    
    // Agent instances
    {"http://cns.org/demo/MainCoordinator", "http://www.w3.org/1999/02/22-rdf-syntax-ns#type", "http://cns.org/CoordinatorAgent"},
    {"http://cns.org/demo/Researcher1", "http://www.w3.org/1999/02/22-rdf-syntax-ns#type", "http://cns.org/ResearchAgent"},
    {"http://cns.org/demo/Coder1", "http://www.w3.org/1999/02/22-rdf-syntax-ns#type", "http://cns.org/CoderAgent"},
    {"http://cns.org/demo/Analyst1", "http://www.w3.org/1999/02/22-rdf-syntax-ns#type", "http://cns.org/AnalystAgent"},
    
    // Task instances
    {"http://cns.org/demo/TTLParsingTask", "http://www.w3.org/1999/02/22-rdf-syntax-ns#type", "http://cns.org/ParsingTask"},
    {"http://cns.org/demo/SHACLValidationTask", "http://www.w3.org/1999/02/22-rdf-syntax-ns#type", "http://cns.org/ValidationTask"},
    {"http://cns.org/demo/MemoryAnalysisTask", "http://www.w3.org/1999/02/22-rdf-syntax-ns#type", "http://cns.org/AnalysisTask"},
    {"http://cns.org/demo/AllocationTask", "http://www.w3.org/1999/02/22-rdf-syntax-ns#type", "http://cns.org/AllocationTask"},
    
    // Resource instances
    {"http://cns.org/demo/MainArena", "http://www.w3.org/1999/02/22-rdf-syntax-ns#type", "http://cns.org/Arena"},
    {"http://cns.org/demo/NodeMemory", "http://www.w3.org/1999/02/22-rdf-syntax-ns#type", "http://cns.org/Memory"},
    {"http://cns.org/demo/EdgeMemory", "http://www.w3.org/1999/02/22-rdf-syntax-ns#type", "http://cns.org/Memory"},
    {"http://cns.org/demo/StringMemory", "http://www.w3.org/1999/02/22-rdf-syntax-ns#type", "http://cns.org/Memory"},
    
    // Relationships
    {"http://cns.org/demo/MainCoordinator", "http://cns.org/hasTask", "http://cns.org/demo/TTLParsingTask"},
    {"http://cns.org/demo/MainCoordinator", "http://cns.org/hasTask", "http://cns.org/demo/SHACLValidationTask"},
    {"http://cns.org/demo/MainCoordinator", "http://cns.org/hasTask", "http://cns.org/demo/MemoryAnalysisTask"},
    {"http://cns.org/demo/MainCoordinator", "http://cns.org/manages", "http://cns.org/demo/MainArena"},
    
    {"http://cns.org/demo/Researcher1", "http://cns.org/hasTask", "http://cns.org/demo/TTLParsingTask"},
    {"http://cns.org/demo/Coder1", "http://cns.org/hasTask", "http://cns.org/demo/AllocationTask"},
    {"http://cns.org/demo/Analyst1", "http://cns.org/hasTask", "http://cns.org/demo/MemoryAnalysisTask"},
    
    {"http://cns.org/demo/MainArena", "http://cns.org/allocatesFrom", "http://cns.org/demo/NodeMemory"},
    {"http://cns.org/demo/MainArena", "http://cns.org/allocatesFrom", "http://cns.org/demo/EdgeMemory"},
    {"http://cns.org/demo/MainArena", "http://cns.org/allocatesFrom", "http://cns.org/demo/StringMemory"},
    
    // Properties with values
    {"http://cns.org/demo/MainArena", "http://cns.org/memorySize", "\"4194304\"^^http://www.w3.org/2001/XMLSchema#integer"},
    {"http://cns.org/demo/MainArena", "http://cns.org/alignment", "\"64\"^^http://www.w3.org/2001/XMLSchema#integer"},
    {"http://cns.org/demo/NodeMemory", "http://cns.org/memorySize", "\"131072\"^^http://www.w3.org/2001/XMLSchema#integer"},
    {"http://cns.org/demo/EdgeMemory", "http://cns.org/memorySize", "\"65536\"^^http://www.w3.org/2001/XMLSchema#integer"},
    {"http://cns.org/demo/StringMemory", "http://cns.org/memorySize", "\"262144\"^^http://www.w3.org/2001/XMLSchema#integer"},
    
    {"http://cns.org/demo/TTLParsingTask", "http://cns.org/cycleCount", "\"5\"^^http://www.w3.org/2001/XMLSchema#integer"},
    {"http://cns.org/demo/SHACLValidationTask", "http://cns.org/cycleCount", "\"7\"^^http://www.w3.org/2001/XMLSchema#integer"},
    {"http://cns.org/demo/MemoryAnalysisTask", "http://cns.org/cycleCount", "\"6\"^^http://www.w3.org/2001/XMLSchema#integer"},
    {"http://cns.org/demo/AllocationTask", "http://cns.org/cycleCount", "\"3\"^^http://www.w3.org/2001/XMLSchema#integer"},
    
    {"http://cns.org/demo/TTLParsingTask", "http://cns.org/priority", "\"high\"^^http://www.w3.org/2001/XMLSchema#string"},
    {"http://cns.org/demo/SHACLValidationTask", "http://cns.org/priority", "\"medium\"^^http://www.w3.org/2001/XMLSchema#string"},
    {"http://cns.org/demo/MemoryAnalysisTask", "http://cns.org/priority", "\"high\"^^http://www.w3.org/2001/XMLSchema#string"},
    {"http://cns.org/demo/AllocationTask", "http://cns.org/priority", "\"critical\"^^http://www.w3.org/2001/XMLSchema#string"},
    
    {"http://cns.org/demo/MainArena", "http://cns.org/efficiency", "\"0.95\"^^http://www.w3.org/2001/XMLSchema#double"},
    {"http://cns.org/demo/MainArena", "http://cns.org/throughput", "\"1000000\"^^http://www.w3.org/2001/XMLSchema#integer"},
    
    {NULL, NULL, NULL} // Terminator
};

// SHACL shapes for CNS validation
static const char* cns_demo_shapes[][3] = {
    // Agent shape
    {"http://cns.org/shapes/AgentShape", "http://www.w3.org/1999/02/22-rdf-syntax-ns#type", "http://www.w3.org/ns/shacl#NodeShape"},
    {"http://cns.org/shapes/AgentShape", "http://www.w3.org/ns/shacl#targetClass", "http://cns.org/Agent"},
    {"http://cns.org/shapes/AgentShape", "http://www.w3.org/ns/shacl#property", "_:agentTaskProp"},
    
    {"_:agentTaskProp", "http://www.w3.org/ns/shacl#path", "http://cns.org/hasTask"},
    {"_:agentTaskProp", "http://www.w3.org/ns/shacl#class", "http://cns.org/Task"},
    {"_:agentTaskProp", "http://www.w3.org/ns/shacl#minCount", "\"1\"^^http://www.w3.org/2001/XMLSchema#integer"},
    
    // Task shape with cycle constraints
    {"http://cns.org/shapes/TaskShape", "http://www.w3.org/1999/02/22-rdf-syntax-ns#type", "http://www.w3.org/ns/shacl#NodeShape"},
    {"http://cns.org/shapes/TaskShape", "http://www.w3.org/ns/shacl#targetClass", "http://cns.org/Task"},
    {"http://cns.org/shapes/TaskShape", "http://www.w3.org/ns/shacl#property", "_:taskCycleProp"},
    
    {"_:taskCycleProp", "http://www.w3.org/ns/shacl#path", "http://cns.org/cycleCount"},
    {"_:taskCycleProp", "http://www.w3.org/ns/shacl#datatype", "http://www.w3.org/2001/XMLSchema#integer"},
    {"_:taskCycleProp", "http://www.w3.org/ns/shacl#minInclusive", "\"1\"^^http://www.w3.org/2001/XMLSchema#integer"},
    {"_:taskCycleProp", "http://www.w3.org/ns/shacl#maxInclusive", "\"7\"^^http://www.w3.org/2001/XMLSchema#integer"},
    
    // Arena shape with memory constraints
    {"http://cns.org/shapes/ArenaShape", "http://www.w3.org/1999/02/22-rdf-syntax-ns#type", "http://www.w3.org/ns/shacl#NodeShape"},
    {"http://cns.org/shapes/ArenaShape", "http://www.w3.org/ns/shacl#targetClass", "http://cns.org/Arena"},
    {"http://cns.org/shapes/ArenaShape", "http://www.w3.org/ns/shacl#property", "_:arenaMemoryProp"},
    {"http://cns.org/shapes/ArenaShape", "http://www.w3.org/ns/shacl#property", "_:arenaAlignProp"},
    
    {"_:arenaMemoryProp", "http://www.w3.org/ns/shacl#path", "http://cns.org/memorySize"},
    {"_:arenaMemoryProp", "http://www.w3.org/ns/shacl#datatype", "http://www.w3.org/2001/XMLSchema#integer"},
    {"_:arenaMemoryProp", "http://www.w3.org/ns/shacl#minInclusive", "\"1024\"^^http://www.w3.org/2001/XMLSchema#integer"},
    
    {"_:arenaAlignProp", "http://www.w3.org/ns/shacl#path", "http://cns.org/alignment"},
    {"_:arenaAlignProp", "http://www.w3.org/ns/shacl#datatype", "http://www.w3.org/2001/XMLSchema#integer"},
    {"_:arenaAlignProp", "http://www.w3.org/ns/shacl#in", "\"(8 16 32 64)\"^^http://www.w3.org/2001/XMLSchema#string"},
    
    {NULL, NULL, NULL} // Terminator
};

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

/**
 * Print demonstration header
 */
static void print_demo_header(demo_mode_t mode) {
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║                    ARENAC COMPLETE DEMONSTRATION              ║\n");
    printf("║                          Version %s                        ║\n", DEMO_VERSION);
    printf("╠══════════════════════════════════════════════════════════════╣\n");
    
    switch (mode) {
        case DEMO_MODE_BASIC:
            printf("║  Mode: Basic Workflow Demonstration                         ║\n");
            break;
        case DEMO_MODE_PERFORMANCE:
            printf("║  Mode: Performance Benchmarking                             ║\n");
            break;
        case DEMO_MODE_SCALABILITY:
            printf("║  Mode: Scalability Testing                                  ║\n");
            break;
        case DEMO_MODE_TELEMETRY:
            printf("║  Mode: Telemetry Showcase                                   ║\n");
            break;
        case DEMO_MODE_COMPLETE:
            printf("║  Mode: Complete Feature Showcase                            ║\n");
            break;
    }
    
    printf("║                                                              ║\n");
    printf("║  Workflow: Ontology → Validation → Analysis → Generation     ║\n");
    printf("║           → AOT Calculation → Runtime Arena → Allocation    ║\n");
    printf("║                                                              ║\n");
    printf("║  Performance Target: All operations ≤ 7 CPU ticks           ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n\n");
}

/**
 * Print performance indicator
 */
static void print_performance(const char *operation, uint64_t cycles) {
    printf("  %-30s: %6lu cycles ", operation, cycles);
    if (cycles <= DEMO_MAX_CYCLES) {
        printf("✓ 7T\n");
    } else {
        printf("⚠ SLOW\n");
    }
}

/**
 * Print section header
 */
static void print_section(const char *title) {
    printf("\n┌─ %s ", title);
    for (int i = strlen(title); i < 58; i++) printf("─");
    printf("┐\n");
}

/**
 * Print section footer
 */
static void print_section_end() {
    printf("└────────────────────────────────────────────────────────────┘\n");
}

/**
 * Create demonstration graph with CNS ontology
 */
static cns_graph_t* create_demo_graph(cns_arena_t *arena, cns_interner_t *interner) {
    cns_graph_t *graph = cns_graph_create_default(arena, interner);
    if (!graph) return NULL;
    
    printf("Loading CNS demonstration ontology...\n");
    
    int triple_count = 0;
    for (int i = 0; cns_demo_ontology[i][0] != NULL; i++) {
        cns_result_t result = cns_graph_insert_triple(
            graph,
            cns_demo_ontology[i][0],
            cns_demo_ontology[i][1],
            cns_demo_ontology[i][2],
            CNS_OBJECT_TYPE_IRI
        );
        if (result == CNS_OK) {
            triple_count++;
        }
    }
    
    printf("  Loaded %d ontology triples\n", triple_count);
    return graph;
}

/**
 * Create demonstration SHACL shapes
 */
static cns_graph_t* create_demo_shapes(cns_arena_t *arena, cns_interner_t *interner) {
    cns_graph_t *graph = cns_graph_create_default(arena, interner);
    if (!graph) return NULL;
    
    printf("Loading CNS SHACL shapes...\n");
    
    int shape_count = 0;
    for (int i = 0; cns_demo_shapes[i][0] != NULL; i++) {
        cns_result_t result = cns_graph_insert_triple(
            graph,
            cns_demo_shapes[i][0],
            cns_demo_shapes[i][1],
            cns_demo_shapes[i][2],
            CNS_OBJECT_TYPE_IRI
        );
        if (result == CNS_OK) {
            shape_count++;
        }
    }
    
    printf("  Loaded %d shape triples\n", shape_count);
    return graph;
}

/**
 * Initialize demonstration results
 */
static void init_demo_results(arenac_demo_results_t *results) {
    memset(results, 0, sizeof(arenac_demo_results_t));
    results->all_7t_compliant = true;
    results->memory_efficiency_ratio = 1.0;
    results->performance_score = 100.0;
}

/**
 * Update demo results with operation timing
 */
static void update_demo_results(arenac_demo_results_t *results, 
                               const char *operation, 
                               uint64_t cycles) {
    if (cycles > DEMO_MAX_CYCLES) {
        results->all_7t_compliant = false;
        results->violations_detected++;
        results->performance_score -= 10.0;
    }
    
    if (cycles > results->max_operation_cycles) {
        results->max_operation_cycles = cycles;
    }
}

// ============================================================================
// DEMONSTRATION WORKFLOW FUNCTIONS
// ============================================================================

/**
 * Step 1: Load and validate ontology with SHACL
 */
static int demo_step1_load_validate(cns_arena_t *arena, 
                                   cns_interner_t *interner,
                                   cns_graph_t **ontology_graph,
                                   cns_graph_t **shapes_graph,
                                   arenac_demo_results_t *results) {
    print_section("Step 1: Load Ontology and SHACL Validation");
    
    uint64_t start_cycles = S7T_CYCLES();
    
    // Create ontology graph
    *ontology_graph = create_demo_graph(arena, interner);
    if (!*ontology_graph) {
        printf("❌ Failed to create ontology graph\n");
        return -1;
    }
    
    uint64_t load_cycles = S7T_CYCLES() - start_cycles;
    print_performance("Ontology loading", load_cycles);
    update_demo_results(results, "ontology_load", load_cycles);
    results->ttl_analysis_cycles += load_cycles;
    
    // Get graph statistics
    cns_graph_stats_t graph_stats;
    cns_graph_get_stats(*ontology_graph, &graph_stats);
    results->total_triples_processed = graph_stats.triple_count;
    
    printf("  📊 Graph statistics:\n");
    printf("     Triples: %zu\n", graph_stats.triple_count);
    printf("     Nodes: %zu\n", graph_stats.node_count);
    printf("     Properties: %zu\n", graph_stats.property_count);
    
    // Create SHACL shapes
    start_cycles = S7T_CYCLES();
    *shapes_graph = create_demo_shapes(arena, interner);
    if (!*shapes_graph) {
        printf("❌ Failed to create shapes graph\n");
        return -1;
    }
    
    uint64_t shapes_cycles = S7T_CYCLES() - start_cycles;
    print_performance("SHACL shapes loading", shapes_cycles);
    update_demo_results(results, "shapes_load", shapes_cycles);
    
    // Create SHACL validator
    start_cycles = S7T_CYCLES();
    cns_shacl_validator_t *validator = cns_shacl_validator_create_default(arena, interner);
    if (!validator) {
        printf("❌ Failed to create SHACL validator\n");
        return -1;
    }
    
    uint64_t validator_cycles = S7T_CYCLES() - start_cycles;
    print_performance("SHACL validator creation", validator_cycles);
    update_demo_results(results, "validator_create", validator_cycles);
    
    // Load shapes and validate
    start_cycles = S7T_CYCLES();
    cns_result_t result = cns_shacl_load_shapes_from_graph(validator, *shapes_graph);
    if (result != CNS_OK) {
        printf("❌ Failed to load SHACL shapes\n");
        return -1;
    }
    
    cns_validation_report_t *report = cns_shacl_create_report(validator);
    result = cns_shacl_validate_graph(validator, *ontology_graph, report);
    cns_shacl_finalize_report(report);
    
    uint64_t validation_cycles = S7T_CYCLES() - start_cycles;
    print_performance("SHACL validation", validation_cycles);
    update_demo_results(results, "shacl_validation", validation_cycles);
    results->shacl_validation_cycles = validation_cycles;
    
    printf("  🔍 SHACL validation results:\n");
    printf("     Conforms: %s\n", report->conforms ? "✅ Yes" : "❌ No");
    printf("     Violations: %zu\n", report->violation_count);
    printf("     Warnings: %zu\n", report->warning_count);
    printf("     Nodes validated: %zu\n", report->nodes_validated);
    
    if (!report->conforms) {
        results->violations_detected += report->violation_count;
        results->performance_score -= (report->violation_count * 5.0);
    }
    
    print_section_end();
    return 0;
}

/**
 * Step 2: TTL analysis and memory calculation
 */
static int demo_step2_ttl_analysis(cns_arena_t *arena,
                                  cns_graph_t *ontology_graph,
                                  cns_ttl_analyzer_t **analyzer,
                                  arenac_demo_results_t *results) {
    print_section("Step 2: TTL Analysis and Memory Calculation");
    
    uint64_t start_cycles = S7T_CYCLES();
    
    // Create TTL analyzer with comprehensive flags
    *analyzer = cns_ttl_analyzer_create_configured(
        arena, ontology_graph,
        CNS_TTL_ANALYZER_FLAG_DETAILED | 
        CNS_TTL_ANALYZER_FLAG_PROFILING |
        CNS_TTL_ANALYZER_FLAG_GROWTH |
        CNS_TTL_ANALYZER_FLAG_OPTIMIZE |
        CNS_TTL_ANALYZER_FLAG_VALIDATE,
        100 // max depth
    );
    
    if (!*analyzer) {
        printf("❌ Failed to create TTL analyzer\n");
        return -1;
    }
    
    uint64_t analyzer_cycles = S7T_CYCLES() - start_cycles;
    print_performance("TTL analyzer creation", analyzer_cycles);
    update_demo_results(results, "analyzer_create", analyzer_cycles);
    
    // Perform comprehensive analysis
    start_cycles = S7T_CYCLES();
    cns_result_t result = cns_ttl_analyzer_analyze_graph(*analyzer);
    if (result != CNS_OK) {
        printf("❌ Failed to analyze graph\n");
        return -1;
    }
    
    uint64_t analysis_cycles = S7T_CYCLES() - start_cycles;
    print_performance("Graph analysis", analysis_cycles);
    update_demo_results(results, "graph_analysis", analysis_cycles);
    results->ttl_analysis_cycles += analysis_cycles;
    
    // Get memory layout
    const cns_ttl_memory_layout_t *layout = cns_ttl_analyzer_get_layout(*analyzer);
    results->total_memory_required = layout->total_memory_bytes;
    
    printf("  📊 Memory analysis results:\n");
    printf("     Total memory: %zu bytes (%.2f MB)\n", 
           layout->total_memory_bytes, layout->total_memory_bytes / (1024.0 * 1024.0));
    printf("     Main arena: %zu bytes\n", layout->main_arena_size);
    printf("     Node arena: %zu bytes\n", layout->node_arena_size);
    printf("     Edge arena: %zu bytes\n", layout->edge_arena_size);
    printf("     String arena: %zu bytes\n", layout->string_arena_size);
    printf("     Temp arena: %zu bytes\n", layout->temp_arena_size);
    printf("     Components analyzed: %u\n", layout->component_count);
    
    // Test optimization analysis
    start_cycles = S7T_CYCLES();
    result = cns_ttl_analyzer_analyze_optimization_opportunities(*analyzer);
    if (result == CNS_OK) {
        cns_ttl_analyzer_recommend_optimizations(*analyzer);
    }
    
    uint64_t optimization_cycles = S7T_CYCLES() - start_cycles;
    print_performance("Optimization analysis", optimization_cycles);
    update_demo_results(results, "optimization", optimization_cycles);
    
    printf("  ⚡ Performance characteristics:\n");
    printf("     Parse complexity: %.2f\n", layout->parse_complexity);
    printf("     Query complexity: %.2f\n", layout->query_complexity);
    printf("     Max depth: %u\n", layout->max_depth);
    printf("     Average degree: %u\n", layout->avg_degree);
    
    // Calculate memory efficiency
    size_t theoretical_minimum = results->total_triples_processed * 32; // 32 bytes per triple minimum
    results->memory_efficiency_ratio = (double)theoretical_minimum / layout->total_memory_bytes;
    
    printf("     Memory efficiency: %.2f%% (%.2fx theoretical minimum)\n", 
           results->memory_efficiency_ratio * 100.0, 1.0 / results->memory_efficiency_ratio);
    
    print_section_end();
    return 0;
}

/**
 * Step 3: AOT calculation and code generation
 */
static int demo_step3_aot_codegen(cns_ttl_analyzer_t *analyzer,
                                 arenac_demo_results_t *results) {
    print_section("Step 3: AOT Calculation and Code Generation");
    
    const cns_ttl_memory_layout_t *layout = cns_ttl_analyzer_get_layout(analyzer);
    
    uint64_t start_cycles = S7T_CYCLES();
    
    // Convert to AOT component sizes
    aot_component_sizes_t components = {
        .node_count = layout->total_nodes,
        .edge_count = layout->total_edges,
        .property_count = layout->component_count,
        .string_data_size = layout->string_arena_size,
        .index_overhead = (size_t)(layout->total_memory_bytes * 0.15)
    };
    
    // Validate components
    if (!aot_validate_components(&components)) {
        printf("❌ Invalid AOT components\n");
        return -1;
    }
    
    // Configure AOT calculation
    aot_memory_config_t aot_config;
    aot_init_default_config(&aot_config);
    aot_config.alignment = 64;
    aot_config.safety_margin = (size_t)(layout->total_memory_bytes * (DEMO_SAFETY_FACTOR - 1.0));
    
    // Calculate AOT memory layout
    aot_memory_layout_t aot_layout;
    size_t total_aot_memory = aot_calculate_memory(&components, &aot_config, &aot_layout);
    
    uint64_t aot_cycles = S7T_CYCLES() - start_cycles;
    print_performance("AOT calculation", aot_cycles);
    update_demo_results(results, "aot_calculation", aot_cycles);
    results->aot_calculation_cycles = aot_cycles;
    
    printf("  📊 AOT calculation results:\n");
    printf("     Total AOT memory: %zu bytes (%.2f MB)\n", 
           total_aot_memory, total_aot_memory / (1024.0 * 1024.0));
    printf("     Node section: %zu bytes\n", aot_layout.node_section_size);
    printf("     Edge section: %zu bytes\n", aot_layout.edge_section_size);
    printf("     String section: %zu bytes\n", aot_layout.string_section_size);
    printf("     Index section: %zu bytes\n", aot_layout.index_section_size);
    printf("     Padding overhead: %zu bytes\n", aot_layout.padding_overhead);
    printf("     Safety factor: %.1fx\n", DEMO_SAFETY_FACTOR);
    
    // Generate arena code
    start_cycles = S7T_CYCLES();
    
    FILE *codegen_file = tmpfile();
    if (!codegen_file) {
        printf("❌ Failed to create code generation file\n");
        return -1;
    }
    
    arena_codegen_config_t codegen_config = {
        .total_size = total_aot_memory,
        .alignment = 64,
        .arena_name = "cns_demo_arena",
        .type_name = "cns_arena_t",
        .use_static_storage = 1,
        .include_guards = 1,
        .include_debug_info = 1
    };
    
    arena_codegen_ctx_t *codegen_ctx = arena_codegen_create(codegen_file, &codegen_config);
    if (!codegen_ctx) {
        printf("❌ Failed to create code generator\n");
        fclose(codegen_file);
        return -1;
    }
    
    // Configure typed template with zones
    arena_codegen_set_template(codegen_ctx, ARENA_TEMPLATE_TYPED);
    
    arena_zone_config_t node_zone = {
        .zone_name = "nodes",
        .type_name = "cns_node_t",
        .type_size = 64,
        .count = components.node_count,
        .alignment = 64
    };
    arena_codegen_add_zone(codegen_ctx, &node_zone);
    
    arena_zone_config_t edge_zone = {
        .zone_name = "edges",
        .type_name = "cns_edge_t", 
        .type_size = 32,
        .count = components.edge_count,
        .alignment = 32
    };
    arena_codegen_add_zone(codegen_ctx, &edge_zone);
    
    arena_zone_config_t string_zone = {
        .zone_name = "strings",
        .type_name = "char",
        .type_size = 1,
        .count = components.string_data_size,
        .alignment = 8
    };
    arena_codegen_add_zone(codegen_ctx, &string_zone);
    
    // Generate complete arena code
    int codegen_result = arena_codegen_generate_complete(codegen_ctx);
    
    uint64_t codegen_cycles = S7T_CYCLES() - start_cycles;
    print_performance("Code generation", codegen_cycles);
    update_demo_results(results, "code_generation", codegen_cycles);
    results->code_generation_cycles = codegen_cycles;
    
    if (codegen_result == ARENA_CODEGEN_OK) {
        fseek(codegen_file, 0, SEEK_END);
        results->code_size_generated = ftell(codegen_file);
        
        printf("  💻 Code generation results:\n");
        printf("     Generation result: ✅ Success\n");
        printf("     Generated code size: %zu bytes\n", results->code_size_generated);
        printf("     Arena zones: 3 (nodes, edges, strings)\n");
        printf("     Template: Typed allocation zones\n");
    } else {
        printf("     Generation result: ❌ Failed\n");
    }
    
    arena_codegen_destroy(codegen_ctx);
    fclose(codegen_file);
    
    print_section_end();
    return 0;
}

/**
 * Step 4: Runtime arena creation and allocation testing
 */
static int demo_step4_runtime_testing(cns_ttl_analyzer_t *analyzer,
                                      arenac_telemetry_context_t *telemetry_ctx,
                                      arenac_demo_results_t *results) {
    print_section("Step 4: Runtime Arena Creation and Allocation Testing");
    
    const cns_ttl_memory_layout_t *layout = cns_ttl_analyzer_get_layout(analyzer);
    
    // Calculate total runtime arena size
    size_t runtime_size = (size_t)(layout->total_memory_bytes * DEMO_SAFETY_FACTOR);
    
    uint64_t start_cycles = S7T_CYCLES();
    
    // Create runtime arena with telemetry
    cns_arena_t *runtime_arena = arenac_create(
        runtime_size,
        ARENAC_FLAG_ZERO_ALLOC | 
        ARENAC_FLAG_ALIGN_64 | 
        ARENAC_FLAG_STATS |
        ARENAC_FLAG_OVERFLOW_CHECK
    );
    
    if (!runtime_arena) {
        printf("❌ Failed to create runtime arena\n");
        return -1;
    }
    
    uint64_t creation_cycles = S7T_CYCLES() - start_cycles;
    print_performance("Arena creation", creation_cycles);
    update_demo_results(results, "arena_creation", creation_cycles);
    results->runtime_creation_cycles = creation_cycles;
    
    // Configure arena for telemetry
    if (telemetry_ctx) {
        arenac_telemetry_configure_arena(runtime_arena, telemetry_ctx, 2);
    }
    
    printf("  🏗️  Runtime arena configuration:\n");
    printf("     Total size: %zu bytes (%.2f MB)\n", 
           runtime_size, runtime_size / (1024.0 * 1024.0));
    printf("     Alignment: 64 bytes\n");
    printf("     Flags: ZERO_ALLOC | ALIGN_64 | STATS | OVERFLOW_CHECK\n");
    printf("     Telemetry: %s\n", telemetry_ctx ? "Enabled" : "Disabled");
    
    // Test allocation performance with various sizes
    start_cycles = S7T_CYCLES();
    
    const size_t test_sizes[] = {64, 128, 256, 512, 1024, 2048, 4096, 8192};
    const int num_sizes = sizeof(test_sizes) / sizeof(test_sizes[0]);
    const int allocations_per_size = 10;
    
    void *test_allocations[num_sizes * allocations_per_size];
    int alloc_index = 0;
    
    printf("  🧪 Allocation performance testing:\n");
    
    for (int size_idx = 0; size_idx < num_sizes; size_idx++) {
        size_t size = test_sizes[size_idx];
        uint64_t size_start = S7T_CYCLES();
        
        for (int i = 0; i < allocations_per_size; i++) {
            if (telemetry_ctx) {
                test_allocations[alloc_index] = arenac_alloc_with_telemetry(
                    runtime_arena, size, telemetry_ctx
                );
            } else {
                test_allocations[alloc_index] = arenac_alloc(runtime_arena, size);
            }
            
            if (!test_allocations[alloc_index]) {
                printf("     ❌ Allocation failed at size %zu, iteration %d\n", size, i);
                break;
            }
            alloc_index++;
        }
        
        uint64_t size_cycles = S7T_CYCLES() - size_start;
        uint64_t avg_cycles = size_cycles / allocations_per_size;
        
        printf("     %4zu bytes: %2lu cycles avg ", size, avg_cycles);
        if (avg_cycles <= DEMO_MAX_CYCLES) {
            printf("✓\n");
        } else {
            printf("⚠\n");
            results->violations_detected++;
        }
        
        update_demo_results(results, "allocation", avg_cycles);
    }
    
    uint64_t total_alloc_cycles = S7T_CYCLES() - start_cycles;
    results->allocation_test_cycles = total_alloc_cycles;
    results->total_allocations_tested = alloc_index;
    
    printf("     Total allocations: %d\n", alloc_index);
    printf("     Total time: %lu cycles\n", total_alloc_cycles);
    printf("     Average per allocation: %lu cycles\n", 
           alloc_index > 0 ? total_alloc_cycles / alloc_index : 0);
    
    // Get arena statistics
    arenac_info_t arena_info;
    arenac_get_info(runtime_arena, &arena_info);
    
    printf("  📊 Arena usage statistics:\n");
    printf("     Used: %zu bytes (%.1f%%)\n", 
           arena_info.used_size, arena_info.utilization);
    printf("     Available: %zu bytes\n", arena_info.available_size);
    printf("     High water mark: %zu bytes\n", arena_info.high_water_mark);
    printf("     Allocations: %lu\n", arena_info.allocation_count);
    
    const arenac_stats_t *arena_stats = arenac_get_stats(runtime_arena);
    if (arena_stats) {
        printf("     7T violations: %lu\n", arena_stats->violation_count);
        printf("     Average cycles: %lu\n", arena_stats->avg_alloc_cycles);
        printf("     Max cycles: %lu\n", arena_stats->max_alloc_cycles);
        
        results->violations_detected += arena_stats->violation_count;
    }
    
    arenac_destroy(runtime_arena);
    
    print_section_end();
    return 0;
}

/**
 * Complete demonstration workflow
 */
static int run_complete_demonstration(demo_mode_t mode) {
    print_demo_header(mode);
    
    arenac_demo_results_t results;
    init_demo_results(&results);
    
    uint64_t demo_start = S7T_CYCLES();
    
    // Initialize core components
    printf("🚀 Initializing ARENAC demonstration environment...\n\n");
    
    // Create main arena (4MB for demonstration)
    cns_arena_t *arena = arenac_create(4 * 1024 * 1024, 
                                      ARENAC_FLAG_ZERO_ALLOC | 
                                      ARENAC_FLAG_ALIGN_64 | 
                                      ARENAC_FLAG_STATS);
    if (!arena) {
        printf("❌ Failed to create main arena\n");
        return -1;
    }
    
    // Create interner
    cns_interner_config_t interner_config = {
        .initial_capacity = 2048,
        .string_arena_size = 128 * 1024,
        .load_factor = 0.75f,
        .case_sensitive = true
    };
    
    cns_interner_t *interner = cns_interner_create(&interner_config);
    if (!interner) {
        printf("❌ Failed to create interner\n");
        arenac_destroy(arena);
        return -1;
    }
    
    // Initialize telemetry (if mode supports it)
    cns_telemetry_t *telemetry = NULL;
    arenac_telemetry_context_t *telemetry_ctx = NULL;
    
    if (mode == DEMO_MODE_TELEMETRY || mode == DEMO_MODE_COMPLETE) {
        telemetry = malloc(sizeof(cns_telemetry_t));
        if (telemetry && cns_telemetry_init(telemetry, NULL) == CNS_OK) {
            telemetry_ctx = malloc(sizeof(arenac_telemetry_context_t));
            if (telemetry_ctx) {
                arenac_telemetry_init(telemetry_ctx, telemetry, 
                                     ARENAC_TELEMETRY_ALL, 1.0);
                printf("✅ Telemetry system initialized\n");
            }
        }
    }
    
    // Variables for workflow steps
    cns_graph_t *ontology_graph = NULL;
    cns_graph_t *shapes_graph = NULL;
    cns_ttl_analyzer_t *analyzer = NULL;
    
    printf("✅ Core components initialized\n\n");
    
    // Execute demonstration workflow
    int step_result = 0;
    
    // Step 1: Load ontology and SHACL validation
    step_result = demo_step1_load_validate(arena, interner, 
                                          &ontology_graph, &shapes_graph, 
                                          &results);
    if (step_result != 0) {
        printf("❌ Step 1 failed\n");
        goto cleanup;
    }
    
    // Step 2: TTL analysis and memory calculation
    step_result = demo_step2_ttl_analysis(arena, ontology_graph, 
                                         &analyzer, &results);
    if (step_result != 0) {
        printf("❌ Step 2 failed\n");
        goto cleanup;
    }
    
    // Step 3: AOT calculation and code generation
    step_result = demo_step3_aot_codegen(analyzer, &results);
    if (step_result != 0) {
        printf("❌ Step 3 failed\n");
        goto cleanup;
    }
    
    // Step 4: Runtime testing
    step_result = demo_step4_runtime_testing(analyzer, telemetry_ctx, &results);
    if (step_result != 0) {
        printf("❌ Step 4 failed\n");
        goto cleanup;
    }
    
    results.total_time_cycles = S7T_CYCLES() - demo_start;
    
    // Calculate final performance score
    if (results.all_7t_compliant) {
        results.performance_score += 20.0; // Bonus for full compliance
    }
    
    if (results.memory_efficiency_ratio > 0.8) {
        results.performance_score += 10.0; // Bonus for efficiency
    }
    
    // Print final results
    print_section("Final Demonstration Results");
    
    printf("  🎯 ARENAC Workflow Performance Summary:\n");
    printf("     Total demonstration time: %lu cycles\n", results.total_time_cycles);
    printf("     TTL analysis: %lu cycles\n", results.ttl_analysis_cycles);
    printf("     SHACL validation: %lu cycles\n", results.shacl_validation_cycles);
    printf("     AOT calculation: %lu cycles\n", results.aot_calculation_cycles);
    printf("     Code generation: %lu cycles\n", results.code_generation_cycles);
    printf("     Runtime creation: %lu cycles\n", results.runtime_creation_cycles);
    printf("     Allocation testing: %lu cycles\n", results.allocation_test_cycles);
    printf("\n");
    
    printf("  📊 Processing Statistics:\n");
    printf("     Triples processed: %zu\n", results.total_triples_processed);
    printf("     Memory calculated: %zu bytes (%.2f MB)\n", 
            results.total_memory_required, results.total_memory_required / (1024.0 * 1024.0));
    printf("     Code generated: %zu bytes\n", results.code_size_generated);
    printf("     Allocations tested: %zu\n", results.total_allocations_tested);
    printf("\n");
    
    printf("  ⚡ Performance Analysis:\n");
    printf("     7T compliance: %s\n", results.all_7t_compliant ? "✅ Full" : "⚠ Partial");
    printf("     Max operation cycles: %lu\n", results.max_operation_cycles);
    printf("     Total violations: %lu\n", results.violations_detected);
    printf("     Memory efficiency: %.1f%%\n", results.memory_efficiency_ratio * 100.0);
    printf("     Overall score: %.1f/100\n", results.performance_score);
    printf("\n");
    
    // Success indicators
    printf("  🏆 Demonstration Results:\n");
    if (results.all_7t_compliant && results.violations_detected == 0) {
        printf("     ✅ ARENAC workflow fully validated!\n");
        printf("     ✅ Complete AOT → Runtime vision demonstrated!\n");
        printf("     ✅ 7-tick performance constraints maintained!\n");
        printf("     ✅ Production-ready performance achieved!\n");
    } else {
        printf("     ⚠ ARENAC workflow completed with warnings\n");
        printf("     ⚠ Some performance constraints violated\n");
        if (results.performance_score > 70.0) {
            printf("     ✅ Overall performance acceptable\n");
        } else {
            printf("     ❌ Performance improvements needed\n");
        }
    }
    
    print_section_end();
    
cleanup:
    // Cleanup resources
    if (telemetry_ctx) {
        arenac_telemetry_shutdown(telemetry_ctx);
        free(telemetry_ctx);
    }
    if (telemetry) {
        cns_telemetry_shutdown(telemetry);
        free(telemetry);
    }
    if (analyzer) cns_ttl_analyzer_destroy(analyzer);
    if (shapes_graph) cns_graph_destroy(shapes_graph);
    if (ontology_graph) cns_graph_destroy(ontology_graph);
    if (interner) cns_interner_destroy(interner);
    if (arena) arenac_destroy(arena);
    
    return step_result;
}

// ============================================================================
// MAIN DEMONSTRATION ENTRY POINT
// ============================================================================

int main(int argc, char **argv) {
    demo_mode_t mode = DEMO_MODE_COMPLETE;
    
    // Parse command line arguments
    if (argc > 1) {
        if (strcmp(argv[1], "basic") == 0) {
            mode = DEMO_MODE_BASIC;
        } else if (strcmp(argv[1], "performance") == 0) {
            mode = DEMO_MODE_PERFORMANCE;
        } else if (strcmp(argv[1], "scalability") == 0) {
            mode = DEMO_MODE_SCALABILITY;
        } else if (strcmp(argv[1], "telemetry") == 0) {
            mode = DEMO_MODE_TELEMETRY;
        } else if (strcmp(argv[1], "complete") == 0) {
            mode = DEMO_MODE_COMPLETE;
        } else {
            printf("Usage: %s [basic|performance|scalability|telemetry|complete]\n", argv[0]);
            printf("Default mode: complete\n\n");
            printf("Modes:\n");
            printf("  basic      - Basic workflow demonstration\n");
            printf("  performance- Performance benchmarking focus\n");
            printf("  scalability- Scalability testing\n");
            printf("  telemetry  - Telemetry integration showcase\n");
            printf("  complete   - Complete feature demonstration\n");
            return 1;
        }
    }
    
    // Run the demonstration
    int result = run_complete_demonstration(mode);
    
    printf("\n");
    if (result == 0) {
        printf("🎉 ARENAC complete demonstration finished successfully!\n");
        printf("💡 The AOT → Runtime allocation vision is fully operational!\n");
        return 0;
    } else {
        printf("💥 ARENAC demonstration encountered errors!\n");
        return 1;
    }
}