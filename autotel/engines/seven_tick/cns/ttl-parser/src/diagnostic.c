#include "../include/diagnostic.h"
#include "../include/token.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

// Create diagnostic engine
diagnostic_engine_t* ttl_diagnostic_create(ttl_error_context_t* error_ctx) {
    diagnostic_engine_t* engine = malloc(sizeof(diagnostic_engine_t));
    if (!engine) return NULL;
    
    engine->error_ctx = error_ctx;
    engine->flags = DIAG_FLAG_SHOW_COLUMN | DIAG_FLAG_SHOW_SOURCE | 
                   DIAG_FLAG_SHOW_SUGGESTIONS | DIAG_FLAG_COLOR;
    engine->format = DIAG_FORMAT_HUMAN;
    engine->output_stream = stderr;
    
    engine->total_lines_processed = 0;
    engine->errors_reported = 0;
    engine->warnings_reported = 0;
    engine->suggestions_made = 0;
    
    engine->context_lines = 2;
    engine->max_errors_per_line = 5;
    engine->stop_on_first_error = false;
    engine->werror = false;
    
    engine->suppressed_warnings = NULL;
    engine->suppressed_count = 0;
    engine->promoted_warnings = NULL;
    engine->promoted_count = 0;
    
    return engine;
}

// Destroy diagnostic engine
void ttl_diagnostic_destroy(diagnostic_engine_t* engine) {
    if (!engine) return;
    
    free(engine->suppressed_warnings);
    free(engine->promoted_warnings);
    free(engine);
}

// Configuration setters
void ttl_diagnostic_set_format(diagnostic_engine_t* engine, diagnostic_format_t format) {
    if (engine) engine->format = format;
}

void ttl_diagnostic_set_flags(diagnostic_engine_t* engine, diagnostic_flags_t flags) {
    if (engine) engine->flags = flags;
}

void ttl_diagnostic_set_output(diagnostic_engine_t* engine, FILE* stream) {
    if (engine && stream) engine->output_stream = stream;
}

// Create enhanced diagnostic info
diagnostic_info_t* ttl_diagnostic_enhance_error(diagnostic_engine_t* engine, ttl_error_t* error) {
    if (!error) return NULL;
    
    diagnostic_info_t* info = malloc(sizeof(diagnostic_info_t));
    if (!info) return NULL;
    
    info->error = error;
    info->notes = NULL;
    info->suggestions = NULL;
    info->suggestion_count = 0;
    info->related_locations = NULL;
    info->related_count = 0;
    
    // Generate automatic suggestions based on error type
    switch (error->type) {
        case ERROR_UNDEFINED_PREFIX:
            ttl_diagnostic_suggest_prefix(engine, error->message);
            break;
            
        case ERROR_INVALID_ESCAPE_SEQUENCE:
            ttl_diagnostic_suggest_escape_sequence(engine, error->message);
            break;
            
        case ERROR_INVALID_IRI:
            ttl_diagnostic_suggest_iri_fix(engine, error->message);
            break;
            
        case ERROR_MISSING_DOT:
            ttl_diagnostic_suggest_fix(info, "Add '.' at the end of the statement", ".");
            break;
            
        case ERROR_MISSING_SEMICOLON:
            ttl_diagnostic_suggest_fix(info, "Add ';' to separate predicates", ";");
            break;
            
        default:
            break;
    }
    
    return info;
}

// Destroy diagnostic info
void ttl_diagnostic_info_destroy(diagnostic_info_t* info) {
    if (!info) return;
    
    // Free notes
    diagnostic_note_t* note = info->notes;
    while (note) {
        diagnostic_note_t* next = note->next;
        free(note->message);
        free(note);
        note = next;
    }
    
    // Free suggestions
    for (size_t i = 0; i < info->suggestion_count; i++) {
        free(info->suggestions[i].text);
        free(info->suggestions[i].replacement);
    }
    free(info->suggestions);
    
    // Free related locations
    for (size_t i = 0; i < info->related_count; i++) {
        free(info->related_locations[i]);
    }
    free(info->related_locations);
    
    free(info);
}

// Add suggestion
void ttl_diagnostic_suggest_fix(diagnostic_info_t* info, const char* suggestion, const char* replacement) {
    if (!info || !suggestion) return;
    
    size_t new_count = info->suggestion_count + 1;
    diagnostic_suggestion_t* new_suggestions = realloc(info->suggestions, 
                                                      new_count * sizeof(diagnostic_suggestion_t));
    if (!new_suggestions) return;
    
    info->suggestions = new_suggestions;
    info->suggestions[info->suggestion_count].text = strdup(suggestion);
    info->suggestions[info->suggestion_count].replacement = replacement ? strdup(replacement) : NULL;
    info->suggestions[info->suggestion_count].location = info->error->location;
    info->suggestion_count++;
}

// Add note
void ttl_diagnostic_add_note(diagnostic_info_t* info, ttl_location_t loc, const char* format, ...) {
    if (!info || !format) return;
    
    diagnostic_note_t* note = malloc(sizeof(diagnostic_note_t));
    if (!note) return;
    
    note->location = loc;
    note->next = info->notes;
    
    // Format message
    va_list args;
    va_start(args, format);
    
    va_list args_copy;
    va_copy(args_copy, args);
    int size = vsnprintf(NULL, 0, format, args_copy);
    va_end(args_copy);
    
    note->message = malloc(size + 1);
    if (note->message) {
        vsnprintf(note->message, size + 1, format, args);
    }
    va_end(args);
    
    info->notes = note;
}

// Common suggestion generators
void ttl_diagnostic_suggest_prefix(diagnostic_engine_t* engine, const char* undefined_prefix) {
    // This would look up common prefixes and suggest similar ones
    // For now, just a placeholder
    (void)engine;
    (void)undefined_prefix;
}

void ttl_diagnostic_suggest_token(diagnostic_engine_t* engine, ttl_token_type_t expected, ttl_token_type_t got) {
    // Generate suggestion based on expected vs actual token
    (void)engine;
    (void)expected;
    (void)got;
}

void ttl_diagnostic_suggest_escape_sequence(diagnostic_engine_t* engine, const char* invalid_sequence) {
    // Suggest valid escape sequences
    (void)engine;
    (void)invalid_sequence;
}

void ttl_diagnostic_suggest_iri_fix(diagnostic_engine_t* engine, const char* invalid_iri) {
    // Suggest IRI fixes (e.g., missing angle brackets, invalid characters)
    (void)engine;
    (void)invalid_iri;
}

// Format location based on output format
const char* ttl_diagnostic_format_location(ttl_location_t loc, diagnostic_format_t format) {
    static char buffer[256];
    
    switch (format) {
        case DIAG_FORMAT_GCC:
            snprintf(buffer, sizeof(buffer), "%zu:%zu", loc.line, loc.column);
            break;
            
        case DIAG_FORMAT_MSVC:
            snprintf(buffer, sizeof(buffer), "(%zu,%zu)", loc.line, loc.column);
            break;
            
        default:
            snprintf(buffer, sizeof(buffer), "%zu:%zu", loc.line, loc.column);
            break;
    }
    
    return buffer;
}

// Print code snippet with context
void ttl_diagnostic_print_code_snippet(FILE* stream, const char* source, ttl_location_t loc,
                                       size_t context_lines, bool use_color) {
    if (!stream || !source) return;
    
    // Find line boundaries
    size_t start_line = (loc.line > context_lines) ? loc.line - context_lines : 1;
    size_t end_line = loc.line + context_lines;
    
    // Find start of first line to print
    const char* line_start = source;
    for (size_t i = 1; i < start_line && *line_start; i++) {
        while (*line_start && *line_start != '\n') line_start++;
        if (*line_start == '\n') line_start++;
    }
    
    // Print lines with context
    for (size_t line_num = start_line; line_num <= end_line && *line_start; line_num++) {
        // Find end of current line
        const char* line_end = line_start;
        while (*line_end && *line_end != '\n') line_end++;
        
        // Print line number
        if (line_num == loc.line) {
            fprintf(stream, "%s%5zu |>%s ", 
                    use_color ? "\033[1m" : "",
                    line_num,
                    use_color ? "\033[0m" : "");
        } else {
            fprintf(stream, "%s%5zu | %s",
                    use_color ? "\033[2m" : "",
                    line_num,
                    use_color ? "\033[0m" : "");
        }
        
        // Print line content
        fwrite(line_start, 1, line_end - line_start, stream);
        fprintf(stream, "\n");
        
        // Print error indicator on error line
        if (line_num == loc.line) {
            fprintf(stream, "      | ");
            for (size_t i = 1; i < loc.column; i++) {
                fputc(' ', stream);
            }
            
            if (use_color) fprintf(stream, "\033[31m");
            fputc('^', stream);
            for (size_t i = 1; i < loc.length; i++) {
                fputc('~', stream);
            }
            if (use_color) fprintf(stream, "\033[0m");
            fprintf(stream, "\n");
        }
        
        // Move to next line
        line_start = line_end;
        if (*line_start == '\n') line_start++;
    }
}

// Print enhanced error
void ttl_diagnostic_print_error(diagnostic_engine_t* engine, diagnostic_info_t* info) {
    if (!engine || !info || !info->error) return;
    
    FILE* stream = engine->output_stream;
    bool use_color = (engine->flags & DIAG_FLAG_COLOR) != 0;
    
    // Update statistics
    if (info->error->severity == ERROR_SEVERITY_WARNING) {
        engine->warnings_reported++;
    } else {
        engine->errors_reported++;
    }
    
    // Print based on format
    switch (engine->format) {
        case DIAG_FORMAT_JSON:
            ttl_error_print_json(engine->error_ctx, stream);
            break;
            
        case DIAG_FORMAT_COMPACT:
            fprintf(stream, "%s:%s: %s: %s\n",
                    engine->error_ctx->source_name,
                    ttl_diagnostic_format_location(info->error->location, engine->format),
                    ttl_error_severity_to_string(info->error->severity),
                    info->error->message);
            break;
            
        case DIAG_FORMAT_HUMAN:
        default:
            // Print main error
            ttl_error_print(engine->error_ctx, info->error, stream);
            
            // Print additional notes
            for (diagnostic_note_t* note = info->notes; note; note = note->next) {
                fprintf(stream, "%s      = note:%s %s\n",
                        use_color ? "\033[36m" : "",
                        use_color ? "\033[0m" : "",
                        note->message);
            }
            
            // Print suggestions
            if ((engine->flags & DIAG_FLAG_SHOW_SUGGESTIONS) && info->suggestion_count > 0) {
                for (size_t i = 0; i < info->suggestion_count; i++) {
                    fprintf(stream, "%s      = help:%s %s\n",
                            use_color ? "\033[32m" : "",
                            use_color ? "\033[0m" : "",
                            info->suggestions[i].text);
                    
                    if (info->suggestions[i].replacement) {
                        fprintf(stream, "        suggested fix: %s\n", 
                                info->suggestions[i].replacement);
                    }
                }
                engine->suggestions_made += info->suggestion_count;
            }
            break;
    }
}

// Print diagnostic summary
void ttl_diagnostic_print_summary(diagnostic_engine_t* engine) {
    if (!engine) return;
    
    FILE* stream = engine->output_stream;
    bool use_color = (engine->flags & DIAG_FLAG_COLOR) != 0;
    
    if (engine->errors_reported > 0 || engine->warnings_reported > 0) {
        fprintf(stream, "\n");
        
        if (engine->errors_reported > 0) {
            fprintf(stream, "%s%zu error%s%s",
                    use_color ? "\033[31m" : "",
                    engine->errors_reported,
                    engine->errors_reported == 1 ? "" : "s",
                    use_color ? "\033[0m" : "");
        }
        
        if (engine->errors_reported > 0 && engine->warnings_reported > 0) {
            fprintf(stream, " and ");
        }
        
        if (engine->warnings_reported > 0) {
            fprintf(stream, "%s%zu warning%s%s",
                    use_color ? "\033[33m" : "",
                    engine->warnings_reported,
                    engine->warnings_reported == 1 ? "" : "s",
                    use_color ? "\033[0m" : "");
        }
        
        fprintf(stream, " generated");
        
        if (engine->suggestions_made > 0) {
            fprintf(stream, " (%zu suggestion%s provided)",
                    engine->suggestions_made,
                    engine->suggestions_made == 1 ? "" : "s");
        }
        
        fprintf(stream, ".\n");
    }
}

// Print statistics
void ttl_diagnostic_print_statistics(diagnostic_engine_t* engine) {
    if (!engine || !(engine->flags & DIAG_FLAG_VERBOSE)) return;
    
    FILE* stream = engine->output_stream;
    
    fprintf(stream, "\nDiagnostic Statistics:\n");
    fprintf(stream, "  Lines processed: %zu\n", engine->total_lines_processed);
    fprintf(stream, "  Errors reported: %zu\n", engine->errors_reported);
    fprintf(stream, "  Warnings reported: %zu\n", engine->warnings_reported);
    fprintf(stream, "  Suggestions made: %zu\n", engine->suggestions_made);
    
    if (engine->suppressed_count > 0) {
        fprintf(stream, "  Warnings suppressed: %zu\n", engine->suppressed_count);
    }
    if (engine->promoted_count > 0) {
        fprintf(stream, "  Warnings promoted: %zu\n", engine->promoted_count);
    }
}

// Warning control
void ttl_diagnostic_suppress_warning(diagnostic_engine_t* engine, error_type_t type) {
    if (!engine) return;
    
    size_t new_count = engine->suppressed_count + 1;
    error_type_t* new_list = realloc(engine->suppressed_warnings, 
                                     new_count * sizeof(error_type_t));
    if (!new_list) return;
    
    engine->suppressed_warnings = new_list;
    engine->suppressed_warnings[engine->suppressed_count] = type;
    engine->suppressed_count++;
}

void ttl_diagnostic_promote_warning(diagnostic_engine_t* engine, error_type_t type) {
    if (!engine) return;
    
    size_t new_count = engine->promoted_count + 1;
    error_type_t* new_list = realloc(engine->promoted_warnings,
                                     new_count * sizeof(error_type_t));
    if (!new_list) return;
    
    engine->promoted_warnings = new_list;
    engine->promoted_warnings[engine->promoted_count] = type;
    engine->promoted_count++;
}

// Check if should report
bool ttl_diagnostic_should_report(diagnostic_engine_t* engine, error_type_t type, error_severity_t severity) {
    if (!engine) return true;
    
    // Check if warning is suppressed
    if (severity == ERROR_SEVERITY_WARNING) {
        for (size_t i = 0; i < engine->suppressed_count; i++) {
            if (engine->suppressed_warnings[i] == type) {
                return false;
            }
        }
    }
    
    // Check if warning should be promoted
    for (size_t i = 0; i < engine->promoted_count; i++) {
        if (engine->promoted_warnings[i] == type) {
            severity = ERROR_SEVERITY_ERROR;
            break;
        }
    }
    
    // Check if warnings are errors
    if (engine->werror && severity == ERROR_SEVERITY_WARNING) {
        severity = ERROR_SEVERITY_ERROR;
    }
    
    return true;
}

// Recovery hint
void ttl_diagnostic_recovery_hint(diagnostic_engine_t* engine, error_recovery_t strategy) {
    if (!engine || !(engine->flags & DIAG_FLAG_VERBOSE)) return;
    
    FILE* stream = engine->output_stream;
    
    fprintf(stream, "      = recovery: ");
    switch (strategy) {
        case RECOVERY_SKIP_TOKEN:
            fprintf(stream, "skipping invalid token\n");
            break;
        case RECOVERY_SKIP_STATEMENT:
            fprintf(stream, "skipping to next statement\n");
            break;
        case RECOVERY_SYNC_DELIMITER:
            fprintf(stream, "synchronizing to next delimiter\n");
            break;
        case RECOVERY_PANIC_MODE:
            fprintf(stream, "entering panic mode recovery\n");
            break;
        default:
            fprintf(stream, "stopping parse\n");
            break;
    }
}

// Check if can continue
bool ttl_diagnostic_can_continue(diagnostic_engine_t* engine) {
    if (!engine) return false;
    
    if (engine->stop_on_first_error && engine->errors_reported > 0) {
        return false;
    }
    
    if (engine->error_ctx && 
        engine->error_ctx->error_count >= engine->error_ctx->max_errors) {
        return false;
    }
    
    return true;
}

// Batch operations
diagnostic_batch_t* ttl_diagnostic_batch_create(void) {
    diagnostic_batch_t* batch = malloc(sizeof(diagnostic_batch_t));
    if (!batch) return NULL;
    
    batch->diagnostics = NULL;
    batch->count = 0;
    batch->capacity = 0;
    
    return batch;
}

void ttl_diagnostic_batch_destroy(diagnostic_batch_t* batch) {
    if (!batch) return;
    
    for (size_t i = 0; i < batch->count; i++) {
        ttl_diagnostic_info_destroy(batch->diagnostics[i]);
    }
    free(batch->diagnostics);
    free(batch);
}

void ttl_diagnostic_batch_add(diagnostic_batch_t* batch, diagnostic_info_t* info) {
    if (!batch || !info) return;
    
    if (batch->count >= batch->capacity) {
        size_t new_capacity = batch->capacity ? batch->capacity * 2 : 8;
        diagnostic_info_t** new_diagnostics = realloc(batch->diagnostics,
                                                     new_capacity * sizeof(diagnostic_info_t*));
        if (!new_diagnostics) return;
        
        batch->diagnostics = new_diagnostics;
        batch->capacity = new_capacity;
    }
    
    batch->diagnostics[batch->count++] = info;
}

// Compare diagnostics by location for sorting
static int diagnostic_compare(const void* a, const void* b) {
    const diagnostic_info_t* da = *(const diagnostic_info_t**)a;
    const diagnostic_info_t* db = *(const diagnostic_info_t**)b;
    
    if (da->error->location.line != db->error->location.line) {
        return da->error->location.line - db->error->location.line;
    }
    return da->error->location.column - db->error->location.column;
}

void ttl_diagnostic_batch_sort(diagnostic_batch_t* batch) {
    if (!batch || batch->count < 2) return;
    
    qsort(batch->diagnostics, batch->count, sizeof(diagnostic_info_t*), diagnostic_compare);
}

void ttl_diagnostic_batch_print(diagnostic_engine_t* engine, diagnostic_batch_t* batch) {
    if (!engine || !batch) return;
    
    // Sort by location
    ttl_diagnostic_batch_sort(batch);
    
    // Print all diagnostics
    for (size_t i = 0; i < batch->count; i++) {
        ttl_diagnostic_print_error(engine, batch->diagnostics[i]);
    }
    
    // Print summary
    ttl_diagnostic_print_summary(engine);
}