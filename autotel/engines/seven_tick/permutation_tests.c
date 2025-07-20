/**
 * @file permutation_tests.c
 * @brief Comprehensive Permutation Test Implementation for CJinja
 */

#include "permutation_tests.h"

// =============================================================================
// VARIABLE NAME PERMUTATION TESTS
// =============================================================================

void generate_variable_name_permutations(PermutationTestCase* cases, int* count) {
    int idx = *count;
    
    // Single character variables
    const char* single_chars[] = {"a", "b", "x", "y", "z", "A", "Z", "_", "1", "9"};
    for (int i = 0; i < 10 && idx < MAX_TEST_CASES; i++) {
        snprintf(cases[idx].template, sizeof(cases[idx].template), "Value: {{%s}}", single_chars[i]);
        snprintf(cases[idx].variables[0][0], 64, "%s", single_chars[i]);
        snprintf(cases[idx].variables[0][1], 64, "test_%d", i);
        cases[idx].var_count = 1;
        snprintf(cases[idx].expected_result, sizeof(cases[idx].expected_result), "Value: test_%d", i);
        cases[idx].type = TEST_BASIC_VARIABLES;
        cases[idx].should_pass = true;
        snprintf(cases[idx].description, sizeof(cases[idx].description), "Single char variable: %s", single_chars[i]);
        idx++;
    }
    
    // Two character variables
    const char* two_chars[] = {"ab", "xy", "A1", "_x", "99", "aB", "X_", "1a"};
    for (int i = 0; i < 8 && idx < MAX_TEST_CASES; i++) {
        snprintf(cases[idx].template, sizeof(cases[idx].template), "{{%s}} end", two_chars[i]);
        snprintf(cases[idx].variables[0][0], 64, "%s", two_chars[i]);
        snprintf(cases[idx].variables[0][1], 64, "val%d", i);
        cases[idx].var_count = 1;
        snprintf(cases[idx].expected_result, sizeof(cases[idx].expected_result), "val%d end", i);
        cases[idx].type = TEST_BASIC_VARIABLES;
        cases[idx].should_pass = true;
        snprintf(cases[idx].description, sizeof(cases[idx].description), "Two char variable: %s", two_chars[i]);
        idx++;
    }
    
    // Long variable names
    const char* long_names[] = {
        "very_long_variable_name",
        "AnotherLongVariableName",
        "mixed_Case_Variable_123",
        "variable_with_numbers_456789",
        "UPPERCASE_VARIABLE_NAME",
        "lowercase_variable_name"
    };
    for (int i = 0; i < 6 && idx < MAX_TEST_CASES; i++) {
        snprintf(cases[idx].template, sizeof(cases[idx].template), "Start {{%s}} end", long_names[i]);
        snprintf(cases[idx].variables[0][0], 64, "%s", long_names[i]);
        snprintf(cases[idx].variables[0][1], 64, "longval_%d", i);
        cases[idx].var_count = 1;
        snprintf(cases[idx].expected_result, sizeof(cases[idx].expected_result), "Start longval_%d end", i);
        cases[idx].type = TEST_LONG_VARIABLES;
        cases[idx].should_pass = true;
        snprintf(cases[idx].description, sizeof(cases[idx].description), "Long variable: %s", long_names[i]);
        idx++;
    }
    
    *count = idx;
}

// =============================================================================
// TEMPLATE SYNTAX PERMUTATION TESTS  
// =============================================================================

void generate_template_syntax_permutations(PermutationTestCase* cases, int* count) {
    int idx = *count;
    
    // Basic syntax variations
    const char* templates[] = {
        "{{var}}",                    // Simple
        " {{var}} ",                  // With spaces
        "{{var}}{{var}}",            // Consecutive
        "pre{{var}}post",            // Surrounded
        "{{var1}}mid{{var2}}",       // Multiple variables
        "{{var}} and {{var}}",      // Repeated variable
        "start{{var}}middle{{var}}end", // Complex
        "{{a}}{{b}}{{c}}{{d}}",      // Many consecutive
        "{{var}}\n{{var}}",          // With newlines
        "{{var}}\t{{var}}"           // With tabs
    };
    
    for (int i = 0; i < 10 && idx < MAX_TEST_CASES; i++) {
        strcpy(cases[idx].template, templates[i]);
        
        // Set up variables
        snprintf(cases[idx].variables[0][0], 64, "var");
        snprintf(cases[idx].variables[0][1], 64, "X");
        snprintf(cases[idx].variables[1][0], 64, "var1");
        snprintf(cases[idx].variables[1][1], 64, "A");
        snprintf(cases[idx].variables[2][0], 64, "var2");
        snprintf(cases[idx].variables[2][1], 64, "B");
        snprintf(cases[idx].variables[3][0], 64, "a");
        snprintf(cases[idx].variables[3][1], 64, "1");
        snprintf(cases[idx].variables[4][0], 64, "b");
        snprintf(cases[idx].variables[4][1], 64, "2");
        snprintf(cases[idx].variables[5][0], 64, "c");
        snprintf(cases[idx].variables[5][1], 64, "3");
        snprintf(cases[idx].variables[6][0], 64, "d");
        snprintf(cases[idx].variables[6][1], 64, "4");
        cases[idx].var_count = 7;
        
        // Generate expected results
        if (i == 0) strcpy(cases[idx].expected_result, "X");
        else if (i == 1) strcpy(cases[idx].expected_result, " X ");
        else if (i == 2) strcpy(cases[idx].expected_result, "XX");
        else if (i == 3) strcpy(cases[idx].expected_result, "preXpost");
        else if (i == 4) strcpy(cases[idx].expected_result, "AmidB");
        else if (i == 5) strcpy(cases[idx].expected_result, "X and X");
        else if (i == 6) strcpy(cases[idx].expected_result, "startXmiddleXend");
        else if (i == 7) strcpy(cases[idx].expected_result, "1234");
        else if (i == 8) strcpy(cases[idx].expected_result, "X\nX");
        else if (i == 9) strcpy(cases[idx].expected_result, "X\tX");
        
        cases[idx].type = TEST_BASIC_VARIABLES;
        cases[idx].should_pass = true;
        snprintf(cases[idx].description, sizeof(cases[idx].description), "Template syntax %d", i);
        idx++;
    }
    
    *count = idx;
}

// =============================================================================
// EDGE CASE PERMUTATION TESTS
// =============================================================================

void generate_edge_case_permutations(PermutationTestCase* cases, int* count) {
    int idx = *count;
    
    // Empty and null cases
    strcpy(cases[idx].template, "");
    cases[idx].var_count = 0;
    strcpy(cases[idx].expected_result, "");
    cases[idx].type = TEST_EDGE_CASES;
    cases[idx].should_pass = true;
    strcpy(cases[idx].description, "Empty template");
    idx++;
    
    // No variables in template
    strcpy(cases[idx].template, "Hello World!");
    cases[idx].var_count = 0;
    strcpy(cases[idx].expected_result, "Hello World!");
    cases[idx].type = TEST_EDGE_CASES;
    cases[idx].should_pass = true;
    strcpy(cases[idx].description, "No variables");
    idx++;
    
    // Missing variable
    strcpy(cases[idx].template, "Hello {{missing}}!");
    cases[idx].var_count = 0;
    strcpy(cases[idx].expected_result, "Hello !");
    cases[idx].type = TEST_EDGE_CASES;
    cases[idx].should_pass = true;
    strcpy(cases[idx].description, "Missing variable");
    idx++;
    
    // Empty variable value
    strcpy(cases[idx].template, "Value: {{empty}}");
    snprintf(cases[idx].variables[0][0], 64, "empty");
    snprintf(cases[idx].variables[0][1], 64, "");
    cases[idx].var_count = 1;
    strcpy(cases[idx].expected_result, "Value: ");
    cases[idx].type = TEST_EMPTY_VALUES;
    cases[idx].should_pass = true;
    strcpy(cases[idx].description, "Empty variable value");
    idx++;
    
    // Very long values
    strcpy(cases[idx].template, "{{longval}}");
    snprintf(cases[idx].variables[0][0], 64, "longval");
    char long_value[1024];
    for (int i = 0; i < 500; i++) {
        long_value[i] = 'A' + (i % 26);
    }
    long_value[500] = '\0';
    snprintf(cases[idx].variables[0][1], 64, "%.50s...", long_value); // Truncated for storage
    cases[idx].var_count = 1;
    snprintf(cases[idx].expected_result, sizeof(cases[idx].expected_result), "%.50s...", long_value);
    cases[idx].type = TEST_LONG_VARIABLES;
    cases[idx].should_pass = true;
    strcpy(cases[idx].description, "Very long value");
    idx++;
    
    *count = idx;
}

// =============================================================================
// MALFORMED SYNTAX PERMUTATION TESTS
// =============================================================================

void generate_malformed_permutations(PermutationTestCase* cases, int* count) {
    int idx = *count;
    
    // Malformed templates - these should gracefully handle errors
    const char* malformed[] = {
        "{{",                    // Incomplete opening
        "}}",                    // Incomplete closing  
        "{{var",                 // Missing closing
        "var}}",                 // Missing opening
        "{var}",                 // Single braces
        "{{{var}}}",             // Triple braces
        "{{}}",                  // Empty variable
        "{{ }}",                 // Whitespace only
        "{{var1}{var2}}",        // Mixed braces
        "{{var\n}}",             // Newline in variable
        "{{var\t}}",             // Tab in variable
        "{{var }}",              // Trailing space
        "{{ var}}",              // Leading space
        "{{var.name}}",          // Dot notation
        "{{var[0]}}",            // Array notation
        "{{var->field}}"         // Pointer notation
    };
    
    for (int i = 0; i < 16 && idx < MAX_TEST_CASES; i++) {
        strcpy(cases[idx].template, malformed[i]);
        snprintf(cases[idx].variables[0][0], 64, "var");
        snprintf(cases[idx].variables[0][1], 64, "value");
        snprintf(cases[idx].variables[1][0], 64, "var1");
        snprintf(cases[idx].variables[1][1], 64, "val1");
        snprintf(cases[idx].variables[2][0], 64, "var2");
        snprintf(cases[idx].variables[2][1], 64, "val2");
        cases[idx].var_count = 3;
        
        // Expected results for malformed syntax (implementation-dependent)
        if (i == 0) strcpy(cases[idx].expected_result, "{{");
        else if (i == 1) strcpy(cases[idx].expected_result, "}}");
        else if (i == 2) strcpy(cases[idx].expected_result, "{{var");
        else if (i == 3) strcpy(cases[idx].expected_result, "var}}");
        else if (i == 4) strcpy(cases[idx].expected_result, "{var}");
        else if (i == 5) strcpy(cases[idx].expected_result, "{value}");
        else if (i == 6) strcpy(cases[idx].expected_result, "");
        else if (i == 7) strcpy(cases[idx].expected_result, "");
        else if (i == 8) strcpy(cases[idx].expected_result, "{{var1}{var2}}");
        else strcpy(cases[idx].expected_result, malformed[i]); // Pass through
        
        cases[idx].type = TEST_MALFORMED_SYNTAX;
        cases[idx].should_pass = true; // Should handle gracefully, not crash
        snprintf(cases[idx].description, sizeof(cases[idx].description), "Malformed: %s", malformed[i]);
        idx++;
    }
    
    *count = idx;
}

// =============================================================================
// PERFORMANCE STRESS PERMUTATION TESTS
// =============================================================================

void generate_performance_permutations(PermutationTestCase* cases, int* count) {
    int idx = *count;
    
    // Many variables test
    char many_vars_template[MAX_TEMPLATE_LEN] = "";
    char expected_many[MAX_RESULT_LEN] = "";
    for (int i = 0; i < 20 && idx < MAX_TEST_CASES; i++) {
        char var_ref[32];
        snprintf(var_ref, sizeof(var_ref), "{{var%d}}", i);
        strcat(many_vars_template, var_ref);
        
        snprintf(cases[idx].variables[i][0], 64, "var%d", i);
        snprintf(cases[idx].variables[i][1], 64, "%d", i);
        
        char expected_val[8];
        snprintf(expected_val, sizeof(expected_val), "%d", i);
        strcat(expected_many, expected_val);
    }
    
    strcpy(cases[idx].template, many_vars_template);
    cases[idx].var_count = 20;
    strcpy(cases[idx].expected_result, expected_many);
    cases[idx].type = TEST_PERFORMANCE_STRESS;
    cases[idx].should_pass = true;
    strcpy(cases[idx].description, "Many variables (20)");
    idx++;
    
    // Large template test
    char large_template[MAX_TEMPLATE_LEN] = "Start ";
    for (int i = 0; i < 50; i++) {
        strcat(large_template, "text {{var}} more text ");
    }
    strcat(large_template, "End");
    
    strcpy(cases[idx].template, large_template);
    snprintf(cases[idx].variables[0][0], 64, "var");
    snprintf(cases[idx].variables[0][1], 64, "X");
    cases[idx].var_count = 1;
    
    // Build expected result
    char large_expected[MAX_RESULT_LEN] = "Start ";
    for (int i = 0; i < 50; i++) {
        strcat(large_expected, "text X more text ");
    }
    strcat(large_expected, "End");
    strcpy(cases[idx].expected_result, large_expected);
    
    cases[idx].type = TEST_PERFORMANCE_STRESS;
    cases[idx].should_pass = true;
    strcpy(cases[idx].description, "Large template (50 substitutions)");
    idx++;
    
    *count = idx;
}

// =============================================================================
// UNICODE AND SPECIAL CHARACTER TESTS
// =============================================================================

void generate_unicode_permutations(PermutationTestCase* cases, int* count) {
    int idx = *count;
    
    // Special characters in values
    const char* special_values[] = {
        "hello@world.com",
        "path/to/file",
        "key=value&other=thing",
        "100% complete",
        "line1\nline2",
        "tab\there",
        "quote\"inside",
        "single'quote",
        "back\\slash",
        "dollar$sign"
    };
    
    for (int i = 0; i < 10 && idx < MAX_TEST_CASES; i++) {
        strcpy(cases[idx].template, "Value: {{special}}");
        snprintf(cases[idx].variables[0][0], 64, "special");
        snprintf(cases[idx].variables[0][1], 64, "%s", special_values[i]);
        cases[idx].var_count = 1;
        snprintf(cases[idx].expected_result, sizeof(cases[idx].expected_result), "Value: %s", special_values[i]);
        cases[idx].type = TEST_SPECIAL_CHARS;
        cases[idx].should_pass = true;
        snprintf(cases[idx].description, sizeof(cases[idx].description), "Special chars: %s", special_values[i]);
        idx++;
    }
    
    *count = idx;
}

// =============================================================================
// IMPLEMENTATION WRAPPERS
// =============================================================================

PermutationTestResult execute_final_impl(const PermutationTestCase* test) {
    PermutationTestResult result = {0};
    result.impl_type = IMPL_FINAL;
    
    uint64_t start = get_timestamp_ns();
    
    CJinjaContext* ctx = cjinja_create_context();
    if (!ctx) {
        strcpy(result.error_message, "Failed to create context");
        return result;
    }
    
    // Set variables
    for (int i = 0; i < test->var_count; i++) {
        cjinja_set_var(ctx, test->variables[i][0], test->variables[i][1]);
    }
    
    // Render template
    char* rendered = cjinja_render_string(test->template, ctx);
    uint64_t end = get_timestamp_ns();
    
    result.execution_time_ns = end - start;
    
    if (rendered) {
        strncpy(result.actual_result, rendered, sizeof(result.actual_result) - 1);
        result.actual_result[sizeof(result.actual_result) - 1] = '\0';
        free(rendered);
        result.passed = safe_string_equals(result.actual_result, test->expected_result);
    } else {
        strcpy(result.error_message, "Render returned NULL");
    }
    
    cjinja_destroy_context(ctx);
    return result;
}

PermutationTestResult execute_ultra_impl(const PermutationTestCase* test) {
    PermutationTestResult result = {0};
    result.impl_type = IMPL_ULTRA_PORTABLE;
    
    uint64_t start = get_timestamp_ns();
    
    CJinjaUltraContext* ctx = cjinja_ultra_create_context();
    if (!ctx) {
        strcpy(result.error_message, "Failed to create ultra context");
        return result;
    }
    
    // Set variables
    for (int i = 0; i < test->var_count; i++) {
        cjinja_ultra_set_var(ctx, test->variables[i][0], test->variables[i][1]);
    }
    
    // Render template
    char* rendered = cjinja_ultra_render_variables(test->template, ctx);
    uint64_t end = get_timestamp_ns();
    
    result.execution_time_ns = end - start;
    
    if (rendered) {
        strncpy(result.actual_result, rendered, sizeof(result.actual_result) - 1);
        result.actual_result[sizeof(result.actual_result) - 1] = '\0';
        free(rendered);
        result.passed = safe_string_equals(result.actual_result, test->expected_result);
    } else {
        strcpy(result.error_message, "Ultra render returned NULL");
    }
    
    cjinja_ultra_destroy_context(ctx);
    return result;
}

PermutationTestResult execute_blazing_impl(const PermutationTestCase* test) {
    PermutationTestResult result = {0};
    result.impl_type = IMPL_BLAZING_FAST;
    
    uint64_t start = get_timestamp_ns();
    
    CJinjaBlazingContext* ctx = cjinja_blazing_create_context();
    if (!ctx) {
        strcpy(result.error_message, "Failed to create blazing context");
        return result;
    }
    
    // Set variables
    for (int i = 0; i < test->var_count; i++) {
        cjinja_blazing_set_var(ctx, test->variables[i][0], test->variables[i][1]);
    }
    
    // Render template
    char* rendered = cjinja_blazing_render(test->template, ctx);
    uint64_t end = get_timestamp_ns();
    
    result.execution_time_ns = end - start;
    
    if (rendered) {
        strncpy(result.actual_result, rendered, sizeof(result.actual_result) - 1);
        result.actual_result[sizeof(result.actual_result) - 1] = '\0';
        free(rendered);
        result.passed = safe_string_equals(result.actual_result, test->expected_result);
    } else {
        strcpy(result.error_message, "Blazing render returned NULL");
    }
    
    cjinja_blazing_destroy_context(ctx);
    return result;
}

// =============================================================================
// TEST EXECUTION
// =============================================================================

void run_permutation_test(const PermutationTestCase* test, PermutationTestResult* results) {
    results[0] = execute_final_impl(test);
    results[1] = execute_ultra_impl(test);
    results[2] = execute_blazing_impl(test);
}

PermutationTestStats run_all_permutation_tests(void) {
    PermutationTestStats stats = {0};
    
    // Generate all test cases
    PermutationTestCase* cases = malloc(MAX_TEST_CASES * sizeof(PermutationTestCase));
    if (!cases) {
        printf("Failed to allocate test cases\n");
        return stats;
    }
    
    int test_count = 0;
    
    printf("Generating permutation tests...\n");
    generate_variable_name_permutations(cases, &test_count);
    printf("Variable name tests: %d\n", test_count);
    
    int prev_count = test_count;
    generate_template_syntax_permutations(cases, &test_count);
    printf("Template syntax tests: %d\n", test_count - prev_count);
    
    prev_count = test_count;
    generate_edge_case_permutations(cases, &test_count);
    printf("Edge case tests: %d\n", test_count - prev_count);
    
    prev_count = test_count;
    generate_malformed_permutations(cases, &test_count);
    printf("Malformed tests: %d\n", test_count - prev_count);
    
    prev_count = test_count;
    generate_performance_permutations(cases, &test_count);
    printf("Performance tests: %d\n", test_count - prev_count);
    
    prev_count = test_count;
    generate_unicode_permutations(cases, &test_count);
    printf("Unicode tests: %d\n", test_count - prev_count);
    
    printf("\nTotal test cases generated: %d\n\n", test_count);
    
    stats.total_tests = test_count * IMPL_COUNT;
    uint64_t total_start = get_timestamp_ns();
    
    // Run all tests
    for (int i = 0; i < test_count; i++) {
        PermutationTestResult results[IMPL_COUNT];
        run_permutation_test(&cases[i], results);
        
        // Update statistics
        for (int j = 0; j < IMPL_COUNT; j++) {
            if (results[j].passed) {
                stats.passed_tests++;
                stats.impl_results[j][0]++;
            } else {
                stats.failed_tests++;
                stats.impl_results[j][1]++;
                
                // Print failed test details
                printf("FAILED: %s [%s]\n", cases[i].description, 
                       j == 0 ? "Final" : j == 1 ? "Ultra" : "Blazing");
                printf("  Expected: '%s'\n", cases[i].expected_result);
                printf("  Actual:   '%s'\n", results[j].actual_result);
                if (strlen(results[j].error_message) > 0) {
                    printf("  Error:    %s\n", results[j].error_message);
                }
                printf("\n");
            }
        }
        
        // Progress indicator
        if ((i + 1) % 100 == 0) {
            printf("Completed %d/%d test cases...\n", i + 1, test_count);
        }
    }
    
    uint64_t total_end = get_timestamp_ns();
    stats.total_time_ns = total_end - total_start;
    
    free(cases);
    return stats;
}

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

void generate_random_string(char* buffer, int length, bool alphanumeric_only) {
    const char* chars = alphanumeric_only ? 
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789" :
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()_+-=[]{}|;:,.<>?";
    
    int chars_len = strlen(chars);
    for (int i = 0; i < length - 1; i++) {
        buffer[i] = chars[rand() % chars_len];
    }
    buffer[length - 1] = '\0';
}

void print_test_case(const PermutationTestCase* test) {
    printf("Test: %s\n", test->description);
    printf("  Template: '%s'\n", test->template);
    printf("  Variables: ");
    for (int i = 0; i < test->var_count; i++) {
        printf("%s='%s' ", test->variables[i][0], test->variables[i][1]);
    }
    printf("\n  Expected: '%s'\n", test->expected_result);
}

void print_permutation_stats(const PermutationTestStats* stats) {
    printf("\n🧪 PERMUTATION TEST RESULTS\n");
    printf("==========================\n\n");
    
    printf("📊 Overall Statistics:\n");
    printf("  Total tests:  %u\n", stats->total_tests);
    printf("  Passed:       %u (%.1f%%)\n", stats->passed_tests, 
           100.0 * stats->passed_tests / stats->total_tests);
    printf("  Failed:       %u (%.1f%%)\n", stats->failed_tests,
           100.0 * stats->failed_tests / stats->total_tests);
    printf("  Execution time: %.2f ms\n\n", stats->total_time_ns / 1000000.0);
    
    printf("🔍 Implementation Results:\n");
    const char* impl_names[] = {"Final", "Ultra-Portable", "Blazing-Fast"};
    for (int i = 0; i < IMPL_COUNT; i++) {
        uint32_t total = stats->impl_results[i][0] + stats->impl_results[i][1];
        printf("  %s: %u/%u passed (%.1f%%)\n", impl_names[i],
               stats->impl_results[i][0], total,
               100.0 * stats->impl_results[i][0] / total);
    }
    
    printf("\n");
    if (stats->failed_tests == 0) {
        printf("🎉 ALL PERMUTATION TESTS PASSED!\n");
    } else {
        printf("⚠️  %u tests failed - check output above for details\n", stats->failed_tests);
    }
}