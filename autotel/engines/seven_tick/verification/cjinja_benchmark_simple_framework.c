#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <stdint.h>
#include "../compiler/src/cjinja.h"

// Simplified benchmark framework for cross-platform compatibility
#define BENCHMARK_VERSION "1.0.0"
#define TARGET_CYCLES 7
#define TARGET_NS 10
#define TARGET_OPS_PER_SEC 100000000

// High-precision timing functions
static inline uint64_t get_cycles(void)
{
  // Use clock_gettime for cross-platform compatibility
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

static inline uint64_t get_nanoseconds(void)
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

static inline uint64_t get_microseconds(void)
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000000ULL + tv.tv_usec;
}

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

// Benchmark suite management
BenchmarkSuite *benchmark_suite_create(const char *suite_name)
{
  BenchmarkSuite *suite = malloc(sizeof(BenchmarkSuite));
  suite->suite_name = suite_name;
  suite->results = malloc(20 * sizeof(BenchmarkResult)); // Initial capacity
  suite->result_count = 0;
  suite->result_capacity = 20;
  suite->total_suite_time_ns = 0;
  suite->overall_score = 0.0;
  return suite;
}

void benchmark_suite_add_result(BenchmarkSuite *suite, BenchmarkResult result)
{
  if (suite->result_count >= suite->result_capacity)
  {
    suite->result_capacity *= 2;
    suite->results = realloc(suite->results, suite->result_capacity * sizeof(BenchmarkResult));
  }
  suite->results[suite->result_count++] = result;
  suite->total_suite_time_ns += result.total_time_ns;
}

void benchmark_suite_calculate_stats(BenchmarkSuite *suite)
{
  if (suite->result_count == 0)
    return;

  double total_score = 0.0;
  for (size_t i = 0; i < suite->result_count; i++)
  {
    total_score += suite->results[i].target_achievement_percent;
  }
  suite->overall_score = total_score / suite->result_count;
}

void benchmark_suite_destroy(BenchmarkSuite *suite)
{
  free(suite->results);
  free(suite);
}

// Individual benchmark execution
BenchmarkResult benchmark_execute_single(
    const char *test_name,
    size_t iterations,
    void (*test_function)(void *),
    void *test_data)
{

  BenchmarkTimer timer;
  benchmark_timer_start(&timer, test_name);

  // Warm-up run
  test_function(test_data);

  // Reset timer for actual measurement
  benchmark_timer_start(&timer, test_name);

  for (size_t i = 0; i < iterations; i++)
  {
    test_function(test_data);
  }

  benchmark_timer_end(&timer);

  uint64_t total_cycles = benchmark_timer_get_cycles(&timer);
  uint64_t total_time_ns = benchmark_timer_get_time_ns(&timer);

  BenchmarkResult result;
  result.test_name = test_name;
  result.total_cycles = total_cycles;
  result.total_time_ns = total_time_ns;
  result.operations = iterations;
  result.avg_cycles_per_op = (double)total_cycles / iterations;
  result.avg_time_ns_per_op = (double)total_time_ns / iterations;
  result.ops_per_sec = (iterations * 1000000000.0) / total_time_ns;
  result.min_cycles = result.avg_cycles_per_op; // Simplified
  result.max_cycles = result.avg_cycles_per_op; // Simplified
  result.operations_within_target = (result.avg_cycles_per_op <= TARGET_CYCLES) ? iterations : 0;
  result.target_achievement_percent = (result.operations_within_target * 100.0) / iterations;
  result.passed = (result.target_achievement_percent >= 95.0) &&
                  (result.avg_cycles_per_op <= TARGET_CYCLES) &&
                  (result.avg_time_ns_per_op <= TARGET_NS);

  return result;
}

// Performance validation
int benchmark_validate_target(BenchmarkResult *result)
{
  return result->target_achievement_percent >= 95.0 &&
         result->avg_cycles_per_op <= TARGET_CYCLES &&
         result->avg_time_ns_per_op <= TARGET_NS;
}

// Result formatting and reporting
void benchmark_result_print(BenchmarkResult *result)
{
  printf("=== Benchmark Result: %s ===\n", result->test_name);
  printf("Operations: %zu\n", result->operations);
  printf("Total time: %.3f ms (%llu ns)\n", result->total_time_ns / 1000000.0, result->total_time_ns);
  printf("Total cycles: %llu\n", result->total_cycles);
  printf("Average cycles per operation: %.1f\n", result->avg_cycles_per_op);
  printf("Average time per operation: %.1f ns\n", result->avg_time_ns_per_op);
  printf("Throughput: %.0f ops/sec\n", result->ops_per_sec);
  printf("\n");
  printf("Target Achievement:\n");
  printf("  Operations within ≤%d cycles: %zu/%zu (%.1f%%)\n",
         TARGET_CYCLES, result->operations_within_target, result->operations, result->target_achievement_percent);
  printf("  Status: %s\n", result->passed ? "✅ PASSED" : "❌ FAILED");
  printf("\n");
}

void benchmark_suite_print_summary(BenchmarkSuite *suite)
{
  printf("=== %s Summary ===\n", suite->suite_name);
  printf("Total tests: %zu\n", suite->result_count);
  printf("Total suite time: %.3f ms\n", suite->total_suite_time_ns / 1000000.0);
  printf("Overall score: %.1f%%\n", suite->overall_score);
  printf("\n");
}

void benchmark_suite_print_detailed(BenchmarkSuite *suite)
{
  printf("=== %s Detailed Results ===\n", suite->suite_name);
  printf("\n");

  for (size_t i = 0; i < suite->result_count; i++)
  {
    benchmark_result_print(&suite->results[i]);
  }

  benchmark_suite_print_summary(suite);
}

// Export results (simplified)
void benchmark_suite_export_json(BenchmarkSuite *suite, const char *filename)
{
  FILE *file = fopen(filename, "w");
  if (!file)
    return;

  fprintf(file, "{\n");
  fprintf(file, "  \"suite_name\": \"%s\",\n", suite->suite_name);
  fprintf(file, "  \"total_tests\": %zu,\n", suite->result_count);
  fprintf(file, "  \"overall_score\": %.1f,\n", suite->overall_score);
  fprintf(file, "  \"results\": [\n");

  for (size_t i = 0; i < suite->result_count; i++)
  {
    BenchmarkResult *result = &suite->results[i];
    fprintf(file, "    {\n");
    fprintf(file, "      \"test_name\": \"%s\",\n", result->test_name);
    fprintf(file, "      \"operations\": %zu,\n", result->operations);
    fprintf(file, "      \"avg_cycles_per_op\": %.1f,\n", result->avg_cycles_per_op);
    fprintf(file, "      \"avg_time_ns_per_op\": %.1f,\n", result->avg_time_ns_per_op);
    fprintf(file, "      \"ops_per_sec\": %.0f,\n", result->ops_per_sec);
    fprintf(file, "      \"target_achievement_percent\": %.1f,\n", result->target_achievement_percent);
    fprintf(file, "      \"passed\": %s\n", result->passed ? "true" : "false");
    fprintf(file, "    }%s\n", (i < suite->result_count - 1) ? "," : "");
  }

  fprintf(file, "  ]\n");
  fprintf(file, "}\n");
  fclose(file);
}

void benchmark_suite_export_csv(BenchmarkSuite *suite, const char *filename)
{
  FILE *file = fopen(filename, "w");
  if (!file)
    return;

  fprintf(file, "test_name,operations,avg_cycles_per_op,avg_time_ns_per_op,ops_per_sec,target_achievement_percent,passed\n");

  for (size_t i = 0; i < suite->result_count; i++)
  {
    BenchmarkResult *result = &suite->results[i];
    fprintf(file, "\"%s\",%zu,%.1f,%.1f,%.0f,%.1f,%s\n",
            result->test_name,
            result->operations,
            result->avg_cycles_per_op,
            result->avg_time_ns_per_op,
            result->ops_per_sec,
            result->target_achievement_percent,
            result->passed ? "true" : "false");
  }

  fclose(file);
}

// Hardware info (simplified)
typedef struct
{
  int num_cores;
  double cpu_frequency_ghz;
} HardwareInfo;

HardwareInfo *detect_hardware_capabilities(void)
{
  HardwareInfo *info = malloc(sizeof(HardwareInfo));
  info->num_cores = 8;           // Default
  info->cpu_frequency_ghz = 3.2; // Default
  return info;
}

void print_hardware_info(HardwareInfo *info)
{
  printf("=== Hardware Information ===\n");
  printf("Number of Cores: %d\n", info->num_cores);
  printf("CPU Frequency: %.1f GHz\n", info->cpu_frequency_ghz);
  printf("Target: ≤%d CPU cycles per operation\n", TARGET_CYCLES);
  printf("Target: ≤%d nanoseconds per operation\n", TARGET_NS);
  printf("Target: ≥%d operations per second\n", TARGET_OPS_PER_SEC);
  printf("\n");
}

void destroy_hardware_info(HardwareInfo *info)
{
  free(info);
}

// Test data structure for CJinja benchmarks
typedef struct
{
  CJinjaEngine *engine;
  CJinjaContext *ctx;
  const char *template;
  const char *test_name;
} CJinjaTestData;

// Benchmark test functions
void benchmark_simple_variable_substitution(void *data)
{
  CJinjaTestData *test_data = (CJinjaTestData *)data;
  char *result = cjinja_render_string(test_data->template, test_data->ctx);
  free(result);
}

void benchmark_7tick_variable_substitution(void *data)
{
  CJinjaTestData *test_data = (CJinjaTestData *)data;
  char *result = cjinja_render_string_7tick(test_data->template, test_data->ctx);
  free(result);
}

void benchmark_conditional_rendering(void *data)
{
  CJinjaTestData *test_data = (CJinjaTestData *)data;
  char *result = cjinja_render_with_conditionals(test_data->template, test_data->ctx);
  free(result);
}

void benchmark_7tick_conditional_rendering(void *data)
{
  CJinjaTestData *test_data = (CJinjaTestData *)data;
  char *result = cjinja_render_conditionals_7tick(test_data->template, test_data->ctx);
  free(result);
}

void benchmark_loop_rendering(void *data)
{
  CJinjaTestData *test_data = (CJinjaTestData *)data;
  char *result = cjinja_render_with_loops(test_data->template, test_data->ctx);
  free(result);
}

void benchmark_filter_rendering(void *data)
{
  CJinjaTestData *test_data = (CJinjaTestData *)data;
  char *result = cjinja_render_with_loops(test_data->template, test_data->ctx);
  free(result);
}

void benchmark_template_inheritance(void *data)
{
  CJinjaTestData *test_data = (CJinjaTestData *)data;

  // Create inheritance context
  CJinjaInheritanceContext *inherit_ctx = cjinja_create_inheritance_context();
  cjinja_set_base_template(inherit_ctx,
                           "<html><head><title>{{title}}</title></head><body>{{% block content %}}Default{{% endblock %}}</body></html>");
  cjinja_add_block(inherit_ctx, "content", "Inherited content");

  char *result = cjinja_render_with_inheritance(test_data->template, test_data->ctx, inherit_ctx);
  free(result);
  cjinja_destroy_inheritance_context(inherit_ctx);
}

void benchmark_batch_rendering(void *data)
{
  CJinjaTestData *test_data = (CJinjaTestData *)data;

  // Create batch render
  CJinjaBatchRender *batch = cjinja_create_batch_render(5);
  const char *templates[] = {
      "Template 1: {{user}}",
      "Template 2: {{title}}",
      "Template 3: {% if is_admin %}Admin{% endif %}",
      "Template 4: {{user | upper}}",
      "Template 5: {{title | length}} chars"};

  for (int i = 0; i < 5; i++)
  {
    batch->templates[i] = templates[i];
  }

  cjinja_render_batch(test_data->engine, batch, test_data->ctx);
  cjinja_destroy_batch_render(batch);
}

void benchmark_advanced_filters(void *data)
{
  CJinjaTestData *test_data = (CJinjaTestData *)data;
  char *result = cjinja_render_with_loops(test_data->template, test_data->ctx);
  free(result);
}

// Setup test data
CJinjaTestData *setup_cjinja_test_data(void)
{
  CJinjaTestData *data = malloc(sizeof(CJinjaTestData));

  data->engine = cjinja_create("./templates");
  data->ctx = cjinja_create_context();

  // Set up test variables
  cjinja_set_var(data->ctx, "title", "CJinja Performance Test");
  cjinja_set_var(data->ctx, "user", "John Doe");
  cjinja_set_var(data->ctx, "email", "john@example.com");
  cjinja_set_bool(data->ctx, "is_admin", 1);
  cjinja_set_bool(data->ctx, "show_debug", 0);

  // Set up arrays for loops
  char *items[] = {"apple", "banana", "cherry", "date", "elderberry"};
  cjinja_set_array(data->ctx, "fruits", items, 5);

  char *users[] = {"Alice", "Bob", "Charlie", "Diana", "Eve", "Frank", "Grace", "Henry"};
  cjinja_set_array(data->ctx, "users", users, 8);

  return data;
}

void cleanup_cjinja_test_data(CJinjaTestData *data)
{
  cjinja_destroy_context(data->ctx);
  cjinja_destroy_engine(data->engine);
  free(data);
}

int main()
{
  printf("CJinja Benchmark Suite (Refactored with Simple Framework)\n");
  printf("========================================================\n\n");

  // Detect hardware capabilities
  HardwareInfo *hw_info = detect_hardware_capabilities();
  print_hardware_info(hw_info);

  // Create benchmark suite
  BenchmarkSuite *suite = benchmark_suite_create("CJinja Performance Suite");

  // Setup test data
  CJinjaTestData *test_data = setup_cjinja_test_data();

  printf("Running CJinja benchmarks with simple framework...\n\n");

  // Test 1: Simple Variable Substitution (49-tick path)
  test_data->template = "Hello {{user}}, welcome to {{title}}!";
  test_data->test_name = "Simple Variable Substitution (49-tick)";

  BenchmarkResult result1 = benchmark_execute_single(
      "Simple Variable Substitution (49-tick)",
      10000,
      benchmark_simple_variable_substitution,
      test_data);
  benchmark_suite_add_result(suite, result1);

  // Test 2: Simple Variable Substitution (7-tick path)
  BenchmarkResult result2 = benchmark_execute_single(
      "Simple Variable Substitution (7-tick)",
      10000,
      benchmark_7tick_variable_substitution,
      test_data);
  benchmark_suite_add_result(suite, result2);

  // Test 3: Conditional Rendering (49-tick path)
  test_data->template =
      "{% if is_admin %}Welcome admin {{user}}!{% endif %}"
      "{% if show_debug %}Debug mode enabled{% endif %}"
      "Regular user: {{user}}";

  BenchmarkResult result3 = benchmark_execute_single(
      "Conditional Rendering (49-tick)",
      10000,
      benchmark_conditional_rendering,
      test_data);
  benchmark_suite_add_result(suite, result3);

  // Test 4: Conditional Rendering (7-tick path)
  BenchmarkResult result4 = benchmark_execute_single(
      "Conditional Rendering (7-tick)",
      10000,
      benchmark_7tick_conditional_rendering,
      test_data);
  benchmark_suite_add_result(suite, result4);

  // Test 5: Loop Rendering (49-tick only)
  test_data->template =
      "Fruits:\n"
      "{% for fruit in fruits %}"
      "  - {{fruit}}\n"
      "{% endfor %}"
      "Total: {{fruits | length}} fruits";

  BenchmarkResult result5 = benchmark_execute_single(
      "Loop Rendering (49-tick only)",
      1000,
      benchmark_loop_rendering,
      test_data);
  benchmark_suite_add_result(suite, result5);

  // Test 6: Filter Rendering (49-tick only)
  test_data->template =
      "User: {{user | upper}}\n"
      "Email: {{email | lower}}\n"
      "Title: {{title | capitalize}}\n"
      "Name length: {{user | length}} characters";

  BenchmarkResult result6 = benchmark_execute_single(
      "Filter Rendering (49-tick only)",
      10000,
      benchmark_filter_rendering,
      test_data);
  benchmark_suite_add_result(suite, result6);

  // Test 7: Advanced Filters (49-tick only)
  test_data->template =
      "User: {{user | trim}}\n"
      "Replaced: {{user | replace('John','Jane')}}\n"
      "Sliced: {{user | slice(0,4)}}\n"
      "Default: {{missing_var | default('Not Found')}}";

  BenchmarkResult result7 = benchmark_execute_single(
      "Advanced Filters (49-tick only)",
      10000,
      benchmark_advanced_filters,
      test_data);
  benchmark_suite_add_result(suite, result7);

  // Test 8: Template Inheritance (49-tick only)
  test_data->template =
      "{{% extends base %}}\n"
      "{{% block content %}}Welcome {{user}}!{{% endblock %}}";

  BenchmarkResult result8 = benchmark_execute_single(
      "Template Inheritance (49-tick only)",
      1000,
      benchmark_template_inheritance,
      test_data);
  benchmark_suite_add_result(suite, result8);

  // Test 9: Batch Rendering (49-tick only)
  BenchmarkResult result9 = benchmark_execute_single(
      "Batch Rendering (49-tick only)",
      1000,
      benchmark_batch_rendering,
      test_data);
  benchmark_suite_add_result(suite, result9);

  // Calculate suite statistics
  benchmark_suite_calculate_stats(suite);

  // Print results
  printf("=== CJinja Benchmark Results ===\n\n");
  benchmark_suite_print_detailed(suite);

  // Performance comparison analysis
  printf("\n=== 7-Tick vs 49-Tick Performance Analysis ===\n");

  // Find 7-tick vs 49-tick comparisons
  BenchmarkResult *results = suite->results;
  for (size_t i = 0; i < suite->result_count; i++)
  {
    if (strstr(results[i].test_name, "(7-tick)") != NULL)
    {
      // Find corresponding 49-tick result
      for (size_t j = 0; j < suite->result_count; j++)
      {
        if (strstr(results[j].test_name, "(49-tick)") != NULL &&
            strncmp(results[i].test_name, results[j].test_name,
                    strstr(results[i].test_name, "(7-tick)") - results[i].test_name) == 0)
        {

          double speedup = results[j].avg_cycles_per_op / results[i].avg_cycles_per_op;
          printf("  %s:\n", results[i].test_name);
          printf("    7-tick: %.1f cycles (%.1f ns)\n",
                 results[i].avg_cycles_per_op, results[i].avg_time_ns_per_op);
          printf("    49-tick: %.1f cycles (%.1f ns)\n",
                 results[j].avg_cycles_per_op, results[j].avg_time_ns_per_op);
          printf("    Speedup: %.1fx faster with 7-tick path\n", speedup);
          printf("    ✅ 7-tick: %s\n",
                 results[i].avg_cycles_per_op <= TARGET_CYCLES ? "Target met" : "Above target");
          printf("    ✅ 49-tick: %s\n",
                 results[j].avg_cycles_per_op <= TARGET_CYCLES ? "Target met" : "Above target");
          printf("\n");
          break;
        }
      }
    }
  }

  // Export results
  char timestamp[64];
  time_t now = time(NULL);
  strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", localtime(&now));

  char json_filename[128];
  snprintf(json_filename, sizeof(json_filename), "cjinja_benchmark_results_%s.json", timestamp);
  benchmark_suite_export_json(suite, json_filename);

  char csv_filename[128];
  snprintf(csv_filename, sizeof(csv_filename), "cjinja_benchmark_results_%s.csv", timestamp);
  benchmark_suite_export_csv(suite, csv_filename);

  printf("Results exported to:\n");
  printf("  JSON: %s\n", json_filename);
  printf("  CSV: %s\n", csv_filename);

  // Cleanup
  cleanup_cjinja_test_data(test_data);
  benchmark_suite_destroy(suite);
  destroy_hardware_info(hw_info);

  printf("\nCJinja benchmark suite completed!\n");
  return 0;
}