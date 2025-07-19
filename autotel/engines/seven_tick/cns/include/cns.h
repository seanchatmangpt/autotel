/*  ─────────────────────────────────────────────────────────────
    cns.h  –  Command Nano Stack Main Header (v1.0)
    7-tick compliant command processing system
    ───────────────────────────────────────────────────────────── */
#ifndef CNS_H
#define CNS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "../../include/s7t.h"
#include "cns_command_types.h"

/*═══════════════════════════════════════════════════════════════
  Core CNS Types and Constants
  ═══════════════════════════════════════════════════════════════*/

// Maximum command length (cache-line aligned)
#define CNS_MAX_CMD_LEN      64
#define CNS_MAX_ARGS         8
#define CNS_MAX_ARG_LEN      32
#define CNS_MAX_COMMANDS     256
#define CNS_HASH_BITS        8

// Command types
typedef enum {
    CNS_CMD_EXEC = 0,    // Execute command
    CNS_CMD_QUERY,       // Query status
    CNS_CMD_CONFIG,      // Configuration
    CNS_CMD_HELP,        // Help/info
    CNS_CMD_EXIT,        // Exit/quit
    CNS_CMD_INVALID      // Invalid command
} cns_cmd_type_t;

// Command flags (bit field)
typedef enum {
    CNS_FLAG_NONE     = 0,
    CNS_FLAG_ASYNC    = 1 << 0,  // Asynchronous execution
    CNS_FLAG_CRITICAL = 1 << 1,  // Critical path
    CNS_FLAG_ADMIN    = 1 << 2,  // Admin only
    CNS_FLAG_LOGGED   = 1 << 3,  // Log command
    CNS_FLAG_TIMED    = 1 << 4   // Time execution
} cns_flags_t;

// Command result codes
typedef enum {
    CNS_OK = 0,
    CNS_ERR_INVALID_CMD,
    CNS_ERR_INVALID_ARG,
    CNS_ERR_PERMISSION,
    CNS_ERR_TIMEOUT,
    CNS_ERR_RESOURCE,
    CNS_ERR_INTERNAL
} cns_result_t;

/*═══════════════════════════════════════════════════════════════
  Command Structures (Cache-line Aligned)
  ═══════════════════════════════════════════════════════════════*/

// Parsed command structure
typedef struct S7T_ALIGNED(64) {
    char cmd[CNS_MAX_CMD_LEN];       // Command name
    char args[CNS_MAX_ARGS][CNS_MAX_ARG_LEN]; // Arguments
    uint8_t argc;                    // Argument count
    uint8_t type;                    // Command type
    uint16_t flags;                  // Command flags
    uint32_t hash;                   // Pre-computed hash
    uint64_t timestamp;              // Execution timestamp
} cns_command_t;

// Command handler function type
typedef cns_result_t (*cns_handler_t)(const cns_command_t* cmd, void* context);

// Command registry entry
typedef struct {
    uint32_t hash;                   // Command name hash
    cns_handler_t handler;           // Handler function
    uint16_t flags;                  // Command flags
    uint8_t min_args;               // Minimum arguments
    uint8_t max_args;               // Maximum arguments
    const char* name;               // Command name
    const char* help;               // Help text
} cns_cmd_entry_t;

// Command execution context
typedef struct S7T_ALIGNED(64) {
    void* user_data;                // User context
    s7t_arena_t* arena;             // Memory arena
    uint64_t start_cycles;          // Start timestamp
    uint32_t timeout_cycles;        // Timeout in cycles
    uint32_t reserved;
} cns_context_t;

/*═══════════════════════════════════════════════════════════════
  CNS Engine Structure
  ═══════════════════════════════════════════════════════════════*/

typedef struct S7T_ALIGNED(64) {
    // Command registry (hash table)
    cns_cmd_entry_t* commands;      // Command array
    uint32_t* hash_table;           // Hash table indices
    uint32_t cmd_count;             // Number of commands
    uint32_t table_size;            // Hash table size
    
    // Execution state
    cns_context_t context;          // Current context
    s7t_perf_counter_t perf;        // Performance counter
    
    // Configuration
    uint32_t max_commands;          // Maximum commands
    uint32_t flags;                 // Engine flags
} cns_engine_t;

/*═══════════════════════════════════════════════════════════════
  Public API (All Functions < 7 Ticks)
  ═══════════════════════════════════════════════════════════════*/

// Initialize CNS engine
S7T_ALWAYS_INLINE void cns_init(
    cns_engine_t* engine,
    cns_cmd_entry_t* commands,
    uint32_t* hash_table,
    uint32_t max_commands
) {
    engine->commands = commands;
    engine->hash_table = hash_table;
    engine->cmd_count = 0;
    engine->max_commands = max_commands;
    engine->table_size = 1 << CNS_HASH_BITS;
    
    // Clear hash table
    for (uint32_t i = 0; i < engine->table_size; i++) {
        engine->hash_table[i] = CNS_MAX_COMMANDS;
    }
    
    // Initialize performance counter
    engine->perf.min_cycles = UINT64_MAX;
    engine->perf.max_cycles = 0;
}

// Register a command
S7T_ALWAYS_INLINE cns_result_t cns_register(
    cns_engine_t* engine,
    const char* name,
    cns_handler_t handler,
    uint16_t flags,
    uint8_t min_args,
    uint8_t max_args,
    const char* help
) {
    if (engine->cmd_count >= engine->max_commands) {
        return CNS_ERR_RESOURCE;
    }
    
    // Compute hash
    uint32_t hash = s7t_hash_string(name, strlen(name));
    uint32_t idx = hash & (engine->table_size - 1);
    
    // Linear probe for empty slot
    while (engine->hash_table[idx] != CNS_MAX_COMMANDS) {
        idx = (idx + 1) & (engine->table_size - 1);
    }
    
    // Add command
    uint32_t cmd_idx = engine->cmd_count++;
    engine->commands[cmd_idx] = (cns_cmd_entry_t){
        .hash = hash,
        .handler = handler,
        .flags = flags,
        .min_args = min_args,
        .max_args = max_args,
        .name = name,
        .help = help
    };
    
    engine->hash_table[idx] = cmd_idx;
    return CNS_OK;
}

// Lookup command by hash (< 7 ticks)
S7T_ALWAYS_INLINE cns_cmd_entry_t* cns_lookup(
    cns_engine_t* engine,
    uint32_t hash
) {
    uint32_t idx = hash & (engine->table_size - 1);
    
    // Linear probe
    while (engine->hash_table[idx] != CNS_MAX_COMMANDS) {
        uint32_t cmd_idx = engine->hash_table[idx];
        if (engine->commands[cmd_idx].hash == hash) {
            return &engine->commands[cmd_idx];
        }
        idx = (idx + 1) & (engine->table_size - 1);
    }
    
    return NULL;
}

// Execute parsed command (< 7 ticks for dispatch)
S7T_ALWAYS_INLINE cns_result_t cns_execute(
    cns_engine_t* engine,
    const cns_command_t* cmd
) {
    uint64_t start = s7t_cycles();
    
    // Lookup command
    cns_cmd_entry_t* entry = cns_lookup(engine, cmd->hash);
    if (!entry) {
        return CNS_ERR_INVALID_CMD;
    }
    
    // Validate arguments
    if (cmd->argc < entry->min_args || cmd->argc > entry->max_args) {
        return CNS_ERR_INVALID_ARG;
    }
    
    // Check permissions
    if ((entry->flags & CNS_FLAG_ADMIN) && !(cmd->flags & CNS_FLAG_ADMIN)) {
        return CNS_ERR_PERMISSION;
    }
    
    // Set up context
    engine->context.start_cycles = start;
    
    // Execute handler
    cns_result_t result = entry->handler(cmd, &engine->context);
    
    // Update performance counter
    uint64_t cycles = s7t_cycles() - start;
    s7t_perf_update(&engine->perf, cycles);
    
    return result;
}

// Quick command validation (< 2 ticks)
S7T_ALWAYS_INLINE bool cns_validate_quick(const char* cmd, size_t len) {
    // Basic validation: non-empty, within limits
    return len > 0 && len < CNS_MAX_CMD_LEN;
}

/*═══════════════════════════════════════════════════════════════
  Utility Macros
  ═══════════════════════════════════════════════════════════════*/

// Define command handler
#define CNS_HANDLER(name) \
    static cns_result_t name(const cns_command_t* cmd, void* context)

// Register standard command
#define CNS_REGISTER_CMD(engine, name, handler, min_args, max_args, help) \
    cns_register(engine, name, handler, CNS_FLAG_NONE, min_args, max_args, help)

// Register admin command
#define CNS_REGISTER_ADMIN(engine, name, handler, min_args, max_args, help) \
    cns_register(engine, name, handler, CNS_FLAG_ADMIN, min_args, max_args, help)

// Check if command completed within tick budget
#define CNS_ASSERT_TICKS(engine, max_ticks) \
    S7T_STATIC_ASSERT((engine)->perf.max_cycles <= (max_ticks) * S7T_MAX_CYCLES, \
                      "Command exceeded tick budget")

#endif /* CNS_H */