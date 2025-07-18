# PM7T Benchmarking Guide

## Overview
This guide provides comprehensive instructions for benchmarking the PM7T process mining library across different use cases and performance scenarios.

## 5 Benchmark Use Cases

### 1. High-Volume Banking Transactions
**Purpose**: Test PM7T performance with large-scale financial transaction processing.

**Characteristics**:
- 1,000,000 events
- 100,000 banking cases
- 8 transaction types
- 50 tellers/resources
- Standard banking workflow patterns

**Performance Targets**:
- Event processing: < 10ns per event
- Throughput: > 100M events/sec
- Memory usage: < 50MB for 1M events

**Key Metrics**:
- Event log creation time
- Trace extraction performance
- Alpha algorithm discovery speed
- Conformance checking latency

### 2. Healthcare Patient Care Pathways
**Purpose**: Evaluate PM7T with complex healthcare workflows and variable case lengths.

**Characteristics**:
- 500,000 events
- 10,000 patient care pathways
- 15 medical activities
- 100 medical staff
- Complex, variable-length workflows

**Performance Targets**:
- Event processing: < 15ns per event
- Throughput: > 50M events/sec
- Memory usage: < 25MB for 500K events

**Key Metrics**:
- Performance analysis speed
- Heuristic mining efficiency
- Memory usage with variable case lengths
- Healthcare-specific pattern recognition

### 3. E-commerce Order Processing
**Purpose**: Test real-time monitoring capabilities with filtering and analysis.

**Characteristics**:
- 300,000 events
- 50,000 order workflows
- 12 order activities
- 200 warehouse workers
- Real-time filtering requirements

**Performance Targets**:
- Event processing: < 20ns per event
- Throughput: > 30M events/sec
- Memory usage: < 15MB for 300K events

**Key Metrics**:
- Real-time filtering performance
- Bottleneck analysis speed
- Variant analysis efficiency
- Resource utilization patterns

### 4. Manufacturing IoT Production Lines
**Purpose**: Evaluate high-frequency IoT data processing capabilities.

**Characteristics**:
- 2,000,000 events
- 20,000 production batches
- 20 manufacturing steps
- 500 IoT sensors
- High-frequency data streams

**Performance Targets**:
- Event processing: < 5ns per event
- Throughput: > 200M events/sec
- Memory usage: < 100MB for 2M events

**Key Metrics**:
- High-frequency trace extraction
- Inductive mining performance
- Social network analysis speed
- IoT data processing efficiency

### 5. Financial Trading Systems
**Purpose**: Test ultra-low latency performance for high-frequency trading.

**Characteristics**:
- 5,000,000 events
- 100,000 trading sessions
- 10 trading activities
- 1000 trading algorithms
- Ultra-low latency requirements

**Performance Targets**:
- Event processing: < 2ns per event
- Throughput: > 500M events/sec
- Memory usage: < 200MB for 5M events

**Key Metrics**:
- Ultra-low latency processing
- Multi-algorithm comparison
- Comprehensive conformance checking
- Trading system optimization

## Running Benchmarks

### Prerequisites
```bash
# Install required tools
sudo apt-get install build-essential linux-tools-common linux-tools-generic

# Set up performance monitoring
echo -1 | sudo tee /proc/sys/kernel/perf_event_paranoid
```

### Compilation
```bash
# Compile PM7T library
make libpm7t.so

# Compile benchmarks with optimizations
make pm7t_benchmarks
```

### Execution
```bash
# Run all benchmarks
make benchmark

# Run specific use case
./pm7t_benchmarks

# Run with performance monitoring
perf stat ./pm7t_benchmarks

# Run with detailed profiling
perf record ./pm7t_benchmarks
perf report
```

## Performance Analysis

### Latency Measurement
```c
// High-precision timing
static inline uint64_t get_nanoseconds() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

// CPU cycle counting
static inline uint64_t get_cycles() {
    return __builtin_readcyclecounter();
}
```

### Memory Usage Tracking
```c
// Memory usage monitoring
size_t memory_usage = pm7t_get_memory_usage();
printf("Memory usage: %.2f MB\n", memory_usage / (1024.0 * 1024.0));
```

### Throughput Calculation
```c
double throughput = operations / (time_ns / 1000000000.0);
printf("Throughput: %.0f ops/sec\n", throughput);
```

## Benchmark Results Interpretation

### Success Criteria
- **Latency**: All operations complete within target times
- **Throughput**: Achieve or exceed target event processing rates
- **Memory**: Stay within memory usage limits
- **Scalability**: Linear performance scaling with data size

### Performance Indicators
- **Event Processing**: < 10ns per event (target)
- **Trace Extraction**: < 1μs per trace (target)
- **Process Discovery**: < 1ms per model (target)
- **Conformance Checking**: < 1ms per case (target)

### Memory Efficiency
- **Event Storage**: ~32 bytes per event
- **Trace Storage**: ~8 bytes per activity
- **Model Storage**: ~24 bytes per transition
- **Linear Scaling**: Memory usage scales linearly with data size

## Optimization Techniques

### Compiler Optimizations
```bash
# Maximum optimization flags
CFLAGS="-O3 -march=native -mtune=native -ffast-math -funroll-loops -fomit-frame-pointer"
```

### Memory Management
```c
// Set appropriate memory limits
pm7t_set_memory_limit(4ULL * 1024 * 1024 * 1024); // 4GB

// Monitor memory usage
size_t usage = pm7t_get_memory_usage();
```

### Performance Tuning
```c
// Use appropriate data structures
EventLog* log = pm7t_create_event_log(expected_capacity);

// Batch operations for better performance
for (size_t i = 0; i < batch_size; i++) {
    pm7t_add_event(log, cases[i], activities[i], timestamps[i], resources[i], costs[i]);
}
```

## Benchmark Environment

### Hardware Requirements
- **CPU**: Modern x86_64 processor with AVX2 support
- **Memory**: 8GB+ RAM for large datasets
- **Storage**: SSD for I/O operations
- **OS**: Linux with performance monitoring tools

### Software Requirements
- **Compiler**: GCC 9+ with optimization support
- **Tools**: perf, valgrind, gprof
- **Libraries**: Standard C library, math library

### Environment Setup
```bash
# Set CPU governor to performance
echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor

# Disable CPU frequency scaling
sudo cpupower frequency-set -g performance

# Set process priority
sudo nice -n -20 ./pm7t_benchmarks
```

## Troubleshooting

### Common Issues
1. **Memory allocation failures**: Increase memory limits
2. **Performance degradation**: Check CPU governor settings
3. **Inconsistent results**: Ensure system is idle during benchmarks
4. **Compilation errors**: Verify compiler and library versions

### Debugging
```bash
# Run with debug information
CFLAGS="-g -O0" make pm7t_benchmarks

# Use valgrind for memory analysis
valgrind --tool=memcheck ./pm7t_benchmarks

# Profile with gprof
CFLAGS="-pg" make pm7t_benchmarks
./pm7t_benchmarks
gprof pm7t_benchmarks gmon.out > profile.txt
```

## Reporting Results

### Standard Format
```
=== Use Case X: Description ===
Event Log Creation:
  Time: X.XXX ms
  Throughput: XXX ops/sec
  Latency: XX.X ns/op
  Memory: XX.X MB

Trace Extraction:
  Time: X.XXX ms
  Throughput: XXX ops/sec
  Latency: XX.X ns/op

Process Discovery:
  Time: X.XXX ms
  Transitions: XXX
  Latency: XX.X ns/transition
```

### Performance Summary
- **Latency Achievements**: Compare against targets
- **Throughput Achievements**: Measure actual vs expected
- **Memory Efficiency**: Bytes per event/operation
- **Scalability**: Performance with different data sizes

## Continuous Benchmarking

### Automated Testing
```bash
#!/bin/bash
# benchmark_runner.sh

echo "Running PM7T benchmarks..."
make benchmark

echo "Generating report..."
./generate_report.sh

echo "Comparing with previous results..."
./compare_results.sh
```

### Performance Regression Testing
- Run benchmarks on every commit
- Compare against baseline performance
- Alert on performance degradation
- Track performance trends over time

This benchmarking guide ensures comprehensive evaluation of PM7T performance across all use cases and provides the tools needed for continuous performance monitoring and optimization. 