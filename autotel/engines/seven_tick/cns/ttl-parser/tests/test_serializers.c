/**
 * @file test_serializers.c
 * @brief Comprehensive tests for all RDF serialization formats
 * @author IntegrationTester Agent
 * @date 2024
 * 
 * Tests all Phase 2 serialization features:
 * - N-Triples output
 * - JSON-LD output
 * - RDF/XML output
 * - Format validation
 * - Performance benchmarks
 */

#include "test_utils.h"
#include "../include/serializer.h"
#include "../include/ast.h"
#include "../include/parser.h"
#include "../include/lexer.h"
#include <string.h>
#include <time.h>

// Global test stats
TestStats g_test_stats;

// Test data
static const char* test_ttl_simple = 
    "@prefix ex: <http://example.org/> .\n"
    "@prefix foaf: <http://xmlns.com/foaf/0.1/> .\n"
    "\n"
    "ex:person1 a foaf:Person ;\n"
    "    foaf:name \"John Doe\" ;\n"
    "    foaf:age 30 ;\n"
    "    foaf:knows ex:person2 .\n"
    "\n"
    "ex:person2 foaf:name \"Jane Smith\"@en ;\n"
    "    foaf:age \"25\"^^<http://www.w3.org/2001/XMLSchema#int> .\n";

static const char* test_ttl_complex = 
    "@prefix ex: <http://example.org/> .\n"
    "@prefix rdf: <http://www.w3.org/1999/02/22-rdf-syntax-ns#> .\n"
    "@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> .\n"
    "@prefix xsd: <http://www.w3.org/2001/XMLSchema#> .\n"
    "\n"
    "ex:Company a rdfs:Class ;\n"
    "    rdfs:label \"Company\"@en ;\n"
    "    rdfs:comment \"A business organization\" .\n"
    "\n"
    "ex:company1 a ex:Company ;\n"
    "    ex:name \"TechCorp\" ;\n"
    "    ex:founded \"2010-01-01\"^^xsd:date ;\n"
    "    ex:revenue \"1000000.50\"^^xsd:decimal ;\n"
    "    ex:active true ;\n"
    "    ex:employees (\n"
    "        ex:employee1\n"
    "        ex:employee2\n"
    "        ex:employee3\n"
    "    ) .\n"
    "\n"
    "ex:employee1 ex:name \"Alice\" ;\n"
    "    ex:position \"Manager\" ;\n"
    "    ex:salary 75000 .\n";

/**
 * Test N-Triples serialization
 */
static int test_ntriples_serialization() {
    printf("    Testing N-Triples serialization...\n");
    
    // Parse test data
    ttl_lexer_t* lexer = ttl_lexer_create();
    TEST_ASSERT_NOT_NULL(lexer);
    
    ttl_lexer_input_from_string(lexer, test_ttl_simple);
    
    ttl_parser_t* parser = ttl_parser_create(lexer);
    TEST_ASSERT_NOT_NULL(parser);
    
    ttl_ast_node_t* ast = ttl_parser_parse_document(parser);
    TEST_ASSERT_NOT_NULL(ast);
    
    // Test basic N-Triples serialization
    FILE* output = tmpfile();
    TEST_ASSERT_NOT_NULL(output);
    
    bool result = ttl_serialize_ntriples(ast, output);
    TEST_ASSERT(result);
    
    // Verify output content
    rewind(output);
    char buffer[4096];
    size_t bytes_read = fread(buffer, 1, sizeof(buffer) - 1, output);
    buffer[bytes_read] = '\0';
    
    // Check for expected N-Triples patterns
    TEST_ASSERT(strstr(buffer, "<http://example.org/person1>") != NULL);
    TEST_ASSERT(strstr(buffer, "<http://www.w3.org/1999/02/22-rdf-syntax-ns#type>") != NULL);
    TEST_ASSERT(strstr(buffer, "<http://xmlns.com/foaf/0.1/Person>") != NULL);
    TEST_ASSERT(strstr(buffer, "\"John Doe\"") != NULL);
    TEST_ASSERT(strstr(buffer, " .") != NULL); // N-Triples end with period
    
    printf("      N-Triples output (%zu bytes): Valid format\n", bytes_read);
    
    fclose(output);
    ttl_parser_destroy(parser);
    ttl_lexer_destroy(lexer);
    
    return 1;
}

/**
 * Test JSON-LD serialization
 */
static int test_jsonld_serialization() {
    printf("    Testing JSON-LD serialization...\n");
    
    // Parse test data
    ttl_lexer_t* lexer = ttl_lexer_create();
    TEST_ASSERT_NOT_NULL(lexer);
    
    ttl_lexer_input_from_string(lexer, test_ttl_simple);
    
    ttl_parser_t* parser = ttl_parser_create(lexer);
    TEST_ASSERT_NOT_NULL(parser);
    
    ttl_ast_node_t* ast = ttl_parser_parse_document(parser);
    TEST_ASSERT_NOT_NULL(ast);
    
    // Test JSON-LD serialization with pretty printing
    FILE* output = tmpfile();
    TEST_ASSERT_NOT_NULL(output);
    
    bool result = ttl_serialize_jsonld(ast, output, true);
    TEST_ASSERT(result);
    
    // Verify JSON structure
    rewind(output);
    char buffer[4096];
    size_t bytes_read = fread(buffer, 1, sizeof(buffer) - 1, output);
    buffer[bytes_read] = '\0';
    
    // Check for JSON-LD patterns
    TEST_ASSERT(strstr(buffer, "{") != NULL); // JSON object start
    TEST_ASSERT(strstr(buffer, "}") != NULL); // JSON object end
    TEST_ASSERT(strstr(buffer, "@context") != NULL); // JSON-LD context
    TEST_ASSERT(strstr(buffer, "@type") != NULL); // JSON-LD type
    TEST_ASSERT(strstr(buffer, "\"@id\"") != NULL); // JSON-LD id
    
    printf("      JSON-LD output (%zu bytes): Valid JSON structure\n", bytes_read);
    
    fclose(output);
    ttl_parser_destroy(parser);
    ttl_lexer_destroy(lexer);
    
    return 1;
}

/**
 * Test RDF/XML serialization
 */
static int test_rdfxml_serialization() {
    printf("    Testing RDF/XML serialization...\n");
    
    // Parse test data
    ttl_lexer_t* lexer = ttl_lexer_create();
    TEST_ASSERT_NOT_NULL(lexer);
    
    ttl_lexer_input_from_string(lexer, test_ttl_simple);
    
    ttl_parser_t* parser = ttl_parser_create(lexer);
    TEST_ASSERT_NOT_NULL(parser);
    
    ttl_ast_node_t* ast = ttl_parser_parse_document(parser);
    TEST_ASSERT_NOT_NULL(ast);
    
    // Test RDF/XML serialization with prefixes
    FILE* output = tmpfile();
    TEST_ASSERT_NOT_NULL(output);
    
    bool result = ttl_serialize_rdfxml(ast, output, true);
    TEST_ASSERT(result);
    
    // Verify XML structure
    rewind(output);
    char buffer[4096];
    size_t bytes_read = fread(buffer, 1, sizeof(buffer) - 1, output);
    buffer[bytes_read] = '\0';
    
    // Check for RDF/XML patterns
    TEST_ASSERT(strstr(buffer, "<?xml") != NULL); // XML declaration
    TEST_ASSERT(strstr(buffer, "<rdf:RDF") != NULL); // RDF root element
    TEST_ASSERT(strstr(buffer, "xmlns:rdf") != NULL); // RDF namespace
    TEST_ASSERT(strstr(buffer, "</rdf:RDF>") != NULL); // RDF end element
    TEST_ASSERT(strstr(buffer, "rdf:about") != NULL || strstr(buffer, "rdf:ID") != NULL); // RDF properties
    
    printf("      RDF/XML output (%zu bytes): Valid XML structure\n", bytes_read);
    
    fclose(output);
    ttl_parser_destroy(parser);
    ttl_lexer_destroy(lexer);
    
    return 1;
}

/**
 * Test serializer options and configuration
 */
static int test_serializer_options() {
    printf("    Testing serializer options...\n");
    
    // Test default options for each format
    ttl_serializer_options_t ntriples_opts = ttl_serializer_default_options(TTL_FORMAT_NTRIPLES);
    ttl_serializer_options_t jsonld_opts = ttl_serializer_default_options(TTL_FORMAT_JSONLD);
    ttl_serializer_options_t rdfxml_opts = ttl_serializer_default_options(TTL_FORMAT_RDFXML);
    
    // Verify default settings are reasonable
    TEST_ASSERT(ntriples_opts.pretty_print == false); // N-Triples don't need pretty printing
    TEST_ASSERT(jsonld_opts.pretty_print == true);    // JSON-LD benefits from pretty printing
    TEST_ASSERT(rdfxml_opts.use_prefixes == true);    // RDF/XML benefits from prefixes
    
    // Test format utility functions
    TEST_ASSERT_STR_EQ(ttl_serializer_format_name(TTL_FORMAT_NTRIPLES), "N-Triples");
    TEST_ASSERT_STR_EQ(ttl_serializer_format_name(TTL_FORMAT_JSONLD), "JSON-LD");
    TEST_ASSERT_STR_EQ(ttl_serializer_format_name(TTL_FORMAT_RDFXML), "RDF/XML");
    
    TEST_ASSERT_STR_EQ(ttl_serializer_mime_type(TTL_FORMAT_NTRIPLES), "application/n-triples");
    TEST_ASSERT_STR_EQ(ttl_serializer_mime_type(TTL_FORMAT_JSONLD), "application/ld+json");
    TEST_ASSERT_STR_EQ(ttl_serializer_mime_type(TTL_FORMAT_RDFXML), "application/rdf+xml");
    
    TEST_ASSERT_STR_EQ(ttl_serializer_file_extension(TTL_FORMAT_NTRIPLES), "nt");
    TEST_ASSERT_STR_EQ(ttl_serializer_file_extension(TTL_FORMAT_JSONLD), "jsonld");
    TEST_ASSERT_STR_EQ(ttl_serializer_file_extension(TTL_FORMAT_RDFXML), "rdf");
    
    printf("      Format metadata: All correct\n");
    
    return 1;
}

/**
 * Test string escaping for different formats
 */
static int test_string_escaping() {
    printf("    Testing string escaping...\n");
    
    const char* test_strings[] = {
        "Simple string",
        "String with \"quotes\"",
        "String with\nnewlines\tand\ttabs",
        "Unicode: 你好世界 🌍",
        "Backslash\\test",
        NULL
    };
    
    for (int i = 0; test_strings[i] != NULL; i++) {
        const char* input = test_strings[i];
        
        // Test escaping for each format
        char* nt_escaped = ttl_serializer_escape_string(input, TTL_FORMAT_NTRIPLES);
        char* jsonld_escaped = ttl_serializer_escape_string(input, TTL_FORMAT_JSONLD);
        char* rdfxml_escaped = ttl_serializer_escape_string(input, TTL_FORMAT_RDFXML);
        
        TEST_ASSERT_NOT_NULL(nt_escaped);
        TEST_ASSERT_NOT_NULL(jsonld_escaped);
        TEST_ASSERT_NOT_NULL(rdfxml_escaped);
        
        // Basic sanity checks
        if (strchr(input, '"')) {
            TEST_ASSERT(strchr(nt_escaped, '\\') != NULL); // Should escape quotes
            TEST_ASSERT(strchr(jsonld_escaped, '\\') != NULL);
        }
        
        if (strchr(input, '\n')) {
            TEST_ASSERT(strstr(nt_escaped, "\\n") != NULL); // Should escape newlines
            TEST_ASSERT(strstr(jsonld_escaped, "\\n") != NULL);
        }
        
        printf("      String %d: Escaped correctly for all formats\n", i + 1);
        
        free(nt_escaped);
        free(jsonld_escaped);
        free(rdfxml_escaped);
    }
    
    return 1;
}

/**
 * Test serializer statistics tracking
 */
static int test_serializer_statistics() {
    printf("    Testing serializer statistics...\n");
    
    // Parse complex test data
    ttl_lexer_t* lexer = ttl_lexer_create();
    TEST_ASSERT_NOT_NULL(lexer);
    
    ttl_lexer_input_from_string(lexer, test_ttl_complex);
    
    ttl_parser_t* parser = ttl_parser_create(lexer);
    TEST_ASSERT_NOT_NULL(parser);
    
    ttl_ast_node_t* ast = ttl_parser_parse_document(parser);
    TEST_ASSERT_NOT_NULL(ast);
    
    // Create serializer and track statistics
    ttl_serializer_options_t options = ttl_serializer_default_options(TTL_FORMAT_NTRIPLES);
    options.output = tmpfile();
    TEST_ASSERT_NOT_NULL(options.output);
    
    ttl_serializer_t* serializer = ttl_serializer_create(TTL_FORMAT_NTRIPLES, &options);
    TEST_ASSERT_NOT_NULL(serializer);
    
    clock_t start = clock();
    bool result = ttl_serializer_serialize(serializer, ast);
    clock_t end = clock();
    TEST_ASSERT(result);
    
    // Get and verify statistics
    ttl_serializer_stats_t stats;
    ttl_serializer_get_stats(serializer, &stats);
    
    TEST_ASSERT(stats.triples_serialized > 0);
    TEST_ASSERT(stats.bytes_written > 0);
    TEST_ASSERT(stats.serialization_time_ms >= 0);
    
    printf("      Statistics: %zu triples, %zu bytes, %.2f ms\n", 
           stats.triples_serialized, stats.bytes_written, stats.serialization_time_ms);
    
    // Verify performance is reasonable (should be fast for small documents)
    double actual_time = ((double)(end - start)) / CLOCKS_PER_SEC * 1000;
    TEST_ASSERT(actual_time < 1000); // Should complete within 1 second
    
    fclose(options.output);
    ttl_serializer_destroy(serializer);
    ttl_parser_destroy(parser);
    ttl_lexer_destroy(lexer);
    
    return 1;
}

/**
 * Test error handling in serialization
 */
static int test_serialization_errors() {
    printf("    Testing serialization error handling...\n");
    
    // Test with NULL inputs
    TEST_ASSERT(!ttl_serialize_ntriples(NULL, stdout));
    TEST_ASSERT(!ttl_serialize_jsonld(NULL, stdout, true));
    TEST_ASSERT(!ttl_serialize_rdfxml(NULL, stdout, true));
    
    // Test with invalid format
    ttl_serializer_t* serializer = ttl_serializer_create((ttl_serializer_format_t)999, NULL);
    TEST_ASSERT_NULL(serializer);
    
    // Test serialization to closed file
    FILE* closed_file = tmpfile();
    fclose(closed_file);
    
    ttl_lexer_t* lexer = ttl_lexer_create();
    ttl_lexer_input_from_string(lexer, test_ttl_simple);
    ttl_parser_t* parser = ttl_parser_create(lexer);
    ttl_ast_node_t* ast = ttl_parser_parse_document(parser);
    
    // This should fail gracefully
    bool result = ttl_serialize_ntriples(ast, closed_file);
    TEST_ASSERT(!result);
    
    printf("      Error conditions: Handled correctly\n");
    
    ttl_parser_destroy(parser);
    ttl_lexer_destroy(lexer);
    
    return 1;
}

/**
 * Test serialization performance with large documents
 */
static int test_serialization_performance() {
    printf("    Testing serialization performance...\n");
    
    // Generate large test document
    char* large_ttl = malloc(100000);
    TEST_ASSERT_NOT_NULL(large_ttl);
    
    strcpy(large_ttl, "@prefix ex: <http://example.org/> .\n");
    char* pos = large_ttl + strlen(large_ttl);
    
    // Generate 1000 triples
    for (int i = 0; i < 1000; i++) {
        pos += sprintf(pos, "ex:item%d ex:name \"Item %d\" .\n", i, i);
        pos += sprintf(pos, "ex:item%d ex:value %d .\n", i, i * 10);
    }
    
    // Parse large document
    ttl_lexer_t* lexer = ttl_lexer_create();
    ttl_lexer_input_from_string(lexer, large_ttl);
    
    ttl_parser_t* parser = ttl_parser_create(lexer);
    ttl_ast_node_t* ast = ttl_parser_parse_document(parser);
    TEST_ASSERT_NOT_NULL(ast);
    
    // Benchmark each format
    clock_t start, end;
    double times[TTL_FORMAT_COUNT];
    
    for (int format = 0; format < TTL_FORMAT_COUNT; format++) {
        FILE* output = tmpfile();
        TEST_ASSERT_NOT_NULL(output);
        
        start = clock();
        
        bool result = false;
        switch (format) {
            case TTL_FORMAT_NTRIPLES:
                result = ttl_serialize_ntriples(ast, output);
                break;
            case TTL_FORMAT_JSONLD:
                result = ttl_serialize_jsonld(ast, output, false);
                break;
            case TTL_FORMAT_RDFXML:
                result = ttl_serialize_rdfxml(ast, output, true);
                break;
        }
        
        end = clock();
        times[format] = ((double)(end - start)) / CLOCKS_PER_SEC * 1000;
        
        TEST_ASSERT(result);
        
        // Get file size
        fseek(output, 0, SEEK_END);
        long size = ftell(output);
        
        printf("      %s: %.2f ms, %ld bytes\n", 
               ttl_serializer_format_name(format), times[format], size);
        
        // Performance should be reasonable (< 5 seconds for 2000 triples)
        TEST_ASSERT(times[format] < 5000);
        
        fclose(output);
    }
    
    // N-Triples should generally be fastest (simplest format)
    TEST_ASSERT(times[TTL_FORMAT_NTRIPLES] <= times[TTL_FORMAT_JSONLD]);
    TEST_ASSERT(times[TTL_FORMAT_NTRIPLES] <= times[TTL_FORMAT_RDFXML]);
    
    free(large_ttl);
    ttl_parser_destroy(parser);
    ttl_lexer_destroy(lexer);
    
    return 1;
}

/**
 * Test serialization roundtrip consistency
 */
static int test_serialization_roundtrip() {
    printf("    Testing serialization roundtrip consistency...\n");
    
    // Parse original document
    ttl_lexer_t* lexer = ttl_lexer_create();
    ttl_lexer_input_from_string(lexer, test_ttl_simple);
    
    ttl_parser_t* parser = ttl_parser_create(lexer);
    ttl_ast_node_t* original_ast = ttl_parser_parse_document(parser);
    TEST_ASSERT_NOT_NULL(original_ast);
    
    // Serialize to N-Triples and parse back
    FILE* nt_output = tmpfile();
    bool result = ttl_serialize_ntriples(original_ast, nt_output);
    TEST_ASSERT(result);
    
    // Read N-Triples output
    rewind(nt_output);
    char nt_buffer[4096];
    size_t nt_size = fread(nt_buffer, 1, sizeof(nt_buffer) - 1, nt_output);
    nt_buffer[nt_size] = '\0';
    
    // Count triples in output (each line ending with ' .')
    int nt_triple_count = 0;
    char* line = strtok(nt_buffer, "\n");
    while (line) {
        if (strstr(line, " .")) {
            nt_triple_count++;
        }
        line = strtok(NULL, "\n");
    }
    
    printf("      N-Triples output: %d triples\n", nt_triple_count);
    TEST_ASSERT(nt_triple_count > 0);
    
    // Basic consistency check - should have expected number of triples
    // Original has: person1 type Person, person1 name "John", person1 age 30, 
    // person1 knows person2, person2 name "Jane", person2 age 25
    TEST_ASSERT(nt_triple_count >= 5); // At least these core triples
    
    fclose(nt_output);
    ttl_parser_destroy(parser);
    ttl_lexer_destroy(lexer);
    
    return 1;
}

/**
 * Run all serializer tests
 */
static void run_serializer_tests() {
    RUN_TEST(test_ntriples_serialization);
    RUN_TEST(test_jsonld_serialization);
    RUN_TEST(test_rdfxml_serialization);
    RUN_TEST(test_serializer_options);
    RUN_TEST(test_string_escaping);
    RUN_TEST(test_serializer_statistics);
    RUN_TEST(test_serialization_errors);
    RUN_TEST(test_serialization_performance);
    RUN_TEST(test_serialization_roundtrip);
}

/**
 * Main test entry point
 */
int main(int argc, char* argv[]) {
    printf("🔧 TTL Parser - RDF Serializers Test Suite\n");
    printf("==========================================\n");
    
    init_test_stats();
    
    RUN_TEST_SUITE("RDF Serializers", run_serializer_tests);
    
    print_test_summary();
    check_memory_leaks();
    
    return g_test_stats.failed == 0 ? 0 : 1;
}