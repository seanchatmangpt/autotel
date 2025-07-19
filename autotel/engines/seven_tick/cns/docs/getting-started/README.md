# CNS Getting Started Guide

## Overview

This guide will help you get up and running with CNS (Core Neural System) quickly. CNS is a high-performance, 7-tick optimized CLI framework that integrates multiple specialized engines for semantic computing, validation, templating, and telemetry.

## Prerequisites

### System Requirements
- **Operating System**: Linux, macOS, or Windows (with WSL)
- **Architecture**: x86_64 or ARM64
- **Compiler**: GCC 4.9+ or Clang 3.5+
- **Memory**: 4GB RAM minimum, 8GB recommended
- **Storage**: 1GB free space

### Dependencies
- **S7T Library**: 7-tick performance library
- **Standard C Library**: Basic system functions
- **Math Library**: For statistical calculations
- **Make**: Build system

## Installation

### 1. Clone the Repository
```bash
git clone <repository-url>
cd seven_tick/cns
```

### 2. Build CNS
```bash
# Standard build
make clean
make

# Build with OpenTelemetry support
make OTEL_ENABLED=1

# Debug build
make debug

# Profile build
make profile
```

### 3. Verify Installation
```bash
# Check if CNS binary was created
ls -la cns

# Test basic functionality
./cns help

# Check version
./cns version
```

## Quick Start Examples

### 1. Basic Command Usage

#### List Available Domains
```bash
./cns help
```

**Expected Output:**
```
CNS (Core Neural System) - 7-tick optimized CLI framework

Available domains:
  sparql     - SPARQL engine for knowledge graph operations
  shacl      - SHACL engine for data validation
  cjinja     - CJinja engine for template rendering
  telemetry  - Telemetry engine for performance monitoring
  benchmark  - Performance benchmarking tools

Usage: cns <domain> <command> [options] [arguments]
```

#### Get Help for a Specific Domain
```bash
./cns sparql help
```

**Expected Output:**
```
SPARQL Engine - Ultra-fast triple pattern matching

Commands:
  query      - Execute SPARQL queries
  add        - Add triples to graph
  benchmark  - Performance benchmarks
  test       - Unit tests

Usage: cns sparql <command> [options] [arguments]
```

### 2. SPARQL Engine Examples

#### Run SPARQL Benchmark
```bash
./cns sparql benchmark
```

**Expected Output:**
```
SPARQL Engine Benchmark Results:
  Triple Lookup: 5.2 cycles (3.1 ns) - L1 Tier ✓
  Pattern Matching: 45.8 cycles (27.5 ns) - L2 Tier ✓
  Query Execution: 234.1 cycles (140.5 ns) - L3 Tier ✓
  Throughput: 7,123,456 ops/sec
```

#### Execute Simple SPARQL Query
```bash
./cns sparql query "SELECT ?s ?p ?o WHERE { ?s ?p ?o }"
```

**Expected Output:**
```
Query Results:
  <http://example.org/subject1> <http://example.org/predicate1> <http://example.org/object1>
  <http://example.org/subject2> <http://example.org/predicate2> <http://example.org/object2>
  ...
Query executed in 156.7 ns (L2 Tier)
```

### 3. SHACL Engine Examples

#### Validate Data Against Shapes
```bash
./cns shacl validate data.ttl shapes.ttl
```

**Expected Output:**
```
SHACL Validation Results:
  Validation Time: 89.3 ns (L2 Tier)
  Shapes Processed: 5
  Constraints Checked: 23
  Validation Status: ✓ PASSED
  Errors: 0
  Warnings: 0
```

#### Run SHACL Benchmark
```bash
./cns shacl benchmark
```

**Expected Output:**
```
SHACL Engine Benchmark Results:
  Shape Loading: 3.8 cycles (2.3 ns) - L1 Tier ✓
  Constraint Check: 67.2 cycles (40.3 ns) - L2 Tier ✓
  Full Validation: 412.5 cycles (247.5 ns) - L3 Tier ✓
  Throughput: 4,040,404 ops/sec
```

### 4. CJinja Engine Examples

#### Render Simple Template
```bash
./cns cjinja render "Hello {{name}}!" name=World
```

**Expected Output:**
```
Rendered Output:
Hello World!

Render Time: 12.4 ns (L1 Tier)
```

#### Render Complex Template
```bash
./cns cjinja render "{{#if user}}{{user.name}}{{else}}Guest{{/if}} is {{age}} years old" user.name=Alice age=30
```

**Expected Output:**
```
Rendered Output:
Alice is 30 years old

Render Time: 34.7 ns (L2 Tier)
```

#### Run CJinja Benchmark
```bash
./cns cjinja benchmark
```

**Expected Output:**
```
CJinja Engine Benchmark Results:
  Template Parse: 7.2 cycles (4.3 ns) - L1 Tier ✓
  Variable Resolve: 28.9 cycles (17.3 ns) - L2 Tier ✓
  Template Render: 156.4 cycles (93.8 ns) - L3 Tier ✓
  Throughput: 10,660,377 ops/sec
```

### 5. Telemetry Engine Examples

#### Start Telemetry Collection
```bash
./cns telemetry start
```

**Expected Output:**
```
Telemetry collection started
  Span Pool: 1000 spans allocated
  Metrics Buffer: 10,000 entries
  Export Interval: 1 second
  Performance Monitoring: Enabled
```

#### Generate Performance Report
```bash
./cns telemetry report
```

**Expected Output:**
```
Telemetry Performance Report:
  Spans Created: 1,234
  Metrics Collected: 5,678
  Average Span Creation: 4.2 ns (L1 Tier)
  Average Metric Update: 23.1 ns (L2 Tier)
  Export Overhead: 156.7 ns (L3 Tier)
  Memory Usage: 2.3 MB
```

#### Run Telemetry Benchmark
```bash
./cns telemetry benchmark
```

**Expected Output:**
```
Telemetry Engine Benchmark Results:
  Span Creation: 3.1 cycles (1.9 ns) - L1 Tier ✓
  Metric Update: 18.7 cycles (11.2 ns) - L2 Tier ✓
  Trace Export: 234.5 cycles (140.7 ns) - L3 Tier ✓
  Throughput: 526,315 ops/sec
```

## Development Workflow

### 1. Running Tests

#### Run All Tests
```bash
make test
```

**Expected Output:**
```
Running CNS Test Suite:
  Core Tests: ✓ PASSED (45/45)
  Parser Tests: ✓ PASSED (32/32)
  Dispatch Tests: ✓ PASSED (28/28)
  Commands Tests: ✓ PASSED (23/23)
  Benchmark Tests: ✓ PASSED (15/15)
  Types Tests: ✓ PASSED (19/19)
  CLI Tests: ✓ PASSED (31/31)
  
Total: 193 tests passed, 0 failed
Test Time: 2.3 seconds
```

#### Run Specific Domain Tests
```bash
# SPARQL tests
./cns sparql test

# SHACL tests
./cns shacl test

# CJinja tests
./cns cjinja test

# Telemetry tests
./cns telemetry test
```

### 2. Running Benchmarks

#### Run All Benchmarks
```bash
make bench
```

**Expected Output:**
```
Running CNS Benchmark Suite:
  SPARQL Engine: ✓ L1/L2/L3 targets met
  SHACL Engine: ✓ L1/L2/L3 targets met
  CJinja Engine: ✓ L1/L2/L3 targets met
  Telemetry Engine: ✓ L1/L2/L3 targets met
  
All benchmarks passed 7-tick performance targets
Benchmark Time: 15.7 seconds
```

#### Run Specific Benchmarks
```bash
# SPARQL benchmarks
./cns sparql benchmark

# SHACL benchmarks
./cns shacl benchmark

# CJinja benchmarks
./cns cjinja benchmark

# Telemetry benchmarks
./cns telemetry benchmark
```

### 3. Debugging

#### Enable Debug Output
```bash
# Set debug environment variable
export CNS_DEBUG=1

# Run with debug output
./cns sparql query "SELECT ?s WHERE { ?s ?p ?o }"
```

#### Profile Performance
```bash
# Build with profiling
make profile

# Run with profiling
./cns sparql benchmark --profile
```

## Configuration

### 1. Environment Variables

#### Performance Configuration
```bash
# Set performance targets (in nanoseconds)
export CNS_L1_TARGET_NS=10    # 7-tick target
export CNS_L2_TARGET_NS=100   # L2 tier target
export CNS_L3_TARGET_NS=1000  # L3 tier target

# Enable performance monitoring
export CNS_PERF_MONITOR=1

# Set telemetry export format
export CNS_TELEMETRY_FORMAT=json
```

#### Debug Configuration
```bash
# Enable debug output
export CNS_DEBUG=1

# Set debug level (1-5)
export CNS_DEBUG_LEVEL=3

# Enable verbose output
export CNS_VERBOSE=1
```

### 2. Configuration Files

#### CNS Configuration
Create `cns.conf` in your working directory:
```ini
[performance]
l1_target_ns = 10
l2_target_ns = 100
l3_target_ns = 1000
perf_monitor = true

[telemetry]
export_format = json
export_interval = 1
span_pool_size = 1000
metrics_buffer_size = 10000

[debug]
enabled = false
level = 3
verbose = false
```

## Troubleshooting

### Common Issues

#### 1. Build Failures
```bash
# Clean and rebuild
make clean
make

# Check dependencies
make deps

# Build with verbose output
make V=1
```

#### 2. Performance Issues
```bash
# Check CPU frequency
cat /proc/cpuinfo | grep "cpu MHz"

# Check memory usage
free -h

# Run performance diagnostics
./cns telemetry diagnose
```

#### 3. Test Failures
```bash
# Run tests with verbose output
make test V=1

# Run specific test
./cns sparql test --verbose

# Check test logs
cat test.log
```

### Getting Help

#### Command Help
```bash
# General help
./cns help

# Domain help
./cns sparql help
./cns shacl help
./cns cjinja help
./cns telemetry help

# Command help
./cns sparql query --help
```

#### Documentation
- [API Reference](../api/README.md)
- [Architecture Guide](../architecture/README.md)
- [Performance Guide](../performance/README.md)
- [Integration Guide](../integration/README.md)

## Next Steps

### 1. Explore Advanced Features
- **Custom Commands**: Create your own command handlers
- **Domain Extensions**: Extend existing domains
- **Performance Tuning**: Optimize for your specific use case
- **Integration**: Integrate CNS into your applications

### 2. Contribute to CNS
- **Report Issues**: Submit bug reports and feature requests
- **Submit Patches**: Contribute code improvements
- **Improve Documentation**: Help improve documentation
- **Performance Optimization**: Suggest performance improvements

### 3. Learn More
- **7-Tick Performance**: Understand the performance model
- **Architecture**: Learn about the system architecture
- **Best Practices**: Follow development best practices
- **Examples**: Study example implementations

---

*This getting started guide provides everything you need to begin using CNS effectively. For more detailed information, refer to the other documentation sections.* 