/*
 * TURTLE LOOP ARCHITECTURE - Automated RDF/Turtle Processing Pipeline
 * 
 * Implements the 80/20 principle for semantic data processing:
 * - 20% of components (core loop, validation, reasoning) deliver 80% functionality
 * - Continuous processing pipeline with DSPy integration points
 * - Zero-copy architecture for maximum performance
 * 
 * Key Features:
 * 1. Pareto-optimized component selection
 * 2. Continuous turtle data ingestion and processing
 * 3. Integrated SHACL validation and OWL reasoning
 * 4. DSPy hooks for intelligent processing
 * 5. 7-tick performance guarantees
 */

#ifndef CNS_TURTLE_LOOP_ARCHITECTURE_H
#define CNS_TURTLE_LOOP_ARCHITECTURE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "cns/types.h"
#include "cns/shacl.h"
#include "cns/owl.h"
#include "cns/sparql.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// CORE ARCHITECTURE TYPES (20% that delivers 80%)
// ============================================================================

// Processing stage identifiers
typedef enum {
    TLA_STAGE_PARSE    = 0x01,  // Turtle parsing
    TLA_STAGE_VALIDATE = 0x02,  // SHACL validation
    TLA_STAGE_REASON   = 0x04,  // OWL reasoning
    TLA_STAGE_QUERY    = 0x08,  // SPARQL query
    TLA_STAGE_DSPY     = 0x10,  // DSPy processing
    TLA_STAGE_OUTPUT   = 0x20,  // Result output
    TLA_STAGE_ALL      = 0x3F   // All stages
} tla_stage_t;

// Loop processing statistics
typedef struct {
    uint64_t triples_processed;
    uint64_t validation_errors;
    uint64_t inferences_made;
    uint64_t queries_executed;
    uint64_t dspy_invocations;
    uint64_t total_cycles;
    uint64_t stage_cycles[6];
} tla_stats_t;

// DSPy integration hook
typedef struct {
    const char* model_name;
    void* (*process_fn)(void* data, size_t size, void* context);
    void* context;
    uint32_t flags;
} tla_dspy_hook_t;

// Main loop configuration
typedef struct {
    // Core components (20%)
    uint32_t enabled_stages;
    size_t batch_size;
    size_t max_memory;
    
    // Performance settings
    uint32_t target_ticks;      // 7 or 8 tick target
    bool enable_simd;
    bool enable_prefetch;
    
    // Processing hooks
    tla_dspy_hook_t* dspy_hooks;
    size_t dspy_hook_count;
    
    // Validation rules
    const char* shacl_shapes_path;
    const char* owl_ontology_path;
    
    // Output configuration
    void (*output_handler)(void* result, void* context);
    void* output_context;
} tla_config_t;

// Processing pipeline state
typedef struct {
    // Current state
    tla_stage_t current_stage;
    uint64_t iteration;
    
    // Buffers (zero-copy design)
    void* input_buffer;
    void* working_buffer;
    void* output_buffer;
    size_t buffer_sizes[3];
    
    // Components
    void* parser;
    cns_shacl_validator_t* validator;
    void* reasoner;
    SPARQLEngine* query_engine;
    
    // Statistics
    tla_stats_t stats;
    
    // Configuration
    tla_config_t config;
} tla_pipeline_t;

// ============================================================================
// CORE API FUNCTIONS (80% functionality)
// ============================================================================

// Pipeline lifecycle
tla_pipeline_t* tla_pipeline_create(const tla_config_t* config);
void tla_pipeline_destroy(tla_pipeline_t* pipeline);

// Main processing loop
int tla_process_stream(tla_pipeline_t* pipeline, 
                      int input_fd, 
                      int output_fd);

// Batch processing
int tla_process_batch(tla_pipeline_t* pipeline,
                     const void* input_data,
                     size_t input_size,
                     void** output_data,
                     size_t* output_size);

// Single iteration (for testing/debugging)
int tla_process_iteration(tla_pipeline_t* pipeline);

// ============================================================================
// PARETO-OPTIMIZED COMPONENT FUNCTIONS
// ============================================================================

// Core parsing (handles 80% of Turtle syntax)
int tla_parse_turtle_80_20(const char* input, 
                           size_t size,
                           void* triple_buffer,
                           size_t* triple_count);

// Core validation (80% of SHACL shapes)
int tla_validate_shacl_80_20(void* triple_buffer,
                             size_t triple_count,
                             cns_shacl_validator_t* validator,
                             uint32_t* error_mask);

// Core reasoning (80% of OWL constructs)
int tla_reason_owl_80_20(void* triple_buffer,
                        size_t* triple_count,
                        void* reasoner,
                        uint32_t inference_depth);

// ============================================================================
// DSPY INTEGRATION
// ============================================================================

// Register DSPy processing hook
int tla_register_dspy_hook(tla_pipeline_t* pipeline,
                          const char* stage_name,
                          tla_dspy_hook_t* hook);

// Execute DSPy processing
int tla_execute_dspy(tla_pipeline_t* pipeline,
                    void* data,
                    size_t size,
                    void** result,
                    size_t* result_size);

// ============================================================================
// MONITORING AND CONTROL
// ============================================================================

// Get current statistics
const tla_stats_t* tla_get_stats(const tla_pipeline_t* pipeline);

// Control loop execution
int tla_pause_pipeline(tla_pipeline_t* pipeline);
int tla_resume_pipeline(tla_pipeline_t* pipeline);
int tla_set_stage_enabled(tla_pipeline_t* pipeline, 
                         tla_stage_t stage, 
                         bool enabled);

// Performance tuning
int tla_tune_for_latency(tla_pipeline_t* pipeline);
int tla_tune_for_throughput(tla_pipeline_t* pipeline);

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

// Configuration helpers
tla_config_t tla_config_default(void);
tla_config_t tla_config_minimal(void);  // 20% components only
tla_config_t tla_config_full(void);     // All components

// Stage name lookup
const char* tla_stage_name(tla_stage_t stage);

// Error handling
const char* tla_error_string(int error_code);

#ifdef __cplusplus
}
#endif

#endif // CNS_TURTLE_LOOP_ARCHITECTURE_H