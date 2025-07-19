# CNS Unit Testing Framework

The CNS unit testing framework is adapted from the 7T unit testing framework, providing high-performance testing capabilities specifically designed for CNS commands.

## Overview

The CNS test framework provides:
- Fast, cycle-accurate performance testing
- Comprehensive assertion macros
- Test suite management
- Performance benchmarking
- Memory usage tracking
- Command-specific test patterns

## Quick Start

### Running Tests

```bash
# Run all tests
./cns test

# Run tests verbosely
./cns test -v

# Run specific test
./cns test parser_basic

# List available tests
./cns test -l

# Run test suite
./cns test core
```

### Writing Tests

1. Create a test function:
```c
static void test_my_command(test_suite_t* suite, void* context) {
    cns_engine_t* engine = (cns_engine_t*)context;
    cns_command_t cmd;
    
    // Register your command
    CNS_REGISTER_CMD(engine, "mycommand", cmd_handler, 0, 2, "My command");
    
    // Test parsing
    cns_result_t result = cns_parse("mycommand arg1 arg2", &cmd);
    TEST_ASSERT_EQ(suite, CNS_OK, result);
    
    // Test execution
    result = cns_execute(engine, &cmd);
    TEST_ASSERT_EQ(suite, CNS_OK, result);
    
    // Record success
    add_test_result(suite, TEST_PASS, "My command test passed", __FILE__, __LINE__);
}
```

2. Add to test suite in `cmd_test.c`:
```c
static test_case_t cns_test_cases[] = {
    // ... existing tests ...
    {"my_command", test_my_command, "Test my command", true},
};
```

## Assertion Macros

### Basic Assertions

```c
// Test condition
TEST_ASSERT(suite, condition, "Error message");

// Test equality
TEST_ASSERT_EQ(suite, expected, actual);

// Test string equality
TEST_ASSERT_STR_EQ(suite, "expected", actual_string);
```

### Performance Assertions

```c
// Test performance (in CPU cycles)
TEST_ASSERT_PERFORMANCE(suite, {
    // Code to benchmark
    for (int i = 0; i < 1000; i++) {
        cns_execute(engine, &cmd);
    }
}, 100000);  // Max 100k cycles
```

## Test Categories

Tests are organized into categories:

1. **Core Tests** - Parser, engine, basic commands
2. **Performance Tests** - Benchmarking and optimization
3. **Integration Tests** - Multi-command workflows
4. **Stress Tests** - High-load scenarios

## Example: Complete Test

```c
// Command that processes numbers
CNS_HANDLER(cmd_stats) {
    if (cmd->argc < 1) {
        return CNS_ERR_INVALID_ARG;
    }
    
    int sum = 0, count = cmd->argc;
    for (int i = 0; i < count; i++) {
        sum += atoi(cmd->args[i]);
    }
    
    printf("Sum: %d, Avg: %.2f\n", sum, (float)sum / count);
    return CNS_OK;
}

// Test the stats command
static void test_stats_command(test_suite_t* suite, void* context) {
    cns_engine_t* engine = (cns_engine_t*)context;
    cns_command_t cmd;
    cns_result_t result;
    
    // Register command
    CNS_REGISTER_CMD(engine, "stats", cmd_stats, 1, CNS_MAX_ARGS, "Calculate statistics");
    
    // Test with valid input
    result = cns_parse("stats 10 20 30", &cmd);
    TEST_ASSERT_EQ(suite, CNS_OK, result);
    TEST_ASSERT_EQ(suite, 3, cmd.argc);
    
    result = cns_execute(engine, &cmd);
    TEST_ASSERT_EQ(suite, CNS_OK, result);
    
    // Test with no arguments
    result = cns_parse("stats", &cmd);
    TEST_ASSERT_EQ(suite, CNS_OK, result);
    
    result = cns_execute(engine, &cmd);
    TEST_ASSERT_EQ(suite, CNS_ERR_INVALID_ARG, result);
    
    // Test performance
    cns_parse("stats 1 2 3 4 5", &cmd);
    TEST_ASSERT_PERFORMANCE(suite, {
        for (int i = 0; i < 100; i++) {
            cns_execute(engine, &cmd);
        }
    }, 10000);  // Should complete in under 10k cycles
    
    add_test_result(suite, TEST_PASS, "Stats command test passed", __FILE__, __LINE__);
}
```

## Performance Guidelines

### Cycle Budgets

- Simple commands: < 100 cycles
- Parser operations: < 200 cycles
- Hash lookups: < 50 cycles
- Complex commands: < 1000 cycles

### Memory Budgets

- Command structure: 64 bytes (cache-line aligned)
- Engine overhead: < 1KB
- Test framework: < 10KB

## Best Practices

1. **Test Isolation**: Each test should be independent
2. **Fast Tests**: Target < 1000 cycles per test
3. **Clear Names**: Use descriptive test and function names
4. **Edge Cases**: Test boundaries and error conditions
5. **Performance**: Always include performance assertions
6. **Documentation**: Comment complex test logic

## Integration with CI/CD

```bash
# Run tests in CI pipeline
make test

# Run with coverage (if configured)
make test-coverage

# Run specific test suite
./cns test core
./cns test perf
```

## Debugging Failed Tests

1. Run with verbose flag: `./cns test -v`
2. Check failure location in output
3. Use printf debugging in test functions
4. Verify command registration
5. Check assertion messages

## Advanced Testing

### Mocking External Dependencies

```c
// Mock file operations
static int mock_file_exists = 1;

CNS_HANDLER(cmd_file_check) {
    // Use mock instead of real file check
    if (mock_file_exists) {
        printf("File exists\n");
        return CNS_OK;
    }
    return CNS_ERR_NOT_FOUND;
}
```

### Testing State Management

```c
static void test_stateful_command(test_suite_t* suite, void* context) {
    // Reset state
    global_state = INITIAL_STATE;
    
    // Test state transitions
    // ...
    
    // Verify final state
    TEST_ASSERT_EQ(suite, EXPECTED_STATE, global_state);
}
```

### Stress Testing

```c
static void test_command_stress(test_suite_t* suite, void* context) {
    cns_engine_t* engine = (cns_engine_t*)context;
    
    // Register many commands
    for (int i = 0; i < 100; i++) {
        char name[16];
        snprintf(name, sizeof(name), "cmd%d", i);
        CNS_REGISTER_CMD(engine, name, dummy_handler, 0, 0, "Stress test");
    }
    
    // Test lookup performance under load
    TEST_ASSERT_PERFORMANCE(suite, {
        for (int i = 0; i < 1000; i++) {
            uint32_t hash = s7t_hash_string("cmd50", 5);
            cns_lookup(engine, hash);
        }
    }, 50000);  // Should still be fast
}
```

## Contributing Tests

1. Follow the existing test patterns
2. Include performance assertions
3. Document test purpose
4. Submit with implementation PR
5. Ensure all tests pass

The CNS unit testing framework ensures high-quality, performant command implementations that meet the 7-tick performance guarantee.