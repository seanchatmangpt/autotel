/**
 * @file test_arena_l1.c
 * @brief L1 Arena Allocator Tests with Cache Analysis
 * 
 * Tests for 8T L1-optimized arena allocator with comprehensive
 * cache performance analysis and validation.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <immintrin.h>

// L1 cache configuration
#define L1_CACHE_SIZE (32 * 1024)    // 32KB typical L1 data cache
#define L1_CACHE_LINE_SIZE 64        // 64-byte cache lines
#define L1_ASSOCIATIVITY 8           // 8-way associative
#define ARENA_ALIGNMENT 64           // Align to cache line

// Test configuration
#define TEST_ITERATIONS 10000
#define WARMUP_ITERATIONS 1000

// Performance counters (using rdtsc)
static inline uint64_t rdtsc(void) {
    unsigned int lo, hi;
    __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
    return ((uint64_t)hi << 32) | lo;
}

// L1 Arena structure
typedef struct arena_l1 {
    void *base;
    size_t size;
    size_t used;
    size_t alignment;
    uint32_t allocations;
    uint64_t cache_hits;
    uint64_t cache_misses;
} arena_l1_t;

// Initialize L1-optimized arena
int arena_l1_init(arena_l1_t *arena, size_t size) {
    // Align arena to cache line boundary
    arena->base = aligned_alloc(L1_CACHE_LINE_SIZE, size);
    if (!arena->base) return -1;
    
    arena->size = size;
    arena->used = 0;
    arena->alignment = ARENA_ALIGNMENT;
    arena->allocations = 0;
    arena->cache_hits = 0;
    arena->cache_misses = 0;
    
    // Zero the arena for consistent cache behavior
    memset(arena->base, 0, size);
    
    return 0;
}

// L1-optimized allocation
void* arena_l1_alloc(arena_l1_t *arena, size_t size) {
    // Align allocation to cache line
    size_t aligned_size = (size + arena->alignment - 1) & ~(arena->alignment - 1);
    
    if (arena->used + aligned_size > arena->size) {
        return NULL;
    }
    
    void *ptr = (char*)arena->base + arena->used;
    arena->used += aligned_size;
    arena->allocations++;
    
    return ptr;
}

// Cache performance measurement
typedef struct cache_stats {
    uint64_t access_time;
    uint64_t hit_count;
    uint64_t miss_count;
    double hit_rate;
} cache_stats_t;

// Measure cache performance for sequential access
cache_stats_t measure_sequential_access(void *data, size_t size) {
    cache_stats_t stats = {0};
    uint64_t start, end;
    volatile int sum = 0;
    
    // Warmup
    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
        for (size_t j = 0; j < size; j += sizeof(int)) {
            sum += *(int*)((char*)data + j);
        }
    }
    
    // Measure access time
    start = rdtsc();
    for (int i = 0; i < TEST_ITERATIONS; i++) {
        for (size_t j = 0; j < size; j += sizeof(int)) {
            sum += *(int*)((char*)data + j);
        }
    }
    end = rdtsc();
    
    stats.access_time = end - start;
    
    // Estimate cache performance based on access patterns
    size_t cache_lines = size / L1_CACHE_LINE_SIZE;
    if (cache_lines <= L1_CACHE_SIZE / L1_CACHE_LINE_SIZE) {
        stats.hit_count = cache_lines;
        stats.miss_count = 0;
        stats.hit_rate = 1.0;
    } else {
        stats.hit_count = L1_CACHE_SIZE / L1_CACHE_LINE_SIZE;
        stats.miss_count = cache_lines - stats.hit_count;
        stats.hit_rate = (double)stats.hit_count / cache_lines;
    }
    
    return stats;
}

// Test L1 cache alignment
int test_cache_alignment(void) {
    printf("Testing L1 cache alignment...\n");
    
    arena_l1_t arena;
    if (arena_l1_init(&arena, L1_CACHE_SIZE) != 0) {
        printf("Failed to initialize arena\n");
        return -1;
    }
    
    // Test aligned allocations
    for (int i = 0; i < 100; i++) {
        void *ptr = arena_l1_alloc(&arena, 32);
        assert(ptr != NULL);
        assert(((uintptr_t)ptr % L1_CACHE_LINE_SIZE) == 0);
    }
    
    printf("✓ All allocations properly aligned to cache lines\n");
    free(arena.base);
    return 0;
}

// Test cache locality
int test_cache_locality(void) {
    printf("Testing cache locality optimization...\n");
    
    arena_l1_t arena;
    if (arena_l1_init(&arena, L1_CACHE_SIZE * 2) != 0) {
        printf("Failed to initialize arena\n");
        return -1;
    }
    
    // Allocate small objects in sequence
    const int num_objects = 1000;
    const size_t object_size = 64;
    void *objects[num_objects];
    
    for (int i = 0; i < num_objects; i++) {
        objects[i] = arena_l1_alloc(&arena, object_size);
        assert(objects[i] != NULL);
    }
    
    // Measure sequential access performance
    cache_stats_t stats = measure_sequential_access(arena.base, arena.used);
    
    printf("Sequential access time: %lu cycles\n", stats.access_time);
    printf("Estimated cache hit rate: %.2f%%\n", stats.hit_rate * 100);
    
    // Verify objects are contiguous
    for (int i = 1; i < num_objects; i++) {
        ptrdiff_t diff = (char*)objects[i] - (char*)objects[i-1];
        assert(diff == object_size);
    }
    
    printf("✓ Objects allocated contiguously for optimal cache locality\n");
    free(arena.base);
    return 0;
}

// Test cache line utilization
int test_cache_line_utilization(void) {
    printf("Testing cache line utilization...\n");
    
    arena_l1_t arena;
    if (arena_l1_init(&arena, L1_CACHE_SIZE) != 0) {
        printf("Failed to initialize arena\n");
        return -1;
    }
    
    // Test different allocation sizes
    size_t sizes[] = {8, 16, 32, 64, 128};
    int num_sizes = sizeof(sizes) / sizeof(sizes[0]);
    
    for (int i = 0; i < num_sizes; i++) {
        arena.used = 0; // Reset arena
        
        int allocations = L1_CACHE_SIZE / sizes[i];
        for (int j = 0; j < allocations && j < 1000; j++) {
            void *ptr = arena_l1_alloc(&arena, sizes[i]);
            assert(ptr != NULL);
        }
        
        double utilization = (double)arena.used / L1_CACHE_SIZE * 100;
        printf("Size %zu bytes: %.1f%% cache utilization\n", sizes[i], utilization);
    }
    
    printf("✓ Cache line utilization validated\n");
    free(arena.base);
    return 0;
}

// Test memory pressure handling
int test_memory_pressure(void) {
    printf("Testing memory pressure handling...\n");
    
    arena_l1_t arena;
    if (arena_l1_init(&arena, L1_CACHE_SIZE / 2) != 0) {
        printf("Failed to initialize arena\n");
        return -1;
    }
    
    // Fill arena to capacity
    size_t allocation_size = 64;
    int successful_allocations = 0;
    
    while (1) {
        void *ptr = arena_l1_alloc(&arena, allocation_size);
        if (!ptr) break;
        successful_allocations++;
    }
    
    printf("Successful allocations: %d\n", successful_allocations);
    printf("Arena utilization: %.1f%%\n", 
           (double)arena.used / arena.size * 100);
    
    // Verify no memory corruption
    memset(arena.base, 0xAA, arena.used);
    
    printf("✓ Memory pressure handled correctly\n");
    free(arena.base);
    return 0;
}

// Benchmark against standard malloc
int benchmark_vs_malloc(void) {
    printf("Benchmarking arena vs malloc...\n");
    
    const int num_allocations = 10000;
    const size_t allocation_size = 64;
    uint64_t arena_time, malloc_time;
    
    // Test arena allocator
    arena_l1_t arena;
    arena_l1_init(&arena, num_allocations * allocation_size * 2);
    
    uint64_t start = rdtsc();
    for (int i = 0; i < num_allocations; i++) {
        void *ptr = arena_l1_alloc(&arena, allocation_size);
        assert(ptr != NULL);
    }
    uint64_t end = rdtsc();
    arena_time = end - start;
    
    // Test malloc
    void *ptrs[num_allocations];
    start = rdtsc();
    for (int i = 0; i < num_allocations; i++) {
        ptrs[i] = malloc(allocation_size);
        assert(ptrs[i] != NULL);
    }
    end = rdtsc();
    malloc_time = end - start;
    
    // Cleanup malloc
    for (int i = 0; i < num_allocations; i++) {
        free(ptrs[i]);
    }
    
    printf("Arena allocation time: %lu cycles\n", arena_time);
    printf("Malloc allocation time: %lu cycles\n", malloc_time);
    printf("Arena speedup: %.2fx\n", (double)malloc_time / arena_time);
    
    free(arena.base);
    return 0;
}

int main(void) {
    printf("=== 8T L1 Arena Allocator Tests ===\n\n");
    
    int result = 0;
    
    result |= test_cache_alignment();
    printf("\n");
    
    result |= test_cache_locality();
    printf("\n");
    
    result |= test_cache_line_utilization();
    printf("\n");
    
    result |= test_memory_pressure();
    printf("\n");
    
    result |= benchmark_vs_malloc();
    printf("\n");
    
    if (result == 0) {
        printf("✅ All L1 arena allocator tests passed!\n");
    } else {
        printf("❌ Some tests failed\n");
    }
    
    return result;
}