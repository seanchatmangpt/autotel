/*
 * AOT Compiler 80/20 Benchmark Suite
 * 
 * Tests the AOT compiler performance with 80% common cases and 20% edge cases
 * Measures compilation speed, generated code performance, memory usage, and incremental builds
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>

// Benchmark categories
typedef enum {
    BENCH_COMPILATION_SPEED,
    BENCH_GENERATED_CODE_PERF,
    BENCH_MEMORY_USAGE,
    BENCH_INCREMENTAL_BUILD,
    BENCH_ERROR_HANDLING,
    BENCH_INTEGRATION
} BenchmarkCategory;

// Test case complexity levels
typedef enum {
    COMPLEXITY_SIMPLE,    // 80% - Common cases
    COMPLEXITY_MODERATE,  // 15% - Slightly complex
    COMPLEXITY_EDGE      // 5% - Edge cases
} ComplexityLevel;

// Benchmark result structure
typedef struct {
    const char* test_name;
    BenchmarkCategory category;
    ComplexityLevel complexity;
    double compilation_time_ms;
    double execution_time_us;
    size_t peak_memory_kb;
    size_t generated_code_size;
    bool passed;
    uint64_t cycles;
    const char* error_msg;
} BenchmarkResult;

// Test case definition
typedef struct {
    const char* name;
    const char* description;
    ComplexityLevel complexity;
    const char* input_file;
    const char* expected_output;
    size_t expected_cycles;
} TestCase;

// Global benchmark state
typedef struct {
    size_t total_tests;
    size_t passed_tests;
    size_t failed_tests;
    double total_compilation_time;
    double total_execution_time;
    size_t peak_memory_overall;
    FILE* report_file;
} BenchmarkState;

static BenchmarkState g_bench_state = {0};

// CPU cycle measurement
static inline uint64_t get_cycles(void) {
#if defined(__x86_64__) || defined(__i386__)
    uint32_t lo, hi;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
#elif defined(__aarch64__)
    uint64_t val;
    __asm__ __volatile__("mrs %0, cntvct_el0" : "=r" (val));
    return val;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 3000000000ULL + (uint64_t)ts.tv_nsec * 3;
#endif
}

// High-resolution timer
static double get_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
}

// Memory usage measurement
static size_t get_memory_usage_kb(void) {
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        return (size_t)usage.ru_maxrss; // KB on Linux, bytes on macOS
    }
    return 0;
}

// File size utility
static size_t get_file_size(const char* filename) {
    struct stat st;
    if (stat(filename, &st) == 0) {
        return (size_t)st.st_size;
    }
    return 0;
}

/*
 * 80/20 Test Cases Definition
 */

// Simple OWL ontology (80% common case)
static const char* SIMPLE_OWL = 
    "@prefix owl: <http://www.w3.org/2002/07/owl#> .\n"
    "@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> .\n"
    "@prefix ex: <http://example.org/> .\n"
    "ex:Person a owl:Class .\n"
    "ex:Employee rdfs:subClassOf ex:Person .\n"
    "ex:hasName a owl:DatatypeProperty ;\n"
    "    rdfs:domain ex:Person ;\n"
    "    rdfs:range xsd:string .\n";

// Complex OWL with restrictions (20% edge case)
static const char* COMPLEX_OWL = 
    "@prefix owl: <http://www.w3.org/2002/07/owl#> .\n"
    "@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> .\n"
    "@prefix ex: <http://example.org/> .\n"
    "ex:Person a owl:Class ;\n"
    "    owl:equivalentClass [\n"
    "        a owl:Restriction ;\n"
    "        owl:onProperty ex:hasAge ;\n"
    "        owl:minCardinality 1\n"
    "    ] .\n"
    "ex:Adult owl:intersectionOf (\n"
    "    ex:Person\n"
    "    [ a owl:Restriction ;\n"
    "      owl:onProperty ex:hasAge ;\n"
    "      owl:minInclusive 18 ]\n"
    ") .\n";

// Simple SHACL shapes (80% common case)
static const char* SIMPLE_SHACL = 
    "@prefix sh: <http://www.w3.org/ns/shacl#> .\n"
    "@prefix ex: <http://example.org/> .\n"
    "ex:PersonShape a sh:NodeShape ;\n"
    "    sh:targetClass ex:Person ;\n"
    "    sh:property [\n"
    "        sh:path ex:name ;\n"
    "        sh:minCount 1 ;\n"
    "        sh:datatype xsd:string\n"
    "    ] .\n";

// Complex SHACL with nested constraints (20% edge case)
static const char* COMPLEX_SHACL = 
    "@prefix sh: <http://www.w3.org/ns/shacl#> .\n"
    "@prefix ex: <http://example.org/> .\n"
    "ex:PersonShape a sh:NodeShape ;\n"
    "    sh:targetClass ex:Person ;\n"
    "    sh:property [\n"
    "        sh:path ex:address ;\n"
    "        sh:node ex:AddressShape\n"
    "    ] ;\n"
    "    sh:sparql [\n"
    "        sh:message \"Person must have unique SSN\" ;\n"
    "        sh:select \"\"\"\n"
    "            SELECT $this WHERE {\n"
    "                $this ex:ssn ?ssn .\n"
    "                ?other ex:ssn ?ssn .\n"
    "                FILTER (?other != $this)\n"
    "            }\n"
    "        \"\"\"\n"
    "    ] .\n";

// Simple SQL queries (80% common case)
static const char* SIMPLE_SQL[] = {
    "SELECT * FROM customers WHERE age > 18",
    "INSERT INTO orders (customer_id, total) VALUES (?, ?)",
    "UPDATE products SET price = price * 1.1 WHERE category = 'electronics'",
    "DELETE FROM sessions WHERE last_activity < ?",
    "SELECT COUNT(*) FROM orders WHERE status = 'pending'"
};

// Complex SQL queries (20% edge case)
static const char* COMPLEX_SQL[] = {
    "WITH RECURSIVE cte AS ("
    "  SELECT id, parent_id, name, 1 as level FROM categories WHERE parent_id IS NULL"
    "  UNION ALL"
    "  SELECT c.id, c.parent_id, c.name, cte.level + 1"
    "  FROM categories c JOIN cte ON c.parent_id = cte.id"
    ") SELECT * FROM cte WHERE level <= 3",
    
    "SELECT c.name, COUNT(o.id) as order_count,"
    "  SUM(o.total) as total_revenue,"
    "  AVG(o.total) as avg_order_value,"
    "  RANK() OVER (ORDER BY SUM(o.total) DESC) as revenue_rank"
    "FROM customers c"
    "LEFT JOIN orders o ON c.id = o.customer_id"
    "WHERE o.created_at >= DATE_SUB(NOW(), INTERVAL 1 YEAR)"
    "GROUP BY c.id, c.name"
    "HAVING COUNT(o.id) > 5"
};

/*
 * Test Case Definitions
 */
static TestCase g_test_cases[] = {
    // 80% Common Cases - Compilation Speed
    {"simple_owl_compile", "Compile simple OWL ontology", COMPLEXITY_SIMPLE, "simple.ttl", "ontology_ids.h", 100},
    {"simple_shacl_compile", "Compile basic SHACL shapes", COMPLEXITY_SIMPLE, "shapes.ttl", "shacl_validators.h", 100},
    {"simple_sql_compile", "Compile common SQL queries", COMPLEXITY_SIMPLE, "queries.sql", "sql_queries.h", 80},
    {"small_sparql_compile", "Compile small SPARQL queries", COMPLEXITY_SIMPLE, "sparql.rq", "sparql_queries.h", 90},
    
    // 80% Common Cases - Generated Code Performance
    {"simple_owl_exec", "Execute simple OWL inference", COMPLEXITY_SIMPLE, NULL, NULL, 49},
    {"simple_shacl_exec", "Execute basic SHACL validation", COMPLEXITY_SIMPLE, NULL, NULL, 49},
    {"simple_sql_exec", "Execute common SQL query", COMPLEXITY_SIMPLE, NULL, NULL, 30},
    {"small_sparql_exec", "Execute small SPARQL query", COMPLEXITY_SIMPLE, NULL, NULL, 40},
    
    // 15% Moderate Cases
    {"medium_owl_compile", "Compile OWL with imports", COMPLEXITY_MODERATE, "medium.ttl", "ontology_ids.h", 200},
    {"medium_shacl_compile", "Compile SHACL with dependencies", COMPLEXITY_MODERATE, "medium_shapes.ttl", "shacl_validators.h", 180},
    {"join_sql_compile", "Compile SQL with joins", COMPLEXITY_MODERATE, "joins.sql", "sql_queries.h", 150},
    
    // 5% Edge Cases
    {"complex_owl_compile", "Compile complex OWL with restrictions", COMPLEXITY_EDGE, "complex.ttl", "ontology_ids.h", 500},
    {"complex_shacl_compile", "Compile nested SHACL constraints", COMPLEXITY_EDGE, "complex_shapes.ttl", "shacl_validators.h", 400},
    {"recursive_sql_compile", "Compile recursive SQL CTEs", COMPLEXITY_EDGE, "recursive.sql", "sql_queries.h", 300},
    
    // Memory Usage Tests
    {"large_dataset_memory", "Memory usage with 10K triples", COMPLEXITY_MODERATE, "large.ttl", NULL, 1000},
    {"incremental_memory", "Memory usage during incremental builds", COMPLEXITY_SIMPLE, NULL, NULL, 200},
    
    // Incremental Build Tests
    {"incremental_add_class", "Add single class to ontology", COMPLEXITY_SIMPLE, NULL, NULL, 50},
    {"incremental_modify_shape", "Modify existing SHACL shape", COMPLEXITY_SIMPLE, NULL, NULL, 60},
    {"incremental_add_query", "Add new SQL query", COMPLEXITY_SIMPLE, NULL, NULL, 40},
    
    // Error Handling Tests
    {"malformed_ttl", "Handle malformed Turtle syntax", COMPLEXITY_EDGE, "malformed.ttl", NULL, 100},
    {"circular_dependency", "Detect circular dependencies", COMPLEXITY_EDGE, "circular.ttl", NULL, 150},
    {"invalid_sql", "Handle invalid SQL syntax", COMPLEXITY_SIMPLE, "invalid.sql", NULL, 50}
};

#define NUM_TEST_CASES (sizeof(g_test_cases) / sizeof(g_test_cases[0]))

/*
 * Benchmark Execution Functions
 */

// Create test input files
static void create_test_file(const char* filename, const char* content) {
    FILE* f = fopen(filename, "w");
    if (f) {
        fprintf(f, "%s", content);
        fclose(f);
    }
}

// Run AOT compiler
static BenchmarkResult run_compilation_benchmark(const TestCase* test) {
    BenchmarkResult result = {
        .test_name = test->name,
        .category = BENCH_COMPILATION_SPEED,
        .complexity = test->complexity,
        .passed = false
    };
    
    // Create test input file
    const char* content = NULL;
    if (strstr(test->name, "simple_owl")) {
        content = SIMPLE_OWL;
    } else if (strstr(test->name, "complex_owl")) {
        content = COMPLEX_OWL;
    } else if (strstr(test->name, "simple_shacl")) {
        content = SIMPLE_SHACL;
    } else if (strstr(test->name, "complex_shacl")) {
        content = COMPLEX_SHACL;
    } else if (strstr(test->name, "simple_sql")) {
        content = SIMPLE_SQL[0];
    } else if (strstr(test->name, "recursive_sql")) {
        content = COMPLEX_SQL[0];
    }
    
    if (content && test->input_file) {
        create_test_file(test->input_file, content);
    }
    
    // Measure compilation
    size_t mem_before = get_memory_usage_kb();
    double start_time = get_time_ms();
    uint64_t start_cycles = get_cycles();
    
    // Run the AOT compiler
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "python3 codegen/aot_compiler.py -i %s -o %s 2>/dev/null",
             test->input_file, test->expected_output);
    
    int ret = system(cmd);
    
    uint64_t end_cycles = get_cycles();
    double end_time = get_time_ms();
    size_t mem_after = get_memory_usage_kb();
    
    result.compilation_time_ms = end_time - start_time;
    result.cycles = end_cycles - start_cycles;
    result.peak_memory_kb = mem_after - mem_before;
    
    if (test->expected_output) {
        result.generated_code_size = get_file_size(test->expected_output);
    }
    
    result.passed = (ret == 0);
    if (!result.passed) {
        result.error_msg = "Compilation failed";
    }
    
    // Cleanup
    if (test->input_file) {
        unlink(test->input_file);
    }
    
    return result;
}

// Run generated code performance test
static BenchmarkResult run_execution_benchmark(const TestCase* test) {
    BenchmarkResult result = {
        .test_name = test->name,
        .category = BENCH_GENERATED_CODE_PERF,
        .complexity = test->complexity,
        .passed = false
    };
    
    // For execution benchmarks, we need to compile and link a test program
    // This would use the generated headers and measure runtime performance
    
    double start_time = get_time_ms();
    uint64_t start_cycles = get_cycles();
    
    // Simulate execution (in real implementation, this would run actual generated code)
    usleep(100); // 100us simulation
    
    uint64_t end_cycles = get_cycles();
    double end_time = get_time_ms();
    
    result.execution_time_us = (end_time - start_time) * 1000.0;
    result.cycles = end_cycles - start_cycles;
    result.passed = (result.cycles <= test->expected_cycles);
    
    return result;
}

// Run memory usage benchmark
static BenchmarkResult run_memory_benchmark(const TestCase* test) {
    BenchmarkResult result = {
        .test_name = test->name,
        .category = BENCH_MEMORY_USAGE,
        .complexity = test->complexity,
        .passed = false
    };
    
    // Create large test dataset
    if (strstr(test->name, "large_dataset")) {
        FILE* f = fopen("large.ttl", "w");
        if (f) {
            fprintf(f, "@prefix ex: <http://example.org/> .\n");
            for (int i = 0; i < 10000; i++) {
                fprintf(f, "ex:entity%d a ex:Class%d .\n", i, i % 100);
            }
            fclose(f);
        }
    }
    
    size_t mem_before = get_memory_usage_kb();
    double start_time = get_time_ms();
    
    // Run compilation with memory tracking
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "python3 codegen/aot_compiler.py -i large.ttl -o large_out.h 2>/dev/null");
    system(cmd);
    
    double end_time = get_time_ms();
    size_t mem_after = get_memory_usage_kb();
    
    result.compilation_time_ms = end_time - start_time;
    result.peak_memory_kb = mem_after - mem_before;
    result.passed = (result.peak_memory_kb < 50000); // Less than 50MB
    
    // Cleanup
    unlink("large.ttl");
    unlink("large_out.h");
    
    return result;
}

// Run incremental build benchmark
static BenchmarkResult run_incremental_benchmark(const TestCase* test) {
    BenchmarkResult result = {
        .test_name = test->name,
        .category = BENCH_INCREMENTAL_BUILD,
        .complexity = test->complexity,
        .passed = false
    };
    
    // First, create initial file
    create_test_file("base.ttl", SIMPLE_OWL);
    system("python3 codegen/aot_compiler.py -i base.ttl -o base.h 2>/dev/null");
    
    // Modify the file
    FILE* f = fopen("base.ttl", "a");
    if (f) {
        fprintf(f, "\nex:NewClass a owl:Class .\n");
        fclose(f);
    }
    
    // Measure incremental compilation
    double start_time = get_time_ms();
    uint64_t start_cycles = get_cycles();
    
    system("python3 codegen/aot_compiler.py -i base.ttl -o base.h --incremental 2>/dev/null");
    
    uint64_t end_cycles = get_cycles();
    double end_time = get_time_ms();
    
    result.compilation_time_ms = end_time - start_time;
    result.cycles = end_cycles - start_cycles;
    result.passed = (result.cycles <= test->expected_cycles);
    
    // Cleanup
    unlink("base.ttl");
    unlink("base.h");
    
    return result;
}

// Run error handling benchmark
static BenchmarkResult run_error_benchmark(const TestCase* test) {
    BenchmarkResult result = {
        .test_name = test->name,
        .category = BENCH_ERROR_HANDLING,
        .complexity = test->complexity,
        .passed = false
    };
    
    const char* error_content = NULL;
    
    if (strstr(test->name, "malformed_ttl")) {
        error_content = "@prefix ex: <http://example.org/> .\nex:Bad [ [ nested without closing";
    } else if (strstr(test->name, "circular")) {
        error_content = "@prefix ex: <http://example.org/> .\n"
                       "ex:A rdfs:subClassOf ex:B .\n"
                       "ex:B rdfs:subClassOf ex:C .\n"
                       "ex:C rdfs:subClassOf ex:A .\n";
    } else if (strstr(test->name, "invalid_sql")) {
        error_content = "SELECT * FORM customers WHERE"; // Typo: FORM instead of FROM
    }
    
    if (error_content) {
        create_test_file(test->input_file, error_content);
    }
    
    double start_time = get_time_ms();
    
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "python3 codegen/aot_compiler.py -i %s -o error_out.h 2>/dev/null",
             test->input_file);
    int ret = system(cmd);
    
    double end_time = get_time_ms();
    
    result.compilation_time_ms = end_time - start_time;
    result.passed = (ret != 0); // Should fail gracefully
    
    // Cleanup
    unlink(test->input_file);
    unlink("error_out.h");
    
    return result;
}

/*
 * Reporting Functions
 */

static void print_result(const BenchmarkResult* result) {
    const char* complexity_str[] = {"SIMPLE", "MODERATE", "EDGE"};
    const char* status = result->passed ? "PASS" : "FAIL";
    
    printf("%-30s [%s] %s - %.2fms compile, %lluKB mem, %llu cycles\n",
           result->test_name,
           complexity_str[result->complexity],
           status,
           result->compilation_time_ms,
           (unsigned long long)result->peak_memory_kb,
           (unsigned long long)result->cycles);
    
    if (!result->passed && result->error_msg) {
        printf("  ERROR: %s\n", result->error_msg);
    }
}

static void generate_report(void) {
    if (!g_bench_state.report_file) {
        g_bench_state.report_file = fopen("aot_benchmark_report.md", "w");
    }
    
    FILE* f = g_bench_state.report_file;
    
    fprintf(f, "# AOT Compiler 80/20 Benchmark Report\n\n");
    fprintf(f, "## Summary\n\n");
    fprintf(f, "- Total Tests: %zu\n", g_bench_state.total_tests);
    fprintf(f, "- Passed: %zu (%.1f%%)\n", g_bench_state.passed_tests,
            (double)g_bench_state.passed_tests / g_bench_state.total_tests * 100.0);
    fprintf(f, "- Failed: %zu\n", g_bench_state.failed_tests);
    fprintf(f, "- Average Compilation Time: %.2fms\n",
            g_bench_state.total_compilation_time / g_bench_state.total_tests);
    fprintf(f, "- Peak Memory Usage: %zuKB\n", g_bench_state.peak_memory_overall);
    
    fprintf(f, "\n## 80/20 Distribution\n\n");
    fprintf(f, "- 80%% Common Cases: Simple ontologies, basic shapes, standard queries\n");
    fprintf(f, "- 15%% Moderate Cases: Dependencies, joins, imports\n");
    fprintf(f, "- 5%% Edge Cases: Complex restrictions, recursive queries, error handling\n");
    
    fprintf(f, "\n## Performance Targets\n\n");
    fprintf(f, "- Simple Compilation: < 100ms\n");
    fprintf(f, "- Generated Code Execution: < 49 cycles\n");
    fprintf(f, "- Memory Usage: < 50MB for 10K triples\n");
    fprintf(f, "- Incremental Builds: < 50%% of full compilation time\n");
    
    fclose(g_bench_state.report_file);
}

/*
 * Main Benchmark Runner
 */
int main(int argc, char* argv[]) {
    printf("=== AOT Compiler 80/20 Benchmark Suite ===\n");
    printf("Testing %zu scenarios across compilation, execution, memory, and builds\n\n",
           NUM_TEST_CASES);
    
    // Initialize benchmark state
    g_bench_state.total_tests = NUM_TEST_CASES;
    
    // Run all benchmarks
    for (size_t i = 0; i < NUM_TEST_CASES; i++) {
        const TestCase* test = &g_test_cases[i];
        BenchmarkResult result = {0};
        
        // Select appropriate benchmark function
        if (strstr(test->name, "_compile")) {
            result = run_compilation_benchmark(test);
        } else if (strstr(test->name, "_exec")) {
            result = run_execution_benchmark(test);
        } else if (strstr(test->name, "_memory")) {
            result = run_memory_benchmark(test);
        } else if (strstr(test->name, "incremental_")) {
            result = run_incremental_benchmark(test);
        } else if (strstr(test->name, "malformed") || 
                   strstr(test->name, "circular") || 
                   strstr(test->name, "invalid")) {
            result = run_error_benchmark(test);
        }
        
        // Update statistics
        if (result.passed) {
            g_bench_state.passed_tests++;
        } else {
            g_bench_state.failed_tests++;
        }
        
        g_bench_state.total_compilation_time += result.compilation_time_ms;
        g_bench_state.total_execution_time += result.execution_time_us;
        
        if (result.peak_memory_kb > g_bench_state.peak_memory_overall) {
            g_bench_state.peak_memory_overall = result.peak_memory_kb;
        }
        
        // Print result
        print_result(&result);
    }
    
    // Generate final report
    printf("\n=== Generating Report ===\n");
    generate_report();
    printf("Report written to: aot_benchmark_report.md\n");
    
    // Summary
    printf("\n=== Summary ===\n");
    printf("Total: %zu, Passed: %zu (%.1f%%), Failed: %zu\n",
           g_bench_state.total_tests,
           g_bench_state.passed_tests,
           (double)g_bench_state.passed_tests / g_bench_state.total_tests * 100.0,
           g_bench_state.failed_tests);
    
    return (g_bench_state.failed_tests == 0) ? 0 : 1;
}