/**
 * @file test_performance.c
 * @brief Performance benchmarks for all Phase 2 features
 * @author IntegrationTester Agent
 * @date 2024
 * 
 * Comprehensive performance testing:
 * - Parsing benchmarks
 * - Serialization benchmarks  
 * - Query execution benchmarks
 * - Memory usage analysis
 * - Throughput measurements
 */

#include "test_utils.h"
#include "../include/parser.h"
#include "../include/lexer.h"
#include "../include/serializer.h"
#include "../include/query.h"
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>

// Global test stats
TestStats g_test_stats;

// Performance thresholds (in milliseconds)
#define SMALL_PARSE_THRESHOLD_MS     10.0
#define MEDIUM_PARSE_THRESHOLD_MS    100.0
#define LARGE_PARSE_THRESHOLD_MS     1000.0
#define SERIALIZATION_THRESHOLD_MS   50.0
#define QUERY_THRESHOLD_MS          20.0

// Test data sizes
#define SMALL_DATASET_TRIPLES       100
#define MEDIUM_DATASET_TRIPLES      1000
#define LARGE_DATASET_TRIPLES       10000

/**
 * High-precision timer
 */
static double get_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000.0) + (tv.tv_usec / 1000.0);
}

/**
 * Get memory usage in KB
 */
static long get_memory_usage_kb() {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    return usage.ru_maxrss; // On Linux, this is in KB; on macOS, it's in bytes
}

/**
 * Generate test dataset of specified size
 */
static char* generate_test_dataset(int triple_count, const char* base_prefix) {
    size_t estimated_size = triple_count * 200; // Rough estimate
    char* dataset = malloc(estimated_size);
    if (!dataset) return NULL;
    
    sprintf(dataset, 
        "@prefix ex: <http://example.org/%s/> .\n"
        "@prefix foaf: <http://xmlns.com/foaf/0.1/> .\n"
        "@prefix rdf: <http://www.w3.org/1999/02/22-rdf-syntax-ns#> .\n"
        "@prefix xsd: <http://www.w3.org/2001/XMLSchema#> .\n\n",
        base_prefix);
    
    char* pos = dataset + strlen(dataset);
    
    // Generate people
    int people_count = triple_count / 10; // 10 triples per person
    for (int i = 0; i < people_count; i++) {
        pos += sprintf(pos,
            "ex:person%d a foaf:Person ;\n"
            "    foaf:name \"Person %d\" ;\n"
            "    foaf:age %d ;\n"
            "    foaf:email \"person%d@%s.com\" ;\n"
            "    ex:id %d ;\n"
            "    ex:salary \"%d\"^^xsd:decimal ;\n"
            "    ex:active %s ;\n"
            "    ex:department ex:dept%d ;\n"
            "    foaf:knows ex:person%d, ex:person%d .\n\n",
            i, i, 20 + (i % 50), i, base_prefix, i,
            50000 + (i * 1000), (i % 2) ? "true" : "false",
            i % 5, (i + 1) % people_count, (i + 2) % people_count);
    }
    
    // Generate departments
    for (int i = 0; i < 5; i++) {
        pos += sprintf(pos,
            "ex:dept%d ex:name \"Department %d\" ;\n"
            "    ex:budget \"%d\"^^xsd:decimal ;\n"
            "    ex:manager ex:person%d .\n\n",
            i, i, 100000 + (i * 50000), i);
    }
    
    return dataset;
}

/**
 * Test parsing performance with different dataset sizes
 */
static int test_parsing_performance() {
    printf("    Testing parsing performance...\n");
    
    struct {
        int triple_count;
        const char* size_name;
        double threshold_ms;
    } test_cases[] = {
        {SMALL_DATASET_TRIPLES, "Small", SMALL_PARSE_THRESHOLD_MS},
        {MEDIUM_DATASET_TRIPLES, "Medium", MEDIUM_PARSE_THRESHOLD_MS},
        {LARGE_DATASET_TRIPLES, "Large", LARGE_PARSE_THRESHOLD_MS}
    };
    
    for (int i = 0; i < 3; i++) {
        printf("      Testing %s dataset (%d triples)...\n", 
               test_cases[i].size_name, test_cases[i].triple_count);
        
        char* dataset = generate_test_dataset(test_cases[i].triple_count, test_cases[i].size_name);
        TEST_ASSERT_NOT_NULL(dataset);
        
        long mem_before = get_memory_usage_kb();
        double start_time = get_time_ms();
        
        // Parse the dataset
        ttl_lexer_t* lexer = ttl_lexer_create();
        TEST_ASSERT_NOT_NULL(lexer);
        
        ttl_lexer_input_from_string(lexer, dataset);
        
        ttl_parser_t* parser = ttl_parser_create(lexer);
        TEST_ASSERT_NOT_NULL(parser);
        
        ttl_ast_node_t* ast = ttl_parser_parse_document(parser);
        TEST_ASSERT_NOT_NULL(ast);
        
        double end_time = get_time_ms();
        long mem_after = get_memory_usage_kb();
        
        double parse_time = end_time - start_time;
        long mem_delta = mem_after - mem_before;
        
        printf("        Parse time: %.2f ms\n", parse_time);
        printf("        Memory used: %ld KB\n", mem_delta);
        printf("        Throughput: %.0f triples/sec\n", 
               test_cases[i].triple_count / (parse_time / 1000.0));
        
        // Performance assertions
        TEST_ASSERT(parse_time < test_cases[i].threshold_ms);
        TEST_ASSERT(mem_delta < (test_cases[i].triple_count * 2)); // Reasonable memory usage
        
        ttl_parser_destroy(parser);
        ttl_lexer_destroy(lexer);
        free(dataset);
    }
    
    return 1;
}

/**
 * Test serialization performance for all formats
 */
static int test_serialization_performance() {
    printf("    Testing serialization performance...\n");
    
    // Generate medium-size test dataset
    char* dataset = generate_test_dataset(MEDIUM_DATASET_TRIPLES, "serialize_test");
    TEST_ASSERT_NOT_NULL(dataset);
    
    // Parse once
    ttl_lexer_t* lexer = ttl_lexer_create();
    ttl_lexer_input_from_string(lexer, dataset);
    ttl_parser_t* parser = ttl_parser_create(lexer);
    ttl_ast_node_t* ast = ttl_parser_parse_document(parser);
    TEST_ASSERT_NOT_NULL(ast);
    
    // Test each serialization format
    const char* format_names[] = {"N-Triples", "JSON-LD", "RDF/XML"};
    ttl_serializer_format_t formats[] = {
        TTL_FORMAT_NTRIPLES,
        TTL_FORMAT_JSONLD,
        TTL_FORMAT_RDFXML
    };
    
    for (int i = 0; i < 3; i++) {
        printf("      Testing %s serialization...\n", format_names[i]);
        
        FILE* output = tmpfile();
        TEST_ASSERT_NOT_NULL(output);
        
        long mem_before = get_memory_usage_kb();
        double start_time = get_time_ms();
        
        bool result = false;
        switch (formats[i]) {
            case TTL_FORMAT_NTRIPLES:
                result = ttl_serialize_ntriples(ast, output);
                break;
            case TTL_FORMAT_JSONLD:
                result = ttl_serialize_jsonld(ast, output, false); // No pretty print for speed
                break;
            case TTL_FORMAT_RDFXML:
                result = ttl_serialize_rdfxml(ast, output, true);
                break;
        }
        
        double end_time = get_time_ms();
        long mem_after = get_memory_usage_kb();
        
        TEST_ASSERT(result);
        
        // Get output size
        fseek(output, 0, SEEK_END);
        long output_size = ftell(output);
        
        double serialize_time = end_time - start_time;
        long mem_delta = mem_after - mem_before;
        
        printf("        Serialize time: %.2f ms\n", serialize_time);
        printf("        Output size: %ld bytes\n", output_size);
        printf("        Memory used: %ld KB\n", mem_delta);
        printf("        Throughput: %.0f KB/sec\n", 
               (output_size / 1024.0) / (serialize_time / 1000.0));
        
        // Performance assertions
        TEST_ASSERT(serialize_time < SERIALIZATION_THRESHOLD_MS);
        TEST_ASSERT(output_size > 0);
        
        fclose(output);
    }
    
    ttl_parser_destroy(parser);
    ttl_lexer_destroy(lexer);
    free(dataset);
    
    return 1;
}

/**
 * Test query execution performance
 */
static int test_query_performance() {
    printf("    Testing query execution performance...\n");
    
    // Generate large dataset for query testing
    char* dataset = generate_test_dataset(LARGE_DATASET_TRIPLES, "query_test");
    TEST_ASSERT_NOT_NULL(dataset);
    
    // Parse dataset
    ttl_lexer_t* lexer = ttl_lexer_create();
    ttl_lexer_input_from_string(lexer, dataset);
    ttl_parser_t* parser = ttl_parser_create(lexer);
    ttl_ast_node_t* ast = ttl_parser_parse_document(parser);
    TEST_ASSERT_NOT_NULL(ast);
    
    // Create query engine
    ttl_query_engine_t* engine = ttl_query_engine_create(ast, parser->context);
    TEST_ASSERT_NOT_NULL(engine);
    
    // Test different query types
    struct {
        const char* name;
        const char* query;
        bool expect_many_results;
    } query_tests[] = {
        {"Type query", "?s a foaf:Person", true},
        {"Property query", "?s foaf:name ?name", true},
        {"Specific subject", "ex:person1 ?p ?o", false},
        {"Complex pattern", "?s foaf:knows ?friend", true}
    };
    
    for (int i = 0; i < 4; i++) {
        printf("      Testing %s...\n", query_tests[i].name);
        
        long mem_before = get_memory_usage_kb();
        double start_time = get_time_ms();
        
        ttl_query_result_t* result = ttl_query_execute_simple(engine, query_tests[i].query);
        
        double end_time = get_time_ms();
        long mem_after = get_memory_usage_kb();
        
        TEST_ASSERT_NOT_NULL(result);
        
        size_t result_count = ttl_query_result_count(result);
        double query_time = end_time - start_time;
        long mem_delta = mem_after - mem_before;
        
        printf("        Query time: %.2f ms\n", query_time);
        printf("        Results: %zu\n", result_count);
        printf("        Memory used: %ld KB\n", mem_delta);
        printf("        Throughput: %.0f results/sec\n", 
               result_count / (query_time / 1000.0));
        
        // Performance assertions
        TEST_ASSERT(query_time < QUERY_THRESHOLD_MS);
        TEST_ASSERT(result_count > 0);
        if (query_tests[i].expect_many_results) {
            TEST_ASSERT(result_count > 100); // Should find many in large dataset
        }
        
        ttl_query_result_destroy(result);
    }
    
    ttl_query_engine_destroy(engine);
    ttl_parser_destroy(parser);
    ttl_lexer_destroy(lexer);
    free(dataset);
    
    return 1;
}

/**
 * Test memory efficiency and leak detection
 */
static int test_memory_efficiency() {
    printf("    Testing memory efficiency...\n");
    
    long initial_memory = get_memory_usage_kb();
    
    // Stress test: Parse and destroy many small documents
    for (int iteration = 0; iteration < 100; iteration++) {
        char small_ttl[1024];
        snprintf(small_ttl, sizeof(small_ttl),
            "@prefix ex: <http://example.org/iter%d/> .\n"
            "ex:item1 ex:value %d .\n"
            "ex:item2 ex:value %d .\n",
            iteration, iteration, iteration * 2);
        
        ttl_lexer_t* lexer = ttl_lexer_create();
        ttl_lexer_input_from_string(lexer, small_ttl);
        ttl_parser_t* parser = ttl_parser_create(lexer);
        ttl_ast_node_t* ast = ttl_parser_parse_document(parser);
        
        // Quick serialization test
        FILE* output = tmpfile();
        ttl_serialize_ntriples(ast, output);
        fclose(output);
        
        ttl_parser_destroy(parser);
        ttl_lexer_destroy(lexer);
        
        if (iteration % 20 == 0) {
            long current_memory = get_memory_usage_kb();
            printf("        Iteration %d: %ld KB\n", iteration, current_memory - initial_memory);
        }
    }
    
    long final_memory = get_memory_usage_kb();
    long memory_growth = final_memory - initial_memory;
    
    printf("      Memory growth after 100 iterations: %ld KB\n", memory_growth);
    
    // Memory growth should be reasonable (not linear with iterations)
    TEST_ASSERT(memory_growth < 10000); // Less than 10MB growth
    
    return 1;
}

/**
 * Test concurrent operation performance
 */
static int test_concurrent_performance() {
    printf("    Testing concurrent operation performance...\n");
    
    // Generate dataset for concurrent testing
    char* dataset = generate_test_dataset(MEDIUM_DATASET_TRIPLES, "concurrent");
    TEST_ASSERT_NOT_NULL(dataset);
    
    // Simulate concurrent operations by rapidly creating/destroying parsers
    double start_time = get_time_ms();
    
    for (int i = 0; i < 10; i++) {
        ttl_lexer_t* lexer = ttl_lexer_create();
        ttl_lexer_input_from_string(lexer, dataset);
        ttl_parser_t* parser = ttl_parser_create(lexer);
        ttl_ast_node_t* ast = ttl_parser_parse_document(parser);
        
        // Quick operations
        ttl_query_engine_t* engine = ttl_query_engine_create(ast, parser->context);
        ttl_query_result_t* result = ttl_query_find_instances(engine, "foaf:Person");
        
        size_t count = ttl_query_result_count(result);
        TEST_ASSERT(count > 0);
        
        ttl_query_result_destroy(result);
        ttl_query_engine_destroy(engine);
        ttl_parser_destroy(parser);
        ttl_lexer_destroy(lexer);
    }
    
    double end_time = get_time_ms();
    double total_time = end_time - start_time;
    
    printf("      10 concurrent operations: %.2f ms total\n", total_time);
    printf("      Average per operation: %.2f ms\n", total_time / 10.0);
    
    // Should complete reasonably quickly
    TEST_ASSERT(total_time < 1000.0);
    TEST_ASSERT((total_time / 10.0) < 100.0);
    
    free(dataset);
    
    return 1;
}

/**
 * Test scalability with increasing dataset sizes
 */
static int test_scalability() {
    printf("    Testing scalability characteristics...\n");
    
    int sizes[] = {100, 500, 1000, 2000, 5000};
    double parse_times[5];
    double query_times[5];
    
    for (int i = 0; i < 5; i++) {
        printf("      Testing with %d triples...\n", sizes[i]);
        
        char* dataset = generate_test_dataset(sizes[i], "scale_test");
        TEST_ASSERT_NOT_NULL(dataset);
        
        // Measure parse time
        double start_time = get_time_ms();
        
        ttl_lexer_t* lexer = ttl_lexer_create();
        ttl_lexer_input_from_string(lexer, dataset);
        ttl_parser_t* parser = ttl_parser_create(lexer);
        ttl_ast_node_t* ast = ttl_parser_parse_document(parser);
        
        double mid_time = get_time_ms();
        parse_times[i] = mid_time - start_time;
        
        // Measure query time
        ttl_query_engine_t* engine = ttl_query_engine_create(ast, parser->context);
        ttl_query_result_t* result = ttl_query_find_instances(engine, "foaf:Person");
        
        double end_time = get_time_ms();
        query_times[i] = end_time - mid_time;
        
        printf("        Parse: %.2f ms, Query: %.2f ms\n", 
               parse_times[i], query_times[i]);
        
        ttl_query_result_destroy(result);
        ttl_query_engine_destroy(engine);
        ttl_parser_destroy(parser);
        ttl_lexer_destroy(lexer);
        free(dataset);
    }
    
    // Analyze scalability
    printf("      Scalability analysis:\n");
    for (int i = 1; i < 5; i++) {
        double size_ratio = (double)sizes[i] / sizes[0];
        double parse_ratio = parse_times[i] / parse_times[0];
        double query_ratio = query_times[i] / query_times[0];
        
        printf("        %dx size: Parse %.1fx, Query %.1fx\n", 
               (int)size_ratio, parse_ratio, query_ratio);
        
        // Performance should scale reasonably (not exponentially)
        TEST_ASSERT(parse_ratio < (size_ratio * 2.0)); // Less than 2x linear
        TEST_ASSERT(query_ratio < (size_ratio * 1.5)); // Less than 1.5x linear
    }
    
    return 1;
}

/**
 * Benchmark comparison with baseline
 */
static int test_performance_baseline() {
    printf("    Establishing performance baseline...\n");
    
    // Standard test document
    const char* baseline_ttl = 
        "@prefix ex: <http://example.org/> .\n"
        "@prefix foaf: <http://xmlns.com/foaf/0.1/> .\n"
        "\n"
        "ex:alice a foaf:Person ;\n"
        "    foaf:name \"Alice Johnson\" ;\n"
        "    foaf:age 28 ;\n"
        "    foaf:email \"alice@example.com\" .\n"
        "\n"
        "ex:bob a foaf:Person ;\n"
        "    foaf:name \"Bob Smith\" ;\n"
        "    foaf:age 35 ;\n"
        "    foaf:email \"bob@example.com\" .\n";
    
    // Run baseline tests multiple times for accuracy
    double parse_times[10];
    double serialize_times[10];
    double query_times[10];
    
    for (int run = 0; run < 10; run++) {
        // Parse test
        double start = get_time_ms();
        ttl_lexer_t* lexer = ttl_lexer_create();
        ttl_lexer_input_from_string(lexer, baseline_ttl);
        ttl_parser_t* parser = ttl_parser_create(lexer);
        ttl_ast_node_t* ast = ttl_parser_parse_document(parser);
        double mid1 = get_time_ms();
        parse_times[run] = mid1 - start;
        
        // Serialize test
        FILE* output = tmpfile();
        ttl_serialize_ntriples(ast, output);
        double mid2 = get_time_ms();
        serialize_times[run] = mid2 - mid1;
        fclose(output);
        
        // Query test
        ttl_query_engine_t* engine = ttl_query_engine_create(ast, parser->context);
        ttl_query_result_t* result = ttl_query_find_instances(engine, "foaf:Person");
        double end = get_time_ms();
        query_times[run] = end - mid2;
        
        ttl_query_result_destroy(result);
        ttl_query_engine_destroy(engine);
        ttl_parser_destroy(parser);
        ttl_lexer_destroy(lexer);
    }
    
    // Calculate averages
    double avg_parse = 0, avg_serialize = 0, avg_query = 0;
    for (int i = 0; i < 10; i++) {
        avg_parse += parse_times[i];
        avg_serialize += serialize_times[i];
        avg_query += query_times[i];
    }
    avg_parse /= 10.0;
    avg_serialize /= 10.0;
    avg_query /= 10.0;
    
    printf("      Baseline performance (10-run average):\n");
    printf("        Parse: %.3f ms\n", avg_parse);
    printf("        Serialize: %.3f ms\n", avg_serialize);
    printf("        Query: %.3f ms\n", avg_query);
    printf("        Total: %.3f ms\n", avg_parse + avg_serialize + avg_query);
    
    // Baseline expectations
    TEST_ASSERT(avg_parse < 5.0);      // Should parse in under 5ms
    TEST_ASSERT(avg_serialize < 5.0);  // Should serialize in under 5ms
    TEST_ASSERT(avg_query < 2.0);      // Should query in under 2ms
    
    return 1;
}

/**
 * Run all performance tests
 */
static void run_performance_tests() {
    RUN_TEST(test_parsing_performance);
    RUN_TEST(test_serialization_performance);
    RUN_TEST(test_query_performance);
    RUN_TEST(test_memory_efficiency);
    RUN_TEST(test_concurrent_performance);
    RUN_TEST(test_scalability);
    RUN_TEST(test_performance_baseline);
}

/**
 * Main test entry point
 */
int main(int argc, char* argv[]) {
    printf("⚡ TTL Parser - Performance Benchmarks\n");
    printf("=====================================\n");
    
    init_test_stats();
    
    RUN_TEST_SUITE("Performance", run_performance_tests);
    
    print_test_summary();
    check_memory_leaks();
    
    return g_test_stats.failed == 0 ? 0 : 1;
}