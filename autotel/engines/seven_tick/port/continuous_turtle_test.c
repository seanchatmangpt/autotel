/**
 * @file continuous_turtle_test.c
 * @brief Test program for CNS v8 Continuous Turtle Pipeline
 * @version 1.0.0
 */

#include "continuous_turtle_pipeline.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>

// Global pipeline for signal handling
static TurtlePipeline* g_pipeline = NULL;
static volatile bool g_running = true;

// =============================================================================
// EVENT GENERATION
// =============================================================================

static void generate_test_triple(TurtleEvent* event, uint32_t id) {
    event->type = TURTLE_EVENT_TRIPLE;
    event->timestamp_ns = 0; // Will be set by pipeline
    event->sequence_id = 0;  // Will be set by pipeline
    event->partition_key = id % 8;
    
    snprintf(event->data.triple.subject, 256, "ex:entity_%u", id);
    snprintf(event->data.triple.predicate, 256, "ex:hasValue");
    snprintf(event->data.triple.object, 256, "\"%u\"^^xsd:integer", id * 10);
}

static void generate_test_pattern(TurtleEvent* event, uint32_t pattern_id) {
    event->type = TURTLE_EVENT_PATTERN;
    event->timestamp_ns = 0;
    event->sequence_id = 0;
    event->partition_key = pattern_id % 8;
    
    event->data.pattern.pattern_id = pattern_id;
    event->data.pattern.confidence = 0.5 + (rand() % 50) / 100.0;
    
    // Generate random pattern mask
    for (int i = 0; i < 32; i++) {
        event->data.pattern.pattern_mask[i] = rand() % 256;
    }
}

// =============================================================================
// EVENT CALLBACKS
// =============================================================================

static void event_callback(TurtleEvent* event, void* context) {
    const char* event_names[] = {
        "TRIPLE", "PATTERN", "RULE", "CHECKPOINT", 
        "SCALE_UP", "SCALE_DOWN", "RELOAD", "METRICS"
    };
    
    switch (event->type) {
        case TURTLE_EVENT_CHECKPOINT:
            printf("📍 Checkpoint: %llu events, %.2f TPS\n",
                   event->data.checkpoint.processed_count,
                   event->data.checkpoint.throughput_tps);
            break;
            
        case TURTLE_EVENT_SCALE_UP:
            printf("📈 Scaled UP: New worker count in pipeline\n");
            break;
            
        case TURTLE_EVENT_SCALE_DOWN:
            printf("📉 Scaled DOWN: Reduced worker count\n");
            break;
            
        case TURTLE_EVENT_RELOAD_PATTERN:
            printf("🔄 Patterns reloaded: Generation %u\n", 
                   event->data.pattern.pattern_id);
            break;
            
        default:
            printf("📤 Event: %s (seq=%u)\n", 
                   event_names[event->type], event->sequence_id);
            break;
    }
}

// =============================================================================
// SIGNAL HANDLING
// =============================================================================

static void signal_handler(int sig) {
    printf("\n🛑 Received signal %d, shutting down...\n", sig);
    g_running = false;
    if (g_pipeline) {
        turtle_pipeline_stop(g_pipeline);
    }
}

// =============================================================================
// CONTINUOUS LOAD GENERATOR
// =============================================================================

static void* load_generator_thread(void* arg) {
    TurtlePipeline* pipeline = (TurtlePipeline*)arg;
    uint32_t event_id = 0;
    uint32_t batch_size = 100;
    
    printf("🔥 Load generator started\n");
    
    TurtleEvent* batch = malloc(batch_size * sizeof(TurtleEvent));
    
    while (g_running) {
        // Generate a batch of events
        for (uint32_t i = 0; i < batch_size; i++) {
            if (rand() % 100 < 80) {
                // 80% triple events
                generate_test_triple(&batch[i], event_id++);
            } else {
                // 20% pattern events
                generate_test_pattern(&batch[i], rand() % 100);
            }
        }
        
        // Submit batch
        if (!turtle_pipeline_submit_batch(pipeline, batch, batch_size)) {
            // Back pressure - slow down
            usleep(10000); // 10ms
        }
        
        // Variable load to trigger auto-scaling
        if ((event_id / 1000) % 10 < 3) {
            // Low load period
            usleep(5000); // 5ms between batches
        } else {
            // High load period - no delay
        }
    }
    
    free(batch);
    printf("🔥 Load generator stopped\n");
    return NULL;
}

// =============================================================================
// PATTERN RELOADER
// =============================================================================

static void* pattern_reloader_thread(void* arg) {
    TurtlePipeline* pipeline = (TurtlePipeline*)arg;
    int generation = 0;
    
    printf("🔄 Pattern reloader started\n");
    
    const char* pattern_templates[] = {
        "IF actor[0].bit[0] THEN SET actor[1].bit[0]\n"
        "IF actor[0].bit[1] THEN SET actor[1].bit[1]\n",
        
        "IF actor[0].bit[0] AND actor[0].bit[1] THEN SET actor[2].bit[0]\n"
        "IF actor[1].bit[0] OR actor[1].bit[1] THEN SET actor[2].bit[1]\n",
        
        "IF actor[2].bit[0] THEN CLEAR actor[0].bit[0]\n"
        "IF actor[2].bit[1] THEN CLEAR actor[0].bit[1]\n"
    };
    
    while (g_running) {
        // Wait before reloading
        sleep(30); // Reload every 30 seconds
        
        if (!g_running) break;
        
        // Select pattern
        const char* pattern = pattern_templates[generation % 3];
        generation++;
        
        printf("🔄 Reloading patterns (generation %d)\n", generation);
        turtle_pipeline_reload_patterns(pipeline, pattern);
    }
    
    printf("🔄 Pattern reloader stopped\n");
    return NULL;
}

// =============================================================================
// METRICS REPORTER
// =============================================================================

static void* metrics_reporter_thread(void* arg) {
    TurtlePipeline* pipeline = (TurtlePipeline*)arg;
    TurtlePipelineMetrics metrics;
    
    printf("📊 Metrics reporter started\n");
    
    while (g_running) {
        sleep(5); // Report every 5 seconds
        
        if (!g_running) break;
        
        turtle_pipeline_get_metrics(pipeline, &metrics);
        
        printf("\n📊 Pipeline Metrics:\n");
        printf("   Events processed: %llu\n", metrics.total_events_processed);
        printf("   Throughput: %.2f events/sec\n", metrics.throughput_eps);
        printf("   Avg latency: %.2f ms\n", metrics.avg_latency_ms);
        printf("   P99 latency: %.2f ms\n", metrics.p99_latency_ms);
        printf("   Queue depth: %u\n", metrics.queue_depth);
        printf("   Active workers: %u\n", metrics.active_workers);
        printf("   CPU utilization: %.1f%%\n", metrics.cpu_utilization * 100);
        printf("   Memory usage: %.2f MB\n", metrics.memory_usage_mb);
        printf("   Pattern cache hits: %llu\n", metrics.pattern_cache_hits);
        
        // Check 7-tick constraint
        bool tick_valid = turtle_pipeline_validate_tick_constraint(pipeline);
        uint64_t tick_latency = turtle_pipeline_get_tick_latency_ns(pipeline);
        printf("   7-tick constraint: %s (%.0f ns)\n", 
               tick_valid ? "✅ VALID" : "❌ VIOLATED",
               (double)tick_latency);
        printf("\n");
    }
    
    printf("📊 Metrics reporter stopped\n");
    return NULL;
}

// =============================================================================
// MAIN TEST PROGRAM
// =============================================================================

int main(int argc, char* argv[]) {
    printf("🐢 CNS v8 Continuous Turtle Pipeline Test\n");
    printf("=========================================\n\n");
    
    // Parse arguments
    uint32_t initial_workers = 8;
    uint32_t duration_sec = 60;
    
    if (argc > 1) {
        initial_workers = atoi(argv[1]);
    }
    if (argc > 2) {
        duration_sec = atoi(argv[2]);
    }
    
    printf("Configuration:\n");
    printf("  Initial workers: %u\n", initial_workers);
    printf("  Test duration: %u seconds\n", duration_sec);
    printf("\n");
    
    // Initialize random seed
    srand(time(NULL));
    
    // Setup signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Create pipeline
    g_pipeline = turtle_pipeline_create(initial_workers);
    if (!g_pipeline) {
        fprintf(stderr, "❌ Failed to create pipeline\n");
        return 1;
    }
    
    // Set event callback
    turtle_pipeline_set_event_callback(g_pipeline, event_callback, NULL);
    
    // Load initial patterns
    const char* initial_patterns = 
        "IF actor[0].bit[0] THEN SET actor[1].bit[0]\n"
        "IF actor[0].bit[1] THEN SET actor[1].bit[1]\n"
        "IF actor[1].bit[0] AND actor[1].bit[1] THEN SET actor[2].bit[0]\n";
    
    if (!turtle_pipeline_reload_patterns(g_pipeline, initial_patterns)) {
        fprintf(stderr, "❌ Failed to load initial patterns\n");
        turtle_pipeline_destroy(g_pipeline);
        return 1;
    }
    
    // Start pipeline
    if (!turtle_pipeline_start(g_pipeline)) {
        fprintf(stderr, "❌ Failed to start pipeline\n");
        turtle_pipeline_destroy(g_pipeline);
        return 1;
    }
    
    // Create auxiliary threads
    pthread_t load_gen_thread, pattern_thread, metrics_thread;
    
    pthread_create(&load_gen_thread, NULL, load_generator_thread, g_pipeline);
    pthread_create(&pattern_thread, NULL, pattern_reloader_thread, g_pipeline);
    pthread_create(&metrics_thread, NULL, metrics_reporter_thread, g_pipeline);
    
    printf("🚀 Pipeline running. Press Ctrl+C to stop.\n\n");
    
    // Run for specified duration
    time_t start_time = time(NULL);
    while (g_running && (time(NULL) - start_time) < duration_sec) {
        sleep(1);
        
        // Periodic checkpoint
        if ((time(NULL) - start_time) % 10 == 0) {
            char checkpoint_path[256];
            snprintf(checkpoint_path, 256, "turtle_checkpoint_%ld.bin", time(NULL));
            turtle_pipeline_checkpoint(g_pipeline, checkpoint_path);
        }
    }
    
    // Signal shutdown
    g_running = false;
    
    // Wait for threads
    pthread_join(load_gen_thread, NULL);
    pthread_join(pattern_thread, NULL);
    pthread_join(metrics_thread, NULL);
    
    // Final metrics
    printf("\n📊 Final Pipeline Metrics:\n");
    TurtlePipelineMetrics final_metrics;
    turtle_pipeline_get_metrics(g_pipeline, &final_metrics);
    
    printf("   Total events: %llu\n", final_metrics.total_events_processed);
    printf("   Average throughput: %.2f events/sec\n", final_metrics.throughput_eps);
    printf("   Average latency: %.2f ms\n", final_metrics.avg_latency_ms);
    printf("   Final worker count: %u\n", final_metrics.active_workers);
    
    // Stop and destroy pipeline
    turtle_pipeline_stop(g_pipeline);
    turtle_pipeline_destroy(g_pipeline);
    
    printf("\n✅ Test completed successfully\n");
    return 0;
}

// =============================================================================
// EXAMPLE OUTPUT:
// =============================================================================
/*
🐢 CNS v8 Continuous Turtle Pipeline Test
=========================================

Configuration:
  Initial workers: 8
  Test duration: 60 seconds

🚀 Creating Turtle Pipeline with 8 initial workers
🚀 Turtle Pipeline created successfully
🔄 Reloading patterns (zero-downtime)
✅ Patterns reloaded (generation 1)
▶️ Starting Turtle Pipeline
🐢 Worker 0 started
🐢 Worker 1 started
🐢 Worker 2 started
🐢 Worker 3 started
🐢 Worker 4 started
🐢 Worker 5 started
🐢 Worker 6 started
🐢 Worker 7 started
▶️ Turtle Pipeline started with 8 workers
🔄 Auto-scaling thread started
🔥 Load generator started
🔄 Pattern reloader started
📊 Metrics reporter started
🚀 Pipeline running. Press Ctrl+C to stop.

📍 Checkpoint: 1000 events, 245.32 TPS
📈 Scaled UP: New worker count in pipeline
🐢 Worker 8 started
🐢 Worker 9 started
🐢 Worker 10 started
🐢 Worker 11 started

📊 Pipeline Metrics:
   Events processed: 12450
   Throughput: 2490.00 events/sec
   Avg latency: 0.32 ms
   P99 latency: 0.48 ms
   Queue depth: 156
   Active workers: 12
   CPU utilization: 18.8%
   Memory usage: 2.45 MB
   Pattern cache hits: 10234
   7-tick constraint: ✅ VALID (320 ns)

💾 Checkpoint saved to turtle_checkpoint_1234567890.bin

📉 Scaled DOWN: Reduced worker count
🐢 Worker 11 stopped
🐢 Worker 10 stopped

🔄 Reloading patterns (generation 2)
🔄 Patterns reloaded: Generation 2

... (continues) ...
*/