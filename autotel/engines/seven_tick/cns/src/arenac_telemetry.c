/*  ─────────────────────────────────────────────────────────────
    src/arenac_telemetry.c  –  ARENAC OpenTelemetry Implementation (v2.0)
    7T-compliant telemetry for arena memory allocator
    ───────────────────────────────────────────────────────────── */

#define _DEFAULT_SOURCE
#define _BSD_SOURCE

#include "cns/arenac_telemetry.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <pthread.h>
#include <math.h>

/*═══════════════════════════════════════════════════════════════
  Internal Utilities and Constants
  ═══════════════════════════════════════════════════════════════*/

// Thread-local storage for telemetry context
static __thread arenac_telemetry_context_t* tls_context = NULL;

// Global telemetry configuration
static struct {
    bool initialized;
    uint32_t next_arena_id;
    pthread_mutex_t mutex;
} g_telemetry_state = {false, 1, PTHREAD_MUTEX_INITIALIZER};

// Pattern hash function (FNV-1a)
static inline uint64_t hash_allocation_pattern(size_t size, uint32_t alignment, uint64_t timestamp) {
    uint64_t hash = 14695981039346656037ULL; // FNV offset basis
    const uint64_t prime = 1099511628211ULL; // FNV prime
    
    // Hash size
    hash ^= size;
    hash *= prime;
    
    // Hash alignment
    hash ^= alignment;
    hash *= prime;
    
    // Hash timestamp bucket (reduce to 1ms granularity)
    hash ^= (timestamp / 1000000);
    hash *= prime;
    
    return hash;
}

// Get high-resolution timestamp
static inline uint64_t get_nanoseconds(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

// Check if sampling should occur
static inline bool should_sample(arenac_telemetry_context_t* context) {
    if (!(context->flags & ARENAC_TELEMETRY_SAMPLING)) {
        return true; // Always sample when sampling is disabled
    }
    
    context->sample_counter++;
    double random_value = (double)(context->sample_counter % 10000) / 10000.0;
    return random_value < context->sample_rate;
}

/*═══════════════════════════════════════════════════════════════
  ARENAC Telemetry Initialization
  ═══════════════════════════════════════════════════════════════*/

CNSResult arenac_telemetry_init(
    arenac_telemetry_context_t* context,
    cns_telemetry_t* telemetry,
    uint32_t flags,
    double sample_rate
) {
    if (!context) {
        return CNS_ERROR_INVALID_ARGUMENT;
    }
    
    // Initialize context
    memset(context, 0, sizeof(arenac_telemetry_context_t));
    context->telemetry = telemetry;
    context->flags = flags;
    context->sample_rate = (sample_rate < 0.0) ? 0.0 : (sample_rate > 1.0) ? 1.0 : sample_rate;
    context->sample_counter = 0;
    
    // Initialize spans
    context->current_alloc_span = NULL;
    context->current_free_span = NULL;
    context->current_pattern_span = NULL;
    
    // Initialize counters
    context->total_spans = 0;
    context->sampled_spans = 0;
    context->dropped_spans = 0;
    context->telemetry_memory_usage = sizeof(arenac_telemetry_context_t);
    
    // Initialize pattern tracking
    context->allocation_pattern_hash = 0;
    context->pattern_change_count = 0;
    
    // Initialize distributed tracing
    context->arena_id = 0;
    context->distributed_trace_id = 0;
    context->last_gc_timestamp = get_nanoseconds();
    
    // Set as thread-local context
    tls_context = context;
    
    // Initialize global state if needed
    pthread_mutex_lock(&g_telemetry_state.mutex);
    if (!g_telemetry_state.initialized) {
        g_telemetry_state.initialized = true;
    }
    pthread_mutex_unlock(&g_telemetry_state.mutex);
    
    return CNS_OK;
}

void arenac_telemetry_shutdown(arenac_telemetry_context_t* context) {
    if (!context) {
        return;
    }
    
    // End any active spans
    if (context->current_alloc_span) {
        cns_span_end(context->current_alloc_span, CNS_SPAN_STATUS_OK);
        context->current_alloc_span = NULL;
    }
    
    if (context->current_free_span) {
        cns_span_end(context->current_free_span, CNS_SPAN_STATUS_OK);
        context->current_free_span = NULL;
    }
    
    if (context->current_pattern_span) {
        cns_span_end(context->current_pattern_span, CNS_SPAN_STATUS_OK);
        context->current_pattern_span = NULL;
    }
    
    // Flush any pending data
    if (context->telemetry) {
        cns_telemetry_flush(context->telemetry);
    }
    
    // Clear thread-local context
    if (tls_context == context) {
        tls_context = NULL;
    }
    
    // Zero out context
    memset(context, 0, sizeof(arenac_telemetry_context_t));
}

CNSResult arenac_telemetry_configure_arena(
    arena_t* arena,
    arenac_telemetry_context_t* context,
    uint32_t arena_id
) {
    if (!arena || !context) {
        return CNS_ERROR_INVALID_ARGUMENT;
    }
    
    // Assign arena ID
    if (arena_id == 0) {
        pthread_mutex_lock(&g_telemetry_state.mutex);
        arena_id = g_telemetry_state.next_arena_id++;
        pthread_mutex_unlock(&g_telemetry_state.mutex);
    }
    
    context->arena_id = arena_id;
    
    // Create arena configuration span
    if (context->flags & ARENAC_TELEMETRY_SPANS && context->telemetry) {
        cns_span_t* config_span = cns_span_start(
            context->telemetry,
            "arenac.configure_arena",
            NULL
        );
        
        if (config_span) {
            cns_attribute_t attrs[] = {
                {.key = "arenac.arena_id", .type = CNS_ATTR_INT64, .int64_value = (int64_t)arena_id},
                {.key = "arenac.arena_size", .type = CNS_ATTR_INT64, .int64_value = (int64_t)arena->size},
                {.key = "arenac.arena_flags", .type = CNS_ATTR_INT64, .int64_value = (int64_t)arena->flags},
                {.key = "arenac.zone_count", .type = CNS_ATTR_INT64, .int64_value = (int64_t)arena->zone_count}
            };
            cns_span_set_attributes(config_span, attrs, 4);
            cns_span_end(config_span, CNS_SPAN_STATUS_OK);
        }
    }
    
    // Record initial metrics
    if (context->flags & ARENAC_TELEMETRY_METRICS) {
        arenac_metric_capacity_usage(context, arena);
    }
    
    return CNS_OK;
}

/*═══════════════════════════════════════════════════════════════
  Memory Usage Metrics Implementation
  ═══════════════════════════════════════════════════════════════*/

void arenac_metric_allocation_rate(
    arenac_telemetry_context_t* context,
    uint64_t allocations_per_second
) {
    if (!context || !(context->flags & ARENAC_TELEMETRY_METRICS) || !context->telemetry) {
        return;
    }
    
    // Record allocation rate as a gauge metric
    cns_attribute_t attrs[] = {
        {.key = "arenac.arena_id", .type = CNS_ATTR_INT64, .int64_value = (int64_t)context->arena_id},
        {.key = "arenac.metric_type", .type = CNS_ATTR_STRING, .string_value = "allocation_rate"}
    };
    
    // Create span for metric recording
    cns_span_t* metric_span = cns_span_start(
        context->telemetry,
        "arenac.metric.allocation_rate",
        NULL
    );
    
    if (metric_span) {
        cns_span_set_attributes(metric_span, attrs, 2);
        
        cns_attribute_t rate_attr = {
            .key = "arenac.allocations_per_second",
            .type = CNS_ATTR_INT64,
            .int64_value = (int64_t)allocations_per_second
        };
        cns_span_set_attributes(metric_span, &rate_attr, 1);
        
        cns_span_end(metric_span, CNS_SPAN_STATUS_OK);
    }
}

void arenac_metric_fragmentation(
    arenac_telemetry_context_t* context,
    double fragmentation_percentage
) {
    if (!context || !(context->flags & ARENAC_TELEMETRY_METRICS) || !context->telemetry) {
        return;
    }
    
    // Record fragmentation as a gauge metric
    cns_attribute_t attrs[] = {
        {.key = "arenac.arena_id", .type = CNS_ATTR_INT64, .int64_value = (int64_t)context->arena_id},
        {.key = "arenac.metric_type", .type = CNS_ATTR_STRING, .string_value = "fragmentation"},
        {.key = "arenac.fragmentation_percentage", .type = CNS_ATTR_DOUBLE, .double_value = fragmentation_percentage}
    };
    
    // Create span for metric recording
    cns_span_t* metric_span = cns_span_start(
        context->telemetry,
        "arenac.metric.fragmentation",
        NULL
    );
    
    if (metric_span) {
        cns_span_set_attributes(metric_span, attrs, 3);
        cns_span_end(metric_span, CNS_SPAN_STATUS_OK);
    }
}

/*═══════════════════════════════════════════════════════════════
  Memory Pattern Tracing Implementation
  ═══════════════════════════════════════════════════════════════*/

arenac_pattern_telemetry_t arenac_trace_memory_pattern_start(
    arenac_telemetry_context_t* context,
    const char* pattern_name
) {
    arenac_pattern_telemetry_t pattern = {0};
    
    if (!context || !(context->flags & ARENAC_TELEMETRY_PATTERNS)) {
        return pattern;
    }
    
    // Initialize pattern tracking
    pattern.pattern_hash = hash_allocation_pattern(0, 0, get_nanoseconds());
    pattern.allocation_count = 0;
    pattern.average_size = 0;
    pattern.size_variance = 0;
    pattern.pattern_duration = get_nanoseconds();
    pattern.allocation_frequency = 0;
    pattern.burst_count = 0;
    pattern.fragmentation_ratio = 0.0;
    pattern.utilization_ratio = 0.0;
    pattern.wasted_bytes = 0;
    pattern.average_cycles = 0;
    pattern.max_cycles = 0;
    pattern.violation_count = 0;
    
    // Start pattern span
    if (context->flags & ARENAC_TELEMETRY_SPANS && context->telemetry) {
        context->current_pattern_span = cns_span_start(
            context->telemetry,
            "arenac.memory_pattern",
            NULL
        );
        
        if (context->current_pattern_span) {
            cns_attribute_t attrs[] = {
                {.key = "arenac.pattern_name", .type = CNS_ATTR_STRING, .string_value = pattern_name},
                {.key = "arenac.pattern_hash", .type = CNS_ATTR_INT64, .int64_value = (int64_t)pattern.pattern_hash},
                {.key = "arenac.arena_id", .type = CNS_ATTR_INT64, .int64_value = (int64_t)context->arena_id}
            };
            cns_span_set_attributes(context->current_pattern_span, attrs, 3);
        }
    }
    
    return pattern;
}

void arenac_trace_memory_pattern_end(
    arenac_telemetry_context_t* context,
    arenac_pattern_telemetry_t* pattern
) {
    if (!context || !pattern || !(context->flags & ARENAC_TELEMETRY_PATTERNS)) {
        return;
    }
    
    // Calculate final pattern statistics
    uint64_t end_time = get_nanoseconds();
    pattern->pattern_duration = end_time - pattern->pattern_duration;
    
    if (pattern->allocation_count > 0) {
        pattern->allocation_frequency = pattern->allocation_count * 1000000000ULL / pattern->pattern_duration;
    }
    
    // End pattern span
    if (context->current_pattern_span) {
        cns_attribute_t attrs[] = {
            {.key = "arenac.allocation_count", .type = CNS_ATTR_INT64, .int64_value = (int64_t)pattern->allocation_count},
            {.key = "arenac.average_size", .type = CNS_ATTR_INT64, .int64_value = (int64_t)pattern->average_size},
            {.key = "arenac.pattern_duration_ns", .type = CNS_ATTR_INT64, .int64_value = (int64_t)pattern->pattern_duration},
            {.key = "arenac.allocation_frequency", .type = CNS_ATTR_INT64, .int64_value = (int64_t)pattern->allocation_frequency},
            {.key = "arenac.violation_count", .type = CNS_ATTR_INT64, .int64_value = (int64_t)pattern->violation_count},
            {.key = "arenac.fragmentation_ratio", .type = CNS_ATTR_DOUBLE, .double_value = pattern->fragmentation_ratio},
            {.key = "arenac.utilization_ratio", .type = CNS_ATTR_DOUBLE, .double_value = pattern->utilization_ratio}
        };
        cns_span_set_attributes(context->current_pattern_span, attrs, 7);
        cns_span_end(context->current_pattern_span, CNS_SPAN_STATUS_OK);
        context->current_pattern_span = NULL;
    }
    
    // Update pattern change detection
    if (pattern->pattern_hash != context->allocation_pattern_hash) {
        context->allocation_pattern_hash = pattern->pattern_hash;
        context->pattern_change_count++;
    }
}

void arenac_trace_pattern_allocation(
    arenac_telemetry_context_t* context,
    arenac_pattern_telemetry_t* pattern,
    size_t size,
    uint64_t cycles
) {
    if (!context || !pattern || !(context->flags & ARENAC_TELEMETRY_PATTERNS)) {
        return;
    }
    
    // Update allocation statistics
    pattern->allocation_count++;
    
    // Update size statistics (running average)
    if (pattern->allocation_count == 1) {
        pattern->average_size = (uint32_t)size;
    } else {
        uint64_t total_size = (uint64_t)pattern->average_size * (pattern->allocation_count - 1) + size;
        pattern->average_size = (uint32_t)(total_size / pattern->allocation_count);
    }
    
    // Update cycle statistics
    if (pattern->allocation_count == 1) {
        pattern->average_cycles = cycles;
        pattern->max_cycles = cycles;
    } else {
        uint64_t total_cycles = pattern->average_cycles * (pattern->allocation_count - 1) + cycles;
        pattern->average_cycles = total_cycles / pattern->allocation_count;
        if (cycles > pattern->max_cycles) {
            pattern->max_cycles = cycles;
        }
    }
    
    // Check for 7-tick violations
    if (cycles > ARENAC_TELEMETRY_MAX_CYCLES) {
        pattern->violation_count++;
    }
    
    // Update pattern hash
    pattern->pattern_hash = hash_allocation_pattern(size, 8, get_nanoseconds());
}

/*═══════════════════════════════════════════════════════════════
  Distributed Tracing Implementation
  ═══════════════════════════════════════════════════════════════*/

uint64_t arenac_distributed_trace_start(
    arenac_telemetry_context_t* context,
    const char* operation_name,
    uint32_t arena_count
) {
    if (!context || !(context->flags & ARENAC_TELEMETRY_DISTRIBUTED)) {
        return 0;
    }
    
    // Generate unique trace ID
    uint64_t trace_id = get_nanoseconds() ^ ((uint64_t)context->arena_id << 32);
    context->distributed_trace_id = trace_id;
    
    // Start distributed trace span
    if (context->flags & ARENAC_TELEMETRY_SPANS && context->telemetry) {
        cns_span_t* trace_span = cns_span_start(
            context->telemetry,
            "arenac.distributed_operation",
            NULL
        );
        
        if (trace_span) {
            cns_attribute_t attrs[] = {
                {.key = "arenac.operation_name", .type = CNS_ATTR_STRING, .string_value = operation_name},
                {.key = "arenac.trace_id", .type = CNS_ATTR_INT64, .int64_value = (int64_t)trace_id},
                {.key = "arenac.arena_count", .type = CNS_ATTR_INT64, .int64_value = (int64_t)arena_count},
                {.key = "arenac.source_arena_id", .type = CNS_ATTR_INT64, .int64_value = (int64_t)context->arena_id}
            };
            cns_span_set_attributes(trace_span, attrs, 4);
            
            // Add event for trace start
            cns_span_add_event(trace_span, "distributed_trace_started", attrs, 4);
        }
    }
    
    return trace_id;
}

void arenac_distributed_trace_end(
    arenac_telemetry_context_t* context,
    uint64_t trace_id,
    bool success
) {
    if (!context || !(context->flags & ARENAC_TELEMETRY_DISTRIBUTED) || 
        context->distributed_trace_id != trace_id) {
        return;
    }
    
    // End distributed trace span
    if (context->flags & ARENAC_TELEMETRY_SPANS && context->telemetry) {
        // Find and end the distributed trace span
        // In a full implementation, we would maintain a span stack
        cns_attribute_t attrs[] = {
            {.key = "arenac.trace_id", .type = CNS_ATTR_INT64, .int64_value = (int64_t)trace_id},
            {.key = "arenac.success", .type = CNS_ATTR_BOOL, .bool_value = success}
        };
        
        // Create completion event
        cns_span_t* completion_span = cns_span_start(
            context->telemetry,
            "arenac.distributed_trace_complete",
            NULL
        );
        
        if (completion_span) {
            cns_span_set_attributes(completion_span, attrs, 2);
            cns_span_end(completion_span, success ? CNS_SPAN_STATUS_OK : CNS_SPAN_STATUS_ERROR);
        }
    }
    
    // Clear distributed trace ID
    context->distributed_trace_id = 0;
}

void arenac_distributed_trace_propagate(
    const arenac_telemetry_context_t* source_context,
    arenac_telemetry_context_t* target_context,
    uint64_t trace_id
) {
    if (!source_context || !target_context || 
        !(source_context->flags & ARENAC_TELEMETRY_DISTRIBUTED) ||
        !(target_context->flags & ARENAC_TELEMETRY_DISTRIBUTED)) {
        return;
    }
    
    // Propagate trace context
    target_context->distributed_trace_id = trace_id;
    
    // Create propagation event
    if (target_context->flags & ARENAC_TELEMETRY_SPANS && target_context->telemetry) {
        cns_span_t* propagation_span = cns_span_start(
            target_context->telemetry,
            "arenac.trace_propagation",
            NULL
        );
        
        if (propagation_span) {
            cns_attribute_t attrs[] = {
                {.key = "arenac.trace_id", .type = CNS_ATTR_INT64, .int64_value = (int64_t)trace_id},
                {.key = "arenac.source_arena_id", .type = CNS_ATTR_INT64, .int64_value = (int64_t)source_context->arena_id},
                {.key = "arenac.target_arena_id", .type = CNS_ATTR_INT64, .int64_value = (int64_t)target_context->arena_id}
            };
            cns_span_set_attributes(propagation_span, attrs, 3);
            cns_span_end(propagation_span, CNS_SPAN_STATUS_OK);
        }
    }
}

/*═══════════════════════════════════════════════════════════════
  Telemetry Data Management Implementation
  ═══════════════════════════════════════════════════════════════*/

void arenac_telemetry_get_stats(
    const arenac_telemetry_context_t* context,
    void* stats_ptr
) {
    struct {
        uint64_t total_spans;
        uint64_t sampled_spans;
        uint64_t dropped_spans;
        size_t memory_usage;
        double sample_efficiency;
        uint32_t pattern_changes;
    }* stats = (struct {
        uint64_t total_spans;
        uint64_t sampled_spans;
        uint64_t dropped_spans;
        size_t memory_usage;
        double sample_efficiency;
        uint32_t pattern_changes;
    }*)stats_ptr;
    if (!context || !stats) {
        return;
    }
    
    stats->total_spans = context->total_spans;
    stats->sampled_spans = context->sampled_spans;
    stats->dropped_spans = context->dropped_spans;
    stats->memory_usage = context->telemetry_memory_usage;
    stats->pattern_changes = context->pattern_change_count;
    
    // Calculate sample efficiency
    if (context->total_spans > 0) {
        stats->sample_efficiency = (double)context->sampled_spans / context->total_spans;
    } else {
        stats->sample_efficiency = 0.0;
    }
}

CNSResult arenac_telemetry_flush(arenac_telemetry_context_t* context) {
    if (!context || !context->telemetry) {
        return CNS_ERROR_INVALID_ARGUMENT;
    }
    
    // Flush underlying telemetry system
    CNSResult result = cns_telemetry_flush(context->telemetry);
    
    // Update garbage collection timestamp
    context->last_gc_timestamp = get_nanoseconds();
    
    return result;
}

void arenac_telemetry_set_sampling(
    arenac_telemetry_context_t* context,
    double sample_rate
) {
    if (!context) {
        return;
    }
    
    // Clamp sample rate to valid range
    if (sample_rate < 0.0) {
        sample_rate = 0.0;
    } else if (sample_rate > 1.0) {
        sample_rate = 1.0;
    }
    
    context->sample_rate = sample_rate;
    
    // Enable or disable sampling flag based on rate
    if (sample_rate >= 1.0) {
        context->flags &= ~ARENAC_TELEMETRY_SAMPLING;
    } else {
        context->flags |= ARENAC_TELEMETRY_SAMPLING;
    }
    
    // Record sampling configuration change
    if (context->flags & ARENAC_TELEMETRY_SPANS && context->telemetry) {
        cns_span_t* config_span = cns_span_start(
            context->telemetry,
            "arenac.sampling_config_changed",
            NULL
        );
        
        if (config_span) {
            cns_attribute_t attrs[] = {
                {.key = "arenac.new_sample_rate", .type = CNS_ATTR_DOUBLE, .double_value = sample_rate},
                {.key = "arenac.arena_id", .type = CNS_ATTR_INT64, .int64_value = (int64_t)context->arena_id}
            };
            cns_span_set_attributes(config_span, attrs, 2);
            cns_span_end(config_span, CNS_SPAN_STATUS_OK);
        }
    }
}

/*═══════════════════════════════════════════════════════════════
  Optional Integration Functions
  ═══════════════════════════════════════════════════════════════*/

// Get thread-local telemetry context
arenac_telemetry_context_t* arenac_telemetry_get_context(void) {
    return tls_context;
}

// Set thread-local telemetry context
void arenac_telemetry_set_context(arenac_telemetry_context_t* context) {
    tls_context = context;
}

// Helper function to calculate arena fragmentation
double arenac_calculate_fragmentation(const arena_t* arena) {
    if (!arena || arena->size == 0) {
        return 0.0;
    }
    
    // Simple fragmentation calculation based on zone utilization variance
    double total_utilization = 0.0;
    double utilization_variance = 0.0;
    
    for (uint32_t i = 0; i < arena->zone_count; i++) {
        double zone_utilization = (double)arena->zones[i].used / arena->zones[i].size;
        total_utilization += zone_utilization;
    }
    
    double average_utilization = total_utilization / arena->zone_count;
    
    for (uint32_t i = 0; i < arena->zone_count; i++) {
        double zone_utilization = (double)arena->zones[i].used / arena->zones[i].size;
        double diff = zone_utilization - average_utilization;
        utilization_variance += diff * diff;
    }
    
    utilization_variance /= arena->zone_count;
    
    // Fragmentation is proportional to utilization variance
    return sqrt(utilization_variance) * 100.0;
}

// Helper function to estimate allocation efficiency
double arenac_calculate_efficiency(const arena_t* arena, const arenac_telemetry_context_t* context) {
    if (!arena || !context) {
        return 0.0;
    }
    
    // Calculate efficiency based on utilization and performance
    double utilization = (double)arena->used / arena->size;
    double fragmentation = arenac_calculate_fragmentation(arena);
    double performance_factor = 1.0;
    
    // Adjust for 7-tick violations if we have telemetry data
    if (context->total_spans > 0) {
        // Estimate performance factor based on pattern violations
        // This is a simplified calculation
        performance_factor = 1.0 - ((double)context->pattern_change_count / context->total_spans * 0.1);
        if (performance_factor < 0.0) performance_factor = 0.0;
    }
    
    // Efficiency = Utilization * (1 - Fragmentation) * Performance
    double efficiency = utilization * (1.0 - fragmentation / 100.0) * performance_factor;
    
    return efficiency * 100.0; // Convert to percentage
}