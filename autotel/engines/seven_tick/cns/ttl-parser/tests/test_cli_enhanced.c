/**
 * @file test_cli_enhanced.c
 * @brief Comprehensive tests for enhanced CLI features
 * @author IntegrationTester Agent
 * @date 2024
 * 
 * Tests all Phase 2 CLI enhancements:
 * - Output format selection
 * - Multiple input files
 * - Query execution
 * - Performance reporting
 * - Configuration options
 */

#include "test_utils.h"
#include "../include/serializer.h"
#include "../include/query.h"
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

// Global test stats
TestStats g_test_stats;

// Test fixture paths
static const char* test_input_file = "tests/fixtures/phase2/cli/test_input.ttl";
static const char* test_query_file = "tests/fixtures/phase2/cli/test_query.ttl";
static const char* test_config_file = "tests/fixtures/phase2/cli/test_config.json";

/**
 * Create test fixtures for CLI testing
 */
static int setup_cli_test_fixtures() {
    printf("    Setting up CLI test fixtures...\n");
    
    // Create test TTL file
    FILE* ttl_file = fopen(test_input_file, "w");
    TEST_ASSERT_NOT_NULL(ttl_file);
    
    fprintf(ttl_file, 
        "@prefix ex: <http://example.org/> .\n"
        "@prefix foaf: <http://xmlns.com/foaf/0.1/> .\n"
        "\n"
        "ex:person1 a foaf:Person ;\n"
        "    foaf:name \"Alice Johnson\" ;\n"
        "    foaf:age 28 ;\n"
        "    foaf:email \"alice@example.com\" ;\n"
        "    foaf:knows ex:person2, ex:person3 .\n"
        "\n"
        "ex:person2 a foaf:Person ;\n"
        "    foaf:name \"Bob Smith\" ;\n"
        "    foaf:age 35 ;\n"
        "    foaf:occupation \"Engineer\" .\n"
        "\n"
        "ex:person3 a foaf:Person ;\n"
        "    foaf:name \"Carol Davis\" ;\n"
        "    foaf:age 42 ;\n"
        "    foaf:occupation \"Manager\" .\n"
    );
    fclose(ttl_file);
    
    // Create test query file
    FILE* query_file = fopen(test_query_file, "w");
    TEST_ASSERT_NOT_NULL(query_file);
    
    fprintf(query_file, "?person a foaf:Person");
    fclose(query_file);
    
    // Create test configuration file
    FILE* config_file = fopen(test_config_file, "w");
    TEST_ASSERT_NOT_NULL(config_file);
    
    fprintf(config_file,
        "{\n"
        "  \"default_format\": \"ntriples\",\n"
        "  \"pretty_print\": true,\n"
        "  \"use_prefixes\": true,\n"
        "  \"performance_tracking\": true,\n"
        "  \"output_directory\": \"./output\",\n"
        "  \"max_memory_mb\": 512\n"
        "}\n"
    );
    fclose(config_file);
    
    printf("      Test fixtures created successfully\n");
    return 1;
}

/**
 * Test basic CLI functionality
 */
static int test_basic_cli_functionality() {
    printf("    Testing basic CLI functionality...\n");
    
    // Test help option
    int status = system("./ttl-parser --help > /dev/null 2>&1");
    TEST_ASSERT(WEXITSTATUS(status) == 0);
    
    // Test version option
    status = system("./ttl-parser --version > /dev/null 2>&1");
    TEST_ASSERT(WEXITSTATUS(status) == 0);
    
    // Test basic parsing without output format
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "./ttl-parser %s > /dev/null 2>&1", test_input_file);
    status = system(cmd);
    TEST_ASSERT(WEXITSTATUS(status) == 0);
    
    printf("      Basic CLI operations: Successful\n");
    return 1;
}

/**
 * Test output format selection
 */
static int test_output_format_selection() {
    printf("    Testing output format selection...\n");
    
    const char* formats[] = {"ntriples", "jsonld", "rdfxml"};
    const char* extensions[] = {"nt", "jsonld", "rdf"};
    
    for (int i = 0; i < 3; i++) {
        char cmd[512];
        char output_file[256];
        
        snprintf(output_file, sizeof(output_file), "test_output.%s", extensions[i]);
        snprintf(cmd, sizeof(cmd), 
                "./ttl-parser --format %s --output %s %s 2>/dev/null", 
                formats[i], output_file, test_input_file);
        
        int status = system(cmd);
        TEST_ASSERT(WEXITSTATUS(status) == 0);
        
        // Verify output file was created
        FILE* output = fopen(output_file, "r");
        TEST_ASSERT_NOT_NULL(output);
        
        // Check file has content
        fseek(output, 0, SEEK_END);
        long size = ftell(output);
        TEST_ASSERT(size > 0);
        
        printf("      Format %s: Generated %ld bytes\n", formats[i], size);
        
        fclose(output);
        unlink(output_file);
    }
    
    return 1;
}

/**
 * Test multiple input files
 */
static int test_multiple_input_files() {
    printf("    Testing multiple input files...\n");
    
    // Create second input file
    const char* input2 = "tests/fixtures/phase2/cli/test_input2.ttl";
    FILE* ttl_file2 = fopen(input2, "w");
    TEST_ASSERT_NOT_NULL(ttl_file2);
    
    fprintf(ttl_file2,
        "@prefix ex2: <http://example2.org/> .\n"
        "@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> .\n"
        "\n"
        "ex2:Company a rdfs:Class ;\n"
        "    rdfs:label \"Company Class\" .\n"
        "\n"
        "ex2:company1 a ex2:Company ;\n"
        "    ex2:name \"TechCorp\" ;\n"
        "    ex2:founded \"2010\" .\n"
    );
    fclose(ttl_file2);
    
    // Test parsing multiple files
    char cmd[512];
    snprintf(cmd, sizeof(cmd), 
            "./ttl-parser --format ntriples --output combined.nt %s %s 2>/dev/null",
            test_input_file, input2);
    
    int status = system(cmd);
    TEST_ASSERT(WEXITSTATUS(status) == 0);
    
    // Verify combined output
    FILE* output = fopen("combined.nt", "r");
    TEST_ASSERT_NOT_NULL(output);
    
    // Count triples by counting lines ending with " ."
    char line[1024];
    int triple_count = 0;
    while (fgets(line, sizeof(line), output)) {
        if (strstr(line, " .")) {
            triple_count++;
        }
    }
    
    printf("      Multiple files: Combined %d triples\n", triple_count);
    TEST_ASSERT(triple_count > 5); // Should have triples from both files
    
    fclose(output);
    unlink("combined.nt");
    unlink(input2);
    
    return 1;
}

/**
 * Test query execution via CLI
 */
static int test_cli_query_execution() {
    printf("    Testing CLI query execution...\n");
    
    // Test simple query execution
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
            "./ttl-parser --query \"?s a foaf:Person\" --format jsonld %s > query_result.jsonld 2>/dev/null",
            test_input_file);
    
    int status = system(cmd);
    TEST_ASSERT(WEXITSTATUS(status) == 0);
    
    // Verify query results
    FILE* result = fopen("query_result.jsonld", "r");
    TEST_ASSERT_NOT_NULL(result);
    
    fseek(result, 0, SEEK_END);
    long size = ftell(result);
    TEST_ASSERT(size > 0);
    
    printf("      Query results: %ld bytes of JSON-LD\n", size);
    
    fclose(result);
    unlink("query_result.jsonld");
    
    // Test query from file
    snprintf(cmd, sizeof(cmd),
            "./ttl-parser --query-file %s --format ntriples %s > query_result.nt 2>/dev/null",
            test_query_file, test_input_file);
    
    status = system(cmd);
    TEST_ASSERT(WEXITSTATUS(status) == 0);
    
    result = fopen("query_result.nt", "r");
    TEST_ASSERT_NOT_NULL(result);
    
    fseek(result, 0, SEEK_END);
    size = ftell(result);
    TEST_ASSERT(size > 0);
    
    printf("      Query from file: %ld bytes of N-Triples\n", size);
    
    fclose(result);
    unlink("query_result.nt");
    
    return 1;
}

/**
 * Test performance reporting
 */
static int test_performance_reporting() {
    printf("    Testing performance reporting...\n");
    
    // Test with performance flag
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
            "./ttl-parser --performance --format ntriples %s > perf_output.nt 2>perf_report.txt",
            test_input_file);
    
    int status = system(cmd);
    TEST_ASSERT(WEXITSTATUS(status) == 0);
    
    // Check performance report
    FILE* perf_report = fopen("perf_report.txt", "r");
    TEST_ASSERT_NOT_NULL(perf_report);
    
    char report_content[4096];
    size_t report_size = fread(report_content, 1, sizeof(report_content) - 1, perf_report);
    report_content[report_size] = '\0';
    
    // Look for performance metrics
    TEST_ASSERT(strstr(report_content, "Performance") != NULL || 
                strstr(report_content, "time") != NULL ||
                strstr(report_content, "ms") != NULL);
    
    printf("      Performance report: Generated (%zu bytes)\n", report_size);
    
    fclose(perf_report);
    unlink("perf_output.nt");
    unlink("perf_report.txt");
    
    return 1;
}

/**
 * Test configuration file handling
 */
static int test_configuration_handling() {
    printf("    Testing configuration file handling...\n");
    
    // Test with configuration file
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
            "./ttl-parser --config %s --format ntriples %s > config_output.nt 2>/dev/null",
            test_config_file, test_input_file);
    
    int status = system(cmd);
    TEST_ASSERT(WEXITSTATUS(status) == 0);
    
    // Verify output was created
    FILE* output = fopen("config_output.nt", "r");
    TEST_ASSERT_NOT_NULL(output);
    
    fseek(output, 0, SEEK_END);
    long size = ftell(output);
    TEST_ASSERT(size > 0);
    
    printf("      Configuration file: Applied successfully (%ld bytes)\n", size);
    
    fclose(output);
    unlink("config_output.nt");
    
    return 1;
}

/**
 * Test error handling in CLI
 */
static int test_cli_error_handling() {
    printf("    Testing CLI error handling...\n");
    
    // Test with non-existent file
    int status = system("./ttl-parser non_existent_file.ttl 2>/dev/null");
    TEST_ASSERT(WEXITSTATUS(status) != 0);
    
    // Test with invalid format
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
            "./ttl-parser --format invalid_format %s 2>/dev/null",
            test_input_file);
    status = system(cmd);
    TEST_ASSERT(WEXITSTATUS(status) != 0);
    
    // Test with invalid query syntax
    snprintf(cmd, sizeof(cmd),
            "./ttl-parser --query \"invalid query syntax\" %s 2>/dev/null",
            test_input_file);
    status = system(cmd);
    TEST_ASSERT(WEXITSTATUS(status) != 0);
    
    printf("      Error conditions: Handled correctly\n");
    
    return 1;
}

/**
 * Test verbose and debug output
 */
static int test_verbose_debug_output() {
    printf("    Testing verbose and debug output...\n");
    
    // Test verbose mode
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
            "./ttl-parser --verbose --format ntriples %s > verbose_output.nt 2>verbose_log.txt",
            test_input_file);
    
    int status = system(cmd);
    TEST_ASSERT(WEXITSTATUS(status) == 0);
    
    // Check verbose log
    FILE* verbose_log = fopen("verbose_log.txt", "r");
    TEST_ASSERT_NOT_NULL(verbose_log);
    
    fseek(verbose_log, 0, SEEK_END);
    long log_size = ftell(verbose_log);
    
    printf("      Verbose mode: Generated %ld bytes of logging\n", log_size);
    
    fclose(verbose_log);
    unlink("verbose_output.nt");
    unlink("verbose_log.txt");
    
    // Test debug mode
    snprintf(cmd, sizeof(cmd),
            "./ttl-parser --debug --format ntriples %s > debug_output.nt 2>debug_log.txt",
            test_input_file);
    
    status = system(cmd);
    TEST_ASSERT(WEXITSTATUS(status) == 0);
    
    // Check debug log
    FILE* debug_log = fopen("debug_log.txt", "r");
    TEST_ASSERT_NOT_NULL(debug_log);
    
    fseek(debug_log, 0, SEEK_END);
    log_size = ftell(debug_log);
    
    printf("      Debug mode: Generated %ld bytes of debugging\n", log_size);
    
    fclose(debug_log);
    unlink("debug_output.nt");
    unlink("debug_log.txt");
    
    return 1;
}

/**
 * Test batch processing capabilities
 */
static int test_batch_processing() {
    printf("    Testing batch processing capabilities...\n");
    
    // Create multiple input files
    const char* batch_files[] = {
        "tests/fixtures/phase2/cli/batch1.ttl",
        "tests/fixtures/phase2/cli/batch2.ttl",
        "tests/fixtures/phase2/cli/batch3.ttl"
    };
    
    for (int i = 0; i < 3; i++) {
        FILE* batch_file = fopen(batch_files[i], "w");
        TEST_ASSERT_NOT_NULL(batch_file);
        
        fprintf(batch_file,
            "@prefix ex%d: <http://example%d.org/> .\n"
            "ex%d:item1 ex%d:value %d .\n"
            "ex%d:item2 ex%d:value %d .\n",
            i, i, i, i, i * 10, i, i, i * 20);
        
        fclose(batch_file);
    }
    
    // Test batch processing
    char cmd[1024];
    snprintf(cmd, sizeof(cmd),
            "./ttl-parser --batch --format ntriples --output-dir batch_output/ %s %s %s 2>/dev/null",
            batch_files[0], batch_files[1], batch_files[2]);
    
    // Create output directory
    system("mkdir -p batch_output");
    
    int status = system(cmd);
    TEST_ASSERT(WEXITSTATUS(status) == 0);
    
    // Verify batch outputs
    for (int i = 0; i < 3; i++) {
        char output_path[256];
        snprintf(output_path, sizeof(output_path), "batch_output/batch%d.nt", i + 1);
        
        FILE* output = fopen(output_path, "r");
        if (output) {
            fseek(output, 0, SEEK_END);
            long size = ftell(output);
            printf("      Batch file %d: %ld bytes\n", i + 1, size);
            fclose(output);
            unlink(output_path);
        }
        
        unlink(batch_files[i]);
    }
    
    system("rmdir batch_output");
    
    return 1;
}

/**
 * Clean up test fixtures
 */
static int cleanup_cli_test_fixtures() {
    printf("    Cleaning up CLI test fixtures...\n");
    
    unlink(test_input_file);
    unlink(test_query_file);
    unlink(test_config_file);
    
    printf("      Test fixtures cleaned up\n");
    return 1;
}

/**
 * Run all CLI enhanced tests
 */
static void run_cli_enhanced_tests() {
    RUN_TEST(setup_cli_test_fixtures);
    RUN_TEST(test_basic_cli_functionality);
    RUN_TEST(test_output_format_selection);
    RUN_TEST(test_multiple_input_files);
    RUN_TEST(test_cli_query_execution);
    RUN_TEST(test_performance_reporting);
    RUN_TEST(test_configuration_handling);
    RUN_TEST(test_cli_error_handling);
    RUN_TEST(test_verbose_debug_output);
    RUN_TEST(test_batch_processing);
    RUN_TEST(cleanup_cli_test_fixtures);
}

/**
 * Main test entry point
 */
int main(int argc, char* argv[]) {
    printf("🖥️  TTL Parser - Enhanced CLI Test Suite\n");
    printf("=======================================\n");
    
    // Create necessary directories
    system("mkdir -p tests/fixtures/phase2/cli");
    
    init_test_stats();
    
    RUN_TEST_SUITE("Enhanced CLI", run_cli_enhanced_tests);
    
    print_test_summary();
    check_memory_leaks();
    
    return g_test_stats.failed == 0 ? 0 : 1;
}