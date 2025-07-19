/*
 * SPARQL AOT Benchmark - 80/20 Implementation Test
 * Tests the performance of compiled SPARQL queries vs interpreter
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <stdbool.h>

// Include our AOT compiled queries and kernels
#include "sparql_queries.h"
#include "cns/engines/sparql.h"

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

// Benchmark configuration
#define BENCHMARK_ITERATIONS 10000
#define TEST_DATA_SIZE 1000

typedef struct {
    const char* name;
    const char* description;
    uint64_t aot_total_cycles;
    uint64_t interpreter_total_cycles;
    double aot_avg_cycles;
    double interpreter_avg_cycles;
    double speedup_ratio;
    bool seven_tick_compliant;
    int result_count;
} benchmark_result_t;

void setup_test_data(CNSSparqlEngine* engine) {
    printf("🔄 Setting up test data...\n");
    
    // Add customers
    for (int i = 1000; i < 1010; i++) {
        cns_sparql_add_triple(engine, i, RDF_TYPE, CUSTOMER_CLASS);
        cns_sparql_add_triple(engine, i, HAS_NAME, 5000 + i);
        cns_sparql_add_triple(engine, i, HAS_EMAIL, 6000 + i);
        cns_sparql_add_triple(engine, i, LIFETIME_VALUE, 7000 + i);
    }
    
    // Add persons
    for (int i = 2000; i < 2010; i++) {
        cns_sparql_add_triple(engine, i, RDF_TYPE, PERSON_CLASS);
        cns_sparql_add_triple(engine, i, FOAF_NAME, 8000 + i);
        cns_sparql_add_triple(engine, i, FOAF_EMAIL, 9000 + i);
        if (i % 2 == 0) {
            cns_sparql_add_triple(engine, i, FOAF_KNOWS, i + 1);
        }
    }
    
    // Add documents
    for (int i = 3000; i < 3010; i++) {
        cns_sparql_add_triple(engine, i, RDF_TYPE, DOCUMENT_CLASS);
        cns_sparql_add_triple(engine, i, DC_TITLE, 10000 + i);
        cns_sparql_add_triple(engine, i, DC_CREATOR, 2000 + (i % 10));
    }
    
    // Add organizations
    for (int i = 4000; i < 4005; i++) {
        cns_sparql_add_triple(engine, i, RDF_TYPE, ORGANIZATION_CLASS);
        for (int j = 2000; j < 2005; j++) {
            cns_sparql_add_triple(engine, j, MEMBER_OF, i);
        }
    }
    
    printf("✅ Test data setup complete\n");
}

// Interpreter-based query execution (for comparison)
int interpreter_query(CNSSparqlEngine* engine, const char* query_name) {
    // Simplified interpreter simulation
    int result_count = 0;
    
    if (strcmp(query_name, "getHighValueCustomers") == 0) {
        // Find customers with high lifetime value (simplified)
        for (int i = 1000; i < 1010; i++) {
            if (cns_sparql_ask_pattern(engine, i, RDF_TYPE, CUSTOMER_CLASS)) {
                result_count++;
            }
        }
    } else if (strcmp(query_name, "socialConnections") == 0) {
        // Find social connections
        for (int i = 2000; i < 2010; i++) {
            if (cns_sparql_ask_pattern(engine, i, RDF_TYPE, PERSON_CLASS)) {
                for (int j = 2000; j < 2010; j++) {
                    if (cns_sparql_ask_pattern(engine, i, FOAF_KNOWS, j)) {
                        result_count++;
                    }
                }
            }
        }
    } else {
        // Default: simple type scan
        for (int i = 1000; i < 5000; i++) {
            if (cns_sparql_ask_pattern(engine, i, RDF_TYPE, PERSON_CLASS)) {
                result_count++;
            }
        }
    }
    
    return result_count;
}

benchmark_result_t run_query_benchmark(CNSSparqlEngine* engine, const char* query_name) {
    benchmark_result_t result = {0};
    result.name = query_name;
    
    QueryResult aot_results[100];
    uint64_t aot_total = 0;
    uint64_t interpreter_total = 0;
    
    printf("🏃 Benchmarking query: %s\n", query_name);
    
    // Warm up
    for (int i = 0; i < 100; i++) {
        execute_compiled_sparql_query(query_name, engine, aot_results, 100);
        interpreter_query(engine, query_name);
    }
    
    // Benchmark AOT execution
    printf("  📊 AOT execution...\n");
    for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
        uint64_t start = get_cycles();
        int count = execute_compiled_sparql_query(query_name, engine, aot_results, 100);
        uint64_t end = get_cycles();
        
        aot_total += (end - start);
        if (i == 0) result.result_count = count;
    }
    
    // Benchmark interpreter execution
    printf("  📊 Interpreter execution...\n");
    for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
        uint64_t start = get_cycles();
        interpreter_query(engine, query_name);
        uint64_t end = get_cycles();
        
        interpreter_total += (end - start);
    }
    
    // Calculate results
    result.aot_total_cycles = aot_total;
    result.interpreter_total_cycles = interpreter_total;
    result.aot_avg_cycles = (double)aot_total / BENCHMARK_ITERATIONS;
    result.interpreter_avg_cycles = (double)interpreter_total / BENCHMARK_ITERATIONS;
    result.speedup_ratio = result.interpreter_avg_cycles / result.aot_avg_cycles;
    result.seven_tick_compliant = (result.aot_avg_cycles <= 7.0);
    
    return result;
}

void print_benchmark_results(benchmark_result_t* results, int count) {
    printf("\n📊 SPARQL AOT Benchmark Results\n");
    printf("================================\n\n");
    
    printf("%-25s %10s %10s %10s %8s %6s\n", 
           "Query", "AOT Cyc", "Interp Cyc", "Speedup", "Results", "7T");
    printf("%-25s %10s %10s %10s %8s %6s\n", 
           "-----", "-------", "----------", "-------", "-------", "--");
    
    double total_aot = 0, total_interpreter = 0;
    int compliant_count = 0;
    
    for (int i = 0; i < count; i++) {
        printf("%-25s %10.1f %10.1f %10.2fx %8d %6s\n",
               results[i].name,
               results[i].aot_avg_cycles,
               results[i].interpreter_avg_cycles,
               results[i].speedup_ratio,
               results[i].result_count,
               results[i].seven_tick_compliant ? "✅" : "❌");
        
        total_aot += results[i].aot_avg_cycles;
        total_interpreter += results[i].interpreter_avg_cycles;
        if (results[i].seven_tick_compliant) compliant_count++;
    }
    
    double avg_aot = total_aot / count;
    double avg_interpreter = total_interpreter / count;
    double avg_speedup = avg_interpreter / avg_aot;
    
    printf("\n📈 Summary:\n");
    printf("  Average AOT cycles: %.1f\n", avg_aot);
    printf("  Average interpreter cycles: %.1f\n", avg_interpreter);
    printf("  Average speedup: %.2fx\n", avg_speedup);
    printf("  7-tick compliant queries: %d/%d (%.1f%%)\n", 
           compliant_count, count, 100.0 * compliant_count / count);
    printf("  Overall performance: %s\n", 
           (compliant_count >= count/2) ? "✅ GOOD" : "⚠️ NEEDS OPTIMIZATION");
    
    // JSON output for automated analysis
    printf("\n🔍 JSON Results:\n");
    printf("{\n");
    printf("  \"benchmark_type\": \"sparql_aot\",\n");
    printf("  \"iterations\": %d,\n", BENCHMARK_ITERATIONS);
    printf("  \"queries_tested\": %d,\n", count);
    printf("  \"avg_aot_cycles\": %.1f,\n", avg_aot);
    printf("  \"avg_interpreter_cycles\": %.1f,\n", avg_interpreter);
    printf("  \"avg_speedup\": %.2f,\n", avg_speedup);
    printf("  \"seven_tick_compliant\": %d,\n", compliant_count);
    printf("  \"compliance_rate\": %.3f,\n", (double)compliant_count / count);
    printf("  \"queries\": [\n");
    
    for (int i = 0; i < count; i++) {
        printf("    {\n");
        printf("      \"name\": \"%s\",\n", results[i].name);
        printf("      \"aot_cycles\": %.1f,\n", results[i].aot_avg_cycles);
        printf("      \"interpreter_cycles\": %.1f,\n", results[i].interpreter_avg_cycles);
        printf("      \"speedup\": %.2f,\n", results[i].speedup_ratio);
        printf("      \"results\": %d,\n", results[i].result_count);
        printf("      \"seven_tick_compliant\": %s\n", 
               results[i].seven_tick_compliant ? "true" : "false");
        printf("    }%s\n", (i < count - 1) ? "," : "");
    }
    
    printf("  ]\n");
    printf("}\n");
}

int main(int argc, char** argv) {
    (void)argc; (void)argv;
    
    printf("🚀 SPARQL AOT Benchmark Suite\n");
    printf("==============================\n");
    printf("Testing 80/20 implementation of SPARQL AOT compilation\n\n");
    
    printf("Configuration:\n");
    printf("  Benchmark iterations: %d\n", BENCHMARK_ITERATIONS);
    printf("  Test data size: %d triples\n", TEST_DATA_SIZE);
    printf("  7-tick target: ≤ 7 cycles per query\n\n");
    
    // Create SPARQL engine
    CNSSparqlEngine* engine = cns_sparql_create(5000, 500, 5000);
    if (!engine) {
        printf("❌ Failed to create SPARQL engine\n");
        return 1;
    }
    
    // Setup test data
    setup_test_data(engine);
    
    // Test queries
    const char* test_queries[] = {
        "getHighValueCustomers",
        "findPersonsByName", 
        "getDocumentsByCreator",
        "socialConnections",
        "organizationMembers"
    };
    int query_count = sizeof(test_queries) / sizeof(test_queries[0]);
    
    benchmark_result_t results[10];
    
    // Run benchmarks
    for (int i = 0; i < query_count; i++) {
        results[i] = run_query_benchmark(engine, test_queries[i]);
    }
    
    // Print results
    print_benchmark_results(results, query_count);
    
    // Cleanup
    cns_sparql_destroy(engine);
    
    // Return success if at least half the queries are 7-tick compliant
    int compliant = 0;
    for (int i = 0; i < query_count; i++) {
        if (results[i].seven_tick_compliant) compliant++;
    }
    
    return (compliant >= query_count / 2) ? 0 : 1;
}