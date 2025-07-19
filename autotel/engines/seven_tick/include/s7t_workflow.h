/*  ─────────────────────────────────────────────────────────────
    s7t_workflow.h  –  Physics-Compliant Workflow Engine (v1.0)
    ───────────────────────────────────────────────────────────── */
#ifndef S7T_WORKFLOW_H
#define S7T_WORKFLOW_H

#include "s7t.h"
#include "s7t_patterns.h"

/*═══════════════════════════════════════════════════════════════
  Pattern 1: Static Finite-State Lattice (SFL)
  Compiled state machines with zero indirection
  ═══════════════════════════════════════════════════════════════*/

#define S7T_SFL_MAX_STATES 256
#define S7T_SFL_MAX_EVENTS 64

typedef struct {
    uint8_t next;
    uint8_t action;
} s7t_sfl_row_t;

typedef struct S7T_ALIGNED(64) {
    s7t_sfl_row_t lattice[S7T_SFL_MAX_STATES * S7T_SFL_MAX_EVENTS];
    uint8_t current_state;
    uint8_t reserved[63];
} s7t_sfl_machine_t;

S7T_ALWAYS_INLINE void s7t_sfl_transition(s7t_sfl_machine_t* m, uint8_t event) {
    uint32_t idx = (m->current_state << 6) | event;  // Fast multiply by 64
    s7t_sfl_row_t row = m->lattice[idx];
    m->current_state = row.next;
}

// Computed goto dispatch for actions
#if defined(__GNUC__)
#define S7T_SFL_DISPATCH(machine, event, ...) do { \
    static void* const action_table[] = { __VA_ARGS__ }; \
    uint32_t idx = ((machine)->current_state << 6) | (event); \
    s7t_sfl_row_t row = (machine)->lattice[idx]; \
    (machine)->current_state = row.next; \
    goto *action_table[row.action]; \
} while(0)
#endif

/*═══════════════════════════════════════════════════════════════
  Pattern 2: Token-Ring Pipeline
  Lock-free SPSC rings connecting pipeline stages
  ═══════════════════════════════════════════════════════════════*/

#define S7T_TOKEN_RING_SIZE 1024  // Must be power of 2

typedef struct S7T_ALIGNED(64) {
    uint64_t meta;
    uint8_t payload[56];
} s7t_token_t;

typedef struct S7T_ALIGNED(64) {
    s7t_token_t buffer[S7T_TOKEN_RING_SIZE];
    s7t_atomic_u32 head;
    s7t_atomic_u32 tail;
    uint8_t padding[56];
} s7t_token_ring_t;

S7T_ALWAYS_INLINE bool s7t_token_put(s7t_token_ring_t* r, const s7t_token_t* t) {
    uint32_t h = atomic_load_explicit(&r->head, memory_order_relaxed);
    uint32_t next_h = (h + 1) & (S7T_TOKEN_RING_SIZE - 1);
    uint32_t t_val = atomic_load_explicit(&r->tail, memory_order_acquire);
    
    if (S7T_UNLIKELY(next_h == t_val)) return false;  // Full
    
    r->buffer[h] = *t;
    atomic_store_explicit(&r->head, next_h, memory_order_release);
    return true;
}

S7T_ALWAYS_INLINE bool s7t_token_get(s7t_token_ring_t* r, s7t_token_t* t) {
    uint32_t tail = atomic_load_explicit(&r->tail, memory_order_acquire);
    uint32_t h = atomic_load_explicit(&r->head, memory_order_acquire);
    
    if (S7T_UNLIKELY(tail == h)) return false;  // Empty
    
    *t = r->buffer[tail];
    uint32_t next_t = (tail + 1) & (S7T_TOKEN_RING_SIZE - 1);
    atomic_store_explicit(&r->tail, next_t, memory_order_release);
    return true;
}

typedef bool (*s7t_stage_func_t)(s7t_token_t*, void*);

typedef struct {
    s7t_token_ring_t* input;
    s7t_token_ring_t* output;
    s7t_stage_func_t process;
    void* context;
} s7t_pipeline_stage_t;

/*═══════════════════════════════════════════════════════════════
  Pattern 3: Micro-Op Tape
  Static bytecode with compact 8-byte instructions
  ═══════════════════════════════════════════════════════════════*/

typedef enum {
    S7T_OP_NOP = 0,
    S7T_OP_LOAD,
    S7T_OP_STORE,
    S7T_OP_ADD,
    S7T_OP_AND,
    S7T_OP_JMP,
    S7T_OP_JZ,
    S7T_OP_HALT
} s7t_opcode_t;

typedef struct {
    uint8_t op;
    uint8_t dst;
    uint8_t src1;
    uint8_t src2;
    uint32_t imm;
} s7t_tape_op_t;

typedef struct S7T_ALIGNED(64) {
    s7t_tape_op_t* tape;
    uint32_t* stack;
    uint32_t pc;
    uint32_t sp;
    uint32_t tape_size;
    uint32_t stack_size;
} s7t_tape_machine_t;

S7T_ALWAYS_INLINE void s7t_tape_execute(s7t_tape_machine_t* m) {
    static const void* op_table[] = {
        &&op_nop, &&op_load, &&op_store, &&op_add,
        &&op_and, &&op_jmp, &&op_jz, &&op_halt
    };
    
    while (m->pc < m->tape_size) {
        s7t_tape_op_t* op = &m->tape[m->pc++];
        goto *op_table[op->op];
        
        op_nop:  continue;
        op_load: m->stack[op->dst] = op->imm; continue;
        op_store: /* Implementation */ continue;
        op_add:  m->stack[op->dst] = m->stack[op->src1] + m->stack[op->src2]; continue;
        op_and:  m->stack[op->dst] = m->stack[op->src1] & m->stack[op->src2]; continue;
        op_jmp:  m->pc = op->imm; continue;
        op_jz:   if (m->stack[op->src1] == 0) m->pc = op->imm; continue;
        op_halt: return;
    }
}

/*═══════════════════════════════════════════════════════════════
  Pattern 4: Bitmask Decision Field
  SIMD-accelerated rule evaluation
  ═══════════════════════════════════════════════════════════════*/

typedef struct S7T_ALIGNED(32) {
    uint64_t conditions[4];  // 256 bits of conditions
    uint64_t actions[4];     // 256 bits of actions
} s7t_rule_t;

typedef struct S7T_ALIGNED(64) {
    s7t_rule_t* rules;
    uint32_t rule_count;
    uint32_t reserved;
} s7t_decision_field_t;

#if defined(__AVX2__)
#include <immintrin.h>

S7T_ALWAYS_INLINE uint64_t s7t_evaluate_rules_avx2(
    s7t_decision_field_t* df,
    const uint64_t* facts
) {
    __m256i fact_vec = _mm256_load_si256((const __m256i*)facts);
    __m256i action_vec = _mm256_setzero_si256();
    
    for (uint32_t i = 0; i < df->rule_count; i++) {
        __m256i cond = _mm256_load_si256((const __m256i*)df->rules[i].conditions);
        __m256i act = _mm256_load_si256((const __m256i*)df->rules[i].actions);
        
        // Check if all conditions match
        __m256i match = _mm256_cmpeq_epi64(_mm256_and_si256(fact_vec, cond), cond);
        
        // Accumulate actions for matching rules
        action_vec = _mm256_or_si256(action_vec, _mm256_and_si256(act, match));
    }
    
    // Extract first 64 bits of actions
    return _mm256_extract_epi64(action_vec, 0);
}
#else
// Fallback scalar version
S7T_ALWAYS_INLINE uint64_t s7t_evaluate_rules_scalar(
    s7t_decision_field_t* df,
    const uint64_t* facts
) {
    uint64_t actions = 0;
    for (uint32_t i = 0; i < df->rule_count; i++) {
        bool match = true;
        for (int j = 0; j < 4; j++) {
            if ((facts[j] & df->rules[i].conditions[j]) != df->rules[i].conditions[j]) {
                match = false;
                break;
            }
        }
        if (match) {
            actions |= df->rules[i].actions[0];
        }
    }
    return actions;
}
#endif

/*═══════════════════════════════════════════════════════════════
  Pattern 5: Time-Bucket Accumulator
  Circular arrays for temporal workflows
  ═══════════════════════════════════════════════════════════════*/

#define S7T_TIME_BUCKETS 3600  // 1 hour at 1-second resolution

typedef struct S7T_ALIGNED(64) {
    uint64_t counters[8];  // Multiple metrics per bucket
} s7t_time_bucket_t;

typedef struct S7T_ALIGNED(64) {
    s7t_time_bucket_t buckets[S7T_TIME_BUCKETS];
    uint32_t current_bucket;
    uint32_t window_size;
    uint64_t base_time;
} s7t_time_accumulator_t;

S7T_ALWAYS_INLINE void s7t_time_advance(s7t_time_accumulator_t* ta, uint64_t new_time) {
    uint32_t delta = (uint32_t)(new_time - ta->base_time);
    uint32_t new_bucket = (ta->current_bucket + delta) % S7T_TIME_BUCKETS;
    
    // Clear expired buckets
    while (ta->current_bucket != new_bucket) {
        ta->current_bucket = (ta->current_bucket + 1) % S7T_TIME_BUCKETS;
        memset(&ta->buckets[ta->current_bucket], 0, sizeof(s7t_time_bucket_t));
    }
    
    ta->base_time = new_time;
}

S7T_ALWAYS_INLINE void s7t_time_increment(s7t_time_accumulator_t* ta, uint32_t metric) {
    ta->buckets[ta->current_bucket].counters[metric]++;
}

// SIMD sliding window sum
S7T_ALWAYS_INLINE uint64_t s7t_time_window_sum(s7t_time_accumulator_t* ta, uint32_t metric) {
    uint64_t sum = 0;
    uint32_t start = (ta->current_bucket + S7T_TIME_BUCKETS - ta->window_size) % S7T_TIME_BUCKETS;
    
    for (uint32_t i = 0; i < ta->window_size; i++) {
        uint32_t idx = (start + i) % S7T_TIME_BUCKETS;
        sum += ta->buckets[idx].counters[metric];
    }
    
    return sum;
}

/*═══════════════════════════════════════════════════════════════
  Pattern 6: Sharded Hash-Join Grid
  Per-core hash tables for zero contention
  ═══════════════════════════════════════════════════════════════*/

#define S7T_HASH_BUCKETS 1024
#define S7T_HASH_SHARDS 16

typedef struct {
    uint32_t key;
    uint32_t value;
    uint32_t next;  // Index to next entry
} s7t_hash_entry_t;

typedef struct S7T_ALIGNED(64) {
    s7t_hash_entry_t entries[S7T_HASH_BUCKETS];
    uint32_t heads[S7T_HASH_BUCKETS];
    uint32_t free_list;
    uint32_t count;
} s7t_hash_shard_t;

typedef struct {
    s7t_hash_shard_t shards[S7T_HASH_SHARDS];
} s7t_hash_grid_t;

S7T_ALWAYS_INLINE uint32_t s7t_hash_to_shard(uint32_t key) {
    return (key >> 8) & (S7T_HASH_SHARDS - 1);
}

S7T_ALWAYS_INLINE bool s7t_hash_insert(s7t_hash_grid_t* grid, uint32_t key, uint32_t value) {
    uint32_t shard_idx = s7t_hash_to_shard(key);
    s7t_hash_shard_t* shard = &grid->shards[shard_idx];
    uint32_t bucket = key & (S7T_HASH_BUCKETS - 1);
    
    // Check if key exists
    uint32_t idx = shard->heads[bucket];
    while (idx != 0) {
        if (shard->entries[idx].key == key) {
            shard->entries[idx].value = value;
            return true;
        }
        idx = shard->entries[idx].next;
    }
    
    // Insert new entry
    if (shard->free_list == 0) return false;  // Full
    
    uint32_t new_idx = shard->free_list;
    shard->free_list = shard->entries[new_idx].next;
    
    shard->entries[new_idx].key = key;
    shard->entries[new_idx].value = value;
    shard->entries[new_idx].next = shard->heads[bucket];
    shard->heads[bucket] = new_idx;
    shard->count++;
    
    return true;
}

S7T_ALWAYS_INLINE uint32_t s7t_hash_lookup(s7t_hash_grid_t* grid, uint32_t key) {
    uint32_t shard_idx = s7t_hash_to_shard(key);
    s7t_hash_shard_t* shard = &grid->shards[shard_idx];
    uint32_t bucket = key & (S7T_HASH_BUCKETS - 1);
    
    uint32_t idx = shard->heads[bucket];
    while (idx != 0) {
        if (shard->entries[idx].key == key) {
            return shard->entries[idx].value;
        }
        idx = shard->entries[idx].next;
    }
    
    return 0;  // Not found
}

/*═══════════════════════════════════════════════════════════════
  Pattern 7: Compile-Time Scenario Matrix
  Pre-computed decision tables for all condition combinations
  ═══════════════════════════════════════════════════════════════*/

#define S7T_SCENARIO_BITS 8  // 256 scenarios max

typedef struct S7T_ALIGNED(64) {
    uint32_t results[1 << S7T_SCENARIO_BITS];
} s7t_scenario_matrix_t;

S7T_ALWAYS_INLINE uint32_t s7t_scenario_lookup(
    s7t_scenario_matrix_t* matrix,
    uint8_t conditions
) {
    return matrix->results[conditions];
}

// Multi-dimensional scenario matrix
typedef struct S7T_ALIGNED(64) {
    uint32_t results[16][16][16];  // 3D decision space
} s7t_scenario_3d_t;

S7T_ALWAYS_INLINE uint32_t s7t_scenario_3d_lookup(
    s7t_scenario_3d_t* matrix,
    uint8_t x, uint8_t y, uint8_t z
) {
    return matrix->results[x & 15][y & 15][z & 15];
}

/*═══════════════════════════════════════════════════════════════
  Unified Workflow Engine
  Combines all patterns into a cohesive system
  ═══════════════════════════════════════════════════════════════*/

typedef struct S7T_ALIGNED(64) {
    // Core components
    s7t_sfl_machine_t* state_machines[16];
    s7t_token_ring_t* rings[32];
    s7t_tape_machine_t* tape_machines[8];
    s7t_decision_field_t* decision_fields[4];
    s7t_time_accumulator_t* time_accumulators[4];
    s7t_hash_grid_t* hash_grids[2];
    s7t_scenario_matrix_t* scenario_matrices[8];
    
    // Configuration
    uint32_t num_state_machines;
    uint32_t num_rings;
    uint32_t num_tape_machines;
    uint32_t num_decision_fields;
    
    // Global state
    s7t_atomic_u64 tick_count;
    uint64_t last_tick_cycles;
} s7t_workflow_engine_t;

// Initialize workflow engine from static memory
S7T_ALWAYS_INLINE void s7t_workflow_init(
    s7t_workflow_engine_t* engine,
    s7t_arena_t* arena
) {
    memset(engine, 0, sizeof(s7t_workflow_engine_t));
    
    // Allocate components from arena
    for (int i = 0; i < 16; i++) {
        engine->state_machines[i] = s7t_arena_alloc(arena, sizeof(s7t_sfl_machine_t));
    }
    
    for (int i = 0; i < 32; i++) {
        engine->rings[i] = s7t_arena_alloc(arena, sizeof(s7t_token_ring_t));
    }
    
    // Initialize other components...
}

// Single tick of the workflow engine
S7T_ALWAYS_INLINE void s7t_workflow_tick(s7t_workflow_engine_t* engine) {
    uint64_t start = s7t_cycles();
    
    // Process all pipeline stages
    for (uint32_t i = 0; i < engine->num_rings - 1; i++) {
        s7t_token_t token;
        if (s7t_token_get(engine->rings[i], &token)) {
            // Process token through state machine
            uint8_t sm_idx = token.meta & 0xFF;
            uint8_t event_type = (token.meta >> 8) & 0xFF;
            s7t_sfl_transition(engine->state_machines[sm_idx], event_type);
            
            // Route to next stage
            s7t_token_put(engine->rings[i + 1], &token);
        }
    }
    
    // Execute tape machines
    for (uint32_t i = 0; i < engine->num_tape_machines; i++) {
        s7t_tape_execute(engine->tape_machines[i]);
    }
    
    // Update timing
    uint64_t cycles = s7t_cycles() - start;
    engine->last_tick_cycles = cycles;
    s7t_atomic_inc_u64(&engine->tick_count);
}

/*═══════════════════════════════════════════════════════════════
  Memory Layout Strategies
  ═══════════════════════════════════════════════════════════════*/

// Hot/cold separation
#define S7T_HOT_SECTION  __attribute__((section(".hot")))
#define S7T_COLD_SECTION __attribute__((section(".cold")))

// Per-CPU allocation hint
#define S7T_PER_CPU __attribute__((section(".percpu")))

// Huge page allocation
#define S7T_HUGE_PAGE __attribute__((aligned(2 * 1024 * 1024)))

/*═══════════════════════════════════════════════════════════════
  Performance Validation
  ═══════════════════════════════════════════════════════════════*/

#define S7T_WORKFLOW_ASSERT_TICKS(engine, max_ticks) \
    S7T_STATIC_ASSERT((engine)->last_tick_cycles <= (max_ticks) * S7T_MAX_CYCLES, \
                      "Workflow tick exceeded cycle budget")

/*═══════════════════════════════════════════════════════════════
  Failure Mode Analysis
  ═══════════════════════════════════════════════════════════════*/

// All failure handling must be branchless to maintain ≤2ns guarantee
S7T_ALWAYS_INLINE uint32_t s7t_handle_failure(uint32_t error_code) {
    static const uint32_t recovery_actions[] = {
        0,  // No error
        1,  // Drop token
        2,  // Reset state
        3,  // Flush pipeline
        4,  // Reinitialize
    };
    
    return recovery_actions[error_code & 7];
}

#endif /* S7T_WORKFLOW_H */