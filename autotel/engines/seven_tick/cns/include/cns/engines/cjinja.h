#ifndef CNS_CJINJA_H
#define CNS_CJINJA_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Branch prediction hints for optimal performance
#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

// Template context for variable substitution
typedef struct
{
  char **keys;
  char **values;
  size_t count;
  size_t capacity;
} CNSCjinjaContext;

// Template engine
typedef struct
{
  char *template_dir;
  int cache_enabled;
  void *template_cache;
  size_t memory_usage;
} CNSCjinjaEngine;

// Filter function type
typedef char *(*CNSCjinjaFilter)(const char *input, const char *args);

// Function declarations

// Engine creation and destruction
CNSCjinjaEngine *cns_cjinja_create(const char *template_dir);
void cns_cjinja_destroy(CNSCjinjaEngine *engine);

// Context management
CNSCjinjaContext *cns_cjinja_create_context(void);
void cns_cjinja_destroy_context(CNSCjinjaContext *ctx);
void cns_cjinja_set_var(CNSCjinjaContext *ctx, const char *key, const char *value);
char *cns_cjinja_get_var(CNSCjinjaContext *ctx, const char *key);

// Template rendering (7-tick optimized)
char *cns_cjinja_render(CNSCjinjaEngine *engine, const char *template_name, CNSCjinjaContext *ctx);
char *cns_cjinja_render_string(const char *template_str, CNSCjinjaContext *ctx);

// 7-tick optimized fast paths
char *cns_cjinja_render_string_7tick(const char *template_str, CNSCjinjaContext *ctx);
char *cns_cjinja_render_conditionals_7tick(const char *template_str, CNSCjinjaContext *ctx);

// Advanced features
void cns_cjinja_set_array(CNSCjinjaContext *ctx, const char *key, char **items, size_t count);
void cns_cjinja_set_bool(CNSCjinjaContext *ctx, const char *key, int value);
char *cns_cjinja_render_with_loops(const char *template_str, CNSCjinjaContext *ctx);
char *cns_cjinja_render_with_conditionals(const char *template_str, CNSCjinjaContext *ctx);

// Filter system
void cns_cjinja_register_filter(const char *name, CNSCjinjaFilter filter);
char *cns_cjinja_apply_filter(const char *filter_name, const char *input, const char *args);

// Built-in filters
char *cns_cjinja_filter_upper(const char *input, const char *args);
char *cns_cjinja_filter_lower(const char *input, const char *args);
char *cns_cjinja_filter_capitalize(const char *input, const char *args);
char *cns_cjinja_filter_length(const char *input, const char *args);

// Performance optimizations
void cns_cjinja_enable_cache(CNSCjinjaEngine *engine, int enabled);
char *cns_cjinja_render_cached(CNSCjinjaEngine *engine, const char *template_name, CNSCjinjaContext *ctx);

// Performance monitoring
uint64_t cns_cjinja_get_cycles(void);
void cns_cjinja_measure_rendering_cycles(const char *template_str, CNSCjinjaContext *ctx);

// Memory management
size_t cns_cjinja_get_memory_usage(CNSCjinjaEngine *engine);

// Utility functions
char *cns_cjinja_escape_html(const char *input);
char *cns_cjinja_trim(const char *input);
int cns_cjinja_is_empty(const char *input);

#endif // CNS_CJINJA_H