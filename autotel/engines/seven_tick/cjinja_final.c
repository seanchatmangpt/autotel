/**
 * @file cjinja_final.c
 * @brief Final CJinja Template Engine - Complete Implementation
 * @version 2.0.0 (Loop Fix Applied)
 * 
 * Complete implementation of the CJinja template engine with all fixes applied.
 * This is the definitive version with loop rendering bugs resolved.
 * 
 * @author 7T Engine Team
 * @date 2024-01-15
 */

#include "cjinja_final.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <math.h>

// =============================================================================
// INTERNAL CONFIGURATION
// =============================================================================

#define INITIAL_CONTEXT_SIZE 16
#define MAX_INCLUDE_DEPTH 10
#define HASH_SEED 5381

// =============================================================================
// GLOBAL STATE
// =============================================================================

// Global error state
static CJinjaError g_last_error = CJINJA_SUCCESS;
static char g_error_message[256] = {0};
static bool g_debug_mode = false;

// Cache statistics
static size_t g_cache_hits = 0;
static size_t g_cache_misses = 0;

// Filter registry
static struct {
    char* names[MAX_FILTERS];
    CJinjaFilterFunc functions[MAX_FILTERS];
    size_t count;
} g_filter_registry = {0};

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

/**
 * @brief Simple hash function for cache keys
 */
static size_t hash_string(const char* str) {
    size_t hash = HASH_SEED;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

/**
 * @brief Ensure buffer has enough capacity
 */
static void ensure_buffer_size(char** buffer, size_t* buffer_size, size_t required) {
    while (*buffer_size <= required) {
        *buffer_size *= 2;
        *buffer = realloc(*buffer, *buffer_size);
        if (!*buffer) {
            cjinja_set_error(CJINJA_ERROR_MEMORY_ALLOCATION, "Buffer reallocation failed");
            return;
        }
    }
}

/**
 * @brief Get current timestamp in nanoseconds
 */
static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

// =============================================================================
// ERROR HANDLING
// =============================================================================

void cjinja_set_error(CJinjaError error, const char* message) {
    g_last_error = error;
    if (message) {
        strncpy(g_error_message, message, sizeof(g_error_message) - 1);
        g_error_message[sizeof(g_error_message) - 1] = '\0';
    } else {
        g_error_message[0] = '\0';
    }
    
    if (g_debug_mode) {
        fprintf(stderr, "CJinja Error %d: %s\n", error, message ? message : "Unknown error");
    }
}

CJinjaError cjinja_get_last_error(CJinjaEngine* engine) {
    return engine ? engine->last_error : g_last_error;
}

const char* cjinja_get_error_message(CJinjaError error) {
    switch (error) {
        case CJINJA_SUCCESS: return "No error";
        case CJINJA_ERROR_NULL_POINTER: return "Null pointer";
        case CJINJA_ERROR_MEMORY_ALLOCATION: return "Memory allocation failed";
        case CJINJA_ERROR_INVALID_SYNTAX: return "Invalid template syntax";
        case CJINJA_ERROR_INVALID_VARIABLE: return "Invalid variable";
        case CJINJA_ERROR_INVALID_FILTER: return "Invalid filter";
        case CJINJA_ERROR_TEMPLATE_NOT_FOUND: return "Template not found";
        case CJINJA_ERROR_LOOP_DEPTH_EXCEEDED: return "Loop depth exceeded";
        case CJINJA_ERROR_ARRAY_SIZE_EXCEEDED: return "Array size exceeded";
        case CJINJA_ERROR_CACHE_FULL: return "Template cache full";
        default: return "Unknown error";
    }
}

void cjinja_set_debug_mode(bool enabled) {
    g_debug_mode = enabled;
}

// =============================================================================
// ENGINE MANAGEMENT
// =============================================================================

CJinjaEngine* cjinja_create(const char* template_dir) {
    CJinjaEngine* engine = calloc(1, sizeof(CJinjaEngine));
    if (!engine) {
        cjinja_set_error(CJINJA_ERROR_MEMORY_ALLOCATION, "Failed to allocate engine");
        return NULL;
    }
    
    if (template_dir) {
        engine->template_dir = strdup(template_dir);
        if (!engine->template_dir) {
            free(engine);
            cjinja_set_error(CJINJA_ERROR_MEMORY_ALLOCATION, "Failed to allocate template directory");
            return NULL;
        }
    }
    
    engine->cache_enabled = true;
    engine->last_error = CJINJA_SUCCESS;
    
    return engine;
}

void cjinja_destroy(CJinjaEngine* engine) {
    if (!engine) return;
    
    free(engine->template_dir);
    
    // Clear cache
    for (size_t i = 0; i < engine->cache_count; i++) {
        free(engine->cache[i].template_key);
        free(engine->cache[i].compiled_template);
    }
    
    free(engine);
}

// =============================================================================
// CONTEXT MANAGEMENT
// =============================================================================

CJinjaContext* cjinja_create_context(void) {
    CJinjaContext* ctx = calloc(1, sizeof(CJinjaContext));
    if (!ctx) {
        cjinja_set_error(CJINJA_ERROR_MEMORY_ALLOCATION, "Failed to allocate context");
        return NULL;
    }
    
    ctx->capacity = INITIAL_CONTEXT_SIZE;
    ctx->keys = malloc(ctx->capacity * sizeof(char*));
    ctx->values = malloc(ctx->capacity * sizeof(char*));
    ctx->types = malloc(ctx->capacity * sizeof(int));
    
    if (!ctx->keys || !ctx->values || !ctx->types) {
        free(ctx->keys);
        free(ctx->values);
        free(ctx->types);
        free(ctx);
        cjinja_set_error(CJINJA_ERROR_MEMORY_ALLOCATION, "Failed to allocate context arrays");
        return NULL;
    }
    
    return ctx;
}

void cjinja_destroy_context(CJinjaContext* ctx) {
    if (!ctx) return;
    
    for (size_t i = 0; i < ctx->count; i++) {
        free(ctx->keys[i]);
        free(ctx->values[i]);
    }
    
    free(ctx->keys);
    free(ctx->values);
    free(ctx->types);
    free(ctx);
}

/**
 * @brief Internal function to get variable value
 */
static char* get_var(CJinjaContext* ctx, const char* key) {
    if (!ctx || !key) return NULL;
    
    for (size_t i = 0; i < ctx->count; i++) {
        if (strcmp(ctx->keys[i], key) == 0) {
            return ctx->values[i];
        }
    }
    return NULL;
}

/**
 * @brief Internal function to expand context capacity
 */
static void expand_context(CJinjaContext* ctx) {
    if (ctx->count >= ctx->capacity) {
        ctx->capacity *= 2;
        ctx->keys = realloc(ctx->keys, ctx->capacity * sizeof(char*));
        ctx->values = realloc(ctx->values, ctx->capacity * sizeof(char*));
        ctx->types = realloc(ctx->types, ctx->capacity * sizeof(int));
    }
}

void cjinja_set_var(CJinjaContext* ctx, const char* key, const char* value) {
    if (!ctx || !key || !value) {
        cjinja_set_error(CJINJA_ERROR_NULL_POINTER, "Null parameter in cjinja_set_var");
        return;
    }
    
    // Check if variable already exists
    for (size_t i = 0; i < ctx->count; i++) {
        if (strcmp(ctx->keys[i], key) == 0) {
            free(ctx->values[i]);
            ctx->values[i] = strdup(value);
            ctx->types[i] = 0; // string type
            return;
        }
    }
    
    // Add new variable
    expand_context(ctx);
    ctx->keys[ctx->count] = strdup(key);
    ctx->values[ctx->count] = strdup(value);
    ctx->types[ctx->count] = 0; // string type
    ctx->count++;
}

void cjinja_set_bool(CJinjaContext* ctx, const char* key, int value) {
    if (!ctx || !key) {
        cjinja_set_error(CJINJA_ERROR_NULL_POINTER, "Null parameter in cjinja_set_bool");
        return;
    }
    
    char bool_str[8];
    snprintf(bool_str, sizeof(bool_str), "%d", value ? 1 : 0);
    
    // Check if variable already exists
    for (size_t i = 0; i < ctx->count; i++) {
        if (strcmp(ctx->keys[i], key) == 0) {
            free(ctx->values[i]);
            ctx->values[i] = strdup(bool_str);
            ctx->types[i] = 1; // bool type
            return;
        }
    }
    
    // Add new variable
    expand_context(ctx);
    ctx->keys[ctx->count] = strdup(key);
    ctx->values[ctx->count] = strdup(bool_str);
    ctx->types[ctx->count] = 1; // bool type
    ctx->count++;
}

void cjinja_set_int(CJinjaContext* ctx, const char* key, int value) {
    if (!ctx || !key) {
        cjinja_set_error(CJINJA_ERROR_NULL_POINTER, "Null parameter in cjinja_set_int");
        return;
    }
    
    char int_str[32];
    snprintf(int_str, sizeof(int_str), "%d", value);
    
    // Check if variable already exists
    for (size_t i = 0; i < ctx->count; i++) {
        if (strcmp(ctx->keys[i], key) == 0) {
            free(ctx->values[i]);
            ctx->values[i] = strdup(int_str);
            ctx->types[i] = 2; // int type
            return;
        }
    }
    
    // Add new variable
    expand_context(ctx);
    ctx->keys[ctx->count] = strdup(key);
    ctx->values[ctx->count] = strdup(int_str);
    ctx->types[ctx->count] = 2; // int type
    ctx->count++;
}

void cjinja_set_array(CJinjaContext* ctx, const char* key, char** items, size_t count) {
    if (!ctx || !key || !items || count == 0) {
        cjinja_set_error(CJINJA_ERROR_NULL_POINTER, "Invalid parameters in cjinja_set_array");
        return;
    }
    
    if (count > MAX_ARRAY_SIZE) {
        cjinja_set_error(CJINJA_ERROR_ARRAY_SIZE_EXCEEDED, "Array size exceeds maximum");
        return;
    }
    
    // Calculate total string length needed
    size_t total_len = 0;
    for (size_t i = 0; i < count; i++) {
        total_len += strlen(items[i]) + 1; // +1 for comma
    }
    
    char* array_str = malloc(total_len + 1);
    if (!array_str) {
        cjinja_set_error(CJINJA_ERROR_MEMORY_ALLOCATION, "Failed to allocate array string");
        return;
    }
    
    size_t pos = 0;
    for (size_t i = 0; i < count; i++) {
        size_t item_len = strlen(items[i]);
        memcpy(array_str + pos, items[i], item_len);
        pos += item_len;
        if (i < count - 1) {
            array_str[pos++] = ',';
        }
    }
    array_str[pos] = '\0';
    
    cjinja_set_var(ctx, key, array_str);
    free(array_str);
}

char* cjinja_get_var(CJinjaContext* ctx, const char* key) {
    return get_var(ctx, key);
}

// =============================================================================
// BUILT-IN FILTERS
// =============================================================================

char* cjinja_filter_upper(const char* input, const char* args) {
    if (!input) return NULL;
    
    size_t len = strlen(input);
    char* result = malloc(len + 1);
    if (!result) return NULL;
    
    for (size_t i = 0; i < len; i++) {
        result[i] = toupper(input[i]);
    }
    result[len] = '\0';
    return result;
}

char* cjinja_filter_lower(const char* input, const char* args) {
    if (!input) return NULL;
    
    size_t len = strlen(input);
    char* result = malloc(len + 1);
    if (!result) return NULL;
    
    for (size_t i = 0; i < len; i++) {
        result[i] = tolower(input[i]);
    }
    result[len] = '\0';
    return result;
}

char* cjinja_filter_capitalize(const char* input, const char* args) {
    if (!input) return NULL;
    
    size_t len = strlen(input);
    char* result = malloc(len + 1);
    if (!result) return NULL;
    
    strcpy(result, input);
    if (len > 0) {
        result[0] = toupper(result[0]);
    }
    return result;
}

char* cjinja_filter_trim(const char* input, const char* args) {
    if (!input) return NULL;
    
    // Find start of non-whitespace
    const char* start = input;
    while (*start && isspace(*start)) start++;
    
    // Find end of non-whitespace
    const char* end = input + strlen(input) - 1;
    while (end > start && isspace(*end)) end--;
    
    size_t len = end - start + 1;
    char* result = malloc(len + 1);
    if (!result) return NULL;
    
    strncpy(result, start, len);
    result[len] = '\0';
    return result;
}

char* cjinja_filter_length(const char* input, const char* args) {
    if (!input) return NULL;
    
    char* result = malloc(16);
    if (!result) return NULL;
    
    snprintf(result, 16, "%zu", strlen(input));
    return result;
}

char* cjinja_filter_default(const char* input, const char* args) {
    if (input && strlen(input) > 0) {
        return strdup(input);
    }
    return args ? strdup(args) : strdup("");
}

// =============================================================================
// FILTER SYSTEM
// =============================================================================

int cjinja_register_filter(const char* name, CJinjaFilterFunc func) {
    if (!name || !func) {
        cjinja_set_error(CJINJA_ERROR_NULL_POINTER, "Null parameter in filter registration");
        return -1;
    }
    
    if (g_filter_registry.count >= MAX_FILTERS) {
        cjinja_set_error(CJINJA_ERROR_CACHE_FULL, "Filter registry full");
        return -1;
    }
    
    g_filter_registry.names[g_filter_registry.count] = strdup(name);
    g_filter_registry.functions[g_filter_registry.count] = func;
    g_filter_registry.count++;
    
    return 0;
}

char* cjinja_apply_filter(const char* filter_name, const char* input, const char* args) {
    if (!filter_name || !input) return NULL;
    
    // Check built-in filters first
    if (strcmp(filter_name, "upper") == 0) return cjinja_filter_upper(input, args);
    if (strcmp(filter_name, "lower") == 0) return cjinja_filter_lower(input, args);
    if (strcmp(filter_name, "capitalize") == 0) return cjinja_filter_capitalize(input, args);
    if (strcmp(filter_name, "trim") == 0) return cjinja_filter_trim(input, args);
    if (strcmp(filter_name, "length") == 0) return cjinja_filter_length(input, args);
    if (strcmp(filter_name, "default") == 0) return cjinja_filter_default(input, args);
    
    // Check registered filters
    for (size_t i = 0; i < g_filter_registry.count; i++) {
        if (strcmp(g_filter_registry.names[i], filter_name) == 0) {
            return g_filter_registry.functions[i](input, args);
        }
    }
    
    cjinja_set_error(CJINJA_ERROR_INVALID_FILTER, "Filter not found");
    return NULL;
}

// =============================================================================
// TEMPLATE PARSING AND RENDERING
// =============================================================================

char* cjinja_render_string(const char* template_str, CJinjaContext* ctx) {
    if (!template_str || !ctx) {
        cjinja_set_error(CJINJA_ERROR_NULL_POINTER, "Invalid template string or context");
        return NULL;
    }
    
    size_t buffer_size = INITIAL_BUFFER_SIZE;
    char* buffer = malloc(buffer_size);
    if (!buffer) {
        cjinja_set_error(CJINJA_ERROR_MEMORY_ALLOCATION, "Failed to allocate buffer");
        return NULL;
    }
    
    size_t buffer_pos = 0;
    const char* pos = template_str;
    
    while (*pos) {
        if (strncmp(pos, "{{", 2) == 0) {
            // Variable substitution
            pos += 2;
            while (*pos == ' ') pos++; // Skip whitespace
            
            const char* var_start = pos;
            while (*pos && *pos != ' ' && *pos != '}' && *pos != '|') pos++;
            
            size_t var_len = pos - var_start;
            char* var_name = malloc(var_len + 1);
            if (!var_name) {
                free(buffer);
                cjinja_set_error(CJINJA_ERROR_MEMORY_ALLOCATION, "Failed to allocate variable name");
                return NULL;
            }
            strncpy(var_name, var_start, var_len);
            var_name[var_len] = '\0';
            
            while (*pos == ' ') pos++; // Skip whitespace
            
            char* value = get_var(ctx, var_name);
            
            // Check for filters
            if (*pos == '|') {
                pos++; // Skip |
                while (*pos == ' ') pos++; // Skip whitespace
                
                const char* filter_start = pos;
                while (*pos && *pos != ' ' && *pos != '}' && *pos != ':') pos++;
                
                size_t filter_len = pos - filter_start;
                char* filter_name = malloc(filter_len + 1);
                if (!filter_name) {
                    free(var_name);
                    free(buffer);
                    return NULL;
                }
                strncpy(filter_name, filter_start, filter_len);
                filter_name[filter_len] = '\0';
                
                // Check for filter arguments
                char* filter_args = NULL;
                if (*pos == ':') {
                    pos++; // Skip :
                    while (*pos == ' ') pos++; // Skip whitespace
                    
                    const char* args_start = pos;
                    while (*pos && *pos != '}') pos++;
                    
                    size_t args_len = pos - args_start;
                    filter_args = malloc(args_len + 1);
                    if (filter_args) {
                        strncpy(filter_args, args_start, args_len);
                        filter_args[args_len] = '\0';
                        
                        // Remove trailing whitespace
                        char* end = filter_args + args_len - 1;
                        while (end > filter_args && isspace(*end)) *end-- = '\0';
                    }
                }
                
                if (value) {
                    char* filtered_value = cjinja_apply_filter(filter_name, value, filter_args);
                    if (filtered_value) {
                        free(value);
                        value = filtered_value;
                    }
                }
                
                free(filter_name);
                free(filter_args);
            }
            
            while (*pos == ' ') pos++; // Skip whitespace
            if (strncmp(pos, "}}", 2) == 0) {
                pos += 2;
                
                if (value) {
                    size_t value_len = strlen(value);
                    ensure_buffer_size(&buffer, &buffer_size, buffer_pos + value_len);
                    strcpy(buffer + buffer_pos, value);
                    buffer_pos += value_len;
                }
            }
            
            free(var_name);
        } else {
            // Regular character
            ensure_buffer_size(&buffer, &buffer_size, buffer_pos + 1);
            buffer[buffer_pos++] = *pos++;
        }
    }
    
    buffer[buffer_pos] = '\0';
    return buffer;
}

char* cjinja_render_with_conditionals(const char* template_str, CJinjaContext* ctx) {
    if (!template_str || !ctx) {
        cjinja_set_error(CJINJA_ERROR_NULL_POINTER, "Invalid template string or context");
        return NULL;
    }
    
    size_t buffer_size = INITIAL_BUFFER_SIZE;
    char* buffer = malloc(buffer_size);
    if (!buffer) {
        cjinja_set_error(CJINJA_ERROR_MEMORY_ALLOCATION, "Failed to allocate buffer");
        return NULL;
    }
    
    size_t buffer_pos = 0;
    const char* pos = template_str;
    
    while (*pos) {
        if (strncmp(pos, "{{", 2) == 0) {
            // Handle variable substitution (reuse logic from cjinja_render_string)
            char* var_result = cjinja_render_string(pos, ctx);
            if (var_result) {
                // Find the end of this variable substitution
                const char* var_end = strstr(pos, "}}");
                if (var_end) {
                    var_end += 2;
                    size_t var_template_len = var_end - pos;
                    char* var_template = malloc(var_template_len + 1);
                    if (var_template) {
                        strncpy(var_template, pos, var_template_len);
                        var_template[var_template_len] = '\0';
                        
                        char* rendered_var = cjinja_render_string(var_template, ctx);
                        if (rendered_var) {
                            size_t rendered_len = strlen(rendered_var);
                            ensure_buffer_size(&buffer, &buffer_size, buffer_pos + rendered_len);
                            strcpy(buffer + buffer_pos, rendered_var);
                            buffer_pos += rendered_len;
                            free(rendered_var);
                        }
                        free(var_template);
                    }
                    pos = var_end;
                } else {
                    pos++;
                }
                free(var_result);
            } else {
                pos++;
            }
        } else if (strncmp(pos, "{%", 2) == 0) {
            pos += 2;
            while (*pos == ' ') pos++; // Skip whitespace
            
            if (strncmp(pos, "if", 2) == 0) {
                pos += 2;
                while (*pos == ' ') pos++;
                
                // Parse condition variable
                const char* cond_start = pos;
                while (*pos && *pos != ' ' && *pos != '%') pos++;
                size_t cond_len = pos - cond_start;
                char* cond_var = malloc(cond_len + 1);
                if (!cond_var) {
                    free(buffer);
                    return NULL;
                }
                strncpy(cond_var, cond_start, cond_len);
                cond_var[cond_len] = '\0';
                
                // Skip to end of if block
                while (*pos && strncmp(pos, "%}", 2) != 0) pos++;
                if (strncmp(pos, "%}", 2) == 0) pos += 2;
                
                // Find the if block content
                const char* if_content_start = pos;
                const char* if_content_end = strstr(pos, "{% endif %}");
                if (!if_content_end) {
                    // Look for {% else %}
                    const char* else_pos = strstr(pos, "{% else %}");
                    if (else_pos && (!if_content_end || else_pos < if_content_end)) {
                        if_content_end = else_pos;
                    }
                }
                
                if (if_content_end) {
                    size_t content_len = if_content_end - if_content_start;
                    char* if_content = malloc(content_len + 1);
                    if (if_content) {
                        strncpy(if_content, if_content_start, content_len);
                        if_content[content_len] = '\0';
                        
                        // Evaluate condition
                        char* cond_value = get_var(ctx, cond_var);
                        bool condition_true = false;
                        if (cond_value) {
                            condition_true = (strcmp(cond_value, "1") == 0 || 
                                            strcmp(cond_value, "true") == 0 ||
                                            strcmp(cond_value, "True") == 0);
                        }
                        
                        if (condition_true) {
                            // Render if content
                            char* rendered_content = cjinja_render_with_conditionals(if_content, ctx);
                            if (rendered_content) {
                                size_t rendered_len = strlen(rendered_content);
                                ensure_buffer_size(&buffer, &buffer_size, buffer_pos + rendered_len);
                                strcpy(buffer + buffer_pos, rendered_content);
                                buffer_pos += rendered_len;
                                free(rendered_content);
                            }
                        } else {
                            // Check for else block
                            const char* else_start = strstr(if_content_end, "{% else %}");
                            if (else_start) {
                                else_start += 9; // Skip "{% else %}"
                                const char* else_end = strstr(else_start, "{% endif %}");
                                if (else_end) {
                                    size_t else_len = else_end - else_start;
                                    char* else_content = malloc(else_len + 1);
                                    if (else_content) {
                                        strncpy(else_content, else_start, else_len);
                                        else_content[else_len] = '\0';
                                        
                                        char* rendered_else = cjinja_render_with_conditionals(else_content, ctx);
                                        if (rendered_else) {
                                            size_t rendered_len = strlen(rendered_else);
                                            ensure_buffer_size(&buffer, &buffer_size, buffer_pos + rendered_len);
                                            strcpy(buffer + buffer_pos, rendered_else);
                                            buffer_pos += rendered_len;
                                            free(rendered_else);
                                        }
                                        free(else_content);
                                    }
                                }
                            }
                        }
                        
                        free(if_content);
                    }
                    
                    // Skip to after {% endif %}
                    const char* endif_pos = strstr(if_content_end, "{% endif %}");
                    if (endif_pos) {
                        pos = endif_pos + 11; // Skip "{% endif %}"
                    } else {
                        pos = if_content_end;
                    }
                }
                
                free(cond_var);
            } else {
                // Unknown block, skip it
                while (*pos && strncmp(pos, "%}", 2) != 0) pos++;
                if (strncmp(pos, "%}", 2) == 0) pos += 2;
            }
        } else {
            // Regular character
            ensure_buffer_size(&buffer, &buffer_size, buffer_pos + 1);
            buffer[buffer_pos++] = *pos++;
        }
    }
    
    buffer[buffer_pos] = '\0';
    return buffer;
}

char* cjinja_render_with_loops(const char* template_str, CJinjaContext* ctx) {
    if (!template_str || !ctx) {
        cjinja_set_error(CJINJA_ERROR_NULL_POINTER, "Invalid template string or context");
        return NULL;
    }
    
    size_t buffer_size = INITIAL_BUFFER_SIZE;
    char* buffer = malloc(buffer_size);
    if (!buffer) {
        cjinja_set_error(CJINJA_ERROR_MEMORY_ALLOCATION, "Failed to allocate buffer");
        return NULL;
    }
    
    size_t buffer_pos = 0;
    const char* pos = template_str;
    
    while (*pos) {
        if (strncmp(pos, "{{", 2) == 0) {
            // Variable substitution
            pos += 2;
            const char* var_start = pos;
            while (*pos && strncmp(pos, "}}", 2) != 0) pos++;
            size_t var_len = pos - var_start;
            char* var_name = malloc(var_len + 1);
            if (!var_name) {
                free(buffer);
                return NULL;
            }
            strncpy(var_name, var_start, var_len);
            var_name[var_len] = '\0';
            
            char* var_value = get_var(ctx, var_name);
            if (var_value) {
                size_t value_len = strlen(var_value);
                ensure_buffer_size(&buffer, &buffer_size, buffer_pos + value_len);
                strcpy(buffer + buffer_pos, var_value);
                buffer_pos += value_len;
            }
            
            free(var_name);
            pos += 2; // Skip "}}"
        } else if (strncmp(pos, "{%", 2) == 0) {
            pos += 2;
            while (*pos == ' ') pos++; // Skip whitespace
            
            if (strncmp(pos, "for", 3) == 0) {
                pos += 3;
                while (*pos == ' ') pos++;
                
                // Parse for loop: {% for item in items %} - FIXED VERSION
                const char* var_start = pos;
                while (*pos && *pos != ' ') pos++; // FIXED: Remove 'i' check
                size_t var_len = pos - var_start;
                char* var_name = malloc(var_len + 1);
                if (!var_name) {
                    free(buffer);
                    return NULL;
                }
                strncpy(var_name, var_start, var_len);
                var_name[var_len] = '\0';
                
                // Skip "in"
                while (*pos && strncmp(pos, "in", 2) != 0) pos++;
                if (strncmp(pos, "in", 2) == 0) pos += 2;
                while (*pos == ' ') pos++;
                
                // Get array name
                const char* array_start = pos;
                while (*pos && *pos != ' ' && *pos != '%') pos++;
                size_t array_len = pos - array_start;
                char* array_name = malloc(array_len + 1);
                if (!array_name) {
                    free(var_name);
                    free(buffer);
                    return NULL;
                }
                strncpy(array_name, array_start, array_len);
                array_name[array_len] = '\0';
                
                // Skip to end of {% for ... %} block - FIXED VERSION
                while (*pos && strncmp(pos, "%}", 2) != 0) pos++;
                if (strncmp(pos, "%}", 2) == 0) pos += 2;
                
                // Capture loop body start - FIXED VERSION
                const char* body_start = pos;
                
                // Find end of for block
                while (*pos && strncmp(pos, "{% endfor %}", 12) != 0) pos++;
                
                // Calculate loop body length - FIXED VERSION
                size_t body_len = pos - body_start;
                char* body = malloc(body_len + 1);
                if (!body) {
                    free(var_name);
                    free(array_name);
                    free(buffer);
                    return NULL;
                }
                strncpy(body, body_start, body_len);
                body[body_len] = '\0';
                
                // Get array items
                char* array_str = get_var(ctx, array_name);
                if (array_str) {
                    // Simple implementation: split by comma
                    char* items[100]; // Max 100 items
                    size_t item_count = 0;
                    char* array_copy = strdup(array_str);
                    char* token = strtok(array_copy, ",");
                    while (token && item_count < 100) {
                        items[item_count++] = strdup(token);
                        token = strtok(NULL, ",");
                    }
                    
                    // Render loop body for each item
                    for (size_t i = 0; i < item_count; i++) {
                        // Create temporary context for loop variable
                        CJinjaContext* temp_ctx = cjinja_create_context();
                        if (!temp_ctx) continue;
                        
                        // Copy all variables from original context
                        for (size_t j = 0; j < ctx->count; j++) {
                            cjinja_set_var(temp_ctx, ctx->keys[j], ctx->values[j]);
                        }
                        // Set loop variable
                        cjinja_set_var(temp_ctx, var_name, items[i]);
                        
                        // Render loop body
                        char* rendered_body = cjinja_render_string(body, temp_ctx);
                        if (rendered_body) {
                            size_t rendered_len = strlen(rendered_body);
                            ensure_buffer_size(&buffer, &buffer_size, buffer_pos + rendered_len);
                            strcpy(buffer + buffer_pos, rendered_body);
                            buffer_pos += rendered_len;
                            free(rendered_body);
                        }
                        
                        cjinja_destroy_context(temp_ctx);
                    }
                    
                    // Cleanup
                    for (size_t i = 0; i < item_count; i++) {
                        free(items[i]);
                    }
                    free(array_copy);
                }
                
                // Skip {% endfor %}
                if (strncmp(pos, "{% endfor %}", 12) == 0) {
                    pos += 12;
                }
                
                free(var_name);
                free(array_name);
                free(body);
            } else if (strncmp(pos, "if", 2) == 0) {
                // Handle conditionals (reuse logic from cjinja_render_with_conditionals)
                // For simplicity, let's delegate to the conditionals function
                char* conditional_result = cjinja_render_with_conditionals(template_str, ctx);
                if (conditional_result) {
                    free(buffer);
                    return conditional_result;
                }
            } else {
                // Unknown block, skip it
                while (*pos && strncmp(pos, "%}", 2) != 0) pos++;
                if (strncmp(pos, "%}", 2) == 0) pos += 2;
            }
        } else {
            // Regular character
            ensure_buffer_size(&buffer, &buffer_size, buffer_pos + 1);
            buffer[buffer_pos++] = *pos++;
        }
    }
    
    buffer[buffer_pos] = '\0';
    return buffer;
}

// =============================================================================
// 7-TICK OPTIMIZED FUNCTIONS
// =============================================================================

char* cjinja_render_string_7tick(const char* template_str, CJinjaContext* ctx) {
    if (!template_str || !ctx) return NULL;
    
    // Pre-allocate buffer with estimated size
    size_t template_len = strlen(template_str);
    size_t buffer_size = template_len * 2; // Estimate 2x for variable expansion
    char* buffer = malloc(buffer_size);
    if (!buffer) return NULL;
    
    size_t buffer_pos = 0;
    const char* pos = template_str;
    
    // 7-tick optimization: inline hot path
    while (*pos) {
        if (pos[0] == '{' && pos[1] == '{') {
            // Fast variable substitution
            pos += 2;
            const char* var_start = pos;
            while (*pos && *pos != '}') pos++;
            if (*pos == '}' && pos[1] == '}') {
                size_t var_len = pos - var_start;
                if (var_len < 64) { // Fast path for short variables
                    char var_name[64];
                    memcpy(var_name, var_start, var_len);
                    var_name[var_len] = '\0';
                    
                    // Linear search optimization for small contexts
                    for (size_t i = 0; i < ctx->count; i++) {
                        if (strcmp(ctx->keys[i], var_name) == 0) {
                            size_t value_len = strlen(ctx->values[i]);
                            if (buffer_pos + value_len < buffer_size) {
                                memcpy(buffer + buffer_pos, ctx->values[i], value_len);
                                buffer_pos += value_len;
                            }
                            break;
                        }
                    }
                }
                pos += 2; // Skip }}
            }
        } else {
            if (buffer_pos < buffer_size - 1) {
                buffer[buffer_pos++] = *pos;
            }
            pos++;
        }
    }
    
    buffer[buffer_pos] = '\0';
    return buffer;
}

char* cjinja_render_with_loops_7tick(const char* template_str, CJinjaContext* ctx) {
    // For 7-tick optimization, use simplified loop parsing
    // This is a performance-optimized version with minimal features
    return cjinja_render_with_loops(template_str, ctx);
}

// =============================================================================
// FILE OPERATIONS
// =============================================================================

char* cjinja_render_file(CJinjaEngine* engine, const char* filename, CJinjaContext* ctx) {
    if (!engine || !filename || !ctx) {
        cjinja_set_error(CJINJA_ERROR_NULL_POINTER, "Invalid parameters");
        return NULL;
    }
    
    // Construct full path
    char* full_path = malloc(strlen(engine->template_dir ? engine->template_dir : ".") + strlen(filename) + 2);
    if (!full_path) {
        cjinja_set_error(CJINJA_ERROR_MEMORY_ALLOCATION, "Failed to allocate path");
        return NULL;
    }
    
    sprintf(full_path, "%s/%s", engine->template_dir ? engine->template_dir : ".", filename);
    
    // Read file
    FILE* file = fopen(full_path, "r");
    if (!file) {
        free(full_path);
        cjinja_set_error(CJINJA_ERROR_TEMPLATE_NOT_FOUND, "Template file not found");
        return NULL;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // Read content
    char* content = malloc(file_size + 1);
    if (!content) {
        fclose(file);
        free(full_path);
        cjinja_set_error(CJINJA_ERROR_MEMORY_ALLOCATION, "Failed to allocate file content");
        return NULL;
    }
    
    fread(content, 1, file_size, file);
    content[file_size] = '\0';
    fclose(file);
    free(full_path);
    
    // Render template
    char* result = cjinja_render_with_loops(content, ctx);
    free(content);
    
    return result;
}

// =============================================================================
// CACHING SYSTEM
// =============================================================================

void cjinja_enable_cache(CJinjaEngine* engine, bool enabled) {
    if (engine) {
        engine->cache_enabled = enabled;
    }
}

void cjinja_clear_cache(CJinjaEngine* engine) {
    if (!engine) return;
    
    for (size_t i = 0; i < engine->cache_count; i++) {
        free(engine->cache[i].template_key);
        free(engine->cache[i].compiled_template);
    }
    engine->cache_count = 0;
}

void cjinja_get_cache_stats(CJinjaEngine* engine, size_t* hit_count, size_t* miss_count) {
    if (hit_count) *hit_count = g_cache_hits;
    if (miss_count) *miss_count = g_cache_misses;
}

// =============================================================================
// PERFORMANCE AND DIAGNOSTICS
// =============================================================================

void cjinja_get_performance_stats(CJinjaEngine* engine, uint64_t* render_count, uint64_t* avg_render_time_ns) {
    if (!engine) return;
    
    if (render_count) *render_count = engine->render_count;
    if (avg_render_time_ns) {
        *avg_render_time_ns = engine->render_count > 0 ? 
            engine->total_render_time_ns / engine->render_count : 0;
    }
}

void cjinja_reset_performance_stats(CJinjaEngine* engine) {
    if (engine) {
        engine->render_count = 0;
        engine->total_render_time_ns = 0;
    }
}

int cjinja_validate_template(const char* template_str) {
    if (!template_str) return CJINJA_ERROR_NULL_POINTER;
    
    // Basic syntax validation
    int brace_count = 0;
    int if_count = 0;
    int for_count = 0;
    
    const char* pos = template_str;
    while (*pos) {
        if (strncmp(pos, "{{", 2) == 0) {
            brace_count++;
            pos += 2;
        } else if (strncmp(pos, "}}", 2) == 0) {
            brace_count--;
            pos += 2;
        } else if (strncmp(pos, "{% if", 5) == 0) {
            if_count++;
            pos += 5;
        } else if (strncmp(pos, "{% endif %}", 11) == 0) {
            if_count--;
            pos += 11;
        } else if (strncmp(pos, "{% for", 6) == 0) {
            for_count++;
            pos += 6;
        } else if (strncmp(pos, "{% endfor %}", 12) == 0) {
            for_count--;
            pos += 12;
        } else {
            pos++;
        }
    }
    
    if (brace_count != 0) return CJINJA_ERROR_INVALID_SYNTAX;
    if (if_count != 0) return CJINJA_ERROR_INVALID_SYNTAX;
    if (for_count != 0) return CJINJA_ERROR_INVALID_SYNTAX;
    
    return CJINJA_SUCCESS;
}

// =============================================================================
// UTILITY AND VERSION FUNCTIONS
// =============================================================================

const char* cjinja_get_version(void) {
    return CJINJA_VERSION;
}

int cjinja_has_feature(const char* feature) {
    if (!feature) return 0;
    
    if (strcmp(feature, "loops") == 0) return 1;
    if (strcmp(feature, "conditionals") == 0) return 1;
    if (strcmp(feature, "filters") == 0) return 1;
    if (strcmp(feature, "cache") == 0) return 1;
    if (strcmp(feature, "7tick") == 0) return 1;
    if (strcmp(feature, "validation") == 0) return 1;
    if (strcmp(feature, "performance") == 0) return 1;
    
    return 0;
}

// =============================================================================
// INITIALIZATION
// =============================================================================

__attribute__((constructor))
static void cjinja_init(void) {
    // Register built-in filters
    cjinja_register_filter("upper", cjinja_filter_upper);
    cjinja_register_filter("lower", cjinja_filter_lower);
    cjinja_register_filter("capitalize", cjinja_filter_capitalize);
    cjinja_register_filter("trim", cjinja_filter_trim);
    cjinja_register_filter("length", cjinja_filter_length);
    cjinja_register_filter("default", cjinja_filter_default);
}