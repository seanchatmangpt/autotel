# TTL Lexer Comprehensive Test Suite Documentation

## Overview

This document describes the comprehensive test suite for the TTL (Time-To-Live) Lexer, a high-performance lexical analyzer designed with Six Sigma quality standards and 7-tick performance compliance.

## Test Suite Architecture

### Test Categories

1. **Unit Tests** - Individual token type testing
2. **Edge Case Tests** - Boundary conditions and error handling
3. **Performance Benchmarks** - 7-tick compliance verification
4. **Memory Leak Tests** - Memory safety validation
5. **Integration Tests** - Real-world code parsing
6. **Stress Tests** - High-volume, multi-threaded testing

### Quality Metrics

- **Coverage Target**: >95% code coverage
- **Performance Target**: ≤7 CPU cycles per token (7-tick compliance)
- **Quality Target**: 6σ (3.4 DPMO - Defects Per Million Opportunities)

## Test Files

### 1. `test_ttl_lexer.c` - Main Test Suite

Comprehensive test suite with 20 test cases covering all aspects of the lexer:

#### Unit Tests (Tests 1-5)
- **test_identifier_tokenization**: Validates identifier parsing
- **test_number_tokenization**: Tests integer and floating-point parsing
- **test_keyword_tokenization**: Verifies keyword recognition
- **test_operator_tokenization**: Tests operator parsing
- **test_delimiter_tokenization**: Validates delimiter handling

#### Edge Case Tests (Tests 6-10)
- **test_empty_input**: Empty string handling
- **test_whitespace_only**: Pure whitespace input
- **test_invalid_characters**: Error character detection
- **test_long_identifiers**: 1000+ character identifiers
- **test_number_edge_cases**: Overflow and malformed numbers

#### Performance Tests (Tests 11-12)
- **test_performance_7tick_compliance**: Verifies ≤7 cycles/token
- **test_simd_performance**: SIMD optimization validation

#### Memory Tests (Tests 13-14)
- **test_memory_leaks**: Leak detection across iterations
- **test_memory_stress**: Large file parsing stability

#### Integration Tests (Tests 15-16)
- **test_real_c_code**: Fibonacci function parsing
- **test_ttl_config_parsing**: TTL configuration syntax

#### Stress Tests (Tests 17-20)
- **test_high_frequency_tokens**: 10,000+ token parsing
- **test_line_counting**: Multi-line accuracy
- **test_six_sigma_metrics**: Quality metric validation
- **test_concurrent_safety**: Multi-lexer simulation

### 2. `ttl_lexer_stress_test.c` - Production Stress Testing

Multi-threaded stress testing with 6 scenarios:

1. **Normal Code**: Typical C programming patterns
2. **Keyword Heavy**: Maximum keyword density
3. **Number Heavy**: Numeric literal stress
4. **Whitespace Heavy**: SIMD optimization stress
5. **Mixed Complexity**: Real-world complexity
6. **Error Prone**: Error handling validation

Features:
- Multi-threaded execution (default 8 threads)
- 10,000 iterations per thread
- Performance analysis per scenario
- DPMO calculation and Six Sigma reporting

### 3. Build and Execution Files

#### `Makefile.ttl_lexer`
- Standard build (`make all`)
- Debug build with sanitizers (`make debug`)
- Optimized benchmark build (`make benchmark`)
- Code coverage generation (`make coverage`)
- Memory leak checking (`make memcheck`)

#### `run_ttl_lexer_tests.sh`
Automated test runner that:
1. Builds all test variants
2. Runs standard tests
3. Executes debug tests with sanitizers
4. Performs benchmarks
5. Generates coverage reports
6. Checks for memory leaks
7. Creates performance reports

## Running the Tests

### Quick Test
```bash
cd tests
make -f Makefile.ttl_lexer test
```

### Full Test Suite
```bash
cd tests
./run_ttl_lexer_tests.sh
```

### Stress Testing
```bash
cd tests
gcc -O3 -pthread ttl_lexer_stress_test.c ../lean_sigma_lexer.c -o stress_test
./stress_test [num_threads]
```

## Test Metrics and Reporting

### Console Output Format
- 🧪 Test category headers
- ✓ Passed tests (green)
- ✗ Failed tests (red)
- 📊 Performance metrics
- ⚡ Cycle counts
- 🎯 7-tick compliance status

### Generated Reports
1. **performance_report.txt** - Detailed performance metrics
2. **leaks_report.txt** - Memory leak analysis (macOS)
3. **\*.gcov** - Code coverage files
4. **Console output** - Real-time test results

## Quality Assurance Features

### Six Sigma Tracking
- Opportunities counted for each lexer operation
- Defects tracked for parsing errors
- DPMO calculation (Defects Per Million Opportunities)
- Sigma level reporting (target: 6σ = 3.4 DPMO)

### 7-Tick Compliance
- Per-token cycle measurement
- SIMD optimization verification
- Average cycles/token reporting
- Real-time compliance checking

### Memory Safety
- Address sanitizer integration
- Undefined behavior detection
- Leak detection (platform-specific)
- Stress testing with large inputs

## Test Coverage Matrix

| Component | Unit | Edge | Perf | Memory | Integration | Stress |
|-----------|------|------|------|--------|-------------|--------|
| Identifiers | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| Numbers | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| Keywords | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| Operators | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| Delimiters | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| Whitespace | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| Error handling | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| SIMD paths | - | - | ✓ | ✓ | - | ✓ |
| Line counting | ✓ | ✓ | - | - | ✓ | ✓ |

## Continuous Integration

The test suite is designed for CI/CD integration:

1. **Exit codes**: 0 for success, 1 for any failure
2. **Deterministic**: Consistent results across runs
3. **Platform aware**: Handles macOS/Linux differences
4. **Parallel safe**: No shared state between tests
5. **Report generation**: Automated metric collection

## Future Enhancements

1. **Fuzzing Integration**: AFL or LibFuzzer support
2. **Benchmark Comparison**: Historical performance tracking
3. **Visual Coverage**: HTML coverage reports
4. **Distributed Testing**: Multi-machine stress testing
5. **Property-Based Testing**: QuickCheck-style testing

## Conclusion

This comprehensive test suite ensures the TTL Lexer meets production-quality standards with:
- >95% code coverage
- 7-tick performance compliance
- Six Sigma quality levels
- Zero memory leaks
- Robust error handling
- Scalable performance

The combination of unit, integration, performance, and stress testing provides confidence in the lexer's reliability and efficiency for production use.