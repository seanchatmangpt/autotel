/**
 * @file cjinja_ultra_portable.h
 * @brief Ultra-Fast CJinja Template Engine - Portable Implementation
 * @version 3.0.0 (Ultra-Fast Portable)
 * 
 * Portable ultra-fast CJinja implementation targeting <100ns variable substitution.
 * Uses hash tables, optimized algorithms, and platform-agnostic optimizations.
 * 
 * Performance Targets:
 * - Variable substitution: <100 ns (was 206 ns)
 * - Hash table lookup: O(1) average case
 * - Memory-efficient operations
 * - Cache-friendly data structures
 * 
 * @author 7T Engine Team
 * @date 2024-01-15
 */

#ifndef CJINJA_ULTRA_PORTABLE_H
#define CJINJA_ULTRA_PORTABLE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// CONFIGURATION
// =============================================================================

#define CJINJA_VERSION_ULTRA "3.0.0"
#define HASH_TABLE_SIZE 256         // Must be power of 2
#define HASH_TABLE_MASK 255
#define MAX_VARIABLE_NAME_LEN 64
#define MAX_VARIABLE_VALUE_LEN 512
#define MEMORY_POOL_SIZE 32768      // 32KB pool

// =============================================================================
// HASH TABLE STRUCTURES
// =============================================================================

typedef struct CJinjaHashEntry {
    char key[MAX_VARIABLE_NAME_LEN];
    char value[MAX_VARIABLE_VALUE_LEN];
    uint32_t key_hash;
    uint16_t key_len;
    uint16_t value_len;
    struct CJinjaHashEntry* next;
} CJinjaHashEntry;

typedef struct {
    CJinjaHashEntry* buckets[HASH_TABLE_SIZE];
    CJinjaHashEntry* pool;
    size_t pool_used;
    size_t total_variables;
    uint64_t lookup_count;
    uint64_t collision_count;
} CJinjaUltraContext;

// =============================================================================
// PERFORMANCE STATISTICS
// =============================================================================

typedef struct {
    uint64_t total_renders;
    uint64_t avg_render_time_ns;
    uint64_t min_render_time_ns;
    uint64_t max_render_time_ns;
    uint64_t hash_lookups;
    uint64_t hash_collisions;
    double collision_rate;
    size_t memory_pool_used;
} CJinjaUltraStats;

typedef struct {
    uint64_t iterations;
    uint64_t total_time_ns;
    uint64_t avg_time_ns;
    uint64_t min_time_ns;
    uint64_t max_time_ns;
    double ops_per_second;
} CJinjaBenchmarkResult;

// =============================================================================
// API FUNCTIONS
// =============================================================================

/**
 * @brief Create ultra-fast context
 */
CJinjaUltraContext* cjinja_ultra_create_context(void);

/**
 * @brief Destroy context
 */
void cjinja_ultra_destroy_context(CJinjaUltraContext* ctx);

/**
 * @brief Ultra-fast hash function
 */
uint32_t cjinja_ultra_hash(const char* key, size_t len);

/**
 * @brief Set variable with pre-computed hash
 */
void cjinja_ultra_set_var_fast(CJinjaUltraContext* ctx, const char* key, const char* value, uint32_t key_hash);

/**
 * @brief Set variable (computes hash)
 */
void cjinja_ultra_set_var(CJinjaUltraContext* ctx, const char* key, const char* value);

/**
 * @brief Get variable with pre-computed hash
 */
const char* cjinja_ultra_get_var_fast(CJinjaUltraContext* ctx, const char* key, uint32_t key_hash, size_t key_len);

/**
 * @brief Get variable (computes hash)
 */
const char* cjinja_ultra_get_var(CJinjaUltraContext* ctx, const char* key);

/**
 * @brief Ultra-fast variable substitution
 */
char* cjinja_ultra_render_variables(const char* template_str, CJinjaUltraContext* ctx);

/**
 * @brief Optimized memory copy
 */
void cjinja_ultra_memcpy_fast(char* dest, const char* src, size_t len);

/**
 * @brief Get performance statistics
 */
void cjinja_ultra_get_stats(CJinjaUltraContext* ctx, CJinjaUltraStats* stats);

/**
 * @brief Reset performance counters
 */
void cjinja_ultra_reset_stats(CJinjaUltraContext* ctx);

/**
 * @brief Run performance benchmark
 */
CJinjaBenchmarkResult cjinja_ultra_benchmark_variables(size_t iterations);

/**
 * @brief Compare performance with baseline
 */
void cjinja_ultra_benchmark_comparison(void);

#ifdef __cplusplus
}
#endif

#endif // CJINJA_ULTRA_PORTABLE_H