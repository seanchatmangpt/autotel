/*
 * SPARQL Realistic Benchmark - Corrected Implementation
 * Tests actual SPARQL performance with realistic datasets and measurement
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <stdbool.h>

// Prevent compiler optimizations that would skew benchmarks
#define FORCE_EVAL(x) __asm__ __volatile__("" : : "g"(x) : "memory")

// SPARQL engine with realistic complexity
typedef struct {
    uint32_t* subjects;
    uint32_t* predicates; 
    uint32_t* objects;
    size_t triple_count;
    size_t max_triples;
    // Add some cache-unfriendly fields to simulate real engine
    uint64_t padding[8];
    char* string_pool;
    size_t pool_size;
} RealSparqlEngine;

// Vocabulary IDs
#define RDF_TYPE 1
#define FOAF_NAME 2
#define FOAF_KNOWS 3
#define DC_CREATOR 4
#define DC_TITLE 5

#define PERSON_CLASS 100
#define DOCUMENT_CLASS 101
#define CUSTOMER_CLASS 102

// More accurate cycle measurement
static inline uint64_t get_cycles_precise(void) {
    uint64_t cycles;
    
#if defined(__x86_64__) || defined(__i386__)
    // Use RDTSCP for more accurate measurement
    uint32_t lo, hi;
    __asm__ __volatile__ ("rdtscp\n\t"
                         "mov %%edx, %0\n\t"
                         "mov %%eax, %1\n\t"
                         "cpuid\n\t"
                         : "=r" (hi), "=r" (lo)
                         :
                         : "%rax", "%rbx", "%rcx", "%rdx");
    cycles = ((uint64_t)hi << 32) | lo;
#elif defined(__aarch64__)
    __asm__ __volatile__("mrs %0, cntvct_el0" : "=r" (cycles));
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    cycles = (uint64_t)ts.tv_sec * 3000000000ULL + (uint64_t)ts.tv_nsec * 3;
#endif
    
    return cycles;
}

RealSparqlEngine* create_real_engine(size_t max_triples) {
    RealSparqlEngine* engine = malloc(sizeof(RealSparqlEngine));
    if (!engine) return NULL;
    
    // Allocate larger, more realistic arrays
    engine->subjects = malloc(max_triples * sizeof(uint32_t));
    engine->predicates = malloc(max_triples * sizeof(uint32_t));
    engine->objects = malloc(max_triples * sizeof(uint32_t));
    engine->string_pool = malloc(max_triples * 64); // String storage
    engine->triple_count = 0;
    engine->max_triples = max_triples;
    engine->pool_size = max_triples * 64;
    
    // Initialize padding to prevent optimizations
    for (int i = 0; i < 8; i++) {
        engine->padding[i] = rand();
    }
    
    if (!engine->subjects || !engine->predicates || !engine->objects || !engine->string_pool) {
        free(engine->subjects);
        free(engine->predicates);
        free(engine->objects);
        free(engine->string_pool);
        free(engine);
        return NULL;
    }
    
    return engine;
}

void destroy_real_engine(RealSparqlEngine* engine) {
    if (engine) {
        free(engine->subjects);
        free(engine->predicates);
        free(engine->objects);
        free(engine->string_pool);
        free(engine);
    }
}

void add_real_triple(RealSparqlEngine* engine, uint32_t s, uint32_t p, uint32_t o) {
    if (engine->triple_count < engine->max_triples) {
        size_t idx = engine->triple_count;
        engine->subjects[idx] = s;
        engine->predicates[idx] = p;
        engine->objects[idx] = o;
        
        // Add some realistic overhead - string operations
        char* str_pos = engine->string_pool + (idx % (engine->pool_size - 64));
        snprintf(str_pos, 64, "entity_%u_%u_%u", s, p, o);
        
        engine->triple_count++;
    }
}

// Realistic query implementations with actual SPARQL complexity
int realistic_type_query(RealSparqlEngine* engine, uint32_t type_class, uint32_t* results, int max_results) {
    int count = 0;
    volatile uint32_t checksum = 0; // Prevent optimization
    
    // More realistic scan with additional operations
    for (size_t i = 0; i < engine->triple_count && count < max_results; i++) {
        // Add memory access pattern that prevents optimization
        volatile uint32_t pred = engine->predicates[i];
        volatile uint32_t obj = engine->objects[i];
        
        if (pred == RDF_TYPE && obj == type_class) {
            results[count] = engine->subjects[i];
            checksum ^= results[count]; // Prevent dead code elimination
            count++;
        }
        
        // Add some realistic overhead - string comparison
        if (i % 10 == 0) {
            char* str_pos = engine->string_pool + (i % (engine->pool_size - 64));
            volatile int len = strlen(str_pos); // Force string operation
            checksum ^= len;
        }
    }
    
    FORCE_EVAL(checksum);
    return count;
}

int realistic_subject_predicate_query(RealSparqlEngine* engine, uint32_t subject, uint32_t predicate, uint32_t* results, int max_results) {
    int count = 0;
    volatile uint32_t checksum = 0;
    
    for (size_t i = 0; i < engine->triple_count && count < max_results; i++) {
        volatile uint32_t subj = engine->subjects[i];
        volatile uint32_t pred = engine->predicates[i];
        
        if (subj == subject && pred == predicate) {
            results[count] = engine->objects[i];
            checksum ^= results[count];
            count++;
        }
        
        // Realistic overhead - hash computation
        if (i % 5 == 0) {
            volatile uint32_t hash = subj * 31 + pred * 17 + engine->objects[i] * 7;
            checksum ^= hash;
        }
    }
    
    FORCE_EVAL(checksum);
    return count;
}

int realistic_join_query(RealSparqlEngine* engine, uint32_t pred1, uint32_t pred2, uint32_t* results, int max_results) {
    int count = 0;
    volatile uint32_t checksum = 0;
    
    // Realistic nested loop join with hash table simulation
    uint32_t hash_table[1024] = {0}; // Simulate hash table overhead
    
    // First pass: populate "hash table" 
    for (size_t i = 0; i < engine->triple_count; i++) {
        if (engine->predicates[i] == pred1) {
            uint32_t hash = engine->subjects[i] % 1024;
            hash_table[hash] = engine->subjects[i];
        }
    }
    
    // Second pass: probe and join
    for (size_t i = 0; i < engine->triple_count && count < max_results; i++) {
        if (engine->predicates[i] == pred2) {
            uint32_t subject = engine->subjects[i];
            uint32_t hash = subject % 1024;
            
            // Simulate hash table lookup overhead
            for (int probe = 0; probe < 3; probe++) {
                volatile uint32_t entry = hash_table[(hash + probe) % 1024];
                if (entry == subject) {
                    results[count] = subject;
                    checksum ^= subject;
                    count++;
                    break;
                }
            }
        }
    }
    
    FORCE_EVAL(checksum);
    return count;
}

int realistic_social_connections(RealSparqlEngine* engine, uint32_t* results, int max_results) {
    int count = 0;
    volatile uint32_t checksum = 0;
    
    // Complex pattern with multiple joins and type checking
    for (size_t i = 0; i < engine->triple_count && count < max_results; i++) {
        if (engine->predicates[i] == FOAF_KNOWS) {
            uint32_t person1 = engine->subjects[i];
            uint32_t person2 = engine->objects[i];
            
            bool person1_valid = false, person2_valid = false;
            
            // Type verification with realistic scan cost
            for (size_t j = 0; j < engine->triple_count; j++) {
                volatile uint32_t subj = engine->subjects[j];
                volatile uint32_t pred = engine->predicates[j];
                volatile uint32_t obj = engine->objects[j];
                
                if (subj == person1 && pred == RDF_TYPE && obj == PERSON_CLASS) {
                    person1_valid = true;
                    checksum ^= person1;
                }
                if (subj == person2 && pred == RDF_TYPE && obj == PERSON_CLASS) {
                    person2_valid = true;
                    checksum ^= person2;
                }
                
                // Additional overhead to simulate real SPARQL complexity
                if (j % 20 == 0) {
                    volatile uint32_t hash = subj ^ pred ^ obj;
                    checksum ^= hash;
                }
            }
            
            if (person1_valid && person2_valid) {
                results[count] = person1;
                count++;
            }
        }
    }
    
    FORCE_EVAL(checksum);
    return count;
}

typedef struct {
    const char* name;
    const char* description;
    uint64_t total_cycles;
    double avg_cycles;
    int result_count;
    bool seven_tick_compliant;
} realistic_benchmark_result_t;

void setup_realistic_test_data(RealSparqlEngine* engine) {
    printf("🔄 Setting up realistic test data...\n");
    
    srand(42); // Reproducible results
    
    // Add persons with realistic distribution
    for (int i = 1000; i < 1500; i++) {
        add_real_triple(engine, i, RDF_TYPE, PERSON_CLASS);
        add_real_triple(engine, i, FOAF_NAME, 5000 + i);
        
        // Social connections (sparse)
        if (rand() % 5 == 0 && i < 1495) {
            add_real_triple(engine, i, FOAF_KNOWS, i + rand() % 10 + 1);
        }
    }
    
    // Add documents
    for (int i = 2000; i < 2200; i++) {
        add_real_triple(engine, i, RDF_TYPE, DOCUMENT_CLASS);
        add_real_triple(engine, i, DC_TITLE, 6000 + i);
        add_real_triple(engine, i, DC_CREATOR, 1000 + rand() % 500);
    }
    
    // Add customers (subset of persons)
    for (int i = 1000; i < 1100; i++) {
        if (rand() % 3 == 0) {
            add_real_triple(engine, i, RDF_TYPE, CUSTOMER_CLASS);
        }
    }
    
    printf("✅ Realistic test data: %zu triples loaded\n", engine->triple_count);
}

realistic_benchmark_result_t run_realistic_benchmark(const char* name, const char* description, 
                                                    RealSparqlEngine* engine,
                                                    int (*query_func)(RealSparqlEngine*, uint32_t*, int)) {
    const int iterations = 1000; // Fewer iterations for more accurate measurement
    const int warmup = 100;
    uint32_t results[1000];
    uint64_t total_cycles = 0;
    uint64_t min_cycles = UINT64_MAX;
    uint64_t max_cycles = 0;
    int result_count = 0;
    
    printf("🏃 Benchmarking: %s\n", name);
    
    // Warm up phase
    for (int i = 0; i < warmup; i++) {
        query_func(engine, results, 1000);
    }
    
    // Clear cache and reset measurement
    volatile char dummy_array[1024*1024]; // 1MB to clear cache
    for (int i = 0; i < 1024*1024; i++) {
        dummy_array[i] = i % 256;
    }
    FORCE_EVAL(dummy_array[1023*1024]);
    
    // Actual benchmark with more accurate measurement
    for (int i = 0; i < iterations; i++) {
        // Serialize execution
        __asm__ __volatile__("mfence" ::: "memory");
        
        uint64_t start = get_cycles_precise();
        int count = query_func(engine, results, 1000);
        uint64_t end = get_cycles_precise();
        
        __asm__ __volatile__("mfence" ::: "memory");
        
        uint64_t cycles = end - start;
        total_cycles += cycles;
        
        if (cycles < min_cycles) min_cycles = cycles;
        if (cycles > max_cycles) max_cycles = cycles;
        
        if (i == 0) result_count = count;
        
        // Prevent over-optimization between iterations
        FORCE_EVAL(results[0]);
    }
    
    realistic_benchmark_result_t result = {
        .name = name,
        .description = description,
        .total_cycles = total_cycles,
        .avg_cycles = (double)total_cycles / iterations,
        .result_count = result_count,
        .seven_tick_compliant = ((double)total_cycles / iterations) <= 7.0
    };
    
    printf("  Min cycles: %llu, Max cycles: %llu, Avg: %.2f\n", 
           min_cycles, max_cycles, result.avg_cycles);
    
    return result;
}

// Wrapper functions
int wrapper_realistic_persons(RealSparqlEngine* engine, uint32_t* results, int max_results) {
    return realistic_type_query(engine, PERSON_CLASS, results, max_results);
}

int wrapper_realistic_documents(RealSparqlEngine* engine, uint32_t* results, int max_results) {
    return realistic_type_query(engine, DOCUMENT_CLASS, results, max_results);
}

int wrapper_realistic_customers(RealSparqlEngine* engine, uint32_t* results, int max_results) {
    return realistic_type_query(engine, CUSTOMER_CLASS, results, max_results);
}

int wrapper_realistic_joins(RealSparqlEngine* engine, uint32_t* results, int max_results) {
    return realistic_join_query(engine, RDF_TYPE, FOAF_NAME, results, max_results);
}

int wrapper_realistic_social(RealSparqlEngine* engine, uint32_t* results, int max_results) {
    return realistic_social_connections(engine, results, max_results);
}

void print_realistic_results(realistic_benchmark_result_t* results, int count) {
    printf("\n📊 Realistic SPARQL Benchmark Results\n");
    printf("======================================\n\n");
    
    printf("%-30s %12s %8s %6s %s\n", "Query Pattern", "Avg Cycles", "Results", "7T", "Status");
    printf("%-30s %12s %8s %6s %s\n", "-------------", "-----------", "-------", "--", "------");
    
    int compliant_count = 0;
    double total_cycles = 0;
    
    for (int i = 0; i < count; i++) {
        printf("%-30s %12.1f %8d %6s %s\n",
               results[i].name,
               results[i].avg_cycles,
               results[i].result_count,
               results[i].seven_tick_compliant ? "✅" : "❌",
               results[i].seven_tick_compliant ? "PASS" : "FAIL");
        
        total_cycles += results[i].avg_cycles;
        if (results[i].seven_tick_compliant) compliant_count++;
    }
    
    double avg_cycles = total_cycles / count;
    
    printf("\n📈 Realistic Performance Summary:\n");
    printf("  Patterns tested: %d\n", count);
    printf("  Average cycles: %.1f\n", avg_cycles);
    printf("  7-tick compliant: %d/%d (%.1f%%)\n", 
           compliant_count, count, 100.0 * compliant_count / count);
    
    const char* rating;
    if (compliant_count == count) rating = "✅ EXCELLENT";
    else if (compliant_count >= count * 0.8) rating = "⚠️ GOOD";
    else if (compliant_count >= count * 0.6) rating = "⚠️ ACCEPTABLE";
    else rating = "❌ NEEDS OPTIMIZATION";
    
    printf("  Performance rating: %s\n", rating);
    printf("  Assessment: %s\n", 
           (avg_cycles <= 7.0) ? "Target achieved" : 
           (avg_cycles <= 15.0) ? "Close to target" :
           (avg_cycles <= 30.0) ? "Needs optimization" : "Significant work needed");
}

int main() {
    printf("🚀 Realistic SPARQL Performance Benchmark\n");
    printf("==========================================\n");
    printf("Testing actual SPARQL performance with realistic complexity\n\n");
    
    // Create realistic engine
    RealSparqlEngine* engine = create_real_engine(50000);
    if (!engine) {
        printf("❌ Failed to create SPARQL engine\n");
        return 1;
    }
    
    // Setup realistic test data
    setup_realistic_test_data(engine);
    
    // Run realistic benchmarks
    realistic_benchmark_result_t results[10];
    int result_count = 0;
    
    results[result_count++] = run_realistic_benchmark("Type Query (Person)", "?s rdf:type :Person", engine, wrapper_realistic_persons);
    results[result_count++] = run_realistic_benchmark("Type Query (Document)", "?s rdf:type :Document", engine, wrapper_realistic_documents);
    results[result_count++] = run_realistic_benchmark("Type Query (Customer)", "?s rdf:type :Customer", engine, wrapper_realistic_customers);
    results[result_count++] = run_realistic_benchmark("Property Join", "?s rdf:type ?t . ?s foaf:name ?n", engine, wrapper_realistic_joins);
    results[result_count++] = run_realistic_benchmark("Social Network", "?p1 foaf:knows ?p2", engine, wrapper_realistic_social);
    
    // Print results
    print_realistic_results(results, result_count);
    
    // Cleanup
    destroy_real_engine(engine);
    
    // Return success if performance is reasonable
    int compliant = 0;
    for (int i = 0; i < result_count; i++) {
        if (results[i].seven_tick_compliant) compliant++;
    }
    
    return (compliant >= result_count / 2) ? 0 : 1;
}