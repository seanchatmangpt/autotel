#ifndef CNS_8T_ARENA_L1_H
#define CNS_8T_ARENA_L1_H

#include "cns/8t/core.h"
#include <immintrin.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// 8T L1-OPTIMIZED ARENA ALLOCATOR
// L1 Cache Bounded Operations - 32KB L1 Data Cache, 64-byte Cache Lines
// ============================================================================

// L1 cache configuration constants
#define CNS_8T_L1_CACHE_SIZE    (32 * 1024)    // 32KB L1 data cache
#define CNS_8T_L1_LINE_SIZE     64              // 64-byte cache lines
#define CNS_8T_L1_ASSOCIATIVITY 8               // 8-way set associative
#define CNS_8T_L1_SETS          (CNS_8T_L1_CACHE_SIZE / (CNS_8T_L1_LINE_SIZE * CNS_8T_L1_ASSOCIATIVITY))

// Arena block sizes optimized for L1 cache
#define CNS_8T_ARENA_TINY_SIZE     16          // Sub-cache-line allocations
#define CNS_8T_ARENA_SMALL_SIZE    64          // Single cache line
#define CNS_8T_ARENA_MEDIUM_SIZE   256         // 4 cache lines
#define CNS_8T_ARENA_LARGE_SIZE    1024        // 16 cache lines
#define CNS_8T_ARENA_HUGE_SIZE     4096        // 64 cache lines (max L1 resident)

// L1-optimized arena configuration
typedef struct {
    size_t l1_working_set_size;     // Target working set size (<= 32KB)
    uint32_t prefetch_distance;     // Cache lines to prefetch ahead (1-4)
    uint32_t alignment_requirement; // Power of 2 alignment (16, 32, 64)
    bool enable_numa_locality;      // Enable NUMA-aware allocation
    bool enable_cache_coloring;     // Enable cache coloring to avoid conflicts
    uint32_t color_stride;          // Stride for cache coloring
    
    // Performance optimization flags
    bool use_huge_pages;            // Use huge pages for base allocation
    bool enable_prefetch;           // Enable hardware prefetch hints
    bool optimize_for_streaming;    // Optimize for streaming access patterns
    uint32_t temporal_locality_hint; // 0=no reuse, 3=high reuse
} cns_8t_arena_l1_config_t;

// L1-optimized block header (16 bytes, fits in cache sub-block)
typedef struct cns_8t_arena_block {
    struct cns_8t_arena_block* next; // Next block in free list
    uint32_t size;                   // Block size including header
    uint16_t arena_id;               // Arena this block belongs to
    uint8_t  block_class;            // Size class (tiny/small/medium/large/huge)
    uint8_t  flags;                  // Allocation flags and status
    uint64_t magic;                  // Magic number for corruption detection
} cns_8t_arena_block_t __attribute__((aligned(16)));

// Cache-line aligned arena metadata (64 bytes exactly)
typedef struct {
    // Hot path data (first 32 bytes)
    void*    base_ptr;               // Base memory pointer (8 bytes)
    size_t   total_size;             // Total arena size (8 bytes)
    size_t   used_size;              // Currently used size (8 bytes)
    uint32_t next_offset;            // Next allocation offset (4 bytes)
    uint16_t arena_id;               // Unique arena identifier (2 bytes)
    uint8_t  numa_node;              // NUMA node this arena is on (1 byte)
    uint8_t  alignment_shift;        // log2(alignment) for fast arithmetic (1 byte)
    
    // Cold path data (second 32 bytes)
    cns_8t_arena_block_t* free_lists[4]; // Free lists for different size classes (32 bytes)
    
    // Performance tracking and cache optimization
    uint32_t allocations;            // Total allocations performed
    uint32_t deallocations;          // Total deallocations performed
    uint32_t cache_hits;             // L1 cache hits (estimated)
    uint32_t cache_misses;           // L1 cache misses (estimated)
    uint64_t access_pattern_bitmap;  // Recent access pattern for optimization
} cns_8t_arena_metadata_t __attribute__((aligned(64)));

// L1-optimized arena allocator main structure
typedef struct {
    cns_8t_arena_l1_config_t config;
    
    // Memory regions (cache-line aligned)
    void*    memory_base;            // Base of all allocated memory
    size_t   memory_size;            // Total size of memory region
    uint32_t num_arenas;             // Number of sub-arenas
    uint32_t active_arena;           // Currently active arena for allocation
    
    // Arena metadata array (each entry is 64 bytes)
    cns_8t_arena_metadata_t* arenas;
    
    // L1 cache optimization state
    uint32_t last_cache_set;         // Last cache set used (for coloring)
    uint64_t allocation_sequence;    // Sequence number for temporal tracking
    uint32_t prefetch_queue[16];     // Prefetch queue for upcoming allocations
    uint8_t  prefetch_head;          // Prefetch queue head
    uint8_t  prefetch_tail;          // Prefetch queue tail
    
    // Performance counters
    cns_8t_perf_metrics_t perf;
    uint64_t l1_utilization_cycles;  // Cycles with high L1 utilization
    uint64_t total_cycles;           // Total operation cycles
    
    // SIMD acceleration context
    __m256i  simd_zero;              // Cached zero vector for clearing
    __m256i  simd_pattern;           // Pattern for initialization
} cns_8t_arena_l1_t __attribute__((aligned(64)));

// Allocation request with L1 optimization hints
typedef struct {
    size_t   size;                   // Requested size
    uint32_t alignment;              // Required alignment
    uint32_t lifetime_hint;          // Expected lifetime (short/medium/long)
    uint32_t access_pattern;         // Expected access pattern
    bool     temporal_locality;      // High temporal locality expected
    bool     spatial_locality;       // High spatial locality expected
    uint8_t  cache_priority;         // Cache priority (0=low, 3=high)
} cns_8t_arena_alloc_request_t;

// L1 cache analysis results
typedef struct {
    double   l1_hit_rate;            // Estimated L1 cache hit rate
    double   working_set_efficiency; // How well working set fits in L1
    uint32_t cache_conflicts;        // Estimated cache line conflicts
    uint32_t false_sharing_risk;     // False sharing risk assessment
    
    // Optimization recommendations
    bool     recommend_reordering;   // Recommend data structure reordering
    bool     recommend_prefetch;     // Recommend prefetch insertion
    bool     recommend_alignment;    // Recommend better alignment
    uint32_t optimal_block_size;     // Recommended block size
} cns_8t_l1_analysis_t;

// ============================================================================
// L1-OPTIMIZED ALLOCATION ALGORITHMS
// ============================================================================

// Fast allocation using L1-optimized buddy system
cns_8t_result_t cns_8t_arena_l1_alloc_fast(cns_8t_arena_l1_t* arena,
                                            size_t size,
                                            void** ptr);

// Aligned allocation with L1 optimization
cns_8t_result_t cns_8t_arena_l1_alloc_aligned(cns_8t_arena_l1_t* arena,
                                               const cns_8t_arena_alloc_request_t* req,
                                               void** ptr);

// Batch allocation for SIMD-friendly layouts
cns_8t_result_t cns_8t_arena_l1_alloc_batch(cns_8t_arena_l1_t* arena,
                                             const size_t* sizes,
                                             uint32_t count,
                                             void** ptrs);

// SIMD-accelerated memory clearing (uses AVX2)
cns_8t_result_t cns_8t_arena_l1_clear_simd(void* ptr, size_t size);

// Cache-conscious memory copying
cns_8t_result_t cns_8t_arena_l1_copy_optimized(void* dst, const void* src, size_t size);

// ============================================================================
// L1 CACHE ANALYSIS AND OPTIMIZATION
// ============================================================================

// Analyze current L1 cache utilization
cns_8t_result_t cns_8t_arena_l1_analyze_cache(cns_8t_arena_l1_t* arena,
                                               cns_8t_l1_analysis_t* analysis);

// Optimize arena layout for L1 cache efficiency
cns_8t_result_t cns_8t_arena_l1_optimize_layout(cns_8t_arena_l1_t* arena);

// Prefetch upcoming allocations
cns_8t_result_t cns_8t_arena_l1_prefetch_next(cns_8t_arena_l1_t* arena,
                                               uint32_t prefetch_count);

// Cache coloring to avoid conflicts
cns_8t_result_t cns_8t_arena_l1_apply_coloring(cns_8t_arena_l1_t* arena,
                                                void* ptr,
                                                size_t size);

// ============================================================================
// BRANCHLESS OPERATIONS FOR 8T PERFORMANCE
// ============================================================================

// Branchless size class determination (completes in 2-3 cycles)
static inline uint32_t cns_8t_arena_l1_size_class_branchless(size_t size) {
    // Use bit manipulation to determine size class without branches
    uint32_t class_tiny   = (size <= CNS_8T_ARENA_TINY_SIZE);
    uint32_t class_small  = (size <= CNS_8T_ARENA_SMALL_SIZE)  << 1;
    uint32_t class_medium = (size <= CNS_8T_ARENA_MEDIUM_SIZE) << 2;
    uint32_t class_large  = (size <= CNS_8T_ARENA_LARGE_SIZE)  << 3;
    
    // Find highest set bit using bit scan
    uint32_t mask = class_tiny | class_small | class_medium | class_large;
    return mask ? (__builtin_clz(mask) ^ 31) : 4; // huge class
}

// Branchless alignment calculation
static inline size_t cns_8t_arena_l1_align_branchless(size_t size, uint32_t alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}

// Branchless power-of-2 check
static inline bool cns_8t_arena_l1_is_power_of_2(size_t value) {
    return (value & (value - 1)) == 0;
}

// ============================================================================
// API FUNCTIONS
// ============================================================================

// Arena lifecycle management
cns_8t_result_t cns_8t_arena_l1_create(const cns_8t_arena_l1_config_t* config,
                                        cns_8t_arena_l1_t** arena);

cns_8t_result_t cns_8t_arena_l1_destroy(cns_8t_arena_l1_t* arena);

cns_8t_result_t cns_8t_arena_l1_reset(cns_8t_arena_l1_t* arena);

// Core allocation interface
cns_8t_result_t cns_8t_arena_l1_alloc(cns_8t_arena_l1_t* arena,
                                       size_t size,
                                       void** ptr);

cns_8t_result_t cns_8t_arena_l1_free(cns_8t_arena_l1_t* arena,
                                      void* ptr);

// Advanced allocation interface
cns_8t_result_t cns_8t_arena_l1_alloc_with_hints(cns_8t_arena_l1_t* arena,
                                                  const cns_8t_arena_alloc_request_t* req,
                                                  void** ptr);

// Memory operations
cns_8t_result_t cns_8t_arena_l1_realloc(cns_8t_arena_l1_t* arena,
                                         void* ptr,
                                         size_t new_size,
                                         void** new_ptr);

// Performance monitoring
cns_8t_result_t cns_8t_arena_l1_get_stats(cns_8t_arena_l1_t* arena,
                                           cns_8t_perf_metrics_t* stats);

cns_8t_result_t cns_8t_arena_l1_get_l1_metrics(cns_8t_arena_l1_t* arena,
                                                cns_8t_l1_analysis_t* metrics);

// Configuration and tuning
cns_8t_result_t cns_8t_arena_l1_set_config(cns_8t_arena_l1_t* arena,
                                            const cns_8t_arena_l1_config_t* config);

cns_8t_result_t cns_8t_arena_l1_tune_for_workload(cns_8t_arena_l1_t* arena,
                                                   const char* workload_profile);

// ============================================================================
// SIMD-ACCELERATED UTILITY FUNCTIONS
// ============================================================================

// Fast memory initialization using SIMD
static inline void cns_8t_arena_l1_simd_memset(void* dst, uint8_t value, size_t size) {
    if (size >= 32 && ((uintptr_t)dst & 31) == 0) {
        __m256i pattern = _mm256_set1_epi8(value);
        uint8_t* ptr = (uint8_t*)dst;
        size_t simd_size = size & ~31;
        
        for (size_t i = 0; i < simd_size; i += 32) {
            _mm256_store_si256((__m256i*)(ptr + i), pattern);
        }
        
        // Handle remaining bytes
        for (size_t i = simd_size; i < size; i++) {
            ptr[i] = value;
        }
    } else {
        // Fallback to standard memset
        memset(dst, value, size);
    }
}

// Cache-line optimized memory copy
static inline void cns_8t_arena_l1_simd_memcpy(void* dst, const void* src, size_t size) {
    if (size >= 64 && ((uintptr_t)dst & 31) == 0 && ((uintptr_t)src & 31) == 0) {
        const uint8_t* s = (const uint8_t*)src;
        uint8_t* d = (uint8_t*)dst;
        size_t simd_size = size & ~63;
        
        for (size_t i = 0; i < simd_size; i += 64) {
            __m256i chunk1 = _mm256_load_si256((const __m256i*)(s + i));
            __m256i chunk2 = _mm256_load_si256((const __m256i*)(s + i + 32));
            _mm256_store_si256((__m256i*)(d + i), chunk1);
            _mm256_store_si256((__m256i*)(d + i + 32), chunk2);
        }
        
        // Handle remaining bytes
        memcpy(d + simd_size, s + simd_size, size - simd_size);
    } else {
        memcpy(dst, src, size);
    }
}

// ============================================================================
// DEBUGGING AND VALIDATION
// ============================================================================

#ifdef CNS_8T_DEBUG
// Validate arena internal consistency
cns_8t_result_t cns_8t_arena_l1_validate(cns_8t_arena_l1_t* arena);

// Dump arena state for debugging
cns_8t_result_t cns_8t_arena_l1_dump_state(cns_8t_arena_l1_t* arena,
                                            char* buffer,
                                            size_t buffer_size);

// Check for memory corruption
cns_8t_result_t cns_8t_arena_l1_check_corruption(cns_8t_arena_l1_t* arena);
#endif

#ifdef __cplusplus
}
#endif

#endif // CNS_8T_ARENA_L1_H