/*
 * bench_arena.c - Performance benchmarks for arena allocator
 * 
 * Measures allocation speed and compares with malloc
 * Validates 7T compliance and O(1) characteristics
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>

// Include arena implementation
#include "../include/cns/arena.h"

// Benchmark configuration
#define BENCHMARK_ITERATIONS 10000
#define WARMUP_ITERATIONS 1000
#define MAX_ALLOC_SIZE 1024
#define MIN_ALLOC_SIZE 8

// Timing utilities
static inline uint64_t get_cycles() {
    uint64_t cycles;
    __asm__ volatile ("rdtsc" : "=A" (cycles));
    return cycles;
}

static inline double get_time_seconds() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec * 1e-6;
}

// Statistics structure
typedef struct {
    double min_time;
    double max_time;
    double avg_time;
    double total_time;
    uint64_t min_cycles;
    uint64_t max_cycles;
    uint64_t avg_cycles;
    uint64_t total_cycles;
    size_t iterations;
    size_t bytes_allocated;
} benchmark_stats_t;

// Initialize statistics
void init_stats(benchmark_stats_t* stats) {
    memset(stats, 0, sizeof(benchmark_stats_t));
    stats->min_time = 1e9;
    stats->max_time = 0;
    stats->min_cycles = UINT64_MAX;
    stats->max_cycles = 0;
}

// Update statistics with new measurement
void update_stats(benchmark_stats_t* stats, double time, uint64_t cycles, size_t bytes) {
    stats->iterations++;
    stats->bytes_allocated += bytes;
    stats->total_time += time;
    stats->total_cycles += cycles;
    
    if (time < stats->min_time) stats->min_time = time;
    if (time > stats->max_time) stats->max_time = time;
    if (cycles < stats->min_cycles) stats->min_cycles = cycles;
    if (cycles > stats->max_cycles) stats->max_cycles = cycles;
}

// Finalize statistics calculations
void finalize_stats(benchmark_stats_t* stats) {
    if (stats->iterations > 0) {
        stats->avg_time = stats->total_time / stats->iterations;
        stats->avg_cycles = stats->total_cycles / stats->iterations;
    }
}

// Print statistics
void print_stats(const char* name, benchmark_stats_t* stats) {
    printf("\n%s Benchmark Results:\n", name);
    printf("====================\n");
    printf("Iterations:     %zu\n", stats->iterations);
    printf("Bytes allocated: %zu\n", stats->bytes_allocated);
    printf("Total time:     %.6f seconds\n", stats->total_time);
    printf("Average time:   %.9f seconds (%.3f ns)\n", 
           stats->avg_time, stats->avg_time * 1e9);
    printf("Min time:       %.9f seconds (%.3f ns)\n", 
           stats->min_time, stats->min_time * 1e9);
    printf("Max time:       %.9f seconds (%.3f ns)\n", 
           stats->max_time, stats->max_time * 1e9);
    printf("Average cycles: %llu\n", stats->avg_cycles);
    printf("Min cycles:     %llu\n", stats->min_cycles);
    printf("Max cycles:     %llu\n", stats->max_cycles);
    printf("Throughput:     %.3f MB/s\n", 
           (stats->bytes_allocated / (1024.0 * 1024.0)) / stats->total_time);
    printf("Allocs/sec:     %.0f\n", stats->iterations / stats->total_time);
}

// Benchmark arena allocator
void benchmark_arena() {
    printf("Benchmarking Arena Allocator...\n");
    
    arena_t arena;
    size_t arena_size = 64 * 1024 * 1024; // 64MB
    arena_create(&arena, arena_size);
    
    benchmark_stats_t stats;
    init_stats(&stats);
    
    // Warmup
    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
        arena_alloc(&arena, 64);
        if (i % 100 == 0) arena_reset(&arena);
    }
    arena_reset(&arena);
    
    // Benchmark fixed size allocations
    printf("Testing fixed 64-byte allocations...\n");
    for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
        uint64_t start_cycles = get_cycles();
        double start_time = get_time_seconds();
        
        void* ptr = arena_alloc(&arena, 64);
        
        double end_time = get_time_seconds();
        uint64_t end_cycles = get_cycles();
        
        if (ptr == NULL) {
            arena_reset(&arena);
            ptr = arena_alloc(&arena, 64);
        }
        
        update_stats(&stats, end_time - start_time, end_cycles - start_cycles, 64);
    }
    
    finalize_stats(&stats);
    print_stats("Arena (64-byte)", &stats);
    
    // Benchmark variable size allocations
    arena_reset(&arena);
    init_stats(&stats);
    
    printf("\nTesting variable size allocations (8-1024 bytes)...\n");
    for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
        size_t size = MIN_ALLOC_SIZE + (i % (MAX_ALLOC_SIZE - MIN_ALLOC_SIZE));
        
        uint64_t start_cycles = get_cycles();
        double start_time = get_time_seconds();
        
        void* ptr = arena_alloc(&arena, size);
        
        double end_time = get_time_seconds();
        uint64_t end_cycles = get_cycles();
        
        if (ptr == NULL) {
            arena_reset(&arena);
            ptr = arena_alloc(&arena, size);
        }
        
        update_stats(&stats, end_time - start_time, end_cycles - start_cycles, size);
    }
    
    finalize_stats(&stats);
    print_stats("Arena (variable)", &stats);
    
    arena_destroy(&arena);
}

// Benchmark malloc for comparison
void benchmark_malloc() {
    printf("\nBenchmarking Malloc for Comparison...\n");
    
    benchmark_stats_t stats;
    init_stats(&stats);
    
    void** ptrs = malloc(BENCHMARK_ITERATIONS * sizeof(void*));
    
    // Warmup
    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
        void* ptr = malloc(64);
        free(ptr);
    }
    
    // Benchmark fixed size allocations
    printf("Testing fixed 64-byte allocations...\n");
    for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
        uint64_t start_cycles = get_cycles();
        double start_time = get_time_seconds();
        
        ptrs[i] = malloc(64);
        
        double end_time = get_time_seconds();
        uint64_t end_cycles = get_cycles();
        
        update_stats(&stats, end_time - start_time, end_cycles - start_cycles, 64);
    }
    
    // Free all allocations
    for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
        free(ptrs[i]);
    }
    
    finalize_stats(&stats);
    print_stats("Malloc (64-byte)", &stats);
    
    // Benchmark variable size allocations
    init_stats(&stats);
    
    printf("\nTesting variable size allocations (8-1024 bytes)...\n");
    for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
        size_t size = MIN_ALLOC_SIZE + (i % (MAX_ALLOC_SIZE - MIN_ALLOC_SIZE));
        
        uint64_t start_cycles = get_cycles();
        double start_time = get_time_seconds();
        
        ptrs[i] = malloc(size);
        
        double end_time = get_time_seconds();
        uint64_t end_cycles = get_cycles();
        
        update_stats(&stats, end_time - start_time, end_cycles - start_cycles, size);
    }
    
    // Free all allocations
    for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
        free(ptrs[i]);
    }
    
    finalize_stats(&stats);
    print_stats("Malloc (variable)", &stats);
    
    free(ptrs);
}

// 7T compliance validation
void validate_7t_compliance() {
    printf("\n7T Compliance Validation...\n");
    printf("===========================\n");
    
    arena_t arena;
    arena_create(&arena, 1024 * 1024);
    
    int compliant_count = 0;
    int total_tests = 1000;
    uint64_t max_cycles = 0;
    
    // Warmup
    for (int i = 0; i < 100; i++) {
        arena_alloc(&arena, 64);
    }
    arena_reset(&arena);
    
    for (int i = 0; i < total_tests; i++) {
        uint64_t start_cycles = get_cycles();
        void* ptr = arena_alloc(&arena, 64);
        uint64_t end_cycles = get_cycles();
        
        uint64_t cycles = end_cycles - start_cycles;
        
        if (cycles <= 7) {
            compliant_count++;
        }
        
        if (cycles > max_cycles) {
            max_cycles = cycles;
        }
        
        if (ptr == NULL) {
            arena_reset(&arena);
        }
    }
    
    double compliance_rate = (double)compliant_count / total_tests * 100.0;
    
    printf("7T Compliance Results:\n");
    printf("Total tests:      %d\n", total_tests);
    printf("Compliant:        %d\n", compliant_count);
    printf("Compliance rate:  %.1f%%\n", compliance_rate);
    printf("Max cycles seen:  %llu\n", max_cycles);
    printf("Status:           %s\n", 
           compliance_rate >= 95.0 ? "PASS" : "FAIL");
    
    arena_destroy(&arena);
}

// Memory fragmentation analysis
void analyze_fragmentation() {
    printf("\nMemory Fragmentation Analysis...\n");
    printf("================================\n");
    
    arena_t arena;
    size_t capacity = 1024 * 1024; // 1MB
    arena_create(&arena, capacity);
    
    // Allocate random sizes to test fragmentation
    void* ptrs[1000];
    size_t total_requested = 0;
    int alloc_count = 0;
    
    for (int i = 0; i < 1000; i++) {
        size_t size = 16 + (rand() % 512); // 16-528 bytes
        ptrs[i] = arena_alloc(&arena, size);
        
        if (ptrs[i] != NULL) {
            total_requested += size;
            alloc_count++;
        } else {
            break;
        }
    }
    
    double efficiency = (double)total_requested / arena.used * 100.0;
    double utilization = (double)arena.used / arena.capacity * 100.0;
    
    printf("Fragmentation Results:\n");
    printf("Allocations:      %d\n", alloc_count);
    printf("Requested:        %zu bytes\n", total_requested);
    printf("Actually used:    %zu bytes\n", arena.used);
    printf("Memory efficiency: %.1f%%\n", efficiency);
    printf("Arena utilization: %.1f%%\n", utilization);
    printf("Overhead:         %zu bytes (%.1f%%)\n", 
           arena.used - total_requested,
           (double)(arena.used - total_requested) / total_requested * 100.0);
    
    arena_destroy(&arena);
}

// Scalability test
void test_scalability() {
    printf("\nScalability Test...\n");
    printf("==================\n");
    
    size_t sizes[] = { 1024, 10240, 102400, 1024000, 10240000 }; // 1KB to 10MB
    int num_sizes = sizeof(sizes) / sizeof(sizes[0]);
    
    for (int i = 0; i < num_sizes; i++) {
        arena_t arena;
        arena_create(&arena, sizes[i]);
        
        benchmark_stats_t stats;
        init_stats(&stats);
        
        int iterations = 1000;
        size_t alloc_size = 64;
        
        for (int j = 0; j < iterations; j++) {
            double start_time = get_time_seconds();
            void* ptr = arena_alloc(&arena, alloc_size);
            double end_time = get_time_seconds();
            
            if (ptr == NULL) {
                arena_reset(&arena);
                ptr = arena_alloc(&arena, alloc_size);
            }
            
            update_stats(&stats, end_time - start_time, 0, alloc_size);
        }
        
        finalize_stats(&stats);
        
        printf("Arena size: %6zu KB, Avg time: %.3f ns\n", 
               sizes[i] / 1024, stats.avg_time * 1e9);
        
        arena_destroy(&arena);
    }
}

int main() {
    printf("Arena Allocator Performance Benchmark Suite\n");
    printf("===========================================\n");
    
    // Run benchmarks
    benchmark_arena();
    benchmark_malloc();
    validate_7t_compliance();
    analyze_fragmentation();
    test_scalability();
    
    printf("\nBenchmark Complete!\n");
    
    return 0;
}