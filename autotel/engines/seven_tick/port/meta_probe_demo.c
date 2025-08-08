/**
 * @file meta_probe_demo.c
 * @brief Simple implementations of meta_probe functions for demo purposes
 */

#include "meta_probe.h"
#include <stdio.h>
#include <string.h>

// Demo function declarations
void meta_probe_demo_init(void);
void meta_probe_demo_cleanup(void);
meta_probe_span_t* meta_probe_begin_span(uint8_t layer, const char* name, uint64_t spec_hash);
void meta_probe_end_span(meta_probe_span_t* span);

// Global collector for demo
static meta_probe_collector_t g_demo_collector = {0};
static bool g_initialized = false;

// Simple init function for demo
void meta_probe_demo_init(void) {
    if (g_initialized) return;
    
    meta_probe_init(&g_demo_collector, "BitActor-Demo");
    g_initialized = true;
}

// Simple cleanup for demo  
void meta_probe_demo_cleanup(void) {
    if (!g_initialized) return;
    
    meta_probe_cleanup(&g_demo_collector);
    g_initialized = false;
}

// Simple span creation for demo
meta_probe_span_t* meta_probe_begin_span(uint8_t layer, const char* name, uint64_t spec_hash) {
    static meta_probe_span_t demo_span = {0};
    
    demo_span.spec_hash = spec_hash;
    demo_span.tick_start = meta_probe_rdtsc();
    
    // Store layer info in unused field for demo
    demo_span.fiber_id = layer;
    
    return &demo_span;
}

// Simple span end for demo
void meta_probe_end_span(meta_probe_span_t* span) {
    if (!span) return;
    
    span->tick_end = meta_probe_rdtsc();
    span->elapsed_cycles = span->tick_end - span->tick_start;
    span->spec_exec_validated = true; // Assume valid for demo
}

// Layer constant for demo
const uint8_t META_PROBE_L5_SPECIFICATION = 5;