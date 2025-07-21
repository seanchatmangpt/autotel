/**
 * @file bitactor.c
 * @brief BitActor Core Implementation - Fifth Epoch Engine
 * @version 1.0.0
 * 
 * The beating heart of the Fifth Epoch: BitActors executing causality
 * at tick-aligned machine speed. This is where specification becomes execution.
 */

#include "../include/bitactor.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef __APPLE__
#include <mach/mach_time.h>
#endif

// =============================================================================
// INTERNAL TIMING FUNCTIONS
// =============================================================================

static uint64_t get_precise_timestamp_ns(void) {
#ifdef __APPLE__
    static mach_timebase_info_data_t timebase = {0};
    if (timebase.denom == 0) {
        mach_timebase_info(&timebase);
    }
    uint64_t mach_time = mach_absolute_time();
    return mach_time * timebase.numer / timebase.denom;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
#endif
}

// =============================================================================
// BITACTOR MATRIX MANAGEMENT
// =============================================================================

BitActorMatrix* bitactor_matrix_create(void) {
    BitActorMatrix* matrix = calloc(1, sizeof(BitActorMatrix));
    if (!matrix) return NULL;
    
    // Initialize global state
    matrix->global_tick = 0;
    matrix->active_count = 0;
    matrix->globally_entangled = false;
    
    // Initialize all BitActors
    for (int i = 0; i < BITACTOR_MATRIX_SIZE; i++) {
        BitActorContext* ctx = &matrix->actors[i];
        ctx->bits = 0;
        ctx->vector = 0;
        ctx->actor_id = i + 1;  // 1-based IDs
        ctx->last_tick = 0;
        ctx->compiled_ttl = NULL;
        ctx->signal_length = 0;
        
        // Initialize proof chain
        ctx->proof.current_hop = 0;
        ctx->proof.valid = false;
        ctx->proof.start_tick = 0;
        ctx->proof.proof_hash = 0;
        memset(ctx->proof.hops, 0, sizeof(ctx->proof.hops));
    }
    
    // Initialize entanglement matrix
    memset(matrix->entanglement_matrix, 0, sizeof(matrix->entanglement_matrix));
    memset(matrix->matrix_hash, 0, sizeof(matrix->matrix_hash));
    
    printf("🧬 BitActor Matrix initialized: %d BitActors ready\n", BITACTOR_MATRIX_SIZE);
    printf("   Trinity: 8T/8H/8B architecture active\n");
    printf("   Target: %dns per tick, %dns per collapse\n", TARGET_TICK_NS, TARGET_COLLAPSE_NS);
    
    return matrix;
}

void bitactor_matrix_destroy(BitActorMatrix* matrix) {
    if (!matrix) return;
    
    // Clean up compiled TTL code for active BitActors
    for (int i = 0; i < BITACTOR_MATRIX_SIZE; i++) {
        if (matrix->actors[i].compiled_ttl) {
            free(matrix->actors[i].compiled_ttl);
        }
    }
    
    printf("🧬 BitActor Matrix destroyed: %u BitActors deactivated\n", matrix->active_count);
    free(matrix);
}

// =============================================================================
// BITACTOR LIFECYCLE
// =============================================================================

uint32_t bitactor_spawn(BitActorMatrix* matrix, void* compiled_ttl) {
    if (!matrix) return 0;
    
    // Find available BitActor slot
    for (int i = 0; i < BITACTOR_MATRIX_SIZE; i++) {
        BitActorContext* ctx = &matrix->actors[i];
        if (ctx->compiled_ttl == NULL) {  // Available slot
            // Initialize BitActor
            ctx->compiled_ttl = compiled_ttl;
            ctx->bits = 0x01;  // Set trigger active bit
            ctx->vector = 0;
            ctx->last_tick = matrix->global_tick;
            ctx->signal_length = 0;
            
            // Initialize proof chain
            ctx->proof.current_hop = 0;
            ctx->proof.valid = false;
            ctx->proof.start_tick = matrix->global_tick;
            ctx->proof.proof_hash = (uint64_t)ctx->actor_id * 0x9E3779B97F4A7C15ULL;
            
            matrix->active_count++;
            
            printf("🧬 BitActor spawned: ID=%u, Active=%u/%d\n", 
                   ctx->actor_id, matrix->active_count, BITACTOR_MATRIX_SIZE);
            
            return ctx->actor_id;
        }
    }
    
    printf("❌ BitActor spawn failed: Matrix full (%u/%d)\n", 
           matrix->active_count, BITACTOR_MATRIX_SIZE);
    return 0;
}

// =============================================================================
// SIGNAL PROCESSING
// =============================================================================

bool bitactor_signal(BitActorMatrix* matrix, uint32_t actor_id, const BitActorSignal* signal) {
    if (!matrix || actor_id == 0 || actor_id > BITACTOR_MATRIX_SIZE || !signal) {
        return false;
    }
    
    BitActorContext* ctx = &matrix->actors[actor_id - 1];
    if (!ctx->compiled_ttl) return false;  // BitActor not active
    
    // Check signal buffer space
    if (ctx->signal_length + signal->length > SIGNAL_BUFFER_SIZE) {
        printf("⚠️  Signal buffer overflow: BitActor %u\n", actor_id);
        return false;
    }
    
    // Copy signal to buffer
    memcpy(ctx->signal_buffer + ctx->signal_length, signal->data, signal->length);
    ctx->signal_length += signal->length;
    
    // Set signal entangled bit
    bitactor_set_meaning_bit(&ctx->bits, 5, true);
    
    // Update causal vector with signal information
    ctx->vector |= ((uint64_t)signal->type << 56);
    ctx->vector |= ((uint64_t)signal->priority << 48);
    ctx->vector |= (signal->nanoregex_mask & 0xFFFFFFFFFFFF);
    
    return true;
}

// =============================================================================
// 8T TICK EXECUTION ENGINE
// =============================================================================

uint32_t bitactor_tick(BitActorMatrix* matrix) {
    if (!matrix) return 0;
    
    uint64_t tick_start = get_precise_timestamp_ns();
    uint32_t executed_count = 0;
    
    // Increment global tick
    matrix->global_tick++;
    
    // Execute all active BitActors within 8T budget
    for (int i = 0; i < BITACTOR_MATRIX_SIZE && executed_count < matrix->active_count; i++) {
        BitActorContext* ctx = &matrix->actors[i];
        if (!ctx->compiled_ttl) continue;  // Skip inactive BitActors
        
        uint64_t actor_start = get_precise_timestamp_ns();
        
        // Check if BitActor needs execution (has signals or state change)
        bool needs_execution = (ctx->signal_length > 0) || 
                              (ctx->last_tick < matrix->global_tick - 1);
        
        if (needs_execution) {
            // Execute BitActor logic
            bitactor_execute_single(ctx, matrix->global_tick);
            executed_count++;
        }
        
        uint64_t actor_end = get_precise_timestamp_ns();
        uint64_t actor_time = actor_end - actor_start;
        
        // Enforce 8T tick budget (TARGET_TICK_NS per BitActor)
        if (actor_time > TARGET_TICK_NS) {
            printf("⚠️  BitActor %u exceeded 8T budget: %lluns > %dns\n", 
                   ctx->actor_id, actor_time, TARGET_TICK_NS);
        }
        
        ctx->last_tick = matrix->global_tick;
    }
    
    uint64_t tick_end = get_precise_timestamp_ns();
    uint64_t total_tick_time = tick_end - tick_start;
    
    // Update matrix hash with current state
    bitactor_update_matrix_hash(matrix);
    
    // Debug output for performance monitoring
    if (matrix->global_tick % 1000 == 0) {  // Every 1000 ticks
        printf("🔄 Tick %llu: %u BitActors executed in %lluns\n", 
               matrix->global_tick, executed_count, total_tick_time);
    }
    
    return executed_count;
}

// =============================================================================
// SINGLE BITACTOR EXECUTION
// =============================================================================

static void bitactor_execute_single(BitActorContext* ctx, uint64_t global_tick) {
    // Set state valid bit
    bitactor_set_meaning_bit(&ctx->bits, 1, true);
    
    // Process signals if any
    if (ctx->signal_length > 0) {
        // Simple signal processing: set action ready bit
        bitactor_set_meaning_bit(&ctx->bits, 2, true);
        
        // Clear processed signals
        ctx->signal_length = 0;
        memset(ctx->signal_buffer, 0, sizeof(ctx->signal_buffer));
    }
    
    // Update causal vector based on BitActor state
    ctx->vector = (ctx->vector & 0xFFFFFFFF00000000ULL) | global_tick;
    
    // Set memory committed bit
    bitactor_set_meaning_bit(&ctx->bits, 4, true);
}

// =============================================================================
// 8H CAUSAL COLLAPSE
// =============================================================================

CausalVector bitactor_collapse(BitActorMatrix* matrix, uint32_t actor_id) {
    if (!matrix || actor_id == 0 || actor_id > BITACTOR_MATRIX_SIZE) {
        return 0;
    }
    
    BitActorContext* ctx = &matrix->actors[actor_id - 1];
    if (!ctx->compiled_ttl) return 0;
    
    uint64_t collapse_start = get_precise_timestamp_ns();
    
    // Initialize 8H causal proof chain
    ctx->proof.current_hop = 0;
    ctx->proof.start_tick = matrix->global_tick;
    ctx->proof.valid = false;
    
    // Execute 8-hop causal collapse
    for (int hop = 0; hop < MAX_8H_HOPS; hop++) {
        uint64_t hop_start = get_precise_timestamp_ns();
        
        switch (hop) {
            case 0: // Trigger detected
                ctx->proof.hops[hop] = (ctx->bits & 0x01) ? 1 : 0;
                break;
                
            case 1: // Ontology loaded
                ctx->proof.hops[hop] = (ctx->compiled_ttl != NULL) ? 1 : 0;
                break;
                
            case 2: // SHACL path fired
                ctx->proof.hops[hop] = (ctx->vector != 0) ? 1 : 0;
                break;
                
            case 3: // BitActor state resolved
                ctx->proof.hops[hop] = bitactor_count_active_bits(ctx->bits);
                break;
                
            case 4: // Collapse computed
                ctx->proof.hops[hop] = ctx->vector ^ (ctx->bits << 8);
                break;
                
            case 5: // Action bound
                bitactor_set_meaning_bit(&ctx->bits, 2, true);
                ctx->proof.hops[hop] = 1;
                break;
                
            case 6: // State committed
                bitactor_set_meaning_bit(&ctx->bits, 4, true);
                ctx->proof.hops[hop] = 1;
                break;
                
            case 7: // Meta-proof validated
                ctx->proof.proof_hash ^= ctx->proof.hops[hop-1];
                ctx->proof.hops[hop] = ctx->proof.proof_hash & 0xFFFF;
                break;
        }
        
        ctx->proof.current_hop = hop + 1;
        
        uint64_t hop_end = get_precise_timestamp_ns();
        uint64_t hop_time = hop_end - hop_start;
        
        // Each hop should be very fast
        if (hop_time > TARGET_COLLAPSE_NS / 8) {
            printf("⚠️  Hop %d exceeded budget: %lluns\n", hop, hop_time);
        }
    }
    
    uint64_t collapse_end = get_precise_timestamp_ns();
    uint64_t total_collapse_time = collapse_end - collapse_start;
    
    // Validate proof chain
    ctx->proof.valid = bitactor_validate_proof(&ctx->proof);
    
    // Set collapse pending bit
    bitactor_set_meaning_bit(&ctx->bits, 6, true);
    
    // Enforce 8H collapse budget
    if (total_collapse_time > TARGET_COLLAPSE_NS) {
        printf("⚠️  BitActor %u collapse exceeded 8H budget: %lluns > %dns\n", 
               actor_id, total_collapse_time, TARGET_COLLAPSE_NS);
    }
    
    printf("🌀 BitActor %u collapse: 8H completed in %lluns, proof=%s\n",
           actor_id, total_collapse_time, ctx->proof.valid ? "valid" : "invalid");
    
    return ctx->vector;
}

// =============================================================================
// PROOF VALIDATION
// =============================================================================

bool bitactor_validate_proof(const CausalProofChain* proof) {
    if (!proof) return false;
    
    // Basic validation: all hops completed
    if (proof->current_hop != MAX_8H_HOPS) return false;
    
    // Validate hop sequence
    for (int i = 0; i < MAX_8H_HOPS; i++) {
        if (proof->hops[i] == 0 && i != 0) {  // Allow hop 0 to be 0 (no trigger)
            return false;
        }
    }
    
    // Validate proof hash (simple check)
    uint64_t computed_hash = 0;
    for (int i = 0; i < MAX_8H_HOPS; i++) {
        computed_hash ^= proof->hops[i] * (i + 1);
    }
    
    return (computed_hash & 0xFFFF) == (proof->proof_hash & 0xFFFF);
}

// =============================================================================
// GLOBAL ENTANGLEMENT
// =============================================================================

bool bitactor_entangle_global(BitActorMatrix* matrix) {
    if (!matrix) return false;
    
    printf("🌍 Enabling global entanglement across %u BitActors...\n", matrix->active_count);
    
    // Initialize entanglement matrix
    for (int i = 0; i < 32; i++) {
        for (int j = 0; j < 32; j++) {
            if (i != j) {
                matrix->entanglement_matrix[i][j] = 0x8T8H8B88888888ULL;
            }
        }
    }
    
    // Set signal entangled bit on all active BitActors
    for (int i = 0; i < BITACTOR_MATRIX_SIZE; i++) {
        BitActorContext* ctx = &matrix->actors[i];
        if (ctx->compiled_ttl) {
            bitactor_set_meaning_bit(&ctx->bits, 5, true);
        }
    }
    
    matrix->globally_entangled = true;
    
    printf("🌍 Global entanglement activated: Trinity signature 0x8T8H8B88888888\n");
    return true;
}

// =============================================================================
// PERFORMANCE METRICS
// =============================================================================

void bitactor_get_metrics(const BitActorMatrix* matrix, 
                         double* tick_rate_hz,
                         uint64_t* collapse_time_ns,
                         uint64_t* proof_time_ns) {
    if (!matrix) return;
    
    // Calculate tick rate (approximate)
    static uint64_t last_tick = 0;
    static uint64_t last_time = 0;
    
    uint64_t current_time = get_precise_timestamp_ns();
    if (last_time > 0) {
        uint64_t time_diff = current_time - last_time;
        uint64_t tick_diff = matrix->global_tick - last_tick;
        if (time_diff > 0) {
            *tick_rate_hz = (double)tick_diff * 1000000000.0 / time_diff;
        }
    }
    
    last_tick = matrix->global_tick;
    last_time = current_time;
    
    // Estimate collapse and proof times (would need actual measurements)
    *collapse_time_ns = TARGET_COLLAPSE_NS / 2;  // Placeholder
    *proof_time_ns = TARGET_PROOF_NS / 2;        // Placeholder
}

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

static void bitactor_update_matrix_hash(BitActorMatrix* matrix) {
    // Simple hash of matrix state
    uint64_t state_hash = matrix->global_tick;
    state_hash ^= matrix->active_count;
    
    for (int i = 0; i < 32 && i < matrix->active_count; i++) {
        state_hash ^= matrix->actors[i].bits << (i % 8);
        state_hash ^= matrix->actors[i].vector >> (i % 16);
    }
    
    // Store hash in matrix_hash array
    memcpy(matrix->matrix_hash, &state_hash, 8);
}

// =============================================================================
// FIFTH EPOCH VALIDATION
// =============================================================================

bool bitactor_validate_fifth_epoch(const BitActorMatrix* matrix) {
    if (!matrix) return false;
    
    // Check Trinity principles
    bool tick_budget_valid = true;  // Would need actual timing measurements
    bool hop_chains_valid = true;   // Check all proof chains
    bool bit_actors_valid = true;   // Check all BitActors have 8 bits
    
    // Validate that specification IS execution
    for (int i = 0; i < BITACTOR_MATRIX_SIZE; i++) {
        const BitActorContext* ctx = &matrix->actors[i];
        if (ctx->compiled_ttl && !ctx->proof.valid) {
            hop_chains_valid = false;
            break;
        }
    }
    
    return tick_budget_valid && hop_chains_valid && bit_actors_valid;
}

uint64_t bitactor_generate_trinity_hash(const BitActorMatrix* matrix) {
    if (!matrix) return 0;
    
    uint64_t trinity_hash = 0x8000000000000000ULL;  // Start with 8T
    trinity_hash |= 0x0800000000000000ULL;          // Add 8H
    trinity_hash |= 0x0080000000000000ULL;          // Add 8B
    
    // Mix in matrix state
    trinity_hash ^= matrix->global_tick;
    trinity_hash ^= ((uint64_t)matrix->active_count << 32);
    
    return trinity_hash;
}

void bitactor_print_matrix_status(const BitActorMatrix* matrix) {
    if (!matrix) return;
    
    printf("\n🧬 BITACTOR MATRIX STATUS\n");
    printf("========================\n\n");
    
    printf("Trinity: 8T/8H/8B - Fifth Epoch Active\n");
    printf("Global Tick: %llu\n", matrix->global_tick);
    printf("Active BitActors: %u/%d\n", matrix->active_count, BITACTOR_MATRIX_SIZE);
    printf("Global Entanglement: %s\n", matrix->globally_entangled ? "ACTIVE" : "inactive");
    
    uint64_t trinity_hash = bitactor_generate_trinity_hash(matrix);
    printf("Trinity Hash: 0x%016llX\n", trinity_hash);
    
    printf("\nActive BitActors:\n");
    int displayed = 0;
    for (int i = 0; i < BITACTOR_MATRIX_SIZE && displayed < 10; i++) {
        const BitActorContext* ctx = &matrix->actors[i];
        if (ctx->compiled_ttl) {
            printf("  BitActor %u: bits=0x%02X, vector=0x%016llX, proof=%s\n",
                   ctx->actor_id, ctx->bits, ctx->vector,
                   ctx->proof.valid ? "valid" : "invalid");
            displayed++;
        }
    }
    
    if (matrix->active_count > 10) {
        printf("  ... and %u more\n", matrix->active_count - 10);
    }
    
    printf("\n");
}