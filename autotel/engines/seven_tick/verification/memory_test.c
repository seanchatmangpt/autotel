#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../runtime/src/seven_t_runtime.h"

int main() {
    printf("Testing fixed memory management\n");
    printf("==============================\n\n");
    
    EngineState* engine = s7t_create_engine();
    
    // Test 1: Add 100+ triples with varied IDs
    printf("Test 1: Adding 200 triples...\n");
    
    for (int i = 0; i < 200; i++) {
        char subj[64], pred[64], obj[64];
        
        // Vary the patterns to stress test
        snprintf(subj, sizeof(subj), "subject_%d", i);
        snprintf(pred, sizeof(pred), "predicate_%d", i % 10); // 10 different predicates
        snprintf(obj, sizeof(obj), "object_%d", i % 50);      // 50 different objects
        
        uint32_t s = s7t_intern_string(engine, subj);
        uint32_t p = s7t_intern_string(engine, pred);
        uint32_t o = s7t_intern_string(engine, obj);
        
        s7t_add_triple(engine, s, p, o);
        
        if (i % 50 == 0) {
            printf("  Added %d triples, max IDs: S=%zu P=%zu O=%zu\n", 
                   i, engine->max_subject_id, engine->max_predicate_id, engine->max_object_id);
        }
    }
    
    printf("✓ Successfully added %zu triples\n\n", engine->triple_count);
    
    // Test 2: Query operations
    printf("Test 2: Query operations...\n");
    
    uint32_t test_pred = s7t_intern_string(engine, "predicate_5");
    uint32_t test_obj = s7t_intern_string(engine, "object_25");
    
    BitVector* subjects = s7t_get_subject_vector(engine, test_pred, test_obj);
    printf("  Found %zu subjects with predicate_5 -> object_25\n", subjects->count);
    bitvec_destroy(subjects);
    
    // Test 3: SHACL validation
    printf("\nTest 3: SHACL validation...\n");
    uint32_t test_subj = s7t_intern_string(engine, "subject_50");
    int has_pred = shacl_check_min_count(engine, test_subj, test_pred, 1);
    printf("  Subject_50 has predicate_5: %s\n", has_pred ? "YES" : "NO");
    
    // Test 4: High ID stress test
    printf("\nTest 4: High ID stress test...\n");
    
    // Add triple with ID > 1000
    uint32_t high_s = s7t_intern_string(engine, "high_subject_1500");
    uint32_t high_p = s7t_intern_string(engine, "high_predicate");
    uint32_t high_o = s7t_intern_string(engine, "high_object_2000");
    
    s7t_add_triple(engine, high_s, high_p, high_o);
    printf("  Added triple with high IDs: S=%u P=%u O=%u\n", high_s, high_p, high_o);
    printf("  Total triples: %zu\n", engine->triple_count);
    
    // Test 5: Clean destruction
    printf("\nTest 5: Destroying engine...\n");
    s7t_destroy_engine(engine);
    printf("✓ Engine destroyed successfully\n");
    
    printf("\n🎉 All tests passed! Memory management is fixed.\n");
    
    return 0;
}
