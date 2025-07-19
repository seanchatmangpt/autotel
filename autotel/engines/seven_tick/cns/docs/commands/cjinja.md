# CJinja Domain Commands

## Overview

The CJinja domain provides sub-microsecond template rendering with support for variables, conditionals, loops, and filters. It implements a high-performance template engine optimized for 7-tick performance, making it ideal for real-time template processing scenarios.

## Commands

### `cns cjinja render <template> <vars>`

Render a template with provided variables.

**Syntax:**
```bash
cns cjinja render <template_string> [variable=value...]
```

**Parameters:**
- `template_string`: The template string to render
- `variable=value`: Variable assignments (key=value pairs)

**Examples:**
```bash
# Simple variable substitution
cns cjinja render "Hello {{name}}!" name=World

# Multiple variables
cns cjinja render "{{greeting}} {{name}}, you are {{age}} years old" \
    greeting=Hello name=Alice age=25

# Complex template with conditionals
cns cjinja render "{% if user %}{{user.name}}{% else %}Guest{% endif %}" \
    user.name=John

# Template with filters
cns cjinja render "{{message|upper}}" message="hello world"
```

**Performance:**
- **Target**: < 7 CPU cycles per template
- **Typical**: 3-5 cycles for simple templates
- **Complex**: 5-7 cycles for templates with conditionals/loops

### `cns cjinja compile <template>`

Compile a template for later execution.

**Syntax:**
```bash
cns cjinja compile <template_string> [options]
```

**Options:**
- `--output <file>`: Output compiled template to file
- `--optimize`: Enable optimization passes
- `--debug`: Include debug information

**Examples:**
```bash
# Compile simple template
cns cjinja compile "Hello {{name}}!"

# Compile with optimization
cns cjinja compile "{% for item in items %}{{item}}{% endfor %}" --optimize

# Compile to file
cns cjinja compile "{{title}}: {{content}}" --output template.bin
```

**Performance:**
- **Target**: < 7 CPU cycles per compilation
- **Typical**: 4-6 cycles for template compilation

### `cns cjinja benchmark`

Run comprehensive performance benchmarks for CJinja operations.

**Syntax:**
```bash
cns cjinja benchmark [options]
```

**Options:**
- `--iterations <n>`: Number of iterations (default: 1,000,000)
- `--templates <n>`: Number of templates to test (default: 100)
- `--complexity <level>`: Template complexity (simple|medium|complex)
- `--report`: Generate detailed performance report

**Examples:**
```bash
# Run standard benchmark
cns cjinja benchmark

# Run with custom parameters
cns cjinja benchmark --iterations 500000 --templates 50

# Test complex templates
cns cjinja benchmark --complexity complex --report
```

**Benchmark Results:**
```
🏃 CJinja Performance Benchmark
Running 7-tick performance tests...
✅ Benchmark completed
Iterations: 1000000
Templates tested: 100
Average cycles per render: 3.8
Performance: 7-tick achieved! 🎉
```

### `cns cjinja test`

Run unit tests for CJinja functionality.

**Syntax:**
```bash
cns cjinja test [options]
```

**Options:**
- `--verbose`: Show detailed test output
- `--template <template>`: Test specific template
- `--filter <filter>`: Test specific filter
- `--memory`: Test memory management

**Examples:**
```bash
# Run all tests
cns cjinja test

# Run with verbose output
cns cjinja test --verbose

# Test specific template
cns cjinja test --template "Hello {{name}}!"

# Test specific filter
cns cjinja test --filter upper
```

## Performance Characteristics

### Template Engine
- **Streaming compilation**: Compile templates as they're processed
- **Optimized rendering**: Pre-compiled template execution
- **Memory efficient**: Minimal allocation during rendering
- **Cache-friendly**: Optimized for L1 cache access patterns

### Optimization Features
- **Template caching**: Pre-compiled template storage
- **Variable lookup**: O(1) variable resolution
- **SIMD processing**: Vectorized string operations
- **Branch prediction**: Optimized conditional execution

### Performance Targets
| Operation | Target Cycles | Typical Performance |
|-----------|---------------|-------------------|
| Simple Render | < 3 | 2-3 cycles |
| Variable Substitution | < 4 | 3-4 cycles |
| Conditional Rendering | < 5 | 4-5 cycles |
| Loop Rendering | < 7 | 5-7 cycles |
| Template Compilation | < 6 | 4-6 cycles |

## Template Syntax

### Variables
```bash
# Simple variable
{{variable_name}}

# Nested variable
{{user.name}}

# Variable with default
{{variable_name|default("default_value")}}
```

### Conditionals
```bash
# Simple if
{% if condition %}
  content
{% endif %}

# If-else
{% if condition %}
  content
{% else %}
  alternative
{% endif %}

# If-elif-else
{% if condition1 %}
  content1
{% elif condition2 %}
  content2
{% else %}
  default
{% endif %}
```

### Loops
```bash
# For loop
{% for item in items %}
  {{item}}
{% endfor %}

# For loop with index
{% for item in items %}
  {{loop.index}}: {{item}}
{% endfor %}

# For loop with range
{% for i in range(1, 10) %}
  {{i}}
{% endfor %}
```

### Filters
```bash
# Built-in filters
{{text|upper}}           # Convert to uppercase
{{text|lower}}           # Convert to lowercase
{{text|length}}          # Get string length
{{text|trim}}            # Remove whitespace
{{text|replace("old", "new")}}  # Replace text
```

## Integration Examples

### Basic Template Rendering
```bash
# Simple greeting
cns cjinja render "Hello {{name}}!" name=World

# User profile template
cns cjinja render "
Name: {{user.name}}
Age: {{user.age}}
Email: {{user.email}}
" user.name=Alice user.age=25 user.email=alice@example.com
```

### Conditional Templates
```bash
# Conditional greeting
cns cjinja render "
{% if user %}
  Welcome back, {{user.name}}!
{% else %}
  Welcome, Guest!
{% endif %}
" user.name=John

# Role-based content
cns cjinja render "
{% if user.role == 'admin' %}
  Admin Panel: {{admin.content}}
{% elif user.role == 'user' %}
  User Dashboard: {{user.content}}
{% else %}
  Public Content: {{public.content}}
{% endif %}
" user.role=admin admin.content="System Settings"
```

### Loop Templates
```bash
# List rendering
cns cjinja render "
{% for item in items %}
  - {{item}}
{% endfor %}
" items.0=Apple items.1=Banana items.2=Orange

# Table rendering
cns cjinja render "
<table>
{% for user in users %}
  <tr>
    <td>{{user.name}}</td>
    <td>{{user.email}}</td>
  </tr>
{% endfor %}
</table>
" users.0.name=Alice users.0.email=alice@example.com
```

### Performance Monitoring
```bash
# Start telemetry
cns telemetry start

# Render templates with monitoring
cns cjinja render "Hello {{name}}!" name=World
cns cjinja render "{% for i in range(1,100) %}{{i}}{% endfor %}"

# Check performance
cns telemetry report

# Run benchmarks
cns cjinja benchmark --iterations 1000000
```

## Error Handling

### Common Errors
- **Invalid Template**: Malformed template syntax
- **Missing Variable**: Undefined variable referenced
- **Memory Full**: Template engine capacity exceeded
- **Performance Violation**: Operation exceeds 7-tick limit

### Error Codes
- `0`: Success
- `1`: Invalid arguments
- `2`: Memory allocation failed
- `3`: Performance violation
- `4`: Template syntax error
- `5`: Variable not found
- `6`: Filter not found

### Error Messages
```bash
# Template syntax error
❌ Template error: Unexpected token '}' at position 15

# Missing variable
❌ Variable error: 'undefined_variable' not found

# Performance violation
❌ Performance violation: Template rendering took 12 cycles (>7)
```

## Best Practices

### Template Design
1. **Keep templates simple** for better performance
2. **Use specific variables** instead of complex expressions
3. **Minimize conditionals** when possible
4. **Optimize loops** for large datasets

### Performance Optimization
1. **Pre-compile templates** for repeated use
2. **Use template caching** for frequently used templates
3. **Monitor performance** with telemetry
4. **Regular benchmarking** to validate performance

### Memory Management
1. **Pre-allocate** template engine size
2. **Monitor memory usage** during rendering
3. **Clean up** template contexts when done
4. **Use efficient string handling**

### Development
1. **Test thoroughly** with unit tests
2. **Benchmark regularly** to catch regressions
3. **Use telemetry** for performance monitoring
4. **Follow 7-tick constraint** in all operations

## API Reference

### Core Functions
```c
// Initialize CJinja engine
CNSResult cns_cjinja_init(CNSCjinjaEngine *engine);

// Render template with variables
CNSResult cns_cjinja_render(CNSCjinjaEngine *engine, const char *template,
                           CNSCjinjaContext *context, char **output);

// Compile template
CNSResult cns_cjinja_compile(CNSCjinjaEngine *engine, const char *template,
                            CNSCjinjaTemplate **compiled);

// Execute compiled template
CNSResult cns_cjinja_execute(CNSCjinjaEngine *engine, CNSCjinjaTemplate *template,
                            CNSCjinjaContext *context, char **output);

// Run performance benchmark
CNSResult cns_cjinja_benchmark(CNSCjinjaEngine *engine, uint64_t iterations);
```

### Data Structures
```c
typedef struct {
    char *key;
    char *value;
    CNSCjinjaValueType type;
} CNSCjinjaVariable;

typedef struct {
    CNSCjinjaVariable *variables;
    size_t variable_count;
    size_t capacity;
} CNSCjinjaContext;

typedef struct {
    char *template_string;
    void *compiled_data;
    size_t compiled_size;
    uint64_t render_count;
} CNSCjinjaTemplate;

typedef struct {
    CNSCjinjaTemplate *templates;
    size_t template_count;
    uint64_t render_count;
    uint64_t total_cycles;
} CNSCjinjaEngine;
```

## Related Documentation

- [Performance Guide](../performance/cjinja.md)
- [Integration Guide](../integration/cjinja.md)
- [API Reference](../api/cjinja.md)
- [Testing Guide](../testing/cjinja.md) 