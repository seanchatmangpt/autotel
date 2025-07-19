# CNS Comprehensive Benchmark Suite

A complete performance validation framework for the CNS (CHATMAN NANO-STACK) compiler, designed to measure and validate 7-tick performance compliance across all system components.

## Overview

This benchmark suite provides comprehensive testing of CNS functionality with **real performance measurements** (not mocked results). It covers:

- **Core CNS functionality** (CLI parsing, memory management, string processing)
- **7T-SQL operations** (SELECT, INSERT, aggregation, SIMD optimization)
- **SPARQL query processing** (pattern matching, joins, graph traversal)
- **SHACL constraint validation** (shape checking, rule enforcement)
- **CJinja template processing** (rendering, compilation, caching)
- **Memory operations** (arena allocation, caching, optimization)
- **Telemetry integration** (OpenTelemetry spans, metrics, tracing)

## Key Features

### ✅ Real Performance Measurement
- Uses actual CPU cycle counting (not clock() overhead)
- Tests genuine CNS operations with realistic data
- Provides accurate 7-tick compliance validation
- Generates both text and OTEL format reports

### 🎯 7-Tick Performance Validation
- Target: ≤ 7 CPU cycles per core operation
- Validates physics-compliant performance constraints
- Identifies operations exceeding cycle budget
- Provides optimization recommendations

### 📊 Comprehensive Reporting
- Text-based performance reports
- JSON/OTEL structured output for tooling
- HTML reports with interactive visualizations
- Integration with monitoring and alerting systems

### 🔧 Multiple Execution Modes
- **Quick mode**: 1K iterations for CI/CD pipelines
- **Standard mode**: 10K iterations for regular testing
- **Comprehensive mode**: 50K+ iterations for deep analysis
- **Profile mode**: Generates profiling data for optimization

## Files in This Suite

### Core Benchmark Files

| File | Description | Coverage |
|------|-------------|----------|
| `cns_comprehensive_benchmark.c` | Main benchmark suite | Core CNS functionality, memory, string processing |
| `cns_sql_benchmarks.c` | 7T-SQL dedicated tests | SELECT, INSERT, aggregation, SIMD operations |
| `cns_sparql_benchmarks.c` | SPARQL query tests | Pattern matching, joins, graph operations |

### Build and Execution Files

| File | Description | Purpose |
|------|-------------|---------|
| `Makefile.benchmark` | Benchmark build system | Compiles all benchmark executables |
| `run_comprehensive_benchmarks.sh` | Main execution script | Runs all benchmarks and generates reports |

### Generated Files (After Execution)

| Directory/File | Description | Contains |
|----------------|-------------|----------|
| `benchmark_results/` | All benchmark outputs | Results, logs, reports, profiles |
| `benchmark_results/logs/` | Detailed execution logs | Text output from each benchmark |
| `benchmark_results/json/` | Structured data | JSON/OTEL formatted results |
| `benchmark_results/profiles/` | Profiling data | gprof and perf analysis data |

## Quick Start

### 1. Build CNS and Benchmarks

```bash
# Build CNS compiler
make

# Build benchmark suite
make -f Makefile.benchmark
```

### 2. Run Standard Benchmarks

```bash
# Run standard benchmark suite (10K iterations)
./run_comprehensive_benchmarks.sh

# Or use the makefile
make -f Makefile.benchmark bench
```

### 3. View Results

```bash
# View comprehensive report
cat benchmark_results/cns_benchmark_*_comprehensive_report.txt

# View specific benchmark logs
ls benchmark_results/logs/

# View JSON data for tooling integration
cat benchmark_results/json/cns_benchmark_*_core.json
```

## Usage Examples

### Quick CI/CD Testing
```bash
# Fast benchmarks for continuous integration
./run_comprehensive_benchmarks.sh --quick
```

### Comprehensive Analysis
```bash
# Full benchmark suite with all reporting
./run_comprehensive_benchmarks.sh --comprehensive --otel --html
```

### Performance Profiling
```bash
# Generate profiling data for optimization
./run_comprehensive_benchmarks.sh --profile
```

### Custom Configuration
```bash
# Custom results directory and specific mode
./run_comprehensive_benchmarks.sh --standard --results-dir ./my_results --html
```

## Benchmark Categories

### 1. Core CNS Functionality
- **CLI Parsing**: Command line argument processing and validation
- **Memory Arena**: Zero-copy arena allocation and management
- **String Operations**: Hashing, parsing, and manipulation
- **Domain Registration**: Plugin and module loading

### 2. 7T-SQL Operations
- **SELECT Operations**: Full table scans, filtered queries, range queries
- **SIMD Optimization**: AVX2/NEON vectorized filtering and processing
- **Aggregation Functions**: SUM, COUNT, AVG, MIN, MAX operations
- **Join Operations**: Hash joins and nested loop joins
- **DML Operations**: INSERT, UPDATE, DELETE with performance validation

### 3. SPARQL Query Processing
- **Triple Pattern Matching**: Basic (?s ?p ?o) pattern evaluation
- **Complex Patterns**: Property paths, OPTIONAL, UNION queries
- **Graph Operations**: Named graph queries and traversal
- **Join Optimization**: Multi-pattern query execution

### 4. SHACL Constraint Validation
- **Shape Checking**: NodeShape and PropertyShape validation
- **Constraint Types**: minCount, maxCount, datatype, pattern validation
- **Rule Enforcement**: SHACL-AF rules and custom constraints
- **Performance Scaling**: Validation performance with graph size

### 5. CJinja Template Processing
- **Template Rendering**: Variable substitution and control structures
- **Compilation**: Template to bytecode compilation
- **Caching**: Template caching and reuse optimization
- **Complex Templates**: Nested templates and macro expansion

## Performance Targets

### 7-Tick Compliance
The benchmark suite validates that core operations complete within **7 CPU cycles** (approximately 2.3ns at 3GHz):

| Operation Category | Target | Typical Range |
|-------------------|--------|---------------|
| Memory allocation | ≤ 7 cycles | 3-5 cycles |
| String hashing | ≤ 7 cycles | 4-6 cycles |
| CLI parsing | ≤ 7 cycles/token | 5-8 cycles |
| SQL row processing | ≤ 7 cycles/row | 6-12 cycles |
| SPARQL triple match | ≤ 7 cycles/triple | 8-15 cycles |

### Performance Interpretation

| Compliance Rate | Status | Action Required |
|----------------|--------|-----------------|
| 100% | ✅ **EXCELLENT** | No action needed |
| 80-99% | ⚠️ **GOOD** | Monitor and optimize outliers |
| 60-79% | ⚠️ **FAIR** | Investigate performance issues |
| <60% | ❌ **POOR** | Major optimization required |

## Output Formats

### 1. Text Reports
Human-readable performance summaries with:
- Execution statistics (min/max/avg cycles)
- 7-tick compliance indicators
- Performance recommendations
- System information and configuration

### 2. JSON/OTEL Output
Structured data format for integration with:
- Monitoring systems (Prometheus, Grafana)
- CI/CD pipelines (Jenkins, GitHub Actions)
- Performance tracking databases
- Alert management systems

### 3. HTML Reports
Interactive web-based reports with:
- Visual performance charts
- Trend analysis over time
- Detailed breakdowns by category
- Exportable data and sharing capabilities

## Integration with Monitoring

### Prometheus Integration
```yaml
# Example Prometheus scrape config
scrape_configs:
  - job_name: 'cns-benchmarks'
    static_configs:
      - targets: ['localhost:8080']
    metrics_path: '/metrics'
    scrape_interval: 300s
```

### Grafana Dashboard
Key metrics to monitor:
- Average cycle count per operation type
- 7-tick compliance percentage over time
- Performance regression detection
- Resource utilization during benchmarks

### Alerting Rules
```yaml
# Example alert for performance regression
groups:
  - name: cns-performance
    rules:
      - alert: CNSPerformanceRegression
        expr: cns_avg_cycles > 7
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "CNS operation exceeding 7-tick budget"
```

## Troubleshooting

### Common Issues

#### Build Failures
```bash
# Missing dependencies
sudo apt-get install build-essential clang libc6-dev

# Missing SIMD support
export CFLAGS="-march=native -mavx2"
make -f Makefile.benchmark clean && make -f Makefile.benchmark
```

#### Performance Anomalies
```bash
# Run with profiling to identify bottlenecks
./run_comprehensive_benchmarks.sh --profile

# Check system load during benchmarks
./run_comprehensive_benchmarks.sh --quick  # Reduce system impact
```

#### Memory Issues
```bash
# Run with memory checking
make -f Makefile.benchmark bench-memcheck

# Monitor memory usage during benchmarks
top -p $(pgrep cns_comprehensive_benchmark)
```

### Performance Tuning

#### CPU Scaling
```bash
# Set CPU governor to performance mode
echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
```

#### NUMA Optimization
```bash
# Run on specific NUMA node
numactl --cpunodebind=0 --membind=0 ./run_comprehensive_benchmarks.sh
```

#### Compiler Optimization
```bash
# Enable aggressive optimization
export CFLAGS="-O3 -march=native -flto -ffast-math"
make -f Makefile.benchmark clean && make -f Makefile.benchmark
```

## Contributing

### Adding New Benchmarks

1. **Create benchmark function**:
```c
static benchmark_result_t benchmark_my_operation(test_data_t* data, uint64_t iterations) {
    // Implementation here
}
```

2. **Add to main runner**:
```c
results[result_count++] = benchmark_my_operation(&test_data, iterations);
```

3. **Update this README** with new benchmark description

### Modifying Performance Targets

Edit the 7-tick validation logic in each benchmark file:
```c
result.seven_tick_compliant = (result.cycles_per_operation <= TARGET_CYCLES);
```

### Adding New Output Formats

Implement new export functions following the pattern in existing benchmarks:
```c
static void export_my_format(benchmark_result_t* results, size_t count, const char* filename);
```

## Technical Implementation

### Cycle Measurement
The benchmarks use hardware-specific cycle counters for maximum accuracy:

```c
static inline uint64_t get_cycles(void) {
#if defined(__x86_64__) || defined(__i386__)
    uint32_t lo, hi;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
#elif defined(__aarch64__)
    uint64_t val;
    __asm__ __volatile__("mrs %0, cntvct_el0" : "=r" (val));
    return val;
#endif
}
```

### Statistical Analysis
Each benchmark calculates:
- **Mean, min, max**: Basic performance statistics
- **Standard deviation**: Measurement consistency
- **Percentiles**: P95, P99 for tail performance
- **Outlier detection**: Identifies anomalous measurements

### Memory Management
Benchmarks use:
- **Aligned allocation**: SIMD-optimized data structures
- **Arena allocators**: Zero-copy memory management
- **Realistic data**: Production-like data patterns and sizes

## License

This benchmark suite is part of the CNS project and follows the same licensing terms.

## Support

For questions, issues, or contributions:
- Review the benchmark output logs for detailed error information
- Check system prerequisites and dependencies
- Verify CNS is built and functional before running benchmarks
- Monitor system resources during benchmark execution

---

**Note**: The benchmark results reflect the actual performance of CNS operations. While the 7-tick target is extremely aggressive, it represents the physics-compliant performance goal of the CNS compiler architecture.