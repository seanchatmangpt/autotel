/**
 * @file meta_probe.h
 * BitActor L4: Meta-Probe & Telemetry Layer
 * 
 * Provides OTEL-style spans, cycle measurement, and causal bounds validation
 * for the BitActor execution stack. Ensures specification=execution integrity
 * through hash delta monitoring and entropy measurement.
 */

#ifndef META_PROBE_H
#define META_PROBE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Trinity constraints for L4 operations
#define META_PROBE_8T_MAX_CYCLES 8
#define META_PROBE_8H_HOP_LIMIT 8
#define META_PROBE_8M_QUANTUM 8

// Causal bounds thresholds
#define CAUSAL_HASH_DELTA_THRESHOLD 0x1000
#define ENTROPY_THRESHOLD_MAX 0xFF
#define FIBER_BUDGET_DEFAULT_CYCLES 49  // 7 ticks @ 7GHz

// Forward declarations
typedef struct bitactor_fiber_t {
    uint64_t fiber_id; // Unique ID for the fiber
    void* state_ptr;   // Pointer to the fiber's state
    // Add other relevant fiber context here
} bitactor_fiber_t;
typedef struct conductor_manifest_t conductor_manifest_t;

/**
 * Span context for OTEL-style telemetry tracking
 * Cache-aligned for performance in L4 hot path
 */
typedef struct __attribute__((aligned(64))) {
    uint64_t span_id;              // Unique span identifier
    uint64_t trace_id;             // Parent trace identifier
    uint64_t parent_span_id;       // Parent span for nesting
    
    // Timing measurements (Trinity-compliant)
    uint64_t tick_start;           // rdtsc at span start
    uint64_t tick_end;             // rdtsc at span end
    uint64_t elapsed_cycles;       // Computed duration
    
    // Causal validation
    uint64_t spec_hash;            // Expected specification hash
    uint64_t exec_hash;            // Actual execution hash
    uint64_t hash_delta;           // |spec_hash ⊕ exec_hash|
    
    // Entropy measurement
    uint8_t pre_collapse_entropy;  // Entropy before causal collapse
    uint8_t post_collapse_entropy; // Entropy after causal collapse
    uint8_t entropy_delta;         // Change in entropy
    
    // Fiber execution context
    uint32_t fiber_id;             // Associated fiber ID
    uint16_t opcode_count;         // Number of opcodes executed
    uint8_t register_state[8];     // Final register state (R0-R7)
    
    // Validation flags
    bool within_causal_bounds;     // Hash delta < threshold
    bool entropy_valid;            // Entropy change acceptable
    bool tick_budget_met;          // Execution within time budget
    bool spec_exec_validated;      // Specification=execution validated
    
} meta_probe_span_t;

/**
 * Telemetry collector for aggregating spans
 * Manages circular buffer of active spans
 */
typedef struct {
    // Span storage (circular buffer)
    meta_probe_span_t spans[256] __attribute__((aligned(64)));
    uint8_t span_head;             // Current write position
    uint8_t span_tail;             // Current read position
    uint8_t active_span_count;     // Number of active spans
    
    // Global metrics
    struct {
        uint64_t total_spans_created;
        uint64_t spans_within_bounds;
        uint64_t spans_over_budget;
        uint64_t causal_violations;
        uint64_t entropy_violations;
        uint64_t genesis_resets_triggered;
    } metrics;
    
    // Performance thresholds
    struct {
        uint64_t max_allowed_cycles;    // Cycle budget per span
        uint64_t hash_delta_threshold;  // Causal bounds threshold
        uint8_t entropy_threshold;      // Max entropy change
        bool fault_recovery_enabled;   // Enable automatic recovery
    } thresholds;
    
    // OpenTelemetry integration
    struct {
        bool otel_enabled;             // Export to OTEL
        void* otel_exporter;           // OTEL exporter instance
        char service_name[64];         // Service name for traces
        char version[16];              // BitActor version
    } otel_config;
    
} meta_probe_collector_t;

/**
 * Fiber execution measurement context
 * Tracks performance during fiber execution
 */
typedef struct {
    uint64_t execution_start_cycle; // When fiber started
    uint64_t cycle_budget;          // Maximum allowed cycles
    uint32_t opcodes_executed;      // Count of opcodes processed
    uint8_t initial_entropy;        // System entropy at start
    bool measurement_active;        // Currently measuring
} fiber_measurement_t;

// === Core API Functions ===

/**
 * Initialize the meta-probe telemetry system
 * @param collector Telemetry collector to initialize
 * @param service_name Name for OTEL service identification
 * @return 0 on success, -1 on failure
 */
int meta_probe_init(meta_probe_collector_t* collector, const char* service_name);

/**
 * Start a new telemetry span for fiber execution
 * @param collector Telemetry collector
 * @param fiber Associated fiber being executed
 * @param spec_hash Expected specification hash
 * @return Span ID, or 0 on failure
 */
uint64_t meta_probe_span_start(
    meta_probe_collector_t* collector,
    const bitactor_fiber_t* fiber,
    uint64_t spec_hash
);

/**
 * End a telemetry span and validate causal bounds
 * @param collector Telemetry collector
 * @param span_id Span to end
 * @param exec_hash Actual execution hash
 * @return 0 if valid, -1 if causal violation detected
 */
int meta_probe_span_end(
    meta_probe_collector_t* collector,
    uint64_t span_id,
    uint64_t exec_hash
);

/**
 * Measure fiber execution performance in real-time
 * @param measurement Measurement context
 * @param fiber Fiber being executed
 * @param cycle_budget Maximum allowed cycles
 * @return 0 on success, -1 on failure
 */
int meta_probe_measure_fiber_start(
    fiber_measurement_t* measurement,
    const bitactor_fiber_t* fiber,
    uint64_t cycle_budget
);

/**
 * Complete fiber measurement and validate performance
 * @param measurement Measurement context
 * @param collector Telemetry collector for reporting
 * @return 0 if within bounds, -1 if violation detected
 */
int meta_probe_measure_fiber_end(
    fiber_measurement_t* measurement,
    meta_probe_collector_t* collector
);

// === Causal Validation Functions ===

/**
 * Validate that execution matches specification
 * @param spec_hash Expected specification hash
 * @param exec_hash Actual execution hash
 * @return true if within causal bounds, false otherwise
 */
bool meta_probe_validate_causal_bounds(uint64_t spec_hash, uint64_t exec_hash);

/**
 * Measure entropy change during causal collapse
 * @param pre_state System state before collapse
 * @param post_state System state after collapse
 * @return Entropy delta value
 */
uint8_t meta_probe_measure_entropy_delta(
    const uint8_t pre_state[8],
    const uint8_t post_state[8]
);

/**
 * Check if execution is within Trinity constraints
 * @param elapsed_cycles Actual execution cycles
 * @param opcode_count Number of opcodes executed
 * @return true if Trinity-compliant, false otherwise
 */
bool meta_probe_validate_trinity_compliance(
    uint64_t elapsed_cycles,
    uint16_t opcode_count
);

// === Fault Handling Functions ===

/**
 * Handle causal violation by triggering Genesis reset
 * @param collector Telemetry collector
 * @param violation_type Type of violation detected
 * @param context Violation context information
 * @return 0 on successful recovery, -1 on failure
 */
int meta_probe_handle_causal_violation(
    meta_probe_collector_t* collector,
    uint32_t violation_type,
    const void* context
);

/**
 * Trigger Genesis reset for system recovery
 * @param reason Reset reason code
 * @param preserve_state Whether to preserve actor state
 * @return 0 on success, -1 on failure
 */
int meta_probe_trigger_genesis_reset(uint32_t reason, bool preserve_state);

// === OpenTelemetry Integration ===

/**
 * Configure OpenTelemetry exporter
 * @param collector Telemetry collector
 * @param endpoint OTEL endpoint URL
 * @param headers Additional headers for export
 * @return 0 on success, -1 on failure
 */
int meta_probe_configure_otel(
    meta_probe_collector_t* collector,
    const char* endpoint,
    const char** headers
);

/**
 * Export spans to OpenTelemetry endpoint
 * @param collector Telemetry collector
 * @param max_spans Maximum spans to export in batch
 * @return Number of spans exported, -1 on failure
 */
int meta_probe_export_otel_spans(
    meta_probe_collector_t* collector,
    size_t max_spans
);

// === Performance Monitoring ===

/**
 * Get telemetry performance metrics
 * @param collector Telemetry collector
 * @param metrics Output metrics structure
 */
void meta_probe_get_metrics(
    const meta_probe_collector_t* collector,
    struct meta_probe_metrics* metrics
);

/**
 * Reset performance counters
 * @param collector Telemetry collector
 */
void meta_probe_reset_metrics(meta_probe_collector_t* collector);

// === Utility Functions ===

/**
 * Generate unique span ID
 * @return New unique span identifier
 */
uint64_t meta_probe_generate_span_id(void);

/**
 * Calculate hash delta for causal validation
 * @param spec_hash Specification hash
 * @param exec_hash Execution hash
 * @return Hash delta value
 */
uint64_t meta_probe_calculate_hash_delta(uint64_t spec_hash, uint64_t exec_hash);

/**
 * Format span for human-readable output
 * @param span Span to format
 * @param buffer Output buffer
 * @param buffer_size Size of output buffer
 * @return Number of characters written
 */
int meta_probe_format_span(
    const meta_probe_span_t* span,
    char* buffer,
    size_t buffer_size
);

// === Cleanup ===

/**
 * Cleanup telemetry system and release resources
 * @param collector Telemetry collector to cleanup
 */
void meta_probe_cleanup(meta_probe_collector_t* collector);

// === Inline Performance Helpers ===

/**
 * Fast cycle counter read for timing measurements
 * @return Current CPU cycle count
 */
static inline uint64_t meta_probe_rdtsc(void) {
    uint32_t lo, hi;
    __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

/**
 * Quick entropy calculation for 8-byte state
 * @param state State bytes to analyze
 * @return Entropy estimate (0-255)
 */
static inline uint8_t meta_probe_quick_entropy(const uint8_t state[8]) {
    uint8_t entropy = 0;
    for (int i = 0; i < 8; i++) {
        entropy ^= state[i];
        entropy = (entropy << 1) | (entropy >> 7);  // Rotate left
    }
    return entropy;
}

/**
 * Fast hash XOR for causal validation
 * @param a First hash
 * @param b Second hash
 * @return XOR result
 */
static inline uint64_t meta_probe_hash_xor(uint64_t a, uint64_t b) {
    return a ^ b;
}

// === Constants and Enums ===

// Violation types for fault handling
typedef enum {
    VIOLATION_NONE = 0,
    VIOLATION_CAUSAL_BOUNDS = 1,
    VIOLATION_ENTROPY_EXCEEDED = 2,
    VIOLATION_TICK_BUDGET = 3,
    VIOLATION_TRINITY_8T = 4,
    VIOLATION_TRINITY_8H = 5,
    VIOLATION_TRINITY_8M = 6,
    VIOLATION_SPEC_EXEC_MISMATCH = 7
} meta_probe_violation_t;

// Genesis reset reasons
typedef enum {
    GENESIS_RESET_CAUSAL_VIOLATION = 1,
    GENESIS_RESET_ENTROPY_FAULT = 2,
    GENESIS_RESET_PERFORMANCE_FAULT = 3,
    GENESIS_RESET_MANUAL_TRIGGER = 4,
    GENESIS_RESET_SYSTEM_CORRUPTION = 5
} genesis_reset_reason_t;

// Metrics structure for performance monitoring
struct meta_probe_metrics {
    uint64_t total_spans;
    uint64_t successful_spans;
    uint64_t violated_spans;
    double average_execution_cycles;
    double causal_validation_rate;
    double entropy_stability_rate;
    uint64_t genesis_resets;
    uint64_t otel_exports;
};

#endif // META_PROBE_H