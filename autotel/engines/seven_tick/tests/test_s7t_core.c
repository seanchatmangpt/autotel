#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../include/s7t.h"

// Test the core s7t.h functionality
void test_interning() {
    printf("\n=== Testing String Interning ===\n");
    
    s7t_intern_table_t table;
    s7t_intern_init(&table, 1024);
    
    // Test basic interning
    s7t_id_t id1 = s7t_intern(&table, "hello");
    s7t_id_t id2 = s7t_intern(&table, "world");
    s7t_id_t id3 = s7t_intern(&table, "hello"); // Should return same as id1
    
    printf("ID for 'hello': %u\n", id1);
    printf("ID for 'world': %u\n", id2);
    printf("ID for 'hello' again: %u (should be %u)\n", id3, id1);
    
    // Verify interning works
    if (id1 == id3) {
        printf("✓ String interning works correctly\n");
    } else {
        printf("✗ String interning failed\n");
    }
    
    // Test performance
    s7t_cycle_t cycles;
    S7T_MEASURE_CYCLES(cycles, {
        for (int i = 0; i < 1000; i++) {
            s7t_intern(&table, "test_string");
        }
    });
    printf("1000 intern operations: %llu cycles (%.2f cycles/op)\n", 
           (unsigned long long)cycles, cycles / 1000.0);
}

void test_bit_manipulation() {
    printf("\n=== Testing Bit Manipulation ===\n");
    
    s7t_mask_t mask = 0xAAAAAAAAAAAAAAAAULL;
    printf("Mask: 0x%016llx\n", (unsigned long long)mask);
    printf("Popcount: %u (should be 32)\n", s7t_popcount(mask));
    printf("Leading zeros: %u\n", s7t_clz(mask));
    printf("Trailing zeros: %u\n", s7t_ctz(mask));
    
    mask = 0x8000000000000000ULL;
    printf("\nMask: 0x%016llx\n", (unsigned long long)mask);
    printf("Lowest bit: 0x%016llx\n", (unsigned long long)s7t_lowest_bit(mask));
    
    // Test performance
    s7t_cycle_t cycles;
    volatile uint32_t result = 0;
    S7T_MEASURE_CYCLES(cycles, {
        for (int i = 0; i < 1000000; i++) {
            result += s7t_popcount(i);
        }
    });
    printf("1M popcount operations: %llu cycles (%.2f cycles/op)\n",
           (unsigned long long)cycles, cycles / 1000000.0);
}

void test_branch_free() {
    printf("\n=== Testing Branch-Free Operations ===\n");
    
    uint32_t a = 42, b = 37;
    printf("a=%u, b=%u\n", a, b);
    printf("min(a,b) = %u\n", s7t_min(a, b));
    printf("max(a,b) = %u\n", s7t_max(a, b));
    printf("select(true, a, b) = %u\n", s7t_select(true, a, b));
    printf("select(false, a, b) = %u\n", s7t_select(false, a, b));
    
    // Test performance vs branching
    s7t_cycle_t cycles_branchfree, cycles_branching;
    volatile uint32_t sum = 0;
    
    S7T_MEASURE_CYCLES(cycles_branchfree, {
        for (int i = 0; i < 1000000; i++) {
            sum += s7t_select(i & 1, a, b);
        }
    });
    
    S7T_MEASURE_CYCLES(cycles_branching, {
        for (int i = 0; i < 1000000; i++) {
            sum += (i & 1) ? a : b;
        }
    });
    
    printf("Branch-free select: %llu cycles (%.2f cycles/op)\n",
           (unsigned long long)cycles_branchfree, cycles_branchfree / 1000000.0);
    printf("Branching select: %llu cycles (%.2f cycles/op)\n",
           (unsigned long long)cycles_branching, cycles_branching / 1000000.0);
}

void test_arena_allocator() {
    printf("\n=== Testing Arena Allocator ===\n");
    
    // Create a 1MB arena
    size_t arena_size = 1024 * 1024;
    void *buffer = aligned_alloc(S7T_CACHE_LINE, arena_size);
    
    s7t_arena_t arena;
    s7t_arena_init(&arena, buffer, arena_size);
    
    // Test allocations
    void *p1 = s7t_arena_alloc(&arena, 64);
    void *p2 = s7t_arena_alloc(&arena, 128);
    void *p3 = s7t_arena_alloc(&arena, 256);
    
    printf("Allocated p1: %p\n", p1);
    printf("Allocated p2: %p (offset: %ld)\n", p2, (char*)p2 - (char*)p1);
    printf("Allocated p3: %p (offset: %ld)\n", p3, (char*)p3 - (char*)p2);
    printf("Arena used: %zu bytes\n", arena.used);
    
    // Test performance
    s7t_cycle_t cycles;
    S7T_MEASURE_CYCLES(cycles, {
        s7t_arena_reset(&arena);
        for (int i = 0; i < 10000; i++) {
            s7t_arena_alloc(&arena, 64);
        }
    });
    printf("10K allocations: %llu cycles (%.2f cycles/alloc)\n",
           (unsigned long long)cycles, cycles / 10000.0);
    
    free(buffer);
}

void test_atomics() {
    printf("\n=== Testing Lock-Free Atomics ===\n");
    
    volatile uint64_t counter = 0;
    
    // Test CAS
    bool success = s7t_cas(&counter, 0, 42);
    printf("CAS(0->42): %s, counter=%llu\n", 
           success ? "success" : "failed", (unsigned long long)counter);
    
    // Test fetch-add
    uint64_t old = s7t_fetch_add(&counter, 8);
    printf("Fetch-add(8): old=%llu, new=%llu\n",
           (unsigned long long)old, (unsigned long long)counter);
    
    // Test performance
    s7t_cycle_t cycles;
    S7T_MEASURE_CYCLES(cycles, {
        for (int i = 0; i < 1000000; i++) {
            s7t_fetch_add(&counter, 1);
        }
    });
    printf("1M atomic increments: %llu cycles (%.2f cycles/op)\n",
           (unsigned long long)cycles, cycles / 1000000.0);
}

void test_bitvec() {
    printf("\n=== Testing Bit Vectors ===\n");
    
    // Create bit vectors
    s7t_bitvec_t bv1 = {
        .words = calloc(16, sizeof(s7t_mask_t)),
        .num_words = 16,
        .capacity = 16 * 64
    };
    
    s7t_bitvec_t bv2 = {
        .words = calloc(16, sizeof(s7t_mask_t)),
        .num_words = 16,
        .capacity = 16 * 64
    };
    
    // Set some bits
    s7t_bitvec_set(&bv1, 10);
    s7t_bitvec_set(&bv1, 20);
    s7t_bitvec_set(&bv1, 30);
    
    s7t_bitvec_set(&bv2, 20);
    s7t_bitvec_set(&bv2, 30);
    s7t_bitvec_set(&bv2, 40);
    
    // Test operations
    printf("bv1[10] = %d\n", s7t_bitvec_test(&bv1, 10));
    printf("bv1[20] = %d\n", s7t_bitvec_test(&bv1, 20));
    printf("bv2[40] = %d\n", s7t_bitvec_test(&bv2, 40));
    
    // Test AND operation
    s7t_bitvec_t result = {
        .words = calloc(16, sizeof(s7t_mask_t)),
        .num_words = 16,
        .capacity = 16 * 64
    };
    memcpy(result.words, bv1.words, 16 * sizeof(s7t_mask_t));
    s7t_bitvec_and(&result, &bv2);
    
    printf("\nAfter AND:\n");
    printf("result[10] = %d (should be 0)\n", s7t_bitvec_test(&result, 10));
    printf("result[20] = %d (should be 1)\n", s7t_bitvec_test(&result, 20));
    printf("result[30] = %d (should be 1)\n", s7t_bitvec_test(&result, 30));
    printf("result[40] = %d (should be 0)\n", s7t_bitvec_test(&result, 40));
    
    // Cleanup
    free(bv1.words);
    free(bv2.words);
    free(result.words);
}

void test_simd_operations() {
#ifdef __SSE4_2__
    printf("\n=== Testing SIMD Operations (SSE) ===\n");
    
    // Create aligned data
    s7t_vec128_t *v1 = aligned_alloc(16, sizeof(s7t_vec128_t));
    s7t_vec128_t *v2 = aligned_alloc(16, sizeof(s7t_vec128_t));
    
    // Initialize with pattern
    uint64_t *p1 = (uint64_t*)v1;
    uint64_t *p2 = (uint64_t*)v2;
    p1[0] = 0xFFFFFFFF00000000ULL;
    p1[1] = 0x00000000FFFFFFFFULL;
    p2[0] = 0xAAAAAAAAAAAAAAAAULL;
    p2[1] = 0x5555555555555555ULL;
    
    // Test operations
    s7t_vec128_t result = s7t_vec128_and(*v1, *v2);
    uint64_t *pres = (uint64_t*)&result;
    printf("AND result: 0x%016llx 0x%016llx\n", 
           (unsigned long long)pres[0], (unsigned long long)pres[1]);
    
    // Test performance
    s7t_cycle_t cycles;
    S7T_MEASURE_CYCLES(cycles, {
        for (int i = 0; i < 1000000; i++) {
            result = s7t_vec128_or(result, *v1);
        }
    });
    printf("1M SIMD OR operations: %llu cycles (%.2f cycles/op)\n",
           (unsigned long long)cycles, cycles / 1000000.0);
    
    free(v1);
    free(v2);
#else
    printf("\n=== SIMD Not Available ===\n");
#endif
}

int main() {
    printf("S7T Core Library Test Suite\n");
    printf("===========================\n");
    
    test_interning();
    test_bit_manipulation();
    test_branch_free();
    test_arena_allocator();
    test_atomics();
    test_bitvec();
    test_simd_operations();
    
    printf("\nAll tests completed!\n");
    return 0;
}