# CNS Unit Tests

Comprehensive unit test suite for all CNS (Command Nano Stack) include files, designed to validate functionality, performance, and 7-tick compliance.

## Overview

This test suite provides comprehensive coverage for all CNS include files:

- **Core Functionality** (`cns.h`) - Engine initialization, command registration, lookup, execution
- **Parser** (`cns_parser.h`) - Command parsing, tokenization, validation
- **Dispatch** (`cns_dispatch.h`) - Command routing, batch execution, metrics
- **Commands** (`cns_commands.h`) - Built-in command handlers, registration macros
- **Benchmark** (`cns_benchmark.h`) - Performance testing, tier classification
- **Types** (`cns/types.h`) - Type definitions, structures, enums
- **CLI** (`cns/cli.h`) - Command-line interface, domain management

## Test Philosophy

### 80/20 Coverage Focus
- **Happy Path Testing**: Primary focus on successful execution paths
- **Critical Path Validation**: Essential error conditions and edge cases
- **Performance Validation**: 7-tick compliance and telemetry validation
- **Integration Testing**: End-to-end workflow validation

### 7-Tick Compliance
- All tests validate cycle count compliance
- Performance assertions ensure sub-7-tick execution
- Telemetry spans are validated for accuracy
- Memory allocation is minimized for predictable performance

## Test Structure

### Test Infrastructure
Each test file includes:
- **Test Context**: Cycle counting and performance tracking
- **Mock Functions**: Simulated handlers and dependencies
- **Assertion Macros**: Standardized test validation
- **Performance Validation**: Cycle budget enforcement

### Test Categories
1. **Unit Tests**: Individual function validation
2. **Integration Tests**: Multi-component workflows
3. **Performance Tests**: Cycle count and timing validation
4. **Error Tests**: Error condition handling
5. **Boundary Tests**: Edge case validation

## Building and Running

### Prerequisites
- GCC compiler (C99 support)
- Make build system
- Standard C library

### Quick Start
```bash
# Run all tests
./run_tests.sh

# Run specific test
./run_tests.sh test_cns_core

# Build only
./run_tests.sh --build

# Generate report
./run_tests.sh --report
```

### Using Make
```bash
# Build all tests
make all

# Run all tests
make test

# Run specific test
make test-core
make test-parser
make test-dispatch
make test-commands
make test-benchmark
make test-types
make test-cli

# Clean build artifacts
make clean

# Show help
make help
```

## Test Files

### Core Tests (`test_cns_core.c`)
- Engine initialization and configuration
- Command registration and lookup
- Command execution and validation
- Performance tracking and metrics
- Resource limit validation

### Parser Tests (`test_cns_parser.c`)
- Character classification and tokenization
- Command parsing and argument extraction
- Quote handling and escape sequences
- Batch parsing and error handling
- Performance optimization validation

### Dispatch Tests (`test_cns_dispatch.c`)
- Dispatch table initialization
- Command lookup and routing
- Batch execution and error handling
- Performance metrics collection
- Help system and command listing

### Commands Tests (`test_cns_commands.c`)
- Built-in command handlers
- Command registration macros
- Help and status commands
- Debug and configuration commands
- Command completion system

### Benchmark Tests (`test_cns_benchmark.c`)
- Performance target validation
- Benchmark configuration
- Result calculation and validation
- Tier classification (L1/L2/L3)
- Statistical analysis

### Types Tests (`test_cns_types.c`)
- Type definition validation
- Structure initialization
- Enum value verification
- Option and argument handling
- Context and result code validation

### CLI Tests (`test_cns_cli.c`)
- CLI initialization and cleanup
- Domain registration and management
- Argument parsing and validation
- Help system and error handling
- Performance tracking integration

## Test Results

### Success Criteria
- **100% Pass Rate**: All tests must pass
- **7-Tick Compliance**: All operations within cycle budget
- **Memory Safety**: No memory leaks or corruption
- **Error Handling**: Proper error condition management

### Performance Targets
- **L1 Tier**: < 10ns (7-tick target)
- **L2 Tier**: < 100ns (sub-100ns)
- **L3 Tier**: < 1μs (sub-microsecond)

### Coverage Metrics
- **Function Coverage**: All public API functions tested
- **Path Coverage**: Critical execution paths validated
- **Error Coverage**: Essential error conditions tested
- **Performance Coverage**: Cycle count validation

## Continuous Integration

### Automated Testing
```bash
# CI pipeline integration
./run_tests.sh --report --verbose

# Exit code indicates success/failure
# 0 = all tests passed
# 1 = one or more tests failed
```

### Report Generation
- Timestamped test reports
- Performance metrics collection
- Coverage analysis
- Failure analysis and debugging

## Debugging

### Common Issues
1. **Compilation Errors**: Check include paths and dependencies
2. **Linker Errors**: Verify S7T library availability
3. **Runtime Errors**: Check mock function implementations
4. **Performance Failures**: Validate cycle counting accuracy

### Debug Mode
```bash
# Verbose output
./run_tests.sh --verbose

# Individual test debugging
make test-core && ./test_cns_core
```

## Contributing

### Adding New Tests
1. Follow existing test structure and naming conventions
2. Include comprehensive happy path testing
3. Validate performance and cycle counts
4. Add appropriate error condition testing
5. Update Makefile and test runner

### Test Guidelines
- **Descriptive Names**: Clear test function names
- **Isolated Tests**: Independent test execution
- **Mock Dependencies**: Minimal external dependencies
- **Performance Validation**: Cycle count assertions
- **Error Handling**: Proper error condition testing

## Performance Validation

### Cycle Counting
- All tests include cycle count validation
- Performance assertions enforce 7-tick compliance
- Telemetry spans are validated for accuracy
- Memory allocation is tracked and minimized

### Benchmark Integration
- Performance tier classification
- Statistical analysis and reporting
- Target achievement validation
- Comparative performance analysis

## Future Enhancements

### Planned Features
- **Property-Based Testing**: Automated test case generation
- **Fuzzing**: Input validation and edge case discovery
- **Memory Profiling**: Detailed memory usage analysis
- **Performance Regression**: Automated performance monitoring
- **Coverage Analysis**: Code coverage reporting

### Integration Goals
- **CI/CD Pipeline**: Automated testing integration
- **Performance Monitoring**: Continuous performance tracking
- **Regression Detection**: Automated regression testing
- **Documentation**: Automated test documentation generation

## Support

For issues or questions regarding the test suite:
1. Check the test output for specific error messages
2. Review the test documentation and examples
3. Validate the test environment and dependencies
4. Consult the CNS documentation for API details

---

*This test suite ensures CNS reliability, performance, and 7-tick compliance through comprehensive validation of all include files.* 