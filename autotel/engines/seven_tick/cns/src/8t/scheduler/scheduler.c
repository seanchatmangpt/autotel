#include "cns/8t/interfaces.h"
#include <stdlib.h>
#include <pthread.h>

// 8T Scheduler Implementation - 80/20 work-stealing scheduler

typedef struct work_queue {
    task_t* tasks;
    size_t head;
    size_t tail;
    size_t capacity;
    pthread_mutex_t lock;
} work_queue_t;

typedef struct {
    work_queue_t* queues;
    size_t num_threads;
    pthread_t* threads;
    int running;
} scheduler_context_t;

static scheduler_context_t* g_sched_ctx = NULL;

static int scheduler_init(size_t max_tasks) {
    if (g_sched_ctx != NULL) {
        return -1;
    }
    
    g_sched_ctx = calloc(1, sizeof(scheduler_context_t));
    if (g_sched_ctx == NULL) {
        return -1;
    }
    
    // 80/20: Simple fixed thread pool
    g_sched_ctx->num_threads = 4; // TODO: Make configurable
    g_sched_ctx->queues = calloc(g_sched_ctx->num_threads, sizeof(work_queue_t));
    
    if (g_sched_ctx->queues == NULL) {
        free(g_sched_ctx);
        g_sched_ctx = NULL;
        return -1;
    }
    
    // Initialize work queues
    for (size_t i = 0; i < g_sched_ctx->num_threads; i++) {
        g_sched_ctx->queues[i].capacity = max_tasks / g_sched_ctx->num_threads;
        g_sched_ctx->queues[i].tasks = calloc(g_sched_ctx->queues[i].capacity, sizeof(task_t));
        pthread_mutex_init(&g_sched_ctx->queues[i].lock, NULL);
    }
    
    return 0;
}

static int scheduler_schedule(task_t* task) {
    if (g_sched_ctx == NULL || task == NULL) {
        return -1;
    }
    
    // 80/20: Simple round-robin scheduling
    static size_t next_queue = 0;
    work_queue_t* queue = &g_sched_ctx->queues[next_queue];
    
    pthread_mutex_lock(&queue->lock);
    
    if ((queue->tail + 1) % queue->capacity == queue->head) {
        // Queue full
        pthread_mutex_unlock(&queue->lock);
        return -1;
    }
    
    queue->tasks[queue->tail] = *task;
    queue->tail = (queue->tail + 1) % queue->capacity;
    
    pthread_mutex_unlock(&queue->lock);
    
    next_queue = (next_queue + 1) % g_sched_ctx->num_threads;
    return 0;
}

static void scheduler_cleanup(void) {
    if (g_sched_ctx != NULL) {
        for (size_t i = 0; i < g_sched_ctx->num_threads; i++) {
            pthread_mutex_destroy(&g_sched_ctx->queues[i].lock);
            free(g_sched_ctx->queues[i].tasks);
        }
        free(g_sched_ctx->queues);
        free(g_sched_ctx);
        g_sched_ctx = NULL;
    }
}

// Export scheduler interface
static scheduler_interface_t t8_scheduler = {
    .init = scheduler_init,
    .schedule = scheduler_schedule,
    .run = NULL, // TODO: Implement worker threads
    .cleanup = scheduler_cleanup
};

scheduler_interface_t* t8_get_scheduler(void) {
    return &t8_scheduler;
}