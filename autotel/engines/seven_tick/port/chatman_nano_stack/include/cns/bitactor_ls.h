#ifndef BITACTOR_LS_H
#define BITACTOR_LS_H

#include "bitactor_80_20.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// ---
// BITACTOR-LS: Learning System Extension
// Adds adaptive learning to BitActor while preserving sub-100ns hot path
// Based on 80/20 principle: Learning in setup, execution in hot path
// ---

// L9: Learning Layer Extension (added to L1-L8 stack)
#define BITACTOR_L9_LEARNING_LAYER 9
#define BITACTOR_MAX_PATTERNS 64
#define BITACTOR_LEARNING_WINDOW 1000  // Ticks to analyze

// Learning-enhanced Trinity constraints
#define BITACTOR_LS_8T_MAX_CYCLES 8     // Learning must not break 8T
#define BITACTOR_LS_8H_HOP_COUNT 8      // Learning adds to 8H chain
#define BITACTOR_LS_8M_QUANTUM 8        // Learning uses 8M alignment

// Forward declarations
typedef struct bitactor_ls_pattern_t bitactor_ls_pattern_t;
typedef struct bitactor_ls_core_t bitactor_ls_core_t;
typedef struct bitactor_ls_matrix_t bitactor_ls_matrix_t;
typedef struct learning_metrics_t learning_metrics_t;

// Learning pattern structure (cache-aligned)
struct bitactor_ls_pattern_t {
    // Hot data (accessed during execution)
    uint64_t pattern_hash;              // Pattern identifier
    uint32_t activation_count;          // Usage frequency
    uint16_t confidence_score;          // Learning confidence (0-65535)
    uint8_t validity_flags;             // Pattern validation bits
    uint8_t priority;                   // Execution priority
    
    // Learning metadata (setup/analysis phase)
    uint64_t first_seen;                // Tick when pattern emerged
    uint64_t last_activated;            // Recent usage
    double success_rate;                // Historical performance
    uint32_t adaptation_count;          // How many times adapted
    
    // Pre-compiled pattern data
    uint8_t compiled_logic[32];         // AOT compiled pattern logic
    uint32_t logic_size;                // Size of compiled logic
    
    // Trinity compliance validation
    bool trinity_compliant;             // 8T/8H/8M validation
    uint64_t execution_cycles;          // Pattern execution time
} __attribute__((aligned(64)));

// Learning-enhanced BitActor core
struct bitactor_ls_core_t {
    // Base BitActor (hot path - must remain fast)
    compiled_bitactor_t base_actor;
    
    // Learning hot data (first cache line of learning data)
    uint8_t learning_mode;              // 0=off, 1=observe, 2=adapt
    uint8_t pattern_active_mask;        // Which patterns are active
    uint16_t current_pattern_id;        // Currently executing pattern
    uint32_t learning_tick_count;       // Learning-specific counter
    
    // Pattern storage (cache-aligned)
    bitactor_ls_pattern_t patterns[8];  // Most frequent patterns (hot)
    uint32_t pattern_count;             // Number of learned patterns
    
    // Learning state (cold data)
    uint64_t learning_state_vector;     // Compact learning state
    double adaptation_threshold;        // When to adapt patterns
    uint32_t observation_window;        // Ticks to observe before learning
    
    // Performance validation
    learning_metrics_t* metrics;        // Learning performance data
    bool learning_trinity_compliant;    // Learning preserves Trinity
} __attribute__((aligned(128)));

// Learning metrics for validation
struct learning_metrics_t {
    // Performance tracking
    uint64_t total_learning_cycles;     // Time spent learning
    uint64_t hot_path_cycles;           // Time in execution
    uint64_t pattern_adaptations;       // Successful adaptations
    uint64_t trinity_violations;        // Learning-caused violations
    
    // Learning effectiveness
    double pattern_accuracy;            // Pattern prediction accuracy
    double adaptation_success_rate;     // Successful adaptation rate
    uint32_t patterns_discovered;       // New patterns found
    uint32_t patterns_discarded;        // Obsolete patterns removed
    
    // System health
    bool sub_100ns_preserved;           // Hot path still sub-100ns
    bool learning_overhead_acceptable;  // Learning cost < 5% of execution
    uint64_t memory_usage_bytes;        // Learning memory overhead
};

// Learning-enhanced matrix
struct bitactor_ls_matrix_t {
    // Base matrix (preserves hot path performance)
    bitactor_matrix_t base_matrix;
    
    // Learning coordination
    bitactor_ls_core_t learning_actors[256];  // Learning-enhanced actors
    uint32_t learning_actor_count;
    uint64_t global_learning_tick;
    
    // Pattern sharing infrastructure
    bitactor_ls_pattern_t shared_patterns[BITACTOR_MAX_PATTERNS];
    uint32_t shared_pattern_count;
    uint64_t pattern_sharing_mask;
    
    // Learning system metrics
    learning_metrics_t global_metrics;
    
    // AOT learning compilation cache
    void* compiled_learning_cache;      // Pre-compiled learning logic
    size_t cache_size;
    bool cache_valid;
} __attribute__((aligned(4096)));

// ---
// Core Learning Functions (Hot Path - Must be Sub-100ns)
// ---

// THE CRITICAL LEARNING FUNCTION - Must preserve sub-100ns
void bitactor_ls_execute_hot_path(bitactor_ls_core_t* ls_actor);

// Pattern activation (hot path)
bool bitactor_ls_activate_pattern(
    bitactor_ls_core_t* ls_actor, 
    uint16_t pattern_id
);

// Learning state update (hot path)
void bitactor_ls_update_state(
    bitactor_ls_core_t* ls_actor,
    bitactor_signal_t signal
);

// Matrix tick with learning (critical performance path)
uint32_t bitactor_ls_matrix_tick(
    bitactor_ls_matrix_t* ls_matrix,
    bitactor_signal_t* signals,
    uint32_t signal_count
);

// ---
// Learning Setup Functions (Setup Phase - Can be Slower)
// ---

// Initialize learning system
bitactor_ls_matrix_t* bitactor_ls_matrix_create(void);
void bitactor_ls_matrix_destroy(bitactor_ls_matrix_t* ls_matrix);

// Add learning-enhanced actor
uint32_t bitactor_ls_add_actor(
    bitactor_ls_matrix_t* ls_matrix,
    bitactor_meaning_t meaning,
    bitactor_manifest_t* manifest
);

// Pattern management
bool bitactor_ls_add_pattern(
    bitactor_ls_core_t* ls_actor,
    const void* pattern_data,
    size_t data_size,
    double initial_confidence
);

bool bitactor_ls_remove_pattern(
    bitactor_ls_core_t* ls_actor,
    uint16_t pattern_id
);

// Learning configuration
void bitactor_ls_set_learning_mode(
    bitactor_ls_core_t* ls_actor,
    uint8_t mode
);

void bitactor_ls_set_adaptation_threshold(
    bitactor_ls_core_t* ls_actor,
    double threshold
);

// ---
// L9 Learning Layer Integration
// ---

// Extend 8-hop chain with learning hop
typedef enum {
    HOP_LEARNING_OBSERVE = 8,           // L9 learning observation
    HOP_LEARNING_ADAPT = 9,             // L9 pattern adaptation
    HOP_LEARNING_VALIDATE = 10          // L9 learning validation
} bitactor_ls_hop_t;

// Extended cognitive cycle with learning
uint64_t execute_ls_cognitive_cycle(
    bitactor_ls_core_t* ls_actor,
    void* context
);

// Learning-specific hop functions
uint64_t hop_learning_observe(bitactor_ls_core_t* ls_actor, void* context);
uint64_t hop_learning_adapt(bitactor_ls_core_t* ls_actor, void* context);
uint64_t hop_learning_validate(bitactor_ls_core_t* ls_actor, void* context);

// ---
// Pattern Discovery and Adaptation
// ---

// Pattern discovery (setup phase)
uint32_t bitactor_ls_discover_patterns(
    bitactor_ls_core_t* ls_actor,
    bitactor_signal_t* historical_signals,
    uint32_t signal_count,
    uint32_t window_size
);

// Pattern adaptation (setup phase)
bool bitactor_ls_adapt_pattern(
    bitactor_ls_pattern_t* pattern,
    const void* new_data,
    size_t data_size,
    double feedback_score
);

// Pattern sharing between actors
bool bitactor_ls_share_pattern(
    bitactor_ls_matrix_t* ls_matrix,
    uint32_t source_actor_id,
    uint32_t target_actor_id,
    uint16_t pattern_id
);

// ---
// Performance Validation for Learning
// ---

// Validate that learning preserves Trinity constraints
typedef struct {
    bool trinity_preserved;             // Learning doesn't break 8T/8H/8M
    bool sub_100ns_maintained;          // Hot path still sub-100ns
    bool learning_effective;            // Learning improves performance
    double learning_overhead_percent;   // Learning cost as % of execution
    uint64_t adaptation_cycles;         // Time spent adapting
    uint64_t execution_cycles;          // Time in hot path
} learning_performance_result_t;

learning_performance_result_t validate_ls_performance(
    bitactor_ls_matrix_t* ls_matrix
);

// Learning system benchmarks
void benchmark_bitactor_ls(void);

// Learning effectiveness metrics
void emit_learning_metrics(bitactor_ls_matrix_t* ls_matrix);

// ---
// CNS v8 Bridge Enhancement
// ---

// Enhanced CNS system with learning
typedef struct {
    cns_bitactor_system_t base_system;  // Base CNS integration
    bitactor_ls_matrix_t* ls_matrix;    // Learning-enhanced matrix
    
    // Learning-specific configuration
    uint8_t global_learning_mode;       // System-wide learning setting
    double global_adaptation_threshold; // System adaptation sensitivity
    uint32_t pattern_sharing_policy;    // How patterns are shared
    
    // Learning performance tracking
    learning_metrics_t system_metrics;
    bool learning_enabled;
} cns_bitactor_ls_system_t;

// Enhanced CNS functions
cns_bitactor_ls_system_t* cns_bitactor_ls_create(void);
void cns_bitactor_ls_destroy(cns_bitactor_ls_system_t* ls_sys);
bool cns_bitactor_ls_execute(
    cns_bitactor_ls_system_t* ls_sys,
    const char* ttl_input
);

// Learning system configuration
bool cns_bitactor_ls_enable_learning(
    cns_bitactor_ls_system_t* ls_sys,
    bool enable
);

bool cns_bitactor_ls_configure_adaptation(
    cns_bitactor_ls_system_t* ls_sys,
    double threshold,
    uint32_t window_size
);

// ---
// Registry Extension for Learning
// ---

// Learning-aware registry
typedef struct {
    bitactor_registry_t base_registry;  // Base registry
    
    // Learning-specific entries
    struct {
        char name[64];
        bitactor_ls_core_t* ls_actor;
        learning_metrics_t metrics;
        uint64_t learning_hash;
    } ls_entries[256];
    
    uint32_t ls_count;
    uint64_t global_learning_hash;
} bitactor_ls_registry_t;

// Registry functions for learning actors
void bitactor_ls_registry_init(bitactor_ls_registry_t* ls_registry);
bool bitactor_ls_registry_register(
    bitactor_ls_registry_t* ls_registry,
    const char* name,
    bitactor_ls_core_t* ls_actor
);
bitactor_ls_core_t* bitactor_ls_registry_lookup(
    bitactor_ls_registry_t* ls_registry,
    const char* name
);

// ---
// AOT Learning Compilation
// ---

// Compile learning patterns to bytecode (setup phase)
typedef struct {
    uint64_t pattern_spec_hash;         // Original pattern hash
    uint64_t compiled_hash;             // Compiled learning hash
    uint8_t* learning_bytecode;         // Executable learning logic
    size_t bytecode_size;
    bool compilation_valid;             // spec_hash == compiled_hash
    double optimization_level;          // Compilation optimization
} compiled_learning_pattern_t;

// AOT learning compiler
compiled_learning_pattern_t* compile_learning_pattern(
    const bitactor_ls_pattern_t* pattern
);

// Validate compiled learning pattern
bool validate_compiled_learning(
    const compiled_learning_pattern_t* compiled
);

// Apply compiled learning to actor
bool apply_compiled_learning(
    bitactor_ls_core_t* ls_actor,
    const compiled_learning_pattern_t* compiled
);

#endif // BITACTOR_LS_H