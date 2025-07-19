/*  ─────────────────────────────────────────────────────────────
    cns/core/perf.h  –  Performance Tracking (v2.0)
    Cycle-accurate performance monitoring with 7-tick assertions
    ───────────────────────────────────────────────────────────── */
#ifndef CNS_CORE_PERF_H
#define CNS_CORE_PERF_H

#include "../../../include/s7t.h"
#include <stdint.h>
#include <stdbool.h>

/*═══════════════════════════════════════════════════════════════
  Performance Counter
  ═══════════════════════════════════════════════════════════════*/

typedef struct S7T_ALIGNED(64) {
    uint64_t total_cycles;      // Total CPU cycles
    uint64_t total_count;       // Total measurements
    uint64_t min_cycles;        // Minimum cycles
    uint64_t max_cycles;        // Maximum cycles
    uint64_t violations;        // 7-tick violations
    uint64_t threshold;         // Threshold cycles (7 ticks)
    
    // Histogram buckets (powers of 2)
    uint64_t histogram[16];     // 2^0 to 2^15 cycles
} cns_perf_counter_t;

/*═══════════════════════════════════════════════════════════════
  Performance Tracker (Multiple Counters)
  ═══════════════════════════════════════════════════════════════*/

typedef struct S7T_ALIGNED(64) {
    cns_perf_counter_t parse;       // Parser performance
    cns_perf_counter_t lookup;      // Command lookup
    cns_perf_counter_t execute;     // Command execution
    cns_perf_counter_t total;       // Total performance
    
    // Configuration
    bool strict_mode;               // Fail on violations
    bool histogram_enabled;         // Track histogram
    uint64_t tick_threshold;        // Cycles per tick
} cns_perf_tracker_t;

/*═══════════════════════════════════════════════════════════════
  Performance Measurement
  ═══════════════════════════════════════════════════════════════*/

// Initialize performance counter
S7T_ALWAYS_INLINE void cns_perf_init(
    cns_perf_counter_t* counter,
    uint64_t threshold
) {
    counter->total_cycles = 0;
    counter->total_count = 0;
    counter->min_cycles = UINT64_MAX;
    counter->max_cycles = 0;
    counter->violations = 0;
    counter->threshold = threshold;
    
    // Clear histogram
    for (int i = 0; i < 16; i++) {
        counter->histogram[i] = 0;
    }
}

// Update performance counter (< 7 ticks)
S7T_ALWAYS_INLINE void cns_perf_update(
    cns_perf_counter_t* counter,
    uint64_t cycles
) {
    // Update statistics
    counter->total_cycles += cycles;
    counter->total_count++;
    
    if (cycles < counter->min_cycles) {
        counter->min_cycles = cycles;
    }
    if (cycles > counter->max_cycles) {
        counter->max_cycles = cycles;
    }
    
    // Check threshold violation
    if (cycles > counter->threshold) {
        counter->violations++;
    }
    
    // Update histogram (find bucket)
    uint32_t bucket = 0;
    uint64_t value = cycles;
    while (value > 1 && bucket < 15) {
        value >>= 1;
        bucket++;
    }
    counter->histogram[bucket]++;
}

// Get average cycles
S7T_ALWAYS_INLINE uint64_t cns_perf_average(
    const cns_perf_counter_t* counter
) {
    return counter->total_count > 0 
        ? counter->total_cycles / counter->total_count 
        : 0;
}

/*═══════════════════════════════════════════════════════════════
  7-Tick Assertions
  ═══════════════════════════════════════════════════════════════*/

// Assert operation completed within tick budget
#define CNS_ASSERT_TICKS(start, ticks) \
    do { \
        uint64_t __cycles = s7t_cycles() - (start); \
        uint64_t __threshold = (ticks) * S7T_CYCLES_PER_TICK; \
        if (__cycles > __threshold) { \
            cns_perf_violation(__FILE__, __LINE__, __cycles, __threshold); \
        } \
    } while(0)

// Conditional tick assertion (only in strict mode)
#define CNS_ASSERT_TICKS_STRICT(tracker, start, ticks) \
    do { \
        if ((tracker)->strict_mode) { \
            CNS_ASSERT_TICKS(start, ticks); \
        } \
    } while(0)

// Performance violation handler
void cns_perf_violation(
    const char* file,
    int line,
    uint64_t actual_cycles,
    uint64_t threshold_cycles
);

/*═══════════════════════════════════════════════════════════════
  Performance Scopes
  ═══════════════════════════════════════════════════════════════*/

// Scoped performance measurement
typedef struct {
    cns_perf_counter_t* counter;
    uint64_t start_cycles;
} cns_perf_scope_t;

// Start performance scope
S7T_ALWAYS_INLINE cns_perf_scope_t cns_perf_scope_start(
    cns_perf_counter_t* counter
) {
    return (cns_perf_scope_t){
        .counter = counter,
        .start_cycles = s7t_cycles()
    };
}

// End performance scope
S7T_ALWAYS_INLINE void cns_perf_scope_end(cns_perf_scope_t* scope) {
    uint64_t cycles = s7t_cycles() - scope->start_cycles;
    cns_perf_update(scope->counter, cycles);
}

// Automatic scope (using cleanup attribute)
#define CNS_PERF_SCOPE(counter) \
    __attribute__((cleanup(cns_perf_scope_end))) \
    cns_perf_scope_t _perf_scope = cns_perf_scope_start(counter)

/*═══════════════════════════════════════════════════════════════
  Performance Statistics
  ═══════════════════════════════════════════════════════════════*/

typedef struct {
    uint64_t count;             // Number of measurements
    uint64_t total_cycles;      // Total cycles
    uint64_t avg_cycles;        // Average cycles
    uint64_t min_cycles;        // Minimum cycles
    uint64_t max_cycles;        // Maximum cycles
    uint64_t violations;        // Threshold violations
    double violation_rate;      // Violation percentage
    double p50;                 // 50th percentile
    double p90;                 // 90th percentile
    double p99;                 // 99th percentile
} cns_perf_stats_t;

// Calculate performance statistics
void cns_perf_calculate_stats(
    const cns_perf_counter_t* counter,
    cns_perf_stats_t* stats
);

// Export performance data
void cns_perf_export(
    const cns_perf_tracker_t* tracker,
    const char* filename
);

/*═══════════════════════════════════════════════════════════════
  Performance Benchmarking
  ═══════════════════════════════════════════════════════════════*/

// Benchmark result
typedef struct {
    const char* name;           // Benchmark name
    uint64_t iterations;        // Number of iterations
    uint64_t total_cycles;      // Total cycles
    uint64_t avg_cycles;        // Average per iteration
    uint64_t min_cycles;        // Best case
    uint64_t max_cycles;        // Worst case
    bool passed;                // Passed constraints
} cns_benchmark_result_t;

// Run benchmark
#define CNS_BENCHMARK(name, iterations, code) \
    ({ \
        cns_benchmark_result_t __result = { \
            .name = name, \
            .iterations = iterations \
        }; \
        uint64_t __min = UINT64_MAX, __max = 0, __total = 0; \
        for (uint64_t __i = 0; __i < iterations; __i++) { \
            uint64_t __start = s7t_cycles(); \
            code; \
            uint64_t __cycles = s7t_cycles() - __start; \
            __total += __cycles; \
            if (__cycles < __min) __min = __cycles; \
            if (__cycles > __max) __max = __cycles; \
        } \
        __result.total_cycles = __total; \
        __result.avg_cycles = __total / iterations; \
        __result.min_cycles = __min; \
        __result.max_cycles = __max; \
        __result.passed = __result.avg_cycles <= 7 * S7T_CYCLES_PER_TICK; \
        __result; \
    })

/*═══════════════════════════════════════════════════════════════
  Default Configuration
  ═══════════════════════════════════════════════════════════════*/

// Default cycles per tick (platform-specific)
#ifndef S7T_CYCLES_PER_TICK
  #if defined(__x86_64__) || defined(__i386__)
    #define S7T_CYCLES_PER_TICK 3500  // ~1ns at 3.5GHz
  #elif defined(__aarch64__)
    #define S7T_CYCLES_PER_TICK 2400  // ~1ns at 2.4GHz
  #else
    #define S7T_CYCLES_PER_TICK 1000  // Conservative default
  #endif
#endif

// 7-tick threshold
#define CNS_7TICK_THRESHOLD (7 * S7T_CYCLES_PER_TICK)

#endif /* CNS_CORE_PERF_H */