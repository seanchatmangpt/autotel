#ifndef CNS_8T_SCHEDULER_H
#define CNS_8T_SCHEDULER_H

#include "cns/8t/core.h"
#include "cns/8t/processor.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// 8T TASK SCHEDULING - ADVANCED WORKLOAD MANAGEMENT
// ============================================================================

// Task priority levels
typedef enum {
    CNS_8T_PRIORITY_CRITICAL = 0,  // Must complete within 8 ticks
    CNS_8T_PRIORITY_HIGH = 1,      // High priority
    CNS_8T_PRIORITY_NORMAL = 2,    // Normal priority
    CNS_8T_PRIORITY_LOW = 3,       // Low priority
    CNS_8T_PRIORITY_BACKGROUND = 4 // Background processing
} cns_8t_priority_t;

// Task execution modes
typedef enum {
    CNS_8T_EXEC_SEQUENTIAL,     // Sequential execution
    CNS_8T_EXEC_PARALLEL,       // Parallel execution across cores
    CNS_8T_EXEC_PIPELINE,       // Pipeline execution
    CNS_8T_EXEC_SIMD,          // SIMD vectorized execution
    CNS_8T_EXEC_HYBRID         // Hybrid mode (auto-select)
} cns_8t_execution_mode_t;

// Task state
typedef enum {
    CNS_8T_TASK_CREATED,       // Task created but not scheduled
    CNS_8T_TASK_QUEUED,        // Task queued for execution
    CNS_8T_TASK_RUNNING,       // Task currently executing
    CNS_8T_TASK_COMPLETED,     // Task completed successfully
    CNS_8T_TASK_FAILED,        // Task failed
    CNS_8T_TASK_CANCELLED,     // Task was cancelled
    CNS_8T_TASK_SUSPENDED      // Task suspended (waiting for resources)
} cns_8t_task_state_t;

// Task descriptor
typedef struct {
    uint64_t task_id;           // Unique task identifier
    const char* name;           // Task name
    cns_8t_priority_t priority; // Task priority
    cns_8t_execution_mode_t exec_mode; // Execution mode
    cns_8t_task_state_t state;  // Current state
    
    // Function and data
    cns_8t_stage_fn_t function; // Task function
    void* input_data;           // Input data pointer
    void* output_data;          // Output data pointer
    size_t input_size;          // Input data size
    size_t output_size;         // Output data size
    
    // Resource requirements
    uint32_t cpu_cores;         // Number of CPU cores needed
    size_t memory_requirement;  // Memory requirement in bytes
    uint32_t numa_node;         // Preferred NUMA node
    cns_8t_precision_mode_t precision; // Required precision
    
    // Timing constraints
    cns_tick_t max_execution_ticks; // Maximum execution time
    cns_tick_t deadline_tick;   // Absolute deadline
    cns_tick_t submit_tick;     // When task was submitted
    cns_tick_t start_tick;      // When task started executing
    cns_tick_t end_tick;        // When task completed
    
    // Dependencies
    uint64_t* dependencies;     // Array of task IDs this depends on
    uint32_t dependency_count;  // Number of dependencies
    uint64_t* dependents;       // Array of task IDs that depend on this
    uint32_t dependent_count;   // Number of dependents
    
    // Error handling
    cns_8t_error_context_t* error; // Error information if failed
    uint32_t retry_count;       // Number of retries attempted
    uint32_t max_retries;       // Maximum retry attempts
} cns_8t_task_descriptor_t __attribute__((aligned(64)));

// Work queue for different priority levels
typedef struct {
    cns_8t_task_descriptor_t** tasks; // Array of task pointers
    uint32_t capacity;          // Maximum queue capacity
    uint32_t head;              // Queue head pointer
    uint32_t tail;              // Queue tail pointer
    uint32_t count;             // Current number of tasks
    uint32_t padding;           // Alignment padding
} cns_8t_work_queue_t __attribute__((aligned(64)));

// Worker thread context
typedef struct {
    uint32_t worker_id;         // Worker thread ID
    uint32_t cpu_affinity;      // CPU affinity mask
    uint32_t numa_node;         // NUMA node this worker is on
    cns_8t_task_state_t state;  // Worker state
    
    cns_8t_task_descriptor_t* current_task; // Currently executing task
    cns_8t_context_t* execution_context;    // Execution context
    cns_8t_processor_t* processor;          // Associated processor
    
    // Performance metrics
    cns_8t_perf_metrics_t metrics;
    uint64_t tasks_completed;   // Total tasks completed
    uint64_t tasks_failed;      // Total tasks failed
    cns_tick_t total_idle_time; // Total idle time
    cns_tick_t total_work_time; // Total work time
} cns_8t_worker_context_t __attribute__((aligned(64)));

// Scheduler configuration
typedef struct {
    uint32_t worker_count;      // Number of worker threads
    uint32_t max_queued_tasks;  // Maximum queued tasks per priority
    cns_8t_execution_mode_t default_mode; // Default execution mode
    
    // Load balancing configuration
    bool enable_work_stealing;  // Enable work stealing between workers
    bool enable_numa_awareness; // Enable NUMA-aware scheduling
    bool enable_priority_boost; // Enable priority boost for starved tasks
    
    // Performance tuning
    uint32_t context_switch_penalty; // Context switch penalty in ticks
    uint32_t cache_miss_penalty;     // Cache miss penalty in ticks
    uint32_t simd_preference;        // Preference for SIMD tasks
    
    // Resource limits
    size_t max_memory_per_task;      // Maximum memory per task
    uint32_t max_execution_ticks;    // Maximum execution time per task
    uint32_t scheduler_quantum;      // Time quantum for preemption
} cns_8t_scheduler_config_t;

// Main scheduler structure
struct cns_8t_scheduler {
    cns_8t_scheduler_config_t config;
    
    // Work queues for different priorities
    cns_8t_work_queue_t queues[5]; // One for each priority level
    
    // Worker management
    cns_8t_worker_context_t* workers;
    uint32_t active_workers;
    uint32_t idle_workers;
    
    // Task management
    cns_8t_task_descriptor_t* task_pool; // Pre-allocated task pool
    uint32_t task_pool_size;
    uint32_t next_task_id;
    
    // Synchronization (simplified for C interface)
    volatile uint32_t scheduler_lock;
    volatile uint32_t task_counter;
    volatile uint32_t shutdown_flag;
    
    // Performance tracking
    cns_8t_perf_metrics_t global_metrics;
    uint64_t total_tasks_processed;
    uint64_t total_scheduling_overhead;
    
    // Memory pool for task data
    cns_8t_memory_pool_t* memory_pool;
    
    // Algorithm assignment
    cns_8t_algorithm_descriptor_t* algorithms;
    uint32_t algorithm_count;
} __attribute__((aligned(64)));

// ============================================================================
// 8T LOAD BALANCING ALGORITHMS
// ============================================================================

// Load balancing strategies
typedef enum {
    CNS_8T_BALANCE_ROUND_ROBIN,  // Simple round-robin
    CNS_8T_BALANCE_LEAST_LOADED, // Assign to least loaded worker
    CNS_8T_BALANCE_NUMA_AWARE,   // NUMA-aware balancing
    CNS_8T_BALANCE_WORK_STEALING, // Work stealing between workers
    CNS_8T_BALANCE_ADAPTIVE      // Adaptive based on workload
} cns_8t_balance_strategy_t;

// Load balancing context
typedef struct {
    cns_8t_balance_strategy_t strategy;
    uint32_t rebalance_threshold; // When to trigger rebalancing
    uint32_t steal_threshold;     // Work stealing threshold
    uint32_t numa_penalty;        // Penalty for cross-NUMA assignments
    cns_tick_t last_balance_tick; // Last rebalancing time
} cns_8t_load_balancer_t;

// ============================================================================
// 8T SCHEDULER API
// ============================================================================

// Scheduler lifecycle
cns_8t_result_t cns_8t_scheduler_create(const cns_8t_scheduler_config_t* config,
                                         cns_8t_scheduler_t** scheduler);

cns_8t_result_t cns_8t_scheduler_destroy(cns_8t_scheduler_t* scheduler);

cns_8t_result_t cns_8t_scheduler_start(cns_8t_scheduler_t* scheduler);

cns_8t_result_t cns_8t_scheduler_stop(cns_8t_scheduler_t* scheduler);

// Task management
cns_8t_result_t cns_8t_scheduler_submit_task(cns_8t_scheduler_t* scheduler,
                                              const cns_8t_task_descriptor_t* task,
                                              uint64_t* task_id);

cns_8t_result_t cns_8t_scheduler_cancel_task(cns_8t_scheduler_t* scheduler,
                                              uint64_t task_id);

cns_8t_result_t cns_8t_scheduler_wait_task(cns_8t_scheduler_t* scheduler,
                                            uint64_t task_id,
                                            cns_tick_t timeout);

cns_8t_result_t cns_8t_scheduler_get_task_status(cns_8t_scheduler_t* scheduler,
                                                  uint64_t task_id,
                                                  cns_8t_task_state_t* state);

// Batch operations
cns_8t_result_t cns_8t_scheduler_submit_batch(cns_8t_scheduler_t* scheduler,
                                               const cns_8t_task_descriptor_t* tasks,
                                               uint32_t task_count,
                                               uint64_t* task_ids);

cns_8t_result_t cns_8t_scheduler_wait_batch(cns_8t_scheduler_t* scheduler,
                                             const uint64_t* task_ids,
                                             uint32_t task_count,
                                             cns_tick_t timeout);

// Algorithm integration
cns_8t_result_t cns_8t_scheduler_register_algorithm(cns_8t_scheduler_t* scheduler,
                                                     const cns_8t_algorithm_descriptor_t* algo);

cns_8t_result_t cns_8t_scheduler_execute_algorithm(cns_8t_scheduler_t* scheduler,
                                                    const char* algorithm_name,
                                                    const void* input,
                                                    void* output,
                                                    cns_8t_priority_t priority,
                                                    uint64_t* task_id);

// Performance monitoring
cns_8t_result_t cns_8t_scheduler_get_metrics(cns_8t_scheduler_t* scheduler,
                                              cns_8t_perf_metrics_t* metrics);

cns_8t_result_t cns_8t_scheduler_get_worker_metrics(cns_8t_scheduler_t* scheduler,
                                                     uint32_t worker_id,
                                                     cns_8t_perf_metrics_t* metrics);

cns_8t_result_t cns_8t_scheduler_reset_metrics(cns_8t_scheduler_t* scheduler);

// Load balancing control
cns_8t_result_t cns_8t_scheduler_set_balance_strategy(cns_8t_scheduler_t* scheduler,
                                                       cns_8t_balance_strategy_t strategy);

cns_8t_result_t cns_8t_scheduler_trigger_rebalance(cns_8t_scheduler_t* scheduler);

// Resource management
cns_8t_result_t cns_8t_scheduler_set_worker_affinity(cns_8t_scheduler_t* scheduler,
                                                      uint32_t worker_id,
                                                      uint32_t cpu_mask);

cns_8t_result_t cns_8t_scheduler_add_worker(cns_8t_scheduler_t* scheduler,
                                             const cns_8t_worker_context_t* worker_config);

cns_8t_result_t cns_8t_scheduler_remove_worker(cns_8t_scheduler_t* scheduler,
                                                uint32_t worker_id);

// ============================================================================
// 8T UTILITY FUNCTIONS
// ============================================================================

// Task creation helpers
cns_8t_result_t cns_8t_task_create_simple(const char* name,
                                           cns_8t_stage_fn_t function,
                                           void* input,
                                           void* output,
                                           size_t input_size,
                                           size_t output_size,
                                           cns_8t_task_descriptor_t* task);

// Dependency management
cns_8t_result_t cns_8t_task_add_dependency(cns_8t_task_descriptor_t* task,
                                            uint64_t dependency_task_id);

cns_8t_result_t cns_8t_task_remove_dependency(cns_8t_task_descriptor_t* task,
                                               uint64_t dependency_task_id);

// Performance utilities
double cns_8t_scheduler_get_utilization(cns_8t_scheduler_t* scheduler);
double cns_8t_scheduler_get_throughput(cns_8t_scheduler_t* scheduler);
cns_tick_t cns_8t_scheduler_get_average_latency(cns_8t_scheduler_t* scheduler);

#ifdef __cplusplus
}
#endif

#endif // CNS_8T_SCHEDULER_H