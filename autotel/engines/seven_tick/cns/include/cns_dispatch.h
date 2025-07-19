/*  ─────────────────────────────────────────────────────────────
    cns_dispatch.h  –  Command Dispatch System (v1.0)
    7-tick compliant command routing with telemetry hooks
    ───────────────────────────────────────────────────────────── */
#ifndef CNS_DISPATCH_H
#define CNS_DISPATCH_H

#include "cns.h"
#include "cns_parser.h"
#include <string.h>

/*═══════════════════════════════════════════════════════════════
  Dispatch Table Structure
  ═══════════════════════════════════════════════════════════════*/

// Command dispatch entry
typedef struct {
    const char* name;               // Command name
    uint32_t hash;                  // Pre-computed hash
    cns_handler_t handler;          // Handler function
    uint8_t min_args;              // Minimum arguments
    uint8_t max_args;              // Maximum arguments
    uint16_t flags;                // Command flags
    const char* help;              // Help text
    const char* usage;             // Usage example
} cns_dispatch_entry_t;

// Dispatch table
typedef struct S7T_ALIGNED(64) {
    const cns_dispatch_entry_t* entries;  // Command entries
    uint32_t count;                       // Number of entries
    uint32_t* hash_index;                // Hash to index mapping
    uint32_t hash_size;                  // Hash table size
} cns_dispatch_table_t;

/*═══════════════════════════════════════════════════════════════
  Fast Dispatch Functions (< 7 ticks)
  ═══════════════════════════════════════════════════════════════*/

// Initialize dispatch table
S7T_ALWAYS_INLINE void cns_dispatch_init(
    cns_dispatch_table_t* table,
    const cns_dispatch_entry_t* entries,
    uint32_t count,
    uint32_t* hash_index,
    uint32_t hash_size
) {
    table->entries = entries;
    table->count = count;
    table->hash_index = hash_index;
    table->hash_size = hash_size;
    
    // Clear hash index
    memset(hash_index, 0xFF, hash_size * sizeof(uint32_t));
    
    // Build hash index
    for (uint32_t i = 0; i < count; i++) {
        uint32_t hash = entries[i].hash;
        uint32_t idx = hash & (hash_size - 1);
        
        // Linear probe for empty slot
        while (hash_index[idx] != 0xFFFFFFFF) {
            idx = (idx + 1) & (hash_size - 1);
        }
        
        hash_index[idx] = i;
    }
}

// Lookup command by hash (< 3 ticks)
S7T_ALWAYS_INLINE const cns_dispatch_entry_t* cns_dispatch_lookup(
    const cns_dispatch_table_t* table,
    uint32_t hash
) {
    uint32_t idx = hash & (table->hash_size - 1);
    uint32_t probes = 0;
    
    // Linear probe with early exit
    while (probes < table->hash_size) {
        uint32_t entry_idx = table->hash_index[idx];
        
        if (entry_idx == 0xFFFFFFFF) {
            return NULL; // Empty slot
        }
        
        if (table->entries[entry_idx].hash == hash) {
            return &table->entries[entry_idx];
        }
        
        idx = (idx + 1) & (table->hash_size - 1);
        probes++;
    }
    
    return NULL;
}

// Direct dispatch from parsed command (< 7 ticks)
S7T_ALWAYS_INLINE cns_result_t cns_dispatch_execute(
    const cns_dispatch_table_t* table,
    const cns_command_t* cmd,
    void* context
) {
    // Lookup by pre-computed hash
    const cns_dispatch_entry_t* entry = cns_dispatch_lookup(table, cmd->hash);
    
    if (!entry) {
        return CNS_ERR_INVALID_CMD;
    }
    
    // Validate argument count
    if (cmd->argc < entry->min_args || cmd->argc > entry->max_args) {
        return CNS_ERR_INVALID_ARG;
    }
    
    // Check flags
    if ((entry->flags & CNS_FLAG_ADMIN) && !(cmd->flags & CNS_FLAG_ADMIN)) {
        return CNS_ERR_PERMISSION;
    }
    
    // Execute handler
    return entry->handler(cmd, context);
}

/*═══════════════════════════════════════════════════════════════
  Batch Dispatch Support
  ═══════════════════════════════════════════════════════════════*/

// Execute multiple commands in sequence
S7T_ALWAYS_INLINE uint32_t cns_dispatch_batch(
    const cns_dispatch_table_t* table,
    const cns_command_t* commands,
    uint32_t count,
    void* context,
    cns_result_t* results
) {
    uint32_t success_count = 0;
    
    for (uint32_t i = 0; i < count; i++) {
        results[i] = cns_dispatch_execute(table, &commands[i], context);
        if (results[i] == CNS_OK) {
            success_count++;
        }
    }
    
    return success_count;
}

/*═══════════════════════════════════════════════════════════════
  Dispatch Helpers
  ═══════════════════════════════════════════════════════════════*/

// Get command help text
S7T_ALWAYS_INLINE const char* cns_dispatch_help(
    const cns_dispatch_table_t* table,
    const char* command
) {
    uint32_t hash = s7t_hash_string(command, strlen(command));
    const cns_dispatch_entry_t* entry = cns_dispatch_lookup(table, hash);
    
    return entry ? entry->help : "Unknown command";
}

// List all commands
S7T_ALWAYS_INLINE void cns_dispatch_list(
    const cns_dispatch_table_t* table,
    void (*callback)(const char* name, const char* help, void* data),
    void* data
) {
    for (uint32_t i = 0; i < table->count; i++) {
        callback(table->entries[i].name, table->entries[i].help, data);
    }
}

// Check if command exists
S7T_ALWAYS_INLINE bool cns_dispatch_exists(
    const cns_dispatch_table_t* table,
    const char* command
) {
    uint32_t hash = s7t_hash_string(command, strlen(command));
    return cns_dispatch_lookup(table, hash) != NULL;
}

/*═══════════════════════════════════════════════════════════════
  Performance Monitoring
  ═══════════════════════════════════════════════════════════════*/

// Dispatch metrics
typedef struct {
    uint64_t total_dispatches;
    uint64_t successful_dispatches;
    uint64_t failed_dispatches;
    uint64_t total_cycles;
    uint64_t min_cycles;
    uint64_t max_cycles;
    uint32_t command_counts[256];  // Per-command counters
} cns_dispatch_metrics_t;

// Update dispatch metrics
S7T_ALWAYS_INLINE void cns_dispatch_update_metrics(
    cns_dispatch_metrics_t* metrics,
    uint32_t command_idx,
    cns_result_t result,
    uint64_t cycles
) {
    metrics->total_dispatches++;
    metrics->total_cycles += cycles;
    
    if (result == CNS_OK) {
        metrics->successful_dispatches++;
    } else {
        metrics->failed_dispatches++;
    }
    
    if (cycles < metrics->min_cycles) {
        metrics->min_cycles = cycles;
    }
    if (cycles > metrics->max_cycles) {
        metrics->max_cycles = cycles;
    }
    
    if (command_idx < 256) {
        metrics->command_counts[command_idx]++;
    }
}

/*═══════════════════════════════════════════════════════════════
  Dispatch Macros
  ═══════════════════════════════════════════════════════════════*/

// Define dispatch table
#define CNS_DISPATCH_TABLE(name, size) \
    static cns_dispatch_entry_t name##_entries[size]; \
    static uint32_t name##_hash_index[size * 2]; \
    static cns_dispatch_table_t name = { \
        .entries = name##_entries, \
        .count = 0, \
        .hash_index = name##_hash_index, \
        .hash_size = size * 2 \
    }

// Add command to dispatch table
#define CNS_DISPATCH_ADD(table, cmd_name, handler, min, max, flags, help) \
    do { \
        uint32_t idx = (table).count++; \
        (table).entries[idx] = (cns_dispatch_entry_t){ \
            .name = cmd_name, \
            .hash = s7t_hash_string(cmd_name, strlen(cmd_name)), \
            .handler = handler, \
            .min_args = min, \
            .max_args = max, \
            .flags = flags, \
            .help = help, \
            .usage = NULL \
        }; \
    } while(0)

#endif /* CNS_DISPATCH_H */