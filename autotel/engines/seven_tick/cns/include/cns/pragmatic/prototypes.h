#ifndef CNS_PROTOTYPES_H
#define CNS_PROTOTYPES_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

  // Prototype manager structure
  typedef struct cns_prototype_manager cns_prototype_manager_t;

  // Prototype types
  typedef enum
  {
    CNS_PROTOTYPE_PERFORMANCE,
    CNS_PROTOTYPE_ARCHITECTURE,
    CNS_PROTOTYPE_INTEGRATION,
    CNS_PROTOTYPE_WORKFLOW,
    CNS_PROTOTYPE_CUSTOM
  } cns_prototype_type_t;

  // Prototype status
  typedef enum
  {
    CNS_PROTOTYPE_PENDING,
    CNS_PROTOTYPE_RUNNING,
    CNS_PROTOTYPE_SUCCESS,
    CNS_PROTOTYPE_FAILED,
    CNS_PROTOTYPE_TIMEOUT
  } cns_prototype_status_t;

  // Prototype component function pointer
  typedef bool (*cns_prototype_component_func)(void *context);

  // Prototype component structure
  typedef struct
  {
    uint32_t component_id;
    const char *component_name;
    const char *description;
    cns_prototype_component_func component_func;
    void *context;
    cns_prototype_status_t status;
    uint64_t execution_time_ns;
    uint32_t execution_cycles;
    bool performance_compliant;
    const char *result_data;
  } cns_prototype_component_t;

  // Prototype structure
  typedef struct
  {
    uint32_t prototype_id;
    const char *prototype_name;
    const char *description;
    cns_prototype_type_t type;
    cns_prototype_component_t *components;
    uint32_t component_count;
    uint32_t max_components;
    cns_prototype_status_t status;
    uint64_t start_time;
    uint64_t end_time;
    uint64_t total_execution_time_ns;
    double performance_score;
    bool validated;
  } cns_prototype_t;

  // Prototype manager structure
  struct cns_prototype_manager
  {
    cns_prototype_t prototypes[32]; // Max 32 prototypes
    uint32_t prototype_count;
    uint32_t next_prototype_id;
    uint32_t next_component_id;
    bool enabled;
    uint64_t total_prototypes_executed;
    uint64_t successful_prototypes;
    uint64_t failed_prototypes;
    uint64_t total_execution_time_ns;
  };

// Constants
#define CNS_MAX_PROTOTYPES 32
#define CNS_MAX_PROTOTYPE_COMPONENTS 16
#define CNS_DEFAULT_PROTOTYPE_TIMEOUT_MS 5000        // 5 seconds
#define CNS_PROTOTYPE_PERFORMANCE_THRESHOLD_NS 10000 // 10ns for 7-tick compliance

  // Function declarations

  // Manager lifecycle
  cns_prototype_manager_t *cns_prototype_init(void);
  void cns_prototype_cleanup(cns_prototype_manager_t *manager);

  // Prototype management
  uint32_t cns_prototype_create(cns_prototype_manager_t *manager,
                                const char *prototype_name,
                                const char *description,
                                cns_prototype_type_t type);
  bool cns_prototype_destroy(cns_prototype_manager_t *manager, uint32_t prototype_id);

  // Component management
  uint32_t cns_prototype_add_component(cns_prototype_manager_t *manager,
                                       uint32_t prototype_id,
                                       const char *component_name,
                                       const char *description,
                                       cns_prototype_component_func component_func,
                                       void *context);
  bool cns_prototype_remove_component(cns_prototype_manager_t *manager, uint32_t prototype_id, uint32_t component_id);

  // Prototype execution
  bool cns_prototype_execute(cns_prototype_manager_t *manager, uint32_t prototype_id);
  bool cns_prototype_execute_component(cns_prototype_manager_t *manager, uint32_t prototype_id, uint32_t component_id);
  bool cns_prototype_execute_all(cns_prototype_manager_t *manager);

  // Performance validation
  double cns_prototype_get_performance_score(cns_prototype_manager_t *manager, uint32_t prototype_id);
  bool cns_prototype_validate_performance(cns_prototype_manager_t *manager, uint32_t prototype_id);
  bool cns_prototype_validate_7_tick_compliance(cns_prototype_manager_t *manager);

  // Status and results
  cns_prototype_status_t cns_prototype_get_status(cns_prototype_manager_t *manager, uint32_t prototype_id);
  cns_prototype_component_t *cns_prototype_get_component(cns_prototype_manager_t *manager, uint32_t prototype_id, uint32_t component_id);
  bool cns_prototype_get_components(cns_prototype_manager_t *manager, uint32_t prototype_id,
                                    cns_prototype_component_t **components, uint32_t *component_count);

  // Statistics
  uint64_t cns_prototype_get_total_executed(cns_prototype_manager_t *manager);
  uint64_t cns_prototype_get_successful_count(cns_prototype_manager_t *manager);
  uint64_t cns_prototype_get_failed_count(cns_prototype_manager_t *manager);
  double cns_prototype_get_success_rate(cns_prototype_manager_t *manager);
  uint64_t cns_prototype_get_total_execution_time_ns(cns_prototype_manager_t *manager);

  // Configuration
  bool cns_prototype_set_enabled(cns_prototype_manager_t *manager, bool enabled);
  bool cns_prototype_is_enabled(cns_prototype_manager_t *manager);

  // Built-in prototype components
  bool cns_prototype_component_memory_layout(void *context);
  bool cns_prototype_component_cache_optimization(void *context);
  bool cns_prototype_component_branch_free_logic(void *context);
  bool cns_prototype_component_string_interning(void *context);
  bool cns_prototype_component_hash_join(void *context);
  bool cns_prototype_component_static_planning(void *context);
  bool cns_prototype_component_memory_pooling(void *context);
  bool cns_prototype_component_telemetry_integration(void *context);

  // Performance validation
  void cns_prototype_validate_performance_comprehensive(cns_prototype_manager_t *manager);

#ifdef __cplusplus
}
#endif

#endif // CNS_PROTOTYPES_H