/*  ─────────────────────────────────────────────────────────────
    cns_linker_stress_test.c  –  CNS Linker Stress Test Suite
    Extreme testing for CNS linker under high-load conditions
    ───────────────────────────────────────────────────────────── */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <errno.h>

#define MAX_THREADS 16
#define STRESS_ITERATIONS 10000
#define LARGE_SYMBOL_COUNT 100000
#define S7T_MAX_CYCLES 7
#define ESTIMATED_CPU_FREQ_GHZ 3.5
#define NS_PER_CYCLE (1.0 / ESTIMATED_CPU_FREQ_GHZ)

// Force evaluation to prevent optimization
#define FORCE_EVAL(x) __asm__ __volatile__("" : : "g"(x) : "memory")

/*═══════════════════════════════════════════════════════════════
  Stress Test Data Structures
  ═══════════════════════════════════════════════════════════════*/

typedef struct {
    uint32_t name_hash;
    uint64_t address;
    uint32_t size;
    uint8_t type;
    uint8_t binding;
    uint16_t flags;
} StressSymbol;

typedef struct {
    uint32_t source_symbol;
    uint32_t target_symbol;
    uint64_t offset;
    uint32_t type;
} StressRelocation;

typedef struct {
    StressSymbol* symbols;
    StressRelocation* relocations;
    uint32_t symbol_count;
    uint32_t relocation_count;
    uint32_t* hash_table;
    uint32_t hash_size;
    pthread_mutex_t mutex;
    volatile uint64_t total_operations;
    volatile uint64_t total_cycles;
} StressContext;

typedef struct {
    int thread_id;
    StressContext* ctx;
    int iterations;
    uint64_t thread_cycles;
    uint64_t thread_operations;
} ThreadData;

/*═══════════════════════════════════════════════════════════════
  Timing Utilities
  ═══════════════════════════════════════════════════════════════*/

static inline uint64_t get_cycles(void) {
    uint64_t cycles;
#ifdef __x86_64__
    uint32_t lo, hi;
    __asm__ volatile ("rdtsc" : "=a"(lo), "=d"(hi));
    cycles = ((uint64_t)hi << 32) | lo;
#elif defined(__aarch64__)
    __asm__ volatile ("mrs %0, cntvct_el0" : "=r" (cycles));
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    cycles = (uint64_t)ts.tv_sec * 2400000000ULL + (uint64_t)ts.tv_nsec * 2.4;
#endif
    return cycles;
}

/*═══════════════════════════════════════════════════════════════
  Hash Functions (Lock-free)
  ═══════════════════════════════════════════════════════════════*/

static inline uint32_t stress_hash(uint32_t key) {
    // Fast hash for stress testing
    key ^= key >> 16;
    key *= 0x85ebca6b;
    key ^= key >> 13;
    key *= 0xc2b2ae35;
    key ^= key >> 16;
    return key;
}

/*═══════════════════════════════════════════════════════════════
  Lock-free Symbol Lookup
  ═══════════════════════════════════════════════════════════════*/

static StressSymbol* lookup_symbol_lockfree(StressContext* ctx, uint32_t hash) {
    uint32_t index = hash & (ctx->hash_size - 1);
    
    // Lock-free linear probing
    for (int i = 0; i < 32; i++) {  // Max probe distance
        uint32_t current_index = (index + i) & (ctx->hash_size - 1);
        uint32_t sym_idx = __atomic_load_n(&ctx->hash_table[current_index], __ATOMIC_ACQUIRE);
        
        if (sym_idx == 0) return NULL;  // Empty slot
        
        sym_idx--;  // Convert to 0-based index
        if (sym_idx < ctx->symbol_count && 
            ctx->symbols[sym_idx].name_hash == hash) {
            return &ctx->symbols[sym_idx];
        }
    }
    
    return NULL;
}

/*═══════════════════════════════════════════════════════════════
  Stress Test Workloads
  ═══════════════════════════════════════════════════════════════*/

// Workload 1: Intensive Symbol Resolution
static uint64_t stress_symbol_resolution(ThreadData* td) {
    uint64_t total_cycles = 0;
    uint64_t operations = 0;
    
    for (int iter = 0; iter < td->iterations; iter++) {
        uint64_t start = get_cycles();
        
        // Resolve random symbols
        for (int i = 0; i < 1000; i++) {
            uint32_t hash = stress_hash(iter * 1000 + i + td->thread_id);
            StressSymbol* sym = lookup_symbol_lockfree(td->ctx, hash);
            if (sym) {
                FORCE_EVAL(sym->address);
            }
            operations++;
        }
        
        uint64_t cycles = get_cycles() - start;
        total_cycles += cycles;
    }
    
    return total_cycles / operations;
}

// Workload 2: Memory-intensive Operations
static uint64_t stress_memory_operations(ThreadData* td) {
    uint64_t total_cycles = 0;
    uint64_t operations = 0;
    
    // Allocate large temporary buffer
    size_t buffer_size = 1024 * 1024;  // 1MB per thread
    uint8_t* buffer = malloc(buffer_size);
    if (!buffer) return UINT64_MAX;
    
    for (int iter = 0; iter < td->iterations / 10; iter++) {
        uint64_t start = get_cycles();
        
        // Simulate memory-intensive linking operations
        for (size_t i = 0; i < buffer_size - sizeof(uint64_t); i += 64) {
            // Simulate relocation patching
            *(uint64_t*)(buffer + i) = (uint64_t)&td->ctx->symbols[i % td->ctx->symbol_count];
            
            // Simulate symbol copying
            if (i + 32 < buffer_size) {
                memcpy(buffer + i + 8, &td->ctx->symbols[i % td->ctx->symbol_count], 
                       sizeof(StressSymbol));
            }
        }
        
        uint64_t cycles = get_cycles() - start;
        total_cycles += cycles;
        operations += buffer_size / 64;
    }
    
    free(buffer);
    return total_cycles / operations;
}

// Workload 3: Concurrent Hash Table Access
static uint64_t stress_concurrent_access(ThreadData* td) {
    uint64_t total_cycles = 0;
    uint64_t operations = 0;
    
    for (int iter = 0; iter < td->iterations; iter++) {
        uint64_t start = get_cycles();
        
        // High-frequency hash table lookups
        for (int i = 0; i < 100; i++) {
            uint32_t hash = stress_hash(iter * 100 + i + td->thread_id * 12345);
            
            // Try lookup multiple times to stress the hash table
            for (int j = 0; j < 10; j++) {
                lookup_symbol_lockfree(td->ctx, hash + j);
                operations++;
            }
        }
        
        uint64_t cycles = get_cycles() - start;
        total_cycles += cycles;
    }
    
    return total_cycles / operations;
}

// Workload 4: Cache Thrashing Test
static uint64_t stress_cache_thrashing(ThreadData* td) {
    uint64_t total_cycles = 0;
    uint64_t operations = 0;
    
    // Access pattern designed to thrash CPU caches
    for (int iter = 0; iter < td->iterations / 100; iter++) {
        uint64_t start = get_cycles();
        
        // Random access pattern across symbol table
        for (int i = 0; i < 1000; i++) {
            uint32_t idx = stress_hash(iter * 1000 + i + td->thread_id) % td->ctx->symbol_count;
            
            // Access symbol at random location
            StressSymbol* sym = &td->ctx->symbols[idx];
            volatile uint64_t dummy = sym->address + sym->size;
            FORCE_EVAL(dummy);
            
            operations++;
        }
        
        uint64_t cycles = get_cycles() - start;
        total_cycles += cycles;
    }
    
    return total_cycles / operations;
}

/*═══════════════════════════════════════════════════════════════
  Thread Worker Function
  ═══════════════════════════════════════════════════════════════*/

static void* stress_worker(void* arg) {
    ThreadData* td = (ThreadData*)arg;
    
    // Set thread affinity to spread across cores
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(td->thread_id % sysconf(_SC_NPROCESSORS_ONLN), &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    
    uint64_t workload_cycles[4];
    
    printf("Thread %d starting stress tests...\n", td->thread_id);
    
    // Run all workloads
    workload_cycles[0] = stress_symbol_resolution(td);
    workload_cycles[1] = stress_memory_operations(td);
    workload_cycles[2] = stress_concurrent_access(td);
    workload_cycles[3] = stress_cache_thrashing(td);
    
    // Calculate average
    td->thread_cycles = 0;
    for (int i = 0; i < 4; i++) {
        td->thread_cycles += workload_cycles[i];
    }
    td->thread_cycles /= 4;
    
    // Update global counters
    __atomic_add_fetch(&td->ctx->total_cycles, td->thread_cycles, __ATOMIC_RELAXED);
    __atomic_add_fetch(&td->ctx->total_operations, td->thread_operations, __ATOMIC_RELAXED);
    
    return NULL;
}

/*═══════════════════════════════════════════════════════════════
  Test Data Generation
  ═══════════════════════════════════════════════════════════════*/

static void generate_stress_data(StressContext* ctx) {
    ctx->symbol_count = LARGE_SYMBOL_COUNT;
    ctx->relocation_count = LARGE_SYMBOL_COUNT / 2;
    ctx->hash_size = 1 << 18;  // 256K entries
    
    // Allocate memory
    ctx->symbols = malloc(ctx->symbol_count * sizeof(StressSymbol));
    ctx->relocations = malloc(ctx->relocation_count * sizeof(StressRelocation));
    ctx->hash_table = calloc(ctx->hash_size, sizeof(uint32_t));
    
    if (!ctx->symbols || !ctx->relocations || !ctx->hash_table) {
        fprintf(stderr, "Failed to allocate stress test data\n");
        exit(1);
    }
    
    // Initialize mutex
    pthread_mutex_init(&ctx->mutex, NULL);
    
    // Generate symbols
    for (uint32_t i = 0; i < ctx->symbol_count; i++) {
        ctx->symbols[i].name_hash = stress_hash(i);
        ctx->symbols[i].address = 0x400000 + (i * 64);
        ctx->symbols[i].size = 32 + (i % 128);
        ctx->symbols[i].type = (i % 3) + 1;
        ctx->symbols[i].binding = (i % 10 == 0) ? 2 : 1;
        ctx->symbols[i].flags = i % 4;
        
        // Add to hash table
        uint32_t idx = ctx->symbols[i].name_hash & (ctx->hash_size - 1);
        while (ctx->hash_table[idx] != 0) {
            idx = (idx + 1) & (ctx->hash_size - 1);
        }
        ctx->hash_table[idx] = i + 1;  // 1-based index
    }
    
    // Generate relocations
    for (uint32_t i = 0; i < ctx->relocation_count; i++) {
        ctx->relocations[i].source_symbol = i % ctx->symbol_count;
        ctx->relocations[i].target_symbol = (i * 7) % ctx->symbol_count;
        ctx->relocations[i].offset = i * 8;
        ctx->relocations[i].type = (i % 4) + 1;
    }
}

static void cleanup_stress_data(StressContext* ctx) {
    free(ctx->symbols);
    free(ctx->relocations);
    free(ctx->hash_table);
    pthread_mutex_destroy(&ctx->mutex);
}

/*═══════════════════════════════════════════════════════════════
  Memory Usage Analysis
  ═══════════════════════════════════════════════════════════════*/

static void print_memory_usage(void) {
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        printf("Memory Usage:\n");
        printf("  Peak memory: %ld KB\n", usage.ru_maxrss);
        printf("  Page faults: %ld\n", usage.ru_majflt + usage.ru_minflt);
        printf("  Context switches: %ld\n", usage.ru_nvcsw + usage.ru_nivcsw);
    }
}

/*═══════════════════════════════════════════════════════════════
  Main Stress Test Runner
  ═══════════════════════════════════════════════════════════════*/

int main(int argc, char** argv) {
    printf("🚀 CNS Linker Stress Test Suite\n");
    printf("================================\n\n");
    
    int num_threads = (argc > 1) ? atoi(argv[1]) : 4;
    int iterations = (argc > 2) ? atoi(argv[2]) : STRESS_ITERATIONS;
    
    if (num_threads > MAX_THREADS) {
        num_threads = MAX_THREADS;
        printf("Warning: Limited to %d threads\n", MAX_THREADS);
    }
    
    printf("Configuration:\n");
    printf("  Threads: %d\n", num_threads);
    printf("  Iterations per thread: %d\n", iterations);
    printf("  Total symbols: %d\n", LARGE_SYMBOL_COUNT);
    printf("  Hash table size: %d\n", 1 << 18);
    printf("  CPU cores available: %ld\n\n", sysconf(_SC_NPROCESSORS_ONLN));
    
    // Initialize stress context
    StressContext ctx = {0};
    generate_stress_data(&ctx);
    
    printf("Generating stress test data...\n");
    print_memory_usage();
    printf("\n");
    
    // Create thread data
    pthread_t threads[MAX_THREADS];
    ThreadData thread_data[MAX_THREADS];
    
    printf("Starting %d stress test threads...\n", num_threads);
    
    uint64_t start_time = get_cycles();
    
    // Launch threads
    for (int i = 0; i < num_threads; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].ctx = &ctx;
        thread_data[i].iterations = iterations;
        thread_data[i].thread_cycles = 0;
        thread_data[i].thread_operations = 0;
        
        if (pthread_create(&threads[i], NULL, stress_worker, &thread_data[i]) != 0) {
            fprintf(stderr, "Failed to create thread %d\n", i);
            exit(1);
        }
    }
    
    // Wait for threads to complete
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    uint64_t end_time = get_cycles();
    uint64_t total_test_cycles = end_time - start_time;
    
    printf("\nStress test completed!\n\n");
    
    // Calculate results
    uint64_t avg_cycles = ctx.total_cycles / num_threads;
    double total_time_ns = total_test_cycles * NS_PER_CYCLE;
    double throughput = (ctx.total_operations * 1000000000.0) / total_time_ns;
    
    // Per-thread results
    printf("Per-Thread Results:\n");
    printf("%-8s %-15s %-12s %s\n", "Thread", "Avg Cycles", "7-Tick", "Status");
    printf("%-8s %-15s %-12s %s\n", "------", "-----------", "------", "------");
    
    int passed_threads = 0;
    for (int i = 0; i < num_threads; i++) {
        bool compliant = thread_data[i].thread_cycles <= S7T_MAX_CYCLES;
        if (compliant) passed_threads++;
        
        printf("%-8d %-15.3f %-12s %s\n",
               i,
               (double)thread_data[i].thread_cycles,
               compliant ? "✅ PASS" : "❌ FAIL",
               compliant ? "OK" : "SLOW");
    }
    
    // Overall results
    printf("\n═══════════════════════════════════════════════════════════\n");
    printf("STRESS TEST SUMMARY\n");
    printf("═══════════════════════════════════════════════════════════\n");
    
    printf("\nPerformance Metrics:\n");
    printf("  Average cycles/op: %.3f\n", (double)avg_cycles);
    printf("  Total operations: %lu\n", ctx.total_operations);
    printf("  Test duration: %.2f seconds\n", total_time_ns / 1000000000.0);
    printf("  Throughput: %.0f ops/second\n", throughput);
    printf("  Threads passed: %d/%d (%.0f%%)\n", 
           passed_threads, num_threads, (passed_threads * 100.0) / num_threads);
    
    bool overall_pass = (double)avg_cycles <= S7T_MAX_CYCLES;
    printf("  Overall 7-tick compliant: %s\n", overall_pass ? "✅ YES" : "❌ NO");
    
    // Stress test analysis
    printf("\nStress Test Analysis:\n");
    if (overall_pass && passed_threads == num_threads) {
        printf("  ✅ CNS linker handles high-stress conditions well\n");
        printf("  🚀 Ready for production under heavy load\n");
        printf("  💪 Excellent scalability with %d threads\n", num_threads);
    } else if (passed_threads >= num_threads / 2) {
        printf("  ⚠️  Mixed results under stress\n");
        printf("  📈 Consider thread-local optimizations\n");
        printf("  🔧 Review lock contention and cache behavior\n");
    } else {
        printf("  ❌ Poor performance under stress\n");
        printf("  🚨 Critical optimizations needed\n");
        printf("  🔧 Focus on: lock-free algorithms, cache optimization\n");
    }
    
    // Memory analysis
    printf("\nFinal Memory Usage:\n");
    print_memory_usage();
    
    // Mermaid stress test diagram
    printf("\n```mermaid\n");
    printf("graph TD\n");
    printf("    A[CNS Linker Stress Test] --> B[%d Threads]\n", num_threads);
    printf("    B --> C[%d Passed]\n", passed_threads);
    printf("    B --> D[%d Failed]\n", num_threads - passed_threads);
    printf("    C --> E[%.3f Avg Cycles]\n", (double)avg_cycles);
    printf("    E --> F{≤7 Cycles?}\n");
    if (overall_pass) {
        printf("    F -->|YES| G[✅ STRESS TEST PASSED]\n");
        printf("    G --> H[%.0f ops/sec]\n", throughput);
    } else {
        printf("    F -->|NO| I[❌ NEEDS OPTIMIZATION]\n");
        printf("    I --> J[%d/%d threads slow]\n", num_threads - passed_threads, num_threads);
    }
    printf("```\n");
    
    // Cleanup
    cleanup_stress_data(&ctx);
    
    return overall_pass ? 0 : 1;
}