#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "compiler/src/cjinja.h"

int main() {
    printf("=== Debugging CJinja Loop Issue ===\n\n");
    
    // Test 1: Basic variable substitution
    printf("1. Testing basic variable substitution:\n");
    CJinjaContext* ctx1 = cjinja_create_context();
    cjinja_set_var(ctx1, "fruit", "apple");
    
    char* result1 = cjinja_render_string("- {{fruit}}", ctx1);
    printf("   Template: '- {{fruit}}'\n");
    printf("   Context: fruit='apple'\n");
    printf("   Result: '%s'\n\n", result1 ? result1 : "NULL");
    
    // Test 2: Array storage and retrieval
    printf("2. Testing array storage:\n");
    CJinjaContext* ctx2 = cjinja_create_context();
    char* fruits[] = {"apple", "banana", "cherry"};
    cjinja_set_array(ctx2, "fruits", fruits, 3);
    
    // Check how array is stored
    printf("   Array stored as: '%s'\n\n", get_var(ctx2, "fruits"));
    
    // Test 3: Manual loop simulation
    printf("3. Testing manual loop simulation:\n");
    char* array_str = get_var(ctx2, "fruits");
    if (array_str) {
        char* items[10];
        int item_count = 0;
        char* token = strtok(strdup(array_str), ",");
        while (token && item_count < 10) {
            items[item_count++] = strdup(token);
            token = strtok(NULL, ",");
            printf("   Item %d: '%s'\n", item_count, items[item_count-1]);
        }
        
        // Test rendering each item
        for (int i = 0; i < item_count; i++) {
            CJinjaContext* temp_ctx = cjinja_create_context();
            cjinja_set_var(temp_ctx, "fruit", items[i]);
            char* rendered = cjinja_render_string("- {{fruit}}", temp_ctx);
            printf("   Rendered item %d: '%s'\n", i+1, rendered ? rendered : "NULL");
            free(rendered);
            cjinja_destroy_context(temp_ctx);
        }
    }
    
    // Cleanup
    free(result1);
    cjinja_destroy_context(ctx1);
    cjinja_destroy_context(ctx2);
    
    return 0;
}