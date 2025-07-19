#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../compiler/src/cjinja.h"

#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            printf("❌ FAIL: \n", message); \
            return 0; \
        } else { \
            printf("✅ PASS: \n", message); \
        } \
    } while(0)

int test_cjinja_7tick() {
    printf("\n=== CJinja 7-Tick Path ===\n");
    
    CJinjaEngine *engine = cjinja_create("./templates");
    CJinjaContext *ctx = cjinja_create_context();
    
    TEST_ASSERT(engine != NULL, "Engine creation");
    TEST_ASSERT(ctx != NULL, "Context creation");
    
    cjinja_set_var(ctx, "name", "John");
    cjinja_set_var(ctx, "title", "Developer");
    
    const char *template = "Hello {{name}}, you are a {{title}}!";
    char *result = cjinja_render_string_7tick(template, ctx);
    
    TEST_ASSERT(result != NULL, "7-tick render result not null");
    TEST_ASSERT(strstr(result, "Hello John") != NULL, "7-tick variable substitution");
    
    free(result);
    cjinja_destroy_context(ctx);
    cjinja_destroy_engine(engine);
    
    return 1;
}

int test_cjinja_49tick() {
    printf("\n=== CJinja 49-Tick Path ===\n");
    
    CJinjaEngine *engine = cjinja_create("./templates");
    CJinjaContext *ctx = cjinja_create_context();
    
    TEST_ASSERT(engine != NULL, "Engine creation");
    TEST_ASSERT(ctx != NULL, "Context creation");
    
    cjinja_set_var(ctx, "user", "Alice");
    
    const char *template = "User: {{user | upper}}";
    char *result = cjinja_render_string(template, ctx);
    
    TEST_ASSERT(result != NULL, "49-tick render result not null");
    TEST_ASSERT(strstr(result, "ALICE") != NULL, "49-tick upper filter");
    
    free(result);
    cjinja_destroy_context(ctx);
    cjinja_destroy_engine(engine);
    
    return 1;
}

int main() {
    printf("80/20 Critical Unit Tests\n");
    printf("========================\n");
    
    int total_tests = 2;
    int passed_tests = 0;
    
    if (test_cjinja_7tick()) passed_tests++;
    if (test_cjinja_49tick()) passed_tests++;
    
    printf("\n=== Test Summary ===\n");
    printf("Total tests: 0\n", total_tests);
    printf("Passed: 0\n", passed_tests);
    printf("Failed: 0\n", total_tests - passed_tests);
    printf("Success rate: 0.0%\n", (passed_tests * 100.0) / total_tests);
    
    if (passed_tests == total_tests) {
        printf("\n🎉 All tests passed! Critical functionality is working.\n");
        return 0;
    } else {
        printf("\n❌ Some tests failed.\n");
        return 1;
    }
}