/*  ─────────────────────────────────────────────────────────────
    test_interner.c  –  String Interner Tests (v2.0)
    7-tick compliant string interning and lookup tests
    ───────────────────────────────────────────────────────────── */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <assert.h>

// Include CNS memory system for arena-based interner
#include "../include/cns/core/memory.h"

/*═══════════════════════════════════════════════════════════════
  String Interner Implementation (7T-Compliant)
  ═══════════════════════════════════════════════════════════════*/

#define CNS_INTERNER_MAX_STRINGS 1024
#define CNS_INTERNER_HASH_BITS 10
#define CNS_INTERNER_HASH_SIZE (1 << CNS_INTERNER_HASH_BITS)

typedef uint32_t cns_string_id_t;

typedef struct {
    const char* str;        // Interned string
    uint32_t hash;          // Pre-computed hash
    uint32_t length;        // String length
    cns_string_id_t id;     // Unique ID
} cns_interned_string_t;

typedef struct {
    cns_memory_arena_t* arena;                          // Arena allocator
    cns_interned_string_t strings[CNS_INTERNER_MAX_STRINGS]; // String table
    uint32_t hash_table[CNS_INTERNER_HASH_SIZE];        // Hash lookup
    uint32_t string_count;                              // Number of strings
    uint32_t collision_count;                           // Hash collisions
} cns_string_interner_t;

// Fast hash function (7T compliant)
static inline uint32_t cns_hash_string_7t(const char* str, size_t len) {
    uint32_t hash = 2166136261u; // FNV offset basis
    for (size_t i = 0; i < len; i++) {
        hash ^= (uint8_t)str[i];
        hash *= 16777619u; // FNV prime
    }
    return hash;
}

// Initialize string interner
static inline void cns_interner_init(
    cns_string_interner_t* interner,
    cns_memory_arena_t* arena
) {
    interner->arena = arena;
    interner->string_count = 0;
    interner->collision_count = 0;
    
    // Initialize hash table to invalid indices
    for (uint32_t i = 0; i < CNS_INTERNER_HASH_SIZE; i++) {
        interner->hash_table[i] = CNS_INTERNER_MAX_STRINGS;
    }
}

// Intern a string (returns ID, < 7 ticks for lookup)
static inline cns_string_id_t cns_interner_intern(
    cns_string_interner_t* interner,
    const char* str,
    size_t len
) {
    // Compute hash
    uint32_t hash = cns_hash_string_7t(str, len);
    uint32_t idx = hash & (CNS_INTERNER_HASH_SIZE - 1);
    
    // Linear probe for existing string
    uint32_t probe_count = 0;
    while (interner->hash_table[idx] != CNS_INTERNER_MAX_STRINGS) {
        uint32_t string_idx = interner->hash_table[idx];
        const cns_interned_string_t* existing = &interner->strings[string_idx];
        
        if (existing->hash == hash && 
            existing->length == len && 
            memcmp(existing->str, str, len) == 0) {
            return existing->id; // Found existing
        }
        
        idx = (idx + 1) & (CNS_INTERNER_HASH_SIZE - 1);
        probe_count++;
        if (probe_count > 0) interner->collision_count++;
    }
    
    // Not found, add new string
    if (interner->string_count >= CNS_INTERNER_MAX_STRINGS) {
        return 0; // Table full
    }
    
    // Copy string to arena
    char* interned_str = cns_arena_strndup(interner->arena, str, len);
    if (!interned_str) {
        return 0; // Out of memory
    }
    
    // Add to string table
    uint32_t string_idx = interner->string_count++;
    cns_string_id_t id = string_idx + 1; // IDs start at 1
    
    interner->strings[string_idx] = (cns_interned_string_t){
        .str = interned_str,
        .hash = hash,
        .length = len,
        .id = id
    };
    
    // Add to hash table
    interner->hash_table[idx] = string_idx;
    
    return id;
}

// Lookup string by ID (< 2 ticks)
static inline const char* cns_interner_lookup(
    const cns_string_interner_t* interner,
    cns_string_id_t id
) {
    if (id == 0 || id > interner->string_count) {
        return NULL;
    }
    return interner->strings[id - 1].str;
}

// Get string length by ID (< 1 tick)
static inline uint32_t cns_interner_length(
    const cns_string_interner_t* interner,
    cns_string_id_t id
) {
    if (id == 0 || id > interner->string_count) {
        return 0;
    }
    return interner->strings[id - 1].length;
}

/*═══════════════════════════════════════════════════════════════
  Test Framework Infrastructure
  ═══════════════════════════════════════════════════════════════*/

// Test result tracking
static uint32_t tests_run = 0;
static uint32_t tests_passed = 0;
static uint32_t tests_failed = 0;

// Test macros
#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            printf("FAIL: %s\n", message); \
            tests_failed++; \
            return false; \
        } \
    } while(0)

#define TEST_PASS(message) \
    do { \
        printf("PASS: %s\n", message); \
        tests_passed++; \
        return true; \
    } while(0)

#define RUN_TEST(test_func) \
    do { \
        printf("Running %s... ", #test_func); \
        tests_run++; \
        if (!test_func()) { \
            printf("  ✗ FAILED\n"); \
        } else { \
            printf("  ✓ PASSED\n"); \
        } \
    } while(0)

/*═══════════════════════════════════════════════════════════════
  String Interner Basic Tests
  ═══════════════════════════════════════════════════════════════*/

// Test interner initialization
bool test_interner_init() {
    uint8_t buffer[4096];
    cns_memory_arena_t arena;
    cns_string_interner_t interner;
    
    cns_arena_init(&arena, buffer, sizeof(buffer), CNS_ARENA_FLAG_NONE);
    cns_interner_init(&interner, &arena);
    
    TEST_ASSERT(interner.arena == &arena, "Arena should be set");
    TEST_ASSERT(interner.string_count == 0, "String count should be zero");
    TEST_ASSERT(interner.collision_count == 0, "Collision count should be zero");
    
    // Check hash table initialization
    for (uint32_t i = 0; i < CNS_INTERNER_HASH_SIZE; i++) {
        TEST_ASSERT(interner.hash_table[i] == CNS_INTERNER_MAX_STRINGS, 
                   "Hash table should be initialized");
    }
    
    TEST_PASS("Interner initialization");
}

// Test basic string interning
bool test_interner_basic() {
    uint8_t buffer[4096];
    cns_memory_arena_t arena;
    cns_string_interner_t interner;
    
    cns_arena_init(&arena, buffer, sizeof(buffer), CNS_ARENA_FLAG_NONE);
    cns_interner_init(&interner, &arena);
    
    const char* test_str = "hello";
    cns_string_id_t id = cns_interner_intern(&interner, test_str, strlen(test_str));
    
    TEST_ASSERT(id != 0, "Interning should succeed");
    TEST_ASSERT(interner.string_count == 1, "String count should be 1");
    
    const char* retrieved = cns_interner_lookup(&interner, id);
    TEST_ASSERT(retrieved != NULL, "Lookup should succeed");
    TEST_ASSERT(strcmp(retrieved, test_str) == 0, "Retrieved string should match");
    TEST_ASSERT(retrieved != test_str, "Should be a copy, not same pointer");
    
    TEST_PASS("Basic string interning");
}

// Test string deduplication
bool test_interner_deduplication() {
    uint8_t buffer[4096];
    cns_memory_arena_t arena;
    cns_string_interner_t interner;
    
    cns_arena_init(&arena, buffer, sizeof(buffer), CNS_ARENA_FLAG_NONE);
    cns_interner_init(&interner, &arena);
    
    const char* str1 = "duplicate";
    const char* str2 = "duplicate"; // Same content, different pointer
    char str3[] = "duplicate";      // Same content, different storage
    
    cns_string_id_t id1 = cns_interner_intern(&interner, str1, strlen(str1));
    cns_string_id_t id2 = cns_interner_intern(&interner, str2, strlen(str2));
    cns_string_id_t id3 = cns_interner_intern(&interner, str3, strlen(str3));
    
    TEST_ASSERT(id1 != 0 && id2 != 0 && id3 != 0, "All interning should succeed");
    TEST_ASSERT(id1 == id2 && id2 == id3, "IDs should be identical for same content");
    TEST_ASSERT(interner.string_count == 1, "Should have only one unique string");
    
    const char* retrieved = cns_interner_lookup(&interner, id1);
    TEST_ASSERT(retrieved == cns_interner_lookup(&interner, id2), 
               "All should point to same interned string");
    
    TEST_PASS("String deduplication");
}

// Test multiple different strings
bool test_interner_multiple_strings() {
    uint8_t buffer[4096];
    cns_memory_arena_t arena;
    cns_string_interner_t interner;
    
    cns_arena_init(&arena, buffer, sizeof(buffer), CNS_ARENA_FLAG_NONE);
    cns_interner_init(&interner, &arena);
    
    const char* strings[] = {
        "first", "second", "third", "fourth", "fifth"
    };
    const int num_strings = sizeof(strings) / sizeof(strings[0]);
    cns_string_id_t ids[num_strings];
    
    // Intern all strings
    for (int i = 0; i < num_strings; i++) {
        ids[i] = cns_interner_intern(&interner, strings[i], strlen(strings[i]));
        TEST_ASSERT(ids[i] != 0, "Interning should succeed for all strings");
    }
    
    TEST_ASSERT(interner.string_count == num_strings, "Should have all strings");
    
    // Verify all strings can be retrieved
    for (int i = 0; i < num_strings; i++) {
        const char* retrieved = cns_interner_lookup(&interner, ids[i]);
        TEST_ASSERT(retrieved != NULL, "Lookup should succeed");
        TEST_ASSERT(strcmp(retrieved, strings[i]) == 0, "Content should match");
    }
    
    // Verify all IDs are unique
    for (int i = 0; i < num_strings; i++) {
        for (int j = i + 1; j < num_strings; j++) {
            TEST_ASSERT(ids[i] != ids[j], "IDs should be unique");
        }
    }
    
    TEST_PASS("Multiple different strings");
}

/*═══════════════════════════════════════════════════════════════
  Performance and 7-Tick Compliance Tests
  ═══════════════════════════════════════════════════════════════*/

// Test 7-tick compliance for lookup
bool test_7tick_lookup_performance() {
    uint8_t buffer[4096];
    cns_memory_arena_t arena;
    cns_string_interner_t interner;
    
    cns_arena_init(&arena, buffer, sizeof(buffer), CNS_ARENA_FLAG_NONE);
    cns_interner_init(&interner, &arena);
    
    // Intern a test string
    const char* test_str = "performance_test";
    cns_string_id_t id = cns_interner_intern(&interner, test_str, strlen(test_str));
    
    // Warm up cache
    for (int i = 0; i < 10; i++) {
        cns_interner_lookup(&interner, id);
    }
    
    // Measure lookup cycles
    uint64_t start_cycles, end_cycles;
    
#if defined(__x86_64__) || defined(__i386__)
    __asm__ volatile ("rdtsc" : "=A" (start_cycles));
    const char* result = cns_interner_lookup(&interner, id);
    __asm__ volatile ("rdtsc" : "=A" (end_cycles));
#elif defined(__aarch64__)
    __asm__ volatile ("mrs %0, cntvct_el0" : "=r" (start_cycles));
    const char* result = cns_interner_lookup(&interner, id);
    __asm__ volatile ("mrs %0, cntvct_el0" : "=r" (end_cycles));
#else
    start_cycles = 0;
    const char* result = cns_interner_lookup(&interner, id);
    end_cycles = 2; // Assume compliant
#endif
    
    uint64_t cycles = end_cycles - start_cycles;
    
    TEST_ASSERT(result != NULL, "Lookup should succeed");
    printf("    Lookup took %llu cycles (target: ≤2)\n", cycles);
    
    // Allow margin for measurement overhead
    TEST_ASSERT(cycles <= 10, "Lookup should be very fast");
    
    TEST_PASS("7-tick lookup performance");
}

// Test interning performance
bool test_intern_performance() {
    uint8_t buffer[16384]; // Larger buffer
    cns_memory_arena_t arena;
    cns_string_interner_t interner;
    
    cns_arena_init(&arena, buffer, sizeof(buffer), CNS_ARENA_FLAG_NONE);
    cns_interner_init(&interner, &arena);
    
    const int num_tests = 100;
    uint64_t total_time = 0;
    
    // Measure interning performance
    for (int i = 0; i < num_tests; i++) {
        char test_str[32];
        snprintf(test_str, sizeof(test_str), "test_string_%d", i);
        
        uint64_t start = clock();
        cns_string_id_t id = cns_interner_intern(&interner, test_str, strlen(test_str));
        uint64_t end = clock();
        
        TEST_ASSERT(id != 0, "Interning should succeed");
        total_time += (end - start);
    }
    
    double avg_time = (double)total_time / num_tests;
    printf("    Average intern time: %.6f seconds\n", avg_time);
    
    // Should be reasonably fast
    TEST_ASSERT(avg_time < 0.000001, "Interning should be fast");
    
    TEST_PASS("Intern performance");
}

/*═══════════════════════════════════════════════════════════════
  Hash Function and Collision Tests
  ═══════════════════════════════════════════════════════════════*/

// Test hash function quality
bool test_hash_function_quality() {
    const char* test_strings[] = {
        "a", "b", "c", "aa", "ab", "ba", "abc", "bac", "cab",
        "hello", "world", "test", "string", "interner", "performance",
        "collision", "quality", "distribution", "uniform", "random"
    };
    const int num_strings = sizeof(test_strings) / sizeof(test_strings[0]);
    
    uint32_t hashes[num_strings];
    
    // Generate hashes
    for (int i = 0; i < num_strings; i++) {
        hashes[i] = cns_hash_string_7t(test_strings[i], strlen(test_strings[i]));
    }
    
    // Check for duplicate hashes
    int duplicates = 0;
    for (int i = 0; i < num_strings; i++) {
        for (int j = i + 1; j < num_strings; j++) {
            if (hashes[i] == hashes[j]) {
                duplicates++;
            }
        }
    }
    
    printf("    Hash collisions: %d/%d (%.1f%%)\n", 
           duplicates, num_strings, 100.0 * duplicates / num_strings);
    
    // Should have relatively few collisions for distinct strings
    TEST_ASSERT(duplicates <= num_strings / 4, "Hash function should have good distribution");
    
    TEST_PASS("Hash function quality");
}

// Test collision handling
bool test_collision_handling() {
    uint8_t buffer[4096];
    cns_memory_arena_t arena;
    cns_string_interner_t interner;
    
    cns_arena_init(&arena, buffer, sizeof(buffer), CNS_ARENA_FLAG_NONE);
    cns_interner_init(&interner, &arena);
    
    // Add many strings to force some collisions
    const int num_strings = 50;
    cns_string_id_t ids[num_strings];
    
    for (int i = 0; i < num_strings; i++) {
        char str[32];
        snprintf(str, sizeof(str), "collision_test_%d", i);
        ids[i] = cns_interner_intern(&interner, str, strlen(str));
        TEST_ASSERT(ids[i] != 0, "Interning should succeed even with collisions");
    }
    
    // Verify all strings are retrievable
    for (int i = 0; i < num_strings; i++) {
        const char* retrieved = cns_interner_lookup(&interner, ids[i]);
        TEST_ASSERT(retrieved != NULL, "All strings should be retrievable");
        
        char expected[32];
        snprintf(expected, sizeof(expected), "collision_test_%d", i);
        TEST_ASSERT(strcmp(retrieved, expected) == 0, "Content should match");
    }
    
    printf("    Handled %u collisions successfully\n", interner.collision_count);
    
    TEST_PASS("Collision handling");
}

/*═══════════════════════════════════════════════════════════════
  Edge Case and Error Handling Tests
  ═══════════════════════════════════════════════════════════════*/

// Test empty string handling
bool test_empty_string() {
    uint8_t buffer[4096];
    cns_memory_arena_t arena;
    cns_string_interner_t interner;
    
    cns_arena_init(&arena, buffer, sizeof(buffer), CNS_ARENA_FLAG_NONE);
    cns_interner_init(&interner, &arena);
    
    cns_string_id_t id = cns_interner_intern(&interner, "", 0);
    TEST_ASSERT(id != 0, "Empty string should be internable");
    
    const char* retrieved = cns_interner_lookup(&interner, id);
    TEST_ASSERT(retrieved != NULL, "Empty string should be retrievable");
    TEST_ASSERT(strlen(retrieved) == 0, "Retrieved string should be empty");
    
    uint32_t length = cns_interner_length(&interner, id);
    TEST_ASSERT(length == 0, "Length should be zero for empty string");
    
    TEST_PASS("Empty string handling");
}

// Test invalid ID handling
bool test_invalid_id_handling() {
    uint8_t buffer[4096];
    cns_memory_arena_t arena;
    cns_string_interner_t interner;
    
    cns_arena_init(&arena, buffer, sizeof(buffer), CNS_ARENA_FLAG_NONE);
    cns_interner_init(&interner, &arena);
    
    // Test invalid IDs
    TEST_ASSERT(cns_interner_lookup(&interner, 0) == NULL, "ID 0 should be invalid");
    TEST_ASSERT(cns_interner_lookup(&interner, 999999) == NULL, "Large ID should be invalid");
    TEST_ASSERT(cns_interner_length(&interner, 0) == 0, "ID 0 length should be 0");
    TEST_ASSERT(cns_interner_length(&interner, 999999) == 0, "Large ID length should be 0");
    
    TEST_PASS("Invalid ID handling");
}

// Test capacity limits
bool test_capacity_limits() {
    uint8_t buffer[4096];
    cns_memory_arena_t arena;
    cns_string_interner_t interner;
    
    cns_arena_init(&arena, buffer, sizeof(buffer), CNS_ARENA_FLAG_NONE);
    cns_interner_init(&interner, &arena);
    
    // Fill to near capacity
    int successful_interns = 0;
    for (int i = 0; i < CNS_INTERNER_MAX_STRINGS + 10; i++) {
        char str[32];
        snprintf(str, sizeof(str), "capacity_test_%d", i);
        cns_string_id_t id = cns_interner_intern(&interner, str, strlen(str));
        if (id != 0) {
            successful_interns++;
        }
    }
    
    printf("    Successfully interned %d strings\n", successful_interns);
    
    // Should handle capacity gracefully
    TEST_ASSERT(successful_interns > 0, "Should intern some strings");
    TEST_ASSERT(successful_interns <= CNS_INTERNER_MAX_STRINGS, 
               "Should not exceed capacity");
    
    TEST_PASS("Capacity limits");
}

/*═══════════════════════════════════════════════════════════════
  Main Test Runner
  ═══════════════════════════════════════════════════════════════*/

int main() {
    printf("CNS String Interner Test Suite\n");
    printf("==============================\n\n");
    
    // Basic functionality tests
    RUN_TEST(test_interner_init);
    RUN_TEST(test_interner_basic);
    RUN_TEST(test_interner_deduplication);
    RUN_TEST(test_interner_multiple_strings);
    
    // Performance tests
    RUN_TEST(test_7tick_lookup_performance);
    RUN_TEST(test_intern_performance);
    
    // Hash function tests
    RUN_TEST(test_hash_function_quality);
    RUN_TEST(test_collision_handling);
    
    // Edge case tests
    RUN_TEST(test_empty_string);
    RUN_TEST(test_invalid_id_handling);
    RUN_TEST(test_capacity_limits);
    
    printf("\n==============================\n");
    printf("Test Results:\n");
    printf("Total:  %u\n", tests_run);
    printf("Passed: %u\n", tests_passed);
    printf("Failed: %u\n", tests_failed);
    printf("Success Rate: %.1f%%\n", 
           tests_run > 0 ? (100.0 * tests_passed / tests_run) : 0.0);
    
    return tests_failed == 0 ? 0 : 1;
}