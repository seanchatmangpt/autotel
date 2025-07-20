# TTL Parser Test Suite

Comprehensive test suite for the TTL (Turtle) parser implementation.

## Test Structure

```
tests/
├── test_main.c           # Test runner and main entry point
├── test_utils.h          # Test utilities and macros
├── test_lexer_new.c      # Lexer unit tests
├── test_parser.c         # Parser unit tests
├── test_integration.c    # Integration tests
├── test_error_handling.c # Error handling tests
├── Makefile             # Build and test automation
├── README.md            # This file
└── fixtures/            # Test data files
    ├── simple.ttl       # Basic TTL examples
    ├── complex.ttl      # Complex TTL with all features
    ├── invalid.ttl      # Invalid TTL for error testing
    ├── edge_cases.ttl   # Edge cases and special scenarios
    ├── large.ttl        # Large file for performance testing
    └── unicode.ttl      # Unicode content testing
```

## Test Categories

### 🔤 Lexer Tests (`test_lexer_new.c`)
- Token recognition and classification
- String parsing with escapes
- Number parsing (integers and decimals)
- URI parsing and validation
- Blank node handling
- Comment and whitespace processing
- Error detection and reporting
- Position tracking (line/column)
- Unicode support
- Performance with large inputs

### 🌳 Parser Tests (`test_parser.c`)
- Prefix directive parsing
- Base URI handling
- Triple parsing (subject-predicate-object)
- Syntax shortcuts (`;` `,` `a`)
- Complex nested structures
- Data type recognition
- Error recovery mechanisms
- Large file handling
- Memory management

### 🔗 Integration Tests (`test_integration.c`)
- End-to-end parsing of real TTL files
- Round-trip parsing and serialization
- Multi-file processing
- Streaming parser capabilities
- Concurrent parsing scenarios
- Error location reporting
- Memory efficiency testing

### ⚠️ Error Handling Tests (`test_error_handling.c`)
- Syntax error detection
- Recovery mechanisms
- Error message quality
- Line/column tracking
- Partial parsing scenarios

## Running Tests

### Quick Start
```bash
# Build and run all tests
make test

# Run specific test suites
make test-lexer
make test-parser
make test-integration
make test-error
```

### Advanced Testing
```bash
# Memory leak detection
make test-memory
make test-valgrind

# Performance analysis
make test-perf

# Code coverage
make test-coverage

# Static analysis
make analyze
```

### Development Workflow
```bash
# Watch for changes and auto-run tests
make test-watch

# Debug build with symbols
make debug

# Generate test report
make test-report
```

## Test Options

The test runner supports various command-line options:

```bash
./test_runner [options]

Options:
  -h, --help           Show help
  -v, --verbose        Verbose output
  -m, --memory         Enable memory leak detection
  -q, --quick          Quick tests only (skip slow tests)
  -s, --suite NAME     Run specific test suite
  -f, --filter PATTERN Filter tests by pattern

Test suites:
  lexer       - Lexer unit tests
  parser      - Parser unit tests
  integration - Integration tests
  error       - Error handling tests
  all         - Run all tests (default)
```

## Test Fixtures

### `simple.ttl`
Basic TTL file with common patterns:
- Prefix declarations
- Simple triples
- Person and relationship data

### `complex.ttl`
Advanced TTL features:
- Base URI declarations
- Multiple data types
- Nested structures
- Collections and lists
- Language tags
- Typed literals

### `invalid.ttl`
Invalid TTL for error testing:
- Syntax errors
- Missing components
- Malformed URIs
- Unclosed strings

### `edge_cases.ttl`
Special scenarios:
- Empty strings
- Unicode characters
- Very long strings
- Numeric edge cases
- Whitespace variations
- Comments in unusual places

### `unicode.ttl`
International character support:
- Multiple scripts (Latin, Cyrillic, CJK, Arabic)
- Emoji and symbols
- Mathematical notation
- RTL text handling

### `large.ttl`
Performance testing:
- Many triples for throughput testing
- Memory usage validation
- Scalability assessment

## Test Utilities

### Assertion Macros
```c
TEST_ASSERT(condition)              // Basic assertion
TEST_ASSERT_EQ(actual, expected)    // Equality check
TEST_ASSERT_STR_EQ(str1, str2)      // String comparison
TEST_ASSERT_NULL(ptr)               // Null pointer check
TEST_ASSERT_NOT_NULL(ptr)           // Non-null check
```

### Test Organization
```c
RUN_TEST(test_function)             // Run single test
RUN_TEST_SUITE(name, function)      // Run test suite
SKIP_TEST(test_function, reason)    // Skip test with reason
```

### Memory Management
```c
TEST_MALLOC(size)                   // Tracked allocation
TEST_FREE(ptr)                      // Tracked deallocation
check_memory_leaks()                // Verify no leaks
```

### Performance Measurement
```c
MEASURE_TIME(code, time_var)        // Time code execution
measure_time(start, end)            // Calculate elapsed time
```

## Expected Test Results

### Coverage Targets
- **Lexer**: 95%+ line coverage
- **Parser**: 90%+ line coverage
- **Error handling**: 85%+ branch coverage

### Performance Benchmarks
- **Lexer**: >50,000 tokens/second
- **Parser**: >10,000 triples/second
- **Memory**: <1MB for 10,000 triples

### Quality Metrics
- Zero memory leaks
- No undefined behavior (AddressSanitizer clean)
- All edge cases handled gracefully
- Clear error messages with location info

## Adding New Tests

### 1. Create Test Function
```c
int test_new_feature() {
    // Setup
    Lexer lexer;
    init_lexer(&lexer, "test input");
    
    // Test
    Token token;
    int result = next_token(&lexer, &token);
    
    // Assertions
    TEST_ASSERT(result == 1);
    TEST_ASSERT_EQ(token.type, EXPECTED_TYPE);
    
    // Cleanup
    free_token(&token);
    free_lexer(&lexer);
    
    return 1;
}
```

### 2. Add to Test Suite
```c
void run_lexer_tests() {
    RUN_TEST_SUITE("Lexer", () => {
        // ... existing tests ...
        RUN_TEST(test_new_feature);
    });
}
```

### 3. Update Makefile Dependencies
Add new source files to the Makefile if needed.

## Continuous Integration

The test suite is designed for CI environments:

```yaml
# Example GitHub Actions
- name: Run Tests
  run: |
    cd tests
    make test
    make test-memory
    make test-coverage
```

## Troubleshooting

### Common Issues

**Segmentation Fault**
```bash
# Run with debugging
make debug
gdb ./test_runner
(gdb) run
(gdb) bt
```

**Memory Leaks**
```bash
# Use Valgrind
make test-valgrind

# Or AddressSanitizer
make test-asan
```

**Slow Tests**
```bash
# Run only quick tests
make test-quick

# Profile performance
make test-perf
```

**Build Errors**
```bash
# Clean and rebuild
make clean
make all
```

### Debug Output

Enable verbose mode for detailed information:
```bash
./test_runner --verbose --memory
```

This provides:
- Test execution details
- Memory allocation tracking
- Error context information
- Performance metrics

## Contributing

When adding new features:

1. Write tests first (TDD approach)
2. Ensure all existing tests pass
3. Add appropriate test fixtures
4. Update documentation
5. Verify memory safety
6. Check performance impact

The test suite serves as both validation and documentation of expected behavior.