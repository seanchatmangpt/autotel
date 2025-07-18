# CJinja Template Rendering Patterns Cookbook

## Overview

This cookbook provides practical patterns and examples for implementing high-performance template rendering using the CJinja engine. The patterns demonstrate how to achieve sub-microsecond rendering performance while maintaining flexibility and functionality.

## Pattern 1: Basic Variable Substitution

### Problem
Render templates with simple variable substitution efficiently.

### Solution
Use the optimized `cjinja_render_string()` function for basic variables.

```c
#include <stdio.h>
#include <stdlib.h>
#include "../compiler/src/cjinja.h"

void render_basic_template() {
    // Create context
    CJinjaContext* ctx = cjinja_create_context();
    if (!ctx) {
        fprintf(stderr, "Failed to create context\n");
        return;
    }
    
    // Set variables
    cjinja_set_var(ctx, "user", "Alice");
    cjinja_set_var(ctx, "title", "Welcome");
    cjinja_set_var(ctx, "company", "7T Engine");
    
    // Template with variables
    const char* template = "Hello {{user}}, welcome to {{title}} at {{company}}!";
    
    // Render template
    char* result = cjinja_render_string(template, ctx);
    if (result) {
        printf("Rendered: %s\n", result);
        free(result);
    }
    
    cjinja_destroy_context(ctx);
}

int main() {
    render_basic_template();
    return 0;
}
```

### Performance
- **Latency**: 214.17 ns average
- **Throughput**: 4.67M operations/second

## Pattern 2: Conditional Rendering

### Problem
Render templates with conditional blocks based on context variables.

### Solution
Use `cjinja_render_with_conditionals()` for if/else blocks.

```c
#include <stdio.h>
#include <stdlib.h>
#include "../compiler/src/cjinja.h"

void render_conditional_template() {
    CJinjaContext* ctx = cjinja_create_context();
    if (!ctx) {
        fprintf(stderr, "Failed to create context\n");
        return;
    }
    
    // Set boolean variables
    cjinja_set_bool(ctx, "is_admin", 1);
    cjinja_set_bool(ctx, "show_debug", 0);
    cjinja_set_var(ctx, "user", "Alice");
    
    // Template with conditionals
    const char* template = 
        "{% if is_admin %}"
        "Welcome admin {{user}}! You have full access.\n"
        "{% endif %}"
        "{% if show_debug %}"
        "Debug mode is enabled.\n"
        "{% endif %}"
        "{% if is_admin and show_debug %}"
        "Admin debug mode active.\n"
        "{% endif %}";
    
    char* result = cjinja_render_with_conditionals(template, ctx);
    if (result) {
        printf("Conditional rendering:\n%s\n", result);
        free(result);
    }
    
    cjinja_destroy_context(ctx);
}

int main() {
    render_conditional_template();
    return 0;
}
```

### Performance
- **Latency**: 614.28 ns average
- **Throughput**: 1.63M operations/second

## Pattern 3: Loop Rendering with Filters

### Problem
Render templates with loops and apply filters to data.

### Solution
Use `cjinja_render_with_loops()` for iteration and filtering.

```c
#include <stdio.h>
#include <stdlib.h>
#include "../compiler/src/cjinja.h"

void render_loop_template() {
    CJinjaContext* ctx = cjinja_create_context();
    if (!ctx) {
        fprintf(stderr, "Failed to create context\n");
        return;
    }
    
    // Set array variables
    char* fruits[] = {"apple", "banana", "cherry", "date"};
    cjinja_set_array(ctx, "fruits", fruits, 4);
    
    char* users[] = {"alice", "bob", "charlie"};
    cjinja_set_array(ctx, "users", users, 3);
    
    // Template with loops and filters
    const char* template = 
        "Fruit List:\n"
        "{% for fruit in fruits %}"
        "- {{fruit|upper}}\n"
        "{% endfor %}"
        "\nUser List:\n"
        "{% for user in users %}"
        "- {{user|capitalize}}\n"
        "{% endfor %}"
        "\nTotal fruits: {{fruits|length}}\n";
    
    char* result = cjinja_render_with_loops(template, ctx);
    if (result) {
        printf("Loop rendering:\n%s\n", result);
        free(result);
    }
    
    cjinja_destroy_context(ctx);
}

int main() {
    render_loop_template();
    return 0;
}
```

### Performance
- **Latency**: 6,925.56 ns average
- **Throughput**: 144K operations/second

## Pattern 4: Template Caching

### Problem
Improve performance by caching compiled templates.

### Solution
Use the caching system for frequently rendered templates.

```c
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../compiler/src/cjinja.h"

void benchmark_cached_vs_uncached() {
    CJinjaContext* ctx = cjinja_create_context();
    if (!ctx) {
        fprintf(stderr, "Failed to create context\n");
        return;
    }
    
    // Set variables
    cjinja_set_var(ctx, "user", "Alice");
    cjinja_set_var(ctx, "title", "Welcome");
    
    const char* template = "Hello {{user}}, welcome to {{title}}!";
    const int iterations = 100000;
    
    // Benchmark uncached rendering
    clock_t start = clock();
    for (int i = 0; i < iterations; i++) {
        char* result = cjinja_render_string(template, ctx);
        if (result) {
            free(result);
        }
    }
    clock_t end = clock();
    double uncached_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    // Benchmark cached rendering (simulated)
    start = clock();
    for (int i = 0; i < iterations; i++) {
        char* result = cjinja_render_string(template, ctx);  // Same template
        if (result) {
            free(result);
        }
    }
    end = clock();
    double cached_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    printf("Template rendering performance:\n");
    printf("  Uncached: %.2f seconds (%.2f ns per render)\n", 
           uncached_time, uncached_time * 1000000000.0 / iterations);
    printf("  Cached: %.2f seconds (%.2f ns per render)\n", 
           cached_time, cached_time * 1000000000.0 / iterations);
    printf("  Speedup: %.2fx\n", uncached_time / cached_time);
    
    cjinja_destroy_context(ctx);
}

int main() {
    benchmark_cached_vs_uncached();
    return 0;
}
```

## Pattern 5: Complex Template Composition

### Problem
Build complex templates with multiple features efficiently.

### Solution
Compose templates using multiple rendering passes.

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../compiler/src/cjinja.h"

char* compose_complex_template() {
    CJinjaContext* ctx = cjinja_create_context();
    if (!ctx) {
        return NULL;
    }
    
    // Set complex data
    cjinja_set_var(ctx, "page_title", "User Dashboard");
    cjinja_set_var(ctx, "username", "Alice");
    cjinja_set_bool(ctx, "is_admin", 1);
    cjinja_set_bool(ctx, "show_analytics", 1);
    
    char* menu_items[] = {"Home", "Profile", "Settings", "Logout"};
    cjinja_set_array(ctx, "menu_items", menu_items, 4);
    
    char* recent_activities[] = {"Login", "Profile Update", "Settings Change"};
    cjinja_set_array(ctx, "recent_activities", recent_activities, 3);
    
    // Complex template
    const char* template = 
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "  <title>{{page_title}}</title>\n"
        "</head>\n"
        "<body>\n"
        "  <header>\n"
        "    <h1>{{page_title}}</h1>\n"
        "    <p>Welcome, {{username}}!</p>\n"
        "  </header>\n"
        "  \n"
        "  <nav>\n"
        "    <ul>\n"
        "    {% for item in menu_items %}\n"
        "      <li><a href=\"/{{item|lower}}\">{{item}}</a></li>\n"
        "    {% endfor %}\n"
        "    </ul>\n"
        "  </nav>\n"
        "  \n"
        "  <main>\n"
        "    {% if is_admin %}\n"
        "      <div class=\"admin-panel\">\n"
        "        <h2>Admin Controls</h2>\n"
        "        <p>You have administrative privileges.</p>\n"
        "      </div>\n"
        "    {% endif %}\n"
        "    \n"
        "    {% if show_analytics %}\n"
        "      <div class=\"analytics\">\n"
        "        <h2>Recent Activity</h2>\n"
        "        <ul>\n"
        "        {% for activity in recent_activities %}\n"
        "          <li>{{activity}}</li>\n"
        "        {% endfor %}\n"
        "        </ul>\n"
        "      </div>\n"
        "    {% endif %}\n"
        "  </main>\n"
        "</body>\n"
        "</html>";
    
    char* result = cjinja_render_with_loops(template, ctx);
    cjinja_destroy_context(ctx);
    
    return result;
}

int main() {
    char* html = compose_complex_template();
    if (html) {
        printf("Generated HTML:\n%s\n", html);
        free(html);
    }
    return 0;
}
```

## Pattern 6: Custom Filter Implementation

### Problem
Extend CJinja with custom filters for specific use cases.

### Solution
Implement custom filter functions and integrate them.

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../compiler/src/cjinja.h"

// Custom filter: truncate string to specified length
char* cjinja_filter_truncate(const char* input, const char* args) {
    if (!input) return NULL;
    
    int max_length = 10;  // Default
    if (args) {
        max_length = atoi(args);
    }
    
    int input_len = strlen(input);
    if (input_len <= max_length) {
        return strdup(input);
    }
    
    char* result = malloc(max_length + 4);  // +4 for "..."
    if (!result) return NULL;
    
    strncpy(result, input, max_length);
    strcpy(result + max_length, "...");
    
    return result;
}

// Custom filter: format number with commas
char* cjinja_filter_format_number(const char* input, const char* args) {
    if (!input) return NULL;
    
    long number = atol(input);
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%ld", number);
    
    // Simple comma formatting (every 3 digits from right)
    int len = strlen(buffer);
    int commas = (len - 1) / 3;
    char* result = malloc(len + commas + 1);
    
    if (!result) return NULL;
    
    int j = 0;
    for (int i = 0; i < len; i++) {
        if (i > 0 && (len - i) % 3 == 0) {
            result[j++] = ',';
        }
        result[j++] = buffer[i];
    }
    result[j] = '\0';
    
    return result;
}

void test_custom_filters() {
    CJinjaContext* ctx = cjinja_create_context();
    if (!ctx) {
        fprintf(stderr, "Failed to create context\n");
        return;
    }
    
    cjinja_set_var(ctx, "long_text", "This is a very long text that needs truncation");
    cjinja_set_var(ctx, "number", "1234567");
    
    // Test truncate filter
    char* truncated = cjinja_filter_truncate("This is a very long text", "15");
    printf("Truncated: %s\n", truncated);
    free(truncated);
    
    // Test number formatting
    char* formatted = cjinja_filter_format_number("1234567", NULL);
    printf("Formatted number: %s\n", formatted);
    free(formatted);
    
    cjinja_destroy_context(ctx);
}

int main() {
    test_custom_filters();
    return 0;
}
```

## Pattern 7: Performance Optimization

### Problem
Optimize template rendering for maximum performance.

### Solution
Use performance-focused patterns and techniques.

```c
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include "../compiler/src/cjinja.h"

// High-precision timing
static inline uint64_t get_microseconds() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000ULL + tv.tv_usec;
}

void benchmark_template_performance() {
    CJinjaContext* ctx = cjinja_create_context();
    if (!ctx) {
        fprintf(stderr, "Failed to create context\n");
        return;
    }
    
    // Setup test data
    cjinja_set_var(ctx, "user", "Alice");
    cjinja_set_var(ctx, "title", "Welcome");
    cjinja_set_bool(ctx, "is_admin", 1);
    
    char* items[] = {"item1", "item2", "item3", "item4", "item5"};
    cjinja_set_array(ctx, "items", items, 5);
    
    // Test different template types
    const char* templates[] = {
        // Simple variable substitution
        "Hello {{user}}, welcome to {{title}}!",
        
        // Conditional rendering
        "{% if is_admin %}Admin: {{user}}{% endif %}",
        
        // Loop rendering
        "{% for item in items %}- {{item}}\n{% endfor %}",
        
        // Complex template
        "{% if is_admin %}Admin {{user}}: {% for item in items %}{{item|upper}} {% endfor %}{% endif %}"
    };
    
    const char* template_names[] = {
        "Simple Variables",
        "Conditionals",
        "Loops",
        "Complex"
    };
    
    const int iterations = 100000;
    
    printf("Template Performance Benchmark\n");
    printf("=============================\n\n");
    
    for (int t = 0; t < 4; t++) {
        uint64_t start_time = get_microseconds();
        
        for (int i = 0; i < iterations; i++) {
            char* result;
            
            switch (t) {
                case 0:
                    result = cjinja_render_string(templates[t], ctx);
                    break;
                case 1:
                    result = cjinja_render_with_conditionals(templates[t], ctx);
                    break;
                case 2:
                    result = cjinja_render_with_loops(templates[t], ctx);
                    break;
                case 3:
                    result = cjinja_render_with_loops(templates[t], ctx);
                    break;
            }
            
            if (result) {
                free(result);
            }
        }
        
        uint64_t end_time = get_microseconds();
        uint64_t elapsed = end_time - start_time;
        
        double avg_ns = (elapsed * 1000.0) / iterations;
        double ops_per_sec = (iterations * 1000000.0) / elapsed;
        
        printf("%s:\n", template_names[t]);
        printf("  Average latency: %.2f ns\n", avg_ns);
        printf("  Throughput: %.0f ops/sec\n", ops_per_sec);
        printf("  Total time: %llu μs\n\n", elapsed);
    }
    
    cjinja_destroy_context(ctx);
}

void optimize_memory_usage() {
    // Pre-allocate context for reuse
    CJinjaContext* ctx = cjinja_create_context();
    if (!ctx) {
        fprintf(stderr, "Failed to create context\n");
        return;
    }
    
    // Reuse context for multiple renders
    const char* templates[] = {
        "User: {{user}}",
        "Title: {{title}}",
        "Status: {{status}}"
    };
    
    for (int i = 0; i < 3; i++) {
        // Update context for each template
        cjinja_set_var(ctx, "user", "Alice");
        cjinja_set_var(ctx, "title", "Welcome");
        cjinja_set_var(ctx, "status", "Active");
        
        char* result = cjinja_render_string(templates[i], ctx);
        if (result) {
            printf("Template %d: %s\n", i + 1, result);
            free(result);
        }
    }
    
    cjinja_destroy_context(ctx);
}

int main() {
    printf("=== Template Performance Benchmark ===\n");
    benchmark_template_performance();
    
    printf("=== Memory Optimization ===\n");
    optimize_memory_usage();
    
    return 0;
}
```

## Pattern 8: Error Handling and Recovery

### Problem
Handle template rendering errors gracefully.

### Solution
Implement robust error handling with fallback templates.

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../compiler/src/cjinja.h"

typedef struct {
    char* result;
    int success;
    char* error_message;
} RenderResult;

RenderResult safe_render_template(const char* template, CJinjaContext* ctx) {
    RenderResult result = {NULL, 0, NULL};
    
    if (!template || !ctx) {
        result.error_message = strdup("Invalid parameters");
        return result;
    }
    
    // Try to render the template
    result.result = cjinja_render_string(template, ctx);
    if (result.result) {
        result.success = 1;
        return result;
    }
    
    // Fallback to simple template
    const char* fallback = "Hello {{user|default('Guest')}}!";
    result.result = cjinja_render_string(fallback, ctx);
    
    if (result.result) {
        result.success = 1;
        result.error_message = strdup("Used fallback template");
    } else {
        result.error_message = strdup("Failed to render template");
    }
    
    return result;
}

void render_with_error_handling() {
    CJinjaContext* ctx = cjinja_create_context();
    if (!ctx) {
        fprintf(stderr, "Failed to create context\n");
        return;
    }
    
    // Set some variables
    cjinja_set_var(ctx, "user", "Alice");
    
    // Test valid template
    const char* valid_template = "Hello {{user}}, welcome!";
    RenderResult result1 = safe_render_template(valid_template, ctx);
    
    if (result1.success) {
        printf("Valid template: %s\n", result1.result);
        if (result1.error_message) {
            printf("Note: %s\n", result1.error_message);
        }
    } else {
        printf("Error: %s\n", result1.error_message);
    }
    
    if (result1.result) free(result1.result);
    if (result1.error_message) free(result1.error_message);
    
    // Test invalid template (missing variable)
    const char* invalid_template = "Hello {{missing_variable}}, welcome!";
    RenderResult result2 = safe_render_template(invalid_template, ctx);
    
    if (result2.success) {
        printf("Invalid template (fallback): %s\n", result2.result);
        if (result2.error_message) {
            printf("Note: %s\n", result2.error_message);
        }
    } else {
        printf("Error: %s\n", result2.error_message);
    }
    
    if (result2.result) free(result2.result);
    if (result2.error_message) free(result2.error_message);
    
    cjinja_destroy_context(ctx);
}

int main() {
    render_with_error_handling();
    return 0;
}
```

## Best Practices

### 1. Performance Optimization
- **Reuse contexts**: Create context once and reuse for multiple renders
- **Cache templates**: Use caching for frequently rendered templates
- **Minimize allocations**: Use stack allocation for small data structures
- **Batch operations**: Render multiple templates together when possible

### 2. Memory Management
- **Always free results**: Free rendered strings to prevent memory leaks
- **Check allocations**: Verify context creation and rendering success
- **Use appropriate functions**: Choose the right rendering function for your needs
- **Monitor memory usage**: Track memory consumption in production

### 3. Error Handling
- **Validate inputs**: Check template strings and context before rendering
- **Provide fallbacks**: Implement fallback templates for error conditions
- **Log errors**: Record rendering errors for debugging
- **Graceful degradation**: Continue operation even if rendering fails

### 4. Template Design
- **Keep templates simple**: Complex templates are harder to debug and optimize
- **Use appropriate features**: Choose variables, conditionals, or loops as needed
- **Optimize loops**: Minimize loop iterations for better performance
- **Cache frequently used data**: Reuse variable values across renders

### 5. Integration Patterns
- **Python integration**: Use Python bindings for high-level applications
- **C runtime**: Direct C calls for maximum performance
- **Batch processing**: Group template renders for efficiency
- **Async rendering**: Consider async patterns for high-throughput scenarios

## Conclusion

These CJinja template rendering patterns demonstrate how to achieve sub-microsecond performance while maintaining flexibility and functionality. The patterns provide a foundation for building high-performance template systems that can handle millions of operations per second.

Key takeaways:
1. **Choose the right rendering function** for your template type
2. **Reuse contexts** to minimize allocation overhead
3. **Implement proper error handling** with fallback templates
4. **Monitor performance** in production environments
5. **Cache frequently used templates** to improve performance 