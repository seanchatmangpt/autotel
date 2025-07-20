#include "cjinja.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <math.h>

#define INITIAL_CONTEXT_SIZE 16
#define INITIAL_BUFFER_SIZE 4096
#define MAX_FILTERS 32
#define MAX_TEMPLATE_CACHE 64
#define MAX_INCLUDE_DEPTH 10

// 80/20 FEATURE: Global error state
static CJinjaError last_error = CJINJA_SUCCESS;
static char error_message[256] = {0};

// 80/20 FEATURE: Cache statistics
static size_t cache_hits = 0;
static size_t cache_misses = 0;

// Filter registry
static struct
{
    char *names[MAX_FILTERS];
    CJinjaFilter functions[MAX_FILTERS];
    size_t count;
} filter_registry = {0};

// Template cache entry (defined in header)

// Template cache
typedef struct
{
    TemplateCacheEntry entries[MAX_TEMPLATE_CACHE];
    size_t count;
    size_t max_entries;
} TemplateCache;

// 80/20 FEATURE: Simple hash function for cache keys
static size_t hash_string(const char *str)
{
    size_t hash = 5381;
    int c;
    while ((c = *str++))
    {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

// 80/20 FEATURE: Error handling functions
void cjinja_set_error(CJinjaError error, const char *message)
{
    last_error = error;
    strncpy(error_message, message, sizeof(error_message) - 1);
    error_message[sizeof(error_message) - 1] = '\0';
}

CJinjaError cjinja_get_last_error(void)
{
    return last_error;
}

const char *cjinja_get_error_message(CJinjaError error)
{
    return error_message;
}

void cjinja_clear_error(void)
{
    last_error = CJINJA_SUCCESS;
    memset(error_message, 0, sizeof(error_message));
}

CJinjaEngine *cjinja_create(const char *template_dir)
{
    CJinjaEngine *engine = malloc(sizeof(CJinjaEngine));
    if (!engine)
    {
        cjinja_set_error(CJINJA_ERROR_MEMORY, "Failed to allocate engine");
        return NULL;
    }

    engine->template_dir = strdup(template_dir);
    engine->cache_enabled = 1; // Enable by default
    engine->template_cache = malloc(sizeof(TemplateCache));
    if (!engine->template_cache)
    {
        cjinja_set_error(CJINJA_ERROR_MEMORY, "Failed to allocate template cache");
        free(engine);
        return NULL;
    }

    TemplateCache *cache = (TemplateCache *)engine->template_cache;
    memset(cache, 0, sizeof(TemplateCache));
    cache->max_entries = MAX_TEMPLATE_CACHE;

    // Register built-in filters
    cjinja_register_filter("upper", cjinja_filter_upper);
    cjinja_register_filter("lower", cjinja_filter_lower);
    cjinja_register_filter("capitalize", cjinja_filter_capitalize);
    cjinja_register_filter("length", cjinja_filter_length);

    // 80/20 FEATURE: Register advanced filters
    cjinja_register_filter("trim", cjinja_filter_trim);
    cjinja_register_filter("replace", cjinja_filter_replace);
    cjinja_register_filter("slice", cjinja_filter_slice);
    cjinja_register_filter("default", cjinja_filter_default);
    cjinja_register_filter("join", cjinja_filter_join);
    cjinja_register_filter("split", cjinja_filter_split);

    cjinja_clear_error();
    return engine;
}

void cjinja_destroy(CJinjaEngine *engine)
{
    if (engine->template_cache)
    {
        TemplateCache *cache = (TemplateCache *)engine->template_cache;
        for (size_t i = 0; i < cache->count; i++)
        {
            free(cache->entries[i].name);
            free(cache->entries[i].content);
        }
        free(engine->template_cache);
    }
    free(engine->template_dir);
    free(engine);
}

void cjinja_destroy_engine(CJinjaEngine *engine)
{
    if (!engine)
        return;

    free(engine->template_dir);

    if (engine->template_cache)
    {
        TemplateCache *cache = (TemplateCache *)engine->template_cache;
        for (size_t i = 0; i < cache->count; i++)
        {
            free(cache->entries[i].name);
            free(cache->entries[i].content);
            if (cache->entries[i].compiled_template)
            {
                free(cache->entries[i].compiled_template);
            }
        }
        free(engine->template_cache);
    }

    free(engine);
}

CJinjaContext *cjinja_create_context(void)
{
    CJinjaContext *ctx = malloc(sizeof(CJinjaContext));
    ctx->capacity = INITIAL_CONTEXT_SIZE;
    ctx->count = 0;
    ctx->keys = calloc(ctx->capacity, sizeof(char *));
    ctx->values = calloc(ctx->capacity, sizeof(char *));
    return ctx;
}

void cjinja_destroy_context(CJinjaContext *ctx)
{
    for (size_t i = 0; i < ctx->count; i++)
    {
        free(ctx->keys[i]);
        free(ctx->values[i]);
    }
    free(ctx->keys);
    free(ctx->values);
    free(ctx);
}

void cjinja_set_var(CJinjaContext *ctx, const char *key, const char *value)
{
    // Check if key exists
    for (size_t i = 0; i < ctx->count; i++)
    {
        if (strcmp(ctx->keys[i], key) == 0)
        {
            free(ctx->values[i]);
            ctx->values[i] = strdup(value);
            return;
        }
    }

    // Add new variable
    if (ctx->count >= ctx->capacity)
    {
        ctx->capacity *= 2;
        ctx->keys = realloc(ctx->keys, ctx->capacity * sizeof(char *));
        ctx->values = realloc(ctx->values, ctx->capacity * sizeof(char *));
    }

    ctx->keys[ctx->count] = strdup(key);
    ctx->values[ctx->count] = strdup(value);
    ctx->count++;
}

// 80/20 Feature: Set array for loops
void cjinja_set_array(CJinjaContext *ctx, const char *key, char **items, size_t count)
{
    // Store array as comma-separated string for simplicity
    size_t total_len = 0;
    for (size_t i = 0; i < count; i++)
    {
        total_len += strlen(items[i]) + 1; // +1 for comma
    }

    char *array_str = malloc(total_len + 1);
    size_t pos = 0;
    for (size_t i = 0; i < count; i++)
    {
        strcpy(array_str + pos, items[i]);
        pos += strlen(items[i]);
        if (i < count - 1)
        {
            array_str[pos++] = ',';
        }
    }
    array_str[pos] = '\0';

    cjinja_set_var(ctx, key, array_str);
    free(array_str);
}

// 80/20 Feature: Set boolean for conditionals
void cjinja_set_bool(CJinjaContext *ctx, const char *key, int value)
{
    cjinja_set_var(ctx, key, value ? "true" : "false");
}

char *get_var(CJinjaContext *ctx, const char *key)
{
    for (size_t i = 0; i < ctx->count; i++)
    {
        if (strcmp(ctx->keys[i], key) == 0)
        {
            return ctx->values[i];
        }
    }
    return NULL;
}

// 80/20 Feature: Register filter
void cjinja_register_filter(const char *name, CJinjaFilter filter)
{
    if (filter_registry.count < MAX_FILTERS)
    {
        filter_registry.names[filter_registry.count] = strdup(name);
        filter_registry.functions[filter_registry.count] = filter;
        filter_registry.count++;
    }
}

// 80/20 Feature: Apply filter
char *cjinja_apply_filter(const char *filter_name, const char *input, const char *args)
{
    for (size_t i = 0; i < filter_registry.count; i++)
    {
        if (strcmp(filter_registry.names[i], filter_name) == 0)
        {
            return filter_registry.functions[i](input, args);
        }
    }
    return strdup(input); // Return original if filter not found
}

// Built-in filters
char *cjinja_filter_upper(const char *input, const char *args)
{
    char *result = strdup(input);
    for (char *p = result; *p; p++)
    {
        *p = toupper(*p);
    }
    return result;
}

char *cjinja_filter_lower(const char *input, const char *args)
{
    char *result = strdup(input);
    for (char *p = result; *p; p++)
    {
        *p = tolower(*p);
    }
    return result;
}

char *cjinja_filter_capitalize(const char *input, const char *args)
{
    char *result = strdup(input);
    if (result[0])
    {
        result[0] = toupper(result[0]);
    }
    return result;
}

char *cjinja_filter_length(const char *input, const char *args)
{
    char *result = malloc(32);
    snprintf(result, 32, "%zu", strlen(input));
    return result;
}

// 80/20 Feature: Enable cache
void cjinja_enable_cache(CJinjaEngine *engine, int enabled)
{
    engine->cache_enabled = enabled;
}

// 80/20 Feature: Render with caching
char *cjinja_render_cached(CJinjaEngine *engine, const char *template_name, CJinjaContext *ctx)
{
    if (!engine->cache_enabled)
    {
        return cjinja_render(engine, template_name, ctx);
    }

    TemplateCache *cache = (TemplateCache *)engine->template_cache;

    // Check cache
    for (size_t i = 0; i < cache->count; i++)
    {
        if (strcmp(cache->entries[i].name, template_name) == 0)
        {
            return cjinja_render_string(cache->entries[i].content, ctx);
        }
    }

    // Load and cache template
    char path[4096];
    snprintf(path, sizeof(path), "%s/%s", engine->template_dir, template_name);

    FILE *f = fopen(path, "r");
    if (!f)
        return NULL;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *template_str = malloc(size + 1);
    fread(template_str, 1, size, f);
    template_str[size] = '\0';
    fclose(f);

    // Add to cache
    if (cache->count < MAX_TEMPLATE_CACHE)
    {
        cache->entries[cache->count].name = strdup(template_name);
        cache->entries[cache->count].content = strdup(template_str);
        cache->count++;
    }

    char *result = cjinja_render_string(template_str, ctx);
    free(template_str);
    return result;
}

// 80/20 Feature: Render with loops
char *cjinja_render_with_loops(const char *template_str, CJinjaContext *ctx)
{
    size_t buffer_size = INITIAL_BUFFER_SIZE;
    char *buffer = malloc(buffer_size);
    size_t buffer_pos = 0;

    const char *pos = template_str;

    while (*pos)
    {
        if (strncmp(pos, "{%", 2) == 0)
        {
            pos += 2;
            while (*pos == ' ')
                pos++; // Skip whitespace

            if (strncmp(pos, "for", 3) == 0)
            {
                pos += 3;
                while (*pos == ' ')
                    pos++;

                // Parse for loop: {% for item in items %}
                const char *var_start = pos;
                while (*pos && *pos != ' ')
                    pos++;
                size_t var_len = pos - var_start;
                char *var_name = malloc(var_len + 1);
                strncpy(var_name, var_start, var_len);
                var_name[var_len] = '\0';

                // Skip "in"
                while (*pos && strncmp(pos, "in", 2) != 0)
                    pos++;
                if (strncmp(pos, "in", 2) == 0)
                    pos += 2;
                while (*pos == ' ')
                    pos++;

                // Get array name
                const char *array_start = pos;
                while (*pos && *pos != ' ' && *pos != '%')
                    pos++;
                size_t array_len = pos - array_start;
                char *array_name = malloc(array_len + 1);
                strncpy(array_name, array_start, array_len);
                array_name[array_len] = '\0';

                // Skip to end of {% for ... %} block
                while (*pos && strncmp(pos, "%}", 2) != 0)
                    pos++;
                if (strncmp(pos, "%}", 2) == 0)
                    pos += 2;

                // Capture loop body start
                const char *body_start = pos;

                // Find end of for block
                while (*pos && strncmp(pos, "{% endfor %}", 12) != 0)
                    pos++;
                size_t body_len = pos - body_start;
                char *body = malloc(body_len + 1);
                strncpy(body, body_start, body_len);
                body[body_len] = '\0';

                // Get array items
                char *array_str = get_var(ctx, array_name);
                if (array_str)
                {
                    // Simple implementation: split by comma
                    char *items[100]; // Max 100 items
                    size_t item_count = 0;
                    char *token = strtok(strdup(array_str), ",");
                    while (token && item_count < 100)
                    {
                        items[item_count++] = strdup(token);
                        token = strtok(NULL, ",");
                    }

                    // Render loop body for each item
                    for (size_t i = 0; i < item_count; i++)
                    {
                        // Create temporary context for loop variable
                        CJinjaContext *temp_ctx = cjinja_create_context();
                        // Copy all variables from original context
                        for (size_t j = 0; j < ctx->count; j++)
                        {
                            cjinja_set_var(temp_ctx, ctx->keys[j], ctx->values[j]);
                        }
                        // Set loop variable
                        cjinja_set_var(temp_ctx, var_name, items[i]);

                        // Render loop body
                        char *rendered_body = cjinja_render_string(body, temp_ctx);
                        size_t rendered_len = strlen(rendered_body);

                        // Ensure buffer capacity
                        while (buffer_pos + rendered_len >= buffer_size)
                        {
                            buffer_size *= 2;
                            buffer = realloc(buffer, buffer_size);
                        }

                        strcpy(&buffer[buffer_pos], rendered_body);
                        buffer_pos += rendered_len;

                        free(rendered_body);
                        cjinja_destroy_context(temp_ctx);
                        free(items[i]);
                    }
                }

                free(var_name);
                free(array_name);
                free(body);

                if (strncmp(pos, "{% endfor %}", 12) == 0)
                {
                    pos += 12;
                }
            }
            else
            {
                // Skip other control structures
                while (*pos && strncmp(pos, "%}", 2) != 0)
                    pos++;
                if (*pos)
                    pos += 2;
            }
        }
        else if (strncmp(pos, "{{", 2) == 0)
        {
            // Variable substitution with filter support
            pos += 2;
            while (*pos == ' ')
                pos++; // Skip whitespace

            const char *var_start = pos;
            while (*pos && *pos != ' ' && *pos != '|' && *pos != '}')
                pos++;

            size_t var_len = pos - var_start;
            char *var_name = malloc(var_len + 1);
            strncpy(var_name, var_start, var_len);
            var_name[var_len] = '\0';

            char *value = get_var(ctx, var_name);
            char *final_value = value ? strdup(value) : strdup("");

            // Check for filters
            while (*pos == ' ')
                pos++;
            if (*pos == '|')
            {
                pos++; // Skip |
                while (*pos == ' ')
                    pos++;

                const char *filter_start = pos;
                while (*pos && *pos != ' ' && *pos != '}')
                    pos++;
                size_t filter_len = pos - filter_start;
                char *filter_name = malloc(filter_len + 1);
                strncpy(filter_name, filter_start, filter_len);
                filter_name[filter_len] = '\0';

                // Apply filter
                char *filtered_value = cjinja_apply_filter(filter_name, final_value, "");
                free(final_value);
                final_value = filtered_value;

                free(filter_name);
            }

            while (*pos == ' ')
                pos++; // Skip whitespace
            if (strncmp(pos, "}}", 2) == 0)
            {
                pos += 2;

                size_t value_len = strlen(final_value);

                // Ensure buffer capacity
                while (buffer_pos + value_len >= buffer_size)
                {
                    buffer_size *= 2;
                    buffer = realloc(buffer, buffer_size);
                }

                strcpy(&buffer[buffer_pos], final_value);
                buffer_pos += value_len;
            }

            free(var_name);
            free(final_value);
        }
        else
        {
            // Regular character
            if (buffer_pos >= buffer_size - 1)
            {
                buffer_size *= 2;
                buffer = realloc(buffer, buffer_size);
            }
            buffer[buffer_pos++] = *pos++;
        }
    }

    buffer[buffer_pos] = '\0';
    return buffer;
}

// 80/20 Feature: Render with conditionals
char *cjinja_render_with_conditionals(const char *template_str, CJinjaContext *ctx)
{
    size_t buffer_size = INITIAL_BUFFER_SIZE;
    char *buffer = malloc(buffer_size);
    size_t buffer_pos = 0;

    const char *pos = template_str;

    while (*pos)
    {
        if (strncmp(pos, "{%", 2) == 0)
        {
            pos += 2;
            while (*pos == ' ')
                pos++; // Skip whitespace

            if (strncmp(pos, "if", 2) == 0)
            {
                pos += 2;
                while (*pos == ' ')
                    pos++;

                // Parse condition: {% if condition %}
                const char *cond_start = pos;
                while (*pos && *pos != ' ' && *pos != '%')
                    pos++;
                size_t cond_len = pos - cond_start;
                char *condition = malloc(cond_len + 1);
                strncpy(condition, cond_start, cond_len);
                condition[cond_len] = '\0';

                // Check condition
                char *value = get_var(ctx, condition);
                int condition_met = value && (strcmp(value, "true") == 0 || strlen(value) > 0);

                // Find end of if block
                while (*pos && strncmp(pos, "{% endif %}", 11) != 0)
                    pos++;

                // Get if body
                const char *body_start = pos;
                while (*pos && strncmp(pos, "{% endif %}", 11) != 0)
                    pos++;
                size_t body_len = pos - body_start;
                char *body = malloc(body_len + 1);
                strncpy(body, body_start, body_len);
                body[body_len] = '\0';

                // Render if body if condition is met
                if (condition_met)
                {
                    char *rendered_body = cjinja_render_string(body, ctx);
                    size_t rendered_len = strlen(rendered_body);

                    // Ensure buffer capacity
                    while (buffer_pos + rendered_len >= buffer_size)
                    {
                        buffer_size *= 2;
                        buffer = realloc(buffer, buffer_size);
                    }

                    strcpy(&buffer[buffer_pos], rendered_body);
                    buffer_pos += rendered_len;

                    free(rendered_body);
                }

                free(condition);
                free(body);

                if (strncmp(pos, "{% endif %}", 11) == 0)
                {
                    pos += 11;
                }
            }
            else
            {
                // Skip other control structures
                while (*pos && strncmp(pos, "%}", 2) != 0)
                    pos++;
                if (*pos)
                    pos += 2;
            }
        }
        else if (strncmp(pos, "{{", 2) == 0)
        {
            // Variable substitution
            pos += 2;
            while (*pos == ' ')
                pos++; // Skip whitespace

            const char *var_start = pos;
            while (*pos && *pos != ' ' && *pos != '}')
                pos++;

            size_t var_len = pos - var_start;
            char *var_name = malloc(var_len + 1);
            strncpy(var_name, var_start, var_len);
            var_name[var_len] = '\0';

            while (*pos == ' ')
                pos++; // Skip whitespace
            if (strncmp(pos, "}}", 2) == 0)
            {
                pos += 2;

                char *value = get_var(ctx, var_name);
                if (value)
                {
                    size_t value_len = strlen(value);

                    // Ensure buffer capacity
                    while (buffer_pos + value_len >= buffer_size)
                    {
                        buffer_size *= 2;
                        buffer = realloc(buffer, buffer_size);
                    }

                    strcpy(&buffer[buffer_pos], value);
                    buffer_pos += value_len;
                }
            }

            free(var_name);
        }
        else
        {
            // Regular character
            if (buffer_pos >= buffer_size - 1)
            {
                buffer_size *= 2;
                buffer = realloc(buffer, buffer_size);
            }
            buffer[buffer_pos++] = *pos++;
        }
    }

    buffer[buffer_pos] = '\0';
    return buffer;
}

char *cjinja_render_string(const char *template_str, CJinjaContext *ctx)
{
    if (!template_str || !ctx)
    {
        cjinja_set_error(CJINJA_ERROR_INVALID_VARIABLE, "Invalid template string or context");
        return NULL;
    }

    size_t buffer_size = INITIAL_BUFFER_SIZE;
    char *buffer = malloc(buffer_size);
    size_t buffer_pos = 0;

    const char *pos = template_str;

    while (*pos)
    {
        if (strncmp(pos, "{{", 2) == 0)
        {
            // Variable substitution
            pos += 2;
            while (*pos == ' ')
                pos++; // Skip whitespace

            const char *var_start = pos;
            while (*pos && *pos != ' ' && *pos != '}')
                pos++;

            size_t var_len = pos - var_start;
            char *var_name = malloc(var_len + 1);
            strncpy(var_name, var_start, var_len);
            var_name[var_len] = '\0';

            while (*pos == ' ')
                pos++; // Skip whitespace
            if (strncmp(pos, "}}", 2) == 0)
            {
                pos += 2;

                char *value = get_var(ctx, var_name);
                if (value)
                {
                    size_t value_len = strlen(value);

                    // Ensure buffer capacity
                    while (buffer_pos + value_len >= buffer_size)
                    {
                        buffer_size *= 2;
                        buffer = realloc(buffer, buffer_size);
                    }

                    strcpy(&buffer[buffer_pos], value);
                    buffer_pos += value_len;
                }
            }

            free(var_name);
        }
        else if (strncmp(pos, "{%", 2) == 0)
        {
            // Control structures (simplified for MVP)
            pos += 2;
            while (*pos && strncmp(pos, "%}", 2) != 0)
                pos++;
            if (*pos)
                pos += 2;
        }
        else
        {
            // Regular character
            if (buffer_pos >= buffer_size - 1)
            {
                buffer_size *= 2;
                buffer = realloc(buffer, buffer_size);
            }
            buffer[buffer_pos++] = *pos++;
        }
    }

    buffer[buffer_pos] = '\0';
    return buffer;
}

char *cjinja_render(CJinjaEngine *engine, const char *template_name, CJinjaContext *ctx)
{
    char path[4096];
    snprintf(path, sizeof(path), "%s/%s", engine->template_dir, template_name);

    FILE *f = fopen(path, "r");
    if (!f)
        return NULL;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *template_str = malloc(size + 1);
    fread(template_str, 1, size, f);
    template_str[size] = '\0';
    fclose(f);

    char *result = cjinja_render_string(template_str, ctx);
    free(template_str);

    return result;
}

// Utility functions
char *cjinja_escape_html(const char *input)
{
    size_t len = strlen(input);
    size_t escaped_len = len;

    // Count characters that need escaping
    for (size_t i = 0; i < len; i++)
    {
        switch (input[i])
        {
        case '<':
            escaped_len += 3;
            break; // &lt;
        case '>':
            escaped_len += 3;
            break; // &gt;
        case '&':
            escaped_len += 4;
            break; // &amp;
        case '"':
            escaped_len += 5;
            break; // &quot;
        }
    }

    char *result = malloc(escaped_len + 1);
    size_t j = 0;

    for (size_t i = 0; i < len; i++)
    {
        switch (input[i])
        {
        case '<':
            strcpy(result + j, "&lt;");
            j += 4;
            break;
        case '>':
            strcpy(result + j, "&gt;");
            j += 4;
            break;
        case '&':
            strcpy(result + j, "&amp;");
            j += 5;
            break;
        case '"':
            strcpy(result + j, "&quot;");
            j += 6;
            break;
        default:
            result[j++] = input[i];
        }
    }

    result[j] = '\0';
    return result;
}

char *cjinja_trim(const char *input)
{
    const char *start = input;
    const char *end = input + strlen(input) - 1;

    // Skip leading whitespace
    while (*start && isspace(*start))
        start++;

    // Skip trailing whitespace
    while (end > start && isspace(*end))
        end--;

    size_t len = end - start + 1;
    char *result = malloc(len + 1);
    strncpy(result, start, len);
    result[len] = '\0';

    return result;
}

int cjinja_is_empty(const char *input)
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

// 80/20 FEATURE: Advanced built-in filters
char *cjinja_filter_trim(const char *input, const char *args)
{
    if (!input)
        return strdup("");

    const char *start = input;
    const char *end = input + strlen(input) - 1;

    // Trim leading whitespace
    while (*start && isspace(*start))
        start++;

    // Trim trailing whitespace
    while (end > start && isspace(*end))
        end--;

    size_t len = end - start + 1;
    char *result = malloc(len + 1);
    strncpy(result, start, len);
    result[len] = '\0';

    return result;
}

char *cjinja_filter_replace(const char *input, const char *args)
{
    if (!input || !args)
        return strdup(input ? input : "");

    // Parse args: "old,new"
    char *args_copy = strdup(args);
    char *old_str = strtok(args_copy, ",");
    char *new_str = strtok(NULL, ",");

    if (!old_str || !new_str)
    {
        free(args_copy);
        return strdup(input);
    }

    size_t old_len = strlen(old_str);
    size_t new_len = strlen(new_str);
    size_t input_len = strlen(input);

    // Count occurrences
    size_t count = 0;
    const char *pos = input;
    while ((pos = strstr(pos, old_str)))
    {
        count++;
        pos += old_len;
    }

    // Allocate result buffer
    size_t result_len = input_len + count * (new_len - old_len);
    char *result = malloc(result_len + 1);
    char *write_pos = result;

    // Perform replacement
    pos = input;
    while (*pos)
    {
        if (strncmp(pos, old_str, old_len) == 0)
        {
            strcpy(write_pos, new_str);
            write_pos += new_len;
            pos += old_len;
        }
        else
        {
            *write_pos++ = *pos++;
        }
    }
    *write_pos = '\0';

    free(args_copy);
    return result;
}

char *cjinja_filter_slice(const char *input, const char *args)
{
    if (!input || !args)
        return strdup(input ? input : "");

    int start = 0, end = strlen(input);
    sscanf(args, "%d,%d", &start, &end);

    if (start < 0)
        start = strlen(input) + start;
    if (end < 0)
        end = strlen(input) + end;
    if (start < 0)
        start = 0;
    if (end > (int)strlen(input))
        end = strlen(input);
    if (start >= end)
        return strdup("");

    size_t len = end - start;
    char *result = malloc(len + 1);
    strncpy(result, input + start, len);
    result[len] = '\0';

    return result;
}

char *cjinja_filter_default(const char *input, const char *args)
{
    if (input && strlen(input) > 0)
    {
        return strdup(input);
    }
    return strdup(args ? args : "");
}

char *cjinja_filter_join(const char *input, const char *args)
{
    if (!input)
        return strdup("");

    // For arrays stored as comma-separated strings
    if (strchr(input, ','))
    {
        char *result = strdup(input);
        char *pos = result;
        while (*pos)
        {
            if (*pos == ',')
                *pos = args ? args[0] : ' ';
            pos++;
        }
        return result;
    }

    return strdup(input);
}

char *cjinja_filter_split(const char *input, const char *args)
{
    if (!input)
        return strdup("");

    char delimiter = args && args[0] ? args[0] : ',';
    size_t count = 1;
    const char *pos = input;

    // Count delimiters
    while (*pos)
    {
        if (*pos == delimiter)
            count++;
        pos++;
    }

    // Return count as string (useful for length operations)
    char result[32];
    snprintf(result, sizeof(result), "%zu", count);
    return strdup(result);
}

// 80/20 FEATURE: Enhanced template caching
void cjinja_clear_cache(CJinjaEngine *engine)
{
    if (!engine || !engine->template_cache)
        return;

    TemplateCache *cache = (TemplateCache *)engine->template_cache;
    for (size_t i = 0; i < cache->count; i++)
    {
        free(cache->entries[i].name);
        free(cache->entries[i].content);
        if (cache->entries[i].compiled_template)
        {
            free(cache->entries[i].compiled_template);
        }
    }
    cache->count = 0;
    cache_hits = 0;
    cache_misses = 0;
}

void cjinja_set_cache_size(CJinjaEngine *engine, size_t max_entries)
{
    if (!engine || !engine->template_cache)
        return;

    TemplateCache *cache = (TemplateCache *)engine->template_cache;
    if (max_entries < cache->count)
    {
        // Remove excess entries (LRU-style)
        for (size_t i = max_entries; i < cache->count; i++)
        {
            free(cache->entries[i].name);
            free(cache->entries[i].content);
            if (cache->entries[i].compiled_template)
            {
                free(cache->entries[i].compiled_template);
            }
        }
        cache->count = max_entries;
    }
    cache->max_entries = max_entries;
}

size_t cjinja_get_cache_stats(CJinjaEngine *engine, size_t *hits, size_t *misses)
{
    if (hits)
        *hits = cache_hits;
    if (misses)
        *misses = cache_misses;
    return cache_hits + cache_misses;
}

// 80/20 FEATURE: Template inheritance
CJinjaInheritanceContext *cjinja_create_inheritance_context(void)
{
    CJinjaInheritanceContext *ctx = malloc(sizeof(CJinjaInheritanceContext));
    if (!ctx)
    {
        cjinja_set_error(CJINJA_ERROR_MEMORY, "Failed to allocate inheritance context");
        return NULL;
    }

    ctx->base_template = NULL;
    ctx->block_names = malloc(INITIAL_CONTEXT_SIZE * sizeof(char *));
    ctx->block_contents = malloc(INITIAL_CONTEXT_SIZE * sizeof(char *));
    ctx->block_count = 0;
    ctx->block_capacity = INITIAL_CONTEXT_SIZE;

    if (!ctx->block_names || !ctx->block_contents)
    {
        cjinja_set_error(CJINJA_ERROR_MEMORY, "Failed to allocate inheritance blocks");
        cjinja_destroy_inheritance_context(ctx);
        return NULL;
    }

    return ctx;
}

void cjinja_destroy_inheritance_context(CJinjaInheritanceContext *ctx)
{
    if (!ctx)
        return;

    free(ctx->base_template);
    for (size_t i = 0; i < ctx->block_count; i++)
    {
        free(ctx->block_names[i]);
        free(ctx->block_contents[i]);
    }
    free(ctx->block_names);
    free(ctx->block_contents);
    free(ctx);
}

void cjinja_set_base_template(CJinjaInheritanceContext *ctx, const char *base_template)
{
    if (!ctx)
        return;
    free(ctx->base_template);
    ctx->base_template = strdup(base_template);
}

void cjinja_add_block(CJinjaInheritanceContext *ctx, const char *block_name, const char *content)
{
    if (!ctx || !block_name || !content)
        return;

    // Expand capacity if needed
    if (ctx->block_count >= ctx->block_capacity)
    {
        ctx->block_capacity *= 2;
        ctx->block_names = realloc(ctx->block_names, ctx->block_capacity * sizeof(char *));
        ctx->block_contents = realloc(ctx->block_contents, ctx->block_capacity * sizeof(char *));
    }

    ctx->block_names[ctx->block_count] = strdup(block_name);
    ctx->block_contents[ctx->block_count] = strdup(content);
    ctx->block_count++;
}

char *cjinja_render_with_inheritance(const char *template_str, CJinjaContext *ctx, CJinjaInheritanceContext *inherit_ctx)
{
    if (!template_str || !ctx || !inherit_ctx)
    {
        cjinja_set_error(CJINJA_ERROR_INVALID_VARIABLE, "Invalid parameters for inheritance");
        return NULL;
    }

    // First render the child template to extract blocks
    char *child_rendered = cjinja_render_string(template_str, ctx);
    if (!child_rendered)
        return NULL;

    // If no base template, return child as-is
    if (!inherit_ctx->base_template)
    {
        return child_rendered;
    }

    // Render base template with block substitutions
    char *base_rendered = cjinja_render_string(inherit_ctx->base_template, ctx);
    if (!base_rendered)
    {
        free(child_rendered);
        return NULL;
    }

    // Replace blocks in base template
    char *result = strdup(base_rendered);
    for (size_t i = 0; i < inherit_ctx->block_count; i++)
    {
        char block_pattern[256];
        snprintf(block_pattern, sizeof(block_pattern), "{{%% block %s %%}}", inherit_ctx->block_names[i]);

        char *pos = strstr(result, block_pattern);
        if (pos)
        {
            // Find end of block
            char *end_pattern = strstr(pos, "{{% endblock %}}");
            if (end_pattern)
            {
                size_t start_len = pos - result;
                size_t end_len = strlen(end_pattern + 15);
                size_t total_len = start_len + strlen(inherit_ctx->block_contents[i]) + end_len;

                char *new_result = malloc(total_len + 1);
                strncpy(new_result, result, start_len);
                strcpy(new_result + start_len, inherit_ctx->block_contents[i]);
                strcpy(new_result + start_len + strlen(inherit_ctx->block_contents[i]), end_pattern + 15);

                free(result);
                result = new_result;
            }
        }
    }

    free(child_rendered);
    free(base_rendered);
    return result;
}

// 80/20 FEATURE: Include statements
char *cjinja_load_template_file(CJinjaEngine *engine, const char *template_name)
{
    if (!engine || !template_name)
        return NULL;

    // Build full path
    char full_path[1024];
    snprintf(full_path, sizeof(full_path), "%s/%s", engine->template_dir, template_name);

    FILE *file = fopen(full_path, "r");
    if (!file)
    {
        cjinja_set_error(CJINJA_ERROR_TEMPLATE_NOT_FOUND, full_path);
        return NULL;
    }

    // Read file content
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *content = malloc(file_size + 1);
    fread(content, 1, file_size, file);
    content[file_size] = '\0';

    fclose(file);
    return content;
}

char *cjinja_render_with_includes(CJinjaEngine *engine, const char *template_str, CJinjaContext *ctx)
{
    if (!engine || !template_str || !ctx)
        return NULL;

    size_t buffer_size = INITIAL_BUFFER_SIZE;
    char *buffer = malloc(buffer_size);
    size_t buffer_pos = 0;

    const char *pos = template_str;

    while (*pos)
    {
        if (strncmp(pos, "{%", 2) == 0)
        {
            pos += 2;
            while (*pos == ' ')
                pos++;

            if (strncmp(pos, "include", 7) == 0)
            {
                pos += 7;
                while (*pos == ' ')
                    pos++;

                // Parse include filename
                const char *filename_start = pos;
                while (*pos && *pos != ' ' && *pos != '%')
                    pos++;
                size_t filename_len = pos - filename_start;
                char *filename = malloc(filename_len + 1);
                strncpy(filename, filename_start, filename_len);
                filename[filename_len] = '\0';

                // Load and render included template
                char *included_content = cjinja_load_template_file(engine, filename);
                if (included_content)
                {
                    char *rendered_include = cjinja_render_string(included_content, ctx);
                    if (rendered_include)
                    {
                        size_t rendered_len = strlen(rendered_include);
                        while (buffer_pos + rendered_len >= buffer_size)
                        {
                            buffer_size *= 2;
                            buffer = realloc(buffer, buffer_size);
                        }
                        strcpy(&buffer[buffer_pos], rendered_include);
                        buffer_pos += rendered_len;
                        free(rendered_include);
                    }
                    free(included_content);
                }

                free(filename);

                // Skip to end of include block
                while (*pos && strncmp(pos, "%}", 2) != 0)
                    pos++;
                if (*pos)
                    pos += 2;
            }
            else
            {
                // Skip other control structures
                while (*pos && strncmp(pos, "%}", 2) != 0)
                    pos++;
                if (*pos)
                    pos += 2;
            }
        }
        else
        {
            // Copy regular text
            if (buffer_pos >= buffer_size - 1)
            {
                buffer_size *= 2;
                buffer = realloc(buffer, buffer_size);
            }
            buffer[buffer_pos++] = *pos++;
        }
    }

    buffer[buffer_pos] = '\0';
    return buffer;
}

// 80/20 FEATURE: Advanced utility functions
char *cjinja_safe_string(const char *input)
{
    if (!input)
        return strdup("");
    return strdup(input);
}

int cjinja_string_equals(const char *a, const char *b)
{
    if (!a && !b)
        return 1;
    if (!a || !b)
        return 0;
    return strcmp(a, b) == 0;
}

char *cjinja_concat_strings(const char *a, const char *b)
{
    if (!a && !b)
        return strdup("");
    if (!a)
        return strdup(b);
    if (!b)
        return strdup(a);

    size_t len = strlen(a) + strlen(b);
    char *result = malloc(len + 1);
    strcpy(result, a);
    strcat(result, b);
    return result;
}

char *cjinja_format_number(double number, int precision)
{
    char result[64];
    snprintf(result, sizeof(result), "%.*f", precision, number);
    return strdup(result);
}

// 80/20 FEATURE: Template compilation for performance
CJinjaCompiledTemplate *cjinja_compile_template(const char *template_str)
{
    if (!template_str)
    {
        cjinja_set_error(CJINJA_ERROR_INVALID_VARIABLE, "Invalid template string");
        return NULL;
    }

    CJinjaCompiledTemplate *compiled = malloc(sizeof(CJinjaCompiledTemplate));
    if (!compiled)
    {
        cjinja_set_error(CJINJA_ERROR_MEMORY, "Failed to allocate compiled template");
        return NULL;
    }

    // For now, just store the template string
    // In a full implementation, this would parse and optimize the template
    compiled->compiled_template = strdup(template_str);
    compiled->size = strlen(template_str);

    return compiled;
}

char *cjinja_render_compiled(CJinjaCompiledTemplate *compiled, CJinjaContext *ctx)
{
    if (!compiled || !ctx)
    {
        cjinja_set_error(CJINJA_ERROR_INVALID_VARIABLE, "Invalid compiled template or context");
        return NULL;
    }

    // For now, just render the stored template string
    // In a full implementation, this would use the optimized structure
    return cjinja_render_string((char *)compiled->compiled_template, ctx);
}

void cjinja_destroy_compiled_template(CJinjaCompiledTemplate *compiled)
{
    if (!compiled)
        return;
    free(compiled->compiled_template);
    free(compiled);
}

// 80/20 FEATURE: Batch rendering for high throughput
CJinjaBatchRender *cjinja_create_batch_render(size_t count)
{
    CJinjaBatchRender *batch = malloc(sizeof(CJinjaBatchRender));
    if (!batch)
    {
        cjinja_set_error(CJINJA_ERROR_MEMORY, "Failed to allocate batch render");
        return NULL;
    }

    batch->templates = malloc(count * sizeof(char *));
    batch->results = malloc(count * sizeof(char *));
    batch->count = count;

    if (!batch->templates || !batch->results)
    {
        cjinja_set_error(CJINJA_ERROR_MEMORY, "Failed to allocate batch arrays");
        cjinja_destroy_batch_render(batch);
        return NULL;
    }

    // Initialize results to NULL
    for (size_t i = 0; i < count; i++)
    {
        batch->results[i] = NULL;
    }

    return batch;
}

void cjinja_destroy_batch_render(CJinjaBatchRender *batch)
{
    if (!batch)
        return;

    // Free results
    for (size_t i = 0; i < batch->count; i++)
    {
        free(batch->results[i]);
    }

    free(batch->templates);
    free(batch->results);
    free(batch);
}

int cjinja_render_batch(CJinjaEngine *engine, CJinjaBatchRender *batch, CJinjaContext *ctx)
{
    if (!engine || !batch || !ctx)
    {
        cjinja_set_error(CJINJA_ERROR_INVALID_VARIABLE, "Invalid batch render parameters");
        return -1;
    }

    // Clear previous results
    for (size_t i = 0; i < batch->count; i++)
    {
        free(batch->results[i]);
        batch->results[i] = NULL;
    }

    // Render all templates
    for (size_t i = 0; i < batch->count; i++)
    {
        if (batch->templates[i])
        {
            batch->results[i] = cjinja_render_string(batch->templates[i], ctx);
            if (!batch->results[i])
            {
                // If any template fails, clean up and return error
                for (size_t j = 0; j < batch->count; j++)
                {
                    free(batch->results[j]);
                    batch->results[j] = NULL;
                }
                return -1;
            }
        }
    }

    return 0;
}

// 80/20 FEATURE: Enhanced loop performance optimization
char *cjinja_render_with_loops_optimized(const char *template_str, CJinjaContext *ctx)
{
    if (!template_str || !ctx)
        return NULL;

    size_t buffer_size = INITIAL_BUFFER_SIZE;
    char *buffer = malloc(buffer_size);
    size_t buffer_pos = 0;

    const char *pos = template_str;

    while (*pos)
    {
        if (strncmp(pos, "{%", 2) == 0)
        {
            pos += 2;
            while (*pos == ' ')
                pos++;

            if (strncmp(pos, "for", 3) == 0)
            {
                pos += 3;
                while (*pos == ' ')
                    pos++;

                // Parse for loop: {% for item in items %}
                const char *var_start = pos;
                while (*pos && *pos != ' ')
                    pos++;
                size_t var_len = pos - var_start;
                char *var_name = malloc(var_len + 1);
                strncpy(var_name, var_start, var_len);
                var_name[var_len] = '\0';

                // Skip "in"
                while (*pos && strncmp(pos, "in", 2) != 0)
                    pos++;
                if (strncmp(pos, "in", 2) == 0)
                    pos += 2;
                while (*pos == ' ')
                    pos++;

                // Get array name
                const char *array_start = pos;
                while (*pos && *pos != ' ' && *pos != '%')
                    pos++;
                size_t array_len = pos - array_start;
                char *array_name = malloc(array_len + 1);
                strncpy(array_name, array_start, array_len);
                array_name[array_len] = '\0';

                // Skip to end of {% for ... %} block
                while (*pos && strncmp(pos, "%}", 2) != 0)
                    pos++;
                if (strncmp(pos, "%}", 2) == 0)
                    pos += 2;

                // Capture loop body start
                const char *body_start = pos;

                // Find end of for block
                while (*pos && strncmp(pos, "{% endfor %}", 12) != 0)
                    pos++;
                size_t body_len = pos - body_start;
                char *body = malloc(body_len + 1);
                strncpy(body, body_start, body_len);
                body[body_len] = '\0';

                // Get array items - OPTIMIZED: Use direct array access if available
                char *array_str = get_var(ctx, array_name);
                if (array_str)
                {
                    // Check if this is a direct array (not comma-separated)
                    // For now, use the existing comma-separated approach but optimize
                    char *items[1000]; // Increased max items
                    size_t item_count = 0;
                    char *token = strtok(strdup(array_str), ",");
                    while (token && item_count < 1000)
                    {
                        // Trim whitespace from token
                        while (*token == ' ')
                            token++;
                        char *end = token + strlen(token) - 1;
                        while (end > token && *end == ' ')
                            end--;
                        *(end + 1) = '\0';

                        items[item_count++] = strdup(token);
                        token = strtok(NULL, ",");
                    }

                    // Pre-allocate buffer for all iterations
                    size_t estimated_size = body_len * item_count + buffer_pos;
                    if (estimated_size > buffer_size)
                    {
                        buffer_size = estimated_size * 2;
                        buffer = realloc(buffer, buffer_size);
                    }

                    // Render loop body for each item - OPTIMIZED
                    for (size_t i = 0; i < item_count; i++)
                    {
                        // Create temporary context for loop variable
                        CJinjaContext *temp_ctx = cjinja_create_context();

                        // Copy all variables from original context
                        for (size_t j = 0; j < ctx->count; j++)
                        {
                            cjinja_set_var(temp_ctx, ctx->keys[j], ctx->values[j]);
                        }

                        // Set loop variable
                        cjinja_set_var(temp_ctx, var_name, items[i]);

                        // Render loop body
                        char *rendered_body = cjinja_render_string(body, temp_ctx);
                        if (rendered_body)
                        {
                            size_t rendered_len = strlen(rendered_body);

                            // Ensure buffer capacity
                            while (buffer_pos + rendered_len >= buffer_size)
                            {
                                buffer_size *= 2;
                                buffer = realloc(buffer, buffer_size);
                            }

                            strcpy(&buffer[buffer_pos], rendered_body);
                            buffer_pos += rendered_len;
                            free(rendered_body);
                        }

                        cjinja_destroy_context(temp_ctx);
                        free(items[i]);
                    }
                }

                free(var_name);
                free(array_name);
                free(body);

                if (strncmp(pos, "{% endfor %}", 12) == 0)
                {
                    pos += 12;
                }
            }
            else
            {
                // Skip other control structures
                while (*pos && strncmp(pos, "%}", 2) != 0)
                    pos++;
                if (*pos)
                    pos += 2;
            }
        }
        else if (strncmp(pos, "{{", 2) == 0)
        {
            // Variable substitution with filter support
            pos += 2;
            while (*pos == ' ')
                pos++;

            const char *var_start = pos;
            while (*pos && *pos != ' ' && *pos != '|' && *pos != '}')
                pos++;

            size_t var_len = pos - var_start;
            char *var_name = malloc(var_len + 1);
            strncpy(var_name, var_start, var_len);
            var_name[var_len] = '\0';

            char *value = get_var(ctx, var_name);
            char *final_value = value ? strdup(value) : strdup("");

            // Check for filters
            while (*pos == ' ')
                pos++;
            if (*pos == '|')
            {
                pos++; // Skip |
                while (*pos == ' ')
                    pos++;

                const char *filter_start = pos;
                while (*pos && *pos != ' ' && *pos != '}')
                    pos++;
                size_t filter_len = pos - filter_start;
                char *filter_name = malloc(filter_len + 1);
                strncpy(filter_name, filter_start, filter_len);
                filter_name[filter_len] = '\0';

                // Apply filter
                char *filtered_value = cjinja_apply_filter(filter_name, final_value, "");
                free(final_value);
                final_value = filtered_value;

                free(filter_name);
            }

            while (*pos == ' ')
                pos++; // Skip whitespace

            if (*pos == '}')
            {
                pos++; // Skip }
                if (*pos == '}')
                {
                    pos++; // Skip second }

                    // Add to buffer
                    size_t value_len = strlen(final_value);
                    while (buffer_pos + value_len >= buffer_size)
                    {
                        buffer_size *= 2;
                        buffer = realloc(buffer, buffer_size);
                    }
                    strcpy(&buffer[buffer_pos], final_value);
                    buffer_pos += value_len;
                }
            }

            free(var_name);
            free(final_value);
        }
        else
        {
            // Copy regular text
            if (buffer_pos >= buffer_size - 1)
            {
                buffer_size *= 2;
                buffer = realloc(buffer, buffer_size);
            }
            buffer[buffer_pos++] = *pos++;
        }
    }

    buffer[buffer_pos] = '\0';
    return buffer;
}

// 7-TICK OPTIMIZED: Ultra-fast path for basic variable substitution only
char *cjinja_render_string_7tick(const char *template_str, CJinjaContext *ctx)
{
    if (!template_str || !ctx)
    {
        return NULL;
    }

    // Pre-allocate buffer with estimated size
    size_t template_len = strlen(template_str);
    size_t buffer_size = template_len * 2; // Estimate 2x for variable expansion
    char *buffer = malloc(buffer_size);
    size_t buffer_pos = 0;

    const char *pos = template_str;

    while (*pos)
    {
        if (strncmp(pos, "{{", 2) == 0)
        {
            pos += 2;

            // Skip whitespace efficiently
            while (*pos == ' ')
                pos++;

            const char *var_start = pos;
            while (*pos && *pos != ' ' && *pos != '|' && *pos != '}')
                pos++;

            size_t var_len = pos - var_start;

            // Fast variable lookup without malloc
            char *value = NULL;
            for (size_t i = 0; i < ctx->count; i++)
            {
                if (strlen(ctx->keys[i]) == var_len && strncmp(ctx->keys[i], var_start, var_len) == 0)
                {
                    value = ctx->values[i];
                    break;
                }
            }

            // Skip any filters or additional processing
            while (*pos && *pos != '}')
                pos++;
            if (*pos == '}')
            {
                pos++;
                if (*pos == '}')
                {
                    pos++;

                    // Direct copy without malloc
                    if (value)
                    {
                        size_t value_len = strlen(value);
                        if (buffer_pos + value_len >= buffer_size)
                        {
                            buffer_size = (buffer_pos + value_len) * 2;
                            buffer = realloc(buffer, buffer_size);
                        }
                        memcpy(&buffer[buffer_pos], value, value_len);
                        buffer_pos += value_len;
                    }
                }
            }
        }
        else
        {
            // Direct character copy
            if (buffer_pos >= buffer_size - 1)
            {
                buffer_size *= 2;
                buffer = realloc(buffer, buffer_size);
            }
            buffer[buffer_pos++] = *pos++;
        }
    }

    buffer[buffer_pos] = '\0';
    return buffer;
}

// 7-TICK OPTIMIZED: Ultra-fast path for basic conditionals only
char *cjinja_render_conditionals_7tick(const char *template_str, CJinjaContext *ctx)
{
    if (!template_str || !ctx)
    {
        return NULL;
    }

    size_t template_len = strlen(template_str);
    size_t buffer_size = template_len * 2;
    char *buffer = malloc(buffer_size);
    size_t buffer_pos = 0;

    const char *pos = template_str;

    while (*pos)
    {
        if (strncmp(pos, "{%", 2) == 0)
        {
            pos += 2;
            while (*pos == ' ')
                pos++;

            if (strncmp(pos, "if", 2) == 0)
            {
                pos += 2;
                while (*pos == ' ')
                    pos++;

                const char *cond_start = pos;
                while (*pos && *pos != ' ' && *pos != '%')
                    pos++;
                size_t cond_len = pos - cond_start;

                // Fast boolean check
                int condition_met = 0;
                for (size_t i = 0; i < ctx->count; i++)
                {
                    if (strlen(ctx->keys[i]) == cond_len && strncmp(ctx->keys[i], cond_start, cond_len) == 0)
                    {
                        condition_met = ctx->values[i] && strlen(ctx->values[i]) > 0;
                        break;
                    }
                }

                // Skip to endif
                while (*pos && strncmp(pos, "{% endif %}", 11) != 0)
                    pos++;

                if (condition_met)
                {
                    const char *body_start = pos;
                    while (*pos && strncmp(pos, "{% endif %}", 11) != 0)
                        pos++;
                    size_t body_len = pos - body_start;

                    // Render body with 7-tick path
                    char *body = malloc(body_len + 1);
                    strncpy(body, body_start, body_len);
                    body[body_len] = '\0';

                    char *rendered_body = cjinja_render_string_7tick(body, ctx);
                    if (rendered_body)
                    {
                        size_t rendered_len = strlen(rendered_body);
                        if (buffer_pos + rendered_len >= buffer_size)
                        {
                            buffer_size = (buffer_pos + rendered_len) * 2;
                            buffer = realloc(buffer, buffer_size);
                        }
                        memcpy(&buffer[buffer_pos], rendered_body, rendered_len);
                        buffer_pos += rendered_len;
                        free(rendered_body);
                    }
                    free(body);
                }

                if (strncmp(pos, "{% endif %}", 11) == 0)
                {
                    pos += 11;
                }
            }
            else
            {
                while (*pos && strncmp(pos, "%}", 2) != 0)
                    pos++;
                if (*pos)
                    pos += 2;
            }
        }
        else if (strncmp(pos, "{{", 2) == 0)
        {
            // Use 7-tick variable substitution
            pos += 2;
            while (*pos == ' ')
                pos++;

            const char *var_start = pos;
            while (*pos && *pos != ' ' && *pos != '|' && *pos != '}')
                pos++;

            size_t var_len = pos - var_start;

            char *value = NULL;
            for (size_t i = 0; i < ctx->count; i++)
            {
                if (strlen(ctx->keys[i]) == var_len && strncmp(ctx->keys[i], var_start, var_len) == 0)
                {
                    value = ctx->values[i];
                    break;
                }
            }

            while (*pos && *pos != '}')
                pos++;
            if (*pos == '}')
            {
                pos++;
                if (*pos == '}')
                {
                    pos++;

                    if (value)
                    {
                        size_t value_len = strlen(value);
                        if (buffer_pos + value_len >= buffer_size)
                        {
                            buffer_size = (buffer_pos + value_len) * 2;
                            buffer = realloc(buffer, buffer_size);
                        }
                        memcpy(&buffer[buffer_pos], value, value_len);
                        buffer_pos += value_len;
                    }
                }
            }
        }
        else
        {
            if (buffer_pos >= buffer_size - 1)
            {
                buffer_size *= 2;
                buffer = realloc(buffer, buffer_size);
            }
            buffer[buffer_pos++] = *pos++;
        }
    }

    buffer[buffer_pos] = '\0';
    return buffer;
}
