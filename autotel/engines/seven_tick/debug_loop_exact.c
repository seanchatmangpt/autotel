#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "compiler/src/cjinja.h"

int main() {
    printf("=== Debugging Exact Loop Logic ===\n\n");
    
    // Create context exactly like our test
    CJinjaContext* ctx = cjinja_create_context();
    char* fruits[] = {"apple", "banana", "cherry"};
    cjinja_set_array(ctx, "fruits", fruits, 3);
    
    // Manually parse the template like the loop function does
    const char* template = "Fruits:\n{% for fruit in fruits %}- {{fruit}}\n{% endfor %}Done!";
    printf("Template: %s\n\n", template);
    
    // Find the loop part and extract the body exactly like the function
    const char* pos = strstr(template, "{% for fruit in fruits %}");
    if (!pos) {
        printf("ERROR: Could not find loop start\n");
        return 1;
    }
    
    // Skip to end of {% for ... %} block
    pos = strstr(pos, "%}");
    if (pos) pos += 2;
    
    printf("Loop body start: '%s'\n", pos);
    
    // Find the end of the loop
    const char* end_pos = strstr(pos, "{% endfor %}");
    if (!end_pos) {
        printf("ERROR: Could not find loop end\n");
        return 1;
    }
    
    // Extract body
    size_t body_len = end_pos - pos;
    char* body = malloc(body_len + 1);
    strncpy(body, pos, body_len);
    body[body_len] = '\0';
    
    printf("Extracted body: '%s' (length: %zu)\n\n", body, body_len);
    
    // Get array and test iteration
    char* array_str = get_var(ctx, "fruits");
    printf("Array value: '%s'\n", array_str);
    
    if (array_str) {
        char* items[100];
        size_t item_count = 0;
        char* token = strtok(strdup(array_str), ",");
        while (token && item_count < 100) {
            items[item_count++] = strdup(token);
            token = strtok(NULL, ",");
        }
        
        printf("Found %zu items\n", item_count);
        
        // Test loop rendering exactly like the function
        for (size_t i = 0; i < item_count; i++) {
            printf("\n--- Iteration %zu ---\n", i+1);
            
            // Create temp context
            CJinjaContext* temp_ctx = cjinja_create_context();
            
            // Copy all variables from original context
            for (size_t j = 0; j < ctx->count; j++) {
                cjinja_set_var(temp_ctx, ctx->keys[j], ctx->values[j]);
                printf("Copied var: %s = %s\n", ctx->keys[j], ctx->values[j]);
            }
            
            // Set loop variable
            cjinja_set_var(temp_ctx, "fruit", items[i]);
            printf("Set loop var: fruit = %s\n", items[i]);
            
            // Render loop body
            char* rendered_body = cjinja_render_string(body, temp_ctx);
            printf("Rendered body: '%s'\n", rendered_body ? rendered_body : "NULL");
            
            free(rendered_body);
            cjinja_destroy_context(temp_ctx);
        }
    }
    
    free(body);
    cjinja_destroy_context(ctx);
    return 0;
}