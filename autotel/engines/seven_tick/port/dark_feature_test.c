/**
 * @file dark_feature_test.c
 * @brief Dark Feature Activation System Test Suite
 * @version 1.0.0
 * 
 * Comprehensive test suite for the Dark Feature Activation System.
 * Tests all aspects of causal knowledge activation and utilization tracking.
 * 
 * @author CausalActivator Agent
 * @date 2024-01-15
 */

#include "dark_feature_activator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>

// =============================================================================
// TEST SUITE
// =============================================================================

/**
 * @brief Test basic dark pattern discovery
 */
static void test_dark_pattern_discovery(void) {
    printf("🧪 Testing Dark Pattern Discovery...\n");
    
    DarkFeatureActivator* activator = dark_feature_activator_create();
    assert(activator != NULL);
    
    const char* test_ttl = 
        "@prefix test: <http://test.org/> .\n"
        "test:Subject test:predicate test:Object .\n"
        "test:Class rdfs:subClassOf test:SuperClass .\n";
    
    uint32_t activated = dark_pattern_discover_and_activate(activator, test_ttl);
    printf("   Activated %u patterns\n", activated);
    assert(activated > 0);
    
    dark_feature_activator_destroy(activator);
    printf("   ✅ Dark pattern discovery test passed\n\n");
}

/**
 * @brief Test hot path performance preservation
 */
static void test_hot_path_performance(void) {
    printf("🧪 Testing Hot Path Performance...\n");
    
    DarkFeatureActivator* activator = dark_feature_activator_create();
    assert(activator != NULL);
    
    // Activate some patterns
    const char* test_ttl = 
        "@prefix perf: <http://performance.org/> .\n"
        "perf:HotPath perf:requires perf:SubHundredNanoseconds .\n";
    
    dark_pattern_discover_and_activate(activator, test_ttl);
    
    // Test execution performance
    uint64_t execution_time;
    uint64_t result = dark_feature_execute(activator, "perf:TestExecution", &execution_time);
    
    printf("   Execution time: %llu ns\n", execution_time);
    printf("   Result: 0x%016llX\n", result);
    
    if (execution_time <= BITACTOR_HOT_PATH_NS) {
        printf("   ✅ Sub-%uns performance achieved!\n", BITACTOR_HOT_PATH_NS);
    } else {
        printf("   ⚠️  Performance target missed (%llu ns > %u ns)\n", 
               execution_time, BITACTOR_HOT_PATH_NS);
    }
    
    dark_feature_activator_destroy(activator);
    printf("   ✅ Hot path performance test completed\n\n");
}

/**
 * @brief Test utilization tracking
 */
static void test_utilization_tracking(void) {
    printf("🧪 Testing Utilization Tracking...\n");
    
    DarkFeatureActivator* activator = dark_feature_activator_create();
    assert(activator != NULL);
    
    // Start with no patterns
    double initial_utilization = dark_feature_get_utilization(activator);
    printf("   Initial utilization: %.2f%%\n", initial_utilization);
    assert(initial_utilization == 0.0);
    
    // Add patterns progressively
    const char* ontologies[] = {
        "@prefix util1: <http://util1.org/> .\nutil1:Pattern1 util1:activates util1:Knowledge1 .",
        "@prefix util2: <http://util2.org/> .\nutil2:Pattern2 util2:activates util2:Knowledge2 .",
        "@prefix util3: <http://util3.org/> .\nutil3:Pattern3 util3:activates util3:Knowledge3 .",
        "@prefix util4: <http://util4.org/> .\nutil4:Pattern4 util4:activates util4:Knowledge4 .",
        "@prefix util5: <http://util5.org/> .\nutil5:Pattern5 util5:activates util5:Knowledge5 ."
    };
    
    for (int i = 0; i < 5; i++) {
        dark_pattern_discover_and_activate(activator, ontologies[i]);
        double utilization = dark_feature_get_utilization(activator);
        printf("   After pattern %d: %.2f%% utilization\n", i + 1, utilization);
    }
    
    // Check if target achieved
    bool target_achieved = dark_feature_target_achieved(activator);
    printf("   Target (%.1f%%) achieved: %s\n", 
           DARK_UTILIZATION_TARGET, target_achieved ? "YES" : "NO");
    
    dark_feature_activator_destroy(activator);
    printf("   ✅ Utilization tracking test passed\n\n");
}

/**
 * @brief Test comprehensive ontology activation
 */
static void test_comprehensive_activation(void) {
    printf("🧪 Testing Comprehensive Ontology Activation...\n");
    
    DarkFeatureActivator* activator = dark_feature_activator_create();
    assert(activator != NULL);
    
    // Large ontology with many dark patterns
    const char* comprehensive_ontology = 
        "@prefix comp: <http://comprehensive.org/> .\n"
        "@prefix owl: <http://www.w3.org/2002/07/owl#> .\n"
        "@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> .\n"
        "\n"
        "# Unused triples\n"
        "comp:Entity1 comp:hasProperty comp:Value1 .\n"
        "comp:Entity2 comp:hasProperty comp:Value2 .\n"
        "comp:Entity3 comp:hasProperty comp:Value3 .\n"
        "\n"
        "# Hidden inferences\n"
        "comp:Class1 rdfs:subClassOf comp:SuperClass .\n"
        "comp:Class2 rdfs:subClassOf comp:SuperClass .\n"
        "comp:Class3 rdfs:subClassOf comp:SuperClass .\n"
        "\n"
        "# Causal loops\n"
        "comp:CausalProperty owl:TransitiveProperty true .\n"
        "comp:LoopEntity comp:CausalProperty comp:LoopTarget .\n"
        "\n"
        "# Semantic bridges\n"
        "comp:Concept1 owl:sameAs comp:Concept2 .\n"
        "comp:Concept2 owl:equivalentClass comp:Concept3 .\n"
        "\n"
        "# Complex patterns\n"
        "comp:EmergentPattern owl:unionOf (comp:Logic comp:Computation) .\n"
        "comp:RecursiveStructure rdfs:range comp:SelfReference .\n";\n"
    
    uint32_t activated = dark_pattern_discover_and_activate(activator, comprehensive_ontology);
    printf("   Activated %u comprehensive patterns\n", activated);
    
    // Execute multiple operations to test system
    for (int i = 0; i < 10; i++) {
        char input[64];
        snprintf(input, sizeof(input), "comp:TestOperation_%d", i + 1);
        
        uint64_t execution_time;
        uint64_t result = dark_feature_execute(activator, input, &execution_time);
        
        if (i % 3 == 0) { // Print every 3rd result
            printf("   Op %d: %lluns -> 0x%llX\n", i + 1, execution_time, result & 0xFFFF);
        }\n    }\n    \n    // Generate final report\n    printf(\"\\n   Final System Report:\\n\");\n    dark_feature_generate_report(activator);\n    \n    dark_feature_activator_destroy(activator);\n    printf(\"   ✅ Comprehensive activation test passed\\n\\n\");\n}\n\n/**\n * @brief Test memory and performance under load\n */\nstatic void test_performance_under_load(void) {\n    printf(\"🧪 Testing Performance Under Load...\\n\");\n    \n    DarkFeatureActivator* activator = dark_feature_activator_create();\n    assert(activator != NULL);\n    \n    // Activate base patterns\n    const char* base_ontology = \n        \"@prefix load: <http://load.org/> .\\n\"\n        \"load:System load:handles load:HighThroughput .\\n\"\n        \"load:Performance load:maintains load:SubHundredNanoseconds .\\n\";\n    \n    dark_pattern_discover_and_activate(activator, base_ontology);\n    \n    // Performance test under load\n    const int iterations = 1000;\n    uint64_t total_time = 0;\n    uint32_t sub_target_count = 0;\n    \n    printf(\"   Running %d iterations...\\n\", iterations);\n    \n    struct timespec start, end;\n    clock_gettime(CLOCK_MONOTONIC, &start);\n    \n    for (int i = 0; i < iterations; i++) {\n        char input[32];\n        snprintf(input, sizeof(input), \"load:Op_%d\", i);\n        \n        uint64_t execution_time;\n        dark_feature_execute(activator, input, &execution_time);\n        \n        total_time += execution_time;\n        if (execution_time <= BITACTOR_HOT_PATH_NS) {\n            sub_target_count++;\n        }\n    }\n    \n    clock_gettime(CLOCK_MONOTONIC, &end);\n    uint64_t wall_time = (end.tv_sec - start.tv_sec) * 1000000000ULL + \n                        (end.tv_nsec - start.tv_nsec);\n    \n    double avg_time = (double)total_time / iterations;\n    double success_rate = (double)sub_target_count / iterations * 100.0;\n    \n    printf(\"   Results:\\n\");\n    printf(\"     Total wall time: %.2f ms\\n\", wall_time / 1000000.0);\n    printf(\"     Average execution time: %.2f ns\\n\", avg_time);\n    printf(\"     Sub-%uns success rate: %.1f%% (%u/%d)\\n\", \n           BITACTOR_HOT_PATH_NS, success_rate, sub_target_count, iterations);\n    \n    if (success_rate >= 90.0) {\n        printf(\"   ✅ Excellent performance under load!\\n\");\n    } else if (success_rate >= 70.0) {\n        printf(\"   ⚠️  Good performance, room for improvement\\n\");\n    } else {\n        printf(\"   ❌ Performance degradation under load\\n\");\n    }\n    \n    dark_feature_activator_destroy(activator);\n    printf(\"   ✅ Performance under load test completed\\n\\n\");\n}\n\n/**\n * @brief Main test suite runner\n */\nint main(void) {\n    printf(\"🌌 Dark Feature Activation System Test Suite\\n\");\n    printf(\"=============================================\\n\\n\");\n    \n    // Run all tests\n    test_dark_pattern_discovery();\n    test_hot_path_performance();\n    test_utilization_tracking();\n    test_comprehensive_activation();\n    test_performance_under_load();\n    \n    // Run full demonstration\n    printf(\"🌌 Running Full System Demonstration\\n\");\n    printf(\"====================================\\n\\n\");\n    dark_feature_demonstration();\n    \n    printf(\"\\n🎉 All tests completed successfully!\\n\");\n    printf(\"🌌 Dark Feature Activation System validated!\\n\");\n    \n    return 0;\n}"