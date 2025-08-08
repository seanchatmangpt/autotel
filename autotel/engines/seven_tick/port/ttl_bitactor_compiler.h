/**
 * @file ttl_bitactor_compiler.h
 * @brief TTL→BitActor Compiler - Specification=Execution through 7-tick compilation
 * 
 * This implements the core principle: "THE WHOLE POINT OF THE SYSTEM IS TO USE TTL 
 * TO DEFINE THE WHOLE SYSTEM" with compile-time 7-tick constraint.
 */

#ifndef TTL_BITACTOR_COMPILER_H
#define TTL_BITACTOR_COMPILER_H

#include <stdint.h>
#include <stdbool.h>
#include "bitactor.h"
#include "meta_probe.h"

// === COMPILE-TIME CONSTANTS ===
#define TTL_COMPILE_7TICK_LIMIT     7        // Maximum ticks for TTL→bytecode compilation
#define TTL_MAX_TRIPLES            1024      // Maximum triples per specification
#define TTL_MAX_BEHAVIORS          256       // Maximum behaviors per actor
#define TTL_MAX_ENTANGLEMENTS      128       // Maximum entanglements
#define TTL_BYTECODE_SIZE          256       // Fixed bytecode size per behavior

// === TTL TRIPLE REPRESENTATION ===
typedef struct {
    const char* subject;
    const char* predicate;
    const char* object;
    enum { TTL_URI, TTL_LITERAL, TTL_BLANK } object_type;
} ttl_triple_t;

// === COMPILED BEHAVIOR ===
typedef struct {
    uint64_t spec_hash;                      // TTL specification hash
    uint8_t bytecode[TTL_BYTECODE_SIZE];     // Compiled BitActor bytecode
    uint32_t bytecode_len;                   // Actual bytecode length
    uint8_t tick_cost;                       // Compile-time tick cost analysis
    const char* label;                       // rdfs:label from TTL
} ttl_compiled_behavior_t;

// === COMPILED ENTANGLEMENT ===
typedef struct {
    uint32_t source_actor;                   // Source actor ID
    uint32_t target_actor;                   // Target actor ID  
    uint8_t trigger_mask;                    // Entanglement trigger mask
    uint8_t max_hops;                        // Maximum propagation hops
    bool is_dark_triple;                     // Dark triple (dormant until activated)
} ttl_compiled_entanglement_t;

// === COMPILED ACTOR ===
typedef struct {
    uint32_t actor_id;                       // Unique actor ID
    const char* label;                       // rdfs:label from TTL
    ttl_compiled_behavior_t* behaviors;      // Array of compiled behaviors
    uint32_t behavior_count;                 // Number of behaviors
    ttl_compiled_entanglement_t* entanglements; // Array of entanglements
    uint32_t entanglement_count;             // Number of entanglements
    uint8_t max_cycles;                      // Maximum allowed cycles (from TTL)
} ttl_compiled_actor_t;

// === COMPILATION CONTEXT ===
typedef struct {
    // TTL parsing state
    ttl_triple_t* triples;
    uint32_t triple_count;
    
    // Compilation output
    ttl_compiled_actor_t* actors;
    uint32_t actor_count;
    
    // Compile-time metrics
    uint64_t compile_start_cycles;
    uint64_t compile_end_cycles;
    uint8_t compile_ticks_used;
    
    // BitActor matrix for runtime
    bitactor_matrix_t* target_matrix;
    
    // Meta-probe for compile-time telemetry
    struct meta_probe_span* compile_span;
} ttl_compiler_context_t;

// === CORE API ===

/**
 * Create a new TTL→BitActor compiler context
 */
ttl_compiler_context_t* ttl_bitactor_compiler_create(bitactor_matrix_t* matrix);

/**
 * Parse TTL specification into triples (must complete in <7 ticks)
 */
bool ttl_bitactor_parse(ttl_compiler_context_t* ctx, const char* ttl_spec);

/**
 * Compile TTL triples into BitActor bytecode (must complete in <7 ticks)
 * This is where specification=execution happens through causal collapse
 */
bool ttl_bitactor_compile(ttl_compiler_context_t* ctx);

/**
 * Deploy compiled actors to BitActor matrix
 */
bool ttl_bitactor_deploy(ttl_compiler_context_t* ctx);

/**
 * Execute deployed actors (runtime, not compile-time constrained)
 */
uint64_t ttl_bitactor_execute(ttl_compiler_context_t* ctx, const void* message);

/**
 * Get compilation metrics
 */
void ttl_bitactor_get_metrics(ttl_compiler_context_t* ctx, 
                             uint8_t* compile_ticks,
                             uint32_t* actors_created,
                             uint32_t* behaviors_compiled,
                             uint64_t* total_bytecode_size);

/**
 * Destroy compiler context
 */
void ttl_bitactor_compiler_destroy(ttl_compiler_context_t* ctx);

// === BYTECODE GENERATION ===

/**
 * Generate BitActor bytecode from behavior specification
 * Must complete within remaining tick budget
 */
bool ttl_generate_behavior_bytecode(const ttl_triple_t* behavior_triples,
                                   uint32_t triple_count,
                                   ttl_compiled_behavior_t* output,
                                   uint8_t tick_budget);

/**
 * Generate entanglement configuration from TTL
 */
bool ttl_generate_entanglement(const ttl_triple_t* entangle_triples,
                              uint32_t triple_count,
                              ttl_compiled_entanglement_t* output);

// === DARK 80/20 OPTIMIZATION ===

/**
 * Apply Dark 80/20 pattern to activate dormant triples
 * This enables 95% ontology utilization through pattern recognition
 */
bool ttl_apply_dark_80_20(ttl_compiler_context_t* ctx);

/**
 * Check if specification equals execution
 * Validates that TTL directly compiles to executable code
 */
bool ttl_validate_specification_equals_execution(ttl_compiler_context_t* ctx);

#endif // TTL_BITACTOR_COMPILER_H