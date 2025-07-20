/*
 * test_arena_corrected.c - Comprehensive unit tests for arena allocator
 * 
 * Tests 7T compliance and O(1) performance characteristics
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <assert.h>
#include <unistd.h>

// Include arena implementation
#include "../include/cns/arena.h"
#include "test_arena_helper.h"

// s7t_minimal.h should be included via arena.h

// Global test counters
static int tests_total = 0;
static int tests_passed = 0;
static int tests_failed = 0;

// Test arena creation and initialization
int test_arena_create() {
    cns_arena_t arena;
    uint8_t memory[1024];
    
    cns_arena_init(&arena, memory, sizeof(memory), CNS_ARENA_NONE);
    
    TEST_ASSERT(arena.beg == memory, "Arena beg should point to memory");
    TEST_ASSERT(arena.end == memory + sizeof(memory), "Arena end should be correct");
    TEST_ASSERT(arena.total_size == sizeof(memory), "Arena total_size should match");
    TEST_ASSERT(arena.alloc_count == 0, "Arena should start empty");
    TEST_ASSERT(arena.flags == CNS_ARENA_NONE, "Arena flags should be set");
    
    TEST_PASS("Arena creation test");
}

// Test basic allocation
int test_arena_alloc_basic() {
    cns_arena_t arena;
    uint8_t memory[1024];
    cns_arena_init(&arena, memory, sizeof(memory), CNS_ARENA_NONE);
    
    void* ptr1 = cns_arena_alloc(&arena, 64, 8);
    TEST_ASSERT(ptr1 != NULL, "First allocation should succeed");
    
    cns_arena_stats_t stats;
    cns_arena_get_stats(&arena, &stats);
    TEST_ASSERT(stats.used_size == 64, "Used memory should be 64 bytes");
    
    void* ptr2 = cns_arena_alloc(&arena, 32, 8);
    TEST_ASSERT(ptr2 != NULL, "Second allocation should succeed");
    TEST_ASSERT(ptr2 > ptr1, "Second pointer should be after first");
    
    cns_arena_get_stats(&arena, &stats);
    TEST_ASSERT(stats.used_size == 96, "Used memory should be 96 bytes");
    
    TEST_PASS("Basic allocation test");
}

// Test alignment calculations
int test_arena_alignment() {
    cns_arena_t arena;
    uint8_t memory[1024];
    cns_arena_init(&arena, memory, sizeof(memory), CNS_ARENA_NONE);
    
    // Test 8-byte alignment
    void* ptr1 = cns_arena_alloc(&arena, 1, 8);
    TEST_ASSERT(((uintptr_t)ptr1 % 8) == 0, "Pointer should be 8-byte aligned");
    
    void* ptr2 = cns_arena_alloc(&arena, 3, 8);
    TEST_ASSERT(((uintptr_t)ptr2 % 8) == 0, "Second pointer should be 8-byte aligned");
    
    // Test 16-byte alignment
    void* ptr3 = cns_arena_alloc(&arena, 1, 16);
    TEST_ASSERT(((uintptr_t)ptr3 % 16) == 0, "Pointer should be 16-byte aligned");
    
    TEST_PASS("Alignment test");
}

// Test capacity limits
int test_arena_capacity_limits() {
    cns_arena_t arena;
    uint8_t memory[64];
    cns_arena_init(&arena, memory, sizeof(memory), CNS_ARENA_NONE);
    
    // Fill arena to capacity
    void* ptr1 = cns_arena_alloc(&arena, 32, 8);
    TEST_ASSERT(ptr1 != NULL, "First allocation should succeed");
    
    void* ptr2 = cns_arena_alloc(&arena, 32, 8);
    TEST_ASSERT(ptr2 != NULL, "Second allocation should succeed");
    
    // Try to exceed capacity
    void* ptr3 = cns_arena_alloc(&arena, 1, 8);
    TEST_ASSERT(ptr3 == NULL, "Allocation beyond capacity should fail");
    
    TEST_PASS("Capacity limits test");
}

// Test reset functionality
int test_arena_reset() {
    cns_arena_t arena;
    uint8_t memory[1024];
    cns_arena_init(&arena, memory, sizeof(memory), CNS_ARENA_NONE);
    
    // Allocate some memory
    cns_arena_alloc(&arena, 100, 8);
    cns_arena_alloc(&arena, 200, 8);
    
    cns_arena_stats_t stats;
    cns_arena_get_stats(&arena, &stats);
    TEST_ASSERT(stats.used_size == 300, "Should have allocated 300 bytes");
    
    // Reset arena
    cns_arena_reset(&arena);
    cns_arena_get_stats(&arena, &stats);
    TEST_ASSERT(stats.used_size == 0, "Arena should be empty after reset");
    
    // Allocate again
    void* ptr = cns_arena_alloc(&arena, 64, 8);
    TEST_ASSERT(ptr == memory, "Allocation should start from beginning");
    
    TEST_PASS("Reset test");
}

// Test O(1) performance characteristics
int test_arena_o1_performance() {
    cns_arena_t arena;
    size_t capacity = 1024 * 1024; // 1MB
    uint8_t* memory = malloc(capacity);
    cns_arena_init(&arena, memory, capacity, CNS_ARENA_NONE);
    
    clock_t start, end;
    double cpu_time_used;
    
    // Time multiple allocations
    const int num_allocs = 1000;
    start = clock();
    
    for (int i = 0; i < num_allocs; i++) {
        void* ptr = cns_arena_alloc(&arena, 64, 8);
        if (ptr == NULL) break; // Arena full
    }
    
    end = clock();
    cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    // Each allocation should be extremely fast (< 1us on modern hardware)
    double avg_time_per_alloc = cpu_time_used / num_allocs;
    TEST_ASSERT(avg_time_per_alloc < 0.000001, "Each allocation should be sub-microsecond");
    
    printf("  Average allocation time: %.6f seconds\n", avg_time_per_alloc);
    
    free(memory);
    TEST_PASS("O(1) performance test");
}

// Test 7T (7-tick) compliance - allocation must complete within 7 CPU cycles
int test_7t_compliance() {
    cns_arena_t arena;
    uint8_t memory[1024];
    cns_arena_init(&arena, memory, sizeof(memory), CNS_ARENA_NONE);
    
    // Use inline assembly to measure CPU cycles
    uint64_t start_cycles, end_cycles;
    
    // Warm up cache
    for (int i = 0; i < 10; i++) {
        cns_arena_alloc(&arena, 8, 8);
    }
    cns_arena_reset(&arena);
    
    // Measure allocation cycles
    start_cycles = rdtsc();
    void* ptr = cns_arena_alloc(&arena, 64, 8);
    end_cycles = rdtsc();
    
    uint64_t cycles = end_cycles - start_cycles;
    
    TEST_ASSERT(ptr != NULL, "Allocation should succeed");
    TEST_ASSERT(cycles <= 7, "Allocation should complete within 7 cycles");
    
    printf("  Allocation took %llu cycles\n", cycles);
    
    TEST_PASS("7T compliance test");
}

// Test zero-allocation scenarios
int test_arena_zero_allocation() {
    cns_arena_t arena;
    uint8_t memory[1024];
    cns_arena_init(&arena, memory, sizeof(memory), CNS_ARENA_NONE);
    
    void* ptr = cns_arena_alloc(&arena, 0, 8);
    TEST_ASSERT(ptr == NULL, "Zero-size allocation should return NULL");
    
    cns_arena_stats_t stats;
    cns_arena_get_stats(&arena, &stats);
    TEST_ASSERT(stats.used_size == 0, "Zero-size allocation should not consume space");
    
    TEST_PASS("Zero allocation test");
}

// Test large allocation scenarios
int test_arena_large_allocation() {
    cns_arena_t arena;
    size_t capacity = 1024 * 1024; // 1MB
    uint8_t* memory = malloc(capacity);
    cns_arena_init(&arena, memory, capacity, CNS_ARENA_NONE);
    
    void* ptr = cns_arena_alloc(&arena, capacity - 64, 8); // Leave room for alignment
    TEST_ASSERT(ptr != NULL, "Large allocation should succeed");
    
    cns_arena_stats_t stats;
    cns_arena_get_stats(&arena, &stats);
    TEST_ASSERT(stats.used_size <= capacity, "Used memory should not exceed capacity");
    
    free(memory);
    TEST_PASS("Large allocation test");
}

// Test memory pattern integrity
int test_arena_memory_integrity() {
    cns_arena_t arena;
    uint8_t memory[1024];
    cns_arena_init(&arena, memory, sizeof(memory), CNS_ARENA_NONE);
    
    // Allocate and write pattern
    char* ptr1 = (char*)cns_arena_alloc(&arena, 64, 8);
    memset(ptr1, 0xAA, 64);
    
    char* ptr2 = (char*)cns_arena_alloc(&arena, 64, 8);
    memset(ptr2, 0xBB, 64);
    
    // Verify patterns are intact
    for (int i = 0; i < 64; i++) {
        TEST_ASSERT(ptr1[i] == (char)0xAA, "First allocation pattern should be intact");
        TEST_ASSERT(ptr2[i] == (char)0xBB, "Second allocation pattern should be intact");
    }
    
    TEST_PASS("Memory integrity test");
}

// Test concurrent allocation safety (single-threaded simulation)
int test_arena_allocation_safety() {
    cns_arena_t arena;
    uint8_t memory[1024];
    cns_arena_init(&arena, memory, sizeof(memory), CNS_ARENA_NONE);
    
    void* ptrs[10];
    
    // Rapid allocations
    for (int i = 0; i < 10; i++) {
        ptrs[i] = cns_arena_alloc(&arena, 64, 8);
        TEST_ASSERT(ptrs[i] != NULL, "Allocation should succeed");
        
        // Verify no overlap with previous allocations
        for (int j = 0; j < i; j++) {
            uintptr_t addr1 = (uintptr_t)ptrs[i];
            uintptr_t addr2 = (uintptr_t)ptrs[j];
            TEST_ASSERT(addr1 != addr2, "Allocations should not overlap");
            TEST_ASSERT(addr1 >= addr2 + 64 || addr2 >= addr1 + 64, 
                       "Allocations should not overlap in memory");
        }
    }
    
    TEST_PASS("Allocation safety test");
}

// Main test runner
int main() {
    printf("Arena Allocator Test Suite\n");
    printf("=========================\n\n");
    
    RUN_TEST(test_arena_create);
    RUN_TEST(test_arena_alloc_basic);
    RUN_TEST(test_arena_alignment);
    RUN_TEST(test_arena_capacity_limits);
    RUN_TEST(test_arena_reset);
    RUN_TEST(test_arena_o1_performance);
    RUN_TEST(test_7t_compliance);
    RUN_TEST(test_arena_zero_allocation);
    RUN_TEST(test_arena_large_allocation);
    RUN_TEST(test_arena_memory_integrity);
    RUN_TEST(test_arena_allocation_safety);
    
    print_test_summary(tests_total, tests_passed, tests_failed);
    
    return tests_failed == 0 ? 0 : 1;
}