#ifndef TELEMETRY7T_7TICK_H
#define TELEMETRY7T_7TICK_H

#include <stdint.h>
#include <stddef.h>
#include <time.h>
#include <sys/time.h>
#include <stdatomic.h>

#ifdef __cplusplus
extern "C"
{
#endif

  // 7-Tick Optimized Telemetry Span - Minimal for maximum performance
  typedef struct
  {
    uint64_t span_id;       // Unique span identifier
    uint64_t start_time_ns; // Start time in nanoseconds
    uint64_t end_time_ns;   // End time in nanoseconds (0 if active)
    const char *name;       // Span name (interned string)
    uint8_t status;         // Status: 0=OK, 1=ERROR, 2=UNSET
  } Telemetry7TSpan7Tick;

  // 7-Tick Optimized Telemetry Context - Minimal for maximum performance
  typedef struct
  {
    Telemetry7TSpan7Tick *current_span;
    uint64_t next_span_id;
    uint8_t enabled; // Telemetry enabled flag
  } Telemetry7TContext7Tick;

// Span status constants
#define TELEMETRY7T_STATUS_OK 0
#define TELEMETRY7T_STATUS_ERROR 1
#define TELEMETRY7T_STATUS_UNSET 2

  // High-precision timing (7-tick optimized)
  static inline uint64_t telemetry7t_get_nanoseconds_7tick(void)
  {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000ULL + ts.tv_nsec;
  }

  // Generate unique span ID (7-tick optimized)
  static inline uint64_t telemetry7t_generate_span_id_7tick(void)
  {
    static uint64_t counter = 0;
    return __sync_fetch_and_add(&counter, 1);
  }

  // Initialize telemetry context (7-tick optimized)
  static inline void telemetry7t_init_context_7tick(Telemetry7TContext7Tick *ctx)
  {
    ctx->current_span = NULL;
    ctx->next_span_id = 1;
    ctx->enabled = 0; // Disabled by default for 7-tick performance
  }

  // Create a new span (7-tick optimized - target: ≤7 cycles)
  static inline Telemetry7TSpan7Tick *telemetry7t_create_span_7tick(
      Telemetry7TContext7Tick *ctx,
      const char *name)
  {
    if (!ctx->enabled)
      return NULL;

    // Stack allocation for 7-tick performance
    static Telemetry7TSpan7Tick span_buffer;
    Telemetry7TSpan7Tick *span = &span_buffer;

    span->span_id = telemetry7t_generate_span_id_7tick();
    span->start_time_ns = telemetry7t_get_nanoseconds_7tick();
    span->end_time_ns = 0; // Active span
    span->name = name;
    span->status = TELEMETRY7T_STATUS_UNSET;

    return span;
  }

  // Start a span (7-tick optimized - target: ≤7 cycles)
  static inline void telemetry7t_start_span_7tick(
      Telemetry7TContext7Tick *ctx,
      Telemetry7TSpan7Tick *span)
  {
    if (!ctx->enabled || !span)
      return;
    ctx->current_span = span;
  }

  // End a span (7-tick optimized - target: ≤7 cycles)
  static inline void telemetry7t_end_span_7tick(
      Telemetry7TContext7Tick *ctx,
      Telemetry7TSpan7Tick *span,
      uint8_t status)
  {
    if (!ctx->enabled || !span)
      return;

    span->end_time_ns = telemetry7t_get_nanoseconds_7tick();
    span->status = status;

    if (ctx->current_span == span)
    {
      ctx->current_span = NULL;
    }
  }

  // Get current span duration in nanoseconds (7-tick optimized)
  static inline uint64_t telemetry7t_get_span_duration_ns_7tick(Telemetry7TSpan7Tick *span)
  {
    if (!span)
      return 0;

    uint64_t end_time = span->end_time_ns;
    if (end_time == 0)
    {
      end_time = telemetry7t_get_nanoseconds_7tick();
    }

    return end_time - span->start_time_ns;
  }

  // Check if span is active (7-tick optimized)
  static inline int telemetry7t_is_span_active_7tick(Telemetry7TSpan7Tick *span)
  {
    return span && span->end_time_ns == 0;
  }

  // Get current span from context (7-tick optimized)
  static inline Telemetry7TSpan7Tick *telemetry7t_get_current_span_7tick(Telemetry7TContext7Tick *ctx)
  {
    return ctx ? ctx->current_span : NULL;
  }

  // Enable/disable telemetry (7-tick optimized)
  static inline void telemetry7t_set_enabled_7tick(Telemetry7TContext7Tick *ctx, int enabled)
  {
    if (ctx)
    {
      ctx->enabled = enabled ? 1 : 0;
    }
  }

  // 7-Tick optimized span creation with automatic timing
  static inline Telemetry7TSpan7Tick *telemetry7t_span_begin_7tick(
      Telemetry7TContext7Tick *ctx,
      const char *name)
  {
    Telemetry7TSpan7Tick *span = telemetry7t_create_span_7tick(ctx, name);
    if (span)
    {
      telemetry7t_start_span_7tick(ctx, span);
    }
    return span;
  }

  // 7-Tick optimized span ending with automatic timing
  static inline void telemetry7t_span_finish_7tick(
      Telemetry7TContext7Tick *ctx,
      Telemetry7TSpan7Tick *span,
      uint8_t status)
  {
    if (span)
    {
      telemetry7t_end_span_7tick(ctx, span, status);
    }
  }

// Macro for automatic 7-tick span management
#define TELEMETRY7T_SPAN_7TICK(ctx, name)                                                   \
  Telemetry7TSpan7Tick *span_##__LINE__ = telemetry7t_span_begin_7tick(ctx, name);          \
  for (int span_##__LINE__##_done = 0; !span_##__LINE__##_done; span_##__LINE__##_done = 1, \
           telemetry7t_span_finish_7tick(ctx, span_##__LINE__, TELEMETRY7T_STATUS_OK))

// Macro for error 7-tick span
#define TELEMETRY7T_SPAN_ERROR_7TICK(ctx, name)                                             \
  Telemetry7TSpan7Tick *span_##__LINE__ = telemetry7t_span_begin_7tick(ctx, name);          \
  for (int span_##__LINE__##_done = 0; !span_##__LINE__##_done; span_##__LINE__##_done = 1, \
           telemetry7t_span_finish_7tick(ctx, span_##__LINE__, TELEMETRY7T_STATUS_ERROR))

#ifdef __cplusplus
}
#endif

#endif // TELEMETRY7T_7TICK_H