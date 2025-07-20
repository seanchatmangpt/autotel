/**
 * @file test_query_engine.c
 * @brief Comprehensive tests for query engine functionality
 * @author IntegrationTester Agent
 * @date 2024
 * 
 * Tests all Phase 2 query engine features:
 * - Pattern matching
 * - Variable binding
 * - Filtering
 * - Result iteration
 * - Performance optimization
 */

#include "test_utils.h"
#include "../include/query.h"
#include "../include/parser.h"
#include "../include/lexer.h"
#include <string.h>
#include <time.h>

// Global test stats
TestStats g_test_stats;

// Test data
static const char* test_ttl_dataset = 
    "@prefix ex: <http://example.org/> .\n"
    "@prefix foaf: <http://xmlns.com/foaf/0.1/> .\n"
    "@prefix rdf: <http://www.w3.org/1999/02/22-rdf-syntax-ns#> .\n"
    "@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> .\n"
    "@prefix xsd: <http://www.w3.org/2001/XMLSchema#> .\n"
    "\n"
    "# People\n"
    "ex:alice a foaf:Person ;\n"
    "    foaf:name \"Alice Johnson\" ;\n"
    "    foaf:age 28 ;\n"
    "    foaf:email \"alice@example.com\" ;\n"
    "    foaf:knows ex:bob, ex:carol ;\n"
    "    ex:salary \"75000\"^^xsd:decimal ;\n"
    "    ex:active true .\n"
    "\n"
    "ex:bob a foaf:Person ;\n"
    "    foaf:name \"Bob Smith\" ;\n"
    "    foaf:age 35 ;\n"
    "    foaf:email \"bob@example.com\" ;\n"
    "    foaf:knows ex:alice ;\n"
    "    ex:salary \"85000\"^^xsd:decimal ;\n"
    "    ex:active true .\n"
    "\n"
    "ex:carol a foaf:Person ;\n"
    "    foaf:name \"Carol Davis\" ;\n"
    "    foaf:age 42 ;\n"
    "    foaf:email \"carol@example.com\" ;\n"
    "    ex:salary \"95000\"^^xsd:decimal ;\n"
    "    ex:active false .\n"
    "\n"
    "# Organizations\n"
    "ex:TechCorp a ex:Company ;\n"
    "    ex:name \"TechCorp Inc.\" ;\n"
    "    ex:founded \"2010-01-01\"^^xsd:date ;\n"
    "    ex:employees ex:alice, ex:bob, ex:carol ;\n"
    "    ex:revenue \"10000000\"^^xsd:decimal .\n"
    "\n"
    "# Skills\n"
    "ex:alice ex:hasSkill ex:programming, ex:management .\n"
    "ex:bob ex:hasSkill ex:programming, ex:testing .\n"
    "ex:carol ex:hasSkill ex:management, ex:planning .\n"
    "\n"
    "# Skill definitions\n"
    "ex:programming rdfs:label \"Programming\"@en ;\n"
    "    rdfs:comment \"Software development skills\" .\n"
    "\n"
    "ex:management rdfs:label \"Management\"@en ;\n"
    "    rdfs:comment \"Team and project management\" .\n"
    "\n"
    "ex:testing rdfs:label \"Testing\"@en ;\n"
    "    rdfs:comment \"Software testing and QA\" .\n"
    "\n"
    "ex:planning rdfs:label \"Planning\"@en ;\n"
    "    rdfs:comment \"Strategic planning skills\" .\n";

/**
 * Setup query engine with test data
 */
static ttl_query_engine_t* setup_test_query_engine() {
    ttl_lexer_t* lexer = ttl_lexer_create();
    if (!lexer) return NULL;
    
    ttl_lexer_input_from_string(lexer, test_ttl_dataset);
    
    ttl_parser_t* parser = ttl_parser_create(lexer);
    if (!parser) {
        ttl_lexer_destroy(lexer);
        return NULL;
    }
    
    ttl_ast_node_t* ast = ttl_parser_parse_document(parser);
    if (!ast) {
        ttl_parser_destroy(parser);
        ttl_lexer_destroy(lexer);
        return NULL;
    }
    
    ttl_query_engine_t* engine = ttl_query_engine_create(ast, parser->context);
    
    // Note: Keep lexer and parser alive for the lifetime of the engine
    // In a real implementation, the engine would manage this
    
    return engine;
}

/**
 * Test basic query engine creation and destruction
 */
static int test_query_engine_creation() {
    printf("    Testing query engine creation...\n");
    
    ttl_query_engine_t* engine = setup_test_query_engine();
    TEST_ASSERT_NOT_NULL(engine);
    
    // Verify engine has processed the document
    size_t queries, patterns, results;
    ttl_query_engine_get_stats(engine, &queries, &patterns, &results);
    
    printf("      Engine stats: %zu queries, %zu patterns, %zu results\n", 
           queries, patterns, results);
    
    ttl_query_engine_destroy(engine);
    
    return 1;
}

/**
 * Test simple pattern matching
 */
static int test_simple_pattern_matching() {
    printf("    Testing simple pattern matching...\n");
    
    ttl_query_engine_t* engine = setup_test_query_engine();
    TEST_ASSERT_NOT_NULL(engine);
    
    // Test: Find all people
    ttl_query_pattern_t* pattern = ttl_query_pattern_create("?person", "a", "foaf:Person");
    TEST_ASSERT_NOT_NULL(pattern);
    
    ttl_query_result_t* result = ttl_query_execute(engine, pattern);
    TEST_ASSERT_NOT_NULL(result);
    
    size_t count = ttl_query_result_count(result);
    printf("      Found %zu people\n", count);
    TEST_ASSERT(count == 3); // alice, bob, carol
    
    // Test result iteration
    ttl_query_result_reset(result);
    int iteration_count = 0;
    do {
        const ttl_query_binding_t* binding = ttl_query_result_get_binding(result, "person");
        if (binding) {
            printf("      Person %d: %s\n", iteration_count + 1, binding->string_value);
            iteration_count++;
        }
    } while (ttl_query_result_next(result));
    
    TEST_ASSERT(iteration_count == count);
    
    ttl_query_result_destroy(result);
    ttl_query_pattern_destroy(pattern);
    ttl_query_engine_destroy(engine);
    
    return 1;
}

/**
 * Test variable binding in different positions
 */
static int test_variable_binding() {
    printf("    Testing variable binding...\n");
    
    ttl_query_engine_t* engine = setup_test_query_engine();
    TEST_ASSERT_NOT_NULL(engine);
    
    // Test: Find all properties of alice
    ttl_query_pattern_t* pattern = ttl_query_pattern_create("ex:alice", "?property", "?value");
    TEST_ASSERT_NOT_NULL(pattern);
    
    ttl_query_result_t* result = ttl_query_execute(engine, pattern);
    TEST_ASSERT_NOT_NULL(result);
    
    size_t count = ttl_query_result_count(result);
    printf("      Found %zu properties for Alice\n", count);
    TEST_ASSERT(count > 5); // Should have multiple properties
    
    // Check variable names
    char** variables;
    size_t var_count;
    ttl_query_result_get_variables(result, &variables, &var_count);
    TEST_ASSERT(var_count == 2); // property and value
    
    printf("      Variables: ");
    for (size_t i = 0; i < var_count; i++) {
        printf("%s ", variables[i]);
    }
    printf("\n");
    
    // Test specific bindings
    ttl_query_result_reset(result);
    bool found_name = false, found_age = false, found_email = false;
    
    do {
        const ttl_query_binding_t* prop_binding = ttl_query_result_get_binding(result, "property");
        const ttl_query_binding_t* val_binding = ttl_query_result_get_binding(result, "value");
        
        if (prop_binding && val_binding) {
            if (strstr(prop_binding->string_value, "name")) found_name = true;
            if (strstr(prop_binding->string_value, "age")) found_age = true;
            if (strstr(prop_binding->string_value, "email")) found_email = true;
        }
    } while (ttl_query_result_next(result));
    
    TEST_ASSERT(found_name);
    TEST_ASSERT(found_age);
    TEST_ASSERT(found_email);
    
    ttl_query_result_destroy(result);
    ttl_query_pattern_destroy(pattern);
    ttl_query_engine_destroy(engine);
    
    return 1;
}

/**
 * Test wildcard patterns
 */
static int test_wildcard_patterns() {
    printf("    Testing wildcard patterns...\n");
    
    ttl_query_engine_t* engine = setup_test_query_engine();
    TEST_ASSERT_NOT_NULL(engine);
    
    // Test: Find all triples (using wildcards)
    ttl_query_pattern_t* pattern = ttl_query_pattern_create("*", "*", "*");
    TEST_ASSERT_NOT_NULL(pattern);
    
    ttl_query_result_t* result = ttl_query_execute(engine, pattern);
    TEST_ASSERT_NOT_NULL(result);
    
    size_t count = ttl_query_result_count(result);
    printf("      Found %zu total triples\n", count);
    TEST_ASSERT(count > 20); // Should have many triples in the dataset
    
    ttl_query_result_destroy(result);
    ttl_query_pattern_destroy(pattern);
    
    // Test: Find all subjects with any property value
    pattern = ttl_query_pattern_create("?subject", "*", "*");
    result = ttl_query_execute(engine, pattern);
    TEST_ASSERT_NOT_NULL(result);
    
    count = ttl_query_result_count(result);
    printf("      Found %zu subject bindings\n", count);
    TEST_ASSERT(count > 0);
    
    ttl_query_result_destroy(result);
    ttl_query_pattern_destroy(pattern);
    ttl_query_engine_destroy(engine);
    
    return 1;
}

/**
 * Test query filters
 */
static int test_query_filters() {
    printf("    Testing query filters...\n");
    
    ttl_query_engine_t* engine = setup_test_query_engine();
    TEST_ASSERT_NOT_NULL(engine);
    
    // Test: Find people with specific name
    ttl_query_pattern_t* pattern = ttl_query_pattern_create("?person", "foaf:name", "?name");
    TEST_ASSERT_NOT_NULL(pattern);
    
    ttl_query_pattern_add_string_filter(pattern, "name", "Alice Johnson");
    
    ttl_query_result_t* result = ttl_query_execute(engine, pattern);
    TEST_ASSERT_NOT_NULL(result);
    
    size_t count = ttl_query_result_count(result);
    printf("      Found %zu people named 'Alice Johnson'\n", count);
    TEST_ASSERT(count == 1);
    
    ttl_query_result_destroy(result);
    ttl_query_pattern_destroy(pattern);
    
    // Test: Find people with age in range
    pattern = ttl_query_pattern_create("?person", "foaf:age", "?age");
    ttl_query_pattern_add_numeric_filter(pattern, "age", 30.0, 40.0);
    
    result = ttl_query_execute(engine, pattern);
    TEST_ASSERT_NOT_NULL(result);
    
    count = ttl_query_result_count(result);
    printf("      Found %zu people aged 30-40\n", count);
    TEST_ASSERT(count >= 1); // Should find Bob (35)
    
    ttl_query_result_destroy(result);
    ttl_query_pattern_destroy(pattern);
    
    // Test: Regex filter for email domains
    pattern = ttl_query_pattern_create("?person", "foaf:email", "?email");
    ttl_query_pattern_add_regex_filter(pattern, "email", "@example\\.com$", false);
    
    result = ttl_query_execute(engine, pattern);
    TEST_ASSERT_NOT_NULL(result);
    
    count = ttl_query_result_count(result);
    printf("      Found %zu people with @example.com emails\n", count);
    TEST_ASSERT(count == 3); // All test users have @example.com emails
    
    ttl_query_result_destroy(result);
    ttl_query_pattern_destroy(pattern);
    ttl_query_engine_destroy(engine);
    
    return 1;
}

/**
 * Test utility query functions
 */
static int test_utility_queries() {
    printf("    Testing utility query functions...\n");
    
    ttl_query_engine_t* engine = setup_test_query_engine();
    TEST_ASSERT_NOT_NULL(engine);
    
    // Test: Find by predicate
    ttl_query_result_t* result = ttl_query_find_by_predicate(engine, "foaf:name");
    TEST_ASSERT_NOT_NULL(result);
    
    size_t count = ttl_query_result_count(result);
    printf("      Found %zu names\n", count);
    TEST_ASSERT(count >= 3);
    
    ttl_query_result_destroy(result);
    
    // Test: Find by subject
    result = ttl_query_find_by_subject(engine, "ex:alice");
    TEST_ASSERT_NOT_NULL(result);
    
    count = ttl_query_result_count(result);
    printf("      Found %zu properties for Alice\n", count);
    TEST_ASSERT(count > 5);
    
    ttl_query_result_destroy(result);
    
    // Test: Find instances of type
    result = ttl_query_find_instances(engine, "foaf:Person");
    TEST_ASSERT_NOT_NULL(result);
    
    count = ttl_query_result_count(result);
    printf("      Found %zu Person instances\n", count);
    TEST_ASSERT(count == 3);
    
    ttl_query_result_destroy(result);
    ttl_query_engine_destroy(engine);
    
    return 1;
}

/**
 * Test simple query string execution
 */
static int test_simple_query_execution() {
    printf("    Testing simple query string execution...\n");
    
    ttl_query_engine_t* engine = setup_test_query_engine();
    TEST_ASSERT_NOT_NULL(engine);
    
    // Test simple query strings
    const char* queries[] = {
        "?s a foaf:Person",
        "ex:alice ?p ?o",
        "?s foaf:name ?name",
        "?s ex:hasSkill ?skill"
    };
    
    for (int i = 0; i < 4; i++) {
        ttl_query_result_t* result = ttl_query_execute_simple(engine, queries[i]);
        TEST_ASSERT_NOT_NULL(result);
        
        size_t count = ttl_query_result_count(result);
        printf("      Query '%s': %zu results\n", queries[i], count);
        TEST_ASSERT(count > 0);
        
        ttl_query_result_destroy(result);
    }
    
    ttl_query_engine_destroy(engine);
    
    return 1;
}

/**
 * Test result output formats
 */
static int test_result_output_formats() {
    printf("    Testing result output formats...\n");
    
    ttl_query_engine_t* engine = setup_test_query_engine();
    TEST_ASSERT_NOT_NULL(engine);
    
    ttl_query_result_t* result = ttl_query_find_instances(engine, "foaf:Person");
    TEST_ASSERT_NOT_NULL(result);
    
    // Test table format output
    FILE* table_output = tmpfile();
    TEST_ASSERT_NOT_NULL(table_output);
    
    ttl_query_result_print(result, table_output);
    
    rewind(table_output);
    fseek(table_output, 0, SEEK_END);
    long table_size = ftell(table_output);
    TEST_ASSERT(table_size > 0);
    
    printf("      Table format: %ld bytes\n", table_size);
    fclose(table_output);
    
    // Test JSON format output
    FILE* json_output = tmpfile();
    TEST_ASSERT_NOT_NULL(json_output);
    
    ttl_query_result_print_json(result, json_output);
    
    rewind(json_output);
    fseek(json_output, 0, SEEK_END);
    long json_size = ftell(json_output);
    TEST_ASSERT(json_size > 0);
    
    printf("      JSON format: %ld bytes\n", json_size);
    
    // Verify JSON structure
    rewind(json_output);
    char json_buffer[4096];
    size_t read_size = fread(json_buffer, 1, sizeof(json_buffer) - 1, json_output);
    json_buffer[read_size] = '\0';
    
    TEST_ASSERT(strstr(json_buffer, "{") != NULL);
    TEST_ASSERT(strstr(json_buffer, "}") != NULL);
    TEST_ASSERT(strstr(json_buffer, "results") != NULL || 
                strstr(json_buffer, "bindings") != NULL);
    
    fclose(json_output);
    ttl_query_result_destroy(result);
    ttl_query_engine_destroy(engine);
    
    return 1;
}

/**
 * Test query performance with large datasets
 */
static int test_query_performance() {
    printf("    Testing query performance...\n");
    
    // Generate larger test dataset
    char* large_dataset = malloc(50000);
    TEST_ASSERT_NOT_NULL(large_dataset);
    
    strcpy(large_dataset, 
        "@prefix ex: <http://example.org/> .\n"
        "@prefix foaf: <http://xmlns.com/foaf/0.1/> .\n\n");
    
    char* pos = large_dataset + strlen(large_dataset);
    
    // Generate 1000 people
    for (int i = 0; i < 1000; i++) {
        pos += sprintf(pos, 
            "ex:person%d a foaf:Person ;\n"
            "    foaf:name \"Person %d\" ;\n"
            "    foaf:age %d ;\n"
            "    ex:id %d .\n\n",
            i, i, 20 + (i % 50), i);
    }
    
    // Parse large dataset
    ttl_lexer_t* lexer = ttl_lexer_create();
    ttl_lexer_input_from_string(lexer, large_dataset);
    
    ttl_parser_t* parser = ttl_parser_create(lexer);
    ttl_ast_node_t* ast = ttl_parser_parse_document(parser);
    TEST_ASSERT_NOT_NULL(ast);
    
    ttl_query_engine_t* engine = ttl_query_engine_create(ast, parser->context);
    TEST_ASSERT_NOT_NULL(engine);
    
    // Benchmark different query types
    clock_t start, end;
    double times[3];
    
    // 1. Simple type query
    start = clock();
    ttl_query_result_t* result1 = ttl_query_find_instances(engine, "foaf:Person");
    end = clock();
    times[0] = ((double)(end - start)) / CLOCKS_PER_SEC * 1000;
    
    TEST_ASSERT_NOT_NULL(result1);
    size_t count1 = ttl_query_result_count(result1);
    printf("      Type query: %zu results in %.2f ms\n", count1, times[0]);
    TEST_ASSERT(count1 == 1000);
    ttl_query_result_destroy(result1);
    
    // 2. Pattern matching query
    start = clock();
    ttl_query_pattern_t* pattern = ttl_query_pattern_create("?person", "foaf:name", "?name");
    ttl_query_result_t* result2 = ttl_query_execute(engine, pattern);
    end = clock();
    times[1] = ((double)(end - start)) / CLOCKS_PER_SEC * 1000;
    
    TEST_ASSERT_NOT_NULL(result2);
    size_t count2 = ttl_query_result_count(result2);
    printf("      Pattern query: %zu results in %.2f ms\n", count2, times[1]);
    TEST_ASSERT(count2 == 1000);
    ttl_query_result_destroy(result2);
    ttl_query_pattern_destroy(pattern);
    
    // 3. Filtered query
    start = clock();
    pattern = ttl_query_pattern_create("?person", "foaf:age", "?age");
    ttl_query_pattern_add_numeric_filter(pattern, "age", 30.0, 40.0);
    ttl_query_result_t* result3 = ttl_query_execute(engine, pattern);
    end = clock();
    times[2] = ((double)(end - start)) / CLOCKS_PER_SEC * 1000;
    
    TEST_ASSERT_NOT_NULL(result3);
    size_t count3 = ttl_query_result_count(result3);
    printf("      Filtered query: %zu results in %.2f ms\n", count3, times[2]);
    TEST_ASSERT(count3 > 0 && count3 < 1000); // Should be filtered subset
    ttl_query_result_destroy(result3);
    ttl_query_pattern_destroy(pattern);
    
    // Performance assertions - should complete within reasonable time
    for (int i = 0; i < 3; i++) {
        TEST_ASSERT(times[i] < 1000); // Should complete within 1 second
    }
    
    free(large_dataset);
    ttl_query_engine_destroy(engine);
    ttl_parser_destroy(parser);
    ttl_lexer_destroy(lexer);
    
    return 1;
}

/**
 * Test error handling in queries
 */
static int test_query_error_handling() {
    printf("    Testing query error handling...\n");
    
    ttl_query_engine_t* engine = setup_test_query_engine();
    TEST_ASSERT_NOT_NULL(engine);
    
    // Test with invalid patterns
    ttl_query_result_t* result = ttl_query_execute_simple(engine, "invalid query syntax");
    TEST_ASSERT_NULL(result);
    
    // Test with NULL inputs
    result = ttl_query_execute(NULL, NULL);
    TEST_ASSERT_NULL(result);
    
    result = ttl_query_execute_simple(NULL, "?s ?p ?o");
    TEST_ASSERT_NULL(result);
    
    result = ttl_query_execute_simple(engine, NULL);
    TEST_ASSERT_NULL(result);
    
    // Test pattern creation with invalid inputs
    ttl_query_pattern_t* pattern = ttl_query_pattern_create(NULL, NULL, NULL);
    TEST_ASSERT_NULL(pattern);
    
    printf("      Error conditions: Handled correctly\n");
    
    ttl_query_engine_destroy(engine);
    
    return 1;
}

/**
 * Run all query engine tests
 */
static void run_query_engine_tests() {
    RUN_TEST(test_query_engine_creation);
    RUN_TEST(test_simple_pattern_matching);
    RUN_TEST(test_variable_binding);
    RUN_TEST(test_wildcard_patterns);
    RUN_TEST(test_query_filters);
    RUN_TEST(test_utility_queries);
    RUN_TEST(test_simple_query_execution);
    RUN_TEST(test_result_output_formats);
    RUN_TEST(test_query_performance);
    RUN_TEST(test_query_error_handling);
}

/**
 * Main test entry point
 */
int main(int argc, char* argv[]) {
    printf("🔍 TTL Parser - Query Engine Test Suite\n");
    printf("======================================\n");
    
    init_test_stats();
    
    RUN_TEST_SUITE("Query Engine", run_query_engine_tests);
    
    print_test_summary();
    check_memory_leaks();
    
    return g_test_stats.failed == 0 ? 0 : 1;
}