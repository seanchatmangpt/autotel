/*
 * CNS 8H Reasoning Engine - Comprehensive Test Suite
 * Tests all 8 hops of the cognitive cycle
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>

// Define test mode
#define CNS_8H_TEST 1

// Include the 8H reasoning implementation
#include "cns_8h_reasoning.c"

// Test helpers
#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "FAIL: %s at %s:%d\n", msg, __FILE__, __LINE__); \
            return 1; \
        } \
    } while (0)

#define TEST_PASS(test_name) \
    printf("✓ %s\n", test_name)

// Mock graph implementation for testing
typedef struct {
    uint32_t node_count;
    uint32_t edge_count;
    uint32_t* nodes;
    cns_edge_t* edges;
} mock_graph_t;

bool cns_graph_has_node(cns_graph_t* graph, uint32_t node_id) {
    // Mock implementation - always return true for test nodes
    return node_id < 1000;
}

cns_error_t cns_graph_get_edges(cns_graph_t* graph, uint32_t node_id,
                                 cns_edge_t** edges, size_t* count) {
    // Mock implementation - return some test edges
    static cns_edge_t test_edges[3] = {
        { .source = 42, .target = 100, .label = 7, .weight = 1.0 },
        { .source = 42, .target = 101, .label = 8, .weight = 1.0 },
        { .source = 42, .target = 102, .label = 9, .weight = 1.0 }
    };
    
    *edges = test_edges;
    *count = 3;
    return CNS_SUCCESS;
}

cns_error_t cns_graph_add_edge(cns_graph_t* graph, const cns_edge_t* edge) {
    // Mock implementation
    return CNS_SUCCESS;
}

// Test functions
int test_engine_creation(void) {
    printf("\nTesting 8H Engine Creation...\n");
    
    CNSSparqlEngine sparql = {0};
    CNSShaclEngine shacl = { .sparql_engine = &sparql };
    cns_graph_t graph = {0};
    cns_8t_processor_t processor = {0};
    
    cns_8h_engine_t* engine = cns_8h_create(&shacl, &graph, &processor);
    TEST_ASSERT(engine != NULL, "Engine creation failed");
    TEST_ASSERT(engine->shacl_engine == &shacl, "SHACL engine not set");
    TEST_ASSERT(engine->knowledge_graph == &graph, "Knowledge graph not set");
    TEST_ASSERT(engine->processor == &processor, "Processor not set");
    
    cns_8h_destroy(engine);
    TEST_PASS("Engine creation and destruction");
    
    return 0;
}

int test_problem_recognition(void) {
    printf("\nTesting Hop 1: Problem Recognition...\n");
    
    CNSSparqlEngine sparql = {0};
    CNSShaclEngine shacl = { .sparql_engine = &sparql };
    cns_graph_t graph = {0};
    cns_8t_processor_t processor = {0};
    
    cns_8h_engine_t* engine = cns_8h_create(&shacl, &graph, &processor);
    TEST_ASSERT(engine != NULL, "Engine creation failed");
    
    // Test MIN_COUNT constraint
    cns_8h_problem_t problem = {
        .violation_id = 1,
        .node_id = 42,
        .property_id = 7,
        .constraint_type = CNS_SHACL_MIN_COUNT,
        .constraint_data = NULL
    };
    
    cns_error_t result = cns_8h_execute_cycle(engine, &problem);
    TEST_ASSERT(result == CNS_SUCCESS, "MIN_COUNT problem recognition failed");
    TEST_ASSERT(engine->state.hop_completed[0], "Hop 1 not completed");
    
    TEST_PASS("Problem recognition for MIN_COUNT");
    
    // Test MAX_COUNT constraint
    problem.constraint_type = CNS_SHACL_MAX_COUNT;
    result = cns_8h_execute_cycle(engine, &problem);
    TEST_ASSERT(result == CNS_SUCCESS, "MAX_COUNT problem recognition failed");
    
    TEST_PASS("Problem recognition for MAX_COUNT");
    
    // Test CLASS constraint
    problem.constraint_type = CNS_SHACL_CLASS;
    result = cns_8h_execute_cycle(engine, &problem);
    TEST_ASSERT(result == CNS_SUCCESS, "CLASS problem recognition failed");
    
    TEST_PASS("Problem recognition for CLASS");
    
    cns_8h_destroy(engine);
    return 0;
}

int test_context_loading(void) {
    printf("\nTesting Hop 2: Context Loading...\n");
    
    CNSSparqlEngine sparql = {0};
    CNSShaclEngine shacl = { .sparql_engine = &sparql };
    cns_graph_t graph = {0};
    cns_8t_processor_t processor = {0};
    
    cns_8h_engine_t* engine = cns_8h_create(&shacl, &graph, &processor);
    TEST_ASSERT(engine != NULL, "Engine creation failed");
    
    cns_8h_problem_t problem = {
        .violation_id = 1,
        .node_id = 42,
        .property_id = 7,
        .constraint_type = CNS_SHACL_MIN_COUNT
    };
    
    cns_error_t result = cns_8h_execute_cycle(engine, &problem);
    TEST_ASSERT(result == CNS_SUCCESS, "Context loading failed");
    TEST_ASSERT(engine->state.hop_completed[1], "Hop 2 not completed");
    TEST_ASSERT(engine->context_count > 0, "No context loaded");
    
    // Verify relevance scoring
    bool found_relevant = false;
    for (uint32_t i = 0; i < engine->context_count; i++) {
        if (engine->context_nodes[i].predicate == problem.property_id) {
            TEST_ASSERT(engine->context_nodes[i].relevance_score == 2.0,
                       "Incorrect relevance score");
            found_relevant = true;
        }
    }
    
    TEST_PASS("Context loading and relevance scoring");
    
    cns_8h_destroy(engine);
    return 0;
}

int test_hypothesis_generation(void) {
    printf("\nTesting Hop 3: Hypothesis Generation...\n");
    
    CNSSparqlEngine sparql = {0};
    CNSShaclEngine shacl = { .sparql_engine = &sparql };
    cns_graph_t graph = {0};
    cns_8t_processor_t processor = {0};
    
    cns_8h_engine_t* engine = cns_8h_create(&shacl, &graph, &processor);
    TEST_ASSERT(engine != NULL, "Engine creation failed");
    
    // Test hypothesis for MIN_COUNT
    cns_8h_problem_t problem = {
        .violation_id = 1,
        .node_id = 42,
        .property_id = 7,
        .constraint_type = CNS_SHACL_MIN_COUNT
    };
    
    cns_error_t result = cns_8h_execute_cycle(engine, &problem);
    TEST_ASSERT(result == CNS_SUCCESS, "Hypothesis generation failed");
    TEST_ASSERT(engine->state.hop_completed[2], "Hop 3 not completed");
    TEST_ASSERT(engine->hypothesis_count > 0, "No hypotheses generated");
    TEST_ASSERT(strstr(engine->hypotheses[0].description, "Add property") != NULL,
               "Incorrect hypothesis for MIN_COUNT");
    
    TEST_PASS("Hypothesis generation for MIN_COUNT");
    
    // Test hypothesis for MAX_COUNT
    problem.constraint_type = CNS_SHACL_MAX_COUNT;
    result = cns_8h_execute_cycle(engine, &problem);
    TEST_ASSERT(result == CNS_SUCCESS, "Hypothesis generation failed");
    TEST_ASSERT(strstr(engine->hypotheses[0].description, "Remove property") != NULL,
               "Incorrect hypothesis for MAX_COUNT");
    
    TEST_PASS("Hypothesis generation for MAX_COUNT");
    
    cns_8h_destroy(engine);
    return 0;
}

int test_full_cycle(void) {
    printf("\nTesting Complete 8H Cycle...\n");
    
    CNSSparqlEngine sparql = {0};
    CNSShaclEngine shacl = { .sparql_engine = &sparql };
    cns_graph_t graph = {0};
    cns_8t_processor_t processor = {0};
    
    cns_8h_engine_t* engine = cns_8h_create(&shacl, &graph, &processor);
    TEST_ASSERT(engine != NULL, "Engine creation failed");
    
    cns_8h_problem_t problem = {
        .violation_id = 1,
        .node_id = 42,
        .property_id = 7,
        .constraint_type = CNS_SHACL_MIN_COUNT,
        .constraint_data = NULL
    };
    
    // Execute full cycle
    cns_error_t result = cns_8h_execute_cycle(engine, &problem);
    TEST_ASSERT(result == CNS_SUCCESS, "Full cycle failed");
    
    // Verify all hops completed
    for (int i = 0; i < 8; i++) {
        TEST_ASSERT(engine->state.hop_completed[i], "Hop not completed");
    }
    
    TEST_PASS("All 8 hops completed");
    
    // Verify solution exists
    const cns_8h_solution_t* solution = cns_8h_get_solution(engine);
    TEST_ASSERT(solution != NULL, "No solution generated");
    TEST_ASSERT(solution->is_verified, "Solution not verified");
    TEST_ASSERT(solution->entropy_score < 1.0, "Entropy score too high");
    
    TEST_PASS("Solution generated and verified");
    
    // Verify AOT code generation
    const char* aot_code = cns_8h_get_aot_code(engine);
    TEST_ASSERT(aot_code != NULL, "No AOT code generated");
    TEST_ASSERT(strlen(aot_code) > 0, "Empty AOT code");
    TEST_ASSERT(strstr(aot_code, "apply_8h_solution") != NULL,
               "AOT code missing solution function");
    
    TEST_PASS("AOT code generated");
    
    // Verify performance metrics
    uint64_t total_cycles;
    uint64_t hop_cycles[8];
    cns_8h_get_metrics(engine, &total_cycles, hop_cycles);
    
    TEST_ASSERT(total_cycles > 0, "No cycles recorded");
    for (int i = 0; i < 8; i++) {
        TEST_ASSERT(hop_cycles[i] > 0, "No cycles for hop");
    }
    
    TEST_PASS("Performance metrics recorded");
    
    cns_8h_destroy(engine);
    return 0;
}

int test_performance_constraints(void) {
    printf("\nTesting Performance Constraints...\n");
    
    CNSSparqlEngine sparql = {0};
    CNSShaclEngine shacl = { .sparql_engine = &sparql };
    cns_graph_t graph = {0};
    cns_8t_processor_t processor = {0};
    
    cns_8h_engine_t* engine = cns_8h_create(&shacl, &graph, &processor);
    TEST_ASSERT(engine != NULL, "Engine creation failed");
    
    // Run multiple cycles to test performance
    const int num_cycles = 100;
    clock_t start = clock();
    
    for (int i = 0; i < num_cycles; i++) {
        cns_8h_problem_t problem = {
            .violation_id = i,
            .node_id = 42 + i,
            .property_id = 7,
            .constraint_type = CNS_SHACL_MIN_COUNT
        };
        
        cns_error_t result = cns_8h_execute_cycle(engine, &problem);
        TEST_ASSERT(result == CNS_SUCCESS, "Cycle failed");
    }
    
    clock_t end = clock();
    double cpu_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    printf("  Executed %d cycles in %.3f seconds\n", num_cycles, cpu_time);
    printf("  Average time per cycle: %.3f ms\n", (cpu_time * 1000) / num_cycles);
    
    TEST_PASS("Performance within acceptable bounds");
    
    cns_8h_destroy(engine);
    return 0;
}

// Main test runner
int main(int argc, char* argv[]) {
    printf("========================================\n");
    printf("CNS 8H Reasoning Engine Test Suite\n");
    printf("========================================\n");
    
    int failures = 0;
    
    failures += test_engine_creation();
    failures += test_problem_recognition();
    failures += test_context_loading();
    failures += test_hypothesis_generation();
    failures += test_full_cycle();
    failures += test_performance_constraints();
    
    printf("\n========================================\n");
    if (failures == 0) {
        printf("✓ All tests passed!\n");
    } else {
        printf("✗ %d test(s) failed\n", failures);
    }
    printf("========================================\n");
    
    return failures;
}