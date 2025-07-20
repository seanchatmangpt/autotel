#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../include/error.h"
#include "../include/diagnostic.h"
#include "../include/token.h"

// Test error context creation and destruction
void test_error_context() {
    printf("Testing error context creation...\n");
    
    ttl_error_context_t* ctx = ttl_error_context_create();
    assert(ctx != NULL);
    assert(ctx->error_count == 0);
    assert(ctx->warning_count == 0);
    assert(ctx->errors == NULL);
    
    ttl_error_context_destroy(ctx);
    printf("✓ Error context creation/destruction passed\n");
}

// Test error reporting
void test_error_reporting() {
    printf("Testing error reporting...\n");
    
    ttl_error_context_t* ctx = ttl_error_context_create();
    const char* source = "@prefix foaf: <http://xmlns.com/foaf/0.1/>.\n"
                        "@prefix foaf: <http://duplicate.org/>.\n"
                        "invalid syntax here\n";
    
    ttl_error_context_set_source(ctx, "test.ttl", source);
    
    // Report various errors
    ttl_location_t loc1 = {2, 1, 45, 7};
    ttl_error_report(ctx, ERROR_DUPLICATE_PREFIX, ERROR_SEVERITY_ERROR, loc1,
                     "Duplicate prefix definition: 'foaf'");
    
    ttl_location_t loc2 = {3, 1, 85, 19};
    ttl_error_report(ctx, ERROR_INVALID_SYNTAX, ERROR_SEVERITY_ERROR, loc2,
                     "Invalid syntax: expected subject, found 'invalid'");
    
    ttl_location_t loc3 = {3, 20, 104, 0};
    ttl_error_report_with_suggestion(ctx, ERROR_MISSING_DOT, ERROR_SEVERITY_WARNING, loc3,
                                     "Missing '.' at end of statement",
                                     "Add '.' to terminate the statement");
    
    assert(ttl_error_context_error_count(ctx) == 2);
    assert(ttl_error_context_warning_count(ctx) == 1);
    assert(ttl_error_context_has_errors(ctx) == true);
    assert(ttl_error_context_has_warnings(ctx) == true);
    
    // Print errors
    printf("\nError output (human-readable):\n");
    printf("--------------------------------\n");
    ttl_error_print_all(ctx, stdout);
    printf("--------------------------------\n");
    
    ttl_error_context_destroy(ctx);
    printf("✓ Error reporting passed\n");
}

// Test JSON output
void test_json_output() {
    printf("\nTesting JSON error output...\n");
    
    ttl_error_context_t* ctx = ttl_error_context_create();
    const char* source = "_:invalid-blank-node <p> \"literal\"^^<invalid-datatype> .";
    
    ttl_error_context_set_source(ctx, "test.ttl", source);
    ttl_error_context_set_json_output(ctx, true);
    
    ttl_location_t loc1 = {1, 1, 0, 20};
    ttl_error_report(ctx, ERROR_INVALID_BLANK_NODE, ERROR_SEVERITY_ERROR, loc1,
                     "Invalid blank node label: contains hyphen");
    
    ttl_location_t loc2 = {1, 38, 37, 19};
    ttl_error_report(ctx, ERROR_INVALID_IRI, ERROR_SEVERITY_ERROR, loc2,
                     "Invalid IRI: relative IRIs not allowed for datatypes");
    
    printf("\nJSON output:\n");
    printf("--------------------------------\n");
    ttl_error_print_json(ctx, stdout);
    printf("--------------------------------\n");
    
    ttl_error_context_destroy(ctx);
    printf("✓ JSON output passed\n");
}

// Test diagnostic engine
void test_diagnostic_engine() {
    printf("\nTesting diagnostic engine...\n");
    
    ttl_error_context_t* ctx = ttl_error_context_create();
    diagnostic_engine_t* diag = ttl_diagnostic_create(ctx);
    
    const char* source = "@prefix : <http://example.org/> .\n"
                        ":subject :predicate \"unterminated string\n"
                        ":another :triple :object .\n";
    
    ttl_error_context_set_source(ctx, "test.ttl", source);
    
    // Create error
    ttl_location_t loc = {2, 21, 55, 20};
    ttl_error_report(ctx, ERROR_UNTERMINATED_STRING, ERROR_SEVERITY_ERROR, loc,
                     "Unterminated string literal");
    
    // Enhance with diagnostic info
    ttl_error_t* error = ctx->errors;
    diagnostic_info_t* info = ttl_diagnostic_enhance_error(diag, error);
    
    // Add notes and suggestions
    ttl_location_t note_loc = {2, 41, 75, 0};
    ttl_diagnostic_add_note(info, note_loc, "String opened here but never closed");
    ttl_diagnostic_suggest_fix(info, "Add closing quote", "\"");
    
    // Print enhanced diagnostic
    printf("\nEnhanced diagnostic output:\n");
    printf("--------------------------------\n");
    ttl_diagnostic_print_error(diag, info);
    ttl_diagnostic_print_summary(diag);
    printf("--------------------------------\n");
    
    ttl_diagnostic_info_destroy(info);
    ttl_diagnostic_destroy(diag);
    ttl_error_context_destroy(ctx);
    
    printf("✓ Diagnostic engine passed\n");
}

// Test error recovery strategies
void test_error_recovery() {
    printf("\nTesting error recovery strategies...\n");
    
    // Test recovery strategy mapping
    assert(ttl_error_get_recovery_strategy(ERROR_INVALID_CHARACTER) == RECOVERY_SKIP_TOKEN);
    assert(ttl_error_get_recovery_strategy(ERROR_UNTERMINATED_STRING) == RECOVERY_SKIP_TOKEN);
    assert(ttl_error_get_recovery_strategy(ERROR_MISSING_DOT) == RECOVERY_SKIP_STATEMENT);
    assert(ttl_error_get_recovery_strategy(ERROR_UNEXPECTED_TOKEN) == RECOVERY_SYNC_DELIMITER);
    assert(ttl_error_get_recovery_strategy(ERROR_OUT_OF_MEMORY) == RECOVERY_NONE);
    
    // Test recoverability
    assert(ttl_error_is_recoverable(ERROR_INVALID_CHARACTER) == true);
    assert(ttl_error_is_recoverable(ERROR_MISSING_SEMICOLON) == true);
    assert(ttl_error_is_recoverable(ERROR_OUT_OF_MEMORY) == false);
    assert(ttl_error_is_recoverable(ERROR_INTERNAL_ERROR) == false);
    
    printf("✓ Error recovery strategies passed\n");
}

// Test batch error reporting
void test_batch_reporting() {
    printf("\nTesting batch error reporting...\n");
    
    ttl_error_context_t* ctx = ttl_error_context_create();
    diagnostic_engine_t* diag = ttl_diagnostic_create(ctx);
    diagnostic_batch_t* batch = ttl_diagnostic_batch_create();
    
    const char* source = "@base <http://example.org/> .\n"
                        "@prefix : <> .\n"
                        ":s1 :p1 :o1 ; :p2 :o2\n"  // Missing dot
                        ":s2 :p3 \"literal\"@invalid-lang .\n"
                        ":s3 <invalid iri> :o3 .\n";
    
    ttl_error_context_set_source(ctx, "test.ttl", source);
    
    // Create multiple errors
    ttl_location_t loc1 = {3, 23, 67, 0};
    ttl_error_report(ctx, ERROR_MISSING_DOT, ERROR_SEVERITY_ERROR, loc1,
                     "Missing '.' at end of statement");
    
    ttl_location_t loc2 = {4, 18, 85, 13};
    ttl_error_report(ctx, ERROR_INVALID_LANGUAGE_TAG, ERROR_SEVERITY_ERROR, loc2,
                     "Invalid language tag: 'invalid-lang'");
    
    ttl_location_t loc3 = {5, 5, 119, 13};
    ttl_error_report(ctx, ERROR_INVALID_IRI, ERROR_SEVERITY_ERROR, loc3,
                     "Invalid IRI: contains space");
    
    // Add to batch
    for (ttl_error_t* e = ctx->errors; e; e = e->next) {
        diagnostic_info_t* info = ttl_diagnostic_enhance_error(diag, e);
        ttl_diagnostic_batch_add(batch, info);
    }
    
    // Print batch (sorted by location)
    printf("\nBatch error report:\n");
    printf("--------------------------------\n");
    ttl_diagnostic_batch_print(diag, batch);
    printf("--------------------------------\n");
    
    ttl_diagnostic_batch_destroy(batch);
    ttl_diagnostic_destroy(diag);
    ttl_error_context_destroy(ctx);
    
    printf("✓ Batch error reporting passed\n");
}

// Test warning control
void test_warning_control() {
    printf("\nTesting warning control...\n");
    
    ttl_error_context_t* ctx = ttl_error_context_create();
    diagnostic_engine_t* diag = ttl_diagnostic_create(ctx);
    
    // Suppress specific warning
    ttl_diagnostic_suppress_warning(diag, ERROR_MISSING_DOT);
    
    // Promote warning to error
    ttl_diagnostic_promote_warning(diag, ERROR_INVALID_LANGUAGE_TAG);
    
    // Test suppression
    assert(ttl_diagnostic_should_report(diag, ERROR_MISSING_DOT, ERROR_SEVERITY_WARNING) == false);
    assert(ttl_diagnostic_should_report(diag, ERROR_MISSING_SEMICOLON, ERROR_SEVERITY_WARNING) == true);
    
    // Enable warnings as errors
    diag->werror = true;
    assert(ttl_diagnostic_should_report(diag, ERROR_INVALID_SYNTAX, ERROR_SEVERITY_WARNING) == true);
    
    ttl_diagnostic_destroy(diag);
    ttl_error_context_destroy(ctx);
    
    printf("✓ Warning control passed\n");
}

// Test code snippet display
void test_code_snippets() {
    printf("\nTesting code snippet display...\n");
    
    const char* source = "# This is a comment\n"
                        "@prefix foaf: <http://xmlns.com/foaf/0.1/> .\n"
                        "@prefix rdf: <http://www.w3.org/1999/02/22-rdf-syntax-ns#> .\n"
                        "\n"
                        "<http://example.org/alice> a foaf:Person ;\n"
                        "    foaf:name \"Alice\" ;\n"
                        "    foaf:knows <http://example.org/bob> .\n"
                        "\n"
                        "<http://example.org/bob> a foaf:Person ;\n"
                        "    foaf:name \"Bob\" ;\n"
                        "    foaf:age \"not a number\" .  # Error here\n";
    
    ttl_location_t loc = {11, 15, 310, 14};
    
    printf("\nCode snippet with error indicator:\n");
    printf("--------------------------------\n");
    ttl_diagnostic_print_code_snippet(stdout, source, loc, 2, true);
    printf("--------------------------------\n");
    
    printf("✓ Code snippet display passed\n");
}

int main() {
    printf("TTL Parser Error Handling Test Suite\n");
    printf("====================================\n\n");
    
    test_error_context();
    test_error_reporting();
    test_json_output();
    test_diagnostic_engine();
    test_error_recovery();
    test_batch_reporting();
    test_warning_control();
    test_code_snippets();
    
    printf("\n====================================\n");
    printf("All error handling tests passed! ✓\n\n");
    
    return 0;
}