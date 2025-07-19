#include "cns/cli.h"
#include "cns/telemetry/otel.h"
#include "../../../include/s7t.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

// Global telemetry instance
static cns_telemetry_t* g_telemetry = NULL;

// Initialize telemetry if needed
static void ensure_telemetry_init(void) {
    if (!g_telemetry) {
        g_telemetry = malloc(sizeof(cns_telemetry_t));
        if (g_telemetry) {
            cns_telemetry_config_t config = CNS_DEFAULT_TELEMETRY_CONFIG;
            config.service_name = "cns-test";
            cns_telemetry_init(g_telemetry, &config);
        }
    }
}

// Test all command handler
static int cmd_test_all(CNSContext* ctx, int argc, char** argv) {
    ensure_telemetry_init();
    
    // Create parent span for all tests
    CNS_SPAN_SCOPE(g_telemetry, "test.all", NULL);
    
    // Check if test binary exists
    struct stat st;
    if (stat("build/cns_tests", &st) != 0) {
        cns_cli_error("Test binary not found. Run 'cns build all' first");
        cns_span_end(_span, CNS_SPAN_STATUS_ERROR);
        return CNS_ERROR_IO;
    }
    
    cns_cli_info("Running all tests...");
    
    // Add test event
    cns_span_add_event(_span, "test_start", NULL, 0);
    
    uint64_t start_cycles = s7t_cycles();
    int result = system("./build/cns_tests");
    uint64_t elapsed_cycles = s7t_cycles() - start_cycles;
    
    // Record metrics
    cns_metric_record_latency(g_telemetry, "test.all", elapsed_cycles);
    cns_metric_inc_counter(g_telemetry, "test.all", result == 0 ? CNS_OK : CNS_ERR_INTERNAL);
    
    // Add result attributes
    cns_attribute_t attrs[] = {
        {.key = "test.result", .type = CNS_ATTR_STRING, .string_value = result == 0 ? "passed" : "failed"},
        {.key = "test.elapsed_cycles", .type = CNS_ATTR_INT64, .int64_value = elapsed_cycles}
    };
    cns_span_set_attributes(_span, attrs, 2);
    
    if (result != 0) {
        cns_cli_error("Tests failed");
        cns_span_end(_span, CNS_SPAN_STATUS_ERROR);
        return CNS_ERROR;
    }
    
    cns_cli_success("All tests passed");
    return CNS_OK;
}

// Test unit command handler
static int cmd_test_unit(CNSContext* ctx, int argc, char** argv) {
    ensure_telemetry_init();
    
    CNS_SPAN_SCOPE(g_telemetry, "test.unit", NULL);
    
    // Check if test binary exists
    struct stat st;
    if (stat("build/cns_tests", &st) != 0) {
        cns_cli_error("Test binary not found. Run 'cns build all' first");
        cns_span_end(_span, CNS_SPAN_STATUS_ERROR);
        return CNS_ERROR_IO;
    }
    
    cns_cli_info("Running unit tests...");
    
    // Run with filter if provided
    char cmd[256];
    const char* filter = argc > 0 ? argv[0] : NULL;
    if (filter) {
        snprintf(cmd, sizeof(cmd), "./build/cns_tests -n %s", filter);
        CNS_SPAN_SET_ATTR(_span, "test.filter", filter);
    } else {
        snprintf(cmd, sizeof(cmd), "./build/cns_tests");
    }
    
    uint64_t start_cycles = s7t_cycles();
    int result = system(cmd);
    uint64_t elapsed_cycles = s7t_cycles() - start_cycles;
    
    // Record metrics
    cns_metric_record_latency(g_telemetry, "test.unit", elapsed_cycles);
    
    if (result != 0) {
        cns_cli_error("Unit tests failed");
        cns_span_end(_span, CNS_SPAN_STATUS_ERROR);
        return CNS_ERROR;
    }
    
    cns_cli_success("Unit tests passed");
    return CNS_OK;
}

// Test coverage command handler
static int cmd_test_coverage(CNSContext* ctx, int argc, char** argv) {
    ensure_telemetry_init();
    
    CNS_SPAN_SCOPE(g_telemetry, "test.coverage", NULL);
    
    cns_cli_info("Generating test coverage report...");
    
    // Build with coverage flags
    cns_span_add_event(_span, "build_start", NULL, 0);
    
    uint64_t start_cycles = s7t_cycles();
    int result = system("cmake -S . -B build-coverage "
                       "-DCMAKE_C_FLAGS=\"--coverage\" "
                       "-DCMAKE_EXE_LINKER_FLAGS=\"--coverage\" && "
                       "ninja -C build-coverage && "
                       "./build-coverage/cns_tests && "
                       "gcov -o build-coverage src/*.c");
    uint64_t elapsed_cycles = s7t_cycles() - start_cycles;
    
    // Record metrics
    cns_metric_record_latency(g_telemetry, "test.coverage", elapsed_cycles);
    
    if (result != 0) {
        cns_cli_error("Coverage generation failed");
        cns_span_end(_span, CNS_SPAN_STATUS_ERROR);
        return CNS_ERROR;
    }
    
    cns_cli_success("Coverage report generated");
    return CNS_OK;
}

// Test arguments
static CNSArgument test_unit_args[] = {
    {
        .name = "filter",
        .description = "Test name filter pattern",
        .required = false,
        .variadic = false
    }
};

// Test commands
static CNSCommand test_commands[] = {
    {
        .name = "all",
        .description = "Run all tests",
        .handler = cmd_test_all,
        .options = NULL,
        .option_count = 0,
        .arguments = NULL,
        .argument_count = 0
    },
    {
        .name = "unit",
        .description = "Run unit tests",
        .handler = cmd_test_unit,
        .options = NULL,
        .option_count = 0,
        .arguments = test_unit_args,
        .argument_count = 1
    },
    {
        .name = "coverage",
        .description = "Generate test coverage report",
        .handler = cmd_test_coverage,
        .options = NULL,
        .option_count = 0,
        .arguments = NULL,
        .argument_count = 0
    }
};

// Test domain
CNSDomain cns_test_domain = {
    .name = "test",
    .description = "Testing framework",
    .commands = test_commands,
    .command_count = sizeof(test_commands) / sizeof(test_commands[0])
};

// Cleanup function to be called at program exit
void __attribute__((destructor)) test_cleanup(void) {
    if (g_telemetry) {
        cns_telemetry_shutdown(g_telemetry);
        free(g_telemetry);
        g_telemetry = NULL;
    }
}