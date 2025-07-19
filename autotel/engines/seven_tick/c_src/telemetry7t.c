#include "telemetry7t.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdatomic.h>

// Global telemetry context (lock-free for 7-tick performance)
static Telemetry7TContext global_context;
static atomic_flag global_context_lock = ATOMIC_FLAG_INIT;

// Initialize global telemetry context
void telemetry7t_global_init(void)
{
  telemetry7t_init_context(&global_context);
  // Note: telemetry is disabled by default for performance
}

// Enable telemetry globally (lock-free)
void telemetry7t_enable(void)
{
  global_context.enabled = 1;
}

// Disable telemetry globally (lock-free)
void telemetry7t_disable(void)
{
  global_context.enabled = 0;
}

// Check if telemetry is enabled (lock-free)
int telemetry7t_is_enabled(void)
{
  return global_context.enabled;
}

// Get global telemetry context (lock-free)
Telemetry7TContext *telemetry7t_get_global_context(void)
{
  return &global_context;
}

// Create span with global context (7-tick optimized)
Telemetry7TSpan *telemetry7t_span_create(
    const char *name,
    const char *operation,
    uint8_t kind)
{
  return telemetry7t_create_span(&global_context, name, operation, kind);
}

// Start span with global context (7-tick optimized)
void telemetry7t_span_start(Telemetry7TSpan *span)
{
  telemetry7t_start_span(&global_context, span);
}

// End span with global context (7-tick optimized)
void telemetry7t_span_end(Telemetry7TSpan *span, uint8_t status)
{
  telemetry7t_end_span(&global_context, span, status);
}

// Get current span from global context (7-tick optimized)
Telemetry7TSpan *telemetry7t_get_current_span_global(void)
{
  return telemetry7t_get_current_span(&global_context);
}

// High-performance span creation with automatic timing (7-tick optimized)
Telemetry7TSpan *telemetry7t_span_begin(
    const char *name,
    const char *operation,
    uint8_t kind)
{
  Telemetry7TSpan *span = telemetry7t_span_create(name, operation, kind);
  if (span)
  {
    telemetry7t_span_start(span);
  }
  return span;
}

// High-performance span ending with automatic timing (7-tick optimized)
void telemetry7t_span_finish(Telemetry7TSpan *span, uint8_t status)
{
  if (span)
  {
    telemetry7t_span_end(span, status);
    telemetry7t_free_span(span);
  }
}

// Add performance metrics to span
void telemetry7t_span_add_performance_metrics(Telemetry7TSpan *span)
{
  if (!span)
    return;

  uint64_t duration_ns = telemetry7t_get_span_duration_ns(span);

  // Add duration as attribute
  char duration_str[32];
  snprintf(duration_str, sizeof(duration_str), "%llu", (unsigned long long)duration_ns);
  telemetry7t_add_attribute_string(span, "duration_ns", duration_str);

  // Add performance classification
  if (duration_ns < 1000)
  {
    telemetry7t_add_attribute_string(span, "performance_class", "sub_microsecond");
  }
  else if (duration_ns < 1000000)
  {
    telemetry7t_add_attribute_string(span, "performance_class", "sub_millisecond");
  }
  else
  {
    telemetry7t_add_attribute_string(span, "performance_class", "millisecond_plus");
  }
}

// Add 7T-specific metrics
void telemetry7t_span_add_7t_metrics(Telemetry7TSpan *span, const char *operation_type)
{
  if (!span)
    return;

  telemetry7t_add_attribute_string(span, "engine", "7T");
  telemetry7t_add_attribute_string(span, "operation_type", operation_type);

  // Add 7T performance targets
  if (strcmp(operation_type, "shacl_validation") == 0)
  {
    telemetry7t_add_attribute_string(span, "target_cycles", "7");
    telemetry7t_add_attribute_string(span, "target_latency_ns", "10");
  }
  else if (strcmp(operation_type, "template_rendering") == 0)
  {
    telemetry7t_add_attribute_string(span, "target_latency_ns", "1000");
  }
}

// Create SHACL validation span
Telemetry7TSpan *telemetry7t_shacl_span_begin(const char *constraint_type)
{
  Telemetry7TSpan *span = telemetry7t_span_begin(
      "shacl_validation",
      constraint_type,
      TELEMETRY7T_KIND_INTERNAL);

  if (span)
  {
    telemetry7t_span_add_7t_metrics(span, "shacl_validation");
    telemetry7t_add_attribute_string(span, "constraint_type", constraint_type);
  }

  return span;
}

// Create template rendering span
Telemetry7TSpan *telemetry7t_template_span_begin(const char *template_type)
{
  Telemetry7TSpan *span = telemetry7t_span_begin(
      "template_rendering",
      template_type,
      TELEMETRY7T_KIND_INTERNAL);

  if (span)
  {
    telemetry7t_span_add_7t_metrics(span, "template_rendering");
    telemetry7t_add_attribute_string(span, "template_type", template_type);
  }

  return span;
}

// Create pattern matching span
Telemetry7TSpan *telemetry7t_pattern_span_begin(const char *pattern_type)
{
  Telemetry7TSpan *span = telemetry7t_span_begin(
      "pattern_matching",
      pattern_type,
      TELEMETRY7T_KIND_INTERNAL);

  if (span)
  {
    telemetry7t_span_add_7t_metrics(span, "pattern_matching");
    telemetry7t_add_attribute_string(span, "pattern_type", pattern_type);
  }

  return span;
}

// Print span information (for debugging)
void telemetry7t_span_print(Telemetry7TSpan *span)
{
  if (!span)
    return;

  uint64_t duration_ns = telemetry7t_get_span_duration_ns(span);

  printf("7T Span: %s (%s)\n", span->name, span->operation);
  printf("  Span ID: %llu\n", (unsigned long long)span->span_id);
  printf("  Trace ID: %llu\n", (unsigned long long)span->trace_id);
  printf("  Parent: %llu\n", (unsigned long long)span->parent_span_id);
  printf("  Duration: %llu ns (%.3f μs)\n",
         (unsigned long long)duration_ns, duration_ns / 1000.0);
  printf("  Status: %d\n", span->status);
  printf("  Kind: %d\n", span->kind);
  printf("  Attributes: %u\n", span->attributes_count);
  printf("  Events: %u\n", span->events_count);
  printf("  Active: %s\n", telemetry7t_is_span_active(span) ? "Yes" : "No");
}

// Export span data for external systems
void telemetry7t_span_export_json(Telemetry7TSpan *span, char *buffer, size_t buffer_size)
{
  if (!span || !buffer)
    return;

  uint64_t duration_ns = telemetry7t_get_span_duration_ns(span);

  snprintf(buffer, buffer_size,
           "{"
           "\"span_id\":\"%llu\","
           "\"trace_id\":\"%llu\","
           "\"parent_span_id\":\"%llu\","
           "\"name\":\"%s\","
           "\"operation\":\"%s\","
           "\"start_time_ns\":%llu,"
           "\"end_time_ns\":%llu,"
           "\"duration_ns\":%llu,"
           "\"status\":%d,"
           "\"kind\":%d,"
           "\"attributes_count\":%u,"
           "\"events_count\":%u,"
           "\"engine\":\"7T\","
           "\"active\":%s"
           "}",
           (unsigned long long)span->span_id,
           (unsigned long long)span->trace_id,
           (unsigned long long)span->parent_span_id,
           span->name ? span->name : "",
           span->operation ? span->operation : "",
           (unsigned long long)span->start_time_ns,
           (unsigned long long)span->end_time_ns,
           (unsigned long long)duration_ns,
           span->status,
           span->kind,
           span->attributes_count,
           span->events_count,
           telemetry7t_is_span_active(span) ? "true" : "false");
}

// Performance benchmarking for telemetry system
void telemetry7t_benchmark(void)
{
  printf("7T Telemetry System Benchmark\n");
  printf("=============================\n\n");

  // Initialize telemetry
  telemetry7t_global_init();

  const int iterations = 1000000;

  // Benchmark span creation
  uint64_t start_time = telemetry7t_get_nanoseconds();

  for (int i = 0; i < iterations; i++)
  {
    Telemetry7TSpan *span = telemetry7t_span_begin("test_span", "test_operation", TELEMETRY7T_KIND_INTERNAL);
    telemetry7t_span_finish(span, TELEMETRY7T_STATUS_OK);
  }

  uint64_t end_time = telemetry7t_get_nanoseconds();
  uint64_t total_time = end_time - start_time;

  double avg_ns = (double)total_time / iterations;
  double ops_per_sec = (iterations * 1000000000.0) / total_time;

  printf("Span Creation Performance:\n");
  printf("  Iterations: %d\n", iterations);
  printf("  Total time: %llu ns\n", (unsigned long long)total_time);
  printf("  Average per span: %.2f ns\n", avg_ns);
  printf("  Throughput: %.0f spans/sec\n", ops_per_sec);

  if (avg_ns < 1000.0)
  {
    printf("🎉 ACHIEVING SUB-MICROSECOND SPAN CREATION!\n");
  }
  else if (avg_ns < 10000.0)
  {
    printf("✅ ACHIEVING SUB-10μs SPAN CREATION!\n");
  }
  else
  {
    printf("⚠️ Span creation above 10μs\n");
  }

  // Benchmark SHACL validation spans
  start_time = telemetry7t_get_nanoseconds();

  for (int i = 0; i < iterations; i++)
  {
    Telemetry7TSpan *span = telemetry7t_shacl_span_begin("min_count");
    telemetry7t_span_finish(span, TELEMETRY7T_STATUS_OK);
  }

  end_time = telemetry7t_get_nanoseconds();
  total_time = end_time - start_time;

  avg_ns = (double)total_time / iterations;
  ops_per_sec = (iterations * 1000000000.0) / total_time;

  printf("\nSHACL Validation Span Performance:\n");
  printf("  Average per span: %.2f ns\n", avg_ns);
  printf("  Throughput: %.0f spans/sec\n", ops_per_sec);

  // Benchmark template rendering spans
  start_time = telemetry7t_get_nanoseconds();

  for (int i = 0; i < iterations; i++)
  {
    Telemetry7TSpan *span = telemetry7t_template_span_begin("variable_substitution");
    telemetry7t_span_finish(span, TELEMETRY7T_STATUS_OK);
  }

  end_time = telemetry7t_get_nanoseconds();
  total_time = end_time - start_time;

  avg_ns = (double)total_time / iterations;
  ops_per_sec = (iterations * 1000000000.0) / total_time;

  printf("\nTemplate Rendering Span Performance:\n");
  printf("  Average per span: %.2f ns\n", avg_ns);
  printf("  Throughput: %.0f spans/sec\n", ops_per_sec);

  printf("\n7T Telemetry System: Ready for production use!\n");
}

// Example usage demonstration
void telemetry7t_example_usage(void)
{
  printf("7T Telemetry Example Usage\n");
  printf("==========================\n\n");

  // Initialize telemetry
  telemetry7t_global_init();

  // Example 1: Basic span
  printf("Example 1: Basic Span\n");
  Telemetry7TSpan *span1 = telemetry7t_span_begin("database_query", "SELECT", TELEMETRY7T_KIND_CLIENT);
  telemetry7t_add_attribute_string(span1, "table", "users");
  telemetry7t_add_attribute_int(span1, "limit", 100);

  // Simulate some work
  for (int i = 0; i < 1000; i++)
  {
    // Simulate work
  }

  telemetry7t_span_finish(span1, TELEMETRY7T_STATUS_OK);

  // Example 2: SHACL validation span
  printf("\nExample 2: SHACL Validation Span\n");
  Telemetry7TSpan *span2 = telemetry7t_shacl_span_begin("min_count");
  telemetry7t_add_attribute_string(span2, "subject", "ex:Alice");
  telemetry7t_add_attribute_string(span2, "predicate", "ex:name");
  telemetry7t_add_attribute_int(span2, "min_count", 1);

  // Simulate validation
  for (int i = 0; i < 100; i++)
  {
    // Simulate validation work
  }

  telemetry7t_span_finish(span2, TELEMETRY7T_STATUS_OK);

  // Example 3: Template rendering span
  printf("\nExample 3: Template Rendering Span\n");
  Telemetry7TSpan *span3 = telemetry7t_template_span_begin("variable_substitution");
  telemetry7t_add_attribute_string(span3, "template", "Hello {{user}}!");
  telemetry7t_add_attribute_string(span3, "variables", "user=Alice");

  // Simulate rendering
  for (int i = 0; i < 50; i++)
  {
    // Simulate rendering work
  }

  telemetry7t_span_finish(span3, TELEMETRY7T_STATUS_OK);

  // Example 4: Error span
  printf("\nExample 4: Error Span\n");
  Telemetry7TSpan *span4 = telemetry7t_span_begin("file_operation", "READ", TELEMETRY7T_KIND_INTERNAL);
  telemetry7t_add_attribute_string(span4, "file_path", "/nonexistent/file.txt");

  // Simulate error
  for (int i = 0; i < 10; i++)
  {
    // Simulate error condition
  }

  telemetry7t_span_finish(span4, TELEMETRY7T_STATUS_ERROR);

  // Example 5: Nested spans
  printf("\nExample 5: Nested Spans\n");
  Telemetry7TSpan *parent_span = telemetry7t_span_begin("request_processing", "HTTP_GET", TELEMETRY7T_KIND_SERVER);
  telemetry7t_add_attribute_string(parent_span, "endpoint", "/api/users");

  Telemetry7TSpan *child_span = telemetry7t_span_begin("database_query", "SELECT", TELEMETRY7T_KIND_CLIENT);
  telemetry7t_add_attribute_string(child_span, "query", "SELECT * FROM users");

  // Simulate nested work
  for (int i = 0; i < 500; i++)
  {
    // Simulate database work
  }

  telemetry7t_span_finish(child_span, TELEMETRY7T_STATUS_OK);

  for (int i = 0; i < 200; i++)
  {
    // Simulate response processing
  }

  telemetry7t_span_finish(parent_span, TELEMETRY7T_STATUS_OK);

  printf("\n7T Telemetry Examples: Complete!\n");
}