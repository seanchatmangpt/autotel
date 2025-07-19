/*  ─────────────────────────────────────────────────────────────
    s7t_patterns.h  –  Nanosecond Design Patterns (v1.0)
    ───────────────────────────────────────────────────────────── */
#ifndef S7T_PATTERNS_H
#define S7T_PATTERNS_H

#include "s7t.h"

/*═══════════════════════════════════════════════════════════════
  Pattern 1: Singleton → Static Cache-Aligned Struct
  ═══════════════════════════════════════════════════════════════*/

#define S7T_SINGLETON_DECLARE(type, name) \
    static type S7T_ALIGNED(64) name##_instance; \
    S7T_ALWAYS_INLINE type* name##_get(void) { \
        return &name##_instance; \
    }

// Example usage:
// typedef struct { uint32_t config; uint64_t state; } Config;
// S7T_SINGLETON_DECLARE(Config, g_config)

/*═══════════════════════════════════════════════════════════════
  Pattern 2: Factory → Enum-Indexed Constructor LUT
  ═══════════════════════════════════════════════════════════════*/

#define S7T_FACTORY_DECLARE(type, name, ...) \
    typedef type* (*name##_ctor_t)(s7t_arena_t*); \
    static name##_ctor_t const name##_ctors[] = { __VA_ARGS__ }; \
    S7T_ALWAYS_INLINE type* name##_create(s7t_arena_t* arena, uint32_t kind) { \
        return name##_ctors[kind](arena); \
    }

/*═══════════════════════════════════════════════════════════════
  Pattern 3: Builder → Designated Initializer Macro
  ═══════════════════════════════════════════════════════════════*/

#define S7T_BUILDER(type, ...) \
    ((type){ __VA_ARGS__ })

#define S7T_BUILDER_ALLOC(arena, type, ...) ({ \
    type* p = (type*)s7t_arena_alloc(arena, sizeof(type)); \
    if (p) *p = (type){ __VA_ARGS__ }; \
    p; \
})

/*═══════════════════════════════════════════════════════════════
  Pattern 4: Strategy → Dense Function-Pointer Jump Table
  ═══════════════════════════════════════════════════════════════*/

typedef struct S7T_ALIGNED(64) {
    uint8_t strategy_id;
    uint8_t reserved[7];
    void* context;
} s7t_strategy_t;

#define S7T_STRATEGY_TABLE(name, return_type, ...) \
    typedef return_type (*name##_func_t)(void*); \
    static name##_func_t const name##_table[] = { __VA_ARGS__ }; \
    S7T_ALWAYS_INLINE return_type name##_execute(s7t_strategy_t* s) { \
        return name##_table[s->strategy_id](s->context); \
    }

/*═══════════════════════════════════════════════════════════════
  Pattern 5: State → Static Finite-State Lattice
  ═══════════════════════════════════════════════════════════════*/

typedef struct {
    uint16_t next_state;
    uint16_t action;
} s7t_state_transition_t;

typedef struct S7T_ALIGNED(64) {
    s7t_state_transition_t* lattice;  // [state][event] → transition
    uint16_t current_state;
    uint16_t num_states;
    uint16_t num_events;
    uint16_t reserved;
} s7t_state_machine_t;

S7T_ALWAYS_INLINE void s7t_state_transition(s7t_state_machine_t* sm, uint16_t event) {
    uint32_t idx = (sm->current_state * sm->num_events) + event;
    s7t_state_transition_t* trans = &sm->lattice[idx];
    sm->current_state = trans->next_state;
}

#define S7T_STATE_MACHINE_DECLARE(name, states, events) \
    static s7t_state_transition_t name##_lattice[(states) * (events)] S7T_ALIGNED(64); \
    static s7t_state_machine_t name = { \
        .lattice = name##_lattice, \
        .current_state = 0, \
        .num_states = (states), \
        .num_events = (events) \
    }

/*═══════════════════════════════════════════════════════════════
  Pattern 6: Observer → Ring-Buffer Fan-Out
  ═══════════════════════════════════════════════════════════════*/

#define S7T_RING_SIZE 1024  // Must be power of 2

typedef struct S7T_ALIGNED(64) {
    uint64_t data[8];  // 64-byte event
} s7t_event_t;

typedef struct S7T_ALIGNED(64) {
    s7t_event_t buffer[S7T_RING_SIZE];
    s7t_atomic_u32 head;
    s7t_atomic_u32 tail;
} s7t_ring_buffer_t;

S7T_ALWAYS_INLINE bool s7t_ring_push(s7t_ring_buffer_t* ring, const s7t_event_t* event) {
    uint32_t head = atomic_load_explicit(&ring->head, memory_order_acquire);
    uint32_t next = (head + 1) & (S7T_RING_SIZE - 1);
    uint32_t tail = atomic_load_explicit(&ring->tail, memory_order_acquire);
    
    if (S7T_UNLIKELY(next == tail)) return false;  // Full
    
    ring->buffer[head] = *event;
    atomic_store_explicit(&ring->head, next, memory_order_release);
    return true;
}

S7T_ALWAYS_INLINE bool s7t_ring_pop(s7t_ring_buffer_t* ring, s7t_event_t* event) {
    uint32_t tail = atomic_load_explicit(&ring->tail, memory_order_acquire);
    uint32_t head = atomic_load_explicit(&ring->head, memory_order_acquire);
    
    if (S7T_UNLIKELY(tail == head)) return false;  // Empty
    
    *event = ring->buffer[tail];
    uint32_t next = (tail + 1) & (S7T_RING_SIZE - 1);
    atomic_store_explicit(&ring->tail, next, memory_order_release);
    return true;
}

/*═══════════════════════════════════════════════════════════════
  Pattern 7: Command → Micro-Op Tape Execution
  ═══════════════════════════════════════════════════════════════*/

typedef struct {
    uint8_t opcode;
    uint8_t dst;
    uint8_t src1;
    uint8_t src2;
    uint32_t immediate;
} s7t_micro_op_t;

typedef struct S7T_ALIGNED(64) {
    s7t_micro_op_t* ops;
    uint32_t* registers;
    uint32_t op_count;
    uint32_t reg_count;
    uint32_t pc;  // Program counter
} s7t_command_processor_t;

#define S7T_OP_HANDLER(name) \
    S7T_ALWAYS_INLINE void name(s7t_command_processor_t* cpu, s7t_micro_op_t* op)

// Example handlers
S7T_OP_HANDLER(op_load) { cpu->registers[op->dst] = op->immediate; }
S7T_OP_HANDLER(op_add) { cpu->registers[op->dst] = cpu->registers[op->src1] + cpu->registers[op->src2]; }
S7T_OP_HANDLER(op_store) { (void)cpu; (void)op; /* Implementation */ }

typedef void (*s7t_op_handler_t)(s7t_command_processor_t*, s7t_micro_op_t*);

#define S7T_COMMAND_TABLE_DECLARE(name, ...) \
    static s7t_op_handler_t const name##_handlers[] = { __VA_ARGS__ }; \
    S7T_ALWAYS_INLINE void name##_execute(s7t_command_processor_t* cpu) { \
        while (cpu->pc < cpu->op_count) { \
            s7t_micro_op_t* op = &cpu->ops[cpu->pc++]; \
            name##_handlers[op->opcode](cpu, op); \
        } \
    }

/*═══════════════════════════════════════════════════════════════
  Pattern 8: Chain of Responsibility → Token-Ring Pipeline
  ═══════════════════════════════════════════════════════════════*/

// Token type moved to s7t_workflow.h to avoid redefinition

// Stage handler type moved to s7t_workflow.h

// Pipeline stage type moved to s7t_workflow.h to avoid redefinition

// Pipeline run function moved to s7t_workflow.h

/*═══════════════════════════════════════════════════════════════
  Pattern 9: Flyweight → Interned-ID Table
  ═══════════════════════════════════════════════════════════════*/

typedef struct S7T_ALIGNED(64) {
    s7t_id_t* table;
    uint32_t* hashes;
    const char** strings;
    uint32_t capacity;
    uint32_t count;
} s7t_flyweight_t;

S7T_ALWAYS_INLINE s7t_id_t s7t_flyweight_intern(s7t_flyweight_t* fw, const char* str, uint32_t len) {
    uint32_t hash = s7t_hash_string(str, len);
    uint32_t idx = hash & (fw->capacity - 1);
    
    // Linear probe
    while (fw->table[idx] != S7T_ID_NULL) {
        if (fw->hashes[idx] == hash && 
            memcmp(fw->strings[idx], str, len) == 0) {
            return fw->table[idx];  // Found
        }
        idx = (idx + 1) & (fw->capacity - 1);
    }
    
    // Insert new
    s7t_id_t id = fw->count++;
    fw->table[idx] = id;
    fw->hashes[idx] = hash;
    fw->strings[idx] = str;
    return id;
}

/*═══════════════════════════════════════════════════════════════
  Pattern 10: Iterator → Index Cursor with Stride
  ═══════════════════════════════════════════════════════════════*/

typedef struct {
    uint32_t* data;
    uint32_t current;
    uint32_t end;
    uint32_t stride;
} s7t_iterator_t;

S7T_ALWAYS_INLINE void s7t_iterator_init(s7t_iterator_t* it, uint32_t* data, uint32_t count, uint32_t stride) {
    it->data = data;
    it->current = 0;
    it->end = count;
    it->stride = stride;
}

S7T_ALWAYS_INLINE bool s7t_iterator_next(s7t_iterator_t* it, uint32_t* value) {
    if (it->current >= it->end) return false;
    *value = it->data[it->current];
    it->current += it->stride;
    return true;
}

// Unrolled iteration macro
#define S7T_ITERATE_UNROLL4(it, value, body) do { \
    S7T_UNROLL(4) \
    while (s7t_iterator_next(&(it), &(value))) { \
        body \
    } \
} while(0)

/*═══════════════════════════════════════════════════════════════
  Pattern 11: Visitor → Switch-to-Table Dispatch
  ═══════════════════════════════════════════════════════════════*/

typedef struct {
    uint8_t type;
    uint8_t reserved[7];
    void* data;
} s7t_node_t;

#define S7T_VISITOR_TABLE(name, return_type, ...) \
    typedef return_type (*name##_visit_func_t)(s7t_node_t*, void*); \
    static name##_visit_func_t const name##_visitors[] = { __VA_ARGS__ }; \
    S7T_ALWAYS_INLINE return_type name##_visit(s7t_node_t* node, void* ctx) { \
        return name##_visitors[node->type](node, ctx); \
    }

/*═══════════════════════════════════════════════════════════════
  Pattern 12: Template Method → Inlined Skeleton
  ═══════════════════════════════════════════════════════════════*/

#define S7T_TEMPLATE_METHOD(name, setup, process, cleanup) \
    S7T_ALWAYS_INLINE void name(void* ctx) { \
        setup(ctx); \
        process(ctx); \
        cleanup(ctx); \
    }

// Hook implementations must be always_inline
#define S7T_HOOK(name) S7T_ALWAYS_INLINE void name(void* ctx)

/*═══════════════════════════════════════════════════════════════
  Pattern 13: Decorator → Bitmask Attribute Field
  ═══════════════════════════════════════════════════════════════*/

typedef struct S7T_ALIGNED(64) {
    uint64_t attributes;  // Bit flags
    void* base_object;
} s7t_decorated_t;

#define S7T_ATTR_FLAG(n) (1ULL << (n))

S7T_ALWAYS_INLINE void s7t_decorate_add(s7t_decorated_t* obj, uint64_t attrs) {
    obj->attributes |= attrs;
}

S7T_ALWAYS_INLINE void s7t_decorate_remove(s7t_decorated_t* obj, uint64_t attrs) {
    obj->attributes &= ~attrs;
}

S7T_ALWAYS_INLINE bool s7t_decorate_has(s7t_decorated_t* obj, uint64_t attrs) {
    return (obj->attributes & attrs) == attrs;
}

// Branch-free conditional decoration
S7T_ALWAYS_INLINE uint32_t s7t_decorate_apply(s7t_decorated_t* obj, uint32_t base_value) {
    // Example: Each attribute bit doubles the value
    uint32_t multiplier = s7t_popcount(obj->attributes) + 1;
    return base_value * multiplier;
}

/*═══════════════════════════════════════════════════════════════
  Pattern 14: Prototype → Memcpy from Cache-Aligned Template
  ═══════════════════════════════════════════════════════════════*/

#define S7T_PROTOTYPE_DECLARE(type, name) \
    static const type name##_prototype S7T_ALIGNED(64) = 

#define S7T_PROTOTYPE_CLONE(arena, type, proto) ({ \
    type* clone = (type*)s7t_arena_alloc(arena, sizeof(type)); \
    if (clone) memcpy(clone, &(proto), sizeof(type)); \
    clone; \
})

// Fast clone with prefetch
#define S7T_PROTOTYPE_CLONE_FAST(arena, type, proto) ({ \
    s7t_prefetch_r(&(proto)); \
    type* clone = (type*)s7t_arena_alloc(arena, sizeof(type)); \
    if (clone) memcpy(clone, &(proto), sizeof(type)); \
    clone; \
})

/*═══════════════════════════════════════════════════════════════
  Composite Pattern Example: Request Processing Pipeline
  ═══════════════════════════════════════════════════════════════*/

// Combines Factory, Strategy, State, and Command patterns
typedef struct S7T_ALIGNED(64) {
    s7t_id_t request_type;       // Flyweight
    uint8_t strategy_id;         // Strategy selection
    uint8_t state;               // Current state
    uint16_t command_count;      // Number of commands
    s7t_micro_op_t commands[10]; // Inline command buffer
} s7t_request_t;

// Process request using combined patterns
S7T_ALWAYS_INLINE uint32_t s7t_process_request(
    s7t_request_t* req,
    s7t_state_machine_t* sm,
    s7t_strategy_t* strategies,
    s7t_command_processor_t* cpu
) {
    // State transition based on request type
    s7t_state_transition(sm, req->request_type);
    
    // Execute strategy
    strategies[req->strategy_id].context = req;
    uint32_t result = 0; // strategy_execute would update this
    
    // Execute commands
    cpu->ops = req->commands;
    cpu->op_count = req->command_count;
    cpu->pc = 0;
    // command_execute(cpu);
    
    return result;
}

#endif /* S7T_PATTERNS_H */