#include "cns/owl.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    printf("=== Debugging Transitive Reasoning ===\n");
    
    CNSOWLEngine *engine = cns_owl_create(100);
    if (!engine) {
        printf("Failed to create engine\n");
        return 1;
    }
    
    uint32_t ancestor = 10;
    uint32_t alice = 20;
    uint32_t bob = 21;
    uint32_t charlie = 22;
    
    // Set up transitive property
    printf("Setting property %u as transitive\n", ancestor);
    int result = cns_owl_set_transitive(engine, ancestor);
    printf("Set transitive result: %d\n", result);
    
    // Check if it's actually transitive
    bool is_trans = cns_owl_has_property_characteristic(engine, ancestor, OWL_TRANSITIVE);
    printf("Property %u is transitive: %s\n", ancestor, is_trans ? "YES" : "NO");
    
    // Add axioms
    printf("\nAdding axioms:\n");
    result = cns_owl_add_axiom(engine, alice, ancestor, bob, 0);
    printf("Add axiom (alice->bob) result: %d\n", result);
    printf("Axiom count after first add: %zu\n", engine->axiom_count);
    
    result = cns_owl_add_axiom(engine, bob, ancestor, charlie, 0);
    printf("Add axiom (bob->charlie) result: %d\n", result);
    printf("Axiom count after second add: %zu\n", engine->axiom_count);
    
    // Check axioms
    printf("\nChecking axioms in engine:\n");
    for (size_t i = 0; i < engine->axiom_count; i++) {
        OWLAxiom *axiom = &engine->axioms[i];
        printf("Axiom %zu: subject=%u, predicate=%u, object=%u, type=%u\n",
               i, axiom->subject_id, axiom->predicate_id, axiom->object_id, axiom->axiom_type);
    }
    
    // Test queries
    printf("\nTesting queries:\n");
    bool result1 = cns_owl_transitive_query(engine, alice, ancestor, bob);
    printf("Alice ancestor of Bob: %s\n", result1 ? "YES" : "NO");
    
    bool result2 = cns_owl_transitive_query(engine, bob, ancestor, charlie);
    printf("Bob ancestor of Charlie: %s\n", result2 ? "YES" : "NO");
    
    bool result3 = cns_owl_transitive_query(engine, alice, ancestor, charlie);
    printf("Alice ancestor of Charlie (transitive): %s\n", result3 ? "YES" : "NO");
    
    cns_owl_destroy(engine);
    return 0;
}