#ifndef CJINJA_H
#define CJINJA_H

#include <stddef.h>

// Template context for variable substitution
typedef struct
{
    char **keys;
    char **values;
    size_t count;
    size_t capacity;
} CJinjaContext;

// Template engine
typedef struct
{
    char *template_dir;
    int cache_enabled;
    void *template_cache; // Simple cache for compiled templates
} CJinjaEngine;

// Loop context for {% for %} blocks
typedef struct
{
    char *var_name;
    char **items;
    size_t item_count;
    size_t current_index;
} CJinjaLoopContext;

// Conditional context for {% if %} blocks
typedef struct
{
    int condition_met;
    int in_else_block;
} CJinjaConditionContext;

// Filter function type
typedef char *(*CJinjaFilter)(const char *input, const char *args);

// Public API - Core functionality
CJinjaEngine *cjinja_create(const char *template_dir);
void cjinja_destroy(CJinjaEngine *engine);

CJinjaContext *cjinja_create_context(void);
void cjinja_destroy_context(CJinjaContext *ctx);
void cjinja_set_var(CJinjaContext *ctx, const char *key, const char *value);

// Template rendering
char *cjinja_render(CJinjaEngine *engine, const char *template_name, CJinjaContext *ctx);
char *cjinja_render_string(const char *template_str, CJinjaContext *ctx);

// 80/20 Features - Loops
void cjinja_set_array(CJinjaContext *ctx, const char *key, char **items, size_t count);
char *cjinja_render_with_loops(const char *template_str, CJinjaContext *ctx);

// 80/20 Features - Conditionals
void cjinja_set_bool(CJinjaContext *ctx, const char *key, int value);
char *cjinja_render_with_conditionals(const char *template_str, CJinjaContext *ctx);

// 80/20 Features - Filters
void cjinja_register_filter(const char *name, CJinjaFilter filter);
char *cjinja_apply_filter(const char *filter_name, const char *input, const char *args);

// Built-in filters
char *cjinja_filter_upper(const char *input, const char *args);
char *cjinja_filter_lower(const char *input, const char *args);
char *cjinja_filter_capitalize(const char *input, const char *args);
char *cjinja_filter_length(const char *input, const char *args);

// 80/20 Features - Performance optimizations
void cjinja_enable_cache(CJinjaEngine *engine, int enabled);
char *cjinja_render_cached(CJinjaEngine *engine, const char *template_name, CJinjaContext *ctx);

// 80/20 Features - Template inheritance
char *cjinja_render_with_inheritance(const char *template_str, CJinjaContext *ctx, const char *base_template);

// 80/20 Features - Includes
char *cjinja_render_with_includes(CJinjaEngine *engine, const char *template_str, CJinjaContext *ctx);

// Utility functions
char *cjinja_escape_html(const char *input);
char *cjinja_trim(const char *input);
int cjinja_is_empty(const char *input);

#endif // CJINJA_H
