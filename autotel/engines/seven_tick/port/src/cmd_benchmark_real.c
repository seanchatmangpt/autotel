// Real 80/20 Benchmark Command - Actual performance measurement
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <math.h>

// Real performance measurement
static inline uint64_t rdtsc(void) {
#if defined(__x86_64__) || defined(__i386__)
    uint32_t lo, hi;
    __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
#elif defined(__aarch64__)
    uint64_t val;
    __asm__ volatile("mrs %0, cntvct_el0" : "=r"(val));
    return val;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000ULL + ts.tv_nsec;
#endif
}

// Real hash function to benchmark
static uint32_t real_hash(const char* str, size_t len) {
    uint32_t hash = 5381;
    for (size_t i = 0; i < len; i++) {
        hash = ((hash << 5) + hash) + str[i];
    }
    return hash;
}

// Real memory copy to benchmark
static void real_memcopy(void* dst, const void* src, size_t len) {
    char* d = (char*)dst;
    const char* s = (const char*)src;
    for (size_t i = 0; i < len; i++) {
        d[i] = s[i];
    }
}

// Real integer parsing to benchmark
static int real_parse_int(const char* str) {
    int result = 0;
    int sign = 1;
    size_t i = 0;
    
    if (str[0] == '-') {
        sign = -1;
        i = 1;
    }
    
    while (str[i] >= '0' && str[i] <= '9') {
        result = result * 10 + (str[i] - '0');
        i++;
    }
    
    return result * sign;
}

// Real benchmark statistics
typedef struct {
    uint64_t min_cycles;
    uint64_t max_cycles;
    uint64_t total_cycles;
    uint64_t* samples;
    size_t count;
} BenchStats;

// Real statistical analysis
void analyze_stats(BenchStats* stats) {
    if (stats->count == 0) return;
    
    // Sort samples for percentiles (simple bubble sort for 80/20 impl)
    for (size_t i = 0; i < stats->count - 1; i++) {
        for (size_t j = 0; j < stats->count - i - 1; j++) {
            if (stats->samples[j] > stats->samples[j + 1]) {
                uint64_t temp = stats->samples[j];
                stats->samples[j] = stats->samples[j + 1];
                stats->samples[j + 1] = temp;
            }
        }
    }
    
    double avg = (double)stats->total_cycles / stats->count;
    uint64_t p50 = stats->samples[stats->count / 2];
    uint64_t p95 = stats->samples[(stats->count * 95) / 100];
    uint64_t p99 = stats->samples[(stats->count * 99) / 100];
    
    printf("  Min: %llu cycles\n", stats->min_cycles);
    printf("  Max: %llu cycles\n", stats->max_cycles);
    printf("  Avg: %.1f cycles\n", avg);
    printf("  P50: %llu cycles\n", p50);
    printf("  P95: %llu cycles\n", p95);
    printf("  P99: %llu cycles\n", p99);
    
    // 7-tick compliance analysis
    size_t compliant = 0;
    for (size_t i = 0; i < stats->count; i++) {
        if (stats->samples[i] <= 7) compliant++;
    }
    double compliance_rate = (double)compliant / stats->count * 100.0;
    
    printf("  7-tick compliance: %.1f%% (%zu/%zu samples)\n", 
           compliance_rate, compliant, stats->count);
    
    if (compliance_rate >= 90.0) {
        printf("  ✅ Excellent 7-tick compliance\n");
    } else if (compliance_rate >= 75.0) {
        printf("  ⚠️  Good 7-tick compliance\n");
    } else if (compliance_rate >= 50.0) {
        printf("  ⚠️  Moderate 7-tick compliance\n");
    } else {
        printf("  ❌ Poor 7-tick compliance\n");
    }
}

// Real benchmark execution
void run_benchmark(const char* name, void (*bench_func)(void), int iterations) {
    printf("Running %s benchmark (%d iterations)...\n", name, iterations);
    
    BenchStats stats = {
        .min_cycles = UINT64_MAX,
        .max_cycles = 0,
        .total_cycles = 0,
        .samples = malloc(iterations * sizeof(uint64_t)),
        .count = iterations
    };
    
    if (!stats.samples) {
        printf("Failed to allocate memory for benchmark\n");
        return;
    }
    
    // Real benchmark loop
    for (int i = 0; i < iterations; i++) {
        uint64_t start = rdtsc();
        bench_func();
        uint64_t cycles = rdtsc() - start;
        
        stats.samples[i] = cycles;
        stats.total_cycles += cycles;
        
        if (cycles < stats.min_cycles) stats.min_cycles = cycles;
        if (cycles > stats.max_cycles) stats.max_cycles = cycles;
    }
    
    analyze_stats(&stats);
    free(stats.samples);
    printf("\n");
}

// Real benchmark functions
static char test_string[] = "benchmark_test_string_1234567890";
static char copy_buffer[64];
static char int_string[] = "42";

void bench_hash(void) {
    volatile uint32_t result = real_hash(test_string, strlen(test_string));
    (void)result; // Prevent optimization
}

void bench_memcopy(void) {
    real_memcopy(copy_buffer, test_string, sizeof(test_string));
}

void bench_parse_int(void) {
    volatile int result = real_parse_int(int_string);
    (void)result; // Prevent optimization
}

void bench_nop(void) {
    __asm__ volatile("nop");
}

void bench_arithmetic(void) {
    volatile int a = 42;
    volatile int b = 37;
    volatile int c = a + b * 2 - 15;
    (void)c; // Prevent optimization
}

// Real stress test
void stress_test(int duration_seconds) {
    printf("Running stress test for %d seconds...\n", duration_seconds);
    
    time_t start_time = time(NULL);
    uint64_t operations = 0;
    uint64_t total_cycles = 0;
    uint64_t min_cycles = UINT64_MAX;
    uint64_t max_cycles = 0;
    
    while (time(NULL) - start_time < duration_seconds) {
        uint64_t start = rdtsc();
        
        // Mix of operations
        bench_hash();
        bench_arithmetic();
        bench_nop();
        
        uint64_t cycles = rdtsc() - start;
        
        total_cycles += cycles;
        operations++;
        
        if (cycles < min_cycles) min_cycles = cycles;
        if (cycles > max_cycles) max_cycles = cycles;
    }
    
    printf("Stress test results:\n");
    printf("  Operations: %llu\n", operations);
    printf("  Avg cycles/op: %.1f\n", (double)total_cycles / operations);
    printf("  Min cycles: %llu\n", min_cycles);
    printf("  Max cycles: %llu\n", max_cycles);
    printf("  Ops/second: %.0f\n", (double)operations / duration_seconds);
}

// Real benchmark command implementation
int cmd_benchmark_real(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: cns benchmark <test> [iterations]\n");
        printf("Tests:\n");
        printf("  hash     - Hash function performance\n");
        printf("  memory   - Memory operations\n");
        printf("  parse    - Integer parsing\n");
        printf("  basic    - Basic operations\n");
        printf("  all      - All benchmarks\n");
        printf("  stress   - Stress test (10 seconds)\n");
        return 1;
    }
    
    int iterations = (argc >= 3) ? atoi(argv[2]) : 1000;
    
    printf("Real CNS Benchmark Suite\n");
    printf("========================\n");
    printf("CPU frequency assumed: 2.4 GHz\n");
    printf("7-tick limit: 7 cycles (2.9 ns)\n\n");
    
    uint64_t suite_start = rdtsc();
    
    if (strcmp(argv[1], "hash") == 0) {
        run_benchmark("Hash Function", bench_hash, iterations);
    } 
    else if (strcmp(argv[1], "memory") == 0) {
        run_benchmark("Memory Copy", bench_memcopy, iterations);
    }
    else if (strcmp(argv[1], "parse") == 0) {
        run_benchmark("Integer Parse", bench_parse_int, iterations);
    }
    else if (strcmp(argv[1], "basic") == 0) {
        run_benchmark("NOP", bench_nop, iterations);
        run_benchmark("Arithmetic", bench_arithmetic, iterations);
    }
    else if (strcmp(argv[1], "all") == 0) {
        run_benchmark("NOP", bench_nop, iterations);
        run_benchmark("Arithmetic", bench_arithmetic, iterations);
        run_benchmark("Hash Function", bench_hash, iterations);
        run_benchmark("Memory Copy", bench_memcopy, iterations);
        run_benchmark("Integer Parse", bench_parse_int, iterations);
    }
    else if (strcmp(argv[1], "stress") == 0) {
        int duration = (argc >= 3) ? atoi(argv[2]) : 10;
        stress_test(duration);
    }
    else {
        printf("Unknown benchmark: %s\n", argv[1]);
        return 1;
    }
    
    uint64_t suite_cycles = rdtsc() - suite_start;
    printf("Total benchmark time: %llu cycles (%.2f ms)\n", 
           suite_cycles, suite_cycles / 2400000.0);
    
    return 0;
}