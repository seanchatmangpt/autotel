
#ifndef BITACTOR_80_20_H
#define BITACTOR_80_20_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// ---
// Part 1: Core Architecture (Essential 20%)
// ---

// 8T: 8-Tick Execution Budget
#define BITACTOR_8T_MAX_CYCLES 8
#define VALIDATE_8T(cycles) static_assert(cycles <= 8, "8T violation")

// 8H: 8-Hop Reasoning Chain
#define BITACTOR_8H_HOP_COUNT 8
typedef enum {
    HOP_TRIGGER_DETECT = 0,
    HOP_ONTOLOGY_LOAD = 1,
    HOP_SHACL_FIRE = 2,
    HOP_STATE_RESOLVE = 3,
    HOP_COLLAPSE_COMPUTE = 4,
    HOP_ACTION_BIND = 5,
    HOP_STATE_COMMIT = 6,
    HOP_META_VALIDATE = 7
} bitactor_hop_t;

// 8M: 8-Bit Memory Quantum
typedef uint8_t bitactor_meaning_t;  // Atomic unit of causal significance
#define VALIDATE_8M(size) static_assert(size % 8 == 0, "8M violation")

typedef uint64_t bitactor_signal_t;

typedef uint64_t cns_bitmask_t;

// Forward declarations for interdependent types
typedef struct bitactor_nanoregex_t bitactor_nanoregex_t;
typedef struct bitactor_feed_actor_t bitactor_feed_actor_t;
typedef struct compiled_bitactor_t compiled_bitactor_t;
typedef struct bitactor_domain_t bitactor_domain_t;
typedef struct bitactor_matrix_t bitactor_matrix_t;
typedef struct compiled_specification_t compiled_specification_t;
typedef struct bitactor_manifest_t bitactor_manifest_t;
typedef struct bitactor_registry_entry_t bitactor_registry_entry_t;
typedef struct bitactor_registry_t bitactor_registry_t;
typedef struct cns_bitactor_system_t cns_bitactor_system_t;

// Nanoregex structure
struct bitactor_nanoregex_t {
    uint64_t pattern_hash;
    uint64_t match_mask;
    uint16_t pattern_length;
    char pattern_data[64 - sizeof(uint64_t) * 2 - sizeof(uint16_t)];
};

// Feed actor structure
struct bitactor_feed_actor_t {
    bitactor_nanoregex_t patterns[8];
    uint32_t match_count;
    uint64_t last_match_cycles;
};

// Manifest structure
struct bitactor_manifest_t {
    uint64_t spec_hash;
    uint8_t* bytecode;
    uint32_t bytecode_size;
};

// Pre-compiled BitActor - everything pre-computed for zero overhead
struct compiled_bitactor_t {
    // Hot data (first cache line) - accessed every tick
    bitactor_meaning_t meaning;           // 8-bit causal state
    uint8_t signal_pending;               // Quick signal check
    uint16_t bytecode_offset;             // Current execution position
    uint32_t tick_count;                  // Execution counter
    uint64_t causal_vector;               // Pre-computed relationships

    // Pre-compiled bytecode (aligned for SIMD)
    uint8_t bytecode[256] __attribute__((aligned(32)));
    uint32_t bytecode_size;
    bitactor_manifest_t* manifest; // Reference to its manifest

    // Performance validation
    uint64_t execution_cycles;            // Last execution time
    bool trinity_compliant;               // 8T/8H/8M validation
} __attribute__((aligned(64)));

// Domain structure
struct bitactor_domain_t {
    uint32_t domain_id;
    uint32_t actor_count;
    uint64_t active_mask;
    compiled_bitactor_t actors[256];
    bitactor_feed_actor_t feed_actor;
};

// Matrix structure
struct bitactor_matrix_t {
    // Hot data - accessed every tick
    uint64_t global_tick;
    uint32_t domain_count;
    uint64_t domain_active_mask;
    bitactor_domain_t domains[8];

    // Performance metrics
    struct {
        uint64_t total_executions;
        uint64_t sub_100ns_count;
        uint64_t min_cycles;
        uint64_t max_cycles;
        double avg_cycles;
    } performance;
} __attribute__((aligned(4096)));

// Registry structures
struct bitactor_registry_entry_t {
    char name[64];
    compiled_bitactor_t* actor;
};

struct bitactor_registry_t {
    bitactor_registry_entry_t entries[256]; // Simple array for 80/20 registry
    uint32_t count;
};

typedef struct {
    bitactor_registry_t* registry; // For 80/20, bus directly uses registry
} bitactor_entanglement_bus_t;

// Specification structure
struct compiled_specification_t {
    uint64_t specification_hash;          // Original TTL hash
    uint64_t execution_hash;              // Compiled bytecode hash
    uint8_t* bytecode;                    // Executable instructions
    size_t bytecode_size;
    bool hash_validated;                  // spec_hash == exec_hash
};

// Minimal CNS bridge
struct cns_bitactor_system_t {
    bitactor_matrix_t* matrix;
    compiled_specification_t* specs[64];
    uint32_t spec_count;
    uint64_t trinity_hash;
    bitactor_registry_t registry;
    bitactor_entanglement_bus_t entanglement_bus;
};

// Function declarations

// Part 1: Core Architecture
void bitactor_execute_hot_path(compiled_bitactor_t* actor);
uint32_t bitactor_matrix_tick(bitactor_matrix_t* matrix, bitactor_signal_t* signals, uint32_t signal_count);
uint32_t bitactor_domain_create(bitactor_matrix_t* matrix);
uint32_t bitactor_add_to_domain(bitactor_domain_t* domain, bitactor_meaning_t meaning, bitactor_manifest_t* manifest, const char* actor_name, cns_bitactor_system_t* sys);

// Part 2: AOT Specification Compiler
bool bitactor_nanoregex_compile(bitactor_nanoregex_t* regex, const char* pattern);
cns_bitmask_t bitactor_nanoregex_match(const bitactor_nanoregex_t* regex, const bitactor_signal_t* signals, uint32_t signal_count);
compiled_specification_t* compile_ttl_to_bitactor(const char* ttl_spec);
uint64_t hash_ttl_content(const char* ttl_spec);
bitactor_manifest_t* create_bitactor_manifest(const char* ttl_spec);

// Part 3: Cognitive Reasoning
typedef uint64_t (*bitactor_hop_fn_t)(compiled_bitactor_t* actor, void* context);
uint64_t execute_cognitive_cycle(compiled_bitactor_t* actor, void* context);
bool bitactor_feed_actor_update(bitactor_feed_actor_t* feed_actor, const bitactor_signal_t* signals, uint32_t signal_count);

// Part 4: Validation Framework
typedef struct {
    bool sub_100ns_achieved;
    bool trinity_compliant;
    uint64_t execution_cycles;
    double improvement_factor;
    double avg_cycles;
} performance_result_t;

performance_result_t validate_performance(bitactor_matrix_t* matrix);
void benchmark_bitactor_80_20(void);
void benchmark_l6_l7_permutations(void);

// Part 5: System Integration
cns_bitactor_system_t* cns_bitactor_create(void);
void cns_bitactor_destroy(cns_bitactor_system_t* sys);
bool cns_bitactor_execute(cns_bitactor_system_t* sys, const char* ttl_input);
void emit_performance_metrics(bitactor_matrix_t* matrix);
bitactor_matrix_t* bitactor_matrix_create(void);

// Part 6: Registry
void bitactor_registry_init(bitactor_registry_t* registry);
bool bitactor_registry_register_actor(
    bitactor_registry_t* registry,
    const char* name,
    compiled_bitactor_t* actor
);
compiled_bitactor_t* bitactor_registry_lookup_actor(
    bitactor_registry_t* registry,
    const char* name
);

// Part 7: Entanglement Bus
void bitactor_entanglement_bus_init(bitactor_entanglement_bus_t* bus, bitactor_registry_t* registry);
bool bitactor_entanglement_bus_propagate_signal(
    bitactor_entanglement_bus_t* bus,
    const char* target_actor_name,
    bitactor_signal_t signal
);

#endif // BITACTOR_80_20_H
