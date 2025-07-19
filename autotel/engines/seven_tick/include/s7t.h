/*  ─────────────────────────────────────────────────────────────
    s7t.h  –  Physics-compliant helper library  (v1.0)
    ───────────────────────────────────────────────────────────── */
#ifndef S7T_H
#define S7T_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdatomic.h>
#include <stdbool.h>

/*— Physics Constants —*/
#define S7T_CACHE_LINE_SIZE    64
#define S7T_L1_SIZE           (32 * 1024)
#define S7T_MAX_CYCLES         7
#define S7T_NS_PER_CYCLE      0.3  // ~3.3GHz CPU

/*— Compiler Hints —*/
#define S7T_ALWAYS_INLINE     static inline __attribute__((always_inline))
#define S7T_ALIGNED(N)        __attribute__((aligned(N)))
#define S7T_SECTION(sec)      __attribute__((section(sec)))
#define S7T_HOT               __attribute__((hot))
#define S7T_COLD              __attribute__((cold))
#define S7T_PURE              __attribute__((pure))
#define S7T_CONST             __attribute__((const))
#define S7T_RESTRICT          __restrict__
#define S7T_LIKELY(x)         __builtin_expect(!!(x), 1)
#define S7T_UNLIKELY(x)       __builtin_expect(!!(x), 0)

/*— Compile-time Utilities —*/
#define S7T_STRINGIFY(x)      #x
#define S7T_CONCAT(a,b)       a##b
#define S7T_UNROLL(N)         _Pragma(S7T_STRINGIFY(GCC unroll N))
#define S7T_STATIC_ASSERT(expr, msg) _Static_assert(expr, msg)

/*— Cycle Counting —*/
#if defined(__x86_64__) || defined(__i386__)
S7T_ALWAYS_INLINE uint64_t s7t_cycles(void) {
    uint32_t lo, hi;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}
#elif defined(__aarch64__)
S7T_ALWAYS_INLINE uint64_t s7t_cycles(void) {
    uint64_t val;
    __asm__ __volatile__ ("mrs %0, cntvct_el0" : "=r"(val));
    return val;
}
#else
S7T_ALWAYS_INLINE uint64_t s7t_cycles(void) {
    return __builtin_readcyclecounter();
}
#endif

/*— Memory Prefetch —*/
S7T_ALWAYS_INLINE void s7t_prefetch_r(const void* p) {
    __builtin_prefetch(p, 0, 3);  // Read, high locality
}

S7T_ALWAYS_INLINE void s7t_prefetch_w(void* p) {
    __builtin_prefetch(p, 1, 3);  // Write, high locality
}

/*— Performance Assertions —*/
#ifdef S7T_DEBUG
#define S7T_ASSERT_CYCLES(operation, max_cycles) do { \
    uint64_t __start = s7t_cycles(); \
    operation; \
    uint64_t __cycles = s7t_cycles() - __start; \
    if (S7T_UNLIKELY(__cycles > (max_cycles))) { \
        __builtin_trap(); \
    } \
} while(0)
#else
#define S7T_ASSERT_CYCLES(operation, max_cycles) operation
#endif

/*— Interned ID System —*/
typedef uint32_t s7t_id_t;
#define S7T_ID_NULL  ((s7t_id_t)0)
#define S7T_ID_MAX   ((s7t_id_t)0xFFFFFFFF)

/*— Bit Utilities —*/
S7T_ALWAYS_INLINE uint32_t s7t_popcount(uint32_t x) {
    return __builtin_popcount(x);
}

S7T_ALWAYS_INLINE uint32_t s7t_ctz(uint32_t x) {
    return x ? __builtin_ctz(x) : 32;
}

S7T_ALWAYS_INLINE uint32_t s7t_clz(uint32_t x) {
    return x ? __builtin_clz(x) : 32;
}

S7T_ALWAYS_INLINE uint32_t s7t_bswap32(uint32_t x) {
    return __builtin_bswap32(x);
}

S7T_ALWAYS_INLINE uint64_t s7t_bswap64(uint64_t x) {
    return __builtin_bswap64(x);
}

/*— Branch-free Selection —*/
S7T_ALWAYS_INLINE int s7t_select_i32(int cond, int a, int b) {
    return cond ? a : b;  // Compiler optimizes to cmov
}

S7T_ALWAYS_INLINE uint32_t s7t_min_u32(uint32_t a, uint32_t b) {
    return a < b ? a : b;
}

S7T_ALWAYS_INLINE uint32_t s7t_max_u32(uint32_t a, uint32_t b) {
    return a > b ? a : b;
}

/*— Lookup Table Access —*/
#define S7T_LUT_GET(table, idx) ((table)[(idx)])

/*— Memory Arena —*/
typedef struct S7T_ALIGNED(64) {
    uint8_t* data;
    size_t size;
    size_t used;
} s7t_arena_t;

S7T_ALWAYS_INLINE void* s7t_arena_alloc(s7t_arena_t* arena, size_t n) {
    size_t aligned_n = (n + 63) & ~63;  // 64-byte align
    if (S7T_UNLIKELY(arena->used + aligned_n > arena->size)) {
        return NULL;
    }
    void* p = arena->data + arena->used;
    arena->used += aligned_n;
    return p;
}

S7T_ALWAYS_INLINE void s7t_arena_reset(s7t_arena_t* arena) {
    arena->used = 0;
}

/*— Static Pool Declaration —*/
#define S7T_DECLARE_POOL(name, SIZE) \
    static struct { \
        _Alignas(64) uint8_t data[SIZE]; \
        size_t used; \
    } name = { {0}, 0 }

#define S7T_POOL_ALLOC(pool, n) ({ \
    size_t __n = (n + 63) & ~63; \
    void* __p = NULL; \
    if ((pool).used + __n <= sizeof((pool).data)) { \
        __p = &(pool).data[(pool).used]; \
        (pool).used += __n; \
    } \
    __p; \
})

/*— Atomic Operations —*/
typedef _Atomic(uint32_t) s7t_atomic_u32;
typedef _Atomic(uint64_t) s7t_atomic_u64;

S7T_ALWAYS_INLINE uint32_t s7t_atomic_inc_u32(s7t_atomic_u32* a) {
    return atomic_fetch_add_explicit(a, 1, memory_order_relaxed);
}

S7T_ALWAYS_INLINE uint64_t s7t_atomic_inc_u64(s7t_atomic_u64* a) {
    return atomic_fetch_add_explicit(a, 1, memory_order_relaxed);
}

S7T_ALWAYS_INLINE bool s7t_atomic_cas_u32(s7t_atomic_u32* a, uint32_t* expected, uint32_t desired) {
    return atomic_compare_exchange_weak_explicit(a, expected, desired,
        memory_order_relaxed, memory_order_relaxed);
}

/*— SIMD Helpers —*/
#if defined(__SSE2__)
#include <emmintrin.h>
typedef __m128i s7t_vec128;

S7T_ALWAYS_INLINE s7t_vec128 s7t_vec_load(const void* p) {
    return _mm_load_si128((const __m128i*)p);
}

S7T_ALWAYS_INLINE void s7t_vec_store(void* p, s7t_vec128 v) {
    _mm_store_si128((__m128i*)p, v);
}

S7T_ALWAYS_INLINE s7t_vec128 s7t_vec_and(s7t_vec128 a, s7t_vec128 b) {
    return _mm_and_si128(a, b);
}

S7T_ALWAYS_INLINE s7t_vec128 s7t_vec_or(s7t_vec128 a, s7t_vec128 b) {
    return _mm_or_si128(a, b);
}
#endif

#if defined(__AVX2__)
#include <immintrin.h>
typedef __m256i s7t_vec256;

S7T_ALWAYS_INLINE s7t_vec256 s7t_vec256_load(const void* p) {
    return _mm256_load_si256((const __m256i*)p);
}

S7T_ALWAYS_INLINE void s7t_vec256_store(void* p, s7t_vec256 v) {
    _mm256_store_si256((__m256i*)p, v);
}
#endif

#if defined(__ARM_NEON)
#include <arm_neon.h>
typedef uint32x4_t s7t_vec128;

S7T_ALWAYS_INLINE s7t_vec128 s7t_vec_load(const void* p) {
    return vld1q_u32((const uint32_t*)p);
}

S7T_ALWAYS_INLINE void s7t_vec_store(void* p, s7t_vec128 v) {
    vst1q_u32((uint32_t*)p, v);
}
#endif

/*— Computed Goto Support —*/
#if defined(__GNUC__)
#define S7T_DISPATCH_TABLE(name, ...) \
    static void* const name[] = { __VA_ARGS__ }

#define S7T_DISPATCH(table, idx) \
    goto *(table)[(idx)]
#endif

/*— Cache-aligned Structures —*/
typedef struct S7T_ALIGNED(64) {
    uint32_t id;
    uint16_t flags : 4;
    uint16_t type  : 4;
    uint16_t len   : 8;
    uint32_t data[14];  // Total: 64 bytes
} s7t_packet_t;

S7T_STATIC_ASSERT(sizeof(s7t_packet_t) == 64, "Packet must be cache-line sized");

/*— MMIO Register Access —*/
#define S7T_MMIO_READ32(addr) \
    (*(volatile uint32_t*)(addr))

#define S7T_MMIO_WRITE32(addr, val) \
    (*(volatile uint32_t*)(addr) = (val))

/*— Generic Type Selection —*/
#define s7t_min(a, b) _Generic((a), \
    int: s7t_min_i32, \
    uint32_t: s7t_min_u32, \
    uint64_t: s7t_min_u64 \
)(a, b)

S7T_ALWAYS_INLINE int s7t_min_i32(int a, int b) { return a < b ? a : b; }
S7T_ALWAYS_INLINE uint64_t s7t_min_u64(uint64_t a, uint64_t b) { return a < b ? a : b; }

/*— String Interning —*/
typedef struct {
    uint32_t hash;
    uint32_t len;
    const char* str;
} s7t_intern_entry_t;

typedef struct S7T_ALIGNED(64) {
    s7t_intern_entry_t* entries;
    uint32_t* table;  // Hash table indices
    uint32_t capacity;
    uint32_t count;
    uint32_t table_size;
} s7t_intern_t;

S7T_ALWAYS_INLINE uint32_t s7t_hash_string(const char* str, size_t len) {
    uint32_t hash = 5381;
    for (size_t i = 0; i < len; i++) {
        hash = ((hash << 5) + hash) + str[i];
    }
    return hash;
}

/*— Performance Monitoring —*/
typedef struct S7T_ALIGNED(64) {
    uint64_t cycles;
    uint64_t count;
    uint64_t min_cycles;
    uint64_t max_cycles;
} s7t_perf_counter_t;

S7T_ALWAYS_INLINE void s7t_perf_update(s7t_perf_counter_t* pc, uint64_t cycles) {
    pc->cycles += cycles;
    pc->count++;
    if (cycles < pc->min_cycles) pc->min_cycles = cycles;
    if (cycles > pc->max_cycles) pc->max_cycles = cycles;
}

/*— Batch Operations —*/
#define S7T_BATCH_SIZE 64

typedef struct S7T_ALIGNED(64) {
    uint32_t items[S7T_BATCH_SIZE];
    uint32_t count;
} s7t_batch_t;

S7T_ALWAYS_INLINE bool s7t_batch_add(s7t_batch_t* batch, uint32_t item) {
    if (S7T_UNLIKELY(batch->count >= S7T_BATCH_SIZE)) {
        return false;
    }
    batch->items[batch->count++] = item;
    return true;
}

S7T_ALWAYS_INLINE void s7t_batch_reset(s7t_batch_t* batch) {
    batch->count = 0;
}

/*— Validation Macros —*/
#define S7T_VALIDATE_ALIGNMENT(ptr, align) \
    S7T_STATIC_ASSERT(((uintptr_t)(ptr) & ((align) - 1)) == 0, "Misaligned pointer")

#define S7T_VALIDATE_SIZE(type, expected) \
    S7T_STATIC_ASSERT(sizeof(type) == (expected), "Unexpected size")

#endif /* S7T_H */