// Simple test for query engine with hardcoded triples to verify functionality
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "query.h"

int main() {
    printf("=== TTL Query Engine Priority 2 Test ===\n");
    
    // Create AST context
    ttl_ast_context_t *context = ttl_ast_context_create(true);
    if (!context) {
        printf("ERROR: Failed to create AST context\n");
        return 1;
    }
    
    // Create a simple document with test triples
    ttl_ast_node_t *document = ttl_ast_create_document(context);
    if (!document) {
        printf("ERROR: Failed to create document\n");
        ttl_ast_context_destroy(context);
        return 1;
    }
    
    printf("✓ Created document and context\n");
    
    // Create test triple 1: ex:john a foaf:Person
    ttl_ast_node_t *subject1 = ttl_ast_create_prefixed_name(context, "ex", "john");
    ttl_ast_node_t *predicate1 = ttl_ast_create_rdf_type(context);
    ttl_ast_node_t *object1 = ttl_ast_create_prefixed_name(context, "foaf", "Person");
    
    if (subject1 && predicate1 && object1) {
        ttl_ast_node_t *po_list = ttl_ast_create_predicate_object_list(context);
        ttl_ast_node_t *obj_list = ttl_ast_create_object_list(context);
        ttl_ast_add_object(obj_list, object1);
        ttl_ast_add_predicate_object(po_list, predicate1, obj_list);
        
        ttl_ast_node_t *triple = ttl_ast_create_triple(context, subject1, po_list);
        ttl_ast_add_statement(document, triple);
        printf("✓ Created test triple 1: ex:john a foaf:Person\n");
    }
    
    // Create test triple 2: ex:john foaf:name "John Doe"
    ttl_ast_node_t *subject2 = ttl_ast_create_prefixed_name(context, "ex", "john");
    ttl_ast_node_t *predicate2 = ttl_ast_create_prefixed_name(context, "foaf", "name");
    ttl_ast_node_t *object2 = ttl_ast_create_string_literal(context, "John Doe", TTL_STRING_DOUBLE_QUOTE);
    
    if (subject2 && predicate2 && object2) {
        ttl_ast_node_t *po_list = ttl_ast_create_predicate_object_list(context);
        ttl_ast_node_t *obj_list = ttl_ast_create_object_list(context);
        ttl_ast_add_object(obj_list, object2);
        ttl_ast_add_predicate_object(po_list, predicate2, obj_list);
        
        ttl_ast_node_t *triple = ttl_ast_create_triple(context, subject2, po_list);
        ttl_ast_add_statement(document, triple);
        printf("✓ Created test triple 2: ex:john foaf:name \"John Doe\"\n");
    }
    
    // Create query engine
    ttl_query_engine_t *engine = ttl_query_engine_create(document, context);
    if (!engine) {
        printf("ERROR: Failed to create query engine\n");
        ttl_ast_node_unref(document);
        ttl_ast_context_destroy(context);
        return 1;
    }
    
    printf("✓ Created query engine\n\n");
    
    // Test Priority 2: Basic Pattern Matching
    printf("=== Priority 2: Basic Pattern Matching Tests ===\n");
    
    // Test 1: ?s ?p ?o (find all triples)
    printf("\nTest 1: ?s ?p ?o (find all triples)\n");
    ttl_query_result_t *result1 = ttl_query_execute_simple(engine, "?s ?p ?o");
    if (result1) {
        size_t count = ttl_query_result_count(result1);
        printf("Found %zu results\n", count);
        if (count > 0) {
            ttl_query_result_print(result1, stdout);
        }
        ttl_query_result_destroy(result1);
    } else {
        printf("Query failed\n");
    }
    
    // Test 2: ?s a foaf:Person (find persons)
    printf("\nTest 2: ?s a foaf:Person (find persons)\n");
    ttl_query_result_t *result2 = ttl_query_execute_simple(engine, "?s a foaf:Person");
    if (result2) {
        size_t count = ttl_query_result_count(result2);
        printf("Found %zu results\n", count);
        if (count > 0) {
            ttl_query_result_print(result2, stdout);
        }
        ttl_query_result_destroy(result2);
    } else {
        printf("Query failed\n");
    }
    
    // Test 3: ex:john ?p ?o (find properties of john)
    printf("\nTest 3: ex:john ?p ?o (find properties of john)\n");
    ttl_query_result_t *result3 = ttl_query_execute_simple(engine, "ex:john ?p ?o");
    if (result3) {
        size_t count = ttl_query_result_count(result3);
        printf("Found %zu results\n", count);
        if (count > 0) {
            ttl_query_result_print(result3, stdout);
        }
        ttl_query_result_destroy(result3);
    } else {
        printf("Query failed\n");
    }
    
    // Test 4: ?s foaf:name ?name (find names)
    printf("\nTest 4: ?s foaf:name ?name (find names)\n");
    ttl_query_result_t *result4 = ttl_query_execute_simple(engine, "?s foaf:name ?name");
    if (result4) {
        size_t count = ttl_query_result_count(result4);
        printf("Found %zu results\n", count);
        if (count > 0) {
            ttl_query_result_print(result4, stdout);
        }
        ttl_query_result_destroy(result4);
    } else {
        printf("Query failed\n");
    }
    
    // Get statistics
    size_t queries_executed, patterns_matched, total_results;
    ttl_query_engine_get_stats(engine, &queries_executed, &patterns_matched, &total_results);
    printf("\n=== Query Engine Statistics ===\n");
    printf("  Queries executed: %zu\n", queries_executed);
    printf("  Patterns matched: %zu\n", patterns_matched);
    printf("  Total results: %zu\n", total_results);
    
    // Cleanup
    ttl_query_engine_destroy(engine);
    ttl_ast_node_unref(document);
    ttl_ast_context_destroy(context);
    
    printf("\n=== Priority 2 Test completed ===\n");
    return 0;
}