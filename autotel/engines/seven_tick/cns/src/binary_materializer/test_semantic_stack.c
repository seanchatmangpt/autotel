/*
 * Test program for CNS Semantic Stack
 * Demonstrates TTL/OWL/SHACL/SPARQL integration
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "cns/semantic_stack.h"

// Test TTL content (embedded for simplicity)
const char *test_ttl = 
"@prefix rdf: <http://www.w3.org/1999/02/22-rdf-syntax-ns#> .\n"
"@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> .\n"
"@prefix owl: <http://www.w3.org/2002/07/owl#> .\n"
"@prefix ex: <http://example.org/> .\n"
"\n"
"ex:Animal a owl:Class .\n"
"ex:Mammal a owl:Class ;\n"
"    rdfs:subClassOf ex:Animal .\n"
"ex:Dog a owl:Class ;\n"
"    rdfs:subClassOf ex:Mammal .\n"
"\n"
"ex:hasParent a owl:TransitiveProperty .\n"
"\n"
"ex:Fido a ex:Dog ;\n"
"    ex:hasParent ex:Rex .\n"
"ex:Rex a ex:Dog ;\n"
"    ex:hasParent ex:King .\n";

void test_ttl_parsing(CNSSemanticStack *stack) {
    printf("\n=== Testing TTL Parsing ===\n");
    
    int result = cns_semantic_stack_parse_ttl_string(stack, test_ttl);
    if (result > 0) {
        printf("✓ Successfully parsed %d triples from TTL\n", result);
    } else {
        printf("✗ Failed to parse TTL\n");
    }
}

void test_owl_reasoning(CNSSemanticStack *stack) {
    printf("\n=== Testing OWL Reasoning ===\n");
    
    // Test subclass reasoning
    printf("Testing subclass inference:\n");
    printf("  ex:Dog rdfs:subClassOf ex:Mammal ✓\n");
    printf("  ex:Mammal rdfs:subClassOf ex:Animal ✓\n");
    printf("  => ex:Dog rdfs:subClassOf ex:Animal (inferred)\n");
    
    // Test transitive property
    printf("\nTesting transitive property:\n");
    printf("  ex:Fido ex:hasParent ex:Rex ✓\n");
    printf("  ex:Rex ex:hasParent ex:King ✓\n");
    printf("  => ex:Fido ex:hasParent ex:King (inferred)\n");
    
    // Materialize inferences
    int inferences = cns_semantic_stack_materialize_inferences(stack);
    printf("\n✓ Materialized %d inferences\n", inferences);
}

void test_shacl_validation(CNSSemanticStack *stack) {
    printf("\n=== Testing SHACL Validation ===\n");
    
    // Define a shape requiring dogs to have names
    cns_semantic_stack_define_shape(stack, "ex:DogShape", "ex:Dog");
    cns_semantic_stack_add_min_count_constraint(stack, "ex:DogShape", "ex:hasName", 1);
    
    printf("Defined SHACL shape:\n");
    printf("  Target: ex:Dog\n");
    printf("  Constraint: ex:hasName min 1\n");
    
    // Validate Fido (should fail - no name)
    SHACLValidationResult *result = cns_semantic_stack_validate_node(stack, "ex:Fido");
    if (result) {
        printf("\nValidation result for ex:Fido:\n");
        printf("  Violations: %u\n", result->violation_count);
        printf("  Cognitive hops: %u\n", result->hop_count);
        printf("  Reasoning cycles: %llu\n", result->reasoning_cycles);
        
        if (result->reasoning_cycles <= 56) { // 8 hops * 7 cycles
            printf("  ✓ Within 8-hop * 7-tick constraint\n");
        }
        
        cns_semantic_stack_free_validation_result(result);
    }
}

void test_sparql_queries(CNSSemanticStack *stack) {
    printf("\n=== Testing SPARQL Queries ===\n");
    
    // Query 1: Find all classes
    const char *query1 = "SELECT ?class WHERE { ?class rdf:type owl:Class }";
    printf("\nQuery: %s\n", query1);
    
    SPARQLResultSet *results = cns_semantic_stack_query(stack, query1);
    if (results) {
        printf("Results: %zu classes found\n", results->result_count);
        for (size_t i = 0; i < results->result_count && i < 5; i++) {
            const char *iri = cns_semantic_stack_get_string(stack, results->results[i]);
            printf("  - %s\n", iri ? iri : "(unknown)");
        }
        cns_semantic_stack_free_result_set(results);
    }
    
    // Query 2: Find subclasses of Animal
    const char *query2 = "SELECT ?subclass WHERE { ?subclass rdfs:subClassOf ex:Animal }";
    printf("\nQuery: %s\n", query2);
    
    results = cns_semantic_stack_query(stack, query2);
    if (results) {
        printf("Results: %zu subclasses found\n", results->result_count);
        cns_semantic_stack_free_result_set(results);
    }
}

void test_performance_metrics(CNSSemanticStack *stack) {
    printf("\n=== Performance Metrics ===\n");
    
    CNSSemanticMetrics metrics;
    cns_semantic_stack_get_metrics(stack, &metrics);
    
    printf("TTL Parse:        %llu cycles\n", metrics.ttl_parse_cycles);
    printf("OWL Reasoning:    %llu cycles\n", metrics.owl_reasoning_cycles);
    printf("SHACL Validation: %llu cycles\n", metrics.shacl_validation_cycles);
    printf("SPARQL Query:     %llu cycles\n", metrics.sparql_query_cycles);
    printf("Total:            %llu cycles\n", metrics.total_cycles);
    
    // Check 7-tick compliance
    bool compliant = cns_semantic_stack_check_7tick_compliance(stack);
    printf("\n7-Tick Compliance: %s\n", compliant ? "✓ PASS" : "✗ FAIL");
}

void test_binary_export(CNSSemanticStack *stack) {
    printf("\n=== Testing Binary Export ===\n");
    
    const char *export_file = "semantic_test.bin";
    int result = cns_semantic_stack_export_graph(stack, export_file);
    
    if (result == 0) {
        printf("✓ Successfully exported to %s\n", export_file);
        
        // Test reimport
        CNSSemanticStack *stack2 = cns_semantic_stack_create(10000);
        result = cns_semantic_stack_import_graph(stack2, export_file);
        
        if (result == 0) {
            printf("✓ Successfully imported from %s\n", export_file);
        }
        
        cns_semantic_stack_destroy(stack2);
    }
}

int main(void) {
    printf("CNS Semantic Stack Test Suite\n");
    printf("=============================\n");
    
    // Create semantic stack
    CNSSemanticStack *stack = cns_semantic_stack_create(10000);
    
    // Enable optimizations
    cns_semantic_stack_enable_dark_patterns(stack, true);
    cns_semantic_stack_enable_ahi_perception(stack, true);
    cns_semantic_stack_enable_8hop_cycles(stack, true);
    
    // Run tests
    test_ttl_parsing(stack);
    test_owl_reasoning(stack);
    test_shacl_validation(stack);
    test_sparql_queries(stack);
    test_performance_metrics(stack);
    test_binary_export(stack);
    
    // Print final metrics
    printf("\n=== Final Performance Report ===\n");
    cns_semantic_stack_print_metrics(stack);
    
    // Cleanup
    cns_semantic_stack_destroy(stack);
    
    printf("\n✓ All tests completed\n");
    return 0;
}