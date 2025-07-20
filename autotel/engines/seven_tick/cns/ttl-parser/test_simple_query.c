// Simple test for query engine functionality
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "query.h"

int main() {
    printf("=== TTL Query Engine Test ===\n");
    
    // Create AST context
    ttl_ast_context_t *context = ttl_ast_context_create(true);
    if (!context) {
        printf("ERROR: Failed to create AST context\n");
        return 1;
    }
    
    // Create a simple document with sample triples
    ttl_ast_node_t *document = ttl_ast_create_document(context);
    if (!document) {
        printf("ERROR: Failed to create document\n");
        ttl_ast_context_destroy(context);
        return 1;
    }
    
    printf("✓ Created document and context\n");
    
    // Create sample triple: <http://example.org/person1> rdf:type foaf:Person
    ttl_ast_node_t *subject = ttl_ast_create_iri(context, "http://example.org/person1");
    ttl_ast_node_t *predicate = ttl_ast_create_rdf_type(context);
    ttl_ast_node_t *object = ttl_ast_create_prefixed_name(context, "foaf", "Person");
    
    if (subject && predicate && object) {
        ttl_ast_node_t *po_list = ttl_ast_create_predicate_object_list(context);
        ttl_ast_node_t *obj_list = ttl_ast_create_object_list(context);
        ttl_ast_add_object(obj_list, object);
        ttl_ast_add_predicate_object(po_list, predicate, obj_list);
        
        ttl_ast_node_t *triple = ttl_ast_create_triple(context, subject, po_list);
        ttl_ast_add_statement(document, triple);
        printf("✓ Created sample triple\n");
    } else {
        printf("ERROR: Failed to create sample triple\n");
    }
    
    // Create query engine
    ttl_query_engine_t *engine = ttl_query_engine_create(document, context);
    if (!engine) {
        printf("ERROR: Failed to create query engine\n");
        ttl_ast_node_unref(document);
        ttl_ast_context_destroy(context);
        return 1;
    }
    
    printf("✓ Created query engine\n");
    
    // Test simple query pattern
    ttl_query_pattern_t *pattern = ttl_query_pattern_create("?s", "a", "foaf:Person");
    if (!pattern) {
        printf("ERROR: Failed to create query pattern\n");
        goto cleanup;
    }
    
    printf("✓ Created query pattern: ?s a foaf:Person\n");
    
    // Execute query
    ttl_query_result_t *result = ttl_query_execute(engine, pattern);
    if (!result) {
        printf("ERROR: Query execution failed\n");
        ttl_query_pattern_destroy(pattern);
        goto cleanup;
    }
    
    size_t count = ttl_query_result_count(result);
    printf("✓ Query executed successfully\n");
    printf("✓ Found %zu results\n", count);
    
    // Print results
    if (count > 0) {
        printf("\nResults:\n");
        ttl_query_result_print(result, stdout);
    }
    
    // Get statistics
    size_t queries_executed, patterns_matched, total_results;
    ttl_query_engine_get_stats(engine, &queries_executed, &patterns_matched, &total_results);
    printf("\nStatistics:\n");
    printf("  Queries executed: %zu\n", queries_executed);
    printf("  Patterns matched: %zu\n", patterns_matched);
    printf("  Total results: %zu\n", total_results);
    
    // Cleanup
    ttl_query_result_destroy(result);
    ttl_query_pattern_destroy(pattern);
    
cleanup:
    ttl_query_engine_destroy(engine);
    ttl_ast_node_unref(document);
    ttl_ast_context_destroy(context);
    
    printf("\n=== Test completed ===\n");
    return 0;
}