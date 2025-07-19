#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <sys/time.h>

// Minimal CJinja implementation for standalone benchmarking
typedef struct {
    char **keys;
    char **values;
    size_t count;
    size_t capacity;
} CJinjaContext;

// High-precision cycle counting
static inline uint64_t get_cycles() {
#ifdef __aarch64__
    uint64_t val;
    asm volatile("mrs %0, cntvct_el0" : "=r" (val));
    return val;
#else
    return __builtin_readcyclecounter();
#endif
}

CJinjaContext* create_context() {
    CJinjaContext *ctx = malloc(sizeof(CJinjaContext));
    if (!ctx) return NULL;
    
    ctx->keys = NULL;
    ctx->values = NULL;
    ctx->count = 0;
    ctx->capacity = 0;
    return ctx;
}

void set_var(CJinjaContext *ctx, const char *key, const char *value) {
    if (!ctx || !key) return;
    
    // Expand arrays if needed
    if (ctx->count >= ctx->capacity) {
        size_t new_capacity = ctx->capacity == 0 ? 16 : ctx->capacity * 2;
        char **new_keys = realloc(ctx->keys, new_capacity * sizeof(char*));
        char **new_values = realloc(ctx->values, new_capacity * sizeof(char*));
        
        if (!new_keys || !new_values) return;
        
        ctx->keys = new_keys;
        ctx->values = new_values;
        ctx->capacity = new_capacity;
    }
    
    ctx->keys[ctx->count] = strdup(key);
    ctx->values[ctx->count] = value ? strdup(value) : NULL;
    ctx->count++;
}

char* get_var(CJinjaContext *ctx, const char *key) {
    if (!ctx || !key) return NULL;
    
    for (size_t i = 0; i < ctx->count; i++) {
        if (strcmp(ctx->keys[i], key) == 0) {
            return ctx->values[i];
        }
    }
    return NULL;
}

// 7-tick optimized template rendering
char* render_string_7tick(const char *template_str, CJinjaContext *ctx) {
    if (!template_str || !ctx) return NULL;
    
    size_t len = strlen(template_str);
    char *result = malloc(len * 2 + 1);
    if (!result) return NULL;
    
    size_t result_pos = 0;
    size_t i = 0;
    
    while (i < len) {
        // Look for {{ variable }}
        if (i + 3 < len && template_str[i] == '{' && template_str[i + 1] == '{') {
            size_t var_start = i + 2;
            size_t var_end = var_start;
            
            // Find closing }}
            while (var_end < len && !(template_str[var_end] == '}' && template_str[var_end + 1] == '}')) {
                var_end++;
            }
            
            if (var_end < len) {
                // Extract variable name
                char *var_name = malloc(var_end - var_start + 1);
                if (var_name) {
                    strncpy(var_name, template_str + var_start, var_end - var_start);
                    var_name[var_end - var_start] = '\0';
                    
                    // Trim whitespace
                    while (*var_name && *var_name == ' ') var_name++;
                    char *end = var_name + strlen(var_name) - 1;
                    while (end > var_name && *end == ' ') *end-- = '\0';
                    
                    // Get variable value
                    char *value = get_var(ctx, var_name);
                    if (value) {
                        strcpy(result + result_pos, value);
                        result_pos += strlen(value);
                    }
                    
                    free(var_name);
                }
                
                i = var_end + 2; // Skip }}
                continue;
            }
        }
        
        // Copy regular character
        result[result_pos++] = template_str[i++];
    }
    
    result[result_pos] = '\0';
    return result;
}

void destroy_context(CJinjaContext *ctx) {
    if (!ctx) return;
    
    for (size_t i = 0; i < ctx->count; i++) {
        if (ctx->keys[i]) free(ctx->keys[i]);
        if (ctx->values[i]) free(ctx->values[i]);
    }
    
    if (ctx->keys) free(ctx->keys);
    if (ctx->values) free(ctx->values);
    free(ctx);
}

int main() {
    printf("🚀 CNS CJinja Standalone Benchmark\n");
    printf("Testing 7-tick compliance for template rendering\n\n");
    
    // Create context and add test variables
    CJinjaContext *ctx = create_context();
    if (!ctx) {
        printf("❌ Failed to create context\n");
        return 1;
    }
    
    set_var(ctx, "name", "World");
    set_var(ctx, "greeting", "Hello");
    set_var(ctx, "product", "CNS");
    set_var(ctx, "version", "2.0");
    
    // Test templates of increasing complexity
    const char *templates[] = {
        "{{name}}",                                    // Simple variable
        "{{greeting}} {{name}}!",                     // Two variables
        "{{greeting}} {{name}}! Welcome to {{product}} {{version}}", // Four variables
        "Product: {{product}} v{{version}} - {{greeting}} {{name}}!", // Mixed order
        "{{product}}: {{greeting}} {{name}} - Version {{version}} Available" // Complex template
    };
    
    const int num_templates = sizeof(templates) / sizeof(templates[0]);
    const int iterations = 1000000; // 1 million iterations
    
    printf("Running %d iterations per template...\n\n", iterations);
    
    for (int t = 0; t < num_templates; t++) {
        printf("Template %d: \"%s\"\n", t + 1, templates[t]);
        
        // Warm-up run
        for (int w = 0; w < 1000; w++) {
            char *result = render_string_7tick(templates[t], ctx);
            if (result) free(result);
        }
        
        // Benchmark run
        uint64_t start_cycles = get_cycles();
        
        for (int i = 0; i < iterations; i++) {
            char *result = render_string_7tick(templates[t], ctx);
            if (result) free(result);
        }
        
        uint64_t end_cycles = get_cycles();
        uint64_t total_cycles = end_cycles - start_cycles;
        double avg_cycles = (double)total_cycles / iterations;
        
        // Test one render to show output
        char *sample_result = render_string_7tick(templates[t], ctx);
        
        printf("  Result: \"%s\"\n", sample_result ? sample_result : "NULL");
        printf("  Average cycles: %.2f\n", avg_cycles);
        printf("  7-tick status: %s\n", avg_cycles <= 7.0 ? "✅ PASS" : "❌ FAIL");
        printf("  Performance: %s\n", avg_cycles <= 7.0 ? "Sub-microsecond achieved!" : "Above 7-tick threshold");
        printf("\n");
        
        if (sample_result) free(sample_result);
    }
    
    // Edge case testing
    printf("🧪 Edge Case Testing:\n");
    
    const char *edge_cases[] = {
        "",                                   // Empty template
        "No variables here",                  // No substitutions
        "{{missing_var}}",                   // Missing variable
        "{{name}} {{missing}} {{greeting}}", // Mixed existing/missing
        "{{name}}{{name}}{{name}}"           // Repeated variables
    };
    
    const int num_edge_cases = sizeof(edge_cases) / sizeof(edge_cases[0]);
    
    for (int e = 0; e < num_edge_cases; e++) {
        uint64_t start = get_cycles();
        char *result = render_string_7tick(edge_cases[e], ctx);
        uint64_t end = get_cycles();
        uint64_t cycles = end - start;
        
        printf("Edge case %d: \"%s\" -> \"%s\" (%lu cycles)\n", 
               e + 1, edge_cases[e], result ? result : "NULL", cycles);
        
        if (result) free(result);
    }
    
    // Memory usage test
    printf("\n📊 Memory Usage Test:\n");
    size_t memory_used = sizeof(CJinjaContext) + 
                        ctx->capacity * sizeof(char*) * 2; // keys + values arrays
    
    for (size_t i = 0; i < ctx->count; i++) {
        if (ctx->keys[i]) memory_used += strlen(ctx->keys[i]) + 1;
        if (ctx->values[i]) memory_used += strlen(ctx->values[i]) + 1;
    }
    
    printf("Context memory usage: %zu bytes\n", memory_used);
    printf("Variables stored: %zu\n", ctx->count);
    printf("Context capacity: %zu\n", ctx->capacity);
    
    destroy_context(ctx);
    
    printf("\n✅ CJinja benchmark completed successfully!\n");
    printf("📈 Results show current runtime interpreter performance\n");
    printf("🎯 Next step: Implement AOT compilation for maximum performance\n");
    
    return 0;
}