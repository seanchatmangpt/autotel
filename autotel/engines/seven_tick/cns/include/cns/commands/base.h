/*  ─────────────────────────────────────────────────────────────
    cns/commands/base.h  –  Base Command Definitions (v2.0)
    Core command structures and interfaces
    ───────────────────────────────────────────────────────────── */
#ifndef CNS_COMMANDS_BASE_H
#define CNS_COMMANDS_BASE_H

#include "../../../../include/s7t.h"
#include "../core/memory.h"
#include <stdint.h>
#include <stdbool.h>

/*═══════════════════════════════════════════════════════════════
  Forward Declarations
  ═══════════════════════════════════════════════════════════════*/

typedef struct cns_command_t cns_command_t;
typedef struct cns_context_t cns_context_t;
typedef struct cns_command_def_t cns_command_def_t;

/*═══════════════════════════════════════════════════════════════
  Result Codes
  ═══════════════════════════════════════════════════════════════*/

typedef enum {
    CNS_OK = 0,                 // Success
    CNS_ERR_INVALID_CMD = 1,    // Invalid command
    CNS_ERR_INVALID_ARG = 2,    // Invalid argument
    CNS_ERR_PERMISSION = 3,     // Permission denied
    CNS_ERR_TIMEOUT = 4,        // Operation timeout
    CNS_ERR_RESOURCE = 5,       // Resource exhausted
    CNS_ERR_INTERNAL = 6,       // Internal error
    CNS_ERR_NOT_FOUND = 7,      // Not found
    CNS_ERR_ALREADY_EXISTS = 8, // Already exists
    CNS_ERR_IO = 9,             // I/O error
    CNS_ERR_CYCLE_VIOLATION = 10, // 7-tick violation
    
    // Special codes
    CNS_EXIT = 100,             // Exit requested
    CNS_HELP = 101,             // Help requested
} cns_result_t;

/*═══════════════════════════════════════════════════════════════
  Command Types and Flags
  ═══════════════════════════════════════════════════════════════*/

// Command types
typedef enum {
    CNS_CMD_EXEC = 0,           // Execute command
    CNS_CMD_QUERY,              // Query status
    CNS_CMD_CONFIG,             // Configuration
    CNS_CMD_HELP,               // Help/info
    CNS_CMD_ADMIN,              // Administrative
    CNS_CMD_DOMAIN,             // Domain-specific
} cns_cmd_type_t;

// Command flags
typedef enum {
    CNS_FLAG_NONE       = 0,
    CNS_FLAG_ASYNC      = 1 << 0,  // Asynchronous execution
    CNS_FLAG_CRITICAL   = 1 << 1,  // Critical path (< 7 ticks)
    CNS_FLAG_ADMIN      = 1 << 2,  // Admin only
    CNS_FLAG_TRACED     = 1 << 3,  // Enable tracing
    CNS_FLAG_TIMED      = 1 << 4,  // Time execution
    CNS_FLAG_BATCH      = 1 << 5,  // Batch operation
    CNS_FLAG_INTERACTIVE = 1 << 6, // Interactive mode
} cns_flags_t;

/*═══════════════════════════════════════════════════════════════
  Command Structure (Parsed)
  ═══════════════════════════════════════════════════════════════*/

struct cns_command_t {
    // Command identification
    char name[64];              // Command name
    uint32_t hash;              // Pre-computed hash
    
    // Arguments
    char** args;                // Argument array
    uint8_t argc;               // Argument count
    
    // Metadata
    uint8_t type;               // Command type
    uint16_t flags;             // Command flags
    uint64_t timestamp;         // Execution timestamp
    
    // Source information
    const char* source;         // Source string
    uint32_t source_len;        // Source length
};

/*═══════════════════════════════════════════════════════════════
  Command Context
  ═══════════════════════════════════════════════════════════════*/

struct cns_context_t {
    // Memory management
    cns_memory_arena_t* arena;  // Command arena
    
    // User data
    void* user_data;            // User context
    
    // Execution state
    uint64_t start_cycles;      // Start timestamp
    uint32_t timeout_cycles;    // Timeout in cycles
    
    // Options
    bool verbose;               // Verbose output
    bool json_output;           // JSON output
    bool no_color;              // Disable colors
    
    // Telemetry
    void* span;                 // Current span
    void* telemetry;            // Telemetry system
};

/*═══════════════════════════════════════════════════════════════
  Command Handler
  ═══════════════════════════════════════════════════════════════*/

// Command handler function type
typedef cns_result_t (*cns_handler_fn)(
    const cns_command_t* cmd,
    cns_context_t* ctx
);

// Async command handler
typedef void (*cns_async_handler_fn)(
    const cns_command_t* cmd,
    cns_context_t* ctx,
    void (*callback)(cns_result_t result, void* data),
    void* callback_data
);

/*═══════════════════════════════════════════════════════════════
  Command Definition
  ═══════════════════════════════════════════════════════════════*/

struct cns_command_def_t {
    // Identification
    const char* name;           // Command name
    uint32_t hash;              // Pre-computed hash
    
    // Handler
    cns_handler_fn handler;     // Sync handler
    cns_async_handler_fn async_handler; // Async handler
    
    // Constraints
    uint8_t min_args;           // Minimum arguments
    uint8_t max_args;           // Maximum arguments
    uint16_t flags;             // Command flags
    
    // Documentation
    const char* description;    // Short description
    const char* usage;          // Usage string
    const char* help;           // Detailed help
    
    // Performance
    uint64_t max_cycles;        // Maximum allowed cycles
    bool critical_path;         // Is on critical path?
};

/*═══════════════════════════════════════════════════════════════
  Command Builder Helpers
  ═══════════════════════════════════════════════════════════════*/

// Define a command handler
#define CNS_HANDLER(name) \
    static cns_result_t cns_handler_##name( \
        const cns_command_t* cmd, \
        cns_context_t* ctx)

// Define an async handler
#define CNS_ASYNC_HANDLER(name) \
    static void cns_async_handler_##name( \
        const cns_command_t* cmd, \
        cns_context_t* ctx, \
        void (*callback)(cns_result_t, void*), \
        void* callback_data)

// Define command with pre-computed hash
#define CNS_COMMAND_DEF(cmd_name, handler, min, max, desc) { \
    .name = cmd_name, \
    .hash = s7t_const_hash(cmd_name), \
    .handler = handler, \
    .async_handler = NULL, \
    .min_args = min, \
    .max_args = max, \
    .flags = CNS_FLAG_NONE, \
    .description = desc, \
    .usage = NULL, \
    .help = NULL, \
    .max_cycles = 0, \
    .critical_path = false \
}

// Define critical path command
#define CNS_CRITICAL_COMMAND_DEF(cmd_name, handler, min, max, desc) { \
    .name = cmd_name, \
    .hash = s7t_const_hash(cmd_name), \
    .handler = handler, \
    .async_handler = NULL, \
    .min_args = min, \
    .max_args = max, \
    .flags = CNS_FLAG_CRITICAL, \
    .description = desc, \
    .usage = NULL, \
    .help = NULL, \
    .max_cycles = 7 * S7T_CYCLES_PER_TICK, \
    .critical_path = true \
}

/*═══════════════════════════════════════════════════════════════
  Result Code Helpers
  ═══════════════════════════════════════════════════════════════*/

// Get result string
S7T_ALWAYS_INLINE const char* cns_result_str(cns_result_t result) {
    static const char* const strings[] = {
        [CNS_OK] = "Success",
        [CNS_ERR_INVALID_CMD] = "Invalid command",
        [CNS_ERR_INVALID_ARG] = "Invalid argument",
        [CNS_ERR_PERMISSION] = "Permission denied",
        [CNS_ERR_TIMEOUT] = "Operation timeout",
        [CNS_ERR_RESOURCE] = "Resource exhausted",
        [CNS_ERR_INTERNAL] = "Internal error",
        [CNS_ERR_NOT_FOUND] = "Not found",
        [CNS_ERR_ALREADY_EXISTS] = "Already exists",
        [CNS_ERR_IO] = "I/O error",
        [CNS_ERR_CYCLE_VIOLATION] = "7-tick violation",
        [CNS_EXIT] = "Exit requested",
        [CNS_HELP] = "Help requested"
    };
    
    if (result < sizeof(strings) / sizeof(strings[0]) && strings[result]) {
        return strings[result];
    }
    return "Unknown error";
}

// Check if result is error
S7T_ALWAYS_INLINE bool cns_is_error(cns_result_t result) {
    return result != CNS_OK && result < 100;
}

#endif /* CNS_COMMANDS_BASE_H */