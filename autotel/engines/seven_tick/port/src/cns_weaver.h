
#ifndef CNS_WEAVER_H
#define CNS_WEAVER_H

#include <stdint.h>
#include <stddef.h>

// Performance tracking structure - always available
typedef struct {
    uint64_t start_cycles;
    uint64_t end_cycles;
    void* span;
} cns_perf_tracker_t;

// Cycle counting functions - always available
#ifdef __aarch64__
    #define CNS_GET_CYCLES() __builtin_readcyclecounter()
#else
    #define CNS_GET_CYCLES() (uint64_t)clock()
#endif

// Base weaver functions - always available (no OpenTelemetry)
static inline cns_perf_tracker_t cns_perf_start_spqlAsk(void) {
    cns_perf_tracker_t tracker = {0};
    tracker.start_cycles = CNS_GET_CYCLES();
    tracker.span = NULL; // No OpenTelemetry span by default
    return tracker;
}

static inline void cns_perf_end_spqlAsk(cns_perf_tracker_t* tracker) {
    if (tracker) {
        tracker->end_cycles = CNS_GET_CYCLES();
        // No OpenTelemetry operations by default
    }
}

static inline cns_perf_tracker_t cns_perf_start_spqlSelect(void) {
    cns_perf_tracker_t tracker = {0};
    tracker.start_cycles = CNS_GET_CYCLES();
    tracker.span = NULL;
    return tracker;
}

static inline void cns_perf_end_spqlSelect(cns_perf_tracker_t* tracker) {
    if (tracker) {
        tracker->end_cycles = CNS_GET_CYCLES();
    }
}

static inline cns_perf_tracker_t cns_perf_start_spqlConstruct(void) {
    cns_perf_tracker_t tracker = {0};
    tracker.start_cycles = CNS_GET_CYCLES();
    tracker.span = NULL;
    return tracker;
}

static inline void cns_perf_end_spqlConstruct(cns_perf_tracker_t* tracker) {
    if (tracker) {
        tracker->end_cycles = CNS_GET_CYCLES();
    }
}

static inline cns_perf_tracker_t cns_perf_start_spqlDescribe(void) {
    cns_perf_tracker_t tracker = {0};
    tracker.start_cycles = CNS_GET_CYCLES();
    tracker.span = NULL;
    return tracker;
}

static inline void cns_perf_end_spqlDescribe(cns_perf_tracker_t* tracker) {
    if (tracker) {
        tracker->end_cycles = CNS_GET_CYCLES();
    }
}

static inline cns_perf_tracker_t cns_perf_start_spqlUpdate(void) {
    cns_perf_tracker_t tracker = {0};
    tracker.start_cycles = CNS_GET_CYCLES();
    tracker.span = NULL;
    return tracker;
}

static inline void cns_perf_end_spqlUpdate(cns_perf_tracker_t* tracker) {
    if (tracker) {
        tracker->end_cycles = CNS_GET_CYCLES();
    }
}

// Optional OpenTelemetry integration - only when CNS_USE_OPENTELEMETRY is defined
#ifdef CNS_USE_OPENTELEMETRY

// OpenTelemetry includes - only included when explicitly requested
#include <opentelemetry/trace/tracer.h>
#include <opentelemetry/trace/span.h>
#include <opentelemetry/trace/provider.h>

namespace otel = opentelemetry;
namespace otel_trace = opentelemetry::trace;

// OpenTelemetry-enabled versions - these will be used instead of the base functions
// when CNS_USE_OPENTELEMETRY is defined, due to C++ function overloading rules

static inline cns_perf_tracker_t cns_perf_start_spqlAsk_otel(void) {
    cns_perf_tracker_t tracker = {0};
    tracker.start_cycles = CNS_GET_CYCLES();
    
    auto provider = otel_trace::Provider::GetTracerProvider();
    auto tracer = provider->GetTracer("cns-weaver");
    tracker.span = tracer->StartSpan("spqlAsk").get();
    
    return tracker;
}

static inline void cns_perf_end_spqlAsk_otel(cns_perf_tracker_t* tracker) {
    if (tracker && tracker->span) {
        tracker->end_cycles = CNS_GET_CYCLES();
        uint64_t cycles = tracker->end_cycles - tracker->start_cycles;
        
        auto span = static_cast<otel_trace::Span*>(tracker->span);
        span->SetAttribute("cycles", cycles);
        span->End();
    }
}

static inline cns_perf_tracker_t cns_perf_start_spqlSelect_otel(void) {
    cns_perf_tracker_t tracker = {0};
    tracker.start_cycles = CNS_GET_CYCLES();
    
    auto provider = otel_trace::Provider::GetTracerProvider();
    auto tracer = provider->GetTracer("cns-weaver");
    tracker.span = tracer->StartSpan("spqlSelect").get();
    
    return tracker;
}

static inline void cns_perf_end_spqlSelect_otel(cns_perf_tracker_t* tracker) {
    if (tracker && tracker->span) {
        tracker->end_cycles = CNS_GET_CYCLES();
        uint64_t cycles = tracker->end_cycles - tracker->start_cycles;
        
        auto span = static_cast<otel_trace::Span*>(tracker->span);
        span->SetAttribute("cycles", cycles);
        span->End();
    }
}

static inline cns_perf_tracker_t cns_perf_start_spqlConstruct_otel(void) {
    cns_perf_tracker_t tracker = {0};
    tracker.start_cycles = CNS_GET_CYCLES();
    
    auto provider = otel_trace::Provider::GetTracerProvider();
    auto tracer = provider->GetTracer("cns-weaver");
    tracker.span = tracer->StartSpan("spqlConstruct").get();
    
    return tracker;
}

static inline void cns_perf_end_spqlConstruct_otel(cns_perf_tracker_t* tracker) {
    if (tracker && tracker->span) {
        tracker->end_cycles = CNS_GET_CYCLES();
        uint64_t cycles = tracker->end_cycles - tracker->start_cycles;
        
        auto span = static_cast<otel_trace::Span*>(tracker->span);
        span->SetAttribute("cycles", cycles);
        span->End();
    }
}

static inline cns_perf_tracker_t cns_perf_start_spqlDescribe_otel(void) {
    cns_perf_tracker_t tracker = {0};
    tracker.start_cycles = CNS_GET_CYCLES();
    
    auto provider = otel_trace::Provider::GetTracerProvider();
    auto tracer = provider->GetTracer("cns-weaver");
    tracker.span = tracer->StartSpan("spqlDescribe").get();
    
    return tracker;
}

static inline void cns_perf_end_spqlDescribe_otel(cns_perf_tracker_t* tracker) {
    if (tracker && tracker->span) {
        tracker->end_cycles = CNS_GET_CYCLES();
        uint64_t cycles = tracker->end_cycles - tracker->start_cycles;
        
        auto span = static_cast<otel_trace::Span*>(tracker->span);
        span->SetAttribute("cycles", cycles);
        span->End();
    }
}

static inline cns_perf_tracker_t cns_perf_start_spqlUpdate_otel(void) {
    cns_perf_tracker_t tracker = {0};
    tracker.start_cycles = CNS_GET_CYCLES();
    
    auto provider = otel_trace::Provider::GetTracerProvider();
    auto tracer = provider->GetTracer("cns-weaver");
    tracker.span = tracer->StartSpan("spqlUpdate").get();
    
    return tracker;
}

static inline void cns_perf_end_spqlUpdate_otel(cns_perf_tracker_t* tracker) {
    if (tracker && tracker->span) {
        tracker->end_cycles = CNS_GET_CYCLES();
        uint64_t cycles = tracker->end_cycles - tracker->start_cycles;
        
        auto span = static_cast<otel_trace::Span*>(tracker->span);
        span->SetAttribute("cycles", cycles);
        span->End();
    }
}

// Macro aliases to use OpenTelemetry versions when available
#define cns_perf_start_spqlAsk cns_perf_start_spqlAsk_otel
#define cns_perf_end_spqlAsk cns_perf_end_spqlAsk_otel
#define cns_perf_start_spqlSelect cns_perf_start_spqlSelect_otel
#define cns_perf_end_spqlSelect cns_perf_end_spqlSelect_otel
#define cns_perf_start_spqlConstruct cns_perf_start_spqlConstruct_otel
#define cns_perf_end_spqlConstruct cns_perf_end_spqlConstruct_otel
#define cns_perf_start_spqlDescribe cns_perf_start_spqlDescribe_otel
#define cns_perf_end_spqlDescribe cns_perf_end_spqlDescribe_otel
#define cns_perf_start_spqlUpdate cns_perf_start_spqlUpdate_otel
#define cns_perf_end_spqlUpdate cns_perf_end_spqlUpdate_otel

#endif // CNS_USE_OPENTELEMETRY

#endif // CNS_WEAVER_H
