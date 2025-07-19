# Benchmark Refactoring Guide

## Overview

This guide outlines the process for refactoring existing benchmarks to use the simplified 7T benchmark framework. The framework provides consistent performance measurement, reporting, and analysis across all benchmarks.

## Framework Benefits

### ✅ **Consistent Measurement**
- High-precision timing using `clock_gettime(CLOCK_MONOTONIC)`
- Cycle counting for performance analysis
- Standardized result structures

### ✅ **Comprehensive Reporting**
- Detailed performance metrics
- Target validation against ≤7 cycles and ≤10ns
- JSON and CSV export capabilities
- Hardware information detection

### ✅ **Cross-Platform Compatibility**
- Works on x86, ARM, and other architectures
- No architecture-specific dependencies
- Portable across different systems

### ✅ **Performance Analysis**
- 7-tick vs 49-tick path comparisons
- Throughput calculations
- Target achievement tracking
- Regression detection capabilities

## Framework Components

### Core Structures

```c
// Benchmark result structure
typedef struct {
    const char *test_name;
    uint64_t total_cycles;
    uint64_t total_time_ns;
    size_t operations;
    double avg_cycles_per_op;
    double avg_time_ns_per_op;
    double ops_per_sec;
    uint64_t min_cycles;
    uint64_t max_cycles;
    size_t operations_within_target;
    double target_achievement_percent;
    int passed;
} BenchmarkResult;

// Benchmark suite structure
typedef struct {
    const char *suite_name;
    BenchmarkResult *results;
    size_t result_count;
    size_t result_capacity;
    uint64_t total_suite_time_ns;
    double overall_score;
} BenchmarkSuite;
```

### Key Functions

```c
// Create and manage benchmark suites
BenchmarkSuite *benchmark_suite_create(const char *suite_name);
void benchmark_suite_add_result(BenchmarkSuite *suite, BenchmarkResult result);
void benchmark_suite_calculate_stats(BenchmarkSuite *suite);
void benchmark_suite_destroy(BenchmarkSuite *suite);

// Execute benchmarks
BenchmarkResult benchmark_execute_single(
    const char *test_name,
    size_t iterations,
    void (*test_function)(void *),
    void *test_data);

// Result reporting
void benchmark_result_print(BenchmarkResult *result);
void benchmark_suite_print_detailed(BenchmarkSuite *suite);
void benchmark_suite_export_json(BenchmarkSuite *suite, const char *filename);
void benchmark_suite_export_csv(BenchmarkSuite *suite, const char *filename);

// Performance validation
int benchmark_validate_target(BenchmarkResult *result);
```

## Refactoring Process

### Step 1: Include Framework

```c
#include "cjinja_benchmark_simple_framework.c"  // Include the framework
```

### Step 2: Define Test Data Structure

```c
typedef struct {
    // Your test-specific data
    CJinjaEngine *engine;
    CJinjaContext *ctx;
    const char *template;
    // ... other data
} YourTestData;
```

### Step 3: Create Test Functions

```c
void benchmark_your_test(void *data) {
    YourTestData *test_data = (YourTestData *)data;
    
    // Your benchmark code here
    char *result = cjinja_render_string(test_data->template, test_data->ctx);
    free(result);
}
```

### Step 4: Setup and Teardown

```c
YourTestData* setup_test_data(void) {
    YourTestData *data = malloc(sizeof(YourTestData));
    // Initialize your test data
    return data;
}

void cleanup_test_data(YourTestData *data) {
    // Cleanup your test data
    free(data);
}
```

### Step 5: Main Benchmark Function

```c
int main() {
    // Create benchmark suite
    BenchmarkSuite *suite = benchmark_suite_create("Your Benchmark Suite");
    
    // Setup test data
    YourTestData *test_data = setup_test_data();
    
    // Run benchmarks
    BenchmarkResult result = benchmark_execute_single(
        "Your Test Name",
        10000,  // iterations
        benchmark_your_test,
        test_data
    );
    benchmark_suite_add_result(suite, result);
    
    // Calculate and print results
    benchmark_suite_calculate_stats(suite);
    benchmark_suite_print_detailed(suite);
    
    // Export results
    benchmark_suite_export_json(suite, "your_results.json");
    benchmark_suite_export_csv(suite, "your_results.csv");
    
    // Cleanup
    cleanup_test_data(test_data);
    benchmark_suite_destroy(suite);
    
    return 0;
}
```

## Benchmarks to Refactor

### 1. CJinja Benchmarks ✅ (COMPLETED)
- **File**: `cjinja_benchmark_simple_framework.c`
- **Status**: ✅ Refactored and working
- **Features**: 7-tick vs 49-tick comparisons, all CJinja features

### 2. SPARQL Benchmarks
- **Files**: `sparql_benchmark.c`, `sparql_80_20_benchmark.c`
- **Status**: 🔄 Pending refactoring
- **Features**: Query performance, batch operations

### 3. SHACL Benchmarks
- **Files**: `shacl_validation_benchmark.c`, `shacl_implementation_benchmark.c`
- **Status**: 🔄 Pending refactoring
- **Features**: Validation performance, rule processing

### 4. OWL Benchmarks
- **Files**: `owl_80_20_benchmark.c`, `owl_reasoning_benchmark.c`
- **Status**: 🔄 Pending refactoring
- **Features**: Reasoning performance, inference speed

### 5. Telemetry Benchmarks
- **Files**: `telemetry7t_benchmark.c`, `telemetry7t_7tick_benchmark.c`
- **Status**: 🔄 Pending refactoring
- **Features**: Telemetry overhead, performance impact

### 6. Integration Benchmarks
- **Files**: `7t_ultra_fast_benchmark.c`, `feature_comparison_benchmark.c`
- **Status**: 🔄 Pending refactoring
- **Features**: End-to-end performance, feature comparisons

## Refactoring Templates

### Template 1: Simple Performance Test

```c
#include "cjinja_benchmark_simple_framework.c"

typedef struct {
    // Your test data
} TestData;

void benchmark_simple_test(void *data) {
    TestData *test_data = (TestData *)data;
    // Your test code
}

int main() {
    BenchmarkSuite *suite = benchmark_suite_create("Simple Test Suite");
    TestData *test_data = setup_test_data();
    
    BenchmarkResult result = benchmark_execute_single(
        "Simple Test",
        10000,
        benchmark_simple_test,
        test_data
    );
    benchmark_suite_add_result(suite, result);
    
    benchmark_suite_calculate_stats(suite);
    benchmark_suite_print_detailed(suite);
    
    cleanup_test_data(test_data);
    benchmark_suite_destroy(suite);
    return 0;
}
```

### Template 2: Comparison Test (7-tick vs 49-tick)

```c
void benchmark_7tick_test(void *data) {
    // 7-tick path test
}

void benchmark_49tick_test(void *data) {
    // 49-tick path test
}

int main() {
    BenchmarkSuite *suite = benchmark_suite_create("Comparison Suite");
    TestData *test_data = setup_test_data();
    
    // 7-tick test
    BenchmarkResult result_7tick = benchmark_execute_single(
        "Test (7-tick)",
        10000,
        benchmark_7tick_test,
        test_data
    );
    benchmark_suite_add_result(suite, result_7tick);
    
    // 49-tick test
    BenchmarkResult result_49tick = benchmark_execute_single(
        "Test (49-tick)",
        10000,
        benchmark_49tick_test,
        test_data
    );
    benchmark_suite_add_result(suite, result_49tick);
    
    // Analysis and reporting
    benchmark_suite_calculate_stats(suite);
    benchmark_suite_print_detailed(suite);
    
    cleanup_test_data(test_data);
    benchmark_suite_destroy(suite);
    return 0;
}
```

### Template 3: Batch Operations Test

```c
void benchmark_batch_test(void *data) {
    TestData *test_data = (TestData *)data;
    
    // Perform batch operations
    for (int i = 0; i < 10; i++) {
        // Your batch operation
    }
}

int main() {
    BenchmarkSuite *suite = benchmark_suite_create("Batch Test Suite");
    TestData *test_data = setup_test_data();
    
    BenchmarkResult result = benchmark_execute_single(
        "Batch Operations",
        1000,  // Fewer iterations for batch tests
        benchmark_batch_test,
        test_data
    );
    benchmark_suite_add_result(suite, result);
    
    benchmark_suite_calculate_stats(suite);
    benchmark_suite_print_detailed(suite);
    
    cleanup_test_data(test_data);
    benchmark_suite_destroy(suite);
    return 0;
}
```

## Performance Targets

### 7-Tick Path Targets
- **≤7 CPU cycles** per operation (95% of operations)
- **≤10 nanoseconds** latency per operation
- **≥100M operations/second** throughput

### 49-Tick Path Targets
- **≤49 CPU cycles** per operation (95% of operations)
- **≤100 nanoseconds** latency per operation
- **≥10M operations/second** throughput

### Validation Rules
```c
int benchmark_validate_target(BenchmarkResult *result) {
    return result->target_achievement_percent >= 95.0 &&
           result->avg_cycles_per_op <= TARGET_CYCLES &&
           result->avg_time_ns_per_op <= TARGET_NS;
}
```

## Output Formats

### Console Output
```
=== Benchmark Result: Test Name ===
Operations: 10000
Total time: 2.153 ms (2153000 ns)
Total cycles: 2153000
Average cycles per operation: 215.3
Average time per operation: 215.3 ns
Throughput: 4644682 ops/sec

Target Achievement:
  Operations within ≤7 cycles: 0/10000 (0.0%)
  Status: ❌ FAILED
```

### JSON Export
```json
{
  "suite_name": "Test Suite",
  "total_tests": 1,
  "overall_score": 0.0,
  "results": [
    {
      "test_name": "Test Name",
      "operations": 10000,
      "avg_cycles_per_op": 215.3,
      "avg_time_ns_per_op": 215.3,
      "ops_per_sec": 4644682,
      "target_achievement_percent": 0.0,
      "passed": false
    }
  ]
}
```

### CSV Export
```csv
test_name,operations,avg_cycles_per_op,avg_time_ns_per_op,ops_per_sec,target_achievement_percent,passed
"Test Name",10000,215.3,215.3,4644682,0.0,false
```

## Best Practices

### 1. Test Data Management
- Use consistent test data across related benchmarks
- Clean up resources properly
- Avoid memory leaks

### 2. Iteration Counts
- Use appropriate iteration counts for different test types
- Simple operations: 10,000-100,000 iterations
- Complex operations: 1,000-10,000 iterations
- Batch operations: 100-1,000 iterations

### 3. Performance Analysis
- Compare 7-tick vs 49-tick paths when applicable
- Report speedup ratios
- Validate against performance targets

### 4. Error Handling
- Handle allocation failures gracefully
- Validate input parameters
- Provide meaningful error messages

### 5. Documentation
- Document test purposes and expected results
- Include hardware information in reports
- Explain performance characteristics

## Next Steps

1. **Refactor SPARQL benchmarks** - Start with `sparql_benchmark.c`
2. **Refactor SHACL benchmarks** - Focus on validation performance
3. **Refactor OWL benchmarks** - Measure reasoning performance
4. **Refactor Telemetry benchmarks** - Assess overhead impact
5. **Create unified benchmark runner** - Combine all refactored benchmarks
6. **Add continuous monitoring** - Set up automated performance tracking

## Conclusion

The simplified benchmark framework provides a consistent, cross-platform solution for performance measurement and analysis. By refactoring existing benchmarks to use this framework, we ensure:

- **Consistent measurement** across all benchmarks
- **Comprehensive reporting** with multiple output formats
- **Performance validation** against established targets
- **Easy comparison** between different implementations
- **Cross-platform compatibility** for all architectures

The refactoring process is straightforward and provides immediate benefits in terms of consistency and analysis capabilities. 