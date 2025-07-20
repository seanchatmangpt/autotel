#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include "../src/lexer.h"
#include "../src/parser.h"

// Test result tracking
typedef struct {
    int total;
    int passed;
    int failed;
    int skipped;
    clock_t start_time;
    clock_t end_time;
} TestStats;

// Global test stats
extern TestStats g_test_stats;

// Test assertion macros
#define TEST_ASSERT(cond) do { \
    if (!(cond)) { \
        printf("    ❌ Assertion failed: %s\n", #cond); \
        printf("       at %s:%d\n", __FILE__, __LINE__); \
        return 0; \
    } \
} while(0)

#define TEST_ASSERT_EQ(actual, expected) do { \
    if ((actual) != (expected)) { \
        printf("    ❌ Assertion failed: %s == %s\n", #actual, #expected); \
        printf("       Expected: %d, Actual: %d\n", (int)(expected), (int)(actual)); \
        printf("       at %s:%d\n", __FILE__, __LINE__); \
        return 0; \
    } \
} while(0)

#define TEST_ASSERT_STR_EQ(actual, expected) do { \
    if (strcmp((actual), (expected)) != 0) { \
        printf("    ❌ String assertion failed\n"); \
        printf("       Expected: \"%s\"\n", (expected)); \
        printf("       Actual:   \"%s\"\n", (actual)); \
        printf("       at %s:%d\n", __FILE__, __LINE__); \
        return 0; \
    } \
} while(0)

#define TEST_ASSERT_NULL(ptr) do { \
    if ((ptr) != NULL) { \
        printf("    ❌ Assertion failed: %s is not NULL\n", #ptr); \
        printf("       at %s:%d\n", __FILE__, __LINE__); \
        return 0; \
    } \
} while(0)

#define TEST_ASSERT_NOT_NULL(ptr) do { \
    if ((ptr) == NULL) { \
        printf("    ❌ Assertion failed: %s is NULL\n", #ptr); \
        printf("       at %s:%d\n", __FILE__, __LINE__); \
        return 0; \
    } \
} while(0)

// Test running macros
#define RUN_TEST(test_func) do { \
    printf("  Running %s...\n", #test_func); \
    g_test_stats.total++; \
    if (test_func()) { \
        printf("    ✅ PASS\n"); \
        g_test_stats.passed++; \
    } else { \
        printf("    ❌ FAIL\n"); \
        g_test_stats.failed++; \
    } \
} while(0)

#define RUN_TEST_SUITE(suite_name, suite_func) do { \
    printf("\n🧪 Running %s Test Suite\n", suite_name); \
    printf("="); \
    for (int i = 0; i < strlen(suite_name) + 19; i++) printf("="); \
    printf("\n"); \
    suite_func(); \
} while(0)

#define SKIP_TEST(test_func, reason) do { \
    printf("  Skipping %s: %s\n", #test_func, reason); \
    g_test_stats.total++; \
    g_test_stats.skipped++; \
} while(0)

// Utility functions
static inline void init_test_stats() {
    g_test_stats.total = 0;
    g_test_stats.passed = 0;
    g_test_stats.failed = 0;
    g_test_stats.skipped = 0;
    g_test_stats.start_time = clock();
}

static inline void print_test_summary() {
    g_test_stats.end_time = clock();
    double elapsed = (double)(g_test_stats.end_time - g_test_stats.start_time) / CLOCKS_PER_SEC;
    
    printf("\n");
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("📊 Test Summary\n");
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("  Total:   %d\n", g_test_stats.total);
    printf("  ✅ Passed: %d (%.1f%%)\n", g_test_stats.passed, 
           g_test_stats.total > 0 ? (100.0 * g_test_stats.passed / g_test_stats.total) : 0);
    printf("  ❌ Failed: %d (%.1f%%)\n", g_test_stats.failed,
           g_test_stats.total > 0 ? (100.0 * g_test_stats.failed / g_test_stats.total) : 0);
    printf("  ⏭️  Skipped: %d\n", g_test_stats.skipped);
    printf("  ⏱️  Time: %.3f seconds\n", elapsed);
    printf("═══════════════════════════════════════════════════════════════\n");
    
    if (g_test_stats.failed == 0 && g_test_stats.passed > 0) {
        printf("\n🎉 All tests passed!\n");
    } else if (g_test_stats.failed > 0) {
        printf("\n⚠️  Some tests failed!\n");
    }
}

// Memory leak detection helpers
#ifdef DEBUG_MEMORY
static int g_alloc_count = 0;

#define TEST_MALLOC(size) test_malloc(size, __FILE__, __LINE__)
#define TEST_FREE(ptr) test_free(ptr, __FILE__, __LINE__)

static inline void* test_malloc(size_t size, const char* file, int line) {
    void* ptr = malloc(size);
    if (ptr) {
        g_alloc_count++;
        printf("[ALLOC] %p (%zu bytes) at %s:%d (total: %d)\n", ptr, size, file, line, g_alloc_count);
    }
    return ptr;
}

static inline void test_free(void* ptr, const char* file, int line) {
    if (ptr) {
        g_alloc_count--;
        printf("[FREE] %p at %s:%d (remaining: %d)\n", ptr, file, line, g_alloc_count);
        free(ptr);
    }
}

static inline void check_memory_leaks() {
    if (g_alloc_count != 0) {
        printf("\n⚠️  Memory leak detected: %d allocations not freed\n", g_alloc_count);
    } else {
        printf("\n✅ No memory leaks detected\n");
    }
}
#else
#define TEST_MALLOC(size) malloc(size)
#define TEST_FREE(ptr) free(ptr)
#define check_memory_leaks()
#endif

// File reading utility
static inline char* read_test_file(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        return NULL;
    }
    
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char* content = (char*)TEST_MALLOC(size + 1);
    if (!content) {
        fclose(file);
        return NULL;
    }
    
    size_t read = fread(content, 1, size, file);
    content[read] = '\0';
    fclose(file);
    
    return content;
}

// Token comparison utility
static inline int compare_tokens(Token* t1, Token* t2) {
    if (t1->type != t2->type) return 0;
    if (t1->line != t2->line) return 0;
    if (t1->column != t2->column) return 0;
    if (strcmp(t1->value, t2->value) != 0) return 0;
    return 1;
}

// Error checking utilities
static inline int has_error(ParseError* error) {
    return error && error->message[0] != '\0';
}

static inline void print_error(ParseError* error) {
    if (has_error(error)) {
        printf("    Error at line %d, column %d: %s\n", 
               error->line, error->column, error->message);
    }
}

// Performance measurement
static inline double measure_time(clock_t start, clock_t end) {
    return ((double)(end - start)) / CLOCKS_PER_SEC;
}

#define MEASURE_TIME(code, time_var) do { \
    clock_t start = clock(); \
    code; \
    clock_t end = clock(); \
    time_var = measure_time(start, end); \
} while(0)

#endif // TEST_UTILS_H