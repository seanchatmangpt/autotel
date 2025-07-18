#ifndef PICO_JIT_H
#define PICO_JIT_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Query shape representation
typedef struct {
    uint32_t pattern_ids[16];  // Max 16 patterns per shape
    uint8_t pattern_count;
    uint64_t hash;            // Canonical hash of shape
} QueryShape;

// JIT'd kernel function pointer
typedef bool (*jit_kernel_fn)(uint64_t* pred_vecs, uint64_t* obj_vecs, 
                             uint32_t* params, uint64_t* result);

// Compiled kernel entry
typedef struct {
    QueryShape shape;
    jit_kernel_fn kernel;
    void* handle;             // dlopen handle
    uint64_t compile_cycles;
    uint64_t hit_count;
    uint64_t total_cycles_saved;
} JitKernel;

// JIT daemon configuration
typedef struct {
    uint32_t hot_threshold;      // Executions before JIT
    size_t max_cache_size;       // Max JIT'd kernels
    size_t max_code_bytes;       // Max total code size
    uint32_t compile_threads;    // Worker thread count
    char compiler_flags[256];    // Custom flags
} JitConfig;

// Telemetry callback
typedef void (*jit_telemetry_fn)(const char* event, double value);

// JIT daemon handle
typedef struct JitDaemon JitDaemon;

// Core API
JitDaemon* jit_create(const JitConfig* config);
void jit_destroy(JitDaemon* daemon);

// Runtime interface
void jit_record_execution(JitDaemon* daemon, const QueryShape* shape, uint64_t cycles);
jit_kernel_fn jit_lookup_kernel(JitDaemon* daemon, const QueryShape* shape);

// Management
void jit_set_telemetry(JitDaemon* daemon, jit_telemetry_fn fn);
void jit_get_stats(JitDaemon* daemon, char* buf, size_t buf_size);

#endif
