/*  ─────────────────────────────────────────────────────────────
    cns/core/registry.h  –  Command Registry (v2.0)
    Fast hash-based command lookup with perfect hashing
    ───────────────────────────────────────────────────────────── */
#ifndef CNS_CORE_REGISTRY_H
#define CNS_CORE_REGISTRY_H

#include "../../../../include/s7t.h"
#include "../commands/base.h"

/*═══════════════════════════════════════════════════════════════
  Registry Configuration
  ═══════════════════════════════════════════════════════════════*/

typedef struct {
    uint32_t max_commands;       // Maximum commands
    uint32_t hash_bits;          // Hash table bits (2^n size)
    bool allow_overwrite;        // Allow command overwriting
    bool use_perfect_hash;       // Use perfect hashing
} cns_registry_config_t;

/*═══════════════════════════════════════════════════════════════
  Command Entry
  ═══════════════════════════════════════════════════════════════*/

typedef struct {
    uint32_t hash;               // Pre-computed hash
    cns_command_def_t* command;  // Command definition
    void* domain_data;           // Domain-specific data
    uint32_t call_count;         // Call statistics
    uint64_t total_cycles;       // Total execution cycles
} cns_registry_entry_t;

/*═══════════════════════════════════════════════════════════════
  Registry Structure (Cache-aligned)
  ═══════════════════════════════════════════════════════════════*/

typedef struct S7T_ALIGNED(64) {
    // Hash table
    uint32_t* hash_table;        // Hash table (indices)
    uint32_t table_mask;         // Table size - 1
    
    // Command storage
    cns_registry_entry_t* entries; // Command entries
    uint32_t entry_count;          // Current entries
    uint32_t max_entries;          // Maximum entries
    
    // Configuration
    cns_registry_config_t config;  // Registry config
    
    // Statistics
    uint64_t lookup_count;         // Total lookups
    uint64_t collision_count;      // Hash collisions
} cns_registry_t;

/*═══════════════════════════════════════════════════════════════
  Registry Management
  ═══════════════════════════════════════════════════════════════*/

// Initialize registry
cns_result_t cns_registry_init(
    cns_registry_t* registry,
    const cns_registry_config_t* config
);

// Cleanup registry
void cns_registry_cleanup(cns_registry_t* registry);

// Register command
cns_result_t cns_registry_register(
    cns_registry_t* registry,
    const cns_command_def_t* command,
    void* domain_data
);

// Register multiple commands
cns_result_t cns_registry_register_batch(
    cns_registry_t* registry,
    const cns_command_def_t* commands,
    size_t count,
    void* domain_data
);

// Unregister command
cns_result_t cns_registry_unregister(
    cns_registry_t* registry,
    const char* name
);

/*═══════════════════════════════════════════════════════════════
  Command Lookup (< 7 ticks)
  ═══════════════════════════════════════════════════════════════*/

// Lookup by hash (fastest)
S7T_ALWAYS_INLINE cns_registry_entry_t* cns_registry_lookup_hash(
    cns_registry_t* registry,
    uint32_t hash
) {
    uint32_t idx = hash & registry->table_mask;
    
    // Linear probe
    while (registry->hash_table[idx] != UINT32_MAX) {
        uint32_t entry_idx = registry->hash_table[idx];
        if (registry->entries[entry_idx].hash == hash) {
            registry->lookup_count++;
            return &registry->entries[entry_idx];
        }
        idx = (idx + 1) & registry->table_mask;
        registry->collision_count++;
    }
    
    return NULL;
}

// Lookup by name
S7T_ALWAYS_INLINE cns_registry_entry_t* cns_registry_lookup(
    cns_registry_t* registry,
    const char* name
) {
    uint32_t hash = s7t_hash_string(name, strlen(name));
    return cns_registry_lookup_hash(registry, hash);
}

/*═══════════════════════════════════════════════════════════════
  Registry Iteration
  ═══════════════════════════════════════════════════════════════*/

// Iterator callback
typedef bool (*cns_registry_iter_fn)(
    const cns_registry_entry_t* entry,
    void* user_data
);

// Iterate all commands
void cns_registry_iterate(
    const cns_registry_t* registry,
    cns_registry_iter_fn callback,
    void* user_data
);

// Find commands by prefix
size_t cns_registry_find_prefix(
    const cns_registry_t* registry,
    const char* prefix,
    cns_registry_entry_t** results,
    size_t max_results
);

/*═══════════════════════════════════════════════════════════════
  Registry Statistics
  ═══════════════════════════════════════════════════════════════*/

typedef struct {
    uint32_t total_commands;     // Total registered commands
    uint32_t hash_table_size;    // Hash table size
    double load_factor;          // Hash table load factor
    uint64_t total_lookups;      // Total lookup operations
    uint64_t total_collisions;   // Total hash collisions
    double collision_rate;       // Collision rate
} cns_registry_stats_t;

// Get registry statistics
void cns_registry_get_stats(
    const cns_registry_t* registry,
    cns_registry_stats_t* stats
);

/*═══════════════════════════════════════════════════════════════
  Compile-time Hash Computation
  ═══════════════════════════════════════════════════════════════*/

// Compile-time string hash (for static commands)
#define CNS_HASH(str) s7t_const_hash(str)

// Pre-compute hash for registration
#define CNS_COMMAND_HASH(name) { \
    .name = name, \
    .hash = CNS_HASH(name) \
}

/*═══════════════════════════════════════════════════════════════
  Default Configuration
  ═══════════════════════════════════════════════════════════════*/

#define CNS_DEFAULT_REGISTRY_CONFIG { \
    .max_commands = 256, \
    .hash_bits = 8, \
    .allow_overwrite = false, \
    .use_perfect_hash = true \
}

#endif /* CNS_CORE_REGISTRY_H */