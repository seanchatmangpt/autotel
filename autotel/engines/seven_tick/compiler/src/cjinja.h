#ifndef CJINJA_H
#define CJINJA_H

#include <stddef.h>

// Template context for variable substitution
typedef struct {
    char** keys;
    char** values;
    size_t count;
    size_t capacity;
} CJinjaContext;

// Template engine
typedef struct {
    char* template_dir;
} CJinjaEngine;

// Public API
CJinjaEngine* cjinja_create(const char* template_dir);
void cjinja_destroy(CJinjaEngine* engine);

CJinjaContext* cjinja_create_context(void);
void cjinja_destroy_context(CJinjaContext* ctx);
void cjinja_set_var(CJinjaContext* ctx, const char* key, const char* value);

char* cjinja_render(CJinjaEngine* engine, const char* template_name, CJinjaContext* ctx);
char* cjinja_render_string(const char* template_str, CJinjaContext* ctx);

#endif // CJINJA_H
