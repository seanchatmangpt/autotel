/*
 * Simple 8T test without complex SIMD dependencies
 */

#include <stdio.h>
#include <stdint.h>
#include <time.h>

// Simplified types for testing
typedef uint64_t cns_tick_t;

static inline cns_tick_t cns_get_tick_count(void) {
#ifdef __x86_64__
    uint32_t lo, hi;
    __asm__ __volatile__("rdtsc" : "=a" (lo), "=d" (hi));
    return ((uint64_t)hi << 32) | lo;
#elif defined(__aarch64__)
    uint64_t val;
    __asm__ volatile("mrs %0, cntvct_el0" : "=r" (val));
    return val;
#else
    return (uint64_t)clock();
#endif
}

int main() {
    printf("8T Simple Test\n");
    printf("==============\n");
    
    cns_tick_t start = cns_get_tick_count();
    
    // Simple 8-tick test operation
    volatile int sum = 0;
    for (int i = 0; i < 8; i++) {
        sum += i;
    }
    
    cns_tick_t end = cns_get_tick_count();
    cns_tick_t elapsed = end - start;
    
    printf("Operation took %llu ticks\n", elapsed);
    printf("Sum result: %d\n", sum);
    
    if (elapsed <= 8) {
        printf("✅ 8-TICK CONSTRAINT SATISFIED!\n");
    } else {
        printf("❌ 8-tick constraint violated (%llu ticks)\n", elapsed);
    }
    
    // Test basic ARM64/x86 detection
#ifdef __x86_64__
    printf("Platform: x86_64\n");
#elif defined(__aarch64__)
    printf("Platform: ARM64\n");
#else
    printf("Platform: Unknown\n");
#endif
    
    return 0;
}