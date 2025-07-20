/*  ─────────────────────────────────────────────────────────────
    cns/telemetry/otel.h  –  OpenTelemetry Integration (v2.0)
    Distributed tracing and metrics for CNS commands
    ───────────────────────────────────────────────────────────── */
#ifndef CNS_TELEMETRY_OTEL_H
#define CNS_TELEMETRY_OTEL_H

#include "../../../../include/s7t.h"
#include "../core/perf.h"
#include "../types.h"
#include <stdint.h>
#include <stdbool.h>

/*═══════════════════════════════════════════════════════════════
  Forward Declarations
  ═══════════════════════════════════════════════════════════════*/

typedef struct cns_span_t cns_span_t;
typedef struct cns_tracer_t cns_tracer_t;
typedef struct cns_meter_t cns_meter_t;
typedef struct cns_exporter_t cns_exporter_t;

/*═══════════════════════════════════════════════════════════════
  Telemetry Configuration
  ═══════════════════════════════════════════════════════════════*/

typedef struct
{
    const char *service_name;    // Service name
    const char *service_version; // Service version
    const char *deployment_env;  // Deployment environment

    // Tracing configuration
    bool tracing_enabled;     // Enable tracing
    double trace_sample_rate; // Sample rate (0.0-1.0)
    uint32_t max_spans;       // Maximum concurrent spans

    // Metrics configuration
    bool metrics_enabled;        // Enable metrics
    uint32_t metric_interval_ms; // Export interval

    // Export configuration
    const char *otlp_endpoint;  // OTLP endpoint
    const char *otlp_headers;   // OTLP headers
    bool use_grpc;              // Use gRPC (vs HTTP)
    uint32_t export_timeout_ms; // Export timeout
} cns_telemetry_config_t;

/*═══════════════════════════════════════════════════════════════
  Telemetry System
  ═══════════════════════════════════════════════════════════════*/

typedef struct S7T_ALIGNED(64)
{
    // Components
    cns_tracer_t *tracer;     // Trace provider
    cns_meter_t *meter;       // Metrics provider
    cns_exporter_t *exporter; // Data exporter

    // Configuration
    cns_telemetry_config_t config; // Telemetry config

    // State
    bool initialized;      // System initialized
    uint64_t trace_count;  // Total traces
    uint64_t metric_count; // Total metrics
} cns_telemetry_t;

/*═══════════════════════════════════════════════════════════════
  Span Attributes
  ═══════════════════════════════════════════════════════════════*/

typedef enum
{
    CNS_ATTR_STRING,
    CNS_ATTR_INT64,
    CNS_ATTR_DOUBLE,
    CNS_ATTR_BOOL,
    CNS_ATTR_BYTES
} cns_attr_type_t;

typedef struct
{
    const char *key;
    cns_attr_type_t type;
    union
    {
        const char *string_value;
        int64_t int64_value;
        double double_value;
        bool bool_value;
        struct
        {
            const uint8_t *data;
            size_t len;
        } bytes_value;
    };
} cns_attribute_t;

/*═══════════════════════════════════════════════════════════════
  Span Status
  ═══════════════════════════════════════════════════════════════*/

typedef enum
{
    CNS_SPAN_STATUS_UNSET = 0,
    CNS_SPAN_STATUS_OK,
    CNS_SPAN_STATUS_ERROR
} cns_span_status_t;

/*═══════════════════════════════════════════════════════════════
  Telemetry Initialization
  ═══════════════════════════════════════════════════════════════*/

// Initialize telemetry system
CNSResult cns_telemetry_init(
    cns_telemetry_t *telemetry,
    const cns_telemetry_config_t *config);

// Shutdown telemetry system
void cns_telemetry_shutdown(cns_telemetry_t *telemetry);

// Force flush all pending data
CNSResult cns_telemetry_flush(cns_telemetry_t *telemetry);

/*═══════════════════════════════════════════════════════════════
  Span Operations (< 7 ticks for start/end)
  ═══════════════════════════════════════════════════════════════*/

// Start a new span
cns_span_t *cns_span_start(
    cns_telemetry_t *telemetry,
    const char *name,
    const cns_span_t *parent);

// End span with status
void cns_span_end(
    cns_span_t *span,
    cns_span_status_t status);

// Set span attributes
void cns_span_set_attributes(
    cns_span_t *span,
    const cns_attribute_t *attrs,
    size_t count);

// Add span event
void cns_span_add_event(
    cns_span_t *span,
    const char *name,
    const cns_attribute_t *attrs,
    size_t count);

// Record exception
void cns_span_record_exception(
    cns_span_t *span,
    const char *type,
    const char *message,
    const char *stacktrace);

/*═══════════════════════════════════════════════════════════════
  Metrics Operations
  ═══════════════════════════════════════════════════════════════*/

// Record command latency
void cns_metric_record_latency(
    cns_telemetry_t *telemetry,
    const char *command,
    uint64_t cycles);

// Increment command counter
void cns_metric_inc_counter(
    cns_telemetry_t *telemetry,
    const char *command,
    CNSResult result);

// Record memory usage
void cns_metric_record_memory(
    cns_telemetry_t *telemetry,
    size_t used,
    size_t total);

// Record performance violation
void cns_metric_record_violation(
    cns_telemetry_t *telemetry,
    const char *operation,
    uint64_t actual_cycles,
    uint64_t threshold_cycles);

/*═══════════════════════════════════════════════════════════════
  Context Propagation
  ═══════════════════════════════════════════════════════════════*/

// Extract trace context from string
bool cns_context_extract(
    const char *traceparent,
    const char *tracestate,
    cns_span_t **parent);

// Inject trace context to string
bool cns_context_inject(
    const cns_span_t *span,
    char *traceparent,
    size_t traceparent_size,
    char *tracestate,
    size_t tracestate_size);

/*═══════════════════════════════════════════════════════════════
  Convenience Macros
  ═══════════════════════════════════════════════════════════════*/

// Start span with automatic name
#define CNS_SPAN_START(telemetry, parent) \
    cns_span_start(telemetry, __func__, parent)

// Auto-end span on scope exit
#define CNS_SPAN_SCOPE(telemetry, name, parent) \
    __attribute__((cleanup(cns_span_auto_end))) \
    cns_span_t *_span = cns_span_start(telemetry, name, parent)

// Set single attribute
#define CNS_SPAN_SET_ATTR(span, key, value)        \
    do                                             \
    {                                              \
        cns_attribute_t __attr = {                 \
            .key = key,                            \
            .type = _Generic((value),              \
                char *: CNS_ATTR_STRING,           \
                const char *: CNS_ATTR_STRING,     \
                int64_t: CNS_ATTR_INT64,           \
                double: CNS_ATTR_DOUBLE,           \
                bool: CNS_ATTR_BOOL),              \
            ._Generic((value),                     \
                char *:.string_value,              \
                const char *:.string_value,        \
                int64_t:.int64_value,              \
                double:.double_value,              \
                bool:.bool_value) = value};        \
        cns_span_set_attributes(span, &__attr, 1); \
    } while (0)

// Record command execution
#define CNS_TELEMETRY_COMMAND(telemetry, cmd_name, code)                                      \
    do                                                                                        \
    {                                                                                         \
        CNS_SPAN_SCOPE(telemetry, cmd_name, NULL);                                            \
        uint64_t __start = s7t_cycles();                                                      \
        CNSResult __result = code;                                                            \
        uint64_t __cycles = s7t_cycles() - __start;                                           \
        cns_metric_record_latency(telemetry, cmd_name, __cycles);                             \
        cns_metric_inc_counter(telemetry, cmd_name, __result);                                \
        cns_span_end(_span, __result == CNS_OK ? CNS_SPAN_STATUS_OK : CNS_SPAN_STATUS_ERROR); \
    } while (0)

/*═══════════════════════════════════════════════════════════════
  Default Configuration
  ═══════════════════════════════════════════════════════════════*/

#define CNS_DEFAULT_TELEMETRY_CONFIG { \
    .service_name = "cns",             \
    .service_version = "2.0.0",        \
    .deployment_env = "production",    \
    .tracing_enabled = true,           \
    .trace_sample_rate = 1.0,          \
    .max_spans = 1000,                 \
    .metrics_enabled = true,           \
    .metric_interval_ms = 60000,       \
    .otlp_endpoint = "localhost:4317", \
    .otlp_headers = NULL,              \
    .use_grpc = true,                  \
    .export_timeout_ms = 10000}

// Helper for span cleanup
static inline void cns_span_auto_end(cns_span_t **span)
{
    if (*span)
    {
        cns_span_end(*span, CNS_SPAN_STATUS_OK);
    }
}

#endif /* CNS_TELEMETRY_OTEL_H */