/*  ─────────────────────────────────────────────────────────────
    cns/core/engine.h  –  Core CNS Engine (v2.0)
    Enhanced modular architecture with telemetry support
    ───────────────────────────────────────────────────────────── */
#ifndef CNS_CORE_ENGINE_H
#define CNS_CORE_ENGINE_H

#include "../../../../include/s7t.h"
#include "registry.h"
#include "memory.h"
#include "perf.h"
#include "../telemetry/otel.h"

/*═══════════════════════════════════════════════════════════════
  Engine Configuration
  ═══════════════════════════════════════════════════════════════*/

typedef struct {
    uint32_t max_commands;      // Maximum commands to register
    uint32_t hash_table_bits;   // Hash table size (2^n)
    uint32_t arena_size;        // Memory arena size
    uint32_t flags;             // Engine flags
    cns_telemetry_config_t telemetry; // Telemetry configuration
} cns_engine_config_t;

/*═══════════════════════════════════════════════════════════════
  Engine State (Cache-aligned)
  ═══════════════════════════════════════════════════════════════*/

typedef struct S7T_ALIGNED(64) {
    // Core components
    cns_registry_t* registry;        // Command registry
    cns_memory_arena_t* arena;       // Memory arena
    cns_perf_tracker_t* perf;        // Performance tracker
    cns_telemetry_t* telemetry;      // Telemetry system
    
    // Configuration
    cns_engine_config_t config;      // Engine configuration
    
    // Runtime state
    uint64_t command_count;          // Total commands executed
    uint64_t error_count;            // Total errors
    uint32_t flags;                  // Runtime flags
    uint32_t reserved;
} cns_engine_t;

/*═══════════════════════════════════════════════════════════════
  Engine Lifecycle
  ═══════════════════════════════════════════════════════════════*/

// Initialize engine with configuration
cns_result_t cns_engine_init(
    cns_engine_t* engine,
    const cns_engine_config_t* config
);

// Shutdown engine and cleanup resources
void cns_engine_shutdown(cns_engine_t* engine);

// Reset engine state (keep configuration)
void cns_engine_reset(cns_engine_t* engine);

/*═══════════════════════════════════════════════════════════════
  Command Execution
  ═══════════════════════════════════════════════════════════════*/

// Execute command with full telemetry
cns_result_t cns_engine_execute(
    cns_engine_t* engine,
    const cns_command_t* cmd,
    cns_context_t* ctx
);

// Execute command string (parse + execute)
cns_result_t cns_engine_execute_string(
    cns_engine_t* engine,
    const char* cmd_string,
    cns_context_t* ctx
);

// Execute batch of commands
cns_result_t cns_engine_execute_batch(
    cns_engine_t* engine,
    const char** commands,
    size_t count,
    cns_context_t* ctx
);

/*═══════════════════════════════════════════════════════════════
  Engine Statistics
  ═══════════════════════════════════════════════════════════════*/

typedef struct {
    uint64_t total_commands;     // Total commands executed
    uint64_t total_errors;       // Total errors
    uint64_t total_cycles;       // Total CPU cycles
    uint64_t avg_cycles;         // Average cycles per command
    uint64_t min_cycles;         // Minimum cycles
    uint64_t max_cycles;         // Maximum cycles
    double success_rate;         // Success rate percentage
    double throughput;           // Commands per second
} cns_engine_stats_t;

// Get engine statistics
void cns_engine_get_stats(
    const cns_engine_t* engine,
    cns_engine_stats_t* stats
);

// Export statistics to telemetry
void cns_engine_export_stats(cns_engine_t* engine);

/*═══════════════════════════════════════════════════════════════
  Engine Flags and Configuration
  ═══════════════════════════════════════════════════════════════*/

// Engine flags
typedef enum {
    CNS_ENGINE_FLAG_NONE        = 0,
    CNS_ENGINE_FLAG_TRACE       = 1 << 0,  // Enable tracing
    CNS_ENGINE_FLAG_METRICS     = 1 << 1,  // Enable metrics
    CNS_ENGINE_FLAG_STRICT      = 1 << 2,  // Strict 7-tick enforcement
    CNS_ENGINE_FLAG_PROFILE     = 1 << 3,  // Enable profiling
    CNS_ENGINE_FLAG_VALIDATE    = 1 << 4,  // Validate all inputs
    CNS_ENGINE_FLAG_BENCHMARK   = 1 << 5,  // Benchmark mode
} cns_engine_flags_t;

// Set engine flags
void cns_engine_set_flags(cns_engine_t* engine, uint32_t flags);

// Get engine flags
uint32_t cns_engine_get_flags(const cns_engine_t* engine);

/*═══════════════════════════════════════════════════════════════
  Default Configuration
  ═══════════════════════════════════════════════════════════════*/

#define CNS_DEFAULT_ENGINE_CONFIG { \
    .max_commands = 256, \
    .hash_table_bits = 8, \
    .arena_size = 65536, \
    .flags = CNS_ENGINE_FLAG_TRACE | CNS_ENGINE_FLAG_METRICS, \
    .telemetry = CNS_DEFAULT_TELEMETRY_CONFIG \
}

/*═══════════════════════════════════════════════════════════════
  Inline Implementation
  ═══════════════════════════════════════════════════════════════*/

// Quick flag check
S7T_ALWAYS_INLINE bool cns_engine_has_flag(
    const cns_engine_t* engine,
    cns_engine_flags_t flag
) {
    return (engine->flags & flag) != 0;
}

// Get current cycle count
S7T_ALWAYS_INLINE uint64_t cns_engine_cycles(void) {
    return s7t_cycles();
}

#endif /* CNS_CORE_ENGINE_H */