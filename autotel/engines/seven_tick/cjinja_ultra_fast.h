/**
 * @file cjinja_ultra_fast.h
 * @brief Ultra-Fast CJinja Template Engine - Sub-100ns Variable Substitution
 * @version 3.0.0 (Ultra-Fast Optimization)
 * 
 * Optimized CJinja implementation targeting <100ns variable substitution performance.
 * Uses hash tables, SIMD operations, and aggressive caching for maximum speed.
 * 
 * Performance Targets:
 * - Variable substitution: <100 ns (was 206 ns)
 * - Conditional rendering: <400 ns  
 * - Loop rendering: <5 μs
 * - Filter operations: <50 ns
 * 
 * Optimizations Applied:
 * - Hash table for O(1) variable lookup
 * - SIMD-optimized string operations
 * - Template pre-compilation and caching
 * - Branchless parsing algorithms
 * - Memory pool allocation
 * - Vectorized memcpy operations
 * 
 * @author 7T Engine Team
 * @date 2024-01-15
 */

#ifndef CJINJA_ULTRA_FAST_H
#define CJINJA_ULTRA_FAST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <immintrin.h>  // For SIMD operations

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// ULTRA-FAST CONFIGURATION
// =============================================================================

#define CJINJA_VERSION_ULTRAFAST "3.0.0"
#define HASH_TABLE_SIZE 256         // Must be power of 2 for fast modulo
#define HASH_TABLE_MASK 255         // (HASH_TABLE_SIZE - 1)
#define MAX_VARIABLE_NAME_LEN 64    // For stack allocation optimization
#define MAX_VARIABLE_VALUE_LEN 1024 // For stack allocation optimization
#define TEMPLATE_CACHE_SIZE 64      // Pre-compiled template cache
#define MEMORY_POOL_SIZE 65536      // 64KB memory pool
#define SIMD_ALIGNMENT 32           // AVX2 alignment

// =============================================================================
// HASH TABLE FOR O(1) VARIABLE LOOKUP
// =============================================================================

/**
 * @brief Hash table entry for ultra-fast variable lookup
 */
typedef struct CJinjaHashEntry {
    char key[MAX_VARIABLE_NAME_LEN];     // Variable name
    char value[MAX_VARIABLE_VALUE_LEN];  // Variable value
    uint32_t key_hash;                   // Pre-computed hash
    uint16_t key_len;                    // Key length
    uint16_t value_len;                  // Value length
    int type;                            // Variable type (0=string, 1=bool, 2=int)
    struct CJinjaHashEntry* next;        // Collision handling
} CJinjaHashEntry;

/**
 * @brief Ultra-fast hash table context
 */
typedef struct {
    CJinjaHashEntry* buckets[HASH_TABLE_SIZE];
    CJinjaHashEntry* pool;               // Memory pool for entries
    size_t pool_used;                    // Pool usage counter
    size_t total_variables;              // Variable count
    uint64_t lookup_count;               // Performance counter
    uint64_t collision_count;            // Collision counter
} CJinjaUltraContext;

// =============================================================================
// SIMD-OPTIMIZED TEMPLATE PARSING
// =============================================================================

/**
 * @brief Pre-compiled template segment
 */
typedef struct {
    enum {
        CJINJA_SEG_TEXT = 0,
        CJINJA_SEG_VARIABLE = 1,
        CJINJA_SEG_CONDITIONAL = 2,
        CJINJA_SEG_LOOP = 3
    } type;
    union {
        struct {
            const char* text;
            uint16_t length;
        } text_seg;
        struct {
            char var_name[MAX_VARIABLE_NAME_LEN];
            uint32_t var_hash;
            uint16_t var_len;
            bool has_filter;
            char filter_name[32];
        } var_seg;
    } data;
} CJinjaTemplateSegment;

/**
 * @brief Pre-compiled template for ultra-fast rendering
 */
typedef struct {
    CJinjaTemplateSegment* segments;
    size_t segment_count;
    size_t allocated_segments;
    char* original_template;
    uint32_t template_hash;
    uint64_t compile_time_ns;
    uint64_t usage_count;
} CJinjaCompiledTemplate;

/**
 * @brief Template cache for O(1) template lookup
 */
typedef struct {
    CJinjaCompiledTemplate templates[TEMPLATE_CACHE_SIZE];
    size_t cache_count;
    uint64_t cache_hits;
    uint64_t cache_misses;
} CJinjaTemplateCache;

/**
 * @brief Ultra-fast engine with SIMD optimizations
 */
typedef struct {
    CJinjaTemplateCache* template_cache;
    char* memory_pool;                   // Pre-allocated memory pool
    size_t memory_pool_used;
    bool simd_enabled;                   // SIMD capability flag
    uint64_t render_count;               // Performance counter
    uint64_t total_render_time_ns;       // Total render time
} CJinjaUltraEngine;

// =============================================================================
// ULTRA-FAST HASH FUNCTIONS
// =============================================================================

/**
 * @brief Ultra-fast hash function optimized for short strings
 * Uses FNV-1a with SIMD when possible
 */
static inline uint32_t cjinja_ultra_hash(const char* key, size_t len) {
    uint32_t hash = 2166136261U; // FNV offset basis
    
    #ifdef __AVX2__
    // SIMD-optimized hash for longer strings
    if (len >= 32) {
        __m256i hash_vec = _mm256_set1_epi32(hash);
        __m256i prime_vec = _mm256_set1_epi32(16777619U);
        
        size_t simd_len = len & ~31; // Round down to 32-byte boundary
        for (size_t i = 0; i < simd_len; i += 32) {
            __m256i data = _mm256_loadu_si256((__m256i*)(key + i));
            hash_vec = _mm256_xor_si256(hash_vec, data);
            hash_vec = _mm256_mullo_epi32(hash_vec, prime_vec);
        }
        
        // Extract hash from vector (simplified)
        uint32_t hashes[8];
        _mm256_storeu_si256((__m256i*)hashes, hash_vec);
        hash = hashes[0] ^ hashes[1] ^ hashes[2] ^ hashes[3] ^
               hashes[4] ^ hashes[5] ^ hashes[6] ^ hashes[7];
        
        // Process remaining bytes
        for (size_t i = simd_len; i < len; i++) {
            hash ^= (uint32_t)key[i];
            hash *= 16777619U; // FNV prime
        }
    } else
    #endif
    {
        // Standard FNV-1a for short strings
        for (size_t i = 0; i < len; i++) {
            hash ^= (uint32_t)key[i];
            hash *= 16777619U; // FNV prime
        }
    }
    
    return hash;
}

/**
 * @brief Fast hash with length hint for stack-allocated strings
 */
#define CJINJA_HASH_STACK(str) cjinja_ultra_hash(str, strlen(str))

// =============================================================================
// ULTRA-FAST API FUNCTIONS
// =============================================================================

/**
 * @brief Create ultra-fast context with hash table
 */
CJinjaUltraContext* cjinja_ultra_create_context(void);

/**
 * @brief Destroy ultra-fast context
 */
void cjinja_ultra_destroy_context(CJinjaUltraContext* ctx);

/**
 * @brief Create ultra-fast engine
 */
CJinjaUltraEngine* cjinja_ultra_create_engine(void);

/**
 * @brief Destroy ultra-fast engine
 */
void cjinja_ultra_destroy_engine(CJinjaUltraEngine* engine);

/**
 * @brief Ultra-fast variable setting with pre-computed hash
 */
void cjinja_ultra_set_var_fast(CJinjaUltraContext* ctx, const char* key, const char* value, uint32_t key_hash);

/**
 * @brief Ultra-fast variable setting (computes hash internally)
 */
void cjinja_ultra_set_var(CJinjaUltraContext* ctx, const char* key, const char* value);

/**
 * @brief Ultra-fast variable lookup - O(1) average case
 */
const char* cjinja_ultra_get_var_fast(CJinjaUltraContext* ctx, const char* key, uint32_t key_hash, size_t key_len);

/**
 * @brief Ultra-fast variable lookup (computes hash internally)
 */
const char* cjinja_ultra_get_var(CJinjaUltraContext* ctx, const char* key);

// =============================================================================
// ULTRA-FAST RENDERING FUNCTIONS
// =============================================================================

/**
 * @brief Ultra-fast variable substitution - Target <100ns
 * Uses hash table, SIMD, and aggressive optimizations
 */
char* cjinja_ultra_render_variables(const char* template_str, CJinjaUltraContext* ctx);

/**
 * @brief Ultra-fast pre-compiled template rendering
 * Pre-compiles template on first use, then uses O(1) lookup
 */
char* cjinja_ultra_render_compiled(CJinjaUltraEngine* engine, const char* template_str, CJinjaUltraContext* ctx);

/**
 * @brief SIMD-accelerated string copy with alignment
 */
void cjinja_ultra_memcpy_simd(char* dest, const char* src, size_t len);

/**
 * @brief Branchless variable substitution parser
 * Uses lookup tables and bit manipulation to avoid branches
 */
char* cjinja_ultra_render_branchless(const char* template_str, CJinjaUltraContext* ctx);

// =============================================================================
// TEMPLATE COMPILATION
// =============================================================================

/**
 * @brief Compile template for ultra-fast repeated rendering
 */
CJinjaCompiledTemplate* cjinja_ultra_compile_template(const char* template_str);

/**
 * @brief Render pre-compiled template
 */
char* cjinja_ultra_render_precompiled(CJinjaCompiledTemplate* template, CJinjaUltraContext* ctx);

/**
 * @brief Destroy compiled template
 */
void cjinja_ultra_destroy_template(CJinjaCompiledTemplate* template);

// =============================================================================
// PERFORMANCE MONITORING
// =============================================================================

/**
 * @brief Get ultra-fast performance statistics
 */
typedef struct {
    uint64_t total_renders;
    uint64_t avg_render_time_ns;
    uint64_t min_render_time_ns;
    uint64_t max_render_time_ns;
    uint64_t hash_lookups;
    uint64_t hash_collisions;
    double collision_rate;
    uint64_t cache_hits;
    uint64_t cache_misses;
    double cache_hit_rate;
    size_t memory_pool_used;
    double memory_efficiency;
} CJinjaUltraStats;

/**
 * @brief Get performance statistics
 */
void cjinja_ultra_get_stats(CJinjaUltraEngine* engine, CJinjaUltraContext* ctx, CJinjaUltraStats* stats);

/**
 * @brief Reset performance counters
 */
void cjinja_ultra_reset_stats(CJinjaUltraEngine* engine, CJinjaUltraContext* ctx);

// =============================================================================
// SIMD FEATURE DETECTION
// =============================================================================

/**
 * @brief Detect available SIMD features
 */
typedef struct {
    bool sse2_available;
    bool sse4_available;
    bool avx_available;
    bool avx2_available;
    bool avx512_available;
} CJinjaSIMDFeatures;

/**
 * @brief Detect SIMD capabilities
 */
CJinjaSIMDFeatures cjinja_ultra_detect_simd(void);

/**
 * @brief Enable/disable SIMD optimizations
 */
void cjinja_ultra_set_simd_enabled(CJinjaUltraEngine* engine, bool enabled);

// =============================================================================
// BENCHMARKING FUNCTIONS
// =============================================================================

/**
 * @brief Benchmark variable substitution performance
 */
typedef struct {
    uint64_t iterations;
    uint64_t total_time_ns;
    uint64_t avg_time_ns;
    uint64_t min_time_ns;
    uint64_t max_time_ns;
    double ops_per_second;
} CJinjaBenchmarkResult;

/**
 * @brief Run variable substitution benchmark
 */
CJinjaBenchmarkResult cjinja_ultra_benchmark_variables(size_t iterations);

/**
 * @brief Compare performance with standard implementation
 */
void cjinja_ultra_benchmark_comparison(void);

#ifdef __cplusplus
}
#endif

#endif // CJINJA_ULTRA_FAST_H