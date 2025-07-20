#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "test_utils.h"

// Global test stats
TestStats g_test_stats = {0};

// External test suite functions
void run_lexer_tests();
void run_parser_tests();
void run_integration_tests();
void run_error_handling_tests();

// Signal handler for catching crashes
void crash_handler(int sig) {
    printf("\n🔥 Test crashed with signal %d!\n", sig);
    printf("Failed test stats: %d passed, %d failed\n", 
           g_test_stats.passed, g_test_stats.failed);
    exit(1);
}

// Command line options
typedef struct {
    int verbose;
    int memory_debug;
    int performance;
    int quick;
    char* filter;
    char* suite;
} TestOptions;

void print_usage(const char* program) {
    printf("Usage: %s [options]\n", program);
    printf("Options:\n");
    printf("  -h, --help           Show this help\n");
    printf("  -v, --verbose        Verbose output\n");
    printf("  -m, --memory         Enable memory leak detection\n");
    printf("  -p, --performance    Run performance tests\n");
    printf("  -q, --quick          Quick tests only (skip slow tests)\n");
    printf("  -f, --filter PATTERN Filter tests by pattern\n");
    printf("  -s, --suite NAME     Run specific test suite\n");
    printf("\nTest suites:\n");
    printf("  lexer       - Lexer unit tests\n");
    printf("  parser      - Parser unit tests\n");
    printf("  integration - Integration tests\n");
    printf("  error       - Error handling tests\n");
    printf("  performance - Performance tests\n");
    printf("  edge        - Edge case tests\n");
    printf("  all         - Run all tests (default)\n");
}

TestOptions parse_options(int argc, char** argv) {
    TestOptions opts = {0};
    opts.suite = "all";
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            exit(0);
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            opts.verbose = 1;
        } else if (strcmp(argv[i], "-m") == 0 || strcmp(argv[i], "--memory") == 0) {
            opts.memory_debug = 1;
        } else if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--performance") == 0) {
            opts.performance = 1;
        } else if (strcmp(argv[i], "-q") == 0 || strcmp(argv[i], "--quick") == 0) {
            opts.quick = 1;
        } else if ((strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--filter") == 0) && i + 1 < argc) {
            opts.filter = argv[++i];
        } else if ((strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--suite") == 0) && i + 1 < argc) {
            opts.suite = argv[++i];
        } else {
            printf("Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            exit(1);
        }
    }
    
    return opts;
}

int main(int argc, char** argv) {
    printf("🚀 TTL Parser Test Suite\n");
    printf("========================\n");
    
    // Parse command line options
    TestOptions opts = parse_options(argc, argv);
    
    // Set up signal handlers
    signal(SIGSEGV, crash_handler);
    signal(SIGABRT, crash_handler);
    
    // Initialize test statistics
    init_test_stats();
    
    // Run test suites based on options
    if (strcmp(opts.suite, "all") == 0) {
        run_lexer_tests();
        run_parser_tests();
        run_integration_tests();
        run_error_handling_tests();
    } else if (strcmp(opts.suite, "lexer") == 0) {
        run_lexer_tests();
    } else if (strcmp(opts.suite, "parser") == 0) {
        run_parser_tests();
    } else if (strcmp(opts.suite, "integration") == 0) {
        run_integration_tests();
    } else if (strcmp(opts.suite, "error") == 0) {
        run_error_handling_tests();
    } else {
        printf("Unknown test suite: %s\n", opts.suite);
        print_usage(argv[0]);
        return 1;
    }
    
    // Print test summary
    print_test_summary();
    
    // Check for memory leaks if enabled
    if (opts.memory_debug) {
        check_memory_leaks();
    }
    
    // Return exit code based on test results
    return (g_test_stats.failed > 0) ? 1 : 0;
}

// Stub implementations for missing test suites (will be linked with actual implementations)