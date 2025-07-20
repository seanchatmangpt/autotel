/**
 * @file quick_permutation_test.c
 * @brief Quick validation test for CJinja implementations
 */

#include "permutation_tests.h"

void quick_consistency_test(void) {
    printf("🎯 QUICK CONSISTENCY VALIDATION\n");
    printf("===============================\n\n");
    
    // Test 1: Basic variable substitution
    printf("Test 1: Basic Variable Substitution\n");
    PermutationTestCase basic_test = {
        .template = "Hello {{name}}!",
        .var_count = 1,
        .type = TEST_BASIC_VARIABLES,
        .should_pass = true
    };
    strcpy(basic_test.variables[0][0], "name");
    strcpy(basic_test.variables[0][1], "World");
    strcpy(basic_test.expected_result, "Hello World!");
    strcpy(basic_test.description, "Basic substitution");
    
    PermutationTestResult results[IMPL_COUNT];
    run_permutation_test(&basic_test, results);
    
    printf("  Expected: '%s'\n", basic_test.expected_result);
    printf("  Final:    '%s' (%s) [%llu ns]\n", 
           results[0].actual_result, 
           results[0].passed ? "✅ PASS" : "❌ FAIL",
           results[0].execution_time_ns);
    printf("  Ultra:    '%s' (%s) [%llu ns]\n",
           results[1].actual_result,
           results[1].passed ? "✅ PASS" : "❌ FAIL", 
           results[1].execution_time_ns);
    printf("  Blazing:  '%s' (%s) [%llu ns]\n",
           results[2].actual_result,
           results[2].passed ? "✅ PASS" : "❌ FAIL",
           results[2].execution_time_ns);
    
    // Test 2: Multiple variables
    printf("\nTest 2: Multiple Variables\n");
    PermutationTestCase multi_test = {
        .template = "{{a}} + {{b}} = {{result}}",
        .var_count = 3,
        .type = TEST_BASIC_VARIABLES,
        .should_pass = true
    };
    strcpy(multi_test.variables[0][0], "a");
    strcpy(multi_test.variables[0][1], "2");
    strcpy(multi_test.variables[1][0], "b");
    strcpy(multi_test.variables[1][1], "3");
    strcpy(multi_test.variables[2][0], "result");
    strcpy(multi_test.variables[2][1], "5");
    strcpy(multi_test.expected_result, "2 + 3 = 5");
    
    PermutationTestResult multi_results[IMPL_COUNT];
    run_permutation_test(&multi_test, multi_results);
    
    printf("  Expected: '%s'\n", multi_test.expected_result);
    printf("  Final:    '%s' (%s) [%llu ns]\n", 
           multi_results[0].actual_result, 
           multi_results[0].passed ? "✅ PASS" : "❌ FAIL",
           multi_results[0].execution_time_ns);
    printf("  Ultra:    '%s' (%s) [%llu ns]\n",
           multi_results[1].actual_result,
           multi_results[1].passed ? "✅ PASS" : "❌ FAIL",
           multi_results[1].execution_time_ns);
    printf("  Blazing:  '%s' (%s) [%llu ns]\n",
           multi_results[2].actual_result,
           multi_results[2].passed ? "✅ PASS" : "❌ FAIL",
           multi_results[2].execution_time_ns);
           
    // Test 3: Edge cases
    printf("\nTest 3: Edge Cases\n");
    const char* edge_templates[] = {
        "",                     // Empty
        "No variables",         // No substitution  
        "{{missing}}",         // Missing variable
        "{{a}}{{b}}",          // Consecutive
        "pre{{var}}post"       // Surrounded
    };
    
    for (int i = 0; i < 5; i++) {
        PermutationTestCase edge_test = {
            .var_count = 2,
            .type = TEST_EDGE_CASES,
            .should_pass = true
        };
        strcpy(edge_test.template, edge_templates[i]);
        strcpy(edge_test.variables[0][0], "var");
        strcpy(edge_test.variables[0][1], "X");
        strcpy(edge_test.variables[1][0], "a");
        strcpy(edge_test.variables[1][1], "1");
        strcpy(edge_test.variables[1][0], "b");
        strcpy(edge_test.variables[1][1], "2");
        
        PermutationTestResult edge_results[IMPL_COUNT];
        run_permutation_test(&edge_test, edge_results);
        
        bool consistent = safe_string_equals(edge_results[0].actual_result, edge_results[1].actual_result) &&
                         safe_string_equals(edge_results[1].actual_result, edge_results[2].actual_result);
        
        printf("  '%-15s' -> '%-15s' %s\n", 
               edge_templates[i], 
               edge_results[0].actual_result ? edge_results[0].actual_result : "NULL",
               consistent ? "✅" : "❌");
               
        if (!consistent) {
            printf("    Final: '%s', Ultra: '%s', Blazing: '%s'\n",
                   edge_results[0].actual_result ? edge_results[0].actual_result : "NULL",
                   edge_results[1].actual_result ? edge_results[1].actual_result : "NULL", 
                   edge_results[2].actual_result ? edge_results[2].actual_result : "NULL");
        }
    }
    
    printf("\n");
}

void quick_performance_test(void) {
    printf("⚡ QUICK PERFORMANCE COMPARISON\n");
    printf("==============================\n\n");
    
    const int iterations = 100;
    const char* perf_template = "Hello {{user}}! Welcome to {{service}} v{{version}}.";
    
    // Setup test case
    PermutationTestCase perf_test = {
        .var_count = 3,
        .type = TEST_PERFORMANCE_STRESS,
        .should_pass = true
    };
    strcpy(perf_test.template, perf_template);
    strcpy(perf_test.variables[0][0], "user");
    strcpy(perf_test.variables[0][1], "Alice");
    strcpy(perf_test.variables[1][0], "service");
    strcpy(perf_test.variables[1][1], "CJinja");
    strcpy(perf_test.variables[2][0], "version");
    strcpy(perf_test.variables[2][1], "2.0");
    
    uint64_t impl_times[IMPL_COUNT] = {0};
    
    printf("Running %d iterations per implementation...\n", iterations);
    printf("Template: '%s'\n\n", perf_template);
    
    // Test each implementation
    for (int impl = 0; impl < IMPL_COUNT; impl++) {
        uint64_t total_time = 0;
        
        for (int i = 0; i < iterations; i++) {
            PermutationTestResult result;
            if (impl == 0) result = execute_final_impl(&perf_test);
            else if (impl == 1) result = execute_ultra_impl(&perf_test);
            else result = execute_blazing_impl(&perf_test);
            
            total_time += result.execution_time_ns;
        }
        
        impl_times[impl] = total_time / iterations;
    }
    
    printf("Performance Results:\n");
    printf("  Final Implementation:     %6llu ns\n", impl_times[0]);
    printf("  Ultra-Portable:           %6llu ns (%.2fx vs Final)\n", 
           impl_times[1], (double)impl_times[1] / impl_times[0]);
    printf("  Blazing-Fast:             %6llu ns (%.2fx vs Final)\n", 
           impl_times[2], (double)impl_times[2] / impl_times[0]);
    
    // Find fastest
    uint64_t fastest = impl_times[0];
    int fastest_idx = 0;
    for (int i = 1; i < IMPL_COUNT; i++) {
        if (impl_times[i] < fastest) {
            fastest = impl_times[i];
            fastest_idx = i;
        }
    }
    
    const char* impl_names[] = {"Final", "Ultra-Portable", "Blazing-Fast"};
    printf("\n🏆 Fastest: %s (%llu ns)\n", impl_names[fastest_idx], fastest);
    
    // Check if blazing fast meets target
    if (impl_times[2] < 100) {
        printf("🎯 Blazing-Fast ACHIEVES sub-100ns target!\n");
    } else if (impl_times[2] < 150) {
        printf("⚡ Blazing-Fast achieves excellent sub-150ns performance\n");
    } else {
        printf("⚠️  Blazing-Fast needs more optimization\n");
    }
    
    printf("\n");
}

int main(void) {
    printf("🚀 QUICK CJINJA PERMUTATION VALIDATION\n");
    printf("======================================\n\n");
    
    printf("Testing implementations:\n");
    printf("  • Final Implementation (cjinja_final.h/c)\n");
    printf("  • Ultra-Portable (cjinja_ultra_portable.h/c)\n");
    printf("  • Blazing-Fast (cjinja_blazing_fast.h/c)\n\n");
    
    quick_consistency_test();
    quick_performance_test();
    
    printf("✅ QUICK VALIDATION COMPLETE\n");
    printf("\n💡 For comprehensive testing, run: ./test_all_permutations\n");
    
    return 0;
}