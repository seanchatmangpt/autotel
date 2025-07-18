#include <stdio.h>
#include <stdlib.h>
#include "sparql7t.h"
#include "shacl7t.h"
#include "owl7t.h"

int main() {
    printf("Creating engines...\n");
    
    // Create smaller test instances
    S7TEngine* sparql = s7t_create(1000, 10, 1000);
    if (!sparql) {
        printf("Failed to create SPARQL engine\n");
        return 1;
    }
    printf("SPARQL engine created\n");
    
    ShaclEngine* shacl = shacl_create(1000, 10);
    if (!shacl) {
        printf("Failed to create SHACL engine\n");
        return 1;
    }
    printf("SHACL engine created\n");
    
    OWLEngine* owl = owl_create(sparql, 100, 10);
    if (!owl) {
        printf("Failed to create OWL engine\n");
        return 1;
    }
    printf("OWL engine created\n");
    
    // Test basic operations
    printf("\nTesting basic operations...\n");
    
    // Add a triple
    s7t_add_triple(sparql, 1, 0, 10);
    printf("Added triple (1, 0, 10)\n");
    
    // Query it
    int result = s7t_ask_pattern(sparql, 1, 0, 10);
    printf("Query result: %d\n", result);
    
    // Add OWL subclass
    owl_add_subclass(owl, 11, 10);
    printf("Added subclass relation\n");
    
    // Test SHACL
    shacl_set_node_class(shacl, 1, 10);
    printf("Set node class\n");
    
    CompiledShape shape = {
        .target_class_mask = 1ULL << 10,
        .property_mask = 0,
        .constraint_flags = SHACL_TARGET_CLASS
    };
    shacl_add_shape(shacl, 0, &shape);
    printf("Added shape\n");
    
    int valid = shacl_validate_node(shacl, 1, 0);
    printf("Validation result: %d\n", valid);
    
    printf("\nAll tests passed!\n");
    
    // Cleanup
    owl_destroy(owl);
    free(shacl->node_class_vectors);
    free(shacl->node_property_vectors);
    free(shacl->property_value_vectors);
    free(shacl->node_datatype_index);
    free(shacl->shapes);
    free(shacl);
    
    free(sparql->predicate_vectors);
    free(sparql->object_vectors);
    free(sparql->ps_to_o_index);
    free(sparql);
    
    return 0;
}