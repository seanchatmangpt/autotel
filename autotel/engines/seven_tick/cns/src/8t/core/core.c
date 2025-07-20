#include "cns/8t/core.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

// 8T Core Implementation - 80/20 approach

typedef struct {
    parser_interface_t* parser;
    cache_interface_t* cache;
    scheduler_interface_t* scheduler;
    materializer_interface_t* materializer;
    monitor_interface_t* monitor;
    metrics_t metrics;
    pthread_mutex_t metrics_lock;
} t8_context_t;

static t8_context_t* g_ctx = NULL;

int t8_init(void) {
    if (g_ctx != NULL) {
        return -1; // Already initialized
    }
    
    g_ctx = calloc(1, sizeof(t8_context_t));
    if (g_ctx == NULL) {
        return -1;
    }
    
    pthread_mutex_init(&g_ctx->metrics_lock, NULL);
    
    // Initialize default components if registered
    if (g_ctx->cache && g_ctx->cache->init) {
        g_ctx->cache->init(T8_DEFAULT_CACHE_SIZE);
    }
    
    if (g_ctx->scheduler && g_ctx->scheduler->init) {
        g_ctx->scheduler->init(T8_DEFAULT_BATCH_SIZE);
    }
    
    if (g_ctx->monitor && g_ctx->monitor->init) {
        g_ctx->monitor->init();
    }
    
    return 0;
}

void t8_shutdown(void) {
    if (g_ctx == NULL) {
        return;
    }
    
    // Cleanup components
    if (g_ctx->parser && g_ctx->parser->cleanup) {
        g_ctx->parser->cleanup();
    }
    
    if (g_ctx->cache && g_ctx->cache->cleanup) {
        g_ctx->cache->cleanup();
    }
    
    if (g_ctx->scheduler && g_ctx->scheduler->cleanup) {
        g_ctx->scheduler->cleanup();
    }
    
    if (g_ctx->materializer && g_ctx->materializer->cleanup) {
        g_ctx->materializer->cleanup();
    }
    
    if (g_ctx->monitor && g_ctx->monitor->cleanup) {
        g_ctx->monitor->cleanup();
    }
    
    pthread_mutex_destroy(&g_ctx->metrics_lock);
    free(g_ctx);
    g_ctx = NULL;
}

int t8_process_tick(tick_t* tick) {
    if (g_ctx == NULL || tick == NULL) {
        return -1;
    }
    
    // Update metrics
    pthread_mutex_lock(&g_ctx->metrics_lock);
    g_ctx->metrics.ticks_processed++;
    pthread_mutex_unlock(&g_ctx->metrics_lock);
    
    // Process based on tick type
    // 80/20: Focus on most common operations
    
    return 0;
}

int t8_process_batch(tick_t* ticks, size_t count) {
    if (g_ctx == NULL || ticks == NULL || count == 0) {
        return -1;
    }
    
    for (size_t i = 0; i < count; i++) {
        int ret = t8_process_tick(&ticks[i]);
        if (ret != 0) {
            return ret;
        }
    }
    
    return 0;
}

void t8_get_metrics(metrics_t* metrics) {
    if (g_ctx == NULL || metrics == NULL) {
        return;
    }
    
    pthread_mutex_lock(&g_ctx->metrics_lock);
    *metrics = g_ctx->metrics;
    pthread_mutex_unlock(&g_ctx->metrics_lock);
}

void t8_reset_metrics(void) {
    if (g_ctx == NULL) {
        return;
    }
    
    pthread_mutex_lock(&g_ctx->metrics_lock);
    memset(&g_ctx->metrics, 0, sizeof(metrics_t));
    pthread_mutex_unlock(&g_ctx->metrics_lock);
}

// Component registration
void t8_register_parser(parser_interface_t* parser) {
    if (g_ctx != NULL) {
        g_ctx->parser = parser;
    }
}

void t8_register_cache(cache_interface_t* cache) {
    if (g_ctx != NULL) {
        g_ctx->cache = cache;
    }
}

void t8_register_scheduler(scheduler_interface_t* scheduler) {
    if (g_ctx != NULL) {
        g_ctx->scheduler = scheduler;
    }
}

void t8_register_materializer(materializer_interface_t* materializer) {
    if (g_ctx != NULL) {
        g_ctx->materializer = materializer;
    }
}

void t8_register_monitor(monitor_interface_t* monitor) {
    if (g_ctx != NULL) {
        g_ctx->monitor = monitor;
    }
}