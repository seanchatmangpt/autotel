#ifndef SEVEN_TICK_BENCHMARK_FRAMEWORK_H
#define SEVEN_TICK_BENCHMARK_FRAMEWORK_H

#include <stdint.h>
#include <stddef.h>
#include <time.h>
#include <immintrin.h>

// Benchmark framework version
#define SEVEN_TICK_BENCHMARK_VERSION "1.0.0"

// Performance targets
#define SEVEN_TICK_TARGET_CYCLES 7
#define SEVEN_TICK_TARGET_NS 10
#define SEVEN_TICK_TARGET_OPS_PER_SEC 100000000 // 100M ops/sec

// Benchmark result structure
typedef struct
{
  const char *test_name;
  uint64_t total_cycles;
  uint64_t total_time_ns;
  size_t operations;
  double avg_cycles_per_op;
  double avg_time_ns_per_op;
  double ops_per_sec;
  double p50_cycles;
  double p95_cycles;
  double p99_cycles;
  double p99_9_cycles;
  uint64_t min_cycles;
  uint64_t max_cycles;
  size_t operations_within_target;
  double target_achievement_percent;
  int passed;
} BenchmarkResult;

// Benchmark suite structure
typedef struct
{
  const char *suite_name;
  BenchmarkResult *results;
  size_t result_count;
  size_t result_capacity;
  uint64_t total_suite_time_ns;
  double overall_score;
} BenchmarkSuite;

// High-precision timing functions
static inline uint64_t get_cycles(void)
{
  return __builtin_readcyclecounter();
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
} BenchmarkTimer;

// Initialize benchmark timer
static inline void benchmark_timer_start(BenchmarkTimer *timer, const char *operation_name)
{
  timer->operation_name = operation_name;
  timer->start_cycles = get_cycles();
  timer->start_time_ns = get_nanoseconds();
}

// End benchmark timer
static inline void benchmark_timer_end(BenchmarkTimer *timer)
{
  timer->end_cycles = get_cycles();
  timer->end_time_ns = get_nanoseconds();
}

// Get timer results
static inline uint64_t benchmark_timer_get_cycles(BenchmarkTimer *timer)
{
  return timer->end_cycles - timer->start_cycles;
}

static inline uint64_t benchmark_timer_get_time_ns(BenchmarkTimer *timer)
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
} CycleDistribution;

// Initialize cycle distribution
CycleDistribution *cycle_distribution_create(size_t capacity);

// Add cycle count to distribution
void cycle_distribution_add(CycleDistribution *dist, uint64_t cycles);

// Calculate percentiles
double cycle_distribution_percentile(CycleDistribution *dist, double percentile);

// Destroy cycle distribution
void cycle_distribution_destroy(CycleDistribution *dist);

// Benchmark suite management
BenchmarkSuite *benchmark_suite_create(const char *suite_name);

// Add result to suite
void benchmark_suite_add_result(BenchmarkSuite *suite, BenchmarkResult result);

// Calculate suite statistics
void benchmark_suite_calculate_stats(BenchmarkSuite *suite);

// Destroy benchmark suite
void benchmark_suite_destroy(BenchmarkSuite *suite);

// Individual benchmark execution
BenchmarkResult benchmark_execute_single(
    const char *test_name,
    size_t iterations,
    void (*test_function)(void *),
    void *test_data);

// Batch benchmark execution
BenchmarkResult benchmark_execute_batch(
    const char *test_name,
    size_t iterations,
    void (*test_function)(void *, size_t),
    void *test_data,
    size_t batch_size);

// Performance validation
int benchmark_validate_target(BenchmarkResult *result);

// Result formatting and reporting
void benchmark_result_print(BenchmarkResult *result);
void benchmark_suite_print_summary(BenchmarkSuite *suite);
void benchmark_suite_print_detailed(BenchmarkSuite *suite);

// Export results
void benchmark_suite_export_json(BenchmarkSuite *suite, const char *filename);
void benchmark_suite_export_csv(BenchmarkSuite *suite, const char *filename);

// Memory usage tracking
typedef struct
{
  size_t initial_memory;
  size_t peak_memory;
  size_t final_memory;
  size_t memory_limit;
} MemoryTracker;

// Initialize memory tracker
MemoryTracker *memory_tracker_create(size_t memory_limit);

// Track memory usage
void memory_tracker_update(MemoryTracker *tracker);

// Get memory statistics
size_t memory_tracker_get_peak(MemoryTracker *tracker);
size_t memory_tracker_get_current(MemoryTracker *tracker);

// Destroy memory tracker
void memory_tracker_destroy(MemoryTracker *tracker);

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
} HardwareInfo;

// Detect hardware capabilities
HardwareInfo *detect_hardware_capabilities(void);

// Print hardware information
void print_hardware_info(HardwareInfo *info);

// Destroy hardware info
void destroy_hardware_info(HardwareInfo *info);

// Benchmark categories
typedef enum
{
  BENCHMARK_CATEGORY_LATENCY,
  BENCHMARK_CATEGORY_THROUGHPUT,
  BENCHMARK_CATEGORY_MEMORY,
  BENCHMARK_CATEGORY_CACHE,
  BENCHMARK_CATEGORY_BRANCH,
  BENCHMARK_CATEGORY_SIMD,
  BENCHMARK_CATEGORY_INTEGRATION,
  BENCHMARK_CATEGORY_STRESS
} BenchmarkCategory;

// Category-specific validation
int benchmark_validate_latency(BenchmarkResult *result);
int benchmark_validate_throughput(BenchmarkResult *result);
int benchmark_validate_memory(BenchmarkResult *result);

// Performance regression detection
typedef struct
{
  double threshold_percent;
  BenchmarkResult *baseline;
  BenchmarkResult *current;
  int regression_detected;
  double performance_change_percent;
} RegressionDetector;

// Initialize regression detector
RegressionDetector *regression_detector_create(BenchmarkResult *baseline, double threshold_percent);

// Check for regression
int regression_detector_check(RegressionDetector *detector, BenchmarkResult *current);

// Destroy regression detector
void regression_detector_destroy(RegressionDetector *detector);

// Continuous monitoring
typedef struct
{
  size_t sample_interval;
  size_t max_samples;
  BenchmarkResult *samples;
  size_t sample_count;
  double trend_slope;
} ContinuousMonitor;

// Initialize continuous monitor
ContinuousMonitor *continuous_monitor_create(size_t max_samples, size_t sample_interval);

// Add sample to monitor
void continuous_monitor_add_sample(ContinuousMonitor *monitor, BenchmarkResult *result);

// Calculate trend
void continuous_monitor_calculate_trend(ContinuousMonitor *monitor);

// Destroy continuous monitor
void continuous_monitor_destroy(ContinuousMonitor *monitor);

// Macro for easy benchmark definition
#define BENCHMARK_DEFINE(name, iterations, func, data) \
  BenchmarkResult name##_result = benchmark_execute_single(#name, iterations, func, data)

#define BENCHMARK_BATCH_DEFINE(name, iterations, func, data, batch_size) \
  BenchmarkResult name##_result = benchmark_execute_batch(#name, iterations, func, data, batch_size)

#define BENCHMARK_ASSERT(result, condition)                   \
  do                                                          \
  {                                                           \
    if (!(condition))                                         \
    {                                                         \
      printf("Benchmark assertion failed: %s\n", #condition); \
      result.passed = 0;                                      \
    }                                                         \
  } while (0)

#define BENCHMARK_TARGET_ASSERT(result) \
  BENCHMARK_ASSERT(result, result.target_achievement_percent >= 95.0)

#endif // SEVEN_TICK_BENCHMARK_FRAMEWORK_H