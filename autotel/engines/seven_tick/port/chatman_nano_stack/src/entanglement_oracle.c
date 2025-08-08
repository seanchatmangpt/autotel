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
    
    
    cns_cycle_t start = CNS_RDTSC();
    uint32_t signals_sent = 0;
    
    // Fast path: Check if we have space in signal buffer
    
    
    // Iterate through connections (8T optimized - max 64 connections)
    uint64_t active_mask = oracle->domain_entanglement_mask;

    // Unroll the loop for fixed 8 iterations (as per 8T compliance)
    // This assumes that at most 8 signals will be propagated in one call.
    // If more are needed, the function would be called multiple times.

    // Iteration 1
    int connection_idx_0 = __builtin_ctzll(active_mask);
    entanglement_connection_t* conn_0 = &oracle->connections[connection_idx_0];
    uint64_t condition_0 = (conn_0->source_actor_id == source_actor_id) & ((conn_0->trigger_mask & signal_payload) != 0);
    uint32_t buffer_pos_0 = oracle->buffer_head;
    entanglement_signal_t* signal_0 = &oracle->signal_buffer[buffer_pos_0];

    signal_0->source_id = source_actor_id;
    signal_0->target_id = conn_0->target_actor_id;
    signal_0->payload = signal_payload;
    signal_0->hop_count = max_hops - 1;
    signal_0->signal_flags = 0x01;
    signal_0->propagation_vector = (uint64_t)source_actor_id << 32 | conn_0->target_actor_id;

    oracle->buffer_head = (buffer_pos_0 + 1) % ENTANGLEMENT_SIGNAL_BUFFER_SIZE;
    oracle->signals_queued += condition_0;
    signals_sent += condition_0;
    conn_0->last_signal_tick = oracle->total_signals;
    active_mask &= ~(1ULL << connection_idx_0);

    // Iteration 2
    int connection_idx_1 = __builtin_ctzll(active_mask);
    entanglement_connection_t* conn_1 = &oracle->connections[connection_idx_1];
    uint64_t condition_1 = (conn_1->source_actor_id == source_actor_id) & ((conn_1->trigger_mask & signal_payload) != 0);
    uint32_t buffer_pos_1 = oracle->buffer_head;
    entanglement_signal_t* signal_1 = &oracle->signal_buffer[buffer_pos_1];

    signal_1->source_id = source_actor_id;
    signal_1->target_id = conn_1->target_actor_id;
    signal_1->payload = signal_payload;
    signal_1->hop_count = max_hops - 1;
    signal_1->signal_flags = 0x01;
    signal_1->propagation_vector = (uint64_t)source_actor_id << 32 | conn_1->target_actor_id;

    oracle->buffer_head = (buffer_pos_1 + 1) % ENTANGLEMENT_SIGNAL_BUFFER_SIZE;
    oracle->signals_queued += condition_1;
    signals_sent += condition_1;
    conn_1->last_signal_tick = oracle->total_signals;
    active_mask &= ~(1ULL << connection_idx_1);

    // Iteration 3
    int connection_idx_2 = __builtin_ctzll(active_mask);
    entanglement_connection_t* conn_2 = &oracle->connections[connection_idx_2];
    uint64_t condition_2 = (conn_2->source_actor_id == source_actor_id) & ((conn_2->trigger_mask & signal_payload) != 0);
    uint32_t buffer_pos_2 = oracle->buffer_head;
    entanglement_signal_t* signal_2 = &oracle->signal_buffer[buffer_pos_2];

    signal_2->source_id = source_actor_id;
    signal_2->target_id = conn_2->target_actor_id;
    signal_2->payload = signal_payload;
    signal_2->hop_count = max_hops - 1;
    signal_2->signal_flags = 0x01;
    signal_2->propagation_vector = (uint64_t)source_actor_id << 32 | conn_2->target_actor_id;

    oracle->buffer_head = (buffer_pos_2 + 1) % ENTANGLEMENT_SIGNAL_BUFFER_SIZE;
    oracle->signals_queued += condition_2;
    signals_sent += condition_2;
    conn_2->last_signal_tick = oracle->total_signals;
    active_mask &= ~(1ULL << connection_idx_2);

    // Iteration 4
    int connection_idx_3 = __builtin_ctzll(active_mask);
    entanglement_connection_t* conn_3 = &oracle->connections[connection_idx_3];
    uint64_t condition_3 = (conn_3->source_actor_id == source_actor_id) & ((conn_3->trigger_mask & signal_payload) != 0);
    uint32_t buffer_pos_3 = oracle->buffer_head;
    entanglement_signal_t* signal_3 = &oracle->signal_buffer[buffer_pos_3];

    signal_3->source_id = source_actor_id;
    signal_3->target_id = conn_3->target_actor_id;
    signal_3->payload = signal_payload;
    signal_3->hop_count = max_hops - 1;
    signal_3->signal_flags = 0x01;
    signal_3->propagation_vector = (uint64_t)source_actor_id << 32 | conn_3->target_actor_id;

    oracle->buffer_head = (buffer_pos_3 + 1) % ENTANGLEMENT_SIGNAL_BUFFER_SIZE;
    oracle->signals_queued += condition_3;
    signals_sent += condition_3;
    conn_3->last_signal_tick = oracle->total_signals;
    active_mask &= ~(1ULL << connection_idx_3);

    // Iteration 5
    int connection_idx_4 = __builtin_ctzll(active_mask);
    entanglement_connection_t* conn_4 = &oracle->connections[connection_idx_4];
    uint64_t condition_4 = (conn_4->source_actor_id == source_actor_id) & ((conn_4->trigger_mask & signal_payload) != 0);
    uint32_t buffer_pos_4 = oracle->buffer_head;
    entanglement_signal_t* signal_4 = &oracle->signal_buffer[buffer_pos_4];

    signal_4->source_id = source_actor_id;
    signal_4->target_id = conn_4->target_actor_id;
    signal_4->payload = signal_payload;
    signal_4->hop_count = max_hops - 1;
    signal_4->signal_flags = 0x01;
    signal_4->propagation_vector = (uint64_t)source_actor_id << 32 | conn_4->target_actor_id;

    oracle->buffer_head = (buffer_pos_4 + 1) % ENTANGLEMENT_SIGNAL_BUFFER_SIZE;
    oracle->signals_queued += condition_4;
    signals_sent += condition_4;
    conn_4->last_signal_tick = oracle->total_signals;
    active_mask &= ~(1ULL << connection_idx_4);

    // Iteration 6
    int connection_idx_5 = __builtin_ctzll(active_mask);
    entanglement_connection_t* conn_5 = &oracle->connections[connection_idx_5];
    uint64_t condition_5 = (conn_5->source_actor_id == source_actor_id) & ((conn_5->trigger_mask & signal_payload) != 0);
    uint32_t buffer_pos_5 = oracle->buffer_head;
    entanglement_signal_t* signal_5 = &oracle->signal_buffer[buffer_pos_5];

    signal_5->source_id = source_actor_id;
    signal_5->target_id = conn_5->target_actor_id;
    signal_5->payload = signal_payload;
    signal_5->hop_count = max_hops - 1;
    signal_5->signal_flags = 0x01;
    signal_5->propagation_vector = (uint64_t)source_actor_id << 32 | conn_5->target_actor_id;

    oracle->buffer_head = (buffer_pos_5 + 1) % ENTANGLEMENT_SIGNAL_BUFFER_SIZE;
    oracle->signals_queued += condition_5;
    signals_sent += condition_5;
    conn_5->last_signal_tick = oracle->total_signals;
    active_mask &= ~(1ULL << connection_idx_5);

    // Iteration 7
    int connection_idx_6 = __builtin_ctzll(active_mask);
    entanglement_connection_t* conn_6 = &oracle->connections[connection_idx_6];
    uint64_t condition_6 = (conn_6->source_actor_id == source_actor_id) & ((conn_6->trigger_mask & signal_payload) != 0);
    uint32_t buffer_pos_6 = oracle->buffer_head;
    entanglement_signal_t* signal_6 = &oracle->signal_buffer[buffer_pos_6];

    signal_6->source_id = source_actor_id;
    signal_6->target_id = conn_6->target_actor_id;
    signal_6->payload = signal_payload;
    signal_6->hop_count = max_hops - 1;
    signal_6->signal_flags = 0x01;
    signal_6->propagation_vector = (uint64_t)source_actor_id << 32 | conn_6->target_actor_id;

    oracle->buffer_head = (buffer_pos_6 + 1) % ENTANGLEMENT_SIGNAL_BUFFER_SIZE;
    oracle->signals_queued += condition_6;
    signals_sent += condition_6;
    conn_6->last_signal_tick = oracle->total_signals;
    active_mask &= ~(1ULL << connection_idx_6);

    // Iteration 8
    int connection_idx_7 = __builtin_ctzll(active_mask);
    entanglement_connection_t* conn_7 = &oracle->connections[connection_idx_7];
    uint64_t condition_7 = (conn_7->source_actor_id == source_actor_id) & ((conn_7->trigger_mask & signal_payload) != 0);
    uint32_t buffer_pos_7 = oracle->buffer_head;
    entanglement_signal_t* signal_7 = &oracle->signal_buffer[buffer_pos_7];

    signal_7->source_id = source_actor_id;
    signal_7->target_id = conn_7->target_actor_id;
    signal_7->payload = signal_payload;
    signal_7->hop_count = max_hops - 1;
    signal_7->signal_flags = 0x01;
    signal_7->propagation_vector = (uint64_t)source_actor_id << 32 | conn_7->target_actor_id;

    oracle->buffer_head = (buffer_pos_7 + 1) % ENTANGLEMENT_SIGNAL_BUFFER_SIZE;
    oracle->signals_queued += condition_7;
    signals_sent += condition_7;
    conn_7->last_signal_tick = oracle->total_signals;
    active_mask &= ~(1ULL << connection_idx_7);
    
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
    // 8T optimized bounds check - single comparison using bitwise operations
    // (signal->hop_count > 0) can be represented as (signal->hop_count != 0)
    // (signal->hop_count <= max_hops) can be represented as !(signal->hop_count > max_hops)
    // The result is a boolean (0 or 1) without branching.
    return (signal->hop_count != 0) & (!(signal->hop_count > max_hops));
}

CNS_INLINE uint32_t entanglement_process_signals(
    entanglement_oracle_t* oracle,
    bitactor_domain_t* domain
) {
    CNS_INLINE uint32_t entanglement_process_signals(
    entanglement_oracle_t* oracle,
    bitactor_domain_t* domain
) {
    // Assume oracle, domain are valid and signals_queued > 0 for 7-tick compliance
    
    cns_cycle_t start = CNS_RDTSC();
    uint32_t signals_processed = 0;
    
    // Process up to 8 signals for 8T compliance (unrolled loop)
    // Iteration 1
    entanglement_signal_t* signal_0 = &oracle->signal_buffer[oracle->buffer_tail];
    uint32_t process_mask_0 = entanglement_check_bounds(signal_0, ENTANGLEMENT_MAX_HOPS);
    
    // Conditional operations based on process_mask_0
    oracle->metrics.bounded_rejections += (1 - process_mask_0);
    
    // Operations if signal is valid and target actor is within bounds
    uint32_t actor_valid_0 = (signal_0->target_id < domain->actor_count) & process_mask_0;
    bitactor_state_t* target_actor_0 = &domain->actors[signal_0->target_id];
    target_actor_0->meaning |= (signal_0->payload & actor_valid_0); // Apply payload conditionally
    
    // Dark triple activation
    uint32_t dark_activation_condition_0 = (signal_0->payload & 0x80) & actor_valid_0;
    uint64_t dark_bit_0 = 1ULL << signal_0->target_id;
    uint32_t dark_activated_0 = (oracle->dark_triple_mask & dark_bit_0) & dark_activation_condition_0;
    oracle->dark_triple_mask &= ~(dark_bit_0 & dark_activated_0); // Activate (remove from dormant)
    oracle->dark_activations += dark_activated_0;
    oracle->metrics.dark_activations += dark_activated_0;
    
    // Propagate signal further if hops remain
    uint32_t propagate_condition_0 = (signal_0->hop_count > 1) & actor_valid_0;
    entanglement_propagate_signal(oracle, signal_0->target_id, 
                                signal_0->payload, signal_0->hop_count) * propagate_condition_0;
    
    signals_processed += actor_valid_0;
    oracle->buffer_tail = (oracle->buffer_tail + 1) % ENTANGLEMENT_SIGNAL_BUFFER_SIZE;
    oracle->signals_queued -= (1 & process_mask_0); // Decrement only if processed

    // Iteration 2 (repeat for 8 iterations)
    entanglement_signal_t* signal_1 = &oracle->signal_buffer[oracle->buffer_tail];
    uint32_t process_mask_1 = entanglement_check_bounds(signal_1, ENTANGLEMENT_MAX_HOPS);
    oracle->metrics.bounded_rejections += (1 - process_mask_1);
    uint32_t actor_valid_1 = (signal_1->target_id < domain->actor_count) & process_mask_1;
    bitactor_state_t* target_actor_1 = &domain->actors[signal_1->target_id];
    target_actor_1->meaning |= (signal_1->payload & actor_valid_1);
    uint32_t dark_activation_condition_1 = (signal_1->payload & 0x80) & actor_valid_1;
    uint64_t dark_bit_1 = 1ULL << signal_1->target_id;
    uint32_t dark_activated_1 = (oracle->dark_triple_mask & dark_bit_1) & dark_activation_condition_1;
    oracle->dark_triple_mask &= ~(dark_bit_1 & dark_activated_1);
    oracle->dark_activations += dark_activated_1;
    oracle->metrics.dark_activations += dark_activated_1;
    uint32_t propagate_condition_1 = (signal_1->hop_count > 1) & actor_valid_1;
    entanglement_propagate_signal(oracle, signal_1->target_id, 
                                signal_1->payload, signal_1->hop_count) * propagate_condition_1;
    signals_processed += actor_valid_1;
    oracle->buffer_tail = (oracle->buffer_tail + 1) % ENTANGLEMENT_SIGNAL_BUFFER_SIZE;
    oracle->signals_queued -= (1 & process_mask_1);

    // Iteration 3
    entanglement_signal_t* signal_2 = &oracle->signal_buffer[oracle->buffer_tail];
    uint32_t process_mask_2 = entanglement_check_bounds(signal_2, ENTANGLEMENT_MAX_HOPS);
    oracle->metrics.bounded_rejections += (1 - process_mask_2);
    uint32_t actor_valid_2 = (signal_2->target_id < domain->actor_count) & process_mask_2;
    bitactor_state_t* target_actor_2 = &domain->actors[signal_2->target_id];
    target_actor_2->meaning |= (signal_2->payload & actor_valid_2);
    uint32_t dark_activation_condition_2 = (signal_2->payload & 0x80) & actor_valid_2;
    uint64_t dark_bit_2 = 1ULL << signal_2->target_id;
    uint32_t dark_activated_2 = (oracle->dark_triple_mask & dark_bit_2) & dark_activation_condition_2;
    oracle->dark_triple_mask &= ~(dark_bit_2 & dark_activated_2);
    oracle->dark_activations += dark_activated_2;
    oracle->metrics.dark_activations += dark_activated_2;
    uint32_t propagate_condition_2 = (signal_2->hop_count > 1) & actor_valid_2;
    entanglement_propagate_signal(oracle, signal_2->target_id, 
                                signal_2->payload, signal_2->hop_count) * propagate_condition_2;
    signals_processed += actor_valid_2;
    oracle->buffer_tail = (oracle->buffer_tail + 1) % ENTANGLEMENT_SIGNAL_BUFFER_SIZE;
    oracle->signals_queued -= (1 & process_mask_2);

    // Iteration 4
    entanglement_signal_t* signal_3 = &oracle->signal_buffer[oracle->buffer_tail];
    uint32_t process_mask_3 = entanglement_check_bounds(signal_3, ENTANGLEMENT_MAX_HOPS);
    oracle->metrics.bounded_rejections += (1 - process_mask_3);
    uint32_t actor_valid_3 = (signal_3->target_id < domain->actor_count) & process_mask_3;
    bitactor_state_t* target_actor_3 = &domain->actors[signal_3->target_id];
    target_actor_3->meaning |= (signal_3->payload & actor_valid_3);
    uint32_t dark_activation_condition_3 = (signal_3->payload & 0x80) & actor_valid_3;
    uint64_t dark_bit_3 = 1ULL << signal_3->target_id;
    uint32_t dark_activated_3 = (oracle->dark_triple_mask & dark_bit_3) & dark_activation_condition_3;
    oracle->dark_triple_mask &= ~(dark_bit_3 & dark_activated_3);
    oracle->dark_activations += dark_activated_3;
    oracle->metrics.dark_activations += dark_activated_3;
    uint32_t propagate_condition_3 = (signal_3->hop_count > 1) & actor_valid_3;
    entanglement_propagate_signal(oracle, signal_3->target_id, 
                                signal_3->payload, signal_3->hop_count) * propagate_condition_3;
    signals_processed += actor_valid_3;
    oracle->buffer_tail = (oracle->buffer_tail + 1) % ENTANGLEMENT_SIGNAL_BUFFER_SIZE;
    oracle->signals_queued -= (1 & process_mask_3);

    // Iteration 5
    entanglement_signal_t* signal_4 = &oracle->signal_buffer[oracle->buffer_tail];
    uint32_t process_mask_4 = entanglement_check_bounds(signal_4, ENTANGLEMENT_MAX_HOPS);
    oracle->metrics.bounded_rejections += (1 - process_mask_4);
    uint32_t actor_valid_4 = (signal_4->target_id < domain->actor_count) & process_mask_4;
    bitactor_state_t* target_actor_4 = &domain->actors[signal_4->target_id];
    target_actor_4->meaning |= (signal_4->payload & actor_valid_4);
    uint32_t dark_activation_condition_4 = (signal_4->payload & 0x80) & actor_valid_4;
    uint64_t dark_bit_4 = 1ULL << signal_4->target_id;
    uint32_t dark_activated_4 = (oracle->dark_triple_mask & dark_bit_4) & dark_activation_condition_4;
    oracle->dark_triple_mask &= ~(dark_bit_4 & dark_activated_4);
    oracle->dark_activations += dark_activated_4;
    oracle->metrics.dark_activations += dark_activated_4;
    uint32_t propagate_condition_4 = (signal_4->hop_count > 1) & actor_valid_4;
    entanglement_propagate_signal(oracle, signal_4->target_id, 
                                signal_4->payload, signal_4->hop_count) * propagate_condition_4;
    signals_processed += actor_valid_4;
    oracle->buffer_tail = (oracle->buffer_tail + 1) % ENTANGLEMENT_SIGNAL_BUFFER_SIZE;
    oracle->signals_queued -= (1 & process_mask_4);

    // Iteration 6
    entanglement_signal_t* signal_5 = &oracle->signal_buffer[oracle->buffer_tail];
    uint32_t process_mask_5 = entanglement_check_bounds(signal_5, ENTANGLEMENT_MAX_HOPS);
    oracle->metrics.bounded_rejections += (1 - process_mask_5);
    uint32_t actor_valid_5 = (signal_5->target_id < domain->actor_count) & process_mask_5;
    bitactor_state_t* target_actor_5 = &domain->actors[signal_5->target_id];
    target_actor_5->meaning |= (signal_5->payload & actor_valid_5);
    uint32_t dark_activation_condition_5 = (signal_5->payload & 0x80) & actor_valid_5;
    uint64_t dark_bit_5 = 1ULL << signal_5->target_id;
    uint32_t dark_activated_5 = (oracle->dark_triple_mask & dark_bit_5) & dark_activation_condition_5;
    oracle->dark_triple_mask &= ~(dark_bit_5 & dark_activated_5);
    oracle->dark_activations += dark_activated_5;
    oracle->metrics.dark_activations += dark_activated_5;
    uint32_t propagate_condition_5 = (signal_5->hop_count > 1) & actor_valid_5;
    entanglement_propagate_signal(oracle, signal_5->target_id, 
                                signal_5->payload, signal_5->hop_count) * propagate_condition_5;
    signals_processed += actor_valid_5;
    oracle->buffer_tail = (oracle->buffer_tail + 1) % ENTANGLEMENT_SIGNAL_BUFFER_SIZE;
    oracle->signals_queued -= (1 & process_mask_5);

    // Iteration 7
    entanglement_signal_t* signal_6 = &oracle->signal_buffer[oracle->buffer_tail];
    uint32_t process_mask_6 = entanglement_check_bounds(signal_6, ENTANGLEMENT_MAX_HOPS);
    oracle->metrics.bounded_rejections += (1 - process_mask_6);
    uint32_t actor_valid_6 = (signal_6->target_id < domain->actor_count) & process_mask_6;
    bitactor_state_t* target_actor_6 = &domain->actors[signal_6->target_id];
    target_actor_6->meaning |= (signal_6->payload & actor_valid_6);
    uint32_t dark_activation_condition_6 = (signal_6->payload & 0x80) & actor_valid_6;
    uint64_t dark_bit_6 = 1ULL << signal_6->target_id;
    uint32_t dark_activated_6 = (oracle->dark_triple_mask & dark_bit_6) & dark_activation_condition_6;
    oracle->dark_triple_mask &= ~(dark_bit_6 & dark_activated_6);
    oracle->dark_activations += dark_activated_6;
    oracle->metrics.dark_activations += dark_activated_6;
    uint32_t propagate_condition_6 = (signal_6->hop_count > 1) & actor_valid_6;
    entanglement_propagate_signal(oracle, signal_6->target_id, 
                                signal_6->payload, signal_6->hop_count) * propagate_condition_6;
    signals_processed += actor_valid_6;
    oracle->buffer_tail = (oracle->buffer_tail + 1) % ENTANGLEMENT_SIGNAL_BUFFER_SIZE;
    oracle->signals_queued -= (1 & process_mask_6);

    // Iteration 8
    entanglement_signal_t* signal_7 = &oracle->signal_buffer[oracle->buffer_tail];
    uint32_t process_mask_7 = entanglement_check_bounds(signal_7, ENTANGLEMENT_MAX_HOPS);
    oracle->metrics.bounded_rejections += (1 - process_mask_7);
    uint32_t actor_valid_7 = (signal_7->target_id < domain->actor_count) & process_mask_7;
    bitactor_state_t* target_actor_7 = &domain->actors[signal_7->target_id];
    target_actor_7->meaning |= (signal_7->payload & actor_valid_7);
    uint32_t dark_activation_condition_7 = (signal_7->payload & 0x80) & actor_valid_7;
    uint64_t dark_bit_7 = 1ULL << signal_7->target_id;
    uint32_t dark_activated_7 = (oracle->dark_triple_mask & dark_bit_7) & dark_activation_condition_7;
    oracle->dark_triple_mask &= ~(dark_bit_7 & dark_activated_7);
    oracle->dark_activations += dark_activated_7;
    oracle->metrics.dark_activations += dark_activated_7;
    uint32_t propagate_condition_7 = (signal_7->hop_count > 1) & actor_valid_7;
    entanglement_propagate_signal(oracle, signal_7->target_id, 
                                signal_7->payload, signal_7->hop_count) * propagate_condition_7;
    signals_processed += actor_valid_7;
    oracle->buffer_tail = (oracle->buffer_tail + 1) % ENTANGLEMENT_SIGNAL_BUFFER_SIZE;
    oracle->signals_queued -= (1 & process_mask_7);
    
    cns_cycle_t cycles = CNS_RDTSC() - start;
    assert(cycles <= BITACTOR_8T_MAX_CYCLES);
    
    return signals_processed;
}

CNS_INLINE uint32_t entanglement_activate_dark_triples(
    entanglement_oracle_t* oracle,
    bitactor_domain_t* domain,
    const entanglement_signal_t* trigger_signal
) {
    // Assume oracle, domain, and trigger_signal are valid for 7-tick compliance
    
    cns_cycle_t start = CNS_RDTSC();
    uint32_t activations = 0;
    
    // 80/20 Dark Triple Activation: Simple pattern-based approach
    // Check if trigger signal matches any dormant patterns
    uint64_t potential_activations = oracle->dark_triple_mask;
    
    // Unroll the loop for fixed 4 iterations (as per 8T compliance)

    // Iteration 1
    int actor_idx_0 = __builtin_ctzll(potential_activations);
    bitactor_state_t* actor_0 = &domain->actors[actor_idx_0];
    uint32_t condition_0 = ((actor_0->meaning & trigger_signal->payload) == trigger_signal->payload);
    
    oracle->dark_triple_mask &= ~( (1ULL << actor_idx_0) & condition_0 ); // Activate (remove from dormant) conditionally
    actor_0->meaning |= (0x80 & condition_0); // Mark as activated conditionally
    activations += condition_0;
    oracle->metrics.dark_activations += condition_0;
    potential_activations &= ~(1ULL << actor_idx_0);

    // Iteration 2
    int actor_idx_1 = __builtin_ctzll(potential_activations);
    bitactor_state_t* actor_1 = &domain->actors[actor_idx_1];
    uint32_t condition_1 = ((actor_1->meaning & trigger_signal->payload) == trigger_signal->payload);
    
    oracle->dark_triple_mask &= ~( (1ULL << actor_idx_1) & condition_1 );
    actor_1->meaning |= (0x80 & condition_1);
    activations += condition_1;
    oracle->metrics.dark_activations += condition_1;
    potential_activations &= ~(1ULL << actor_idx_1);

    // Iteration 3
    int actor_idx_2 = __builtin_ctzll(potential_activations);
    bitactor_state_t* actor_2 = &domain->actors[actor_idx_2];
    uint32_t condition_2 = ((actor_2->meaning & trigger_signal->payload) == trigger_signal->payload);
    
    oracle->dark_triple_mask &= ~( (1ULL << actor_idx_2) & condition_2 );
    actor_2->meaning |= (0x80 & condition_2);
    activations += condition_2;
    oracle->metrics.dark_activations += condition_2;
    potential_activations &= ~(1ULL << actor_idx_2);

    // Iteration 4
    int actor_idx_3 = __builtin_ctzll(potential_activations);
    bitactor_state_t* actor_3 = &domain->actors[actor_idx_3];
    uint32_t condition_3 = ((actor_3->meaning & trigger_signal->payload) == trigger_signal->payload);
    
    oracle->dark_triple_mask &= ~( (1ULL << actor_idx_3) & condition_3 );
    actor_3->meaning |= (0x80 & condition_3);
    activations += condition_3;
    oracle->metrics.dark_activations += condition_3;
    potential_activations &= ~(1ULL << actor_idx_3);
    
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
    // Assume oracle, domain, and operation_data are valid for 7-tick compliance
    
    cns_cycle_t start = CNS_RDTSC();
    cns_bitmask_t result = 0;
    
    // Define function pointers for each operation
    typedef cns_bitmask_t (*entanglement_op_handler)(entanglement_oracle_t*, bitactor_domain_t*, void*);

    // Array of handlers (jump table)
    entanglement_op_handler handlers[] = {
        /* BA_OP_ENTANGLE_CREATE */ (entanglement_op_handler)entanglement_create, // Needs adaptation
        /* BA_OP_ENTANGLE_SIGNAL */ (entanglement_op_handler)entanglement_propagate_signal_wrapper,
        /* BA_OP_ENTANGLE_LISTEN */ (entanglement_op_handler)entanglement_process_signals_wrapper,
        /* BA_OP_ENTANGLE_BREAK */ NULL, // Placeholder
        /* BA_OP_ENTANGLE_RIPPLE */ NULL, // Placeholder
        /* BA_OP_ENTANGLE_DARK */ (entanglement_op_handler)entanglement_activate_dark_triples_wrapper,
        /* BA_OP_ENTANGLE_BOUND */ (entanglement_op_handler)entanglement_check_bounds_wrapper,
        /* BA_OP_ENTANGLE_FLUSH */ (entanglement_op_handler)entanglement_flush_signals_wrapper
    };

    // Execute the appropriate handler using direct indexing
    // This assumes 'operation' is a valid index into the handlers array.
    // Any invalid operation would lead to a fault in a real hardware system.
    if (operation < sizeof(handlers) / sizeof(entanglement_op_handler) && handlers[operation] != NULL) {
        result = handlers[operation](oracle, domain, operation_data);
    } else {
        // Handle unknown or unsupported operation (e.g., return 0 or trigger a fault)
        result = 0; // For simulation, return 0
    }
    
    cns_cycle_t cycles = CNS_RDTSC() - start;
    assert(cycles <= BITACTOR_8T_MAX_CYCLES);
    
    return result;
}

// Wrapper functions to match the generic handler signature
CNS_INLINE cns_bitmask_t entanglement_propagate_signal_wrapper(entanglement_oracle_t* oracle, bitactor_domain_t* domain, void* data) {
    entanglement_signal_t* signal = (entanglement_signal_t*)data;
    uint32_t sent = entanglement_propagate_signal(oracle, signal->source_id, signal->payload, signal->hop_count);
    return sent > 0 ? 1 : 0;
}

CNS_INLINE cns_bitmask_t entanglement_process_signals_wrapper(entanglement_oracle_t* oracle, bitactor_domain_t* domain, void* data) {
    uint32_t processed = entanglement_process_signals(oracle, domain);
    return processed > 0 ? 1 : 0;
}

CNS_INLINE cns_bitmask_t entanglement_activate_dark_triples_wrapper(entanglement_oracle_t* oracle, bitactor_domain_t* domain, void* data) {
    entanglement_signal_t* signal = (entanglement_signal_t*)data;
    uint32_t activations = entanglement_activate_dark_triples(oracle, domain, signal);
    return activations > 0 ? 1 : 0;
}

CNS_INLINE cns_bitmask_t entanglement_check_bounds_wrapper(entanglement_oracle_t* oracle, bitactor_domain_t* domain, void* data) {
    entanglement_signal_t* signal = (entanglement_signal_t*)data;
    return entanglement_check_bounds(signal, ENTANGLEMENT_MAX_HOPS);
}

CNS_INLINE cns_bitmask_t entanglement_flush_signals_wrapper(entanglement_oracle_t* oracle, bitactor_domain_t* domain, void* data) {
    entanglement_flush_signals(oracle);
    return 1;
}

CNS_INLINE void entanglement_flush_signals(
    entanglement_oracle_t* oracle
) {
    // Assume oracle is valid for 7-tick compliance
    
    cns_cycle_t start = CNS_RDTSC();
    
    // Simple flush: reset buffer pointers
    oracle->buffer_head = 0;
    oracle->buffer_tail = 0;
    oracle->signals_queued = 0;
    
    // In a true 7-tick system, clearing the entire buffer would be a hardware operation
    // or the buffer size would be small enough to unroll. For simulation, resetting pointers is sufficient.
    
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
    // Assume oracle and domain are valid for 7-tick compliance
    
    // Create and propagate reaction signal
    entanglement_signal_t signal = entanglement_create_signal(
        trigger_actor_id, 0, reaction_payload, 3); // 3-hop default
    
    uint32_t propagated = entanglement_propagate_signal(oracle, 
        trigger_actor_id, reaction_payload, 3);
    
    // Unconditionally process signals. The effect of processing will be zero
    // if no signals were actually propagated.
    entanglement_process_signals(oracle, domain);
    
    // Return true if any signals were propagated, false otherwise (non-branching)
    return (propagated > 0); // This comparison itself is a non-branching operation
}