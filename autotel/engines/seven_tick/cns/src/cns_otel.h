// AUTO-GENERATED — OpenTelemetry stubs for CNS
// Generated from: docs/ontology/cns-core.ttl
// Extracted: 2025-07-19T00:00:00Z

#ifndef CNS_OTEL_H
#define CNS_OTEL_H

#include <opentelemetry/c/trace.h>
#include <opentelemetry/c/context.h>
#include <stdint.h>

// ============================================================================
// CNS OpenTelemetry Initialization
// ============================================================================

static inline void cns_otel_init(void) {
    // Initialize OpenTelemetry SDK
    opentelemetry_sdk_init();
}

static inline void cns_otel_shutdown(void) {
    // Shutdown OpenTelemetry SDK
    opentelemetry_sdk_shutdown();
}

// ============================================================================
// Span Management Functions
// ============================================================================

// SPARQL Ask Span
static inline opentelemetry_span_t* cns_otel_start_spqlAsk() {
    opentelemetry_context_t ctx = opentelemetry_context_get_current();
    return opentelemetry_tracer_start_span(
        "spqlAskSpan", 
        NULL, 
        0,
        ctx
    );
}

static inline void cns_otel_end_spqlAsk(opentelemetry_span_t* span) {
    if (span) {
        opentelemetry_span_end(span);
    }
}

static inline void cns_otel_set_attribute_spqlAsk(
    opentelemetry_span_t* span, 
    const char* key, 
    const char* value) {
    if (span) {
        opentelemetry_span_set_attribute_string(span, key, value);
    }
}

static inline void cns_otel_set_attribute_spqlAsk_int(
    opentelemetry_span_t* span, 
    const char* key, 
    int64_t value) {
    if (span) {
        opentelemetry_span_set_attribute_int(span, key, value);
    }
}

// SPARQL Select Span
static inline opentelemetry_span_t* cns_otel_start_spqlSelect() {
    opentelemetry_context_t ctx = opentelemetry_context_get_current();
    return opentelemetry_tracer_start_span(
        "spqlSelectSpan", 
        NULL, 
        0,
        ctx
    );
}

static inline void cns_otel_end_spqlSelect(opentelemetry_span_t* span) {
    if (span) {
        opentelemetry_span_end(span);
    }
}

static inline void cns_otel_set_attribute_spqlSelect(
    opentelemetry_span_t* span, 
    const char* key, 
    const char* value) {
    if (span) {
        opentelemetry_span_set_attribute_string(span, key, value);
    }
}

static inline void cns_otel_set_attribute_spqlSelect_int(
    opentelemetry_span_t* span, 
    const char* key, 
    int64_t value) {
    if (span) {
        opentelemetry_span_set_attribute_int(span, key, value);
    }
}

// SPARQL Add Span
static inline opentelemetry_span_t* cns_otel_start_spqlAdd() {
    opentelemetry_context_t ctx = opentelemetry_context_get_current();
    return opentelemetry_tracer_start_span(
        "spqlAddSpan", 
        NULL, 
        0,
        ctx
    );
}

static inline void cns_otel_end_spqlAdd(opentelemetry_span_t* span) {
    if (span) {
        opentelemetry_span_end(span);
    }
}

static inline void cns_otel_set_attribute_spqlAdd(
    opentelemetry_span_t* span, 
    const char* key, 
    const char* value) {
    if (span) {
        opentelemetry_span_set_attribute_string(span, key, value);
    }
}

static inline void cns_otel_set_attribute_spqlAdd_int(
    opentelemetry_span_t* span, 
    const char* key, 
    int64_t value) {
    if (span) {
        opentelemetry_span_set_attribute_int(span, key, value);
    }
}

// Jinja Exec Span
static inline opentelemetry_span_t* cns_otel_start_jinjaExec() {
    opentelemetry_context_t ctx = opentelemetry_context_get_current();
    return opentelemetry_tracer_start_span(
        "jinjaExecSpan", 
        NULL, 
        0,
        ctx
    );
}

static inline void cns_otel_end_jinjaExec(opentelemetry_span_t* span) {
    if (span) {
        opentelemetry_span_end(span);
    }
}

static inline void cns_otel_set_attribute_jinjaExec(
    opentelemetry_span_t* span, 
    const char* key, 
    const char* value) {
    if (span) {
        opentelemetry_span_set_attribute_string(span, key, value);
    }
}

static inline void cns_otel_set_attribute_jinjaExec_int(
    opentelemetry_span_t* span, 
    const char* key, 
    int64_t value) {
    if (span) {
        opentelemetry_span_set_attribute_int(span, key, value);
    }
}

// Jinja Compile Span
static inline opentelemetry_span_t* cns_otel_start_jinjaCompile() {
    opentelemetry_context_t ctx = opentelemetry_context_get_current();
    return opentelemetry_tracer_start_span(
        "jinjaCompileSpan", 
        NULL, 
        0,
        ctx
    );
}

static inline void cns_otel_end_jinjaCompile(opentelemetry_span_t* span) {
    if (span) {
        opentelemetry_span_end(span);
    }
}

static inline void cns_otel_set_attribute_jinjaCompile(
    opentelemetry_span_t* span, 
    const char* key, 
    const char* value) {
    if (span) {
        opentelemetry_span_set_attribute_string(span, key, value);
    }
}

static inline void cns_otel_set_attribute_jinjaCompile_int(
    opentelemetry_span_t* span, 
    const char* key, 
    int64_t value) {
    if (span) {
        opentelemetry_span_set_attribute_int(span, key, value);
    }
}

// SHACL Validate Span
static inline opentelemetry_span_t* cns_otel_start_shaclValidate() {
    opentelemetry_context_t ctx = opentelemetry_context_get_current();
    return opentelemetry_tracer_start_span(
        "shaclValidateSpan", 
        NULL, 
        0,
        ctx
    );
}

static inline void cns_otel_end_shaclValidate(opentelemetry_span_t* span) {
    if (span) {
        opentelemetry_span_end(span);
    }
}

static inline void cns_otel_set_attribute_shaclValidate(
    opentelemetry_span_t* span, 
    const char* key, 
    const char* value) {
    if (span) {
        opentelemetry_span_set_attribute_string(span, key, value);
    }
}

static inline void cns_otel_set_attribute_shaclValidate_int(
    opentelemetry_span_t* span, 
    const char* key, 
    int64_t value) {
    if (span) {
        opentelemetry_span_set_attribute_int(span, key, value);
    }
}

// SHACL Load Shape Span
static inline opentelemetry_span_t* cns_otel_start_shaclLoadShape() {
    opentelemetry_context_t ctx = opentelemetry_context_get_current();
    return opentelemetry_tracer_start_span(
        "shaclLoadShapeSpan", 
        NULL, 
        0,
        ctx
    );
}

static inline void cns_otel_end_shaclLoadShape(opentelemetry_span_t* span) {
    if (span) {
        opentelemetry_span_end(span);
    }
}

static inline void cns_otel_set_attribute_shaclLoadShape(
    opentelemetry_span_t* span, 
    const char* key, 
    const char* value) {
    if (span) {
        opentelemetry_span_set_attribute_string(span, key, value);
    }
}

static inline void cns_otel_set_attribute_shaclLoadShape_int(
    opentelemetry_span_t* span, 
    const char* key, 
    int64_t value) {
    if (span) {
        opentelemetry_span_set_attribute_int(span, key, value);
    }
}

// Slab Allocate Span
static inline opentelemetry_span_t* cns_otel_start_slabAlloc() {
    opentelemetry_context_t ctx = opentelemetry_context_get_current();
    return opentelemetry_tracer_start_span(
        "slabAllocSpan", 
        NULL, 
        0,
        ctx
    );
}

static inline void cns_otel_end_slabAlloc(opentelemetry_span_t* span) {
    if (span) {
        opentelemetry_span_end(span);
    }
}

static inline void cns_otel_set_attribute_slabAlloc(
    opentelemetry_span_t* span, 
    const char* key, 
    const char* value) {
    if (span) {
        opentelemetry_span_set_attribute_string(span, key, value);
    }
}

static inline void cns_otel_set_attribute_slabAlloc_int(
    opentelemetry_span_t* span, 
    const char* key, 
    int64_t value) {
    if (span) {
        opentelemetry_span_set_attribute_int(span, key, value);
    }
}

// Radix Sort Span
static inline opentelemetry_span_t* cns_otel_start_radixSort() {
    opentelemetry_context_t ctx = opentelemetry_context_get_current();
    return opentelemetry_tracer_start_span(
        "radixSortSpan", 
        NULL, 
        0,
        ctx
    );
}

static inline void cns_otel_end_radixSort(opentelemetry_span_t* span) {
    if (span) {
        opentelemetry_span_end(span);
    }
}

static inline void cns_otel_set_attribute_radixSort(
    opentelemetry_span_t* span, 
    const char* key, 
    const char* value) {
    if (span) {
        opentelemetry_span_set_attribute_string(span, key, value);
    }
}

static inline void cns_otel_set_attribute_radixSort_int(
    opentelemetry_span_t* span, 
    const char* key, 
    int64_t value) {
    if (span) {
        opentelemetry_span_set_attribute_int(span, key, value);
    }
}

// Command Queue Push Span
static inline opentelemetry_span_t* cns_otel_start_cmdqPush() {
    opentelemetry_context_t ctx = opentelemetry_context_get_current();
    return opentelemetry_tracer_start_span(
        "cmdqPushSpan", 
        NULL, 
        0,
        ctx
    );
}

static inline void cns_otel_end_cmdqPush(opentelemetry_span_t* span) {
    if (span) {
        opentelemetry_span_end(span);
    }
}

static inline void cns_otel_set_attribute_cmdqPush(
    opentelemetry_span_t* span, 
    const char* key, 
    const char* value) {
    if (span) {
        opentelemetry_span_set_attribute_string(span, key, value);
    }
}

static inline void cns_otel_set_attribute_cmdqPush_int(
    opentelemetry_span_t* span, 
    const char* key, 
    int64_t value) {
    if (span) {
        opentelemetry_span_set_attribute_int(span, key, value);
    }
}

// Command Queue Pop Span
static inline opentelemetry_span_t* cns_otel_start_cmdqPop() {
    opentelemetry_context_t ctx = opentelemetry_context_get_current();
    return opentelemetry_tracer_start_span(
        "cmdqPopSpan", 
        NULL, 
        0,
        ctx
    );
}

static inline void cns_otel_end_cmdqPop(opentelemetry_span_t* span) {
    if (span) {
        opentelemetry_span_end(span);
    }
}

static inline void cns_otel_set_attribute_cmdqPop(
    opentelemetry_span_t* span, 
    const char* key, 
    const char* value) {
    if (span) {
        opentelemetry_span_set_attribute_string(span, key, value);
    }
}

static inline void cns_otel_set_attribute_cmdqPop_int(
    opentelemetry_span_t* span, 
    const char* key, 
    int64_t value) {
    if (span) {
        opentelemetry_span_set_attribute_int(span, key, value);
    }
}

// String ID Span
static inline opentelemetry_span_t* cns_otel_start_stringID() {
    opentelemetry_context_t ctx = opentelemetry_context_get_current();
    return opentelemetry_tracer_start_span(
        "stringIDSpan", 
        NULL, 
        0,
        ctx
    );
}

static inline void cns_otel_end_stringID(opentelemetry_span_t* span) {
    if (span) {
        opentelemetry_span_end(span);
    }
}

static inline void cns_otel_set_attribute_stringID(
    opentelemetry_span_t* span, 
    const char* key, 
    const char* value) {
    if (span) {
        opentelemetry_span_set_attribute_string(span, key, value);
    }
}

static inline void cns_otel_set_attribute_stringID_int(
    opentelemetry_span_t* span, 
    const char* key, 
    int64_t value) {
    if (span) {
        opentelemetry_span_set_attribute_int(span, key, value);
    }
}

// Roaring Bitset AND Span
static inline opentelemetry_span_t* cns_otel_start_roarAnd() {
    opentelemetry_context_t ctx = opentelemetry_context_get_current();
    return opentelemetry_tracer_start_span(
        "roarAndSpan", 
        NULL, 
        0,
        ctx
    );
}

static inline void cns_otel_end_roarAnd(opentelemetry_span_t* span) {
    if (span) {
        opentelemetry_span_end(span);
    }
}

static inline void cns_otel_set_attribute_roarAnd(
    opentelemetry_span_t* span, 
    const char* key, 
    const char* value) {
    if (span) {
        opentelemetry_span_set_attribute_string(span, key, value);
    }
}

static inline void cns_otel_set_attribute_roarAnd_int(
    opentelemetry_span_t* span, 
    const char* key, 
    int64_t value) {
    if (span) {
        opentelemetry_span_set_attribute_int(span, key, value);
    }
}

// ============================================================================
// Performance Tracking
// ============================================================================

typedef struct {
    uint64_t start_cycles;
    uint64_t end_cycles;
    opentelemetry_span_t* span;
} cns_perf_tracker_t;

static inline cns_perf_tracker_t cns_perf_start_spqlAsk() {
    cns_perf_tracker_t tracker = {0};
    tracker.start_cycles = __builtin_readcyclecounter();
    tracker.span = cns_otel_start_spqlAsk();
    return tracker;
}

static inline void cns_perf_end_spqlAsk(cns_perf_tracker_t* tracker) {
    if (tracker && tracker->span) {
        tracker->end_cycles = __builtin_readcyclecounter();
        uint64_t cycles = tracker->end_cycles - tracker->start_cycles;
        
        // Add cycle count as attribute
        cns_otel_set_attribute_spqlAsk_int(tracker->span, "cycles", cycles);
        
        // Check against Chatman constant (7 cycles)
        if (cycles > 7) {
            cns_otel_set_attribute_spqlAsk(tracker->span, "violation", "true");
        }
        
        cns_otel_end_spqlAsk(tracker->span);
    }
}

static inline cns_perf_tracker_t cns_perf_start_spqlSelect() {
    cns_perf_tracker_t tracker = {0};
    tracker.start_cycles = __builtin_readcyclecounter();
    tracker.span = cns_otel_start_spqlSelect();
    return tracker;
}

static inline void cns_perf_end_spqlSelect(cns_perf_tracker_t* tracker) {
    if (tracker && tracker->span) {
        tracker->end_cycles = __builtin_readcyclecounter();
        uint64_t cycles = tracker->end_cycles - tracker->start_cycles;
        
        // Add cycle count as attribute
        cns_otel_set_attribute_spqlSelect_int(tracker->span, "cycles", cycles);
        
        // Check against Chatman constant (7 cycles)
        if (cycles > 7) {
            cns_otel_set_attribute_spqlSelect(tracker->span, "violation", "true");
        }
        
        cns_otel_end_spqlSelect(tracker->span);
    }
}

static inline cns_perf_tracker_t cns_perf_start_spqlAdd() {
    cns_perf_tracker_t tracker = {0};
    tracker.start_cycles = __builtin_readcyclecounter();
    tracker.span = cns_otel_start_spqlAdd();
    return tracker;
}

static inline void cns_perf_end_spqlAdd(cns_perf_tracker_t* tracker) {
    if (tracker && tracker->span) {
        tracker->end_cycles = __builtin_readcyclecounter();
        uint64_t cycles = tracker->end_cycles - tracker->start_cycles;
        
        // Add cycle count as attribute
        cns_otel_set_attribute_spqlAdd_int(tracker->span, "cycles", cycles);
        
        // Check against Chatman constant (7 cycles)
        if (cycles > 7) {
            cns_otel_set_attribute_spqlAdd(tracker->span, "violation", "true");
        }
        
        cns_otel_end_spqlAdd(tracker->span);
    }
}

static inline cns_perf_tracker_t cns_perf_start_jinjaExec() {
    cns_perf_tracker_t tracker = {0};
    tracker.start_cycles = __builtin_readcyclecounter();
    tracker.span = cns_otel_start_jinjaExec();
    return tracker;
}

static inline void cns_perf_end_jinjaExec(cns_perf_tracker_t* tracker) {
    if (tracker && tracker->span) {
        tracker->end_cycles = __builtin_readcyclecounter();
        uint64_t cycles = tracker->end_cycles - tracker->start_cycles;
        
        // Add cycle count as attribute
        cns_otel_set_attribute_jinjaExec_int(tracker->span, "cycles", cycles);
        
        // Check against Chatman constant (7 cycles)
        if (cycles > 7) {
            cns_otel_set_attribute_jinjaExec(tracker->span, "violation", "true");
        }
        
        cns_otel_end_jinjaExec(tracker->span);
    }
}

static inline cns_perf_tracker_t cns_perf_start_jinjaCompile() {
    cns_perf_tracker_t tracker = {0};
    tracker.start_cycles = __builtin_readcyclecounter();
    tracker.span = cns_otel_start_jinjaCompile();
    return tracker;
}

static inline void cns_perf_end_jinjaCompile(cns_perf_tracker_t* tracker) {
    if (tracker && tracker->span) {
        tracker->end_cycles = __builtin_readcyclecounter();
        uint64_t cycles = tracker->end_cycles - tracker->start_cycles;
        
        // Add cycle count as attribute
        cns_otel_set_attribute_jinjaCompile_int(tracker->span, "cycles", cycles);
        
        // Check against Chatman constant (7 cycles)
        if (cycles > 7) {
            cns_otel_set_attribute_jinjaCompile(tracker->span, "violation", "true");
        }
        
        cns_otel_end_jinjaCompile(tracker->span);
    }
}

static inline cns_perf_tracker_t cns_perf_start_shaclValidate() {
    cns_perf_tracker_t tracker = {0};
    tracker.start_cycles = __builtin_readcyclecounter();
    tracker.span = cns_otel_start_shaclValidate();
    return tracker;
}

static inline void cns_perf_end_shaclValidate(cns_perf_tracker_t* tracker) {
    if (tracker && tracker->span) {
        tracker->end_cycles = __builtin_readcyclecounter();
        uint64_t cycles = tracker->end_cycles - tracker->start_cycles;
        
        // Add cycle count as attribute
        cns_otel_set_attribute_shaclValidate_int(tracker->span, "cycles", cycles);
        
        // Check against Chatman constant (7 cycles)
        if (cycles > 7) {
            cns_otel_set_attribute_shaclValidate(tracker->span, "violation", "true");
        }
        
        cns_otel_end_shaclValidate(tracker->span);
    }
}

static inline cns_perf_tracker_t cns_perf_start_shaclLoadShape() {
    cns_perf_tracker_t tracker = {0};
    tracker.start_cycles = __builtin_readcyclecounter();
    tracker.span = cns_otel_start_shaclLoadShape();
    return tracker;
}

static inline void cns_perf_end_shaclLoadShape(cns_perf_tracker_t* tracker) {
    if (tracker && tracker->span) {
        tracker->end_cycles = __builtin_readcyclecounter();
        uint64_t cycles = tracker->end_cycles - tracker->start_cycles;
        
        // Add cycle count as attribute
        cns_otel_set_attribute_shaclLoadShape_int(tracker->span, "cycles", cycles);
        
        // Check against Chatman constant (7 cycles)
        if (cycles > 7) {
            cns_otel_set_attribute_shaclLoadShape(tracker->span, "violation", "true");
        }
        
        cns_otel_end_shaclLoadShape(tracker->span);
    }
}

static inline cns_perf_tracker_t cns_perf_start_slabAlloc() {
    cns_perf_tracker_t tracker = {0};
    tracker.start_cycles = __builtin_readcyclecounter();
    tracker.span = cns_otel_start_slabAlloc();
    return tracker;
}

static inline void cns_perf_end_slabAlloc(cns_perf_tracker_t* tracker) {
    if (tracker && tracker->span) {
        tracker->end_cycles = __builtin_readcyclecounter();
        uint64_t cycles = tracker->end_cycles - tracker->start_cycles;
        
        // Add cycle count as attribute
        cns_otel_set_attribute_slabAlloc_int(tracker->span, "cycles", cycles);
        
        // Check against Chatman constant (7 cycles)
        if (cycles > 7) {
            cns_otel_set_attribute_slabAlloc(tracker->span, "violation", "true");
        }
        
        cns_otel_end_slabAlloc(tracker->span);
    }
}

static inline cns_perf_tracker_t cns_perf_start_radixSort() {
    cns_perf_tracker_t tracker = {0};
    tracker.start_cycles = __builtin_readcyclecounter();
    tracker.span = cns_otel_start_radixSort();
    return tracker;
}

static inline void cns_perf_end_radixSort(cns_perf_tracker_t* tracker) {
    if (tracker && tracker->span) {
        tracker->end_cycles = __builtin_readcyclecounter();
        uint64_t cycles = tracker->end_cycles - tracker->start_cycles;
        
        // Add cycle count as attribute
        cns_otel_set_attribute_radixSort_int(tracker->span, "cycles", cycles);
        
        // Check against Chatman constant (7 cycles)
        if (cycles > 7) {
            cns_otel_set_attribute_radixSort(tracker->span, "violation", "true");
        }
        
        cns_otel_end_radixSort(tracker->span);
    }
}

static inline cns_perf_tracker_t cns_perf_start_cmdqPush() {
    cns_perf_tracker_t tracker = {0};
    tracker.start_cycles = __builtin_readcyclecounter();
    tracker.span = cns_otel_start_cmdqPush();
    return tracker;
}

static inline void cns_perf_end_cmdqPush(cns_perf_tracker_t* tracker) {
    if (tracker && tracker->span) {
        tracker->end_cycles = __builtin_readcyclecounter();
        uint64_t cycles = tracker->end_cycles - tracker->start_cycles;
        
        // Add cycle count as attribute
        cns_otel_set_attribute_cmdqPush_int(tracker->span, "cycles", cycles);
        
        // Check against Chatman constant (7 cycles)
        if (cycles > 7) {
            cns_otel_set_attribute_cmdqPush(tracker->span, "violation", "true");
        }
        
        cns_otel_end_cmdqPush(tracker->span);
    }
}

static inline cns_perf_tracker_t cns_perf_start_cmdqPop() {
    cns_perf_tracker_t tracker = {0};
    tracker.start_cycles = __builtin_readcyclecounter();
    tracker.span = cns_otel_start_cmdqPop();
    return tracker;
}

static inline void cns_perf_end_cmdqPop(cns_perf_tracker_t* tracker) {
    if (tracker && tracker->span) {
        tracker->end_cycles = __builtin_readcyclecounter();
        uint64_t cycles = tracker->end_cycles - tracker->start_cycles;
        
        // Add cycle count as attribute
        cns_otel_set_attribute_cmdqPop_int(tracker->span, "cycles", cycles);
        
        // Check against Chatman constant (7 cycles)
        if (cycles > 7) {
            cns_otel_set_attribute_cmdqPop(tracker->span, "violation", "true");
        }
        
        cns_otel_end_cmdqPop(tracker->span);
    }
}

static inline cns_perf_tracker_t cns_perf_start_stringID() {
    cns_perf_tracker_t tracker = {0};
    tracker.start_cycles = __builtin_readcyclecounter();
    tracker.span = cns_otel_start_stringID();
    return tracker;
}

static inline void cns_perf_end_stringID(cns_perf_tracker_t* tracker) {
    if (tracker && tracker->span) {
        tracker->end_cycles = __builtin_readcyclecounter();
        uint64_t cycles = tracker->end_cycles - tracker->start_cycles;
        
        // Add cycle count as attribute
        cns_otel_set_attribute_stringID_int(tracker->span, "cycles", cycles);
        
        // Check against Chatman constant (7 cycles)
        if (cycles > 7) {
            cns_otel_set_attribute_stringID(tracker->span, "violation", "true");
        }
        
        cns_otel_end_stringID(tracker->span);
    }
}

static inline cns_perf_tracker_t cns_perf_start_roarAnd() {
    cns_perf_tracker_t tracker = {0};
    tracker.start_cycles = __builtin_readcyclecounter();
    tracker.span = cns_otel_start_roarAnd();
    return tracker;
}

static inline void cns_perf_end_roarAnd(cns_perf_tracker_t* tracker) {
    if (tracker && tracker->span) {
        tracker->end_cycles = __builtin_readcyclecounter();
        uint64_t cycles = tracker->end_cycles - tracker->start_cycles;
        
        // Add cycle count as attribute
        cns_otel_set_attribute_roarAnd_int(tracker->span, "cycles", cycles);
        
        // Check against Chatman constant (7 cycles)
        if (cycles > 7) {
            cns_otel_set_attribute_roarAnd(tracker->span, "violation", "true");
        }
        
        cns_otel_end_roarAnd(tracker->span);
    }
}

// ============================================================================
// Pattern Macros
// ============================================================================

#define CNS_PATTERN_SLABALLOCATOR 1
#define CNS_PATTERN_FASTRADIXBUCKET 1
#define CNS_PATTERN_SPSCCOMMANDQUEUE 1
#define CNS_PATTERN_STRINGIDMAP 1
#define CNS_PATTERN_ROARINGBITSET 1

// ============================================================================
// Function Cycle Costs
// ============================================================================

#define CNS_CYCLE_COST_SPQLASK 3
#define CNS_CYCLE_COST_SPQLSELECT 5
#define CNS_CYCLE_COST_SPQLADD 2
#define CNS_CYCLE_COST_JINJAEXEC 4
#define CNS_CYCLE_COST_JINJACOMPILE 6
#define CNS_CYCLE_COST_SHACLVALIDATE 4
#define CNS_CYCLE_COST_SHACLLOADSHAPE 3
#define CNS_CYCLE_COST_SLABALLOC 2
#define CNS_CYCLE_COST_RADIXSORT 5
#define CNS_CYCLE_COST_CMDQPUSH 3
#define CNS_CYCLE_COST_CMDQPOP 3
#define CNS_CYCLE_COST_STRINGID 1
#define CNS_CYCLE_COST_ROARAND 4

// ============================================================================
// Utility Macros
// ============================================================================

#define CNS_OTEL_SPAN_START(fn) cns_perf_start_##fn()
#define CNS_OTEL_SPAN_END(tracker) cns_perf_end_##fn(&tracker)

#define CNS_OTEL_WRAP(fn, ...) do { \
    cns_perf_tracker_t _tracker = cns_otel_start_##fn(); \
    fn(__VA_ARGS__); \
    cns_otel_end_##fn(&_tracker); \
} while(0)

#endif // CNS_OTEL_H
