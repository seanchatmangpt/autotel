/*
 * SPARQL Corrected Benchmark - Fixed Implementation
 * Tests realistic SPARQL performance with proper measurement
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <stdbool.h>

// Prevent compiler optimizations
#define FORCE_EVAL(x) __asm__ __volatile__("" : : "g"(x) : "memory")

// SPARQL engine
typedef struct {
    uint32_t* subjects;
    uint32_t* predicates; 
    uint32_t* objects;
    size_t triple_count;
    size_t max_triples;
} SparqlEngine;

// Vocabulary
#define RDF_TYPE 1
#define FOAF_NAME 2
#define FOAF_KNOWS 3
#define DC_CREATOR 4
#define DC_TITLE 5
#define PERSON_CLASS 100
#define DOCUMENT_CLASS 101
#define CUSTOMER_CLASS 102

// Cross-platform cycle measurement
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
    return (uint64_t)ts.tv_sec * 2400000000ULL + (uint64_t)ts.tv_nsec * 2.4;
#endif
}

SparqlEngine* create_engine(size_t max_triples) {
    SparqlEngine* engine = malloc(sizeof(SparqlEngine));
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

void destroy_engine(SparqlEngine* engine) {
    if (engine) {
        free(engine->subjects);
        free(engine->predicates);
        free(engine->objects);
        free(engine);
    }
}

void add_triple(SparqlEngine* engine, uint32_t s, uint32_t p, uint32_t o) {
    if (engine->triple_count < engine->max_triples) {
        size_t idx = engine->triple_count;
        engine->subjects[idx] = s;
        engine->predicates[idx] = p;
        engine->objects[idx] = o;
        engine->triple_count++;
    }
}

// Realistic SPARQL query implementations with proper complexity
int sparql_type_query(SparqlEngine* engine, uint32_t type_class, uint32_t* results, int max_results) {
    int count = 0;
    volatile uint32_t work = 0; // Force actual work
    
    // Linear scan with realistic overhead
    for (size_t i = 0; i < engine->triple_count && count < max_results; i++) {
        uint32_t pred = engine->predicates[i];
        uint32_t obj = engine->objects[i];
        
        // Additional work to simulate real SPARQL engine overhead
        work += pred ^ obj; // Hash computation
        
        if (pred == RDF_TYPE && obj == type_class) {
            results[count] = engine->subjects[i];
            work += results[count]; // Prevent optimization
            count++;
        }
        
        // Simulate string/URI processing overhead every few iterations
        if (i % 8 == 0) {
            work += (engine->subjects[i] * 31) + (pred * 17) + (obj * 7);
        }
    }
    
    FORCE_EVAL(work);
    return count;
}

int sparql_predicate_scan(SparqlEngine* engine, uint32_t predicate, uint32_t* results, int max_results) {
    int count = 0;
    volatile uint32_t work = 0;
    
    for (size_t i = 0; i < engine->triple_count && count < max_results; i++) {
        uint32_t pred = engine->predicates[i];
        
        // Simulate predicate index lookup overhead
        work += pred * 13;
        
        if (pred == predicate) {
            results[count * 2] = engine->subjects[i];     // subject
            results[count * 2 + 1] = engine->objects[i];  // object
            work += results[count * 2] + results[count * 2 + 1];
            count++;
        }
        
        // Additional overhead
        if (i % 4 == 0) {
            work += engine->subjects[i] + engine->objects[i];
        }
    }
    
    FORCE_EVAL(work);
    return count;
}

int sparql_join_query(SparqlEngine* engine, uint32_t pred1, uint32_t pred2, uint32_t* results, int max_results) {
    int count = 0;
    volatile uint32_t work = 0;
    
    // Nested loop join - realistic O(n²) complexity
    for (size_t i = 0; i < engine->triple_count && count < max_results; i++) {
        if (engine->predicates[i] == pred1) {
            uint32_t subject = engine->subjects[i];
            work += subject * 23;
            
            // Find matching subject with second predicate
            for (size_t j = 0; j < engine->triple_count; j++) {
                work += engine->predicates[j]; // Prevent optimization
                
                if (engine->subjects[j] == subject && engine->predicates[j] == pred2) {
                    results[count] = subject;
                    work += subject;
                    count++;
                    break; // Found match, move to next
                }
            }
        }
    }
    
    FORCE_EVAL(work);
    return count;
}

int sparql_complex_pattern(SparqlEngine* engine, uint32_t* results, int max_results) {
    int count = 0;
    volatile uint32_t work = 0;
    
    // Complex pattern: person foaf:knows person (both must be typed as Person)
    for (size_t i = 0; i < engine->triple_count && count < max_results; i++) {
        if (engine->predicates[i] == FOAF_KNOWS) {
            uint32_t person1 = engine->subjects[i];
            uint32_t person2 = engine->objects[i];
            
            work += person1 + person2;
            
            bool person1_valid = false, person2_valid = false;
            
            // Check if both are typed as Person - expensive nested scan
            for (size_t j = 0; j < engine->triple_count; j++) {
                work += j % 256; // Prevent optimization
                
                if (engine->subjects[j] == person1 && 
                    engine->predicates[j] == RDF_TYPE && 
                    engine->objects[j] == PERSON_CLASS) {
                    person1_valid = true;
                    work += person1;
                }
                
                if (engine->subjects[j] == person2 && 
                    engine->predicates[j] == RDF_TYPE && 
                    engine->objects[j] == PERSON_CLASS) {
                    person2_valid = true;
                    work += person2;
                }
                
                // Early exit if both found
                if (person1_valid && person2_valid) break;
            }
            
            if (person1_valid && person2_valid) {
                results[count] = person1;
                work += person1;
                count++;
            }
        }
    }
    
    FORCE_EVAL(work);
    return count;
}

typedef struct {
    const char* name;
    uint64_t total_cycles;
    double avg_cycles;
    int result_count;
    bool seven_tick_compliant;
} benchmark_result_t;

void setup_test_data(SparqlEngine* engine) {
    printf("🔄 Setting up test data...\n");
    
    srand(42); // Reproducible
    
    // Add substantial dataset to stress the engine
    
    // Add persons (1000-1999)
    for (int i = 1000; i < 2000; i++) {
        add_triple(engine, i, RDF_TYPE, PERSON_CLASS);
        add_triple(engine, i, FOAF_NAME, 5000 + i);
        
        // Add social connections (sparse graph)
        if (i % 7 == 0 && i < 1990) {
            add_triple(engine, i, FOAF_KNOWS, i + (rand() % 20) + 1);
        }
    }
    
    // Add documents (2000-2499)
    for (int i = 2000; i < 2500; i++) {
        add_triple(engine, i, RDF_TYPE, DOCUMENT_CLASS);
        add_triple(engine, i, DC_TITLE, 6000 + i);
        add_triple(engine, i, DC_CREATOR, 1000 + (rand() % 1000));
    }
    
    // Add customers (subset of persons)
    for (int i = 1000; i < 1300; i++) {
        if (i % 5 == 0) {
            add_triple(engine, i, RDF_TYPE, CUSTOMER_CLASS);
        }
    }
    
    printf("✅ Test data loaded: %zu triples\n", engine->triple_count);
}

benchmark_result_t run_benchmark(const char* name, SparqlEngine* engine,
                                 int (*query_func)(SparqlEngine*, uint32_t*, int)) {
    const int iterations = 500; // Reasonable sample size
    const int warmup = 50;
    uint32_t results[2000];
    uint64_t total_cycles = 0;
    int result_count = 0;
    
    printf("🏃 Benchmarking: %s", name);
    
    // Warmup phase
    for (int i = 0; i < warmup; i++) {
        query_func(engine, results, 2000);
    }
    
    // Measurement phase
    for (int i = 0; i < iterations; i++) {
        uint64_t start = get_cycles();
        int count = query_func(engine, results, 2000);
        uint64_t end = get_cycles();
        
        total_cycles += (end - start);
        if (i == 0) result_count = count;
        
        FORCE_EVAL(results[0]); // Prevent optimization
        
        if (i % 100 == 0) printf(".");
    }
    printf(" done\n");
    
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
int wrapper_persons(SparqlEngine* engine, uint32_t* results, int max_results) {
    return sparql_type_query(engine, PERSON_CLASS, results, max_results);
}

int wrapper_documents(SparqlEngine* engine, uint32_t* results, int max_results) {
    return sparql_type_query(engine, DOCUMENT_CLASS, results, max_results);
}

int wrapper_customers(SparqlEngine* engine, uint32_t* results, int max_results) {
    return sparql_type_query(engine, CUSTOMER_CLASS, results, max_results);
}

int wrapper_foaf_scan(SparqlEngine* engine, uint32_t* results, int max_results) {
    return sparql_predicate_scan(engine, FOAF_NAME, results, max_results / 2);
}

int wrapper_joins(SparqlEngine* engine, uint32_t* results, int max_results) {
    return sparql_join_query(engine, RDF_TYPE, FOAF_NAME, results, max_results);
}

int wrapper_complex(SparqlEngine* engine, uint32_t* results, int max_results) {
    return sparql_complex_pattern(engine, results, max_results);
}

void print_results(benchmark_result_t* results, int count) {
    printf("\n📊 Corrected SPARQL Benchmark Results\n");
    printf("======================================\n\n");
    
    printf("%-35s %12s %8s %6s %s\n", "Query Pattern", "Avg Cycles", "Results", "7T", "Status");
    printf("%-35s %12s %8s %6s %s\n", "-----------", "-----------", "-------", "--", "------");
    
    int compliant_count = 0;
    double total_cycles = 0;
    
    for (int i = 0; i < count; i++) {
        const char* status_icon = results[i].seven_tick_compliant ? "✅" : "❌";
        const char* status_text = results[i].seven_tick_compliant ? "PASS" : "FAIL";
        
        printf("%-35s %12.1f %8d %6s %s\n",
               results[i].name,
               results[i].avg_cycles,
               results[i].result_count,
               status_icon,
               status_text);
        
        total_cycles += results[i].avg_cycles;
        if (results[i].seven_tick_compliant) compliant_count++;
    }
    
    double avg_cycles = total_cycles / count;
    
    printf("\n📈 Performance Analysis:\n");
    printf("  Patterns tested: %d\n", count);
    printf("  Average cycles: %.1f\n", avg_cycles);
    printf("  7-tick compliant: %d/%d (%.1f%%)\n", 
           compliant_count, count, 100.0 * compliant_count / count);
    
    // Performance assessment
    const char* assessment;
    if (avg_cycles <= 7.0) {
        assessment = "✅ TARGET ACHIEVED - Ready for production";
    } else if (avg_cycles <= 15.0) {
        assessment = "⚠️ CLOSE TO TARGET - Minor optimizations needed";
    } else if (avg_cycles <= 50.0) {
        assessment = "⚠️ MODERATE PERFORMANCE - Significant optimizations needed";
    } else {
        assessment = "❌ POOR PERFORMANCE - Major architectural changes required";
    }
    
    printf("  Assessment: %s\n", assessment);
    
    // Recommendations
    printf("\n💡 Recommendations:\n");
    if (avg_cycles > 7.0) {
        printf("  - Implement hash-based indexing for faster lookups\n");
        printf("  - Use SIMD instructions for parallel comparison operations\n");
        printf("  - Add specialized indexes for common query patterns\n");
        printf("  - Consider AOT compilation for frequently used queries\n");
    } else {
        printf("  - Current performance meets 7-tick requirements\n");
        printf("  - Consider additional query pattern optimizations\n");
        printf("  - Monitor performance with larger datasets\n");
    }
}

int main() {
    printf("🚀 Corrected SPARQL Performance Benchmark\n");
    printf("==========================================\n");
    printf("Testing realistic SPARQL performance with proper measurement\n\n");
    
    // Create engine with substantial capacity
    SparqlEngine* engine = create_engine(100000);
    if (!engine) {
        printf("❌ Failed to create SPARQL engine\n");
        return 1;
    }
    
    // Setup test data
    setup_test_data(engine);
    
    // Run benchmarks
    benchmark_result_t results[10];
    int result_count = 0;
    
    results[result_count++] = run_benchmark("Type Query (Person) - ?s rdf:type :Person", engine, wrapper_persons);
    results[result_count++] = run_benchmark("Type Query (Document) - ?s rdf:type :Document", engine, wrapper_documents);
    results[result_count++] = run_benchmark("Type Query (Customer) - ?s rdf:type :Customer", engine, wrapper_customers);
    results[result_count++] = run_benchmark("Predicate Scan - ?s foaf:name ?o", engine, wrapper_foaf_scan);
    results[result_count++] = run_benchmark("Join Pattern - ?s rdf:type ?t . ?s foaf:name ?n", engine, wrapper_joins);
    results[result_count++] = run_benchmark("Complex Pattern - ?p1 foaf:knows ?p2 (typed)", engine, wrapper_complex);
    
    // Print results
    print_results(results, result_count);
    
    // Cleanup
    destroy_engine(engine);
    
    // Return based on overall performance
    double total_avg = 0;
    for (int i = 0; i < result_count; i++) {
        total_avg += results[i].avg_cycles;
    }
    total_avg /= result_count;
    
    return (total_avg <= 15.0) ? 0 : 1; // Allow some margin for "close to target"
}