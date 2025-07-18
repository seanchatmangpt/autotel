#include <stdio.h>
#include <stdlib.h>
#include "../runtime/src/seven_t_runtime.h"

int main() {
    printf("Debug test\n");
    
    EngineState* engine = s7t_create_engine();
    printf("Engine created\n");
    
    // Test 1: Simple triple
    uint32_t s = s7t_intern_string(engine, "subject");
    uint32_t p = s7t_intern_string(engine, "predicate");
    uint32_t o = s7t_intern_string(engine, "object");
    
    printf("IDs: s=%u, p=%u, o=%u\n", s, p, o);
    
    printf("Adding triple...\n");
    s7t_add_triple(engine, s, p, o);
    printf("Triple added. Count: %zu\n", engine->triple_count);
    
    // Test 2: Multiple triples
    for (int i = 0; i < 10; i++) {
        char buf[32];
        snprintf(buf, sizeof(buf), "s%d", i);
        uint32_t si = s7t_intern_string(engine, buf);
        
        s7t_add_triple(engine, si, p, o);
        printf("Added triple %d, total: %zu\n", i, engine->triple_count);
    }
    
    s7t_destroy_engine(engine);
    printf("Done\n");
    
    return 0;
}
