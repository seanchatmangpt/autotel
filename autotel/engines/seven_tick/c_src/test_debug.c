#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "sparql7t.h"

int main() {
    printf("=== SPARQL-7T Detailed Test ===\n\n");
    
    // Create a small engine
    S7TEngine* engine = s7t_create(100, 10, 100);
    
    printf("Adding triples:\n");
    printf("  (42, 1, 100) - Subject 42 has predicate 1 with object 100\n");
    s7t_add_triple(engine, 42, 1, 100);
    
    printf("  (42, 2, 200) - Subject 42 has predicate 2 with object 200\n");
    s7t_add_triple(engine, 42, 2, 200);
    
    printf("  (99, 1, 100) - Subject 99 has predicate 1 with object 100\n");
    s7t_add_triple(engine, 99, 1, 100);
    
    printf("\nChecking ps_to_o_index:\n");
    printf("  ps_to_o[1][42] = %u (should be 100)\n", engine->ps_to_o_index[1 * engine->max_subjects + 42]);
    printf("  ps_to_o[2][42] = %u (should be 200)\n", engine->ps_to_o_index[2 * engine->max_subjects + 42]);
    printf("  ps_to_o[1][99] = %u (should be 100)\n", engine->ps_to_o_index[1 * engine->max_subjects + 99]);
    
    printf("\nThe issue: Our current s7t_ask_pattern doesn't use ps_to_o_index!\n");
    printf("It only checks if subject bit is set in BOTH predicate and object vectors.\n");
    printf("This causes false positives when a subject has multiple predicates/objects.\n");
    
    // Cleanup
    free(engine->predicate_vectors);
    free(engine->object_vectors);
    free(engine->ps_to_o_index);
    free(engine);
    
    return 0;
}
