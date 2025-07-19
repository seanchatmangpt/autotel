/*  ─────────────────────────────────────────────────────────────
    s7t_minimal.h  –  Minimal S7T definitions for testing
    ───────────────────────────────────────────────────────────── */

#ifndef S7T_MINIMAL_H
#define S7T_MINIMAL_H

#include <stdint.h>

#define S7T_MAX_CYCLES 7
#define S7T_ALIGNED(x) __attribute__((aligned(x)))

typedef struct {
    uint64_t start_cycles;
    uint64_t end_cycles;
} s7t_span_t;

static inline void s7t_span_start(s7t_span_t* span, const char* name) {
    span->start_cycles = 1000;
}

static inline void s7t_span_end(s7t_span_t* span) {
    span->end_cycles = 2000;
}

#endif /* S7T_MINIMAL_H */