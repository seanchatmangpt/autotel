/*  ─────────────────────────────────────────────────────────────
    cns/include/cns/pragmatic/reversibility.h  –  Reversibility (v1.0)
    Ability to undo changes and maintain system state
    ───────────────────────────────────────────────────────────── */
#ifndef CNS_PRAGMATIC_REVERSIBILITY_H
#define CNS_PRAGMATIC_REVERSIBILITY_H

#include "cns/types.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <time.h>

/*═══════════════════════════════════════════════════════════════
  Configuration
  ═══════════════════════════════════════════════════════════════*/

#define CNS_MAX_REVERSIBLE_OPERATIONS 1024
#define CNS_MAX_OPERATION_DATA_SIZE 4096
#define CNS_MAX_UNDO_STACK_SIZE 256
#define CNS_REVERSIBILITY_TIMEOUT_MS 30000

/*═══════════════════════════════════════════════════════════════
  Operation Types
  ═══════════════════════════════════════════════════════════════*/

typedef enum
{
  CNS_OP_TYPE_CREATE,    // Create new resource
  CNS_OP_TYPE_UPDATE,    // Update existing resource
  CNS_OP_TYPE_DELETE,    // Delete resource
  CNS_OP_TYPE_CONFIGURE, // Configuration change
  CNS_OP_TYPE_STATE,     // State change
  CNS_OP_TYPE_CUSTOM     // Custom operation
} cns_operation_type_t;

typedef enum
{
  CNS_OP_STATUS_PENDING,
  CNS_OP_STATUS_EXECUTED,
  CNS_OP_STATUS_REVERSED,
  CNS_OP_STATUS_FAILED
} cns_operation_status_t;

/*═══════════════════════════════════════════════════════════════
  Reversible Operation
  ═══════════════════════════════════════════════════════════════*/

typedef struct
{
  uint32_t operation_id;
  char name[64];
  char description[256];
  cns_operation_type_t type;
  cns_operation_status_t status;

  uint64_t timestamp_ns;
  uint64_t execution_time_ns;

  // Operation data (before and after states)
  uint32_t data_size;
  uint8_t before_data[CNS_MAX_OPERATION_DATA_SIZE];
  uint8_t after_data[CNS_MAX_OPERATION_DATA_SIZE];

  // Reversibility information
  bool is_reversible;
  char reverse_description[256];
  uint32_t reverse_operation_id;

  // Dependencies
  uint32_t dependency_count;
  uint32_t dependencies[16];

  // Error information
  cns_result_t result;
  char error_message[512];
} cns_reversible_operation_t;

/*═══════════════════════════════════════════════════════════════
  Undo Stack
  ═══════════════════════════════════════════════════════════════*/

typedef struct
{
  uint32_t stack_id;
  char name[64];
  uint32_t operation_count;
  uint32_t operations[CNS_MAX_UNDO_STACK_SIZE];
  uint64_t created_time_ns;
  bool is_active;
} cns_undo_stack_t;

/*═══════════════════════════════════════════════════════════════
  Reversibility Manager
  ═══════════════════════════════════════════════════════════════*/

typedef struct
{
  cns_reversible_operation_t operations[CNS_MAX_REVERSIBLE_OPERATIONS];
  uint32_t operation_count;
  uint32_t next_operation_id;

  cns_undo_stack_t undo_stacks[8];
  uint32_t stack_count;
  uint32_t active_stack_id;

  bool reversibility_enabled;
  uint64_t last_checkpoint_ns;
  double overall_reversibility_score;
} cns_reversibility_manager_t;

/*═══════════════════════════════════════════════════════════════
  Function Pointer Types
  ═══════════════════════════════════════════════════════════════*/

typedef cns_result_t (*cns_operation_execute_t)(void *before_data, void *after_data, uint32_t data_size);
typedef cns_result_t (*cns_operation_reverse_t)(void *before_data, void *after_data, uint32_t data_size);

/*═══════════════════════════════════════════════════════════════
  Core Functions
  ═══════════════════════════════════════════════════════════════*/

/**
 * Initialize reversibility manager
 */
cns_reversibility_manager_t *cns_reversibility_init(void);

/**
 * Register a reversible operation
 */
cns_result_t cns_reversibility_register_operation(
    cns_reversibility_manager_t *manager,
    const char *name,
    const char *description,
    cns_operation_type_t type,
    const void *before_data,
    const void *after_data,
    uint32_t data_size,
    cns_operation_execute_t execute_func,
    cns_operation_reverse_t reverse_func);

/**
 * Execute a registered operation
 */
cns_result_t cns_reversibility_execute_operation(
    cns_reversibility_manager_t *manager,
    uint32_t operation_id);

/**
 * Reverse an operation
 */
cns_result_t cns_reversibility_reverse_operation(
    cns_reversibility_manager_t *manager,
    uint32_t operation_id);

/**
 * Create a new undo stack
 */
cns_result_t cns_reversibility_create_undo_stack(
    cns_reversibility_manager_t *manager,
    const char *name);

/**
 * Add operation to current undo stack
 */
cns_result_t cns_reversibility_add_to_stack(
    cns_reversibility_manager_t *manager,
    uint32_t operation_id);

/**
 * Undo last operation in stack
 */
cns_result_t cns_reversibility_undo_last(
    cns_reversibility_manager_t *manager);

/**
 * Undo all operations in stack
 */
cns_result_t cns_reversibility_undo_all(
    cns_reversibility_manager_t *manager);

/**
 * Create checkpoint
 */
cns_result_t cns_reversibility_create_checkpoint(
    cns_reversibility_manager_t *manager,
    const char *checkpoint_name);

/**
 * Rollback to checkpoint
 */
cns_result_t cns_reversibility_rollback_to_checkpoint(
    cns_reversibility_manager_t *manager,
    const char *checkpoint_name);

/**
 * Get operation status
 */
cns_operation_status_t cns_reversibility_get_operation_status(
    cns_reversibility_manager_t *manager,
    uint32_t operation_id);

/**
 * Check if operation is reversible
 */
bool cns_reversibility_is_operation_reversible(
    cns_reversibility_manager_t *manager,
    uint32_t operation_id);

/**
 * Get reversibility report
 */
cns_result_t cns_reversibility_get_report(
    cns_reversibility_manager_t *manager,
    char *report_buffer,
    size_t buffer_size);

/**
 * Validate reversibility constraints
 */
cns_result_t cns_reversibility_validate(
    cns_reversibility_manager_t *manager);

/**
 * Cleanup reversibility manager
 */
void cns_reversibility_cleanup(cns_reversibility_manager_t *manager);

/*═══════════════════════════════════════════════════════════════
  Utility Functions
  ═══════════════════════════════════════════════════════════════*/

/**
 * Get current timestamp in nanoseconds
 */
uint64_t cns_reversibility_get_timestamp_ns(void);

/**
 * Calculate operation execution time
 */
uint64_t cns_reversibility_calculate_execution_time(
    uint64_t start_time_ns,
    uint64_t end_time_ns);

/**
 * Check if operation timed out
 */
bool cns_reversibility_is_timeout(
    uint64_t start_time_ns,
    uint64_t timeout_ms);

/**
 * Calculate reversibility score
 */
double cns_reversibility_calculate_score(
    cns_reversibility_manager_t *manager);

#endif /* CNS_PRAGMATIC_REVERSIBILITY_H */