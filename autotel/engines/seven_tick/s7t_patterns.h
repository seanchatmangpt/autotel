#ifndef S7T_PATTERNS_H
#define S7T_PATTERNS_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "lib/7t_common.h"

// ============================================================================
// NANOSECOND DESIGN PATTERNS - Physics-Compliant Versions
// ============================================================================
// Zero allocation at steady state, ID-based behavior, data locality,
// compile-time wiring, ≤1 predictable branch per operation

// ============================================================================
// 1. SINGLETON → Static Cache-Aligned Struct
// ============================================================================
// Instead of heap allocation and pointer indirection, use static storage
// aligned to cache line boundaries for optimal CPU access

#define CACHE_LINE_SIZE 64

typedef struct __attribute__((aligned(CACHE_LINE_SIZE)))
{
    uint32_t instance_id;
    uint64_t data[7]; // 56 bytes of data to fill cache line
    uint32_t initialized;
} S7T_Singleton;

// Static singleton instance - no allocation needed
static S7T_Singleton g_singleton __attribute__((aligned(CACHE_LINE_SIZE))) = {0};

static inline S7T_Singleton *s7t_singleton_get(void)
{
    if (!g_singleton.initialized)
    {
        g_singleton.instance_id = 0x7777;
        g_singleton.initialized = 1;
    }
    return &g_singleton;
}

// Example usage:
// S7T_Singleton* config = s7t_singleton_get();
// config->data[0] = settings_value;

// ============================================================================
// 2. FACTORY → Enum-Indexed Constructor LUT
// ============================================================================
// Replace virtual dispatch with compile-time lookup table indexed by enum

typedef enum
{
    S7T_TYPE_PROCESSOR = 0,
    S7T_TYPE_ANALYZER = 1,
    S7T_TYPE_VALIDATOR = 2,
    S7T_TYPE_TRANSFORMER = 3,
    S7T_TYPE_MAX
} S7T_ObjectType;

typedef struct
{
    uint32_t type_id;
    uint32_t flags;
    uint64_t data;
} S7T_Object;

// Constructor function type
typedef void (*S7T_Constructor)(S7T_Object *obj);

// Compile-time constructor lookup table
static void s7t_construct_processor(S7T_Object *obj)
{
    obj->flags = 0x01;
    obj->data = 0;
}

static void s7t_construct_analyzer(S7T_Object *obj)
{
    obj->flags = 0x02;
    obj->data = 0;
}

static void s7t_construct_validator(S7T_Object *obj)
{
    obj->flags = 0x04;
    obj->data = 0;
}

static void s7t_construct_transformer(S7T_Object *obj)
{
    obj->flags = 0x08;
    obj->data = 0;
}

static const S7T_Constructor s7t_constructors[S7T_TYPE_MAX] = {
    s7t_construct_processor,
    s7t_construct_analyzer,
    s7t_construct_validator,
    s7t_construct_transformer};

static inline void s7t_factory_create(S7T_Object *obj, S7T_ObjectType type)
{
    obj->type_id = type;
    s7t_constructors[type](obj); // Direct indexed call, no branches
}

// Example usage:
// S7T_Object obj;
// s7t_factory_create(&obj, S7T_TYPE_ANALYZER);

// ============================================================================
// 3. BUILDER → Designated Initializer Macro
// ============================================================================
// Use C99 designated initializers for compile-time object construction

#define S7T_BUILDER_INIT(name, ...) \
    S7T_Config name = {             \
        .version = 1,               \
        .flags = 0,                 \
        __VA_ARGS__}

typedef struct
{
    uint32_t version;
    uint32_t flags;
    uint32_t buffer_size;
    uint32_t max_connections;
    uint32_t timeout_ms;
    uint32_t reserved[3];
} S7T_Config;

// Example usage:
// S7T_BUILDER_INIT(config,
//     .buffer_size = 4096,
//     .max_connections = 100,
//     .timeout_ms = 5000
// );

// ============================================================================
// 4. STRATEGY → Dense Function-Pointer Jump Table
// ============================================================================
// Pack function pointers contiguously for cache efficiency

typedef uint32_t (*S7T_Strategy)(uint32_t input);

// Dense strategy table - all pointers packed together
static uint32_t s7t_strategy_fast(uint32_t x) { return x << 1; }
static uint32_t s7t_strategy_normal(uint32_t x) { return x * 3; }
static uint32_t s7t_strategy_precise(uint32_t x) { return x * x; }

static const S7T_Strategy s7t_strategies[] = {
    s7t_strategy_fast,
    s7t_strategy_normal,
    s7t_strategy_precise};

static inline uint32_t s7t_execute_strategy(uint32_t strategy_id, uint32_t input)
{
    return s7t_strategies[strategy_id & 0x3](input); // Mask for safety
}

// Example usage:
// uint32_t result = s7t_execute_strategy(STRATEGY_FAST, 42);

// ============================================================================
// 5. STATE → Static Finite-State Lattice
// ============================================================================
// Pre-computed state transition table with zero branches

typedef enum
{
    S7T_STATE_IDLE = 0,
    S7T_STATE_LOADING = 1,
    S7T_STATE_PROCESSING = 2,
    S7T_STATE_COMPLETE = 3,
    S7T_STATE_ERROR = 4,
    S7T_STATE_COUNT
} S7T_State;

typedef enum
{
    S7T_EVENT_START = 0,
    S7T_EVENT_DATA = 1,
    S7T_EVENT_FINISH = 2,
    S7T_EVENT_ABORT = 3,
    S7T_EVENT_COUNT
} S7T_Event;

// State transition lattice: [current_state][event] = next_state
static const uint8_t s7t_state_lattice[S7T_STATE_COUNT][S7T_EVENT_COUNT] = {
    //                START,           DATA,            FINISH,          ABORT
    [S7T_STATE_IDLE] = {S7T_STATE_LOADING, S7T_STATE_IDLE, S7T_STATE_IDLE, S7T_STATE_IDLE},
    [S7T_STATE_LOADING] = {S7T_STATE_LOADING, S7T_STATE_PROCESSING, S7T_STATE_ERROR, S7T_STATE_IDLE},
    [S7T_STATE_PROCESSING] = {S7T_STATE_ERROR, S7T_STATE_PROCESSING, S7T_STATE_COMPLETE, S7T_STATE_IDLE},
    [S7T_STATE_COMPLETE] = {S7T_STATE_LOADING, S7T_STATE_ERROR, S7T_STATE_COMPLETE, S7T_STATE_IDLE},
    [S7T_STATE_ERROR] = {S7T_STATE_LOADING, S7T_STATE_ERROR, S7T_STATE_ERROR, S7T_STATE_IDLE}};

static inline S7T_State s7t_state_transition(S7T_State current, S7T_Event event)
{
    return (S7T_State)s7t_state_lattice[current][event]; // Direct lookup, no branches
}

// Example usage:
// S7T_State state = S7T_STATE_IDLE;
// state = s7t_state_transition(state, S7T_EVENT_START);

// ============================================================================
// 6. OBSERVER → Ring-Buffer Fan-Out
// ============================================================================
// Fixed-size ring buffer for event distribution without allocation

#define S7T_MAX_OBSERVERS 16
#define S7T_EVENT_QUEUE_SIZE 64

typedef struct
{
    uint32_t event_type;
    uint32_t data;
    uint32_t timestamp;
} S7T_Event_Data;

typedef void (*S7T_Observer)(const S7T_Event_Data *event);

typedef struct
{
    S7T_Observer observers[S7T_MAX_OBSERVERS];
    uint32_t observer_count;

    S7T_Event_Data events[S7T_EVENT_QUEUE_SIZE];
    uint32_t write_idx;
    uint32_t read_idx;
} S7T_EventSystem;

static inline void s7t_publish_event(S7T_EventSystem *sys, uint32_t type, uint32_t data)
{
    uint32_t idx = sys->write_idx & (S7T_EVENT_QUEUE_SIZE - 1);
    sys->events[idx].event_type = type;
    sys->events[idx].data = data;
    sys->events[idx].timestamp = 0; // Safe timestamp for testing

    sys->write_idx++;

    // Fan-out to all observers
    for (uint32_t i = 0; i < sys->observer_count; i++)
    {
        sys->observers[i](&sys->events[idx]);
    }
}

// Example usage:
// S7T_EventSystem event_sys = {0};
// event_sys.observers[event_sys.observer_count++] = my_handler;
// s7t_publish_event(&event_sys, EVENT_DATA_READY, 42);

// ============================================================================
// 7. COMMAND → Micro-Op Tape Execution
// ============================================================================
// Commands as bytecode on a tape for sequential execution

typedef enum
{
    S7T_OP_NOP = 0,
    S7T_OP_LOAD = 1,
    S7T_OP_STORE = 2,
    S7T_OP_ADD = 3,
    S7T_OP_MUL = 4,
    S7T_OP_JUMP = 5,
    S7T_OP_HALT = 6
} S7T_OpCode;

typedef struct
{
    uint8_t opcode;
    uint8_t reg;
    uint16_t operand;
} S7T_Command;

typedef struct
{
    S7T_Command tape[256];
    uint32_t pc; // Program counter
    uint32_t registers[8];
} S7T_CommandProcessor;

static inline void s7t_execute_commands(S7T_CommandProcessor *proc, uint32_t count)
{
    for (uint32_t i = 0; i < count && proc->pc < 256; i++)
    {
        S7T_Command *cmd = &proc->tape[proc->pc];

        switch (cmd->opcode)
        {
        case S7T_OP_LOAD:
            proc->registers[cmd->reg] = cmd->operand;
            break;
        case S7T_OP_STORE: /* Store to memory */
            break;
        case S7T_OP_ADD:
            proc->registers[cmd->reg] += cmd->operand;
            break;
        case S7T_OP_MUL:
            proc->registers[cmd->reg] *= cmd->operand;
            break;
        case S7T_OP_JUMP:
            proc->pc = cmd->operand - 1;
            break;
        case S7T_OP_HALT:
            return;
        default:
            break;
        }
        proc->pc++;
    }
}

// Example usage:
// S7T_CommandProcessor proc = {0};
// proc.tape[0] = (S7T_Command){S7T_OP_LOAD, 0, 100};
// proc.tape[1] = (S7T_Command){S7T_OP_ADD, 0, 50};
// s7t_execute_commands(&proc, 2);

// ============================================================================
// 8. CHAIN OF RESPONSIBILITY → Token-Ring Pipeline
// ============================================================================
// Fixed pipeline with token passing, no dynamic allocation

#define S7T_PIPELINE_STAGES 8

typedef struct
{
    uint32_t token_id;
    uint32_t data;
    uint32_t flags;
} S7T_Token;

typedef uint32_t (*S7T_StageHandler)(S7T_Token *token);

typedef struct
{
    S7T_StageHandler stages[S7T_PIPELINE_STAGES];
    uint32_t stage_count;
} S7T_Pipeline;

static inline uint32_t s7t_process_pipeline(S7T_Pipeline *pipe, S7T_Token *token)
{
    for (uint32_t i = 0; i < pipe->stage_count; i++)
    {
        uint32_t result = pipe->stages[i](token);
        if (result == 0)
            break; // Stage consumed token
    }
    return token->flags;
}

// Example usage:
// S7T_Pipeline pipe = {0};
// pipe.stages[pipe.stage_count++] = validate_stage;
// pipe.stages[pipe.stage_count++] = transform_stage;
// S7T_Token token = {.token_id = 1, .data = 42};
// s7t_process_pipeline(&pipe, &token);

// ============================================================================
// 9. FLYWEIGHT → Interned-ID Table
// ============================================================================
// Shared immutable data referenced by IDs

#define S7T_INTERN_TABLE_SIZE 1024

typedef struct
{
    uint32_t hash;
    const char *data;
    uint32_t length;
} S7T_InternEntry;

typedef struct
{
    S7T_InternEntry entries[S7T_INTERN_TABLE_SIZE];
    uint32_t count;
} S7T_InternTable;

static inline uint32_t s7t_intern_string(S7T_InternTable *table, const char *str)
{
    uint32_t hash = fnv1a_hash32(str);
    uint32_t idx = hash & (S7T_INTERN_TABLE_SIZE - 1);

    // Linear probe for existing entry
    for (uint32_t i = 0; i < 4; i++)
    {
        uint32_t slot = (idx + i) & (S7T_INTERN_TABLE_SIZE - 1);
        if (table->entries[slot].hash == hash &&
            strcmp(table->entries[slot].data, str) == 0)
        {
            return slot; // Return existing ID
        }
        if (table->entries[slot].data == NULL)
        {
            // New entry
            table->entries[slot].hash = hash;
            table->entries[slot].data = str;
            table->entries[slot].length = strlen(str);
            table->count++;
            return slot;
        }
    }
    return 0; // Table full
}

// Example usage:
// S7T_InternTable intern_table = {0};
// uint32_t id1 = s7t_intern_string(&intern_table, "hello");
// uint32_t id2 = s7t_intern_string(&intern_table, "hello"); // Same ID

// ============================================================================
// 10. ITERATOR → Index Cursor with Stride
// ============================================================================
// Zero-allocation iteration using index and stride

typedef struct
{
    const void *data;
    size_t element_size;
    size_t count;
    size_t current;
    size_t stride;
} S7T_Iterator;

static inline void s7t_iterator_init(S7T_Iterator *it, const void *data,
                                     size_t elem_size, size_t count, size_t stride)
{
    it->data = data;
    it->element_size = elem_size;
    it->count = count;
    it->current = 0;
    it->stride = stride;
}

static inline void *s7t_iterator_next(S7T_Iterator *it)
{
    if (it->current >= it->count)
        return NULL;

    void *elem = (char *)it->data + (it->current * it->element_size);
    it->current += it->stride;
    return elem;
}

// Example usage:
// uint32_t data[100];
// S7T_Iterator it;
// s7t_iterator_init(&it, data, sizeof(uint32_t), 100, 2); // Every 2nd element
// while ((uint32_t* val = s7t_iterator_next(&it)) != NULL) { ... }

// ============================================================================
// 11. VISITOR → Switch-to-Table Dispatch
// ============================================================================
// Replace virtual dispatch with switch statement compiled to jump table

typedef enum
{
    S7T_NODE_LITERAL = 0,
    S7T_NODE_BINARY = 1,
    S7T_NODE_UNARY = 2,
    S7T_NODE_CALL = 3,
    S7T_NODE_TYPE_COUNT
} S7T_NodeType;

typedef struct
{
    S7T_NodeType type;
    uint32_t data;
} S7T_Node;

typedef void (*S7T_Visitor)(S7T_Node *node, void *context);

typedef struct
{
    S7T_Visitor visitors[S7T_NODE_TYPE_COUNT];
} S7T_VisitorTable;

static inline void s7t_accept_visitor(S7T_Node *node, S7T_VisitorTable *table, void *context)
{
    table->visitors[node->type](node, context); // Direct dispatch
}

// Example usage:
// S7T_VisitorTable vtable = {
//     .visitors = {visit_literal, visit_binary, visit_unary, visit_call}
// };
// S7T_Node node = {.type = S7T_NODE_LITERAL, .data = 42};
// s7t_accept_visitor(&node, &vtable, context);

// ============================================================================
// 12. TEMPLATE METHOD → Inlined Skeleton with Compile-Time Hooks
// ============================================================================
// Use macros for compile-time template method pattern

#define S7T_TEMPLATE_ALGORITHM(name, pre_hook, process, post_hook) \
    static inline uint32_t name(uint32_t input)                    \
    {                                                              \
        uint32_t state = input;                                    \
        pre_hook(&state);                                          \
        process(&state);                                           \
        post_hook(&state);                                         \
        return state;                                              \
    }

// Define hooks
static inline void default_pre(uint32_t *state) { *state |= 0x1000; }
static inline void default_process(uint32_t *state) { *state *= 2; }
static inline void default_post(uint32_t *state) { *state &= 0xFFFF; }

// Instantiate template
S7T_TEMPLATE_ALGORITHM(process_standard, default_pre, default_process, default_post)

// Example usage:
// uint32_t result = process_standard(42);

// ============================================================================
// 13. DECORATOR → Bitmask Attribute Field
// ============================================================================
// Stack decorations as bit flags instead of wrapper objects

typedef struct
{
    uint32_t core_data;
    uint32_t attributes; // Bit flags for decorations
} S7T_Decorated;

// Attribute flags
#define S7T_ATTR_CACHED (1 << 0)
#define S7T_ATTR_VALIDATED (1 << 1)
#define S7T_ATTR_COMPRESSED (1 << 2)
#define S7T_ATTR_ENCRYPTED (1 << 3)
#define S7T_ATTR_LOGGED (1 << 4)

static inline void s7t_add_decoration(S7T_Decorated *obj, uint32_t attr)
{
    obj->attributes |= attr;
}

static inline void s7t_remove_decoration(S7T_Decorated *obj, uint32_t attr)
{
    obj->attributes &= ~attr;
}

static inline int s7t_has_decoration(S7T_Decorated *obj, uint32_t attr)
{
    return (obj->attributes & attr) != 0;
}

// Example usage:
// S7T_Decorated obj = {.core_data = 42, .attributes = 0};
// s7t_add_decoration(&obj, S7T_ATTR_CACHED | S7T_ATTR_VALIDATED);

// ============================================================================
// 14. PROTOTYPE → memcpy from Cache-Aligned Template
// ============================================================================
// Clone objects using memcpy from pre-initialized templates

typedef struct __attribute__((aligned(CACHE_LINE_SIZE)))
{
    uint32_t type;
    uint32_t version;
    uint64_t config[6];
    uint32_t checksum;
} S7T_Prototype;

// Static prototypes for common configurations
static const S7T_Prototype s7t_prototypes[] __attribute__((aligned(CACHE_LINE_SIZE))) = {
    {.type = 1, .version = 1, .config = {100, 200, 300, 0, 0, 0}, .checksum = 0x1234},
    {.type = 2, .version = 1, .config = {500, 600, 700, 0, 0, 0}, .checksum = 0x5678},
    {.type = 3, .version = 2, .config = {1000, 2000, 3000, 0, 0, 0}, .checksum = 0x9ABC}};

static inline void s7t_clone_from_prototype(S7T_Prototype *dest, uint32_t prototype_id)
{
    memcpy(dest, &s7t_prototypes[prototype_id & 0x3], sizeof(S7T_Prototype));
}

// Example usage:
// S7T_Prototype instance;
// s7t_clone_from_prototype(&instance, PROTOTYPE_STANDARD);

// ============================================================================
// USAGE EXAMPLES AND BENCHMARKS
// ============================================================================

#ifdef S7T_PATTERNS_EXAMPLE

#include <stdio.h>

void s7t_patterns_demo(void)
{
    printf("=== S7T Nanosecond Design Patterns Demo ===\n\n");

    // 1. Singleton Pattern
    printf("1. Singleton Pattern:\n");
    S7T_Singleton *config = s7t_singleton_get();
    config->data[0] = 0xDEADBEEF;
    printf("   Singleton ID: 0x%X, Data[0]: 0x%llX\n\n",
           config->instance_id, (unsigned long long)config->data[0]);

    // 2. Factory Pattern
    printf("2. Factory Pattern:\n");
    S7T_Object analyzer;
    s7t_factory_create(&analyzer, S7T_TYPE_ANALYZER);
    printf("   Created object type %u with flags 0x%X\n\n",
           analyzer.type_id, analyzer.flags);

    // 3. Builder Pattern
    printf("3. Builder Pattern:\n");
    S7T_BUILDER_INIT(server_config,
                     .buffer_size = 8192,
                     .max_connections = 1000,
                     .timeout_ms = 30000);
    printf("   Built config: buffer=%u, connections=%u, timeout=%u\n\n",
           server_config.buffer_size, server_config.max_connections,
           server_config.timeout_ms);

    // 4. Strategy Pattern
    printf("4. Strategy Pattern:\n");
    uint32_t result = s7t_execute_strategy(0, 10); // Fast strategy
    printf("   Fast strategy(10) = %u\n", result);
    result = s7t_execute_strategy(2, 10); // Precise strategy
    printf("   Precise strategy(10) = %u\n\n", result);

    // 5. State Pattern
    printf("5. State Pattern:\n");
    S7T_State state = S7T_STATE_IDLE;
    printf("   Initial state: %u\n", state);
    state = s7t_state_transition(state, S7T_EVENT_START);
    printf("   After START event: %u\n", state);
    state = s7t_state_transition(state, S7T_EVENT_DATA);
    printf("   After DATA event: %u\n\n", state);

    // 6. Observer Pattern
    printf("6. Observer Pattern:\n");
    S7T_EventSystem event_sys = {0};
    // Would add observers here
    s7t_publish_event(&event_sys, 0x100, 42);
    printf("   Published event type 0x100 with data 42\n\n");

    // 7. Command Pattern
    printf("7. Command Pattern:\n");
    S7T_CommandProcessor proc = {0};
    proc.tape[0] = (S7T_Command){S7T_OP_LOAD, 0, 100};
    proc.tape[1] = (S7T_Command){S7T_OP_ADD, 0, 50};
    proc.tape[2] = (S7T_Command){S7T_OP_MUL, 0, 2};
    s7t_execute_commands(&proc, 3);
    printf("   Result in R0: %u\n\n", proc.registers[0]);

    // 9. Flyweight Pattern
    printf("9. Flyweight Pattern:\n");
    S7T_InternTable intern_table = {0};
    uint32_t id1 = s7t_intern_string(&intern_table, "hello");
    uint32_t id2 = s7t_intern_string(&intern_table, "world");
    uint32_t id3 = s7t_intern_string(&intern_table, "hello");
    printf("   'hello' -> ID %u\n", id1);
    printf("   'world' -> ID %u\n", id2);
    printf("   'hello' again -> ID %u (reused)\n\n", id3);

    // 10. Iterator Pattern
    printf("10. Iterator Pattern:\n");
    uint32_t data[] = {10, 20, 30, 40, 50};
    S7T_Iterator it;
    s7t_iterator_init(&it, data, sizeof(uint32_t), 5, 2);
    printf("   Iterating with stride 2: ");
    uint32_t *val;
    while ((val = s7t_iterator_next(&it)) != NULL)
    {
        printf("%u ", *val);
    }
    printf("\n\n");

    // 13. Decorator Pattern
    printf("13. Decorator Pattern:\n");
    S7T_Decorated obj = {.core_data = 42, .attributes = 0};
    s7t_add_decoration(&obj, S7T_ATTR_CACHED | S7T_ATTR_VALIDATED);
    printf("   Object decorated with: ");
    if (s7t_has_decoration(&obj, S7T_ATTR_CACHED))
        printf("CACHED ");
    if (s7t_has_decoration(&obj, S7T_ATTR_VALIDATED))
        printf("VALIDATED ");
    printf("\n\n");

    // 14. Prototype Pattern
    printf("14. Prototype Pattern:\n");
    S7T_Prototype instance;
    s7t_clone_from_prototype(&instance, 1);
    printf("   Cloned prototype type %u, config[0]=%llu\n\n",
           instance.type, (unsigned long long)instance.config[0]);

    printf("All patterns demonstrated with zero heap allocation!\n");
}

#endif // S7T_PATTERNS_EXAMPLE

#endif // S7T_PATTERNS_H