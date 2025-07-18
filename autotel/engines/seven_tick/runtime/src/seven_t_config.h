#ifndef SEVEN_T_CONFIG_H
#define SEVEN_T_CONFIG_H

// Build configuration
#ifdef NDEBUG
    #define S7T_PRODUCTION 1
    #define S7T_DEBUG(x)
    #define S7T_ASSERT(x) ((void)0)
#else
    #define S7T_PRODUCTION 0
    #define S7T_DEBUG(x) x
    #define S7T_ASSERT(x) assert(x)
#endif

// Performance-critical inline hints
#define S7T_INLINE static inline __attribute__((always_inline))
#define S7T_HOT __attribute__((hot))
#define S7T_COLD __attribute__((cold))
#define S7T_PURE __attribute__((pure))
#define S7T_RESTRICT __restrict__

// Cache optimization hints
#define S7T_LIKELY(x) __builtin_expect(!!(x), 1)
#define S7T_UNLIKELY(x) __builtin_expect(!!(x), 0)
#define S7T_PREFETCH(addr) __builtin_prefetch(addr, 0, 3)

#endif // SEVEN_T_CONFIG_H
