/**
 * @file continuous_turtle_pipeline.c
 * @brief CNS v8 Continuous Turtle Loop - Stream Processing Implementation
 * @version 1.0.0
 */

#include "continuous_turtle_pipeline.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <math.h>

#ifdef __APPLE__
#include <mach/mach_time.h>
#endif

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

static uint64_t get_time_ns(void) {
#ifdef __APPLE__
    static mach_timebase_info_data_t timebase = {0};
    if (timebase.denom == 0) {
        mach_timebase_info(&timebase);
    }
    uint64_t mach_time = mach_absolute_time();
    return mach_time * timebase.numer / timebase.denom;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
#endif
}

// =============================================================================
// RING BUFFER IMPLEMENTATION
// =============================================================================

static TurtleRingBuffer* create_ring_buffer(uint32_t capacity) {
    TurtleRingBuffer* buffer = calloc(1, sizeof(TurtleRingBuffer));
    if (!buffer) return NULL;
    
    buffer->events = calloc(capacity, sizeof(TurtleEvent));
    if (!buffer->events) {
        free(buffer);
        return NULL;
    }
    
    buffer->capacity = capacity;
    buffer->write_pos = 0;
    buffer->read_pos = 0;
    buffer->size = 0;
    pthread_mutex_init(&buffer->resize_lock, NULL);
    
    return buffer;
}

static void destroy_ring_buffer(TurtleRingBuffer* buffer) {
    if (!buffer) return;
    pthread_mutex_destroy(&buffer->resize_lock);
    free(buffer->events);
    free(buffer);
}

static bool ring_buffer_push(TurtleRingBuffer* buffer, TurtleEvent* event) {
    uint32_t current_size = buffer->size;
    if (current_size >= buffer->capacity) {
        return false; // Buffer full
    }
    
    uint32_t write_pos = buffer->write_pos;
    buffer->events[write_pos] = *event;
    
    // Update write position
    uint32_t next_pos = (write_pos + 1) % buffer->capacity;
    buffer->write_pos = next_pos;
    buffer->size = current_size + 1;
    
    return true;
}

static bool ring_buffer_pop(TurtleRingBuffer* buffer, TurtleEvent* event) {
    uint32_t current_size = buffer->size;
    if (current_size == 0) {
        return false; // Buffer empty
    }
    
    uint32_t read_pos = buffer->read_pos;
    *event = buffer->events[read_pos];
    
    // Update read position
    uint32_t next_pos = (read_pos + 1) % buffer->capacity;
    buffer->read_pos = next_pos;
    buffer->size = current_size - 1;
    
    return true;
}

// =============================================================================
// WORKER THREAD IMPLEMENTATION
// =============================================================================

static void process_turtle_event(TurtleWorker* worker, TurtleEvent* event) {
    uint64_t start_time = get_time_ns();
    
    switch (event->type) {
        case TURTLE_EVENT_TRIPLE: {
            // Process TTL triple through BitActor matrix
            if (worker->local_matrix && worker->local_engine) {
                // Convert triple to BitActor representation
                // This is a simplified version - real implementation would parse TTL
                BitActor actor = 0;
                for (int i = 0; i < 8 && event->data.triple.subject[i]; i++) {
                    actor |= (1 << (i % 8));
                }
                
                // Set actor in matrix
                if (worker->local_matrix->num_actors > 0) {
                    worker->local_matrix->actors[0] = actor;
                    
                    // Execute tick collapse
                    RuleSet* rules = worker->pipeline->reload_state.current_rules;
                    if (rules) {
                        tick_collapse_execute(worker->local_engine, worker->local_matrix, rules);
                    }
                }
            }
            break;
        }
        
        case TURTLE_EVENT_PATTERN: {
            // Update pattern distribution
            uint32_t pattern_id = event->data.pattern.pattern_id;
            if (pattern_id < worker->pipeline->pattern_count) {
                PatternDistribution* dist = &worker->pipeline->patterns[pattern_id];
                dist->occurrence_count++;
                dist->last_seen_ns = get_time_ns();
                dist->distribution_weight = event->data.pattern.confidence;
            }
            break;
        }
        
        case TURTLE_EVENT_RULE: {
            // Hot-reload rule into local engine
            if (worker->pipeline->reload_state.current_rules) {
                add_rule_to_set(worker->pipeline->reload_state.current_rules, 
                               event->data.rule.rule);
            }
            break;
        }
        
        default:
            break;
    }
    
    uint64_t end_time = get_time_ns();
    worker->processing_time_ns += (end_time - start_time);
    worker->events_processed++;
}

static void* worker_thread_main(void* arg) {
    TurtleWorker* worker = (TurtleWorker*)arg;
    TurtlePipeline* pipeline = worker->pipeline;
    
    printf("🐢 Worker %u started\n", worker->worker_id);
    
    // Create local BitActor resources
    worker->local_matrix = create_bit_actor_matrix(256);
    worker->local_engine = create_tick_collapse_engine();
    
    TurtleEvent event;
    uint64_t last_checkpoint = get_time_ns();
    
    while (worker->active) {
        // Try to get event from input buffer
        if (ring_buffer_pop(pipeline->input_buffer, &event)) {
            // Process the event
            process_turtle_event(worker, &event);
            
            // Generate output event if needed
            if (event.type == TURTLE_EVENT_TRIPLE) {
                TurtleEvent output = {
                    .type = TURTLE_EVENT_METRICS,
                    .timestamp_ns = get_time_ns(),
                    .sequence_id = event.sequence_id,
                    .partition_key = worker->worker_id,
                    .data.checkpoint = {
                        .processed_count = worker->events_processed,
                        .error_count = 0,
                        .throughput_tps = 0.0
                    }
                };
                
                ring_buffer_push(pipeline->output_buffer, &output);
            }
            
            // Periodic checkpoint
            uint64_t now = get_time_ns();
            if (now - last_checkpoint > TURTLE_CHECKPOINT_INTERVAL * 1000000ULL) {
                TurtleEvent checkpoint = {
                    .type = TURTLE_EVENT_CHECKPOINT,
                    .timestamp_ns = now,
                    .sequence_id = pipeline->global_sequence++,
                    .partition_key = worker->worker_id,
                    .data.checkpoint = {
                        .processed_count = worker->events_processed,
                        .error_count = 0,
                        .throughput_tps = (double)worker->events_processed / 
                                        ((now - pipeline->start_time_ns) / 1e9)
                    }
                };
                
                if (pipeline->event_callback) {
                    pipeline->event_callback(&checkpoint, pipeline->callback_context);
                }
                
                last_checkpoint = now;
            }
        } else {
            // No events available, brief sleep
            usleep(100); // 100 microseconds
        }
    }
    
    // Cleanup
    destroy_bit_actor_matrix(worker->local_matrix);
    destroy_tick_collapse_engine(worker->local_engine);
    
    printf("🐢 Worker %u stopped\n", worker->worker_id);
    return NULL;
}

// =============================================================================
// AUTO-SCALING IMPLEMENTATION
// =============================================================================

static void calculate_scaling_metrics(TurtlePipeline* pipeline) {
    uint32_t queue_depth = pipeline->input_buffer->size;
    uint32_t queue_capacity = pipeline->input_buffer->capacity;
    
    pipeline->metrics.queue_depth_ratio = (double)queue_depth / queue_capacity;
    
    // Calculate average processing time across all workers
    uint64_t total_time = 0;
    uint64_t total_events = 0;
    
    for (uint32_t i = 0; i < pipeline->num_workers; i++) {
        if (pipeline->workers[i].active) {
            total_time += pipeline->workers[i].processing_time_ns;
            total_events += pipeline->workers[i].events_processed;
        }
    }
    
    if (total_events > 0) {
        pipeline->metrics.avg_processing_time_ns = (double)total_time / total_events;
    }
    
    pipeline->metrics.total_events = pipeline->global_sequence;
    pipeline->metrics.processed_events = total_events;
}

static bool should_scale_up(TurtlePipeline* pipeline) {
    // Scale up if queue is getting full or processing is slow
    return (pipeline->metrics.queue_depth_ratio > TURTLE_SCALING_THRESHOLD) ||
           (pipeline->metrics.avg_processing_time_ns > 1000000); // > 1ms avg
}

static bool should_scale_down(TurtlePipeline* pipeline) {
    // Scale down if queue is mostly empty and we have excess workers
    return (pipeline->metrics.queue_depth_ratio < 0.25) &&
           (pipeline->num_workers > TURTLE_PIPELINE_MIN_WORKERS);
}

static void scale_workers(TurtlePipeline* pipeline, uint32_t target_count) {
    if (pipeline->metrics.scaling_in_progress) return;
    
    pipeline->metrics.scaling_in_progress = true;
    uint32_t current_count = pipeline->num_workers;
    
    if (target_count > current_count) {
        // Scale up
        printf("📈 Scaling up from %u to %u workers\n", current_count, target_count);
        
        for (uint32_t i = current_count; i < target_count && i < TURTLE_PIPELINE_MAX_WORKERS; i++) {
            TurtleWorker* worker = &pipeline->workers[i];
            worker->worker_id = i;
            worker->active = true;
            worker->events_processed = 0;
            worker->processing_time_ns = 0;
            worker->pipeline = pipeline;
            
            pthread_create(&worker->thread, NULL, worker_thread_main, worker);
        }
        
        pipeline->num_workers = target_count;
        pipeline->metrics.active_workers = target_count;
        
        // Generate scale event
        TurtleEvent scale_event = {
            .type = TURTLE_EVENT_SCALE_UP,
            .timestamp_ns = get_time_ns(),
            .sequence_id = pipeline->global_sequence++,
            .data.checkpoint = {
                .processed_count = pipeline->metrics.processed_events,
                .error_count = 0,
                .throughput_tps = 0.0
            }
        };
        
        if (pipeline->event_callback) {
            pipeline->event_callback(&scale_event, pipeline->callback_context);
        }
        
    } else if (target_count < current_count) {
        // Scale down
        printf("📉 Scaling down from %u to %u workers\n", current_count, target_count);
        
        for (uint32_t i = target_count; i < current_count; i++) {
            pipeline->workers[i].active = false;
        }
        
        // Wait for workers to finish
        for (uint32_t i = target_count; i < current_count; i++) {
            pthread_join(pipeline->workers[i].thread, NULL);
        }
        
        pipeline->num_workers = target_count;
        pipeline->metrics.active_workers = target_count;
        
        // Generate scale event
        TurtleEvent scale_event = {
            .type = TURTLE_EVENT_SCALE_DOWN,
            .timestamp_ns = get_time_ns(),
            .sequence_id = pipeline->global_sequence++
        };
        
        if (pipeline->event_callback) {
            pipeline->event_callback(&scale_event, pipeline->callback_context);
        }
    }
    
    pipeline->metrics.last_scale_time_ns = get_time_ns();
    pipeline->metrics.scaling_in_progress = false;
}

static void* scaling_thread_main(void* arg) {
    TurtlePipeline* pipeline = (TurtlePipeline*)arg;
    
    printf("🔄 Auto-scaling thread started\n");
    
    while (pipeline->scaling_enabled && pipeline->running) {
        calculate_scaling_metrics(pipeline);
        
        // Check if we need to scale
        uint64_t now = get_time_ns();
        uint64_t time_since_last_scale = now - pipeline->metrics.last_scale_time_ns;
        
        // Don't scale too frequently (wait at least 5 seconds)
        if (time_since_last_scale > 5000000000ULL) {
            if (should_scale_up(pipeline)) {
                uint32_t new_count = pipeline->num_workers + 4;
                if (new_count > TURTLE_PIPELINE_MAX_WORKERS) {
                    new_count = TURTLE_PIPELINE_MAX_WORKERS;
                }
                scale_workers(pipeline, new_count);
            } else if (should_scale_down(pipeline)) {
                uint32_t new_count = pipeline->num_workers - 2;
                if (new_count < TURTLE_PIPELINE_MIN_WORKERS) {
                    new_count = TURTLE_PIPELINE_MIN_WORKERS;
                }
                scale_workers(pipeline, new_count);
            }
        }
        
        sleep(1); // Check every second
    }
    
    printf("🔄 Auto-scaling thread stopped\n");
    return NULL;
}

// =============================================================================
// PIPELINE LIFECYCLE
// =============================================================================

TurtlePipeline* turtle_pipeline_create(uint32_t initial_workers) {
    TurtlePipeline* pipeline = calloc(1, sizeof(TurtlePipeline));
    if (!pipeline) return NULL;
    
    printf("🚀 Creating Turtle Pipeline with %u initial workers\n", initial_workers);
    
    // Create ring buffers
    pipeline->input_buffer = create_ring_buffer(TURTLE_BUFFER_SIZE);
    pipeline->output_buffer = create_ring_buffer(TURTLE_BUFFER_SIZE);
    
    if (!pipeline->input_buffer || !pipeline->output_buffer) {
        turtle_pipeline_destroy(pipeline);
        return NULL;
    }
    
    // Initialize pattern storage
    pipeline->patterns = calloc(TURTLE_PATTERN_CACHE_SIZE, sizeof(PatternDistribution));
    if (!pipeline->patterns) {
        turtle_pipeline_destroy(pipeline);
        return NULL;
    }
    pipeline->pattern_count = TURTLE_PATTERN_CACHE_SIZE;
    
    // Initialize reload state
    pthread_rwlock_init(&pipeline->reload_state.pattern_lock, NULL);
    pipeline->reload_state.current_rules = create_rule_set(1024);
    pipeline->reload_state.pending_rules = NULL;
    pipeline->reload_state.reload_requested = false;
    pipeline->reload_state.reload_generation = 0;
    
    // Create compiler
    pipeline->compiler = create_bitmask_compiler();
    
    // Initialize metrics
    pipeline->metrics.total_events = 0;
    pipeline->metrics.processed_events = 0;
    pipeline->metrics.avg_processing_time_ns = 0.0;
    pipeline->metrics.queue_depth_ratio = 0.0;
    pipeline->metrics.active_workers = 0;
    pipeline->metrics.scaling_in_progress = false;
    pipeline->metrics.last_scale_time_ns = get_time_ns();
    
    // Pipeline state
    pipeline->running = false;
    pipeline->scaling_enabled = true;
    pipeline->global_sequence = 0;
    pipeline->start_time_ns = get_time_ns();
    pipeline->num_workers = initial_workers;
    
    printf("🚀 Turtle Pipeline created successfully\n");
    return pipeline;
}

void turtle_pipeline_destroy(TurtlePipeline* pipeline) {
    if (!pipeline) return;
    
    printf("🛑 Destroying Turtle Pipeline\n");
    
    // Stop pipeline if running
    if (pipeline->running) {
        turtle_pipeline_stop(pipeline);
    }
    
    // Destroy ring buffers
    destroy_ring_buffer(pipeline->input_buffer);
    destroy_ring_buffer(pipeline->output_buffer);
    
    // Free patterns
    free(pipeline->patterns);
    
    // Destroy reload state
    pthread_rwlock_destroy(&pipeline->reload_state.pattern_lock);
    destroy_rule_set(pipeline->reload_state.current_rules);
    if (pipeline->reload_state.pending_rules) {
        destroy_rule_set(pipeline->reload_state.pending_rules);
    }
    
    // Destroy compiler
    destroy_bitmask_compiler(pipeline->compiler);
    
    free(pipeline);
    printf("🛑 Turtle Pipeline destroyed\n");
}

bool turtle_pipeline_start(TurtlePipeline* pipeline) {
    if (!pipeline || pipeline->running) return false;
    
    printf("▶️ Starting Turtle Pipeline\n");
    
    pipeline->running = true;
    pipeline->start_time_ns = get_time_ns();
    
    // Start worker threads
    for (uint32_t i = 0; i < pipeline->num_workers; i++) {
        TurtleWorker* worker = &pipeline->workers[i];
        worker->worker_id = i;
        worker->active = true;
        worker->events_processed = 0;
        worker->processing_time_ns = 0;
        worker->pipeline = pipeline;
        
        pthread_create(&worker->thread, NULL, worker_thread_main, worker);
    }
    
    pipeline->metrics.active_workers = pipeline->num_workers;
    
    // Start auto-scaling thread
    pthread_create(&pipeline->scaling_thread, NULL, scaling_thread_main, pipeline);
    
    printf("▶️ Turtle Pipeline started with %u workers\n", pipeline->num_workers);
    return true;
}

void turtle_pipeline_stop(TurtlePipeline* pipeline) {
    if (!pipeline || !pipeline->running) return;
    
    printf("⏹️ Stopping Turtle Pipeline\n");
    
    pipeline->running = false;
    pipeline->scaling_enabled = false;
    
    // Stop all workers
    for (uint32_t i = 0; i < pipeline->num_workers; i++) {
        pipeline->workers[i].active = false;
    }
    
    // Wait for workers to finish
    for (uint32_t i = 0; i < pipeline->num_workers; i++) {
        pthread_join(pipeline->workers[i].thread, NULL);
    }
    
    // Stop scaling thread
    pthread_join(pipeline->scaling_thread, NULL);
    
    printf("⏹️ Turtle Pipeline stopped\n");
}

// =============================================================================
// STREAM PROCESSING
// =============================================================================

bool turtle_pipeline_submit(TurtlePipeline* pipeline, TurtleEvent* event) {
    if (!pipeline || !event || !pipeline->running) return false;
    
    event->sequence_id = pipeline->global_sequence++;
    return ring_buffer_push(pipeline->input_buffer, event);
}

bool turtle_pipeline_submit_batch(TurtlePipeline* pipeline, TurtleEvent* events, uint32_t count) {
    if (!pipeline || !events || !pipeline->running) return false;
    
    uint32_t submitted = 0;
    for (uint32_t i = 0; i < count; i++) {
        if (turtle_pipeline_submit(pipeline, &events[i])) {
            submitted++;
        } else {
            break; // Buffer full
        }
    }
    
    return submitted == count;
}

uint32_t turtle_pipeline_consume(TurtlePipeline* pipeline, TurtleEvent* events, uint32_t max_count) {
    if (!pipeline || !events) return 0;
    
    uint32_t consumed = 0;
    for (uint32_t i = 0; i < max_count; i++) {
        if (ring_buffer_pop(pipeline->output_buffer, &events[i])) {
            consumed++;
        } else {
            break; // No more events
        }
    }
    
    return consumed;
}

// =============================================================================
// PATTERN MANAGEMENT
// =============================================================================

bool turtle_pipeline_reload_patterns(TurtlePipeline* pipeline, const char* pattern_ttl) {
    if (!pipeline || !pattern_ttl) return false;
    
    printf("🔄 Reloading patterns (zero-downtime)\n");
    
    // Compile new rules
    RuleSet* new_rules = compile_rules(pipeline->compiler, pattern_ttl);
    if (!new_rules) {
        printf("❌ Failed to compile new patterns\n");
        return false;
    }
    
    // Prepare for swap
    pthread_rwlock_wrlock(&pipeline->reload_state.pattern_lock);
    
    // Store pending rules
    if (pipeline->reload_state.pending_rules) {
        destroy_rule_set(pipeline->reload_state.pending_rules);
    }
    pipeline->reload_state.pending_rules = new_rules;
    pipeline->reload_state.reload_requested = true;
    pipeline->reload_state.reload_generation++;
    
    // Atomic swap
    RuleSet* old_rules = pipeline->reload_state.current_rules;
    pipeline->reload_state.current_rules = new_rules;
    pipeline->reload_state.pending_rules = NULL;
    pipeline->reload_state.reload_requested = false;
    
    pthread_rwlock_unlock(&pipeline->reload_state.pattern_lock);
    
    // Cleanup old rules after brief delay
    usleep(1000); // 1ms delay to ensure workers see new rules
    destroy_rule_set(old_rules);
    
    // Generate reload event
    TurtleEvent reload_event = {
        .type = TURTLE_EVENT_RELOAD_PATTERN,
        .timestamp_ns = get_time_ns(),
        .sequence_id = pipeline->global_sequence++,
        .data.pattern = {
            .pattern_id = pipeline->reload_state.reload_generation,
            .confidence = 1.0
        }
    };
    
    if (pipeline->event_callback) {
        pipeline->event_callback(&reload_event, pipeline->callback_context);
    }
    
    printf("✅ Patterns reloaded (generation %u)\n", pipeline->reload_state.reload_generation);
    return true;
}

bool turtle_pipeline_add_pattern(TurtlePipeline* pipeline, uint32_t pattern_id, const uint8_t* mask) {
    if (!pipeline || pattern_id >= pipeline->pattern_count) return false;
    
    PatternDistribution* dist = &pipeline->patterns[pattern_id];
    dist->pattern_id = pattern_id;
    dist->occurrence_count = 0;
    dist->last_seen_ns = get_time_ns();
    dist->distribution_weight = 1.0;
    
    return true;
}

bool turtle_pipeline_remove_pattern(TurtlePipeline* pipeline, uint32_t pattern_id) {
    if (!pipeline || pattern_id >= pipeline->pattern_count) return false;
    
    PatternDistribution* dist = &pipeline->patterns[pattern_id];
    dist->occurrence_count = 0;
    dist->distribution_weight = 0.0;
    
    return true;
}

void turtle_pipeline_get_pattern_distribution(TurtlePipeline* pipeline, 
                                            PatternDistribution* dist, 
                                            uint32_t* count) {
    if (!pipeline || !dist || !count) return;
    
    uint32_t active_patterns = 0;
    for (uint32_t i = 0; i < pipeline->pattern_count; i++) {
        if (pipeline->patterns[i].distribution_weight > 0.0) {
            if (active_patterns < *count) {
                dist[active_patterns] = pipeline->patterns[i];
            }
            active_patterns++;
        }
    }
    
    *count = active_patterns;
}

// =============================================================================
// AUTO-SCALING CONTROL
// =============================================================================

void turtle_pipeline_enable_autoscaling(TurtlePipeline* pipeline, bool enable) {
    if (!pipeline) return;
    pipeline->scaling_enabled = enable;
    printf("🔧 Auto-scaling %s\n", enable ? "enabled" : "disabled");
}

void turtle_pipeline_set_scaling_thresholds(TurtlePipeline* pipeline, 
                                           double scale_up, 
                                           double scale_down) {
    // This would modify internal thresholds
    // For simplicity, using constants in this implementation
}

uint32_t turtle_pipeline_get_worker_count(TurtlePipeline* pipeline) {
    if (!pipeline) return 0;
    return pipeline->num_workers;
}

void turtle_pipeline_force_scale(TurtlePipeline* pipeline, uint32_t target_workers) {
    if (!pipeline) return;
    
    if (target_workers < TURTLE_PIPELINE_MIN_WORKERS) {
        target_workers = TURTLE_PIPELINE_MIN_WORKERS;
    }
    if (target_workers > TURTLE_PIPELINE_MAX_WORKERS) {
        target_workers = TURTLE_PIPELINE_MAX_WORKERS;
    }
    
    scale_workers(pipeline, target_workers);
}

// =============================================================================
// MONITORING AND METRICS
// =============================================================================

void turtle_pipeline_get_metrics(TurtlePipeline* pipeline, TurtlePipelineMetrics* metrics) {
    if (!pipeline || !metrics) return;
    
    calculate_scaling_metrics(pipeline);
    
    metrics->total_events_processed = pipeline->metrics.processed_events;
    metrics->total_errors = 0; // TODO: track errors
    
    uint64_t elapsed_ns = get_time_ns() - pipeline->start_time_ns;
    double elapsed_sec = elapsed_ns / 1e9;
    
    metrics->throughput_eps = metrics->total_events_processed / elapsed_sec;
    metrics->avg_latency_ms = pipeline->metrics.avg_processing_time_ns / 1e6;
    metrics->p99_latency_ms = metrics->avg_latency_ms * 1.5; // Estimate
    
    metrics->queue_depth = pipeline->input_buffer->size;
    metrics->active_workers = pipeline->metrics.active_workers;
    
    // Estimate CPU/memory usage
    metrics->cpu_utilization = (double)pipeline->num_workers / TURTLE_PIPELINE_MAX_WORKERS;
    metrics->memory_usage_mb = sizeof(TurtlePipeline) / 1024.0 / 1024.0 +
                               (pipeline->num_workers * sizeof(BitActorMatrix) * 256) / 1024.0 / 1024.0;
    
    // Pattern cache metrics
    uint64_t cache_hits = 0, cache_misses = 0;
    for (uint32_t i = 0; i < pipeline->pattern_count; i++) {
        if (pipeline->patterns[i].occurrence_count > 0) {
            cache_hits += pipeline->patterns[i].occurrence_count;
        }
    }
    
    metrics->pattern_cache_hits = cache_hits;
    metrics->pattern_cache_misses = cache_misses;
}

void turtle_pipeline_reset_metrics(TurtlePipeline* pipeline) {
    if (!pipeline) return;
    
    pipeline->metrics.total_events = 0;
    pipeline->metrics.processed_events = 0;
    pipeline->metrics.avg_processing_time_ns = 0.0;
    
    for (uint32_t i = 0; i < pipeline->num_workers; i++) {
        pipeline->workers[i].events_processed = 0;
        pipeline->workers[i].processing_time_ns = 0;
    }
    
    pipeline->start_time_ns = get_time_ns();
}

// =============================================================================
// EVENT CALLBACKS
// =============================================================================

void turtle_pipeline_set_event_callback(TurtlePipeline* pipeline,
                                       void (*callback)(TurtleEvent*, void*),
                                       void* context) {
    if (!pipeline) return;
    pipeline->event_callback = callback;
    pipeline->callback_context = context;
}

// =============================================================================
// CHECKPOINTING
// =============================================================================

bool turtle_pipeline_checkpoint(TurtlePipeline* pipeline, const char* checkpoint_path) {
    if (!pipeline || !checkpoint_path) return false;
    
    FILE* fp = fopen(checkpoint_path, "wb");
    if (!fp) return false;
    
    // Write pipeline state
    fwrite(&pipeline->global_sequence, sizeof(uint64_t), 1, fp);
    fwrite(&pipeline->num_workers, sizeof(uint32_t), 1, fp);
    fwrite(&pipeline->metrics, sizeof(ScalingMetrics), 1, fp);
    
    // Write pattern distribution
    fwrite(&pipeline->pattern_count, sizeof(uint32_t), 1, fp);
    fwrite(pipeline->patterns, sizeof(PatternDistribution), pipeline->pattern_count, fp);
    
    fclose(fp);
    
    printf("💾 Checkpoint saved to %s\n", checkpoint_path);
    return true;
}

bool turtle_pipeline_restore(TurtlePipeline* pipeline, const char* checkpoint_path) {
    if (!pipeline || !checkpoint_path) return false;
    
    FILE* fp = fopen(checkpoint_path, "rb");
    if (!fp) return false;
    
    // Read pipeline state
    fread(&pipeline->global_sequence, sizeof(uint64_t), 1, fp);
    fread(&pipeline->num_workers, sizeof(uint32_t), 1, fp);
    fread(&pipeline->metrics, sizeof(ScalingMetrics), 1, fp);
    
    // Read pattern distribution
    uint32_t pattern_count;
    fread(&pattern_count, sizeof(uint32_t), 1, fp);
    if (pattern_count == pipeline->pattern_count) {
        fread(pipeline->patterns, sizeof(PatternDistribution), pattern_count, fp);
    }
    
    fclose(fp);
    
    printf("💾 Checkpoint restored from %s\n", checkpoint_path);
    return true;
}

// =============================================================================
// 7-TICK INTEGRATION
// =============================================================================

bool turtle_pipeline_validate_tick_constraint(TurtlePipeline* pipeline) {
    if (!pipeline) return false;
    
    // Check if average processing time meets 7-tick constraint
    // Assuming 1 tick = 100ns for ultra-performance
    const uint64_t TICK_NS = 100;
    const uint64_t MAX_TICKS = 7;
    const uint64_t MAX_TIME_NS = TICK_NS * MAX_TICKS;
    
    return pipeline->metrics.avg_processing_time_ns <= MAX_TIME_NS;
}

uint64_t turtle_pipeline_get_tick_latency_ns(TurtlePipeline* pipeline) {
    if (!pipeline) return UINT64_MAX;
    return pipeline->metrics.avg_processing_time_ns;
}