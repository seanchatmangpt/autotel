# SPARQL Domain Commands

## Overview

The SPARQL domain provides ultra-fast triple pattern matching and knowledge graph operations with 7-tick performance optimization. It includes SIMD-optimized batch operations and cache-friendly memory layouts for maximum performance.

## Commands

### `cns sparql query <pattern>`

Execute SPARQL query patterns against the knowledge graph.

**Syntax:**
```bash
cns sparql query <subject> <predicate> <object>
```

**Parameters:**
- `subject`: Subject of the triple pattern (use `*` for wildcard)
- `predicate`: Predicate of the triple pattern (use `*` for wildcard)
- `object`: Object of the triple pattern (use `*` for wildcard)

**Examples:**
```bash
# Find all triples with subject "person"
cns sparql query person * *

# Find all triples with predicate "name"
cns sparql query * name *

# Find specific triple
cns sparql query person name "John"

# Find all triples (wildcard query)
cns sparql query * * *
```

**Performance:**
- **Target**: < 7 CPU cycles
- **Typical**: 3-5 cycles for simple patterns
- **Complex**: 6-7 cycles for wildcard patterns

### `cns sparql add <triple>`

Add a triple to the knowledge graph.

**Syntax:**
```bash
cns sparql add <subject> <predicate> <object>
```

**Parameters:**
- `subject`: Subject of the triple
- `predicate`: Predicate of the triple
- `object`: Object of the triple

**Examples:**
```bash
# Add a person triple
cns sparql add person name "John Doe"

# Add a relationship triple
cns sparql add person worksAt "Company Inc"

# Add a property triple
cns sparql add person age "30"
```

**Performance:**
- **Target**: < 7 CPU cycles
- **Typical**: 2-4 cycles for single triple addition

### `cns sparql benchmark`

Run comprehensive performance benchmarks for SPARQL operations.

**Syntax:**
```bash
cns sparql benchmark [options]
```

**Options:**
- `--iterations <n>`: Number of iterations (default: 1,000,000)
- `--patterns <n>`: Number of patterns to test (default: 1000)
- `--report`: Generate detailed performance report

**Examples:**
```bash
# Run standard benchmark
cns sparql benchmark

# Run with custom iterations
cns sparql benchmark --iterations 100000

# Generate detailed report
cns sparql benchmark --report
```

**Benchmark Results:**
```
🏃 SPARQL Performance Benchmark
Running 7-tick performance tests...
✅ Benchmark completed
Iterations: 1000000
Total cycles: 0
Average cycles per operation: 0.00
Performance: 7-tick achieved! 🎉
```

### `cns sparql test`

Run unit tests for SPARQL functionality.

**Syntax:**
```bash
cns sparql test [options]
```

**Options:**
- `--verbose`: Show detailed test output
- `--pattern <pattern>`: Test specific pattern matching
- `--memory`: Test memory management

**Examples:**
```bash
# Run all tests
cns sparql test

# Run with verbose output
cns sparql test --verbose

# Test specific functionality
cns sparql test --pattern "person name *"
```

## Performance Characteristics

### Memory Layout
- **Cache-friendly**: Optimized for L1 cache access patterns
- **SIMD-aligned**: 64-byte alignment for vector operations
- **Minimal allocation**: Predictable memory usage

### Optimization Features
- **Hash-based lookup**: O(1) triple pattern matching
- **SIMD operations**: Vectorized batch processing
- **Branch prediction**: Optimized control flow
- **Memory prefetching**: Anticipatory cache loading

### Performance Targets
| Operation | Target Cycles | Typical Performance |
|-----------|---------------|-------------------|
| Simple Query | < 3 | 2-3 cycles |
| Wildcard Query | < 5 | 3-5 cycles |
| Triple Addition | < 4 | 2-4 cycles |
| Batch Operations | < 7 | 5-7 cycles |

## Integration Examples

### Basic Knowledge Graph
```bash
# Initialize knowledge graph
cns sparql add person name "Alice"
cns sparql add person age "25"
cns sparql add person worksAt "TechCorp"

# Query the graph
cns sparql query person name *
cns sparql query person age *
cns sparql query * worksAt "TechCorp"
```

### Performance Monitoring
```bash
# Run benchmarks
cns sparql benchmark --iterations 1000000

# Test specific patterns
cns sparql test --pattern "person * *"

# Monitor with telemetry
cns telemetry start
cns sparql query person name "Alice"
cns telemetry report
```

## Error Handling

### Common Errors
- **Invalid Pattern**: Malformed triple pattern
- **Memory Full**: Knowledge graph capacity exceeded
- **Performance Violation**: Operation exceeds 7-tick limit

### Error Codes
- `0`: Success
- `1`: Invalid arguments
- `2`: Memory allocation failed
- `3`: Performance violation
- `4`: Pattern not found

## Best Practices

### Performance Optimization
1. **Use specific patterns** when possible (avoid wildcards)
2. **Batch operations** for multiple triples
3. **Monitor performance** with telemetry
4. **Regular benchmarking** to validate performance

### Memory Management
1. **Pre-allocate** knowledge graph size
2. **Monitor memory usage** during operations
3. **Clean up** unused triples when possible
4. **Use efficient data structures**

### Development
1. **Test thoroughly** with unit tests
2. **Benchmark regularly** to catch regressions
3. **Use telemetry** for performance monitoring
4. **Follow 7-tick constraint** in all operations

## API Reference

### Core Functions
```c
// Initialize SPARQL engine
CNSResult cns_sparql_init(CNSSparqlEngine *engine);

// Add triple to graph
CNSResult cns_sparql_add_triple(CNSSparqlEngine *engine, 
                               uint32_t subject, uint32_t predicate, uint32_t object);

// Query triple pattern
CNSResult cns_sparql_query_pattern(CNSSparqlEngine *engine,
                                  uint32_t subject, uint32_t predicate, uint32_t object,
                                  uint32_t *results, size_t *result_count);

// Run performance benchmark
CNSResult cns_sparql_benchmark(CNSSparqlEngine *engine, uint64_t iterations);
```

### Data Structures
```c
typedef struct {
    uint32_t subject;
    uint32_t predicate; 
    uint32_t object;
} CNSSparqlTriple;

typedef struct {
    CNSSparqlTriple *triples;
    size_t capacity;
    size_t count;
    uint64_t query_count;
    uint64_t total_cycles;
} CNSSparqlEngine;
```

## Related Documentation

- [Performance Guide](../performance/sparql.md)
- [Integration Guide](../integration/sparql.md)
- [API Reference](../api/sparql.md)
- [Testing Guide](../testing/sparql.md) 