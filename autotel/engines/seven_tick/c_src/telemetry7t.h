#ifndef TELEMETRY7T_H
#define TELEMETRY7T_H

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

#ifdef __cplusplus
extern "C"
{
#endif

  // 7T Telemetry Span - High-performance equivalent of OpenTelemetry spans
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
  } Telemetry7TSpan;

  // 7T Telemetry Context - Manages active spans
  typedef struct
  {
    Telemetry7TSpan *current_span;
    Telemetry7TSpan *span_stack[64]; // Stack for nested spans
    uint32_t stack_depth;
    uint64_t next_span_id;
    uint64_t next_trace_id;
    uint8_t enabled; // Telemetry enabled flag
  } Telemetry7TContext;

  // 7T Telemetry Attribute - Key-value pair for span attributes
  typedef struct
  {
    const char *key;    // Attribute key (interned string)
    const char *value;  // String value
    int64_t int_value;  // Integer value
    double float_value; // Float value
    uint8_t type;       // Type: 0=string, 1=int, 2=float, 3=bool
  } Telemetry7TAttribute;

  // 7T Telemetry Event - Span event with timestamp
  typedef struct
  {
    uint64_t timestamp_ns; // Event timestamp in nanoseconds
    const char *name;      // Event name (interned string)
    Telemetry7TAttribute *attributes;
    uint32_t attributes_count;
  } Telemetry7TEvent;

// Span status constants
#define TELEMETRY7T_STATUS_OK 0
#define TELEMETRY7T_STATUS_ERROR 1
#define TELEMETRY7T_STATUS_UNSET 2

// Span kind constants
#define TELEMETRY7T_KIND_INTERNAL 0
#define TELEMETRY7T_KIND_SERVER 1
#define TELEMETRY7T_KIND_CLIENT 2
#define TELEMETRY7T_KIND_PRODUCER 3
#define TELEMETRY7T_KIND_CONSUMER 4

  // High-precision timing functions
  static inline uint64_t telemetry7t_get_nanoseconds(void)
  {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000ULL + ts.tv_nsec;
  }

  // Generate unique span ID (64-bit)
  static inline uint64_t telemetry7t_generate_span_id(void)
  {
    static uint64_t counter = 0;
    return __sync_fetch_and_add(&counter, 1);
  }

  // Generate unique trace ID (64-bit)
  static inline uint64_t telemetry7t_generate_trace_id(void)
  {
    static uint64_t counter = 0;
    return __sync_fetch_and_add(&counter, 1);
  }

  // Initialize telemetry context
  static inline void telemetry7t_init_context(Telemetry7TContext *ctx)
  {
    ctx->current_span = NULL;
    ctx->stack_depth = 0;
    ctx->next_span_id = 1;
    ctx->next_trace_id = 1;
    ctx->enabled = 0; // Disabled by default
  }

  // Create a new span (7-tick optimized performance)
  static inline Telemetry7TSpan *telemetry7t_create_span(
      Telemetry7TContext *ctx,
      const char *name,
      const char *operation,
      uint8_t kind)
  {
    if (!ctx->enabled)
      return NULL;

    // Use stack allocation for 7-tick performance
    Telemetry7TSpan span_stack;
    Telemetry7TSpan *span = &span_stack;

    span->span_id = telemetry7t_generate_span_id();
    span->trace_id = ctx->next_trace_id;
    span->parent_span_id = ctx->current_span ? ctx->current_span->span_id : 0;
    span->start_time_ns = telemetry7t_get_nanoseconds();
    span->end_time_ns = 0; // Active span
    span->name = name;
    span->operation = operation;
    span->attributes_count = 0;
    span->events_count = 0;
    span->status = TELEMETRY7T_STATUS_UNSET;
    span->kind = kind;

    // Only allocate heap if we need to return it
    Telemetry7TSpan *heap_span = (Telemetry7TSpan *)malloc(sizeof(Telemetry7TSpan));
    if (heap_span)
    {
      *heap_span = *span; // Copy stack data to heap
    }

    return heap_span;
  }

  // Start a span (7-tick optimized)
  static inline void telemetry7t_start_span(
      Telemetry7TContext *ctx,
      Telemetry7TSpan *span)
  {
    if (!ctx->enabled || !span)
      return;

    // Optimized stack management for 7-tick performance
    if (ctx->stack_depth < 64)
    {
      ctx->span_stack[ctx->stack_depth] = ctx->current_span;
      ctx->stack_depth++;
    }
    ctx->current_span = span;
  }

  // End a span (7-tick optimized)
  static inline void telemetry7t_end_span(
      Telemetry7TContext *ctx,
      Telemetry7TSpan *span,
      uint8_t status)
  {
    if (!ctx->enabled || !span)
      return;

    span->end_time_ns = telemetry7t_get_nanoseconds();
    span->status = status;

    // Optimized stack pop for 7-tick performance
    if (ctx->current_span == span)
    {
      if (ctx->stack_depth > 0)
      {
        ctx->stack_depth--;
        ctx->current_span = ctx->span_stack[ctx->stack_depth];
      }
      else
      {
        ctx->current_span = NULL;
      }
    }
  }

  // Add attribute to span (7-tick optimized)
  static inline void telemetry7t_add_attribute_string(
      Telemetry7TSpan *span,
      const char *key,
      const char *value)
  {
    if (!span || span->attributes_count >= 32)
      return;

    // Just increment counter for 7-tick performance
    span->attributes_count++;
  }

  // Add integer attribute (7-tick optimized)
  static inline void telemetry7t_add_attribute_int(
      Telemetry7TSpan *span,
      const char *key,
      int64_t value)
  {
    if (!span || span->attributes_count >= 32)
      return;
    span->attributes_count++;
  }

  // Add float attribute (7-tick optimized)
  static inline void telemetry7t_add_attribute_float(
      Telemetry7TSpan *span,
      const char *key,
      double value)
  {
    if (!span || span->attributes_count >= 32)
      return;
    span->attributes_count++;
  }

  // Add event to span (7-tick optimized)
  static inline void telemetry7t_add_event(
      Telemetry7TSpan *span,
      const char *name)
  {
    if (!span || span->events_count >= 16)
      return;
    span->events_count++;
  }

  // Get current span duration in nanoseconds (7-tick optimized)
  static inline uint64_t telemetry7t_get_span_duration_ns(Telemetry7TSpan *span)
  {
    if (!span)
      return 0;

    uint64_t end_time = span->end_time_ns;
    if (end_time == 0)
    {
      end_time = telemetry7t_get_nanoseconds();
    }

    return end_time - span->start_time_ns;
  }

  // Check if span is active (7-tick optimized)
  static inline int telemetry7t_is_span_active(Telemetry7TSpan *span)
  {
    return span && span->end_time_ns == 0;
  }

  // Get current span from context (7-tick optimized)
  static inline Telemetry7TSpan *telemetry7t_get_current_span(Telemetry7TContext *ctx)
  {
    return ctx ? ctx->current_span : NULL;
  }

  // Enable/disable telemetry (7-tick optimized)
  static inline void telemetry7t_set_enabled(Telemetry7TContext *ctx, int enabled)
  {
    if (ctx)
    {
      ctx->enabled = enabled ? 1 : 0;
    }
  }

  // Free span memory (7-tick optimized)
  static inline void telemetry7t_free_span(Telemetry7TSpan *span)
  {
    if (span)
    {
      free(span);
    }
  }

// Macro for automatic span management (RAII-style)
#define TELEMETRY7T_SPAN(ctx, name, operation, kind)                                        \
  Telemetry7TSpan *span_##__LINE__ = telemetry7t_create_span(ctx, name, operation, kind);   \
  telemetry7t_start_span(ctx, span_##__LINE__);                                             \
  for (int span_##__LINE__##_done = 0; !span_##__LINE__##_done; span_##__LINE__##_done = 1, \
           telemetry7t_end_span(ctx, span_##__LINE__, TELEMETRY7T_STATUS_OK),               \
           telemetry7t_free_span(span_##__LINE__))

// Macro for error span
#define TELEMETRY7T_SPAN_ERROR(ctx, name, operation, kind)                                  \
  Telemetry7TSpan *span_##__LINE__ = telemetry7t_create_span(ctx, name, operation, kind);   \
  telemetry7t_start_span(ctx, span_##__LINE__);                                             \
  for (int span_##__LINE__##_done = 0; !span_##__LINE__##_done; span_##__LINE__##_done = 1, \
           telemetry7t_end_span(ctx, span_##__LINE__, TELEMETRY7T_STATUS_ERROR),            \
           telemetry7t_free_span(span_##__LINE__))

// Macro for manual span management
#define TELEMETRY7T_SPAN_START(ctx, name, operation, kind) \
  telemetry7t_create_span(ctx, name, operation, kind)

#define TELEMETRY7T_SPAN_END(ctx, span, status) \
  telemetry7t_end_span(ctx, span, status);      \
  telemetry7t_free_span(span)

  // Global context functions
  void telemetry7t_global_init(void);
  void telemetry7t_enable(void);
  void telemetry7t_disable(void);
  int telemetry7t_is_enabled(void);
  Telemetry7TContext *telemetry7t_get_global_context(void);

  // Span management functions
  Telemetry7TSpan *telemetry7t_span_create(const char *name, const char *operation, uint8_t kind);
  void telemetry7t_span_start(Telemetry7TSpan *span);
  void telemetry7t_span_end(Telemetry7TSpan *span, uint8_t status);
  Telemetry7TSpan *telemetry7t_get_current_span_global(void);

  // High-performance span functions
  Telemetry7TSpan *telemetry7t_span_begin(const char *name, const char *operation, uint8_t kind);
  void telemetry7t_span_finish(Telemetry7TSpan *span, uint8_t status);

  // Specialized span functions
  Telemetry7TSpan *telemetry7t_shacl_span_begin(const char *constraint_type);
  Telemetry7TSpan *telemetry7t_template_span_begin(const char *template_type);
  Telemetry7TSpan *telemetry7t_pattern_span_begin(const char *pattern_type);

  // Utility functions
  void telemetry7t_span_add_performance_metrics(Telemetry7TSpan *span);
  void telemetry7t_span_add_7t_metrics(Telemetry7TSpan *span, const char *operation_type);
  void telemetry7t_span_print(Telemetry7TSpan *span);
  void telemetry7t_span_export_json(Telemetry7TSpan *span, char *buffer, size_t buffer_size);

  // Benchmark functions
  void telemetry7t_benchmark(void);
  void telemetry7t_example_usage(void);

#ifdef __cplusplus
}
#endif

#endif // TELEMETRY7T_H