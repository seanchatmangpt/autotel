#ifndef QOP_H
#define QOP_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Configuration for Query Optimization Planner
typedef struct {
    uint32_t max_iterations;
    uint64_t time_budget_ns;
    uint32_t rollout_depth;
    double exploration_constant;  // UCB1 constant (default: sqrt(2))
    uint64_t rng_seed;
} QOP_Config;

// Output plan structure
typedef struct {
    uint32_t* pattern_order;  // Caller-provided buffer
    double score;             // Higher = better
    size_t length;           // Number of patterns in the plan
} QOP_Plan;

// Opaque planner handle
typedef struct QOP_Handle QOP_Handle;

// Core API
QOP_Handle* qop_create(const QOP_Config* config);
bool qop_step(QOP_Handle* handle);  // Returns true if should continue
void qop_get_plan(QOP_Handle* handle, QOP_Plan* plan_out);
void qop_destroy(QOP_Handle* handle);

// Telemetry hook
typedef void (*qop_telemetry_fn)(const char* metric, double value);
void qop_set_telemetry(QOP_Handle* handle, qop_telemetry_fn fn);

// Statistics
typedef struct {
    uint64_t iterations;
    uint64_t nodes_created;
    uint64_t simulations;
    uint64_t expansions;
    double best_score;
    uint64_t elapsed_ns;
    double avg_simulation_score;
} QOP_Stats;

// Extended API
void qop_add_patterns(QOP_Handle* handle, const uint32_t* patterns, size_t count);
void qop_run(QOP_Handle* handle);
void qop_get_stats(QOP_Handle* handle, QOP_Stats* stats);

// Pattern evaluation function
typedef double (*pattern_eval_fn)(uint32_t pattern_id, uint32_t position, void* context);
void qop_set_eval_function(QOP_Handle* handle, pattern_eval_fn fn, void* context);

#endif
