#include "cns/8t/8t.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

// ============================================================================
// 8T COMPREHENSIVE BENCHMARK SUITE
// ============================================================================

// Timing utilities
static inline uint64_t get_cpu_cycles(void) {
#if defined(__x86_64__) || defined(__i386__)
    uint32_t hi, lo;
    __asm__ volatile ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)lo) | (((uint64_t)hi) << 32);
#elif defined(__aarch64__)
    uint64_t tsc;
    __asm__ volatile ("mrs %0, cntvct_el0" : "=r" (tsc));
    return tsc;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
#endif
}

// Benchmark result structure
typedef struct {
    const char* test_name;
    uint64_t cycles_per_operation;
    double operations_per_second;
    double l1_cache_efficiency;
    bool within_8t_limit;
    double error_bound;
} benchmark_result_t;

// ============================================================================
// ARENA ALLOCATOR BENCHMARKS
// ============================================================================

benchmark_result_t bench_arena_allocation(size_t num_allocations) {
    benchmark_result_t result = {0};
    result.test_name = "Arena L1 Allocation";
    
    // Allocate test arena
    size_t arena_size = 1024 * 1024; // 1MB
    void* memory = aligned_alloc(CNS_8T_ALIGNMENT, arena_size);
    if (!memory) {
        result.operations_per_second = 0;
        return result;
    }
    
    cns_8t_arena_t arena;
    cns_8t_arena_init(&arena, memory, arena_size);
    
    uint64_t start_cycles = get_cpu_cycles();
    
    // Perform allocations
    for (size_t i = 0; i < num_allocations; i++) {
        size_t alloc_size = 32 + (i % 64); // Variable size 32-96 bytes
        void* ptr = cns_8t_arena_alloc(&arena, alloc_size, 8);
        if (!ptr) break; // Arena full
        
        // Touch the memory to ensure it's actually allocated
        *((volatile char*)ptr) = (char)i;
    }
    
    uint64_t end_cycles = get_cpu_cycles();
    uint64_t total_cycles = end_cycles - start_cycles;
    
    result.cycles_per_operation = total_cycles / num_allocations;
    result.operations_per_second = (double)num_allocations * 3000000000.0 / total_cycles; // Assume 3GHz
    result.within_8t_limit = result.cycles_per_operation <= CNS_8T_TICK_LIMIT;
    result.l1_cache_efficiency = cns_8t_arena_get_l1_efficiency(&arena);
    
    free(memory);
    return result;
}

// ============================================================================
// NUMERICAL OPERATIONS BENCHMARKS
// ============================================================================

benchmark_result_t bench_numerical_operations(size_t num_operations) {
    benchmark_result_t result = {0};
    result.test_name = "Numerical Operations";
    
    // Create test data
    cns_8t_numerical_t a = cns_8t_numerical_create(3.14159);
    cns_8t_numerical_t b = cns_8t_numerical_create(2.71828);
    
    uint64_t start_cycles = get_cpu_cycles();
    
    cns_8t_numerical_t sum = a;
    for (size_t i = 0; i < num_operations; i++) {
        sum = cns_8t_numerical_add(sum, b);
        sum = cns_8t_numerical_mul(sum, cns_8t_numerical_create(0.999));
    }
    
    uint64_t end_cycles = get_cpu_cycles();
    uint64_t total_cycles = end_cycles - start_cycles;
    
    result.cycles_per_operation = total_cycles / (num_operations * 2); // Add + Mul
    result.operations_per_second = (double)(num_operations * 2) * 3000000000.0 / total_cycles;
    result.within_8t_limit = result.cycles_per_operation <= CNS_8T_TICK_LIMIT;
    result.error_bound = sum.error_bound;
    
    return result;
}

// ============================================================================
// SIMD OPERATIONS BENCHMARKS
// ============================================================================

benchmark_result_t bench_simd_operations(size_t array_size) {
    benchmark_result_t result = {0};
    result.test_name = "SIMD Numerical Operations";
    
    // Allocate test arena
    size_t arena_size = 8 * 1024 * 1024; // 8MB for large arrays
    void* memory = aligned_alloc(CNS_8T_ALIGNMENT, arena_size);
    if (!memory) {
        result.operations_per_second = 0;
        return result;
    }
    
    cns_8t_arena_t arena;
    cns_8t_arena_init(&arena, memory, arena_size);
    
    // Create test arrays
    cns_8t_numerical_array_t array_a, array_b, array_result;
    cns_8t_numerical_array_init(&array_a, &arena, array_size);
    cns_8t_numerical_array_init(&array_b, &arena, array_size);
    cns_8t_numerical_array_init(&array_result, &arena, array_size);
    
    // Fill arrays with test data
    for (size_t i = 0; i < array_size; i++) {
        cns_8t_numerical_array_push(&array_a, cns_8t_numerical_create(1.0 + i * 0.001));
        cns_8t_numerical_array_push(&array_b, cns_8t_numerical_create(2.0 + i * 0.002));
    }
    
    uint64_t start_cycles = get_cpu_cycles();
    
    // Perform SIMD operations
    cns_8t_numerical_add_simd(&array_result, &array_a, &array_b);
    
    uint64_t end_cycles = get_cpu_cycles();
    uint64_t total_cycles = end_cycles - start_cycles;
    
    result.cycles_per_operation = total_cycles / array_size;
    result.operations_per_second = (double)array_size * 3000000000.0 / total_cycles;
    result.within_8t_limit = result.cycles_per_operation <= CNS_8T_TICK_LIMIT;
    result.error_bound = array_result.global_error_bound;
    
    free(memory);
    return result;
}

// ============================================================================
// GRAPH OPERATIONS BENCHMARKS
// ============================================================================

benchmark_result_t bench_graph_operations(size_t num_triples) {
    benchmark_result_t result = {0};
    result.test_name = "Graph L1 Operations";
    
    // Allocate test arena
    size_t arena_size = 4 * 1024 * 1024; // 4MB
    void* memory = aligned_alloc(CNS_8T_ALIGNMENT, arena_size);
    if (!memory) {
        result.operations_per_second = 0;
        return result;
    }
    
    cns_8t_arena_t arena;
    cns_8t_arena_init(&arena, memory, arena_size);
    
    cns_8t_graph_t graph;
    cns_8t_graph_init(&graph, &arena, num_triples);
    
    uint64_t start_cycles = get_cpu_cycles();
    
    // Add triples to graph
    for (size_t i = 0; i < num_triples; i++) {
        cns_8t_triple_t triple = {
            .subject = (uint32_t)(i % 1000),
            .predicate = (uint32_t)(i % 100),
            .object = (uint32_t)i,
            .metadata = 0
        };
        cns_8t_graph_add_triple(&graph, triple);
    }
    
    uint64_t end_cycles = get_cpu_cycles();
    uint64_t total_cycles = end_cycles - start_cycles;
    
    result.cycles_per_operation = total_cycles / num_triples;
    result.operations_per_second = (double)num_triples * 3000000000.0 / total_cycles;
    result.within_8t_limit = result.cycles_per_operation <= CNS_8T_TICK_LIMIT;
    result.l1_cache_efficiency = cns_8t_graph_cache_efficiency(&graph);
    
    free(memory);
    return result;
}

// ============================================================================
// PATTERN MATCHING BENCHMARKS
// ============================================================================

benchmark_result_t bench_pattern_matching(size_t num_triples, size_t num_queries) {
    benchmark_result_t result = {0};
    result.test_name = "SIMD Pattern Matching";
    
    // Setup graph
    size_t arena_size = 4 * 1024 * 1024;
    void* memory = aligned_alloc(CNS_8T_ALIGNMENT, arena_size);
    if (!memory) {
        result.operations_per_second = 0;
        return result;
    }
    
    cns_8t_arena_t arena;
    cns_8t_arena_init(&arena, memory, arena_size);
    
    cns_8t_graph_t graph;
    cns_8t_graph_init(&graph, &arena, num_triples);
    
    // Add test triples
    for (size_t i = 0; i < num_triples; i++) {
        cns_8t_triple_t triple = {
            .subject = (uint32_t)(i % 1000),
            .predicate = (uint32_t)(i % 50),
            .object = (uint32_t)(i % 2000),
            .metadata = 0
        };
        cns_8t_graph_add_triple(&graph, triple);
    }
    
    // Prepare query results storage
    uint32_t* results = (uint32_t*)cns_8t_arena_alloc(&arena, sizeof(uint32_t) * num_triples, 4);
    
    uint64_t start_cycles = get_cpu_cycles();
    
    // Perform pattern matching queries
    for (size_t i = 0; i < num_queries; i++) {
        cns_8t_triple_t pattern = {
            .subject = (uint32_t)(i % 1000),
            .predicate = 0xFFFFFFFF, // Wildcard
            .object = 0xFFFFFFFF,    // Wildcard
            .metadata = 0
        };
        
        size_t result_count;
        cns_8t_graph_find_pattern_simd(&graph, pattern, results, &result_count);
    }
    
    uint64_t end_cycles = get_cpu_cycles();
    uint64_t total_cycles = end_cycles - start_cycles;
    
    result.cycles_per_operation = total_cycles / num_queries;
    result.operations_per_second = (double)num_queries * 3000000000.0 / total_cycles;
    result.within_8t_limit = result.cycles_per_operation <= CNS_8T_TICK_LIMIT;
    result.l1_cache_efficiency = cns_8t_graph_cache_efficiency(&graph);
    
    free(memory);
    return result;
}

// ============================================================================
// MAIN BENCHMARK RUNNER
// ============================================================================

void print_benchmark_header(void) {
    printf("╔══════════════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║                            8T PROTOTYPE BENCHMARK RESULTS                           ║\n");
    printf("╠══════════════════════════════════════════════════════════════════════════════════════╣\n");
    printf("║ Test Name                │ Cycles/Op │ Ops/Sec   │ 8T✓ │ L1 Eff │ Error Bound    ║\n");
    printf("╠══════════════════════════════════════════════════════════════════════════════════════╣\n");
}

void print_benchmark_result(const benchmark_result_t* result) {
    const char* tick_status = result->within_8t_limit ? "✓" : "✗";
    
    printf("║ %-24s │ %9lu │ %9.0f │  %s  │ %5.1f%% │ %13.2e ║\n",
           result->test_name,
           result->cycles_per_operation,
           result->operations_per_second,
           tick_status,
           result->l1_cache_efficiency * 100.0,
           result->error_bound);
}

void print_benchmark_footer(void) {
    printf("╚══════════════════════════════════════════════════════════════════════════════════════╝\n");
    printf("\nLegend:\n");
    printf("  Cycles/Op: CPU cycles per operation (target: ≤ 8)\n");
    printf("  Ops/Sec: Operations per second (higher is better)\n");
    printf("  8T✓: Within 8-tick limit (✓=yes, ✗=no)\n");
    printf("  L1 Eff: L1 cache efficiency percentage\n");
    printf("  Error Bound: Maximum numerical error bound\n");
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    
    printf("8T Prototype Benchmark Suite - L1-Optimized Numerical Substrate\n");
    printf("================================================================\n\n");
    
    // Detect SIMD capabilities
    cns_8t_simd_caps_t simd_caps = cns_8t_detect_simd_capabilities();
    printf("SIMD Capabilities: SSE2=%s AVX=%s AVX2=%s FMA=%s AVX512F=%s\n\n",
           simd_caps.sse2 ? "✓" : "✗",
           simd_caps.avx ? "✓" : "✗",
           simd_caps.avx2 ? "✓" : "✗",
           simd_caps.fma ? "✓" : "✗",
           simd_caps.avx512f ? "✓" : "✗");
    
    print_benchmark_header();
    
    // Run benchmarks
    benchmark_result_t result;
    
    result = bench_arena_allocation(10000);
    print_benchmark_result(&result);
    
    result = bench_numerical_operations(10000);
    print_benchmark_result(&result);
    
    result = bench_simd_operations(1000);
    print_benchmark_result(&result);
    
    result = bench_graph_operations(5000);
    print_benchmark_result(&result);
    
    result = bench_pattern_matching(5000, 1000);
    print_benchmark_result(&result);
    
    print_benchmark_footer();
    
    return 0;
}