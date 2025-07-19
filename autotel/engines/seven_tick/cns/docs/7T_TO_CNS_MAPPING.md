# 7T Engine to CNS Command Mapping

## Executive Summary

This document maps the 7T Engine components to CNS (Command Nano Stack) command structure, enabling 7-tick compliant command-line access to advanced 7T functionality.

## 7T Component Analysis

### Core 7T Components Identified

1. **7t_tpot.h** - Machine Learning Pipeline Optimization
   - Dataset management
   - Pipeline optimization
   - Algorithm registry
   - Performance benchmarking

2. **pm7t.h** - Process Mining Engine
   - Event log management
   - Process discovery algorithms
   - Conformance checking
   - Performance analysis
   - Bottleneck detection

3. **telemetry7t.h** - High-Performance Observability
   - Span management
   - Performance tracking
   - Context propagation
   - 7-tick optimized tracing

4. **7t_benchmark_framework.h** - Performance Measurement
   - Cycle-accurate timing
   - Statistical analysis
   - Hardware detection
   - Regression detection

5. **7t_unit_test_framework.h** - Testing Infrastructure
   - Assertion macros
   - Performance assertions
   - Test suite management
   - Mock/stub support

## CNS Command Mapping Strategy

### Command Categories

```
CNS_CAT_ML       - Machine Learning (7t_tpot)
CNS_CAT_PROCESS  - Process Mining (pm7t)
CNS_CAT_TELEMETRY - Observability (telemetry7t)
CNS_CAT_BENCH    - Benchmarking
CNS_CAT_TEST     - Testing
```

### Proposed Command Structure

#### Machine Learning Commands (7t_tpot → CNS)

```c
// cmd_ml.c - Machine Learning commands
cns ml train     [-d dataset] [-p pipeline] [-t timeout]
cns ml optimize  [-d dataset] [-g generations] [-s population]
cns ml evaluate  [-p pipeline] [-d dataset] [-m metrics]
cns ml predict   [-m model] [-i input] [-o output]
```

**Mapping:**
- `create_iris_dataset()` → `cns ml dataset -t iris`
- `optimize_pipeline_7t()` → `cns ml optimize`
- `evaluate_pipeline_7t()` → `cns ml evaluate`

#### Process Mining Commands (pm7t → CNS)

```c
// cmd_process.c - Process Mining commands
cns pm import    [-f file] [-t type] // Import event logs
cns pm discover  [-a algorithm] [-l log] // Discover process
cns pm analyze   [-l log] [-m metric] // Analyze performance
cns pm conform   [-m model] [-l log] // Check conformance
cns pm bottleneck [-l log] [-t threshold] // Find bottlenecks
```

**Mapping:**
- `pm7t_import_xes()` → `cns pm import -t xes`
- `pm7t_discover_alpha_algorithm()` → `cns pm discover -a alpha`
- `pm7t_analyze_bottlenecks()` → `cns pm bottleneck`

#### Telemetry Commands (telemetry7t → CNS)

```c
// cmd_trace.c - Telemetry commands
cns trace start  [-n name] [-o operation] // Start span
cns trace end    [-s status] // End current span
cns trace view   [-f format] [-s span] // View traces
cns trace export [-f format] [-o output] // Export traces
cns trace enable/disable // Toggle telemetry
```

**Mapping:**
- `telemetry7t_span_begin()` → `cns trace start`
- `telemetry7t_span_end()` → `cns trace end`
- `telemetry7t_export_json()` → `cns trace export -f json`

#### Benchmark Commands (7t_benchmark → CNS)

```c
// cmd_bench.c - Benchmark commands
cns bench run    [-t test] [-i iterations] // Run benchmark
cns bench suite  [-s suite] [-o output] // Run suite
cns bench report [-f format] [-d detailed] // Show results
cns bench compare [-b baseline] [-c current] // Compare results
```

**Mapping:**
- `benchmark_execute_single()` → `cns bench run`
- `benchmark_suite_print_summary()` → `cns bench report`
- `regression_detector_check()` → `cns bench compare`

#### Test Commands (7t_unit_test → CNS)

```c
// cmd_test.c - Testing commands
cns test run     [-s suite] [-t test] // Run tests
cns test list    [-s suite] // List available tests
cns test report  [-f format] [-v verbose] // Test report
cns test perf    [-t test] [-m max_cycles] // Performance test
```

**Mapping:**
- `test_run_suite()` → `cns test run`
- `test_suite_print_summary()` → `cns test report`
- `ASSERT_PERFORMANCE()` → `cns test perf`

## Implementation Architecture

### Command Handler Template

```c
// Standard 7T command handler (< 7 ticks)
CNS_HANDLER(cns_cmd_ml_train) {
    // Tick 1: Parse arguments
    const char* dataset = cns_get_arg(cmd, "-d", "iris");
    const char* pipeline = cns_get_arg(cmd, "-p", "auto");
    
    // Tick 2-3: Load dataset
    Dataset7T* data = load_dataset_7t(dataset);
    if (!data) return CNS_ERR_RESOURCE;
    
    // Tick 4-5: Create/load pipeline
    Pipeline7T* pipe = create_pipeline_7t(pipeline);
    
    // Tick 6: Execute training
    train_pipeline_7t(pipe, data);
    
    // Tick 7: Return result
    return CNS_OK;
}
```

### Integration Points

1. **Memory Management**
   - Use s7t_arena_t for allocation
   - Leverage 7T's cache-aligned structures
   - Maintain < 7 tick allocation overhead

2. **Performance Tracking**
   - Integrate telemetry7t spans
   - Use benchmark framework for command timing
   - Track per-command cycle counts

3. **Error Handling**
   - Map 7T errors to CNS result codes
   - Maintain error context in telemetry
   - Support graceful degradation

## Command Examples

### Machine Learning Workflow
```bash
# Load dataset and optimize pipeline
cns ml dataset -t boston
cns ml optimize -d boston -g 100 -s 50
cns ml evaluate -p best_pipeline -m accuracy

# Enable tracing for ML operations
cns trace enable
cns ml train -d iris -p random_forest
cns trace view -f json
```

### Process Mining Workflow
```bash
# Import and analyze event log
cns pm import -f hospital.xes -t xes
cns pm discover -a alpha -l hospital
cns pm bottleneck -l hospital -t 0.8
cns pm conform -m discovered_model -l hospital
```

### Benchmarking Workflow
```bash
# Run performance benchmarks
cns bench run -t ml_pipeline -i 1000
cns bench suite -s full_7t
cns bench report -f detailed
cns bench compare -b baseline.json -c current
```

## Benefits of CNS Integration

1. **Unified Interface** - Single command interface for all 7T components
2. **7-Tick Compliance** - All commands execute within tick budget
3. **Composability** - Commands can be piped and scripted
4. **Observability** - Built-in telemetry for all operations
5. **Performance** - Benchmarking integrated at command level

## Next Steps

1. Implement command handlers for each category
2. Create command registration system
3. Add tab completion support
4. Integrate with existing CNS commands
5. Add comprehensive help system
6. Create command aliases for common workflows

## Command Naming Conventions

- **Noun-Verb** pattern: `cns <component> <action>`
- **Short aliases**: `ml` → machine learning, `pm` → process mining
- **Consistent flags**: `-d` data/dataset, `-f` format/file, `-o` output
- **7-tick names**: Keep command names under 7 characters where possible