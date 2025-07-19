#ifndef CNS_DFLSS_H
#define CNS_DFLSS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

  // DFLSS engine structure
  typedef struct cns_dflss_engine cns_dflss_engine_t;

  // DFLSS phases
  typedef enum
  {
    CNS_DFLSS_DEFINE,
    CNS_DFLSS_MEASURE,
    CNS_DFLSS_ANALYZE,
    CNS_DFLSS_DESIGN,
    CNS_DFLSS_VERIFY
  } cns_dflss_phase_t;

  // Six Sigma levels
  typedef enum
  {
    CNS_SIGMA_ONE = 1,
    CNS_SIGMA_TWO = 2,
    CNS_SIGMA_THREE = 3,
    CNS_SIGMA_FOUR = 4,
    CNS_SIGMA_FIVE = 5,
    CNS_SIGMA_SIX = 6
  } cns_sigma_level_t;

  // Lean principles
  typedef enum
  {
    CNS_LEAN_VALUE,
    CNS_LEAN_VALUE_STREAM,
    CNS_LEAN_FLOW,
    CNS_LEAN_PULL,
    CNS_LEAN_PERFECTION
  } cns_lean_principle_t;

  // Waste types
  typedef enum
  {
    CNS_WASTE_DEFECTS,
    CNS_WASTE_OVERPRODUCTION,
    CNS_WASTE_WAITING,
    CNS_WASTE_NON_UTILIZED_TALENT,
    CNS_WASTE_TRANSPORTATION,
    CNS_WASTE_INVENTORY,
    CNS_WASTE_MOTION,
    CNS_WASTE_EXTRA_PROCESSING
  } cns_waste_type_t;

  // Design principles
  typedef enum
  {
    CNS_DESIGN_ROBUST,
    CNS_DESIGN_MANUFACTURABILITY,
    CNS_DESIGN_ASSEMBLY,
    CNS_DESIGN_TESTABILITY,
    CNS_DESIGN_RELIABILITY
  } cns_design_principle_t;

  // Statistical tools
  typedef enum
  {
    CNS_STAT_CONTROL_CHART,
    CNS_STAT_HISTOGRAM,
    CNS_STAT_PARETO_CHART,
    CNS_STAT_SCATTER_PLOT,
    CNS_STAT_DESIGN_OF_EXPERIMENTS
  } cns_statistical_tool_t;

  // DFLSS tools
  typedef enum
  {
    CNS_DFLSS_VOICE_OF_CUSTOMER,
    CNS_DFLSS_QUALITY_FUNCTION_DEPLOYMENT,
    CNS_DFLSS_FAILURE_MODE_EFFECTS_ANALYSIS,
    CNS_DFLSS_POKA_YOKE,
    CNS_DFLSS_FIVE_S
  } cns_dflss_tool_t;

  // Performance metrics
  typedef struct
  {
    uint32_t cycle_time_ns;
    uint32_t lead_time_ns;
    uint32_t takt_time_ns;
    double first_pass_yield;
    double process_capability;
    cns_sigma_level_t sigma_level;
    uint32_t defects_per_million;
    double yield_percentage;
  } cns_dflss_metrics_t;

  // DFLSS process structure
  typedef struct
  {
    uint32_t process_id;
    const char *process_name;
    const char *description;
    cns_dflss_phase_t current_phase;
    cns_sigma_level_t target_sigma;
    cns_dflss_metrics_t metrics;
    bool completed;
    uint64_t start_time;
    uint64_t end_time;
    uint64_t total_execution_time_ns;
    uint32_t waste_eliminated_count;
    uint32_t tools_applied_count;
    double performance_score;
  } cns_dflss_process_t;

  // DFLSS engine structure
  struct cns_dflss_engine
  {
    cns_dflss_process_t processes[64]; // Max 64 processes
    uint32_t process_count;
    uint32_t next_process_id;
    bool enabled;
    uint64_t total_processes_executed;
    uint64_t successful_processes;
    uint64_t failed_processes;
    uint64_t total_waste_eliminated;
    uint64_t total_tools_applied;
    double overall_performance_score;
    cns_sigma_level_t current_sigma_level;
  };

// Constants
#define CNS_MAX_DFLSS_PROCESSES 64
#define CNS_DEFAULT_SIGMA_TARGET CNS_SIGMA_SIX
#define CNS_DEFAULT_CYCLE_TIME_NS 7
#define CNS_DEFAULT_MEMORY_USAGE_BYTES 16384
#define CNS_SIGMA_LEVELS 6

  // Function declarations

  // Engine lifecycle
  cns_dflss_engine_t *cns_dflss_init(void);
  void cns_dflss_cleanup(cns_dflss_engine_t *engine);

  // Process management
  uint32_t cns_dflss_create_process(cns_dflss_engine_t *engine,
                                    const char *process_name,
                                    const char *description,
                                    cns_sigma_level_t target_sigma);
  bool cns_dflss_destroy_process(cns_dflss_engine_t *engine, uint32_t process_id);

  // Phase execution
  bool cns_dflss_execute_define_phase(cns_dflss_engine_t *engine, uint32_t process_id);
  bool cns_dflss_execute_measure_phase(cns_dflss_engine_t *engine, uint32_t process_id);
  bool cns_dflss_execute_analyze_phase(cns_dflss_engine_t *engine, uint32_t process_id);
  bool cns_dflss_execute_design_phase(cns_dflss_engine_t *engine, uint32_t process_id);
  bool cns_dflss_execute_verify_phase(cns_dflss_engine_t *engine, uint32_t process_id);
  bool cns_dflss_execute_full_process(cns_dflss_engine_t *engine, uint32_t process_id);

  // Lean principles
  bool cns_dflss_apply_lean_principle(cns_dflss_engine_t *engine, uint32_t process_id, cns_lean_principle_t principle);
  bool cns_dflss_eliminate_waste(cns_dflss_engine_t *engine, uint32_t process_id, cns_waste_type_t waste_type);
  uint32_t cns_dflss_get_waste_eliminated_count(cns_dflss_engine_t *engine, uint32_t process_id);

  // Six Sigma tools
  bool cns_dflss_apply_statistical_tool(cns_dflss_engine_t *engine, uint32_t process_id, cns_statistical_tool_t tool);
  bool cns_dflss_apply_dflss_tool(cns_dflss_engine_t *engine, uint32_t process_id, cns_dflss_tool_t tool);
  uint32_t cns_dflss_get_tools_applied_count(cns_dflss_engine_t *engine, uint32_t process_id);

  // Design principles
  bool cns_dflss_apply_design_principle(cns_dflss_engine_t *engine, uint32_t process_id, cns_design_principle_t principle);
  bool cns_dflss_validate_design_robustness(cns_dflss_engine_t *engine, uint32_t process_id);
  bool cns_dflss_validate_design_manufacturability(cns_dflss_engine_t *engine, uint32_t process_id);

  // Performance measurement
  cns_dflss_metrics_t cns_dflss_measure_performance(cns_dflss_engine_t *engine, uint32_t process_id);
  bool cns_dflss_calculate_sigma_level(cns_dflss_engine_t *engine, uint32_t process_id);
  double cns_dflss_calculate_process_capability(cns_dflss_engine_t *engine, uint32_t process_id);
  double cns_dflss_calculate_first_pass_yield(cns_dflss_engine_t *engine, uint32_t process_id);

  // Statistical analysis
  bool cns_dflss_control_chart_analysis(cns_dflss_engine_t *engine, uint32_t process_id);
  bool cns_dflss_pareto_analysis(cns_dflss_engine_t *engine, uint32_t process_id);
  bool cns_dflss_correlation_analysis(cns_dflss_engine_t *engine, uint32_t process_id);
  bool cns_dflss_design_of_experiments(cns_dflss_engine_t *engine, uint32_t process_id);

  // Quality tools
  bool cns_dflss_voice_of_customer_analysis(cns_dflss_engine_t *engine, uint32_t process_id);
  bool cns_dflss_quality_function_deployment(cns_dflss_engine_t *engine, uint32_t process_id);
  bool cns_dflss_failure_mode_effects_analysis(cns_dflss_engine_t *engine, uint32_t process_id);
  bool cns_dflss_poka_yoke_implementation(cns_dflss_engine_t *engine, uint32_t process_id);
  bool cns_dflss_five_s_implementation(cns_dflss_engine_t *engine, uint32_t process_id);

  // Process validation
  bool cns_dflss_validate_process_performance(cns_dflss_engine_t *engine, uint32_t process_id);
  bool cns_dflss_validate_7_tick_compliance(cns_dflss_engine_t *engine, uint32_t process_id);
  bool cns_dflss_validate_sigma_level_achievement(cns_dflss_engine_t *engine, uint32_t process_id);

  // Statistics
  uint64_t cns_dflss_get_total_processes(cns_dflss_engine_t *engine);
  uint64_t cns_dflss_get_successful_processes(cns_dflss_engine_t *engine);
  uint64_t cns_dflss_get_failed_processes(cns_dflss_engine_t *engine);
  double cns_dflss_get_success_rate(cns_dflss_engine_t *engine);
  double cns_dflss_get_overall_performance_score(cns_dflss_engine_t *engine);
  cns_sigma_level_t cns_dflss_get_current_sigma_level(cns_dflss_engine_t *engine);

  // Configuration
  bool cns_dflss_set_enabled(cns_dflss_engine_t *engine, bool enabled);
  bool cns_dflss_is_enabled(cns_dflss_engine_t *engine);
  bool cns_dflss_set_target_sigma_level(cns_dflss_engine_t *engine, cns_sigma_level_t sigma_level);

  // Built-in DFLSS processes
  uint32_t cns_dflss_create_software_optimization_process(cns_dflss_engine_t *engine);
  uint32_t cns_dflss_create_performance_improvement_process(cns_dflss_engine_t *engine);
  uint32_t cns_dflss_create_quality_enhancement_process(cns_dflss_engine_t *engine);
  uint32_t cns_dflss_create_waste_elimination_process(cns_dflss_engine_t *engine);

  // Performance validation
  void cns_dflss_validate_performance_comprehensive(cns_dflss_engine_t *engine);

#ifdef __cplusplus
}
#endif

#endif // CNS_DFLSS_H