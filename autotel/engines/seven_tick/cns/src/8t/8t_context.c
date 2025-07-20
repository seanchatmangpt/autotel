#include "cns/8t/8t.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// ============================================================================
// 8T CONTEXT IMPLEMENTATION
// ============================================================================

cns_result_t cns_8t_context_init(cns_8t_context_t* ctx, size_t arena_size) {
    assert(ctx != NULL);
    assert(arena_size > 0);
    
    memset(ctx, 0, sizeof(*ctx));
    
    // Allocate main arena
    void* arena_memory = aligned_alloc(CNS_8T_ALIGNMENT, arena_size);
    if (!arena_memory) {
        return CNS_ERROR_MEMORY;
    }
    
    ctx->arena = (cns_8t_arena_t*)arena_memory;
    cns_8t_arena_init(ctx->arena, 
                      (char*)arena_memory + sizeof(cns_8t_arena_t), 
                      arena_size - sizeof(cns_8t_arena_t));
    
    // Initialize graph
    ctx->graph = (cns_8t_graph_t*)cns_8t_arena_alloc(ctx->arena, sizeof(cns_8t_graph_t), CNS_8T_ALIGNMENT);
    if (!ctx->graph) {
        free(arena_memory);
        return CNS_ERROR_MEMORY;
    }
    
    cns_8t_graph_init(ctx->graph, ctx->arena, 1000); // Initial capacity
    
    // Initialize metrics
    cns_8t_metrics_init(&ctx->metrics);
    
    // Detect SIMD capabilities
    cns_8t_simd_caps_t caps = cns_8t_detect_simd_capabilities();
    ctx->avx2_available = caps.avx2;
    ctx->fma_available = caps.fma;
    ctx->avx512_available = caps.avx512f;
    
    // Initialize numerical context
    ctx->global_error_bound = 0.0;
    ctx->precision_mode = 0; // Standard precision
    
    // Initialize L1 cache tracking
    ctx->l1_lru_counter = 0;
    memset(ctx->l1_cache_map, 0, sizeof(ctx->l1_cache_map));
    
    // Configuration
    ctx->flags = 0;
    ctx->strict_8t = true;
    ctx->enable_prefetch = true;
    
    return CNS_OK;
}

void cns_8t_context_destroy(cns_8t_context_t* ctx) {
    assert(ctx != NULL);
    
    if (ctx->arena) {
        free(ctx->arena);
    }
    
    memset(ctx, 0, sizeof(*ctx));
}

// ============================================================================
// 8T METRICS IMPLEMENTATION
// ============================================================================

void cns_8t_metrics_init(cns_8t_metrics_t* metrics) {
    assert(metrics != NULL);
    memset(metrics, 0, sizeof(*metrics));
}

void cns_8t_metrics_start_operation(cns_8t_metrics_t* metrics) {
    assert(metrics != NULL);
    
#if defined(__x86_64__) || defined(__i386__)
    uint32_t hi, lo;
    __asm__ volatile ("rdtsc" : "=a"(lo), "=d"(hi));
    metrics->operation_start = ((uint64_t)lo) | (((uint64_t)hi) << 32);
#elif defined(__aarch64__)
    __asm__ volatile ("mrs %0, cntvct_el0" : "=r" (metrics->operation_start));
#else
    metrics->operation_start = 0;
#endif
}

void cns_8t_metrics_end_operation(cns_8t_metrics_t* metrics) {
    assert(metrics != NULL);
    
#if defined(__x86_64__) || defined(__i386__)
    uint32_t hi, lo;
    __asm__ volatile ("rdtsc" : "=a"(lo), "=d"(hi));
    metrics->operation_end = ((uint64_t)lo) | (((uint64_t)hi) << 32);
#elif defined(__aarch64__)
    __asm__ volatile ("mrs %0, cntvct_el0" : "=r" (metrics->operation_end));
#else
    metrics->operation_end = metrics->operation_start;
#endif
    
    metrics->total_ticks = metrics->operation_end - metrics->operation_start;
}

void cns_8t_metrics_record_cache_access(cns_8t_metrics_t* metrics, bool hit) {
    assert(metrics != NULL);
    
    if (hit) {
        metrics->l1_cache_hits++;
    } else {
        metrics->l1_cache_misses++;
    }
    
    metrics->cache_line_loads++;
}

// ============================================================================
// L1 CACHE ANALYSIS IMPLEMENTATION
// ============================================================================

cns_8t_l1_analysis_t cns_8t_analyze_l1_usage(cns_8t_context_t* ctx) {
    assert(ctx != NULL);
    
    cns_8t_l1_analysis_t analysis = {0};
    
    // Calculate cache line usage from metrics
    uint32_t total_accesses = ctx->metrics.l1_cache_hits + ctx->metrics.l1_cache_misses;
    if (total_accesses > 0) {
        analysis.cache_efficiency = (double)ctx->metrics.l1_cache_hits / total_accesses;
    } else {
        analysis.cache_efficiency = 1.0;
    }
    
    // Estimate cache line usage (simplified)
    analysis.cache_line_usage = ctx->metrics.cache_line_loads;
    
    // Calculate prefetch opportunities based on access patterns
    analysis.prefetch_opportunities = ctx->metrics.l1_cache_misses / 2; // Simplified heuristic
    
    // Alignment analysis based on allocation patterns
    analysis.alignment_issues = ctx->metrics.allocation_count - 
                               (ctx->metrics.allocation_count * analysis.cache_efficiency);
    
    // Estimate memory bandwidth (simplified calculation)
    if (ctx->metrics.total_ticks > 0) {
        // Assume 3GHz CPU, 64 bytes per cache line
        analysis.memory_bandwidth_mb = (uint32_t)(
            (ctx->metrics.cache_line_loads * 64 * 3000) / ctx->metrics.total_ticks
        );
    }
    
    return analysis;
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

bool cns_8t_is_within_tick_limit(const cns_8t_context_t* ctx) {
    assert(ctx != NULL);
    return ctx->metrics.total_ticks <= CNS_8T_TICK_LIMIT;
}

double cns_8t_get_cache_hit_rate(const cns_8t_context_t* ctx) {
    assert(ctx != NULL);
    
    uint32_t total = ctx->metrics.l1_cache_hits + ctx->metrics.l1_cache_misses;
    if (total == 0) return 1.0;
    
    return (double)ctx->metrics.l1_cache_hits / total;
}

void cns_8t_print_performance_summary(const cns_8t_context_t* ctx) {
    assert(ctx != NULL);
    
    printf("8T Performance Summary:\n");
    printf("======================\n");
    printf("Total ticks: %lu (limit: %d) %s\n", 
           ctx->metrics.total_ticks, 
           CNS_8T_TICK_LIMIT,
           cns_8t_is_within_tick_limit(ctx) ? "✓" : "✗");
    printf("L1 cache hit rate: %.1f%%\n", cns_8t_get_cache_hit_rate(ctx) * 100.0);
    printf("Memory allocated: %zu bytes\n", ctx->metrics.memory_allocated);
    printf("Numerical operations: %u\n", ctx->metrics.numerical_operations);
    printf("Max error bound: %.2e\n", ctx->metrics.max_error_bound);
    printf("SIMD operations: %u (%.1f%% utilization)\n", 
           ctx->metrics.simd_operations,
           ctx->metrics.simd_utilization * 100.0);
}