/**
 * @file entanglement_oracle.h
 * @brief L7 Entanglement Bus - Shared-memory causal propagation for BitActor
 *
 * The Entanglement Bus implements L7 of the BitActor stack, providing:
 * - Shared-memory causal propagation between actors
 * - BA_OP_ENTANGLE operations for reactive logic
 * - Bounded signal forwarding (no infinite loops)
 * - Dark triple activation for dormant logic
 *
 * Performance: Sub-100ns hot path, 8T/8H/8B Trinity compliance
 */
#ifndef CNS_ENTANGLEMENT_ORACLE_H
#define CNS_ENTANGLEMENT_ORACLE_H

#include "cns_core.h"
#include "bitactor.h"
#include <stdint.h>
#include <stdbool.h>

// ---
// Part 1: L7 Constants & Trinity Compliance
// ---

#define ENTANGLEMENT_MAX_HOPS 8                    // 8T: Max propagation hops
#define ENTANGLEMENT_MAX_CONNECTIONS 64            // Max entangled actors per domain
#define ENTANGLEMENT_SIGNAL_BUFFER_SIZE 256        // Signal propagation buffer
#define ENTANGLEMENT_ORACLE_HASH 0x7777777777777777ULL // L7 signature

// BA_OP_ENTANGLE opcodes (8 operations for 8T compliance)
typedef enum {
    BA_OP_ENTANGLE_CREATE = 0,     // Create new entanglement
    BA_OP_ENTANGLE_SIGNAL = 1,     // Send signal through entanglement
    BA_OP_ENTANGLE_LISTEN = 2,     // Listen for entangled signals
    BA_OP_ENTANGLE_BREAK = 3,      // Break entanglement connection
    BA_OP_ENTANGLE_RIPPLE = 4,     // Multi-hop ripple effect
    BA_OP_ENTANGLE_DARK = 5,       // Activate dark triple
    BA_OP_ENTANGLE_BOUND = 6,      // Check hop bounds
    BA_OP_ENTANGLE_FLUSH = 7       // Flush signal buffers
} entanglement_opcode_t;

// ---
// Part 2: Core Entanglement Structures (80/20 Essential)
// ---

/**
 * @typedef entanglement_connection_t
 * @brief Single entanglement connection between two actors
 * Hot data structure - must be cache-aligned for performance
 */
typedef struct CNS_ALIGN_64 {
    uint32_t source_actor_id;      // Source actor ID
    uint32_t target_actor_id;      // Target actor ID
    uint8_t hop_count;             // Current hop count (bounded by 8T)
    uint8_t signal_strength;       // Signal strength (8-bit for 8B compliance)
    uint16_t connection_flags;     // Connection state flags
    uint64_t last_signal_tick;     // Last signal timestamp
    bitactor_meaning_t trigger_mask; // 8-bit trigger condition
    bitactor_meaning_t response_pattern; // 8-bit response pattern
} entanglement_connection_t;

/**
 * @typedef entanglement_signal_t
 * @brief Signal propagating through entanglement network
 * Optimized for 8T cycle processing
 */
typedef struct CNS_ALIGN_32 {
    uint32_t source_id;            // Original source actor
    uint32_t target_id;            // Target actor for this hop
    bitactor_meaning_t payload;    // 8-bit signal payload
    uint8_t hop_count;             // Hops remaining (bounded)
    uint16_t signal_flags;         // Signal type flags
    uint64_t propagation_vector;   // Pre-computed propagation path
} entanglement_signal_t;

/**
 * @typedef entanglement_oracle_t
 * @brief L7 Entanglement Bus Oracle - Core coordination structure
 * Manages all entanglements within a BitActor domain
 */
typedef struct CNS_ALIGN_CACHELINE {
    // Hot path data (first cache line)
    entanglement_connection_t connections[ENTANGLEMENT_MAX_CONNECTIONS];
    uint32_t connection_count;     // Active connections
    uint32_t total_signals;        // Total signals processed
    uint64_t domain_entanglement_mask; // Quick active connection mask
    
    // Signal processing buffers
    entanglement_signal_t signal_buffer[ENTANGLEMENT_SIGNAL_BUFFER_SIZE];
    uint32_t buffer_head;          // Buffer write position
    uint32_t buffer_tail;          // Buffer read position
    uint32_t signals_queued;       // Signals awaiting processing
    
    // Dark triple activation (80/20 - simple implementation)
    uint64_t dark_triple_mask;     // Dormant triples that can activate
    uint32_t dark_activations;     // Count of dark activations
    
    // Performance metrics
    struct {
        uint64_t total_propagations; // Total signal propagations
        uint64_t bounded_rejections; // Signals rejected due to hop limits
        uint64_t dark_activations;   // Dark triple activations
        cns_cycle_t last_propagation_cycles; // Last propagation time
    } metrics;
} entanglement_oracle_t;

// ---
// Part 3: Core L7 Operations (Essential 20%)
// ---

/**
 * @brief Initialize entanglement oracle for a domain
 * Setup phase - can be slower, called once per domain
 */
CNS_INLINE bool entanglement_oracle_init(
    entanglement_oracle_t* oracle,
    uint32_t domain_id
);

/**
 * @brief Create entanglement connection between two actors
 * BA_OP_ENTANGLE_CREATE implementation
 * Hot path - must be <100ns
 */
CNS_INLINE bool entanglement_create(
    entanglement_oracle_t* oracle,
    uint32_t source_actor_id,
    uint32_t target_actor_id,
    bitactor_meaning_t trigger_mask
);

/**
 * @brief Propagate signal through entanglement network
 * BA_OP_ENTANGLE_SIGNAL implementation - CRITICAL HOT PATH
 * Must complete in ≤8 cycles for 8T compliance
 */
CNS_INLINE uint32_t entanglement_propagate_signal(
    entanglement_oracle_t* oracle,
    uint32_t source_actor_id,
    bitactor_meaning_t signal_payload,
    uint8_t max_hops
);

/**
 * @brief Process queued entanglement signals
 * BA_OP_ENTANGLE_LISTEN implementation
 * Hot path - processes all signals in one batch
 */
CNS_INLINE uint32_t entanglement_process_signals(
    entanglement_oracle_t* oracle,
    bitactor_domain_t* domain
);

/**
 * @brief Check and enforce hop bounds
 * BA_OP_ENTANGLE_BOUND implementation - prevents infinite loops
 * Critical for system stability
 */
CNS_INLINE bool entanglement_check_bounds(
    const entanglement_signal_t* signal,
    uint8_t max_hops
);

/**
 * @brief Activate dark triples based on entanglement signals
 * BA_OP_ENTANGLE_DARK implementation - 80/20 simplified version
 * Brings dormant logic to life through entanglement ripples
 */
CNS_INLINE uint32_t entanglement_activate_dark_triples(
    entanglement_oracle_t* oracle,
    bitactor_domain_t* domain,
    const entanglement_signal_t* trigger_signal
);

// ---
// Part 4: Integration with BitActor Core
// ---

/**
 * @brief Integrate L7 entanglement into BitActor domain
 * Called during domain initialization
 */
CNS_INLINE bool bitactor_domain_add_entanglement(
    bitactor_domain_t* domain,
    entanglement_oracle_t* oracle
);

/**
 * @brief Execute entanglement operations during cognitive cycle
 * Called from bitactor_domain_collapse() at appropriate hop
 * Hot path integration point
 */
CNS_INLINE cns_bitmask_t bitactor_execute_entanglement_hop(
    entanglement_oracle_t* oracle,
    bitactor_domain_t* domain,
    entanglement_opcode_t operation,
    void* operation_data
);

/**
 * @brief Flush entanglement signals at end of domain tick
 * BA_OP_ENTANGLE_FLUSH implementation
 * Ensures clean state for next tick
 */
CNS_INLINE void entanglement_flush_signals(
    entanglement_oracle_t* oracle
);

// ---
// Part 5: Performance & Debug (80/20 - Essential monitoring only)
// ---

/**
 * @brief Validate L7 performance compliance
 * Ensures entanglement operations meet 8T constraints
 */
CNS_INLINE bool entanglement_validate_performance(
    const entanglement_oracle_t* oracle
);

/**
 * @brief Get entanglement metrics
 * Essential metrics only for 80/20 implementation
 */
CNS_INLINE void entanglement_get_metrics(
    const entanglement_oracle_t* oracle,
    char* output_buffer,
    size_t buffer_size
);

// ---
// Part 6: Trinity Compliance & Contracts
// ---

// Compile-time verification of 8B memory contracts
_Static_assert(sizeof(entanglement_connection_t) % 8 == 0, "8B Contract: entanglement_connection_t");
_Static_assert(sizeof(entanglement_signal_t) % 8 == 0, "8B Contract: entanglement_signal_t");
_Static_assert(sizeof(entanglement_oracle_t) % 64 == 0, "Cache Line: entanglement_oracle_t");

// Verify L7 signature
_Static_assert((ENTANGLEMENT_ORACLE_HASH & 0x7777777777777777ULL) == ENTANGLEMENT_ORACLE_HASH, 
               "L7 Oracle Hash Verification");

// Verify 8T hop constraints
_Static_assert(ENTANGLEMENT_MAX_HOPS == 8, "8T Hop Constraint Verification");

/**
 * @def ENTANGLEMENT_ASSERT_8T(op)
 * @brief Enforce 8T compliance for entanglement operations
 */
#ifdef CNS_DEBUG
#define ENTANGLEMENT_ASSERT_8T(op) do { \
    cns_cycle_t start = CNS_RDTSC(); \
    op; \
    cns_cycle_t cycles = CNS_RDTSC() - start; \
    assert(cycles <= BITACTOR_8T_MAX_CYCLES); \
} while(0)
#else
#define ENTANGLEMENT_ASSERT_8T(op) op
#endif

/**
 * @def ENTANGLEMENT_L7_SIGNATURE
 * @brief Compile-time signature for L7 compliance
 */
#define ENTANGLEMENT_L7_SIGNATURE() \
    static const uint64_t __entanglement_l7_sig __attribute__((unused)) = ENTANGLEMENT_ORACLE_HASH

#endif /* CNS_ENTANGLEMENT_ORACLE_H */