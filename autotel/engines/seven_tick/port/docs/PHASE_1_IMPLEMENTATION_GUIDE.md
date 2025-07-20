# Phase 1 Implementation Guide: Instrumentation & Baseline Calibration

## Overview

This guide provides step-by-step instructions for implementing Phase 1 of the CNS port, focusing on establishing the sensory apparatus and baseline calibration for the v8 codebase.

## Prerequisites

- CNS v8 codebase with basic structure in place
- Access to the original CNS source files
- Understanding of 7-tick performance principles
- Familiarity with OpenTelemetry concepts

## Implementation Steps

### Step 1: Establish Telemetry Infrastructure

#### 1.1 Copy Core Telemetry Files

```bash
# Copy the telemetry weavers
cp cns/src/cns_weaver.h port/src/
cp cns/src/cns_optional_otel.h port/src/

# Copy telemetry implementation
cp cns/src/domains/telemetry.c port/src/
cp cns/src/engines/telemetry.c port/src/
```

#### 1.2 Verify Telemetry Integration

**Validation Criteria:**
- All telemetry spans must be 7-tick compliant
- Cycle counting must use `__builtin_readcyclecounter()`
- OpenTelemetry integration must be optional
- No telemetry overhead should exceed 7 cycles

**Test Command:**
```bash
cd port
make test_telemetry
```

### Step 2: Implement Honest Benchmarking

#### 2.1 Copy Benchmark Files

```bash
# Copy the honest benchmark implementations
cp cns/src/real_7tick_benchmark.c port/src/
cp cns/src/cmd_benchmark_fixed.c port/src/
cp cns/src/cmd_benchmark_real.c port/src/

# Copy benchmark documentation
cp cns/src/binary_materializer/BENCHMARK_RESULTS.md port/docs/
```

#### 2.2 Verify Benchmark Accuracy

**Validation Criteria:**
- Benchmarks must use `volatile` globals to prevent optimization
- Real workloads must process actual data
- Measurements must be statistically significant (minimum 1000 iterations)
- Results must be repeatable within 5% variance

**Test Command:**
```bash
cd port
./cns_benchmark --real --iterations 10000
```

### Step 3: Deploy the Gatekeeper

#### 3.1 Copy Gatekeeper Files

```bash
# Copy the gatekeeper implementation
cp cns/src/gatekeeper.c port/src/
cp cns/src/gatekeeper_test.c port/src/
```

#### 3.2 Verify Gatekeeper Functionality

**Validation Criteria:**
- CTQ checks must be non-negotiable
- Chatman Constant (≤7 cycles) must be enforced
- Six Sigma quality levels must be validated
- Throughput requirements must be met

**Test Command:**
```bash
cd port
./cns_validation --gatekeeper
```

## Validation Procedures

### Validation 1: Full System Trace Generation

**Objective:** Generate a complete telemetry trace for a high-level user scenario.

**Procedure:**
1. Execute the user scenario simulation:
   ```bash
   cd port
   ./simulate_user_scenarios.sh
   ```

2. Capture the telemetry output:
   ```bash
   ./cns_benchmark --trace --output trace.json
   ```

3. Validate the trace:
   - All operations must have telemetry spans
   - Cycle counts must be recorded
   - Span hierarchy must be correct
   - No gaps in the trace

**Success Criteria:**
- Complete trace generated without errors
- All spans have meaningful attributes
- Cycle counts are within expected ranges
- Trace can be parsed and analyzed

### Validation 2: Gatekeeper Baseline Report

**Objective:** Generate a repeatable baseline report of system performance and compliance.

**Procedure:**
1. Run the gatekeeper validation:
   ```bash
   cd port
   ./cns_validation --gatekeeper --baseline
   ```

2. Analyze the report:
   - Check P95 cycle compliance
   - Verify throughput metrics
   - Confirm Six Sigma levels
   - Review quality indicators

3. Repeat the test:
   ```bash
   ./cns_validation --gatekeeper --baseline --repeat 5
   ```

**Success Criteria:**
- Consistent results across multiple runs
- All CTQ checks pass
- Performance metrics meet requirements
- Report format is standardized

## Performance Requirements

### Telemetry Requirements

| Metric | Requirement | Rationale |
|--------|-------------|-----------|
| Span Start Overhead | ≤3 cycles | Must not impact 7-tick compliance |
| Span End Overhead | ≤4 cycles | Total span overhead ≤7 cycles |
| Memory Allocation | ≤64 bytes per span | Minimal memory footprint |
| Thread Safety | Lock-free | No contention in high-throughput scenarios |

### Benchmark Requirements

| Metric | Requirement | Rationale |
|--------|-------------|-----------|
| Measurement Accuracy | ±1 cycle | Precise cycle counting |
| Statistical Significance | ≥1000 iterations | Reliable performance data |
| Repeatability | ±5% variance | Consistent results |
| Real Workload | Non-optimizable | True performance measurement |

### Gatekeeper Requirements

| Metric | Requirement | Rationale |
|--------|-------------|-----------|
| Chatman Constant | ≤7 cycles P95 | Core performance law |
| Throughput | ≥10 MOPS | Minimum viable performance |
| Six Sigma | ≥4.0 sigma | Quality standard |
| Cpk | ≥1.3 | Process capability |

## Troubleshooting

### Common Issues

1. **Telemetry Overhead Too High**
   - Solution: Use minimal OpenTelemetry implementation
   - Check: `cns_optional_otel.h` configuration

2. **Benchmark Results Inconsistent**
   - Solution: Increase iteration count
   - Check: Use `volatile` variables

3. **Gatekeeper CTQ Failures**
   - Solution: Optimize critical paths
   - Check: Review performance bottlenecks

4. **Trace Generation Failures**
   - Solution: Verify span lifecycle management
   - Check: Memory allocation patterns

### Debug Commands

```bash
# Debug telemetry
./cns_benchmark --debug --telemetry

# Debug benchmarks
./cns_benchmark --debug --benchmark

# Debug gatekeeper
./cns_validation --debug --gatekeeper

# Generate detailed reports
./cns_benchmark --verbose --report
```

## Next Steps

Upon successful completion of Phase 1:

1. **Document Baseline Metrics**: Record all performance measurements
2. **Identify Bottlenecks**: Analyze areas exceeding 7-tick compliance
3. **Prepare for Phase 2**: Set up optimization framework
4. **Validate Emergence**: Monitor for unexpected system behaviors

## Success Indicators

- ✅ Complete telemetry trace generation
- ✅ Repeatable gatekeeper baseline report
- ✅ All CTQ checks passing
- ✅ Performance within 7-tick compliance
- ✅ System fully observable and measurable

Phase 1 establishes the empirical foundation for the Dark 80/20 optimizations that will follow in Phase 2. 