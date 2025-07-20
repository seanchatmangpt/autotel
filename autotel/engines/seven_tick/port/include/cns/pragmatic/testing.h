#ifndef CNS_TESTING_H
#define CNS_TESTING_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

  // Testing manager structure
  typedef struct cns_testing_manager cns_testing_manager_t;

  // Test types
  typedef enum
  {
    CNS_TEST_UNIT,
    CNS_TEST_INTEGRATION,
    CNS_TEST_PERFORMANCE,
    CNS_TEST_CONTRACT,
    CNS_TEST_PHYSICS,
    CNS_TEST_TELEMETRY,
    CNS_TEST_BENCHMARK,
    CNS_TEST_CUSTOM
  } cns_test_type_t;

  // Test status
  typedef enum
  {
    CNS_TEST_PENDING,
    CNS_TEST_RUNNING,
    CNS_TEST_PASSED,
    CNS_TEST_FAILED,
    CNS_TEST_SKIPPED,
    CNS_TEST_TIMEOUT
  } cns_test_status_t;

  // Test function pointer
  typedef bool (*cns_test_func)(void *context);

  // Test result structure
  typedef struct
  {
    uint32_t test_id;
    const char *test_name;
    const char *description;
    cns_test_type_t type;
    cns_test_func test_func;
    void *context;
    cns_test_status_t status;
    uint64_t start_time;
    uint64_t end_time;
    uint32_t execution_time_ns;
    uint32_t cycles_used;
    bool performance_compliant;
    const char *failure_reason;
    uint32_t retry_count;
    uint32_t max_retries;
  } cns_test_result_t;

  // Test suite structure
  typedef struct
  {
    uint32_t suite_id;
    const char *suite_name;
    const char *description;
    cns_test_result_t *tests;
    uint32_t test_count;
    uint32_t max_tests;
    bool enabled;
    uint64_t total_executions;
    uint64_t passed_tests;
    uint64_t failed_tests;
    uint64_t skipped_tests;
    uint64_t total_execution_time_ns;
  } cns_test_suite_t;

  // Testing manager structure
  struct cns_testing_manager
  {
    cns_test_suite_t suites[32]; // Max 32 test suites
    uint32_t suite_count;
    uint32_t next_suite_id;
    uint32_t next_test_id;
    bool enabled;
    uint64_t total_tests_executed;
    uint64_t total_tests_passed;
    uint64_t total_tests_failed;
    uint64_t total_tests_skipped;
    uint64_t total_execution_time_ns;
  };

// Constants
#define CNS_MAX_TESTS_PER_SUITE 128
#define CNS_MAX_TEST_SUITES 32
#define CNS_DEFAULT_TEST_TIMEOUT_MS 5000 // 5 seconds
#define CNS_DEFAULT_MAX_RETRIES 1
#define CNS_PERFORMANCE_THRESHOLD_NS 10000 // 10ns for 7-tick compliance

  // Function declarations

  // Manager lifecycle
  cns_testing_manager_t *cns_testing_init(void);
  void cns_testing_cleanup(cns_testing_manager_t *manager);

  // Test suite management
  uint32_t cns_testing_create_suite(cns_testing_manager_t *manager,
                                    const char *suite_name,
                                    const char *description);
  bool cns_testing_destroy_suite(cns_testing_manager_t *manager, uint32_t suite_id);
  bool cns_testing_enable_suite(cns_testing_manager_t *manager, uint32_t suite_id, bool enabled);

  // Test management
  uint32_t cns_testing_add_test(cns_testing_manager_t *manager,
                                uint32_t suite_id,
                                const char *test_name,
                                const char *description,
                                cns_test_type_t type,
                                cns_test_func test_func,
                                void *context,
                                uint32_t timeout_ms,
                                uint32_t max_retries);
  bool cns_testing_remove_test(cns_testing_manager_t *manager, uint32_t suite_id, uint32_t test_id);

  // Test execution
  bool cns_testing_run_test(cns_testing_manager_t *manager, uint32_t suite_id, uint32_t test_id);
  bool cns_testing_run_suite(cns_testing_manager_t *manager, uint32_t suite_id);
  bool cns_testing_run_all_tests(cns_testing_manager_t *manager);
  bool cns_testing_run_tests_by_type(cns_testing_manager_t *manager, cns_test_type_t type);

  // Test retry
  bool cns_testing_retry_test(cns_testing_manager_t *manager, uint32_t suite_id, uint32_t test_id);
  bool cns_testing_retry_failed_tests(cns_testing_manager_t *manager);

  // Status and results
  cns_test_status_t cns_testing_get_test_status(cns_testing_manager_t *manager,
                                                uint32_t suite_id, uint32_t test_id);
  cns_test_result_t *cns_testing_get_test_result(cns_testing_manager_t *manager,
                                                 uint32_t suite_id, uint32_t test_id);
  bool cns_testing_get_suite_results(cns_testing_manager_t *manager, uint32_t suite_id,
                                     cns_test_result_t **results, uint32_t *result_count);

  // Performance validation
  bool cns_testing_validate_performance(cns_testing_manager_t *manager, uint32_t suite_id, uint32_t test_id);
  bool cns_testing_validate_7_tick_compliance(cns_testing_manager_t *manager);
  bool cns_testing_validate_physics_compliance(cns_testing_manager_t *manager);

  // Statistics
  uint64_t cns_testing_get_total_tests_executed(cns_testing_manager_t *manager);
  uint64_t cns_testing_get_total_tests_passed(cns_testing_manager_t *manager);
  uint64_t cns_testing_get_total_tests_failed(cns_testing_manager_t *manager);
  uint64_t cns_testing_get_total_tests_skipped(cns_testing_manager_t *manager);
  double cns_testing_get_success_rate(cns_testing_manager_t *manager);
  uint64_t cns_testing_get_total_execution_time_ns(cns_testing_manager_t *manager);
  double cns_testing_get_average_execution_time_ns(cns_testing_manager_t *manager);

  // Configuration
  bool cns_testing_set_enabled(cns_testing_manager_t *manager, bool enabled);
  bool cns_testing_is_enabled(cns_testing_manager_t *manager);
  bool cns_testing_set_default_timeout(cns_testing_manager_t *manager, uint32_t timeout_ms);
  bool cns_testing_set_default_max_retries(cns_testing_manager_t *manager, uint32_t max_retries);
  bool cns_testing_set_performance_threshold(cns_testing_manager_t *manager, uint32_t threshold_ns);

  // Built-in test functions
  bool cns_testing_unit_test_strategy_pattern(void *context);
  bool cns_testing_unit_test_factory_pattern(void *context);
  bool cns_testing_unit_test_singleton_pattern(void *context);
  bool cns_testing_unit_test_decorator_pattern(void *context);
  bool cns_testing_unit_test_observer_pattern(void *context);
  bool cns_testing_unit_test_command_pattern(void *context);

  bool cns_testing_integration_test_sparql_engine(void *context);
  bool cns_testing_integration_test_shacl_validator(void *context);
  bool cns_testing_integration_test_telemetry_system(void *context);
  bool cns_testing_integration_test_automation_pipeline(void *context);

  bool cns_testing_performance_test_7_tick_compliance(void *context);
  bool cns_testing_performance_test_memory_usage(void *context);
  bool cns_testing_performance_test_cache_efficiency(void *context);
  bool cns_testing_performance_test_throughput(void *context);

  bool cns_testing_contract_test_preconditions(void *context);
  bool cns_testing_contract_test_postconditions(void *context);
  bool cns_testing_contract_test_invariants(void *context);

  bool cns_testing_physics_test_speed_of_light(void *context);
  bool cns_testing_physics_test_cache_hierarchy(void *context);
  bool cns_testing_physics_test_memory_bandwidth(void *context);

  bool cns_testing_telemetry_test_span_generation(void *context);
  bool cns_testing_telemetry_test_attribute_setting(void *context);
  bool cns_testing_telemetry_test_performance_tracking(void *context);

  bool cns_testing_benchmark_test_pattern_performance(void *context);
  bool cns_testing_benchmark_test_system_performance(void *context);
  bool cns_testing_benchmark_test_memory_performance(void *context);

  // Test utilities
  void cns_testing_assert_true(bool condition, const char *message);
  void cns_testing_assert_false(bool condition, const char *message);
  void cns_testing_assert_equal_int(int expected, int actual, const char *message);
  void cns_testing_assert_equal_uint32(uint32_t expected, uint32_t actual, const char *message);
  void cns_testing_assert_equal_uint64(uint64_t expected, uint64_t actual, const char *message);
  void cns_testing_assert_equal_double(double expected, double actual, double tolerance, const char *message);
  void cns_testing_assert_not_null(void *ptr, const char *message);
  void cns_testing_assert_null(void *ptr, const char *message);
  void cns_testing_assert_performance_7_tick(uint32_t cycles_used, const char *message);

  // Performance validation
  void cns_testing_validate_performance_comprehensive(cns_testing_manager_t *manager);

#ifdef __cplusplus
}
#endif

#endif // CNS_TESTING_H