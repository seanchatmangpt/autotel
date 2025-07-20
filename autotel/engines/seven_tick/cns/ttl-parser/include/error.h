#ifndef TTL_ERROR_H
#define TTL_ERROR_H

#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include "location.h"
#include "parser.h"

// Error severity levels are defined in parser.h
// Using ErrorSeverity from parser.h to avoid conflicts
typedef ErrorSeverity error_severity_t;

// Error categories
typedef enum {
    ERROR_CATEGORY_LEXER,
    ERROR_CATEGORY_PARSER,
    ERROR_CATEGORY_SEMANTIC,
    ERROR_CATEGORY_INTERNAL
} error_category_t;

// Specific error types
typedef enum {
    // Lexer errors
    ERROR_INVALID_CHARACTER,
    ERROR_UNTERMINATED_STRING,
    ERROR_INVALID_ESCAPE_SEQUENCE,
    ERROR_INVALID_UNICODE_ESCAPE,
    ERROR_INVALID_IRI,
    ERROR_INVALID_LANGUAGE_TAG,
    ERROR_NUMBER_TOO_LARGE,
    ERROR_INVALID_NUMBER_FORMAT,
    
    // Parser errors
    ERROR_UNEXPECTED_TOKEN,
    ERROR_EXPECTED_TOKEN,
    ERROR_INVALID_SYNTAX,
    ERROR_DUPLICATE_PREFIX,
    ERROR_UNDEFINED_PREFIX,
    ERROR_INVALID_PREDICATE,
    ERROR_INVALID_SUBJECT,
    ERROR_INVALID_OBJECT,
    ERROR_MISSING_DOT,
    ERROR_MISSING_SEMICOLON,
    ERROR_INVALID_COLLECTION,
    ERROR_INVALID_BLANK_NODE,
    
    // Semantic errors
    ERROR_CIRCULAR_PREFIX_DEFINITION,
    ERROR_INVALID_BASE_IRI,
    ERROR_RESOURCE_NOT_FOUND,
    
    // Internal errors
    ERROR_OUT_OF_MEMORY,
    ERROR_IO_ERROR,
    ERROR_INTERNAL_ERROR
} error_type_t;

// Error structure
typedef struct ttl_error {
    error_type_t type;
    error_severity_t severity;
    error_category_t category;
    ttl_location_t location;
    char* message;
    char* suggestion;  // Optional fix suggestion
    char* source_line; // Copy of the source line containing the error
    struct ttl_error* next; // For error list
} ttl_error_t;

// Error context for collecting multiple errors
typedef struct ttl_error_context {
    ttl_error_t* errors;
    ttl_error_t* last_error;
    size_t error_count;
    size_t warning_count;
    size_t max_errors; // Maximum errors before stopping
    bool treat_warnings_as_errors;
    bool json_output;
    bool color_output;
    const char* source_name; // File name or "<stdin>"
    const char* source_content; // Full source content for snippets
} ttl_error_context_t;

// Error recovery strategy
typedef enum {
    RECOVERY_NONE,           // Stop on first error
    RECOVERY_SKIP_TOKEN,     // Skip current token and continue
    RECOVERY_SKIP_STATEMENT, // Skip to next statement
    RECOVERY_SYNC_DELIMITER, // Sync to next delimiter (., ;)
    RECOVERY_PANIC_MODE      // Panic mode recovery
} error_recovery_t;

// Function declarations

// Context management
ttl_error_context_t* ttl_error_context_create(void);
void ttl_error_context_destroy(ttl_error_context_t* ctx);
void ttl_error_context_set_source(ttl_error_context_t* ctx, const char* name, const char* content);
void ttl_error_context_set_max_errors(ttl_error_context_t* ctx, size_t max);
void ttl_error_context_set_json_output(ttl_error_context_t* ctx, bool json);
void ttl_error_context_set_color_output(ttl_error_context_t* ctx, bool color);

// Error reporting
void ttl_error_report(ttl_error_context_t* ctx, error_type_t type, 
                      error_severity_t severity, ttl_location_t loc,
                      const char* format, ...);
void ttl_error_report_with_suggestion(ttl_error_context_t* ctx, error_type_t type,
                                      error_severity_t severity, ttl_location_t loc,
                                      const char* message, const char* suggestion);

// Convenience functions
void ttl_error(ttl_error_context_t* ctx, ttl_location_t loc, const char* format, ...);
void ttl_warning(ttl_error_context_t* ctx, ttl_location_t loc, const char* format, ...);
void ttl_fatal(ttl_error_context_t* ctx, ttl_location_t loc, const char* format, ...);

// Error queries
bool ttl_error_context_has_errors(const ttl_error_context_t* ctx);
bool ttl_error_context_has_warnings(const ttl_error_context_t* ctx);
size_t ttl_error_context_error_count(const ttl_error_context_t* ctx);
size_t ttl_error_context_warning_count(const ttl_error_context_t* ctx);
const ttl_error_t* ttl_error_context_get_errors(const ttl_error_context_t* ctx);

// Error formatting
void ttl_error_print(const ttl_error_context_t* ctx, const ttl_error_t* error, FILE* stream);
void ttl_error_print_all(const ttl_error_context_t* ctx, FILE* stream);
void ttl_error_print_json(const ttl_error_context_t* ctx, FILE* stream);

// Error recovery
error_recovery_t ttl_error_get_recovery_strategy(error_type_t type);
bool ttl_error_is_recoverable(error_type_t type);

// Error message helpers
const char* ttl_error_type_to_string(error_type_t type);
const char* ttl_error_severity_to_string(error_severity_t severity);
const char* ttl_error_category_to_string(error_category_t category);

#endif // TTL_ERROR_H
