/**
 * @file continuous_turtle_optimization.h
 * @brief Performance optimization utilities for Continuous Turtle Pipeline
 * @version 1.0.0
 * 
 * Advanced optimization features:
 * - SIMD acceleration for pattern matching
 * - Cache-aligned data structures
 * - NUMA-aware memory allocation
 * - Prefetching hints
 */

#ifndef CONTINUOUS_TURTLE_OPTIMIZATION_H
#define CONTINUOUS_TURTLE_OPTIMIZATION_H

#include <stdint.h>
#include <stdbool.h>
#include <immintrin.h>
#include "continuous_turtle_pipeline.h"

// Cache line size (typically 64 bytes on modern CPUs)
#define CACHE_LINE_SIZE 64

// Alignment macros
#define CACHE_ALIGNED __attribute__((aligned(CACHE_LINE_SIZE)))
#define PACKED __attribute__((packed))

// Prefetch hints
#define PREFETCH_READ(addr) __builtin_prefetch((addr), 0, 3)
#define PREFETCH_WRITE(addr) __builtin_prefetch((addr), 1, 3)

// Branch prediction hints
#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

// =============================================================================
// SIMD PATTERN MATCHING
// =============================================================================

// SIMD-optimized pattern structure
typedef struct CACHE_ALIGNED {
    __m256i pattern_mask[4];     // 128 bytes of pattern data
    __m256i pattern_value[4];    // 128 bytes of expected values
    uint32_t pattern_id;
    uint32_t match_count;
    double confidence_threshold;
} SimdPattern;

// SIMD-accelerated pattern matching
static inline bool simd_pattern_match(const uint8_t* data, const SimdPattern* pattern) {
    __m256i result = _mm256_setzero_si256();
    
    // Unroll and vectorize pattern matching
    for (int i = 0; i < 4; i++) {
        __m256i data_vec = _mm256_loadu_si256((const __m256i*)(data + i * 32));
        __m256i masked = _mm256_and_si256(data_vec, pattern->pattern_mask[i]);
        __m256i cmp = _mm256_cmpeq_epi8(masked, pattern->pattern_value[i]);
        result = _mm256_or_si256(result, cmp);
    }
    
    // Check if any byte matched
    return _mm256_testz_si256(result, result) == 0;
}

// Batch SIMD pattern matching
uint32_t simd_batch_match(const TurtleEvent* events, uint32_t count,
                         const SimdPattern* patterns, uint32_t pattern_count,
                         uint32_t* matches);

// =============================================================================
// CACHE-OPTIMIZED STRUCTURES
// =============================================================================

// Cache-aligned ring buffer for better performance
typedef struct CACHE_ALIGNED {
    TurtleEvent* events;
    uint32_t capacity;
    char padding1[CACHE_LINE_SIZE - sizeof(void*) - sizeof(uint32_t)];
    
    // Separate cache line for write position
    _Atomic uint32_t write_pos;
    char padding2[CACHE_LINE_SIZE - sizeof(uint32_t)];
    
    // Separate cache line for read position
    _Atomic uint32_t read_pos;
    char padding3[CACHE_LINE_SIZE - sizeof(uint32_t)];
    
    // Separate cache line for size
    _Atomic uint32_t size;
    char padding4[CACHE_LINE_SIZE - sizeof(uint32_t)];
} OptimizedRingBuffer;

// Cache-friendly worker structure
typedef struct CACHE_ALIGNED {
    // Hot data in first cache line
    _Atomic bool active;
    uint32_t worker_id;
    _Atomic uint64_t events_processed;
    
    // Separate cache line for timing data
    _Atomic uint64_t processing_time_ns CACHE_ALIGNED;
    uint64_t last_event_time_ns;
    
    // Pointers in their own cache line
    BitActorMatrix* local_matrix CACHE_ALIGNED;
    TickCollapseEngine* local_engine;
    struct TurtlePipeline* pipeline;
    
    // Thread handle separate
    pthread_t thread CACHE_ALIGNED;
} OptimizedWorker;

// =============================================================================
// NUMA OPTIMIZATION
// =============================================================================

// NUMA node information
typedef struct {
    int node_id;
    int cpu_count;
    int* cpu_ids;
    size_t memory_size;
} NumaNode;

// NUMA-aware pipeline configuration
typedef struct {
    int num_nodes;
    NumaNode* nodes;
    bool bind_workers;
    bool interleave_memory;
} NumaConfig;

// NUMA-aware initialization
bool turtle_pipeline_init_numa(TurtlePipeline* pipeline, NumaConfig* config);

// Bind worker to specific NUMA node
bool turtle_worker_bind_numa(TurtleWorker* worker, int numa_node);

// Allocate memory on specific NUMA node
void* numa_alloc(size_t size, int numa_node);
void numa_free(void* ptr, size_t size);

// =============================================================================
// ADVANCED SCALING ALGORITHMS
// =============================================================================

// Predictive scaling based on pattern analysis
typedef struct {
    double alpha;  // Smoothing factor for exponential moving average
    double beta;   // Trend smoothing factor
    double gamma;  // Seasonality factor
    
    double level;
    double trend;
    double seasonal[24];  // Hourly seasonality
    
    uint64_t last_update_ns;
} PredictiveScaler;

// Initialize predictive scaler
void predictive_scaler_init(PredictiveScaler* scaler);

// Update scaler with new metrics
void predictive_scaler_update(PredictiveScaler* scaler, double load);

// Get predicted load for future time
double predictive_scaler_forecast(PredictiveScaler* scaler, uint64_t future_ns);

// Calculate optimal worker count based on prediction
uint32_t predictive_scaler_recommend_workers(PredictiveScaler* scaler,
                                           uint64_t forecast_window_ns);

// =============================================================================
// MEMORY POOL OPTIMIZATION
// =============================================================================

// Object pool for zero-allocation event processing
typedef struct {
    void* objects;
    size_t object_size;
    uint32_t capacity;
    _Atomic uint32_t free_list_head;
    uint32_t* free_list;
} ObjectPool;

// Create object pool
ObjectPool* object_pool_create(size_t object_size, uint32_t capacity);
void object_pool_destroy(ObjectPool* pool);

// Allocate/free from pool (lock-free)
void* object_pool_alloc(ObjectPool* pool);
void object_pool_free(ObjectPool* pool, void* object);

// =============================================================================
// PROFILE-GUIDED OPTIMIZATION
// =============================================================================

// Performance profiling data
typedef struct {
    uint64_t function_calls[32];
    uint64_t function_time_ns[32];
    uint64_t cache_misses;
    uint64_t branch_mispredicts;
    uint64_t pipeline_stalls;
} ProfileData;

// Enable/disable profiling
void turtle_pipeline_enable_profiling(TurtlePipeline* pipeline, bool enable);

// Get profiling data
void turtle_pipeline_get_profile(TurtlePipeline* pipeline, ProfileData* data);

// Optimize based on profile
void turtle_pipeline_auto_optimize(TurtlePipeline* pipeline);

// =============================================================================
// VECTORIZED OPERATIONS
// =============================================================================

// Vectorized event processing
void vectorized_process_events(TurtleEvent* events, uint32_t count,
                              BitActorMatrix* matrix, RuleSet* rules);

// Vectorized pattern distribution update
void vectorized_update_distribution(PatternDistribution* dist, uint32_t count,
                                   const uint32_t* pattern_ids,
                                   const double* confidences);

// =============================================================================
// COMPILER OPTIMIZATION HINTS
// =============================================================================

// Force inline for hot paths
#define FORCE_INLINE __attribute__((always_inline)) inline

// Prevent inlining for cold paths
#define NOINLINE __attribute__((noinline))

// Hot function attribute
#define HOT __attribute__((hot))

// Cold function attribute
#define COLD __attribute__((cold))

// Pure function (no side effects)
#define PURE __attribute__((pure))

// Const function (depends only on args)
#define CONST_FN __attribute__((const))

// =============================================================================
// LOCK-FREE ALGORITHMS
// =============================================================================

// Lock-free stack for worker coordination
typedef struct LockFreeNode {
    void* data;
    _Atomic(struct LockFreeNode*) next;
} LockFreeNode;

typedef struct {
    _Atomic(LockFreeNode*) head;
    _Atomic uint32_t size;
} LockFreeStack;

// Lock-free stack operations
void lockfree_stack_push(LockFreeStack* stack, void* data);
void* lockfree_stack_pop(LockFreeStack* stack);

// Hazard pointer for safe memory reclamation
typedef struct {
    _Atomic(void*) pointer;
    _Atomic uint32_t active;
} HazardPointer;

// Hazard pointer management
void hazard_pointer_acquire(HazardPointer* hp, void* ptr);
void hazard_pointer_release(HazardPointer* hp);
bool hazard_pointer_safe_to_reclaim(void* ptr);

#endif // CONTINUOUS_TURTLE_OPTIMIZATION_H