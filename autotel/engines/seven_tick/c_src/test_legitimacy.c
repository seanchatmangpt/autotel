#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include "sparql7t.h"

// Measure actual cycles using high-resolution timing
static inline uint64_t rdtsc() {
#ifdef __x86_64__
    unsigned int lo, hi;
    __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
    return ((uint64_t)hi << 32) | lo;
#else
    // ARM doesn't have rdtsc, use clock_gettime
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000ULL + ts.tv_nsec;
#endif
}

int main() {
    printf("=== Benchmark Legitimacy Test ===\n\n");
    
    // Create engine with realistic size
    S7TEngine* engine = s7t_create(1000000, 100, 1000000);
    
    // Add test data
    printf("Adding 100,000 triples...\n");
    for (int i = 0; i < 100000; i++) {
        s7t_add_triple(engine, i, i % 100, i * 2);
    }
    
    // Warmup
    printf("Warming up...\n");
    for (int i = 0; i < 1000000; i++) {
        s7t_ask_pattern(engine, i % 100000, i % 100, (i % 100000) * 2);
    }
    
    // Test 1: Best case - sequential access
    printf("\nTest 1: Sequential access (best case)\n");
    uint64_t start = rdtsc();
    int hits = 0;
    for (int i = 0; i < 1000000; i++) {
        if (s7t_ask_pattern(engine, i % 100000, i % 100, (i % 100000) * 2)) {
            hits++;
        }
    }
    uint64_t end = rdtsc();
    double ns_per_op = (double)(end - start) / 1000000.0;
    printf("  Hits: %d/1000000\n", hits);
    printf("  Time per query: %.2f ns\n", ns_per_op);
    
    // Test 2: Worst case - random access
    printf("\nTest 2: Random access (worst case)\n");
    srand(42);
    int* random_subjects = malloc(1000000 * sizeof(int));
    int* random_predicates = malloc(1000000 * sizeof(int));
    for (int i = 0; i < 1000000; i++) {
        random_subjects[i] = rand() % 100000;
        random_predicates[i] = rand() % 100;
    }
    
    start = rdtsc();
    hits = 0;
    for (int i = 0; i < 1000000; i++) {
        if (s7t_ask_pattern(engine, random_subjects[i], random_predicates[i], 
                           random_subjects[i] * 2)) {
            hits++;
        }
    }
    end = rdtsc();
    ns_per_op = (double)(end - start) / 1000000.0;
    printf("  Hits: %d/1000000\n", hits);
    printf("  Time per query: %.2f ns\n", ns_per_op);
    
    // Test 3: Cache miss pattern
    printf("\nTest 3: Cache miss pattern\n");
    // Access pattern designed to cause cache misses
    start = rdtsc();
    hits = 0;
    for (int i = 0; i < 100000; i++) {
        // Jump around in memory
        int s = (i * 9973) % 100000;
        int p = (i * 7919) % 100;
        if (s7t_ask_pattern(engine, s, p, s * 2)) {
            hits++;
        }
    }
    end = rdtsc();
    ns_per_op = (double)(end - start) / 100000.0;
    printf("  Hits: %d/100000\n", hits);
    printf("  Time per query: %.2f ns\n", ns_per_op);
    
    // Memory footprint
    size_t pred_size = 100 * engine->stride_len * sizeof(uint64_t);
    size_t obj_size = 1000000 * engine->stride_len * sizeof(uint64_t);
    size_t index_size = 100 * 1000000 * sizeof(uint32_t);
    size_t total_mb = (pred_size + obj_size + index_size) / (1024 * 1024);
    
    printf("\nMemory usage:\n");
    printf("  Predicate vectors: %zu MB\n", pred_size / (1024 * 1024));
    printf("  Object vectors: %zu MB\n", obj_size / (1024 * 1024));
    printf("  PS->O index: %zu MB\n", index_size / (1024 * 1024));
    printf("  Total: %zu MB\n", total_mb);
    
    printf("\nAnalysis:\n");
    printf("  L1 cache typical: 32-128 KB\n");
    printf("  L2 cache typical: 256KB-1MB\n");
    printf("  L3 cache typical: 8-32MB\n");
    printf("  Our index alone: %zu MB (won't fit in cache)\n", index_size / (1024 * 1024));
    
    free(random_subjects);
    free(random_predicates);
    free(engine->predicate_vectors);
    free(engine->object_vectors);
    free(engine->ps_to_o_index);
    free(engine);
    
    return 0;
}