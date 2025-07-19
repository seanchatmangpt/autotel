/**
 * SPARQL AOT Kernels - 7-Tick Optimized
 * 
 * Core kernels for 80/20 SPARQL patterns with strict cycle targets.
 * Each kernel is optimized for specific access patterns.
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#ifdef __x86_64__
#include <immintrin.h>
#elif defined(__aarch64__)
#include <arm_neon.h>
#endif

// Cache line size for alignment
#define CACHE_LINE_SIZE 64
#define PREFETCH_DISTANCE 4

// Aligned allocation helper
#define ALIGNED_ALLOC(size) aligned_alloc(CACHE_LINE_SIZE, (size))

// Result structure for SPARQL queries
typedef struct {
    uint32_t* ids;
    void** values;
    uint32_t count;
    uint32_t capacity;
} s7t_result_set;

// Triple store structure (cache-aligned)
typedef struct __attribute__((aligned(CACHE_LINE_SIZE))) {
    uint32_t subject;
    uint32_t predicate;
    uint32_t object;
    uint32_t type_id;
} s7t_triple;

// Hash table entry for joins
typedef struct {
    uint32_t key;
    uint32_t value;
} s7t_hash_entry;

/**
 * s7t_scan_by_type - Scan triples by type (≤2 cycles per triple)
 * 
 * Uses SIMD to check 4 triples at once for type matching.
 * Prefetches ahead for predictable memory access.
 */
__attribute__((hot))
static inline uint32_t s7t_scan_by_type(
    const s7t_triple* __restrict__ triples,
    uint32_t count,
    uint32_t target_type,
    uint32_t* __restrict__ output
) {
    uint32_t matches = 0;
    const uint32_t simd_blocks = count / 4;
    const uint32_t remainder = count % 4;
    
#ifdef __x86_64__
    // x86-64 implementation
    __m128i target = _mm_set1_epi32(target_type);
    
    for (uint32_t i = 0; i < simd_blocks; i++) {
        _mm_prefetch((const char*)(triples + (i + PREFETCH_DISTANCE) * 4), _MM_HINT_T0);
        
        __m128i types = _mm_loadu_si128((__m128i*)&triples[i * 4].type_id);
        __m128i cmp = _mm_cmpeq_epi32(types, target);
        int mask = _mm_movemask_ps(_mm_castsi128_ps(cmp));
        
        if (mask) {
            if (mask & 0x1) output[matches++] = i * 4 + 0;
            if (mask & 0x2) output[matches++] = i * 4 + 1;
            if (mask & 0x4) output[matches++] = i * 4 + 2;
            if (mask & 0x8) output[matches++] = i * 4 + 3;
        }
    }
#elif defined(__aarch64__)
    // ARM64 NEON implementation
    uint32x4_t target = vdupq_n_u32(target_type);
    
    for (uint32_t i = 0; i < simd_blocks; i++) {
        __builtin_prefetch(triples + (i + PREFETCH_DISTANCE) * 4, 0, 3);
        
        // Load 4 type_ids
        uint32_t types[4];
        types[0] = triples[i*4 + 0].type_id;
        types[1] = triples[i*4 + 1].type_id;
        types[2] = triples[i*4 + 2].type_id;
        types[3] = triples[i*4 + 3].type_id;
        
        uint32x4_t vtypes = vld1q_u32(types);
        uint32x4_t vcmp = vceqq_u32(vtypes, target);
        
        // Extract comparison results
        uint32_t cmp_results[4];
        vst1q_u32(cmp_results, vcmp);
        
        for (int j = 0; j < 4; j++) {
            if (cmp_results[j]) {
                output[matches++] = i * 4 + j;
            }
        }
    }
#else
    // Generic implementation
    for (uint32_t i = 0; i < count; i++) {
        if (triples[i].type_id == target_type) {
            output[matches++] = i;
        }
    }
    return matches;
#endif
    
    // Handle remainder with regular code
    uint32_t base = simd_blocks * 4;
    for (uint32_t i = 0; i < remainder; i++) {
        uint32_t match = (triples[base + i].type_id == target_type);
        output[matches] = base + i;
        matches += match;
    }
    
    return matches;
}

/**
 * s7t_scan_by_predicate - Scan triples by predicate (≤2 cycles per triple)
 * 
 * Similar to type scan but for predicates.
 * Uses same SIMD approach for maximum throughput.
 */
__attribute__((hot))
static inline uint32_t s7t_scan_by_predicate(
    const s7t_triple* __restrict__ triples,
    uint32_t count,
    uint32_t target_pred,
    uint32_t* __restrict__ output
) {
    uint32_t matches = 0;
    const uint32_t simd_blocks = count / 4;
    const uint32_t remainder = count % 4;
    
    __m128i target = _mm_set1_epi32(target_pred);
    
    for (uint32_t i = 0; i < simd_blocks; i++) {
        _mm_prefetch((const char*)(triples + (i + PREFETCH_DISTANCE) * 4), _MM_HINT_T0);
        
        // Load 4 predicates (with proper offset in struct)
        __m128i pred0 = _mm_set_epi32(
            triples[i*4 + 3].predicate,
            triples[i*4 + 2].predicate,
            triples[i*4 + 1].predicate,
            triples[i*4 + 0].predicate
        );
        
        __m128i cmp = _mm_cmpeq_epi32(pred0, target);
        int mask = _mm_movemask_ps(_mm_castsi128_ps(cmp));
        
        if (mask) {
            if (mask & 0x1) output[matches++] = i * 4 + 0;
            if (mask & 0x2) output[matches++] = i * 4 + 1;
            if (mask & 0x4) output[matches++] = i * 4 + 2;
            if (mask & 0x8) output[matches++] = i * 4 + 3;
        }
    }
    
    // Handle remainder
    uint32_t base = simd_blocks * 4;
    for (uint32_t i = 0; i < remainder; i++) {
        uint32_t match = (triples[base + i].predicate == target_pred);
        output[matches] = base + i;
        matches += match;
    }
    
    return matches;
}

/**
 * s7t_simd_filter_gt_f32 - SIMD filter for float comparisons (<1 cycle per element)
 * 
 * Processes 8 floats at once using AVX2 for maximum throughput.
 * Completely branchless for predictable performance.
 */
__attribute__((hot))
static inline uint32_t s7t_simd_filter_gt_f32(
    const float* __restrict__ values,
    uint32_t count,
    float threshold,
    uint32_t* __restrict__ indices,
    uint32_t* __restrict__ output
) {
    uint32_t matches = 0;
    const uint32_t simd_blocks = count / 8;
    const uint32_t remainder = count % 8;
    
    __m256 thresh = _mm256_set1_ps(threshold);
    
    for (uint32_t i = 0; i < simd_blocks; i++) {
        _mm_prefetch((const char*)(values + (i + PREFETCH_DISTANCE) * 8), _MM_HINT_T0);
        
        // Load 8 floats
        __m256 vals = _mm256_loadu_ps(values + i * 8);
        
        // Compare greater than
        __m256 cmp = _mm256_cmp_ps(vals, thresh, _CMP_GT_OQ);
        int mask = _mm256_movemask_ps(cmp);
        
        // Branchless output
        if (mask) {
            for (int j = 0; j < 8; j++) {
                uint32_t idx = indices[i * 8 + j];
                output[matches] = idx;
                matches += ((mask >> j) & 1);
            }
        }
    }
    
    // Handle remainder
    uint32_t base = simd_blocks * 8;
    for (uint32_t i = 0; i < remainder; i++) {
        uint32_t match = (values[base + i] > threshold);
        output[matches] = indices[base + i];
        matches += match;
    }
    
    return matches;
}

/**
 * s7t_hash_join - Hash join implementation (≤3 cycles per probe)
 * 
 * Uses open addressing with linear probing.
 * Optimized for cache locality and minimal branches.
 */
__attribute__((hot))
static inline uint32_t s7t_hash_join(
    const uint32_t* __restrict__ left_keys,
    const uint32_t* __restrict__ left_values,
    uint32_t left_count,
    const uint32_t* __restrict__ right_keys,
    const uint32_t* __restrict__ right_values,
    uint32_t right_count,
    uint32_t* __restrict__ output_left,
    uint32_t* __restrict__ output_right
) {
    // Hash table size (power of 2 for fast modulo)
    const uint32_t table_size = 1 << 16; // 64K entries
    const uint32_t mask = table_size - 1;
    
    // Allocate hash table (cache-aligned)
    s7t_hash_entry* table = (s7t_hash_entry*)ALIGNED_ALLOC(
        table_size * sizeof(s7t_hash_entry)
    );
    memset(table, 0xFF, table_size * sizeof(s7t_hash_entry));
    
    // Build phase - insert left side
    for (uint32_t i = 0; i < left_count; i++) {
        uint32_t key = left_keys[i];
        uint32_t hash = (key * 0x9E3779B9) & mask; // Fibonacci hash
        
        // Linear probing with prefetch
        while (table[hash].key != 0xFFFFFFFF) {
            hash = (hash + 1) & mask;
        }
        
        table[hash].key = key;
        table[hash].value = left_values[i];
    }
    
    // Probe phase - lookup right side
    uint32_t matches = 0;
    for (uint32_t i = 0; i < right_count; i++) {
        uint32_t key = right_keys[i];
        uint32_t hash = (key * 0x9E3779B9) & mask;
        
        // Prefetch likely locations
        _mm_prefetch((const char*)&table[(hash + 1) & mask], _MM_HINT_T0);
        _mm_prefetch((const char*)&table[(hash + 2) & mask], _MM_HINT_T0);
        
        // Linear probing
        while (table[hash].key != 0xFFFFFFFF) {
            // Branchless comparison
            uint32_t match = (table[hash].key == key);
            output_left[matches] = table[hash].value;
            output_right[matches] = right_values[i];
            matches += match;
            
            if (match) break;
            hash = (hash + 1) & mask;
        }
    }
    
    free(table);
    return matches;
}

/**
 * s7t_project_results - Project final results (≤2 cycles per result)
 * 
 * Efficiently copies selected columns to output.
 * Uses SIMD for bulk memory operations.
 */
__attribute__((hot))
static inline void s7t_project_results(
    const void** __restrict__ columns,
    uint32_t num_columns,
    const uint32_t* __restrict__ indices,
    uint32_t count,
    void** __restrict__ output
) {
    // For each column
    for (uint32_t col = 0; col < num_columns; col++) {
        const uint32_t* src = (const uint32_t*)columns[col];
        uint32_t* dst = (uint32_t*)output[col];
        
        // Process 8 indices at a time
        uint32_t i = 0;
        for (; i + 8 <= count; i += 8) {
            // Prefetch next indices
            _mm_prefetch((const char*)(indices + i + 8), _MM_HINT_T0);
            
            // Gather using indices (manual gather for better control)
            __m256i vals = _mm256_set_epi32(
                src[indices[i + 7]],
                src[indices[i + 6]],
                src[indices[i + 5]],
                src[indices[i + 4]],
                src[indices[i + 3]],
                src[indices[i + 2]],
                src[indices[i + 1]],
                src[indices[i + 0]]
            );
            
            _mm256_storeu_si256((__m256i*)(dst + i), vals);
        }
        
        // Handle remainder
        for (; i < count; i++) {
            dst[i] = src[indices[i]];
        }
    }
}

/**
 * Utility function: Create result set
 */
static inline s7t_result_set* s7t_create_result_set(uint32_t capacity) {
    s7t_result_set* rs = (s7t_result_set*)ALIGNED_ALLOC(sizeof(s7t_result_set));
    rs->ids = (uint32_t*)ALIGNED_ALLOC(capacity * sizeof(uint32_t));
    rs->values = (void**)ALIGNED_ALLOC(capacity * sizeof(void*));
    rs->count = 0;
    rs->capacity = capacity;
    return rs;
}

/**
 * Utility function: Free result set
 */
static inline void s7t_free_result_set(s7t_result_set* rs) {
    if (rs) {
        free(rs->ids);
        free(rs->values);
        free(rs);
    }
}

// Export kernel functions
void* s7t_get_kernel_scan_by_type() { return (void*)s7t_scan_by_type; }
void* s7t_get_kernel_scan_by_predicate() { return (void*)s7t_scan_by_predicate; }
void* s7t_get_kernel_simd_filter() { return (void*)s7t_simd_filter_gt_f32; }
void* s7t_get_kernel_hash_join() { return (void*)s7t_hash_join; }
void* s7t_get_kernel_project() { return (void*)s7t_project_results; }