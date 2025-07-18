#include "cjinja.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define INITIAL_CONTEXT_SIZE 16
#define INITIAL_BUFFER_SIZE 4096

CJinjaEngine* cjinja_create(const char* template_dir) {
    CJinjaEngine* engine = malloc(sizeof(CJinjaEngine));
    engine->template_dir = strdup(template_dir);
    return engine;
}

void cjinja_destroy(CJinjaEngine* engine) {
    free(engine->template_dir);
    free(engine);
}

CJinjaContext* cjinja_create_context(void) {
    CJinjaContext* ctx = malloc(sizeof(CJinjaContext));
    ctx->capacity = INITIAL_CONTEXT_SIZE;
    ctx->count = 0;
    ctx->keys = calloc(ctx->capacity, sizeof(char*));
    ctx->values = calloc(ctx->capacity, sizeof(char*));
    return ctx;
}

void cjinja_destroy_context(CJinjaContext* ctx) {
    for (size_t i = 0; i < ctx->count; i++) {
        free(ctx->keys[i]);
        free(ctx->values[i]);
    }
    free(ctx->keys);
    free(ctx->values);
    free(ctx);
}

void cjinja_set_var(CJinjaContext* ctx, const char* key, const char* value) {
    // Check if key exists
    for (size_t i = 0; i < ctx->count; i++) {
        if (strcmp(ctx->keys[i], key) == 0) {
            free(ctx->values[i]);
            ctx->values[i] = strdup(value);
            return;
        }
    }
    
    // Add new variable
    if (ctx->count >= ctx->capacity) {
        ctx->capacity *= 2;
        ctx->keys = realloc(ctx->keys, ctx->capacity * sizeof(char*));
        ctx->values = realloc(ctx->values, ctx->capacity * sizeof(char*));
    }
    
    ctx->keys[ctx->count] = strdup(key);
    ctx->values[ctx->count] = strdup(value);
    ctx->count++;
}

static char* get_var(CJinjaContext* ctx, const char* key) {
    for (size_t i = 0; i < ctx->count; i++) {
        if (strcmp(ctx->keys[i], key) == 0) {
            return ctx->values[i];
        }
    }
    return NULL;
}
char* cjinja_render_string(const char* template_str, CJinjaContext* ctx) {
    size_t buffer_size = INITIAL_BUFFER_SIZE;
    char* buffer = malloc(buffer_size);
    size_t buffer_pos = 0;
    
    const char* pos = template_str;
    
    while (*pos) {
        if (strncmp(pos, "{{", 2) == 0) {
            // Variable substitution
            pos += 2;
            while (*pos == ' ') pos++;  // Skip whitespace
            
            const char* var_start = pos;
            while (*pos && *pos != ' ' && *pos != '}') pos++;
            
            size_t var_len = pos - var_start;
            char* var_name = malloc(var_len + 1);
            strncpy(var_name, var_start, var_len);
            var_name[var_len] = '\0';
            
            while (*pos == ' ') pos++;  // Skip whitespace
            if (strncmp(pos, "}}", 2) == 0) {
                pos += 2;
                
                char* value = get_var(ctx, var_name);
                if (value) {
                    size_t value_len = strlen(value);
                    
                    // Ensure buffer capacity
                    while (buffer_pos + value_len >= buffer_size) {
                        buffer_size *= 2;
                        buffer = realloc(buffer, buffer_size);
                    }
                    
                    strcpy(&buffer[buffer_pos], value);
                    buffer_pos += value_len;
                }
            }
            
            free(var_name);
            
        } else if (strncmp(pos, "{%", 2) == 0) {
            // Control structures (simplified for MVP)
            pos += 2;
            while (*pos && strncmp(pos, "%}", 2) != 0) pos++;
            if (*pos) pos += 2;
            
        } else {
            // Regular character
            if (buffer_pos >= buffer_size - 1) {
                buffer_size *= 2;
                buffer = realloc(buffer, buffer_size);
            }
            buffer[buffer_pos++] = *pos++;
        }
    }
    
    buffer[buffer_pos] = '\0';
    return buffer;
}

char* cjinja_render(CJinjaEngine* engine, const char* template_name, CJinjaContext* ctx) {
    char path[4096];
    snprintf(path, sizeof(path), "%s/%s", engine->template_dir, template_name);
    
    FILE* f = fopen(path, "r");
    if (!f) return NULL;
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char* template_str = malloc(size + 1);
    fread(template_str, 1, size, f);
    template_str[size] = '\0';
    fclose(f);
    
    char* result = cjinja_render_string(template_str, ctx);
    free(template_str);
    
    return result;
}
