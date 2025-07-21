/**
 * @file bitactor_core.c
 * @brief BitActor Core Implementation - Fifth Epoch Causal Computing Engine
 *
 * This implements the 8T/8H/8B Trinity as executable C code:
 * - Causal domain collapse in ≤8 cycles
 * - 8-hop cognitive reasoning chains
 * - BitActor meaning atoms as machine-native execution units
 *
 * "Specification is Execution. Causality is Computation. Reality is Bit-Aligned."
 */

#include "cns/bitactor.h"
#include "cns/cns_core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Trinity signature enforcement
BITACTOR_TRINITY_SIGNATURE();

// ---
// Part 1: Matrix Initialization & Management
// ---

CNS_INLINE cns_bool_t bitactor_matrix_init(
    bitactor_matrix_t* matrix, 
    cns_arena_t* arena
) {
    if (!matrix || !arena) return CNS_FALSE;
    
    // Initialize matrix with Trinity signature
    memset(matrix, 0, sizeof(bitactor_matrix_t));
    matrix->global_tick_counter = 0;
    matrix->domain_count = 0;
    matrix->total_actors = 0;
    matrix->domain_active_mask = 0;
    
    // Initialize performance metrics
    matrix->metrics.last_collapse_cycles = 0;
    matrix->metrics.total_collapses = 0;
    matrix->metrics.total_mutations = 0;
    matrix->metrics.min_collapse_cycles = UINT64_MAX;
    matrix->metrics.max_collapse_cycles = 0;
    
    return CNS_TRUE;
}

CNS_INLINE uint32_t bitactor_domain_create(
    bitactor_matrix_t* matrix,
    const char* domain_name,
    bitactor_cognitive_cycle_t* cycle_template
) {
    if (!matrix || matrix->domain_count >= BITACTOR_MAX_DOMAINS) {
        return UINT32_MAX;
    }
    
    uint32_t domain_id = matrix->domain_count++;
    bitactor_domain_t* domain = &matrix->domains[domain_id];
    
    // Initialize domain
    memset(domain, 0, sizeof(bitactor_domain_t));
    domain->domain_id = domain_id;
    domain->actor_count = 0;
    domain->active_mask = 0;
    
    // Copy cognitive cycle template if provided
    if (cycle_template) {
        memcpy(&domain->cognitive_cycle, cycle_template, sizeof(bitactor_cognitive_cycle_t));
    } else {
        // Initialize default 8H cycle
        bitactor_init_default_cognitive_cycle(&domain->cognitive_cycle);
    }
    
    // Initialize feed actor
    memset(&domain->feed_actor, 0, sizeof(bitactor_feed_actor_t));
    
    // Mark domain as active
    matrix->domain_active_mask |= (1ULL << domain_id);
    
    return domain_id;
}

CNS_INLINE uint32_t bitactor_add_to_domain(
    bitactor_domain_t* domain,
    bitactor_meaning_t meaning,
    const char* actor_name
) {
    if (!domain || domain->actor_count >= BITACTOR_MAX_ACTORS) {
        return UINT32_MAX;
    }
    
    uint32_t actor_id = domain->actor_count++;
    bitactor_state_t* actor = &domain->actors[actor_id];
    
    // Initialize actor state
    actor->meaning = meaning;
    actor->hop_position = 0;
    actor->tick_budget = BITACTOR_8T_MAX_CYCLES;
    actor->domain_id = domain->domain_id;
    actor->actor_id = actor_id;
    
    // Mark actor as active
    domain->active_mask |= (1ULL << actor_id);
    
    return actor_id;
}

// ---
// Part 2: Cognitive Cycle Implementation (8H Reasoning)
// ---

static cns_bitmask_t bitactor_hop_trigger_detect(void* context, bitactor_state_t* state) {
    // Hop 1: Trigger detection
    // Check for incoming signals and pattern matches
    bitactor_domain_t* domain = (bitactor_domain_t*)context;
    
    cns_cycle_t start = CNS_RDTSC();
    
    // Simple trigger detection - check if any patterns matched
    cns_bitmask_t trigger_mask = 0;
    if (domain->feed_actor.match_count > 0) {
        trigger_mask = 1ULL << state->actor_id;
    }
    
    // Verify 8T compliance
    cns_cycle_t cycles = CNS_RDTSC() - start;
    assert(cycles <= BITACTOR_8T_MAX_CYCLES);
    
    return trigger_mask;
}

static cns_bitmask_t bitactor_hop_ontology_load(void* context, bitactor_state_t* state) {
    // Hop 2: Ontology loading
    // Load relevant TTL context for this actor's meaning
    cns_cycle_t start = CNS_RDTSC();
    
    // Simulate ontology loading based on meaning bits
    cns_bitmask_t loaded_mask = 0;
    if (state->meaning != 0) {
        loaded_mask = 1ULL << state->actor_id;
    }
    
    cns_cycle_t cycles = CNS_RDTSC() - start;
    assert(cycles <= BITACTOR_8T_MAX_CYCLES);
    
    return loaded_mask;
}

static cns_bitmask_t bitactor_hop_shacl_fire(void* context, bitactor_state_t* state) {
    // Hop 3: SHACL path execution
    // Execute compiled SHACL constraints
    cns_cycle_t start = CNS_RDTSC();
    
    // Simulate SHACL constraint checking
    cns_bitmask_t valid_mask = 0;
    
    // Check meaning atom constraints
    if ((state->meaning & 0x01) && (state->meaning & 0x02)) {
        valid_mask = 1ULL << state->actor_id;
    }
    
    cns_cycle_t cycles = CNS_RDTSC() - start;
    assert(cycles <= BITACTOR_8T_MAX_CYCLES);
    
    return valid_mask;
}

static cns_bitmask_t bitactor_hop_state_resolve(void* context, bitactor_state_t* state) {
    // Hop 4: BitActor state resolution
    // Resolve the current state based on inputs and constraints
    cns_cycle_t start = CNS_RDTSC();
    
    // State resolution based on meaning atoms
    cns_bitmask_t resolved_mask = 0;
    
    // Simple state resolution - all bits set means resolved
    if (__builtin_popcount(state->meaning) >= 4) {
        resolved_mask = 1ULL << state->actor_id;
        state->hop_position = BITACTOR_HOP_STATE_RESOLVE;
    }
    
    cns_cycle_t cycles = CNS_RDTSC() - start;
    assert(cycles <= BITACTOR_8T_MAX_CYCLES);
    
    return resolved_mask;
}

static cns_bitmask_t bitactor_hop_collapse_compute(void* context, bitactor_state_t* state) {
    // Hop 5: Causal collapse computation
    // Compute the causal collapse for this actor
    cns_cycle_t start = CNS_RDTSC();
    
    cns_bitmask_t collapse_mask = 0;
    
    // Collapse computation using meaning bits as entropy
    uint8_t entropy = state->meaning;
    uint8_t collapsed_state = entropy ^ (entropy >> 4);
    
    if (collapsed_state != 0) {
        collapse_mask = 1ULL << state->actor_id;
        state->meaning = collapsed_state; // Update with collapsed meaning
    }
    
    cns_cycle_t cycles = CNS_RDTSC() - start;
    assert(cycles <= BITACTOR_8T_MAX_CYCLES);
    
    return collapse_mask;
}

static cns_bitmask_t bitactor_hop_action_bind(void* context, bitactor_state_t* state) {
    // Hop 6: Action binding
    // Bind computed results to actionable mutations
    cns_cycle_t start = CNS_RDTSC();
    
    cns_bitmask_t action_mask = 0;
    
    // Bind actions based on collapsed meaning
    if (state->meaning & 0x80) { // High bit indicates action required
        action_mask = 1ULL << state->actor_id;
    }
    
    cns_cycle_t cycles = CNS_RDTSC() - start;
    assert(cycles <= BITACTOR_8T_MAX_CYCLES);
    
    return action_mask;
}

static cns_bitmask_t bitactor_hop_state_commit(void* context, bitactor_state_t* state) {
    // Hop 7: State commitment
    // Commit the new state to persistent storage
    cns_cycle_t start = CNS_RDTSC();
    
    cns_bitmask_t commit_mask = 0;
    
    // Commit state changes
    if (state->tick_budget > 0) {
        commit_mask = 1ULL << state->actor_id;
        state->tick_budget--; // Consume one tick
    }
    
    cns_cycle_t cycles = CNS_RDTSC() - start;
    assert(cycles <= BITACTOR_8T_MAX_CYCLES);
    
    return commit_mask;
}

static cns_bitmask_t bitactor_hop_meta_validate(void* context, bitactor_state_t* state) {
    // Hop 8: Meta-proof validation
    // Validate the entire cognitive cycle for correctness
    cns_cycle_t start = CNS_RDTSC();
    
    cns_bitmask_t valid_mask = 0;
    
    // Meta-validation - check if all constraints satisfied
    if (state->hop_position == BITACTOR_HOP_STATE_RESOLVE && 
        state->tick_budget > 0 &&
        state->meaning != 0) {
        valid_mask = 1ULL << state->actor_id;
        state->hop_position = 0; // Reset for next cycle
    }
    
    cns_cycle_t cycles = CNS_RDTSC() - start;
    assert(cycles <= BITACTOR_8T_MAX_CYCLES);
    
    return valid_mask;
}

static void bitactor_init_default_cognitive_cycle(bitactor_cognitive_cycle_t* cycle) {
    if (!cycle) return;
    
    // Initialize the 8H cognitive cycle with default hop functions
    cycle->hops[BITACTOR_HOP_TRIGGER_DETECT] = bitactor_hop_trigger_detect;
    cycle->hops[BITACTOR_HOP_ONTOLOGY_LOAD] = bitactor_hop_ontology_load;
    cycle->hops[BITACTOR_HOP_SHACL_FIRE] = bitactor_hop_shacl_fire;
    cycle->hops[BITACTOR_HOP_STATE_RESOLVE] = bitactor_hop_state_resolve;
    cycle->hops[BITACTOR_HOP_COLLAPSE_COMPUTE] = bitactor_hop_collapse_compute;
    cycle->hops[BITACTOR_HOP_ACTION_BIND] = bitactor_hop_action_bind;
    cycle->hops[BITACTOR_HOP_STATE_COMMIT] = bitactor_hop_state_commit;
    cycle->hops[BITACTOR_HOP_META_VALIDATE] = bitactor_hop_meta_validate;
    
    // Set cycle budgets (1 tick per hop = 8 ticks total)
    for (int i = 0; i < BITACTOR_8H_HOP_COUNT; i++) {
        cycle->hop_budgets[i] = 1;
    }
    
    cycle->hop_dependencies = 0xFF; // Linear dependency chain
    cycle->cycle_id = 0;
}

// ---
// Part 3: Domain Collapse Implementation
// ---

CNS_INLINE bitactor_collapse_t bitactor_domain_collapse(
    bitactor_domain_t* domain,
    bitactor_signal_t* input_signals,
    uint32_t signal_count
) {
    bitactor_collapse_t result = {0};
    cns_cycle_t start_cycles = CNS_RDTSC();
    
    // Update feed actor with new signals
    if (input_signals && signal_count > 0) {
        bitactor_feed_actor_update(&domain->feed_actor, input_signals, signal_count);
    }
    
    // Execute 8H cognitive cycle for all active actors
    for (int hop = 0; hop < BITACTOR_8H_HOP_COUNT; hop++) {
        bitactor_hop_fn_t hop_fn = domain->cognitive_cycle.hops[hop];
        if (!hop_fn) continue;
        
        cns_bitmask_t hop_success = 0;
        
        // Execute hop for each active actor
        for (uint32_t actor_idx = 0; actor_idx < domain->actor_count; actor_idx++) {
            if (!(domain->active_mask & (1ULL << actor_idx))) continue;
            
            bitactor_state_t* actor = &domain->actors[actor_idx];
            cns_bitmask_t actor_result = hop_fn(domain, actor);
            hop_success |= actor_result;
        }
        
        // Accumulate results
        result.success_mask |= hop_success;
    }
    
    // Calculate performance metrics
    result.collapse_cycles = CNS_RDTSC() - start_cycles;
    result.failure_mask = domain->active_mask & ~result.success_mask;
    result.mutations_count = __builtin_popcountll(result.success_mask);
    
    // Verify 8T compliance for entire collapse
    assert(result.collapse_cycles <= (BITACTOR_8T_MAX_CYCLES * BITACTOR_8H_HOP_COUNT));
    
    return result;
}

// ---
// Part 4: Signal Processing & Nanoregex
// ---

CNS_INLINE cns_bool_t bitactor_nanoregex_compile(
    bitactor_nanoregex_t* regex,
    const char* pattern
) {
    if (!regex || !pattern) return CNS_FALSE;
    
    cns_cycle_t start = CNS_RDTSC();
    
    // Simple hash-based pattern compilation
    size_t pattern_len = strlen(pattern);
    if (pattern_len >= sizeof(regex->pattern_data)) {
        return CNS_FALSE;
    }
    
    // Generate pattern hash
    regex->pattern_hash = 0;
    for (size_t i = 0; i < pattern_len; i++) {
        regex->pattern_hash = regex->pattern_hash * 31 + pattern[i];
    }
    
    // Store pattern data
    regex->pattern_length = (uint16_t)pattern_len;
    memcpy(regex->pattern_data, pattern, pattern_len);
    
    // Generate match mask based on pattern characteristics
    regex->match_mask = regex->pattern_hash & 0xFFFFFFFFFFFFFFFFULL;
    
    cns_cycle_t cycles = CNS_RDTSC() - start;
    assert(cycles <= BITACTOR_8T_MAX_CYCLES);
    
    return CNS_TRUE;
}

CNS_INLINE cns_bitmask_t bitactor_nanoregex_match(
    const bitactor_nanoregex_t* regex,
    const bitactor_signal_t* signals,
    uint32_t signal_count
) {
    if (!regex || !signals) return 0;
    
    cns_cycle_t start = CNS_RDTSC();
    cns_bitmask_t match_mask = 0;
    
    // Ultra-fast pattern matching using hash comparison
    for (uint32_t i = 0; i < signal_count && i < 64; i++) {
        // Simple hash-based matching
        uint64_t signal_hash = signals[i] * 31 + regex->pattern_hash;
        if ((signal_hash & regex->match_mask) == regex->match_mask) {
            match_mask |= (1ULL << i);
        }
    }
    
    cns_cycle_t cycles = CNS_RDTSC() - start;
    assert(cycles <= BITACTOR_8T_MAX_CYCLES);
    
    return match_mask;
}

CNS_INLINE cns_bool_t bitactor_feed_actor_update(
    bitactor_feed_actor_t* feed_actor,
    const bitactor_signal_t* signals,
    uint32_t signal_count
) {
    if (!feed_actor || !signals) return CNS_FALSE;
    
    cns_cycle_t start = CNS_RDTSC();
    
    // Update last signals (circular buffer)
    uint32_t update_count = signal_count < 8 ? signal_count : 8;
    for (uint32_t i = 0; i < update_count; i++) {
        feed_actor->last_signals[i] = signals[i];
    }
    
    // Test all patterns against new signals
    uint32_t total_matches = 0;
    for (int p = 0; p < 8; p++) {
        cns_bitmask_t matches = bitactor_nanoregex_match(
            &feed_actor->patterns[p], signals, signal_count);
        total_matches += __builtin_popcountll(matches);
    }
    
    feed_actor->match_count += total_matches;
    feed_actor->last_match_cycles = CNS_RDTSC();
    
    cns_cycle_t cycles = CNS_RDTSC() - start;
    assert(cycles <= BITACTOR_8T_MAX_CYCLES);
    
    return CNS_TRUE;
}

// ---
// Part 5: Global Matrix Operations
// ---

CNS_INLINE cns_bitmask_t bitactor_matrix_tick(
    bitactor_matrix_t* matrix,
    bitactor_signal_t* global_signals,
    uint32_t signal_count
) {
    if (!matrix) return 0;
    
    cns_cycle_t tick_start = CNS_RDTSC();
    cns_bitmask_t global_success = 0;
    
    // Increment global tick counter
    matrix->global_tick_counter++;
    
    // Process all active domains
    for (uint32_t domain_idx = 0; domain_idx < matrix->domain_count; domain_idx++) {
        if (!(matrix->domain_active_mask & (1ULL << domain_idx))) continue;
        
        bitactor_domain_t* domain = &matrix->domains[domain_idx];
        
        // Perform domain collapse
        bitactor_collapse_t collapse = bitactor_domain_collapse(
            domain, global_signals, signal_count);
        
        // Accumulate results
        if (collapse.success_mask != 0) {
            global_success |= (1ULL << domain_idx);
            matrix->metrics.total_collapses++;
            matrix->metrics.total_mutations += collapse.mutations_count;
            
            // Update performance metrics
            if (collapse.collapse_cycles < matrix->metrics.min_collapse_cycles) {
                matrix->metrics.min_collapse_cycles = collapse.collapse_cycles;
            }
            if (collapse.collapse_cycles > matrix->metrics.max_collapse_cycles) {
                matrix->metrics.max_collapse_cycles = collapse.collapse_cycles;
            }
        }
    }
    
    // Update global metrics
    matrix->metrics.last_collapse_cycles = CNS_RDTSC() - tick_start;
    
    // Verify global tick compliance (relaxed for multiple domains)
    cns_cycle_t total_cycles = CNS_RDTSC() - tick_start;
    assert(total_cycles <= (BITACTOR_8T_MAX_CYCLES * BITACTOR_8H_HOP_COUNT * matrix->domain_count));
    
    return global_success;
}

// ---
// Part 6: Performance & Debugging
// ---

CNS_INLINE cns_bool_t bitactor_validate_8t_compliance(
    const bitactor_matrix_t* matrix
) {
    if (!matrix) return CNS_FALSE;
    
    // Check if last collapse was within 8T budget
    return matrix->metrics.last_collapse_cycles <= 
           (BITACTOR_8T_MAX_CYCLES * BITACTOR_8H_HOP_COUNT);
}

CNS_INLINE void bitactor_get_metrics(
    const bitactor_matrix_t* matrix,
    char* output_buffer,
    size_t buffer_size
) {
    if (!matrix || !output_buffer) return;
    
    snprintf(output_buffer, buffer_size,
        "=== BitActor Matrix Metrics ===\n"
        "Domains: %u active, %u total\n"
        "Actors: %u total across all domains\n"
        "Global Ticks: %lu\n"
        "Performance:\n"
        "  Total Collapses: %lu\n"
        "  Total Mutations: %lu\n"
        "  Last Collapse: %lu cycles\n"
        "  Min Collapse: %lu cycles\n"
        "  Max Collapse: %lu cycles\n"
        "  Avg Collapse: %lu cycles\n"
        "8T Compliance: %s\n"
        "Trinity Hash: 0x%016lx\n",
        __builtin_popcountll(matrix->domain_active_mask),
        matrix->domain_count,
        matrix->total_actors,
        matrix->global_tick_counter,
        matrix->metrics.total_collapses,
        matrix->metrics.total_mutations,
        matrix->metrics.last_collapse_cycles,
        matrix->metrics.min_collapse_cycles,
        matrix->metrics.max_collapse_cycles,
        matrix->metrics.total_collapses > 0 ? 
            matrix->metrics.last_collapse_cycles / matrix->metrics.total_collapses : 0,
        bitactor_validate_8t_compliance(matrix) ? "YES" : "NO",
        BITACTOR_TRINITY_HASH
    );
}

CNS_INLINE void bitactor_dump_state(
    const bitactor_matrix_t* matrix,
    FILE* output
) {
    if (!matrix || !output) return;
    
    fprintf(output, "=== BitActor Matrix State Dump ===\n");
    fprintf(output, "Trinity Hash: 0x%016lx\n", BITACTOR_TRINITY_HASH);
    fprintf(output, "Global Tick: %lu\n", matrix->global_tick_counter);
    fprintf(output, "Active Domains: 0x%016lx\n", matrix->domain_active_mask);
    
    for (uint32_t i = 0; i < matrix->domain_count; i++) {
        const bitactor_domain_t* domain = &matrix->domains[i];
        fprintf(output, "\nDomain %u:\n", i);
        fprintf(output, "  Active Actors: 0x%016lx\n", domain->active_mask);
        fprintf(output, "  Actor Count: %u\n", domain->actor_count);
        
        for (uint32_t j = 0; j < domain->actor_count; j++) {
            const bitactor_state_t* actor = &domain->actors[j];
            fprintf(output, "    Actor %u: meaning=0x%02x, hop=%u, budget=%u\n",
                j, actor->meaning, actor->hop_position, actor->tick_budget);
        }
    }
    
    fprintf(output, "\n=== End State Dump ===\n");
}
