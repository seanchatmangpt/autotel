#ifndef SEVEN_T_BENCHMARK_FRAMEWORK_H
#define SEVEN_T_BENCHMARK_FRAMEWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <stdint.h>
#include <stdbool.h>

// Performance targets
#define SEVEN_TICK_TARGET_NS 10.0
#define L2_TIER_TARGET_NS 100.0
#define L3_TIER_TARGET_NS 1000.0

// Default benchmark parameters
#define DEFAULT_ITERATIONS 1000000
#define DEFAULT_WARMUP_ITERATIONS 10000
#define DEFAULT_BATCH_SIZE 1000

// Benchmark result structure
typedef struct
{
  const char *operation_name;
  const char *component_name;
  uint64_t total_operations;
  uint64_t total_time_ns;
  uint64_t min_time_ns;
  uint64_t max_time_ns;
  double avg_time_ns;
  double throughput_ops_per_sec;
  double p50_time_ns;
  double p95_time_ns;
  double p99_time_ns;
  bool target_achieved;
  const char *performance_tier;
} BenchmarkResult;

// Benchmark context structure
typedef struct
{
  const char *benchmark_name;
  uint64_t iterations;
  uint64_t warmup_iterations;
  uint64_t batch_size;
  bool verbose;
  bool validate_correctness;
} BenchmarkConfig;

// High-precision timing functions
static inline uint64_t get_nanoseconds()
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static inline uint64_t get_microseconds()
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)ts.tv_nsec / 1000;
}

// Benchmark operation function type
typedef void (*BenchmarkOperation)(void *context);

// Framework functions
BenchmarkResult run_benchmark(const char *operation_name,
                              const char *component_name,
                              BenchmarkConfig *config,
                              BenchmarkOperation operation,
                              void *context);

void print_benchmark_result(BenchmarkResult *result);
void print_benchmark_summary(BenchmarkResult *results, int count);
const char *get_performance_tier(double avg_time_ns);
bool is_target_achieved(double avg_time_ns);

// Utility functions
void benchmark_warmup(BenchmarkOperation operation, void *context, uint64_t iterations);
double calculate_percentile(uint64_t *times, uint64_t count, double percentile);
void sort_times(uint64_t *times, uint64_t count);

// Standard benchmark configurations
BenchmarkConfig get_default_config(void);
BenchmarkConfig get_quick_config(void);
BenchmarkConfig get_thorough_config(void);

// Performance validation
bool validate_7tick_performance(double avg_time_ns);
bool validate_l2_tier_performance(double avg_time_ns);
bool validate_l3_tier_performance(double avg_time_ns);

// Memory usage tracking
typedef struct
{
  size_t peak_memory_bytes;
  size_t current_memory_bytes;
  size_t allocations_count;
  size_t deallocations_count;
} MemoryStats;

MemoryStats get_memory_stats(void);
void print_memory_stats(MemoryStats *stats);

// Hardware information
typedef struct
{
  const char *cpu_model;
  double cpu_frequency_ghz;
  size_t l1_cache_size_kb;
  size_t l2_cache_size_kb;
  size_t l3_cache_size_kb;
  size_t memory_size_gb;
} HardwareInfo;

HardwareInfo get_hardware_info(void);
void print_hardware_info(HardwareInfo *info);

// Benchmark suite management
typedef struct
{
  const char *suite_name;
  BenchmarkResult *results;
  int result_count;
  int result_capacity;
  HardwareInfo hardware;
  MemoryStats memory;
} BenchmarkSuite;

BenchmarkSuite *create_benchmark_suite(const char *name);
void add_benchmark_result(BenchmarkSuite *suite, BenchmarkResult result);
void print_benchmark_suite(BenchmarkSuite *suite);
void destroy_benchmark_suite(BenchmarkSuite *suite);

// CSV and JSON output
void export_benchmark_results_csv(BenchmarkSuite *suite, const char *filename);
void export_benchmark_results_json(BenchmarkSuite *suite, const char *filename);

#endif // SEVEN_T_BENCHMARK_FRAMEWORK_H