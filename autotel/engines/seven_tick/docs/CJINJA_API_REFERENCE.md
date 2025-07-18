# CJinja API Reference

## Overview

CJinja is a high-performance C template engine that provides Jinja2-like syntax with sub-microsecond rendering performance. This document provides a complete API reference for the enhanced CJinja implementation.

## Table of Contents

1. [Core API](#core-api)
2. [Template Rendering](#template-rendering)
3. [Variable Management](#variable-management)
4. [Control Structures](#control-structures)
5. [Filter System](#filter-system)
6. [Performance Features](#performance-features)
7. [Utility Functions](#utility-functions)
8. [Error Handling](#error-handling)
9. [Examples](#examples)

## Core API

### Engine Management

#### `CJinjaEngine* cjinja_create(const char* template_dir)`
Creates a new CJinja engine instance.

**Parameters:**
- `template_dir`: Directory path for template files

**Returns:**
- Pointer to CJinjaEngine instance, or NULL on failure

**Example:**
```c
CJinjaEngine* engine = cjinja_create("./templates");
if (!engine) {
    // Handle error
}
```

#### `void cjinja_destroy(CJinjaEngine* engine)`
Destroys a CJinja engine instance and frees all associated memory.

**Parameters:**
- `engine`: CJinjaEngine instance to destroy

**Example:**
```c
cjinja_destroy(engine);
```

### Context Management

#### `CJinjaContext* cjinja_create_context(void)`
Creates a new template context for variable storage.

**Returns:**
- Pointer to CJinjaContext instance

**Example:**
```c
CJinjaContext* ctx = cjinja_create_context();
```

#### `void cjinja_destroy_context(CJinjaContext* ctx)`
Destroys a template context and frees all associated memory.

**Parameters:**
- `ctx`: CJinjaContext instance to destroy

**Example:**
```c
cjinja_destroy_context(ctx);
```

## Variable Management

### Basic Variables

#### `void cjinja_set_var(CJinjaContext* ctx, const char* key, const char* value)`
Sets a string variable in the template context.

**Parameters:**
- `ctx`: Template context
- `key`: Variable name
- `value`: Variable value

**Example:**
```c
cjinja_set_var(ctx, "user", "John Doe");
cjinja_set_var(ctx, "title", "Welcome Page");
```

#### `char* get_var(CJinjaContext* ctx, const char* key)`
Retrieves a variable value from the template context.

**Parameters:**
- `ctx`: Template context
- `key`: Variable name

**Returns:**
- Variable value string, or NULL if not found

**Example:**
```c
char* value = get_var(ctx, "user");
if (value) {
    printf("User: %s\n", value);
}
```

### Boolean Variables

#### `void cjinja_set_bool(CJinjaContext* ctx, const char* key, int value)`
Sets a boolean variable in the template context.

**Parameters:**
- `ctx`: Template context
- `key`: Variable name
- `value`: Boolean value (0 = false, non-zero = true)

**Example:**
```c
cjinja_set_bool(ctx, "is_admin", 1);
cjinja_set_bool(ctx, "show_debug", 0);
```

### Array Variables

#### `void cjinja_set_array(CJinjaContext* ctx, const char* key, char** items, size_t count)`
Sets an array variable in the template context.

**Parameters:**
- `ctx`: Template context
- `key`: Variable name
- `items`: Array of string pointers
- `count`: Number of items in array

**Example:**
```c
char* fruits[] = {"apple", "banana", "cherry"};
cjinja_set_array(ctx, "fruits", fruits, 3);
```

## Template Rendering

### Basic Rendering

#### `char* cjinja_render_string(const char* template_str, CJinjaContext* ctx)`
Renders a template string with variable substitution.

**Parameters:**
- `template_str`: Template string to render
- `ctx`: Template context

**Returns:**
- Rendered string (caller must free), or NULL on error

**Example:**
```c
const char* template = "Hello {{user}}, welcome to {{title}}!";
char* result = cjinja_render_string(template, ctx);
printf("%s\n", result);
free(result);
```

#### `char* cjinja_render(CJinjaEngine* engine, const char* template_name, CJinjaContext* ctx)`
Renders a template file with variable substitution.

**Parameters:**
- `engine`: CJinjaEngine instance
- `template_name`: Name of template file
- `ctx`: Template context

**Returns:**
- Rendered string (caller must free), or NULL on error

**Example:**
```c
char* result = cjinja_render(engine, "welcome.html", ctx);
if (result) {
    printf("%s\n", result);
    free(result);
}
```

### Advanced Rendering

#### `char* cjinja_render_with_conditionals(const char* template_str, CJinjaContext* ctx)`
Renders a template with conditional logic support.

**Parameters:**
- `template_str`: Template string with conditionals
- `ctx`: Template context

**Returns:**
- Rendered string (caller must free), or NULL on error

**Example:**
```c
const char* template = "{% if is_admin %}Welcome admin {{user}}!{% endif %}";
char* result = cjinja_render_with_conditionals(template, ctx);
```

#### `char* cjinja_render_with_loops(const char* template_str, CJinjaContext* ctx)`
Renders a template with loop and filter support.

**Parameters:**
- `template_str`: Template string with loops and filters
- `ctx`: Template context

**Returns:**
- Rendered string (caller must free), or NULL on error

**Example:**
```c
const char* template = "{% for fruit in fruits %}- {{fruit}}\n{% endfor %}";
char* result = cjinja_render_with_loops(template, ctx);
```

## Control Structures

### Conditionals

**Syntax:**
```
{% if condition %}
    content
{% endif %}
```

**Example:**
```c
const char* template = 
    "{% if is_admin %}"
    "Welcome admin {{user}}!\n"
    "{% endif %}"
    "Regular user: {{user}}";
```

### Loops

**Syntax:**
```
{% for item in items %}
    content
{% endfor %}
```

**Example:**
```c
const char* template = 
    "Fruits:\n"
    "{% for fruit in fruits %}"
    "  - {{fruit}}\n"
    "{% endfor %}"
    "Total: {{fruits | length}} fruits";
```

## Filter System

### Built-in Filters

#### `char* cjinja_filter_upper(const char* input, const char* args)`
Converts string to uppercase.

**Parameters:**
- `input`: Input string
- `args`: Filter arguments (unused for this filter)

**Returns:**
- Uppercase string (caller must free)

**Example:**
```c
char* result = cjinja_filter_upper("hello world", "");
// Returns: "HELLO WORLD"
free(result);
```

#### `char* cjinja_filter_lower(const char* input, const char* args)`
Converts string to lowercase.

**Parameters:**
- `input`: Input string
- `args`: Filter arguments (unused for this filter)

**Returns:**
- Lowercase string (caller must free)

**Example:**
```c
char* result = cjinja_filter_lower("HELLO WORLD", "");
// Returns: "hello world"
free(result);
```

#### `char* cjinja_filter_capitalize(const char* input, const char* args)`
Capitalizes the first character of the string.

**Parameters:**
- `input`: Input string
- `args`: Filter arguments (unused for this filter)

**Returns:**
- Capitalized string (caller must free)

**Example:**
```c
char* result = cjinja_filter_capitalize("hello world", "");
// Returns: "Hello world"
free(result);
```

#### `char* cjinja_filter_length(const char* input, const char* args)`
Returns the length of the string as a string.

**Parameters:**
- `input`: Input string
- `args`: Filter arguments (unused for this filter)

**Returns:**
- Length as string (caller must free)

**Example:**
```c
char* result = cjinja_filter_length("hello", "");
// Returns: "5"
free(result);
```

### Custom Filters

#### `void cjinja_register_filter(const char* name, CJinjaFilter filter)`
Registers a custom filter function.

**Parameters:**
- `name`: Filter name
- `filter`: Filter function pointer

**Example:**
```c
char* my_filter(const char* input, const char* args) {
    // Custom filter implementation
    return strdup(input);
}

cjinja_register_filter("my_filter", my_filter);
```

#### `char* cjinja_apply_filter(const char* filter_name, const char* input, const char* args)`
Applies a filter to input string.

**Parameters:**
- `filter_name`: Name of filter to apply
- `input`: Input string
- `args`: Filter arguments

**Returns:**
- Filtered string (caller must free), or original string if filter not found

**Example:**
```c
char* result = cjinja_apply_filter("upper", "hello", "");
// Returns: "HELLO"
free(result);
```

## Performance Features

### Template Caching

#### `void cjinja_enable_cache(CJinjaEngine* engine, int enabled)`
Enables or disables template caching.

**Parameters:**
- `engine`: CJinjaEngine instance
- `enabled`: 1 to enable, 0 to disable

**Example:**
```c
cjinja_enable_cache(engine, 1);  // Enable caching
```

#### `char* cjinja_render_cached(CJinjaEngine* engine, const char* template_name, CJinjaContext* ctx)`
Renders a template with caching support.

**Parameters:**
- `engine`: CJinjaEngine instance
- `template_name`: Name of template file
- `ctx`: Template context

**Returns:**
- Rendered string (caller must free), or NULL on error

**Example:**
```c
char* result = cjinja_render_cached(engine, "template.html", ctx);
```

## Utility Functions

### HTML Escaping

#### `char* cjinja_escape_html(const char* input)`
Escapes HTML special characters.

**Parameters:**
- `input`: Input string

**Returns:**
- HTML-escaped string (caller must free)

**Example:**
```c
char* result = cjinja_escape_html("<script>alert('test')</script>");
// Returns: "&lt;script&gt;alert(&#39;test&#39;)&lt;/script&gt;"
free(result);
```

### String Utilities

#### `char* cjinja_trim(const char* input)`
Removes leading and trailing whitespace.

**Parameters:**
- `input`: Input string

**Returns:**
- Trimmed string (caller must free)

**Example:**
```c
char* result = cjinja_trim("   hello world   ");
// Returns: "hello world"
free(result);
```

#### `int cjinja_is_empty(const char* input)`
Checks if string is empty or contains only whitespace.

**Parameters:**
- `input`: Input string

**Returns:**
- 1 if empty/whitespace only, 0 otherwise

**Example:**
```c
if (cjinja_is_empty("   ")) {
    printf("String is empty\n");
}
```

## Error Handling

CJinja functions return NULL on error. Always check return values:

```c
char* result = cjinja_render_string(template, ctx);
if (!result) {
    // Handle error
    fprintf(stderr, "Template rendering failed\n");
    return -1;
}
// Use result
free(result);
```

## Examples

### Complete Example

```c
#include "../compiler/src/cjinja.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    // Create engine and context
    CJinjaEngine* engine = cjinja_create("./templates");
    CJinjaContext* ctx = cjinja_create_context();
    
    // Set variables
    cjinja_set_var(ctx, "user", "John Doe");
    cjinja_set_var(ctx, "title", "Welcome Page");
    cjinja_set_bool(ctx, "is_admin", 1);
    
    char* fruits[] = {"apple", "banana", "cherry"};
    cjinja_set_array(ctx, "fruits", fruits, 3);
    
    // Render template
    const char* template = 
        "{% if is_admin %}"
        "Welcome admin {{user}}!\n"
        "{% endif %}"
        "Fruits:\n"
        "{% for fruit in fruits %}"
        "  - {{fruit | upper}}\n"
        "{% endfor %}"
        "Total: {{fruits | length}} fruits";
    
    char* result = cjinja_render_with_loops(template, ctx);
    if (result) {
        printf("%s\n", result);
        free(result);
    }
    
    // Cleanup
    cjinja_destroy_context(ctx);
    cjinja_destroy(engine);
    
    return 0;
}
```

### Performance Example

```c
#include "../compiler/src/cjinja.h"
#include <stdio.h>
#include <time.h>

int main() {
    CJinjaEngine* engine = cjinja_create("./templates");
    CJinjaContext* ctx = cjinja_create_context();
    
    // Enable caching for performance
    cjinja_enable_cache(engine, 1);
    
    // Set up test data
    cjinja_set_var(ctx, "user", "John Doe");
    cjinja_set_bool(ctx, "is_admin", 1);
    
    const char* template = "Hello {{user}}, welcome!";
    
    // Benchmark rendering
    clock_t start = clock();
    for (int i = 0; i < 10000; i++) {
        char* result = cjinja_render_string(template, ctx);
        free(result);
    }
    clock_t end = clock();
    
    double elapsed = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("Rendered 10,000 templates in %.3f seconds\n", elapsed);
    printf("Average: %.3f ms per render\n", (elapsed * 1000) / 10000);
    
    cjinja_destroy_context(ctx);
    cjinja_destroy(engine);
    
    return 0;
}
```

## Performance Characteristics

| Operation | Typical Performance | Notes |
|-----------|-------------------|-------|
| Variable substitution | ~200 ns | Sub-microsecond performance |
| Conditional rendering | ~600 ns | Sub-microsecond performance |
| Loop rendering | ~7 μs | Sub-10μs performance |
| Filter rendering | ~1.3 μs | Sub-10μs performance |
| Complex templates | ~12 μs | Sub-100μs performance |
| Individual filters | ~30-70 ns | Sub-100ns performance |
| Utility functions | ~30-80 ns | Sub-100ns performance |

## Memory Management

- All functions that return strings allocate memory that must be freed by the caller
- Context and engine objects must be destroyed to prevent memory leaks
- Template caching uses internal memory management
- Filter functions should return newly allocated strings

## Thread Safety

The current implementation is not thread-safe. For multi-threaded applications:
- Create separate engine and context instances per thread
- Do not share contexts between threads
- Consider using thread-local storage for contexts

## Best Practices

1. **Always check return values** for NULL to handle errors
2. **Free allocated memory** returned by rendering functions
3. **Use caching** for frequently rendered templates
4. **Pre-compile templates** when possible for better performance
5. **Use appropriate rendering functions** based on template complexity
6. **Destroy contexts and engines** to prevent memory leaks 