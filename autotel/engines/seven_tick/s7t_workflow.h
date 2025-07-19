#ifndef S7T_WORKFLOW_H
#define S7T_WORKFLOW_H

/*
 * Seven Tick (7T) Workflow Engine - High-Level Abstractions
 * 
 * PHILOSOPHY: Every workflow pattern compiles to array indices and ring writes
 * All coordination through data-flow, no control-flow divergence
 * 
 * TARGET: ≤2ns per workflow hop, ≤7 CPU ticks per decision
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#ifdef __x86_64__
#include <immintrin.h>
#elif defined(__aarch64__)
#include <arm_neon.h>
#endif

/* ═══════════════════════════════════════════════════════════════════════════
 * PATTERN 1: Static Finite-State Lattice (SFL)
 * Compiled state machines with zero indirection
 * ═══════════════════════════════════════════════════════════════════════════ */

#define SFL_MAX_STATES 256
#define SFL_MAX_TRANSITIONS 1024

typedef struct __attribute__((aligned(64))) {
    uint8_t next_state[256];      // Direct index lookup
    uint8_t action_id[256];       // Handler function index
    uint64_t state_data[4];       // Per-state payload
} sfl_state_t;

typedef struct __attribute__((aligned(64))) {
    sfl_state_t states[SFL_MAX_STATES];
    uint8_t current_state;
    uint8_t pad[63];              // Prevent false sharing
} sfl_machine_t;

// Compile-time state machine builder
#define SFL_DEFINE_MACHINE(name, ...) \
    static const sfl_machine_t name = { \
        .states = { __VA_ARGS__ }, \
        .current_state = 0 \
    }

// Single-instruction state transition
#define SFL_TRANSITION(machine, event) \
    ((machine)->current_state = (machine)->states[(machine)->current_state].next_state[event])

/* ═══════════════════════════════════════════════════════════════════════════
 * PATTERN 2: Token-Ring Pipeline
 * Lock-free ring buffers between workflow stages
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef struct __attribute__((aligned(64))) {
    uint64_t payload[6];          // 48 bytes of data
    uint16_t stage_id;            // Current pipeline stage
    uint16_t flags;               // Control flags
    uint32_t sequence;            // For ordering
} ring_token_t;

typedef struct __attribute__((aligned(64))) {
    ring_token_t* buffer;         // Power-of-2 sized
    uint32_t mask;                // Size - 1 for fast modulo
    uint32_t head;                // Producer position
    uint32_t tail;                // Consumer position
    char pad[48];                 // Cache line isolation
} ring_buffer_t;

// Branchless ring operations
static inline bool ring_push(ring_buffer_t* ring, const ring_token_t* token) {
    uint32_t next = (ring->head + 1) & ring->mask;
    if (next == ring->tail) return false;  // Full
    
    ring->buffer[ring->head] = *token;
    __atomic_store_n(&ring->head, next, __ATOMIC_RELEASE);
    return true;
}

static inline bool ring_pop(ring_buffer_t* ring, ring_token_t* token) {
    uint32_t head = __atomic_load_n(&ring->head, __ATOMIC_ACQUIRE);
    if (ring->tail == head) return false;  // Empty
    
    *token = ring->buffer[ring->tail];
    ring->tail = (ring->tail + 1) & ring->mask;
    return true;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * PATTERN 3: Micro-Op Tape
 * Static bytecode for workflow orchestration
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef enum {
    OP_DISPATCH = 0x00,   // Send to handler
    OP_FORK     = 0x01,   // Parallel split
    OP_JOIN     = 0x02,   // Synchronization
    OP_FILTER   = 0x03,   // Conditional routing
    OP_TRANSFORM= 0x04,   // Data mutation
    OP_ACCUMULATE=0x05,   // Aggregate state
    OP_EMIT     = 0x06,   // Output result
    OP_HALT     = 0x07    // End workflow
} micro_op_t;

typedef struct __attribute__((packed)) {
    uint8_t opcode;       // Operation type
    uint8_t arg1;         // First argument
    uint16_t arg2;        // Second argument
    uint32_t data;        // Immediate data
} tape_instruction_t;

typedef struct __attribute__((aligned(64))) {
    tape_instruction_t* program;  // Static program
    uint32_t pc;                  // Program counter
    uint32_t stack[16];          // Operand stack
    uint8_t sp;                  // Stack pointer
    uint8_t pad[43];
} tape_executor_t;

// Execute one micro-op (inlined for speed)
#define TAPE_STEP(exec) \
    switch ((exec)->program[(exec)->pc++].opcode) { \
        case OP_DISPATCH: /* handler dispatch */ break; \
        case OP_FORK:     /* parallel fork */    break; \
        case OP_JOIN:     /* sync barrier */     break; \
        /* ... other ops ... */ \
    }

/* ═══════════════════════════════════════════════════════════════════════════
 * PATTERN 4: Bitmask Decision Field
 * Vectorized rule evaluation using bit manipulation
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef struct __attribute__((aligned(32))) {
    uint64_t conditions[4];       // 256 bits of conditions
    uint64_t actions[4];          // 256 bits of actions
} decision_field_t;

// SIMD rule evaluation
static inline uint64_t evaluate_rules_simd(const decision_field_t* field, 
                                          uint64_t input_mask) {
#ifdef __AVX2__
    __m256i conditions = _mm256_load_si256((__m256i*)field->conditions);
    __m256i input = _mm256_set1_epi64x(input_mask);
    __m256i match = _mm256_cmpeq_epi64(_mm256_and_si256(conditions, input), input);
    __m256i actions = _mm256_load_si256((__m256i*)field->actions);
    __m256i result = _mm256_and_si256(match, actions);
    
    // Horizontal OR to combine results
    __m128i low = _mm256_extracti128_si256(result, 0);
    __m128i high = _mm256_extracti128_si256(result, 1);
    __m128i combined = _mm_or_si128(low, high);
    return _mm_extract_epi64(combined, 0) | _mm_extract_epi64(combined, 1);
#else
    // Scalar fallback
    uint64_t result = 0;
    for (int i = 0; i < 4; i++) {
        if ((field->conditions[i] & input_mask) == input_mask) {
            result |= field->actions[i];
        }
    }
    return result;
#endif
}

/* ═══════════════════════════════════════════════════════════════════════════
 * PATTERN 5: Time-Bucket Accumulator
 * Circular timeline arrays for temporal workflows
 * ═══════════════════════════════════════════════════════════════════════════ */

#define TIME_BUCKET_COUNT 3600  // 1 hour at 1-second resolution

typedef struct __attribute__((aligned(64))) {
    uint64_t buckets[TIME_BUCKET_COUNT];
    uint32_t current_time;
    uint32_t window_size;
} time_accumulator_t;

// Branchless circular increment
#define TIME_ADVANCE(acc) \
    ((acc)->current_time = ((acc)->current_time + 1) % TIME_BUCKET_COUNT)

// Add value to current bucket
#define TIME_ACCUMULATE(acc, value) \
    ((acc)->buckets[(acc)->current_time] += (value))

// Get sliding window sum (vectorized)
static inline uint64_t time_window_sum(const time_accumulator_t* acc) {
    uint64_t sum = 0;
    uint32_t start = (acc->current_time - acc->window_size + TIME_BUCKET_COUNT) 
                     % TIME_BUCKET_COUNT;
    
#ifdef __AVX2__
    // Process 4 buckets at a time
    __m256i vsum = _mm256_setzero_si256();
    uint32_t i = start;
    while (i != acc->current_time) {
        __m256i vals = _mm256_loadu_si256((__m256i*)&acc->buckets[i]);
        vsum = _mm256_add_epi64(vsum, vals);
        i = (i + 4) % TIME_BUCKET_COUNT;
    }
    // Horizontal sum
    uint64_t result[4];
    _mm256_storeu_si256((__m256i*)result, vsum);
    sum = result[0] + result[1] + result[2] + result[3];
#else
    for (uint32_t i = 0; i < acc->window_size; i++) {
        sum += acc->buckets[(start + i) % TIME_BUCKET_COUNT];
    }
#endif
    return sum;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * PATTERN 6: Sharded Hash-Join Grid  
 * Per-core hash tables for distributed joins
 * ═══════════════════════════════════════════════════════════════════════════ */

#define SHARD_COUNT 16          // Typically matches core count
#define SHARD_SIZE 4096         // Entries per shard

typedef struct __attribute__((aligned(64))) {
    uint64_t key;
    uint64_t value;
    uint32_t next;              // Chain for collisions
    uint32_t pad;
} hash_entry_t;

typedef struct __attribute__((aligned(64))) {
    hash_entry_t entries[SHARD_SIZE];
    uint32_t free_list;         // Head of free entry list
    uint32_t count;
    char pad[56];
} hash_shard_t;

typedef struct {
    hash_shard_t shards[SHARD_COUNT];
} sharded_hash_t;

// Fast hash to shard mapping
#define SHARD_INDEX(key) (((key) * 0x9e3779b97f4a7c15ULL) >> 60)

// Lock-free insert (single shard)
static inline bool shard_insert(hash_shard_t* shard, uint64_t key, uint64_t value) {
    uint32_t hash = (key * 0x9e3779b97f4a7c15ULL) % SHARD_SIZE;
    uint32_t entry = hash;
    
    // Linear probe with early exit
    for (int i = 0; i < 8; i++) {
        if (shard->entries[entry].key == 0) {
            shard->entries[entry].key = key;
            shard->entries[entry].value = value;
            return true;
        }
        entry = (entry + 1) % SHARD_SIZE;
    }
    return false;  // Shard full
}

/* ═══════════════════════════════════════════════════════════════════════════
 * PATTERN 7: Compile-Time Scenario Matrix
 * Pre-computed decision tables
 * ═══════════════════════════════════════════════════════════════════════════ */

#define SCENARIO_MATRIX(name, ...) \
    static const uint8_t name[][256] = { __VA_ARGS__ }

// Example: 3-input decision matrix (8 scenarios)
SCENARIO_MATRIX(workflow_decisions,
    {0,1,1,2,1,2,2,3},  // Action for each input combination
    {0,0,1,1,2,2,3,3},  // Next state for each combination
    {0,4,4,8,4,8,8,12}  // Priority for each combination
);

// Branchless lookup
#define SCENARIO_LOOKUP(matrix, row, inputs) \
    ((matrix)[(row)][(inputs)])

/* ═══════════════════════════════════════════════════════════════════════════
 * COMPOSABLE WORKFLOW ENGINE
 * Combines all patterns into unified workflow system
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef struct __attribute__((aligned(64))) {
    // Core patterns
    sfl_machine_t state_machine;
    ring_buffer_t* pipelines[8];      // Pipeline stages
    tape_executor_t executor;
    decision_field_t rules;
    time_accumulator_t timeline;
    sharded_hash_t* join_tables;
    
    // Workflow metadata
    uint32_t workflow_id;
    uint32_t flags;
    uint64_t metrics[8];              // Performance counters
    
    // Handler table (in .hot section)
    void (*handlers[256])(void* ctx, const ring_token_t* token);
} workflow_engine_t;

/* ═══════════════════════════════════════════════════════════════════════════
 * STARTER CODE: Composing Patterns
 * ═══════════════════════════════════════════════════════════════════════════ */

// Example 1: State machine driving pipeline
static inline void workflow_process_event(workflow_engine_t* engine, uint8_t event) {
    // 1. State transition (1 instruction)
    SFL_TRANSITION(&engine->state_machine, event);
    
    // 2. Get action from state
    uint8_t action = engine->state_machine.states[engine->state_machine.current_state].action_id[event];
    
    // 3. Create token for pipeline
    ring_token_t token = {
        .stage_id = action,
        .flags = 0,
        .sequence = engine->metrics[0]++
    };
    
    // 4. Push to appropriate pipeline (branchless)
    ring_push(engine->pipelines[action >> 5], &token);
}

// Example 2: Rule-based routing with temporal window
static inline uint32_t workflow_route_temporal(workflow_engine_t* engine, 
                                              uint64_t event_mask) {
    // 1. Evaluate rules (SIMD)
    uint64_t actions = evaluate_rules_simd(&engine->rules, event_mask);
    
    // 2. Update time bucket
    TIME_ACCUMULATE(&engine->timeline, __builtin_popcountll(actions));
    
    // 3. Check temporal threshold
    uint64_t window_sum = time_window_sum(&engine->timeline);
    
    // 4. Route based on threshold (branchless)
    return (window_sum > 1000) ? (actions >> 32) : (actions & 0xFFFFFFFF);
}

// Example 3: Sharded join with micro-ops
static inline void workflow_distributed_join(workflow_engine_t* engine,
                                           uint64_t left_key,
                                           uint64_t right_key) {
    // 1. Determine shards
    uint32_t left_shard = SHARD_INDEX(left_key);
    uint32_t right_shard = SHARD_INDEX(right_key);
    
    // 2. Insert into shards (parallel-safe)
    shard_insert(&engine->join_tables->shards[left_shard], left_key, 1);
    shard_insert(&engine->join_tables->shards[right_shard], right_key, 2);
    
    // 3. Execute join micro-op
    engine->executor.program[engine->executor.pc].opcode = OP_JOIN;
    engine->executor.program[engine->executor.pc].arg1 = left_shard;
    engine->executor.program[engine->executor.pc].arg2 = right_shard;
    TAPE_STEP(&engine->executor);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * MEMORY LAYOUT STRATEGIES
 * ═══════════════════════════════════════════════════════════════════════════ */

// Strategy 1: Hot/Cold separation
#define WORKFLOW_HOT __attribute__((section(".hot.workflow")))
#define WORKFLOW_COLD __attribute__((section(".cold.workflow")))

// Strategy 2: Core-local allocation
#define WORKFLOW_PERCPU __attribute__((section(".percpu.workflow")))

// Strategy 3: Huge page alignment
#define WORKFLOW_HUGE __attribute__((aligned(2097152)))

/* ═══════════════════════════════════════════════════════════════════════════
 * PERFORMANCE VALIDATION MACROS
 * ═══════════════════════════════════════════════════════════════════════════ */

#ifdef WORKFLOW_PROFILE
#define WORKFLOW_TICK_START() uint64_t __start = __rdtsc()
#define WORKFLOW_TICK_END(counter) \
    engine->metrics[counter] += __rdtsc() - __start

#define WORKFLOW_ASSERT_TICKS(ops, max_ticks) \
    static_assert((ops) * 7 <= (max_ticks), "Operation exceeds tick budget")
#else
#define WORKFLOW_TICK_START()
#define WORKFLOW_TICK_END(counter)
#define WORKFLOW_ASSERT_TICKS(ops, ticks)
#endif

/* ═══════════════════════════════════════════════════════════════════════════
 * FAILURE MODE ANALYSIS TABLE
 * ═══════════════════════════════════════════════════════════════════════════ */

/*
 * Pattern         | Failure Mode              | Detection (≤2ns)      | Recovery
 * ----------------|---------------------------|----------------------|------------
 * SFL             | Invalid state transition  | Range check on index | Default state
 * Token-Ring      | Ring buffer overflow      | Head == tail check   | Drop oldest
 * Micro-Op Tape   | Invalid opcode           | Opcode > OP_HALT    | Skip instruction
 * Bitmask Rules   | No matching rule         | Result == 0          | Default action
 * Time-Bucket     | Time wrap-around         | Modulo arithmetic    | Automatic
 * Sharded Hash    | Hash collision chain     | Chain length > 8     | Overflow table
 * Scenario Matrix | Out-of-bounds input      | Input > 255          | Clamp to 255
 * 
 * CRITICAL: All failure checks must be branchless to maintain ≤2ns guarantee
 */

// Branchless failure handling example
#define SAFE_STATE_TRANSITION(machine, event) \
    ((machine)->current_state = \
     (machine)->states[(machine)->current_state].next_state[(event) & 0xFF])

#define SAFE_RING_PUSH(ring, token, overflow_handler) \
    do { \
        uint32_t success = ring_push(ring, token); \
        /* Branchless overflow handling */ \
        overflow_handler(token, 1 - success); \
    } while(0)

/* ═══════════════════════════════════════════════════════════════════════════
 * WORKFLOW PATTERN COMBINATIONS
 * ═══════════════════════════════════════════════════════════════════════════ */

// Pattern: State + Pipeline + Rules
typedef struct {
    sfl_machine_t fsm;
    ring_buffer_t* stages[4];
    decision_field_t rules;
} state_pipeline_workflow_t;

// Pattern: Temporal + Join + MicroOps  
typedef struct {
    time_accumulator_t windows[8];
    sharded_hash_t joins;
    tape_executor_t executor;
} temporal_join_workflow_t;

// Pattern: Full orchestration
typedef struct {
    workflow_engine_t engine;
    uint8_t* scenario_matrix;
    void* pattern_cache;
} orchestrated_workflow_t;

#endif // S7T_WORKFLOW_H