// Cycle measurement validation - check if our measurements are accurate
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

// Raw cycle measurement
static inline uint64_t get_cycles(void) {
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

// Test the overhead of cycle measurement itself
void test_measurement_overhead(void) {
    printf("Testing cycle measurement overhead...\n");
    
    // Measure back-to-back rdtsc calls
    uint64_t measurements[100];
    for (int i = 0; i < 100; i++) {
        uint64_t start = get_cycles();
        uint64_t end = get_cycles();
        measurements[i] = end - start;
    }
    
    // Analyze overhead
    uint64_t min_overhead = UINT64_MAX, max_overhead = 0, total = 0;
    for (int i = 0; i < 100; i++) {
        if (measurements[i] < min_overhead) min_overhead = measurements[i];
        if (measurements[i] > max_overhead) max_overhead = measurements[i];
        total += measurements[i];
        
        if (i < 10) {
            printf("  Measurement %d: %llu cycles\n", i, measurements[i]);
        }
    }
    
    printf("Measurement overhead analysis:\n");
    printf("  Min overhead: %llu cycles\n", min_overhead);
    printf("  Max overhead: %llu cycles\n", max_overhead);
    printf("  Avg overhead: %.2f cycles\n", (double)total / 100);
    printf("  Note: Any operation showing less than this overhead is likely optimized away\n\n");
}

// Test actual simple operations
volatile int g_volatile_int = 42;
volatile char g_volatile_buffer[64] = "test_string_for_operations";

void test_real_operations(void) {
    printf("Testing real operations with multiple methods...\n");
    
    // Test 1: Simple addition with volatile variables
    printf("\n1. Volatile addition test:\n");
    for (int i = 0; i < 10; i++) {
        uint64_t start = get_cycles();
        g_volatile_int += 1;
        uint64_t cycles = get_cycles() - start;
        printf("  Addition %d: %llu cycles\n", i, cycles);
    }
    
    // Test 2: Memory access
    printf("\n2. Memory access test:\n");
    for (int i = 0; i < 10; i++) {
        uint64_t start = get_cycles();
        volatile char c = g_volatile_buffer[i % 26];
        uint64_t cycles = get_cycles() - start;
        printf("  Memory read %d: %llu cycles (value: %c)\n", i, cycles, c);
        (void)c; // Suppress warning
    }
    
    // Test 3: String length calculation
    printf("\n3. String length test:\n");
    for (int i = 0; i < 10; i++) {
        uint64_t start = get_cycles();
        volatile int len = 0;
        for (volatile char* p = g_volatile_buffer; *p; p++) len++;
        uint64_t cycles = get_cycles() - start;
        printf("  String length %d: %llu cycles (len: %d)\n", i, cycles, len);
    }
    
    // Test 4: Hash calculation (simple)
    printf("\n4. Hash calculation test:\n");
    for (int i = 0; i < 10; i++) {
        uint64_t start = get_cycles();
        volatile uint32_t hash = 5381;
        for (int j = 0; j < 5; j++) {
            hash = ((hash << 5) + hash) + g_volatile_buffer[j];
        }
        uint64_t cycles = get_cycles() - start;
        printf("  Hash %d: %llu cycles (hash: %u)\n", i, cycles, hash);
    }
}

// Test timer resolution and frequency
void test_timer_properties(void) {
    printf("Testing timer properties...\n");
    
    // Test resolution by measuring known delays
    struct timespec delay_1ms = {0, 1000000}; // 1ms
    
    printf("\nTesting 1ms delay measurement:\n");
    for (int i = 0; i < 5; i++) {
        uint64_t start_cycles = get_cycles();
        struct timespec start_time;
        clock_gettime(CLOCK_MONOTONIC, &start_time);
        
        nanosleep(&delay_1ms, NULL);
        
        struct timespec end_time;
        clock_gettime(CLOCK_MONOTONIC, &end_time);
        uint64_t end_cycles = get_cycles();
        
        uint64_t elapsed_cycles = end_cycles - start_cycles;
        uint64_t elapsed_ns = (end_time.tv_sec - start_time.tv_sec) * 1000000000ULL + 
                             (end_time.tv_nsec - start_time.tv_nsec);
        
        double freq_ghz = (double)elapsed_cycles / elapsed_ns;
        
        printf("  Test %d: %llu cycles in %llu ns = %.3f GHz\n", 
               i, elapsed_cycles, elapsed_ns, freq_ghz);
    }
}

// Test for compiler optimization issues
void test_optimization_effects(void) {
    printf("\nTesting compiler optimization effects...\n");
    
    // This should be optimized away
    printf("Test 1 - Should be optimized away:\n");
    for (int i = 0; i < 5; i++) {
        uint64_t start = get_cycles();
        int dummy = 42 + 37; // Compiler should optimize this
        uint64_t cycles = get_cycles() - start;
        printf("  Optimized operation %d: %llu cycles (result: %d)\n", i, cycles, dummy);
    }
    
    // This should NOT be optimized away
    printf("\nTest 2 - Should NOT be optimized away:\n");
    for (int i = 0; i < 5; i++) {
        uint64_t start = get_cycles();
        g_volatile_int = g_volatile_int + 37; // Volatile prevents optimization
        uint64_t cycles = get_cycles() - start;
        printf("  Volatile operation %d: %llu cycles (result: %d)\n", i, cycles, g_volatile_int);
    }
}

int main(void) {
    printf("Cycle Measurement Validation Tool\n");
    printf("=================================\n\n");
    
    test_measurement_overhead();
    test_timer_properties();
    test_real_operations();
    test_optimization_effects();
    
    printf("\nConclusion:\n");
    printf("- Any measurements smaller than the overhead are suspect\n");
    printf("- Operations showing 0 cycles are likely optimized away\n");
    printf("- Real operations should show consistent, measurable cycle counts\n");
    printf("- Frequency calculation helps validate timer accuracy\n");
    
    return 0;
}