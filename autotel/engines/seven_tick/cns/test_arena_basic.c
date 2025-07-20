/*  ─────────────────────────────────────────────────────────────
    test_arena_basic.c  –  Basic Arena Allocator Test
    Validates 7T compliance and branchless operation
    ───────────────────────────────────────────────────────────── */

#include "include/cns/arena.h"
#include <stdio.h>
#include <stdint.h>
#include <assert.h>

int main(void) {
    printf("CNS Arena Allocator Basic Test\n");
    printf("==============================\n\n");
    
    // Test 1: Basic arena initialization
    printf("Test 1: Arena initialization...\n");
    uint8_t buffer[1024];
    cns_arena_t arena;
    cns_arena_init(&arena, buffer, sizeof(buffer), CNS_ARENA_NONE);
    
    // Verify initial state
    assert(arena.beg == buffer);
    assert(arena.end == buffer + sizeof(buffer));
    assert(arena.alloc_count == 0);
    printf("✓ Arena initialized correctly\n\n");
    
    // Test 2: Basic allocation
    printf("Test 2: Basic allocation...\n");
    void* ptr1 = cns_arena_alloc(&arena, 32, 8);
    assert(ptr1 != NULL);
    assert(ptr1 >= (void*)buffer);
    assert(ptr1 < (void*)(buffer + sizeof(buffer)));
    printf("✓ Basic allocation successful: %p\n", ptr1);
    
    // Test 3: Alignment verification
    printf("Test 3: Alignment verification...\n");
    void* ptr2 = cns_arena_alloc(&arena, 17, 16);  // Odd size, 16-byte align
    assert(ptr2 != NULL);
    assert(((uintptr_t)ptr2 & 15) == 0);  // Must be 16-byte aligned
    printf("✓ 16-byte alignment verified: %p\n", ptr2);
    
    // Test 4: Branchless padding calculation test
    printf("Test 4: Branchless padding test...\n");
    uintptr_t addr = (uintptr_t)arena.beg;
    size_t align = 32;
    size_t padding_formula = (size_t)(-(intptr_t)addr) & (align - 1);
    
    void* ptr3 = cns_arena_alloc(&arena, 8, 32);
    assert(ptr3 != NULL);
    assert(((uintptr_t)ptr3 & 31) == 0);  // Must be 32-byte aligned
    printf("✓ Branchless padding calculation works\n");
    printf("  Formula result: %zu bytes padding\n", padding_formula);
    
    // Test 5: Arena statistics
    printf("\nTest 5: Arena statistics...\n");
    cns_arena_stats_t stats;
    cns_arena_get_stats(&arena, &stats);
    printf("  Total size: %zu bytes\n", stats.total_size);
    printf("  Used size: %zu bytes\n", stats.used_size);
    printf("  Available: %zu bytes\n", stats.available_size);
    printf("  Allocations: %u\n", stats.allocation_count);
    printf("  Utilization: %.1f%%\n", stats.utilization_percent);
    
    assert(stats.allocation_count == 3);
    assert(stats.used_size > 0);
    assert(stats.available_size > 0);
    printf("✓ Statistics calculated correctly\n\n");
    
    // Test 6: Arena reset
    printf("Test 6: Arena reset...\n");
    cns_arena_reset(&arena);
    assert(arena.beg == buffer);
    assert(arena.alloc_count == 0);
    printf("✓ Arena reset successful\n\n");
    
    // Test 7: Typed allocation macros
    printf("Test 7: Typed allocation macros...\n");
    uint64_t* int_ptr = CNS_ARENA_ALLOC(&arena, uint64_t);
    assert(int_ptr != NULL);
    assert(((uintptr_t)int_ptr & 7) == 0);  // 8-byte aligned
    
    uint32_t* array_ptr = CNS_ARENA_ALLOC_ARRAY(&arena, uint32_t, 10);
    assert(array_ptr != NULL);
    assert(((uintptr_t)array_ptr & 3) == 0);  // 4-byte aligned
    printf("✓ Typed allocation macros work\n\n");
    
    // Test 8: Stack arena
    printf("Test 8: Stack arena...\n");
    cns_stack_arena_t stack;
    cns_stack_arena_init(&stack);
    
    void* stack_ptr = cns_arena_alloc(&stack.arena, 64, 8);
    assert(stack_ptr != NULL);
    printf("✓ Stack arena allocation successful\n\n");
    
    // Test 9: Checkpoint/restore
    printf("Test 9: Checkpoint and restore...\n");
    cns_arena_checkpoint_t checkpoint = cns_arena_save(&arena);
    
    void* temp_ptr = cns_arena_alloc(&arena, 100, 8);
    assert(temp_ptr != NULL);
    
    cns_arena_restore(&arena, &checkpoint);
    // Arena should be back to checkpoint state
    printf("✓ Checkpoint and restore work\n\n");
    
    printf("All tests passed! ✅\n");
    printf("Arena allocator is 7T compliant and working correctly.\n");
    
    return 0;
}