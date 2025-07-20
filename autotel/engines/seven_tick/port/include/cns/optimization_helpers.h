#ifndef CNS_OPTIMIZATION_HELPERS_H
#define CNS_OPTIMIZATION_HELPERS_H

#include <stdint.h>
#include <stddef.h>
#include <stdatomic.h>
#include <immintrin.h>

// ============================================================================
// COMPILER OPTIMIZATION MACROS
// ============================================================================

// Branch prediction hints
#define CNS_LIKELY(x) __builtin_expect(!!(x), 1)
#define CNS_UNLIKELY(x) __builtin_expect(!!(x), 0)

// Function attributes for optimization
#define CNS_HOT __attribute__((hot))
#define CNS_COLD __attribute__((cold))
#define CNS_PURE __attribute__((pure))
#define CNS_CONST __attribute__((const))
#define CNS_RESTRICT __restrict__
#define CNS_ALIGNED(x) __attribute__((aligned(x)))
#define CNS_PACKED __attribute__((packed))

// Inline functions
#define CNS_INLINE inline __attribute__((always_inline))
#define CNS_NOINLINE __attribute__((noinline))

// ============================================================================
// HARDWARE DETECTION MACROS
// ============================================================================

// CPU architecture detection
#ifdef __AVX512F__
#define CNS_VECTOR_WIDTH 8
#define CNS_VECTOR_ALIGN 64
#define CNS_VECTOR_TYPE __m512i
#define CNS_VECTOR_LOAD _mm512_loadu_si512
#define CNS_VECTOR_STORE _mm512_storeu_si512
#define CNS_VECTOR_GATHER _mm512_i32gather_epi64
#define CNS_VECTOR_SCATTER _mm512_i32scatter_epi64
#define CNS_VECTOR_EXTRACT _mm512_extract_epi32
#define CNS_HAS_AVX512 1
#elif defined(__AVX2__)
#define CNS_VECTOR_WIDTH 4
#define CNS_VECTOR_ALIGN 32
#define CNS_VECTOR_TYPE __m256i
#define CNS_VECTOR_LOAD _mm256_loadu_si256
#define CNS_VECTOR_STORE _mm256_storeu_si256
#define CNS_VECTOR_GATHER _mm256_i32gather_epi64
#define CNS_VECTOR_SCATTER _mm256_i32scatter_epi64
#define CNS_VECTOR_EXTRACT _mm256_extract_epi32
#define CNS_HAS_AVX2 1
#elif defined(__ARM_NEON)
#include <arm_neon.h>
#define CNS_VECTOR_WIDTH 4
#define CNS_VECTOR_ALIGN 16
#define CNS_VECTOR_TYPE uint32x4_t
#define CNS_VECTOR_LOAD vld1q_u32
#define CNS_VECTOR_STORE vst1q_u32
#define CNS_VECTOR_EXTRACT vgetq_lane_u32
#define CNS_HAS_NEON 1
#else
#define CNS_VECTOR_WIDTH 1
#define CNS_VECTOR_ALIGN 8
#define CNS_VECTOR_TYPE uint32_t
#define CNS_VECTOR_LOAD(x) (*(x))
#define CNS_VECTOR_STORE(x, y) (*(x) = (y))
#define CNS_VECTOR_EXTRACT(x, y) (x)
#endif

// Cache line size detection
#define CNS_CACHE_LINE_SIZE 64
#define CNS_L1_CACHE_SIZE (32 * 1024)       // 32KB typical
#define CNS_L2_CACHE_SIZE (256 * 1024)      // 256KB typical
#define CNS_L3_CACHE_SIZE (8 * 1024 * 1024) // 8MB typical

// ============================================================================
// MEMORY ALLOCATION HELPERS
// ============================================================================

// Aligned memory allocation
CNS_INLINE void *cns_aligned_alloc(size_t alignment, size_t size)
{
  return aligned_alloc(alignment, size);
}

// Cache-line aligned allocation
CNS_INLINE void *cns_cache_aligned_alloc(size_t size)
{
  return cns_aligned_alloc(CNS_CACHE_LINE_SIZE, size);
}

// Vector-aligned allocation
CNS_INLINE void *cns_vector_aligned_alloc(size_t size)
{
  return cns_aligned_alloc(CNS_VECTOR_ALIGN, size);
}

// Memory pool for small allocations
typedef struct
{
  char *buffer;
  size_t used;
  size_t size;
  size_t block_size;
} CNSMemoryPool;

CNS_INLINE CNSMemoryPool *cns_memory_pool_create(size_t size, size_t block_size)
{
  CNSMemoryPool *pool = malloc(sizeof(CNSMemoryPool));
  if (!pool)
    return NULL;

  pool->buffer = cns_cache_aligned_alloc(size);
  pool->used = 0;
  pool->size = size;
  pool->block_size = block_size;

  return pool;
}

CNS_INLINE void *cns_memory_pool_alloc(CNSMemoryPool *pool)
{
  if (pool->used + pool->block_size > pool->size)
    return NULL;

  void *ptr = pool->buffer + pool->used;
  pool->used += pool->block_size;
  return ptr;
}

CNS_INLINE void cns_memory_pool_destroy(CNSMemoryPool *pool)
{
  if (pool)
  {
    free(pool->buffer);
    free(pool);
  }
}

// ============================================================================
// CACHE OPTIMIZATION HELPERS
// ============================================================================

// Multi-tier cache structure
typedef struct
{
  uint64_t l1_data[8] CNS_ALIGNED(64);    // L1: 64 bytes
  uint64_t l2_data[32] CNS_ALIGNED(128);  // L2: 256 bytes
  uint64_t l3_data[128] CNS_ALIGNED(512); // L3: 1KB
} CNSCacheTier;

// Cache tier management
typedef struct
{
  CNSCacheTier *tiers;
  size_t tier_count;
  atomic_uint64_t access_count;
  atomic_uint64_t hit_count;
} CNSCacheManager;

CNS_INLINE CNSCacheManager *cns_cache_manager_create(size_t tier_count)
{
  CNSCacheManager *manager = cns_cache_aligned_alloc(sizeof(CNSCacheManager));
  if (!manager)
    return NULL;

  manager->tier_count = tier_count;
  manager->tiers = cns_cache_aligned_alloc(tier_count * sizeof(CNSCacheTier));
  atomic_init(&manager->access_count, 0);
  atomic_init(&manager->hit_count, 0);

  return manager;
}

CNS_INLINE void cns_cache_manager_destroy(CNSCacheManager *manager)
{
  if (manager)
  {
    free(manager->tiers);
    free(manager);
  }
}

// Cache access helpers
CNS_INLINE int cns_cache_l1_check(CNSCacheTier *tier, uint64_t key, uint64_t value)
{
  size_t index = key % 8;
  if (tier->l1_data[index] == value)
    return 1;
  return 0;
}

CNS_INLINE int cns_cache_l2_check(CNSCacheTier *tier, uint64_t key, uint64_t value)
{
  size_t index = key % 32;
  if (tier->l2_data[index] == value)
    return 1;
  return 0;
}

CNS_INLINE void cns_cache_l1_update(CNSCacheTier *tier, uint64_t key, uint64_t value)
{
  size_t index = key % 8;
  tier->l1_data[index] = value;
}

CNS_INLINE void cns_cache_l2_update(CNSCacheTier *tier, uint64_t key, uint64_t value)
{
  size_t index = key % 32;
  tier->l2_data[index] = value;
}

// ============================================================================
// LOCK-FREE DATA STRUCTURES
// ============================================================================

// Lock-free ring buffer
typedef struct
{
  atomic_uint64_t head;
  atomic_uint64_t tail;
  uint64_t *data;
  size_t size;
} CNSLockFreeRing;

CNS_INLINE CNSLockFreeRing *cns_ring_create(size_t size)
{
  CNSLockFreeRing *ring = cns_cache_aligned_alloc(sizeof(CNSLockFreeRing));
  if (!ring)
    return NULL;

  ring->data = cns_cache_aligned_alloc(size * sizeof(uint64_t));
  ring->size = size;
  atomic_init(&ring->head, 0);
  atomic_init(&ring->tail, 0);

  return ring;
}

CNS_INLINE void cns_ring_destroy(CNSLockFreeRing *ring)
{
  if (ring)
  {
    free(ring->data);
    free(ring);
  }
}

CNS_INLINE int cns_ring_push(CNSLockFreeRing *ring, uint64_t value)
{
  uint64_t tail = atomic_load(&ring->tail);
  uint64_t head = atomic_load(&ring->head);

  if ((tail + 1) % ring->size == head)
    return 0; // Full

  ring->data[tail % ring->size] = value;
  atomic_store(&ring->tail, (tail + 1) % ring->size);
  return 1;
}

CNS_INLINE int cns_ring_pop(CNSLockFreeRing *ring, uint64_t *value)
{
  uint64_t head = atomic_load(&ring->head);
  uint64_t tail = atomic_load(&ring->tail);

  if (head == tail)
    return 0; // Empty

  *value = ring->data[head % ring->size];
  atomic_store(&ring->head, (head + 1) % ring->size);
  return 1;
}

// ============================================================================
// SIMD VECTORIZATION HELPERS
// ============================================================================

// Vectorized batch processing
typedef struct
{
  CNS_VECTOR_TYPE *data;
  size_t capacity;
  size_t size;
} CNSVector;

CNS_INLINE CNSVector *cns_vector_create(size_t capacity)
{
  CNSVector *vec = cns_vector_aligned_alloc(sizeof(CNSVector));
  if (!vec)
    return NULL;

  vec->data = cns_vector_aligned_alloc(capacity * sizeof(CNS_VECTOR_TYPE));
  vec->capacity = capacity;
  vec->size = 0;

  return vec;
}

CNS_INLINE void cns_vector_destroy(CNSVector *vec)
{
  if (vec)
  {
    free(vec->data);
    free(vec);
  }
}

// Vectorized operations
CNS_INLINE void cns_vector_load_batch(CNS_VECTOR_TYPE *dest, const uint32_t *src, size_t count)
{
  size_t vector_count = count & ~(CNS_VECTOR_WIDTH - 1);

  for (size_t i = 0; i < vector_count; i += CNS_VECTOR_WIDTH)
  {
    dest[i / CNS_VECTOR_WIDTH] = CNS_VECTOR_LOAD((CNS_VECTOR_TYPE *)&src[i]);
  }
}

CNS_INLINE void cns_vector_store_batch(uint32_t *dest, const CNS_VECTOR_TYPE *src, size_t count)
{
  size_t vector_count = count & ~(CNS_VECTOR_WIDTH - 1);

  for (size_t i = 0; i < vector_count; i += CNS_VECTOR_WIDTH)
  {
    CNS_VECTOR_STORE((CNS_VECTOR_TYPE *)&dest[i], src[i / CNS_VECTOR_WIDTH]);
  }
}

// ============================================================================
// PERFORMANCE MONITORING HELPERS
// ============================================================================

// High-precision timing
CNS_INLINE uint64_t cns_get_nanoseconds(void)
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

CNS_INLINE uint64_t cns_get_cycles(void)
{
  return __builtin_readcyclecounter();
}

// Performance counters
typedef struct
{
  atomic_uint64_t operation_count;
  atomic_uint64_t cache_hits;
  atomic_uint64_t cache_misses;
  atomic_uint64_t total_time_ns;
  atomic_uint64_t min_time_ns;
  atomic_uint64_t max_time_ns;
} CNSPerformanceCounters;

CNS_INLINE CNSPerformanceCounters *cns_performance_counters_create(void)
{
  CNSPerformanceCounters *counters = cns_cache_aligned_alloc(sizeof(CNSPerformanceCounters));
  if (!counters)
    return NULL;

  atomic_init(&counters->operation_count, 0);
  atomic_init(&counters->cache_hits, 0);
  atomic_init(&counters->cache_misses, 0);
  atomic_init(&counters->total_time_ns, 0);
  atomic_init(&counters->min_time_ns, UINT64_MAX);
  atomic_init(&counters->max_time_ns, 0);

  return counters;
}

CNS_INLINE void cns_performance_counters_destroy(CNSPerformanceCounters *counters)
{
  free(counters);
}

CNS_INLINE void cns_performance_record(CNSPerformanceCounters *counters, uint64_t time_ns, int cache_hit)
{
  atomic_fetch_add(&counters->operation_count, 1);
  atomic_fetch_add(&counters->total_time_ns, time_ns);

  if (cache_hit)
  {
    atomic_fetch_add(&counters->cache_hits, 1);
  }
  else
  {
    atomic_fetch_add(&counters->cache_misses, 1);
  }

  // Update min/max (simplified - not atomic)
  uint64_t current_min = atomic_load(&counters->min_time_ns);
  uint64_t current_max = atomic_load(&counters->max_time_ns);

  if (time_ns < current_min)
    atomic_store(&counters->min_time_ns, time_ns);
  if (time_ns > current_max)
    atomic_store(&counters->max_time_ns, time_ns);
}

CNS_INLINE void cns_performance_get_stats(CNSPerformanceCounters *counters,
                                          uint64_t *total_ops, uint64_t *cache_hits,
                                          uint64_t *cache_misses, double *avg_time_ns)
{
  *total_ops = atomic_load(&counters->operation_count);
  *cache_hits = atomic_load(&counters->cache_hits);
  *cache_misses = atomic_load(&counters->cache_misses);

  if (*total_ops > 0)
  {
    *avg_time_ns = (double)atomic_load(&counters->total_time_ns) / *total_ops;
  }
  else
  {
    *avg_time_ns = 0.0;
  }
}

// ============================================================================
// BIT VECTOR HELPERS
// ============================================================================

// Bit vector operations
CNS_INLINE void cns_bit_vector_set(uint64_t *vector, size_t index)
{
  size_t word = index / 64;
  size_t bit = index % 64;
  vector[word] |= (1ULL << bit);
}

CNS_INLINE int cns_bit_vector_test(const uint64_t *vector, size_t index)
{
  size_t word = index / 64;
  size_t bit = index % 64;
  return (vector[word] & (1ULL << bit)) != 0;
}

CNS_INLINE void cns_bit_vector_clear(uint64_t *vector, size_t index)
{
  size_t word = index / 64;
  size_t bit = index % 64;
  vector[word] &= ~(1ULL << bit);
}

CNS_INLINE size_t cns_bit_vector_popcount(const uint64_t *vector, size_t word_count)
{
  size_t count = 0;
  for (size_t i = 0; i < word_count; i++)
  {
    count += __builtin_popcountll(vector[i]);
  }
  return count;
}

// ============================================================================
// PREFETCHING HELPERS
// ============================================================================

// Memory prefetching
CNS_INLINE void cns_prefetch_read(const void *ptr)
{
  __builtin_prefetch(ptr, 0, 3); // Read, high locality
}

CNS_INLINE void cns_prefetch_write(const void *ptr)
{
  __builtin_prefetch(ptr, 1, 3); // Write, high locality
}

CNS_INLINE void cns_prefetch_read_low_locality(const void *ptr)
{
  __builtin_prefetch(ptr, 0, 0); // Read, low locality
}

// ============================================================================
// UTILITY MACROS
// ============================================================================

// Safe array access
#define CNS_ARRAY_GET(arr, index, size) \
  (CNS_LIKELY((index) < (size)) ? (arr)[(index)] : 0)

// Bounds checking
#define CNS_BOUNDS_CHECK(index, size) \
  CNS_LIKELY((index) < (size))

// Cache line padding
#define CNS_CACHE_PAD(size) \
  char padding[CNS_CACHE_LINE_SIZE - ((size) % CNS_CACHE_LINE_SIZE)]

// Vector alignment
#define CNS_VECTOR_ALIGN_PAD(size) \
  char padding[CNS_VECTOR_ALIGN - ((size) % CNS_VECTOR_ALIGN)]

// Performance measurement
#define CNS_PERFORMANCE_START(counters) \
  uint64_t cns_start_time = cns_get_nanoseconds()

#define CNS_PERFORMANCE_END(counters, cache_hit)               \
  do                                                           \
  {                                                            \
    uint64_t cns_end_time = cns_get_nanoseconds();             \
    uint64_t cns_duration = cns_end_time - cns_start_time;     \
    cns_performance_record(counters, cns_duration, cache_hit); \
  } while (0)

// SIMD batch processing
#define CNS_SIMD_BATCH_START(count) \
  size_t cns_simd_count = (count) & ~(CNS_VECTOR_WIDTH - 1)

#define CNS_SIMD_BATCH_END(count) \
  for (size_t cns_i = cns_simd_count; cns_i < (count); cns_i++)

#endif // CNS_OPTIMIZATION_HELPERS_H