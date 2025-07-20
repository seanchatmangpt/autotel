#ifndef CNS_8T_TYPES_H
#define CNS_8T_TYPES_H

#include <stdint.h>
#include <stdbool.h>

// 8T Core Types - 80/20 approach focuses on essential types only

// Tick represents a single processing unit
typedef struct {
    uint32_t id;
    uint32_t type;
    void* data;
    size_t size;
} tick_t;

// Parser state machine states
typedef enum {
    STATE_INIT = 0,
    STATE_SUBJECT,
    STATE_PREDICATE,
    STATE_OBJECT,
    STATE_COMPLETE,
    STATE_ERROR
} parser_state_t;

// Triple representation for RDF data
typedef struct {
    uint32_t subject;
    uint32_t predicate;
    uint32_t object;
} triple_t;

// Cache entry for hot path optimization
typedef struct {
    uint64_t key;
    void* value;
    uint32_t access_count;
    uint32_t last_access;
} cache_entry_t;

// Basic graph structure
typedef struct {
    triple_t* triples;
    size_t count;
    size_t capacity;
} graph_t;

// Scheduler task
typedef struct {
    uint32_t id;
    void (*handler)(void*);
    void* context;
    uint8_t priority;
} task_t;

// Performance metrics
typedef struct {
    uint64_t ticks_processed;
    uint64_t cache_hits;
    uint64_t cache_misses;
    double avg_latency_us;
} metrics_t;

#endif // CNS_8T_TYPES_H