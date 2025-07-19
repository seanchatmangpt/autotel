# CNS Commands Reference

## Overview

This document provides a comprehensive reference for all CNS commands, organized by domain. Each command includes syntax, options, examples, and performance characteristics.

## Command Structure

CNS uses a domain-verb command structure:
```bash
cns <domain> <command> [options] [arguments]
```

### Global Options
- `--help, -h`: Show help for command
- `--verbose, -v`: Enable verbose output
- `--debug, -d`: Enable debug output
- `--json`: Output in JSON format
- `--version`: Show version information

## SPARQL Domain

The SPARQL domain provides ultra-fast triple pattern matching and knowledge graph operations.

### `cns sparql query`

Execute SPARQL queries with 7-tick performance optimization.

#### Syntax
```bash
cns sparql query <query> [options]
```

#### Options
- `--format <format>`: Output format (text, json, csv)
- `--limit <n>`: Limit number of results
- `--timeout <ms>`: Query timeout in milliseconds
- `--profile`: Enable query profiling

#### Examples
```bash
# Simple triple pattern query
cns sparql query "SELECT ?s ?p ?o WHERE { ?s ?p ?o }"

# Query with specific subject
cns sparql query "SELECT ?p ?o WHERE { <http://example.org/alice> ?p ?o }"

# Query with limit
cns sparql query "SELECT ?s WHERE { ?s a <http://example.org/Person> }" --limit 10

# Query with profiling
cns sparql query "SELECT ?s ?p ?o WHERE { ?s ?p ?o }" --profile
```

#### Performance
- **L1 Tier**: < 10ns for simple pattern matching
- **L2 Tier**: < 100ns for complex queries
- **L3 Tier**: < 1μs for full query execution

### `cns sparql add`

Add triples to the knowledge graph.

#### Syntax
```bash
cns sparql add <subject> <predicate> <object> [options]
```

#### Options
- `--format <format>`: Input format (n3, turtle, ntriples)
- `--batch`: Enable batch mode for multiple triples
- `--validate`: Validate triple before adding

#### Examples
```bash
# Add single triple
cns sparql add "http://example.org/alice" "http://example.org/name" "Alice"

# Add multiple triples in batch
cns sparql add "http://example.org/alice" "http://example.org/age" "25" --batch
cns sparql add "http://example.org/alice" "http://example.org/email" "alice@example.com" --batch

# Add with validation
cns sparql add "http://example.org/bob" "http://example.org/name" "Bob" --validate
```

#### Performance
- **L1 Tier**: < 10ns for single triple addition
- **L2 Tier**: < 100ns for batch operations

### `cns sparql benchmark`

Run SPARQL performance benchmarks.

#### Syntax
```bash
cns sparql benchmark [options]
```

#### Options
- `--iterations <n>`: Number of benchmark iterations
- `--warmup <n>`: Number of warmup iterations
- `--batch-size <n>`: Batch size for batch operations
- `--output <file>`: Output results to file

#### Examples
```bash
# Run standard benchmarks
cns sparql benchmark

# Run with custom iterations
cns sparql benchmark --iterations 1000000

# Run with detailed output
cns sparql benchmark --verbose --output sparql_benchmark.json
```

#### Performance Targets
- **Triple Lookup**: < 10ns (7-tick target)
- **Pattern Matching**: < 100ns (L2 tier)
- **Query Execution**: < 1μs (L3 tier)

### `cns sparql test`

Run SPARQL unit tests.

#### Syntax
```bash
cns sparql test [options]
```

#### Options
- `--verbose`: Enable verbose test output
- `--filter <pattern>`: Run only tests matching pattern
- `--coverage`: Generate coverage report

#### Examples
```bash
# Run all tests
cns sparql test

# Run with verbose output
cns sparql test --verbose

# Run specific test pattern
cns sparql test --filter "pattern_matching"
```

## SHACL Domain

The SHACL domain provides high-performance shape validation and constraint checking.

### `cns shacl validate`

Validate data against SHACL shapes.

#### Syntax
```bash
cns shacl validate <data> <shapes> [options]
```

#### Options
- `--format <format>`: Input format (turtle, n3, json-ld)
- `--output <file>`: Output validation report to file
- `--verbose`: Enable verbose validation output
- `--stop-on-error`: Stop validation on first error

#### Examples
```bash
# Validate data against shapes
cns shacl validate data.ttl shapes.ttl

# Validate with custom format
cns shacl validate data.json shapes.ttl --format json-ld

# Validate with detailed output
cns shacl validate data.ttl shapes.ttl --verbose --output validation_report.json
```

#### Performance
- **L1 Tier**: < 10ns for shape loading
- **L2 Tier**: < 100ns for constraint checking
- **L3 Tier**: < 1μs for full validation

### `cns shacl check`

Check specific constraints.

#### Syntax
```bash
cns shacl check <constraint> [options]
```

#### Options
- `--data <file>`: Data file to check
- `--shape <file>`: Shape file containing constraints
- `--constraint <name>`: Specific constraint to check

#### Examples
```bash
# Check minCount constraint
cns shacl check minCount --data data.ttl --shape shapes.ttl

# Check datatype constraint
cns shacl check datatype --constraint "PersonNameConstraint"
```

### `cns shacl benchmark`

Run SHACL performance benchmarks.

#### Syntax
```bash
cns shacl benchmark [options]
```

#### Options
- `--iterations <n>`: Number of benchmark iterations
- `--data-size <n>`: Size of test data
- `--shape-count <n>`: Number of shapes to test
- `--output <file>`: Output results to file

#### Examples
```bash
# Run standard benchmarks
cns shacl benchmark

# Run with large dataset
cns shacl benchmark --data-size 10000 --shape-count 100

# Run with custom output
cns shacl benchmark --output shacl_benchmark.json
```

#### Performance Targets
- **Shape Loading**: < 10ns (7-tick target)
- **Constraint Check**: < 100ns (L2 tier)
- **Full Validation**: < 1μs (L3 tier)

### `cns shacl test`

Run SHACL unit tests.

#### Syntax
```bash
cns shacl test [options]
```

#### Options
- `--verbose`: Enable verbose test output
- `--filter <pattern>`: Run only tests matching pattern
- `--coverage`: Generate coverage report

#### Examples
```bash
# Run all tests
cns shacl test

# Run constraint tests only
cns shacl test --filter "constraint"

# Run with coverage
cns shacl test --coverage
```

## CJinja Domain

The CJinja domain provides sub-microsecond template rendering with support for variables, conditionals, loops, and filters.

### `cns cjinja render`

Render templates with variable substitution.

#### Syntax
```bash
cns cjinja render <template> [variables] [options]
```

#### Options
- `--output <file>`: Output rendered template to file
- `--format <format>`: Output format (text, html, json)
- `--strict`: Enable strict mode for undefined variables
- `--cache`: Enable template caching

#### Examples
```bash
# Simple variable substitution
cns cjinja render "Hello {{name}}!" name=World

# Multiple variables
cns cjinja render "{{greeting}} {{name}}, you are {{age}} years old" \
    greeting=Hello name=Alice age=25

# Template with conditionals
cns cjinja render "{% if user %}{{user.name}}{% else %}Guest{% endif %}" \
    user.name=John

# Render to file
cns cjinja render "Hello {{name}}!" name=World --output greeting.txt
```

#### Performance
- **L1 Tier**: < 10ns for simple variable substitution
- **L2 Tier**: < 100ns for complex templates
- **L3 Tier**: < 1μs for full template rendering

### `cns cjinja compile`

Compile templates for faster rendering.

#### Syntax
```bash
cns cjinja compile <template> [options]
```

#### Options
- `--output <file>`: Output compiled template to file
- `--optimize`: Enable optimization passes
- `--debug`: Include debug information

#### Examples
```bash
# Compile simple template
cns cjinja compile "Hello {{name}}!" --output template.bin

# Compile with optimization
cns cjinja compile "{{#each items}}{{name}}{{/each}}" --optimize

# Compile with debug info
cns cjinja compile "{{user.name}}" --debug
```

### `cns cjinja benchmark`

Run CJinja performance benchmarks.

#### Syntax
```bash
cns cjinja benchmark [options]
```

#### Options
- `--iterations <n>`: Number of benchmark iterations
- `--template-size <n>`: Size of test templates
- `--variable-count <n>`: Number of variables to test
- `--output <file>`: Output results to file

#### Examples
```bash
# Run standard benchmarks
cns cjinja benchmark

# Run with large templates
cns cjinja benchmark --template-size 10000 --variable-count 100

# Run with custom output
cns cjinja benchmark --output cjinja_benchmark.json
```

#### Performance Targets
- **Template Parse**: < 10ns (7-tick target)
- **Variable Resolve**: < 100ns (L2 tier)
- **Template Render**: < 1μs (L3 tier)

### `cns cjinja test`

Run CJinja unit tests.

#### Syntax
```bash
cns cjinja test [options]
```

#### Options
- `--verbose`: Enable verbose test output
- `--filter <pattern>`: Run only tests matching pattern
- `--coverage`: Generate coverage report

#### Examples
```bash
# Run all tests
cns cjinja test

# Run parser tests only
cns cjinja test --filter "parser"

# Run with coverage
cns cjinja test --coverage
```

## Telemetry Domain

The telemetry domain provides OpenTelemetry-like distributed tracing and performance metrics with 7-tick optimization.

### `cns telemetry start`

Start telemetry collection.

#### Syntax
```bash
cns telemetry start [options]
```

#### Options
- `--service <name>`: Service name for telemetry
- `--span-pool <n>`: Size of span pool
- `--metrics-buffer <n>`: Size of metrics buffer
- `--export-interval <s>`: Export interval in seconds
- `--export-format <format>`: Export format (json, protobuf)

#### Examples
```bash
# Start with default settings
cns telemetry start

# Start with custom service name
cns telemetry start --service my-application

# Start with custom configuration
cns telemetry start --span-pool 1000 --metrics-buffer 10000 --export-interval 5
```

### `cns telemetry stop`

Stop telemetry collection.

#### Syntax
```bash
cns telemetry stop [options]
```

#### Options
- `--export`: Export remaining data before stopping
- `--output <file>`: Output final report to file

#### Examples
```bash
# Stop telemetry collection
cns telemetry stop

# Stop and export remaining data
cns telemetry stop --export --output final_report.json
```

### `cns telemetry report`

Generate performance reports.

#### Syntax
```bash
cns telemetry report [options]
```

#### Options
- `--format <format>`: Report format (text, json, csv)
- `--output <file>`: Output report to file
- `--summary`: Generate summary report only
- `--detailed`: Generate detailed report

#### Examples
```bash
# Generate standard report
cns telemetry report

# Generate JSON report
cns telemetry report --format json --output report.json

# Generate summary report
cns telemetry report --summary
```

### `cns telemetry export`

Export telemetry data.

#### Syntax
```bash
cns telemetry export [options]
```

#### Options
- `--format <format>`: Export format (json, protobuf, otlp)
- `--output <file>`: Output file
- `--endpoint <url>`: Export endpoint URL
- `--batch-size <n>`: Batch size for export

#### Examples
```bash
# Export to file
cns telemetry export --format json --output telemetry.json

# Export to endpoint
cns telemetry export --format otlp --endpoint http://localhost:4317

# Export with batching
cns telemetry export --batch-size 1000 --output telemetry.batch
```

### `cns telemetry benchmark`

Run telemetry performance tests.

#### Syntax
```bash
cns telemetry benchmark [options]
```

#### Options
- `--iterations <n>`: Number of benchmark iterations
- `--span-count <n>`: Number of spans to create
- `--metric-count <n>`: Number of metrics to update
- `--output <file>`: Output results to file

#### Examples
```bash
# Run standard benchmarks
cns telemetry benchmark

# Run with high span count
cns telemetry benchmark --span-count 100000

# Run with custom output
cns telemetry benchmark --output telemetry_benchmark.json
```

#### Performance Targets
- **Span Creation**: < 10ns (7-tick target)
- **Metric Update**: < 100ns (L2 tier)
- **Trace Export**: < 1μs (L3 tier)

### `cns telemetry test`

Run telemetry unit tests.

#### Syntax
```bash
cns telemetry test [options]
```

#### Options
- `--verbose`: Enable verbose test output
- `--filter <pattern>`: Run only tests matching pattern
- `--coverage`: Generate coverage report

#### Examples
```bash
# Run all tests
cns telemetry test

# Run span tests only
cns telemetry test --filter "span"

# Run with coverage
cns telemetry test --coverage
```

### `cns telemetry example`

Run telemetry example.

#### Syntax
```bash
cns telemetry example [options]
```

#### Options
- `--duration <s>`: Example duration in seconds
- `--spans <n>`: Number of spans to create
- `--metrics <n>`: Number of metrics to update

#### Examples
```bash
# Run basic example
cns telemetry example

# Run extended example
cns telemetry example --duration 30 --spans 1000 --metrics 500
```

## Benchmark Domain

The benchmark domain provides comprehensive performance testing and validation.

### `cns benchmark all`

Run all benchmarks.

#### Syntax
```bash
cns benchmark all [options]
```

#### Options
- `--iterations <n>`: Number of iterations per benchmark
- `--output <file>`: Output results to file
- `--format <format>`: Output format (text, json, csv)
- `--validate`: Validate 7-tick compliance

#### Examples
```bash
# Run all benchmarks
cns benchmark all

# Run with custom iterations
cns benchmark all --iterations 1000000

# Run with validation
cns benchmark all --validate --output benchmark_results.json
```

### `cns benchmark report`

Generate benchmark reports.

#### Syntax
```bash
cns benchmark report [options]
```

#### Options
- `--input <file>`: Input benchmark results file
- `--output <file>`: Output report file
- `--format <format>`: Report format (text, html, json)
- `--compare <file>`: Compare with previous results

#### Examples
```bash
# Generate report from results
cns benchmark report --input results.json --output report.html

# Compare with previous results
cns benchmark report --input results.json --compare previous.json
```

## Help Commands

### `cns help`

Show general help information.

#### Syntax
```bash
cns help [domain]
```

#### Examples
```bash
# Show general help
cns help

# Show domain help
cns help sparql
cns help shacl
cns help cjinja
cns help telemetry
```

### `cns version`

Show version information.

#### Syntax
```bash
cns version [options]
```

#### Options
- `--verbose`: Show detailed version information
- `--json`: Output in JSON format

#### Examples
```bash
# Show version
cns version

# Show detailed version
cns version --verbose
```

## Performance Characteristics

### 7-Tick Performance Model

All CNS commands are designed to meet strict performance targets:

#### L1 Tier (7-tick): < 10ns
- Command lookup
- Simple argument parsing
- Basic variable substitution
- Span creation
- Shape loading

#### L2 Tier (sub-100ns): < 100ns
- Complex command parsing
- Pattern matching
- Constraint checking
- Template rendering
- Metric updates

#### L3 Tier (sub-μs): < 1μs
- Full query execution
- Complete validation
- Complex template rendering
- Trace export

### Performance Monitoring

All commands automatically track performance and report violations:

```bash
# Example performance output
Command executed in 156.7 ns (L2 Tier)
Performance: ✓ Within target
```

### Performance Validation

Use the `--validate` option to ensure 7-tick compliance:

```bash
cns sparql benchmark --validate
```

This will fail if any operation exceeds the 7-tick target.

## Error Handling

### Error Codes

CNS uses consistent error codes across all domains:

- `CNS_OK` (0): Success
- `CNS_ERROR` (1): General error
- `CNS_ERROR_ARGS` (2): Invalid arguments
- `CNS_ERROR_NOT_FOUND` (3): Command not found
- `CNS_ERROR_PERMISSION` (4): Permission denied
- `CNS_ERROR_IO` (5): I/O error
- `CNS_ERROR_MEMORY` (6): Memory error
- `CNS_ERROR_TIMEOUT` (7): Timeout error
- `CNS_ERROR_CYCLES` (8): Cycle limit exceeded

### Error Reporting

Commands provide detailed error information:

```bash
# Example error output
ERROR: Invalid SPARQL query syntax
  Query: "SELECT ?s WHERE { ?s ?p"
  Error: Unterminated triple pattern
  Position: 25
  Suggestion: Add closing brace
```

## Best Practices

### Command Usage

1. **Use Help**: Always check `--help` for command options
2. **Validate Input**: Use `--validate` options when available
3. **Monitor Performance**: Watch for performance violations
4. **Use Batch Operations**: Use batch modes for multiple operations
5. **Enable Debugging**: Use `--debug` for troubleshooting

### Performance Optimization

1. **Use Appropriate Tiers**: Choose commands that meet your performance needs
2. **Batch Operations**: Group operations for better performance
3. **Cache Results**: Use caching options when available
4. **Monitor Metrics**: Use telemetry to track performance
5. **Validate Compliance**: Regularly check 7-tick compliance

### Integration

1. **Use JSON Output**: Use `--json` for programmatic integration
2. **Handle Errors**: Check return codes and error messages
3. **Use Configuration**: Use configuration files for complex setups
4. **Monitor Performance**: Integrate telemetry for monitoring
5. **Test Thoroughly**: Use test commands to validate functionality

---

*This commands reference provides comprehensive documentation for all CNS commands and their usage patterns.* 