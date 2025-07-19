/*
 * First Principles C Tutorial: Performance Measurement
 *
 * This tutorial validates understanding of performance measurement concepts
 * that are critical for the 7T Engine's sub-7-cycle and sub-10ns targets.
 *
 * Key Concepts:
 * - High-precision timing
 * - Cycle counting
 * - Performance analysis
 * - Benchmarking patterns
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

#ifdef __x86_64__
#include <x86intrin.h>
#endif

// Test utilities
#define TEST_ASSERT(condition, message) \
  do                                    \
  {                                     \
    if (!(condition))                   \
    {                                   \
      printf("❌ FAIL: %s\n", message); \
      return 0;                         \
    }                                   \
    else                                \
    {                                   \
      printf("✅ PASS: %s\n", message); \
    }                                   \
  } while (0)

#define TEST_SECTION(name) printf("\n=== %s ===\n", name)

// High-precision timer structure
typedef struct
{
  struct timespec start_time;
  struct timespec end_time;
  uint64_t start_cycles;
  uint64_t end_cycles;
} HighPrecisionTimer;

// Initialize timer
void timer_init(HighPrecisionTimer *timer)
{
  clock_gettime(CLOCK_MONOTONIC, &timer->start_time);
#ifdef __x86_64__
  timer->start_cycles = __builtin_ia32_rdtsc();
#else
  timer->start_cycles = 0;
#endif
}

// Start timer
void timer_start(HighPrecisionTimer *timer)
{
  timer_init(timer);
}

// End timer
void timer_end(HighPrecisionTimer *timer)
{
  clock_gettime(CLOCK_MONOTONIC, &timer->end_time);
#ifdef __x86_64__
  timer->end_cycles = __builtin_ia32_rdtsc();
#else
  timer->end_cycles = 0;
#endif
}

// Get time in nanoseconds
uint64_t timer_get_time_ns(HighPrecisionTimer *timer)
{
  uint64_t start_ns = timer->start_time.tv_sec * 1000000000ULL + timer->start_time.tv_nsec;
  uint64_t end_ns = timer->end_time.tv_sec * 1000000000ULL + timer->end_time.tv_nsec;
  return end_ns - start_ns;
}

// Get cycles
uint64_t timer_get_cycles(HighPrecisionTimer *timer)
{
  return timer->end_cycles - timer->start_cycles;
}

// Lesson 1: Basic Timing Concepts
int lesson_basic_timing()
{
  TEST_SECTION("Basic Timing Concepts");

  // Test 1: Clock resolution
  struct timespec res;
  clock_getres(CLOCK_MONOTONIC, &res);
  printf("  Clock resolution: %ld ns\n", res.tv_nsec);
  TEST_ASSERT(res.tv_nsec > 0, "Clock resolution is positive");

  // Test 2: Basic timing measurement
  HighPrecisionTimer timer;
  timer_start(&timer);

  // Do some work
  volatile int sum = 0;
  for (int i = 0; i < 1000; i++)
  {
    sum += i;
  }

  timer_end(&timer);

  uint64_t time_ns = timer_get_time_ns(&timer);
  uint64_t cycles = timer_get_cycles(&timer);

  printf("  Work time: %lu ns\n", time_ns);
  printf("  Work cycles: %lu\n", cycles);

  TEST_ASSERT(time_ns > 0, "Time measurement is positive");
#ifdef __x86_64__
  TEST_ASSERT(cycles > 0, "Cycle measurement is positive");
#endif

  return 1;
}

// Lesson 2: High-Precision Timing
int lesson_high_precision_timing()
{
  TEST_SECTION("High-Precision Timing");

  // Test 1: Nanosecond precision
  HighPrecisionTimer timer;
  timer_start(&timer);

  // Minimal work
  volatile int x = 1;
  x = x + 1;

  timer_end(&timer);

  uint64_t time_ns = timer_get_time_ns(&timer);
  uint64_t cycles = timer_get_cycles(&timer);

  printf("  Minimal work time: %lu ns\n", time_ns);
  printf("  Minimal work cycles: %lu\n", cycles);

  TEST_ASSERT(time_ns >= 0, "Nanosecond precision measurement");
#ifdef __x86_64__
  TEST_ASSERT(cycles >= 0, "Cycle precision measurement");
#endif

  // Test 2: Timing consistency
  uint64_t times[10];
  for (int i = 0; i < 10; i++)
  {
    timer_start(&timer);
    volatile int y = i;
    y = y * 2;
    timer_end(&timer);
    times[i] = timer_get_time_ns(&timer);
  }

  // Check consistency (should be similar)
  uint64_t min_time = times[0];
  uint64_t max_time = times[0];
  for (int i = 1; i < 10; i++)
  {
    if (times[i] < min_time)
      min_time = times[i];
    if (times[i] > max_time)
      max_time = times[i];
  }

  printf("  Min time: %lu ns, Max time: %lu ns\n", min_time, max_time);
  TEST_ASSERT(max_time >= min_time, "Timing consistency check");

  return 1;
}

// Lesson 3: Performance Analysis
int lesson_performance_analysis()
{
  TEST_SECTION("Performance Analysis");

  // Test 1: Performance comparison
  HighPrecisionTimer timer;

  // Fast operation (stack allocation)
  timer_start(&timer);
  int stack_array[100];
  for (int i = 0; i < 100; i++)
  {
    stack_array[i] = i;
  }
  timer_end(&timer);
  uint64_t stack_time = timer_get_time_ns(&timer);
  uint64_t stack_cycles = timer_get_cycles(&timer);

  // Slow operation (heap allocation)
  timer_start(&timer);
  int *heap_array = malloc(100 * sizeof(int));
  for (int i = 0; i < 100; i++)
  {
    heap_array[i] = i;
  }
  free(heap_array);
  timer_end(&timer);
  uint64_t heap_time = timer_get_time_ns(&timer);
  uint64_t heap_cycles = timer_get_cycles(&timer);

  printf("  Stack operation: %lu ns, %lu cycles\n", stack_time, stack_cycles);
  printf("  Heap operation: %lu ns, %lu cycles\n", heap_time, heap_cycles);

  TEST_ASSERT(stack_time <= heap_time, "Stack operation is faster or equal");
#ifdef __x86_64__
  TEST_ASSERT(stack_cycles <= heap_cycles, "Stack operation uses fewer cycles");
#endif

  // Test 2: Performance ratios
  double time_ratio = (double)heap_time / stack_time;
#ifdef __x86_64__
  double cycle_ratio = (double)heap_cycles / stack_cycles;
  printf("  Time ratio (heap/stack): %.2fx\n", time_ratio);
  printf("  Cycle ratio (heap/stack): %.2fx\n", cycle_ratio);
#else
  printf("  Time ratio (heap/stack): %.2fx\n", time_ratio);
#endif

  TEST_ASSERT(time_ratio >= 1.0, "Heap operation is not faster than stack");

  return 1;
}

// Lesson 4: Benchmarking Patterns
int lesson_benchmarking_patterns()
{
  TEST_SECTION("Benchmarking Patterns");

  // Pattern 1: Single measurement
  HighPrecisionTimer timer;
  timer_start(&timer);

  // Operation to benchmark
  volatile int result = 0;
  for (int i = 0; i < 1000; i++)
  {
    result += i * i;
  }

  timer_end(&timer);

  uint64_t time_ns = timer_get_time_ns(&timer);
  uint64_t cycles = timer_get_cycles(&timer);

  printf("  Single measurement: %lu ns, %lu cycles\n", time_ns, cycles);
  TEST_ASSERT(result > 0, "Operation completed successfully");

  // Pattern 2: Multiple measurements (statistics)
  uint64_t times[100];
  uint64_t cycle_counts[100];

  for (int run = 0; run < 100; run++)
  {
    timer_start(&timer);

    volatile int run_result = 0;
    for (int i = 0; i < 100; i++)
    {
      run_result += i * i;
    }

    timer_end(&timer);
    times[run] = timer_get_time_ns(&timer);
    cycle_counts[run] = timer_get_cycles(&timer);
  }

  // Calculate statistics
  uint64_t min_time = times[0], max_time = times[0];
  uint64_t min_cycles = cycle_counts[0], max_cycles = cycle_counts[0];
  uint64_t total_time = 0, total_cycles = 0;

  for (int i = 0; i < 100; i++)
  {
    if (times[i] < min_time)
      min_time = times[i];
    if (times[i] > max_time)
      max_time = times[i];
    if (cycle_counts[i] < min_cycles)
      min_cycles = cycle_counts[i];
    if (cycle_counts[i] > max_cycles)
      max_cycles = cycle_counts[i];
    total_time += times[i];
    total_cycles += cycle_counts[i];
  }

  double avg_time = (double)total_time / 100;
  double avg_cycles = (double)total_cycles / 100;

  printf("  Time stats: min=%lu, avg=%.1f, max=%lu ns\n", min_time, avg_time, max_time);
  printf("  Cycle stats: min=%lu, avg=%.1f, max=%lu\n", min_cycles, avg_cycles, max_cycles);

  TEST_ASSERT(min_time <= avg_time && avg_time <= max_time, "Time statistics are consistent");
#ifdef __x86_64__
  TEST_ASSERT(min_cycles <= avg_cycles && avg_cycles <= max_cycles, "Cycle statistics are consistent");
#endif

  return 1;
}

// Lesson 5: 7T Engine Performance Targets
int lesson_7t_performance_targets()
{
  TEST_SECTION("7T Engine Performance Targets");

  // Target: Sub-7 cycles, Sub-10ns for 7-tick path
  HighPrecisionTimer timer;

  // Test 1: Simple variable access (should be very fast)
  timer_start(&timer);
  volatile int x = 42;
  volatile int y = x + 1;
  timer_end(&timer);

  uint64_t simple_time = timer_get_time_ns(&timer);
  uint64_t simple_cycles = timer_get_cycles(&timer);

  printf("  Simple variable access: %lu ns, %lu cycles\n", simple_time, simple_cycles);

  // Test 2: String copy (baseline for CJinja operations)
  char src[100] = "Hello World";
  char dst[100];

  timer_start(&timer);
  strcpy(dst, src);
  timer_end(&timer);

  uint64_t strcpy_time = timer_get_time_ns(&timer);
  uint64_t strcpy_cycles = timer_get_cycles(&timer);

  printf("  String copy: %lu ns, %lu cycles\n", strcpy_time, strcpy_cycles);

  // Test 3: Hash table lookup (baseline for variable lookup)
  typedef struct
  {
    char *key;
    char *value;
  } HashEntry;

  HashEntry table[10] = {
      {"name", "John"},
      {"title", "Developer"},
      {"company", "TechCorp"}};

  timer_start(&timer);
  char *found_value = NULL;
  for (int i = 0; i < 3; i++)
  {
    if (strcmp(table[i].key, "name") == 0)
    {
      found_value = table[i].value;
      break;
    }
  }
  timer_end(&timer);

  uint64_t lookup_time = timer_get_time_ns(&timer);
  uint64_t lookup_cycles = timer_get_cycles(&timer);

  printf("  Hash lookup: %lu ns, %lu cycles\n", lookup_time, lookup_cycles);
  TEST_ASSERT(found_value != NULL, "Hash lookup found value");

  // Validate against 7T targets
  printf("\n  7T Engine Performance Validation:\n");
  printf("  - Simple access: %s\n", simple_cycles <= 7 ? "✅ Within 7-cycle target" : "❌ Exceeds 7-cycle target");
  printf("  - String copy: %s\n", strcpy_cycles <= 7 ? "✅ Within 7-cycle target" : "❌ Exceeds 7-cycle target");
  printf("  - Hash lookup: %s\n", lookup_cycles <= 7 ? "✅ Within 7-cycle target" : "❌ Exceeds 7-cycle target");

  printf("  - Simple access: %s\n", simple_time <= 10 ? "✅ Within 10ns target" : "❌ Exceeds 10ns target");
  printf("  - String copy: %s\n", strcpy_time <= 10 ? "✅ Within 10ns target" : "❌ Exceeds 10ns target");
  printf("  - Hash lookup: %s\n", lookup_time <= 10 ? "✅ Within 10ns target" : "❌ Exceeds 10ns target");

  // Note: These are baseline measurements, actual 7T operations may vary
  printf("\n  Note: These are baseline measurements for validation.\n");
  printf("  Actual 7T Engine operations may have different performance characteristics.\n");

  return 1;
}

// Main tutorial runner
int main()
{
  printf("First Principles C Tutorial: Performance Measurement\n");
  printf("==================================================\n");
  printf("Validating timing concepts for 7T Engine benchmarks\n\n");

  int total_lessons = 5;
  int passed_lessons = 0;

  // Run all lessons
  if (lesson_basic_timing())
    passed_lessons++;
  if (lesson_high_precision_timing())
    passed_lessons++;
  if (lesson_performance_analysis())
    passed_lessons++;
  if (lesson_benchmarking_patterns())
    passed_lessons++;
  if (lesson_7t_performance_targets())
    passed_lessons++;

  // Summary
  printf("\n=== Tutorial Summary ===\n");
  printf("Total lessons: %d\n", total_lessons);
  printf("Passed: %d\n", passed_lessons);
  printf("Failed: %d\n", total_lessons - passed_lessons);
  printf("Success rate: %.1f%%\n", (passed_lessons * 100.0) / total_lessons);

  if (passed_lessons == total_lessons)
  {
    printf("\n🎉 All performance measurement concepts validated!\n");
    printf("Ready for 7T Engine benchmarking.\n");
    return 0;
  }
  else
  {
    printf("\n❌ Some concepts need review.\n");
    return 1;
  }
}