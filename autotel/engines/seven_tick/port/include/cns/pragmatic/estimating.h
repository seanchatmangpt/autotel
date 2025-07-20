#ifndef CNS_ESTIMATING_H
#define CNS_ESTIMATING_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

  // Estimating manager structure
  typedef struct cns_estimating_manager cns_estimating_manager_t;

  // Estimate types
  typedef enum
  {
    CNS_ESTIMATE_PERFORMANCE,
    CNS_ESTIMATE_MEMORY,
    CNS_ESTIMATE_DEVELOPMENT_TIME,
    CNS_ESTIMATE_COMPLEXITY,
    CNS_ESTIMATE_RISK,
    CNS_ESTIMATE_CUSTOM
  } cns_estimate_type_t;

  // Risk levels
  typedef enum
  {
    CNS_RISK_LOW,
    CNS_RISK_MEDIUM,
    CNS_RISK_HIGH,
    CNS_RISK_CRITICAL
  } cns_risk_level_t;

  // Performance estimate structure
  typedef struct
  {
    uint32_t estimate_id;
    const char *component_name;
    const char *description;
    cns_estimate_type_t type;
    uint32_t estimated_cycles;
    uint32_t estimated_memory_bytes;
    double confidence_level;
    uint32_t historical_samples;
    cns_risk_level_t risk_level;
    uint64_t creation_time;
    bool validated;
    uint32_t actual_cycles;
    uint32_t actual_memory_bytes;
    double accuracy_percentage;
  } cns_performance_estimate_t;

  // Historical data structure
  typedef struct
  {
    uint32_t sample_id;
    const char *component_name;
    uint32_t cycles_measured;
    uint32_t memory_used;
    uint64_t timestamp;
    bool success;
  } cns_historical_sample_t;

  // Estimating manager structure
  struct cns_estimating_manager
  {
    cns_performance_estimate_t estimates[128]; // Max 128 estimates
    uint32_t estimate_count;
    uint32_t next_estimate_id;
    cns_historical_sample_t historical_data[1024]; // Max 1024 historical samples
    uint32_t historical_count;
    uint32_t next_sample_id;
    bool enabled;
    uint64_t total_estimates_made;
    uint64_t accurate_estimates;
    uint64_t inaccurate_estimates;
    double overall_accuracy;
  };

// Constants
#define CNS_MAX_ESTIMATES 128
#define CNS_MAX_HISTORICAL_SAMPLES 1024
#define CNS_DEFAULT_CONFIDENCE_THRESHOLD 0.8
#define CNS_ACCURACY_THRESHOLD_PERCENT 20.0

  // Function declarations

  // Manager lifecycle
  cns_estimating_manager_t *cns_estimating_init(void);
  void cns_estimating_cleanup(cns_estimating_manager_t *manager);

  // Estimate management
  uint32_t cns_estimating_create_estimate(cns_estimating_manager_t *manager,
                                          const char *component_name,
                                          const char *description,
                                          cns_estimate_type_t type);
  bool cns_estimating_destroy_estimate(cns_estimating_manager_t *manager, uint32_t estimate_id);

  // Performance estimation
  cns_performance_estimate_t cns_estimating_estimate_performance(cns_estimating_manager_t *manager, const char *component_name);
  cns_performance_estimate_t cns_estimating_estimate_memory_usage(cns_estimating_manager_t *manager, const char *component_name);
  cns_performance_estimate_t cns_estimating_estimate_development_time(cns_estimating_manager_t *manager, const char *component_name);

  // Risk assessment
  cns_risk_level_t cns_estimating_assess_risk(cns_estimating_manager_t *manager, cns_performance_estimate_t *estimate);
  cns_risk_level_t cns_estimating_assess_performance_risk(cns_estimating_manager_t *manager, uint32_t estimated_cycles, double confidence);
  cns_risk_level_t cns_estimating_assess_memory_risk(cns_estimating_manager_t *manager, uint32_t estimated_memory, double confidence);

  // Historical data management
  uint32_t cns_estimating_add_historical_sample(cns_estimating_manager_t *manager,
                                                const char *component_name,
                                                uint32_t cycles_measured,
                                                uint32_t memory_used,
                                                bool success);
  bool cns_estimating_get_historical_data(cns_estimating_manager_t *manager, const char *component_name,
                                          cns_historical_sample_t **samples, uint32_t *sample_count);

  // Validation and accuracy
  bool cns_estimating_validate_estimate(cns_estimating_manager_t *manager, uint32_t estimate_id, uint32_t actual_cycles, uint32_t actual_memory);
  double cns_estimating_calculate_accuracy(cns_estimating_manager_t *manager, uint32_t estimate_id);
  bool cns_estimating_is_estimate_accurate(cns_estimating_manager_t *manager, uint32_t estimate_id);

  // Statistics
  uint64_t cns_estimating_get_total_estimates(cns_estimating_manager_t *manager);
  uint64_t cns_estimating_get_accurate_estimates(cns_estimating_manager_t *manager);
  uint64_t cns_estimating_get_inaccurate_estimates(cns_estimating_manager_t *manager);
  double cns_estimating_get_overall_accuracy(cns_estimating_manager_t *manager);
  uint32_t cns_estimating_get_historical_sample_count(cns_estimating_manager_t *manager);

  // Configuration
  bool cns_estimating_set_enabled(cns_estimating_manager_t *manager, bool enabled);
  bool cns_estimating_is_enabled(cns_estimating_manager_t *manager);
  bool cns_estimating_set_confidence_threshold(cns_estimating_manager_t *manager, double threshold);
  double cns_estimating_get_confidence_threshold(cns_estimating_manager_t *manager);

  // Built-in estimation functions
  cns_performance_estimate_t cns_estimating_strategy_pattern_performance(void);
  cns_performance_estimate_t cns_estimating_factory_pattern_performance(void);
  cns_performance_estimate_t cns_estimating_singleton_pattern_performance(void);
  cns_performance_estimate_t cns_estimating_decorator_pattern_performance(void);
  cns_performance_estimate_t cns_estimating_observer_pattern_performance(void);
  cns_performance_estimate_t cns_estimating_command_pattern_performance(void);

  cns_performance_estimate_t cns_estimating_memory_allocation_performance(void);
  cns_performance_estimate_t cns_estimating_string_operation_performance(void);
  cns_performance_estimate_t cns_estimating_telemetry_operation_performance(void);
  cns_performance_estimate_t cns_estimating_contract_validation_performance(void);

  // Performance validation
  void cns_estimating_validate_performance_comprehensive(cns_estimating_manager_t *manager);

#ifdef __cplusplus
}
#endif

#endif // CNS_ESTIMATING_H