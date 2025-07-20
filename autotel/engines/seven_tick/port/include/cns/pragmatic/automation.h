#ifndef CNS_AUTOMATION_H
#define CNS_AUTOMATION_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

  // Automation manager structure
  typedef struct cns_automation_manager cns_automation_manager_t;

  // Automation task types
  typedef enum
  {
    CNS_AUTOMATION_COMPILE,
    CNS_AUTOMATION_TEST,
    CNS_AUTOMATION_DEPLOY,
    CNS_AUTOMATION_MONITOR,
    CNS_AUTOMATION_VALIDATE,
    CNS_AUTOMATION_BENCHMARK,
    CNS_AUTOMATION_CLEANUP,
    CNS_AUTOMATION_CUSTOM
  } cns_automation_task_type_t;

  // Automation task status
  typedef enum
  {
    CNS_AUTOMATION_PENDING,
    CNS_AUTOMATION_RUNNING,
    CNS_AUTOMATION_SUCCESS,
    CNS_AUTOMATION_FAILED,
    CNS_AUTOMATION_TIMEOUT
  } cns_automation_task_status_t;

  // Automation task function pointer
  typedef bool (*cns_automation_task_func)(void *context);

  // Automation task structure
  typedef struct
  {
    uint32_t task_id;
    const char *task_name;
    const char *description;
    cns_automation_task_type_t type;
    cns_automation_task_func task_func;
    void *context;
    uint32_t timeout_ms;
    cns_automation_task_status_t status;
    uint64_t start_time;
    uint64_t end_time;
    uint32_t retry_count;
    uint32_t max_retries;
  } cns_automation_task_t;

  // Automation pipeline structure
  typedef struct
  {
    uint32_t pipeline_id;
    const char *pipeline_name;
    const char *description;
    cns_automation_task_t *tasks;
    uint32_t task_count;
    uint32_t max_tasks;
    bool enabled;
    uint64_t total_executions;
    uint64_t successful_executions;
    uint64_t failed_executions;
  } cns_automation_pipeline_t;

  // Automation manager structure
  struct cns_automation_manager
  {
    cns_automation_pipeline_t pipelines[32]; // Max 32 pipelines
    uint32_t pipeline_count;
    uint32_t next_pipeline_id;
    uint32_t next_task_id;
    bool enabled;
    uint64_t total_tasks_executed;
    uint64_t total_tasks_succeeded;
    uint64_t total_tasks_failed;
  };

// Constants
#define CNS_MAX_AUTOMATION_TASKS 64
#define CNS_MAX_AUTOMATION_PIPELINES 32
#define CNS_DEFAULT_TIMEOUT_MS 30000 // 30 seconds
#define CNS_DEFAULT_MAX_RETRIES 3

  // Function declarations

  // Manager lifecycle
  cns_automation_manager_t *cns_automation_init(void);
  void cns_automation_cleanup(cns_automation_manager_t *manager);

  // Pipeline management
  uint32_t cns_automation_create_pipeline(cns_automation_manager_t *manager,
                                          const char *pipeline_name,
                                          const char *description);
  bool cns_automation_destroy_pipeline(cns_automation_manager_t *manager, uint32_t pipeline_id);
  bool cns_automation_enable_pipeline(cns_automation_manager_t *manager, uint32_t pipeline_id, bool enabled);

  // Task management
  uint32_t cns_automation_add_task(cns_automation_manager_t *manager,
                                   uint32_t pipeline_id,
                                   const char *task_name,
                                   const char *description,
                                   cns_automation_task_type_t type,
                                   cns_automation_task_func task_func,
                                   void *context,
                                   uint32_t timeout_ms,
                                   uint32_t max_retries);
  bool cns_automation_remove_task(cns_automation_manager_t *manager, uint32_t pipeline_id, uint32_t task_id);

  // Pipeline execution
  bool cns_automation_execute_pipeline(cns_automation_manager_t *manager, uint32_t pipeline_id);
  bool cns_automation_execute_pipeline_async(cns_automation_manager_t *manager, uint32_t pipeline_id);
  bool cns_automation_wait_for_pipeline(cns_automation_manager_t *manager, uint32_t pipeline_id, uint32_t timeout_ms);

  // Task execution
  bool cns_automation_execute_task(cns_automation_manager_t *manager, uint32_t pipeline_id, uint32_t task_id);
  bool cns_automation_retry_task(cns_automation_manager_t *manager, uint32_t pipeline_id, uint32_t task_id);

  // Status and monitoring
  cns_automation_task_status_t cns_automation_get_task_status(cns_automation_manager_t *manager,
                                                              uint32_t pipeline_id, uint32_t task_id);
  bool cns_automation_get_pipeline_status(cns_automation_manager_t *manager, uint32_t pipeline_id,
                                          cns_automation_task_status_t *status_array, uint32_t *status_count);
  uint32_t cns_automation_get_pipeline_task_count(cns_automation_manager_t *manager, uint32_t pipeline_id);
  uint64_t cns_automation_get_pipeline_execution_time(cns_automation_manager_t *manager, uint32_t pipeline_id);

  // Statistics
  uint64_t cns_automation_get_total_tasks_executed(cns_automation_manager_t *manager);
  uint64_t cns_automation_get_total_tasks_succeeded(cns_automation_manager_t *manager);
  uint64_t cns_automation_get_total_tasks_failed(cns_automation_manager_t *manager);
  double cns_automation_get_success_rate(cns_automation_manager_t *manager);

  // Configuration
  bool cns_automation_set_enabled(cns_automation_manager_t *manager, bool enabled);
  bool cns_automation_is_enabled(cns_automation_manager_t *manager);
  bool cns_automation_set_default_timeout(cns_automation_manager_t *manager, uint32_t timeout_ms);
  bool cns_automation_set_default_max_retries(cns_automation_manager_t *manager, uint32_t max_retries);

  // Built-in automation tasks
  bool cns_automation_compile_with_optimization(void *context);
  bool cns_automation_run_unit_tests(void *context);
  bool cns_automation_run_performance_tests(void *context);
  bool cns_automation_deploy_with_validation(void *context);
  bool cns_automation_monitor_performance(void *context);
  bool cns_automation_validate_7_tick_compliance(void *context);
  bool cns_automation_run_benchmarks(void *context);
  bool cns_automation_cleanup_build_artifacts(void *context);

  // Performance validation
  void cns_automation_validate_performance(cns_automation_manager_t *manager);

#ifdef __cplusplus
}
#endif

#endif // CNS_AUTOMATION_H