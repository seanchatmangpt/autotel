#!/usr/bin/env python3
"""
Simplified CNS Weaver - Demonstrates the concept without cjinja dependency
"""

import json
import sys
import os
from pathlib import Path

def load_json_context(json_path):
    """Load JSON context file."""
    with open(json_path, 'r') as f:
        return json.load(f)

def render_header_template(data, output_path):
    """Render the header template."""
    template = f"""// AUTO-GENERATED — OpenTelemetry stubs for CNS
// Generated from: {data['metadata']['source']}
// Extracted: {data['metadata']['extractedAt']}

#ifndef CNS_OTEL_H
#define CNS_OTEL_H

#include <opentelemetry/c/trace.h>
#include <opentelemetry/c/context.h>
#include <stdint.h>

// ============================================================================
// CNS OpenTelemetry Initialization
// ============================================================================

static inline void cns_otel_init(void) {{
    // Initialize OpenTelemetry SDK
    opentelemetry_sdk_init();
}}

static inline void cns_otel_shutdown(void) {{
    // Shutdown OpenTelemetry SDK
    opentelemetry_sdk_shutdown();
}}

// ============================================================================
// Span Management Functions
// ============================================================================

"""
    
    # Add span functions
    for span in data['spans']:
        func_name = span['functionName']
        span_name = span['spanName']
        label = span['label']
        
        template += f"""// {label}
static inline opentelemetry_span_t* cns_otel_start_{func_name}() {{
    opentelemetry_context_t ctx = opentelemetry_context_get_current();
    return opentelemetry_tracer_start_span(
        "{span_name}", 
        NULL, 
        0,
        ctx
    );
}}

static inline void cns_otel_end_{func_name}(opentelemetry_span_t* span) {{
    if (span) {{
        opentelemetry_span_end(span);
    }}
}}

static inline void cns_otel_set_attribute_{func_name}(
    opentelemetry_span_t* span, 
    const char* key, 
    const char* value) {{
    if (span) {{
        opentelemetry_span_set_attribute_string(span, key, value);
    }}
}}

static inline void cns_otel_set_attribute_{func_name}_int(
    opentelemetry_span_t* span, 
    const char* key, 
    int64_t value) {{
    if (span) {{
        opentelemetry_span_set_attribute_int(span, key, value);
    }}
}}

"""
    
    # Add performance tracking
    template += """// ============================================================================
// Performance Tracking
// ============================================================================

typedef struct {
    uint64_t start_cycles;
    uint64_t end_cycles;
    opentelemetry_span_t* span;
} cns_perf_tracker_t;

"""
    
    for span in data['spans']:
        func_name = span['functionName']
        template += f"""static inline cns_perf_tracker_t cns_perf_start_{func_name}() {{
    cns_perf_tracker_t tracker = {{0}};
    tracker.start_cycles = __builtin_readcyclecounter();
    tracker.span = cns_otel_start_{func_name}();
    return tracker;
}}

static inline void cns_perf_end_{func_name}(cns_perf_tracker_t* tracker) {{
    if (tracker && tracker->span) {{
        tracker->end_cycles = __builtin_readcyclecounter();
        uint64_t cycles = tracker->end_cycles - tracker->start_cycles;
        
        // Add cycle count as attribute
        cns_otel_set_attribute_{func_name}_int(tracker->span, "cycles", cycles);
        
        // Check against Chatman constant (7 cycles)
        if (cycles > 7) {{
            cns_otel_set_attribute_{func_name}(tracker->span, "violation", "true");
        }}
        
        cns_otel_end_{func_name}(tracker->span);
    }}
}}

"""
    
    # Add pattern macros
    template += """// ============================================================================
// Pattern Macros
// ============================================================================

"""
    
    for pattern in data['patterns']:
        pattern_name = pattern['patternName'].upper()
        template += f"#define CNS_PATTERN_{pattern_name} 1\n"
    
    # Add function cycle costs
    template += """
// ============================================================================
// Function Cycle Costs
// ============================================================================

"""
    
    for func in data['functions']:
        func_name = func['functionName'].upper()
        cycle_cost = func['cycleCost']
        template += f"#define CNS_CYCLE_COST_{func_name} {cycle_cost}\n"
    
    # Add utility macros
    template += """
// ============================================================================
// Utility Macros
// ============================================================================

#define CNS_OTEL_SPAN_START(fn) cns_perf_start_##fn()
#define CNS_OTEL_SPAN_END(tracker) cns_perf_end_##fn(&tracker)

#define CNS_OTEL_WRAP(fn, ...) do { \\
    cns_perf_tracker_t _tracker = cns_otel_start_##fn(); \\
    fn(__VA_ARGS__); \\
    cns_otel_end_##fn(&_tracker); \\
} while(0)

#endif // CNS_OTEL_H
"""
    
    # Write to file
    with open(output_path, 'w') as f:
        f.write(template)
    
    print(f"✅ Generated header: {output_path}")

def render_inject_template(data, output_path, inject_target_header="cns.h"):
    """Render the inject template."""
    template = f"""// AUTO-INJECTED OTEL instrumentation for CNS
// Generated from: {data['metadata']['source']}
// Extracted: {data['metadata']['extractedAt']}

#include "cns_otel.h"
#include "{inject_target_header}"

// ============================================================================
// Instrumented Function Wrappers
// ============================================================================

"""
    
    # Add instrumented function wrappers
    for span in data['spans']:
        func_name = span['functionName']
        if func_name in data['functionSignatures']:
            signature = data['functionSignatures'][func_name]
            template += f"""// Instrumented wrapper for {func_name}
{signature} {{
    cns_perf_tracker_t _tracker = cns_otel_start_{func_name}();
    
    // ─── original {func_name} body ─────────────────
    #include "{func_name}_body.inc"
    // ───────────────────────────────────────────────────────────
    
    cns_otel_end_{func_name}(&_tracker);
    return _result; // Assuming return value
}}

"""
    
    # Add performance monitoring
    template += """// ============================================================================
// Performance Monitoring Functions
// ============================================================================

typedef struct {
    uint64_t total_cycles;
    uint64_t operation_count;
    uint64_t violation_count;
    double mean_cycles;
    double std_cycles;
} cns_perf_stats_t;

static cns_perf_stats_t g_perf_stats = {0};

static inline void cns_perf_update_stats(uint64_t cycles) {
    g_perf_stats.total_cycles += cycles;
    g_perf_stats.operation_count++;
    
    if (cycles > 7) {
        g_perf_stats.violation_count++;
    }
    
    // Update running statistics
    double delta = cycles - g_perf_stats.mean_cycles;
    g_perf_stats.mean_cycles += delta / g_perf_stats.operation_count;
    
    if (g_perf_stats.operation_count > 1) {
        double delta2 = cycles - g_perf_stats.mean_cycles;
        g_perf_stats.std_cycles = sqrt(
            (g_perf_stats.std_cycles * g_perf_stats.std_cycles * (g_perf_stats.operation_count - 2) + delta * delta2) / 
            (g_perf_stats.operation_count - 1)
        );
    }
}

static inline cns_perf_stats_t* cns_perf_get_stats(void) {
    return &g_perf_stats;
}

static inline void cns_perf_reset_stats(void) {
    memset(&g_perf_stats, 0, sizeof(g_perf_stats));
}

// ============================================================================
// Gatekeeper Integration
// ============================================================================

static inline int cns_gatekeeper_check_cycles(uint64_t cycles) {
    return cycles <= 7; // Chatman constant
}

static inline double cns_gatekeeper_calculate_sigma(double mean, double std) {
    if (std == 0.0) return 0.0;
    return (7.0 - mean) / std; // USL = 7 cycles
}

static inline int cns_gatekeeper_validate_performance(void) {
    double sigma = cns_gatekeeper_calculate_sigma(g_perf_stats.mean_cycles, g_perf_stats.std_cycles);
    double violation_rate = (double)g_perf_stats.violation_count / g_perf_stats.operation_count;
    
    // Check against Six Sigma requirements
    return (sigma >= 4.0) && (violation_rate <= 0.000063); // 63 ppm
}

// ============================================================================
// Pattern Implementation Stubs
// ============================================================================

"""
    
    # Add pattern stubs
    for pattern in data['patterns']:
        pattern_name = pattern['patternName']
        label = pattern['label']
        comment = pattern['comment']
        
        template += f"""// {label}
// {comment}
#ifdef CNS_PATTERN_{pattern_name.upper()}
// Pattern implementation would go here
// This is a placeholder for the actual pattern implementation
#endif

"""
    
    # Add initialization
    template += """// ============================================================================
// Initialization and Cleanup
// ============================================================================

static inline int cns_otel_inject_init(void) {
    cns_otel_init();
    cns_perf_reset_stats();
    return 0;
}

static inline void cns_otel_inject_cleanup(void) {
    cns_otel_shutdown();
}
"""
    
    # Write to file
    with open(output_path, 'w') as f:
        f.write(template)
    
    print(f"✅ Generated inject: {output_path}")

def main():
    """Main function."""
    if len(sys.argv) < 5:
        print("Usage: weaver_simple.py <ctx.json> <header_tmpl> <inject_tmpl> <out_dir> [inject_target_header]")
        print("\nExample:")
        print("  weaver_simple.py build/cns_spans.json templates/otel_header.h.j2 templates/otel_inject.c.j2 src cns.h")
        sys.exit(1)
    
    ctx_path = sys.argv[1]
    hdr_tmpl = sys.argv[2]  # Not used in simplified version
    inj_tmpl = sys.argv[3]  # Not used in simplified version
    out_dir = sys.argv[4]
    inject_target_header = sys.argv[5] if len(sys.argv) > 5 else "cns.h"
    
    print("🔧 CNS Weaver (Simplified) - OpenTelemetry Code Generation")
    print("==========================================================")
    print(f"Context: {ctx_path}")
    print(f"Output directory: {out_dir}")
    print(f"Target header: {inject_target_header}")
    print()
    
    # Ensure output directory exists
    Path(out_dir).mkdir(parents=True, exist_ok=True)
    
    # Load JSON context
    try:
        data = load_json_context(ctx_path)
    except Exception as e:
        print(f"Error loading JSON file {ctx_path}: {e}")
        sys.exit(1)
    
    # Generate header file
    out_hdr = f"{out_dir}/cns_otel.h"
    render_header_template(data, out_hdr)
    
    # Generate injected C file
    out_c = f"{out_dir}/cns_otel_inject.c"
    render_inject_template(data, out_c, inject_target_header)
    
    print(f"\n✅ Code generation completed successfully!")
    print(f"Generated files:")
    print(f"  - {out_hdr}")
    print(f"  - {out_c}")
    
    # Print summary
    print(f"\n📊 Summary:")
    print(f"  Spans: {len(data['spans'])}")
    print(f"  Functions: {len(data['functions'])}")
    print(f"  Patterns: {len(data['patterns'])}")
    
    print(f"\n🔧 Sample spans:")
    for span in data['spans'][:3]:
        print(f"  - {span['spanName']} -> {span['functionName']}")
    
    print(f"\n⚡ Sample functions:")
    for func in data['functions'][:3]:
        print(f"  - {func['functionName']} ({func['cycleCost']} cycles)")

if __name__ == "__main__":
    main() 