/*  ─────────────────────────────────────────────────────────────
    cns/include/cns/pragmatic/tracer_bullets.h  –  Tracer Bullets (v1.0)
    End-to-end working prototypes for system validation
    ───────────────────────────────────────────────────────────── */
#ifndef CNS_PRAGMATIC_TRACER_BULLETS_H
#define CNS_PRAGMATIC_TRACER_BULLETS_H

#include "cns/types.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <time.h>

/*═══════════════════════════════════════════════════════════════
  Configuration
  ═══════════════════════════════════════════════════════════════*/

#define CNS_MAX_TRACER_BULLETS 32
#define CNS_MAX_BULLET_STEPS 64
#define CNS_MAX_BULLET_RESULTS 128
#define CNS_TRACER_TIMEOUT_MS 5000

/*═══════════════════════════════════════════════════════════════
  Tracer Bullet Types
  ═══════════════════════════════════════════════════════════════*/

typedef enum
{
  CNS_TRACER_TYPE_END_TO_END,    // Complete system validation
  CNS_TRACER_TYPE_INTEGRATION,   // Component integration test
  CNS_TRACER_TYPE_PERFORMANCE,   // Performance validation
  CNS_TRACER_TYPE_FUNCTIONALITY, // Feature functionality test
  CNS_TRACER_TYPE_STRESS         // Stress and load testing
} cns_tracer_type_t;

typedef enum
{
  CNS_BULLET_STATUS_PENDING,
  CNS_BULLET_STATUS_RUNNING,
  CNS_BULLET_STATUS_SUCCESS,
  CNS_BULLET_STATUS_FAILED,
  CNS_BULLET_STATUS_TIMEOUT
} cns_bullet_status_t;

/*═══════════════════════════════════════════════════════════════
  Tracer Bullet Step
  ═══════════════════════════════════════════════════════════════*/

typedef struct
{
  uint32_t step_id;
  char description[128];
  uint64_t start_time_ns;
  uint64_t end_time_ns;
  bool completed;
  cns_result_t result;
  char error_message[256];
} cns_tracer_step_t;

/*═══════════════════════════════════════════════════════════════
  Tracer Bullet Result
  ═══════════════════════════════════════════════════════════════*/

typedef struct
{
  uint32_t result_id;
  char name[64];
  char value[256];
  uint64_t timestamp_ns;
} cns_tracer_result_t;

/*═══════════════════════════════════════════════════════════════
  Tracer Bullet
  ═══════════════════════════════════════════════════════════════*/

typedef struct
{
  uint32_t bullet_id;
  char name[64];
  char description[256];
  cns_tracer_type_t type;
  cns_bullet_status_t status;

  uint64_t start_time_ns;
  uint64_t end_time_ns;

  uint32_t step_count;
  cns_tracer_step_t steps[CNS_MAX_BULLET_STEPS];

  uint32_t result_count;
  cns_tracer_result_t results[CNS_MAX_BULLET_RESULTS];

  bool validation_passed;
  char validation_message[512];
} cns_tracer_bullet_t;

/*═══════════════════════════════════════════════════════════════
  Tracer Bullet Manager
  ═══════════════════════════════════════════════════════════════*/

typedef struct
{
  cns_tracer_bullet_t bullets[CNS_MAX_TRACER_BULLETS];
  uint32_t bullet_count;
  uint32_t successful_bullets;
  uint32_t failed_bullets;
  double overall_success_rate;
  bool system_validated;
} cns_tracer_manager_t;

/*═══════════════════════════════════════════════════════════════
  Function Pointer Types
  ═══════════════════════════════════════════════════════════════*/

typedef cns_result_t (*cns_tracer_step_function_t)(void *context);
typedef bool (*cns_tracer_validation_function_t)(cns_tracer_bullet_t *bullet);

/*═══════════════════════════════════════════════════════════════
  Core Functions
  ═══════════════════════════════════════════════════════════════*/

/**
 * Initialize tracer bullet manager
 */
cns_tracer_manager_t *cns_tracer_init(void);

/**
 * Create a new tracer bullet
 */
cns_result_t cns_tracer_create_bullet(
    cns_tracer_manager_t *manager,
    const char *name,
    const char *description,
    cns_tracer_type_t type);

/**
 * Add a step to a tracer bullet
 */
cns_result_t cns_tracer_add_step(
    cns_tracer_manager_t *manager,
    uint32_t bullet_id,
    const char *description,
    cns_tracer_step_function_t step_function,
    void *context);

/**
 * Execute a tracer bullet
 */
cns_result_t cns_tracer_execute_bullet(
    cns_tracer_manager_t *manager,
    uint32_t bullet_id);

/**
 * Execute all tracer bullets
 */
cns_result_t cns_tracer_execute_all(
    cns_tracer_manager_t *manager);

/**
 * Add validation function to bullet
 */
cns_result_t cns_tracer_add_validation(
    cns_tracer_manager_t *manager,
    uint32_t bullet_id,
    cns_tracer_validation_function_t validation_function);

/**
 * Add result to bullet
 */
cns_result_t cns_tracer_add_result(
    cns_tracer_manager_t *manager,
    uint32_t bullet_id,
    const char *name,
    const char *value);

/**
 * Get bullet status
 */
cns_bullet_status_t cns_tracer_get_bullet_status(
    cns_tracer_manager_t *manager,
    uint32_t bullet_id);

/**
 * Get bullet execution time
 */
uint64_t cns_tracer_get_bullet_execution_time(
    cns_tracer_manager_t *manager,
    uint32_t bullet_id);

/**
 * Get bullet report
 */
cns_result_t cns_tracer_get_bullet_report(
    cns_tracer_manager_t *manager,
    uint32_t bullet_id,
    char *report_buffer,
    size_t buffer_size);

/**
 * Get overall system validation report
 */
cns_result_t cns_tracer_get_system_report(
    cns_tracer_manager_t *manager,
    char *report_buffer,
    size_t buffer_size);

/**
 * Validate system based on all bullets
 */
cns_result_t cns_tracer_validate_system(
    cns_tracer_manager_t *manager);

/**
 * Cleanup tracer manager
 */
void cns_tracer_cleanup(cns_tracer_manager_t *manager);

/*═══════════════════════════════════════════════════════════════
  Utility Functions
  ═══════════════════════════════════════════════════════════════*/

/**
 * Get current timestamp in nanoseconds
 */
uint64_t cns_tracer_get_timestamp_ns(void);

/**
 * Calculate execution time in milliseconds
 */
double cns_tracer_calculate_execution_time_ms(
    uint64_t start_time_ns,
    uint64_t end_time_ns);

/**
 * Check if bullet execution timed out
 */
bool cns_tracer_is_timeout(
    uint64_t start_time_ns,
    uint64_t timeout_ms);

/**
 * Format execution time for display
 */
cns_result_t cns_tracer_format_time(
    uint64_t time_ns,
    char *buffer,
    size_t buffer_size);

#endif /* CNS_PRAGMATIC_TRACER_BULLETS_H */