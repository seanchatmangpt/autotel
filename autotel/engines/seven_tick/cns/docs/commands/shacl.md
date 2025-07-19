# SHACL Domain Commands

## Overview

The SHACL domain provides high-performance shape validation and constraint checking for semantic data. It implements the W3C SHACL (Shapes Constraint Language) specification with 7-tick performance optimization for real-time validation scenarios.

## Commands

### `cns shacl validate <data> <shapes>`

Validate data against SHACL shapes and constraints.

**Syntax:**
```bash
cns shacl validate <data_file> <shapes_file>
```

**Parameters:**
- `data_file`: Path to the data file (TTL format)
- `shapes_file`: Path to the shapes file (TTL format)

**Examples:**
```bash
# Validate person data against person shapes
cns shacl validate data/persons.ttl shapes/person.ttl

# Validate organization data
cns shacl validate data/orgs.ttl shapes/organization.ttl

# Validate with multiple shape files
cns shacl validate data/complete.ttl shapes/combined.ttl
```

**Performance:**
- **Target**: < 7 CPU cycles per validation
- **Typical**: 4-6 cycles for simple validations
- **Complex**: 6-7 cycles for constraint-heavy validations

### `cns shacl check <constraints>`

Check specific constraints against loaded data.

**Syntax:**
```bash
cns shacl check <constraint_type> [options]
```

**Constraint Types:**
- `minCount`: Minimum count constraints
- `maxCount`: Maximum count constraints
- `pattern`: Pattern matching constraints
- `datatype`: Data type constraints
- `range`: Value range constraints

**Examples:**
```bash
# Check minimum count constraints
cns shacl check minCount

# Check pattern constraints
cns shacl check pattern --regex "email"

# Check data type constraints
cns shacl check datatype --type xsd:string
```

**Performance:**
- **Target**: < 7 CPU cycles per constraint check
- **Typical**: 3-5 cycles for simple constraints

### `cns shacl benchmark`

Run comprehensive performance benchmarks for SHACL validation.

**Syntax:**
```bash
cns shacl benchmark [options]
```

**Options:**
- `--iterations <n>`: Number of iterations (default: 100,000)
- `--shapes <n>`: Number of shapes to test (default: 100)
- `--constraints <n>`: Number of constraints per shape (default: 10)
- `--report`: Generate detailed performance report

**Examples:**
```bash
# Run standard benchmark
cns shacl benchmark

# Run with custom parameters
cns shacl benchmark --iterations 50000 --shapes 50

# Generate detailed report
cns shacl benchmark --report
```

**Benchmark Results:**
```
🏃 SHACL Performance Benchmark
Running 7-tick performance tests...
✅ Benchmark completed
Iterations: 100000
Shapes tested: 100
Constraints per shape: 10
Average cycles per validation: 4.2
Performance: 7-tick achieved! 🎉
```

### `cns shacl test`

Run unit tests for SHACL functionality.

**Syntax:**
```bash
cns shacl test [options]
```

**Options:**
- `--verbose`: Show detailed test output
- `--shape <shape>`: Test specific shape validation
- `--constraint <constraint>`: Test specific constraint type
- `--memory`: Test memory management

**Examples:**
```bash
# Run all tests
cns shacl test

# Run with verbose output
cns shacl test --verbose

# Test specific shape
cns shacl test --shape PersonShape

# Test specific constraint
cns shacl test --constraint minCount
```

## Performance Characteristics

### Validation Engine
- **Streaming validation**: Process data as it's loaded
- **Early termination**: Stop on first constraint violation
- **Batch processing**: Validate multiple nodes simultaneously
- **Memory efficient**: Minimal memory allocation during validation

### Optimization Features
- **Hash-based shape lookup**: O(1) shape resolution
- **Constraint caching**: Pre-compiled constraint checks
- **SIMD validation**: Vectorized constraint checking
- **Branch prediction**: Optimized validation flow

### Performance Targets
| Operation | Target Cycles | Typical Performance |
|-----------|---------------|-------------------|
| Simple Validation | < 4 | 3-4 cycles |
| Constraint Check | < 5 | 3-5 cycles |
| Shape Resolution | < 3 | 2-3 cycles |
| Complex Validation | < 7 | 5-7 cycles |

## Integration Examples

### Basic Shape Validation
```bash
# Define person shape
echo '
@prefix sh: <http://www.w3.org/ns/shacl#> .
@prefix xsd: <http://www.w3.org/2001/XMLSchema#> .

<PersonShape> a sh:NodeShape ;
    sh:targetClass <Person> ;
    sh:property [
        sh:path <name> ;
        sh:minCount 1 ;
        sh:datatype xsd:string
    ] ;
    sh:property [
        sh:path <age> ;
        sh:datatype xsd:integer ;
        sh:minInclusive 0 ;
        sh:maxInclusive 150
    ] .
' > shapes/person.ttl

# Validate person data
cns shacl validate data/persons.ttl shapes/person.ttl
```

### Performance Monitoring
```bash
# Start telemetry
cns telemetry start

# Run validation with monitoring
cns shacl validate large_dataset.ttl complex_shapes.ttl

# Check performance
cns telemetry report

# Run benchmarks
cns shacl benchmark --iterations 100000
```

### Constraint Testing
```bash
# Test specific constraints
cns shacl check minCount --shape PersonShape
cns shacl check pattern --regex "email" --shape ContactShape
cns shacl check datatype --type xsd:integer --shape AgeShape
```

## Error Handling

### Common Errors
- **Invalid Shape**: Malformed SHACL shape definition
- **Constraint Violation**: Data doesn't meet shape requirements
- **Memory Full**: Validation engine capacity exceeded
- **Performance Violation**: Operation exceeds 7-tick limit

### Error Codes
- `0`: Success (all validations passed)
- `1`: Invalid arguments
- `2`: Memory allocation failed
- `3`: Performance violation
- `4`: Constraint violation
- `5`: Shape not found
- `6`: Invalid data format

### Validation Results
```bash
# Successful validation
✅ Validation passed: 150 nodes validated, 0 violations

# Failed validation
❌ Validation failed: 150 nodes validated, 3 violations
   - Person:123: Missing required property 'name'
   - Person:456: Age value '200' exceeds maximum '150'
   - Person:789: Invalid email format
```

## Best Practices

### Shape Design
1. **Use specific constraints** when possible
2. **Group related properties** in single shapes
3. **Optimize constraint order** for early termination
4. **Use appropriate data types** for validation

### Performance Optimization
1. **Pre-compile shapes** for repeated validation
2. **Use streaming validation** for large datasets
3. **Monitor performance** with telemetry
4. **Regular benchmarking** to validate performance

### Memory Management
1. **Pre-allocate** validation engine size
2. **Monitor memory usage** during validation
3. **Clean up** validation contexts when done
4. **Use efficient data structures**

### Development
1. **Test thoroughly** with unit tests
2. **Benchmark regularly** to catch regressions
3. **Use telemetry** for performance monitoring
4. **Follow 7-tick constraint** in all operations

## API Reference

### Core Functions
```c
// Initialize SHACL engine
CNSResult cns_shacl_init(CNSShaclEngine *engine);

// Load shapes from file
CNSResult cns_shacl_load_shapes(CNSShaclEngine *engine, const char *shapes_file);

// Validate data against shapes
CNSResult cns_shacl_validate_data(CNSShaclEngine *engine, const char *data_file,
                                 CNSValidationResult *result);

// Check specific constraint
CNSResult cns_shacl_check_constraint(CNSShaclEngine *engine, uint32_t node_id,
                                    CNSConstraintType constraint_type, void *value);

// Run performance benchmark
CNSResult cns_shacl_benchmark(CNSShaclEngine *engine, uint64_t iterations);
```

### Data Structures
```c
typedef enum {
    CNS_CONSTRAINT_MIN_COUNT,
    CNS_CONSTRAINT_MAX_COUNT,
    CNS_CONSTRAINT_PATTERN,
    CNS_CONSTRAINT_DATATYPE,
    CNS_CONSTRAINT_RANGE
} CNSConstraintType;

typedef struct {
    uint32_t node_id;
    CNSConstraintType constraint_type;
    const char *message;
    bool violated;
} CNSValidationViolation;

typedef struct {
    uint32_t total_nodes;
    uint32_t validated_nodes;
    uint32_t violation_count;
    CNSValidationViolation *violations;
    uint64_t total_cycles;
} CNSValidationResult;

typedef struct {
    CNSShape *shapes;
    size_t shape_count;
    uint64_t validation_count;
    uint64_t total_cycles;
} CNSShaclEngine;
```

## Related Documentation

- [Performance Guide](../performance/shacl.md)
- [Integration Guide](../integration/shacl.md)
- [API Reference](../api/shacl.md)
- [Testing Guide](../testing/shacl.md) 