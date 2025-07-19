/*
 * CNS Production AOT Compiler Benchmark
 * Validates the production-ready AOT compiler implementation
 * Tests real performance with actual ontology files
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>

// Test case structure
typedef struct {
    const char* test_name;
    const char* ontology_dir;
    const char* sql_dir;
    const char* output_dir;
    int expected_rules;
    int expected_shapes;
    int expected_queries;
    double max_time_ms;
} aot_test_case_t;

// Performance metrics
typedef struct {
    double total_time_ms;
    int total_rules;
    int total_shapes;
    int total_queries;
    double rules_per_ms;
    int files_generated;
    long total_output_size;
} aot_performance_t;

// Test cases for production validation
static aot_test_case_t test_cases[] = {
    {
        "Production Full Ontology",
        "docs/ontology",
        ".",
        "build/generated/production",
        70,  // Expected minimum rules
        30,  // Expected minimum shapes
        1,   // Expected minimum queries
        200.0 // Max time in ms
    },
    {
        "Small Ontology Subset",
        "docs/ontology",
        "examples/sql",
        "build/generated/small",
        20,  // Expected minimum rules
        10,  // Expected minimum shapes
        1,   // Expected minimum queries
        50.0 // Max time in ms
    }
};

static int file_exists(const char* filename) {
    struct stat buffer;
    return (stat(filename, &buffer) == 0);
}

static long get_file_size(const char* filename) {
    struct stat buffer;
    if (stat(filename, &buffer) == 0) {
        return buffer.st_size;
    }
    return 0;
}

static int run_aot_compiler(const char* ontology_dir, const char* sql_dir, 
                           const char* output_dir, aot_performance_t* perf) {
    char cmd[1024];
    FILE *fp;
    char output[4096];
    struct timespec start, end;
    
    // Create output directory
    char mkdir_cmd[512];
    snprintf(mkdir_cmd, sizeof(mkdir_cmd), "mkdir -p %s", output_dir);
    system(mkdir_cmd);
    
    // Build command
    snprintf(cmd, sizeof(cmd), 
        "python3 codegen/aot_compiler_production.py --ontologies %s --sql %s --output %s 2>&1",
        ontology_dir, sql_dir, output_dir);
    
    // Measure execution time
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    fp = popen(cmd, "r");
    if (!fp) {
        return 0;
    }
    
    // Read output
    output[0] = '\0';  // Initialize buffer
    size_t total_read = 0;
    char line[512];
    while (fgets(line, sizeof(line), fp) != NULL && total_read < sizeof(output) - 512) {
        strcat(output + total_read, line);
        total_read += strlen(line);
    }
    
    int exit_code = pclose(fp);
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    // Calculate timing
    perf->total_time_ms = (end.tv_sec - start.tv_sec) * 1000.0 +
                         (end.tv_nsec - start.tv_nsec) / 1000000.0;
    
    // Parse output for metrics with correct format parsing
    char* rules_line = strstr(output, "ontology rules");
    if (rules_line) {
        // Find the number before "ontology rules"
        char* p = rules_line - 1;
        while (p > output && (*p == ' ' || *p == '\t')) p--;
        while (p > output && *p >= '0' && *p <= '9') p--;
        if (p < rules_line) {
            sscanf(p + 1, "%d", &perf->total_rules);
        }
    }
    
    char* shapes_line = strstr(output, "SHACL shapes");
    if (shapes_line) {
        // Find the number before "SHACL shapes"
        char* p = shapes_line - 1;
        while (p > output && (*p == ' ' || *p == '\t')) p--;
        while (p > output && *p >= '0' && *p <= '9') p--;
        if (p < shapes_line) {
            sscanf(p + 1, "%d", &perf->total_shapes);
        }
    }
    
    char* queries_line = strstr(output, "SQL queries");
    if (queries_line) {
        // Find the number before "SQL queries"
        char* p = queries_line - 1;
        while (p > output && (*p == ' ' || *p == '\t')) p--;
        while (p > output && *p >= '0' && *p <= '9') p--;
        if (p < queries_line) {
            sscanf(p + 1, "%d", &perf->total_queries);
        }
    }
    
    char* efficiency_line = strstr(output, "Efficiency: ");
    if (efficiency_line) {
        sscanf(efficiency_line, "Efficiency: %lf rules/ms", &perf->rules_per_ms);
    }
    
    // Count generated files and total size
    char generated_files[][32] = {
        "ontology_ids.h",
        "ontology_rules.h", 
        "shacl_validators.h",
        "sql_queries.h"
    };
    
    perf->files_generated = 0;
    perf->total_output_size = 0;
    
    for (int i = 0; i < 4; i++) {
        char filepath[512];
        snprintf(filepath, sizeof(filepath), "%s/%s", output_dir, generated_files[i]);
        if (file_exists(filepath)) {
            perf->files_generated++;
            perf->total_output_size += get_file_size(filepath);
        }
    }
    
    return (exit_code == 0);
}

static int validate_test_case(aot_test_case_t* test, aot_performance_t* perf) {
    int score = 0;
    int max_score = 7;
    
    printf("  📋 Validating results:\n");
    
    // Check compilation success
    if (perf->total_rules > 0 || perf->total_shapes > 0) {
        printf("    ✅ Compilation successful\n");
        score++;
    } else {
        printf("    ❌ Compilation failed\n");
        return score;
    }
    
    // Check performance timing
    if (perf->total_time_ms <= test->max_time_ms) {
        printf("    ✅ Performance: %.2fms (target: %.2fms)\n", 
               perf->total_time_ms, test->max_time_ms);
        score++;
    } else {
        printf("    ❌ Performance: %.2fms (target: %.2fms)\n", 
               perf->total_time_ms, test->max_time_ms);
    }
    
    // Check minimum rules generated
    if (perf->total_rules >= test->expected_rules) {
        printf("    ✅ Rules: %d (min: %d)\n", perf->total_rules, test->expected_rules);
        score++;
    } else {
        printf("    ❌ Rules: %d (min: %d)\n", perf->total_rules, test->expected_rules);
    }
    
    // Check minimum shapes generated
    if (perf->total_shapes >= test->expected_shapes) {
        printf("    ✅ Shapes: %d (min: %d)\n", perf->total_shapes, test->expected_shapes);
        score++;
    } else {
        printf("    ❌ Shapes: %d (min: %d)\n", perf->total_shapes, test->expected_shapes);
    }
    
    // Check minimum queries generated
    if (perf->total_queries >= test->expected_queries) {
        printf("    ✅ Queries: %d (min: %d)\n", perf->total_queries, test->expected_queries);
        score++;
    } else {
        printf("    ❌ Queries: %d (min: %d)\n", perf->total_queries, test->expected_queries);
    }
    
    // Check files generated
    if (perf->files_generated >= 3) {
        printf("    ✅ Files: %d generated (%.1fKB total)\n", 
               perf->files_generated, perf->total_output_size / 1024.0);
        score++;
    } else {
        printf("    ❌ Files: %d generated\n", perf->files_generated);
    }
    
    // Check efficiency
    if (perf->rules_per_ms >= 500.0) {
        printf("    ✅ Efficiency: %.1f rules/ms\n", perf->rules_per_ms);
        score++;
    } else {
        printf("    ❌ Efficiency: %.1f rules/ms (target: 500+)\n", perf->rules_per_ms);
    }
    
    return score;
}

int main() {
    printf("🚀 CNS Production AOT Compiler Benchmark\n");
    printf("=========================================\n\n");
    
    int total_tests = sizeof(test_cases) / sizeof(test_cases[0]);
    int passed_tests = 0;
    int total_score = 0;
    int max_total_score = 0;
    
    aot_performance_t overall_perf = {0};
    
    for (int i = 0; i < total_tests; i++) {
        aot_test_case_t* test = &test_cases[i];
        aot_performance_t perf = {0};
        
        printf("📝 Test %d: %s\n", i + 1, test->test_name);
        printf("  📂 Ontologies: %s\n", test->ontology_dir);
        printf("  📊 SQL: %s\n", test->sql_dir);
        printf("  📁 Output: %s\n", test->output_dir);
        
        printf("  ⚡ Running AOT compiler...\n");
        int success = run_aot_compiler(test->ontology_dir, test->sql_dir, 
                                     test->output_dir, &perf);
        
        if (success) {
            int score = validate_test_case(test, &perf);
            total_score += score;
            max_total_score += 7;
            
            if (score >= 5) { // 70% pass rate
                passed_tests++;
                printf("  ✅ PASS (%d/7 criteria met)\n", score);
            } else {
                printf("  ❌ FAIL (%d/7 criteria met)\n", score);
            }
            
            // Accumulate overall performance
            overall_perf.total_time_ms += perf.total_time_ms;
            overall_perf.total_rules += perf.total_rules;
            overall_perf.total_shapes += perf.total_shapes;
            overall_perf.total_queries += perf.total_queries;
            overall_perf.total_output_size += perf.total_output_size;
        } else {
            printf("  ❌ FAIL (compiler execution failed)\n");
            max_total_score += 7;
        }
        
        printf("\n");
    }
    
    // Calculate overall results
    double pass_rate = (double)passed_tests / total_tests * 100.0;
    double score_rate = (double)total_score / max_total_score * 100.0;
    
    printf("📊 PRODUCTION BENCHMARK RESULTS\n");
    printf("==============================\n");
    printf("Tests passed: %d/%d (%.1f%%)\n", passed_tests, total_tests, pass_rate);
    printf("Overall score: %d/%d (%.1f%%)\n", total_score, max_total_score, score_rate);
    printf("\n");
    
    printf("📈 AGGREGATE PERFORMANCE\n");
    printf("========================\n");
    printf("Total compilation time: %.2fms\n", overall_perf.total_time_ms);
    printf("Total rules generated: %d\n", overall_perf.total_rules);
    printf("Total shapes generated: %d\n", overall_perf.total_shapes);
    printf("Total queries generated: %d\n", overall_perf.total_queries);
    printf("Total output size: %.1fKB\n", overall_perf.total_output_size / 1024.0);
    
    if (overall_perf.total_time_ms > 0) {
        double avg_efficiency = overall_perf.total_rules / overall_perf.total_time_ms;
        printf("Average efficiency: %.1f rules/ms\n", avg_efficiency);
    }
    
    printf("\n");
    
    // Final assessment
    if (pass_rate >= 80.0 && score_rate >= 75.0) {
        printf("🎉 PRODUCTION VALIDATION: SUCCESS\n");
        printf("   The AOT compiler meets production standards!\n");
        return 0;
    } else if (pass_rate >= 60.0) {
        printf("⚠️  PRODUCTION VALIDATION: PARTIAL\n");
        printf("   The AOT compiler needs minor improvements.\n");
        return 1;
    } else {
        printf("❌ PRODUCTION VALIDATION: FAILED\n");
        printf("   The AOT compiler requires significant work.\n");
        return 2;
    }
}