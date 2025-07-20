#ifndef TTL_DIAGNOSTIC_H
#define TTL_DIAGNOSTIC_H

#include "error.h"
#include "token.h"
#include <stdbool.h>
#include <stdio.h>

// Diagnostic flags
typedef enum {
    DIAG_FLAG_NONE = 0,
    DIAG_FLAG_SHOW_COLUMN = 1 << 0,
    DIAG_FLAG_SHOW_SOURCE = 1 << 1,
    DIAG_FLAG_SHOW_SUGGESTIONS = 1 << 2,
    DIAG_FLAG_SHOW_CONTEXT = 1 << 3,
    DIAG_FLAG_COLOR = 1 << 4,
    DIAG_FLAG_JSON = 1 << 5,
    DIAG_FLAG_VERBOSE = 1 << 6,
    DIAG_FLAG_WARNINGS = 1 << 7,
    DIAG_FLAG_PEDANTIC = 1 << 8
} diagnostic_flags_t;

// Diagnostic output format
typedef enum {
    DIAG_FORMAT_HUMAN,    // Human-readable with source snippets
    DIAG_FORMAT_JSON,     // JSON format
    DIAG_FORMAT_COMPACT,  // Compact single-line format
    DIAG_FORMAT_GCC,      // GCC-style format
    DIAG_FORMAT_MSVC      // MSVC-style format
} diagnostic_format_t;

// Diagnostic suggestion
typedef struct {
    char* text;           // Suggestion text
    ttl_location_t location; // Where to apply the suggestion
    char* replacement;    // Replacement text (if applicable)
} diagnostic_suggestion_t;

// Diagnostic note
typedef struct diagnostic_note {
    ttl_location_t location;
    char* message;
    struct diagnostic_note* next;
} diagnostic_note_t;

// Enhanced diagnostic information
typedef struct {
    ttl_error_t* error;          // Base error
    diagnostic_note_t* notes;    // Additional notes
    diagnostic_suggestion_t* suggestions; // Fix suggestions
    size_t suggestion_count;
    char** related_locations;    // Related source locations
    size_t related_count;
} diagnostic_info_t;

// Diagnostic engine
typedef struct {
    ttl_error_context_t* error_ctx;
    diagnostic_flags_t flags;
    diagnostic_format_t format;
    FILE* output_stream;
    
    // Statistics
    size_t total_lines_processed;
    size_t errors_reported;
    size_t warnings_reported;
    size_t suggestions_made;
    
    // Configuration
    size_t context_lines;        // Lines of context to show
    size_t max_errors_per_line;  // Max errors to report per line
    bool stop_on_first_error;
    bool werror;                 // Treat warnings as errors
    
    // Filters
    error_type_t* suppressed_warnings;
    size_t suppressed_count;
    error_type_t* promoted_warnings; // Warnings to treat as errors
    size_t promoted_count;
} diagnostic_engine_t;

// Diagnostic engine management
diagnostic_engine_t* ttl_diagnostic_create(ttl_error_context_t* error_ctx);
void ttl_diagnostic_destroy(diagnostic_engine_t* engine);
void ttl_diagnostic_set_format(diagnostic_engine_t* engine, diagnostic_format_t format);
void ttl_diagnostic_set_flags(diagnostic_engine_t* engine, diagnostic_flags_t flags);
void ttl_diagnostic_set_output(diagnostic_engine_t* engine, FILE* stream);

// Error enhancement
diagnostic_info_t* ttl_diagnostic_enhance_error(diagnostic_engine_t* engine, ttl_error_t* error);
void ttl_diagnostic_info_destroy(diagnostic_info_t* info);

// Suggestion generation
void ttl_diagnostic_suggest_fix(diagnostic_info_t* info, const char* suggestion, const char* replacement);
void ttl_diagnostic_add_note(diagnostic_info_t* info, ttl_location_t loc, const char* format, ...);
void ttl_diagnostic_add_related_location(diagnostic_info_t* info, ttl_location_t loc, const char* description);

// Common diagnostic suggestions
void ttl_diagnostic_suggest_prefix(diagnostic_engine_t* engine, const char* undefined_prefix);
void ttl_diagnostic_suggest_token(diagnostic_engine_t* engine, ttl_token_type_t expected, ttl_token_type_t got);
void ttl_diagnostic_suggest_escape_sequence(diagnostic_engine_t* engine, const char* invalid_sequence);
void ttl_diagnostic_suggest_iri_fix(diagnostic_engine_t* engine, const char* invalid_iri);

// Output functions
void ttl_diagnostic_print_error(diagnostic_engine_t* engine, diagnostic_info_t* info);
void ttl_diagnostic_print_summary(diagnostic_engine_t* engine);
void ttl_diagnostic_print_statistics(diagnostic_engine_t* engine);

// Warning control
void ttl_diagnostic_suppress_warning(diagnostic_engine_t* engine, error_type_t type);
void ttl_diagnostic_promote_warning(diagnostic_engine_t* engine, error_type_t type);
void ttl_diagnostic_reset_warnings(diagnostic_engine_t* engine);

// Utility functions
bool ttl_diagnostic_should_report(diagnostic_engine_t* engine, error_type_t type, error_severity_t severity);
const char* ttl_diagnostic_format_location(ttl_location_t loc, diagnostic_format_t format);
void ttl_diagnostic_print_code_snippet(FILE* stream, const char* source, ttl_location_t loc, 
                                       size_t context_lines, bool use_color);

// Error recovery helpers
void ttl_diagnostic_recovery_hint(diagnostic_engine_t* engine, error_recovery_t strategy);
bool ttl_diagnostic_can_continue(diagnostic_engine_t* engine);

// Batch error reporting
typedef struct diagnostic_batch {
    diagnostic_info_t** diagnostics;
    size_t count;
    size_t capacity;
} diagnostic_batch_t;

diagnostic_batch_t* ttl_diagnostic_batch_create(void);
void ttl_diagnostic_batch_destroy(diagnostic_batch_t* batch);
void ttl_diagnostic_batch_add(diagnostic_batch_t* batch, diagnostic_info_t* info);
void ttl_diagnostic_batch_sort(diagnostic_batch_t* batch); // Sort by location
void ttl_diagnostic_batch_print(diagnostic_engine_t* engine, diagnostic_batch_t* batch);

#endif // TTL_DIAGNOSTIC_H