#ifndef DEMO_BENCHMARK_FRAMEWORK_H
#define DEMO_BENCHMARK_FRAMEWORK_H

#include <stdint.h>
#include <stddef.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Demo benchmark framework version
#define DEMO_BENCHMARK_VERSION "2.0.0"

// Performance targets for demo use cases
#define DEMO_TARGET_CYCLES 49        // 49-tick target for pipeline evaluation
#define DEMO_TARGET_NS 1000          // 1000ns (1μs) target for pipeline evaluation
#define DEMO_TARGET_FITNESS_MIN 0.7  // Minimum meaningful fitness score
#define DEMO_TARGET_FITNESS_MAX 0.95 // Maximum expected fitness score

// Demo use case types
typedef enum
{
  DEMO_IRIS_CLASSIFICATION,
  DEMO_BOSTON_REGRESSION,
  DEMO_DIGITS_CLASSIFICATION,
  DEMO_BREAST_CANCER_CLASSIFICATION,
  DEMO_WINE_QUALITY_CLASSIFICATION
} DemoUseCase;

// Pipeline step types
typedef enum
{
  PIPELINE_STEP_PREPROCESSING,
  PIPELINE_STEP_FEATURE_SELECTION,
  PIPELINE_STEP_MODEL_TRAINING,
  PIPELINE_STEP_EVALUATION
} PipelineStepType;

// Pipeline step result
typedef struct
{
  PipelineStepType step_type;
  const char *step_name;
  uint64_t execution_time_ns;
  uint64_t execution_cycles;
  double fitness_score;
  uint32_t samples_processed;
  uint32_t features_processed;
  int success;
} PipelineStepResult;

// Pipeline result
typedef struct
{
  DemoUseCase use_case;
  const char *use_case_name;
  PipelineStepResult *steps;
  size_t step_count;
  uint64_t total_pipeline_time_ns;
  uint64_t total_pipeline_cycles;
  double overall_fitness;
  double avg_fitness_per_step;
  int pipeline_success;
  size_t total_samples;
  size_t total_features;
} PipelineResult;

// Benchmark result for demo use cases
typedef struct
{
  const char *test_name;
  DemoUseCase use_case;
  PipelineResult pipeline_result;
  uint64_t total_cycles;
  uint64_t total_time_ns;
  size_t iterations;
  double avg_cycles_per_iteration;
  double avg_time_ns_per_iteration;
  double ops_per_sec;
  double p50_cycles;
  double p95_cycles;
  double p99_cycles;
  uint64_t min_cycles;
  uint64_t max_cycles;
  size_t iterations_within_target;
  double target_achievement_percent;
  double fitness_achievement_percent;
  int passed;
} DemoBenchmarkResult;

// Benchmark suite for demo use cases
typedef struct
{
  const char *suite_name;
  DemoBenchmarkResult *results;
  size_t result_count;
  size_t result_capacity;
  uint64_t total_suite_time_ns;
  double overall_score;
  double overall_fitness_score;
} DemoBenchmarkSuite;

// High-precision timing functions
static inline uint64_t get_cycles(void)
{
#ifdef __aarch64__
  // ARM64 cycle counter
  uint64_t val;
  asm volatile("mrs %0, cntvct_el0" : "=r" (val));
  return val;
#elif defined(__x86_64__) || defined(__i386__)
  // x86 cycle counter
  return __builtin_readcyclecounter();
#else
  // Fallback to nanoseconds
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1000000000ULL + ts.tv_nsec;
#endif
}

static inline uint64_t get_nanoseconds(void)
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

// Benchmark timer structure
typedef struct
{
  uint64_t start_cycles;
  uint64_t end_cycles;
  uint64_t start_time_ns;
  uint64_t end_time_ns;
  const char *operation_name;
} DemoBenchmarkTimer;

// Initialize benchmark timer
static inline void demo_benchmark_timer_start(DemoBenchmarkTimer *timer, const char *operation_name)
{
  timer->operation_name = operation_name;
  timer->start_cycles = get_cycles();
  timer->start_time_ns = get_nanoseconds();
}

// End benchmark timer
static inline void demo_benchmark_timer_end(DemoBenchmarkTimer *timer)
{
  timer->end_cycles = get_cycles();
  timer->end_time_ns = get_nanoseconds();
}

// Get timer results
static inline uint64_t demo_benchmark_timer_get_cycles(DemoBenchmarkTimer *timer)
{
  return timer->end_cycles - timer->start_cycles;
}

static inline uint64_t demo_benchmark_timer_get_time_ns(DemoBenchmarkTimer *timer)
{
  return timer->end_time_ns - timer->start_time_ns;
}

// Cycle distribution tracking
typedef struct
{
  uint64_t *cycle_counts;
  size_t capacity;
  size_t count;
  uint64_t total_cycles;
  uint64_t min_cycles;
  uint64_t max_cycles;
} DemoCycleDistribution;

// Fitness distribution tracking
typedef struct
{
  double *fitness_scores;
  size_t capacity;
  size_t count;
  double total_fitness;
  double min_fitness;
  double max_fitness;
} FitnessDistribution;

// Initialize cycle distribution
DemoCycleDistribution *demo_cycle_distribution_create(size_t capacity);

// Add cycle count to distribution
void demo_cycle_distribution_add(DemoCycleDistribution *dist, uint64_t cycles);

// Calculate percentiles
double demo_cycle_distribution_percentile(DemoCycleDistribution *dist, double percentile);

// Destroy cycle distribution
void demo_cycle_distribution_destroy(DemoCycleDistribution *dist);

// Initialize fitness distribution
FitnessDistribution *fitness_distribution_create(size_t capacity);

// Add fitness score to distribution
void fitness_distribution_add(FitnessDistribution *dist, double fitness);

// Calculate fitness percentiles
double fitness_distribution_percentile(FitnessDistribution *dist, double percentile);

// Destroy fitness distribution
void fitness_distribution_destroy(FitnessDistribution *dist);

// Demo benchmark suite management
DemoBenchmarkSuite *demo_benchmark_suite_create(const char *suite_name);

// Add result to suite
void demo_benchmark_suite_add_result(DemoBenchmarkSuite *suite, DemoBenchmarkResult result);

// Calculate suite statistics
void demo_benchmark_suite_calculate_stats(DemoBenchmarkSuite *suite);

// Destroy benchmark suite
void demo_benchmark_suite_destroy(DemoBenchmarkSuite *suite);

// Individual demo benchmark execution
DemoBenchmarkResult demo_benchmark_execute_single(
    const char *test_name,
    DemoUseCase use_case,
    size_t iterations,
    PipelineResult (*pipeline_function)(void),
    void *pipeline_data);

// Batch demo benchmark execution
DemoBenchmarkResult demo_benchmark_execute_batch(
    const char *test_name,
    DemoUseCase use_case,
    size_t iterations,
    PipelineResult (*pipeline_function)(void *, size_t),
    void *pipeline_data,
    size_t batch_size);

// Performance validation for demo use cases
int demo_benchmark_validate_target(DemoBenchmarkResult *result);

// Fitness validation for demo use cases
int demo_benchmark_validate_fitness(DemoBenchmarkResult *result);

// Result formatting and reporting
void demo_benchmark_result_print(DemoBenchmarkResult *result);
void demo_benchmark_suite_print_summary(DemoBenchmarkSuite *suite);
void demo_benchmark_suite_print_detailed(DemoBenchmarkSuite *suite);

// Export results
void demo_benchmark_suite_export_json(DemoBenchmarkSuite *suite, const char *filename);
void demo_benchmark_suite_export_csv(DemoBenchmarkSuite *suite, const char *filename);

// Memory usage tracking
typedef struct
{
  size_t initial_memory;
  size_t peak_memory;
  size_t final_memory;
  size_t memory_limit;
} DemoMemoryTracker;

// Initialize memory tracker
DemoMemoryTracker *demo_memory_tracker_create(size_t memory_limit);

// Track memory usage
void demo_memory_tracker_update(DemoMemoryTracker *tracker);

// Get memory statistics
size_t demo_memory_tracker_get_peak(DemoMemoryTracker *tracker);
size_t demo_memory_tracker_get_current(DemoMemoryTracker *tracker);

// Destroy memory tracker
void demo_memory_tracker_destroy(DemoMemoryTracker *tracker);

// CPU frequency detection
double detect_cpu_frequency(void);

// Cache line size detection
size_t detect_cache_line_size(void);

// Hardware capabilities detection
typedef struct
{
  int avx2_support;
  int avx512_support;
  int sse4_2_support;
  size_t l1_cache_size;
  size_t l2_cache_size;
  size_t l3_cache_size;
  int num_cores;
  double cpu_frequency_ghz;
} DemoHardwareInfo;

// Detect hardware capabilities
DemoHardwareInfo *detect_demo_hardware_capabilities(void);

// Print hardware information
void print_demo_hardware_info(DemoHardwareInfo *info);

// Destroy hardware info
void destroy_demo_hardware_info(DemoHardwareInfo *info);

// Demo benchmark categories
typedef enum
{
  DEMO_BENCHMARK_CATEGORY_LATENCY,
  DEMO_BENCHMARK_CATEGORY_THROUGHPUT,
  DEMO_BENCHMARK_CATEGORY_FITNESS,
  DEMO_BENCHMARK_CATEGORY_MEMORY,
  DEMO_BENCHMARK_CATEGORY_INTEGRATION,
  DEMO_BENCHMARK_CATEGORY_STRESS
} DemoBenchmarkCategory;

// Category-specific validation
int demo_benchmark_validate_latency(DemoBenchmarkResult *result);
int demo_benchmark_validate_throughput(DemoBenchmarkResult *result);
int demo_benchmark_validate_fitness_quality(DemoBenchmarkResult *result);

// Performance regression detection
typedef struct
{
  double threshold_percent;
  DemoBenchmarkResult *baseline;
  DemoBenchmarkResult *current;
  int regression_detected;
  double performance_change_percent;
  double fitness_change_percent;
} DemoRegressionDetector;

// Initialize regression detector
DemoRegressionDetector *demo_regression_detector_create(DemoBenchmarkResult *baseline, double threshold_percent);

// Check for regression
int demo_regression_detector_check(DemoRegressionDetector *detector, DemoBenchmarkResult *current);

// Destroy regression detector
void demo_regression_detector_destroy(DemoRegressionDetector *detector);

// Continuous monitoring
typedef struct
{
  size_t sample_interval;
  size_t max_samples;
  DemoBenchmarkResult *samples;
  size_t sample_count;
  double trend_slope;
  double fitness_trend_slope;
} DemoContinuousMonitor;

// Initialize continuous monitor
DemoContinuousMonitor *demo_continuous_monitor_create(size_t max_samples, size_t sample_interval);

// Add sample to monitor
void demo_continuous_monitor_add_sample(DemoContinuousMonitor *monitor, DemoBenchmarkResult *result);

// Calculate trend
void demo_continuous_monitor_calculate_trend(DemoContinuousMonitor *monitor);

// Destroy continuous monitor
void demo_continuous_monitor_destroy(DemoContinuousMonitor *monitor);

// Demo use case specific functions
const char *demo_use_case_get_name(DemoUseCase use_case);
size_t demo_use_case_get_sample_count(DemoUseCase use_case);
size_t demo_use_case_get_feature_count(DemoUseCase use_case);
size_t demo_use_case_get_class_count(DemoUseCase use_case);

// Pipeline step management
PipelineStepResult *pipeline_step_result_create(PipelineStepType step_type, const char *step_name);
void pipeline_step_result_destroy(PipelineStepResult *step);

// Pipeline result management
PipelineResult *pipeline_result_create(DemoUseCase use_case);
void pipeline_result_add_step(PipelineResult *pipeline, PipelineStepResult *step);
void pipeline_result_calculate_stats(PipelineResult *pipeline);
void pipeline_result_destroy(PipelineResult *pipeline);

// Utility functions
double calculate_fitness_percentile(double *fitness_scores, size_t count, double percentile);
void print_pipeline_step_result(PipelineStepResult *step);
void print_pipeline_result(PipelineResult *pipeline);

#endif // DEMO_BENCHMARK_FRAMEWORK_H