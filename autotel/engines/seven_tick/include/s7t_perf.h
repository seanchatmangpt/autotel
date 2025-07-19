/*
 * s7t_perf.h - Seven Tick Performance Validation
 * 
 * Runtime verification that operations comply with physics constraints.
 * Provides cycle counting, cache miss detection, and automatic gates.
 */

#ifndef S7T_PERF_H
#define S7T_PERF_H

#include "s7t.h"
#include "s7t_patterns.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ═══════════════════════════════════════════════════════════════════════════
 * PERFORMANCE COUNTERS
 * Hardware performance monitoring
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef struct s7t_perf_counters {
    uint64_t cycles;
    uint64_t instructions;
    uint64_t cache_misses;
    uint64_t branch_misses;
    uint64_t memory_stalls;
} s7t_perf_counters_t;

/* Read performance counters (platform-specific) */
S7T_INLINE void s7t_perf_read(s7t_perf_counters_t* counters) {
    counters->cycles = s7t_cycles();
    
#ifdef __linux__
    /* Could use perf_event_open() for detailed counters */
    counters->instructions = 0;
    counters->cache_misses = 0;
    counters->branch_misses = 0;
    counters->memory_stalls = 0;
#else
    /* Placeholder for other platforms */
    counters->instructions = 0;
    counters->cache_misses = 0;
    counters->branch_misses = 0;
    counters->memory_stalls = 0;
#endif
}

/* Calculate deltas between readings */
S7T_INLINE void s7t_perf_delta(const s7t_perf_counters_t* start,
                               const s7t_perf_counters_t* end,
                               s7t_perf_counters_t* delta) {
    delta->cycles = end->cycles - start->cycles;
    delta->instructions = end->instructions - start->instructions;
    delta->cache_misses = end->cache_misses - start->cache_misses;
    delta->branch_misses = end->branch_misses - start->branch_misses;
    delta->memory_stalls = end->memory_stalls - start->memory_stalls;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * CYCLE BUDGET TRACKING
 * Track cycle consumption against budgets
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef struct s7t_cycle_budget {
    const char* operation;
    uint32_t    budget;
    uint32_t    actual_min;
    uint32_t    actual_max;
    uint32_t    actual_avg;
    uint32_t    count;
    uint64_t    total;
} s7t_cycle_budget_t;

/* Fixed-size budget tracker */
typedef struct s7t_budget_tracker {
    s7t_cycle_budget_t budgets[64];
    uint32_t count;
} s7t_budget_tracker_t;

/* Register operation budget */
S7T_INLINE uint32_t s7t_budget_register(s7t_budget_tracker_t* tracker,
                                       const char* operation,
                                       uint32_t budget) {
    if (tracker->count >= 64) return (uint32_t)-1;
    
    uint32_t id = tracker->count++;
    tracker->budgets[id].operation = operation;
    tracker->budgets[id].budget = budget;
    tracker->budgets[id].actual_min = UINT32_MAX;
    tracker->budgets[id].actual_max = 0;
    tracker->budgets[id].actual_avg = 0;
    tracker->budgets[id].count = 0;
    tracker->budgets[id].total = 0;
    
    return id;
}

/* Update budget statistics */
S7T_INLINE void s7t_budget_update(s7t_budget_tracker_t* tracker,
                                  uint32_t id,
                                  uint32_t cycles) {
    if (id >= tracker->count) return;
    
    s7t_cycle_budget_t* budget = &tracker->budgets[id];
    
    if (cycles < budget->actual_min) budget->actual_min = cycles;
    if (cycles > budget->actual_max) budget->actual_max = cycles;
    
    budget->total += cycles;
    budget->count++;
    budget->actual_avg = (uint32_t)(budget->total / budget->count);
}

/* Check if budget violated */
S7T_INLINE int s7t_budget_violated(const s7t_budget_tracker_t* tracker, uint32_t id) {
    if (id >= tracker->count) return 0;
    return tracker->budgets[id].actual_max > tracker->budgets[id].budget;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * CACHE BEHAVIOR ANALYSIS
 * Detect cache-unfriendly access patterns
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef struct s7t_cache_stats {
    uint64_t accesses;
    uint64_t predicted_misses;
    uint64_t stride_changes;
    uint64_t last_addr;
    int64_t  last_stride;
} s7t_cache_stats_t;

/* Track memory access pattern */
S7T_INLINE void s7t_cache_track(s7t_cache_stats_t* stats, const void* addr) {
    uint64_t current = (uint64_t)addr;
    stats->accesses++;
    
    if (stats->last_addr != 0) {
        int64_t stride = (int64_t)(current - stats->last_addr);
        
        /* Detect stride change (bad for prefetcher) */
        if (stats->last_stride != 0 && stride != stats->last_stride) {
            stats->stride_changes++;
        }
        
        /* Predict miss if stride > cache line */
        if (stride < 0) stride = -stride;
        if (stride > S7T_CACHE_LINE_SIZE) {
            stats->predicted_misses++;
        }
        
        stats->last_stride = stride;
    }
    
    stats->last_addr = current;
}

/* Calculate cache efficiency percentage */
S7T_INLINE uint32_t s7t_cache_efficiency(const s7t_cache_stats_t* stats) {
    if (stats->accesses == 0) return 100;
    uint64_t hits = stats->accesses - stats->predicted_misses;
    return (uint32_t)((hits * 100) / stats->accesses);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * BRANCH PREDICTION ANALYSIS
 * Track branch patterns for predictability
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef struct s7t_branch_stats {
    uint64_t total_branches;
    uint64_t pattern_mask;      /* Last 64 branch outcomes */
    uint32_t predictable;       /* Branches with clear patterns */
    uint32_t unpredictable;     /* Random branches */
} s7t_branch_stats_t;

/* Track branch outcome */
S7T_INLINE void s7t_branch_track(s7t_branch_stats_t* stats, int taken) {
    stats->total_branches++;
    stats->pattern_mask = (stats->pattern_mask << 1) | (taken ? 1 : 0);
    
    /* Simple pattern detection: all 0s, all 1s, or alternating */
    uint64_t mask = stats->pattern_mask;
    if (mask == 0 || mask == UINT64_MAX || 
        mask == 0xAAAAAAAAAAAAAAAAULL || mask == 0x5555555555555555ULL) {
        stats->predictable++;
    } else {
        /* Check for other simple patterns */
        uint32_t changes = s7t_popcount((uint32_t)(mask ^ (mask >> 1)));
        if (changes <= 2) {
            stats->predictable++;
        } else {
            stats->unpredictable++;
        }
    }
}

/* Get branch predictability percentage */
S7T_INLINE uint32_t s7t_branch_predictability(const s7t_branch_stats_t* stats) {
    uint32_t total = stats->predictable + stats->unpredictable;
    if (total == 0) return 100;
    return (stats->predictable * 100) / total;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * PERFORMANCE GATES
 * Automatic validation of performance requirements
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef enum {
    S7T_GATE_PASS = 0,
    S7T_GATE_WARN = 1,
    S7T_GATE_FAIL = 2
} s7t_gate_result_t;

typedef struct s7t_perf_gate {
    const char*       name;
    s7t_gate_result_t (*check)(void* context);
    void*             context;
    s7t_gate_result_t last_result;
} s7t_perf_gate_t;

/* Standard gate: cycle budget */
S7T_INLINE s7t_gate_result_t s7t_gate_cycles(void* context) {
    s7t_cycle_budget_t* budget = (s7t_cycle_budget_t*)context;
    
    if (budget->actual_max <= budget->budget) {
        return S7T_GATE_PASS;
    } else if (budget->actual_max <= budget->budget * 1.1) {
        return S7T_GATE_WARN; /* Within 10% */
    } else {
        return S7T_GATE_FAIL;
    }
}

/* Standard gate: cache efficiency */
S7T_INLINE s7t_gate_result_t s7t_gate_cache(void* context) {
    s7t_cache_stats_t* stats = (s7t_cache_stats_t*)context;
    uint32_t efficiency = s7t_cache_efficiency(stats);
    
    if (efficiency >= 95) {
        return S7T_GATE_PASS;
    } else if (efficiency >= 90) {
        return S7T_GATE_WARN;
    } else {
        return S7T_GATE_FAIL;
    }
}

/* Standard gate: branch predictability */
S7T_INLINE s7t_gate_result_t s7t_gate_branch(void* context) {
    s7t_branch_stats_t* stats = (s7t_branch_stats_t*)context;
    uint32_t predictability = s7t_branch_predictability(stats);
    
    if (predictability >= 99) {
        return S7T_GATE_PASS;
    } else if (predictability >= 95) {
        return S7T_GATE_WARN;
    } else {
        return S7T_GATE_FAIL;
    }
}

/* ═══════════════════════════════════════════════════════════════════════════
 * PERFORMANCE REPORT
 * Generate performance compliance report
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef struct s7t_perf_report {
    s7t_budget_tracker_t* budgets;
    s7t_cache_stats_t*    cache_stats;
    s7t_branch_stats_t*   branch_stats;
    s7t_perf_gate_t*      gates;
    uint32_t              num_gates;
    char                  buffer[4096]; /* Report text buffer */
} s7t_perf_report_t;

/* Generate text report */
S7T_INLINE const char* s7t_perf_report_generate(s7t_perf_report_t* report) {
    char* p = report->buffer;
    char* end = report->buffer + sizeof(report->buffer) - 1;
    
    /* Header */
    p += snprintf(p, end - p, "=== Seven Tick Performance Report ===\n\n");
    
    /* Cycle budgets */
    if (report->budgets) {
        p += snprintf(p, end - p, "Cycle Budgets:\n");
        for (uint32_t i = 0; i < report->budgets->count; i++) {
            s7t_cycle_budget_t* b = &report->budgets->budgets[i];
            const char* status = b->actual_max <= b->budget ? "PASS" : "FAIL";
            p += snprintf(p, end - p, 
                "  %-20s: budget=%3u actual=[%3u,%3u,~%3u] %s\n",
                b->operation, b->budget, b->actual_min, b->actual_max, 
                b->actual_avg, status);
        }
        p += snprintf(p, end - p, "\n");
    }
    
    /* Cache statistics */
    if (report->cache_stats) {
        uint32_t efficiency = s7t_cache_efficiency(report->cache_stats);
        p += snprintf(p, end - p, "Cache Performance:\n");
        p += snprintf(p, end - p, "  Efficiency: %u%%\n", efficiency);
        p += snprintf(p, end - p, "  Stride changes: %llu\n", 
                     (unsigned long long)report->cache_stats->stride_changes);
        p += snprintf(p, end - p, "\n");
    }
    
    /* Branch statistics */
    if (report->branch_stats) {
        uint32_t predictability = s7t_branch_predictability(report->branch_stats);
        p += snprintf(p, end - p, "Branch Prediction:\n");
        p += snprintf(p, end - p, "  Predictability: %u%%\n", predictability);
        p += snprintf(p, end - p, "  Total branches: %llu\n",
                     (unsigned long long)report->branch_stats->total_branches);
        p += snprintf(p, end - p, "\n");
    }
    
    /* Gates summary */
    if (report->gates && report->num_gates > 0) {
        p += snprintf(p, end - p, "Performance Gates:\n");
        uint32_t passed = 0, warned = 0, failed = 0;
        
        for (uint32_t i = 0; i < report->num_gates; i++) {
            s7t_perf_gate_t* gate = &report->gates[i];
            gate->last_result = gate->check(gate->context);
            
            const char* result_str = "UNKNOWN";
            switch (gate->last_result) {
                case S7T_GATE_PASS: result_str = "PASS"; passed++; break;
                case S7T_GATE_WARN: result_str = "WARN"; warned++; break;
                case S7T_GATE_FAIL: result_str = "FAIL"; failed++; break;
            }
            
            p += snprintf(p, end - p, "  %-20s: %s\n", gate->name, result_str);
        }
        
        p += snprintf(p, end - p, "\nSummary: %u passed, %u warnings, %u failed\n",
                     passed, warned, failed);
    }
    
    *p = '\0';
    return report->buffer;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * PERFORMANCE MACROS
 * Convenience macros for common measurements
 * ═══════════════════════════════════════════════════════════════════════════ */

/* Measure cycles for a block of code */
#define S7T_MEASURE_CYCLES(var, code) do { \
    uint64_t _start = s7t_cycles(); \
    code; \
    var = s7t_cycles() - _start; \
} while(0)

/* Assert maximum cycles with custom handler */
#define S7T_ASSERT_MAX_CYCLES(code, max, handler) do { \
    uint64_t _cycles; \
    S7T_MEASURE_CYCLES(_cycles, code); \
    if (s7t_unlikely(_cycles > (max))) { \
        handler(_cycles, (max)); \
    } \
} while(0)

/* Track operation in budget tracker */
#define S7T_TRACK_OPERATION(tracker, id, code) do { \
    uint64_t _cycles; \
    S7T_MEASURE_CYCLES(_cycles, code); \
    s7t_budget_update(tracker, id, (uint32_t)_cycles); \
} while(0)

#ifdef __cplusplus
}
#endif

#endif /* S7T_PERF_H */