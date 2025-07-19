/*
 * SPARQL 80/20 Benchmark - Simplified Implementation
 * Tests the core 80/20 SPARQL patterns without complex AOT compilation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <stdbool.h>

// Simplified SPARQL engine for benchmarking
typedef struct {
    uint32_t* subjects;
    uint32_t* predicates; 
    uint32_t* objects;
    size_t triple_count;
    size_t max_triples;
} SimpleSparqlEngine;

// Basic vocabulary IDs (80/20 subset)
#define RDF_TYPE 1
#define FOAF_NAME 2
#define FOAF_KNOWS 3
#define DC_CREATOR 4
#define DC_TITLE 5

#define PERSON_CLASS 100
#define DOCUMENT_CLASS 101
#define CUSTOMER_CLASS 102

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

SimpleSparqlEngine* create_engine(size_t max_triples) {
    SimpleSparqlEngine* engine = malloc(sizeof(SimpleSparqlEngine));
    if (!engine) return NULL;
    
    engine->subjects = malloc(max_triples * sizeof(uint32_t));
    engine->predicates = malloc(max_triples * sizeof(uint32_t));
    engine->objects = malloc(max_triples * sizeof(uint32_t));
    engine->triple_count = 0;
    engine->max_triples = max_triples;
    
    if (!engine->subjects || !engine->predicates || !engine->objects) {
        free(engine->subjects);
        free(engine->predicates);
        free(engine->objects);
        free(engine);
        return NULL;
    }
    
    return engine;
}

void destroy_engine(SimpleSparqlEngine* engine) {
    if (engine) {
        free(engine->subjects);
        free(engine->predicates);
        free(engine->objects);
        free(engine);
    }
}

void add_triple(SimpleSparqlEngine* engine, uint32_t s, uint32_t p, uint32_t o) {
    if (engine->triple_count < engine->max_triples) {
        engine->subjects[engine->triple_count] = s;
        engine->predicates[engine->triple_count] = p;
        engine->objects[engine->triple_count] = o;
        engine->triple_count++;
    }
}

// 80/20 Query implementations - optimized for 7-tick performance
// These represent the core patterns that matter most

// Pattern 1: Type queries (?s rdf:type Class) - Most common SPARQL pattern
int query_by_type_optimized(SimpleSparqlEngine* engine, uint32_t type_class, uint32_t* results, int max_results) {
    int count = 0;
    // Optimized scan with early termination
    for (size_t i = 0; i < engine->triple_count && count < max_results; i++) {
        if (engine->predicates[i] == RDF_TYPE && engine->objects[i] == type_class) {
            results[count++] = engine->subjects[i];
        }
    }
    return count;
}

// Pattern 2: Subject-predicate queries (s ?p ?o) - Second most common
int query_by_subject_predicate(SimpleSparqlEngine* engine, uint32_t subject, uint32_t predicate, uint32_t* results, int max_results) {
    int count = 0;
    for (size_t i = 0; i < engine->triple_count && count < max_results; i++) {
        if (engine->subjects[i] == subject && engine->predicates[i] == predicate) {
            results[count++] = engine->objects[i];
        }
    }
    return count;
}

// Pattern 3: Simple joins - Join two patterns on common variable
int query_simple_join(SimpleSparqlEngine* engine, uint32_t pred1, uint32_t pred2, uint32_t* results, int max_results) {
    int count = 0;
    // Find subjects that have both predicates
    for (size_t i = 0; i < engine->triple_count && count < max_results; i++) {
        if (engine->predicates[i] == pred1) {
            uint32_t subject = engine->subjects[i];
            // Look for same subject with second predicate
            for (size_t j = 0; j < engine->triple_count; j++) {
                if (engine->subjects[j] == subject && engine->predicates[j] == pred2) {
                    results[count++] = subject;
                    break;
                }
            }
        }
    }
    return count;
}

// Pattern 4: Transitive relationships (person knows person)
int query_social_connections(SimpleSparqlEngine* engine, uint32_t* results, int max_results) {
    int count = 0;
    for (size_t i = 0; i < engine->triple_count && count < max_results; i++) {
        if (engine->predicates[i] == FOAF_KNOWS) {
            // Verify both subject and object are persons
            bool subject_is_person = false, object_is_person = false;
            
            for (size_t j = 0; j < engine->triple_count; j++) {
                if (engine->subjects[j] == engine->subjects[i] && 
                    engine->predicates[j] == RDF_TYPE && 
                    engine->objects[j] == PERSON_CLASS) {
                    subject_is_person = true;
                }
                if (engine->subjects[j] == engine->objects[i] && 
                    engine->predicates[j] == RDF_TYPE && 
                    engine->objects[j] == PERSON_CLASS) {
                    object_is_person = true;
                }
                if (subject_is_person && object_is_person) break;
            }
            
            if (subject_is_person && object_is_person) {
                results[count++] = engine->subjects[i];
            }
        }
    }
    return count;
}

typedef struct {
    const char* name;
    uint64_t total_cycles;
    double avg_cycles;
    int result_count;
    bool seven_tick_compliant;
} benchmark_result_t;

void setup_test_data(SimpleSparqlEngine* engine) {
    printf("🔄 Setting up test data...\n");
    
    // Add persons (most common entity type)
    for (int i = 1000; i < 1100; i++) {
        add_triple(engine, i, RDF_TYPE, PERSON_CLASS);
        add_triple(engine, i, FOAF_NAME, 5000 + i);
        
        // Add some social connections
        if (i % 3 == 0 && i < 1095) {
            add_triple(engine, i, FOAF_KNOWS, i + 1);
        }
    }
    
    // Add documents
    for (int i = 2000; i < 2050; i++) {
        add_triple(engine, i, RDF_TYPE, DOCUMENT_CLASS);
        add_triple(engine, i, DC_TITLE, 6000 + i);
        add_triple(engine, i, DC_CREATOR, 1000 + (i % 50)); // Link to persons
    }
    
    // Add customers (subset of persons)
    for (int i = 1000; i < 1020; i++) {
        add_triple(engine, i, RDF_TYPE, CUSTOMER_CLASS);
    }
    
    printf("✅ Test data: %zu triples loaded\n", engine->triple_count);
}

benchmark_result_t run_benchmark(const char* name, SimpleSparqlEngine* engine, 
                                 int (*query_func)(SimpleSparqlEngine*, uint32_t*, int)) {
    const int iterations = 10000;
    uint32_t results[1000];
    uint64_t total_cycles = 0;
    int result_count = 0;
    
    printf("🏃 Benchmarking: %s\n", name);
    
    // Warm up
    for (int i = 0; i < 1000; i++) {
        query_func(engine, results, 1000);
    }
    
    // Actual benchmark
    for (int i = 0; i < iterations; i++) {
        uint64_t start = get_cycles();
        int count = query_func(engine, results, 1000);
        uint64_t end = get_cycles();
        
        total_cycles += (end - start);
        if (i == 0) result_count = count;
    }
    
    benchmark_result_t result = {
        .name = name,
        .total_cycles = total_cycles,
        .avg_cycles = (double)total_cycles / iterations,
        .result_count = result_count,
        .seven_tick_compliant = ((double)total_cycles / iterations) <= 7.0
    };
    
    return result;
}

// Wrapper functions for consistent interface
int wrapper_query_persons(SimpleSparqlEngine* engine, uint32_t* results, int max_results) {
    return query_by_type_optimized(engine, PERSON_CLASS, results, max_results);
}

int wrapper_query_documents(SimpleSparqlEngine* engine, uint32_t* results, int max_results) {
    return query_by_type_optimized(engine, DOCUMENT_CLASS, results, max_results);
}

int wrapper_query_customers(SimpleSparqlEngine* engine, uint32_t* results, int max_results) {
    return query_by_type_optimized(engine, CUSTOMER_CLASS, results, max_results);
}

int wrapper_query_joins(SimpleSparqlEngine* engine, uint32_t* results, int max_results) {
    return query_simple_join(engine, RDF_TYPE, FOAF_NAME, results, max_results);
}

int wrapper_social(SimpleSparqlEngine* engine, uint32_t* results, int max_results) {
    return query_social_connections(engine, results, max_results);
}

void print_results(benchmark_result_t* results, int count) {
    printf("\n📊 SPARQL 80/20 Benchmark Results\n");
    printf("==================================\n\n");
    
    printf("%-25s %10s %8s %6s %s\n", "Query Pattern", "Avg Cycles", "Results", "7T", "Status");
    printf("%-25s %10s %8s %6s %s\n", "-------------", "----------", "-------", "--", "------");
    
    int compliant_count = 0;
    double total_cycles = 0;
    
    for (int i = 0; i < count; i++) {
        printf("%-25s %10.2f %8d %6s %s\n",
               results[i].name,
               results[i].avg_cycles,
               results[i].result_count,
               results[i].seven_tick_compliant ? "✅" : "❌",
               results[i].seven_tick_compliant ? "PASS" : "FAIL");
        
        total_cycles += results[i].avg_cycles;
        if (results[i].seven_tick_compliant) compliant_count++;
    }
    
    double avg_cycles = total_cycles / count;
    
    printf("\n📈 Summary:\n");
    printf("  Patterns tested: %d\n", count);
    printf("  Average cycles: %.2f\n", avg_cycles);
    printf("  7-tick compliant: %d/%d (%.1f%%)\n", 
           compliant_count, count, 100.0 * compliant_count / count);
    printf("  Overall 80/20 performance: %s\n", 
           (compliant_count >= count * 0.8) ? "✅ EXCELLENT" :
           (compliant_count >= count * 0.6) ? "⚠️ GOOD" : "❌ NEEDS WORK");
    
    // JSON output
    printf("\n🔍 JSON Results:\n");
    printf("{\n");
    printf("  \"benchmark_type\": \"sparql_80_20\",\n");
    printf("  \"patterns_tested\": %d,\n", count);
    printf("  \"avg_cycles\": %.2f,\n", avg_cycles);
    printf("  \"seven_tick_compliant\": %d,\n", compliant_count);
    printf("  \"compliance_rate\": %.3f,\n", (double)compliant_count / count);
    printf("  \"patterns\": [\n");
    
    for (int i = 0; i < count; i++) {
        printf("    {\n");
        printf("      \"name\": \"%s\",\n", results[i].name);
        printf("      \"avg_cycles\": %.2f,\n", results[i].avg_cycles);
        printf("      \"results\": %d,\n", results[i].result_count);
        printf("      \"seven_tick_compliant\": %s\n", 
               results[i].seven_tick_compliant ? "true" : "false");
        printf("    }%s\n", (i < count - 1) ? "," : "");
    }
    
    printf("  ]\n");
    printf("}\n");
}

int main() {
    printf("🚀 SPARQL 80/20 Performance Benchmark\n");
    printf("======================================\n");
    printf("Testing the 20%% of SPARQL patterns that handle 80%% of queries\n\n");
    
    // Create engine
    SimpleSparqlEngine* engine = create_engine(10000);
    if (!engine) {
        printf("❌ Failed to create SPARQL engine\n");
        return 1;
    }
    
    // Setup test data
    setup_test_data(engine);
    
    // Run benchmarks for core 80/20 patterns
    benchmark_result_t results[10];
    int result_count = 0;
    
    results[result_count++] = run_benchmark("Type Query (Person)", engine, wrapper_query_persons);
    results[result_count++] = run_benchmark("Type Query (Document)", engine, wrapper_query_documents);
    results[result_count++] = run_benchmark("Type Query (Customer)", engine, wrapper_query_customers);
    results[result_count++] = run_benchmark("Simple Join Pattern", engine, wrapper_query_joins);
    results[result_count++] = run_benchmark("Social Connections", engine, wrapper_social);
    
    // Print results
    print_results(results, result_count);
    
    // Cleanup
    destroy_engine(engine);
    
    // Return success if most patterns are 7-tick compliant
    int compliant = 0;
    for (int i = 0; i < result_count; i++) {
        if (results[i].seven_tick_compliant) compliant++;
    }
    
    return (compliant >= result_count * 0.6) ? 0 : 1;
}