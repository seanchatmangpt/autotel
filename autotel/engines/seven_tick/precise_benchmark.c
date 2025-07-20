/**
 * @file precise_benchmark.c
 * @brief High-precision benchmark for blazing-fast CJinja
 */

#include "cjinja_blazing_fast.h"
#include <mach/mach_time.h>  // macOS high-precision timing

static uint64_t get_mach_time_ns(void) {
    static mach_timebase_info_data_t timebase = {0};
    if (timebase.denom == 0) {
        mach_timebase_info(&timebase);
    }
    
    uint64_t mach_time = mach_absolute_time();
    return mach_time * timebase.numer / timebase.denom;
}

void precise_blazing_benchmark(void) {
    printf("🔥 HIGH-PRECISION BLAZING BENCHMARK\n");
    printf("===================================\n\n");
    
    CJinjaBlazingContext* ctx = cjinja_blazing_create_context();
    if (!ctx) {
        printf("❌ Failed to create context\n");
        return;
    }
    
    // Setup test variables
    cjinja_blazing_set_var(ctx, "n", "John");
    cjinja_blazing_set_var(ctx, "r", "Dev");
    
    const char* template = "Hi {{n}}, {{r}}!";
    
    printf("Template: '%s'\n", template);
    printf("Using mach_absolute_time() for nanosecond precision\n\n");
    
    // Warm up extensively
    printf("CPU warm-up (10,000 iterations)...\n");
    for (int i = 0; i < 10000; i++) {
        char* warm = cjinja_blazing_render(template, ctx);
        free(warm);
    }
    
    // Batch timing test - measure large batches for accuracy
    const int batch_sizes[] = {1000, 10000, 100000};
    const int num_batches = sizeof(batch_sizes) / sizeof(batch_sizes[0]);
    
    printf("\nBatch Timing Results:\n");
    printf("%-10s %-15s %-15s %-15s\n", "Batch Size", "Total Time (μs)", "Avg per Op (ns)", "Ops/sec");
    printf("%-10s %-15s %-15s %-15s\n", "----------", "---------------", "---------------", "---------------");
    
    for (int b = 0; b < num_batches; b++) {
        int batch_size = batch_sizes[b];
        
        uint64_t start = get_mach_time_ns();
        
        for (int i = 0; i < batch_size; i++) {
            char* result = cjinja_blazing_render(template, ctx);
            free(result);
        }
        
        uint64_t end = get_mach_time_ns();
        uint64_t total_ns = end - start;
        uint64_t avg_ns = total_ns / batch_size;
        double total_us = total_ns / 1000.0;
        double ops_per_sec = batch_size * 1000000000.0 / total_ns;
        
        printf("%-10d %-15.1f %-15llu %-15.0f\n", 
               batch_size, total_us, avg_ns, ops_per_sec);
    }
    
    // Multiple single measurements for statistical analysis
    printf("\nSingle Operation Analysis (1000 samples):\n");
    
    const int samples = 1000;
    uint64_t times[samples];
    
    for (int i = 0; i < samples; i++) {
        uint64_t start = get_mach_time_ns();
        char* result = cjinja_blazing_render(template, ctx);
        uint64_t end = get_mach_time_ns();
        
        times[i] = end - start;
        free(result);
    }
    
    // Sort times for percentile analysis
    for (int i = 0; i < samples - 1; i++) {
        for (int j = 0; j < samples - 1 - i; j++) {
            if (times[j] > times[j + 1]) {
                uint64_t temp = times[j];
                times[j] = times[j + 1];
                times[j + 1] = temp;
            }
        }
    }
    
    uint64_t min_time = times[0];
    uint64_t max_time = times[samples - 1];
    uint64_t p50 = times[samples / 2];
    uint64_t p90 = times[(samples * 9) / 10];
    uint64_t p99 = times[(samples * 99) / 100];
    
    // Calculate average
    uint64_t total = 0;
    for (int i = 0; i < samples; i++) {
        total += times[i];
    }
    uint64_t avg_time = total / samples;
    
    printf("  Samples:     %d operations\n", samples);
    printf("  Average:     %llu ns\n", avg_time);
    printf("  Minimum:     %llu ns\n", min_time);
    printf("  Median:      %llu ns\n", p50);
    printf("  90th perc:   %llu ns\n", p90);
    printf("  99th perc:   %llu ns\n", p99);
    printf("  Maximum:     %llu ns\n", max_time);
    
    // Count fast operations
    int sub_100ns = 0;
    int sub_150ns = 0;
    int sub_200ns = 0;
    
    for (int i = 0; i < samples; i++) {
        if (times[i] < 100) sub_100ns++;
        if (times[i] < 150) sub_150ns++;
        if (times[i] < 200) sub_200ns++;
    }
    
    printf("\nSpeed Distribution:\n");
    printf("  Sub-100ns:   %d/%d (%.1f%%)\n", sub_100ns, samples, 100.0 * sub_100ns / samples);
    printf("  Sub-150ns:   %d/%d (%.1f%%)\n", sub_150ns, samples, 100.0 * sub_150ns / samples);
    printf("  Sub-200ns:   %d/%d (%.1f%%)\n", sub_200ns, samples, 100.0 * sub_200ns / samples);
    
    printf("\n📊 PERFORMANCE VERDICT:\n");
    
    if (p50 < 100) {
        printf("🎯 EXCELLENT: Median time is sub-100ns (%llu ns)!\n", p50);
        printf("🚀 Blazing-Fast vs 206ns baseline: %.2fx faster\n", 206.0 / p50);
        printf("⚡ Blazing-Fast vs 272ns hash table: %.2fx faster\n", 272.0 / p50);
        printf("💎 Achievement: SUB-100NS TARGET MET\n");
    } else if (min_time < 100) {
        printf("🎯 GOOD: Minimum time achieves sub-100ns (%llu ns)\n", min_time);
        printf("📊 Median performance: %llu ns\n", p50);
        printf("🚀 Best case vs 206ns baseline: %.2fx faster\n", 206.0 / min_time);
        printf("💎 Achievement: SUB-100NS ACHIEVABLE\n");
    } else if (p50 < 150) {
        printf("⚡ FAST: Median under 150ns (%llu ns)\n", p50);
        printf("🚀 vs 206ns baseline: %.2fx faster\n", 206.0 / p50);
        printf("⚡ vs 272ns hash table: %.2fx faster\n", 272.0 / p50);
        printf("📈 Performance class: VERY FAST\n");
    } else {
        printf("📊 Current median performance: %llu ns\n", p50);
        printf("🎯 Target: Sub-100ns (need %.2fx improvement)\n", p50 / 100.0);
        printf("📈 vs 206ns baseline: %.2fx faster\n", 206.0 / p50);
    }
    
    printf("\n🔧 TECHNICAL DETAILS:\n");
    printf("  Timer: mach_absolute_time() (nanosecond precision)\n");
    printf("  Template: '%s' (%zu chars)\n", template, strlen(template));
    printf("  Variables: 2 short variables\n");
    printf("  Algorithm: Direct array lookup + stack allocation\n");
    printf("  Optimization: Warm CPU cache + minimal allocations\n");
    
    cjinja_blazing_destroy_context(ctx);
}

int main(void) {
    printf("🚀 PRECISE BLAZING-FAST BENCHMARK\n");
    printf("=================================\n\n");
    
    printf("Objective: Measure true sub-100ns performance with high precision\n");
    printf("Method: mach_absolute_time() + statistical analysis\n\n");
    
    precise_blazing_benchmark();
    
    printf("\n✅ PRECISE BENCHMARK COMPLETE\n");
    
    return 0;
}