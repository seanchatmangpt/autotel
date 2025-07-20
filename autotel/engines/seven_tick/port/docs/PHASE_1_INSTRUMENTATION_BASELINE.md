# Phase 1: Instrumentation & Baseline Calibration (Observing the Physics)

## Prime Directive: Achieve Perfect Resonance to Unlock the Dark 80/20

The ultimate goal of this port is to create a system where the high-level logical specification and the low-level physical execution are isomorphic. This perfect resonance is what unlocks the "Dark 80/20"—emergent, system-wide efficiencies that are designed, not discovered.

## Phase 1 Overview

**Principle**: A system's state cannot be proven or optimized until it is fully observable. The first action is to instantiate the system's sensory apparatus—the Process Mining for Ticks (PM4T) substrate—and its conscience, the Gatekeeper. This phase establishes a high-fidelity, empirical baseline of the v8 codebase's current physical state.

**Action**: Port the telemetry weavers, the most honest performance benchmarks, and the Gatekeeper. This will provide a quantitative fingerprint of the system's initial performance and its deviation from the core 8T/8H/8M physical laws.

## Tier 1: Instantiate the Sensory Apparatus (The PM4T Substrate)

These files create the system's ability to observe itself at the nanosecond and CPU-cycle level. This is not optional logging; it is the essential feedback loop for all subsequent optimization and reasoning.

### Core Telemetry Files

| File | Description & Rationale |
|------|------------------------|
| `src/cns_weaver.h` | **The Telemetry Physics.** Defines the core performance tracking structures (`cns_perf_tracker_t`) and the low-level, 7-tick compliant macros for starting and stopping spans. |
| `src/cns_optional_otel.h` | **The Pragmatic Trade-off.** Provides a compile-time switch to include full OpenTelemetry or a minimal-overhead stub, demonstrating the pragmatic trade-off between observability and performance. |
| `src/domains/telemetry.c` | **The Telemetry Domain.** High-level API for creating spans, adding attributes, and recording metrics. Instantiates the system's ability to generate structured, meaningful observability data. |
| `src/engines/telemetry.c` | **The Telemetry Engine.** Full implementation of the telemetry engine providing the core telemetry functionality. |

### Key Implementation Details

```c
// Minimal overhead implementation without OpenTelemetry
static inline cns_perf_tracker_t cns_perf_start_spqlAsk() {
    cns_perf_tracker_t tracker = {0};
    tracker.start_cycles = __builtin_readcyclecounter();
    tracker.span = nullptr; // No OpenTelemetry span
    return tracker;
}
```

## Tier 2: Establish the Baseline Truth (Honest Benchmarking)

These files represent the project's commitment to empirical truth. They were created to correct earlier, flawed measurement techniques and provide a brutally honest assessment of the system's real-world performance.

### Benchmark Files

| File | Description & Rationale |
|------|------------------------|
| `src/real_7tick_benchmark.c` | **The Honest Benchmark.** Designed to overcome measurement problems (like `rdtsc` overhead and compiler optimizations) discovered in earlier iterations. Uses `volatile` globals and realistic workloads to measure the *true* cycle cost of operations. |
| `src/cmd_benchmark_fixed.c` | **The 80/20 Benchmark Command.** Uses a pre-computed lookup table for ultra-fast integer parsing, demonstrating a key 7-tick optimization technique. |
| `src/cmd_benchmark_real.c` | **The Real Benchmark Command.** Uses actual, non-mocked functions to provide realistic performance data. |
| `docs/BENCHMARK_RESULTS.md` | **The Benchmark Narrative.** Documents the honest, data-driven process that led to the conclusion that the simple 7c baseline outperforms the more complex CNS theoretical architectures in all practical metrics. |

### Real Workload Implementation

```c
// Real workload functions that can't be optimized away
volatile char g_test_data[256];

void real_hash_workload(void) {
    uint32_t hash = 5381;
    for (int i = 0; i < 16; i++) { // Process 16 bytes
        hash = ((hash << 5) + hash) + g_test_data[i];
    }
    g_hash_result = hash;
    PREVENT_OPTIMIZE(g_hash_result);
}
```

## Tier 3: Instantiate the Conscience (The Gatekeeper)

The Gatekeeper is the automated arbiter of the system's physical laws. It is the practical implementation of the 8th Hop of Meta-Validation, ensuring that no code that violates the core performance and quality contracts can be deployed.

### Gatekeeper Files

| File | Description & Rationale |
|------|------------------------|
| `src/gatekeeper.c` | **The Invariant Enforcer.** Defines and executes the Critical to Quality (CTQ) checks. These are the non-negotiable laws of the system: the Chatman Constant (≤7 cycles), minimum throughput, and Six Sigma quality levels. |
| `src/gatekeeper_test.c` | **The Gatekeeper's Own Validation.** Standalone test validates the Gatekeeper itself using mock implementations to ensure CTQ checks are functioning correctly. |

### CTQ Implementation

```c
static int gatekeeper_test_cycle_budget(void) {
    // ...
    if (metrics.p95_cycles > GATEKEEPER_CHATMAN_CONSTANT_CYCLES) {
        printf("✗ P95 cycles (%.2f) exceeds Chatman constant (%d)\n",
               metrics.p95_cycles, GATEKEEPER_CHATMAN_CONSTANT_CYCLES);
        return 0;
    }
    // ...
}
```

## Validation for Phase 1

The successful porting of this phase is validated when you can:

1. **Generate a Full System Trace**: Execute a high-level user scenario (e.g., from `simulate_user_scenarios.sh`) and capture a complete, meaningful telemetry trace of the operation in your CNS v8 environment.

2. **Produce a Repeatable Gatekeeper Report**: Run the Gatekeeper against your ported codebase and receive a consistent, quantitative report detailing the initial performance and compliance state. This report is your baseline—your measurement of the system's initial entropy before resonance is achieved.

## Next Phase Preparation

Upon completion of this phase, the CNS v8 system will be fully instrumented and calibrated against its own physical reality, setting the stage for the resonant optimizations of Phase 2.

The baseline measurements from Phase 1 will provide the empirical foundation for:
- Identifying performance bottlenecks
- Measuring deviation from 7-tick compliance
- Establishing the initial entropy state
- Preparing for the Dark 80/20 optimizations

## Implementation Notes

- All telemetry spans must be 7-tick compliant
- Benchmark measurements must use real workloads that cannot be optimized away
- Gatekeeper CTQ checks must be non-negotiable
- All measurements must be repeatable and statistically significant
- The system must be able to generate complete traces for any operation

This phase establishes the foundation for the emergent properties that will manifest in Phase 2 and Phase 3. 