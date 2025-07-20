/*
 * test_arena_helper.h - Helper utilities for arena testing
 * 
 * Provides test framework and utilities for comprehensive arena validation
 */

#ifndef TEST_ARENA_HELPER_H
#define TEST_ARENA_HELPER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <string.h>

// Test framework macros
#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            printf("FAIL: %s (line %d)\n", message, __LINE__); \
            return 0; \
        } \
    } while(0)

#define TEST_PASS(message) \
    do { \
        printf("PASS: %s\n", message); \
        return 1; \
    } while(0)

#define RUN_TEST(test_func) \
    do { \
        printf("Running %s... ", #test_func); \
        fflush(stdout); \
        if (test_func()) { \
            tests_passed++; \
        } else { \
            tests_failed++; \
        } \
        tests_total++; \
    } while(0)

// Timing utilities
static inline uint64_t rdtsc() {
#if defined(__x86_64__) || defined(__i386__)
    uint64_t tsc;
    __asm__ volatile ("rdtsc" : "=A" (tsc));
    return tsc;
#else
    return 0; // Fallback for non-x86 platforms
#endif
}

static inline double get_time_seconds() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

// Memory pattern utilities
static inline void fill_pattern(void* ptr, size_t size, uint8_t pattern) {
    memset(ptr, pattern, size);
}

static inline int check_pattern(void* ptr, size_t size, uint8_t pattern) {
    uint8_t* bytes = (uint8_t*)ptr;
    for (size_t i = 0; i < size; i++) {
        if (bytes[i] != pattern) {
            return 0; // Pattern mismatch
        }
    }
    return 1; // Pattern intact
}

// Alignment utilities
static inline int is_aligned(void* ptr, size_t alignment) {
    return ((uintptr_t)ptr & (alignment - 1)) == 0;
}

static inline int is_power_of_two(size_t value) {
    return value && ((value & (value - 1)) == 0);
}

// Performance measurement
typedef struct {
    uint64_t start_cycles;
    uint64_t end_cycles;
    double start_time;
    double end_time;
} perf_measurement_t;

static inline perf_measurement_t start_measurement() {
    perf_measurement_t measurement;
    measurement.start_time = get_time_seconds();
    measurement.start_cycles = rdtsc();
    return measurement;
}

static inline void end_measurement(perf_measurement_t* measurement) {
    measurement->end_cycles = rdtsc();
    measurement->end_time = get_time_seconds();
}

static inline uint64_t get_cycles(const perf_measurement_t* measurement) {
    return measurement->end_cycles - measurement->start_cycles;
}

static inline double get_elapsed_time(const perf_measurement_t* measurement) {
    return measurement->end_time - measurement->start_time;
}

// Test result printing
static inline void print_test_summary(int total, int passed, int failed) {
    printf("\n=========================\n");
    printf("Test Results Summary:\n");
    printf("=========================\n");
    printf("Total:  %d\n", total);
    printf("Passed: %d (%s)\n", passed, passed == total ? "ALL" : "PARTIAL");
    printf("Failed: %d\n", failed);
    
    if (total > 0) {
        double success_rate = (100.0 * passed) / total;
        printf("Success Rate: %.1f%%\n", success_rate);
        
        if (success_rate == 100.0) {
            printf("Status: ✅ ALL TESTS PASSED\n");
        } else if (success_rate >= 90.0) {
            printf("Status: ⚠️  MOSTLY PASSED\n");
        } else {
            printf("Status: ❌ FAILURES DETECTED\n");
        }
    }
    printf("=========================\n");
}

// 7T compliance validation
#define MAX_7T_CYCLES 7

static inline int validate_7t_cycles(uint64_t cycles) {
    return cycles <= MAX_7T_CYCLES;
}

static inline void print_7t_status(uint64_t cycles) {
    if (validate_7t_cycles(cycles)) {
        printf("  ✅ 7T Compliant: %llu cycles\n", cycles);
    } else {
        printf("  ❌ 7T Violation: %llu cycles (limit: %d)\n", cycles, MAX_7T_CYCLES);
    }
}

// Stress test utilities
typedef struct {
    void** pointers;
    size_t* sizes;
    size_t count;
    size_t capacity;
} allocation_tracker_t;

static inline allocation_tracker_t* create_allocation_tracker(size_t capacity) {
    allocation_tracker_t* tracker = malloc(sizeof(allocation_tracker_t));
    if (!tracker) return NULL;
    
    tracker->pointers = malloc(capacity * sizeof(void*));
    tracker->sizes = malloc(capacity * sizeof(size_t));
    tracker->count = 0;
    tracker->capacity = capacity;
    
    if (!tracker->pointers || !tracker->sizes) {
        free(tracker->pointers);
        free(tracker->sizes);
        free(tracker);
        return NULL;
    }
    
    return tracker;
}

static inline void destroy_allocation_tracker(allocation_tracker_t* tracker) {
    if (tracker) {
        free(tracker->pointers);
        free(tracker->sizes);
        free(tracker);
    }
}

static inline int track_allocation(allocation_tracker_t* tracker, void* ptr, size_t size) {
    if (tracker->count >= tracker->capacity) {
        return 0; // Tracker full
    }
    
    tracker->pointers[tracker->count] = ptr;
    tracker->sizes[tracker->count] = size;
    tracker->count++;
    return 1;
}

#endif /* TEST_ARENA_HELPER_H */