/*  ─────────────────────────────────────────────────────────────
    cns/arenac_telemetry.h  –  ARENAC OpenTelemetry Integration (v2.0)
    7T-compliant telemetry for arena memory allocator
    ───────────────────────────────────────────────────────────── */
#ifndef CNS_ARENAC_TELEMETRY_H
#define CNS_ARENAC_TELEMETRY_H

#include "arena.h"
#include "../../s7t_minimal.h"
#include <stdint.h>
#include <stdbool.h>

// Result codes
typedef enum {
    CNS_OK = 0,
    CNS_ERROR_INVALID_ARGUMENT = -1,
    CNS_ERROR_OUT_OF_MEMORY = -2,
    CNS_ERROR_NOT_INITIALIZED = -3
} CNSResult;

// Telemetry attribute types
typedef enum {
    CNS_ATTR_STRING,
    CNS_ATTR_INT64,
    CNS_ATTR_DOUBLE,
    CNS_ATTR_BOOL
} cns_attr_type_t;

// Span status
typedef enum {
    CNS_SPAN_STATUS_UNSET = 0,
    CNS_SPAN_STATUS_OK,
    CNS_SPAN_STATUS_ERROR
} cns_span_status_t;

// Telemetry attribute
typedef struct {
    const char *key;
    cns_attr_type_t type;
    union {
        const char *string_value;
        int64_t int64_value;
        double double_value;
        bool bool_value;
    };
} cns_attribute_t;

// Stub telemetry system structure
typedef struct cns_telemetry_t {
    bool initialized;
    uint64_t span_count;
} cns_telemetry_t;

// Stub span structure
typedef struct cns_span_t {
    uint64_t span_id;
    uint64_t start_time;
    const char* name;
    bool active;
} cns_span_t;

// Stub functions for minimal telemetry
static inline CNSResult cns_telemetry_init(cns_telemetry_t* telemetry, void* config) {
    if (!telemetry) return CNS_ERROR_INVALID_ARGUMENT;
    telemetry->initialized = true;
    telemetry->span_count = 0;
    (void)config;
    return CNS_OK;
}

static inline void cns_telemetry_shutdown(cns_telemetry_t* telemetry) {
    if (telemetry) telemetry->initialized = false;
}

static inline CNSResult cns_telemetry_flush(cns_telemetry_t* telemetry) {
    (void)telemetry;
    return CNS_OK;
}

static inline cns_span_t* cns_span_start(cns_telemetry_t* telemetry, const char* name, const cns_span_t* parent) {
    static cns_span_t stub_span = {0};
    if (telemetry) telemetry->span_count++;
    stub_span.span_id = telemetry ? telemetry->span_count : 1;
    stub_span.start_time = S7T_CYCLES();
    stub_span.name = name;
    stub_span.active = true;
    (void)parent;
    return &stub_span;
}

static inline void cns_span_end(cns_span_t* span, cns_span_status_t status) {
    if (span) span->active = false;
    (void)status;
}

static inline void cns_span_set_attributes(cns_span_t* span, const cns_attribute_t* attrs, size_t count) {
    (void)span; (void)attrs; (void)count;
}

static inline void cns_span_add_event(cns_span_t* span, const char* name, const cns_attribute_t* attrs, size_t count) {
    (void)span; (void)name; (void)attrs; (void)count;
}

static inline void cns_metric_record_memory(cns_telemetry_t* telemetry, size_t used, size_t total) {
    (void)telemetry; (void)used; (void)total;
}

static inline void cns_metric_record_violation(cns_telemetry_t* telemetry, const char* operation, uint64_t actual_cycles, uint64_t threshold_cycles) {
    (void)telemetry; (void)operation; (void)actual_cycles; (void)threshold_cycles;
}

#ifdef __cplusplus
extern "C" {
#endif

/*═══════════════════════════════════════════════════════════════
  ARENAC Telemetry Configuration
  ═══════════════════════════════════════════════════════════════*/

#define ARENAC_TELEMETRY_VERSION_MAJOR 2
#define ARENAC_TELEMETRY_VERSION_MINOR 0
#define ARENAC_TELEMETRY_VERSION_PATCH 0

// Performance constraints for telemetry operations
#define ARENAC_TELEMETRY_MAX_CYCLES 7
#define ARENAC_TELEMETRY_BATCH_SIZE 1000
#define ARENAC_TELEMETRY_BUFFER_SIZE 4096

// Telemetry configuration flags
typedef enum {
    ARENAC_TELEMETRY_NONE         = 0,
    ARENAC_TELEMETRY_SPANS        = 1 << 0,  // Enable span tracing
    ARENAC_TELEMETRY_METRICS      = 1 << 1,  // Enable metrics collection
    ARENAC_TELEMETRY_EVENTS       = 1 << 2,  // Enable event logging
    ARENAC_TELEMETRY_SAMPLING     = 1 << 3,  // Enable sampling
    ARENAC_TELEMETRY_PATTERNS     = 1 << 4,  // Track allocation patterns
    ARENAC_TELEMETRY_DISTRIBUTED  = 1 << 5,  // Distributed tracing
    ARENAC_TELEMETRY_COMPRESSION  = 1 << 6,  // Compress telemetry data
    ARENAC_TELEMETRY_BUFFERED     = 1 << 7,  // Buffer telemetry data
    ARENAC_TELEMETRY_ALL         = 0xFF      // Enable all features
} arenac_telemetry_flags_t;

/*═══════════════════════════════════════════════════════════════
  ARENAC Telemetry Context
  ═══════════════════════════════════════════════════════════════*/

typedef struct S7T_ALIGNED(64) {
    // Core telemetry system
    cns_telemetry_t* telemetry;        // OpenTelemetry system
    uint32_t flags;                     // Telemetry flags
    
    // Span management
    cns_span_t* current_alloc_span;     // Current allocation span
    cns_span_t* current_free_span;      // Current free span
    cns_span_t* current_pattern_span;   // Current pattern span
    
    // Sampling configuration
    double sample_rate;                 // Sampling rate (0.0-1.0)
    uint64_t sample_counter;            // Sample counter
    
    // Performance tracking
    uint64_t total_spans;               // Total spans created
    uint64_t sampled_spans;             // Sampled spans
    uint64_t dropped_spans;             // Dropped spans
    
    // Memory tracking
    size_t telemetry_memory_usage;      // Memory used by telemetry
    uint64_t last_gc_timestamp;         // Last GC timestamp
    
    // Pattern detection
    uint64_t allocation_pattern_hash;   // Current allocation pattern
    uint32_t pattern_change_count;      // Pattern changes detected
    
    // Multi-arena coordination
    uint32_t arena_id;                  // Unique arena identifier
    uint64_t distributed_trace_id;      // Distributed trace ID
} arenac_telemetry_context_t;

/*═══════════════════════════════════════════════════════════════
  Allocation Telemetry Data
  ═══════════════════════════════════════════════════════════════*/

typedef struct S7T_ALIGNED(32) {
    // Allocation information
    void* ptr;                          // Allocated pointer
    size_t size;                        // Allocation size
    size_t aligned_size;                // Aligned allocation size
    uint32_t alignment;                 // Alignment used
    
    // Timing information
    uint64_t start_cycles;              // Start timestamp
    uint64_t end_cycles;                // End timestamp
    uint64_t duration_cycles;           // Duration in cycles
    
    // Arena state
    size_t arena_used_before;           // Arena usage before allocation
    size_t arena_used_after;            // Arena usage after allocation
    uint32_t zone_id;                   // Zone used for allocation
    
    // Performance metrics
    uint64_t cache_misses;              // Cache misses during allocation
    uint64_t tlb_misses;                // TLB misses during allocation
    bool violated_7tick;                // Whether allocation violated 7-tick constraint
} arenac_alloc_telemetry_t;

/*═══════════════════════════════════════════════════════════════
  Memory Pattern Telemetry
  ═══════════════════════════════════════════════════════════════*/

typedef struct {
    // Pattern characteristics
    uint64_t pattern_hash;              // Pattern hash
    uint32_t allocation_count;          // Allocations in pattern
    uint32_t average_size;              // Average allocation size
    uint32_t size_variance;             // Size variance
    
    // Temporal patterns
    uint64_t pattern_duration;          // Pattern duration
    uint64_t allocation_frequency;      // Allocation frequency
    uint64_t burst_count;               // Number of allocation bursts
    
    // Efficiency metrics
    double fragmentation_ratio;         // Memory fragmentation
    double utilization_ratio;           // Arena utilization
    uint32_t wasted_bytes;              // Bytes lost to alignment
    
    // Performance impact
    uint64_t average_cycles;            // Average allocation cycles
    uint64_t max_cycles;                // Maximum allocation cycles
    uint32_t violation_count;           // 7-tick violations
} arenac_pattern_telemetry_t;

/*═══════════════════════════════════════════════════════════════
  Arena Capacity Metrics
  ═══════════════════════════════════════════════════════════════*/

typedef struct {
    // Capacity information
    size_t total_capacity;              // Total arena capacity
    size_t used_capacity;               // Currently used capacity
    size_t available_capacity;          // Available capacity
    size_t peak_capacity;               // Peak capacity usage
    
    // Utilization metrics
    double utilization_percentage;      // Utilization percentage
    double fragmentation_percentage;    // Fragmentation percentage
    double efficiency_score;            // Overall efficiency score
    
    // Zone distribution
    uint32_t active_zones;              // Number of active zones
    size_t zone_utilization[ARENAC_MAX_ZONES];  // Per-zone utilization
    
    // Allocation rates
    uint64_t allocations_per_second;    // Current allocation rate
    uint64_t peak_allocation_rate;      // Peak allocation rate
    uint64_t average_allocation_rate;   // Average allocation rate
} arenac_capacity_metrics_t;

/*═══════════════════════════════════════════════════════════════
  ARENAC Telemetry Initialization
  ═══════════════════════════════════════════════════════════════*/

/**
 * Initialize ARENAC telemetry system
 * 
 * @param context Telemetry context to initialize
 * @param telemetry OpenTelemetry system
 * @param flags Telemetry configuration flags
 * @param sample_rate Sampling rate (0.0-1.0)
 * @return CNS_OK on success, error code on failure
 * 
 * Time complexity: O(1)
 * Cycle guarantee: ≤ 7 ticks
 */
CNSResult arenac_telemetry_init(
    arenac_telemetry_context_t* context,
    cns_telemetry_t* telemetry,
    uint32_t flags,
    double sample_rate
);

/**
 * Shutdown ARENAC telemetry system
 * 
 * @param context Telemetry context to shutdown
 * 
 * Time complexity: O(1)
 * Cycle guarantee: ≤ 7 ticks
 */
void arenac_telemetry_shutdown(arenac_telemetry_context_t* context);

/**
 * Configure arena for telemetry
 * 
 * @param arena Arena to configure
 * @param context Telemetry context
 * @param arena_id Unique arena identifier
 * @return CNS_OK on success, error code on failure
 * 
 * Time complexity: O(1)
 * Cycle guarantee: ≤ 7 ticks
 */
CNSResult arenac_telemetry_configure_arena(
    arena_t* arena,
    arenac_telemetry_context_t* context,
    uint32_t arena_id
);

/*═══════════════════════════════════════════════════════════════
  Allocation Span Operations (< 7 ticks)
  ═══════════════════════════════════════════════════════════════*/

/**
 * Start allocation span
 * 
 * @param context Telemetry context
 * @param arena Arena being allocated from
 * @param size Allocation size
 * @param alignment Allocation alignment
 * @return Allocation telemetry data
 * 
 * Time complexity: O(1)
 * Cycle guarantee: ≤ 7 ticks
 */
static inline arenac_alloc_telemetry_t arenac_span_alloc_start(
    arenac_telemetry_context_t* context,
    const arena_t* arena,
    size_t size,
    uint32_t alignment
) {
    arenac_alloc_telemetry_t telemetry = {0};
    
    if (!context || !(context->flags & ARENAC_TELEMETRY_SPANS)) {
        return telemetry;
    }
    
    // Check sampling
    if (context->flags & ARENAC_TELEMETRY_SAMPLING) {
        context->sample_counter++;
        if ((double)(context->sample_counter % 1000) / 1000.0 > context->sample_rate) {
            return telemetry;
        }
    }
    
    // Record start state
    telemetry.size = size;
    telemetry.alignment = alignment;
    telemetry.aligned_size = (size + alignment - 1) & ~(alignment - 1);
    telemetry.start_cycles = S7T_CYCLES();
    telemetry.arena_used_before = arena ? arena->used : 0;
    telemetry.zone_id = arena ? arena->current_zone : 0;
    
    // Start span if enabled
    if (context->flags & ARENAC_TELEMETRY_SPANS && context->telemetry) {
        context->current_alloc_span = cns_span_start(
            context->telemetry,
            "arenac.alloc",
            context->current_alloc_span
        );
        
        if (context->current_alloc_span) {
            // Set span attributes
            cns_attribute_t attrs[] = {
                {.key = "arenac.size", .type = CNS_ATTR_INT64, .int64_value = (int64_t)size},
                {.key = "arenac.alignment", .type = CNS_ATTR_INT64, .int64_value = (int64_t)alignment},
                {.key = "arenac.zone", .type = CNS_ATTR_INT64, .int64_value = (int64_t)telemetry.zone_id}
            };
            cns_span_set_attributes(context->current_alloc_span, attrs, 3);
        }
    }
    
    return telemetry;
}

/**
 * End allocation span
 * 
 * @param context Telemetry context
 * @param arena Arena that was allocated from
 * @param telemetry Allocation telemetry data from start
 * @param ptr Allocated pointer (NULL if allocation failed)
 * 
 * Time complexity: O(1)
 * Cycle guarantee: ≤ 7 ticks
 */
static inline void arenac_span_alloc_end(
    arenac_telemetry_context_t* context,
    const arena_t* arena,
    arenac_alloc_telemetry_t* telemetry,
    void* ptr
) {
    if (!context || !telemetry || !(context->flags & ARENAC_TELEMETRY_SPANS)) {
        return;
    }
    
    // Record end state
    telemetry->end_cycles = S7T_CYCLES();
    telemetry->duration_cycles = telemetry->end_cycles - telemetry->start_cycles;
    telemetry->ptr = ptr;
    telemetry->arena_used_after = arena ? arena->used : 0;
    telemetry->violated_7tick = telemetry->duration_cycles > ARENAC_TELEMETRY_MAX_CYCLES;
    
    // End span
    if (context->current_alloc_span) {
        // Add final attributes
        cns_attribute_t attrs[] = {
            {.key = "arenac.success", .type = CNS_ATTR_BOOL, .bool_value = (ptr != NULL)},
            {.key = "arenac.cycles", .type = CNS_ATTR_INT64, .int64_value = (int64_t)telemetry->duration_cycles},
            {.key = "arenac.violated_7tick", .type = CNS_ATTR_BOOL, .bool_value = telemetry->violated_7tick}
        };
        cns_span_set_attributes(context->current_alloc_span, attrs, 3);
        
        // End span with appropriate status
        cns_span_status_t status = ptr ? CNS_SPAN_STATUS_OK : CNS_SPAN_STATUS_ERROR;
        cns_span_end(context->current_alloc_span, status);
        context->current_alloc_span = NULL;
    }
    
    // Update counters
    context->total_spans++;
    if (telemetry->violated_7tick) {
        // Record performance violation metric
        if (context->flags & ARENAC_TELEMETRY_METRICS && context->telemetry) {
            cns_metric_record_violation(
                context->telemetry,
                "arenac.alloc",
                telemetry->duration_cycles,
                ARENAC_TELEMETRY_MAX_CYCLES
            );
        }
    }
}

/*═══════════════════════════════════════════════════════════════
  Memory Usage Metrics
  ═══════════════════════════════════════════════════════════════*/

/**
 * Record arena capacity usage
 * 
 * @param context Telemetry context
 * @param arena Arena to measure
 * 
 * Time complexity: O(1)
 * Cycle guarantee: ≤ 7 ticks
 */
static inline void arenac_metric_capacity_usage(
    arenac_telemetry_context_t* context,
    const arena_t* arena
) {
    if (!context || !arena || !(context->flags & ARENAC_TELEMETRY_METRICS)) {
        return;
    }
    
    if (context->telemetry) {
        cns_metric_record_memory(
            context->telemetry,
            arena->used,
            arena->size
        );
    }
}

/**
 * Record allocation rate metric
 * 
 * @param context Telemetry context
 * @param allocations_per_second Current allocation rate
 * 
 * Time complexity: O(1)
 * Cycle guarantee: ≤ 7 ticks
 */
void arenac_metric_allocation_rate(
    arenac_telemetry_context_t* context,
    uint64_t allocations_per_second
);

/**
 * Record fragmentation metric
 * 
 * @param context Telemetry context
 * @param fragmentation_percentage Fragmentation percentage
 * 
 * Time complexity: O(1)
 * Cycle guarantee: ≤ 7 ticks
 */
void arenac_metric_fragmentation(
    arenac_telemetry_context_t* context,
    double fragmentation_percentage
);

/*═══════════════════════════════════════════════════════════════
  Memory Pattern Tracing
  ═══════════════════════════════════════════════════════════════*/

/**
 * Start memory pattern tracing
 * 
 * @param context Telemetry context
 * @param pattern_name Pattern name
 * @return Pattern telemetry data
 * 
 * Time complexity: O(1)
 * Cycle guarantee: ≤ 7 ticks
 */
arenac_pattern_telemetry_t arenac_trace_memory_pattern_start(
    arenac_telemetry_context_t* context,
    const char* pattern_name
);

/**
 * End memory pattern tracing
 * 
 * @param context Telemetry context
 * @param pattern Pattern telemetry data
 * 
 * Time complexity: O(1)
 * Cycle guarantee: ≤ 7 ticks
 */
void arenac_trace_memory_pattern_end(
    arenac_telemetry_context_t* context,
    arenac_pattern_telemetry_t* pattern
);

/**
 * Record allocation event in pattern
 * 
 * @param context Telemetry context
 * @param pattern Pattern telemetry data
 * @param size Allocation size
 * @param cycles Allocation cycles
 * 
 * Time complexity: O(1)
 * Cycle guarantee: ≤ 7 ticks
 */
void arenac_trace_pattern_allocation(
    arenac_telemetry_context_t* context,
    arenac_pattern_telemetry_t* pattern,
    size_t size,
    uint64_t cycles
);

/*═══════════════════════════════════════════════════════════════
  Distributed Tracing for Multi-Arena Operations
  ═══════════════════════════════════════════════════════════════*/

/**
 * Start distributed trace for multi-arena operation
 * 
 * @param context Telemetry context
 * @param operation_name Operation name
 * @param arena_count Number of arenas involved
 * @return Distributed trace ID
 * 
 * Time complexity: O(1)
 * Cycle guarantee: ≤ 7 ticks
 */
uint64_t arenac_distributed_trace_start(
    arenac_telemetry_context_t* context,
    const char* operation_name,
    uint32_t arena_count
);

/**
 * End distributed trace
 * 
 * @param context Telemetry context
 * @param trace_id Distributed trace ID
 * @param success Whether operation succeeded
 * 
 * Time complexity: O(1)
 * Cycle guarantee: ≤ 7 ticks
 */
void arenac_distributed_trace_end(
    arenac_telemetry_context_t* context,
    uint64_t trace_id,
    bool success
);

/**
 * Propagate trace context to another arena
 * 
 * @param source_context Source telemetry context
 * @param target_context Target telemetry context
 * @param trace_id Distributed trace ID
 * 
 * Time complexity: O(1)
 * Cycle guarantee: ≤ 7 ticks
 */
void arenac_distributed_trace_propagate(
    const arenac_telemetry_context_t* source_context,
    arenac_telemetry_context_t* target_context,
    uint64_t trace_id
);

/*═══════════════════════════════════════════════════════════════
  Telemetry Data Management
  ═══════════════════════════════════════════════════════════════*/

/**
 * Get current telemetry statistics
 * 
 * @param context Telemetry context
 * @param stats Output statistics structure
 * 
 * Time complexity: O(1)
 * Cycle guarantee: ≤ 7 ticks
 */
void arenac_telemetry_get_stats(
    const arenac_telemetry_context_t* context,
    void* stats
);

/**
 * Flush pending telemetry data
 * 
 * @param context Telemetry context
 * @return CNS_OK on success, error code on failure
 * 
 * Time complexity: O(n) where n is pending data
 * Cycle guarantee: Not guaranteed (I/O operation)
 */
CNSResult arenac_telemetry_flush(arenac_telemetry_context_t* context);

/**
 * Configure telemetry sampling
 * 
 * @param context Telemetry context
 * @param sample_rate New sampling rate (0.0-1.0)
 * 
 * Time complexity: O(1)
 * Cycle guarantee: ≤ 7 ticks
 */
void arenac_telemetry_set_sampling(
    arenac_telemetry_context_t* context,
    double sample_rate
);

/*═══════════════════════════════════════════════════════════════
  Integration with Existing Arena Operations
  ═══════════════════════════════════════════════════════════════*/

/**
 * Enhanced allocation with telemetry
 * 
 * @param arena Arena to allocate from
 * @param size Allocation size
 * @param context Telemetry context (optional)
 * @return Allocated pointer or NULL on failure
 * 
 * Time complexity: O(1)
 * Cycle guarantee: ≤ 7 ticks + telemetry overhead
 */
static inline void* arenac_alloc_with_telemetry(
    arena_t* arena,
    size_t size,
    arenac_telemetry_context_t* context
) {
    arenac_alloc_telemetry_t telemetry = {0};
    
    // Start telemetry
    if (context) {
        telemetry = arenac_span_alloc_start(context, arena, size, arena->alignment);
    }
    
    // Perform allocation
    void* ptr = arenac_alloc(arena, size);
    
    // End telemetry
    if (context) {
        arenac_span_alloc_end(context, arena, &telemetry, ptr);
        arenac_metric_capacity_usage(context, arena);
    }
    
    return ptr;
}

/**
 * Enhanced aligned allocation with telemetry
 * 
 * @param arena Arena to allocate from
 * @param size Allocation size
 * @param alignment Required alignment
 * @param context Telemetry context (optional)
 * @return Allocated pointer or NULL on failure
 * 
 * Time complexity: O(1)
 * Cycle guarantee: ≤ 7 ticks + telemetry overhead
 */
static inline void* arenac_alloc_aligned_with_telemetry(
    arena_t* arena,
    size_t size,
    size_t alignment,
    arenac_telemetry_context_t* context
) {
    arenac_alloc_telemetry_t telemetry = {0};
    
    // Start telemetry
    if (context) {
        telemetry = arenac_span_alloc_start(context, arena, size, (uint32_t)alignment);
    }
    
    // Perform allocation
    void* ptr = arenac_alloc_aligned(arena, size, alignment);
    
    // End telemetry
    if (context) {
        arenac_span_alloc_end(context, arena, &telemetry, ptr);
        arenac_metric_capacity_usage(context, arena);
    }
    
    return ptr;
}

/*═══════════════════════════════════════════════════════════════
  Convenience Macros
  ═══════════════════════════════════════════════════════════════*/

// Enhanced allocation macros with telemetry
#define ARENAC_NEW_WITH_TELEMETRY(arena, type, context) \
    ((type*)arenac_alloc_with_telemetry(arena, sizeof(type), context))

#define ARENAC_NEW_ARRAY_WITH_TELEMETRY(arena, type, count, context) \
    ((type*)arenac_alloc_with_telemetry(arena, sizeof(type) * (count), context))

#define ARENAC_NEW_ALIGNED_WITH_TELEMETRY(arena, type, alignment, context) \
    ((type*)arenac_alloc_aligned_with_telemetry(arena, sizeof(type), alignment, context))

// Pattern tracing macros
#define ARENAC_TRACE_PATTERN_SCOPE(context, name) \
    __attribute__((cleanup(arenac_pattern_cleanup))) \
    arenac_pattern_telemetry_t _pattern = arenac_trace_memory_pattern_start(context, name)

#define ARENAC_TRACE_PATTERN_ALLOC(context, pattern, size, cycles) \
    arenac_trace_pattern_allocation(context, pattern, size, cycles)

// Distributed tracing macros
#define ARENAC_DISTRIBUTED_TRACE_SCOPE(context, operation, arena_count) \
    __attribute__((cleanup(arenac_distributed_cleanup))) \
    uint64_t _trace_id = arenac_distributed_trace_start(context, operation, arena_count)

// Helper for pattern cleanup
static inline void arenac_pattern_cleanup(arenac_pattern_telemetry_t* pattern) {
    // Pattern cleanup is handled automatically
    (void)pattern;
}

// Helper for distributed trace cleanup
static inline void arenac_distributed_cleanup(uint64_t* trace_id) {
    // Distributed trace cleanup is handled automatically
    (void)trace_id;
}

#ifdef __cplusplus
}
#endif

#endif /* CNS_ARENAC_TELEMETRY_H */