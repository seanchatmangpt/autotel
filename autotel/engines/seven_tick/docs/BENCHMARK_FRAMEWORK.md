# 7T Engine Benchmarking Framework

## Overview

The 7T Engine Benchmarking Framework is a comprehensive performance measurement and analysis system designed specifically for validating the ≤7 CPU cycle target and sub-10 nanosecond latency requirements of the 7T Engine.

## Key Features

### 🎯 Performance Targets
- **≤7 CPU cycles** per operation (95% of operations)
- **≤10 nanoseconds** latency per operation
- **≥100M operations/second** throughput
- **32 bytes per event** memory efficiency

### 📊 Comprehensive Metrics
- **Cycle-level precision** using `__builtin_readcyclecounter()`
- **Nanosecond timing** using `clock_gettime(CLOCK_MONOTONIC)`
- **Percentile analysis** (P50, P95, P99, P99.9)
- **Target achievement tracking** (operations within ≤7 cycles)
- **Memory usage monitoring** with configurable limits
- **Hardware capability detection** (AVX2, AVX-512, cache sizes)

### 🔧 Framework Components

#### Core Framework (`7t_benchmark_framework.h/c`)
- High-precision timing and cycle counting
- Benchmark result tracking and analysis
- Performance validation against targets
- Memory usage monitoring
- Hardware capability detection

#### Benchmark Suite (`7t_benchmark_suite.c`)
- 8 comprehensive benchmark tests
- Real-world workload simulation
- Performance regression detection
- Continuous monitoring capabilities

#### Python Runner (`7t_benchmark_runner.py`)
- Automated benchmark execution
- Result analysis and reporting
- Performance visualization
- Continuous monitoring
- Multiple output formats (JSON, CSV, HTML)

## Installation and Setup

### Prerequisites
```bash
# Install required packages
sudo apt-get install build-essential python3 python3-pip matplotlib pandas numpy

# Install Python dependencies
pip3 install matplotlib pandas numpy
```

### Compilation
```bash
# Compile benchmark framework
make benchmark-suite

# Or compile individual components
make lib7t_benchmark_framework.a
make 7t_benchmark_suite
```

## Usage

### Basic Benchmark Execution

#### Run Complete Suite
```bash
# Run all benchmarks
make benchmark-suite

# Or directly
./7t_benchmark_suite
```

#### Run with Python Runner
```bash
# Run with default configuration
make benchmark-runner

# Run specific test
make benchmark-test TEST=basic_operation

# Run continuous monitoring
make benchmark-monitor
```

#### Python Runner Options
```bash
# Run with custom configuration
python3 benchmarks/7t_benchmark_runner.py --config my_config.json

# Run specific test with custom iterations
python3 benchmarks/7t_benchmark_runner.py --test basic_operation --iterations 50000

# Run continuous monitoring for 2 hours
python3 benchmarks/7t_benchmark_runner.py --continuous 120

# Custom output directory
python3 benchmarks/7t_benchmark_runner.py --output-dir my_results
```

### Configuration

#### Default Configuration
```json
{
    "target_cycles": 7,
    "target_ns": 10,
    "target_ops_per_sec": 100000000,
    "iterations": {
        "basic_operation": 100000,
        "batch_operation": 10000,
        "telemetry_overhead": 50000,
        "memory_efficiency": 100000,
        "cache_performance": 50000,
        "branch_prediction": 100000,
        "simd_performance": 50000,
        "stress_test": 10000
    },
    "batch_sizes": [10, 50, 100, 500, 1000],
    "memory_limits": [1024, 4096, 8192, 16384],
    "output_formats": ["json", "csv", "html"],
    "generate_plots": true,
    "save_results": true
}
```

#### Custom Configuration File
```json
{
    "target_cycles": 5,
    "target_ns": 8,
    "target_ops_per_sec": 150000000,
    "iterations": {
        "basic_operation": 200000,
        "batch_operation": 20000
    },
    "batch_sizes": [100, 500, 1000],
    "generate_plots": false
}
```

## Benchmark Tests

### 1. Basic TPOT Operation (Latency)
**Purpose**: Measure basic operation latency
**Metrics**: Cycles per operation, nanoseconds per operation
**Target**: ≤7 cycles, ≤10ns

### 2. TPOT Batch Operations (Throughput)
**Purpose**: Measure batch processing performance
**Metrics**: Operations per second, cycles per batch
**Target**: ≥100M ops/sec

### 3. Telemetry Overhead
**Purpose**: Measure telemetry system impact
**Metrics**: Performance with/without telemetry
**Target**: <5% overhead

### 4. Memory Efficiency
**Purpose**: Validate memory usage targets
**Metrics**: Bytes per operation, peak memory usage
**Target**: ≤32 bytes per event

### 5. Cache Performance
**Purpose**: Test cache-friendly access patterns
**Metrics**: Cache hit rates, memory access patterns
**Target**: >95% cache hit rate

### 6. Branch Prediction
**Purpose**: Validate branch prediction optimization
**Metrics**: Branch misprediction rates
**Target**: <2% misprediction rate

### 7. SIMD Performance
**Purpose**: Test vectorized operations
**Metrics**: SIMD utilization, vector operation efficiency
**Target**: >80% SIMD utilization

### 8. Stress Test
**Purpose**: Validate performance under load
**Metrics**: Performance degradation, stability
**Target**: <10% performance degradation

## Output and Analysis

### Console Output
```
=== 7T Engine Benchmark Suite ===
Framework Version: 1.0.0
Target: ≤7 CPU cycles per operation
Target: ≤10 nanoseconds per operation
Target: ≥100 operations per second

=== Hardware Information ===
CPU Frequency: 3.2 GHz
Number of Cores: 8
L1 Cache Size: 32 KB
L2 Cache Size: 256 KB
L3 Cache Size: 8 MB
AVX2 Support: Yes
AVX-512 Support: No
SSE4.2 Support: Yes

=== Benchmark Result: Basic TPOT Operation ===
Operations: 100000
Total time: 1.234 ms (1234000 ns)
Total cycles: 1234000
Average cycles per operation: 6.17
Average time per operation: 8.23 ns
Throughput: 81000000 ops/sec

Cycle Distribution:
  Min: 5 cycles
  P50: 6.0 cycles
  P95: 7.2 cycles
  P99: 8.1 cycles
  P99.9: 9.5 cycles
  Max: 12 cycles

Target Achievement:
  Operations within ≤7 cycles: 95000/100000 (95.0%)
  Status: ✅ PASSED
```

### Generated Files

#### Performance Plots
- `benchmark_plots/performance_summary.png` - Comprehensive performance visualization

#### Data Files
- `benchmark_results/benchmark_results_YYYYMMDD_HHMMSS.json` - Detailed results in JSON
- `benchmark_results/benchmark_results_YYYYMMDD_HHMMSS.csv` - Results in CSV format
- `benchmark_results/benchmark_report_YYYYMMDD_HHMMSS.html` - HTML report

#### HTML Report Features
- Performance summary with target validation
- Detailed results table
- Performance recommendations
- Interactive visualizations

## Performance Validation

### Success Criteria
```c
// Target validation function
int benchmark_validate_target(BenchmarkResult* result) {
    return result->target_achievement_percent >= 95.0 &&
           result->avg_cycles_per_op <= SEVEN_TICK_TARGET_CYCLES &&
           result->avg_time_ns_per_op <= SEVEN_TICK_TARGET_NS;
}
```

### Validation Rules
1. **95% of operations** must complete within ≤7 cycles
2. **Average latency** must be ≤10 nanoseconds
3. **Throughput** must be ≥100M operations/second
4. **Memory efficiency** must be ≤32 bytes per event

### Regression Detection
```python
# Performance regression detection
def check_regressions(self):
    if len(self.history) < 2:
        return
    
    current = self.history[-1]
    previous = self.history[-2]
    
    for curr_result, prev_result in zip(current["results"], previous["results"]):
        cycles_change = ((curr_result.get("avg_cycles", 0) - prev_result.get("avg_cycles", 0)) / 
                       prev_result.get("avg_cycles", 1)) * 100
        
        if cycles_change > 10:  # 10% regression threshold
            print(f"⚠️  Performance regression detected: {cycles_change:.1f}% increase")
```

## Continuous Monitoring

### Setup Continuous Monitoring
```bash
# Run monitoring for 1 hour
python3 benchmarks/7t_benchmark_runner.py --continuous 60

# Run monitoring for 24 hours
python3 benchmarks/7t_benchmark_runner.py --continuous 1440
```

### Monitoring Features
- **Automatic regression detection** with configurable thresholds
- **Performance trend analysis** using linear regression
- **Real-time alerts** for performance degradation
- **Historical data tracking** for trend analysis

## Integration with CI/CD

### GitHub Actions Example
```yaml
name: 7T Engine Benchmarks
on: [push, pull_request]

jobs:
  benchmark:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - name: Install dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y build-essential python3 python3-pip
          pip3 install matplotlib pandas numpy
      - name: Compile benchmarks
        run: make benchmark-suite
      - name: Run benchmarks
        run: make benchmark-runner
      - name: Upload results
        uses: actions/upload-artifact@v2
        with:
          name: benchmark-results
          path: benchmark_results/
```

### Jenkins Pipeline Example
```groovy
pipeline {
    agent any
    
    stages {
        stage('Compile') {
            steps {
                sh 'make benchmark-suite'
            }
        }
        
        stage('Benchmark') {
            steps {
                sh 'make benchmark-runner'
            }
        }
        
        stage('Analyze') {
            steps {
                script {
                    def results = readJSON file: 'benchmark_results/benchmark_results.json'
                    def analysis = results.analysis
                    
                    if (analysis.summary.avg_cycles > 7) {
                        error "Performance target not met: ${analysis.summary.avg_cycles} cycles"
                    }
                }
            }
        }
    }
}
```

## Troubleshooting

### Common Issues

#### High Cycle Counts
**Symptoms**: Average cycles > 7, target achievement < 95%
**Causes**: 
- Memory access patterns not optimized
- Cache misses
- Branch mispredictions
**Solutions**:
- Check cache line alignment
- Optimize data access patterns
- Add branch prediction hints

#### High Latency
**Symptoms**: Average time > 10ns
**Causes**:
- CPU frequency scaling
- Memory bandwidth bottlenecks
- Suboptimal compiler flags
**Solutions**:
- Set CPU governor to performance
- Use aligned memory allocation
- Enable compiler optimizations

#### Low Throughput
**Symptoms**: Operations/second < 100M
**Causes**:
- Inefficient batch processing
- Memory allocation overhead
- Suboptimal algorithms
**Solutions**:
- Implement SIMD optimizations
- Use memory pools
- Optimize batch sizes

### Debug Mode
```bash
# Compile with debug information
CFLAGS="-g -O0" make benchmark-suite

# Run with verbose output
./7t_benchmark_suite --verbose

# Profile with gprof
CFLAGS="-pg" make benchmark-suite
./7t_benchmark_suite
gprof 7t_benchmark_suite gmon.out > profile.txt
```

## Performance Tuning

### Compiler Optimizations
```bash
# Maximum optimization
CFLAGS="-O3 -march=native -mtune=native -ffast-math -funroll-loops -fomit-frame-pointer"

# Profile-guided optimization
CFLAGS="-O3 -fprofile-generate" make benchmark-suite
./7t_benchmark_suite
CFLAGS="-O3 -fprofile-use" make benchmark-suite
```

### Runtime Optimizations
```bash
# Set CPU governor to performance
echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor

# Disable CPU frequency scaling
sudo cpupower frequency-set -g performance

# Set process priority
sudo nice -n -20 ./7t_benchmark_suite
```

### Memory Optimizations
```bash
# Huge pages
echo 1024 | sudo tee /proc/sys/vm/nr_hugepages

# Memory defragmentation
echo 1 | sudo tee /proc/sys/vm/compact_memory
```

## Conclusion

The 7T Engine Benchmarking Framework provides comprehensive performance measurement and validation capabilities specifically designed for the ≤7 CPU cycle target. The framework enables:

- **Precise performance measurement** with cycle-level accuracy
- **Comprehensive validation** against all performance targets
- **Automated regression detection** for continuous monitoring
- **Rich reporting and visualization** for performance analysis
- **Easy integration** with CI/CD pipelines

The framework ensures that the 7T Engine consistently meets its performance targets and provides early warning of any performance regressions. 