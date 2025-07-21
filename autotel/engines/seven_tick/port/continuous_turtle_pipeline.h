/**
 * @file continuous_turtle_pipeline.h
 * @brief CNS v8 Continuous Turtle Loop - Stream Processing Pipeline
 * @version 1.0.0
 * 
 * Implements infinite stream processing for turtle data with:
 * - Auto-scaling based on pattern distribution changes
 * - Zero-downtime pattern reloading
 * - Integration with 7-tick guarantees
 * - Lock-free concurrent processing
 */

#ifndef CONTINUOUS_TURTLE_PIPELINE_H
#define CONTINUOUS_TURTLE_PIPELINE_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include "bitactor.h"
#include "tick_collapse_engine.h"
#include "bitmask_compiler.h"

// Pipeline configuration constants
#define TURTLE_PIPELINE_MAX_WORKERS 64
#define TURTLE_PIPELINE_MIN_WORKERS 4
#define TURTLE_BUFFER_SIZE 8192
#define TURTLE_PATTERN_CACHE_SIZE 1024
#define TURTLE_SCALING_THRESHOLD 0.75
#define TURTLE_BACKPRESSURE_LIMIT 4096
#define TURTLE_CHECKPOINT_INTERVAL 1000

// Turtle data stream events
typedef enum {
    TURTLE_EVENT_TRIPLE,
    TURTLE_EVENT_PATTERN,
    TURTLE_EVENT_RULE,
    TURTLE_EVENT_CHECKPOINT,
    TURTLE_EVENT_SCALE_UP,
    TURTLE_EVENT_SCALE_DOWN,
    TURTLE_EVENT_RELOAD_PATTERN,
    TURTLE_EVENT_METRICS
} TurtleEventType;

// Stream event structure
typedef struct {
    TurtleEventType type;
    uint64_t timestamp_ns;
    uint32_t sequence_id;
    uint32_t partition_key;
    union {
        struct {
            char subject[256];
            char predicate[256];
            char object[256];
        } triple;
        struct {
            uint32_t pattern_id;
            uint8_t pattern_mask[32];
            double confidence;
        } pattern;
        struct {
            uint32_t rule_id;
            CompiledRule rule;
        } rule;
        struct {
            uint64_t processed_count;
            uint64_t error_count;
            double throughput_tps;
        } checkpoint;
    } data;
} TurtleEvent;

// Ring buffer for lock-free processing
typedef struct {
    TurtleEvent* events;
    uint32_t capacity;
    _Atomic uint32_t write_pos;
    _Atomic uint32_t read_pos;
    _Atomic uint32_t size;
    pthread_mutex_t resize_lock;
} TurtleRingBuffer;

// Worker thread state
typedef struct {
    uint32_t worker_id;
    pthread_t thread;
    _Atomic bool active;
    _Atomic uint64_t events_processed;
    _Atomic uint64_t processing_time_ns;
    BitActorMatrix* local_matrix;
    TickCollapseEngine* local_engine;
    struct TurtlePipeline* pipeline;
} TurtleWorker;

// Pattern distribution tracker
typedef struct {
    uint32_t pattern_id;
    _Atomic uint64_t occurrence_count;
    _Atomic uint64_t last_seen_ns;
    double distribution_weight;
} PatternDistribution;

// Auto-scaling metrics
typedef struct {
    _Atomic uint64_t total_events;
    _Atomic uint64_t processed_events;
    _Atomic double avg_processing_time_ns;
    _Atomic double queue_depth_ratio;
    _Atomic uint32_t active_workers;
    _Atomic bool scaling_in_progress;
    uint64_t last_scale_time_ns;
} ScalingMetrics;

// Pattern reload state
typedef struct {
    RuleSet* current_rules;
    RuleSet* pending_rules;
    _Atomic bool reload_requested;
    _Atomic uint32_t reload_generation;
    pthread_rwlock_t pattern_lock;
} PatternReloadState;

// Main pipeline structure
typedef struct TurtlePipeline {
    // Stream processing
    TurtleRingBuffer* input_buffer;
    TurtleRingBuffer* output_buffer;
    
    // Worker management
    TurtleWorker workers[TURTLE_PIPELINE_MAX_WORKERS];
    _Atomic uint32_t num_workers;
    
    // Pattern management
    PatternDistribution* patterns;
    uint32_t pattern_count;
    PatternReloadState reload_state;
    
    // Auto-scaling
    ScalingMetrics metrics;
    pthread_t scaling_thread;
    _Atomic bool scaling_enabled;
    
    // Pipeline state
    _Atomic bool running;
    _Atomic uint64_t global_sequence;
    uint64_t start_time_ns;
    
    // Integration points
    BitmaskCompiler* compiler;
    void (*event_callback)(TurtleEvent* event, void* context);
    void* callback_context;
} TurtlePipeline;

// Pipeline lifecycle functions
TurtlePipeline* turtle_pipeline_create(uint32_t initial_workers);
void turtle_pipeline_destroy(TurtlePipeline* pipeline);
bool turtle_pipeline_start(TurtlePipeline* pipeline);
void turtle_pipeline_stop(TurtlePipeline* pipeline);

// Stream processing functions
bool turtle_pipeline_submit(TurtlePipeline* pipeline, TurtleEvent* event);
bool turtle_pipeline_submit_batch(TurtlePipeline* pipeline, TurtleEvent* events, uint32_t count);
uint32_t turtle_pipeline_consume(TurtlePipeline* pipeline, TurtleEvent* events, uint32_t max_count);

// Pattern management functions
bool turtle_pipeline_reload_patterns(TurtlePipeline* pipeline, const char* pattern_ttl);
bool turtle_pipeline_add_pattern(TurtlePipeline* pipeline, uint32_t pattern_id, const uint8_t* mask);
bool turtle_pipeline_remove_pattern(TurtlePipeline* pipeline, uint32_t pattern_id);
void turtle_pipeline_get_pattern_distribution(TurtlePipeline* pipeline, PatternDistribution* dist, uint32_t* count);

// Auto-scaling functions
void turtle_pipeline_enable_autoscaling(TurtlePipeline* pipeline, bool enable);
void turtle_pipeline_set_scaling_thresholds(TurtlePipeline* pipeline, double scale_up, double scale_down);
uint32_t turtle_pipeline_get_worker_count(TurtlePipeline* pipeline);
void turtle_pipeline_force_scale(TurtlePipeline* pipeline, uint32_t target_workers);

// Monitoring and metrics
typedef struct {
    uint64_t total_events_processed;
    uint64_t total_errors;
    double throughput_eps; // events per second
    double avg_latency_ms;
    double p99_latency_ms;
    uint32_t queue_depth;
    uint32_t active_workers;
    double cpu_utilization;
    double memory_usage_mb;
    uint64_t pattern_cache_hits;
    uint64_t pattern_cache_misses;
} TurtlePipelineMetrics;

void turtle_pipeline_get_metrics(TurtlePipeline* pipeline, TurtlePipelineMetrics* metrics);
void turtle_pipeline_reset_metrics(TurtlePipeline* pipeline);

// Event callbacks
void turtle_pipeline_set_event_callback(TurtlePipeline* pipeline, 
                                       void (*callback)(TurtleEvent*, void*), 
                                       void* context);

// Checkpointing for fault tolerance
bool turtle_pipeline_checkpoint(TurtlePipeline* pipeline, const char* checkpoint_path);
bool turtle_pipeline_restore(TurtlePipeline* pipeline, const char* checkpoint_path);

// Integration with 7-tick guarantees
bool turtle_pipeline_validate_tick_constraint(TurtlePipeline* pipeline);
uint64_t turtle_pipeline_get_tick_latency_ns(TurtlePipeline* pipeline);

#endif // CONTINUOUS_TURTLE_PIPELINE_H