# CNS Gatekeeper - Quality Gate for Chatman Nano-Stack

## Overview

The CNS Gatekeeper is an automated quality-and-performance gate that enforces the **Chatman constant** (≤7 cycles) and other Critical-to-Quality (CTQ) requirements for the Chatman Nano-Stack (CNS). It serves as the single source of truth for CNS health and blocks any changes that could break the nanosecond-level performance guarantees.

## Core Responsibilities

### 1. Correctness Validation
- Loads representative test datasets (500+ SPARQL tests, 4+ ontologies)
- Asserts that every query, template render, and SHACL validation returns expected results
- Validates functional correctness across all CNS kernels

### 2. Cycle Budget Enforcement
- Measures p95 latency (in CPU cycles) for each red-zone kernel
- Enforces the **Chatman constant**: p95 ≤ 7 cycles
- Calculates sigma levels and process capability indices (Cpk)
- Tracks violations and provides detailed performance analysis

### 3. Throughput Validation
- Drives tight loops of 1M+ operations
- Computes Millions of Operations Per Second (MOPS)
- Enforces minimum throughput: ≥10 MOPS
- Validates sustained performance under load

### 4. Ontology Parsing Validation
- Reloads CNS-Core TTL and SHACL shapes
- Asserts no errors in tape compiler or SHACL validation
- Validates semantic consistency of ontology definitions

## Architecture

### File Structure
```
cns/
├── src/gatekeeper.c              # Main Gatekeeper implementation
├── include/cns/gatekeeper.h      # Gatekeeper API header
├── docs/ontology/cns-telemetry.ttl  # Telemetry spans ontology
├── codegen/extract_spans.py      # TTL to JSON extractor
├── codegen/weaver_main.c         # OpenTelemetry code generator
├── templates/otel_header.h.j2    # OTEL header template
├── templates/otel_inject.c.j2    # OTEL injection template
└── build/cns_spans.json          # Generated spans data
```

### Key Components

#### 1. Gatekeeper Core (`src/gatekeeper.c`)
- Implements all CTQ test functions
- Provides cycle measurement and sigma calculation
- Generates comprehensive performance reports
- Enforces exit codes for CI/CD integration

#### 2. Telemetry Ontology (`docs/ontology/cns-telemetry.ttl`)
- Defines all telemetry spans in RDF/TTL format
- Links spans to C functions via `cns:providesFunction`
- Enables automatic instrumentation generation
- Supports SHACL validation of span definitions

#### 3. Code Generation Pipeline
- **Extract**: Python script reads TTL → JSON
- **Weave**: C weaver uses cjinja to generate OTEL code
- **Inject**: Auto-generated instrumentation wraps existing functions

## Usage

### Building Gatekeeper
```bash
cd cns
make gatekeeper
```

### Running Gatekeeper
```bash
# Run all CTQ tests
./gatekeeper

# Run individual CTQ test
./gatekeeper --ctq correctness
./gatekeeper --ctq cycle-budget
./gatekeeper --ctq throughput
./gatekeeper --ctq ontology
```

### Integration with CI/CD
```bash
# Gatekeeper blocks builds on failure
make gatekeeper
if [ $? -ne 0 ]; then
    echo "Gatekeeper failed - build blocked"
    exit 1
fi
```

### OpenTelemetry Code Generation
```bash
# Extract spans from TTL
make extract-spans

# Generate OTEL instrumentation
make weave

# Full pipeline
make extract-spans && make weave
```

## CTQ Requirements

### CTQ-1: Correctness
- **Requirement**: All functional tests pass
- **Validation**: 500+ regression tests
- **Exit Code**: 0 = pass, 1 = fail

### CTQ-2: Cycle Budget
- **Requirement**: p95 ≤ 7 cycles (Chatman constant)
- **Validation**: 1M+ operation samples
- **Sigma Level**: ≥4.0 (short-term)
- **Cpk**: ≥1.3 (process capability)

### CTQ-3: Throughput
- **Requirement**: ≥10 MOPS
- **Validation**: Sustained load testing
- **Measurement**: Operations per second

### CTQ-4: Ontology Parsing
- **Requirement**: No TTL/SHACL errors
- **Validation**: Full ontology reload
- **Consistency**: Semantic validation

## Performance Metrics

### Sigma Calculation
```c
// Short-term sigma level for one-sided spec (upper)
double gatekeeper_sigma(double USL, double mu, double sigma) {
    return (USL - mu) / sigma;  // Cpk
}

// Convert sigma level to parts-per-million
double gatekeeper_dpm(double sigma_st) {
    return 0.5 * erfc(sigma_st / M_SQRT2) * 1e6;
}
```

### Six Sigma Mapping
| Metric | USL | Target (μ) | σ | Cp | Cpk | Short-term σ | Long-term σ |
|--------|-----|------------|---|----|-----|--------------|-------------|
| CPU cycles | 7 | 5.8 | 0.30 | 3.9 | 1.3 | 4.0 σ (63 ppm) | 2.5 σ (1.2%) |
| Latency | 10ns | 7.2ns | 0.50ns | 3.3 | 1.9 | 4.5 σ (32 ppm) | 3.0 σ (2,700 ppm) |

### Cycle Measurement
```c
// Cross-platform cycle counter
static inline uint64_t gatekeeper_get_cycles(void) {
#if defined(__x86_64__) || defined(__i386__)
    uint32_t lo, hi;
    __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
#elif defined(__aarch64__)
    uint64_t val;
    __asm__ __volatile__("mrs %0, PMCCNTR_EL0" : "=r"(val));
    return val;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000ULL + tv.tv_usec;
#endif
}
```

## OpenTelemetry Integration

### Automatic Instrumentation
The Gatekeeper system automatically generates OpenTelemetry instrumentation from the TTL ontology:

1. **TTL Definition**:
```ttl
cns:spqlAskSpan a cns:TelemetrySpan ;
    cns:providesFunction cns:spqlAsk ;
    rdfs:label "SPARQL Ask Span" .
```

2. **Generated Code**:
```c
// Auto-generated from TTL
static inline opentelemetry_span_t* cns_otel_start_spqlAsk() {
    return opentelemetry_tracer_start_span("SPARQL Ask Span", NULL, 0);
}

static inline void cns_otel_end_spqlAsk(opentelemetry_span_t* s) {
    if (s) opentelemetry_span_end(s);
}
```

3. **Performance Tracking**:
```c
// Automatic cycle counting and violation detection
uint64_t start_cycles = gatekeeper_get_cycles();
// ... function execution ...
uint64_t end_cycles = gatekeeper_get_cycles();
uint64_t cycles = end_cycles - start_cycles;

if (cycles > 7) {
    cns_otel_set_attribute_spqlAsk(span, "violation", "true");
}
```

### Weaver Pipeline
```bash
# 1. Extract spans from TTL
python3 codegen/extract_spans.py docs/ontology/cns-telemetry.ttl build/cns_spans.json

# 2. Generate OTEL code
./cns_weaver build/cns_spans.json templates/otel_header.h.j2 templates/otel_inject.c.j2 src

# 3. Result: Auto-generated instrumentation
src/cns_otel.h
src/cns_otel_inject.c
```

## Integration with 7T Principles

### Physics Compliance
The Gatekeeper enforces the 7T principles through automated validation:

1. **Zero Allocation at Steady State**
   - Validates no heap allocation during normal operation
   - Tracks memory usage patterns

2. **ID-Based Behavior**
   - Validates enum-based type identification
   - Ensures hash-based string interning

3. **Data Locality**
   - Validates cache-line aligned structures
   - Ensures contiguous memory layouts

4. **Compile-Time Wiring**
   - Validates macro-based template instantiation
   - Ensures static lookup tables

5. **≤1 Predictable Branch per Operation**
   - Validates direct array indexing
   - Ensures function pointer calls

### Six Sigma Quality
The Gatekeeper implements Six Sigma methodology for nanosecond engineering:

- **Define**: Clear CTQ requirements
- **Measure**: Automated cycle counting
- **Analyze**: Sigma level calculation
- **Improve**: Performance optimization
- **Control**: Continuous monitoring

## Configuration

### Gatekeeper Configuration
```c
typedef struct {
    int chatman_constant_cycles;    // Default: 7
    double min_throughput_mops;     // Default: 10.0
    double min_sigma_level;         // Default: 4.0
    uint64_t sample_size;           // Default: 1,000,000
    int percentile_95;              // Default: 95
    int verbose_output;             // Default: 1
    int save_reports;               // Default: 1
    const char* report_directory;   // Default: "reports"
} CnsGatekeeperConfig;
```

### Environment Variables
```bash
# Enable verbose output
export CNS_GATEKEEPER_VERBOSE=1

# Set custom thresholds
export CNS_CHATMAN_CONSTANT=7
export CNS_MIN_THROUGHPUT=10
export CNS_MIN_SIGMA=4.0

# Enable report saving
export CNS_SAVE_REPORTS=1
export CNS_REPORT_DIR=reports
```

## Troubleshooting

### Common Issues

1. **Cycle Counter Not Available**
   - ARM64: Enable PMCCNTR_EL0 in kernel
   - x86: RDTSC should be available by default
   - Fallback: Uses gettimeofday() for timing

2. **High Cycle Counts**
   - Check for cache misses
   - Verify branch prediction
   - Review memory access patterns

3. **TTL Parsing Errors**
   - Validate TTL syntax
   - Check SHACL shape definitions
   - Verify namespace declarations

### Debug Mode
```bash
# Enable debug output
make debug
./gatekeeper --debug

# Generate detailed reports
./gatekeeper --report-detail=full
```

## Future Enhancements

### Planned Features
1. **Real-time Monitoring**: Live performance dashboards
2. **Predictive Analysis**: ML-based performance prediction
3. **Automated Optimization**: AI-driven code optimization
4. **Distributed Testing**: Multi-node performance validation
5. **Custom CTQs**: User-defined quality requirements

### Integration Roadmap
1. **CI/CD Integration**: GitHub Actions, GitLab CI
2. **Monitoring Integration**: Prometheus, Grafana
3. **Alerting**: Slack, PagerDuty notifications
4. **Compliance**: SOC2, ISO27001 validation

## Conclusion

The CNS Gatekeeper is the cornerstone of quality assurance for the Chatman Nano-Stack, ensuring that every change maintains the nanosecond-level performance guarantees required for production deployment. By automating the validation of the 7T principles and Six Sigma quality standards, it enables rapid development while maintaining the highest levels of performance and reliability. 