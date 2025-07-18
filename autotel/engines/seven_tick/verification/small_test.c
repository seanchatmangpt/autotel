#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include "../runtime/src/seven_t_runtime.h"

typedef struct {
    uint32_t patient_id;
    uint32_t appointment_id;
    uint32_t doctor_id;
} QueryResult;

// Simple file loader
void load_data(EngineState* engine, const char* filename) {
    FILE* f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "Cannot open %s\n", filename);
        return;
    }
    
    char line[4096];
    int count = 0;
    
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '@' || line[0] == '\n') continue;
        
        // Basic parsing
        char subj[1024], pred[1024], obj[1024];
        if (sscanf(line, "%s %s %[^\n]", subj, pred, obj) == 3) {
            // Remove trailing dot
            size_t len = strlen(obj);
            if (len > 0 && obj[len-1] == '.') obj[len-1] = '\0';
            
            uint32_t s = s7t_intern_string(engine, subj);
            uint32_t p = s7t_intern_string(engine, pred);
            uint32_t o = s7t_intern_string(engine, obj);
            
            s7t_add_triple(engine, s, p, o);
            count++;
        }
    }
    
    fclose(f);
    printf("Loaded %d triples\n", count);
}

int main() {
    printf("Small benchmark test\n");
    
    EngineState* engine = s7t_create_engine();
    
    // Load small dataset
    load_data(engine, "/tmp/small_test_data.ttl");
    
    printf("Engine stats: %zu triples, max IDs: S=%zu P=%zu O=%zu\n",
           engine->triple_count, engine->max_subject_id, 
           engine->max_predicate_id, engine->max_object_id);
    
    // Load kernel
    void* kernel = dlopen("/tmp/kernel.so", RTLD_NOW);
    if (!kernel) {
        fprintf(stderr, "Failed to load kernel: %s\n", dlerror());
        return 1;
    }
    
    typedef QueryResult* (*query_func)(EngineState*, size_t*);
    query_func execute_query = dlsym(kernel, "execute_query_1");
    
    if (execute_query) {
        size_t count;
        QueryResult* results = execute_query(engine, &count);
        printf("Query returned %zu results\n", count);
        free(results);
    }
    
    dlclose(kernel);
    s7t_destroy_engine(engine);
    
    return 0;
}
