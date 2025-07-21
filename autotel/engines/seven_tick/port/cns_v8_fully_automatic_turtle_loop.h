/**
 * CNS v8 Fully Automatic Turtle Loop
 * Complete integration of all components for production-ready automation
 * Combines: Trinity Architecture + DSPy-OWL-SHACL + ML Optimization + Continuous Processing
 */

#ifndef CNS_V8_FULLY_AUTOMATIC_TURTLE_LOOP_H
#define CNS_V8_FULLY_AUTOMATIC_TURTLE_LOOP_H

#include "cns_v8_turtle_loop_integration.h"
#include "continuous_turtle_pipeline.h"
#include "cns_v8_turtle_loop_ml_optimizer.h"

// Error recovery and resilience
typedef enum {
    ERROR_CATEGORY_PARSE = 0x01,
    ERROR_CATEGORY_VALIDATE = 0x02,
    ERROR_CATEGORY_MEMORY = 0x04,
    ERROR_CATEGORY_TIMEOUT = 0x08,
    ERROR_CATEGORY_NETWORK = 0x10
} error_category_t;

typedef enum {
    RECOVERY_RETRY = 0x01,
    RECOVERY_SKIP = 0x02,
    RECOVERY_FALLBACK = 0x04,
    RECOVERY_ESCALATE = 0x08,
    RECOVERY_SELF_HEAL = 0x10
} recovery_strategy_t;

typedef struct {
    error_category_t category;
    recovery_strategy_t strategy;
    uint8_t retry_count;
    uint8_t max_retries;
    cns_cycle_t backoff_cycles;
    void* fallback_context;
    void (*self_heal)(void* context);
} error_recovery_t;

// Complete automatic turtle loop
typedef struct {
    // Core components
    cns_v8_turtle_loop_t core_loop;
    continuous_pipeline_t* continuous_pipeline;
    cns_v8_ml_optimizer_t* ml_optimizer;
    
    // Error recovery
    error_recovery_t error_recovery;
    void (*on_error)(error_recovery_t* recovery, const char* error_msg);
    void (*on_partial_success)(void* partial_result, size_t completed);
    void (*on_self_heal)(const char* component, const char* action);
    
    // Observability
    struct {
        void (*record_trace)(const char* operation, cns_cycle_t cycles);
        void (*record_pattern)(triple_pattern_t pattern, double frequency);
        void (*record_error)(error_category_t category, const char* details);
        void (*record_recovery)(recovery_strategy_t strategy, bool success);
        void (*export_metrics)(void* buffer, size_t* size);
    } telemetry;
    
    // Automation state
    struct {
        bool is_running;
        bool auto_scaling_enabled;
        bool ml_optimization_enabled;
        bool self_healing_enabled;
        uint64_t uptime_seconds;
        uint64_t auto_recoveries;
        uint64_t pattern_adaptations;
    } automation;
    
    // Distributed processing (future)
    struct {
        void* cluster_context;
        uint32_t node_id;
        uint32_t total_nodes;
        void (*sync_state)(void* state, size_t size);
    } distributed;
    
} cns_v8_automatic_turtle_loop_t;

// Initialize fully automatic turtle loop
int cns_v8_automatic_turtle_loop_init(
    cns_v8_automatic_turtle_loop_t* loop,
    const cns_v8_automatic_config_t* config
);

// Configuration for automatic operation
typedef struct {
    // Core settings
    size_t arena_size;
    uint32_t max_workers;
    
    // Automation features
    bool enable_continuous_processing;
    bool enable_ml_optimization;
    bool enable_auto_scaling;
    bool enable_self_healing;
    bool enable_distributed;
    
    // Performance targets
    double target_pareto_efficiency;  // Default: 0.85
    cns_cycle_t max_cycles_per_triple;  // Default: 8
    uint64_t target_throughput;  // triples/sec
    
    // Resilience settings
    uint8_t max_error_retries;
    cns_cycle_t error_backoff_base;
    double error_threshold_percent;  // Trigger self-heal
    
    // ML settings
    ml_optimization_strategy_t ml_strategy;
    double ml_learning_rate;
    uint32_t ml_prediction_window;
    
} cns_v8_automatic_config_t;

// Start automatic processing (runs continuously)
int cns_v8_automatic_start(
    cns_v8_automatic_turtle_loop_t* loop,
    const char* input_source,  // File, stream, or network
    const char* output_sink    // File, stream, or network
);

// Stop automatic processing gracefully
int cns_v8_automatic_stop(
    cns_v8_automatic_turtle_loop_t* loop,
    bool save_checkpoint
);

// Runtime operations
int cns_v8_automatic_reload_patterns(
    cns_v8_automatic_turtle_loop_t* loop,
    const char* new_patterns_file
);

int cns_v8_automatic_adjust_workers(
    cns_v8_automatic_turtle_loop_t* loop,
    int32_t delta  // Positive to add, negative to remove
);

// Monitoring and diagnostics
typedef struct {
    // Performance metrics
    uint64_t triples_processed;
    uint64_t patterns_matched[5];
    double current_throughput;
    double average_latency_ns;
    double pareto_efficiency;
    
    // ML metrics
    double pattern_prediction_accuracy;
    double optimization_gain_percent;
    uint64_t successful_predictions;
    
    // Health metrics
    uint64_t errors_recovered;
    uint64_t self_heals_performed;
    double uptime_percent;
    uint32_t active_workers;
    
    // Resource usage
    size_t memory_used_bytes;
    double cpu_utilization_percent;
    uint64_t cache_hit_rate;
    
} cns_v8_automatic_metrics_t;

void cns_v8_automatic_get_metrics(
    const cns_v8_automatic_turtle_loop_t* loop,
    cns_v8_automatic_metrics_t* metrics
);

// Export operational data
int cns_v8_automatic_export_state(
    const cns_v8_automatic_turtle_loop_t* loop,
    const char* checkpoint_file
);

int cns_v8_automatic_import_state(
    cns_v8_automatic_turtle_loop_t* loop,
    const char* checkpoint_file
);

// Self-healing operations
void cns_v8_automatic_trigger_self_heal(
    cns_v8_automatic_turtle_loop_t* loop,
    const char* component
);

// Cleanup
void cns_v8_automatic_cleanup(
    cns_v8_automatic_turtle_loop_t* loop
);

// Helper to create default configuration
cns_v8_automatic_config_t cns_v8_automatic_default_config(void);

#endif // CNS_V8_FULLY_AUTOMATIC_TURTLE_LOOP_H