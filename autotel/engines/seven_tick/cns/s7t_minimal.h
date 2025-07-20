/*  ─────────────────────────────────────────────────────────────
    s7t_minimal.h  –  Minimal S7T definitions for testing
    ───────────────────────────────────────────────────────────── */

#ifndef S7T_MINIMAL_H
#define S7T_MINIMAL_H

#include <stdint.h>

#define S7T_MAX_CYCLES 7
#define S7T_ALIGNED(x) __attribute__((aligned(x)))

// Basic cycle counter
static inline uint64_t S7T_CYCLES(void) {
#if defined(__x86_64__) || defined(__i386__)
    uint32_t lo, hi;
    __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
#elif defined(__aarch64__)
    uint64_t val;
    __asm__ volatile("mrs %0, cntvct_el0" : "=r"(val));
    return val;
#else
    return 1000; // Fallback for testing
#endif
}

typedef struct {
    uint64_t start_cycles;
    uint64_t end_cycles;
} s7t_span_t;

static inline void s7t_span_start(s7t_span_t* span, const char* name) {
    (void)name; // Suppress unused parameter warning
    span->start_cycles = S7T_CYCLES();
}

static inline void s7t_span_end(s7t_span_t* span) {
    span->end_cycles = S7T_CYCLES();
}

#endif /* S7T_MINIMAL_H */