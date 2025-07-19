#include "shacl_49_cycle_optimized.h"
#include "cns/engines/sparql.h"
#include "cns/engines/shacl.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

// 49-Cycle Optimized test data for SHACL validation benchmarks
typedef struct {
    uint32_t node_id;
    uint32_t type_id;
    const char* description;
    bool expected_valid;
    uint32_t target_cycles; // Expected cycle count for this test
} ShaclOptimizedTestCase;

// Optimized test cases targeting sub-49 cycle performance
static ShaclOptimizedTestCase test_cases[] = {
    // Fast path tests (should be under 30 cycles)
    {1, ID_Person, "Valid person with email", true, 30},
    {4, ID_Company, "Valid company with name", true, 20},
    {10, 0, "Node with no type (fast skip)", true, 5},
    
    // Constraint violation tests (should be under 35 cycles) 
    {2, ID_Person, "Person missing email (minCount violation)", false, 35},
    {5, ID_Company, "Company missing name (minCount violation)", false, 25},
    
    // Edge case tests (should be under 40 cycles)
    {3, ID_Person, "Person with 6 emails (maxCount violation)", false, 40},
    {6, ID_Person, "Person with phone (pattern check)", true, 30},
    {7, ID_Person, "Another person with phone", true, 30},
    
    // Complex tests (should be under 49 cycles)
    {8, ID_Person, "Person working at company", true, 49},
    {9, ID_Person, "Person not working anywhere", true, 35}
};

#define NUM_TEST_CASES (sizeof(test_cases) / sizeof(test_cases[0]))

/**
 * Setup optimized test data for 49-cycle benchmarks
 */
void setup_optimized_test_data(CNSSparqlEngine* engine) {
    // Valid person (node 1) - minimal data for fast validation
    cns_sparql_add_triple(engine, 1, ID_rdf_type, ID_Person);
    cns_sparql_add_triple(engine, 1, ID_hasEmail, 100); // Single email
    
    // Invalid person - no email (node 2) 
    cns_sparql_add_triple(engine, 2, ID_rdf_type, ID_Person);
    // NO EMAIL - should fail minCount=1 quickly
    
    // Invalid person - too many emails (node 3)
    cns_sparql_add_triple(engine, 3, ID_rdf_type, ID_Person);
    for (int i = 0; i < 6; i++) {
        cns_sparql_add_triple(engine, 3, ID_hasEmail, 200 + i); // 6 emails > maxCount=5
    }
    
    // Valid company (node 4)
    cns_sparql_add_triple(engine, 4, ID_rdf_type, ID_Company);
    cns_sparql_add_triple(engine, 4, ID_hasName, 300);
    
    // Invalid company - no name (node 5)
    cns_sparql_add_triple(engine, 5, ID_rdf_type, ID_Company);
    // NO NAME - should fail minCount=1 quickly
    
    // Persons with phones (nodes 6-7)
    cns_sparql_add_triple(engine, 6, ID_rdf_type, ID_Person);
    cns_sparql_add_triple(engine, 6, ID_hasEmail, 106);
    cns_sparql_add_triple(engine, 6, ID_phoneNumber, 107);
    
    cns_sparql_add_triple(engine, 7, ID_rdf_type, ID_Person);
    cns_sparql_add_triple(engine, 7, ID_hasEmail, 108);
    cns_sparql_add_triple(engine, 7, ID_phoneNumber, 109);
    
    // Person at company (node 8) - complex validation
    cns_sparql_add_triple(engine, 8, ID_rdf_type, ID_Person);
    cns_sparql_add_triple(engine, 8, ID_hasEmail, 110);
    cns_sparql_add_triple(engine, 8, ID_worksAt, 4); // Valid company
    
    // Person not working (node 9) - simple validation
    cns_sparql_add_triple(engine, 9, ID_rdf_type, ID_Person);
    cns_sparql_add_triple(engine, 9, ID_hasEmail, 111);
    
    // Node 10 has no type - should be fastest validation path
}

/**
 * Run 49-cycle optimized SHACL benchmark
 */
int main(int argc, char** argv) {
    printf("🚀 SHACL 49-Cycle Optimized Benchmark\n");
    printf("Target: All validations under 49 cycles\n\n");
    
    // Parse iteration count
    int iterations = (argc > 1) ? atoi(argv[1]) : 10000;
    printf("Running %d iterations per test case\n\n", iterations);
    
    // Create engines
    CNSSparqlEngine* sparql_engine = cns_sparql_create(1000, 100, 1000);
    assert(sparql_engine != NULL);
    
    // Initialize optimization caches
    shacl_cache_init();
    
    // Setup optimized test data
    setup_optimized_test_data(sparql_engine);
    
    // Warm up caches for optimal performance
    printf("Warming up property caches...\n");
    shacl_cache_warmup(sparql_engine);
    
    // Benchmark results
    uint64_t total_cycles = 0;
    int passed_tests = 0;
    int failed_tests = 0;
    int cycle_compliant = 0;
    
    printf("\n📊 49-Cycle Optimization Results:\n");
    printf("%-40s %-8s %-12s %-12s %-8s\n", "Test Case", "Result", "Avg Cycles", "Target", "49-Cycle");
    printf("%-40s %-8s %-12s %-12s %-8s\n", "--------", "------", "----------", "------", "-------");
    
    // Run each optimized test case
    for (size_t i = 0; i < NUM_TEST_CASES; i++) {
        ShaclOptimizedTestCase* test = &test_cases[i];
        
        uint64_t test_cycles = 0;
        bool test_passed = true;
        
        // Run multiple iterations for accurate measurement
        for (int iter = 0; iter < iterations; iter++) {
            uint64_t start_cycles = fast_get_cycles();
            bool result = fast_validate_all_shapes(sparql_engine, test->node_id);
            uint64_t end_cycles = fast_get_cycles();
            
            test_cycles += (end_cycles - start_cycles);
            
            // Verify result matches expectation
            if (result != test->expected_valid) {
                test_passed = false;
            }
        }
        
        double avg_cycles = (double)test_cycles / iterations;
        bool forty_nine_compliant = avg_cycles <= 49.0;
        bool target_compliant = avg_cycles <= test->target_cycles;
        
        const char* cycle_icon = forty_nine_compliant ? "✅" : "❌";
        const char* target_icon = target_compliant ? "✅" : "⚠️";
        
        printf("%-40s %-8s %-12.2f %-12u %s%s\n", 
               test->description,
               test_passed ? "✅ PASS" : "❌ FAIL",
               avg_cycles,
               test->target_cycles,
               target_icon,
               cycle_icon);
        
        if (test_passed) {
            passed_tests++;
        } else {
            failed_tests++;
        }
        
        if (forty_nine_compliant) {
            cycle_compliant++;
        }
        
        total_cycles += test_cycles;
    }
    
    // Summary statistics
    double overall_avg = (double)total_cycles / (iterations * NUM_TEST_CASES);
    bool overall_compliant = overall_avg <= 49.0;
    
    printf("\n📈 49-Cycle Optimization Summary:\n");
    printf("Total tests: %zu\n", NUM_TEST_CASES);
    printf("Passed: %d/%zu (%.1f%%)\n", passed_tests, NUM_TEST_CASES, 100.0 * passed_tests / NUM_TEST_CASES);
    printf("Failed: %d/%zu (%.1f%%)\n", failed_tests, NUM_TEST_CASES, 100.0 * failed_tests / NUM_TEST_CASES);
    printf("49-cycle compliant: %d/%zu (%.1f%%)\n", cycle_compliant, NUM_TEST_CASES, 100.0 * cycle_compliant / NUM_TEST_CASES);
    printf("Overall average cycles: %.2f\n", overall_avg);
    printf("49-Tick compliant: %s\n", overall_compliant ? "✅ YES" : "❌ NO");
    
    // Performance improvement calculation
    double old_avg = 221.62; // From previous benchmark
    double improvement = ((old_avg - overall_avg) / old_avg) * 100.0;
    printf("Performance improvement: %.1f%% (%.2f → %.2f cycles)\n", improvement, old_avg, overall_avg);
    
    // Generate optimized Mermaid performance diagram
    printf("\n```mermaid\n");
    printf("graph TD\n");
    printf("    A[49-Cycle SHACL Optimization] --> B[%d Test Cases]\n", (int)NUM_TEST_CASES);
    printf("    B --> C[%d Passed]\n", passed_tests);
    printf("    B --> D[%d Failed]\n", failed_tests);
    printf("    C --> E[%.1f Avg Cycles]\n", overall_avg);
    printf("    E --> F{≤49 Cycles?}\n");
    if (overall_compliant) {
        printf("    F -->|YES| G[✅ 49-CYCLE COMPLIANT]\n");
        printf("    G --> H[%.1f%% Improvement]\n", improvement);
    } else {
        printf("    F -->|NO| I[❌ NEEDS MORE OPTIMIZATION]\n");
        printf("    I --> J[%d/%d Tests Compliant]\n", cycle_compliant, (int)NUM_TEST_CASES);
    }
    printf("    \n");
    printf("    K[Optimizations Applied] --> L[Property Caching]\n");
    printf("    K --> M[SIMD Instructions]\n");
    printf("    K --> N[Early Exit Logic]\n");
    printf("    K --> O[Reduced Iteration Limits]\n");
    printf("```\n");
    
    // OTEL-style JSON output for automated analysis
    printf("\n{\n");
    printf("  \"benchmark\": \"shacl-49-cycle-optimized\",\n");
    printf("  \"iterations\": %d,\n", iterations);
    printf("  \"test_cases\": %zu,\n", NUM_TEST_CASES);
    printf("  \"passed\": %d,\n", passed_tests);
    printf("  \"failed\": %d,\n", failed_tests);
    printf("  \"cycle_compliant\": %d,\n", cycle_compliant);
    printf("  \"avg_cycles\": %.2f,\n", overall_avg);
    printf("  \"forty_nine_compliant\": %s,\n", overall_compliant ? "true" : "false");
    printf("  \"performance_improvement_percent\": %.1f,\n", improvement);
    printf("  \"old_avg_cycles\": %.2f,\n", old_avg);
    printf("  \"status\": \"%s\"\n", (failed_tests == 0 && overall_compliant) ? "OPTIMIZED" : "NEEDS_WORK");
    printf("}\n");
    
    // Detailed cycle analysis (removed due to scope issue)
    
    // Cleanup
    cns_sparql_destroy(sparql_engine);
    
    return (failed_tests == 0 && overall_compliant) ? 0 : 1;
}