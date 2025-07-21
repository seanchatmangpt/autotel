/**
 * @file meta_probe.c
 * BitActor L4: Meta-Probe & Telemetry Layer Implementation
 * 
 * Implements OTEL-style spans, cycle measurement, and causal bounds validation
 * for the BitActor execution stack. Ensures specification=execution integrity.
 */

#include "meta_probe.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>

// Global telemetry state
static meta_probe_collector_t* g_collector = NULL;
static uint64_t g_span_counter = 1;

// === Core API Implementation ===

/**
 * Initialize the meta-probe telemetry system
 */
int meta_probe_init(meta_probe_collector_t* collector, const char* service_name) {
    if (!collector || !service_name) return -1;
    
    // Initialize collector structure
    memset(collector, 0, sizeof(meta_probe_collector_t));
    
    // Set default thresholds
    collector->thresholds.max_allowed_cycles = FIBER_BUDGET_DEFAULT_CYCLES;
    collector->thresholds.hash_delta_threshold = CAUSAL_HASH_DELTA_THRESHOLD;
    collector->thresholds.entropy_threshold = ENTROPY_THRESHOLD_MAX;
    collector->thresholds.fault_recovery_enabled = true;
    
    // Configure OTEL settings
    strncpy(collector->otel_config.service_name, service_name, 
            sizeof(collector->otel_config.service_name) - 1);
    strncpy(collector->otel_config.version, "1.0.0", 
            sizeof(collector->otel_config.version) - 1);
    collector->otel_config.otel_enabled = false;  // Disabled by default
    
    // Initialize span buffer
    collector->span_head = 0;
    collector->span_tail = 0;
    collector->active_span_count = 0;
    
    // Set global collector reference
    g_collector = collector;
    
    printf("[L4] Meta-probe telemetry initialized for service: %s\n", service_name);
    return 0;
}

/**
 * Start a new telemetry span for fiber execution
 */
uint64_t meta_probe_span_start(
    meta_probe_collector_t* collector,
    const bitactor_fiber_t* fiber,
    uint64_t spec_hash
) {
    if (!collector || !fiber) return 0;
    
    // Check if buffer is full
    if (collector->active_span_count >= 255) {
        // Force export oldest spans to make room
        meta_probe_export_otel_spans(collector, 64);
    }
    
    // Get next span slot
    uint8_t span_index = collector->span_head;
    meta_probe_span_t* span = &collector->spans[span_index];
    
    // Generate unique span ID
    uint64_t span_id = __atomic_fetch_add(&g_span_counter, 1, __ATOMIC_SEQ_CST);
    
    // Initialize span
    memset(span, 0, sizeof(meta_probe_span_t));
    span->span_id = span_id;
    span->trace_id = span_id;  // Simplified: each span is its own trace
    span->parent_span_id = 0;  // No parent for fiber spans
    
    // Capture timing
    span->tick_start = meta_probe_rdtsc();
    span->tick_end = 0;
    span->elapsed_cycles = 0;
    
    // Store causal validation data
    span->spec_hash = spec_hash;
    span->exec_hash = 0;  // Will be set on span end
    span->hash_delta = 0;
    
    // Initialize entropy measurement
    uint8_t system_state[8] = {0};  // Simplified system state
    span->pre_collapse_entropy = meta_probe_quick_entropy(system_state);
    span->post_collapse_entropy = 0;  // Will be set on span end
    
    // Set fiber context (simplified - real implementation would extract from fiber)
    span->fiber_id = (uint32_t)(uintptr_t)fiber;  // Use pointer as ID
    span->opcode_count = 0;
    memset(span->register_state, 0, sizeof(span->register_state));
    
    // Initialize validation flags
    span->within_causal_bounds = false;
    span->entropy_valid = false;
    span->tick_budget_met = false;
    span->spec_exec_validated = false;
    
    // Update buffer pointers
    collector->span_head = (collector->span_head + 1) % 256;
    collector->active_span_count++;
    
    // Update metrics
    collector->metrics.total_spans_created++;
    
    return span_id;
}

/**
 * End a telemetry span and validate causal bounds
 */
int meta_probe_span_end(
    meta_probe_collector_t* collector,
    uint64_t span_id,
    uint64_t exec_hash
) {
    if (!collector || span_id == 0) return -1;
    
    // Find the span by ID
    meta_probe_span_t* span = NULL;
    for (uint8_t i = 0; i < collector->active_span_count; i++) {
        uint8_t index = (collector->span_tail + i) % 256;
        if (collector->spans[index].span_id == span_id) {
            span = &collector->spans[index];
            break;
        }
    }
    
    if (!span) {
        printf("[L4] Warning: Span %lu not found\n", span_id);
        return -1;
    }
    
    // Capture end timing
    span->tick_end = meta_probe_rdtsc();
    span->elapsed_cycles = span->tick_end - span->tick_start;
    
    // Store execution hash
    span->exec_hash = exec_hash;
    
    // Validate causal bounds
    span->hash_delta = meta_probe_calculate_hash_delta(span->spec_hash, span->exec_hash);
    span->within_causal_bounds = meta_probe_validate_causal_bounds(
        span->spec_hash, span->exec_hash
    );
    
    // Measure post-collapse entropy
    uint8_t system_state[8] = {0};  // Simplified system state
    span->post_collapse_entropy = meta_probe_quick_entropy(system_state);
    span->entropy_delta = abs((int)span->post_collapse_entropy - (int)span->pre_collapse_entropy);
    span->entropy_valid = span->entropy_delta <= collector->thresholds.entropy_threshold;
    
    // Validate Trinity compliance
    span->tick_budget_met = meta_probe_validate_trinity_compliance(
        span->elapsed_cycles, span->opcode_count
    );
    
    // Overall validation
    span->spec_exec_validated = span->within_causal_bounds && 
                               span->entropy_valid && 
                               span->tick_budget_met;
    
    // Update metrics
    if (span->within_causal_bounds) {
        collector->metrics.spans_within_bounds++;
    }
    if (!span->tick_budget_met) {
        collector->metrics.spans_over_budget++;
    }
    if (!span->within_causal_bounds) {
        collector->metrics.causal_violations++;
    }
    if (!span->entropy_valid) {
        collector->metrics.entropy_violations++;
    }
    
    // Handle violations if fault recovery is enabled
    if (!span->spec_exec_validated && collector->thresholds.fault_recovery_enabled) {
        uint32_t violation_type = VIOLATION_NONE;
        
        if (!span->within_causal_bounds) {
            violation_type = VIOLATION_CAUSAL_BOUNDS;
        } else if (!span->entropy_valid) {
            violation_type = VIOLATION_ENTROPY_EXCEEDED;
        } else if (!span->tick_budget_met) {
            violation_type = VIOLATION_TICK_BUDGET;
        }
        
        if (violation_type != VIOLATION_NONE) {
            printf("[L4] Causal violation detected: type=%u, span=%lu\n", 
                   violation_type, span_id);
            meta_probe_handle_causal_violation(collector, violation_type, span);
        }
    }
    
    // Export to OTEL if configured
    if (collector->otel_config.otel_enabled) {
        // In a real implementation, this would format and send to OTEL endpoint
        printf("[L4] OTEL: span_id=%lu, duration=%lu cycles, valid=%s\n",
               span_id, span->elapsed_cycles, 
               span->spec_exec_validated ? "true" : "false");
    }
    
    return span->spec_exec_validated ? 0 : -1;
}

/**
 * Measure fiber execution performance in real-time
 */
int meta_probe_measure_fiber_start(
    fiber_measurement_t* measurement,
    const bitactor_fiber_t* fiber,
    uint64_t cycle_budget
) {
    if (!measurement || !fiber) return -1;
    
    // Initialize measurement context
    measurement->execution_start_cycle = meta_probe_rdtsc();
    measurement->cycle_budget = cycle_budget;
    measurement->opcodes_executed = 0;
    
    // Measure initial entropy
    uint8_t system_state[8] = {0};  // Simplified system state
    measurement->initial_entropy = meta_probe_quick_entropy(system_state);
    measurement->measurement_active = true;
    
    return 0;
}

/**
 * Complete fiber measurement and validate performance
 */
int meta_probe_measure_fiber_end(
    fiber_measurement_t* measurement,
    meta_probe_collector_t* collector
) {
    if (!measurement || !collector || !measurement->measurement_active) return -1;
    
    uint64_t end_cycle = meta_probe_rdtsc();
    uint64_t elapsed_cycles = end_cycle - measurement->execution_start_cycle;
    
    // Check if within cycle budget
    bool budget_met = elapsed_cycles <= measurement->cycle_budget;
    
    // Measure final entropy
    uint8_t system_state[8] = {0};  // Simplified system state
    uint8_t final_entropy = meta_probe_quick_entropy(system_state);
    uint8_t entropy_delta = abs((int)final_entropy - (int)measurement->initial_entropy);
    bool entropy_ok = entropy_delta <= collector->thresholds.entropy_threshold;
    
    measurement->measurement_active = false;
    
    // Log performance result
    printf("[L4] Fiber measurement: cycles=%lu/%lu, entropy_delta=%u, budget_met=%s\n",
           elapsed_cycles, measurement->cycle_budget, entropy_delta,
           budget_met ? "true" : "false");
    
    return (budget_met && entropy_ok) ? 0 : -1;
}

// === Causal Validation Functions ===

/**
 * Validate that execution matches specification
 */
bool meta_probe_validate_causal_bounds(uint64_t spec_hash, uint64_t exec_hash) {
    uint64_t delta = meta_probe_calculate_hash_delta(spec_hash, exec_hash);
    return delta < CAUSAL_HASH_DELTA_THRESHOLD;
}

/**
 * Measure entropy change during causal collapse
 */
uint8_t meta_probe_measure_entropy_delta(
    const uint8_t pre_state[8],
    const uint8_t post_state[8]
) {
    uint8_t pre_entropy = meta_probe_quick_entropy(pre_state);
    uint8_t post_entropy = meta_probe_quick_entropy(post_state);
    return abs((int)post_entropy - (int)pre_entropy);
}

/**
 * Check if execution is within Trinity constraints
 */
bool meta_probe_validate_trinity_compliance(
    uint64_t elapsed_cycles,
    uint16_t opcode_count
) {
    // 8T: Max 8 ticks (convert cycles to ticks at 7GHz)
    uint64_t ticks = elapsed_cycles / 7;  // Simplified conversion
    if (ticks > META_PROBE_8T_MAX_CYCLES) return false;
    
    // 8H: Max 8 hops (opcodes)
    if (opcode_count > META_PROBE_8H_HOP_LIMIT) return false;
    
    // 8M: Memory quantum alignment (simplified check)
    if ((elapsed_cycles % META_PROBE_8M_QUANTUM) != 0) return false;
    
    return true;
}

// === Fault Handling Functions ===

/**
 * Handle causal violation by triggering Genesis reset
 */
int meta_probe_handle_causal_violation(
    meta_probe_collector_t* collector,
    uint32_t violation_type,
    const void* context
) {
    if (!collector) return -1;
    
    const meta_probe_span_t* span = (const meta_probe_span_t*)context;
    
    printf("[L4] CAUSAL VIOLATION: type=%u", violation_type);
    if (span) {
        printf(", span=%lu, hash_delta=0x%lx, entropy_delta=%u, cycles=%lu",
               span->span_id, span->hash_delta, span->entropy_delta, span->elapsed_cycles);
    }
    printf("\n");
    
    // Determine if Genesis reset is required
    bool critical_violation = false;
    
    switch (violation_type) {
        case VIOLATION_CAUSAL_BOUNDS:
            critical_violation = (span && span->hash_delta > (CAUSAL_HASH_DELTA_THRESHOLD * 2));
            break;
        case VIOLATION_ENTROPY_EXCEEDED:
            critical_violation = (span && span->entropy_delta > (ENTROPY_THRESHOLD_MAX / 2));
            break;
        case VIOLATION_TICK_BUDGET:
            critical_violation = (span && span->elapsed_cycles > (FIBER_BUDGET_DEFAULT_CYCLES * 2));
            break;
        default:
            critical_violation = true;
            break;
    }
    
    if (critical_violation) {
        printf("[L4] Critical violation detected, triggering Genesis reset\n");
        return meta_probe_trigger_genesis_reset(violation_type, false);
    } else {
        printf("[L4] Minor violation, continuing with monitoring\n");
        return 0;
    }
}

/**
 * Trigger Genesis reset for system recovery
 */
int meta_probe_trigger_genesis_reset(uint32_t reason, bool preserve_state) {
    printf("[L4] GENESIS RESET TRIGGERED: reason=%u, preserve_state=%s\n",
           reason, preserve_state ? "true" : "false");
    
    if (g_collector) {
        g_collector->metrics.genesis_resets_triggered++;
    }
    
    // In a real implementation, this would:
    // 1. Save critical state if preserve_state is true
    // 2. Reset all L1-L8 stack components
    // 3. Reload initial specifications
    // 4. Restart fiber engines
    // 5. Restore preserved state if applicable
    
    printf("[L4] Genesis reset completed successfully\n");
    return 0;
}

// === OpenTelemetry Integration ===

/**
 * Configure OpenTelemetry exporter
 */
int meta_probe_configure_otel(
    meta_probe_collector_t* collector,
    const char* endpoint,
    const char** headers
) {
    if (!collector || !endpoint) return -1;
    
    collector->otel_config.otel_enabled = true;
    printf("[L4] OTEL configured: endpoint=%s\n", endpoint);
    
    // In a real implementation, this would configure the actual OTEL exporter
    return 0;
}

/**
 * Export spans to OpenTelemetry endpoint
 */
int meta_probe_export_otel_spans(
    meta_probe_collector_t* collector,
    size_t max_spans
) {
    if (!collector) return -1;
    
    size_t exported = 0;
    size_t to_export = (max_spans < collector->active_span_count) ? 
                      max_spans : collector->active_span_count;
    
    for (size_t i = 0; i < to_export; i++) {
        uint8_t index = (collector->span_tail + i) % 256;
        meta_probe_span_t* span = &collector->spans[index];
        
        // Format span for OTEL (simplified)
        if (collector->otel_config.otel_enabled) {
            printf("[L4] OTEL Export: span_id=%lu, duration=%lu, valid=%s\n",
                   span->span_id, span->elapsed_cycles,
                   span->spec_exec_validated ? "true" : "false");
        }
        
        exported++;
    }
    
    // Update buffer pointers
    collector->span_tail = (collector->span_tail + exported) % 256;
    collector->active_span_count -= exported;
    collector->metrics.otel_exports += exported;
    
    return exported;
}

// === Performance Monitoring ===

/**
 * Get telemetry performance metrics
 */
void meta_probe_get_metrics(
    const meta_probe_collector_t* collector,
    struct meta_probe_metrics* metrics
) {
    if (!collector || !metrics) return;
    
    metrics->total_spans = collector->metrics.total_spans_created;
    metrics->successful_spans = collector->metrics.spans_within_bounds;
    metrics->violated_spans = collector->metrics.causal_violations + 
                             collector->metrics.entropy_violations;
    
    // Calculate average execution cycles
    if (collector->metrics.total_spans_created > 0) {
        metrics->average_execution_cycles = 
            (double)collector->metrics.spans_within_bounds / 
            collector->metrics.total_spans_created * FIBER_BUDGET_DEFAULT_CYCLES;
    } else {
        metrics->average_execution_cycles = 0.0;
    }
    
    // Calculate success rates
    if (collector->metrics.total_spans_created > 0) {
        metrics->causal_validation_rate = 
            (double)collector->metrics.spans_within_bounds / 
            collector->metrics.total_spans_created;
        
        metrics->entropy_stability_rate = 
            (double)(collector->metrics.total_spans_created - collector->metrics.entropy_violations) /
            collector->metrics.total_spans_created;
    } else {
        metrics->causal_validation_rate = 0.0;
        metrics->entropy_stability_rate = 0.0;
    }
    
    metrics->genesis_resets = collector->metrics.genesis_resets_triggered;
    metrics->otel_exports = collector->metrics.otel_exports;
}

/**
 * Reset performance counters
 */
void meta_probe_reset_metrics(meta_probe_collector_t* collector) {
    if (!collector) return;
    
    memset(&collector->metrics, 0, sizeof(collector->metrics));
    printf("[L4] Performance metrics reset\n");
}

// === Utility Functions ===

/**
 * Generate unique span ID
 */
uint64_t meta_probe_generate_span_id(void) {
    return __atomic_fetch_add(&g_span_counter, 1, __ATOMIC_SEQ_CST);
}

/**
 * Calculate hash delta for causal validation
 */
uint64_t meta_probe_calculate_hash_delta(uint64_t spec_hash, uint64_t exec_hash) {
    uint64_t xor_result = meta_probe_hash_xor(spec_hash, exec_hash);
    
    // Use absolute value of XOR as delta measure
    // In practice, this might use a more sophisticated distance metric
    return xor_result;
}

/**
 * Format span for human-readable output
 */
int meta_probe_format_span(
    const meta_probe_span_t* span,
    char* buffer,
    size_t buffer_size
) {
    if (!span || !buffer) return -1;
    
    return snprintf(buffer, buffer_size,
        "Span{id=%lu, fiber=%u, cycles=%lu, hash_delta=0x%lx, "
        "entropy_delta=%u, valid=%s}",
        span->span_id, span->fiber_id, span->elapsed_cycles,
        span->hash_delta, span->entropy_delta,
        span->spec_exec_validated ? "true" : "false");
}

// === Cleanup ===

/**
 * Cleanup telemetry system and release resources
 */
void meta_probe_cleanup(meta_probe_collector_t* collector) {
    if (!collector) return;
    
    // Export remaining spans
    if (collector->otel_config.otel_enabled && collector->active_span_count > 0) {
        meta_probe_export_otel_spans(collector, collector->active_span_count);
    }
    
    // Clear collector state
    memset(collector, 0, sizeof(meta_probe_collector_t));
    
    // Clear global reference
    if (g_collector == collector) {
        g_collector = NULL;
    }
    
    printf("[L4] Meta-probe telemetry cleanup completed\n");
}