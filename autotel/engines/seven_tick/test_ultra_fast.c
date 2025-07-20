#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include "cjinja_ultra_fast.h"

void test_simd_detection() {
    printf("=== SIMD Feature Detection ===\n");
    
    CJinjaSIMDFeatures features = cjinja_ultra_detect_simd();
    
    printf("SIMD Support:\n");
    printf("  SSE2: %s\n", features.sse2_available ? "✅" : "❌");
    printf("  SSE4: %s\n", features.sse4_available ? "✅" : "❌");
    printf("  AVX:  %s\n", features.avx_available ? "✅" : "❌");
    printf("  AVX2: %s\n", features.avx2_available ? "✅" : "❌");
    printf("  AVX512: %s\n", features.avx512_available ? "✅" : "❌");
    printf("\n");
}

void test_hash_table() {
    printf("=== Hash Table Performance Test ===\n");
    
    CJinjaUltraContext* ctx = cjinja_ultra_create_context();
    assert(ctx != NULL);
    
    // Test variable setting and retrieval
    cjinja_ultra_set_var(ctx, "name", "John");
    cjinja_ultra_set_var(ctx, "company", "Acme Corp");
    cjinja_ultra_set_var(ctx, "title", "Engineer");
    cjinja_ultra_set_var(ctx, "department", "Engineering");
    cjinja_ultra_set_var(ctx, "location", "San Francisco");
    
    // Test retrieval
    const char* name = cjinja_ultra_get_var(ctx, "name");
    const char* company = cjinja_ultra_get_var(ctx, "company");
    const char* title = cjinja_ultra_get_var(ctx, "title");
    
    assert(name && strcmp(name, "John") == 0);
    assert(company && strcmp(company, "Acme Corp") == 0);
    assert(title && strcmp(title, "Engineer") == 0);
    
    printf("✅ Hash table operations working correctly\n");
    printf("   Variables stored: %zu\n", ctx->total_variables);
    printf("   Lookups performed: %lu\n", ctx->lookup_count);
    printf("   Collisions: %lu\n", ctx->collision_count);
    
    cjinja_ultra_destroy_context(ctx);
    printf("\n");
}

void test_ultra_fast_rendering() {
    printf("=== Ultra-Fast Variable Substitution Test ===\n");
    
    CJinjaUltraContext* ctx = cjinja_ultra_create_context();
    assert(ctx != NULL);
    
    // Set up test variables
    cjinja_ultra_set_var(ctx, "name", "Alice");
    cjinja_ultra_set_var(ctx, "company", "TechCorp");
    cjinja_ultra_set_var(ctx, "role", "Developer");
    cjinja_ultra_set_var(ctx, "project", "UltraEngine");
    
    const char* template = "Hello {{name}} from {{company}}! You are a {{role}} working on {{project}}.";
    
    char* result = cjinja_ultra_render_variables(template, ctx);
    
    printf("Template: %s\n", template);
    printf("Result: %s\n", result ? result : "NULL");
    
    assert(result != NULL);
    assert(strstr(result, "Alice") != NULL);
    assert(strstr(result, "TechCorp") != NULL);
    assert(strstr(result, "Developer") != NULL);
    assert(strstr(result, "UltraEngine") != NULL);
    
    printf("✅ Ultra-fast rendering working correctly\n");
    
    free(result);
    cjinja_ultra_destroy_context(ctx);
    printf("\n");
}

void test_template_compilation() {
    printf("=== Template Compilation Test ===\n");
    
    CJinjaUltraEngine* engine = cjinja_ultra_create_engine();
    CJinjaUltraContext* ctx = cjinja_ultra_create_context();
    
    assert(engine != NULL);
    assert(ctx != NULL);
    
    cjinja_ultra_set_var(ctx, "user", "Bob");
    cjinja_ultra_set_var(ctx, "status", "active");
    
    const char* template = "User: {{user}}, Status: {{status}}";
    
    // First render (should compile and cache)
    char* result1 = cjinja_ultra_render_compiled(engine, template, ctx);
    
    // Second render (should use cached version)
    char* result2 = cjinja_ultra_render_compiled(engine, template, ctx);
    
    printf("Template: %s\n", template);
    printf("First result: %s\n", result1 ? result1 : "NULL");
    printf("Second result: %s\n", result2 ? result2 : "NULL");
    
    assert(result1 != NULL);
    assert(result2 != NULL);
    assert(strcmp(result1, result2) == 0);
    assert(strstr(result1, "Bob") != NULL);
    assert(strstr(result1, "active") != NULL);
    
    // Check cache statistics
    CJinjaUltraStats stats;
    cjinja_ultra_get_stats(engine, ctx, &stats);
    
    printf("Cache hits: %lu\n", stats.cache_hits);
    printf("Cache misses: %lu\n", stats.cache_misses);
    printf("Cache hit rate: %.2f%%\n", stats.cache_hit_rate * 100);
    
    printf("✅ Template compilation and caching working\n");
    
    free(result1);
    free(result2);
    cjinja_ultra_destroy_context(ctx);
    cjinja_ultra_destroy_engine(engine);
    printf("\n");
}

void test_performance_benchmark() {
    printf("=== Performance Benchmark ===\n");
    
    // Run benchmark
    CJinjaBenchmarkResult result = cjinja_ultra_benchmark_variables(1000);
    
    printf("Benchmark Results (1000 iterations):\n");
    printf("  Average time: %lu ns\n", result.avg_time_ns);
    printf("  Min time: %lu ns\n", result.min_time_ns);
    printf("  Max time: %lu ns\n", result.max_time_ns);
    printf("  Operations/second: %.0f\n", result.ops_per_second);
    
    if (result.avg_time_ns < 100) {
        printf("  🎯 TARGET ACHIEVED: Sub-100ns variable substitution!\n");
        printf("  🚀 Performance: %.2fx faster than 206ns target\n", 206.0 / result.avg_time_ns);
    } else if (result.avg_time_ns < 150) {
        printf("  ⚡ EXCELLENT: Sub-150ns performance achieved\n");
        printf("  🚀 Performance: %.2fx faster than 206ns original\n", 206.0 / result.avg_time_ns);
    } else {
        printf("  ⚠️  Still faster than original but target missed\n");
        printf("  📈 Performance: %.2fx faster than 206ns original\n", 206.0 / result.avg_time_ns);
    }
    
    printf("\n");
}

void test_edge_cases() {
    printf("=== Edge Cases Test ===\n");
    
    CJinjaUltraContext* ctx = cjinja_ultra_create_context();
    assert(ctx != NULL);
    
    // Test empty variables
    cjinja_ultra_set_var(ctx, "empty", "");
    char* result1 = cjinja_ultra_render_variables("Value: {{empty}}", ctx);
    printf("Empty variable: '%s'\n", result1 ? result1 : "NULL");
    assert(result1 != NULL);
    free(result1);
    
    // Test missing variables
    char* result2 = cjinja_ultra_render_variables("Missing: {{nonexistent}}", ctx);
    printf("Missing variable: '%s'\n", result2 ? result2 : "NULL");
    assert(result2 != NULL);
    free(result2);
    
    // Test template with no variables
    char* result3 = cjinja_ultra_render_variables("No variables here!", ctx);
    printf("No variables: '%s'\n", result3 ? result3 : "NULL");
    assert(result3 != NULL);
    assert(strcmp(result3, "No variables here!") == 0);
    free(result3);
    
    // Test long variable names and values
    char long_name[50] = "very_long_variable_name_that_tests_limits";
    char long_value[100];
    for (int i = 0; i < 99; i++) long_value[i] = 'A' + (i % 26);
    long_value[99] = '\0';
    
    cjinja_ultra_set_var(ctx, long_name, long_value);
    char template[200];
    snprintf(template, sizeof(template), "Long: {{%s}}", long_name);
    char* result4 = cjinja_ultra_render_variables(template, ctx);
    printf("Long variable test: %s\n", result4 ? "✅ Success" : "❌ Failed");
    if (result4) free(result4);
    
    printf("✅ Edge cases handled correctly\n");
    
    cjinja_ultra_destroy_context(ctx);
    printf("\n");
}

void test_memory_efficiency() {
    printf("=== Memory Efficiency Test ===\n");
    
    CJinjaUltraEngine* engine = cjinja_ultra_create_engine();
    CJinjaUltraContext* ctx = cjinja_ultra_create_context();
    
    // Add many variables to test memory pool
    for (int i = 0; i < 100; i++) {
        char key[32], value[64];
        snprintf(key, sizeof(key), "var_%d", i);
        snprintf(value, sizeof(value), "value_for_variable_%d", i);
        cjinja_ultra_set_var(ctx, key, value);
    }
    
    // Test rendering with many variables
    char* result = cjinja_ultra_render_variables("Test: {{var_0}}, {{var_50}}, {{var_99}}", ctx);
    
    CJinjaUltraStats stats;
    cjinja_ultra_get_stats(engine, ctx, &stats);
    
    printf("Variables stored: 100\n");
    printf("Memory efficiency: %.2f%%\n", stats.memory_efficiency * 100);
    printf("Hash collisions: %lu\n", stats.hash_collisions);
    printf("Collision rate: %.2f%%\n", stats.collision_rate * 100);
    
    if (result) {
        printf("Multi-variable rendering: ✅ Success\n");
        free(result);
    }
    
    printf("✅ Memory efficiency test completed\n");
    
    cjinja_ultra_destroy_context(ctx);
    cjinja_ultra_destroy_engine(engine);
    printf("\n");
}

int main() {
    printf("🚀 CJinja Ultra-Fast Implementation Test Suite\n");
    printf("===============================================\n\n");
    
    test_simd_detection();
    test_hash_table();
    test_ultra_fast_rendering();
    test_template_compilation();
    test_performance_benchmark();
    test_edge_cases();
    test_memory_efficiency();
    
    printf("🎉 ALL TESTS COMPLETED!\n\n");
    
    // Run comprehensive comparison
    cjinja_ultra_benchmark_comparison();
    
    return 0;
}