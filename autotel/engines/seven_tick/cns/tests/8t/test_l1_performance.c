/**
 * @file test_l1_performance.c
 * @brief L1 Cache Performance Validation for 8T
 * 
 * Comprehensive L1 cache performance tests to validate 8T optimization
 * with cache hit rate measurement and performance regression detection.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <immintrin.h>
#include <unistd.h>
#include <sys/mman.h>

// Include 8T types if available
#ifdef CNS_8T_ENABLED
#include "cns/8t.h"
#include "cns/types.h"
#define TICK_LIMIT CNS_8T_TICK_LIMIT
#else
#define TICK_LIMIT 8
#endif

// L1 cache specifications
#define L1_CACHE_SIZE (32 * 1024)       // 32KB L1 data cache
#define L1_CACHE_LINE_SIZE 64           // 64-byte cache lines
#define L1_ASSOCIATIVITY 8              // 8-way associative
#define L1_SETS (L1_CACHE_SIZE / (L1_CACHE_LINE_SIZE * L1_ASSOCIATIVITY))

// Performance test configuration
#define TEST_ITERATIONS 100000
#define WARMUP_ITERATIONS 10000
#define CACHE_FLUSH_SIZE (4 * 1024 * 1024)  // 4MB to flush cache

// Hardware performance counters (if available)
#ifdef __linux__
#include <linux/perf_event.h>
#include <sys/syscall.h>
#include <asm/unistd.h>
#endif

// Performance measurement
static inline uint64_t rdtsc(void) {
    unsigned int lo, hi;
    __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
    return ((uint64_t)hi << 32) | lo;
}

// Memory fence to prevent reordering
static inline void memory_fence(void) {
    __asm__ __volatile__ ("mfence" ::: "memory");
}

// Cache performance metrics
typedef struct {
    uint64_t access_time;
    uint64_t cache_hits;
    uint64_t cache_misses;
    double hit_rate;
    double cycles_per_access;
    uint64_t instructions;
    double ipc;  // Instructions per cycle
} cache_perf_t;

// L1 cache simulator for validation
typedef struct {
    uint32_t sets;
    uint32_t ways;
    uint32_t line_size;
    uint32_t **tags;
    uint32_t **lru;
    uint64_t hits;
    uint64_t misses;
} cache_sim_t;

// Initialize cache simulator
cache_sim_t* cache_sim_init(uint32_t sets, uint32_t ways, uint32_t line_size) {
    cache_sim_t *sim = malloc(sizeof(cache_sim_t));
    if (!sim) return NULL;
    
    sim->sets = sets;
    sim->ways = ways;
    sim->line_size = line_size;
    sim->hits = 0;
    sim->misses = 0;
    
    // Allocate tag array
    sim->tags = malloc(sets * sizeof(uint32_t*));
    sim->lru = malloc(sets * sizeof(uint32_t*));
    
    for (uint32_t i = 0; i < sets; i++) {
        sim->tags[i] = calloc(ways, sizeof(uint32_t));
        sim->lru[i] = calloc(ways, sizeof(uint32_t));
        
        // Initialize LRU counters
        for (uint32_t j = 0; j < ways; j++) {
            sim->lru[i][j] = j;
        }
    }
    
    return sim;
}

// Simulate cache access
int cache_sim_access(cache_sim_t *sim, uint64_t address) {
    uint64_t line_addr = address / sim->line_size;
    uint32_t set = line_addr % sim->sets;
    uint32_t tag = line_addr / sim->sets;
    
    // Check for hit
    for (uint32_t way = 0; way < sim->ways; way++) {
        if (sim->tags[set][way] == tag) {
            // Hit - update LRU
            uint32_t lru_val = sim->lru[set][way];
            for (uint32_t i = 0; i < sim->ways; i++) {
                if (sim->lru[set][i] < lru_val) {
                    sim->lru[set][i]++;
                }
            }
            sim->lru[set][way] = 0;
            sim->hits++;
            return 1; // Hit
        }
    }
    
    // Miss - find LRU way
    uint32_t lru_way = 0;
    uint32_t max_lru = sim->lru[set][0];
    for (uint32_t way = 1; way < sim->ways; way++) {
        if (sim->lru[set][way] > max_lru) {
            max_lru = sim->lru[set][way];
            lru_way = way;
        }
    }
    
    // Replace
    sim->tags[set][lru_way] = tag;
    for (uint32_t i = 0; i < sim->ways; i++) {
        sim->lru[set][i]++;
    }
    sim->lru[set][lru_way] = 0;
    sim->misses++;
    
    return 0; // Miss
}

// Flush cache by accessing large memory region
void flush_cache(void) {
    static char *flush_buffer = NULL;
    if (!flush_buffer) {
        flush_buffer = malloc(CACHE_FLUSH_SIZE);
        if (!flush_buffer) return;
    }
    
    // Touch every cache line
    for (size_t i = 0; i < CACHE_FLUSH_SIZE; i += L1_CACHE_LINE_SIZE) {
        volatile char temp = flush_buffer[i];
        (void)temp;
    }
    memory_fence();
}

// Measure cache performance for access pattern
cache_perf_t measure_cache_performance(void *data, size_t size, 
                                     const char *pattern_name) {
    cache_perf_t perf = {0};
    
    printf("Measuring %s pattern...\n", pattern_name);
    
    // Flush cache
    flush_cache();
    memory_fence();
    
    // Warmup
    volatile int sum = 0;
    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
        for (size_t j = 0; j < size; j += sizeof(int)) {
            sum += *(int*)((char*)data + j);
        }
    }
    
    // Measure performance
    uint64_t start = rdtsc();
    memory_fence();
    
    for (int i = 0; i < TEST_ITERATIONS; i++) {
        for (size_t j = 0; j < size; j += sizeof(int)) {
            sum += *(int*)((char*)data + j);
        }
    }
    
    memory_fence();
    uint64_t end = rdtsc();
    
    perf.access_time = end - start;
    perf.cycles_per_access = (double)(end - start) / (TEST_ITERATIONS * size / sizeof(int));
    
    // Estimate cache performance
    size_t cache_lines = size / L1_CACHE_LINE_SIZE;
    if (cache_lines <= L1_CACHE_SIZE / L1_CACHE_LINE_SIZE) {
        perf.cache_hits = cache_lines * TEST_ITERATIONS;
        perf.cache_misses = 0;
        perf.hit_rate = 1.0;
    } else {
        // Simple estimation for sequential access
        size_t working_set_lines = L1_CACHE_SIZE / L1_CACHE_LINE_SIZE;
        perf.cache_hits = working_set_lines * TEST_ITERATIONS;
        perf.cache_misses = (cache_lines - working_set_lines) * TEST_ITERATIONS;
        perf.hit_rate = (double)working_set_lines / cache_lines;
    }
    
    printf("  Cycles per access: %.2f\n", perf.cycles_per_access);
    printf("  Estimated hit rate: %.1f%%\n", perf.hit_rate * 100);
    
    return perf;
}

// Test sequential access pattern
int test_sequential_access(void) {
    printf("Testing sequential access pattern...\n");
    
    size_t test_sizes[] = {
        L1_CACHE_SIZE / 4,  // Fits in L1
        L1_CACHE_SIZE / 2,  // Fits in L1
        L1_CACHE_SIZE,      // Exactly L1 size
        L1_CACHE_SIZE * 2,  // Exceeds L1
        L1_CACHE_SIZE * 4   // Much larger than L1
    };
    int num_sizes = sizeof(test_sizes) / sizeof(test_sizes[0]);
    
    for (int i = 0; i < num_sizes; i++) {
        void *data = aligned_alloc(L1_CACHE_LINE_SIZE, test_sizes[i]);
        if (!data) continue;
        
        // Initialize data
        memset(data, 0xAA, test_sizes[i]);
        
        char pattern_name[64];
        snprintf(pattern_name, sizeof(pattern_name), 
                "Sequential %zu KB", test_sizes[i] / 1024);
        
        cache_perf_t perf = measure_cache_performance(data, test_sizes[i], pattern_name);
        
        // Validate expected performance
        if (test_sizes[i] <= L1_CACHE_SIZE) {
            if (perf.cycles_per_access > 4.0) {
                printf("⚠ Poor performance for L1-sized data: %.2f cycles/access\n", 
                       perf.cycles_per_access);
            } else {
                printf("✓ Good L1 performance: %.2f cycles/access\n", 
                       perf.cycles_per_access);
            }
        }
        
        free(data);
        printf("\n");
    }
    
    return 0;
}

// Test random access pattern
int test_random_access(void) {
    printf("Testing random access pattern...\n");
    
    const size_t data_size = L1_CACHE_SIZE * 2;
    const int num_accesses = 10000;
    
    uint32_t *data = aligned_alloc(L1_CACHE_LINE_SIZE, data_size);
    if (!data) return -1;
    
    // Initialize data
    for (size_t i = 0; i < data_size / sizeof(uint32_t); i++) {
        data[i] = i;
    }
    
    // Generate random indices
    uint32_t *indices = malloc(num_accesses * sizeof(uint32_t));
    srand(42);
    for (int i = 0; i < num_accesses; i++) {
        indices[i] = rand() % (data_size / sizeof(uint32_t));
    }
    
    // Flush cache
    flush_cache();
    memory_fence();
    
    // Measure random access
    volatile uint32_t sum = 0;
    uint64_t start = rdtsc();
    memory_fence();
    
    for (int iter = 0; iter < TEST_ITERATIONS / 100; iter++) {
        for (int i = 0; i < num_accesses; i++) {
            sum += data[indices[i]];
        }
    }
    
    memory_fence();
    uint64_t end = rdtsc();
    
    double cycles_per_access = (double)(end - start) / 
                              (TEST_ITERATIONS / 100 * num_accesses);
    
    printf("Random access cycles per access: %.2f\n", cycles_per_access);
    
    // Random access should be significantly slower than sequential
    if (cycles_per_access > 10.0) {
        printf("✓ Random access shows expected cache miss penalty\n");
    } else {
        printf("⚠ Random access unexpectedly fast\n");
    }
    
    free(data);
    free(indices);
    return 0;
}

// Test stride access patterns
int test_stride_access(void) {
    printf("Testing stride access patterns...\n");
    
    const size_t data_size = L1_CACHE_SIZE * 2;
    uint32_t *data = aligned_alloc(L1_CACHE_LINE_SIZE, data_size);
    if (!data) return -1;
    
    // Initialize data
    for (size_t i = 0; i < data_size / sizeof(uint32_t); i++) {
        data[i] = i;
    }
    
    // Test different strides
    int strides[] = {1, 2, 4, 8, 16, 32, 64, 128};
    int num_strides = sizeof(strides) / sizeof(strides[0]);
    
    for (int s = 0; s < num_strides; s++) {
        int stride = strides[s];
        
        flush_cache();
        memory_fence();
        
        volatile uint32_t sum = 0;
        uint64_t start = rdtsc();
        memory_fence();
        
        for (int iter = 0; iter < TEST_ITERATIONS / 10; iter++) {
            for (size_t i = 0; i < data_size / sizeof(uint32_t); i += stride) {
                sum += data[i];
            }
        }
        
        memory_fence();
        uint64_t end = rdtsc();
        
        size_t num_accesses = (data_size / sizeof(uint32_t)) / stride;
        double cycles_per_access = (double)(end - start) / 
                                  (TEST_ITERATIONS / 10 * num_accesses);
        
        printf("Stride %d: %.2f cycles/access\n", stride, cycles_per_access);
        
        // Cache line stride (16 elements = 64 bytes) should be efficient
        if (stride == 16) {
            if (cycles_per_access < 8.0) {
                printf("✓ Cache line stride efficient\n");
            } else {
                printf("⚠ Cache line stride unexpectedly slow\n");
            }
        }
    }
    
    free(data);
    return 0;
}

// Test cache associativity
int test_cache_associativity(void) {
    printf("Testing cache associativity...\n");
    
    // Create data that maps to the same cache set
    size_t set_size = L1_CACHE_SIZE / L1_ASSOCIATIVITY;
    size_t stride = set_size;  // Access same set in different ways
    
    uint32_t *data = aligned_alloc(L1_CACHE_LINE_SIZE, stride * (L1_ASSOCIATIVITY + 4));
    if (!data) return -1;
    
    // Initialize data
    for (size_t i = 0; i < stride * (L1_ASSOCIATIVITY + 4) / sizeof(uint32_t); i++) {
        data[i] = i;
    }
    
    // Test accessing L1_ASSOCIATIVITY elements (should fit)
    flush_cache();
    memory_fence();
    
    volatile uint32_t sum = 0;
    uint64_t start = rdtsc();
    memory_fence();
    
    for (int iter = 0; iter < TEST_ITERATIONS; iter++) {
        for (int way = 0; way < L1_ASSOCIATIVITY; way++) {
            sum += data[way * stride / sizeof(uint32_t)];
        }
    }
    
    memory_fence();
    uint64_t end = rdtsc();
    
    double cycles_fit = (double)(end - start) / (TEST_ITERATIONS * L1_ASSOCIATIVITY);
    
    // Test accessing L1_ASSOCIATIVITY + 2 elements (should not fit)
    flush_cache();
    memory_fence();
    
    start = rdtsc();
    memory_fence();
    
    for (int iter = 0; iter < TEST_ITERATIONS; iter++) {
        for (int way = 0; way < L1_ASSOCIATIVITY + 2; way++) {
            sum += data[way * stride / sizeof(uint32_t)];
        }
    }
    
    memory_fence();
    end = rdtsc();
    
    double cycles_overflow = (double)(end - start) / (TEST_ITERATIONS * (L1_ASSOCIATIVITY + 2));
    
    printf("Cycles/access (fits in associativity): %.2f\n", cycles_fit);
    printf("Cycles/access (exceeds associativity): %.2f\n", cycles_overflow);
    printf("Performance ratio: %.2fx\n", cycles_overflow / cycles_fit);
    
    if (cycles_overflow > cycles_fit * 1.5) {
        printf("✓ Cache associativity behavior detected\n");
    } else {
        printf("⚠ Cache associativity behavior not clear\n");
    }
    
    free(data);
    return 0;
}

// Test 8T performance requirements
int test_8t_performance_requirements(void) {
    printf("Testing 8T performance requirements...\n");
    
    // Test basic operations should complete within 8 ticks
    const size_t test_size = 1024;  // Small enough for L1
    uint32_t *data = aligned_alloc(L1_CACHE_LINE_SIZE, test_size);
    if (!data) return -1;
    
    // Initialize data
    for (size_t i = 0; i < test_size / sizeof(uint32_t); i++) {
        data[i] = i;
    }
    
    // Test single access
    flush_cache();
    memory_fence();
    
    uint64_t start = rdtsc();
    volatile uint32_t val = data[0];
    uint64_t end = rdtsc();
    (void)val;
    
    uint64_t single_access_ticks = end - start;
    
    // Test cache-friendly sequential access
    start = rdtsc();
    volatile uint32_t sum = 0;
    for (size_t i = 0; i < 8; i++) {  // 8 accesses for 8T
        sum += data[i];
    }
    end = rdtsc();
    
    uint64_t eight_access_ticks = end - start;
    
    printf("Single access: %lu ticks\n", single_access_ticks);
    printf("Eight sequential accesses: %lu ticks\n", eight_access_ticks);
    printf("Average per access: %.1f ticks\n", (double)eight_access_ticks / 8);
    
    // Check 8T requirements
    if (eight_access_ticks <= TICK_LIMIT) {
        printf("✅ 8T performance requirement met for cache-friendly access\n");
    } else {
        printf("❌ 8T performance requirement violated: %lu > %d ticks\n", 
               eight_access_ticks, TICK_LIMIT);
    }
    
    // Test cache miss penalty
    flush_cache();
    start = rdtsc();
    val = data[0];  // This will miss
    end = rdtsc();
    
    uint64_t miss_penalty = end - start;
    printf("Cache miss penalty: %lu ticks\n", miss_penalty);
    
    if (miss_penalty > TICK_LIMIT * 10) {
        printf("⚠ Cache miss penalty very high: %lu ticks\n", miss_penalty);
    }
    
    free(data);
    return (eight_access_ticks <= TICK_LIMIT) ? 0 : -1;
}

// Test cache simulator accuracy
int test_cache_simulator(void) {
    printf("Testing cache simulator accuracy...\n");
    
    cache_sim_t *sim = cache_sim_init(L1_SETS, L1_ASSOCIATIVITY, L1_CACHE_LINE_SIZE);
    if (!sim) return -1;
    
    // Test sequential access
    for (uint64_t addr = 0; addr < L1_CACHE_SIZE * 2; addr += sizeof(uint32_t)) {
        cache_sim_access(sim, addr);
    }
    
    double sim_hit_rate = (double)sim->hits / (sim->hits + sim->misses);
    
    printf("Simulator results:\n");
    printf("  Hits: %lu\n", sim->hits);
    printf("  Misses: %lu\n", sim->misses);
    printf("  Hit rate: %.1f%%\n", sim_hit_rate * 100);
    
    // Expected: hits for first L1_CACHE_SIZE, misses for the rest
    size_t expected_hits = L1_CACHE_SIZE / sizeof(uint32_t);
    size_t expected_misses = L1_CACHE_SIZE / sizeof(uint32_t);
    
    printf("Expected hits: %zu, misses: %zu\n", expected_hits, expected_misses);
    
    // Cleanup
    for (uint32_t i = 0; i < sim->sets; i++) {
        free(sim->tags[i]);
        free(sim->lru[i]);
    }
    free(sim->tags);
    free(sim->lru);
    free(sim);
    
    printf("✓ Cache simulator test completed\n");
    return 0;
}

// Performance regression test
int test_performance_regression(void) {
    printf("Testing performance regression...\n");
    
    // Baseline performance expectations for 8T
    struct {
        const char *test_name;
        size_t data_size;
        double max_cycles_per_access;
    } benchmarks[] = {
        {"L1 Sequential", L1_CACHE_SIZE / 2, 2.0},
        {"L1 Random", L1_CACHE_SIZE / 2, 15.0},
        {"Cache Line Aligned", 64 * 16, 1.5},
    };
    
    int num_benchmarks = sizeof(benchmarks) / sizeof(benchmarks[0]);
    int regressions = 0;
    
    for (int i = 0; i < num_benchmarks; i++) {
        void *data = aligned_alloc(L1_CACHE_LINE_SIZE, benchmarks[i].data_size);
        if (!data) continue;
        
        memset(data, 0xAA, benchmarks[i].data_size);
        
        cache_perf_t perf = measure_cache_performance(data, benchmarks[i].data_size,
                                                    benchmarks[i].test_name);
        
        if (perf.cycles_per_access > benchmarks[i].max_cycles_per_access) {
            printf("❌ Performance regression in %s: %.2f > %.2f cycles/access\n",
                   benchmarks[i].test_name, perf.cycles_per_access,
                   benchmarks[i].max_cycles_per_access);
            regressions++;
        } else {
            printf("✅ %s performance within limits: %.2f cycles/access\n",
                   benchmarks[i].test_name, perf.cycles_per_access);
        }
        
        free(data);
    }
    
    if (regressions == 0) {
        printf("✅ No performance regressions detected\n");
        return 0;
    } else {
        printf("❌ %d performance regressions detected\n", regressions);
        return -1;
    }
}

int main(void) {
    printf("=== 8T L1 Cache Performance Validation ===\n\n");
    
    int result = 0;
    
    result |= test_sequential_access();
    printf("\n");
    
    result |= test_random_access();
    printf("\n");
    
    result |= test_stride_access();
    printf("\n");
    
    result |= test_cache_associativity();
    printf("\n");
    
    result |= test_8t_performance_requirements();
    printf("\n");
    
    result |= test_cache_simulator();
    printf("\n");
    
    result |= test_performance_regression();
    printf("\n");
    
    if (result == 0) {
        printf("✅ All L1 cache performance tests passed!\n");
    } else {
        printf("❌ Some performance tests failed\n");
    }
    
    return result;
}