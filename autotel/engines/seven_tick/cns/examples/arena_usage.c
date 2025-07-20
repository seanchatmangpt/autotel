/*  ─────────────────────────────────────────────────────────────
    examples/arena_usage.c  –  ARENAC Usage Example
    Demonstrates 7T-compliant arena allocation patterns
    ───────────────────────────────────────────────────────────── */

#include "cns/arena.h"
#include <stdio.h>
#include <string.h>

/*═══════════════════════════════════════════════════════════════
  Example 1: Basic Arena Usage
  ═══════════════════════════════════════════════════════════════*/

void example_basic_usage(void) {
    printf("=== Basic Arena Usage ===\n");
    
    // Create arena with system allocation
    arena_t* arena = arenac_create(1024 * 1024, ARENAC_FLAG_ZERO_ALLOC);
    if (!arena) {
        printf("Failed to create arena\n");
        return;
    }
    
    // Basic allocations
    int* numbers = ARENAC_NEW_ARRAY(arena, int, 100);
    char* text = arenac_strdup(arena, "Hello, ARENAC!");
    
    // Use allocations
    for (int i = 0; i < 10; i++) {
        numbers[i] = i * i;
    }
    
    printf("Allocated array of 100 integers\n");
    printf("Copied string: %s\n", text);
    printf("First 10 squares: ");
    for (int i = 0; i < 10; i++) {
        printf("%d ", numbers[i]);
    }
    printf("\n");
    
    // Get arena info
    arenac_info_t info;
    arenac_get_info(arena, &info);
    printf("Arena usage: %zu/%zu bytes (%.1f%%)\n", 
           info.used_size, info.total_size, info.utilization);
    
    arenac_destroy(arena);
    printf("\n");
}

/*═══════════════════════════════════════════════════════════════
  Example 2: Checkpoint and Restore
  ═══════════════════════════════════════════════════════════════*/

void example_checkpoint_restore(void) {
    printf("=== Checkpoint and Restore ===\n");
    
    // Create arena with pre-allocated buffer
    uint8_t buffer[256 * 1024];
    arena_t arena;
    arenac_init(&arena, buffer, sizeof(buffer), ARENAC_FLAG_NONE);
    
    // Create checkpoint
    arenac_checkpoint_t checkpoint1 = arenac_checkpoint(&arena);
    
    // Allocate some temporary data
    char* temp1 = arenac_strdup(&arena, "Temporary string 1");
    char* temp2 = arenac_strdup(&arena, "Temporary string 2");
    
    printf("Allocated temporary strings: '%s', '%s'\n", temp1, temp2);
    
    // Create another checkpoint
    arenac_checkpoint_t checkpoint2 = arenac_checkpoint(&arena);
    
    // Allocate more temporary data
    int* temp_array = ARENAC_NEW_ARRAY(&arena, int, 1000);
    printf("Allocated temporary array of 1000 integers\n");
    
    arenac_info_t info;
    arenac_get_info(&arena, &info);
    printf("Arena usage after temps: %zu bytes\n", info.used_size);
    
    // Restore to checkpoint 2 (keeps strings, removes array)
    arenac_restore(&arena, &checkpoint2);
    arenac_get_info(&arena, &info);
    printf("Arena usage after restore to checkpoint 2: %zu bytes\n", info.used_size);
    
    // Restore to checkpoint 1 (removes everything)
    arenac_restore(&arena, &checkpoint1);
    arenac_get_info(&arena, &info);
    printf("Arena usage after restore to checkpoint 1: %zu bytes\n", info.used_size);
    
    printf("\n");
}

/*═══════════════════════════════════════════════════════════════
  Example 3: Multi-Zone Arena
  ═══════════════════════════════════════════════════════════════*/

void example_multi_zone(void) {
    printf("=== Multi-Zone Arena ===\n");
    
    // Create base arena
    uint8_t zone1[128 * 1024];
    arena_t arena;
    arenac_init(&arena, zone1, sizeof(zone1), ARENAC_FLAG_NONE);
    
    printf("Created base zone: %zu bytes\n", sizeof(zone1));
    
    // Add additional zones
    uint8_t zone2[64 * 1024];
    uint8_t zone3[32 * 1024];
    
    int zone2_id = arenac_add_zone(&arena, zone2, sizeof(zone2));
    int zone3_id = arenac_add_zone(&arena, zone3, sizeof(zone3));
    
    printf("Added zone 2 (%d): %zu bytes\n", zone2_id, sizeof(zone2));
    printf("Added zone 3 (%d): %zu bytes\n", zone3_id, sizeof(zone3));
    
    // Allocate from different zones
    char* data1 = arenac_strdup(&arena, "Data in zone 1");
    
    arenac_switch_zone(&arena, zone2_id);
    char* data2 = arenac_strdup(&arena, "Data in zone 2");
    
    arenac_switch_zone(&arena, zone3_id);
    char* data3 = arenac_strdup(&arena, "Data in zone 3");
    
    printf("Zone 1 data: %s\n", data1);
    printf("Zone 2 data: %s\n", data2);
    printf("Zone 3 data: %s\n", data3);
    
    arenac_info_t info;
    arenac_get_info(&arena, &info);
    printf("Total arena: %zu zones, %zu bytes used\n", 
           info.zone_count, info.used_size);
    
    printf("\n");
}

/*═══════════════════════════════════════════════════════════════
  Example 4: Performance Validation
  ═══════════════════════════════════════════════════════════════*/

void example_performance_validation(void) {
    printf("=== Performance Validation ===\n");
    
    arena_t* arena = arenac_create(2 * 1024 * 1024, ARENAC_FLAG_NONE);
    if (!arena) {
        printf("Failed to create arena\n");
        return;
    }
    
    // Run benchmark
    uint64_t avg_cycles = arenac_benchmark(arena, 10000);
    printf("Average allocation cycles: %llu\n", avg_cycles);
    
    // Validate 7-tick compliance
    int result = arenac_validate_7tick(arena);
    if (result == 0) {
        printf("✅ Arena is 7-tick compliant!\n");
    } else {
        printf("❌ Arena failed 7-tick validation (error: %d)\n", result);
    }
    
    // Validate arena integrity
    result = arenac_validate(arena);
    if (result == 0) {
        printf("✅ Arena structure is valid!\n");
    } else {
        printf("❌ Arena validation failed (error: %d)\n", result);
    }
    
    arenac_destroy(arena);
    printf("\n");
}

/*═══════════════════════════════════════════════════════════════
  Example 5: Typed Allocations
  ═══════════════════════════════════════════════════════════════*/

typedef struct {
    int id;
    char name[32];
    double value;
} test_record_t;

void example_typed_allocations(void) {
    printf("=== Typed Allocations ===\n");
    
    arena_t* arena = arenac_create(1024 * 1024, ARENAC_FLAG_ZERO_ALLOC);
    if (!arena) {
        printf("Failed to create arena\n");
        return;
    }
    
    // Allocate single record
    test_record_t* record = ARENAC_NEW(arena, test_record_t);
    record->id = 42;
    strcpy(record->name, "Test Record");
    record->value = 3.14159;
    
    printf("Single record: id=%d, name='%s', value=%.5f\n",
           record->id, record->name, record->value);
    
    // Allocate array of records
    test_record_t* records = ARENAC_NEW_ARRAY(arena, test_record_t, 100);
    for (int i = 0; i < 10; i++) {
        records[i].id = i;
        snprintf(records[i].name, sizeof(records[i].name), "Record %d", i);
        records[i].value = i * 1.5;
    }
    
    printf("Record array (first 5):\n");
    for (int i = 0; i < 5; i++) {
        printf("  [%d] id=%d, name='%s', value=%.1f\n",
               i, records[i].id, records[i].name, records[i].value);
    }
    
    // Allocate aligned data
    uint64_t* aligned_data = ARENAC_NEW_ALIGNED(arena, uint64_t, 64);
    *aligned_data = 0xDEADBEEFCAFEBABEULL;
    printf("64-byte aligned data: 0x%llx (address: %p)\n", 
           *aligned_data, (void*)aligned_data);
    
    arenac_destroy(arena);
    printf("\n");
}

/*═══════════════════════════════════════════════════════════════
  Main Example Runner
  ═══════════════════════════════════════════════════════════════*/

int main(void) {
    printf("ARENAC Usage Examples\n");
    printf("=====================\n\n");
    
    example_basic_usage();
    example_checkpoint_restore();
    example_multi_zone();
    example_performance_validation();
    example_typed_allocations();
    
    printf("All examples completed successfully!\n");
    return 0;
}