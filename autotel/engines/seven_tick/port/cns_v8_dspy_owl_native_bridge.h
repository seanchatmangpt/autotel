/**
 * CNS v8 Native DSPy-OWL Bridge
 * Implements "DSPy signatures ARE OWL entities" with 7-tick guarantees
 * 
 * Key Innovation: DSPy signatures stored as native OWL entities in C memory
 * Performance: Zero-copy integration, <7 tick validation, real-time reasoning
 */

#ifndef CNS_V8_DSPY_OWL_NATIVE_BRIDGE_H
#define CNS_V8_DSPY_OWL_NATIVE_BRIDGE_H

#include "cns_v8_fully_automatic_turtle_loop.h"
#include "include/cns/owl.h"
#include "include/cns/shacl.h"

// Native OWL representation of DSPy signatures (memory-optimized)
typedef struct {
    uint32_t signature_id;          // Unique identifier
    uint16_t owl_class_hash;        // Hash of OWL class URI
    uint8_t field_count;            // Total input + output fields
    uint8_t input_count;            // Number of input fields
    uint8_t output_count;           // Number of output fields
    uint32_t shacl_constraints;     // Bitmask of SHACL shape requirements
    cns_cycle_t validation_cycles;  // Measured validation time
    float confidence_score;         // ML-predicted effectiveness
} native_dspy_owl_signature_t;

// OWL field representation (compact for 7-tick processing)
typedef struct {
    uint16_t property_hash;         // OWL property URI hash
    uint8_t owl_type;              // datatype: 0=string, 1=int, 2=float, etc.
    uint8_t shacl_constraints;     // minCount, maxCount, pattern, etc.
    uint16_t name_hash;            // Field name hash for quick lookup
    uint16_t description_hash;     // Optional description hash
} native_owl_field_t;

// Real-time SHACL validation state (memory-aligned)
typedef struct {
    uint32_t shape_id;             // SHACL shape identifier  
    uint8_t active_constraints;    // Currently active constraint count
    uint8_t violation_count;       // Current violations
    uint16_t validation_bitmap;    // Which constraints are enabled
    cns_cycle_t last_validation;   // When last validated
    float effectiveness_score;     // How effective this shape is
} native_shacl_state_t;

// Complete native DSPy-OWL entity (cache-aligned)
typedef struct {
    native_dspy_owl_signature_t signature;
    native_owl_field_t fields[16];        // Max 16 fields for 7-tick guarantee
    native_shacl_state_t shacl_state;
    
    // Performance tracking
    struct {
        uint64_t validations_performed;
        uint64_t violations_detected;
        uint64_t cycles_total;
        float avg_validation_time;
    } metrics;
    
    // Automatic adaptation state
    struct {
        uint32_t pattern_recognition_bitmap;
        float adaptation_rate;
        cns_cycle_t last_pattern_update;
        uint8_t auto_discovery_enabled;
    } adaptation;
    
} __attribute__((aligned(64))) native_dspy_owl_entity_t;

// Native DSPy-OWL bridge context (full system state)
typedef struct {
    // Entity registry
    native_dspy_owl_entity_t entities[256];  // Max 256 signatures
    uint8_t entity_count;
    uint32_t entity_bitmap;                   // Which slots are occupied
    
    // Real-time OWL reasoning engine
    struct {
        uint64_t inference_rules[64];         // Bitmask-encoded rules
        uint32_t derived_triples[1024];       // Hash-based triple cache
        uint16_t reasoning_cycles;            // Cycles budget for reasoning
        uint8_t reasoning_enabled;
    } owl_reasoner;
    
    // SHACL validation engine  
    struct {
        uint32_t active_shapes[128];          // Active SHACL shapes
        uint16_t constraint_matrix[256][16];  // Constraint applicability
        uint8_t validation_enabled;
        cns_cycle_t max_validation_cycles;    // 7-tick budget
    } shacl_validator;
    
    // Automatic discovery system
    struct {
        uint32_t pattern_frequencies[32];     // 80/20 pattern tracking
        float discovery_threshold;            // Confidence needed for auto-creation
        uint8_t discovery_enabled;
        cns_cycle_t discovery_interval;
    } auto_discovery;
    
    // Integration with CNS v8 automation
    cns_v8_automatic_turtle_loop_t* parent_loop;
    
} cns_v8_dspy_owl_bridge_t;

// === Core API Functions ===

// Initialize native DSPy-OWL bridge with existing turtle loop
int cns_v8_dspy_owl_bridge_init(
    cns_v8_dspy_owl_bridge_t* bridge,
    cns_v8_automatic_turtle_loop_t* parent_loop
);

// Convert Python DSPy signature to native OWL entity (real-time)
int dspy_signature_to_native_owl(
    const char* python_signature_json,    // From Python layer
    native_dspy_owl_entity_t* entity,     // Output entity
    cns_cycle_t max_cycles                // 7-tick budget
);

// Real-time SHACL validation (guaranteed <7 ticks)
bool validate_dspy_output_realtime(
    const native_dspy_owl_entity_t* entity,
    const char* dspy_output,
    cns_cycle_t max_cycles,
    char* violation_report,               // Optional error details
    size_t report_size
);

// Automatic signature discovery from turtle stream patterns
int discover_signatures_from_patterns(
    cns_v8_dspy_owl_bridge_t* bridge,
    const triple_pattern_t* patterns,
    size_t pattern_count,
    float confidence_threshold
);

// Real-time OWL reasoning (7-tick constraint)
int perform_owl_reasoning_realtime(
    cns_v8_dspy_owl_bridge_t* bridge,
    const triple_t* input_triples,
    size_t input_count,
    triple_t* inferred_triples,           // Output buffer
    size_t* inferred_count,
    cns_cycle_t max_cycles
);

// === Automatic Adaptation Functions ===

// Evolve SHACL constraints based on validation effectiveness
void evolve_shacl_constraints(
    native_dspy_owl_entity_t* entity,
    float effectiveness_threshold
);

// Update signature based on ML pattern recognition
void adapt_signature_from_ml(
    native_dspy_owl_entity_t* entity,
    const ml_pattern_prediction_t* prediction
);

// Optimize entity based on 80/20 usage patterns
void optimize_entity_80_20(
    native_dspy_owl_entity_t* entity,
    const usage_statistics_t* stats
);

// === Integration with Existing Systems ===

// Hook into CNS v8 error recovery system
void register_dspy_owl_error_handlers(
    cns_v8_dspy_owl_bridge_t* bridge
);

// Export metrics to CNS v8 telemetry system
void export_dspy_owl_metrics(
    const cns_v8_dspy_owl_bridge_t* bridge,
    cns_v8_automatic_metrics_t* metrics
);

// Memory management (aligned with CNS v8 arena allocator)
void* dspy_owl_arena_alloc(size_t size, size_t alignment);
void dspy_owl_arena_free(void* ptr);

// === Performance Monitoring ===

typedef struct {
    // Signature processing metrics
    uint64_t signatures_processed;
    uint64_t owl_translations_performed;
    uint64_t shacl_validations_completed;
    uint64_t owl_inferences_generated;
    
    // Performance timing
    cns_cycle_t avg_translation_cycles;
    cns_cycle_t avg_validation_cycles;
    cns_cycle_t avg_reasoning_cycles;
    float pareto_efficiency;
    
    // Automation effectiveness
    uint32_t auto_discovered_signatures;
    uint32_t adapted_constraints;
    float adaptation_success_rate;
    
} dspy_owl_bridge_metrics_t;

void get_dspy_owl_bridge_metrics(
    const cns_v8_dspy_owl_bridge_t* bridge,
    dspy_owl_bridge_metrics_t* metrics
);

// === Advanced Features ===

// Signature composition from SHACL dependencies  
int compose_signatures_from_shacl(
    const native_shacl_state_t* shapes,
    size_t shape_count,
    native_dspy_owl_entity_t* composed_signature
);

// Batch processing for high-throughput scenarios
int process_signature_batch(
    cns_v8_dspy_owl_bridge_t* bridge,
    const char** dspy_outputs,
    size_t output_count,
    bool* validation_results,
    cns_cycle_t max_total_cycles
);

// Export OWL ontology for external tools
int export_native_owl_ontology(
    const cns_v8_dspy_owl_bridge_t* bridge,
    char* turtle_output,
    size_t buffer_size
);

// Cleanup and shutdown
void cns_v8_dspy_owl_bridge_cleanup(
    cns_v8_dspy_owl_bridge_t* bridge
);

// Configuration presets
cns_v8_dspy_owl_bridge_t* create_default_dspy_owl_bridge(void);
cns_v8_dspy_owl_bridge_t* create_high_performance_bridge(void);
cns_v8_dspy_owl_bridge_t* create_development_bridge(void);

#endif // CNS_V8_DSPY_OWL_NATIVE_BRIDGE_H