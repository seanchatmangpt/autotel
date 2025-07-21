/**
 * CNS v8 Turtle Loop Integration
 * Combines CNS v8 Trinity Architecture with DSPy-OWL-SHACL for automatic turtle processing
 * Implements 80/20 optimization principle
 */

#ifndef CNS_V8_TURTLE_LOOP_INTEGRATION_H
#define CNS_V8_TURTLE_LOOP_INTEGRATION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Core CNS v8 types
typedef uint64_t cns_cycle_t;
typedef uint64_t cns_id_t;
typedef uint64_t cns_bitmask_t;

// DSPy-OWL types
typedef struct {
    cns_id_t signature_id;
    const char* owl_class;
    const char* instruction;
    cns_bitmask_t input_fields;
    cns_bitmask_t output_fields;
} dspy_signature_t;

// SHACL validation types
typedef struct {
    cns_id_t shape_id;
    const char* target_class;
    cns_bitmask_t constraints;
    bool (*validate)(const void* data);
} shacl_shape_t;

// Turtle Loop stages (80/20 optimized)
typedef enum {
    STAGE_PARSE     = 0x01,  // 20% effort
    STAGE_VALIDATE  = 0x02,  // 20% effort  
    STAGE_TRANSFORM = 0x04,  // Optional
    STAGE_REASON    = 0x08,  // Optional
    STAGE_OPTIMIZE  = 0x10,  // Optional
    STAGE_OUTPUT    = 0x20   // 20% effort
} turtle_stage_t;

// Triple patterns covering 80% of use cases
typedef enum {
    PATTERN_TYPE_DECL = 0x01,  // rdf:type statements (30%)
    PATTERN_LABEL     = 0x02,  // rdfs:label statements (20%)
    PATTERN_PROPERTY  = 0x04,  // Basic properties (20%)
    PATTERN_HIERARCHY = 0x08,  // subClassOf/subPropertyOf (10%)
    PATTERN_OTHER     = 0x10   // Everything else (20%)
} triple_pattern_t;

// Trinity-compliant Turtle Loop
typedef struct {
    // 8T: Temporal contract
    cns_cycle_t max_cycles_per_triple;  // Must be <= 8
    cns_cycle_t current_cycle;
    
    // 8H: Cognitive contract  
    struct {
        void (*observe)(const char* turtle_data);
        void (*analyze)(triple_pattern_t pattern);
        void (*generate)(dspy_signature_t* signature);
        void (*check)(shacl_shape_t* shape);
        void (*construct)(void* output);
        void (*verify)(bool* valid);
        void (*plan)(turtle_stage_t next_stage);
        void (*meta_validate)(void);
    } cognitive_ops;
    
    // 8M: Memory contract
    struct {
        void* arena;          // 8-byte aligned
        size_t arena_size;    // Multiple of 8
        cns_bitmask_t used;   // Track allocations
    } memory;
    
    // DSPy integration
    dspy_signature_t* signatures;
    size_t signature_count;
    
    // SHACL validation
    shacl_shape_t* shapes;
    size_t shape_count;
    
    // 80/20 optimization
    turtle_stage_t enabled_stages;
    triple_pattern_t priority_patterns;
    
    // Statistics
    uint64_t triples_processed;
    uint64_t patterns_matched[5];
} cns_v8_turtle_loop_t;

// Initialize turtle loop with CNS v8 contracts
int cns_v8_turtle_loop_init(
    cns_v8_turtle_loop_t* loop,
    size_t arena_size
);

// Register DSPy signature as OWL class
int cns_v8_register_dspy_signature(
    cns_v8_turtle_loop_t* loop,
    const char* name,
    const char* instruction,
    const char** input_fields,
    size_t input_count,
    const char** output_fields,
    size_t output_count
);

// Add SHACL shape for validation
int cns_v8_add_shacl_shape(
    cns_v8_turtle_loop_t* loop,
    const char* shape_name,
    const char* target_class,
    bool (*validate)(const void*)
);

// Process turtle data through the loop
int cns_v8_process_turtle(
    cns_v8_turtle_loop_t* loop,
    const char* turtle_data,
    size_t data_size,
    void* output_buffer,
    size_t* output_size
);

// Enable/disable stages based on 80/20 analysis
void cns_v8_configure_stages(
    cns_v8_turtle_loop_t* loop,
    turtle_stage_t stages
);

// Get 80/20 performance metrics
typedef struct {
    double pattern_coverage[5];      // % of triples matching each pattern
    double stage_time_percent[6];    // % time in each stage
    double pareto_efficiency;        // How well we achieve 80/20
    cns_cycle_t avg_cycles_per_triple;
} cns_v8_metrics_t;

void cns_v8_get_metrics(
    const cns_v8_turtle_loop_t* loop,
    cns_v8_metrics_t* metrics
);

// Cleanup
void cns_v8_turtle_loop_cleanup(cns_v8_turtle_loop_t* loop);

#endif // CNS_V8_TURTLE_LOOP_INTEGRATION_H