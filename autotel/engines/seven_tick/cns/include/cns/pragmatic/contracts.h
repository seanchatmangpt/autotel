#ifndef CNS_CONTRACTS_H
#define CNS_CONTRACTS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

  // Contract manager structure
  typedef struct cns_contract_manager cns_contract_manager_t;

  // Contract types
  typedef enum
  {
    CNS_CONTRACT_PRECONDITION,
    CNS_CONTRACT_POSTCONDITION,
    CNS_CONTRACT_INVARIANT,
    CNS_CONTRACT_ASSERTION,
    CNS_CONTRACT_PERFORMANCE,
    CNS_CONTRACT_PHYSICS,
    CNS_CONTRACT_CUSTOM
  } cns_contract_type_t;

  // Contract status
  typedef enum
  {
    CNS_CONTRACT_PENDING,
    CNS_CONTRACT_VALIDATING,
    CNS_CONTRACT_PASSED,
    CNS_CONTRACT_FAILED,
    CNS_CONTRACT_VIOLATED,
    CNS_CONTRACT_TIMEOUT
  } cns_contract_status_t;

  // Contract function pointer
  typedef bool (*cns_contract_func)(void *context);

  // Contract violation severity
  typedef enum
  {
    CNS_CONTRACT_VIOLATION_WARNING,
    CNS_CONTRACT_VIOLATION_ERROR,
    CNS_CONTRACT_VIOLATION_CRITICAL,
    CNS_CONTRACT_VIOLATION_FATAL
  } cns_contract_violation_severity_t;

  // Contract structure
  typedef struct
  {
    uint32_t contract_id;
    const char *contract_name;
    const char *description;
    cns_contract_type_t type;
    cns_contract_func contract_func;
    void *context;
    cns_contract_status_t status;
    uint64_t validation_time;
    uint32_t validation_cycles;
    bool performance_compliant;
    const char *violation_message;
    cns_contract_violation_severity_t violation_severity;
    uint32_t violation_count;
    uint32_t total_validations;
    uint64_t last_violation_time;
  } cns_contract_t;

  // Contract group structure
  typedef struct
  {
    uint32_t group_id;
    const char *group_name;
    const char *description;
    cns_contract_t *contracts;
    uint32_t contract_count;
    uint32_t max_contracts;
    bool enabled;
    uint64_t total_validations;
    uint64_t passed_validations;
    uint64_t failed_validations;
    uint64_t total_validation_time_ns;
  } cns_contract_group_t;

  // Contract manager structure
  struct cns_contract_manager
  {
    cns_contract_group_t groups[32]; // Max 32 contract groups
    uint32_t group_count;
    uint32_t next_group_id;
    uint32_t next_contract_id;
    bool enabled;
    uint64_t total_contracts_validated;
    uint64_t total_contracts_passed;
    uint64_t total_contracts_failed;
    uint64_t total_validation_time_ns;
    cns_contract_violation_severity_t min_severity_level;
  };

// Constants
#define CNS_MAX_CONTRACTS_PER_GROUP 64
#define CNS_MAX_CONTRACT_GROUPS 32
#define CNS_DEFAULT_CONTRACT_TIMEOUT_MS 1000       // 1 second
#define CNS_CONTRACT_PERFORMANCE_THRESHOLD_NS 1000 // 1μs for contract validation

  // Function declarations

  // Manager lifecycle
  cns_contract_manager_t *cns_contracts_init(void);
  void cns_contracts_cleanup(cns_contract_manager_t *manager);

  // Contract group management
  uint32_t cns_contracts_create_group(cns_contract_manager_t *manager,
                                      const char *group_name,
                                      const char *description);
  bool cns_contracts_destroy_group(cns_contract_manager_t *manager, uint32_t group_id);
  bool cns_contracts_enable_group(cns_contract_manager_t *manager, uint32_t group_id, bool enabled);

  // Contract management
  uint32_t cns_contracts_add_contract(cns_contract_manager_t *manager,
                                      uint32_t group_id,
                                      const char *contract_name,
                                      const char *description,
                                      cns_contract_type_t type,
                                      cns_contract_func contract_func,
                                      void *context);
  bool cns_contracts_remove_contract(cns_contract_manager_t *manager, uint32_t group_id, uint32_t contract_id);

  // Contract validation
  bool cns_contracts_validate_contract(cns_contract_manager_t *manager, uint32_t group_id, uint32_t contract_id);
  bool cns_contracts_validate_group(cns_contract_manager_t *manager, uint32_t group_id);
  bool cns_contracts_validate_all_contracts(cns_contract_manager_t *manager);
  bool cns_contracts_validate_contracts_by_type(cns_contract_manager_t *manager, cns_contract_type_t type);

  // Contract status and results
  cns_contract_status_t cns_contracts_get_contract_status(cns_contract_manager_t *manager,
                                                          uint32_t group_id, uint32_t contract_id);
  cns_contract_t *cns_contracts_get_contract(cns_contract_manager_t *manager,
                                             uint32_t group_id, uint32_t contract_id);
  bool cns_contracts_get_group_contracts(cns_contract_manager_t *manager, uint32_t group_id,
                                         cns_contract_t **contracts, uint32_t *contract_count);

  // Performance validation
  bool cns_contracts_validate_performance(cns_contract_manager_t *manager, uint32_t group_id, uint32_t contract_id);
  bool cns_contracts_validate_7_tick_compliance(cns_contract_manager_t *manager);
  bool cns_contracts_validate_physics_compliance(cns_contract_manager_t *manager);

  // Statistics
  uint64_t cns_contracts_get_total_contracts_validated(cns_contract_manager_t *manager);
  uint64_t cns_contracts_get_total_contracts_passed(cns_contract_manager_t *manager);
  uint64_t cns_contracts_get_total_contracts_failed(cns_contract_manager_t *manager);
  double cns_contracts_get_success_rate(cns_contract_manager_t *manager);
  uint64_t cns_contracts_get_total_validation_time_ns(cns_contract_manager_t *manager);
  double cns_contracts_get_average_validation_time_ns(cns_contract_manager_t *manager);

  // Configuration
  bool cns_contracts_set_enabled(cns_contract_manager_t *manager, bool enabled);
  bool cns_contracts_is_enabled(cns_contract_manager_t *manager);
  bool cns_contracts_set_min_severity_level(cns_contract_manager_t *manager, cns_contract_violation_severity_t level);
  cns_contract_violation_severity_t cns_contracts_get_min_severity_level(cns_contract_manager_t *manager);

  // Built-in contract functions
  bool cns_contracts_precondition_not_null(void *context);
  bool cns_contracts_precondition_valid_range(void *context);
  bool cns_contracts_precondition_valid_enum(void *context);
  bool cns_contracts_precondition_valid_size(void *context);
  bool cns_contracts_precondition_valid_pointer(void *context);

  bool cns_contracts_postcondition_not_null(void *context);
  bool cns_contracts_postcondition_valid_range(void *context);
  bool cns_contracts_postcondition_valid_enum(void *context);
  bool cns_contracts_postcondition_valid_size(void *context);
  bool cns_contracts_postcondition_valid_pointer(void *context);

  bool cns_contracts_invariant_consistent_state(void *context);
  bool cns_contracts_invariant_valid_memory(void *context);
  bool cns_contracts_invariant_valid_relationships(void *context);
  bool cns_contracts_invariant_valid_constraints(void *context);

  bool cns_contracts_assertion_true(void *context);
  bool cns_contracts_assertion_false(void *context);
  bool cns_contracts_assertion_equal(void *context);
  bool cns_contracts_assertion_not_equal(void *context);
  bool cns_contracts_assertion_greater_than(void *context);
  bool cns_contracts_assertion_less_than(void *context);

  bool cns_contracts_performance_7_tick_compliance(void *context);
  bool cns_contracts_performance_memory_usage(void *context);
  bool cns_contracts_performance_cache_efficiency(void *context);
  bool cns_contracts_performance_throughput(void *context);

  bool cns_contracts_physics_speed_of_light(void *context);
  bool cns_contracts_physics_cache_hierarchy(void *context);
  bool cns_contracts_physics_memory_bandwidth(void *context);
  bool cns_contracts_physics_energy_efficiency(void *context);

// Contract macros for easy use
#define CNS_CONTRACT_PRE(condition)                                        \
  do                                                                       \
  {                                                                        \
    if (!(condition))                                                      \
    {                                                                      \
      otel_span_set_attribute(span, "contract.violation", "precondition"); \
      otel_span_set_attribute(span, "contract.condition", #condition);     \
      return CNS_CONTRACT_VIOLATION_ERROR;                                 \
    }                                                                      \
  } while (0)

#define CNS_CONTRACT_POST(condition)                                        \
  do                                                                        \
  {                                                                         \
    if (!(condition))                                                       \
    {                                                                       \
      otel_span_set_attribute(span, "contract.violation", "postcondition"); \
      otel_span_set_attribute(span, "contract.condition", #condition);      \
      return CNS_CONTRACT_VIOLATION_ERROR;                                  \
    }                                                                       \
  } while (0)

#define CNS_CONTRACT_INV(condition)                                     \
  do                                                                    \
  {                                                                     \
    if (!(condition))                                                   \
    {                                                                   \
      otel_span_set_attribute(span, "contract.violation", "invariant"); \
      otel_span_set_attribute(span, "contract.condition", #condition);  \
      return CNS_CONTRACT_VIOLATION_ERROR;                              \
    }                                                                   \
  } while (0)

#define CNS_CONTRACT_ASSERT(condition)                                  \
  do                                                                    \
  {                                                                     \
    if (!(condition))                                                   \
    {                                                                   \
      otel_span_set_attribute(span, "contract.violation", "assertion"); \
      otel_span_set_attribute(span, "contract.condition", #condition);  \
      return CNS_CONTRACT_VIOLATION_ERROR;                              \
    }                                                                   \
  } while (0)

#define CNS_CONTRACT_PERF(operation, max_cycles)                          \
  do                                                                      \
  {                                                                       \
    uint64_t start = s7t_cycles();                                        \
    operation;                                                            \
    uint64_t end = s7t_cycles();                                          \
    uint32_t cycles = (uint32_t)(end - start);                            \
    if (cycles > max_cycles)                                              \
    {                                                                     \
      otel_span_set_attribute(span, "contract.violation", "performance"); \
      otel_span_set_attribute(span, "contract.cycles_used", cycles);      \
      otel_span_set_attribute(span, "contract.max_cycles", max_cycles);   \
      return CNS_CONTRACT_VIOLATION_ERROR;                                \
    }                                                                     \
  } while (0)

  // Contract utilities
  void cns_contracts_log_violation(cns_contract_manager_t *manager,
                                   uint32_t group_id, uint32_t contract_id,
                                   const char *violation_message,
                                   cns_contract_violation_severity_t severity);
  bool cns_contracts_handle_violation(cns_contract_manager_t *manager,
                                      uint32_t group_id, uint32_t contract_id,
                                      cns_contract_violation_severity_t severity);

  // Performance validation
  void cns_contracts_validate_performance_comprehensive(cns_contract_manager_t *manager);

#ifdef __cplusplus
}
#endif

#endif // CNS_CONTRACTS_H