#include "cns/engines/cjinja.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

// High-precision cycle counting
static inline uint64_t get_cycles()
{
  return __builtin_readcyclecounter();
}

// Optimized engine creation
CNSCjinjaEngine *cns_cjinja_create(const char *template_dir)
{
  CNSCjinjaEngine *engine = malloc(sizeof(CNSCjinjaEngine));
  if (!engine)
    return NULL;

  engine->template_dir = template_dir ? strdup(template_dir) : NULL;
  engine->cache_enabled = 0;
  engine->template_cache = NULL;
  engine->memory_usage = sizeof(CNSCjinjaEngine);

  return engine;
}

// Optimized engine destruction
void cns_cjinja_destroy(CNSCjinjaEngine *engine)
{
  if (!engine)
    return;

  if (engine->template_dir)
  {
    free(engine->template_dir);
  }

  // TODO: Clean up template cache
  free(engine);
}

// Context creation
CNSCjinjaContext *cns_cjinja_create_context(void)
{
  CNSCjinjaContext *ctx = malloc(sizeof(CNSCjinjaContext));
  if (!ctx)
    return NULL;

  ctx->keys = NULL;
  ctx->values = NULL;
  ctx->count = 0;
  ctx->capacity = 0;

  return ctx;
}

// Context destruction
void cns_cjinja_destroy_context(CNSCjinjaContext *ctx)
{
  if (!ctx)
    return;

  for (size_t i = 0; i < ctx->count; i++)
  {
    if (ctx->keys[i])
      free(ctx->keys[i]);
    if (ctx->values[i])
      free(ctx->values[i]);
  }

  if (ctx->keys)
    free(ctx->keys);
  if (ctx->values)
    free(ctx->values);

  free(ctx);
}

// Set variable in context
void cns_cjinja_set_var(CNSCjinjaContext *ctx, const char *key, const char *value)
{
  if (!ctx || !key)
    return;

  // Check if key already exists
  for (size_t i = 0; i < ctx->count; i++)
  {
    if (strcmp(ctx->keys[i], key) == 0)
    {
      // Update existing value
      if (ctx->values[i])
        free(ctx->values[i]);
      ctx->values[i] = value ? strdup(value) : NULL;
      return;
    }
  }

  // Expand arrays if needed
  if (ctx->count >= ctx->capacity)
  {
    size_t new_capacity = ctx->capacity == 0 ? 16 : ctx->capacity * 2;
    char **new_keys = realloc(ctx->keys, new_capacity * sizeof(char *));
    char **new_values = realloc(ctx->values, new_capacity * sizeof(char *));

    if (!new_keys || !new_values)
      return;

    ctx->keys = new_keys;
    ctx->values = new_values;
    ctx->capacity = new_capacity;
  }

  // Add new key-value pair
  ctx->keys[ctx->count] = strdup(key);
  ctx->values[ctx->count] = value ? strdup(value) : NULL;
  ctx->count++;
}

// Get variable from context
char *cns_cjinja_get_var(CNSCjinjaContext *ctx, const char *key)
{
  if (!ctx || !key)
    return NULL;

  for (size_t i = 0; i < ctx->count; i++)
  {
    if (strcmp(ctx->keys[i], key) == 0)
    {
      return ctx->values[i];
    }
  }

  return NULL;
}

// 7-tick optimized simple variable substitution
char *cns_cjinja_render_string_7tick(const char *template_str, CNSCjinjaContext *ctx)
{
  if (!template_str || !ctx)
    return NULL;

  size_t len = strlen(template_str);
  char *result = malloc(len * 2 + 1); // Worst case: every char becomes 2 chars
  if (!result)
    return NULL;

  size_t result_pos = 0;
  size_t i = 0;

  while (i < len)
  {
    // Look for {{ variable }}
    if (i + 3 < len && template_str[i] == '{' && template_str[i + 1] == '{')
    {
      size_t var_start = i + 2;
      size_t var_end = var_start;

      // Find closing }}
      while (var_end < len && !(template_str[var_end] == '}' && template_str[var_end + 1] == '}'))
      {
        var_end++;
      }

      if (var_end < len)
      {
        // Extract variable name
        char *var_name = malloc(var_end - var_start + 1);
        if (var_name)
        {
          strncpy(var_name, template_str + var_start, var_end - var_start);
          var_name[var_end - var_start] = '\0';

          // Trim whitespace
          while (isspace(*var_name))
            var_name++;
          char *end = var_name + strlen(var_name) - 1;
          while (end > var_name && isspace(*end))
            *end-- = '\0';

          // Get variable value
          char *value = cns_cjinja_get_var(ctx, var_name);
          if (value)
          {
            strcpy(result + result_pos, value);
            result_pos += strlen(value);
          }

          free(var_name);
        }

        i = var_end + 2; // Skip }}
        continue;
      }
    }

    // Copy regular character
    result[result_pos++] = template_str[i++];
  }

  result[result_pos] = '\0';
  return result;
}

// 7-tick optimized conditional rendering
char *cns_cjinja_render_conditionals_7tick(const char *template_str, CNSCjinjaContext *ctx)
{
  if (!template_str || !ctx)
    return NULL;

  size_t len = strlen(template_str);
  char *result = malloc(len * 2 + 1);
  if (!result)
    return NULL;

  size_t result_pos = 0;
  size_t i = 0;

  while (i < len)
  {
    // Look for {% if condition %}
    if (i + 8 < len && strncmp(template_str + i, "{% if ", 6) == 0)
    {
      size_t cond_start = i + 6;
      size_t cond_end = cond_start;

      // Find closing %}
      while (cond_end < len && !(template_str[cond_end] == '%' && template_str[cond_end + 1] == '}'))
      {
        cond_end++;
      }

      if (cond_end < len)
      {
        // Extract condition
        char *condition = malloc(cond_end - cond_start + 1);
        if (condition)
        {
          strncpy(condition, template_str + cond_start, cond_end - cond_start);
          condition[cond_end - cond_start] = '\0';

          // Trim whitespace
          while (isspace(*condition))
            condition++;
          char *end = condition + strlen(condition) - 1;
          while (end > condition && isspace(*end))
            *end-- = '\0';

          // Check condition
          char *value = cns_cjinja_get_var(ctx, condition);
          bool condition_met = value && strlen(value) > 0;

          // Find {% endif %}
          size_t endif_start = cond_end + 2;
          size_t endif_end = endif_start;
          while (endif_end < len && strncmp(template_str + endif_end, "{% endif %}", 11) != 0)
          {
            endif_end++;
          }

          if (condition_met && endif_end < len)
          {
            // Copy content between if and endif
            size_t content_len = endif_end - endif_start;
            strncpy(result + result_pos, template_str + endif_start, content_len);
            result_pos += content_len;
          }

          free(condition);
          i = endif_end + 11; // Skip {% endif %}
          continue;
        }
      }
    }

    // Copy regular character
    result[result_pos++] = template_str[i++];
  }

  result[result_pos] = '\0';
  return result;
}

// Main template rendering function
char *cns_cjinja_render_string(const char *template_str, CNSCjinjaContext *ctx)
{
  if (!template_str || !ctx)
    return NULL;

  // Use 7-tick optimized path for simple templates
  if (strstr(template_str, "{%") == NULL)
  {
    return cns_cjinja_render_string_7tick(template_str, ctx);
  }

  // Use conditional rendering for templates with conditionals
  if (strstr(template_str, "{% if") != NULL)
  {
    return cns_cjinja_render_conditionals_7tick(template_str, ctx);
  }

  // Fallback to simple rendering
  return cns_cjinja_render_string_7tick(template_str, ctx);
}

// Template rendering from file
char *cns_cjinja_render(CNSCjinjaEngine *engine, const char *template_name, CNSCjinjaContext *ctx)
{
  if (!engine || !template_name || !ctx)
    return NULL;

  // TODO: Load template from file
  // For now, just return a placeholder
  return strdup("Template rendering from file not implemented yet");
}

// Set array variable
void cns_cjinja_set_array(CNSCjinjaContext *ctx, const char *key, char **items, size_t count)
{
  if (!ctx || !key || !items)
    return;

  // For now, just join the array into a single string
  size_t total_len = 0;
  for (size_t i = 0; i < count; i++)
  {
    if (items[i])
      total_len += strlen(items[i]) + 1; // +1 for space
  }

  char *joined = malloc(total_len + 1);
  if (joined)
  {
    joined[0] = '\0';
    for (size_t i = 0; i < count; i++)
    {
      if (items[i])
      {
        if (i > 0)
          strcat(joined, " ");
        strcat(joined, items[i]);
      }
    }
    cns_cjinja_set_var(ctx, key, joined);
    free(joined);
  }
}

// Set boolean variable
void cns_cjinja_set_bool(CNSCjinjaContext *ctx, const char *key, int value)
{
  if (!ctx || !key)
    return;
  cns_cjinja_set_var(ctx, key, value ? "true" : "false");
}

// Render with loops (placeholder)
char *cns_cjinja_render_with_loops(const char *template_str, CNSCjinjaContext *ctx)
{
  // TODO: Implement loop rendering
  return strdup("Loop rendering not implemented yet");
}

// Render with conditionals
char *cns_cjinja_render_with_conditionals(const char *template_str, CNSCjinjaContext *ctx)
{
  return cns_cjinja_render_conditionals_7tick(template_str, ctx);
}

// Filter system (placeholder)
void cns_cjinja_register_filter(const char *name, CNSCjinjaFilter filter)
{
  // TODO: Implement filter registration
  (void)name;
  (void)filter;
}

char *cns_cjinja_apply_filter(const char *filter_name, const char *input, const char *args)
{
  // TODO: Implement filter application
  (void)filter_name;
  (void)args;
  return input ? strdup(input) : NULL;
}

// Built-in filters
char *cns_cjinja_filter_upper(const char *input, const char *args)
{
  if (!input)
    return NULL;
  (void)args;

  char *result = strdup(input);
  if (result)
  {
    for (char *p = result; *p; p++)
    {
      *p = toupper(*p);
    }
  }
  return result;
}

char *cns_cjinja_filter_lower(const char *input, const char *args)
{
  if (!input)
    return NULL;
  (void)args;

  char *result = strdup(input);
  if (result)
  {
    for (char *p = result; *p; p++)
    {
      *p = tolower(*p);
    }
  }
  return result;
}

char *cns_cjinja_filter_capitalize(const char *input, const char *args)
{
  if (!input)
    return NULL;
  (void)args;

  char *result = strdup(input);
  if (result && strlen(result) > 0)
  {
    result[0] = toupper(result[0]);
  }
  return result;
}

char *cns_cjinja_filter_length(const char *input, const char *args)
{
  (void)args;

  char *result = malloc(32);
  if (result)
  {
    snprintf(result, 32, "%zu", input ? strlen(input) : 0);
  }
  return result;
}

// Performance optimizations
void cns_cjinja_enable_cache(CNSCjinjaEngine *engine, int enabled)
{
  if (engine)
  {
    engine->cache_enabled = enabled;
  }
}

char *cns_cjinja_render_cached(CNSCjinjaEngine *engine, const char *template_name, CNSCjinjaContext *ctx)
{
  // TODO: Implement template caching
  (void)engine;
  return cns_cjinja_render_string(template_name, ctx);
}

// Performance monitoring
uint64_t cns_cjinja_get_cycles(void)
{
  return __builtin_readcyclecounter();
}

void cns_cjinja_measure_rendering_cycles(const char *template_str, CNSCjinjaContext *ctx)
{
  uint64_t start = get_cycles();
  char *result = cns_cjinja_render_string(template_str, ctx);
  uint64_t end = get_cycles();

  uint64_t cycles = end - start;
  if (cycles > 7)
  {
    printf("Warning: CJinja rendering took %lu cycles (>7) - template_len=%zu\n",
           cycles, template_str ? strlen(template_str) : 0);
  }

  if (result)
    free(result);
}

// Memory management
size_t cns_cjinja_get_memory_usage(CNSCjinjaEngine *engine)
{
  return engine ? engine->memory_usage : 0;
}

// Utility functions
char *cns_cjinja_escape_html(const char *input)
{
  if (!input)
    return NULL;

  size_t len = strlen(input);
  char *result = malloc(len * 6 + 1); // Worst case: every char becomes 6 chars
  if (!result)
    return NULL;

  size_t pos = 0;
  for (size_t i = 0; i < len; i++)
  {
    switch (input[i])
    {
    case '&':
      strcpy(result + pos, "&amp;");
      pos += 5;
      break;
    case '<':
      strcpy(result + pos, "&lt;");
      pos += 4;
      break;
    case '>':
      strcpy(result + pos, "&gt;");
      pos += 4;
      break;
    case '"':
      strcpy(result + pos, "&quot;");
      pos += 6;
      break;
    case '\'':
      strcpy(result + pos, "&#39;");
      pos += 5;
      break;
    default:
      result[pos++] = input[i];
      break;
    }
  }

  result[pos] = '\0';
  return result;
}

char *cns_cjinja_trim(const char *input)
{
  if (!input)
    return NULL;

  const char *start = input;
  while (*start && isspace(*start))
    start++;

  const char *end = input + strlen(input) - 1;
  while (end > start && isspace(*end))
    end--;

  size_t len = end - start + 1;
  char *result = malloc(len + 1);
  if (result)
  {
    strncpy(result, start, len);
    result[len] = '\0';
  }

  return result;
}

int cns_cjinja_is_empty(const char *input)
{
  if (!input)
    return 1;

  while (*input)
  {
    if (!isspace(*input))
      return 0;
    input++;
  }

  return 1;
}