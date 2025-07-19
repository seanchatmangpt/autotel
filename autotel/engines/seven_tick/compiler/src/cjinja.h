#ifndef CJINJA_H
#define CJINJA_H

#include <stddef.h>
#include <time.h> // Required for time_t

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

// 80/20 FEATURE: Template inheritance context
typedef struct
{
    char *base_template;
    char **block_names;
    char **block_contents;
    size_t block_count;
    size_t block_capacity;
} CJinjaInheritanceContext;

// 80/20 FEATURE: Enhanced template cache entry
typedef struct
{
    char *name;
    char *content;
    size_t hash;
    int is_compiled;         // New: track if template is pre-compiled
    void *compiled_template; // New: store compiled template structure
    time_t last_modified;    // New: for cache invalidation
} TemplateCacheEntry;

// Filter function type
typedef char *(*CJinjaFilter)(const char *input, const char *args);

// Public API - Core functionality
CJinjaEngine *cjinja_create(const char *template_dir);
void cjinja_destroy(CJinjaEngine *engine);

CJinjaContext *cjinja_create_context(void);
void cjinja_destroy_context(CJinjaContext *ctx);
void cjinja_set_var(CJinjaContext *ctx, const char *key, const char *value);
char *get_var(CJinjaContext *ctx, const char *key); // Make this public for benchmarks

// Template rendering
char *cjinja_render(CJinjaEngine *engine, const char *template_name, CJinjaContext *ctx);
char *cjinja_render_string(const char *template_str, CJinjaContext *ctx);

// 7-TICK OPTIMIZED: Fast paths for basic operations
char *cjinja_render_string_7tick(const char *template_str, CJinjaContext *ctx);
char *cjinja_render_conditionals_7tick(const char *template_str, CJinjaContext *ctx);

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

// 80/20 FEATURE: Advanced built-in filters (49-tick path)
char *cjinja_filter_trim(const char *input, const char *args);
char *cjinja_filter_replace(const char *input, const char *args);
char *cjinja_filter_slice(const char *input, const char *args);
char *cjinja_filter_default(const char *input, const char *args);
char *cjinja_filter_join(const char *input, const char *args);
char *cjinja_filter_split(const char *input, const char *args);

// 80/20 Features - Performance optimizations
void cjinja_enable_cache(CJinjaEngine *engine, int enabled);
char *cjinja_render_cached(CJinjaEngine *engine, const char *template_name, CJinjaContext *ctx);

// 80/20 FEATURE: Enhanced template caching (49-tick path)
void cjinja_clear_cache(CJinjaEngine *engine);
void cjinja_set_cache_size(CJinjaEngine *engine, size_t max_entries);
size_t cjinja_get_cache_stats(CJinjaEngine *engine, size_t *hits, size_t *misses);

// 80/20 FEATURE: Template inheritance (49-tick path)
CJinjaInheritanceContext *cjinja_create_inheritance_context(void);
void cjinja_destroy_inheritance_context(CJinjaInheritanceContext *ctx);
void cjinja_set_base_template(CJinjaInheritanceContext *ctx, const char *base_template);
void cjinja_add_block(CJinjaInheritanceContext *ctx, const char *block_name, const char *content);
char *cjinja_render_with_inheritance(const char *template_str, CJinjaContext *ctx, CJinjaInheritanceContext *inherit_ctx);

// 80/20 FEATURE: Include statements (49-tick path)
char *cjinja_render_with_includes(CJinjaEngine *engine, const char *template_str, CJinjaContext *ctx);
char *cjinja_load_template_file(CJinjaEngine *engine, const char *template_name);

// 80/20 FEATURE: Error handling (49-tick path)
typedef enum
{
    CJINJA_SUCCESS = 0,
    CJINJA_ERROR_MEMORY,
    CJINJA_ERROR_SYNTAX,
    CJINJA_ERROR_TEMPLATE_NOT_FOUND,
    CJINJA_ERROR_INVALID_FILTER,
    CJINJA_ERROR_INVALID_VARIABLE
} CJinjaError;

CJinjaError cjinja_get_last_error(void);
const char *cjinja_get_error_message(CJinjaError error);
void cjinja_clear_error(void);

// 80/20 FEATURE: Template compilation for performance (49-tick path)
typedef struct
{
    char *compiled_template;
    size_t size;
} CJinjaCompiledTemplate;

CJinjaCompiledTemplate *cjinja_compile_template(const char *template_str);
char *cjinja_render_compiled(CJinjaCompiledTemplate *compiled, CJinjaContext *ctx);
void cjinja_destroy_compiled_template(CJinjaCompiledTemplate *compiled);

// 80/20 FEATURE: Batch rendering for high throughput (49-tick path)
typedef struct
{
    const char **templates;
    size_t count;
    char **results;
} CJinjaBatchRender;

CJinjaBatchRender *cjinja_create_batch_render(size_t count);
void cjinja_destroy_batch_render(CJinjaBatchRender *batch);
int cjinja_render_batch(CJinjaEngine *engine, CJinjaBatchRender *batch, CJinjaContext *ctx);

// Utility functions
char *cjinja_escape_html(const char *input);
char *cjinja_trim(const char *input);
int cjinja_is_empty(const char *input);

// 80/20 FEATURE: Advanced utility functions (49-tick path)
char *cjinja_safe_string(const char *input);
int cjinja_string_equals(const char *a, const char *b);
char *cjinja_concat_strings(const char *a, const char *b);
char *cjinja_format_number(double number, int precision);

#endif // CJINJA_H
