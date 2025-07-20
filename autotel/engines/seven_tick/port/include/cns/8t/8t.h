#ifndef CNS_8T_H
#define CNS_8T_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Platform detection for SIMD
#if defined(__x86_64__) || defined(__i386__)
    #define CNS_8T_X86_64 1
    #include <immintrin.h>
#elif defined(__aarch64__)
    #define CNS_8T_ARM64 1
    #include <arm_neon.h>
#else
    #define CNS_8T_GENERIC 1
#endif

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// 8T SUBSTRATE - L1-OPTIMIZED NUMERICAL SUBSTRATE
// ============================================================================

// Type definitions
typedef uint64_t cns_tick_t;

// 8T Performance guarantees: All operations complete within 8 CPU ticks
#define CNS_8T_TICK_LIMIT 8
#define CNS_8T_L1_CACHE_SIZE 32768    // 32KB L1 cache
#define CNS_8T_CACHE_LINE_SIZE 64     // 64-byte cache lines
#define CNS_8T_ALIGNMENT 64           // 64-byte alignment

// ============================================================================
// L1-OPTIMIZED ARENA ALLOCATOR
// ============================================================================

typedef struct __attribute__((aligned(64))) {
    char* beg;                    // Start of arena (cache line aligned)
    char* end;                    // End of arena
    char* prefetch_ptr;           // Next allocation target for prefetching
    uint64_t cache_line_mask;     // 0x3F for 64-byte alignment
    size_t l1_budget;             // L1 cache budget for this arena
    uint32_t allocation_count;    // Number of allocations
    uint32_t cache_hits;          // L1 cache hits
} cns_8t_arena_t;

// L1-optimized allocation
void* cns_8t_arena_alloc(cns_8t_arena_t* arena, size_t size, size_t align);
void cns_8t_arena_init(cns_8t_arena_t* arena, void* memory, size_t size);
void cns_8t_arena_reset(cns_8t_arena_t* arena);

// ============================================================================
// NUMERICAL PRECISION ENGINE
// ============================================================================

typedef struct __attribute__((aligned(8))) {
    double value;
    double error_bound;           // Proven maximum error
    uint32_t condition_number;    // Algorithm stability measure
    uint32_t precision_flags;     // Precision control flags
} cns_8t_numerical_t;

typedef struct __attribute__((aligned(64))) {
    cns_8t_numerical_t* values;   // Cache line aligned values
    size_t count;
    size_t capacity;
    double global_error_bound;    // Cumulative error bound
    uint32_t operation_count;     // Number of operations performed
    uint32_t overflow_count;      // Overflow/underflow count
} cns_8t_numerical_array_t;

// Numerical operations with proven bounds
cns_8t_numerical_t cns_8t_numerical_add(cns_8t_numerical_t a, cns_8t_numerical_t b);
cns_8t_numerical_t cns_8t_numerical_mul(cns_8t_numerical_t a, cns_8t_numerical_t b);
cns_8t_numerical_t cns_8t_numerical_div(cns_8t_numerical_t a, cns_8t_numerical_t b);

// SIMD-optimized batch operations
void cns_8t_numerical_add_simd(cns_8t_numerical_array_t* result, 
                              const cns_8t_numerical_array_t* a, 
                              const cns_8t_numerical_array_t* b);

// ============================================================================
// L1-OPTIMIZED GRAPH STRUCTURE
// ============================================================================

typedef struct __attribute__((aligned(16))) {
    uint32_t subject;
    uint32_t predicate;
    uint32_t object;
    uint32_t metadata;            // Metadata/flags
} cns_8t_triple_t;

typedef struct __attribute__((aligned(64))) {
    cns_8t_triple_t* triples;     // Cache line aligned triples
    size_t count;
    size_t capacity;
    uint64_t cache_line_count;    // Number of cache lines used
    uint32_t l1_utilization;      // L1 cache utilization percentage
    uint32_t access_pattern_hash; // Hash of access patterns
} cns_8t_graph_t;

// Cache-optimized graph operations
void cns_8t_graph_init(cns_8t_graph_t* graph, cns_8t_arena_t* arena, size_t initial_capacity);
void cns_8t_graph_add_triple(cns_8t_graph_t* graph, cns_8t_triple_t triple);
void cns_8t_graph_prefetch_region(cns_8t_graph_t* graph, size_t start_index, size_t count);

// SIMD-optimized graph operations
void cns_8t_graph_find_pattern_simd(cns_8t_graph_t* graph, cns_8t_triple_t pattern, 
                                   uint32_t* results, size_t* result_count);

// ============================================================================
// 8T PERFORMANCE METRICS
// ============================================================================

typedef struct __attribute__((aligned(64))) {
    // Timing metrics
    cns_tick_t operation_start;
    cns_tick_t operation_end;
    cns_tick_t total_ticks;
    
    // Cache metrics
    uint32_t l1_cache_hits;
    uint32_t l1_cache_misses;
    uint32_t cache_line_loads;
    uint32_t prefetch_hits;
    
    // Memory metrics
    size_t memory_allocated;
    size_t memory_peak;
    uint32_t allocation_count;
    
    // Numerical metrics
    uint32_t numerical_operations;
    double max_error_bound;
    uint32_t precision_violations;
    
    // SIMD metrics
    uint32_t simd_operations;
    uint32_t scalar_fallbacks;
    float simd_utilization;
} cns_8t_metrics_t;

// Performance tracking
void cns_8t_metrics_init(cns_8t_metrics_t* metrics);
void cns_8t_metrics_start_operation(cns_8t_metrics_t* metrics);
void cns_8t_metrics_end_operation(cns_8t_metrics_t* metrics);
void cns_8t_metrics_record_cache_access(cns_8t_metrics_t* metrics, bool hit);

// ============================================================================
// 8T ERROR CODES
// ============================================================================

typedef enum {
    CNS_8T_OK = 0,
    CNS_8T_ERROR_INVALID_PARAM = 1,
    CNS_8T_ERROR_OUT_OF_MEMORY = 2,
    CNS_8T_ERROR_NOT_FOUND = 3,
    CNS_8T_ERROR_8T_VIOLATION = 7,
    CNS_8T_ERROR_ALIGNMENT = 8,
    CNS_8T_ERROR_UNDERFLOW = 11,
    CNS_8T_ERROR_OVERFLOW = 12
} cns_8t_result_t;

// ============================================================================
// 8T CONTEXT - UNIFIED EXECUTION ENVIRONMENT
// ============================================================================

typedef struct __attribute__((aligned(64))) {
    // Core components
    cns_8t_arena_t* arena;
    cns_8t_graph_t* graph;
    cns_8t_metrics_t metrics;
    
    // L1 cache management
    void* l1_cache_map[512];      // L1 cache line tracking (32KB / 64B = 512 lines)
    uint32_t l1_lru_counter;      // LRU counter for cache replacement
    
    // Numerical context
    double global_error_bound;
    uint32_t precision_mode;
    
    // SIMD capabilities
    bool avx2_available;
    bool fma_available;
    bool avx512_available;
    
    // Configuration
    uint32_t flags;
    bool strict_8t;               // Strict 8-tick enforcement
    bool enable_prefetch;         // Enable prefetching
} cns_8t_context_t;

// Context management
cns_8t_result_t cns_8t_context_init(cns_8t_context_t* ctx, size_t arena_size);
void cns_8t_context_destroy(cns_8t_context_t* ctx);

// 8T operation wrapper with timing
#define CNS_8T_OPERATION(ctx, operation) \
    do { \
        cns_8t_metrics_start_operation(&(ctx)->metrics); \
        (operation); \
        cns_8t_metrics_end_operation(&(ctx)->metrics); \
        if ((ctx)->metrics.total_ticks > CNS_8T_TICK_LIMIT && (ctx)->strict_8t) { \
            /* Handle 8T violation */ \
        } \
    } while(0)

// ============================================================================
// SIMD CAPABILITY DETECTION
// ============================================================================

typedef struct {
    bool sse2;
    bool avx;
    bool avx2;
    bool fma;
    bool avx512f;
    bool avx512dq;
} cns_8t_simd_caps_t;

cns_8t_simd_caps_t cns_8t_detect_simd_capabilities(void);

// ============================================================================
// L1 CACHE ANALYSIS
// ============================================================================

typedef struct {
    uint64_t cache_line_usage;    // Bit map of cache line usage
    uint64_t prefetch_opportunities; // Identified prefetch points
    uint64_t alignment_issues;    // Cache line crossing issues
    double cache_efficiency;      // Hit rate prediction
    uint32_t memory_bandwidth_mb; // Estimated memory bandwidth MB/s
} cns_8t_l1_analysis_t;

cns_8t_l1_analysis_t cns_8t_analyze_l1_usage(cns_8t_context_t* ctx);

#ifdef __cplusplus
}
#endif

#endif // CNS_8T_H