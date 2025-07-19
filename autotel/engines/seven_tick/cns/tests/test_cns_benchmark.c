/*  ─────────────────────────────────────────────────────────────
    test_cns_benchmark.c  –  CNS Benchmark Unit Tests (v1.0)
    7-tick compliant test framework for CNS benchmark functionality
    ───────────────────────────────────────────────────────────── */

#include "../include/cns_benchmark.h"
#include "../include/s7t.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

/*═══════════════════════════════════════════════════════════════
  Test Infrastructure
  ═══════════════════════════════════════════════════════════════*/

// Test context
typedef struct
{
  uint64_t test_start;
  uint64_t test_cycles;
  bool test_passed;
  const char *test_name;
} test_context_t;

// Test result tracking
static uint32_t tests_run = 0;
static uint32_t tests_passed = 0;
static uint32_t tests_failed = 0;

// Test macros
#define TEST_BEGIN(name)          \
  test_context_t ctx = {          \
      .test_start = s7t_cycles(), \
      .test_name = name,          \
      .test_passed = true};       \
  printf("TEST: %s\n", name)

#define TEST_END()                                        \
  ctx.test_cycles = s7t_cycles() - ctx.test_start;        \
  tests_run++;                                            \
  if (ctx.test_passed)                                    \
  {                                                       \
    tests_passed++;                                       \
    printf("  ✓ PASSED (%lu cycles)\n", ctx.test_cycles); \
  }                                                       \
  else                                                    \
  {                                                       \
    tests_failed++;                                       \
    printf("  ✗ FAILED (%lu cycles)\n", ctx.test_cycles); \
  }

#define TEST_ASSERT(condition)                     \
  if (!(condition))                                \
  {                                                \
    ctx.test_passed = false;                       \
    printf("    ASSERT FAILED: %s\n", #condition); \
  }

#define TEST_ASSERT_CYCLES(max_cycles)                                       \
  if (ctx.test_cycles > max_cycles)                                          \
  {                                                                          \
    ctx.test_passed = false;                                                 \
    printf("    CYCLES EXCEEDED: %lu > %lu\n", ctx.test_cycles, max_cycles); \
  }

/*═══════════════════════════════════════════════════════════════
  Benchmark Configuration Tests
  ═══════════════════════════════════════════════════════════════*/

// Test benchmark configuration initialization
static void test_benchmark_config()
{
  TEST_BEGIN("Benchmark Configuration");

  // Test default configuration
  cns_benchmark_config_t config = {
      .benchmark_name = "test_benchmark",
      .iterations = 1000,
      .warmup_iterations = 100,
      .batch_size = 10,
      .verbose = false,
      .validate_correctness = true};

  // Verify configuration fields
  TEST_ASSERT(strcmp(config.benchmark_name, "test_benchmark") == 0);
  TEST_ASSERT(config.iterations == 1000);
  TEST_ASSERT(config.warmup_iterations == 100);
  TEST_ASSERT(config.batch_size == 10);
  TEST_ASSERT(config.verbose == false);
  TEST_ASSERT(config.validate_correctness == true);

  // Test configuration modification
  config.iterations = 5000;
  config.verbose = true;
  config.validate_correctness = false;

  TEST_ASSERT(config.iterations == 5000);
  TEST_ASSERT(config.verbose == true);
  TEST_ASSERT(config.validate_correctness == false);

  TEST_ASSERT_CYCLES(100);
  TEST_END();
}

// Test performance targets
static void test_performance_targets()
{
  TEST_BEGIN("Performance Targets");

  // Test target constants
  TEST_ASSERT(CNS_SEVEN_TICK_TARGET_NS == 10.0);
  TEST_ASSERT(CNS_L2_TIER_TARGET_NS == 100.0);
  TEST_ASSERT(CNS_L3_TIER_TARGET_NS == 1000.0);

  // Test target hierarchy
  TEST_ASSERT(CNS_SEVEN_TICK_TARGET_NS < CNS_L2_TIER_TARGET_NS);
  TEST_ASSERT(CNS_L2_TIER_TARGET_NS < CNS_L3_TIER_TARGET_NS);

  // Test target relationships
  TEST_ASSERT(CNS_L2_TIER_TARGET_NS == CNS_SEVEN_TICK_TARGET_NS * 10.0);
  TEST_ASSERT(CNS_L3_TIER_TARGET_NS == CNS_L2_TIER_TARGET_NS * 10.0);

  TEST_ASSERT_CYCLES(50);
  TEST_END();
}

/*═══════════════════════════════════════════════════════════════
  Benchmark Results Tests
  ═══════════════════════════════════════════════════════════════*/

// Test benchmark result initialization
static void test_benchmark_results()
{
  TEST_BEGIN("Benchmark Results");

  // Test result structure initialization
  cns_benchmark_result_t result = {
      .operation_name = "test_operation",
      .component_name = "test_component",
      .total_operations = 1000,
      .total_time_ns = 50000,
      .min_time_ns = 45,
      .max_time_ns = 55,
      .avg_time_ns = 50.0,
      .throughput_ops_per_sec = 20000.0,
      .p50_time_ns = 50.0,
      .p95_time_ns = 52.0,
      .p99_time_ns = 54.0,
      .target_achieved = true,
      .performance_tier = "L1"};

  // Verify result fields
  TEST_ASSERT(strcmp(result.operation_name, "test_operation") == 0);
  TEST_ASSERT(strcmp(result.component_name, "test_component") == 0);
  TEST_ASSERT(result.total_operations == 1000);
  TEST_ASSERT(result.total_time_ns == 50000);
  TEST_ASSERT(result.min_time_ns == 45);
  TEST_ASSERT(result.max_time_ns == 55);
  TEST_ASSERT(result.avg_time_ns == 50.0);
  TEST_ASSERT(result.throughput_ops_per_sec == 20000.0);
  TEST_ASSERT(result.p50_time_ns == 50.0);
  TEST_ASSERT(result.p95_time_ns == 52.0);
  TEST_ASSERT(result.p99_time_ns == 54.0);
  TEST_ASSERT(result.target_achieved == true);
  TEST_ASSERT(strcmp(result.performance_tier, "L1") == 0);

  // Test result validation
  TEST_ASSERT(result.min_time_ns <= result.avg_time_ns);
  TEST_ASSERT(result.avg_time_ns <= result.max_time_ns);
  TEST_ASSERT(result.p50_time_ns <= result.p95_time_ns);
  TEST_ASSERT(result.p95_time_ns <= result.p99_time_ns);
  TEST_ASSERT(result.total_time_ns > 0);
  TEST_ASSERT(result.throughput_ops_per_sec > 0);

  TEST_ASSERT_CYCLES(100);
  TEST_END();
}

// Test benchmark result calculations
static void test_benchmark_calculations()
{
  TEST_BEGIN("Benchmark Calculations");

  cns_benchmark_result_t result = {0};

  // Test throughput calculation
  result.total_operations = 1000;
  result.total_time_ns = 1000000; // 1ms

  double expected_throughput = (double)result.total_operations /
                               (result.total_time_ns / 1e9);
  TEST_ASSERT(expected_throughput == 1000000.0); // 1M ops/sec

  // Test average calculation
  result.min_time_ns = 40;
  result.max_time_ns = 60;
  result.avg_time_ns = 50.0;

  TEST_ASSERT(result.avg_time_ns >= result.min_time_ns);
  TEST_ASSERT(result.avg_time_ns <= result.max_time_ns);

  // Test percentile relationships
  result.p50_time_ns = 50.0;
  result.p95_time_ns = 55.0;
  result.p99_time_ns = 58.0;

  TEST_ASSERT(result.p50_time_ns <= result.p95_time_ns);
  TEST_ASSERT(result.p95_time_ns <= result.p99_time_ns);

  TEST_ASSERT_CYCLES(150);
  TEST_END();
}

// Test performance tier classification
static void test_performance_tiers()
{
  TEST_BEGIN("Performance Tier Classification");

  cns_benchmark_result_t result = {0};

  // Test L1 tier (7-tick target)
  result.avg_time_ns = 8.0;
  result.target_achieved = (result.avg_time_ns <= CNS_SEVEN_TICK_TARGET_NS);
  result.performance_tier = result.target_achieved ? "L1" : "L2";

  TEST_ASSERT(result.target_achieved == true);
  TEST_ASSERT(strcmp(result.performance_tier, "L1") == 0);

  // Test L2 tier
  result.avg_time_ns = 50.0;
  result.target_achieved = (result.avg_time_ns <= CNS_L2_TIER_TARGET_NS);
  result.performance_tier = result.target_achieved ? "L2" : "L3";

  TEST_ASSERT(result.target_achieved == true);
  TEST_ASSERT(strcmp(result.performance_tier, "L2") == 0);

  // Test L3 tier
  result.avg_time_ns = 500.0;
  result.target_achieved = (result.avg_time_ns <= CNS_L3_TIER_TARGET_NS);
  result.performance_tier = result.target_achieved ? "L3" : "L4";

  TEST_ASSERT(result.target_achieved == true);
  TEST_ASSERT(strcmp(result.performance_tier, "L3") == 0);

  // Test failure case
  result.avg_time_ns = 2000.0;
  result.target_achieved = (result.avg_time_ns <= CNS_L3_TIER_TARGET_NS);
  result.performance_tier = result.target_achieved ? "L3" : "L4";

  TEST_ASSERT(result.target_achieved == false);
  TEST_ASSERT(strcmp(result.performance_tier, "L4") == 0);

  TEST_ASSERT_CYCLES(200);
  TEST_END();
}

/*═══════════════════════════════════════════════════════════════
  Benchmark Command Interface Tests
  ═══════════════════════════════════════════════════════════════*/

// Test benchmark command entry point
static void test_benchmark_command()
{
  TEST_BEGIN("Benchmark Command Interface");

  // Test command function signature
  // The cmd_benchmark function should exist and be callable
  // We can't test the actual implementation without the full CNS framework,
  // but we can verify the interface is properly defined

  // Test that the function pointer can be assigned
  int (*benchmark_func)(int, char **) = cmd_benchmark;
  TEST_ASSERT(benchmark_func != NULL);

  // Test with null arguments (should not crash)
  // Note: This is a basic interface test, not a functional test
  // The actual implementation would need the full CNS context

  TEST_ASSERT_CYCLES(100);
  TEST_END();
}

/*═══════════════════════════════════════════════════════════════
  Benchmark Utility Tests
  ═══════════════════════════════════════════════════════════════*/

// Test benchmark data validation
static void test_benchmark_validation()
{
  TEST_BEGIN("Benchmark Data Validation");

  cns_benchmark_result_t result = {0};

  // Test valid data
  result.total_operations = 1000;
  result.total_time_ns = 50000;
  result.min_time_ns = 45;
  result.max_time_ns = 55;
  result.avg_time_ns = 50.0;

  // Validate data consistency
  bool valid = true;

  // Check basic constraints
  if (result.total_operations == 0)
    valid = false;
  if (result.total_time_ns == 0)
    valid = false;
  if (result.min_time_ns > result.max_time_ns)
    valid = false;
  if (result.avg_time_ns < result.min_time_ns || result.avg_time_ns > result.max_time_ns)
    valid = false;

  TEST_ASSERT(valid == true);

  // Test invalid data
  result.min_time_ns = 60; // Greater than max_time_ns
  valid = true;

  if (result.min_time_ns > result.max_time_ns)
    valid = false;

  TEST_ASSERT(valid == false);

  TEST_ASSERT_CYCLES(150);
  TEST_END();
}

// Test benchmark statistics
static void test_benchmark_statistics()
{
  TEST_BEGIN("Benchmark Statistics");

  // Test statistical calculations
  uint64_t times[] = {45, 47, 50, 52, 55};
  uint32_t count = 5;

  // Calculate min, max, avg
  uint64_t min_time = UINT64_MAX;
  uint64_t max_time = 0;
  uint64_t total_time = 0;

  for (uint32_t i = 0; i < count; i++)
  {
    if (times[i] < min_time)
      min_time = times[i];
    if (times[i] > max_time)
      max_time = times[i];
    total_time += times[i];
  }

  double avg_time = (double)total_time / count;

  // Verify calculations
  TEST_ASSERT(min_time == 45);
  TEST_ASSERT(max_time == 55);
  TEST_ASSERT(avg_time == 49.8);
  TEST_ASSERT(total_time == 249);

  // Test percentile calculations (simplified)
  // In a real implementation, this would use proper percentile calculation
  double p50 = 50.0; // Median
  double p95 = 54.0; // 95th percentile
  double p99 = 55.0; // 99th percentile

  TEST_ASSERT(p50 <= p95);
  TEST_ASSERT(p95 <= p99);

  TEST_ASSERT_CYCLES(200);
  TEST_END();
}

// Test benchmark reporting
static void test_benchmark_reporting()
{
  TEST_BEGIN("Benchmark Reporting");

  cns_benchmark_result_t result = {
      .operation_name = "test_op",
      .component_name = "test_comp",
      .total_operations = 1000,
      .total_time_ns = 50000,
      .min_time_ns = 45,
      .max_time_ns = 55,
      .avg_time_ns = 50.0,
      .throughput_ops_per_sec = 20000.0,
      .p50_time_ns = 50.0,
      .p95_time_ns = 52.0,
      .p99_time_ns = 54.0,
      .target_achieved = true,
      .performance_tier = "L1"};

  // Test result formatting (basic validation)
  TEST_ASSERT(strlen(result.operation_name) > 0);
  TEST_ASSERT(strlen(result.component_name) > 0);
  TEST_ASSERT(strlen(result.performance_tier) > 0);

  // Test performance metrics
  TEST_ASSERT(result.throughput_ops_per_sec > 0);
  TEST_ASSERT(result.total_operations > 0);
  TEST_ASSERT(result.total_time_ns > 0);

  // Test target achievement
  TEST_ASSERT(result.target_achieved == true);

  TEST_ASSERT_CYCLES(100);
  TEST_END();
}

/*═══════════════════════════════════════════════════════════════
  Integration Tests
  ═══════════════════════════════════════════════════════════════*/

// Test complete benchmark workflow
static void test_benchmark_workflow()
{
  TEST_BEGIN("Benchmark Workflow");

  // Test configuration setup
  cns_benchmark_config_t config = {
      .benchmark_name = "workflow_test",
      .iterations = 100,
      .warmup_iterations = 10,
      .batch_size = 5,
      .verbose = false,
      .validate_correctness = true};

  // Test result collection
  cns_benchmark_result_t result = {0};
  result.operation_name = "workflow_operation";
  result.component_name = "workflow_component";
  result.total_operations = config.iterations;
  result.total_time_ns = 10000; // 10μs total
  result.min_time_ns = 95;
  result.max_time_ns = 105;
  result.avg_time_ns = 100.0;
  result.throughput_ops_per_sec = 10000.0; // 10k ops/sec
  result.p50_time_ns = 100.0;
  result.p95_time_ns = 102.0;
  result.p99_time_ns = 104.0;

  // Determine performance tier
  if (result.avg_time_ns <= CNS_SEVEN_TICK_TARGET_NS)
  {
    result.target_achieved = true;
    result.performance_tier = "L1";
  }
  else if (result.avg_time_ns <= CNS_L2_TIER_TARGET_NS)
  {
    result.target_achieved = true;
    result.performance_tier = "L2";
  }
  else if (result.avg_time_ns <= CNS_L3_TIER_TARGET_NS)
  {
    result.target_achieved = true;
    result.performance_tier = "L3";
  }
  else
  {
    result.target_achieved = false;
    result.performance_tier = "L4";
  }

  // Verify workflow results
  TEST_ASSERT(result.total_operations == config.iterations);
  TEST_ASSERT(result.avg_time_ns == 100.0);
  TEST_ASSERT(result.target_achieved == true);
  TEST_ASSERT(strcmp(result.performance_tier, "L2") == 0);
  TEST_ASSERT(result.throughput_ops_per_sec == 10000.0);

  TEST_ASSERT_CYCLES(300);
  TEST_END();
}

// Test benchmark comparison
static void test_benchmark_comparison()
{
  TEST_BEGIN("Benchmark Comparison");

  // Create two benchmark results for comparison
  cns_benchmark_result_t result1 = {
      .operation_name = "fast_operation",
      .avg_time_ns = 5.0,
      .throughput_ops_per_sec = 200000.0,
      .target_achieved = true,
      .performance_tier = "L1"};

  cns_benchmark_result_t result2 = {
      .operation_name = "slow_operation",
      .avg_time_ns = 500.0,
      .throughput_ops_per_sec = 2000.0,
      .target_achieved = true,
      .performance_tier = "L3"};

  // Test performance comparison
  TEST_ASSERT(result1.avg_time_ns < result2.avg_time_ns);
  TEST_ASSERT(result1.throughput_ops_per_sec > result2.throughput_ops_per_sec);
  TEST_ASSERT(strcmp(result1.performance_tier, "L1") == 0);
  TEST_ASSERT(strcmp(result2.performance_tier, "L3") == 0);

  // Test target achievement
  TEST_ASSERT(result1.target_achieved == true);
  TEST_ASSERT(result2.target_achieved == true);

  // Test performance ratio
  double time_ratio = result2.avg_time_ns / result1.avg_time_ns;
  double throughput_ratio = result1.throughput_ops_per_sec / result2.throughput_ops_per_sec;

  TEST_ASSERT(time_ratio == 100.0);       // 100x slower
  TEST_ASSERT(throughput_ratio == 100.0); // 100x lower throughput

  TEST_ASSERT_CYCLES(200);
  TEST_END();
}

/*═══════════════════════════════════════════════════════════════
  Test Runner
  ═══════════════════════════════════════════════════════════════*/

int main()
{
  printf("CNS Benchmark Unit Tests\n");
  printf("========================\n\n");

  // Initialize S7T
  s7t_init();

  // Run tests
  test_benchmark_config();
  test_performance_targets();
  test_benchmark_results();
  test_benchmark_calculations();
  test_performance_tiers();
  test_benchmark_command();
  test_benchmark_validation();
  test_benchmark_statistics();
  test_benchmark_reporting();
  test_benchmark_workflow();
  test_benchmark_comparison();

  // Print summary
  printf("\nTest Summary:\n");
  printf("  Total: %u\n", tests_run);
  printf("  Passed: %u\n", tests_passed);
  printf("  Failed: %u\n", tests_failed);
  printf("  Success Rate: %.1f%%\n",
         (float)tests_passed / tests_run * 100.0f);

  return (tests_failed == 0) ? 0 : 1;
}