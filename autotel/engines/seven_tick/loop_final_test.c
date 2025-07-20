#include <stdio.h>
#include <string.h>
#include "cjinja_final.h"

int main() {
    printf("Testing CJinja Final Loop Fix...\n");
    
    CJinjaContext* ctx = cjinja_create_context();
    if (!ctx) {
        printf("Failed to create context\n");
        return 1;
    }
    
    // Test loop functionality
    char* fruits[] = {"apple", "banana", "cherry"};
    cjinja_set_array(ctx, "fruits", fruits, 3);
    
    const char* template = 
        "Fruits:\n"
        "{% for fruit in fruits %}"
        "- {{fruit}}\n"
        "{% endfor %}"
        "Done!";
    
    char* result = cjinja_render_with_loops(template, ctx);
    
    if (result) {
        printf("Template: %s\n\n", template);
        printf("Result:\n%s\n\n", result);
        
        // Check if all fruits are present
        if (strstr(result, "apple") && strstr(result, "banana") && strstr(result, "cherry")) {
            printf("✅ Loop rendering SUCCESS! All items rendered correctly.\n");
        } else {
            printf("❌ Loop rendering FAILED! Missing items.\n");
        }
        
        free(result);
    } else {
        printf("❌ FAILED to render template!\n");
    }
    
    cjinja_destroy_context(ctx);
    return 0;
}