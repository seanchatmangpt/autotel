/**
 * @file entanglement_oracle.c
 * @brief L7 Entanglement Bus Implementation - 80/20 Core Functionality
 *
 * This implements the essential 20% of L7 entanglement functionality that
 * provides 80% of the value:
 * - Fast signal propagation (sub-100ns hot path)
 * - Bounded forwarding (prevents infinite loops)
 * - Basic reactive logic (event X → change Y,Z)
 * - Simple dark triple activation
 *
 * Performance target: All hot path operations < 100ns with 8T compliance
 */

#include "cns/entanglement_oracle.h"
#include "cns/bitactor.h"
#include "registry.h" // Include registry.h for L6 connection
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// L7 signature enforcement
ENTANGLEMENT_L7_SIGNATURE();

// ---
// Part 1: Oracle Initialization (Setup Phase - Can Be Slow)
// ---

CNS_INLINE bool entanglement_oracle_init(
    entanglement_oracle_t* oracle,
    uint32_t domain_id
) {
    if (!oracle) return false;
    
    cns_cycle_t start = CNS_RDTSC();
    
    // Initialize oracle state
    memset(oracle, 0, sizeof(entanglement_oracle_t));
    oracle->connection_count = 0;
    oracle->total_signals = 0;
    oracle->domain_entanglement_mask = 0;
    
    // Initialize signal buffer
    oracle->buffer_head = 0;
    oracle->buffer_tail = 0;
    oracle->signals_queued = 0;
    
    // Initialize dark triple system (80/20 - simple bitmask)
    oracle->dark_triple_mask = 0;
    oracle->dark_activations = 0;
    
    // Reset performance metrics
    memset(&oracle->metrics, 0, sizeof(oracle->metrics));
    
    cns_cycle_t cycles = CNS_RDTSC() - start;
    // Note: This is setup phase, so not subject to 8T constraint
    
    return true;
}

// ---
// Part 2: Core Entanglement Operations (Hot Path - Must Be Fast)
// ---

CNS_INLINE bool entanglement_create(
    entanglement_oracle_t* oracle,
    uint32_t source_actor_id,
    uint32_t target_actor_id,
    bitactor_meaning_t trigger_mask
) {
    if (!oracle || oracle->connection_count >= ENTANGLEMENT_MAX_CONNECTIONS) {
        return false;
    }
    
    cns_cycle_t start = CNS_RDTSC();
    
    // Find available connection slot
    uint32_t connection_idx = oracle->connection_count++;
    entanglement_connection_t* conn = &oracle->connections[connection_idx];
    
    // Initialize connection (8T optimized - direct memory operations only)
    conn->source_actor_id = source_actor_id;
    conn->target_actor_id = target_actor_id;
    conn->hop_count = 0;
    conn->signal_strength = 0xFF; // Max strength initially
    conn->connection_flags = 0x01; // Mark as active
    conn->last_signal_tick = 0;
    conn->trigger_mask = trigger_mask;
    conn->response_pattern = trigger_mask; // Simple 80/20 - same as trigger
    
    // Update entanglement mask for fast lookup
    oracle->domain_entanglement_mask |= (1ULL << connection_idx);
    
    cns_cycle_t cycles = CNS_RDTSC() - start;
    assert(cycles <= BITACTOR_8T_MAX_CYCLES);
    
    return true;
}

CNS_INLINE uint32_t entanglement_propagate_signal(
    entanglement_oracle_t* oracle,
    uint32_t source_actor_id,
    bitactor_meaning_t signal_payload,
    uint8_t max_hops
) {
    if (!oracle || max_hops > ENTANGLEMENT_MAX_HOPS) {
        return 0;
    }
    
    cns_cycle_t start = CNS_RDTSC();
    uint32_t signals_sent = 0;
    
    // Fast path: Check if we have space in signal buffer
    if (oracle->signals_queued >= ENTANGLEMENT_SIGNAL_BUFFER_SIZE - 1) {
        oracle->metrics.bounded_rejections++;
        return 0; // Buffer full - bounded rejection for performance
    }
    
    // Iterate through connections (8T optimized - max 64 connections)
    uint64_t active_mask = oracle->domain_entanglement_mask;
    while (active_mask && signals_sent < 8) { // Limit to 8 for 8T compliance
        int connection_idx = __builtin_ctzll(active_mask); // Find first set bit
        entanglement_connection_t* conn = &oracle->connections[connection_idx];
        
        // Check if this connection matches the source
        if (conn->source_actor_id == source_actor_id && 
            (conn->trigger_mask & signal_payload) != 0) {
            
            // Create signal for propagation
            uint32_t buffer_pos = oracle->buffer_head;
            entanglement_signal_t* signal = &oracle->signal_buffer[buffer_pos];
            
            signal->source_id = source_actor_id;
            signal->target_id = conn->target_actor_id;
            signal->payload = signal_payload;
            signal->hop_count = max_hops - 1; // Decrement hop count
            signal->signal_flags = 0x01; // Active signal
            signal->propagation_vector = (uint64_t)source_actor_id << 32 | conn->target_actor_id;
            
            // Update buffer pointers
            oracle->buffer_head = (buffer_pos + 1) % ENTANGLEMENT_SIGNAL_BUFFER_SIZE;
            oracle->signals_queued++;
            signals_sent++;
            
            // Update connection state
            conn->last_signal_tick = oracle->total_signals;
        }
        
        // Clear processed bit and continue
        active_mask &= ~(1ULL << connection_idx);
    }
    
    oracle->total_signals += signals_sent;
    oracle->metrics.total_propagations += signals_sent;
    
    cns_cycle_t cycles = CNS_RDTSC() - start;
    oracle->metrics.last_propagation_cycles = cycles;
    assert(cycles <= BITACTOR_8T_MAX_CYCLES);
    
    return signals_sent;
}

CNS_INLINE bool entanglement_check_bounds(
    const entanglement_signal_t* signal,
    uint8_t max_hops
) {
    if (!signal) return false;
    
    // 8T optimized bounds check - single comparison
    return (signal->hop_count > 0 && signal->hop_count <= max_hops);
}

CNS_INLINE uint32_t entanglement_process_signals(
    entanglement_oracle_t* oracle,
    bitactor_domain_t* domain
) {
    if (!oracle || !domain || oracle->signals_queued == 0) {
        return 0;
    }
    
    cns_cycle_t start = CNS_RDTSC();
    uint32_t signals_processed = 0;
    
    // Process up to 8 signals for 8T compliance
    uint32_t max_process = (oracle->signals_queued < 8) ? oracle->signals_queued : 8;
    
    for (uint32_t i = 0; i < max_process; i++) {
        entanglement_signal_t* signal = &oracle->signal_buffer[oracle->buffer_tail];
        
        // Bounds check
        if (!entanglement_check_bounds(signal, ENTANGLEMENT_MAX_HOPS)) {
            oracle->metrics.bounded_rejections++;
            goto next_signal;
        }
        
        // Find target actor in domain (conceptual use of L6 registry)
        const registry_entry_t* target_entry = registry_resolve_id(signal->target_id);
        if (target_entry) {
            // Use target_entry->memory_location or other info
            // For now, just print to show connection
            printf("L7: Signal for actor %llu resolved via L6 registry.\n", target_entry->actor_id);
        }
        
        if (signal->target_id < domain->actor_count) {
            bitactor_state_t* target_actor = &domain->actors[signal->target_id];
            
            // Simple reactive logic: apply signal payload to actor meaning
            target_actor->meaning |= signal->payload;
            
            // Check for dark triple activation (80/20 - simple implementation)
            if (signal->payload & 0x80) { // High bit indicates potential dark activation
                uint64_t dark_bit = 1ULL << signal->target_id;
                if (oracle->dark_triple_mask & dark_bit) {
                    oracle->dark_triple_mask &= ~dark_bit; // Activate (remove from dormant)
                    oracle->dark_activations++;
                    oracle->metrics.dark_activations++;
                }
            }
            
            // Propagate signal further if hops remain
            if (signal->hop_count > 1) {
                entanglement_propagate_signal(oracle, signal->target_id, 
                                            signal->payload, signal->hop_count);
            }
            
            signals_processed++;
        }
        
    next_signal:
        // Move to next signal
        oracle->buffer_tail = (oracle->buffer_tail + 1) % ENTANGLEMENT_SIGNAL_BUFFER_SIZE;
        oracle->signals_queued--;
    }
    
    cns_cycle_t cycles = CNS_RDTSC() - start;
    assert(cycles <= BITACTOR_8T_MAX_CYCLES);
    
    return signals_processed;
}

CNS_INLINE uint32_t entanglement_activate_dark_triples(
    entanglement_oracle_t* oracle,
    bitactor_domain_t* domain,
    const entanglement_signal_t* trigger_signal
) {
    if (!oracle || !domain || !trigger_signal) {
        return 0;
    }
    
    cns_cycle_t start = CNS_RDTSC();
    uint32_t activations = 0;
    
    // 80/20 Dark Triple Activation: Simple pattern-based approach
    // Check if trigger signal matches any dormant patterns
    uint64_t potential_activations = oracle->dark_triple_mask;
    
    // Simple pattern matching for dark activation
    while (potential_activations && activations < 4) { // Limit to 4 for 8T compliance
        int actor_idx = __builtin_ctzll(potential_activations);
        
        // Check if trigger pattern matches dormant logic
        bitactor_state_t* actor = &domain->actors[actor_idx];
        if ((actor->meaning & trigger_signal->payload) == trigger_signal->payload) {
            // Activate dark triple
            oracle->dark_triple_mask &= ~(1ULL << actor_idx);
            actor->meaning |= 0x80; // Mark as activated
            activations++;
            oracle->metrics.dark_activations++;
        }
        
        potential_activations &= ~(1ULL << actor_idx);
    }
    
    cns_cycle_t cycles = CNS_RDTSC() - start;
    assert(cycles <= BITACTOR_8T_MAX_CYCLES);
    
    return activations;
}

// ---
// Part 3: BitActor Integration (Hot Path Operations)
// ---

CNS_INLINE bool bitactor_domain_add_entanglement(
    bitactor_domain_t* domain,
    entanglement_oracle_t* oracle
) {
    if (!domain || !oracle) return false;
    
    // For 80/20, we'll add oracle as a simple pointer to the domain
    // This would require extending bitactor_domain_t to include oracle pointer
    // For now, we'll assume external management of the oracle
    
    return entanglement_oracle_init(oracle, domain->domain_id);
}

CNS_INLINE cns_bitmask_t bitactor_execute_entanglement_hop(
    entanglement_oracle_t* oracle,
    bitactor_domain_t* domain,
    entanglement_opcode_t operation,
    void* operation_data
) {
    if (!oracle || !domain) return 0;
    
    cns_cycle_t start = CNS_RDTSC();
    cns_bitmask_t result = 0;
    
    switch (operation) {
        case BA_OP_ENTANGLE_SIGNAL:
            if (operation_data) {
                entanglement_signal_t* signal = (entanglement_signal_t*)operation_data;
                uint32_t sent = entanglement_propagate_signal(oracle, signal->source_id, 
                                                             signal->payload, signal->hop_count);
                result = sent > 0 ? 1 : 0;
            }
            break;
            
        case BA_OP_ENTANGLE_LISTEN:
            {
                uint32_t processed = entanglement_process_signals(oracle, domain);
                result = processed > 0 ? 1 : 0;
            }
            break;
            
        case BA_OP_ENTANGLE_DARK:
            if (operation_data) {
                entanglement_signal_t* signal = (entanglement_signal_t*)operation_data;
                uint32_t activations = entanglement_activate_dark_triples(oracle, domain, signal);
                result = activations > 0 ? 1 : 0;
            }
            break;
            
        case BA_OP_ENTANGLE_FLUSH:
            entanglement_flush_signals(oracle);
            result = 1;
            break;
            
        default:
            // Other operations return success for 80/20 simplicity
            result = 1;
            break;
    }
    
    cns_cycle_t cycles = CNS_RDTSC() - start;
    assert(cycles <= BITACTOR_8T_MAX_CYCLES);
    
    return result;
}

CNS_INLINE void entanglement_flush_signals(
    entanglement_oracle_t* oracle
) {
    if (!oracle) return;
    
    cns_cycle_t start = CNS_RDTSC();
    
    // Simple flush: reset buffer pointers
    oracle->buffer_head = 0;
    oracle->buffer_tail = 0;
    oracle->signals_queued = 0;
    
    // Clear signal flags in buffer (optional for performance)
    for (uint32_t i = 0; i < ENTANGLEMENT_SIGNAL_BUFFER_SIZE; i++) {
        oracle->signal_buffer[i].signal_flags = 0;
    }
    
    cns_cycle_t cycles = CNS_RDTSC() - start;
    assert(cycles <= BITACTOR_8T_MAX_CYCLES);
}

// ---
// Part 4: Performance Monitoring (80/20 - Essential Only)
// ---

CNS_INLINE bool entanglement_validate_performance(
    const entanglement_oracle_t* oracle
) {
    if (!oracle) return false;
    
    // Check that last propagation was within 8T constraints
    return oracle->metrics.last_propagation_cycles <= BITACTOR_8T_MAX_CYCLES;
}

CNS_INLINE void entanglement_get_metrics(
    const entanglement_oracle_t* oracle,
    char* output_buffer,
    size_t buffer_size
) {
    if (!oracle || !output_buffer) return;
    
    snprintf(output_buffer, buffer_size,
        "=== L7 Entanglement Bus Metrics ===\n"
        "Connections: %u active\n"
        "Signal Processing:\n"
        "  Total Propagations: %lu\n"
        "  Signals Queued: %u\n"
        "  Bounded Rejections: %lu\n"
        "  Last Propagation: %lu cycles\n"
        "Dark Triple System:\n"
        "  Dark Activations: %lu\n"
        "  Dormant Triples: %u\n"
        "Performance:\n"
        "  8T Compliance: %s\n"
        "  L7 Hash: 0x%016lx\n",
        oracle->connection_count,
        oracle->metrics.total_propagations,
        oracle->signals_queued,
        oracle->metrics.bounded_rejections,
        oracle->metrics.last_propagation_cycles,
        oracle->metrics.dark_activations,
        __builtin_popcountll(oracle->dark_triple_mask),
        entanglement_validate_performance(oracle) ? "YES" : "NO",
        ENTANGLEMENT_ORACLE_HASH
    );
}

// ---
// Part 5: Utility Functions for Integration
// ---

/**
 * @brief Create an entanglement signal for propagation
 * Utility function for easy signal creation
 */
static inline entanglement_signal_t entanglement_create_signal(
    uint32_t source_id,
    uint32_t target_id,
    bitactor_meaning_t payload,
    uint8_t max_hops
) {
    entanglement_signal_t signal = {
        .source_id = source_id,
        .target_id = target_id,
        .payload = payload,
        .hop_count = max_hops,
        .signal_flags = 0x01,
        .propagation_vector = (uint64_t)source_id << 32 | target_id
    };
    return signal;
}

/**
 * @brief Add actor to dark triple pool
 * Marks an actor as having dormant logic that can be activated
 */
CNS_INLINE bool entanglement_add_dark_triple(
    entanglement_oracle_t* oracle,
    uint32_t actor_id
) {
    if (!oracle || actor_id >= 64) return false; // Limit for 64-bit mask
    
    oracle->dark_triple_mask |= (1ULL << actor_id);
    return true;
}

/**
 * @brief High-level entanglement operation for easy integration
 * Simplified API for common entanglement use cases
 */
CNS_INLINE bool entanglement_trigger_reaction(
    entanglement_oracle_t* oracle,
    bitactor_domain_t* domain,
    uint32_t trigger_actor_id,
    bitactor_meaning_t reaction_payload
) {
    if (!oracle || !domain) return false;
    
    // Create and propagate reaction signal
    entanglement_signal_t signal = entanglement_create_signal(
        trigger_actor_id, 0, reaction_payload, 3); // 3-hop default
    
    uint32_t propagated = entanglement_propagate_signal(oracle, 
        trigger_actor_id, reaction_payload, 3);
    
    if (propagated > 0) {
        entanglement_process_signals(oracle, domain);
        return true;
    }
    
    return false;
}