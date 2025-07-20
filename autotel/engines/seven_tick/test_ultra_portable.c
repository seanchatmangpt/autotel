#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "cjinja_ultra_portable.h"

void test_hash_function() {
    printf("=== Hash Function Test ===\n");
    
    // Test hash consistency
    uint32_t hash1 = cjinja_ultra_hash("test", 4);
    uint32_t hash2 = cjinja_ultra_hash("test", 4);
    assert(hash1 == hash2);
    
    // Test different strings produce different hashes
    uint32_t hash3 = cjinja_ultra_hash("different", 9);
    assert(hash1 != hash3);
    
    // Test empty string
    uint32_t hash4 = cjinja_ultra_hash("", 0);
    
    printf("Hash function working correctly:\n");
    printf("  'test' -> %u\n", hash1);
    printf("  'different' -> %u\n", hash3);
    printf("  '' -> %u\n", hash4);
    printf("✅ Hash function test passed\n\n");
}

void test_variable_operations() {
    printf("=== Variable Operations Test ===\n");
    
    CJinjaUltraContext* ctx = cjinja_ultra_create_context();
    assert(ctx != NULL);
    
    // Test setting and getting variables
    cjinja_ultra_set_var(ctx, "name", "Alice");
    cjinja_ultra_set_var(ctx, "company", "UltraCorp");
    cjinja_ultra_set_var(ctx, "role", "Developer");
    
    const char* name = cjinja_ultra_get_var(ctx, "name");
    const char* company = cjinja_ultra_get_var(ctx, "company");
    const char* role = cjinja_ultra_get_var(ctx, "role");
    
    assert(name && strcmp(name, "Alice") == 0);
    assert(company && strcmp(company, "UltraCorp") == 0);
    assert(role && strcmp(role, "Developer") == 0);
    
    // Test non-existent variable
    const char* missing = cjinja_ultra_get_var(ctx, "nonexistent");
    assert(missing == NULL);
    
    // Test variable update
    cjinja_ultra_set_var(ctx, "name", "Bob");
    const char* updated_name = cjinja_ultra_get_var(ctx, "name");
    assert(updated_name && strcmp(updated_name, "Bob") == 0);
    
    printf("Variable operations working correctly:\n");
    printf("  name: %s\n", cjinja_ultra_get_var(ctx, "name"));
    printf("  company: %s\n", cjinja_ultra_get_var(ctx, "company"));
    printf("  role: %s\n", cjinja_ultra_get_var(ctx, "role"));
    printf("✅ Variable operations test passed\n");
    
    cjinja_ultra_destroy_context(ctx);
    printf("\n");
}

void test_ultra_fast_rendering() {
    printf("=== Ultra-Fast Rendering Test ===\n");
    
    CJinjaUltraContext* ctx = cjinja_ultra_create_context();
    assert(ctx != NULL);
    
    // Set up variables
    cjinja_ultra_set_var(ctx, "user", "Charlie");
    cjinja_ultra_set_var(ctx, "service", "FastEngine");
    cjinja_ultra_set_var(ctx, "version", "3.0.0");
    cjinja_ultra_set_var(ctx, "status", "active");
    
    const char* template = "Welcome {{user}} to {{service}} v{{version}}! Status: {{status}}";
    
    char* result = cjinja_ultra_render_variables(template, ctx);
    
    printf("Template: %s\n", template);
    printf("Result: %s\n", result ? result : "NULL");
    
    assert(result != NULL);
    assert(strstr(result, "Charlie") != NULL);
    assert(strstr(result, "FastEngine") != NULL);
    assert(strstr(result, "3.0.0") != NULL);
    assert(strstr(result, "active") != NULL);
    
    printf("✅ Ultra-fast rendering test passed\n");
    
    free(result);
    cjinja_ultra_destroy_context(ctx);
    printf("\n");
}

void test_performance_stats() {
    printf("=== Performance Statistics Test ===\n");
    
    CJinjaUltraContext* ctx = cjinja_ultra_create_context();
    assert(ctx != NULL);
    
    // Add variables to generate some stats
    cjinja_ultra_set_var(ctx, "var1", "value1");
    cjinja_ultra_set_var(ctx, "var2", "value2");
    cjinja_ultra_set_var(ctx, "var3", "value3");
    
    // Perform lookups
    for (int i = 0; i < 10; i++) {
        cjinja_ultra_get_var(ctx, "var1");
        cjinja_ultra_get_var(ctx, "var2");
        cjinja_ultra_get_var(ctx, "var3");
        cjinja_ultra_get_var(ctx, "missing"); // This will increment lookup count
    }
    
    CJinjaUltraStats stats;
    cjinja_ultra_get_stats(ctx, &stats);
    
    printf("Performance statistics:\n");
    printf("  Hash lookups: %lu\n", stats.hash_lookups);
    printf("  Hash collisions: %lu\n", stats.hash_collisions);
    printf("  Collision rate: %.2f%%\n", stats.collision_rate * 100);
    printf("  Memory used: %zu bytes\n", stats.memory_pool_used);
    
    assert(stats.hash_lookups > 0);
    
    printf("✅ Performance statistics test passed\n");
    
    cjinja_ultra_destroy_context(ctx);
    printf("\n");
}

void test_edge_cases() {
    printf("=== Edge Cases Test ===\n");
    
    CJinjaUltraContext* ctx = cjinja_ultra_create_context();
    assert(ctx != NULL);
    
    // Test empty template
    char* result1 = cjinja_ultra_render_variables("", ctx);
    assert(result1 && strlen(result1) == 0);
    free(result1);
    
    // Test template with no variables
    char* result2 = cjinja_ultra_render_variables("Hello World!", ctx);
    assert(result2 && strcmp(result2, "Hello World!") == 0);
    free(result2);
    
    // Test malformed variable (missing closing braces)
    char* result3 = cjinja_ultra_render_variables("Hello {{name", ctx);
    assert(result3 && strcmp(result3, "Hello ") == 0);
    free(result3);
    
    // Test empty variable name
    char* result4 = cjinja_ultra_render_variables("Hello {{}}", ctx);
    assert(result4 && strcmp(result4, "Hello ") == 0);
    free(result4);
    
    // Test consecutive variables
    cjinja_ultra_set_var(ctx, "a", "X");
    cjinja_ultra_set_var(ctx, "b", "Y");
    char* result5 = cjinja_ultra_render_variables("{{a}}{{b}}", ctx);
    assert(result5 && strcmp(result5, "XY") == 0);
    free(result5);
    
    printf("✅ Edge cases test passed\n");
    
    cjinja_ultra_destroy_context(ctx);
    printf("\n");
}

void test_memory_efficiency() {
    printf("=== Memory Efficiency Test ===\n");
    
    CJinjaUltraContext* ctx = cjinja_ultra_create_context();
    assert(ctx != NULL);
    
    // Add many variables
    for (int i = 0; i < 100; i++) {
        char key[32], value[64];
        snprintf(key, sizeof(key), "key_%d", i);
        snprintf(value, sizeof(value), "value_for_key_%d", i);
        cjinja_ultra_set_var(ctx, key, value);
    }
    
    // Test retrieval of first, middle, and last variables
    const char* first = cjinja_ultra_get_var(ctx, "key_0");
    const char* middle = cjinja_ultra_get_var(ctx, "key_50");
    const char* last = cjinja_ultra_get_var(ctx, "key_99");
    
    assert(first && strstr(first, "value_for_key_0"));
    assert(middle && strstr(middle, "value_for_key_50"));
    assert(last && strstr(last, "value_for_key_99"));
    
    CJinjaUltraStats stats;
    cjinja_ultra_get_stats(ctx, &stats);
    
    printf("Memory efficiency with 100 variables:\n");
    printf("  Memory used: %zu bytes\n", stats.memory_pool_used);
    printf("  Hash collisions: %lu\n", stats.hash_collisions);
    printf("  Collision rate: %.2f%%\n", stats.collision_rate * 100);
    
    printf("✅ Memory efficiency test passed\n");
    
    cjinja_ultra_destroy_context(ctx);
    printf("\n");
}

void test_performance_benchmark() {
    printf("=== Performance Benchmark ===\n");
    
    printf("Running ultra-fast performance benchmark...\n");
    CJinjaBenchmarkResult result = cjinja_ultra_benchmark_variables(5000);
    
    printf("\nBenchmark Results (5,000 iterations):\n");
    printf("  Average time: %lu ns\n", result.avg_time_ns);
    printf("  Min time: %lu ns\n", result.min_time_ns);
    printf("  Max time: %lu ns\n", result.max_time_ns);
    printf("  Operations/second: %.0f\n", result.ops_per_second);
    printf("  Total time: %.3f ms\n", result.total_time_ns / 1000000.0);
    
    // Performance analysis
    if (result.avg_time_ns < 100) {
        printf("  🎯 TARGET ACHIEVED: Sub-100ns variable substitution!\n");
        printf("  🚀 Speedup vs 206ns baseline: %.2fx\n", 206.0 / result.avg_time_ns);
        printf("  💎 Performance class: ULTRA-FAST\n");
    } else if (result.avg_time_ns < 150) {
        printf("  ⚡ EXCELLENT: Sub-150ns performance\n");
        printf("  🚀 Speedup vs 206ns baseline: %.2fx\n", 206.0 / result.avg_time_ns);
        printf("  ⭐ Performance class: VERY FAST\n");
    } else if (result.avg_time_ns < 206) {
        printf("  📈 IMPROVED: Faster than baseline\n");
        printf("  🚀 Speedup vs 206ns baseline: %.2fx\n", 206.0 / result.avg_time_ns);
        printf("  ✅ Performance class: FAST\n");
    } else {
        printf("  ⚠️ Performance needs optimization\n");
        printf("  📊 Ratio vs 206ns baseline: %.2fx\n", result.avg_time_ns / 206.0);
    }
    
    printf("✅ Performance benchmark completed\n\n");
}

int main() {
    printf("🚀 CJinja Ultra-Fast Portable Test Suite\n");
    printf("=========================================\n\n");
    
    test_hash_function();
    test_variable_operations();
    test_ultra_fast_rendering();
    test_performance_stats();
    test_edge_cases();
    test_memory_efficiency();
    test_performance_benchmark();
    
    printf("🎉 ALL TESTS COMPLETED SUCCESSFULLY!\n\n");
    
    // Run comprehensive benchmark comparison
    cjinja_ultra_benchmark_comparison();
    
    return 0;
}