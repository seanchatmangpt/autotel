/*  ─────────────────────────────────────────────────────────────
    example_test_usage.c  –  Example: Using CNS Unit Test Framework
    Shows how to write unit tests for custom CNS commands
    ───────────────────────────────────────────────────────────── */

#include "../include/cns.h"
#include "../include/cns_commands.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*═══════════════════════════════════════════════════════════════
  Example 1: Testing a Simple Command Handler
  ═══════════════════════════════════════════════════════════════*/

// Sample command that adds two numbers
CNS_HANDLER(cmd_add) {
    if (cmd->argc != 2) {
        printf("Usage: add <num1> <num2>\n");
        return CNS_ERR_INVALID_ARG;
    }
    
    int a = atoi(cmd->args[0]);
    int b = atoi(cmd->args[1]);
    printf("%d\n", a + b);
    
    return CNS_OK;
}

// To test this command, you would:
// 1. Build with test framework: make test
// 2. Run: ./cns test
// 3. Or run specific test: ./cns test add_command

/*═══════════════════════════════════════════════════════════════
  Example 2: Testing Command with State
  ═══════════════════════════════════════════════════════════════*/

// Counter state
static int g_counter = 0;

CNS_HANDLER(cmd_counter) {
    if (cmd->argc == 0) {
        printf("Counter: %d\n", g_counter);
    } else if (strcmp(cmd->args[0], "inc") == 0) {
        g_counter++;
    } else if (strcmp(cmd->args[0], "dec") == 0) {
        g_counter--;
    } else if (strcmp(cmd->args[0], "reset") == 0) {
        g_counter = 0;
    } else {
        return CNS_ERR_INVALID_ARG;
    }
    
    return CNS_OK;
}

/*═══════════════════════════════════════════════════════════════
  Example 3: Performance-Critical Command
  ═══════════════════════════════════════════════════════════════*/

CNS_HANDLER(cmd_fast_hash) {
    if (cmd->argc != 1) {
        return CNS_ERR_INVALID_ARG;
    }
    
    // Compute hash using 7T optimized function
    uint32_t hash = s7t_hash_string(cmd->args[0], strlen(cmd->args[0]));
    printf("Hash: 0x%08x\n", hash);
    
    return CNS_OK;
}

/*═══════════════════════════════════════════════════════════════
  Example 4: Command with Complex Logic
  ═══════════════════════════════════════════════════════════════*/

CNS_HANDLER(cmd_process_data) {
    if (cmd->argc < 2) {
        printf("Usage: process <operation> <data...>\n");
        return CNS_ERR_INVALID_ARG;
    }
    
    const char* op = cmd->args[0];
    
    if (strcmp(op, "sum") == 0) {
        int sum = 0;
        for (int i = 1; i < cmd->argc; i++) {
            sum += atoi(cmd->args[i]);
        }
        printf("Sum: %d\n", sum);
    } else if (strcmp(op, "avg") == 0) {
        int sum = 0;
        int count = cmd->argc - 1;
        for (int i = 1; i < cmd->argc; i++) {
            sum += atoi(cmd->args[i]);
        }
        printf("Average: %.2f\n", (float)sum / count);
    } else if (strcmp(op, "max") == 0) {
        int max = atoi(cmd->args[1]);
        for (int i = 2; i < cmd->argc; i++) {
            int val = atoi(cmd->args[i]);
            if (val > max) max = val;
        }
        printf("Max: %d\n", max);
    } else {
        return CNS_ERR_INVALID_ARG;
    }
    
    return CNS_OK;
}

/*═══════════════════════════════════════════════════════════════
  How to Write Tests for These Commands
  ═══════════════════════════════════════════════════════════════*/

/*
To test these commands using the CNS test framework:

1. Create a test file (e.g., test_my_commands.c) with:

#include "../src/cmd_test.c"  // Access test framework

// Test the add command
static void test_my_add_command(test_suite_t* suite, void* context) {
    cns_engine_t* engine = (cns_engine_t*)context;
    cns_command_t cmd;
    cns_result_t result;
    
    // Register the command
    CNS_REGISTER_CMD(engine, "add", cmd_add, 2, 2, "Add two numbers");
    
    // Test valid addition
    result = cns_parse("add 5 3", &cmd);
    TEST_ASSERT_EQ(suite, CNS_OK, result);
    
    result = cns_execute(engine, &cmd);
    TEST_ASSERT_EQ(suite, CNS_OK, result);
    
    // Test invalid arguments
    result = cns_parse("add 5", &cmd);
    TEST_ASSERT_EQ(suite, CNS_OK, result);
    
    result = cns_execute(engine, &cmd);
    TEST_ASSERT_EQ(suite, CNS_ERR_INVALID_ARG, result);
    
    add_test_result(suite, TEST_PASS, "Add command test passed", __FILE__, __LINE__);
}

// Test performance
static void test_hash_performance(test_suite_t* suite, void* context) {
    cns_engine_t* engine = (cns_engine_t*)context;
    cns_command_t cmd;
    
    CNS_REGISTER_CMD(engine, "fasthash", cmd_fast_hash, 1, 1, "Fast hash");
    
    cns_parse("fasthash teststring", &cmd);
    
    // Test that 100 hash operations complete in under 10k cycles
    TEST_ASSERT_PERFORMANCE(suite, {
        for (int i = 0; i < 100; i++) {
            cns_execute(engine, &cmd);
        }
    }, 10000);
    
    add_test_result(suite, TEST_PASS, "Hash performance test passed", __FILE__, __LINE__);
}

2. Add your tests to the test suite in cmd_test.c

3. Run tests:
   ./cns test                    # Run all tests
   ./cns test -v                 # Verbose output
   ./cns test my_add_command     # Run specific test
   ./cns test -l                 # List available tests

4. Check results:
   - Tests show PASS/FAIL status
   - Performance metrics in cycles
   - Memory usage tracking
   - Detailed error messages on failure
*/

/*═══════════════════════════════════════════════════════════════
  Integration with CNS
  ═══════════════════════════════════════════════════════════════*/

// To register these commands in your CNS application:
void register_my_commands(cns_engine_t* engine) {
    CNS_REGISTER_CMD(engine, "add", cmd_add, 2, 2, 
        "Add two numbers");
    
    CNS_REGISTER_CMD(engine, "counter", cmd_counter, 0, 1, 
        "Counter operations (inc/dec/reset)");
    
    CNS_REGISTER_CMD(engine, "fasthash", cmd_fast_hash, 1, 1, 
        "Compute fast hash of string");
    
    CNS_REGISTER_CMD(engine, "process", cmd_process_data, 2, CNS_MAX_ARGS, 
        "Process data (sum/avg/max)");
}

/*═══════════════════════════════════════════════════════════════
  Testing Best Practices
  ═══════════════════════════════════════════════════════════════*/

/*
1. Test Edge Cases:
   - Empty arguments
   - Too many arguments
   - Invalid input formats
   - Boundary values

2. Test Performance:
   - Use TEST_ASSERT_PERFORMANCE for time-critical code
   - Measure cycles, not wall time
   - Set realistic thresholds

3. Test Error Handling:
   - Verify all error codes
   - Test recovery from errors
   - Check error messages

4. Test State:
   - Initialize state before tests
   - Verify state changes
   - Clean up after tests

5. Use Descriptive Names:
   - test_<command>_<scenario>
   - Clear test descriptions
   - Meaningful assertion messages

6. Keep Tests Fast:
   - Target < 1000 cycles per test
   - Avoid I/O in unit tests
   - Mock external dependencies
*/