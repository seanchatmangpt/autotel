/**
 * @file test_7t_parser.c
 * @brief 7T TTL Parser compliance test
 * @author CNS Seven-Tick Engine
 * @date 2024
 * 
 * This test verifies that the TTL parser meets 7-tick performance requirements:
 * - DFA-based lexer with fixed-cycle tokenization
 * - Single-pass parser with O(1) per token processing
 * - Immediate triple materialization
 * - Performance measurement and validation
 */

#include "cns/parser.h"
#include "cns/arena.h"
#include "cns/interner.h"
#include "cns/graph.h"
#include "cns/types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>

// Test data
static const char* test_ttl_simple = 
    "@prefix ex: <http://example.org/> .\n"
    "@base <http://example.org/base/> .\n"
    "ex:subject ex:predicate \"literal value\" .\n"
    "ex:subject ex:number 42 .\n"
    "ex:subject ex:decimal 3.14 .\n"
    "ex:subject ex:boolean true .\n"
    "ex:subject a ex:Class .\n";

static const char* test_ttl_complex = 
    "@prefix rdf: <http://www.w3.org/1999/02/22-rdf-syntax-ns#> .\n"
    "@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> .\n"
    "@prefix ex: <http://example.org/> .\n"
    "\n"
    "ex:person1 a ex:Person ;\n"
    "    ex:name \"John Doe\" ;\n"
    "    ex:age 30 ;\n"
    "    ex:knows ex:person2 .\n"
    "\n"
    "ex:person2 a ex:Person ;\n"
    "    ex:name \"Jane Smith\" ;\n"
    "    ex:age 25 ;\n"
    "    ex:address [ ex:street \"123 Main St\" ;\n"
    "                 ex:city \"Anytown\" ;\n"
    "                 ex:zip \"12345\" ] .\n"
    "\n"
    "_:collection rdf:value ( \"item1\" \"item2\" \"item3\" ) .\n";

// Performance measurement utilities
static cns_tick_t get_cpu_ticks(void) {
#if defined(__x86_64__) || defined(__i386__)
    uint64_t tsc;
    __asm__ volatile ("rdtsc" : "=A" (tsc));
    return tsc;
#elif defined(__aarch64__)
    uint64_t tsc;
    __asm__ volatile ("mrs %0, cntvct_el0" : "=r" (tsc));
    return tsc;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
#endif
}

// Test results structure
typedef struct {
    int tests_run;
    int tests_passed;
    int tests_failed;
    cns_tick_t total_ticks;
    cns_tick_t max_token_ticks;
    cns_tick_t min_token_ticks;
    size_t total_tokens;
    size_t total_triples;
} test_results_t;

// Test function prototypes
static bool test_lexer_7t_compliance(test_results_t *results);
static bool test_parser_7t_compliance(test_results_t *results);
static bool test_simple_ttl_parsing(test_results_t *results);
static bool test_complex_ttl_parsing(test_results_t *results);
static bool test_error_handling(test_results_t *results);
static bool test_prefix_resolution(test_results_t *results);

// Utility functions
static void print_test_header(const char *test_name);
static void print_test_result(const char *test_name, bool passed);
static void print_performance_stats(const test_results_t *results);

/**
 * Main test runner
 */
int main(void) {
    printf("==========================================================\n");
    printf("CNS 7T TTL Parser Compliance Test Suite\n");
    printf("==========================================================\n\n");
    
    test_results_t results = {0};
    results.min_token_ticks = UINT64_MAX;
    
    // Run all tests
    bool all_passed = true;
    
    all_passed &= test_lexer_7t_compliance(&results);
    all_passed &= test_parser_7t_compliance(&results);
    all_passed &= test_simple_ttl_parsing(&results);
    all_passed &= test_complex_ttl_parsing(&results);
    all_passed &= test_error_handling(&results);
    all_passed &= test_prefix_resolution(&results);
    
    // Print final results
    printf("\n==========================================================\n");
    printf("TEST SUMMARY\n");
    printf("==========================================================\n");
    printf("Tests run:    %d\n", results.tests_run);
    printf("Tests passed: %d\n", results.tests_passed);
    printf("Tests failed: %d\n", results.tests_failed);
    printf("Overall:      %s\n", all_passed ? "PASS" : "FAIL");
    printf("\n");
    
    print_performance_stats(&results);
    
    return all_passed ? 0 : 1;
}

/**
 * Test lexer 7T compliance
 */
static bool test_lexer_7t_compliance(test_results_t *results) {
    print_test_header("Lexer 7T Compliance");
    results->tests_run++;
    
    // Create test environment
    cns_arena_t *arena = cns_arena_create(1024 * 1024);
    cns_interner_t *interner = cns_interner_create(arena);
    cns_graph_t *graph = cns_graph_create(arena);
    
    if (!arena || !interner || !graph) {
        print_test_result("Lexer 7T Compliance", false);
        results->tests_failed++;
        return false;
    }
    
    cns_parser_t *parser = cns_parser_create_default(arena, interner, graph);
    if (!parser) {
        print_test_result("Lexer 7T Compliance", false);
        results->tests_failed++;
        cns_arena_destroy(arena);
        return false;
    }
    
    // Set up parser with test input
    cns_result_t result = cns_parser_reset(parser);
    assert(result == CNS_OK);
    
    parser->state.input = test_ttl_simple;
    parser->state.current = test_ttl_simple;
    parser->state.length = strlen(test_ttl_simple);
    parser->state.end = test_ttl_simple + parser->state.length;
    
    // Measure tokenization performance
    cns_tick_t start_tick = get_cpu_ticks();
    cns_tick_t total_token_ticks = 0;
    size_t token_count = 0;
    bool compliance_passed = true;
    
    while (true) {
        cns_token_t token;
        
        cns_tick_t token_start = get_cpu_ticks();
        result = cns_parser_next_token(parser, &token);
        cns_tick_t token_end = get_cpu_ticks();
        
        if (result != CNS_OK) {
            printf("  ERROR: Failed to get token: %d\n", result);
            compliance_passed = false;
            break;
        }
        
        cns_tick_t token_ticks = token_end - token_start;
        total_token_ticks += token_ticks;
        token_count++;
        
        // Update statistics
        if (token_ticks > results->max_token_ticks) {
            results->max_token_ticks = token_ticks;
        }
        if (token_ticks < results->min_token_ticks) {
            results->min_token_ticks = token_ticks;
        }
        
        // Check 7T compliance
        if (token_ticks > CNS_7T_TICK_LIMIT) {
            printf("  ERROR: Token exceeded 7-tick limit: %lu ticks\n", token_ticks);
            compliance_passed = false;
        }
        
        if (token.type == CNS_TOKEN_EOF) break;
    }
    
    cns_tick_t end_tick = get_cpu_ticks();
    results->total_ticks += (end_tick - start_tick);
    results->total_tokens += token_count;
    
    double avg_ticks = token_count > 0 ? (double)total_token_ticks / token_count : 0;
    
    printf("  Tokens processed: %zu\n", token_count);
    printf("  Average ticks per token: %.2f\n", avg_ticks);
    printf("  Max ticks per token: %lu\n", results->max_token_ticks);
    printf("  7T compliance: %s\n", compliance_passed ? "PASS" : "FAIL");
    
    print_test_result("Lexer 7T Compliance", compliance_passed);
    
    if (compliance_passed) {
        results->tests_passed++;
    } else {
        results->tests_failed++;
    }
    
    cns_arena_destroy(arena);
    return compliance_passed;
}

/**
 * Test parser 7T compliance
 */
static bool test_parser_7t_compliance(test_results_t *results) {
    print_test_header("Parser 7T Compliance");
    results->tests_run++;
    
    // Create test environment
    cns_arena_t *arena = cns_arena_create(1024 * 1024);
    cns_interner_t *interner = cns_interner_create(arena);
    cns_graph_t *graph = cns_graph_create(arena);
    
    if (!arena || !interner || !graph) {
        print_test_result("Parser 7T Compliance", false);
        results->tests_failed++;
        return false;
    }
    
    cns_parser_t *parser = cns_parser_create_default(arena, interner, graph);
    if (!parser) {
        print_test_result("Parser 7T Compliance", false);
        results->tests_failed++;
        cns_arena_destroy(arena);
        return false;
    }
    
    // Measure parsing performance
    cns_tick_t start_tick = get_cpu_ticks();
    cns_result_t result = cns_parser_parse_document(parser, test_ttl_complex);
    cns_tick_t end_tick = get_cpu_ticks();
    
    bool parsing_passed = (result == CNS_OK);
    cns_tick_t total_parse_ticks = end_tick - start_tick;
    
    // Get parser statistics
    cns_parser_stats_t stats;
    cns_parser_get_stats(parser, &stats);
    
    results->total_tokens += stats.tokens_parsed;
    results->total_triples += stats.triples_parsed;
    results->total_ticks += total_parse_ticks;
    
    double avg_ticks_per_token = stats.tokens_parsed > 0 ? 
        (double)total_parse_ticks / stats.tokens_parsed : 0;
    
    printf("  Tokens parsed: %lu\n", stats.tokens_parsed);
    printf("  Triples parsed: %lu\n", stats.triples_parsed);
    printf("  Total parse ticks: %lu\n", total_parse_ticks);
    printf("  Average ticks per token: %.2f\n", avg_ticks_per_token);
    printf("  Parsing result: %s\n", parsing_passed ? "SUCCESS" : "FAILED");
    
    // Check for errors
    const cns_parser_error_t *errors;
    size_t error_count;
    cns_parser_get_errors(parser, &errors, &error_count);
    
    if (error_count > 0) {
        printf("  Parse errors: %zu\n", error_count);
        for (size_t i = 0; i < error_count && i < 5; i++) {
            printf("    Error %zu: Line %u, Column %u: %s\n", 
                   i + 1, errors[i].line, errors[i].column,
                   cns_interner_get_string(parser->interner, errors[i].message));
        }
    }
    
    bool compliance_passed = parsing_passed && (avg_ticks_per_token <= CNS_7T_TICK_LIMIT);
    
    print_test_result("Parser 7T Compliance", compliance_passed);
    
    if (compliance_passed) {
        results->tests_passed++;
    } else {
        results->tests_failed++;
    }
    
    cns_arena_destroy(arena);
    return compliance_passed;
}

/**
 * Test simple TTL parsing
 */
static bool test_simple_ttl_parsing(test_results_t *results) {
    print_test_header("Simple TTL Parsing");
    results->tests_run++;
    
    // Create test environment
    cns_arena_t *arena = cns_arena_create(1024 * 1024);
    cns_interner_t *interner = cns_interner_create(arena);
    cns_graph_t *graph = cns_graph_create(arena);
    cns_parser_t *parser = cns_parser_create_default(arena, interner, graph);
    
    if (!parser) {
        print_test_result("Simple TTL Parsing", false);
        results->tests_failed++;
        return false;
    }
    
    // Parse simple TTL
    cns_result_t result = cns_parser_parse_document(parser, test_ttl_simple);
    bool parsing_passed = (result == CNS_OK);
    
    // Check results
    cns_parser_stats_t stats;
    cns_parser_get_stats(parser, &stats);
    
    printf("  Triples parsed: %lu\n", stats.triples_parsed);
    printf("  Prefixes defined: %lu\n", stats.prefixes_defined);
    printf("  Parse result: %s\n", parsing_passed ? "SUCCESS" : "FAILED");
    
    // Verify expected number of triples (at least 5 from the test data)
    bool content_valid = (stats.triples_parsed >= 5);
    bool test_passed = parsing_passed && content_valid;
    
    print_test_result("Simple TTL Parsing", test_passed);
    
    if (test_passed) {
        results->tests_passed++;
    } else {
        results->tests_failed++;
    }
    
    cns_arena_destroy(arena);
    return test_passed;
}

/**
 * Test complex TTL parsing
 */
static bool test_complex_ttl_parsing(test_results_t *results) {
    print_test_header("Complex TTL Parsing");
    results->tests_run++;
    
    // Create test environment
    cns_arena_t *arena = cns_arena_create(1024 * 1024);
    cns_interner_t *interner = cns_interner_create(arena);
    cns_graph_t *graph = cns_graph_create(arena);
    cns_parser_t *parser = cns_parser_create_default(arena, interner, graph);
    
    if (!parser) {
        print_test_result("Complex TTL Parsing", false);
        results->tests_failed++;
        return false;
    }
    
    // Parse complex TTL
    cns_result_t result = cns_parser_parse_document(parser, test_ttl_complex);
    bool parsing_passed = (result == CNS_OK);
    
    // Check results
    cns_parser_stats_t stats;
    cns_parser_get_stats(parser, &stats);
    
    printf("  Triples parsed: %lu\n", stats.triples_parsed);
    printf("  Prefixes defined: %lu\n", stats.prefixes_defined);
    printf("  Parse result: %s\n", parsing_passed ? "SUCCESS" : "FAILED");
    
    print_test_result("Complex TTL Parsing", parsing_passed);
    
    if (parsing_passed) {
        results->tests_passed++;
    } else {
        results->tests_failed++;
    }
    
    cns_arena_destroy(arena);
    return parsing_passed;
}

/**
 * Test error handling
 */
static bool test_error_handling(test_results_t *results) {
    print_test_header("Error Handling");
    results->tests_run++;
    
    const char* invalid_ttl = 
        "@prefix ex: <http://example.org/> .\n"
        "ex:subject ex:predicate ; incomplete triple\n"
        "ex:subject < invalid iri > ex:object .\n";
    
    // Create test environment
    cns_arena_t *arena = cns_arena_create(1024 * 1024);
    cns_interner_t *interner = cns_interner_create(arena);
    cns_graph_t *graph = cns_graph_create(arena);
    cns_parser_t *parser = cns_parser_create_default(arena, interner, graph);
    
    if (!parser) {
        print_test_result("Error Handling", false);
        results->tests_failed++;
        return false;
    }
    
    // Parse invalid TTL (should fail gracefully)
    cns_result_t result = cns_parser_parse_document(parser, invalid_ttl);
    
    // Check that errors were reported
    const cns_parser_error_t *errors;
    size_t error_count;
    cns_parser_get_errors(parser, &errors, &error_count);
    
    printf("  Parse result: %s\n", (result != CNS_OK) ? "FAILED (expected)" : "UNEXPECTED SUCCESS");
    printf("  Errors reported: %zu\n", error_count);
    
    bool error_handling_passed = (result != CNS_OK) && (error_count > 0);
    
    print_test_result("Error Handling", error_handling_passed);
    
    if (error_handling_passed) {
        results->tests_passed++;
    } else {
        results->tests_failed++;
    }
    
    cns_arena_destroy(arena);
    return error_handling_passed;
}

/**
 * Test prefix resolution
 */
static bool test_prefix_resolution(test_results_t *results) {
    print_test_header("Prefix Resolution");
    results->tests_run++;
    
    // Create test environment
    cns_arena_t *arena = cns_arena_create(1024 * 1024);
    cns_interner_t *interner = cns_interner_create(arena);
    cns_graph_t *graph = cns_graph_create(arena);
    cns_parser_t *parser = cns_parser_create_default(arena, interner, graph);
    
    if (!parser) {
        print_test_result("Prefix Resolution", false);
        results->tests_failed++;
        return false;
    }
    
    // Define a prefix
    cns_result_t result = cns_parser_define_prefix(parser, "ex", "http://example.org/");
    bool define_passed = (result == CNS_OK);
    
    // Resolve a prefixed name
    cns_string_ref_t resolved_iri;
    result = cns_parser_resolve_prefix(parser, "ex:test", &resolved_iri);
    bool resolve_passed = (result == CNS_OK);
    
    if (resolve_passed) {
        const char* resolved_str = cns_interner_get_string(interner, resolved_iri);
        printf("  Resolved 'ex:test' to: %s\n", resolved_str ? resolved_str : "(null)");
        
        // Check if resolution is correct
        resolve_passed = resolved_str && 
                        strcmp(resolved_str, "http://example.org/test") == 0;
    }
    
    bool test_passed = define_passed && resolve_passed;
    
    printf("  Define prefix: %s\n", define_passed ? "SUCCESS" : "FAILED");
    printf("  Resolve prefix: %s\n", resolve_passed ? "SUCCESS" : "FAILED");
    
    print_test_result("Prefix Resolution", test_passed);
    
    if (test_passed) {
        results->tests_passed++;
    } else {
        results->tests_failed++;
    }
    
    cns_arena_destroy(arena);
    return test_passed;
}

/**
 * Print test header
 */
static void print_test_header(const char *test_name) {
    printf("----------------------------------------------------------\n");
    printf("Running: %s\n", test_name);
    printf("----------------------------------------------------------\n");
}

/**
 * Print test result
 */
static void print_test_result(const char *test_name, bool passed) {
    printf("Result:  %s - %s\n\n", test_name, passed ? "PASS" : "FAIL");
}

/**
 * Print performance statistics
 */
static void print_performance_stats(const test_results_t *results) {
    printf("PERFORMANCE STATISTICS\n");
    printf("==========================================================\n");
    printf("Total tokens processed:     %zu\n", results->total_tokens);
    printf("Total triples parsed:       %zu\n", results->total_triples);
    printf("Total CPU ticks:            %lu\n", results->total_ticks);
    
    if (results->total_tokens > 0) {
        double avg_ticks = (double)results->total_ticks / results->total_tokens;
        printf("Average ticks per token:    %.2f\n", avg_ticks);
        printf("7T compliance:              %s\n", 
               (avg_ticks <= CNS_7T_TICK_LIMIT) ? "PASS" : "FAIL");
    }
    
    printf("Min ticks per token:        %lu\n", 
           results->min_token_ticks == UINT64_MAX ? 0 : results->min_token_ticks);
    printf("Max ticks per token:        %lu\n", results->max_token_ticks);
    printf("7T tick limit:              %d\n", CNS_7T_TICK_LIMIT);
    printf("\n");
}