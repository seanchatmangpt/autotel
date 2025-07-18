#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../runtime/src/seven_t_runtime.h"

int main() {
    printf("Test: Loading TTL data\n");
    
    EngineState* engine = s7t_create_engine();
    FILE* f = fopen("/tmp/small_test_data.ttl", "r");
    if (!f) {
        printf("Cannot open data file\n");
        return 1;
    }
    
    char line[4096];
    int count = 0;
    int errors = 0;
    
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '@' || line[0] == '\n') continue;
        
        // Count lines
        count++;
        
        // Try to parse
        char* subj = strtok(line, " \t");
        char* pred = strtok(NULL, " \t");
        char* obj = strtok(NULL, " \t\n");
        
        if (!subj || !pred || !obj) {
            errors++;
            continue;
        }
        
        // Remove trailing dot from object
        size_t obj_len = strlen(obj);
        if (obj_len > 0 && obj[obj_len-1] == '.') {
            obj[obj_len-1] = '\0';
        }
        
        // Try to intern strings
        uint32_t s_id = s7t_intern_string(engine, subj);
        uint32_t p_id = s7t_intern_string(engine, pred);  
        uint32_t o_id = s7t_intern_string(engine, obj);
        
        // Progress indicator
        if (count % 10000 == 0) {
            printf("Loaded %d lines, %zu triples, max IDs: S=%zu P=%zu O=%zu\n", 
                   count, engine->triple_count, engine->max_subject_id,
                   engine->max_predicate_id, engine->max_object_id);
        }
        
        // Add triple
        s7t_add_triple(engine, s_id, p_id, o_id);
    }
    
    fclose(f);
    
    printf("\nFinal stats:\n");
    printf("Lines processed: %d\n", count);
    printf("Parse errors: %d\n", errors);
    printf("Triples: %zu\n", engine->triple_count);
    printf("Max IDs: S=%zu P=%zu O=%zu\n", 
           engine->max_subject_id, engine->max_predicate_id, engine->max_object_id);
    
    s7t_destroy_engine(engine);
    
    return 0;
}
