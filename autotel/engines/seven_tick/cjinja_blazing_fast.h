/**
 * @file cjinja_blazing_fast.h
 * @brief Blazing Fast CJinja - Sub-100ns Variable Substitution
 * @version 4.0.0 (Blazing Fast)
 * 
 * Aggressively optimized CJinja implementation targeting <100ns variable substitution.
 * Removes all overhead and uses the most direct approaches possible.
 * 
 * Performance Target: <100ns (vs 206ns original, 272ns hash table attempt)
 * 
 * Strategy:
 * - Direct array lookup (no hash table overhead)
 * - Inline everything
 * - Minimize memory allocations
 * - Zero-overhead string operations
 * - Branchless parsing
 * 
 * @author 7T Engine Team
 * @date 2024-01-15
 */

#ifndef CJINJA_BLAZING_FAST_H
#define CJINJA_BLAZING_FAST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// BLAZING FAST CONFIGURATION
// =============================================================================

#define CJINJA_VERSION_BLAZING "4.0.0"
#define MAX_VARIABLES 32        // Small, fast array
#define MAX_VAR_NAME_LEN 16     // Short names for speed
#define MAX_VAR_VALUE_LEN 64    // Short values for speed
#define INITIAL_BUFFER_SIZE 512 // Smaller initial buffer

// =============================================================================
// DIRECT ARRAY CONTEXT (NO HASH TABLE OVERHEAD)
// =============================================================================

typedef struct {
    char keys[MAX_VARIABLES][MAX_VAR_NAME_LEN];   // Fixed-size arrays
    char values[MAX_VARIABLES][MAX_VAR_VALUE_LEN]; // for maximum speed
    uint8_t key_lens[MAX_VARIABLES];               // Cache lengths
    uint8_t value_lens[MAX_VARIABLES];
    uint8_t count;                                 // Variable count
} CJinjaBlazingContext;

// =============================================================================
// PERFORMANCE STATISTICS
// =============================================================================

typedef struct {
    uint64_t total_renders;
    uint64_t avg_render_time_ns;
    uint64_t min_render_time_ns;
    uint64_t max_render_time_ns;
    uint64_t linear_searches;
} CJinjaBlazingStats;

typedef struct {
    uint64_t iterations;
    uint64_t avg_time_ns;
    uint64_t min_time_ns;
    uint64_t max_time_ns;
    double ops_per_second;
} CJinjaBlazingBenchmark;

// =============================================================================
// BLAZING FAST API
// =============================================================================

/**
 * @brief Create blazing fast context (zero malloc overhead)
 */
static inline CJinjaBlazingContext* cjinja_blazing_create_context(void) {
    CJinjaBlazingContext* ctx = calloc(1, sizeof(CJinjaBlazingContext));
    return ctx; // Single malloc, everything else is stack-speed
}

/**
 * @brief Destroy context
 */
static inline void cjinja_blazing_destroy_context(CJinjaBlazingContext* ctx) {
    free(ctx); // Single free
}

/**
 * @brief Set variable - direct array insertion (no hashing)
 */
static inline void cjinja_blazing_set_var(CJinjaBlazingContext* ctx, const char* key, const char* value) {
    if (!ctx || !key || !value || ctx->count >= MAX_VARIABLES) return;
    
    size_t key_len = strlen(key);
    size_t value_len = strlen(value);
    
    if (key_len >= MAX_VAR_NAME_LEN || value_len >= MAX_VAR_VALUE_LEN) return;
    
    // Check if variable exists (linear search is fast for small arrays)
    for (uint8_t i = 0; i < ctx->count; i++) {
        if (ctx->key_lens[i] == key_len && memcmp(ctx->keys[i], key, key_len) == 0) {
            // Update existing
            memcpy(ctx->values[i], value, value_len);
            ctx->values[i][value_len] = '\0';
            ctx->value_lens[i] = value_len;
            return;
        }
    }
    
    // Add new variable
    uint8_t idx = ctx->count++;
    memcpy(ctx->keys[idx], key, key_len);
    ctx->keys[idx][key_len] = '\0';
    memcpy(ctx->values[idx], value, value_len);
    ctx->values[idx][value_len] = '\0';
    ctx->key_lens[idx] = key_len;
    ctx->value_lens[idx] = value_len;
}

/**
 * @brief Get variable - direct array lookup (fastest possible)
 */
static inline const char* cjinja_blazing_get_var(CJinjaBlazingContext* ctx, const char* key, size_t key_len) {
    if (!ctx || !key) return NULL;
    
    // Linear search is fastest for small arrays (better cache locality than hash table)
    for (uint8_t i = 0; i < ctx->count; i++) {
        if (ctx->key_lens[i] == key_len && memcmp(ctx->keys[i], key, key_len) == 0) {
            return ctx->values[i];
        }
    }
    return NULL;
}

/**
 * @brief Blazing fast variable substitution - targeting <100ns
 */
char* cjinja_blazing_render(const char* template_str, CJinjaBlazingContext* ctx);

/**
 * @brief Run blazing fast benchmark
 */
CJinjaBlazingBenchmark cjinja_blazing_benchmark(size_t iterations);

/**
 * @brief Get performance statistics
 */
void cjinja_blazing_get_stats(CJinjaBlazingContext* ctx, CJinjaBlazingStats* stats);

/**
 * @brief Compare with all previous implementations
 */
void cjinja_blazing_benchmark_comparison(void);

#ifdef __cplusplus
}
#endif

#endif // CJINJA_BLAZING_FAST_H