#include "cjinja.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#define INITIAL_CONTEXT_SIZE 16
#define INITIAL_BUFFER_SIZE 4096
#define MAX_FILTERS 32
#define MAX_TEMPLATE_CACHE 64

// Filter registry
static struct
{
    char *names[MAX_FILTERS];
    CJinjaFilter functions[MAX_FILTERS];
    size_t count;
} filter_registry = {0};

// Template cache entry
typedef struct
{
    char *name;
    char *content;
    size_t hash;
} TemplateCacheEntry;

// Template cache
typedef struct
{
    TemplateCacheEntry entries[MAX_TEMPLATE_CACHE];
    size_t count;
} TemplateCache;

CJinjaEngine *cjinja_create(const char *template_dir)
{
    CJinjaEngine *engine = malloc(sizeof(CJinjaEngine));
    engine->template_dir = strdup(template_dir);
    engine->cache_enabled = 0;
    engine->template_cache = malloc(sizeof(TemplateCache));
    memset(engine->template_cache, 0, sizeof(TemplateCache));

    // Register built-in filters
    cjinja_register_filter("upper", cjinja_filter_upper);
    cjinja_register_filter("lower", cjinja_filter_lower);
    cjinja_register_filter("capitalize", cjinja_filter_capitalize);
    cjinja_register_filter("length", cjinja_filter_length);

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

static char *get_var(CJinjaContext *ctx, const char *key)
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
                while (*pos && *pos != ' ' && *pos != 'i')
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

                // Find end of for block
                while (*pos && strncmp(pos, "{% endfor %}", 12) != 0)
                    pos++;

                // Get loop body
                const char *body_start = pos;
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
