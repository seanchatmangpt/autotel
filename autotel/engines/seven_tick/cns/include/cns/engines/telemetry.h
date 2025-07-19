#ifndef CNS_TELEMETRY_H
#define CNS_TELEMETRY_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Branch prediction hints for optimal performance
#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

// CNS Telemetry Span - High-performance equivalent of OpenTelemetry spans
typedef struct
{
  uint64_t span_id;          // Unique span identifier
  uint64_t trace_id;         // Trace identifier for correlation
  uint64_t parent_span_id;   // Parent span ID (0 for root)
  uint64_t start_time_ns;    // Start time in nanoseconds
  uint64_t end_time_ns;      // End time in nanoseconds (0 if active)
  const char *name;          // Span name (interned string)
  const char *operation;     // Operation type (interned string)
  uint32_t attributes_count; // Number of attributes
  uint32_t events_count;     // Number of events
  uint8_t status;            // Status: 0=OK, 1=ERROR, 2=UNSET
  uint8_t kind;              // Span kind: 0=INTERNAL, 1=SERVER, 2=CLIENT, 3=PRODUCER, 4=CONSUMER
} CNSTelemetrySpan;

// CNS Telemetry Context - Manages active spans
typedef struct
{
  CNSTelemetrySpan *current_span;
  CNSTelemetrySpan *span_stack[64]; // Stack for nested spans
  uint32_t stack_depth;
  uint64_t next_span_id;
  uint64_t next_trace_id;
  uint8_t enabled; // Telemetry enabled flag
  size_t memory_usage;
} CNSTelemetryContext;

// CNS Telemetry Attribute - Key-value pair for span attributes
typedef struct
{
  const char *key;    // Attribute key (interned string)
  const char *value;  // String value
  int64_t int_value;  // Integer value
  double float_value; // Float value
  uint8_t type;       // Type: 0=string, 1=int, 2=float, 3=bool
} CNSTelemetryAttribute;

// CNS Telemetry Event - Span event with timestamp
typedef struct
{
  uint64_t timestamp_ns; // Event timestamp in nanoseconds
  const char *name;      // Event name (interned string)
  CNSTelemetryAttribute *attributes;
  uint32_t attributes_count;
} CNSTelemetryEvent;

// Span status constants
#define CNS_TELEMETRY_STATUS_OK 0
#define CNS_TELEMETRY_STATUS_ERROR 1
#define CNS_TELEMETRY_STATUS_UNSET 2

// Span kind constants
#define CNS_TELEMETRY_KIND_INTERNAL 0
#define CNS_TELEMETRY_KIND_SERVER 1
#define CNS_TELEMETRY_KIND_CLIENT 2
#define CNS_TELEMETRY_KIND_PRODUCER 3
#define CNS_TELEMETRY_KIND_CONSUMER 4

// Function declarations

// High-precision timing functions
uint64_t cns_telemetry_get_nanoseconds(void);
uint64_t cns_telemetry_generate_span_id(void);
uint64_t cns_telemetry_generate_trace_id(void);

// Context management
void cns_telemetry_init_context(CNSTelemetryContext *ctx);
void cns_telemetry_set_enabled(CNSTelemetryContext *ctx, int enabled);
int cns_telemetry_is_enabled(CNSTelemetryContext *ctx);

// Span management (7-tick optimized)
CNSTelemetrySpan *cns_telemetry_create_span(CNSTelemetryContext *ctx, const char *name, const char *operation, uint8_t kind);
void cns_telemetry_start_span(CNSTelemetryContext *ctx, CNSTelemetrySpan *span);
void cns_telemetry_end_span(CNSTelemetryContext *ctx, CNSTelemetrySpan *span, uint8_t status);
void cns_telemetry_free_span(CNSTelemetrySpan *span);

// Convenience functions
CNSTelemetrySpan *cns_telemetry_span_begin(const char *name, const char *operation, uint8_t kind);
void cns_telemetry_span_finish(CNSTelemetrySpan *span, uint8_t status);

// Attribute management
void cns_telemetry_add_attribute_string(CNSTelemetrySpan *span, const char *key, const char *value);
void cns_telemetry_add_attribute_int(CNSTelemetrySpan *span, const char *key, int64_t value);
void cns_telemetry_add_attribute_float(CNSTelemetrySpan *span, const char *key, double value);
void cns_telemetry_add_attribute_bool(CNSTelemetrySpan *span, const char *key, bool value);

// Event management
void cns_telemetry_add_event(CNSTelemetrySpan *span, const char *name);

// Span information
uint64_t cns_telemetry_get_span_duration_ns(CNSTelemetrySpan *span);
int cns_telemetry_is_span_active(CNSTelemetrySpan *span);
CNSTelemetrySpan *cns_telemetry_get_current_span(CNSTelemetryContext *ctx);

// Performance monitoring
uint64_t cns_telemetry_get_cycles(void);
void cns_telemetry_measure_span_cycles(const char *name, const char *operation);

// Memory management
size_t cns_telemetry_get_memory_usage(CNSTelemetryContext *ctx);

// Output and reporting
void cns_telemetry_span_print(CNSTelemetrySpan *span);
void cns_telemetry_span_export_json(CNSTelemetrySpan *span, char *buffer, size_t buffer_size);

// Specialized span functions for CNS subsystems
CNSTelemetrySpan *cns_telemetry_shacl_span_begin(const char *constraint_type);
CNSTelemetrySpan *cns_telemetry_template_span_begin(const char *template_type);
CNSTelemetrySpan *cns_telemetry_pattern_span_begin(const char *pattern_type);
void cns_telemetry_span_add_7t_metrics(CNSTelemetrySpan *span, const char *operation_type);

// Benchmarking
void cns_telemetry_benchmark(void);
void cns_telemetry_example_usage(void);

#endif // CNS_TELEMETRY_H