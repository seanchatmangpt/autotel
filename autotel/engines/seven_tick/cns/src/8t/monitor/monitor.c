#include "cns/8t/interfaces.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

// 8T Monitor Implementation - 80/20 performance monitoring

typedef struct metric_entry {
    char name[64];
    double value;
    uint64_t timestamp;
    struct metric_entry* next;
} metric_entry_t;

typedef struct {
    metric_entry_t* metrics;
    size_t count;
    uint64_t start_time;
} monitor_context_t;

static monitor_context_t* g_mon_ctx = NULL;

static uint64_t get_timestamp_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000ULL + ts.tv_nsec / 1000;
}

static int monitor_init(void) {
    if (g_mon_ctx != NULL) {
        return -1;
    }
    
    g_mon_ctx = calloc(1, sizeof(monitor_context_t));
    if (g_mon_ctx == NULL) {
        return -1;
    }
    
    g_mon_ctx->start_time = get_timestamp_us();
    return 0;
}

static void monitor_record(const char* metric, double value) {
    if (g_mon_ctx == NULL || metric == NULL) {
        return;
    }
    
    metric_entry_t* entry = calloc(1, sizeof(metric_entry_t));
    if (entry == NULL) {
        return;
    }
    
    strncpy(entry->name, metric, sizeof(entry->name) - 1);
    entry->value = value;
    entry->timestamp = get_timestamp_us();
    
    // Add to linked list
    entry->next = g_mon_ctx->metrics;
    g_mon_ctx->metrics = entry;
    g_mon_ctx->count++;
}

static void monitor_report(metrics_t* metrics) {
    if (g_mon_ctx == NULL || metrics == NULL) {
        return;
    }
    
    // Calculate aggregate metrics
    uint64_t total_latency = 0;
    uint64_t latency_count = 0;
    
    metric_entry_t* entry = g_mon_ctx->metrics;
    while (entry != NULL) {
        if (strstr(entry->name, "latency") != NULL) {
            total_latency += (uint64_t)entry->value;
            latency_count++;
        }
        entry = entry->next;
    }
    
    if (latency_count > 0) {
        metrics->avg_latency_us = (double)total_latency / latency_count;
    }
}

static void monitor_cleanup(void) {
    if (g_mon_ctx != NULL) {
        // Free all metric entries
        metric_entry_t* entry = g_mon_ctx->metrics;
        while (entry != NULL) {
            metric_entry_t* next = entry->next;
            free(entry);
            entry = next;
        }
        
        free(g_mon_ctx);
        g_mon_ctx = NULL;
    }
}

// Export monitor interface
static monitor_interface_t t8_monitor = {
    .init = monitor_init,
    .record = monitor_record,
    .report = monitor_report,
    .cleanup = monitor_cleanup
};

monitor_interface_t* t8_get_monitor(void) {
    return &t8_monitor;
}