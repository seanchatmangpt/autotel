# Pattern: Performance Benchmarking

## Description
This pattern demonstrates how to benchmark and profile the 7T Engine for latency, throughput, and memory usage, ensuring that performance targets are met in production and development environments.

## Code Example
```c
#include "c_src/sparql7t.h"
#include <stdio.h>
#include <stdint.h>
#include <time.h>

static inline uint64_t get_nanoseconds() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

int main() {
    S7TEngine* engine = s7t_create(100000, 1000, 100000);
    for (int i = 0; i < 10000; i++) {
        s7t_add_triple(engine, i, i % 100, i % 200);
    }

    // Benchmark pattern matching
    uint64_t start = get_nanoseconds();
    int result = 0;
    for (int i = 0; i < 1000000; i++) {
        result += s7t_ask_pattern(engine, i % 1000, i % 100, i % 200);
    }
    uint64_t end = get_nanoseconds();
    double avg_ns = (double)(end - start) / 1000000.0;
    double throughput = 1000000.0 * 1000000000.0 / (end - start);

    printf("Average latency: %.2f ns\n", avg_ns);
    printf("Throughput: %.0f ops/sec\n", throughput);

    s7t_destroy(engine);
    return 0;
}
```

## Tips
- Use `-O3 -march=native` for best performance.
- Profile with `perf` and `valgrind` for deeper analysis.
- Monitor memory usage with `valgrind --tool=massif`. 