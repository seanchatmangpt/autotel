
#ifndef CNS_OPTIONAL_OTEL_H
#define CNS_OPTIONAL_OTEL_H

#include <stdint.h>

// Performance tracking structure
typedef struct {
    uint64_t start_cycles;
    uint64_t end_cycles;
    void* span;
} cns_perf_tracker_t;

// Optional OpenTelemetry integration
#ifdef CNS_USE_OPENTELEMETRY
#include <opentelemetry/trace/tracer.h>
#include <opentelemetry/trace/span.h>
#include <opentelemetry/trace/provider.h>

namespace otel = opentelemetry;
namespace otel_trace = opentelemetry::trace;

// OpenTelemetry span creation
static inline cns_perf_tracker_t cns_perf_start_spqlAsk() {
    cns_perf_tracker_t tracker = {0};
    tracker.start_cycles = __builtin_readcyclecounter();
    
    auto provider = otel_trace::Provider::GetTracerProvider();
    auto tracer = provider->GetTracer("cns-weaver");
    tracker.span = tracer->StartSpan("spqlAsk").get();
    
    return tracker;
}

static inline void cns_perf_end_spqlAsk(cns_perf_tracker_t* tracker) {
    if (tracker && tracker->span) {
        tracker->end_cycles = __builtin_readcyclecounter();
        uint64_t cycles = tracker->end_cycles - tracker->start_cycles;
        
        auto span = static_cast<otel_trace::Span*>(tracker->span);
        span->SetAttribute("cycles", cycles);
        span->End();
    }
}

#else
// Minimal overhead implementation without OpenTelemetry
static inline cns_perf_tracker_t cns_perf_start_spqlAsk() {
    cns_perf_tracker_t tracker = {0};
    tracker.start_cycles = __builtin_readcyclecounter();
    tracker.span = nullptr; // No OpenTelemetry span
    return tracker;
}

static inline void cns_perf_end_spqlAsk(cns_perf_tracker_t* tracker) {
    if (tracker) {
        tracker->end_cycles = __builtin_readcyclecounter();
        // No OpenTelemetry operations
    }
}
#endif

// Additional span types follow the same pattern
static inline cns_perf_tracker_t cns_perf_start_spqlSelect() {
    cns_perf_tracker_t tracker = {0};
    tracker.start_cycles = __builtin_readcyclecounter();
    
    #ifdef CNS_USE_OPENTELEMETRY
    auto provider = otel_trace::Provider::GetTracerProvider();
    auto tracer = provider->GetTracer("cns-weaver");
    tracker.span = tracer->StartSpan("spqlSelect").get();
    #else
    tracker.span = nullptr;
    #endif
    
    return tracker;
}

static inline void cns_perf_end_spqlSelect(cns_perf_tracker_t* tracker) {
    if (tracker) {
        tracker->end_cycles = __builtin_readcyclecounter();
        
        #ifdef CNS_USE_OPENTELEMETRY
        if (tracker->span) {
            uint64_t cycles = tracker->end_cycles - tracker->start_cycles;
            auto span = static_cast<otel_trace::Span*>(tracker->span);
            span->SetAttribute("cycles", cycles);
            span->End();
        }
        #endif
    }
}

#endif // CNS_OPTIONAL_OTEL_H
