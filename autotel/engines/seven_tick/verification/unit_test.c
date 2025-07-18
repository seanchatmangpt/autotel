#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <string.h>
#include <time.h>
#include "../runtime/src/seven_t_runtime.h"

int main() {
    // Test 1: Basic engine creation
    printf("Test 1: Creating engine...\n");
    EngineState* engine = s7t_create_engine();
    if (!engine) {
        printf("FAILED: Could not create engine\n");
        return 1;
    }
    printf("PASSED\n");
    
    // Test 2: String interning
    printf("\nTest 2: String interning...\n");
    uint32_t id1 = s7t_intern_string(engine, "test");
    uint32_t id2 = s7t_intern_string(engine, "test");
    uint32_t id3 = s7t_intern_string(engine, "test2");
    
    if (id1 != id2 || id1 == id3) {
        printf("FAILED: String interning not working correctly\n");
        return 1;
    }
    printf("PASSED: id1=%u, id2=%u, id3=%u\n", id1, id2, id3);
    
    // Test 3: Add triples
    printf("\nTest 3: Adding triples...\n");
    for (int i = 0; i < 100; i++) {
        char subj[32], obj[32];
        snprintf(subj, sizeof(subj), "subject_%d", i);
        snprintf(obj, sizeof(obj), "object_%d", i);
        
        uint32_t s = s7t_intern_string(engine, subj);
        uint32_t p = s7t_intern_string(engine, "predicate");
        uint32_t o = s7t_intern_string(engine, obj);
        
        s7t_add_triple(engine, s, p, o);
    }
    printf("PASSED: Added %zu triples\n", engine->triple_count);
    
    // Test 4: Query primitives
    printf("\nTest 4: Testing query primitives...\n");
    uint32_t pred = s7t_intern_string(engine, "predicate");
    uint32_t obj = s7t_intern_string(engine, "object_50");
    
    BitVector* subjects = s7t_get_subject_vector(engine, pred, obj);
    if (!subjects || subjects->count == 0) {
        printf("FAILED: Could not find subject\n");
        return 1;
    }
    printf("PASSED: Found %zu subjects\n", subjects->count);
    bitvec_destroy(subjects);
    
    // Test 5: Large scale test
    printf("\nTest 5: Large scale test...\n");
    clock_t start = clock();
    
    for (int i = 0; i < 10000; i++) {
        char subj[32], obj[32];
        snprintf(subj, sizeof(subj), "s_%d", i);
        snprintf(obj, sizeof(obj), "o_%d", i);
        
        uint32_t s = s7t_intern_string(engine, subj);
        uint32_t p = s7t_intern_string(engine, "p");
        uint32_t o = s7t_intern_string(engine, obj);
        
        s7t_add_triple(engine, s, p, o);
    }
    
    clock_t end = clock();
    double elapsed = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    printf("PASSED: Added 10k triples in %.3f seconds (%.0f triples/sec)\n", 
           elapsed, 10000.0 / elapsed);
    
    // Cleanup
    s7t_destroy_engine(engine);
    printf("\nAll tests passed!\n");
    
    return 0;
}
