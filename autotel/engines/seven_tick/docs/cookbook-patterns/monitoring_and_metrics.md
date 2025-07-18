# Pattern: Monitoring and Metrics

## Description
This pattern demonstrates how to implement comprehensive monitoring and metrics collection for the 7T Engine, including performance metrics, health checks, and Prometheus-compatible exports for production monitoring.

## Code Example
```c
#include "c_src/sparql7t.h"
#include <stdio.h>
#include <time.h>
#include <stdint.h>

// Engine metrics structure
typedef struct {
    uint64_t total_requests;
    uint64_t successful_requests;
    uint64_t failed_requests;
    uint64_t pattern_matches;
    uint64_t triple_additions;
    uint64_t batch_operations;
    uint64_t total_latency_ns;
    uint64_t max_latency_ns;
    uint64_t min_latency_ns;
    time_t start_time;
    size_t memory_usage_bytes;
} EngineMetrics;

// Initialize metrics
void init_metrics(EngineMetrics* metrics) {
    metrics->total_requests = 0;
    metrics->successful_requests = 0;
    metrics->failed_requests = 0;
    metrics->pattern_matches = 0;
    metrics->triple_additions = 0;
    metrics->batch_operations = 0;
    metrics->total_latency_ns = 0;
    metrics->max_latency_ns = 0;
    metrics->min_latency_ns = UINT64_MAX;
    metrics->start_time = time(NULL);
    metrics->memory_usage_bytes = 0;
}

// High-precision timing
static inline uint64_t get_nanoseconds() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

// Record operation metrics
void record_operation(EngineMetrics* metrics, uint64_t latency_ns, int success) {
    metrics->total_requests++;
    metrics->total_latency_ns += latency_ns;
    
    if (success) {
        metrics->successful_requests++;
    } else {
        metrics->failed_requests++;
    }
    
    if (latency_ns > metrics->max_latency_ns) {
        metrics->max_latency_ns = latency_ns;
    }
    
    if (latency_ns < metrics->min_latency_ns) {
        metrics->min_latency_ns = latency_ns;
    }
}

// Wrapper for pattern matching with metrics
int monitored_ask_pattern(S7TEngine* engine, uint32_t s, uint32_t p, uint32_t o, EngineMetrics* metrics) {
    uint64_t start = get_nanoseconds();
    int result = s7t_ask_pattern(engine, s, p, o);
    uint64_t end = get_nanoseconds();
    
    record_operation(metrics, end - start, 1);
    if (result) metrics->pattern_matches++;
    
    return result;
}

// Wrapper for triple addition with metrics
void monitored_add_triple(S7TEngine* engine, uint32_t s, uint32_t p, uint32_t o, EngineMetrics* metrics) {
    uint64_t start = get_nanoseconds();
    s7t_add_triple(engine, s, p, o);
    uint64_t end = get_nanoseconds();
    
    record_operation(metrics, end - start, 1);
    metrics->triple_additions++;
}

// Wrapper for batch operations with metrics
void monitored_ask_batch(S7TEngine* engine, TriplePattern* patterns, int* results, size_t count, EngineMetrics* metrics) {
    uint64_t start = get_nanoseconds();
    s7t_ask_batch(engine, patterns, results, count);
    uint64_t end = get_nanoseconds();
    
    record_operation(metrics, end - start, 1);
    metrics->batch_operations++;
}

// Calculate derived metrics
void calculate_derived_metrics(EngineMetrics* metrics) {
    time_t uptime = time(NULL) - metrics->start_time;
    if (uptime > 0) {
        double requests_per_sec = (double)metrics->total_requests / uptime;
        double avg_latency_ns = metrics->total_requests > 0 ? 
            (double)metrics->total_latency_ns / metrics->total_requests : 0;
        
        printf("Uptime: %ld seconds\n", uptime);
        printf("Requests/sec: %.2f\n", requests_per_sec);
        printf("Average latency: %.2f ns\n", avg_latency_ns);
        printf("Min/Max latency: %lu/%lu ns\n", metrics->min_latency_ns, metrics->max_latency_ns);
        printf("Success rate: %.2f%%\n", 
               (double)metrics->successful_requests / metrics->total_requests * 100.0);
    }
}

// Export Prometheus metrics
void export_prometheus_metrics(EngineMetrics* metrics, char* buffer, size_t size) {
    time_t uptime = time(NULL) - metrics->start_time;
    double avg_latency_ns = metrics->total_requests > 0 ? 
        (double)metrics->total_latency_ns / metrics->total_requests : 0;
    double requests_per_sec = uptime > 0 ? (double)metrics->total_requests / uptime : 0;
    
    snprintf(buffer, size,
        "# HELP seven_tick_requests_total Total requests processed\n"
        "# TYPE seven_tick_requests_total counter\n"
        "seven_tick_requests_total %lu\n"
        "# HELP seven_tick_successful_requests_total Total successful requests\n"
        "# TYPE seven_tick_successful_requests_total counter\n"
        "seven_tick_successful_requests_total %lu\n"
        "# HELP seven_tick_failed_requests_total Total failed requests\n"
        "# TYPE seven_tick_failed_requests_total counter\n"
        "seven_tick_failed_requests_total %lu\n"
        "# HELP seven_tick_pattern_matches_total Total pattern matches\n"
        "# TYPE seven_tick_pattern_matches_total counter\n"
        "seven_tick_pattern_matches_total %lu\n"
        "# HELP seven_tick_triple_additions_total Total triple additions\n"
        "# TYPE seven_tick_triple_additions_total counter\n"
        "seven_tick_triple_additions_total %lu\n"
        "# HELP seven_tick_batch_operations_total Total batch operations\n"
        "# TYPE seven_tick_batch_operations_total counter\n"
        "seven_tick_batch_operations_total %lu\n"
        "# HELP seven_tick_avg_latency_ns Average latency in nanoseconds\n"
        "# TYPE seven_tick_avg_latency_ns gauge\n"
        "seven_tick_avg_latency_ns %.2f\n"
        "# HELP seven_tick_max_latency_ns Maximum latency in nanoseconds\n"
        "# TYPE seven_tick_max_latency_ns gauge\n"
        "seven_tick_max_latency_ns %lu\n"
        "# HELP seven_tick_min_latency_ns Minimum latency in nanoseconds\n"
        "# TYPE seven_tick_min_latency_ns gauge\n"
        "seven_tick_min_latency_ns %lu\n"
        "# HELP seven_tick_requests_per_sec Requests per second\n"
        "# TYPE seven_tick_requests_per_sec gauge\n"
        "seven_tick_requests_per_sec %.2f\n"
        "# HELP seven_tick_uptime_seconds Engine uptime in seconds\n"
        "# TYPE seven_tick_uptime_seconds gauge\n"
        "seven_tick_uptime_seconds %ld\n",
        metrics->total_requests,
        metrics->successful_requests,
        metrics->failed_requests,
        metrics->pattern_matches,
        metrics->triple_additions,
        metrics->batch_operations,
        avg_latency_ns,
        metrics->max_latency_ns,
        metrics->min_latency_ns,
        requests_per_sec,
        uptime
    );
}

// Health check function
int health_check(S7TEngine* engine) {
    if (!engine) return 0;
    
    // Basic functionality test
    s7t_add_triple(engine, 1, 2, 3);
    int result = s7t_ask_pattern(engine, 1, 2, 3);
    
    return result == 1;
}

// Memory usage estimation
size_t estimate_memory_usage(S7TEngine* engine) {
    if (!engine) return 0;
    
    size_t stride_len = engine->stride_len;
    size_t predicate_memory = engine->max_predicates * stride_len * sizeof(uint64_t);
    size_t object_memory = engine->max_objects * stride_len * sizeof(uint64_t);
    size_t index_memory = engine->max_predicates * engine->max_subjects * sizeof(void*);
    
    return predicate_memory + object_memory + index_memory;
}

int main() {
    S7TEngine* engine = s7t_create(10000, 100, 10000);
    if (!engine) {
        fprintf(stderr, "Failed to create engine\n");
        return 1;
    }
    
    EngineMetrics metrics;
    init_metrics(&metrics);
    
    // Simulate workload with monitoring
    for (int i = 0; i < 1000; i++) {
        monitored_add_triple(engine, i, i % 10, i % 20, &metrics);
        monitored_ask_pattern(engine, i, i % 10, i % 20, &metrics);
        
        if (i % 100 == 0) {
            TriplePattern patterns[4] = {
                {i, i % 10, i % 20},
                {i + 1, (i + 1) % 10, (i + 1) % 20},
                {i + 2, (i + 2) % 10, (i + 2) % 20},
                {i + 3, (i + 3) % 10, (i + 3) % 20}
            };
            int results[4];
            monitored_ask_batch(engine, patterns, results, 4, &metrics);
        }
    }
    
    // Update memory usage
    metrics.memory_usage_bytes = estimate_memory_usage(engine);
    
    // Report metrics
    calculate_derived_metrics(&metrics);
    
    // Export Prometheus metrics
    char prometheus_buffer[4096];
    export_prometheus_metrics(&metrics, prometheus_buffer, sizeof(prometheus_buffer));
    printf("\nPrometheus Metrics:\n%s\n", prometheus_buffer);
    
    // Health check
    printf("Health check: %s\n", health_check(engine) ? "PASS" : "FAIL");
    
    s7t_destroy(engine);
    return 0;
}
```

## Tips
- Use Prometheus metrics for production monitoring.
- Implement health checks for load balancers.
- Monitor memory usage to prevent OOM conditions.
- Set up alerting based on latency and error rates. 