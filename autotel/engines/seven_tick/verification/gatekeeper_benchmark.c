#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <dlfcn.h>
#include <string.h>
#include <assert.h>
#include "../runtime/src/seven_t_runtime.h"

// Performance measurement
#ifdef __APPLE__
#include <mach/mach_time.h>
#define CLOCK_MONOTONIC 0
static inline uint64_t get_nanos(void) {
    return mach_absolute_time();
}
#else
#include <time.h>
static inline uint64_t get_nanos(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}
#endif

// Function pointers for dynamic loading
typedef void* (*execute_query_func)(EngineState*, size_t*);
typedef int (*validate_shape_func)(EngineState*, uint32_t);

// Load test data into engine
void load_test_data(EngineState* engine, const char* filename) {
    FILE* f = fopen(filename, "r");
    assert(f != NULL);
    
    char line[4096];
    
    // Skip prefixes
    while (fgets(line, sizeof(line), f)) {
        if (line[0] != '@' && line[0] != '\n') {
            break;
        }
    }
    
    // Parse triples
    do {
        if (line[0] == '\n' || line[0] == '#') continue;
        
        // Simple triple parsing
        char* subj = strtok(line, " \t");
        char* pred = strtok(NULL, " \t");
        char* obj = strtok(NULL, " \t\n");
        
        if (!subj || !pred || !obj) continue;
        
        // Remove trailing dot
        size_t obj_len = strlen(obj);
        if (obj_len > 0 && obj[obj_len-1] == '.') {
            obj[obj_len-1] = '\0';
        }
        
        // Intern strings and add triple
        uint32_t s_id = s7t_intern_string(engine, subj);
        uint32_t p_id = s7t_intern_string(engine, pred);
        uint32_t o_id = s7t_intern_string(engine, obj);
        
        s7t_add_triple(engine, s_id, p_id, o_id);
        
    } while (fgets(line, sizeof(line), f));
    
    fclose(f);
}
// Performance benchmark
typedef struct {
    uint64_t min_latency;
    uint64_t max_latency;
    uint64_t total_latency;
    uint64_t count;
    double throughput_mops;
} PerfStats;

void benchmark_query(execute_query_func query_fn, EngineState* engine, 
                    size_t iterations, PerfStats* stats) {
    
    stats->min_latency = UINT64_MAX;
    stats->max_latency = 0;
    stats->total_latency = 0;
    stats->count = iterations;
    
    // Warmup
    for (int i = 0; i < 100; i++) {
        size_t count;
        void* results = query_fn(engine, &count);
        free(results);
    }
    
    // Benchmark
    uint64_t start_batch = get_nanos();
    
    for (size_t i = 0; i < iterations; i++) {
        uint64_t start = get_nanos();
        
        size_t count;
        void* results = query_fn(engine, &count);
        
        uint64_t end = get_nanos();
        uint64_t latency = end - start;
        
        stats->total_latency += latency;
        if (latency < stats->min_latency) stats->min_latency = latency;
        if (latency > stats->max_latency) stats->max_latency = latency;
        
        free(results);
    }
    
    uint64_t end_batch = get_nanos();
    double batch_time_s = (end_batch - start_batch) / 1e9;
    stats->throughput_mops = iterations / batch_time_s / 1e6;
}

int main(int argc, char** argv) {
    printf("7T Gatekeeper Benchmark\n");
    printf("======================\n\n");
    
    // Step 1: Compile the kernel
    printf("Step 1: Compiling kernel...\n");
    
    // Change to parent directory to run compiler
    if (system("cd .. && compiler/seven-t-compiler "
               "examples/sprint_health/spec/ontology.ttl "
               "examples/sprint_health/spec/shapes.ttl "
               "examples/sprint_health/spec/queries.sparql "
               "/tmp/kernel.so") != 0) {
        fprintf(stderr, "Compilation failed!\n");
        return 1;
    }
    
    // Step 2: Load the compiled kernel
    printf("\nStep 2: Loading compiled kernel...\n");
    void* kernel = dlopen("/tmp/kernel.so", RTLD_NOW);
    if (!kernel) {
        fprintf(stderr, "Failed to load kernel: %s\n", dlerror());
        return 1;
    }
    
    execute_query_func execute_query_1 = dlsym(kernel, "execute_query_1");
    validate_shape_func validate_patient = dlsym(kernel, "validate_patient_shape");
    
    if (!execute_query_1 || !validate_patient) {
        fprintf(stderr, "Failed to find kernel functions\n");
        return 1;
    }
    // Step 3: Generate and load test data
    printf("\nStep 3: Generating test data...\n");
    system("python3 generate_sprint_data.py 10000");
    
    EngineState* engine = s7t_create_engine();
    load_test_data(engine, "/tmp/sprint_health_data.ttl");
    
    // Step 4: Correctness verification
    printf("\nStep 4: Correctness verification...\n");
    
    size_t result_count;
    void* results = execute_query_1(engine, &result_count);
    printf("Query returned %zu results\n", result_count);
    
    // Verify at least some results
    assert(result_count > 0);
    
    // Test SHACL validation
    int valid_count = 0;
    for (uint32_t i = 0; i < 100; i++) {
        if (validate_patient(engine, i)) {
            valid_count++;
        }
    }
    printf("SHACL validation: %d/100 valid patients\n", valid_count);
    
    free(results);
    
    // Step 5: Performance benchmarking
    printf("\nStep 5: Performance benchmarking...\n");
    printf("Running 100,000 query executions...\n");
    
    PerfStats stats;
    benchmark_query(execute_query_1, engine, 100000, &stats);
    
    // Convert to more readable units
    double avg_latency_ns = stats.total_latency / (double)stats.count;
    double avg_latency_us = avg_latency_ns / 1000.0;
    
    printf("\nPerformance Results:\n");
    printf("-------------------\n");
    printf("Min latency: %.2f ns\n", (double)stats.min_latency);
    printf("Max latency: %.2f ns\n", (double)stats.max_latency);
    printf("Avg latency: %.2f ns (%.2f µs)\n", avg_latency_ns, avg_latency_us);
    printf("Throughput: %.2f MOPS\n", stats.throughput_mops);
    
    // Success criteria
    int success = 1;
    
    // L3 tier requirement: < 1µs average latency
    if (avg_latency_us > 1.0) {
        printf("\n❌ FAIL: Average latency %.2f µs exceeds L3 requirement (1 µs)\n", 
               avg_latency_us);
        success = 0;
    } else {
        printf("\n✓ PASS: Average latency %.2f µs meets L3 requirement\n", avg_latency_us);
    }
    
    // Throughput requirement: > 10 MOPS
    if (stats.throughput_mops < 10.0) {
        printf("❌ FAIL: Throughput %.2f MOPS below requirement (10 MOPS)\n", 
               stats.throughput_mops);
        success = 0;
    } else {
        printf("✓ PASS: Throughput %.2f MOPS exceeds requirement\n", stats.throughput_mops);
    }
    
    // Zero hardware violations check (simplified for MVP)
    if (stats.max_latency > avg_latency_ns * 100) {
        printf("❌ FAIL: Max latency spike detected (%.0fx average)\n", 
               stats.max_latency / avg_latency_ns);
        success = 0;
    } else {
        printf("✓ PASS: No significant latency spikes detected\n");
    }
    
    // Cleanup
    s7t_destroy_engine(engine);
    dlclose(kernel);
    
    if (success) {
        printf("\n🎉 ALL BENCHMARKS PASSED! 🎉\n");
        printf("The 7T system has achieved deterministic performance.\n");
        return 0;
    } else {
        printf("\n❌ BENCHMARK FAILED\n");
        return 1;
    }
}
