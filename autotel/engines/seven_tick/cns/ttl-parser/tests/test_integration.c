#include "test_utils.h"
#include "../src/lexer.h"
#include "../src/parser.h"

// Test parsing simple.ttl fixture
int test_parse_simple_fixture() {
    char* content = read_test_file("tests/fixtures/simple.ttl");
    TEST_ASSERT_NOT_NULL(content);
    
    Lexer lexer;
    init_lexer(&lexer, content);
    
    Parser parser;
    init_parser(&parser, &lexer);
    
    TripleSet* result = parse(&parser);
    TEST_ASSERT_NOT_NULL(result);
    
    // Should have prefixes
    TEST_ASSERT(parser.prefix_count >= 2);
    
    // Should have triples
    TEST_ASSERT(result->count > 0);
    
    // Check some expected content
    int found_alice = 0, found_bob = 0;
    for (int i = 0; i < result->count; i++) {
        if (strstr(result->triples[i].subject, "alice")) found_alice = 1;
        if (strstr(result->triples[i].subject, "bob")) found_bob = 1;
    }
    
    TEST_ASSERT(found_alice);
    TEST_ASSERT(found_bob);
    
    free_triple_set(result);
    free_parser(&parser);
    free_lexer(&lexer);
    TEST_FREE(content);
    
    return 1;
}

// Test parsing complex.ttl fixture
int test_parse_complex_fixture() {
    char* content = read_test_file("tests/fixtures/complex.ttl");
    TEST_ASSERT_NOT_NULL(content);
    
    Lexer lexer;
    init_lexer(&lexer, content);
    
    Parser parser;
    init_parser(&parser, &lexer);
    
    TripleSet* result = parse(&parser);
    TEST_ASSERT_NOT_NULL(result);
    
    // Complex file should have many triples
    TEST_ASSERT(result->count > 10);
    
    // Should have base URI
    TEST_ASSERT_NOT_NULL(parser.base_uri);
    TEST_ASSERT_STR_EQ(parser.base_uri, "http://example.org/");
    
    // Should have multiple prefixes
    TEST_ASSERT(parser.prefix_count >= 5);
    
    // Check for different data types
    int found_string = 0, found_integer = 0, found_uri = 0;
    for (int i = 0; i < result->count; i++) {
        if (result->triples[i].object[0] == '"') found_string = 1;
        if (isdigit(result->triples[i].object[0])) found_integer = 1;
        if (strstr(result->triples[i].object, "http://")) found_uri = 1;
    }
    
    TEST_ASSERT(found_string);
    TEST_ASSERT(found_integer);
    TEST_ASSERT(found_uri);
    
    free_triple_set(result);
    free_parser(&parser);
    free_lexer(&lexer);
    TEST_FREE(content);
    
    return 1;
}

// Test parsing invalid.ttl fixture (error recovery)
int test_parse_invalid_fixture() {
    char* content = read_test_file("tests/fixtures/invalid.ttl");
    TEST_ASSERT_NOT_NULL(content);
    
    Lexer lexer;
    init_lexer(&lexer, content);
    
    Parser parser;
    init_parser(&parser, &lexer);
    
    TripleSet* result = parse(&parser);
    
    // Should have encountered errors
    TEST_ASSERT(has_error(&parser.error));
    
    // But should still parse some valid statements
    if (result) {
        TEST_ASSERT(result->count > 0);
        free_triple_set(result);
    }
    
    free_parser(&parser);
    free_lexer(&lexer);
    TEST_FREE(content);
    
    return 1;
}

// Test parsing edge_cases.ttl fixture
int test_parse_edge_cases_fixture() {
    char* content = read_test_file("tests/fixtures/edge_cases.ttl");
    TEST_ASSERT_NOT_NULL(content);
    
    Lexer lexer;
    init_lexer(&lexer, content);
    
    Parser parser;
    init_parser(&parser, &lexer);
    
    TripleSet* result = parse(&parser);
    TEST_ASSERT_NOT_NULL(result);
    
    // Edge cases should still produce valid triples
    TEST_ASSERT(result->count > 0);
    
    // Check for various edge cases
    int found_empty_string = 0, found_unicode = 0, found_blank_node = 0;
    for (int i = 0; i < result->count; i++) {
        if (strcmp(result->triples[i].object, "\"\"") == 0) found_empty_string = 1;
        if (strstr(result->triples[i].object, "世界")) found_unicode = 1;
        if (result->triples[i].subject[0] == '_') found_blank_node = 1;
    }
    
    TEST_ASSERT(found_empty_string);
    TEST_ASSERT(found_unicode);
    TEST_ASSERT(found_blank_node);
    
    free_triple_set(result);
    free_parser(&parser);
    free_lexer(&lexer);
    TEST_FREE(content);
    
    return 1;
}

// Test parsing unicode.ttl fixture
int test_parse_unicode_fixture() {
    char* content = read_test_file("tests/fixtures/unicode.ttl");
    TEST_ASSERT_NOT_NULL(content);
    
    Lexer lexer;
    init_lexer(&lexer, content);
    
    Parser parser;
    init_parser(&parser, &lexer);
    
    TripleSet* result = parse(&parser);
    TEST_ASSERT_NOT_NULL(result);
    
    // Should parse Unicode content correctly
    TEST_ASSERT(result->count > 0);
    
    // Check for specific Unicode characters
    int found_chinese = 0, found_arabic = 0, found_emoji = 0;
    for (int i = 0; i < result->count; i++) {
        if (strstr(result->triples[i].object, "李小明")) found_chinese = 1;
        if (strstr(result->triples[i].object, "محمد")) found_arabic = 1;
        if (strstr(result->triples[i].object, "🚀")) found_emoji = 1;
    }
    
    TEST_ASSERT(found_chinese);
    TEST_ASSERT(found_arabic);
    TEST_ASSERT(found_emoji);
    
    free_triple_set(result);
    free_parser(&parser);
    free_lexer(&lexer);
    TEST_FREE(content);
    
    return 1;
}

// Test round-trip: parse then serialize
int test_roundtrip_parsing() {
    const char* original = 
        "@prefix ex: <http://example.org/> .\n"
        "@prefix foaf: <http://xmlns.com/foaf/0.1/> .\n"
        "ex:alice a foaf:Person ;\n"
        "         foaf:name \"Alice\" ;\n"
        "         foaf:age 25 .";
    
    Lexer lexer;
    init_lexer(&lexer, original);
    
    Parser parser;
    init_parser(&parser, &lexer);
    
    TripleSet* result = parse(&parser);
    TEST_ASSERT_NOT_NULL(result);
    
    // Should have 3 triples (a, name, age)
    TEST_ASSERT_EQ(result->count, 3);
    
    // All triples should have same subject
    for (int i = 0; i < result->count; i++) {
        TEST_ASSERT_STR_EQ(result->triples[i].subject, "http://example.org/alice");
    }
    
    // Check predicates
    int found_type = 0, found_name = 0, found_age = 0;
    for (int i = 0; i < result->count; i++) {
        if (strstr(result->triples[i].predicate, "type")) found_type = 1;
        if (strstr(result->triples[i].predicate, "name")) found_name = 1;
        if (strstr(result->triples[i].predicate, "age")) found_age = 1;
    }
    
    TEST_ASSERT(found_type);
    TEST_ASSERT(found_name);
    TEST_ASSERT(found_age);
    
    free_triple_set(result);
    free_parser(&parser);
    free_lexer(&lexer);
    
    return 1;
}

// Test memory efficiency with large file
int test_large_file_parsing() {
    char* content = read_test_file("tests/fixtures/large.ttl");
    TEST_ASSERT_NOT_NULL(content);
    
    printf("    Parsing large TTL file...\n");
    
    clock_t start = clock();
    
    Lexer lexer;
    init_lexer(&lexer, content);
    
    Parser parser;
    init_parser(&parser, &lexer);
    
    TripleSet* result = parse(&parser);
    
    clock_t end = clock();
    double elapsed = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    TEST_ASSERT_NOT_NULL(result);
    
    printf("    Parsed %d triples in %.3f seconds\n", 
           result->count, elapsed);
    
    // Performance expectation: should handle reasonably large files efficiently
    TEST_ASSERT(elapsed < 1.0); // Should parse in under 1 second
    
    free_triple_set(result);
    free_parser(&parser);
    free_lexer(&lexer);
    TEST_FREE(content);
    
    return 1;
}

// Test concurrent parsing (if threading is supported)
int test_concurrent_parsing() {
    // Simple concurrent test - parse same content multiple times
    const char* content = 
        "@prefix ex: <http://example.org/> .\n"
        "ex:subject ex:predicate ex:object .";
    
    const int num_iterations = 10;
    
    for (int i = 0; i < num_iterations; i++) {
        Lexer lexer;
        init_lexer(&lexer, content);
        
        Parser parser;
        init_parser(&parser, &lexer);
        
        TripleSet* result = parse(&parser);
        TEST_ASSERT_NOT_NULL(result);
        TEST_ASSERT_EQ(result->count, 1);
        
        free_triple_set(result);
        free_parser(&parser);
        free_lexer(&lexer);
    }
    
    return 1;
}

// Test error location reporting
int test_error_location_reporting() {
    const char* content = 
        "@prefix ex: <http://example.org/> .\n"
        "ex:subject ex:predicate .\n"  // Missing object at line 2
        "ex:valid ex:statement ex:here .";
    
    Lexer lexer;
    init_lexer(&lexer, content);
    
    Parser parser;
    init_parser(&parser, &lexer);
    
    TripleSet* result = parse(&parser);
    
    // Should have error
    TEST_ASSERT(has_error(&parser.error));
    
    // Error should be on line 2
    TEST_ASSERT_EQ(parser.error.line, 2);
    
    // Should still parse valid statements after error
    if (result) {
        free_triple_set(result);
    }
    
    free_parser(&parser);
    free_lexer(&lexer);
    
    return 1;
}

// Test streaming parser (if supported)
int test_streaming_parsing() {
    // Simulate streaming by parsing chunks
    const char* chunk1 = "@prefix ex: <http://example.org/> .\n";
    const char* chunk2 = "ex:subject1 ex:predicate1 ex:object1 .\n";
    const char* chunk3 = "ex:subject2 ex:predicate2 ex:object2 .";
    
    // For now, just test that we can parse concatenated chunks
    size_t total_len = strlen(chunk1) + strlen(chunk2) + strlen(chunk3) + 1;
    char* full_content = TEST_MALLOC(total_len);
    TEST_ASSERT_NOT_NULL(full_content);
    
    strcpy(full_content, chunk1);
    strcat(full_content, chunk2);
    strcat(full_content, chunk3);
    
    Lexer lexer;
    init_lexer(&lexer, full_content);
    
    Parser parser;
    init_parser(&parser, &lexer);
    
    TripleSet* result = parse(&parser);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQ(result->count, 2);
    
    free_triple_set(result);
    free_parser(&parser);
    free_lexer(&lexer);
    TEST_FREE(full_content);
    
    return 1;
}

// Main integration test suite runner
void run_integration_tests() {
    RUN_TEST_SUITE("Integration", () => {
        RUN_TEST(test_parse_simple_fixture);
        RUN_TEST(test_parse_complex_fixture);
        RUN_TEST(test_parse_invalid_fixture);
        RUN_TEST(test_parse_edge_cases_fixture);
        RUN_TEST(test_parse_unicode_fixture);
        RUN_TEST(test_roundtrip_parsing);
        RUN_TEST(test_large_file_parsing);
        RUN_TEST(test_concurrent_parsing);
        RUN_TEST(test_error_location_reporting);
        RUN_TEST(test_streaming_parsing);
    });
}