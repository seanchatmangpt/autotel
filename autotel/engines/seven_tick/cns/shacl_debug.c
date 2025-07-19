#include "shacl_49_cycle_optimized.h"
#include <stdio.h>
#include <assert.h>

void debug_node_properties(CNSSparqlEngine* engine, uint32_t node_id) {
    printf("\nDebug Node %u:\n", node_id);
    
    // Check type
    bool is_person = cns_sparql_ask_pattern(engine, node_id, ID_rdf_type, ID_Person);
    bool is_company = cns_sparql_ask_pattern(engine, node_id, ID_rdf_type, ID_Company);
    printf("  Type: %s %s\n", 
           is_person ? "Person" : "", 
           is_company ? "Company" : "");
    
    // Count properties
    uint32_t email_count = fast_property_count(engine, node_id, ID_hasEmail);
    uint32_t name_count = fast_property_count(engine, node_id, ID_hasName);
    uint32_t phone_count = fast_property_count(engine, node_id, ID_phoneNumber);
    uint32_t works_count = fast_property_count(engine, node_id, ID_worksAt);
    
    printf("  Properties:\n");
    printf("    hasEmail: %u\n", email_count);
    printf("    hasName: %u\n", name_count);
    printf("    phoneNumber: %u\n", phone_count);
    printf("    worksAt: %u\n", works_count);
    
    // Validate
    bool person_valid = fast_validate_PersonShape_aot(engine, node_id);
    bool company_valid = fast_validate_CompanyShape_aot(engine, node_id);
    bool all_valid = fast_validate_all_shapes(engine, node_id);
    
    printf("  Validation:\n");
    printf("    PersonShape: %s\n", person_valid ? "✅ VALID" : "❌ INVALID");
    printf("    CompanyShape: %s\n", company_valid ? "✅ VALID" : "❌ INVALID");
    printf("    All Shapes: %s\n", all_valid ? "✅ VALID" : "❌ INVALID");
}

int main() {
    printf("🔍 SHACL Validation Debug\n\n");
    
    // Create engine
    CNSSparqlEngine* engine = cns_sparql_create(1000, 100, 1000);
    assert(engine != NULL);
    
    // Initialize cache
    shacl_cache_init();
    
    // Setup test data (same as benchmark)
    // Valid person (node 1)
    cns_sparql_add_triple(engine, 1, ID_rdf_type, ID_Person);
    cns_sparql_add_triple(engine, 1, ID_hasEmail, 100);
    
    // Invalid person - no email (node 2) 
    cns_sparql_add_triple(engine, 2, ID_rdf_type, ID_Person);
    
    // Invalid person - too many emails (node 3)
    cns_sparql_add_triple(engine, 3, ID_rdf_type, ID_Person);
    for (int i = 0; i < 6; i++) {
        cns_sparql_add_triple(engine, 3, ID_hasEmail, 200 + i);
    }
    
    // Valid company (node 4)
    cns_sparql_add_triple(engine, 4, ID_rdf_type, ID_Company);
    cns_sparql_add_triple(engine, 4, ID_hasName, 300);
    
    // Invalid company - no name (node 5)
    cns_sparql_add_triple(engine, 5, ID_rdf_type, ID_Company);
    
    // Debug each node
    for (uint32_t node = 1; node <= 5; node++) {
        debug_node_properties(engine, node);
    }
    
    // Test property counting directly
    printf("\n🔧 Direct Property Count Tests:\n");
    for (uint32_t node = 1; node <= 5; node++) {
        printf("Node %u hasEmail count: %u\n", 
               node, fast_property_count(engine, node, ID_hasEmail));
    }
    
    // Cleanup
    cns_sparql_destroy(engine);
    
    return 0;
}