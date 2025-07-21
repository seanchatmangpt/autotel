/**
 * @file bitactor.h
 * @brief BitActor Core - Fifth Epoch Causality Native Systems
 * @version 1.0.0
 * 
 * BitActor: 8-bit causal units where specification IS execution.
 * Each bit represents one atomic unit of causal significance, compiled from TTL.
 * 
 * Revolutionary Principles:
 * - Causality IS computation
 * - Reality IS bit-aligned  
 * - Ontology IS executable hardware
 * - SHACL rules ARE logic circuits
 * - TTL triples ARE hardware vectors
 * 
 * @author Sean Chatman - Architect of the Fifth Epoch
 * @date 2024-01-15
 */

#ifndef BITACTOR_H
#define BITACTOR_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// FIFTH EPOCH CONSTANTS
// =============================================================================

#define CNS_VERSION "1.0.0"
#define BITACTOR_VERSION "8T8H8B"

// Trinity constants
#define MAX_8T_TICKS 8        // 8-tick physics boundary
#define MAX_8H_HOPS 8         // 8-hop causal collapse
#define MAX_8B_BITS 8         // 8-bit meaning atoms

// BitActor matrix dimensions
#define BITACTOR_MATRIX_SIZE 256    // 8x8x4 = 256 BitActors maximum
#define MEANING_ATOM_BITS 8         // Each BitActor is 8 bits
#define CAUSAL_VECTOR_SIZE 64       // 64-bit causal state vector
#define SIGNAL_BUFFER_SIZE 1024     // Input signal buffer

// Performance targets
#define TARGET_TICK_NS 125          // 8MHz tick rate (125ns per tick)
#define TARGET_COLLAPSE_NS 1000     // 1μs max for full 8H collapse
#define TARGET_PROOF_NS 500         // 500ns max for proof validation

// =============================================================================
// BITACTOR CORE TYPES
// =============================================================================

/**
 * @brief 8-bit causal unit - the quantum of meaning
 * 
 * Each bit represents one atomic unit of causal significance:
 * Bit 0: Trigger active
 * Bit 1: State valid
 * Bit 2: Action ready
 * Bit 3: Proof verified
 * Bit 4: Memory committed
 * Bit 5: Signal entangled
 * Bit 6: Collapse pending
 * Bit 7: Meta-validated
 */
typedef uint8_t BitActor;

/**
 * @brief 64-bit causal state vector
 * 
 * Encodes the complete causal state of a BitActor:
 * - Input triggers (16 bits)
 * - Internal state (16 bits)
 * - Output actions (16 bits)
 * - Proof chain (16 bits)
 */
typedef uint64_t CausalVector;

/**
 * @brief 8-hop causal proof chain
 * 
 * Each hop represents one step in the causal collapse:
 * 1. Trigger detected
 * 2. Ontology loaded
 * 3. SHACL path fired
 * 4. BitActor state resolved
 * 5. Collapse computed
 * 6. Action bound
 * 7. State committed
 * 8. Meta-proof validated
 */
typedef struct {
    uint64_t hops[MAX_8H_HOPS];     // Each hop's state
    uint8_t current_hop;            // Current position in chain
    uint64_t start_tick;            // Starting tick count
    uint64_t proof_hash;            // Cryptographic proof
    bool valid;                     // Proof chain valid
} CausalProofChain;

/**
 * @brief BitActor execution context
 * 
 * Contains everything needed for tick-aligned execution:
 * - Current BitActor state
 * - Causal vector
 * - Proof chain
 * - Signal inputs
 * - Action outputs
 */
typedef struct {
    BitActor bits;                          // 8-bit causal unit
    CausalVector vector;                    // 64-bit state vector
    CausalProofChain proof;                 // 8-hop proof chain
    uint8_t signal_buffer[SIGNAL_BUFFER_SIZE]; // Input signals
    uint16_t signal_length;                 // Signal buffer length
    uint64_t last_tick;                     // Last execution tick
    uint32_t actor_id;                      // Unique BitActor ID
    void* compiled_ttl;                     // Compiled TTL logic
} BitActorContext;

/**
 * @brief BitActor matrix - the computational substrate
 * 
 * A 256-element matrix of BitActors forming the computational mesh.
 * Each BitActor can signal to any other BitActor in the matrix.
 */
typedef struct {
    BitActorContext actors[BITACTOR_MATRIX_SIZE];  // 256 BitActors
    uint64_t global_tick;                          // Global tick counter
    uint32_t active_count;                         // Active BitActors
    uint64_t entanglement_matrix[32][32];          // Inter-actor signals
    uint8_t matrix_hash[32];                       // Matrix state hash
    bool globally_entangled;                       // Global entanglement active
} BitActorMatrix;

/**
 * @brief Signal types for nanoregex matching
 */
typedef enum {
    SIGNAL_TRIGGER,      // External trigger
    SIGNAL_NEWS,         // News/text data
    SIGNAL_PRICE,        // Price/numeric data
    SIGNAL_ONTOLOGY,     // Ontology update
    SIGNAL_COLLAPSE,     // Causal collapse
    SIGNAL_ENTANGLE,     // Entanglement signal
    SIGNAL_PROOF,        // Proof validation
    SIGNAL_META          // Meta-signal
} SignalType;

/**
 * @brief Input signal for BitActor processing
 */
typedef struct {
    SignalType type;                    // Signal type
    uint8_t data[256];                  // Signal data
    uint16_t length;                    // Data length
    uint64_t timestamp_ns;              // Nanosecond timestamp
    uint32_t source_id;                 // Source BitActor ID
    uint8_t priority;                   // Signal priority (0-255)
    uint64_t nanoregex_mask;            // Compiled nanoregex pattern
} BitActorSignal;

// =============================================================================
// CORE BITACTOR API
// =============================================================================

/**
 * @brief Initialize BitActor matrix
 * @return Pointer to initialized matrix
 */
BitActorMatrix* bitactor_matrix_create(void);

/**
 * @brief Destroy BitActor matrix
 * @param matrix Matrix to destroy
 */
void bitactor_matrix_destroy(BitActorMatrix* matrix);

/**
 * @brief Spawn new BitActor with compiled TTL logic
 * @param matrix Target matrix
 * @param compiled_ttl Compiled TTL logic
 * @return BitActor ID or 0 on failure
 */
uint32_t bitactor_spawn(BitActorMatrix* matrix, void* compiled_ttl);

/**
 * @brief Send signal to BitActor
 * @param matrix Target matrix
 * @param actor_id Target BitActor ID
 * @param signal Input signal
 * @return true if signal accepted
 */
bool bitactor_signal(BitActorMatrix* matrix, uint32_t actor_id, const BitActorSignal* signal);

/**
 * @brief Execute single 8T tick on entire matrix
 * @param matrix Target matrix
 * @return Number of BitActors that executed
 */
uint32_t bitactor_tick(BitActorMatrix* matrix);

/**
 * @brief Trigger 8H causal collapse on specific BitActor
 * @param matrix Target matrix
 * @param actor_id Target BitActor ID
 * @return Collapse result vector
 */
CausalVector bitactor_collapse(BitActorMatrix* matrix, uint32_t actor_id);

/**
 * @brief Validate 8H proof chain
 * @param proof Proof chain to validate
 * @return true if proof is valid
 */
bool bitactor_validate_proof(const CausalProofChain* proof);

/**
 * @brief Enable global entanglement across matrix
 * @param matrix Target matrix
 * @return true if entanglement successful
 */
bool bitactor_entangle_global(BitActorMatrix* matrix);

/**
 * @brief Get matrix performance metrics
 * @param matrix Target matrix
 * @param[out] tick_rate_hz Current tick rate
 * @param[out] collapse_time_ns Average collapse time
 * @param[out] proof_time_ns Average proof time
 */
void bitactor_get_metrics(const BitActorMatrix* matrix, 
                         double* tick_rate_hz,
                         uint64_t* collapse_time_ns,
                         uint64_t* proof_time_ns);

// =============================================================================
// BITACTOR INTROSPECTION
// =============================================================================

/**
 * @brief Extract individual meaning bits from BitActor
 * @param actor BitActor to analyze
 * @param bit_index Bit index (0-7)
 * @return true if bit is set
 */
static inline bool bitactor_get_meaning_bit(BitActor actor, uint8_t bit_index) {
    return (actor & (1 << bit_index)) != 0;
}

/**
 * @brief Set individual meaning bit in BitActor
 * @param actor Pointer to BitActor
 * @param bit_index Bit index (0-7)
 * @param value New bit value
 */
static inline void bitactor_set_meaning_bit(BitActor* actor, uint8_t bit_index, bool value) {
    if (value) {
        *actor |= (1 << bit_index);
    } else {
        *actor &= ~(1 << bit_index);
    }
}

/**
 * @brief Count active meaning bits in BitActor
 * @param actor BitActor to analyze
 * @return Number of active bits (0-8)
 */
static inline uint8_t bitactor_count_active_bits(BitActor actor) {
    uint8_t count = 0;
    for (int i = 0; i < 8; i++) {
        if (actor & (1 << i)) count++;
    }
    return count;
}

/**
 * @brief Get high-precision timestamp for tick alignment
 * @return Nanosecond timestamp
 */
static inline uint64_t bitactor_get_tick_timestamp(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

// =============================================================================
// FIFTH EPOCH VALIDATION
// =============================================================================

/**
 * @brief Validate Fifth Epoch principles
 * @param matrix Matrix to validate
 * @return true if matrix conforms to Fifth Epoch
 */
bool bitactor_validate_fifth_epoch(const BitActorMatrix* matrix);

/**
 * @brief Generate Trinity hash (8T8H8B signature)
 * @param matrix Matrix to hash
 * @return 64-bit Trinity hash
 */
uint64_t bitactor_generate_trinity_hash(const BitActorMatrix* matrix);

/**
 * @brief Print BitActor matrix status
 * @param matrix Matrix to display
 */
void bitactor_print_matrix_status(const BitActorMatrix* matrix);

// =============================================================================
// INTERNAL FUNCTIONS (IMPLEMENTATION DETAIL)
// =============================================================================

/**
 * @brief Execute single BitActor (internal)
 * @param ctx BitActor context
 * @param global_tick Current global tick
 */
static void bitactor_execute_single(BitActorContext* ctx, uint64_t global_tick);

/**
 * @brief Update matrix state hash (internal)
 * @param matrix Matrix to update
 */
static void bitactor_update_matrix_hash(BitActorMatrix* matrix);

#ifdef __cplusplus
}
#endif

#endif // BITACTOR_H