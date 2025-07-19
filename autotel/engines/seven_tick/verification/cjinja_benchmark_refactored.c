#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../compiler/src/cjinja.h"
#include "../benchmarks/7t_benchmark_framework.h"

// Test data structure for CJinja benchmarks
typedef struct {
    CJinjaEngine *engine;
    CJinjaContext *ctx;
    const char *template;
    const char *test_name;
} CJinjaTestData;

// Benchmark test functions
void benchmark_simple_variable_substitution(void *data) {
    CJinjaTestData *test_data = (CJinjaTestData *)data;
    char *result = cjinja_render_string(test_data->template, test_data->ctx);
    free(result);
}

void benchmark_7tick_variable_substitution(void *data) {
    CJinjaTestData *test_data = (CJinjaTestData *)data;
    char *result = cjinja_render_string_7tick(test_data->template, test_data->ctx);
    free(result);
}

void benchmark_conditional_rendering(void *data) {
    CJinjaTestData *test_data = (CJinjaTestData *)data;
    char *result = cjinja_render_with_conditionals(test_data->template, test_data->ctx);
    free(result);
}

void benchmark_7tick_conditional_rendering(void *data) {
    CJinjaTestData *test_data = (CJinjaTestData *)data;
    char *result = cjinja_render_conditionals_7tick(test_data->template, test_data->ctx);
    free(result);
}

void benchmark_loop_rendering(void *data) {
    CJinjaTestData *test_data = (CJinjaTestData *)data;
    char *result = cjinja_render_with_loops(test_data->template, test_data->ctx);
    free(result);
}

void benchmark_filter_rendering(void *data) {
    CJinjaTestData *test_data = (CJinjaTestData *)data;
    char *result = cjinja_render_with_loops(test_data->template, test_data->ctx);
    free(result);
}

void benchmark_template_inheritance(void *data) {
    CJinjaTestData *test_data = (CJinjaTestData *)data;
    
    // Create inheritance context
    CJinjaInheritanceContext *inherit_ctx = cjinja_create_inheritance_context();
    cjinja_set_base_template(inherit_ctx, 
        "<html><head><title>{{title}}</title></head><body>{{% block content %}}Default{{% endblock %}}</body></html>");
    cjinja_add_block(inherit_ctx, "content", "Inherited content");
    
    char *result = cjinja_render_with_inheritance(test_data->template, test_data->ctx, inherit_ctx);
    free(result);
    cjinja_destroy_inheritance_context(inherit_ctx);
}

void benchmark_batch_rendering(void *data) {
    CJinjaTestData *test_data = (CJinjaTestData *)data;
    
    // Create batch render
    CJinjaBatchRender *batch = cjinja_create_batch_render(5);
    const char *templates[] = {
        "Template 1: {{user}}",
        "Template 2: {{title}}",
        "Template 3: {% if is_admin %}Admin{% endif %}",
        "Template 4: {{user | upper}}",
        "Template 5: {{title | length}} chars"
    };
    
    for (int i = 0; i < 5; i++) {
        batch->templates[i] = templates[i];
    }
    
    cjinja_render_batch(test_data->engine, batch, test_data->ctx);
    cjinja_destroy_batch_render(batch);
}

void benchmark_advanced_filters(void *data) {
    CJinjaTestData *test_data = (CJinjaTestData *)data;
    char *result = cjinja_render_with_loops(test_data->template, test_data->ctx);
    free(result);
}

// Setup test data
CJinjaTestData* setup_cjinja_test_data(void) {
    CJinjaTestData *data = malloc(sizeof(CJinjaTestData));
    
    data->engine = cjinja_create("./templates");
    data->ctx = cjinja_create_context();
    
    // Set up test variables
    cjinja_set_var(data->ctx, "title", "CJinja Performance Test");
    cjinja_set_var(data->ctx, "user", "John Doe");
    cjinja_set_var(data->ctx, "email", "john@example.com");
    cjinja_set_bool(data->ctx, "is_admin", 1);
    cjinja_set_bool(data->ctx, "show_debug", 0);
    
    // Set up arrays for loops
    char *items[] = {"apple", "banana", "cherry", "date", "elderberry"};
    cjinja_set_array(data->ctx, "fruits", items, 5);
    
    char *users[] = {"Alice", "Bob", "Charlie", "Diana", "Eve", "Frank", "Grace", "Henry"};
    cjinja_set_array(data->ctx, "users", users, 8);
    
    return data;
}

void cleanup_cjinja_test_data(CJinjaTestData *data) {
    cjinja_destroy_context(data->ctx);
    cjinja_destroy_engine(data->engine);
    free(data);
}

int main() {
    printf("CJinja Benchmark Suite (Refactored with 7T Framework)\n");
    printf("===================================================\n\n");
    
    // Detect hardware capabilities
    HardwareInfo *hw_info = detect_hardware_capabilities();
    print_hardware_info(hw_info);
    printf("\n");
    
    // Create benchmark suite
    BenchmarkSuite *suite = benchmark_suite_create("CJinja Performance Suite");
    
    // Setup test data
    CJinjaTestData *test_data = setup_cjinja_test_data();
    
    printf("Running CJinja benchmarks with 7T framework...\n\n");
    
    // Test 1: Simple Variable Substitution (49-tick path)
    test_data->template = "Hello {{user}}, welcome to {{title}}!";
    test_data->test_name = "Simple Variable Substitution (49-tick)";
    
    BenchmarkResult result1 = benchmark_execute_single(
        "Simple Variable Substitution (49-tick)",
        10000,
        benchmark_simple_variable_substitution,
        test_data
    );
    benchmark_suite_add_result(suite, result1);
    
    // Test 2: Simple Variable Substitution (7-tick path)
    BenchmarkResult result2 = benchmark_execute_single(
        "Simple Variable Substitution (7-tick)",
        10000,
        benchmark_7tick_variable_substitution,
        test_data
    );
    benchmark_suite_add_result(suite, result2);
    
    // Test 3: Conditional Rendering (49-tick path)
    test_data->template = 
        "{% if is_admin %}Welcome admin {{user}}!{% endif %}"
        "{% if show_debug %}Debug mode enabled{% endif %}"
        "Regular user: {{user}}";
    
    BenchmarkResult result3 = benchmark_execute_single(
        "Conditional Rendering (49-tick)",
        10000,
        benchmark_conditional_rendering,
        test_data
    );
    benchmark_suite_add_result(suite, result3);
    
    // Test 4: Conditional Rendering (7-tick path)
    BenchmarkResult result4 = benchmark_execute_single(
        "Conditional Rendering (7-tick)",
        10000,
        benchmark_7tick_conditional_rendering,
        test_data
    );
    benchmark_suite_add_result(suite, result4);
    
    // Test 5: Loop Rendering (49-tick only)
    test_data->template = 
        "Fruits:\n"
        "{% for fruit in fruits %}"
        "  - {{fruit}}\n"
        "{% endfor %}"
        "Total: {{fruits | length}} fruits";
    
    BenchmarkResult result5 = benchmark_execute_single(
        "Loop Rendering (49-tick only)",
        1000,
        benchmark_loop_rendering,
        test_data
    );
    benchmark_suite_add_result(suite, result5);
    
    // Test 6: Filter Rendering (49-tick only)
    test_data->template = 
        "User: {{user | upper}}\n"
        "Email: {{email | lower}}\n"
        "Title: {{title | capitalize}}\n"
        "Name length: {{user | length}} characters";
    
    BenchmarkResult result6 = benchmark_execute_single(
        "Filter Rendering (49-tick only)",
        10000,
        benchmark_filter_rendering,
        test_data
    );
    benchmark_suite_add_result(suite, result6);
    
    // Test 7: Advanced Filters (49-tick only)
    test_data->template = 
        "User: {{user | trim}}\n"
        "Replaced: {{user | replace('John','Jane')}}\n"
        "Sliced: {{user | slice(0,4)}}\n"
        "Default: {{missing_var | default('Not Found')}}";
    
    BenchmarkResult result7 = benchmark_execute_single(
        "Advanced Filters (49-tick only)",
        10000,
        benchmark_advanced_filters,
        test_data
    );
    benchmark_suite_add_result(suite, result7);
    
    // Test 8: Template Inheritance (49-tick only)
    test_data->template = 
        "{{% extends base %}}\n"
        "{{% block content %}}Welcome {{user}}!{{% endblock %}}";
    
    BenchmarkResult result8 = benchmark_execute_single(
        "Template Inheritance (49-tick only)",
        1000,
        benchmark_template_inheritance,
        test_data
    );
    benchmark_suite_add_result(suite, result8);
    
    // Test 9: Batch Rendering (49-tick only)
    BenchmarkResult result9 = benchmark_execute_single(
        "Batch Rendering (49-tick only)",
        1000,
        benchmark_batch_rendering,
        test_data
    );
    benchmark_suite_add_result(suite, result9);
    
    // Calculate suite statistics
    benchmark_suite_calculate_stats(suite);
    
    // Print results
    printf("=== CJinja Benchmark Results ===\n\n");
    benchmark_suite_print_detailed(suite);
    
    // Performance comparison analysis
    printf("\n=== 7-Tick vs 49-Tick Performance Analysis ===\n");
    
    // Find 7-tick vs 49-tick comparisons
    BenchmarkResult *results = suite->results;
    for (size_t i = 0; i < suite->result_count; i++) {
        if (strstr(results[i].test_name, "(7-tick)") != NULL) {
            // Find corresponding 49-tick result
            for (size_t j = 0; j < suite->result_count; j++) {
                if (strstr(results[j].test_name, "(49-tick)") != NULL && 
                    strncmp(results[i].test_name, results[j].test_name, 
                           strstr(results[i].test_name, "(7-tick)") - results[i].test_name) == 0) {
                    
                    double speedup = results[j].avg_cycles_per_op / results[i].avg_cycles_per_op;
                    printf("  %s:\n", results[i].test_name);
                    printf("    7-tick: %.1f cycles (%.1f ns)\n", 
                           results[i].avg_cycles_per_op, results[i].avg_time_ns_per_op);
                    printf("    49-tick: %.1f cycles (%.1f ns)\n", 
                           results[j].avg_cycles_per_op, results[j].avg_time_ns_per_op);
                    printf("    Speedup: %.1fx faster with 7-tick path\n", speedup);
                    printf("    ✅ 7-tick: %s\n", 
                           results[i].avg_cycles_per_op <= SEVEN_TICK_TARGET_CYCLES ? "Target met" : "Above target");
                    printf("    ✅ 49-tick: %s\n", 
                           results[j].avg_cycles_per_op <= SEVEN_TICK_TARGET_CYCLES ? "Target met" : "Above target");
                    printf("\n");
                    break;
                }
            }
        }
    }
    
    // Export results
    char timestamp[64];
    time_t now = time(NULL);
    strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", localtime(&now));
    
    char json_filename[128];
    snprintf(json_filename, sizeof(json_filename), "cjinja_benchmark_results_%s.json", timestamp);
    benchmark_suite_export_json(suite, json_filename);
    
    char csv_filename[128];
    snprintf(csv_filename, sizeof(csv_filename), "cjinja_benchmark_results_%s.csv", timestamp);
    benchmark_suite_export_csv(suite, csv_filename);
    
    printf("Results exported to:\n");
    printf("  JSON: %s\n", json_filename);
    printf("  CSV: %s\n", csv_filename);
    
    // Cleanup
    cleanup_cjinja_test_data(test_data);
    benchmark_suite_destroy(suite);
    destroy_hardware_info(hw_info);
    
    printf("\nCJinja benchmark suite completed!\n");
    return 0;
} 