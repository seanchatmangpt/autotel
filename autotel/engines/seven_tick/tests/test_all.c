#include "7t_unit_test_framework.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Test suite declarations
extern TestSuiteRegistration seven_t_runtime_registration;
extern TestSuiteRegistration seven_t_tpot_registration;
extern TestSuiteRegistration telemetry7t_registration;

// Test suite registration functions
extern void register_seven_t_runtime_tests(void);
extern void register_seven_t_tpot_tests(void);
extern void register_telemetry7t_tests(void);

// Global test report
TestReport* global_report = NULL;

// Test configuration
typedef struct {
    bool run_runtime_tests;
    bool run_tpot_tests;
    bool run_telemetry_tests;
    bool run_performance_tests;
    bool run_memory_tests;
    bool run_stress_tests;
    bool verbose_output;
    bool stop_on_failure;
    const char* output_format;
    const char* output_file;
} TestRunnerConfig;

TestRunnerConfig runner_config = {
    .run_runtime_tests = true,
    .run_tpot_tests = true,
    .run_telemetry_tests = true,
    .run_performance_tests = false,
    .run_memory_tests = false,
    .run_stress_tests = false,
    .verbose_output = false,
    .stop_on_failure = false,
    .output_format = "text",
    .output_file = NULL
};

// Function prototypes
void print_test_banner(void);
void print_test_footer(void);
void parse_command_line_args(int argc, char* argv[]);
void print_usage(const char* program_name);
void run_test_suite_with_report(TestSuiteRegistration* registration);
void print_summary_report(void);
void export_test_results(void);

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

int main(int argc, char* argv[]) {
    // Parse command line arguments
    parse_command_line_args(argc, argv);
    
    // Print test banner
    print_test_banner();
    
    // Initialize test framework
    test_config_init();
    test_config_set_verbose(runner_config.verbose_output);
    test_config_set_stop_on_failure(runner_config.stop_on_failure);
    
    // Create global test report
    global_report = test_report_create();
    if (!global_report) {
        fprintf(stderr, "Failed to create test report\n");
        return 1;
    }
    
    // Register all test suites
    register_seven_t_runtime_tests();
    register_seven_t_tpot_tests();
    register_telemetry7t_tests();
    
    // Run test suites based on configuration
    if (runner_config.run_runtime_tests) {
        printf("\n=== Running Runtime Tests ===\n");
        run_test_suite_with_report(&seven_t_runtime_registration);
    }
    
    if (runner_config.run_tpot_tests) {
        printf("\n=== Running TPOT Tests ===\n");
        run_test_suite_with_report(&seven_t_tpot_registration);
    }
    
    if (runner_config.run_telemetry_tests) {
        printf("\n=== Running Telemetry Tests ===\n");
        run_test_suite_with_report(&telemetry7t_registration);
    }
    
    // Print summary report
    print_summary_report();
    
    // Export results if requested
    if (runner_config.output_file) {
        export_test_results();
    }
    
    // Print test footer
    print_test_footer();
    
    // Cleanup
    test_report_destroy(global_report);
    
    // Return exit code based on test results
    if (global_report->failed_tests > 0 || global_report->error_tests > 0) {
        return 1; // Tests failed
    }
    
    return 0; // All tests passed
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

void print_test_banner(void) {
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║                    7T Engine Unit Tests                      ║\n");
    printf("║                    Comprehensive Test Suite                  ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    printf("Framework Version: %s\n", SEVEN_TICK_TEST_VERSION);
    printf("Test Configuration:\n");
    printf("  Runtime Tests:     %s\n", runner_config.run_runtime_tests ? "✅ Enabled" : "❌ Disabled");
    printf("  TPOT Tests:        %s\n", runner_config.run_tpot_tests ? "✅ Enabled" : "❌ Disabled");
    printf("  Telemetry Tests:   %s\n", runner_config.run_telemetry_tests ? "✅ Enabled" : "❌ Disabled");
    printf("  Performance Tests: %s\n", runner_config.run_performance_tests ? "✅ Enabled" : "❌ Disabled");
    printf("  Memory Tests:      %s\n", runner_config.run_memory_tests ? "✅ Enabled" : "❌ Disabled");
    printf("  Stress Tests:      %s\n", runner_config.run_stress_tests ? "✅ Enabled" : "❌ Disabled");
    printf("  Verbose Output:    %s\n", runner_config.verbose_output ? "✅ Enabled" : "❌ Disabled");
    printf("  Stop on Failure:   %s\n", runner_config.stop_on_failure ? "✅ Enabled" : "❌ Disabled");
    printf("\n");
}

void print_test_footer(void) {
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║                    Test Execution Complete                   ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");
}

void parse_command_line_args(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            exit(0);
        } else if (strcmp(argv[i], "--runtime-only") == 0) {
            runner_config.run_runtime_tests = true;
            runner_config.run_tpot_tests = false;
            runner_config.run_telemetry_tests = false;
        } else if (strcmp(argv[i], "--tpot-only") == 0) {
            runner_config.run_runtime_tests = false;
            runner_config.run_tpot_tests = true;
            runner_config.run_telemetry_tests = false;
        } else if (strcmp(argv[i], "--telemetry-only") == 0) {
            runner_config.run_runtime_tests = false;
            runner_config.run_tpot_tests = false;
            runner_config.run_telemetry_tests = true;
        } else if (strcmp(argv[i], "--performance") == 0) {
            runner_config.run_performance_tests = true;
        } else if (strcmp(argv[i], "--memory") == 0) {
            runner_config.run_memory_tests = true;
        } else if (strcmp(argv[i], "--stress") == 0) {
            runner_config.run_stress_tests = true;
        } else if (strcmp(argv[i], "--verbose") == 0 || strcmp(argv[i], "-v") == 0) {
            runner_config.verbose_output = true;
        } else if (strcmp(argv[i], "--stop-on-failure") == 0) {
            runner_config.stop_on_failure = true;
        } else if (strcmp(argv[i], "--output") == 0 || strcmp(argv[i], "-o") == 0) {
            if (i + 1 < argc) {
                runner_config.output_file = argv[++i];
            } else {
                fprintf(stderr, "Error: --output requires a filename\n");
                exit(1);
            }
        } else if (strcmp(argv[i], "--format") == 0) {
            if (i + 1 < argc) {
                runner_config.output_format = argv[++i];
            } else {
                fprintf(stderr, "Error: --format requires a format (json, xml, text)\n");
                exit(1);
            }
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            exit(1);
        }
    }
}

void print_usage(const char* program_name) {
    printf("Usage: %s [OPTIONS]\n", program_name);
    printf("\n");
    printf("Options:\n");
    printf("  --help, -h              Show this help message\n");
    printf("  --runtime-only          Run only runtime tests\n");
    printf("  --tpot-only             Run only TPOT tests\n");
    printf("  --telemetry-only        Run only telemetry tests\n");
    printf("  --performance           Enable performance tests\n");
    printf("  --memory                Enable memory tests\n");
    printf("  --stress                Enable stress tests\n");
    printf("  --verbose, -v           Enable verbose output\n");
    printf("  --stop-on-failure       Stop on first test failure\n");
    printf("  --output FILE, -o FILE  Output results to file\n");
    printf("  --format FORMAT         Output format (json, xml, text)\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s                      # Run all tests\n", program_name);
    printf("  %s --runtime-only       # Run only runtime tests\n", program_name);
    printf("  %s --performance        # Run with performance tests\n", program_name);
    printf("  %s --output results.json --format json  # Export to JSON\n", program_name);
    printf("  %s --verbose --stop-on-failure  # Verbose with early exit\n", program_name);
}

void run_test_suite_with_report(TestSuiteRegistration* registration) {
    if (!registration) return;
    
    TestSuite* suite = test_suite_create(registration->suite_name);
    if (!suite) return;
    
    // Run suite setup if provided
    if (registration->suite_setup) {
        TestContext context = {0};
        registration->suite_setup(&context);
    }
    
    // Run all test cases
    for (size_t i = 0; i < registration->test_case_count; i++) {
        TestCase* test_case = &registration->test_cases[i];
        
        if (runner_config.verbose_output) {
            print_test_header(test_case->test_name);
        }
        
        TestContext context = {0};
        TestResult result = test_execute_single(test_case, &context);
        result.test_suite = registration->suite_name;
        
        test_suite_add_result(suite, result);
        test_report_add_result(global_report, result);
        
        if (runner_config.verbose_output) {
            print_test_footer(test_case->test_name, result.status, result.execution_time_ns);
        }
        
        // Stop on failure if configured
        if (runner_config.stop_on_failure && 
            (result.status == TEST_FAIL || result.status == TEST_ERROR)) {
            break;
        }
    }
    
    // Run suite teardown if provided
    if (registration->suite_teardown) {
        TestContext context = {0};
        registration->suite_teardown(&context);
    }
    
    // Print suite results
    if (runner_config.verbose_output) {
        test_suite_print_detailed(suite);
    } else {
        test_suite_print_summary(suite);
    }
    
    test_suite_destroy(suite);
}

void print_summary_report(void) {
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║                      SUMMARY REPORT                          ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    
    test_report_print_summary(global_report);
    
    printf("\n");
    printf("Test Categories:\n");
    printf("  Runtime Tests:     %s\n", runner_config.run_runtime_tests ? "✅ Executed" : "⏭️  Skipped");
    printf("  TPOT Tests:        %s\n", runner_config.run_tpot_tests ? "✅ Executed" : "⏭️  Skipped");
    printf("  Telemetry Tests:   %s\n", runner_config.run_telemetry_tests ? "✅ Executed" : "⏭️  Skipped");
    printf("  Performance Tests: %s\n", runner_config.run_performance_tests ? "✅ Executed" : "⏭️  Skipped");
    printf("  Memory Tests:      %s\n", runner_config.run_memory_tests ? "✅ Executed" : "⏭️  Skipped");
    printf("  Stress Tests:      %s\n", runner_config.run_stress_tests ? "✅ Executed" : "⏭️  Skipped");
    
    printf("\n");
    if (global_report->failed_tests == 0 && global_report->error_tests == 0) {
        printf("🎉 ALL TESTS PASSED! 🎉\n");
    } else {
        printf("❌ SOME TESTS FAILED! ❌\n");
    }
}

void export_test_results(void) {
    if (!runner_config.output_file) return;
    
    printf("\nExporting test results to: %s\n", runner_config.output_file);
    
    if (strcmp(runner_config.output_format, "json") == 0) {
        test_report_export_json(global_report, runner_config.output_file);
        printf("Results exported in JSON format\n");
    } else if (strcmp(runner_config.output_format, "xml") == 0) {
        test_report_export_junit_xml(global_report, runner_config.output_file);
        printf("Results exported in JUnit XML format\n");
    } else {
        // Default to text format
        FILE* file = fopen(runner_config.output_file, "w");
        if (file) {
            fprintf(file, "7T Engine Unit Test Results\n");
            fprintf(file, "===========================\n\n");
            fprintf(file, "Total tests: %d\n", global_report->total_tests);
            fprintf(file, "Passed: %d\n", global_report->passed_tests);
            fprintf(file, "Failed: %d\n", global_report->failed_tests);
            fprintf(file, "Skipped: %d\n", global_report->skipped_tests);
            fprintf(file, "Errors: %d\n", global_report->error_tests);
            fprintf(file, "Success rate: %.1f%%\n", global_report->success_rate);
            fprintf(file, "Total time: %.3f ms\n", global_report->total_time_ns / 1000000.0);
            fprintf(file, "Total memory: %.2f KB\n", global_report->total_memory_bytes / 1024.0);
            fclose(file);
            printf("Results exported in text format\n");
        } else {
            fprintf(stderr, "Error: Could not open output file %s\n", runner_config.output_file);
        }
    }
} 