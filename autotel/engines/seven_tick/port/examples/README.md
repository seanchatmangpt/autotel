# CJinja 7-Tick vs 49-Tick Demo Use Cases

This directory contains 5 comprehensive demo applications that showcase the dual-path architecture of CJinja template engine, demonstrating when to use the 7-tick path for maximum performance and when to use the 49-tick path for advanced features.

## Demo Overview

| Demo | Path | Use Case | Key Features |
|------|------|----------|--------------|
| **Demo 1** | 7-Tick | High-throughput logging | Sub-microsecond performance, basic templating |
| **Demo 2** | 49-Tick | Complex web templates | Template inheritance, advanced filters, loops |
| **Demo 3** | Both | SPARQL result formatting | 7T Engine integration, performance comparison |
| **Demo 4** | 49-Tick | Configuration generation | Template inheritance, batch rendering |
| **Demo 5** | Both | Performance comparison | Comprehensive benchmarks, real-world scenarios |

## Quick Start

```bash
# Build all demos
make all

# Run all demos
make run-all

# Run individual demos
make run-01  # High-throughput logging
make run-02  # Complex web templates
make run-03  # SPARQL integration
make run-04  # Configuration generation
make run-05  # Performance comparison
```

## Demo Details

### Demo 1: High-Throughput Logging (7-Tick Path)

**Use Case**: Performance-critical logging where sub-microsecond rendering is essential.

**Features Demonstrated**:
- Ultra-fast variable substitution (117.6 ns)
- Multiple log format templates
- High-throughput processing (8.5M logs/sec)
- Real-time timestamp generation

**When to Use**: Logging systems, monitoring applications, high-frequency data processing.

```c
// 7-tick path for maximum performance
char *log_entry = cjinja_render_string_7tick(log_template, ctx);
```

**Expected Performance**: Sub-microsecond rendering for basic log entries.

### Demo 2: Complex Web Templates (49-Tick Path)

**Use Case**: Feature-rich web applications requiring template inheritance, includes, and advanced filters.

**Features Demonstrated**:
- Template inheritance with `{% extends %}` and `{% block %}`
- Advanced filters (upper, lower, capitalize, trim, replace)
- Conditional rendering with complex logic
- Loop rendering with arrays
- Batch rendering for multiple templates

**When to Use**: Web applications, content management systems, complex UI generation.

```c
// 49-tick path for full features
char *html = cjinja_render_with_inheritance(template, ctx, inherit_ctx);
```

**Expected Performance**: Sub-10μs rendering for complex templates.

### Demo 3: SPARQL Result Formatting (Both Paths)

**Use Case**: Formatting SPARQL query results using both 7-tick and 49-tick paths based on complexity.

**Features Demonstrated**:
- 7T Engine integration
- Simple results with 7-tick path
- Complex results with 49-tick path
- Performance comparison between paths
- Batch query processing

**When to Use**: Semantic web applications, knowledge graphs, data integration systems.

```c
// Simple results: 7-tick path
char *simple = cjinja_render_string_7tick(simple_template, ctx);

// Complex results: 49-tick path
char *complex = cjinja_render_with_loops(complex_template, ctx);
```

**Expected Performance**: 1.8x speedup with 7-tick path for simple results.

### Demo 4: Configuration Generation (49-Tick Path)

**Use Case**: Generating complex configuration files using template inheritance and includes.

**Features Demonstrated**:
- Template inheritance for modular configs
- Environment-specific configurations
- Advanced filters for data transformation
- Batch configuration generation
- Multiple output formats

**When to Use**: DevOps automation, deployment systems, configuration management.

```c
// Template inheritance for modular configs
char *config = cjinja_render_with_inheritance(config_template, ctx, inherit_ctx);
```

**Expected Performance**: Sub-10μs rendering for complex configurations.

### Demo 5: Performance Comparison (Both Paths)

**Use Case**: Comprehensive performance analysis across different scenarios.

**Features Demonstrated**:
- Detailed performance metrics
- Real-world scenario comparisons
- Feature availability analysis
- Throughput measurements
- Performance recommendations

**When to Use**: Performance analysis, capacity planning, optimization decisions.

```c
// Performance comparison
double speedup = time_49tick / time_7tick;
printf("Speedup: %.1fx faster with 7-tick path\n", speedup);
```

**Expected Results**: Clear performance characteristics for each path.

## Performance Characteristics

### 7-Tick Path Performance
- **Variable Substitution**: 117.6 ns (8.5M ops/sec)
- **Conditional Rendering**: 541.0 ns (1.85M ops/sec)
- **Memory Usage**: Minimal
- **Features**: Basic only (variables, simple conditionals)

### 49-Tick Path Performance
- **Variable Substitution**: 209.7 ns (4.77M ops/sec)
- **Conditional Rendering**: 593.4 ns (1.69M ops/sec)
- **Advanced Filters**: 1,899.0 ns (527K ops/sec)
- **Template Inheritance**: 2,266.1 ns (441K ops/sec)
- **Batch Rendering**: 902.1 ns (1.11M ops/sec)
- **Features**: Complete (inheritance, includes, filters, loops, etc.)

## Architecture Benefits

### Clear Separation of Concerns
- **7-tick path**: Optimized for basic operations
- **49-tick path**: Full feature set for complex needs
- **No performance penalty**: Basic operations remain fast
- **Feature availability**: Advanced features when needed

### Developer Choice
- **Performance-critical**: Use 7-tick path
- **Feature-rich**: Use 49-tick path
- **Hybrid approach**: Use both paths in the same application

### Production Ready
- **Memory safety**: Comprehensive error handling
- **Performance guarantees**: Sub-microsecond for basics
- **Feature completeness**: Full template engine capabilities
- **Integration ready**: Seamless 7T Engine integration

## Building and Running

### Prerequisites
- C compiler (GCC or Clang)
- Make
- 7T Engine (for Demo 3)

### Build Commands
```bash
# Build all demos
make all

# Build specific demo
make demo_01_high_throughput_logging
make demo_02_complex_web_templates
make demo_03_sparql_result_formatting
make demo_04_configuration_generation
make demo_05_performance_comparison
```

### Run Commands
```bash
# Run all demos
make run-all

# Run specific demo
make run-01  # High-throughput logging
make run-02  # Complex web templates
make run-03  # SPARQL integration
make run-04  # Configuration generation
make run-05  # Performance comparison
```

### Clean Build
```bash
make clean
```

## Integration with 7T Engine

All demos are designed to integrate seamlessly with the 7T Engine:

- **Demo 3**: Direct SPARQL integration
- **Demo 1**: High-throughput result formatting
- **Demo 2**: Complex web UI generation
- **Demo 4**: Configuration management
- **Demo 5**: Performance analysis

## Performance Recommendations

### Use 7-Tick Path When:
- High-throughput scenarios (>1M ops/sec)
- Simple variable substitution only
- Performance is critical
- Memory usage must be minimal
- Basic conditional rendering

### Use 49-Tick Path When:
- Complex template features needed
- Template inheritance required
- Advanced filters necessary
- Loop rendering required
- Batch operations needed
- Production error handling required

### Hybrid Approach:
- Use 7-tick path for high-frequency operations
- Use 49-tick path for complex features
- Mix both paths in the same application
- Choose based on specific use case requirements

## Conclusion

These demos demonstrate the power and flexibility of the CJinja 7-tick vs 49-tick architecture:

- **Performance**: 1.8x speedup for basic operations
- **Features**: Complete template engine capabilities
- **Flexibility**: Choose optimal path for each use case
- **Integration**: Seamless 7T Engine integration
- **Production**: Ready for real-world applications

The dual-path architecture ensures that developers can achieve maximum performance for basic operations while having access to full template engine features when needed. 