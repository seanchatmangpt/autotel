#include "7t_unit_test_framework.h"
#include "../c_src/telemetry7t.h"
#include <string.h>

// Test suite registration
TEST_SUITE_BEGIN(telemetry7t)

// Test data structures
typedef struct
{
  Telemetry7TContext *context;
  Telemetry7TSpan *test_span;
  char *test_trace_id;
  char *test_span_id;
} TelemetryTestData;

// Setup and teardown functions
static void setup_telemetry_test(TestContext *context)
{
  TelemetryTestData *data = malloc(sizeof(TelemetryTestData));
  ASSERT_NOT_NULL(data);

  // Initialize telemetry context
  data->context = malloc(sizeof(Telemetry7TContext));
  ASSERT_NOT_NULL(data->context);
  telemetry7t_init_context(data->context);

  // Create test span
  data->test_span = telemetry7t_create_span(data->context, "test_operation");
  ASSERT_NOT_NULL(data->test_span);

  // Generate test IDs
  data->test_trace_id = malloc(33);
  data->test_span_id = malloc(17);
  ASSERT_NOT_NULL(data->test_trace_id);
  ASSERT_NOT_NULL(data->test_span_id);

  // Generate test IDs
  uint64_t trace_id = telemetry7t_generate_trace_id();
  uint64_t span_id = telemetry7t_generate_span_id();
  snprintf(data->test_trace_id, 33, "%016llx", trace_id);
  snprintf(data->test_span_id, 17, "%016llx", span_id);

  context->test_data = data;
}

static void teardown_telemetry_test(TestContext *context)
{
  TelemetryTestData *data = (TelemetryTestData *)context->test_data;
  if (data)
  {
    if (data->test_span)
    {
      free(data->test_span);
    }
    if (data->context)
    {
      free(data->context);
    }
    if (data->test_trace_id)
    {
      free(data->test_trace_id);
    }
    if (data->test_span_id)
    {
      free(data->test_span_id);
    }
    free(data);
  }
}

// ============================================================================
// CONTEXT MANAGEMENT TESTS
// ============================================================================

TEST_CASE(context_initialization, "Test telemetry context initialization")
static void test_context_initialization(TestContext *context)
{
  Telemetry7TContext ctx;
  telemetry7t_init_context(&ctx);

  // Check context initialization
  ASSERT_EQUAL(0, ctx.trace_id);
  ASSERT_EQUAL(0, ctx.span_id);
  ASSERT_EQUAL(0, ctx.span_count);
  ASSERT_EQUAL(0, ctx.event_count);
  ASSERT_EQUAL(0, ctx.metric_count);
  ASSERT_TRUE(ctx.enabled);
}

TEST_CASE(context_creation, "Test telemetry context creation")
static void test_context_creation(TestContext *context)
{
  TelemetryTestData *data = (TelemetryTestData *)context->test_data;
  ASSERT_NOT_NULL(data);
  ASSERT_NOT_NULL(data->context);

  // Check context properties
  ASSERT_TRUE(data->context->enabled);
  ASSERT_EQUAL(0, data->context->span_count);
  ASSERT_EQUAL(0, data->context->event_count);
  ASSERT_EQUAL(0, data->context->metric_count);
}

// ============================================================================
// SPAN MANAGEMENT TESTS
// ============================================================================

TEST_CASE(span_creation, "Test span creation")
static void test_span_creation(TestContext *context)
{
  TelemetryTestData *data = (TelemetryTestData *)context->test_data;
  ASSERT_NOT_NULL(data);

  Telemetry7TSpan *span = telemetry7t_create_span(data->context, "test_span");
  ASSERT_NOT_NULL(span);

  // Check span properties
  ASSERT_NOT_NULL(span->name);
  ASSERT_EQUAL(0, strcmp("test_span", span->name));
  ASSERT_GREATER_THAN(span->span_id, 0);
  ASSERT_GREATER_THAN(span->start_time_ns, 0);
  ASSERT_EQUAL(0, span->end_time_ns);
  ASSERT_EQUAL(0, span->attribute_count);
  ASSERT_EQUAL(0, span->event_count);
  ASSERT_EQUAL(0, span->status);

  free(span);
}

TEST_CASE(span_start_stop, "Test span start and stop")
static void test_span_start_stop(TestContext *context)
{
  TelemetryTestData *data = (TelemetryTestData *)context->test_data;
  ASSERT_NOT_NULL(data);

  Telemetry7TSpan *span = telemetry7t_create_span(data->context, "test_span");
  ASSERT_NOT_NULL(span);

  // Start span
  telemetry7t_start_span(data->context, span);
  ASSERT_GREATER_THAN(span->start_time_ns, 0);

  // End span
  telemetry7t_end_span(data->context, span, 0); // OK status
  ASSERT_GREATER_THAN(span->end_time_ns, span->start_time_ns);
  ASSERT_EQUAL(0, span->status);

  free(span);
}

TEST_CASE(span_performance, "Test span creation performance")
static void test_span_performance(TestContext *context)
{
  TelemetryTestData *data = (TelemetryTestData *)context->test_data;
  ASSERT_NOT_NULL(data);

  // Performance test: create many spans
  ASSERT_PERFORMANCE({
        for (int i = 0; i < 1000; i++) {
            Telemetry7TSpan* span = telemetry7t_create_span(data->context, "test_span");
            if (span) {
                telemetry7t_start_span(data->context, span);
                telemetry7t_end_span(data->context, span, 0);
                free(span);
            }
        } }, 1000000); // 1M cycles for 1000 spans
}

TEST_CASE(span_latency, "Test span creation latency")
static void test_span_latency(TestContext *context)
{
  TelemetryTestData *data = (TelemetryTestData *)context->test_data;
  ASSERT_NOT_NULL(data);

  // Latency test: single span creation
  ASSERT_LATENCY({
        Telemetry7TSpan* span = telemetry7t_create_span(data->context, "test_span");
        if (span) {
            telemetry7t_start_span(data->context, span);
            telemetry7t_end_span(data->context, span, 0);
            free(span);
        } }, 1000); // 1 microsecond limit
}

// ============================================================================
// ATTRIBUTE MANAGEMENT TESTS
// ============================================================================

TEST_CASE(attribute_string, "Test string attribute addition")
static void test_attribute_string(TestContext *context)
{
  TelemetryTestData *data = (TelemetryTestData *)context->test_data;
  ASSERT_NOT_NULL(data);

  Telemetry7TSpan *span = telemetry7t_create_span(data->context, "test_span");
  ASSERT_NOT_NULL(span);

  // Add string attribute
  telemetry7t_add_attribute_string(data->context, span, "test_key", "test_value");

  // Check attribute was added
  ASSERT_EQUAL(1, span->attribute_count);

  free(span);
}

TEST_CASE(attribute_int, "Test integer attribute addition")
static void test_attribute_int(TestContext *context)
{
  TelemetryTestData *data = (TelemetryTestData *)context->test_data;
  ASSERT_NOT_NULL(data);

  Telemetry7TSpan *span = telemetry7t_create_span(data->context, "test_span");
  ASSERT_NOT_NULL(span);

  // Add integer attribute
  telemetry7t_add_attribute_int(data->context, span, "test_key", 42);

  // Check attribute was added
  ASSERT_EQUAL(1, span->attribute_count);

  free(span);
}

TEST_CASE(attribute_double, "Test double attribute addition")
static void test_attribute_double(TestContext *context)
{
  TelemetryTestData *data = (TelemetryTestData *)context->test_data;
  ASSERT_NOT_NULL(data);

  Telemetry7TSpan *span = telemetry7t_create_span(data->context, "test_span");
  ASSERT_NOT_NULL(span);

  // Add double attribute
  telemetry7t_add_attribute_double(data->context, span, "test_key", 3.14159);

  // Check attribute was added
  ASSERT_EQUAL(1, span->attribute_count);

  free(span);
}

TEST_CASE(attribute_multiple, "Test multiple attribute addition")
static void test_attribute_multiple(TestContext *context)
{
  TelemetryTestData *data = (TelemetryTestData *)context->test_data;
  ASSERT_NOT_NULL(data);

  Telemetry7TSpan *span = telemetry7t_create_span(data->context, "test_span");
  ASSERT_NOT_NULL(span);

  // Add multiple attributes
  telemetry7t_add_attribute_string(data->context, span, "string_key", "string_value");
  telemetry7t_add_attribute_int(data->context, span, "int_key", 42);
  telemetry7t_add_attribute_double(data->context, span, "double_key", 3.14159);

  // Check all attributes were added
  ASSERT_EQUAL(3, span->attribute_count);

  free(span);
}

// ============================================================================
// EVENT MANAGEMENT TESTS
// ============================================================================

TEST_CASE(event_creation, "Test event creation")
static void test_event_creation(TestContext *context)
{
  TelemetryTestData *data = (TelemetryTestData *)context->test_data;
  ASSERT_NOT_NULL(data);

  Telemetry7TSpan *span = telemetry7t_create_span(data->context, "test_span");
  ASSERT_NOT_NULL(span);

  // Add event
  telemetry7t_add_event(data->context, span, "test_event");

  // Check event was added
  ASSERT_EQUAL(1, span->event_count);

  free(span);
}

TEST_CASE(event_with_attributes, "Test event creation with attributes")
static void test_event_with_attributes(TestContext *context)
{
  TelemetryTestData *data = (TelemetryTestData *)context->test_data;
  ASSERT_NOT_NULL(data);

  Telemetry7TSpan *span = telemetry7t_create_span(data->context, "test_span");
  ASSERT_NOT_NULL(span);

  // Add event with attributes
  telemetry7t_add_event(data->context, span, "test_event");
  telemetry7t_add_attribute_string(data->context, span, "event_key", "event_value");

  // Check event and attribute were added
  ASSERT_EQUAL(1, span->event_count);
  ASSERT_EQUAL(1, span->attribute_count);

  free(span);
}

// ============================================================================
// METRIC MANAGEMENT TESTS
// ============================================================================

TEST_CASE(metric_counter, "Test counter metric")
static void test_metric_counter(TestContext *context)
{
  TelemetryTestData *data = (TelemetryTestData *)context->test_data;
  ASSERT_NOT_NULL(data);

  // Add counter metric
  telemetry7t_add_counter(data->context, "test_counter", 42);

  // Check metric was added
  ASSERT_EQUAL(1, data->context->metric_count);
}

TEST_CASE(metric_gauge, "Test gauge metric")
static void test_metric_gauge(TestContext *context)
{
  TelemetryTestData *data = (TelemetryTestData *)context->test_data;
  ASSERT_NOT_NULL(data);

  // Add gauge metric
  telemetry7t_add_gauge(data->context, "test_gauge", 3.14159);

  // Check metric was added
  ASSERT_EQUAL(1, data->context->metric_count);
}

TEST_CASE(metric_histogram, "Test histogram metric")
static void test_metric_histogram(TestContext *context)
{
  TelemetryTestData *data = (TelemetryTestData *)context->test_data;
  ASSERT_NOT_NULL(data);

  // Add histogram metric
  telemetry7t_add_histogram(data->context, "test_histogram", 42.0);

  // Check metric was added
  ASSERT_EQUAL(1, data->context->metric_count);
}

// ============================================================================
// ID GENERATION TESTS
// ============================================================================

TEST_CASE(trace_id_generation, "Test trace ID generation")
static void test_trace_id_generation(TestContext *context)
{
  // Generate trace IDs
  uint64_t trace_id1 = telemetry7t_generate_trace_id();
  uint64_t trace_id2 = telemetry7t_generate_trace_id();

  // Check IDs are unique
  ASSERT_NOT_EQUAL(trace_id1, trace_id2);
  ASSERT_GREATER_THAN(trace_id1, 0);
  ASSERT_GREATER_THAN(trace_id2, 0);
}

TEST_CASE(span_id_generation, "Test span ID generation")
static void test_span_id_generation(TestContext *context)
{
  // Generate span IDs
  uint64_t span_id1 = telemetry7t_generate_span_id();
  uint64_t span_id2 = telemetry7t_generate_span_id();

  // Check IDs are unique
  ASSERT_NOT_EQUAL(span_id1, span_id2);
  ASSERT_GREATER_THAN(span_id1, 0);
  ASSERT_GREATER_THAN(span_id2, 0);
}

TEST_CASE(id_generation_performance, "Test ID generation performance")
static void test_id_generation_performance(TestContext *context)
{
  // Performance test: generate many IDs
  ASSERT_PERFORMANCE({
        for (int i = 0; i < 10000; i++) {
            telemetry7t_generate_trace_id();
            telemetry7t_generate_span_id();
        } }, 1000000); // 1M cycles for 20K IDs
}

// ============================================================================
// TIMING TESTS
// ============================================================================

TEST_CASE(nanosecond_timing, "Test nanosecond timing")
static void test_nanosecond_timing(TestContext *context)
{
  // Test nanosecond timing
  uint64_t time1 = telemetry7t_get_nanoseconds();
  uint64_t time2 = telemetry7t_get_nanoseconds();

  // Check timing is monotonically increasing
  ASSERT_GREATER_THAN_OR_EQUAL(time2, time1);
  ASSERT_GREATER_THAN(time1, 0);
  ASSERT_GREATER_THAN(time2, 0);
}

TEST_CASE(timing_accuracy, "Test timing accuracy")
static void test_timing_accuracy(TestContext *context)
{
  // Test timing accuracy with known delay
  uint64_t start_time = telemetry7t_get_nanoseconds();

  // Simulate some work
  for (int i = 0; i < 1000; i++)
  {
    // Do nothing
  }

  uint64_t end_time = telemetry7t_get_nanoseconds();
  uint64_t elapsed = end_time - start_time;

  // Check that some time elapsed
  ASSERT_GREATER_THAN(elapsed, 0);
}

// ============================================================================
// STRESS TESTS
// ============================================================================

TEST_CASE(stress_test_many_spans, "Test with many spans")
static void test_stress_test_many_spans(TestContext *context)
{
  TelemetryTestData *data = (TelemetryTestData *)context->test_data;
  ASSERT_NOT_NULL(data);

  // Create many spans
  for (int i = 0; i < 1000; i++)
  {
    char span_name[64];
    snprintf(span_name, sizeof(span_name), "span_%d", i);

    Telemetry7TSpan *span = telemetry7t_create_span(data->context, span_name);
    ASSERT_NOT_NULL(span);

    telemetry7t_start_span(data->context, span);

    // Add some attributes
    telemetry7t_add_attribute_int(data->context, span, "index", i);
    telemetry7t_add_attribute_string(data->context, span, "name", span_name);

    telemetry7t_end_span(data->context, span, 0);

    free(span);
  }

  // Check span count
  ASSERT_EQUAL(1000, data->context->span_count);
}

TEST_CASE(stress_test_many_attributes, "Test with many attributes")
static void test_stress_test_many_attributes(TestContext *context)
{
  TelemetryTestData *data = (TelemetryTestData *)context->test_data;
  ASSERT_NOT_NULL(data);

  Telemetry7TSpan *span = telemetry7t_create_span(data->context, "test_span");
  ASSERT_NOT_NULL(span);

  // Add many attributes
  for (int i = 0; i < 100; i++)
  {
    char key[64], value[64];
    snprintf(key, sizeof(key), "key_%d", i);
    snprintf(value, sizeof(value), "value_%d", i);

    telemetry7t_add_attribute_string(data->context, span, key, value);
  }

  // Check attribute count
  ASSERT_EQUAL(100, span->attribute_count);

  free(span);
}

TEST_CASE(stress_test_many_events, "Test with many events")
static void test_stress_test_many_events(TestContext *context)
{
  TelemetryTestData *data = (TelemetryTestData *)context->test_data;
  ASSERT_NOT_NULL(data);

  Telemetry7TSpan *span = telemetry7t_create_span(data->context, "test_span");
  ASSERT_NOT_NULL(span);

  // Add many events
  for (int i = 0; i < 100; i++)
  {
    char event_name[64];
    snprintf(event_name, sizeof(event_name), "event_%d", i);

    telemetry7t_add_event(data->context, span, event_name);
  }

  // Check event count
  ASSERT_EQUAL(100, span->event_count);

  free(span);
}

TEST_CASE(stress_test_many_metrics, "Test with many metrics")
static void test_stress_test_many_metrics(TestContext *context)
{
  TelemetryTestData *data = (TelemetryTestData *)context->test_data;
  ASSERT_NOT_NULL(data);

  // Add many metrics
  for (int i = 0; i < 100; i++)
  {
    char metric_name[64];
    snprintf(metric_name, sizeof(metric_name), "metric_%d", i);

    telemetry7t_add_counter(data->context, metric_name, i);
  }

  // Check metric count
  ASSERT_EQUAL(100, data->context->metric_count);
}

TEST_CASE(stress_test_memory_efficiency, "Test memory efficiency under load")
static void test_stress_test_memory_efficiency(TestContext *context)
{
  TelemetryTestData *data = (TelemetryTestData *)context->test_data;
  ASSERT_NOT_NULL(data);

  // Memory efficiency test: create many spans with attributes
  size_t initial_memory = get_memory_usage();

  for (int i = 0; i < 1000; i++)
  {
    char span_name[64];
    snprintf(span_name, sizeof(span_name), "span_%d", i);

    Telemetry7TSpan *span = telemetry7t_create_span(data->context, span_name);
    if (span)
    {
      telemetry7t_start_span(data->context, span);

      // Add some attributes
      for (int j = 0; j < 10; j++)
      {
        char key[64], value[64];
        snprintf(key, sizeof(key), "key_%d_%d", i, j);
        snprintf(value, sizeof(value), "value_%d_%d", i, j);
        telemetry7t_add_attribute_string(data->context, span, key, value);
      }

      telemetry7t_end_span(data->context, span, 0);
      free(span);
    }
  }

  size_t final_memory = get_memory_usage();
  size_t memory_used = final_memory - initial_memory;

  // Memory usage should be reasonable (less than 10MB for 1000 spans with attributes)
  ASSERT_LESS_THAN(memory_used, 10 * 1024 * 1024);
}

// ============================================================================
// EDGE CASE TESTS
// ============================================================================

TEST_CASE(edge_case_null_span, "Test handling of NULL span")
static void test_edge_case_null_span(TestContext *context)
{
  TelemetryTestData *data = (TelemetryTestData *)context->test_data;
  ASSERT_NOT_NULL(data);

  // Test operations with NULL span (should handle gracefully)
  telemetry7t_start_span(data->context, NULL);
  telemetry7t_end_span(data->context, NULL, 0);
  telemetry7t_add_attribute_string(data->context, NULL, "key", "value");
  telemetry7t_add_attribute_int(data->context, NULL, "key", 42);
  telemetry7t_add_attribute_double(data->context, NULL, "key", 3.14159);
  telemetry7t_add_event(data->context, NULL, "event");

  // Test should pass if operations don't crash
  ASSERT_TRUE(true);
}

TEST_CASE(edge_case_null_context, "Test handling of NULL context")
static void test_edge_case_null_context(TestContext *context)
{
  TelemetryTestData *data = (TelemetryTestData *)context->test_data;
  ASSERT_NOT_NULL(data);

  // Test operations with NULL context (should handle gracefully)
  Telemetry7TSpan *span = telemetry7t_create_span(NULL, "test_span");
  // Implementation dependent - should either return NULL or handle gracefully

  if (span)
  {
    telemetry7t_start_span(NULL, span);
    telemetry7t_end_span(NULL, span, 0);
    telemetry7t_add_attribute_string(NULL, span, "key", "value");
    telemetry7t_add_event(NULL, span, "event");
    free(span);
  }

  // Test should pass if operations don't crash
  ASSERT_TRUE(true);
}

TEST_CASE(edge_case_null_strings, "Test handling of NULL strings")
static void test_edge_case_null_strings(TestContext *context)
{
  TelemetryTestData *data = (TelemetryTestData *)context->test_data;
  ASSERT_NOT_NULL(data);

  Telemetry7TSpan *span = telemetry7t_create_span(data->context, NULL);
  // Implementation dependent - should either return NULL or handle gracefully

  if (span)
  {
    telemetry7t_add_attribute_string(data->context, span, NULL, "value");
    telemetry7t_add_attribute_string(data->context, span, "key", NULL);
    telemetry7t_add_event(data->context, span, NULL);
    free(span);
  }

  // Test should pass if operations don't crash
  ASSERT_TRUE(true);
}

TEST_CASE(edge_case_empty_strings, "Test handling of empty strings")
static void test_edge_case_empty_strings(TestContext *context)
{
  TelemetryTestData *data = (TelemetryTestData *)context->test_data;
  ASSERT_NOT_NULL(data);

  Telemetry7TSpan *span = telemetry7t_create_span(data->context, "");
  ASSERT_NOT_NULL(span);

  telemetry7t_add_attribute_string(data->context, span, "", "value");
  telemetry7t_add_attribute_string(data->context, span, "key", "");
  telemetry7t_add_event(data->context, span, "");

  // Test should pass if operations don't crash
  ASSERT_TRUE(true);

  free(span);
}

TEST_SUITE_END(telemetry7t)

// Test suite registration function
void register_telemetry7t_tests(void)
{
  // Register all test cases
  TestCase test_cases[] = {
      // Context management tests
      test_case_context_initialization,
      test_case_context_creation,

      // Span management tests
      test_case_span_creation,
      test_case_span_start_stop,
      test_case_span_performance,
      test_case_span_latency,

      // Attribute management tests
      test_case_attribute_string,
      test_case_attribute_int,
      test_case_attribute_double,
      test_case_attribute_multiple,

      // Event management tests
      test_case_event_creation,
      test_case_event_with_attributes,

      // Metric management tests
      test_case_metric_counter,
      test_case_metric_gauge,
      test_case_metric_histogram,

      // ID generation tests
      test_case_trace_id_generation,
      test_case_span_id_generation,
      test_case_id_generation_performance,

      // Timing tests
      test_case_nanosecond_timing,
      test_case_timing_accuracy,

      // Stress tests
      test_case_stress_test_many_spans,
      test_case_stress_test_many_attributes,
      test_case_stress_test_many_events,
      test_case_stress_test_many_metrics,
      test_case_stress_test_memory_efficiency,

      // Edge cases
      test_case_edge_case_null_span,
      test_case_edge_case_null_context,
      test_case_edge_case_null_strings,
      test_case_edge_case_empty_strings};

  telemetry7t_registration.test_cases = test_cases;
  telemetry7t_registration.test_case_count = sizeof(test_cases) / sizeof(TestCase);
  telemetry7t_registration.suite_setup = setup_telemetry_test;
  telemetry7t_registration.suite_teardown = teardown_telemetry_test;
}

// Main test runner
int main(int argc, char *argv[])
{
  printf("=== 7T Telemetry Unit Tests ===\n");
  printf("Framework Version: %s\n", SEVEN_TICK_TEST_VERSION);

  // Initialize test configuration
  test_config_init();
  test_config_set_verbose(true);

  // Register tests
  register_telemetry7t_tests();

  // Run test suite
  test_run_suite(&telemetry7t_registration);

  printf("\n=== Telemetry Tests Complete ===\n");
  return 0;
}