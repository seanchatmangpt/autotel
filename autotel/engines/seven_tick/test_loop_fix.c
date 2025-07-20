#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "compiler/src/cjinja.h"

int main() {
    printf("Testing CJinja loop fix...\n\n");
    
    // Create engine and context
    CJinjaEngine* engine = cjinja_create("./templates");
    CJinjaContext* ctx = cjinja_create_context();
    
    // Set up test data
    char* fruits[] = {"apple", "banana", "cherry"};
    cjinja_set_array(ctx, "fruits", fruits, 3);
    
    // Debug: Check what was stored
    printf("Debug: Array stored as '%s'\n\n", get_var(ctx, "fruits"));
    
    // Test template with loop
    const char* template = 
        "Fruits:\n"
        "{% for fruit in fruits %}"
        "- {{fruit}}\n"
        "{% endfor %}"
        "Done!";
    
    printf("Template:\n%s\n\n", template);
    
    // Debug: Test simple variable substitution first
    cjinja_set_var(ctx, "test_var", "test_value");
    char* simple_test = cjinja_render_string("{{test_var}}", ctx);
    printf("Simple test: '{{test_var}}' -> '%s'\n\n", simple_test ? simple_test : "NULL");
    free(simple_test);
    
    // Render template
    char* result = cjinja_render_with_loops(template, ctx);
    
    printf("Result:\n%s\n\n", result);
    printf("Result length: %zu\n", result ? strlen(result) : 0);
    
    // Expected output:
    // Fruits:
    // - apple
    // - banana  
    // - cherry
    // Done!
    
    // Check if result contains individual fruits
    if (result && strstr(result, "apple") && strstr(result, "banana") && strstr(result, "cherry")) {
        printf("✅ SUCCESS: Loop rendering works correctly!\n");
        printf("   Individual items are rendered properly.\n");
    } else {
        printf("❌ FAILED: Loop rendering still broken.\n");
        printf("   Expected individual fruit names in output.\n");
    }
    
    // Cleanup
    free(result);
    cjinja_destroy_context(ctx);
    cjinja_destroy(engine);
    
    return 0;
}