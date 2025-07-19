# CNS Benchmark Command

The `benchmark` command provides comprehensive performance testing for the CNS (Cognitive Nano Stack) system, ensuring adherence to the 7-tick performance constraints.

## Overview

The benchmark command is a port of the 7T benchmark framework, adapted specifically for CNS command structure. It measures performance across different tiers:

- **L1 (7-TICK)**: < 10ns average execution time
- **L2 Tier**: < 100ns (sub-100ns performance)
- **L3 Tier**: < 1μs (sub-microsecond performance)

## Usage

```bash
cns benchmark [options] [operations]
```

### Operations

- `hash` - Benchmark the 7-tick hash function
- `lookup` - Benchmark command lookup performance
- `parse` - Benchmark command parsing
- `all` - Run all benchmarks (default)

### Options

- `-i N` - Number of iterations (default: 1,000,000)
- `-w N` - Warmup iterations (default: 10,000)
- `-v` - Verbose output
- `-q` - Quick benchmark mode (100k iterations)
- `-t` - Thorough benchmark mode (10M iterations)
- `-h` - Show help

## Examples

### Quick Hash Benchmark
```bash
cns benchmark -q hash
```

### Thorough All Benchmarks
```bash
cns benchmark -t all
```

### Custom Iterations
```bash
cns benchmark -i 5000000 -w 50000 lookup
```

### Verbose Parse Benchmark
```bash
cns benchmark -v parse
```

## Output Format

The benchmark results include:

1. **Operation Details**
   - Operation name and component
   - Total operations performed
   - Total execution time

2. **Performance Metrics**
   - Average time per operation
   - Min/Max times
   - Percentiles (P50, P95, P99)
   - Throughput (operations/second)

3. **Performance Assessment**
   - Performance tier achieved
   - 7-tick compliance status

### Example Output

```
=== String Hash Benchmark (7T Core) ===
Operations: 1000000
Total time: 12.345 ms
Average: 12.3 ns
Min: 8 ns
Max: 156 ns
P50: 11.2 ns
P95: 14.8 ns
P99: 22.1 ns
Throughput: 81004065 ops/sec
✅ L2 (Sub-100ns): Sub-100ns performance! (12.3 ns)
```

## Integration with CNS

The benchmark command integrates seamlessly with CNS features:

1. **OpenTelemetry Support**: When built with `Makefile.otel`, benchmark operations are traced
2. **7-Tick Validation**: Ensures command execution meets performance constraints
3. **Extensibility**: Easy to add new benchmark operations

## Technical Details

### Implementation

The benchmark command is implemented in `src/cmd_benchmark.c` and includes:

- High-precision nanosecond timing using `clock_gettime(CLOCK_MONOTONIC)`
- Statistical analysis with percentile calculations
- Warmup phases to ensure cache optimization
- Memory-efficient operation with minimal allocations

### Adding New Benchmarks

To add a new benchmark operation:

1. Define a context structure for your operation
2. Create a benchmark operation function matching `cns_benchmark_op_t`
3. Add the benchmark to the command's execution logic

Example:
```c
typedef struct {
    // Your operation context
} my_bench_ctx_t;

static void bench_my_op(void* ctx) {
    my_bench_ctx_t* mctx = (my_bench_ctx_t*)ctx;
    // Perform operation to benchmark
}
```

## Performance Considerations

1. **CPU Affinity**: For best results, consider setting CPU affinity
2. **System Load**: Run benchmarks on an idle system
3. **Frequency Scaling**: Disable CPU frequency scaling if possible
4. **Cache Effects**: Warmup iterations help stabilize cache behavior

## See Also

- CNS Documentation: `/cns/README.md`
- 7T Benchmark Framework: `/verification/7t_benchmark_framework.c`
- Performance Tuning Guide: `/docs/performance.md`