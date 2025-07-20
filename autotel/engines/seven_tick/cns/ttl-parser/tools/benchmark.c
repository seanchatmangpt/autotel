// TTL Parser Benchmarking Tool
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/time.h>
#include "parser.h"
#include "serializer.h"
#include "query.h"

typedef struct {
    double parse_time;
    double serialize_time;
    double query_time;
    size_t memory_usage;
    size_t file_size;
    size_t triple_count;
    size_t error_count;
} BenchmarkResult;

// High precision timer
static double get_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000.0) + (tv.tv_usec / 1000.0);
}

// Memory tracking (simplified)
static size_t get_memory_usage() {
    // Platform-specific memory measurement would go here
    // For now, return estimated usage
    return 0;
}

// Benchmark parsing performance
static void benchmark_parsing(const char* filename, BenchmarkResult* result) {
    printf("Benchmarking parsing performance for %s...\n", filename);
    
    double start_time = get_time_ms();
    
    Parser* parser = parser_create();
    ASTNode* ast = parser_parse_file(parser, filename);
    
    double end_time = get_time_ms();
    result->parse_time = end_time - start_time;
    
    if (ast) {
        // Count triples using visitor
        TripleCountVisitor* counter = visitor_create_triple_counter();
        visitor_walk((Visitor*)counter, ast);
        result->triple_count = visitor_triple_counter_get_count(counter);
        visitor_destroy((Visitor*)counter);
        
        ast_node_release(ast);
    }
    
    // Get file size
    FILE* f = fopen(filename, "r");
    if (f) {
        fseek(f, 0L, SEEK_END);
        result->file_size = ftell(f);
        fclose(f);
    }
    
    ErrorContext* errors = parser_get_errors(parser);
    result->error_count = error_context_count(errors);
    
    parser_destroy(parser);
}

// Benchmark serialization performance
static void benchmark_serialization(const char* filename, BenchmarkResult* result) {
    printf("Benchmarking serialization performance...\n");
    
    // Parse first
    Parser* parser = parser_create();
    ASTNode* ast = parser_parse_file(parser, filename);
    
    if (!ast) {
        printf("Failed to parse file for serialization benchmark\n");
        parser_destroy(parser);
        return;
    }
    
    double start_time = get_time_ms();
    
    // Benchmark N-Triples serialization
    NTriplesSerializer* nt_serializer = ntriples_serializer_create();
    char* nt_output = ntriples_serialize(nt_serializer, ast);
    free(nt_output);
    ntriples_serializer_destroy(nt_serializer);
    
    double end_time = get_time_ms();
    result->serialize_time = end_time - start_time;
    
    ast_node_release(ast);
    parser_destroy(parser);
}

// Benchmark query performance
static void benchmark_queries(const char* filename, BenchmarkResult* result) {
    printf("Benchmarking query performance...\n");
    
    // Parse first
    Parser* parser = parser_create();
    ASTNode* ast = parser_parse_file(parser, filename);
    
    if (!ast) {
        printf("Failed to parse file for query benchmark\n");
        parser_destroy(parser);
        return;
    }
    
    double start_time = get_time_ms();
    
    // Run several common queries
    QueryEngine* engine = query_engine_create(ast);
    
    // Query 1: Find all subjects
    QueryPattern* pattern1 = query_pattern_create("?s", NULL, NULL);
    QueryResult* result1 = query_engine_execute(engine, pattern1);
    query_result_destroy(result1);
    query_pattern_destroy(pattern1);
    
    // Query 2: Find all types
    QueryPattern* pattern2 = query_pattern_create("?s", "rdf:type", "?type");
    QueryResult* result2 = query_engine_execute(engine, pattern2);
    query_result_destroy(result2);
    query_pattern_destroy(pattern2);
    
    // Query 3: Find specific predicate
    QueryPattern* pattern3 = query_pattern_create("?s", "foaf:name", "?name");
    QueryResult* result3 = query_engine_execute(engine, pattern3);
    query_result_destroy(result3);
    query_pattern_destroy(pattern3);
    
    double end_time = get_time_ms();
    result->query_time = end_time - start_time;
    
    query_engine_destroy(engine);
    ast_node_release(ast);
    parser_destroy(parser);
}

// Run comprehensive benchmark
static void run_benchmark_suite(const char* filename) {
    printf("\n=== TTL Parser Benchmark Suite ===\n");
    printf("File: %s\n\n", filename);
    
    BenchmarkResult result = {0};
    
    // Run all benchmarks
    benchmark_parsing(filename, &result);
    benchmark_serialization(filename, &result);
    benchmark_queries(filename, &result);
    
    // Report results
    printf("\n=== Benchmark Results ===\n");
    printf("File size: %zu bytes\n", result.file_size);
    printf("Triple count: %zu\n", result.triple_count);
    printf("Parse time: %.2f ms\n", result.parse_time);
    printf("Serialize time: %.2f ms\n", result.serialize_time);
    printf("Query time: %.2f ms\n", result.query_time);
    printf("Error count: %zu\n", result.error_count);
    
    // Calculate throughput
    if (result.parse_time > 0) {
        double parse_throughput = (result.file_size / 1024.0) / (result.parse_time / 1000.0);
        printf("Parse throughput: %.2f KB/s\n", parse_throughput);
        
        double triple_rate = result.triple_count / (result.parse_time / 1000.0);
        printf("Triple rate: %.0f triples/s\n", triple_rate);
    }
    
    printf("\n");
}

// Performance comparison with different options
static void run_performance_comparison(const char* filename) {
    printf("\n=== Performance Comparison ===\n");
    
    // Test with different parser options
    ParserOptions opts_strict = {
        .strict_mode = true,
        .recover_errors = false,
        .validate_iris = true,
        .resolve_prefixes = true
    };
    
    ParserOptions opts_fast = {
        .strict_mode = false,
        .recover_errors = true,
        .validate_iris = false,
        .resolve_prefixes = false
    };
    
    printf("Testing strict mode...\n");
    double start = get_time_ms();
    Parser* parser1 = parser_create();
    parser_set_options(parser1, &opts_strict);
    ASTNode* ast1 = parser_parse_file(parser1, filename);
    double strict_time = get_time_ms() - start;
    
    printf("Testing fast mode...\n");
    start = get_time_ms();
    Parser* parser2 = parser_create();
    parser_set_options(parser2, &opts_fast);
    ASTNode* ast2 = parser_parse_file(parser2, filename);
    double fast_time = get_time_ms() - start;
    
    printf("\nComparison Results:\n");
    printf("Strict mode: %.2f ms\n", strict_time);
    printf("Fast mode: %.2f ms\n", fast_time);
    printf("Speedup: %.2fx\n", strict_time / fast_time);
    
    // Cleanup
    if (ast1) ast_node_release(ast1);
    if (ast2) ast_node_release(ast2);
    parser_destroy(parser1);
    parser_destroy(parser2);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <ttl-file> [options]\n", argv[0]);
        printf("Options:\n");
        printf("  --suite     Run full benchmark suite\n");
        printf("  --compare   Run performance comparison\n");
        printf("  --parse     Parse benchmark only\n");
        printf("  --query     Query benchmark only\n");
        return 1;
    }
    
    const char* filename = argv[1];
    const char* mode = argc > 2 ? argv[2] : "--suite";
    
    if (strcmp(mode, "--suite") == 0) {
        run_benchmark_suite(filename);
        run_performance_comparison(filename);
    } else if (strcmp(mode, "--compare") == 0) {
        run_performance_comparison(filename);
    } else if (strcmp(mode, "--parse") == 0) {
        BenchmarkResult result = {0};
        benchmark_parsing(filename, &result);
        printf("Parse time: %.2f ms\n", result.parse_time);
        printf("Triple count: %zu\n", result.triple_count);
    } else if (strcmp(mode, "--query") == 0) {
        BenchmarkResult result = {0};
        benchmark_queries(filename, &result);
        printf("Query time: %.2f ms\n", result.query_time);
    }
    
    return 0;
}