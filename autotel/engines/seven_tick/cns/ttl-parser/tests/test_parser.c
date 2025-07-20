#include "test_utils.h"
#include "../src/parser.h"
#include "../src/lexer.h"

// Test basic prefix parsing
int test_parse_prefix() {
    const char* input = "@prefix ex: <http://example.org/> .";
    
    Lexer lexer;
    init_lexer(&lexer, input);
    
    Parser parser;
    init_parser(&parser, &lexer);
    
    TripleSet* result = parse(&parser);
    TEST_ASSERT_NOT_NULL(result);
    
    // Should have one prefix registered
    TEST_ASSERT(parser.prefix_count == 1);
    TEST_ASSERT_STR_EQ(parser.prefixes[0].prefix, "ex");
    TEST_ASSERT_STR_EQ(parser.prefixes[0].namespace, "http://example.org/");
    
    free_triple_set(result);
    free_parser(&parser);
    free_lexer(&lexer);
    
    return 1;
}

// Test multiple prefixes
int test_parse_multiple_prefixes() {
    const char* input = 
        "@prefix ex: <http://example.org/> .\n"
        "@prefix foaf: <http://xmlns.com/foaf/0.1/> .\n"
        "@prefix rdf: <http://www.w3.org/1999/02/22-rdf-syntax-ns#> .";
    
    Lexer lexer;
    init_lexer(&lexer, input);
    
    Parser parser;
    init_parser(&parser, &lexer);
    
    TripleSet* result = parse(&parser);
    TEST_ASSERT_NOT_NULL(result);
    
    TEST_ASSERT(parser.prefix_count == 3);
    
    // Check each prefix
    TEST_ASSERT_STR_EQ(parser.prefixes[0].prefix, "ex");
    TEST_ASSERT_STR_EQ(parser.prefixes[0].namespace, "http://example.org/");
    
    TEST_ASSERT_STR_EQ(parser.prefixes[1].prefix, "foaf");
    TEST_ASSERT_STR_EQ(parser.prefixes[1].namespace, "http://xmlns.com/foaf/0.1/");
    
    TEST_ASSERT_STR_EQ(parser.prefixes[2].prefix, "rdf");
    TEST_ASSERT_STR_EQ(parser.prefixes[2].namespace, "http://www.w3.org/1999/02/22-rdf-syntax-ns#");
    
    free_triple_set(result);
    free_parser(&parser);
    free_lexer(&lexer);
    
    return 1;
}

// Test simple triple parsing
int test_parse_simple_triple() {
    const char* input = 
        "@prefix ex: <http://example.org/> .\n"
        "ex:subject ex:predicate ex:object .";
    
    Lexer lexer;
    init_lexer(&lexer, input);
    
    Parser parser;
    init_parser(&parser, &lexer);
    
    TripleSet* result = parse(&parser);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT(result->count == 1);
    
    Triple* triple = &result->triples[0];
    TEST_ASSERT_STR_EQ(triple->subject, "http://example.org/subject");
    TEST_ASSERT_STR_EQ(triple->predicate, "http://example.org/predicate");
    TEST_ASSERT_STR_EQ(triple->object, "http://example.org/object");
    
    free_triple_set(result);
    free_parser(&parser);
    free_lexer(&lexer);
    
    return 1;
}

// Test multiple triples
int test_parse_multiple_triples() {
    const char* input = 
        "@prefix ex: <http://example.org/> .\n"
        "ex:subject1 ex:predicate1 ex:object1 .\n"
        "ex:subject2 ex:predicate2 ex:object2 .";
    
    Lexer lexer;
    init_lexer(&lexer, input);
    
    Parser parser;
    init_parser(&parser, &lexer);
    
    TripleSet* result = parse(&parser);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT(result->count == 2);
    
    // Check first triple
    TEST_ASSERT_STR_EQ(result->triples[0].subject, "http://example.org/subject1");
    TEST_ASSERT_STR_EQ(result->triples[0].predicate, "http://example.org/predicate1");
    TEST_ASSERT_STR_EQ(result->triples[0].object, "http://example.org/object1");
    
    // Check second triple
    TEST_ASSERT_STR_EQ(result->triples[1].subject, "http://example.org/subject2");
    TEST_ASSERT_STR_EQ(result->triples[1].predicate, "http://example.org/predicate2");
    TEST_ASSERT_STR_EQ(result->triples[1].object, "http://example.org/object2");
    
    free_triple_set(result);
    free_parser(&parser);
    free_lexer(&lexer);
    
    return 1;
}

// Test absolute URIs
int test_parse_absolute_uris() {
    const char* input = 
        "<http://example.org/subject> <http://example.org/predicate> <http://example.org/object> .";
    
    Lexer lexer;
    init_lexer(&lexer, input);
    
    Parser parser;
    init_parser(&parser, &lexer);
    
    TripleSet* result = parse(&parser);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT(result->count == 1);
    
    Triple* triple = &result->triples[0];
    TEST_ASSERT_STR_EQ(triple->subject, "http://example.org/subject");
    TEST_ASSERT_STR_EQ(triple->predicate, "http://example.org/predicate");
    TEST_ASSERT_STR_EQ(triple->object, "http://example.org/object");
    
    free_triple_set(result);
    free_parser(&parser);
    free_lexer(&lexer);
    
    return 1;
}

// Test string literals
int test_parse_string_literals() {
    const char* input = 
        "@prefix ex: <http://example.org/> .\n"
        "ex:subject ex:name \"John Doe\" .";
    
    Lexer lexer;
    init_lexer(&lexer, input);
    
    Parser parser;
    init_parser(&parser, &lexer);
    
    TripleSet* result = parse(&parser);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT(result->count == 1);
    
    Triple* triple = &result->triples[0];
    TEST_ASSERT_STR_EQ(triple->subject, "http://example.org/subject");
    TEST_ASSERT_STR_EQ(triple->predicate, "http://example.org/name");
    TEST_ASSERT_STR_EQ(triple->object, "\"John Doe\"");
    
    free_triple_set(result);
    free_parser(&parser);
    free_lexer(&lexer);
    
    return 1;
}

// Test numbers
int test_parse_numbers() {
    const char* input = 
        "@prefix ex: <http://example.org/> .\n"
        "ex:subject ex:age 25 .\n"
        "ex:subject ex:height 5.9 .";
    
    Lexer lexer;
    init_lexer(&lexer, input);
    
    Parser parser;
    init_parser(&parser, &lexer);
    
    TripleSet* result = parse(&parser);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT(result->count == 2);
    
    // Check integer
    TEST_ASSERT_STR_EQ(result->triples[0].object, "25");
    
    // Check float
    TEST_ASSERT_STR_EQ(result->triples[1].object, "5.9");
    
    free_triple_set(result);
    free_parser(&parser);
    free_lexer(&lexer);
    
    return 1;
}

// Test blank nodes
int test_parse_blank_nodes() {
    const char* input = 
        "@prefix ex: <http://example.org/> .\n"
        "_:blank1 ex:predicate ex:object .\n"
        "ex:subject ex:predicate _:blank2 .";
    
    Lexer lexer;
    init_lexer(&lexer, input);
    
    Parser parser;
    init_parser(&parser, &lexer);
    
    TripleSet* result = parse(&parser);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT(result->count == 2);
    
    // Check blank node as subject
    TEST_ASSERT_STR_EQ(result->triples[0].subject, "_:blank1");
    
    // Check blank node as object
    TEST_ASSERT_STR_EQ(result->triples[1].object, "_:blank2");
    
    free_triple_set(result);
    free_parser(&parser);
    free_lexer(&lexer);
    
    return 1;
}

// Test comments
int test_parse_with_comments() {
    const char* input = 
        "# This is a comment\n"
        "@prefix ex: <http://example.org/> . # Another comment\n"
        "# Full line comment\n"
        "ex:subject ex:predicate ex:object . # End comment";
    
    Lexer lexer;
    init_lexer(&lexer, input);
    
    Parser parser;
    init_parser(&parser, &lexer);
    
    TripleSet* result = parse(&parser);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT(result->count == 1);
    TEST_ASSERT(parser.prefix_count == 1);
    
    free_triple_set(result);
    free_parser(&parser);
    free_lexer(&lexer);
    
    return 1;
}

// Test syntax error recovery
int test_parse_syntax_error() {
    const char* input = 
        "@prefix ex: <http://example.org/> .\n"
        "ex:subject ex:predicate ; # Missing object\n"
        "ex:subject2 ex:predicate2 ex:object2 .";
    
    Lexer lexer;
    init_lexer(&lexer, input);
    
    Parser parser;
    init_parser(&parser, &lexer);
    
    TripleSet* result = parse(&parser);
    
    // Should have error but continue parsing
    TEST_ASSERT(has_error(&parser.error));
    
    // Should still parse valid triples after error
    if (result) {\n        free_triple_set(result);
    }
    free_parser(&parser);
    free_lexer(&lexer);
    
    return 1;
}

// Test empty input
int test_parse_empty() {
    const char* input = "";
    
    Lexer lexer;
    init_lexer(&lexer, input);
    
    Parser parser;
    init_parser(&parser, &lexer);
    
    TripleSet* result = parse(&parser);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT(result->count == 0);
    
    free_triple_set(result);
    free_parser(&parser);
    free_lexer(&lexer);
    
    return 1;
}

// Test only whitespace and comments
int test_parse_whitespace_only() {
    const char* input = 
        "   \n"
        "# Just a comment\n"
        "   \t  \n"
        "# Another comment\n";
    
    Lexer lexer;
    init_lexer(&lexer, input);
    
    Parser parser;
    init_parser(&parser, &lexer);
    
    TripleSet* result = parse(&parser);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT(result->count == 0);
    
    free_triple_set(result);
    free_parser(&parser);
    free_lexer(&lexer);
    
    return 1;
}

// Test prefix resolution
int test_prefix_resolution() {
    const char* input = 
        "@prefix ex: <http://example.org/> .\n"
        "@prefix foaf: <http://xmlns.com/foaf/0.1/> .\n"
        "ex:person foaf:name \"Alice\" .";
    
    Lexer lexer;
    init_lexer(&lexer, input);
    
    Parser parser;
    init_parser(&parser, &lexer);
    
    TripleSet* result = parse(&parser);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT(result->count == 1);
    
    Triple* triple = &result->triples[0];
    TEST_ASSERT_STR_EQ(triple->subject, "http://example.org/person");
    TEST_ASSERT_STR_EQ(triple->predicate, "http://xmlns.com/foaf/0.1/name");
    
    free_triple_set(result);
    free_parser(&parser);
    free_lexer(&lexer);
    
    return 1;
}

// Test base URI
int test_parse_base() {
    const char* input = 
        "@base <http://example.org/> .\n"
        "<subject> <predicate> <object> .";
    
    Lexer lexer;
    init_lexer(&lexer, input);
    
    Parser parser;
    init_parser(&parser, &lexer);
    
    TripleSet* result = parse(&parser);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT(result->count == 1);
    
    // Base URI should be set
    TEST_ASSERT_STR_EQ(parser.base_uri, "http://example.org/");
    
    // Relative URIs should be resolved against base
    Triple* triple = &result->triples[0];
    TEST_ASSERT_STR_EQ(triple->subject, "http://example.org/subject");
    TEST_ASSERT_STR_EQ(triple->predicate, "http://example.org/predicate");
    TEST_ASSERT_STR_EQ(triple->object, "http://example.org/object");
    
    free_triple_set(result);
    free_parser(&parser);
    free_lexer(&lexer);
    
    return 1;
}

// Test semicolon syntax (property lists)
int test_parse_semicolon_syntax() {
    const char* input = 
        "@prefix ex: <http://example.org/> .\n"
        "ex:person ex:name \"Alice\" ;\n"
        "         ex:age 30 ;\n"
        "         ex:city \"NYC\" .";
    
    Lexer lexer;
    init_lexer(&lexer, input);
    
    Parser parser;
    init_parser(&parser, &lexer);
    
    TripleSet* result = parse(&parser);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT(result->count == 3);
    
    // All should have same subject
    for (int i = 0; i < 3; i++) {
        TEST_ASSERT_STR_EQ(result->triples[i].subject, "http://example.org/person");
    }
    
    free_triple_set(result);
    free_parser(&parser);
    free_lexer(&lexer);
    
    return 1;
}

// Test comma syntax (object lists)
int test_parse_comma_syntax() {
    const char* input = 
        "@prefix ex: <http://example.org/> .\n"
        "ex:person ex:friend ex:alice, ex:bob, ex:charlie .";
    
    Lexer lexer;
    init_lexer(&lexer, input);
    
    Parser parser;
    init_parser(&parser, &lexer);
    
    TripleSet* result = parse(&parser);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT(result->count == 3);
    
    // All should have same subject and predicate
    for (int i = 0; i < 3; i++) {
        TEST_ASSERT_STR_EQ(result->triples[i].subject, "http://example.org/person");
        TEST_ASSERT_STR_EQ(result->triples[i].predicate, "http://example.org/friend");
    }
    
    // Check objects
    TEST_ASSERT_STR_EQ(result->triples[0].object, "http://example.org/alice");
    TEST_ASSERT_STR_EQ(result->triples[1].object, "http://example.org/bob");
    TEST_ASSERT_STR_EQ(result->triples[2].object, "http://example.org/charlie");
    
    free_triple_set(result);
    free_parser(&parser);
    free_lexer(&lexer);
    
    return 1;
}

// Test 'a' as rdf:type shorthand
int test_parse_rdf_type_shorthand() {
    const char* input = 
        "@prefix ex: <http://example.org/> .\n"
        "ex:alice a ex:Person .";
    
    Lexer lexer;
    init_lexer(&lexer, input);
    
    Parser parser;
    init_parser(&parser, &lexer);
    
    TripleSet* result = parse(&parser);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT(result->count == 1);
    
    Triple* triple = &result->triples[0];
    TEST_ASSERT_STR_EQ(triple->subject, "http://example.org/alice");
    TEST_ASSERT_STR_EQ(triple->predicate, "http://www.w3.org/1999/02/22-rdf-syntax-ns#type");
    TEST_ASSERT_STR_EQ(triple->object, "http://example.org/Person");
    
    free_triple_set(result);
    free_parser(&parser);
    free_lexer(&lexer);
    
    return 1;
}

// Performance test - large number of triples
int test_parse_large_file() {
    printf("    Generating large TTL content (10000 triples)...\n");
    
    // Generate a large TTL string
    const int num_triples = 10000;
    size_t content_size = num_triples * 100; // Estimate
    char* content = TEST_MALLOC(content_size);
    TEST_ASSERT_NOT_NULL(content);
    
    strcpy(content, "@prefix ex: <http://example.org/> .\n");
    char* pos = content + strlen(content);
    
    for (int i = 0; i < num_triples; i++) {
        sprintf(pos, "ex:subject%d ex:predicate%d ex:object%d .\n", i, i, i);
        pos += strlen(pos);
    }
    
    printf("    Parsing large content (%.1f KB)...\n", strlen(content) / 1024.0);
    
    clock_t start = clock();
    
    Lexer lexer;
    init_lexer(&lexer, content);
    
    Parser parser;
    init_parser(&parser, &lexer);
    
    TripleSet* result = parse(&parser);
    
    clock_t end = clock();
    double elapsed = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT(result->count == num_triples);
    
    printf("    Parsed %d triples in %.3f seconds (%.0f triples/sec)\n", 
           num_triples, elapsed, num_triples / elapsed);
    
    free_triple_set(result);
    free_parser(&parser);
    free_lexer(&lexer);
    TEST_FREE(content);
    
    return 1;
}

// Test memory usage - ensure no leaks
int test_parse_memory_usage() {
    const char* input = 
        "@prefix ex: <http://example.org/> .\n"
        "ex:subject ex:predicate ex:object .";
    
    // Parse the same content multiple times to check for memory leaks
    for (int i = 0; i < 100; i++) {
        Lexer lexer;
        init_lexer(&lexer, input);
        
        Parser parser;
        init_parser(&parser, &lexer);
        
        TripleSet* result = parse(&parser);
        TEST_ASSERT_NOT_NULL(result);
        
        free_triple_set(result);
        free_parser(&parser);
        free_lexer(&lexer);
    }
    
    return 1;
}

// Main test suite runner
void run_parser_tests() {
    RUN_TEST_SUITE("Parser", () => {
        RUN_TEST(test_parse_prefix);
        RUN_TEST(test_parse_multiple_prefixes);
        RUN_TEST(test_parse_simple_triple);
        RUN_TEST(test_parse_multiple_triples);
        RUN_TEST(test_parse_absolute_uris);
        RUN_TEST(test_parse_string_literals);
        RUN_TEST(test_parse_numbers);
        RUN_TEST(test_parse_blank_nodes);
        RUN_TEST(test_parse_with_comments);
        RUN_TEST(test_parse_syntax_error);
        RUN_TEST(test_parse_empty);
        RUN_TEST(test_parse_whitespace_only);
        RUN_TEST(test_prefix_resolution);
        RUN_TEST(test_parse_base);
        RUN_TEST(test_parse_semicolon_syntax);
        RUN_TEST(test_parse_comma_syntax);
        RUN_TEST(test_parse_rdf_type_shorthand);
        RUN_TEST(test_parse_large_file);
        RUN_TEST(test_parse_memory_usage);
    });
}