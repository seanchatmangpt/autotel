#ifndef CNS_DRY_H
#define CNS_DRY_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

  // DRY manager structure
  typedef struct cns_dry_manager cns_dry_manager_t;

  // DRY pattern types
  typedef enum
  {
    CNS_DRY_PATTERN_FUNCTION,
    CNS_DRY_PATTERN_MACRO,
    CNS_DRY_PATTERN_STRUCTURE,
    CNS_DRY_PATTERN_ALGORITHM,
    CNS_DRY_PATTERN_CONSTANT,
    CNS_DRY_PATTERN_TEMPLATE,
    CNS_DRY_PATTERN_ABSTRACTION,
    CNS_DRY_PATTERN_CUSTOM
  } cns_dry_pattern_type_t;

  // DRY pattern status
  typedef enum
  {
    CNS_DRY_PATTERN_PENDING,
    CNS_DRY_PATTERN_ANALYZING,
    CNS_DRY_PATTERN_IDENTIFIED,
    CNS_DRY_PATTERN_EXTRACTED,
    CNS_DRY_PATTERN_REFACTORED,
    CNS_DRY_PATTERN_VALIDATED,
    CNS_DRY_PATTERN_FAILED
  } cns_dry_pattern_status_t;

  // DRY pattern function pointer
  typedef void *(*cns_dry_pattern_func)(void *context);

  // DRY pattern structure
  typedef struct
  {
    uint32_t pattern_id;
    const char *pattern_name;
    const char *description;
    cns_dry_pattern_type_t type;
    cns_dry_pattern_func pattern_func;
    void *context;
    cns_dry_pattern_status_t status;
    uint64_t analysis_time;
    uint32_t analysis_cycles;
    bool performance_compliant;
    uint32_t duplication_count;
    uint32_t extraction_count;
    uint32_t reuse_count;
    uint64_t total_savings_bytes;
    uint64_t total_savings_cycles;
  } cns_dry_pattern_t;

  // DRY analysis result structure
  typedef struct
  {
    uint32_t result_id;
    const char *file_path;
    uint32_t line_number;
    const char *duplicated_code;
    uint32_t duplication_length;
    uint32_t duplication_count;
    uint64_t analysis_time_ns;
    bool can_be_extracted;
    const char *suggested_abstraction;
  } cns_dry_analysis_result_t;

  // DRY manager structure
  struct cns_dry_manager
  {
    cns_dry_pattern_t patterns[128]; // Max 128 patterns
    uint32_t pattern_count;
    uint32_t next_pattern_id;
    bool enabled;
    uint64_t total_analyses;
    uint64_t total_duplications_found;
    uint64_t total_extractions_made;
    uint64_t total_reuses_counted;
    uint64_t total_savings_bytes;
    uint64_t total_savings_cycles;
    uint64_t total_analysis_time_ns;
  };

// Constants
#define CNS_MAX_DRY_PATTERNS 128
#define CNS_MAX_DUPLICATION_LENGTH 1000
#define CNS_MIN_DUPLICATION_LENGTH 10
#define CNS_DRY_ANALYSIS_THRESHOLD_NS 1000 // 1μs for analysis

  // Function declarations

  // Manager lifecycle
  cns_dry_manager_t *cns_dry_init(void);
  void cns_dry_cleanup(cns_dry_manager_t *manager);

  // Pattern management
  uint32_t cns_dry_add_pattern(cns_dry_manager_t *manager,
                               const char *pattern_name,
                               const char *description,
                               cns_dry_pattern_type_t type,
                               cns_dry_pattern_func pattern_func,
                               void *context);
  bool cns_dry_remove_pattern(cns_dry_manager_t *manager, uint32_t pattern_id);
  bool cns_dry_enable_pattern(cns_dry_manager_t *manager, uint32_t pattern_id, bool enabled);

  // Code analysis
  bool cns_dry_analyze_file(cns_dry_manager_t *manager, const char *file_path);
  bool cns_dry_analyze_directory(cns_dry_manager_t *manager, const char *directory_path);
  bool cns_dry_analyze_code_snippet(cns_dry_manager_t *manager, const char *code_snippet);
  bool cns_dry_analyze_function(cns_dry_manager_t *manager, const char *function_name, const char *function_code);

  // Duplication detection
  uint32_t cns_dry_detect_duplications(cns_dry_manager_t *manager, const char *file_path);
  bool cns_dry_get_duplication_results(cns_dry_manager_t *manager, const char *file_path,
                                       cns_dry_analysis_result_t **results, uint32_t *result_count);
  bool cns_dry_is_duplicated_code(cns_dry_manager_t *manager, const char *code_snippet);

  // Pattern extraction
  bool cns_dry_extract_pattern(cns_dry_manager_t *manager, uint32_t pattern_id, const char *code_snippet);
  bool cns_dry_extract_function(cns_dry_manager_t *manager, const char *function_name, const char *function_code);
  bool cns_dry_extract_macro(cns_dry_manager_t *manager, const char *macro_name, const char *macro_definition);
  bool cns_dry_extract_structure(cns_dry_manager_t *manager, const char *structure_name, const char *structure_definition);

  // Pattern reuse
  void *cns_dry_reuse_pattern(cns_dry_manager_t *manager, uint32_t pattern_id, void *context);
  void *cns_dry_reuse_function(cns_dry_manager_t *manager, const char *function_name, void *context);
  void *cns_dry_reuse_macro(cns_dry_manager_t *manager, const char *macro_name, void *context);
  void *cns_dry_reuse_structure(cns_dry_manager_t *manager, const char *structure_name, void *context);

  // Performance validation
  bool cns_dry_validate_performance(cns_dry_manager_t *manager, uint32_t pattern_id);
  bool cns_dry_validate_7_tick_compliance(cns_dry_manager_t *manager);
  bool cns_dry_validate_physics_compliance(cns_dry_manager_t *manager);

  // Statistics
  uint64_t cns_dry_get_total_analyses(cns_dry_manager_t *manager);
  uint64_t cns_dry_get_total_duplications_found(cns_dry_manager_t *manager);
  uint64_t cns_dry_get_total_extractions_made(cns_dry_manager_t *manager);
  uint64_t cns_dry_get_total_reuses_counted(cns_dry_manager_t *manager);
  uint64_t cns_dry_get_total_savings_bytes(cns_dry_manager_t *manager);
  uint64_t cns_dry_get_total_savings_cycles(cns_dry_manager_t *manager);
  double cns_dry_get_efficiency_ratio(cns_dry_manager_t *manager);
  uint64_t cns_dry_get_total_analysis_time_ns(cns_dry_manager_t *manager);

  // Configuration
  bool cns_dry_set_enabled(cns_dry_manager_t *manager, bool enabled);
  bool cns_dry_is_enabled(cns_dry_manager_t *manager);
  bool cns_dry_set_min_duplication_length(cns_dry_manager_t *manager, uint32_t min_length);
  bool cns_dry_set_max_duplication_length(cns_dry_manager_t *manager, uint32_t max_length);

  // Built-in pattern functions
  void *cns_dry_pattern_strategy_execute(void *context);
  void *cns_dry_pattern_factory_create(void *context);
  void *cns_dry_pattern_singleton_get_instance(void *context);
  void *cns_dry_pattern_decorator_operation(void *context);
  void *cns_dry_pattern_observer_notify(void *context);
  void *cns_dry_pattern_command_execute(void *context);

  void *cns_dry_pattern_memory_allocate(void *context);
  void *cns_dry_pattern_memory_free(void *context);
  void *cns_dry_pattern_string_compare(void *context);
  void *cns_dry_pattern_string_copy(void *context);

  void *cns_dry_pattern_telemetry_span_start(void *context);
  void *cns_dry_pattern_telemetry_span_end(void *context);
  void *cns_dry_pattern_telemetry_set_attribute(void *context);

  void *cns_dry_pattern_performance_measure(void *context);
  void *cns_dry_pattern_performance_validate(void *context);
  void *cns_dry_pattern_performance_optimize(void *context);

// DRY macros for easy use
#define CNS_DRY_PATTERN(name, type, func)           \
  static const cns_dry_pattern_t name##_pattern = { \
      .pattern_name = #name,                        \
      .type = type,                                 \
      .pattern_func = func}

#define CNS_DRY_REUSE(pattern_name, context) \
  cns_dry_reuse_pattern(manager, pattern_name##_pattern.pattern_id, context)

#define CNS_DRY_EXTRACT(pattern_name, code) \
  cns_dry_extract_pattern(manager, pattern_name##_pattern.pattern_id, code)

#define CNS_DRY_ANALYZE(file_path) \
  cns_dry_analyze_file(manager, file_path)

#define CNS_DRY_DETECT(file_path) \
  cns_dry_detect_duplications(manager, file_path)

  // DRY utilities
  bool cns_dry_compare_code_snippets(const char *snippet1, const char *snippet2);
  uint32_t cns_dry_calculate_similarity(const char *code1, const char *code2);
  bool cns_dry_can_extract_pattern(const char *code_snippet);
  const char *cns_dry_suggest_abstraction(const char *code_snippet);

  // Performance validation
  void cns_dry_validate_performance_comprehensive(cns_dry_manager_t *manager);

#ifdef __cplusplus
}
#endif

#endif // CNS_DRY_H