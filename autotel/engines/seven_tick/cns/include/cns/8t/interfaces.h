#ifndef CNS_8T_INTERFACES_H
#define CNS_8T_INTERFACES_H

#include "types.h"

// 8T Component Interfaces - 80/20 modular design

// Parser interface
typedef struct {
    int (*init)(void);
    int (*parse)(const char* input, size_t len, triple_t* output);
    void (*cleanup)(void);
} parser_interface_t;

// Cache interface
typedef struct {
    int (*init)(size_t capacity);
    void* (*get)(uint64_t key);
    int (*put)(uint64_t key, void* value);
    void (*clear)(void);
    void (*cleanup)(void);
} cache_interface_t;

// Scheduler interface
typedef struct {
    int (*init)(size_t max_tasks);
    int (*schedule)(task_t* task);
    int (*run)(void);
    void (*cleanup)(void);
} scheduler_interface_t;

// Materializer interface
typedef struct {
    int (*init)(void);
    int (*materialize)(graph_t* graph, const char* query);
    void (*cleanup)(void);
} materializer_interface_t;

// Monitor interface
typedef struct {
    int (*init)(void);
    void (*record)(const char* metric, double value);
    void (*report)(metrics_t* metrics);
    void (*cleanup)(void);
} monitor_interface_t;

// Register component interfaces
void t8_register_parser(parser_interface_t* parser);
void t8_register_cache(cache_interface_t* cache);
void t8_register_scheduler(scheduler_interface_t* scheduler);
void t8_register_materializer(materializer_interface_t* materializer);
void t8_register_monitor(monitor_interface_t* monitor);

#endif // CNS_8T_INTERFACES_H