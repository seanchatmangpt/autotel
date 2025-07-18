#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "sparql7t.h"

int main() {
    printf("=== SPARQL-7T Simple Test ===\n\n");
    
    // Create a small engine
    S7TEngine* engine = s7t_create(100, 10, 100);
    
    printf("Adding triples:\n");
    printf("  (42, 1, 100) - Subject 42 has predicate 1 with object 100\n");
    s7t_add_triple(engine, 42, 1, 100);
    
    printf("  (42, 2, 200) - Subject 42 has predicate 2 with object 200\n");
    s7t_add_triple(engine, 42, 2, 200);
    
    printf("  (99, 1, 100) - Subject 99 has predicate 1 with object 100\n");
    s7t_add_triple(engine, 99, 1, 100);
    
    printf("\nQuerying:\n");
    
    // Test 1: Should match
    int result1 = s7t_ask_pattern(engine, 42, 1, 100);
    printf("  Query (42, 1, 100): %s\n", result1 ? "MATCH" : "NO MATCH");
    assert(result1 == 1);
    
    // Test 2: Should match  
    int result2 = s7t_ask_pattern(engine, 42, 2, 200);
    printf("  Query (42, 2, 200): %s\n", result2 ? "MATCH" : "NO MATCH");
    assert(result2 == 1);
    
    // Test 3: Should NOT match (wrong predicate for this subject/object combo)
    int result3 = s7t_ask_pattern(engine, 42, 1, 200);
    printf("  Query (42, 1, 200): %s\n", result3 ? "MATCH" : "NO MATCH");
    assert(result3 == 0);
    
    // Test 4: Should match
    int result4 = s7t_ask_pattern(engine, 99, 1, 100);
    printf("  Query (99, 1, 100): %s\n", result4 ? "MATCH" : "NO MATCH");
    assert(result4 == 1);
    
    // Test 5: Should NOT match (no such triple)
    int result5 = s7t_ask_pattern(engine, 99, 2, 200);
    printf("  Query (99, 2, 200): %s\n", result5 ? "MATCH" : "NO MATCH");
    assert(result5 == 0);
    
    printf("\n✅ All tests passed!\n");
    
    // Cleanup
    free(engine->predicate_vectors);
    free(engine->object_vectors);
    free(engine->ps_to_o_index);
    free(engine);
    
    return 0;
}
