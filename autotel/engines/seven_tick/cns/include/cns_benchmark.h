/*  ─────────────────────────────────────────────────────────────
    cns_benchmark.h  –  CNS Benchmark Interface (v1.0)
    7-tick performance benchmarking framework
    ───────────────────────────────────────────────────────────── */
#ifndef CNS_BENCHMARK_H
#define CNS_BENCHMARK_H

#include <stdint.h>
#include <stdbool.h>

/*═══════════════════════════════════════════════════════════════
  Performance Targets
  ═══════════════════════════════════════════════════════════════*/

#define CNS_SEVEN_TICK_TARGET_NS 10.0    // L1 tier: 7-tick target
#define CNS_L2_TIER_TARGET_NS    100.0   // L2 tier: Sub-100ns
#define CNS_L3_TIER_TARGET_NS    1000.0  // L3 tier: Sub-1μs

/*═══════════════════════════════════════════════════════════════
  Benchmark Configuration
  ═══════════════════════════════════════════════════════════════*/

typedef struct {
    const char* benchmark_name;
    uint64_t iterations;
    uint64_t warmup_iterations;
    uint64_t batch_size;
    bool verbose;
    bool validate_correctness;
} cns_benchmark_config_t;

/*═══════════════════════════════════════════════════════════════
  Benchmark Results
  ═══════════════════════════════════════════════════════════════*/

typedef struct {
    const char* operation_name;
    const char* component_name;
    uint64_t total_operations;
    uint64_t total_time_ns;
    uint64_t min_time_ns;
    uint64_t max_time_ns;
    double avg_time_ns;
    double throughput_ops_per_sec;
    double p50_time_ns;
    double p95_time_ns;
    double p99_time_ns;
    bool target_achieved;
    const char* performance_tier;
} cns_benchmark_result_t;

/*═══════════════════════════════════════════════════════════════
  Command Interface
  ═══════════════════════════════════════════════════════════════*/

// Main benchmark command entry point
int cmd_benchmark(int argc, char** argv);

#endif /* CNS_BENCHMARK_H */