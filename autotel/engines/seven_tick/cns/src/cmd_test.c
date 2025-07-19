/*  ─────────────────────────────────────────────────────────────
    cmd_test.c  –  CNS Unit Testing Command (v1.0)
    Unit test framework integration for CNS based on 7T framework
    ───────────────────────────────────────────────────────────── */

#include "../include/cns.h"
#include "../include/cns_commands.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <dlfcn.h>

/*═══════════════════════════════════════════════════════════════
  Test Framework Types (Adapted from 7T)
  ═══════════════════════════════════════════════════════════════*/

typedef enum {
    TEST_PASS,
    TEST_FAIL,
    TEST_SKIP,
    TEST_ERROR
} test_status_t;

typedef struct {
    const char* name;
    const char* suite;
    test_status_t status;
    const char* error_msg;
    uint64_t cycles;
    size_t memory_bytes;
    int line;
    const char* file;
} test_result_t;

typedef struct {
    const char* name;
    test_result_t* results;
    size_t count;
    size_t capacity;
    uint64_t total_cycles;
    int passed;
    int failed;
    int skipped;
    int errors;
} test_suite_t;

typedef void (*test_func_t)(test_suite_t* suite, void* context);

typedef struct {
    const char* name;
    test_func_t func;
    const char* description;
    bool enabled;
} test_case_t;

/*═══════════════════════════════════════════════════════════════
  Test Macros (Simplified from 7T for CNS)
  ═══════════════════════════════════════════════════════════════*/

#define TEST_ASSERT(suite, condition, msg) do { \
    if (!(condition)) { \
        add_test_result(suite, TEST_FAIL, msg, __FILE__, __LINE__); \
        return; \
    } \
} while(0)

#define TEST_ASSERT_EQ(suite, expected, actual) do { \
    if ((expected) != (actual)) { \
        char msg[256]; \
        snprintf(msg, sizeof(msg), "Expected %lld, got %lld", \
                 (long long)(expected), (long long)(actual)); \
        add_test_result(suite, TEST_FAIL, msg, __FILE__, __LINE__); \
        return; \
    } \
} while(0)

#define TEST_ASSERT_STR_EQ(suite, expected, actual) do { \
    if (strcmp((expected), (actual)) != 0) { \
        char msg[256]; \
        snprintf(msg, sizeof(msg), "Expected '%s', got '%s'", \
                 (expected), (actual)); \
        add_test_result(suite, TEST_FAIL, msg, __FILE__, __LINE__); \
        return; \
    } \
} while(0)

#define TEST_ASSERT_PERFORMANCE(suite, operation, max_cycles) do { \
    uint64_t start = s7t_cycles(); \
    operation; \
    uint64_t elapsed = s7t_cycles() - start; \
    if (elapsed > (max_cycles)) { \
        char msg[256]; \
        snprintf(msg, sizeof(msg), "Performance fail: %llu > %llu cycles", \
                 elapsed, (uint64_t)(max_cycles)); \
        add_test_result(suite, TEST_FAIL, msg, __FILE__, __LINE__); \
        return; \
    } \
} while(0)

/*═══════════════════════════════════════════════════════════════
  Test Framework Functions
  ═══════════════════════════════════════════════════════════════*/

static void add_test_result(test_suite_t* suite, test_status_t status,
                          const char* msg, const char* file, int line) {
    if (suite->count >= suite->capacity) {
        suite->capacity = suite->capacity ? suite->capacity * 2 : 16;
        suite->results = realloc(suite->results, 
                               suite->capacity * sizeof(test_result_t));
    }
    
    test_result_t* result = &suite->results[suite->count++];
    result->name = suite->name;
    result->suite = suite->name;
    result->status = status;
    result->error_msg = msg;
    result->file = file;
    result->line = line;
    result->cycles = 0;
    result->memory_bytes = 0;
    
    switch(status) {
        case TEST_PASS: suite->passed++; break;
        case TEST_FAIL: suite->failed++; break;
        case TEST_SKIP: suite->skipped++; break;
        case TEST_ERROR: suite->errors++; break;
    }
}

static test_suite_t* create_test_suite(const char* name) {
    test_suite_t* suite = calloc(1, sizeof(test_suite_t));
    suite->name = name;
    suite->capacity = 16;
    suite->results = calloc(suite->capacity, sizeof(test_result_t));
    return suite;
}

static void destroy_test_suite(test_suite_t* suite) {
    if (suite) {
        free(suite->results);
        free(suite);
    }
}

static void print_test_summary(test_suite_t* suite) {
    printf("\n=== Test Suite: %s ===\n", suite->name);
    printf("Total: %zu | Passed: %d | Failed: %d | Skipped: %d | Errors: %d\n",
           suite->count, suite->passed, suite->failed, 
           suite->skipped, suite->errors);
    
    if (suite->failed > 0 || suite->errors > 0) {
        printf("\nFailures:\n");
        for (size_t i = 0; i < suite->count; i++) {
            test_result_t* r = &suite->results[i];
            if (r->status == TEST_FAIL || r->status == TEST_ERROR) {
                printf("  [%s] %s:%d - %s\n", 
                       r->status == TEST_FAIL ? "FAIL" : "ERROR",
                       r->file, r->line, r->error_msg);
            }
        }
    }
    
    if (suite->total_cycles > 0) {
        printf("\nPerformance: %llu total cycles (avg: %llu)\n",
               suite->total_cycles, 
               suite->count > 0 ? suite->total_cycles / suite->count : 0);
    }
}

/*═══════════════════════════════════════════════════════════════
  CNS-Specific Test Cases
  ═══════════════════════════════════════════════════════════════*/

// Test the parser
static void test_parser_basic(test_suite_t* suite, void* context) {
    cns_command_t cmd;
    cns_result_t result;
    
    // Test simple command
    result = cns_parse("echo hello world", &cmd);
    TEST_ASSERT_EQ(suite, CNS_OK, result);
    TEST_ASSERT_STR_EQ(suite, "echo", cmd.name);
    TEST_ASSERT_EQ(suite, 2, cmd.argc);
    TEST_ASSERT_STR_EQ(suite, "hello", cmd.args[0]);
    TEST_ASSERT_STR_EQ(suite, "world", cmd.args[1]);
    
    add_test_result(suite, TEST_PASS, "Parser basic test passed", __FILE__, __LINE__);
}

// Test parser edge cases
static void test_parser_edge_cases(test_suite_t* suite, void* context) {
    cns_command_t cmd;
    cns_result_t result;
    
    // Empty command
    result = cns_parse("", &cmd);
    TEST_ASSERT_EQ(suite, CNS_ERR_EMPTY, result);
    
    // Only whitespace
    result = cns_parse("   \t  ", &cmd);
    TEST_ASSERT_EQ(suite, CNS_ERR_EMPTY, result);
    
    // Too many arguments
    char long_cmd[256] = "test";
    for (int i = 0; i < CNS_MAX_ARGS + 2; i++) {
        strcat(long_cmd, " arg");
    }
    result = cns_parse(long_cmd, &cmd);
    TEST_ASSERT_EQ(suite, CNS_OK, result);
    TEST_ASSERT_EQ(suite, CNS_MAX_ARGS, cmd.argc);
    
    add_test_result(suite, TEST_PASS, "Parser edge cases passed", __FILE__, __LINE__);
}

// Test command lookup performance
static void test_lookup_performance(test_suite_t* suite, void* context) {
    cns_engine_t* engine = (cns_engine_t*)context;
    
    // Register test command
    CNS_REGISTER_CMD(engine, "perftest", NULL, 0, 0, "Performance test");
    
    // Test lookup performance
    TEST_ASSERT_PERFORMANCE(suite, {
        for (int i = 0; i < 1000; i++) {
            uint32_t hash = s7t_hash_string("perftest", 8);
            cns_cmd_entry_t* entry = cns_lookup(engine, hash);
            (void)entry; // Suppress unused warning
        }
    }, 100000); // Max 100k cycles for 1000 lookups
    
    add_test_result(suite, TEST_PASS, "Lookup performance test passed", __FILE__, __LINE__);
}

// Test command execution
static int test_handler_called = 0;
CNS_HANDLER(test_handler) {
    test_handler_called++;
    return CNS_OK;
}

static void test_command_execution(test_suite_t* suite, void* context) {
    cns_engine_t* engine = (cns_engine_t*)context;
    cns_command_t cmd;
    
    // Register and execute test command
    test_handler_called = 0;
    CNS_REGISTER_CMD(engine, "testcmd", test_handler, 0, 0, "Test command");
    
    cns_parse("testcmd", &cmd);
    cns_result_t result = cns_execute(engine, &cmd);
    
    TEST_ASSERT_EQ(suite, CNS_OK, result);
    TEST_ASSERT_EQ(suite, 1, test_handler_called);
    
    add_test_result(suite, TEST_PASS, "Command execution test passed", __FILE__, __LINE__);
}

// Test hash distribution
static void test_hash_distribution(test_suite_t* suite, void* context) {
    const char* test_strings[] = {
        "help", "exit", "echo", "status", "clear", "time",
        "batch", "alias", "history", "debug", "dump",
        "test", "bench", "profile", "monitor", "trace"
    };
    
    uint32_t hashes[16];
    int count = sizeof(test_strings) / sizeof(test_strings[0]);
    
    // Generate hashes
    for (int i = 0; i < count; i++) {
        hashes[i] = s7t_hash_string(test_strings[i], strlen(test_strings[i]));
    }
    
    // Check for collisions
    int collisions = 0;
    for (int i = 0; i < count; i++) {
        for (int j = i + 1; j < count; j++) {
            if (hashes[i] == hashes[j]) {
                collisions++;
            }
        }
    }
    
    TEST_ASSERT_EQ(suite, 0, collisions);
    
    add_test_result(suite, TEST_PASS, "Hash distribution test passed", __FILE__, __LINE__);
}

/*═══════════════════════════════════════════════════════════════
  Test Suite Registration
  ═══════════════════════════════════════════════════════════════*/

static test_case_t cns_test_cases[] = {
    {"parser_basic", test_parser_basic, "Basic parser functionality", true},
    {"parser_edge_cases", test_parser_edge_cases, "Parser edge cases", true},
    {"lookup_performance", test_lookup_performance, "Command lookup performance", true},
    {"command_execution", test_command_execution, "Command execution", true},
    {"hash_distribution", test_hash_distribution, "Hash function distribution", true},
};

/*═══════════════════════════════════════════════════════════════
  Test Command Implementation
  ═══════════════════════════════════════════════════════════════*/

// Main test command handler
int cmd_test(int argc, char** argv) {
    // Parse arguments
    const char* suite_name = "all";
    const char* test_name = NULL;
    bool verbose = false;
    bool list_only = false;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = true;
        } else if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--list") == 0) {
            list_only = true;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printf("Usage: cns test [options] [suite] [test]\n");
            printf("Options:\n");
            printf("  -v, --verbose   Show detailed output\n");
            printf("  -l, --list      List available tests\n");
            printf("  -h, --help      Show this help\n");
            printf("\nTest Suites:\n");
            printf("  all       Run all tests (default)\n");
            printf("  core      Run core CNS tests\n");
            printf("  perf      Run performance tests\n");
            printf("\nExamples:\n");
            printf("  cns test                  # Run all tests\n");
            printf("  cns test core             # Run core tests\n");
            printf("  cns test -v parser_basic  # Run specific test verbosely\n");
            return 0;
        } else if (suite_name == "all" && argv[i][0] != '-') {
            suite_name = argv[i];
        } else if (test_name == NULL && argv[i][0] != '-') {
            test_name = argv[i];
        }
    }
    
    // List tests if requested
    if (list_only) {
        printf("Available tests:\n");
        int num_tests = sizeof(cns_test_cases) / sizeof(cns_test_cases[0]);
        for (int i = 0; i < num_tests; i++) {
            printf("  %-20s %s\n", cns_test_cases[i].name, 
                   cns_test_cases[i].description);
        }
        return 0;
    }
    
    // Create test engine for testing
    cns_engine_t* test_engine = cns_create(CNS_MAX_COMMANDS);
    if (!test_engine) {
        printf("Failed to create test engine\n");
        return 1;
    }
    
    // Initialize with basic commands
    cns_register_builtins(test_engine);
    
    // Create test suite
    test_suite_t* suite = create_test_suite("CNS Unit Tests");
    
    // Run tests
    printf("Running CNS unit tests...\n");
    int num_tests = sizeof(cns_test_cases) / sizeof(cns_test_cases[0]);
    int tests_run = 0;
    
    for (int i = 0; i < num_tests; i++) {
        test_case_t* tc = &cns_test_cases[i];
        
        // Skip if specific test requested and doesn't match
        if (test_name && strcmp(tc->name, test_name) != 0) {
            continue;
        }
        
        // Skip disabled tests
        if (!tc->enabled) {
            continue;
        }
        
        // Run test
        if (verbose) {
            printf("\n[TEST] %s - %s\n", tc->name, tc->description);
        }
        
        uint64_t start = s7t_cycles();
        tc->func(suite, test_engine);
        uint64_t elapsed = s7t_cycles() - start;
        suite->total_cycles += elapsed;
        tests_run++;
        
        if (verbose) {
            printf("  Completed in %llu cycles\n", elapsed);
        }
    }
    
    // Print summary
    if (tests_run > 0) {
        print_test_summary(suite);
    } else {
        printf("No tests matched criteria\n");
    }
    
    // Cleanup
    int exit_code = (suite->failed > 0 || suite->errors > 0) ? 1 : 0;
    destroy_test_suite(suite);
    cns_destroy(test_engine);
    
    return exit_code;
}

/*═══════════════════════════════════════════════════════════════
  CNS Handler for Test Command
  ═══════════════════════════════════════════════════════════════*/

// CNS handler wrapper
CNS_HANDLER(cns_cmd_test) {
    // Convert CNS command to argc/argv format
    char* argv[CNS_MAX_ARGS + 2];
    int argc = 1;
    
    argv[0] = "test";
    for (uint8_t i = 0; i < cmd->argc && argc < CNS_MAX_ARGS + 1; i++) {
        argv[argc++] = cmd->args[i];
    }
    argv[argc] = NULL;
    
    // Call the main test function
    int result = cmd_test(argc, argv);
    
    return (result == 0) ? CNS_OK : CNS_ERR_COMMAND_FAIL;
}