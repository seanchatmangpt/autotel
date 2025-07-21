/**
 * @file bitactor.h
 * @brief BitActor System - Fifth Epoch Causal Computing Implementation
 *
 * The BitActor system implements the 8T/8H/8B Trinity for causal computing:
 * - 8T Physics: Operations in ≤8 cycles with causal determinism
 * - 8H Cognition: 8-hop causal proof chains 
 * - 8B Memory: 8-bit meaning atoms as machine-native causal units
 *
 * This is the core implementation of the Fifth Epoch architecture where
 * specification becomes execution and causality becomes computation.
 */
#ifndef CNS_BITACTOR_H
#define CNS_BITACTOR_H

#include "cns_core.h"
#include <stdint.h>
#include <stddef.h>

// ---
// Part 1: BitActor Trinity Constants
// ---

#define BITACTOR_8T_MAX_CYCLES 8           // 8T Physics: max cycles per operation
#define BITACTOR_8H_HOP_COUNT 8            // 8H Cognition: causal proof chain length
#define BITACTOR_8B_MEANING_BITS 8         // 8B Memory: meaning atom size
#define BITACTOR_TRINITY_HASH 0x8888888888888888ULL // Trinity signature

// BitActor domain constants
#define BITACTOR_MAX_DOMAINS 64            // Max parallel domains
#define BITACTOR_MAX_SIGNALS 1024          // Max signals per domain
#define BITACTOR_MAX_ACTORS 256            // Max actors per domain
#define BITACTOR_TICK_NS 100               // Nanoseconds per tick (UHF trading)

// ---
// Part 2: Core BitActor Types
// ---

/**
 * @typedef bitactor_meaning_t
 * @brief The quantum of meaning - 8-bit semantic unit
 * Each bit represents one atomic causal significance unit compiled from TTL
 */
typedef uint8_t bitactor_meaning_t;

/**
 * @typedef bitactor_signal_t
 * @brief Signal vector for nanoregex pattern matching
 * 64-bit vector for parallel signal processing
 */
typedef uint64_t bitactor_signal_t;

/**
 * @typedef bitactor_state_t
 * @brief BitActor state representation
 * Combines meaning atoms with execution state
 */
typedef struct CNS_ALIGN_64 {
    bitactor_meaning_t meaning;        // 8-bit meaning atom
    uint8_t hop_position;              // Current position in 8H chain
    uint8_t tick_budget;               // Remaining 8T cycles
    uint8_t domain_id;                 // Domain identifier
    uint32_t actor_id;                 // Unique actor identifier
} bitactor_state_t;

/**
 * @typedef bitactor_collapse_t
 * @brief Causal collapse result
 * Contains the outcome of a causal domain collapse operation
 */
typedef struct CNS_ALIGN_64 {
    cns_bitmask_t success_mask;        // Success indicators for parallel actors
    cns_bitmask_t failure_mask;        // Failure indicators
    cns_cycle_t collapse_cycles;       // Cycles consumed in collapse
    uint32_t mutations_count;          // Number of mutations generated
} bitactor_collapse_t;

// ---
// Part 3: 8H Cognitive Architecture
// ---

/**
 * @enum bitactor_hop_type_t
 * @brief The 8 hops of the cognitive cycle
 * Each hop represents one step in the causal proof chain
 */
typedef enum {
    BITACTOR_HOP_TRIGGER_DETECT = 0,   // 1. Trigger detected
    BITACTOR_HOP_ONTOLOGY_LOAD,        // 2. Ontology loaded
    BITACTOR_HOP_SHACL_FIRE,           // 3. SHACL path fired
    BITACTOR_HOP_STATE_RESOLVE,        // 4. BitActor state resolved
    BITACTOR_HOP_COLLAPSE_COMPUTE,     // 5. Collapse computed
    BITACTOR_HOP_ACTION_BIND,          // 6. Action bound
    BITACTOR_HOP_STATE_COMMIT,         // 7. State committed
    BITACTOR_HOP_META_VALIDATE         // 8. Meta-proof validated
} bitactor_hop_type_t;

/**
 * @typedef bitactor_hop_fn_t
 * @brief Function pointer for hop execution
 * Returns success bitmask for parallel execution
 */
typedef cns_bitmask_t (*bitactor_hop_fn_t)(void* context, bitactor_state_t* state);

/**
 * @typedef bitactor_cognitive_cycle_t
 * @brief Complete 8H cognitive cycle definition
 */
typedef struct CNS_ALIGN_CACHELINE {
    bitactor_hop_fn_t hops[BITACTOR_8H_HOP_COUNT];  // 8 hop functions
    cns_cycle_t hop_budgets[BITACTOR_8H_HOP_COUNT]; // Cycle budget per hop
    cns_bitmask_t hop_dependencies;                  // Dependency graph
    uint32_t cycle_id;                               // Unique cycle identifier
} bitactor_cognitive_cycle_t;

// ---
// Part 4: Signal Processing & Nanoregex
// ---

/**
 * @typedef bitactor_nanoregex_t
 * @brief Ultra-fast pattern matcher for signal recognition
 * Compiled regex patterns optimized for L1 cache residence
 */
typedef struct CNS_ALIGN_64 {
    uint64_t pattern_hash;             // Hash of compiled pattern
    cns_bitmask_t match_mask;         // Bitmask for parallel matching
    uint16_t pattern_length;           // Pattern length in bytes
    uint8_t pattern_data[46];          // Inline pattern data (cache-aligned)
} bitactor_nanoregex_t;

/**
 * @typedef bitactor_feed_actor_t
 * @brief Signal input processor
 * Handles real-time signal ingestion and pattern matching
 */
typedef struct CNS_ALIGN_64 {
    bitactor_nanoregex_t patterns[8];  // Up to 8 parallel patterns
    bitactor_signal_t last_signals[8]; // Last signal states
    cns_cycle_t last_match_cycles;     // Timestamp of last match
    uint32_t match_count;              // Total matches processed
} bitactor_feed_actor_t;

// ---
// Part 5: Domain & Matrix Architecture
// ---

/**
 * @typedef bitactor_domain_t
 * @brief Causal domain containing related BitActors
 * Represents one coherent semantic space
 */
typedef struct CNS_ALIGN_CACHELINE {
    bitactor_state_t actors[BITACTOR_MAX_ACTORS];    // Actor array
    bitactor_feed_actor_t feed_actor;                // Signal processor
    bitactor_cognitive_cycle_t cognitive_cycle;      // 8H reasoning
    cns_bitmask_t active_mask;                       // Active actor mask
    cns_arena_t* arena;                              // Memory arena
    uint32_t domain_id;                              // Domain identifier
    uint32_t actor_count;                            // Number of active actors
} bitactor_domain_t;

/**
 * @typedef bitactor_matrix_t
 * @brief Global BitActor matrix - the complete system
 * Manages all domains and provides global entanglement
 */
typedef struct CNS_ALIGN_CACHELINE {
    bitactor_domain_t domains[BITACTOR_MAX_DOMAINS]; // Domain array
    cns_bitmask_t domain_active_mask;                // Active domain mask
    cns_cycle_t global_tick_counter;                 // Global time reference
    uint32_t domain_count;                           // Number of active domains
    uint32_t total_actors;                           // Total actors across domains
    
    // Performance metrics
    struct {
        cns_cycle_t last_collapse_cycles;            // Last collapse duration
        uint64_t total_collapses;                    // Total collapses performed
        uint64_t total_mutations;                    // Total mutations generated
        cns_cycle_t min_collapse_cycles;             // Fastest collapse
        cns_cycle_t max_collapse_cycles;             // Slowest collapse
    } metrics;
} bitactor_matrix_t;

// ---
// Part 6: Core BitActor Operations
// ---

/**
 * @brief Initialize a BitActor matrix
 * Sets up the global matrix with default domains and cognitive cycles
 */
CNS_INLINE cns_bool_t bitactor_matrix_init(
    bitactor_matrix_t* matrix, 
    cns_arena_t* arena
);

/**
 * @brief Create a new BitActor domain
 * Allocates and initializes a causal domain within the matrix
 */
CNS_INLINE uint32_t bitactor_domain_create(
    bitactor_matrix_t* matrix,
    const char* domain_name,
    bitactor_cognitive_cycle_t* cycle_template
);

/**
 * @brief Add a BitActor to a domain
 * Creates a new actor with specified meaning atoms
 */
CNS_INLINE uint32_t bitactor_add_to_domain(
    bitactor_domain_t* domain,
    bitactor_meaning_t meaning,
    const char* actor_name
);

/**
 * @brief Perform causal domain collapse
 * Executes the complete 8H cognitive cycle for all actors in domain
 * Returns collapse result with performance metrics
 */
CNS_INLINE bitactor_collapse_t bitactor_domain_collapse(
    bitactor_domain_t* domain,
    bitactor_signal_t* input_signals,
    uint32_t signal_count
);

/**
 * @brief Global matrix tick
 * Processes all active domains and maintains global entanglement
 * This is the main execution loop for the BitActor system
 */
CNS_INLINE cns_bitmask_t bitactor_matrix_tick(
    bitactor_matrix_t* matrix,
    bitactor_signal_t* global_signals,
    uint32_t signal_count
);

// ---
// Part 7: Signal Processing Functions
// ---

/**
 * @brief Compile nanoregex pattern
 * AOT compilation of regex patterns into BitActor-optimized format
 */
CNS_INLINE cns_bool_t bitactor_nanoregex_compile(
    bitactor_nanoregex_t* regex,
    const char* pattern
);

/**
 * @brief Match signals against nanoregex
 * Ultra-fast pattern matching optimized for tick-level execution
 */
CNS_INLINE cns_bitmask_t bitactor_nanoregex_match(
    const bitactor_nanoregex_t* regex,
    const bitactor_signal_t* signals,
    uint32_t signal_count
);

/**
 * @brief Update feed actor with new signals
 * Real-time signal ingestion and pattern matching
 */
CNS_INLINE cns_bool_t bitactor_feed_actor_update(
    bitactor_feed_actor_t* feed_actor,
    const bitactor_signal_t* signals,
    uint32_t signal_count
);

// ---
// Part 8: TTL Integration & Dark 80/20
// ---

/**
 * @brief Compile TTL ontology to BitActor domain
 * Transforms semantic web ontologies into executable BitActor code
 * This unlocks the "Dark 80/20" - previously unused ontology logic
 */
CNS_INLINE cns_bool_t bitactor_compile_ttl_to_domain(
    bitactor_domain_t* domain,
    const char* ttl_content,
    size_t ttl_length
);

/**
 * @brief Compile SHACL constraints to cognitive cycle
 * Transforms SHACL validation rules into 8H reasoning chains
 */
CNS_INLINE cns_bool_t bitactor_compile_shacl_to_cycle(
    bitactor_cognitive_cycle_t* cycle,
    const char* shacl_content,
    size_t shacl_length
);

/**
 * @brief Generate mutations from collapse
 * Produces provably correct TTL mutations from causal collapse results
 */
CNS_INLINE uint32_t bitactor_generate_mutations(
    const bitactor_collapse_t* collapse,
    cns_mutation_t* mutations,
    uint32_t max_mutations
);

// ---
// Part 9: Performance & Debugging
// ---

/**
 * @brief Validate 8T compliance
 * Ensures all operations complete within 8-tick budget
 */
CNS_INLINE cns_bool_t bitactor_validate_8t_compliance(
    const bitactor_matrix_t* matrix
);

/**
 * @brief Get performance metrics
 * Returns detailed performance statistics for optimization
 */
CNS_INLINE void bitactor_get_metrics(
    const bitactor_matrix_t* matrix,
    char* output_buffer,
    size_t buffer_size
);

/**
 * @brief Dump BitActor state for debugging
 * Outputs complete system state in human-readable format
 */
CNS_INLINE void bitactor_dump_state(
    const bitactor_matrix_t* matrix,
    FILE* output
);

// ---
// Part 10: Architectural Contracts & Proofs
// ---

// Compile-time verification of 8B memory contracts
_Static_assert(sizeof(bitactor_state_t) % 8 == 0, "8B Contract: bitactor_state_t");
_Static_assert(sizeof(bitactor_collapse_t) % 8 == 0, "8B Contract: bitactor_collapse_t");
_Static_assert(sizeof(bitactor_nanoregex_t) % 8 == 0, "8B Contract: bitactor_nanoregex_t");
_Static_assert(sizeof(bitactor_feed_actor_t) % 64 == 0, "Cache Line: bitactor_feed_actor_t");
_Static_assert(sizeof(bitactor_domain_t) % 64 == 0, "Cache Line: bitactor_domain_t");

// Verify Trinity hash alignment
_Static_assert((BITACTOR_TRINITY_HASH & 0x8888888888888888ULL) == BITACTOR_TRINITY_HASH, 
               "Trinity Hash Verification");

// Verify 8H hop count
_Static_assert(BITACTOR_8H_HOP_COUNT == 8, "8H Cognitive Cycle Verification");

/**
 * @def BITACTOR_ASSERT_8T(op)
 * @brief Enforce 8T compliance at runtime
 */
#ifdef CNS_DEBUG
#define BITACTOR_ASSERT_8T(op) do { \
    cns_cycle_t start = CNS_RDTSC(); \
    op; \
    cns_cycle_t cycles = CNS_RDTSC() - start; \
    assert(cycles <= BITACTOR_8T_MAX_CYCLES); \
} while(0)
#else
#define BITACTOR_ASSERT_8T(op) op
#endif

/**
 * @def BITACTOR_TRINITY_SIGNATURE
 * @brief Compile-time signature for Trinity compliance
 */
#define BITACTOR_TRINITY_SIGNATURE() \
    static const uint64_t __bitactor_trinity_sig __attribute__((unused)) = BITACTOR_TRINITY_HASH

#endif /* CNS_BITACTOR_H */
