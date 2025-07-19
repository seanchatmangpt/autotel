# 7T Engine Unit Tests

Comprehensive unit testing framework and test suite for the 7T Engine core components.

## Overview

This directory contains a complete unit testing framework designed specifically for the 7T Engine, providing:

- **Comprehensive Test Coverage**: Tests for all core components (Runtime, TPOT, Telemetry)
- **Performance Validation**: Built-in performance testing with cycle counting and latency measurement
- **Memory Testing**: Memory usage tracking and leak detection
- **Stress Testing**: High-load testing for robustness validation
- **Cross-Platform Compatibility**: Works on x86_64 and ARM64 architectures
- **Multiple Output Formats**: JSON, XML, and text report formats

## Test Framework Features

### Core Testing Capabilities
- **High-precision timing** with nanosecond resolution
- **Cycle counting** using `__builtin_readcyclecounter()`
- **Memory usage tracking** with configurable limits
- **Performance assertions** for latency and throughput validation
- **Comprehensive assertion macros** for various data types
- **Test fixtures** for setup/teardown management
- **Mock function support** for isolated testing

### Test Categories
- **Unit Tests**: Individual component functionality
- **Integration Tests**: Component interaction testing
- **Performance Tests**: Latency and throughput validation
- **Memory Tests**: Memory usage and leak detection
- **Stress Tests**: High-load robustness testing
- **Regression Tests**: Automated regression detection

## Test Components

### 1. Runtime Tests (`test_seven_t_runtime.c`)
Tests for the core 7T runtime engine:

- **Engine Creation/Destruction**: Memory management and initialization
- **String Interning**: String deduplication and ID generation
- **Triple Operations**: Adding, querying, and managing RDF triples
- **Pattern Matching**: SPARQL-like pattern matching performance
- **Materialization**: Query result materialization
- **Bit Vector Operations**: Efficient set operations
- **Stress Testing**: Large dataset handling

### 2. TPOT Tests (`test_7t_tpot.c`)
Tests for the Tree-based Pipeline Optimization Tool:

- **Dataset Management**: Iris, Boston, Breast Cancer, Diabetes, Digits datasets
- **Pipeline Creation**: ML pipeline construction and evaluation
- **Optimization Engine**: Genetic algorithm optimization
- **Feature Processing**: Normalization, standardization, selection
- **Performance Validation**: Optimization speed and memory usage
- **Algorithm Registration**: ML algorithm management

### 3. Telemetry Tests (`test_telemetry7t.c`)
Tests for the telemetry and observability system:

- **Context Management**: Telemetry context initialization and cleanup
- **Span Operations**: Distributed tracing span creation and management
- **Attribute Management**: String, integer, and double attributes
- **Event Tracking**: Event creation and correlation
- **Metric Collection**: Counter, gauge, and histogram metrics
- **ID Generation**: Trace and span ID generation
- **Performance Monitoring**: Timing accuracy and overhead measurement

## Building and Running Tests

### Prerequisites
- GCC or Clang compiler
- Make build system
- Standard C library
- Math library (`-lm`)

### Quick Start
```bash
# Build all tests
make all

# Run all tests
make test

# Run specific test suites
make test-runtime
make test-tpot
make test-telemetry
```

### Advanced Usage
```bash
# Run with verbose output
make test-all

# Run performance tests
make perf-test

# Run memory tests with Valgrind
make mem-test

# Run stress tests
make stress-test

# Generate coverage report
make coverage
```

### Command Line Options
```bash
# Run specific test suites
./test_all --runtime-only
./test_all --tpot-only
./test_all --telemetry-only

# Enable additional test categories
./test_all --performance
./test_all --memory
./test_all --stress

# Output control
./test_all --verbose
./test_all --stop-on-failure
./test_all --output results.json --format json
```

## Test Framework API

### Assertion Macros
```c
// Basic assertions
ASSERT_TRUE(condition);
ASSERT_FALSE(condition);
ASSERT_EQUAL(expected, actual);
ASSERT_NOT_EQUAL(expected, actual);
ASSERT_STRING_EQUAL(expected, actual);
ASSERT_NULL(ptr);
ASSERT_NOT_NULL(ptr);

// Range assertions
ASSERT_GREATER_THAN(value, threshold);
ASSERT_LESS_THAN(value, threshold);
ASSERT_IN_RANGE(value, min, max);

// Performance assertions
ASSERT_PERFORMANCE(operation, max_cycles);
ASSERT_LATENCY(operation, max_ns);
ASSERT_MEMORY_USAGE(operation, max_bytes);
```

### Test Case Definition
```c
// Simple test case
TEST_CASE(test_name, "Test description")
static void test_test_name(TestContext* context) {
    // Test implementation
    ASSERT_TRUE(condition);
}

// Test case with setup/teardown
TEST_CASE_WITH_SETUP(test_name, setup_func, teardown_func, "Test description")
static void test_test_name(TestContext* context) {
    // Test implementation
    ASSERT_TRUE(condition);
}
```

### Test Suite Registration
```c
// Define test suite
TEST_SUITE_BEGIN(suite_name)

// Register test cases
void register_suite_name_tests(void) {
    TestCase test_cases[] = {
        test_case_test1,
        test_case_test2,
        // ...
    };
    
    suite_name_registration.test_cases = test_cases;
    suite_name_registration.test_case_count = sizeof(test_cases) / sizeof(TestCase);
    suite_name_registration.suite_setup = setup_function;
    suite_name_registration.suite_teardown = teardown_function;
}

TEST_SUITE_END(suite_name)
```

## Performance Targets

The test framework validates against the 7T Engine performance targets:

- **≤7 CPU cycles** per operation (95% of operations)
- **≤10 nanoseconds** latency per operation
- **≥100M operations/second** throughput
- **≤32 bytes** per event memory efficiency

## Test Results and Reporting

### Console Output
```
=== 7T Engine Unit Tests ===
Framework Version: 1.0.0

=== Running Runtime Tests ===
Running test: engine_creation
Test engine_creation: PASSED (0.123 ms)
...

=== Test Suite Summary: seven_t_runtime ===
Total tests: 25
Passed: 25
Failed: 0
Skipped: 0
Errors: 0
Success rate: 100.0%
Status: ✅ PASSED
```

### Report Formats

#### JSON Format
```json
{
  "test_report": {
    "total_tests": 75,
    "passed_tests": 75,
    "failed_tests": 0,
    "skipped_tests": 0,
    "error_tests": 0,
    "success_rate": 100.0,
    "total_time_ns": 1234567,
    "total_memory_bytes": 1048576
  }
}
```

#### JUnit XML Format
```xml
<?xml version="1.0" encoding="UTF-8"?>
<testsuites>
  <testsuite name="7T Engine Tests" tests="75" failures="0" errors="0" skipped="0">
    <!-- Individual test results -->
  </testsuite>
</testsuites>
```

## Continuous Integration

### GitHub Actions Example
```yaml
name: Unit Tests
on: [push, pull_request]
jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - name: Build and Test
        run: |
          cd tests
          make all
          make test
      - name: Generate Coverage
        run: |
          cd tests
          make coverage
      - name: Upload Coverage
        uses: codecov/codecov-action@v1
```

### Local Development
```bash
# Run tests before committing
make test

# Check for memory leaks
make mem-test

# Validate performance
make perf-test

# Generate coverage report
make coverage
```

## Troubleshooting

### Common Issues

#### Compilation Errors
```bash
# Check compiler version
gcc --version

# Ensure all dependencies are installed
make clean && make all
```

#### Test Failures
```bash
# Run with verbose output
./test_all --verbose

# Run specific failing test
./test_runtime --test-name failing_test

# Check for memory issues
valgrind --tool=memcheck ./test_all
```

#### Performance Issues
```bash
# Check CPU frequency scaling
cat /proc/cpuinfo | grep MHz

# Run performance tests in isolation
./test_all --performance --runtime-only
```

### Debug Mode
```bash
# Build with debug symbols
make CFLAGS="-g -O0" all

# Run with GDB
gdb ./test_all
```

## Contributing

### Adding New Tests
1. Create test file in `tests/` directory
2. Follow naming convention: `test_component_name.c`
3. Include test framework header
4. Define test cases using `TEST_CASE` macro
5. Register tests in main function
6. Update Makefile if needed

### Test Guidelines
- **Test one thing at a time**: Each test should validate a single behavior
- **Use descriptive names**: Test names should clearly indicate what is being tested
- **Include performance tests**: Validate performance targets for critical operations
- **Test edge cases**: Include tests for NULL pointers, empty data, etc.
- **Validate memory usage**: Ensure no memory leaks in long-running operations
- **Document complex tests**: Add comments for non-obvious test logic

### Code Coverage
```bash
# Generate coverage report
make coverage

# View coverage in browser
open coverage_report/index.html

# Check coverage percentage
lcov --summary coverage.info
```

## License

This testing framework is part of the 7T Engine project and follows the same license terms.

## Support

For issues with the testing framework:
1. Check the troubleshooting section
2. Review existing test examples
3. Create an issue with detailed error information
4. Include system information and compiler version 