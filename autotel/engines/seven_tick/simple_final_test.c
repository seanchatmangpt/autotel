#include <stdio.h>
#include "cjinja_final.h"

int main() {
    printf("Testing CJinja Final...\n");
    
    CJinjaContext* ctx = cjinja_create_context();
    if (!ctx) {
        printf("Failed to create context\n");
        return 1;
    }
    
    cjinja_set_var(ctx, "name", "World");
    char* result = cjinja_render_string("Hello {{name}}!", ctx);
    
    if (result) {
        printf("Result: %s\n", result);
        printf("✅ SUCCESS!\n");
        free(result);
    } else {
        printf("❌ FAILED!\n");
    }
    
    cjinja_destroy_context(ctx);
    return 0;
}