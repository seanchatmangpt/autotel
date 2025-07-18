# 7T Engine API Reference

## Overview

The 7T engine provides a comprehensive set of APIs for high-performance knowledge processing operations. This document covers all public APIs, their parameters, return values, and usage examples.

## Core Runtime API

### Engine Management

#### `s7t_create_engine()`
Creates a new 7T engine instance.

**Signature:**
```c
EngineState* s7t_create_engine(void);
```

**Returns:** Pointer to initialized EngineState structure

**Example:**
```c
EngineState* engine = s7t_create_engine();
if (!engine) {
    fprintf(stderr, "Failed to create engine\n");
    return -1;
}
```

#### `s7t_destroy_engine()`
Destroys an engine instance and frees all associated memory.

**Signature:**
```c
void s7t_destroy_engine(EngineState* engine);
```

**Parameters:**
- `engine`: Pointer to EngineState to destroy

**Example:**
```c
s7t_destroy_engine(engine);
```

### String Management

#### `s7t_intern_string()`
Interns a string and returns a unique integer identifier.

**Signature:**
```c
uint32_t s7t_intern_string(EngineState* engine, const char* str);
```

**Parameters:**
- `engine`: Engine instance
- `str`: String to intern

**Returns:** Unique integer identifier for the string

**Example:**
```c
uint32_t alice_id = s7t_intern_string(engine, "ex:Alice");
uint32_t name_id = s7t_intern_string(engine, "ex:name");
```

### Triple Operations

#### `s7t_add_triple()`
Adds a triple to the engine.

**Signature:**
```c
int s7t_add_triple(EngineState* engine, uint32_t subject, uint32_t predicate, uint32_t object);
```

**Parameters:**
- `engine`: Engine instance
- `subject`: Subject identifier
- `predicate`: Predicate identifier
- `object`: Object identifier

**Returns:** 1 on success, 0 on failure

**Example:**
```c
uint32_t alice = s7t_intern_string(engine, "ex:Alice");
uint32_t name = s7t_intern_string(engine, "ex:name");
uint32_t alice_name = s7t_intern_string(engine, "Alice Smith");

int result = s7t_add_triple(engine, alice, name, alice_name);
if (result) {
    printf("Triple added successfully\n");
}
```

#### `s7t_ask_pattern()`
Checks if a triple pattern exists in the engine.

**Signature:**
```c
int s7t_ask_pattern(EngineState* engine, uint32_t subject, uint32_t predicate, uint32_t object);
```

**Parameters:**
- `engine`: Engine instance
- `subject`: Subject identifier (0 for wildcard)
- `predicate`: Predicate identifier (0 for wildcard)
- `object`: Object identifier (0 for wildcard)

**Returns:** 1 if pattern exists, 0 otherwise

**Example:**
```c
// Check if Alice has a name
int has_name = s7t_ask_pattern(engine, alice, name, 0);
if (has_name) {
    printf("Alice has a name\n");
}

// Check for specific triple
int specific = s7t_ask_pattern(engine, alice, name, alice_name);
if (specific) {
    printf("Alice's name is Alice Smith\n");
}
```

#### `s7t_get_objects()`
Retrieves all objects for a given subject-predicate pair.

**Signature:**
```c
uint32_t* s7t_get_objects(EngineState* engine, uint32_t predicate, uint32_t subject, size_t* count);
```

**Parameters:**
- `engine`: Engine instance
- `predicate`: Predicate identifier
- `subject`: Subject identifier
- `count`: Pointer to store the number of objects

**Returns:** Array of object identifiers (caller must free)

**Example:**
```c
size_t count = 0;
uint32_t* objects = s7t_get_objects(engine, name, alice, &count);
if (objects) {
    printf("Alice has %zu names\n", count);
    for (size_t i = 0; i < count; i++) {
        printf("  Object %zu: %u\n", i, objects[i]);
    }
    free(objects);
}
```

## SHACL Validation API

### Constraint Checking

#### `shacl_check_min_count()`
Validates that a subject has at least the specified number of values for a predicate.

**Signature:**
```c
int shacl_check_min_count(EngineState* engine, uint32_t subject_id, uint32_t predicate_id, uint32_t min_count);
```

**Parameters:**
- `engine`: Engine instance
- `subject_id`: Subject identifier
- `predicate_id`: Predicate identifier
- `min_count`: Minimum number of values required

**Returns:** 1 if constraint is satisfied, 0 otherwise

**Performance:** 2.55 cycles (0.80 ns) average

**Example:**
```c
// Check if Alice has at least one name
int has_min_name = shacl_check_min_count(engine, alice, name, 1);
if (has_min_name) {
    printf("Alice satisfies minCount=1 for name\n");
}
```

#### `shacl_check_max_count()`
Validates that a subject has at most the specified number of values for a predicate.

**Signature:**
```c
int shacl_check_max_count(EngineState* engine, uint32_t subject_id, uint32_t predicate_id, uint32_t max_count);
```

**Parameters:**
- `engine`: Engine instance
- `subject_id`: Subject identifier
- `predicate_id`: Predicate identifier
- `max_count`: Maximum number of values allowed

**Returns:** 1 if constraint is satisfied, 0 otherwise

**Performance:** 2.21 cycles (0.69 ns) average

**Example:**
```c
// Check if Alice has at most one name
int has_max_name = shacl_check_max_count(engine, alice, name, 1);
if (has_max_name) {
    printf("Alice satisfies maxCount=1 for name\n");
}
```

#### `shacl_check_class()`
Validates that a subject is an instance of the specified class.

**Signature:**
```c
int shacl_check_class(EngineState* engine, uint32_t subject_id, uint32_t class_id);
```

**Parameters:**
- `engine`: Engine instance
- `subject_id`: Subject identifier
- `class_id`: Class identifier

**Returns:** 1 if subject is instance of class, 0 otherwise

**Performance:** 1.46 cycles (0.46 ns) average

**Example:**
```c
uint32_t person_class = s7t_intern_string(engine, "ex:Person");
s7t_add_triple(engine, alice, rdf_type, person_class);

int is_person = shacl_check_class(engine, alice, person_class);
if (is_person) {
    printf("Alice is a Person\n");
}
```

## CJinja Template API

### Engine Management

#### `cjinja_create()`
Creates a new CJinja template engine.

**Signature:**
```c
CJinjaEngine* cjinja_create(const char* template_dir);
```

**Parameters:**
- `template_dir`: Directory containing template files

**Returns:** Pointer to CJinjaEngine structure

**Example:**
```c
CJinjaEngine* engine = cjinja_create("./templates");
if (!engine) {
    fprintf(stderr, "Failed to create CJinja engine\n");
    return -1;
}
```

#### `cjinja_destroy()`
Destroys a CJinja engine instance.

**Signature:**
```c
void cjinja_destroy(CJinjaEngine* engine);
```

**Parameters:**
- `engine`: CJinjaEngine to destroy

**Example:**
```c
cjinja_destroy(engine);
```

### Context Management

#### `cjinja_create_context()`
Creates a new template context for variable substitution.

**Signature:**
```c
CJinjaContext* cjinja_create_context(void);
```

**Returns:** Pointer to CJinjaContext structure

**Example:**
```c
CJinjaContext* ctx = cjinja_create_context();
if (!ctx) {
    fprintf(stderr, "Failed to create context\n");
    return -1;
}
```

#### `cjinja_destroy_context()`
Destroys a template context.

**Signature:**
```c
void cjinja_destroy_context(CJinjaContext* ctx);
```

**Parameters:**
- `ctx`: CJinjaContext to destroy

**Example:**
```c
cjinja_destroy_context(ctx);
```

#### `cjinja_set_var()`
Sets a variable in the template context.

**Signature:**
```c
void cjinja_set_var(CJinjaContext* ctx, const char* key, const char* value);
```

**Parameters:**
- `ctx`: Template context
- `key`: Variable name
- `value`: Variable value

**Example:**
```c
cjinja_set_var(ctx, "user", "Alice");
cjinja_set_var(ctx, "title", "Welcome");
```

#### `cjinja_set_bool()`
Sets a boolean variable in the template context.

**Signature:**
```c
void cjinja_set_bool(CJinjaContext* ctx, const char* key, int value);
```

**Parameters:**
- `ctx`: Template context
- `key`: Variable name
- `value`: Boolean value (0 = false, non-zero = true)

**Example:**
```c
cjinja_set_bool(ctx, "is_admin", 1);
cjinja_set_bool(ctx, "show_debug", 0);
```

#### `cjinja_set_array()`
Sets an array variable in the template context.

**Signature:**
```c
void cjinja_set_array(CJinjaContext* ctx, const char* key, char** items, size_t count);
```

**Parameters:**
- `ctx`: Template context
- `key`: Variable name
- `items`: Array of strings
- `count`: Number of items in array

**Example:**
```c
char* fruits[] = {"apple", "banana", "cherry"};
cjinja_set_array(ctx, "fruits", fruits, 3);
```

### Template Rendering

#### `cjinja_render_string()`
Renders a template string with variable substitution.

**Signature:**
```c
char* cjinja_render_string(const char* template_str, CJinjaContext* ctx);
```

**Parameters:**
- `template_str`: Template string to render
- `ctx`: Template context

**Returns:** Rendered string (caller must free)

**Performance:** 214.17 ns average

**Example:**
```c
const char* template = "Hello {{user}}, welcome to {{title}}!";
char* result = cjinja_render_string(template, ctx);
if (result) {
    printf("Rendered: %s\n", result);
    free(result);
}
```

#### `cjinja_render_with_conditionals()`
Renders a template with conditional blocks.

**Signature:**
```c
char* cjinja_render_with_conditionals(const char* template_str, CJinjaContext* ctx);
```

**Parameters:**
- `template_str`: Template string with {% if %} blocks
- `ctx`: Template context

**Returns:** Rendered string (caller must free)

**Performance:** 614.28 ns average

**Example:**
```c
const char* template = "{% if is_admin %}Welcome admin {{user}}!{% endif %}";
char* result = cjinja_render_with_conditionals(template, ctx);
if (result) {
    printf("Rendered: %s\n", result);
    free(result);
}
```

#### `cjinja_render_with_loops()`
Renders a template with loop blocks and filters.

**Signature:**
```c
char* cjinja_render_with_loops(const char* template_str, CJinjaContext* ctx);
```

**Parameters:**
- `template_str`: Template string with {% for %} blocks and filters
- `ctx`: Template context

**Returns:** Rendered string (caller must free)

**Performance:** 6,925.56 ns average

**Example:**
```c
const char* template = "{% for fruit in fruits %}- {{fruit}}\n{% endfor %}";
char* result = cjinja_render_with_loops(template, ctx);
if (result) {
    printf("Rendered:\n%s\n", result);
    free(result);
}
```

### Built-in Filters

#### `cjinja_filter_upper()`
Converts string to uppercase.

**Signature:**
```c
char* cjinja_filter_upper(const char* input, const char* args);
```

**Parameters:**
- `input`: Input string
- `args`: Filter arguments (unused)

**Returns:** Uppercase string (caller must free)

**Performance:** 47.25 ns average

**Example:**
```c
char* upper = cjinja_filter_upper("hello", NULL);
printf("Uppercase: %s\n", upper);  // Output: HELLO
free(upper);
```

#### `cjinja_filter_lower()`
Converts string to lowercase.

**Signature:**
```c
char* cjinja_filter_lower(const char* input, const char* args);
```

**Parameters:**
- `input`: Input string
- `args`: Filter arguments (unused)

**Returns:** Lowercase string (caller must free)

**Example:**
```c
char* lower = cjinja_filter_lower("HELLO", NULL);
printf("Lowercase: %s\n", lower);  // Output: hello
free(lower);
```

#### `cjinja_filter_capitalize()`
Capitalizes the first character of a string.

**Signature:**
```c
char* cjinja_filter_capitalize(const char* input, const char* args);
```

**Parameters:**
- `input`: Input string
- `args`: Filter arguments (unused)

**Returns:** Capitalized string (caller must free)

**Example:**
```c
char* cap = cjinja_filter_capitalize("hello", NULL);
printf("Capitalized: %s\n", cap);  // Output: Hello
free(cap);
```

#### `cjinja_filter_length()`
Returns the length of a string.

**Signature:**
```c
char* cjinja_filter_length(const char* input, const char* args);
```

**Parameters:**
- `input`: Input string
- `args`: Filter arguments (unused)

**Returns:** String representation of length (caller must free)

**Example:**
```c
char* len = cjinja_filter_length("hello", NULL);
printf("Length: %s\n", len);  // Output: 5
free(len);
```

### Template Caching

#### `cjinja_enable_cache()`
Enables or disables template caching.

**Signature:**
```c
void cjinja_enable_cache(CJinjaEngine* engine, int enabled);
```

**Parameters:**
- `engine`: CJinjaEngine instance
- `enabled`: 1 to enable, 0 to disable

**Example:**
```c
cjinja_enable_cache(engine, 1);  // Enable caching
```

#### `cjinja_render_cached()`
Renders a template from file with caching.

**Signature:**
```c
char* cjinja_render_cached(CJinjaEngine* engine, const char* template_name, CJinjaContext* ctx);
```

**Parameters:**
- `engine`: CJinjaEngine instance
- `template_name`: Name of template file
- `ctx`: Template context

**Returns:** Rendered string (caller must free)

**Performance:** 888.36 ns average

**Example:**
```c
char* result = cjinja_render_cached(engine, "welcome.html", ctx);
if (result) {
    printf("Rendered template: %s\n", result);
    free(result);
}
```

## Python Integration API

### RealSHACL Class

#### `RealSHACL.__init__()`
Initializes a RealSHACL instance with a SPARQL engine.

**Signature:**
```python
def __init__(self, sparql_engine)
```

**Parameters:**
- `sparql_engine`: RealSPARQL or MockSPARQL instance

**Example:**
```python
from shacl7t_real import RealSHACL, RealSPARQL

sparql = RealSPARQL()
shacl = RealSHACL(sparql)
```

#### `RealSHACL.define_shape()`
Defines a SHACL shape with constraints.

**Signature:**
```python
def define_shape(self, shape_id, target_class, constraints)
```

**Parameters:**
- `shape_id`: Unique identifier for the shape
- `target_class`: Target class for validation
- `constraints`: Dictionary of constraints

**Example:**
```python
constraints = {
    'properties': ['ex:name', 'ex:email'],
    'min_count': 1,
    'max_count': 1
}
shacl.define_shape('PersonShape', 'ex:Person', constraints)
```

#### `RealSHACL.validate_node()`
Validates a node against defined shapes.

**Signature:**
```python
def validate_node(self, node_id)
```

**Parameters:**
- `node_id`: Node identifier to validate

**Returns:** Dictionary mapping shape IDs to validation results

**Example:**
```python
results = shacl.validate_node('ex:Alice')
if results.get('PersonShape'):
    print("Alice is valid")
else:
    print("Alice is invalid")
```

### RealSPARQL Class

#### `RealSPARQL.__init__()`
Initializes a RealSPARQL instance.

**Signature:**
```python
def __init__(self)
```

**Example:**
```python
sparql = RealSPARQL()
```

#### `RealSPARQL._intern_string()`
Interns a string and returns its identifier.

**Signature:**
```python
def _intern_string(self, string)
```

**Parameters:**
- `string`: String to intern

**Returns:** Integer identifier for the string

**Example:**
```python
alice_id = sparql._intern_string('ex:Alice')
```

## Error Handling

### Common Error Patterns

#### Memory Allocation Errors
```c
EngineState* engine = s7t_create_engine();
if (!engine) {
    fprintf(stderr, "Failed to allocate engine\n");
    return -1;
}
```

#### String Interning Errors
```c
uint32_t id = s7t_intern_string(engine, str);
if (id == 0) {
    fprintf(stderr, "Failed to intern string: %s\n", str);
    return -1;
}
```

#### Template Rendering Errors
```c
char* result = cjinja_render_string(template, ctx);
if (!result) {
    fprintf(stderr, "Failed to render template\n");
    return -1;
}
// Use result...
free(result);
```

### Performance Considerations

#### Hot Path Optimization
- Use `S7T_HOT` attribute for frequently called functions
- Avoid heap allocations in hot paths
- Use pre-allocated buffers where possible

#### Memory Management
- Always free allocated memory
- Use RAII patterns in C++ wrappers
- Check for memory leaks in long-running applications

#### Error Recovery
- Provide fallback behavior for critical operations
- Log errors for debugging
- Graceful degradation when possible

## Best Practices

### Performance
1. **Warm up caches** before benchmarking
2. **Use appropriate iteration counts** for accurate measurements
3. **Measure in production-like conditions**
4. **Profile hot paths** for optimization opportunities

### Memory Management
1. **Always free allocated memory**
2. **Use consistent allocation patterns**
3. **Check for memory leaks**
4. **Pre-allocate buffers for hot paths**

### Error Handling
1. **Check return values** from all functions
2. **Provide meaningful error messages**
3. **Implement fallback behavior**
4. **Log errors for debugging**

### Integration
1. **Use consistent naming conventions**
2. **Document function behavior**
3. **Provide usage examples**
4. **Maintain backward compatibility** 