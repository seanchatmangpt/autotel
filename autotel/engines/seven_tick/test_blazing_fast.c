#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "cjinja_blazing_fast.h"

void test_basic_operations() {
    printf("=== Basic Operations Test ===\n");
    
    CJinjaBlazingContext* ctx = cjinja_blazing_create_context();
    assert(ctx != NULL);
    
    // Test variable setting and getting
    cjinja_blazing_set_var(ctx, "name", "Alice");
    cjinja_blazing_set_var(ctx, "company", "BlazingCorp");
    cjinja_blazing_set_var(ctx, "role", "Developer");
    
    const char* name = cjinja_blazing_get_var(ctx, "name", 4);
    const char* company = cjinja_blazing_get_var(ctx, "company", 7);
    const char* role = cjinja_blazing_get_var(ctx, "role", 4);
    
    assert(name && strcmp(name, "Alice") == 0);
    assert(company && strcmp(company, "BlazingCorp") == 0);
    assert(role && strcmp(role, "Developer") == 0);
    
    printf("Variables stored correctly:\n");
    printf("  name: %s\n", name);
    printf("  company: %s\n", company);
    printf("  role: %s\n", role);
    printf("  count: %d\n", ctx->count);
    
    cjinja_blazing_destroy_context(ctx);
    printf("✅ Basic operations test passed\n\n");
}

void test_blazing_rendering() {
    printf("=== Blazing Fast Rendering Test ===\n");
    
    CJinjaBlazingContext* ctx = cjinja_blazing_create_context();
    assert(ctx != NULL);
    
    // Set up variables
    cjinja_blazing_set_var(ctx, "user", "Bob");
    cjinja_blazing_set_var(ctx, "service", "BlazingEngine");
    cjinja_blazing_set_var(ctx, "version", "4.0.0");
    cjinja_blazing_set_var(ctx, "status", "blazing");
    
    const char* template = "Hello {{user}}! Welcome to {{service}} v{{version}} - Status: {{status}}";
    
    char* result = cjinja_blazing_render(template, ctx);
    
    printf("Template: %s\n", template);
    printf("Result: %s\n", result ? result : "NULL");
    
    assert(result != NULL);
    assert(strstr(result, "Bob") != NULL);
    assert(strstr(result, "BlazingEngine") != NULL);
    assert(strstr(result, "4.0.0") != NULL);
    assert(strstr(result, "blazing") != NULL);
    
    printf("✅ Blazing fast rendering test passed\n");
    
    free(result);
    cjinja_blazing_destroy_context(ctx);
    printf("\n");
}

void test_stack_optimization() {
    printf("=== Stack Optimization Test ===\n");
    
    CJinjaBlazingContext* ctx = cjinja_blazing_create_context();
    assert(ctx != NULL);
    
    cjinja_blazing_set_var(ctx, "test", "stack");
    
    // Small template that should use stack buffer
    const char* small_template = "Small {{test}} template";
    char* small_result = cjinja_blazing_render(small_template, ctx);
    
    printf("Small template (stack): %s -> %s\n", small_template, small_result ? small_result : "NULL");
    assert(small_result && strstr(small_result, "stack"));
    
    // Large template that should use heap buffer
    char large_template[600];
    snprintf(large_template, sizeof(large_template), 
        "This is a very long template that exceeds the stack buffer size and should trigger heap allocation. "
        "It contains a variable {{test}} that should still be substituted correctly. "
        "The template is intentionally long to test the heap allocation path in the blazing fast implementation. "
        "This helps ensure that both code paths work correctly and efficiently. "
        "More text to make it even longer and definitely exceed 400 characters...");
    
    char* large_result = cjinja_blazing_render(large_template, ctx);
    printf("Large template (heap): ...%s (truncated)\n", large_result && strlen(large_result) > 50 ? large_result + strlen(large_result) - 50 : "NULL");
    assert(large_result && strstr(large_result, "stack"));
    
    printf("✅ Stack optimization test passed\n");
    
    free(small_result);
    free(large_result);
    cjinja_blazing_destroy_context(ctx);
    printf("\n");
}

void test_edge_cases() {
    printf("=== Edge Cases Test ===\n");
    
    CJinjaBlazingContext* ctx = cjinja_blazing_create_context();
    assert(ctx != NULL);
    
    // Test empty template
    char* result1 = cjinja_blazing_render("", ctx);
    assert(result1 && strlen(result1) == 0);
    free(result1);
    
    // Test template with no variables
    char* result2 = cjinja_blazing_render("No variables here!", ctx);
    assert(result2 && strcmp(result2, "No variables here!") == 0);
    free(result2);
    
    // Test missing variable
    char* result3 = cjinja_blazing_render("Missing: {{unknown}}", ctx);
    assert(result3 && strcmp(result3, "Missing: ") == 0);
    free(result3);
    
    // Test consecutive variables
    cjinja_blazing_set_var(ctx, "a", "X");
    cjinja_blazing_set_var(ctx, "b", "Y");
    char* result4 = cjinja_blazing_render("{{a}}{{b}}", ctx);
    assert(result4 && strcmp(result4, "XY") == 0);
    free(result4);
    
    // Test malformed variable
    char* result5 = cjinja_blazing_render("Malformed {{var", ctx);
    assert(result5 && strcmp(result5, "Malformed ") == 0);
    free(result5);
    
    printf("✅ Edge cases test passed\n");
    
    cjinja_blazing_destroy_context(ctx);
    printf("\n");
}

void test_performance_limits() {
    printf("=== Performance Limits Test ===\n");
    
    CJinjaBlazingContext* ctx = cjinja_blazing_create_context();
    assert(ctx != NULL);
    
    // Test maximum variables
    for (int i = 0; i < MAX_VARIABLES; i++) {
        char key[16], value[32];
        snprintf(key, sizeof(key), "var%d", i);
        snprintf(value, sizeof(value), "value%d", i);
        cjinja_blazing_set_var(ctx, key, value);
    }
    
    printf("Added %d variables (max capacity)\n", MAX_VARIABLES);
    assert(ctx->count == MAX_VARIABLES);
    
    // Test retrieval of first, middle, and last
    const char* first = cjinja_blazing_get_var(ctx, "var0", 4);
    const char* middle = cjinja_blazing_get_var(ctx, "var15", 5);
    const char* last = cjinja_blazing_get_var(ctx, "var31", 5);
    
    assert(first && strcmp(first, "value0") == 0);
    assert(middle && strcmp(middle, "value15") == 0);
    assert(last && strcmp(last, "value31") == 0);
    
    // Test rendering with multiple variables
    char* result = cjinja_blazing_render("First: {{var0}}, Middle: {{var15}}, Last: {{var31}}", ctx);
    assert(result && strstr(result, "value0") && strstr(result, "value15") && strstr(result, "value31"));
    
    printf("Multi-variable rendering: %s\n", result);
    printf("✅ Performance limits test passed\n");
    
    free(result);
    cjinja_blazing_destroy_context(ctx);
    printf("\n");
}

void test_blazing_benchmark() {
    printf("=== Blazing Fast Benchmark ===\n");
    
    printf("Running intensive benchmark (50,000 iterations)...\n");
    CJinjaBlazingBenchmark result = cjinja_blazing_benchmark(50000);
    
    printf("\nBenchmark Results:\n");
    printf("  Iterations: %llu\n", result.iterations);
    printf("  Average time: %llu ns\n", result.avg_time_ns);
    printf("  Min time: %llu ns\n", result.min_time_ns);
    printf("  Max time: %llu ns\n", result.max_time_ns);
    printf("  Operations/second: %.0f\n", result.ops_per_second);
    
    // Performance analysis
    printf("\nPerformance Analysis:\n");
    if (result.avg_time_ns < 100) {
        printf("  🎯 TARGET ACHIEVED: Sub-100ns variable substitution!\n");
        printf("  🚀 Speedup vs 206ns baseline: %.2fx\n", 206.0 / result.avg_time_ns);
        printf("  ⚡ Speedup vs 272ns hash table: %.2fx\n", 272.0 / result.avg_time_ns);
        printf("  💎 Performance class: BLAZING FAST\n");
    } else if (result.avg_time_ns < 150) {
        printf("  ⚡ EXCELLENT: Sub-150ns performance\n");
        printf("  🚀 Speedup vs 206ns baseline: %.2fx\n", 206.0 / result.avg_time_ns);
        printf("  ⚡ Speedup vs 272ns hash table: %.2fx\n", 272.0 / result.avg_time_ns);
        printf("  ⭐ Performance class: VERY FAST\n");
    } else if (result.avg_time_ns < 206) {
        printf("  ✅ IMPROVED: Faster than original baseline\n");
        printf("  🚀 Speedup vs 206ns baseline: %.2fx\n", 206.0 / result.avg_time_ns);
        printf("  ⚡ Speedup vs 272ns hash table: %.2fx\n", 272.0 / result.avg_time_ns);
        printf("  📈 Performance class: FAST\n");
    } else {
        printf("  ⚠️ Still needs more optimization\n");
        printf("  📊 Ratio vs 206ns baseline: %.2fx slower\n", result.avg_time_ns / 206.0);
        if (result.avg_time_ns < 272) {
            printf("  ✅ But faster than hash table: %.2fx\n", 272.0 / result.avg_time_ns);
        }
    }
    
    printf("✅ Blazing benchmark completed\n\n");
}

int main() {
    printf("🔥 CJinja Blazing Fast Test Suite\n");
    printf("==================================\n\n");
    
    test_basic_operations();
    test_blazing_rendering();
    test_stack_optimization();
    test_edge_cases();
    test_performance_limits();
    test_blazing_benchmark();
    
    printf("🎉 ALL TESTS COMPLETED!\n\n");
    
    // Run comprehensive comparison
    cjinja_blazing_benchmark_comparison();
    
    return 0;
}