#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cjinja_ultra_portable.h"

int main() {
    printf("Debugging edge case...\n");
    
    CJinjaUltraContext* ctx = cjinja_ultra_create_context();
    
    // Test malformed variable (missing closing braces)
    char* result3 = cjinja_ultra_render_variables("Hello {{name", ctx);
    printf("Input: 'Hello {{name'\n");
    printf("Output: '%s'\n", result3 ? result3 : "NULL");
    printf("Expected: 'Hello {{name'\n");
    
    if (result3) free(result3);
    cjinja_ultra_destroy_context(ctx);
    return 0;
}