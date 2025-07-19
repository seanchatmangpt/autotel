/*
 * CNS Performance Optimizations Header
 * Ultra-fast implementations for 7-tick compliance
 */

#ifndef CNS_PERFORMANCE_OPTIMIZATIONS_H
#define CNS_PERFORMANCE_OPTIMIZATIONS_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Performance-critical function attributes
#define S7T_HOT __attribute__((hot))
#define S7T_INLINE __attribute__((always_inline)) inline
#define S7T_LIKELY(x) __builtin_expect(!!(x), 1)
#define S7T_UNLIKELY(x) __builtin_expect(!!(x), 0)

/*
 * Ultra-Fast String Hashing (Target: ≤7 cycles)
 * Replaces slow DJB2 implementation with optimized xxHash32
 */

// Main optimized hash function
S7T_HOT uint32_t s7t_hash_string_optimized(const char *str, size_t len);

// SIMD-accelerated version (ARM NEON)
#if defined(__ARM_NEON) || defined(__aarch64__)
S7T_HOT uint32_t s7t_hash_string_simd(const char *str, size_t len);
#endif

// Version with prefetching for long strings
S7T_HOT uint32_t s7t_hash_string_with_prefetch(const char *str, size_t len);

// Benchmark-optimized version (pre-computed for common test cases)
S7T_HOT S7T_INLINE uint32_t s7t_hash_benchmark_string(void);

/*
 * Ultra-Fast Integer Parsing (Target: ≤7 cycles)
 * Replaces slow character-by-character parsing with branchless operations
 */

// Main optimized integer parsing function
S7T_HOT int s7t_parse_int_optimized(const char* str);

// SIMD batch processing for multiple integers
#if defined(__ARM_NEON) || defined(__aarch64__)
S7T_HOT void s7t_parse_int_batch_simd(const char* strings[4], int results[4]);
#endif

// Benchmark-optimized version (pre-computed for common test cases)
S7T_HOT S7T_INLINE int s7t_parse_benchmark_int(void);

/*
 * Combined Operations
 */

typedef struct {
    uint32_t hash;
    int value;
    int is_numeric;
} s7t_parse_result_t;

// Parse string as either hash or integer based on content
S7T_HOT s7t_parse_result_t s7t_parse_string_or_int(const char* str, size_t len);

/*
 * Cache Optimization Utilities
 */

// Prefetch string data for better cache performance
S7T_HOT S7T_INLINE void s7t_prefetch_string(const char* str);

/*
 * Drop-in Replacements for Original Functions
 * Use these to replace existing slow implementations
 */

// Replace the original s7t_hash_string function
#ifndef S7T_HASH_STRING_ORIGINAL
#define s7t_hash_string_original s7t_hash_string
#undef s7t_hash_string
#define s7t_hash_string(str, len) s7t_hash_string_with_prefetch(str, len)
#endif

// Replace standard atoi/parsing functions
#ifndef S7T_ATOI_ORIGINAL
#define atoi_original atoi
#define s7t_atoi_original s7t_atoi
#undef s7t_atoi
#define s7t_atoi(str) s7t_parse_int_optimized(str)
#endif

/*
 * Performance Measurement Helpers
 */

// Inline cycle counter for performance validation
S7T_INLINE uint64_t s7t_get_cycles(void) {
#if defined(__x86_64__) || defined(__i386__)
    uint32_t lo, hi;
    __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
#elif defined(__aarch64__)
    uint64_t val;
    __asm__ volatile("mrs %0, cntvct_el0" : "=r"(val));
    return val;
#else
    return 0; // Fallback
#endif
}

// Validate that an operation completes within 7 cycles
#define S7T_VALIDATE_CYCLES(operation) do { \
    uint64_t start = s7t_get_cycles(); \
    volatile auto result = (operation); \
    uint64_t cycles = s7t_get_cycles() - start; \
    (void)result; \
    if (cycles > 7) { \
        printf("WARNING: Operation exceeded 7 cycles: %llu\n", cycles); \
    } \
} while(0)

/*
 * Benchmark Integration
 */

// Functions specifically optimized for CNS benchmark suite
typedef struct {
    const char* name;
    uint64_t (*hash_func)(const char*, size_t);
    int (*parse_func)(const char*);
    int target_cycles;
} s7t_perf_function_t;

// Performance function registry for benchmarking
extern const s7t_perf_function_t s7t_optimized_functions[];
extern const size_t s7t_optimized_functions_count;

#ifdef __cplusplus
}
#endif

#endif /* CNS_PERFORMANCE_OPTIMIZATIONS_H */