#include "../include/error.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

// ANSI color codes
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_BOLD    "\033[1m"
#define COLOR_DIM     "\033[2m"

// Create error context
ttl_error_context_t* ttl_error_context_create(void) {
    ttl_error_context_t* ctx = malloc(sizeof(ttl_error_context_t));
    if (!ctx) return NULL;
    
    ctx->errors = NULL;
    ctx->last_error = NULL;
    ctx->error_count = 0;
    ctx->warning_count = 0;
    ctx->max_errors = 100; // Default max errors
    ctx->treat_warnings_as_errors = false;
    ctx->json_output = false;
    ctx->color_output = true; // Default to color if terminal supports it
    ctx->source_name = "<unknown>";
    ctx->source_content = NULL;
    
    return ctx;
}

// Destroy error context
void ttl_error_context_destroy(ttl_error_context_t* ctx) {
    if (!ctx) return;
    
    ttl_error_t* error = ctx->errors;
    while (error) {
        ttl_error_t* next = error->next;
        free(error->message);
        free(error->suggestion);
        free(error->source_line);
        free(error);
        error = next;
    }
    
    free(ctx);
}

// Set source information
void ttl_error_context_set_source(ttl_error_context_t* ctx, const char* name, const char* content) {
    if (!ctx) return;
    ctx->source_name = name ? name : "<unknown>";
    ctx->source_content = content;
}

// Configuration setters
void ttl_error_context_set_max_errors(ttl_error_context_t* ctx, size_t max) {
    if (ctx) ctx->max_errors = max;
}

void ttl_error_context_set_json_output(ttl_error_context_t* ctx, bool json) {
    if (ctx) ctx->json_output = json;
}

void ttl_error_context_set_color_output(ttl_error_context_t* ctx, bool color) {
    if (ctx) ctx->color_output = color;
}

// Extract source line
static char* extract_source_line(const char* source, ttl_location_t loc) {
    if (!source) return NULL;
    
    // Find start of line
    const char* line_start = source;
    for (size_t i = 1; i < loc.line && *line_start; i++) {
        while (*line_start && *line_start != '\n') line_start++;
        if (*line_start == '\n') line_start++;
    }
    
    // Find end of line
    const char* line_end = line_start;
    while (*line_end && *line_end != '\n') line_end++;
    
    // Copy line
    size_t len = line_end - line_start;
    char* line = malloc(len + 1);
    if (!line) return NULL;
    
    memcpy(line, line_start, len);
    line[len] = '\0';
    
    return line;
}

// Add error to context
static void add_error(ttl_error_context_t* ctx, ttl_error_t* error) {
    if (!ctx || !error) return;
    
    if (error->severity == ERROR_SEVERITY_WARNING) {
        ctx->warning_count++;
    } else {
        ctx->error_count++;
    }
    
    error->next = NULL;
    if (ctx->last_error) {
        ctx->last_error->next = error;
    } else {
        ctx->errors = error;
    }
    ctx->last_error = error;
}

// Report error with format
void ttl_error_report(ttl_error_context_t* ctx, error_type_t type,
                      error_severity_t severity, ttl_location_t loc,
                      const char* format, ...) {
    if (!ctx || ctx->error_count >= ctx->max_errors) return;
    
    // Create error
    ttl_error_t* error = malloc(sizeof(ttl_error_t));
    if (!error) return;
    
    error->type = type;
    error->severity = severity;
    error->location = loc;
    error->suggestion = NULL;
    error->next = NULL;
    
    // Determine category from type
    if (type <= ERROR_INVALID_NUMBER_FORMAT) {
        error->category = ERROR_CATEGORY_LEXER;
    } else if (type <= ERROR_INVALID_BLANK_NODE) {
        error->category = ERROR_CATEGORY_PARSER;
    } else if (type <= ERROR_RESOURCE_NOT_FOUND) {
        error->category = ERROR_CATEGORY_SEMANTIC;
    } else {
        error->category = ERROR_CATEGORY_INTERNAL;
    }
    
    // Format message
    va_list args;
    va_start(args, format);
    
    // Calculate message size
    va_list args_copy;
    va_copy(args_copy, args);
    int size = vsnprintf(NULL, 0, format, args_copy);
    va_end(args_copy);
    
    error->message = malloc(size + 1);
    if (error->message) {
        vsnprintf(error->message, size + 1, format, args);
    }
    va_end(args);
    
    // Extract source line
    error->source_line = extract_source_line(ctx->source_content, loc);
    
    add_error(ctx, error);
}

// Report error with suggestion
void ttl_error_report_with_suggestion(ttl_error_context_t* ctx, error_type_t type,
                                      error_severity_t severity, ttl_location_t loc,
                                      const char* message, const char* suggestion) {
    if (!ctx || ctx->error_count >= ctx->max_errors) return;
    
    ttl_error_t* error = malloc(sizeof(ttl_error_t));
    if (!error) return;
    
    error->type = type;
    error->severity = severity;
    error->location = loc;
    error->message = message ? strdup(message) : NULL;
    error->suggestion = suggestion ? strdup(suggestion) : NULL;
    error->next = NULL;
    
    // Determine category
    if (type <= ERROR_INVALID_NUMBER_FORMAT) {
        error->category = ERROR_CATEGORY_LEXER;
    } else if (type <= ERROR_INVALID_BLANK_NODE) {
        error->category = ERROR_CATEGORY_PARSER;
    } else if (type <= ERROR_RESOURCE_NOT_FOUND) {
        error->category = ERROR_CATEGORY_SEMANTIC;
    } else {
        error->category = ERROR_CATEGORY_INTERNAL;
    }
    
    error->source_line = extract_source_line(ctx->source_content, loc);
    add_error(ctx, error);
}

// Convenience functions
void ttl_error(ttl_error_context_t* ctx, ttl_location_t loc, const char* format, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    ttl_error_report(ctx, ERROR_INVALID_SYNTAX, ERROR_SEVERITY_ERROR, loc, "%s", buffer);
}

void ttl_warning(ttl_error_context_t* ctx, ttl_location_t loc, const char* format, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    ttl_error_report(ctx, ERROR_INVALID_SYNTAX, ERROR_SEVERITY_WARNING, loc, "%s", buffer);
}

void ttl_fatal(ttl_error_context_t* ctx, ttl_location_t loc, const char* format, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    ttl_error_report(ctx, ERROR_INTERNAL_ERROR, ERROR_SEVERITY_FATAL, loc, "%s", buffer);
}

// Error queries
bool ttl_error_context_has_errors(const ttl_error_context_t* ctx) {
    return ctx && ctx->error_count > 0;
}

bool ttl_error_context_has_warnings(const ttl_error_context_t* ctx) {
    return ctx && ctx->warning_count > 0;
}

size_t ttl_error_context_error_count(const ttl_error_context_t* ctx) {
    return ctx ? ctx->error_count : 0;
}

size_t ttl_error_context_warning_count(const ttl_error_context_t* ctx) {
    return ctx ? ctx->warning_count : 0;
}

const ttl_error_t* ttl_error_context_get_errors(const ttl_error_context_t* ctx) {
    return ctx ? ctx->errors : NULL;
}

// Print error with source snippet
void ttl_error_print(const ttl_error_context_t* ctx, const ttl_error_t* error, FILE* stream) {
    if (!ctx || !error || !stream) return;
    
    const char* severity_color = "";
    const char* severity_text = "";
    
    if (ctx->color_output) {
        switch (error->severity) {
            case ERROR_SEVERITY_WARNING:
                severity_color = COLOR_YELLOW;
                break;
            case ERROR_SEVERITY_ERROR:
            case ERROR_SEVERITY_FATAL:
                severity_color = COLOR_RED;
                break;
        }
    }
    
    switch (error->severity) {
        case ERROR_SEVERITY_WARNING:
            severity_text = "warning";
            break;
        case ERROR_SEVERITY_ERROR:
            severity_text = "error";
            break;
        case ERROR_SEVERITY_FATAL:
            severity_text = "fatal error";
            break;
    }
    
    // Print error header
    fprintf(stream, "%s%s%s:%zu:%zu: %s%s%s: %s%s%s\n",
            ctx->color_output ? COLOR_BOLD : "",
            ctx->source_name,
            ctx->color_output ? COLOR_RESET : "",
            error->location.line,
            error->location.column,
            severity_color,
            severity_text,
            ctx->color_output ? COLOR_RESET : "",
            error->message ? error->message : "unknown error",
            ctx->color_output ? COLOR_RESET : "",
            "");
    
    // Print source line with error indicator
    if (error->source_line) {
        // Print line number
        fprintf(stream, "%s%5zu | %s", 
                ctx->color_output ? COLOR_DIM : "",
                error->location.line,
                ctx->color_output ? COLOR_RESET : "");
        
        // Print source line
        fprintf(stream, "%s\n", error->source_line);
        
        // Print error indicator
        fprintf(stream, "%s      | %s", 
                ctx->color_output ? COLOR_DIM : "",
                ctx->color_output ? COLOR_RESET : "");
        
        // Print spaces up to error column
        for (size_t i = 1; i < error->location.column; i++) {
            fputc(' ', stream);
        }
        
        // Print caret
        fprintf(stream, "%s^", severity_color);
        
        // Print squiggly line for multi-character errors
        if (error->location.length > 1) {
            for (size_t i = 1; i < error->location.length; i++) {
                fputc('~', stream);
            }
        }
        
        fprintf(stream, "%s\n", ctx->color_output ? COLOR_RESET : "");
    }
    
    // Print suggestion if available
    if (error->suggestion) {
        fprintf(stream, "%s      | %snote:%s %s\n",
                ctx->color_output ? COLOR_DIM : "",
                ctx->color_output ? COLOR_BLUE : "",
                ctx->color_output ? COLOR_RESET : "",
                error->suggestion);
    }
}

// Print all errors
void ttl_error_print_all(const ttl_error_context_t* ctx, FILE* stream) {
    if (!ctx || !stream) return;
    
    for (const ttl_error_t* error = ctx->errors; error; error = error->next) {
        ttl_error_print(ctx, error, stream);
    }
    
    // Print summary
    if (ctx->error_count > 0 || ctx->warning_count > 0) {
        fprintf(stream, "\n");
        if (ctx->error_count > 0) {
            fprintf(stream, "%s%zu error%s%s",
                    ctx->color_output ? COLOR_RED : "",
                    ctx->error_count,
                    ctx->error_count == 1 ? "" : "s",
                    ctx->color_output ? COLOR_RESET : "");
        }
        if (ctx->error_count > 0 && ctx->warning_count > 0) {
            fprintf(stream, " and ");
        }
        if (ctx->warning_count > 0) {
            fprintf(stream, "%s%zu warning%s%s",
                    ctx->color_output ? COLOR_YELLOW : "",
                    ctx->warning_count,
                    ctx->warning_count == 1 ? "" : "s",
                    ctx->color_output ? COLOR_RESET : "");
        }
        fprintf(stream, " generated.\n");
    }
}

// Print errors as JSON
void ttl_error_print_json(const ttl_error_context_t* ctx, FILE* stream) {
    if (!ctx || !stream) return;
    
    fprintf(stream, "{\n");
    fprintf(stream, "  \"source\": \"%s\",\n", ctx->source_name);
    fprintf(stream, "  \"error_count\": %zu,\n", ctx->error_count);
    fprintf(stream, "  \"warning_count\": %zu,\n", ctx->warning_count);
    fprintf(stream, "  \"errors\": [\n");
    
    bool first = true;
    for (const ttl_error_t* error = ctx->errors; error; error = error->next) {
        if (!first) fprintf(stream, ",\n");
        first = false;
        
        fprintf(stream, "    {\n");
        fprintf(stream, "      \"type\": \"%s\",\n", ttl_error_type_to_string(error->type));
        fprintf(stream, "      \"severity\": \"%s\",\n", ttl_error_severity_to_string(error->severity));
        fprintf(stream, "      \"category\": \"%s\",\n", ttl_error_category_to_string(error->category));
        fprintf(stream, "      \"location\": {\n");
        fprintf(stream, "        \"line\": %zu,\n", error->location.line);
        fprintf(stream, "        \"column\": %zu,\n", error->location.column);
        fprintf(stream, "        \"length\": %zu\n", error->location.length);
        fprintf(stream, "      },\n");
        fprintf(stream, "      \"message\": \"%s\"", error->message ? error->message : "");
        
        if (error->suggestion) {
            fprintf(stream, ",\n      \"suggestion\": \"%s\"", error->suggestion);
        }
        if (error->source_line) {
            fprintf(stream, ",\n      \"source_line\": \"");
            // Escape JSON string
            for (const char* p = error->source_line; *p; p++) {
                switch (*p) {
                    case '"': fprintf(stream, "\\\""); break;
                    case '\\': fprintf(stream, "\\\\"); break;
                    case '\n': fprintf(stream, "\\n"); break;
                    case '\r': fprintf(stream, "\\r"); break;
                    case '\t': fprintf(stream, "\\t"); break;
                    default: fputc(*p, stream);
                }
            }
            fprintf(stream, "\"");
        }
        
        fprintf(stream, "\n    }");
    }
    
    fprintf(stream, "\n  ]\n");
    fprintf(stream, "}\n");
}

// Get recovery strategy for error type
error_recovery_t ttl_error_get_recovery_strategy(error_type_t type) {
    switch (type) {
        case ERROR_INVALID_CHARACTER:
        case ERROR_INVALID_ESCAPE_SEQUENCE:
        case ERROR_INVALID_UNICODE_ESCAPE:
            return RECOVERY_SKIP_TOKEN;
            
        case ERROR_UNTERMINATED_STRING:
        case ERROR_INVALID_IRI:
            return RECOVERY_SKIP_TOKEN;
            
        case ERROR_UNEXPECTED_TOKEN:
        case ERROR_EXPECTED_TOKEN:
            return RECOVERY_SYNC_DELIMITER;
            
        case ERROR_MISSING_DOT:
        case ERROR_MISSING_SEMICOLON:
            return RECOVERY_SKIP_STATEMENT;
            
        case ERROR_INVALID_SYNTAX:
        case ERROR_INVALID_PREDICATE:
        case ERROR_INVALID_SUBJECT:
        case ERROR_INVALID_OBJECT:
            return RECOVERY_SYNC_DELIMITER;
            
        case ERROR_OUT_OF_MEMORY:
        case ERROR_INTERNAL_ERROR:
            return RECOVERY_NONE;
            
        default:
            return RECOVERY_SKIP_TOKEN;
    }
}

// Check if error is recoverable
bool ttl_error_is_recoverable(error_type_t type) {
    return ttl_error_get_recovery_strategy(type) != RECOVERY_NONE;
}

// Error type to string
const char* ttl_error_type_to_string(error_type_t type) {
    switch (type) {
        case ERROR_INVALID_CHARACTER: return "invalid_character";
        case ERROR_UNTERMINATED_STRING: return "unterminated_string";
        case ERROR_INVALID_ESCAPE_SEQUENCE: return "invalid_escape_sequence";
        case ERROR_INVALID_UNICODE_ESCAPE: return "invalid_unicode_escape";
        case ERROR_INVALID_IRI: return "invalid_iri";
        case ERROR_INVALID_LANGUAGE_TAG: return "invalid_language_tag";
        case ERROR_NUMBER_TOO_LARGE: return "number_too_large";
        case ERROR_INVALID_NUMBER_FORMAT: return "invalid_number_format";
        case ERROR_UNEXPECTED_TOKEN: return "unexpected_token";
        case ERROR_EXPECTED_TOKEN: return "expected_token";
        case ERROR_INVALID_SYNTAX: return "invalid_syntax";
        case ERROR_DUPLICATE_PREFIX: return "duplicate_prefix";
        case ERROR_UNDEFINED_PREFIX: return "undefined_prefix";
        case ERROR_INVALID_PREDICATE: return "invalid_predicate";
        case ERROR_INVALID_SUBJECT: return "invalid_subject";
        case ERROR_INVALID_OBJECT: return "invalid_object";
        case ERROR_MISSING_DOT: return "missing_dot";
        case ERROR_MISSING_SEMICOLON: return "missing_semicolon";
        case ERROR_INVALID_COLLECTION: return "invalid_collection";
        case ERROR_INVALID_BLANK_NODE: return "invalid_blank_node";
        case ERROR_CIRCULAR_PREFIX_DEFINITION: return "circular_prefix_definition";
        case ERROR_INVALID_BASE_IRI: return "invalid_base_iri";
        case ERROR_RESOURCE_NOT_FOUND: return "resource_not_found";
        case ERROR_OUT_OF_MEMORY: return "out_of_memory";
        case ERROR_IO_ERROR: return "io_error";
        case ERROR_INTERNAL_ERROR: return "internal_error";
        default: return "unknown_error";
    }
}

// Error severity to string
const char* ttl_error_severity_to_string(error_severity_t severity) {
    switch (severity) {
        case ERROR_SEVERITY_WARNING: return "warning";
        case ERROR_SEVERITY_ERROR: return "error";
        case ERROR_SEVERITY_FATAL: return "fatal";
        default: return "unknown";
    }
}

// Error category to string
const char* ttl_error_category_to_string(error_category_t category) {
    switch (category) {
        case ERROR_CATEGORY_LEXER: return "lexer";
        case ERROR_CATEGORY_PARSER: return "parser";
        case ERROR_CATEGORY_SEMANTIC: return "semantic";
        case ERROR_CATEGORY_INTERNAL: return "internal";
        default: return "unknown";
    }
}