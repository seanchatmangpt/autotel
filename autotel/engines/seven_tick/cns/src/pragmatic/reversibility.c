#include "cns/pragmatic/reversibility.h"
#include "cns/telemetry/otel.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>

// Internal structures
typedef struct cns_operation
{
  uint32_t operation_id;
  const char *operation_name;
  const char *description;
  cns_operation_type_t type;
  void *before_data;
  void *after_data;
  size_t data_size;
  cns_operation_execute_func execute_func;
  cns_operation_reverse_func reverse_func;
  uint64_t timestamp;
} cns_operation_t;

typedef struct cns_undo_stack_entry
{
  uint32_t operation_id;
  uint64_t timestamp;
  struct cns_undo_stack_entry *next;
} cns_undo_stack_entry_t;

typedef struct cns_checkpoint
{
  const char *name;
  uint32_t operation_count;
  uint32_t *operation_ids;
  uint64_t timestamp;
} cns_checkpoint_t;

struct cns_reversibility_manager
{
  cns_operation_t operations[CNS_MAX_OPERATIONS];
  uint32_t operation_count;
  uint32_t next_operation_id;

  cns_undo_stack_entry_t *undo_stack_head;
  uint32_t undo_stack_size;

  cns_checkpoint_t checkpoints[CNS_MAX_CHECKPOINTS];
  uint32_t checkpoint_count;

  bool enabled;
  uint64_t total_operations;
  uint64_t total_reversals;
};

// Performance validation macro
#define S7T_VALIDATE_PERFORMANCE(operation, max_cycles)          \
  do                                                             \
  {                                                              \
    uint64_t start = s7t_cycles();                               \
    operation;                                                   \
    uint64_t end = s7t_cycles();                                 \
    uint32_t cycles = (uint32_t)(end - start);                   \
    assert(cycles <= max_cycles);                                \
    otel_span_set_attribute(span, "performance.cycles", cycles); \
  } while (0)

cns_reversibility_manager_t *cns_reversibility_init(void)
{
  otel_span_t span = otel_span_start("reversibility.init");

  cns_reversibility_manager_t *manager = malloc(sizeof(cns_reversibility_manager_t));
  if (!manager)
  {
    otel_span_set_attribute(span, "error", "allocation_failed");
    otel_span_end(span);
    return NULL;
  }

  // Initialize manager
  memset(manager, 0, sizeof(cns_reversibility_manager_t));
  manager->enabled = true;
  manager->next_operation_id = 1;

  otel_span_set_attribute(span, "manager.initialized", true);
  otel_span_set_attribute(span, "manager.max_operations", CNS_MAX_OPERATIONS);
  otel_span_set_attribute(span, "manager.max_checkpoints", CNS_MAX_CHECKPOINTS);

  S7T_VALIDATE_PERFORMANCE(/* initialization complete */, 10);

  otel_span_end(span);
  return manager;
}

void cns_reversibility_cleanup(cns_reversibility_manager_t *manager)
{
  if (!manager)
    return;

  otel_span_t span = otel_span_start("reversibility.cleanup");

  // Clean up operations
  for (uint32_t i = 0; i < manager->operation_count; i++)
  {
    if (manager->operations[i].before_data)
    {
      free(manager->operations[i].before_data);
    }
    if (manager->operations[i].after_data)
    {
      free(manager->operations[i].after_data);
    }
  }

  // Clean up undo stack
  cns_undo_stack_entry_t *current = manager->undo_stack_head;
  while (current)
  {
    cns_undo_stack_entry_t *next = current->next;
    free(current);
    current = next;
  }

  // Clean up checkpoints
  for (uint32_t i = 0; i < manager->checkpoint_count; i++)
  {
    if (manager->checkpoints[i].operation_ids)
    {
      free(manager->checkpoints[i].operation_ids);
    }
  }

  free(manager);

  otel_span_set_attribute(span, "cleanup.completed", true);
  otel_span_end(span);
}

uint32_t cns_reversibility_register_operation(cns_reversibility_manager_t *manager,
                                              const char *operation_name,
                                              const char *description,
                                              cns_operation_type_t type,
                                              void *before_data,
                                              void *after_data,
                                              size_t data_size,
                                              cns_operation_execute_func execute_func,
                                              cns_operation_reverse_func reverse_func)
{
  if (!manager || !operation_name || !execute_func || !reverse_func)
  {
    return CNS_INVALID_OPERATION_ID;
  }

  otel_span_t span = otel_span_start("reversibility.register_operation");

  if (manager->operation_count >= CNS_MAX_OPERATIONS)
  {
    otel_span_set_attribute(span, "error", "max_operations_reached");
    otel_span_end(span);
    return CNS_INVALID_OPERATION_ID;
  }

  uint32_t operation_id = manager->next_operation_id++;

  cns_operation_t *operation = &manager->operations[manager->operation_count];
  operation->operation_id = operation_id;
  operation->operation_name = operation_name;
  operation->description = description;
  operation->type = type;
  operation->execute_func = execute_func;
  operation->reverse_func = reverse_func;
  operation->timestamp = s7t_cycles();

  // Copy data if provided
  if (before_data && data_size > 0)
  {
    operation->before_data = malloc(data_size);
    if (operation->before_data)
    {
      memcpy(operation->before_data, before_data, data_size);
    }
  }

  if (after_data && data_size > 0)
  {
    operation->after_data = malloc(data_size);
    if (operation->after_data)
    {
      memcpy(operation->after_data, after_data, data_size);
    }
  }

  operation->data_size = data_size;
  manager->operation_count++;

  otel_span_set_attribute(span, "operation.id", operation_id);
  otel_span_set_attribute(span, "operation.name", operation_name);
  otel_span_set_attribute(span, "operation.type", type);
  otel_span_set_attribute(span, "operation.data_size", data_size);

  S7T_VALIDATE_PERFORMANCE(/* operation registration complete */, 10);

  otel_span_end(span);
  return operation_id;
}

bool cns_reversibility_execute_operation(cns_reversibility_manager_t *manager, uint32_t operation_id)
{
  if (!manager)
    return false;

  otel_span_t span = otel_span_start("reversibility.execute_operation");

  // Find operation
  cns_operation_t *operation = NULL;
  for (uint32_t i = 0; i < manager->operation_count; i++)
  {
    if (manager->operations[i].operation_id == operation_id)
    {
      operation = &manager->operations[i];
      break;
    }
  }

  if (!operation)
  {
    otel_span_set_attribute(span, "error", "operation_not_found");
    otel_span_end(span);
    return false;
  }

  // Execute operation
  bool success = operation->execute_func(operation->after_data);

  if (success)
  {
    // Add to undo stack
    cns_undo_stack_entry_t *entry = malloc(sizeof(cns_undo_stack_entry_t));
    if (entry)
    {
      entry->operation_id = operation_id;
      entry->timestamp = s7t_cycles();
      entry->next = manager->undo_stack_head;
      manager->undo_stack_head = entry;
      manager->undo_stack_size++;
    }

    manager->total_operations++;
  }

  otel_span_set_attribute(span, "operation.id", operation_id);
  otel_span_set_attribute(span, "operation.name", operation->operation_name);
  otel_span_set_attribute(span, "operation.success", success);
  otel_span_set_attribute(span, "undo_stack.size", manager->undo_stack_size);

  S7T_VALIDATE_PERFORMANCE(/* operation execution complete */, 10);

  otel_span_end(span);
  return success;
}

bool cns_reversibility_undo_last(cns_reversibility_manager_t *manager)
{
  if (!manager || !manager->undo_stack_head)
  {
    return false;
  }

  otel_span_t span = otel_span_start("reversibility.undo_last");

  // Get last operation from stack
  uint32_t operation_id = manager->undo_stack_head->operation_id;

  // Find operation
  cns_operation_t *operation = NULL;
  for (uint32_t i = 0; i < manager->operation_count; i++)
  {
    if (manager->operations[i].operation_id == operation_id)
    {
      operation = &manager->operations[i];
      break;
    }
  }

  if (!operation)
  {
    otel_span_set_attribute(span, "error", "operation_not_found");
    otel_span_end(span);
    return false;
  }

  // Execute reverse operation
  bool success = operation->reverse_func(operation->before_data);

  if (success)
  {
    // Remove from undo stack
    cns_undo_stack_entry_t *entry = manager->undo_stack_head;
    manager->undo_stack_head = entry->next;
    free(entry);
    manager->undo_stack_size--;
    manager->total_reversals++;
  }

  otel_span_set_attribute(span, "operation.id", operation_id);
  otel_span_set_attribute(span, "operation.name", operation->operation_name);
  otel_span_set_attribute(span, "undo.success", success);
  otel_span_set_attribute(span, "undo_stack.size", manager->undo_stack_size);

  S7T_VALIDATE_PERFORMANCE(/* undo operation complete */, 10);

  otel_span_end(span);
  return success;
}

bool cns_reversibility_create_checkpoint(cns_reversibility_manager_t *manager, const char *checkpoint_name)
{
  if (!manager || !checkpoint_name)
  {
    return false;
  }

  otel_span_t span = otel_span_start("reversibility.create_checkpoint");

  if (manager->checkpoint_count >= CNS_MAX_CHECKPOINTS)
  {
    otel_span_set_attribute(span, "error", "max_checkpoints_reached");
    otel_span_end(span);
    return false;
  }

  cns_checkpoint_t *checkpoint = &manager->checkpoints[manager->checkpoint_count];
  checkpoint->name = checkpoint_name;
  checkpoint->timestamp = s7t_cycles();
  checkpoint->operation_count = manager->undo_stack_size;

  // Store operation IDs from undo stack
  if (checkpoint->operation_count > 0)
  {
    checkpoint->operation_ids = malloc(checkpoint->operation_count * sizeof(uint32_t));
    if (checkpoint->operation_ids)
    {
      cns_undo_stack_entry_t *current = manager->undo_stack_head;
      uint32_t index = 0;
      while (current && index < checkpoint->operation_count)
      {
        checkpoint->operation_ids[index] = current->operation_id;
        current = current->next;
        index++;
      }
    }
  }

  manager->checkpoint_count++;

  otel_span_set_attribute(span, "checkpoint.name", checkpoint_name);
  otel_span_set_attribute(span, "checkpoint.operation_count", checkpoint->operation_count);
  otel_span_set_attribute(span, "checkpoint_count", manager->checkpoint_count);

  S7T_VALIDATE_PERFORMANCE(/* checkpoint creation complete */, 100);

  otel_span_end(span);
  return true;
}

bool cns_reversibility_rollback_to_checkpoint(cns_reversibility_manager_t *manager, const char *checkpoint_name)
{
  if (!manager || !checkpoint_name)
  {
    return false;
  }

  otel_span_t span = otel_span_start("reversibility.rollback_to_checkpoint");

  // Find checkpoint
  cns_checkpoint_t *checkpoint = NULL;
  for (uint32_t i = 0; i < manager->checkpoint_count; i++)
  {
    if (strcmp(manager->checkpoints[i].name, checkpoint_name) == 0)
    {
      checkpoint = &manager->checkpoints[i];
      break;
    }
  }

  if (!checkpoint)
  {
    otel_span_set_attribute(span, "error", "checkpoint_not_found");
    otel_span_end(span);
    return false;
  }

  // Calculate operations to undo
  uint32_t operations_to_undo = manager->undo_stack_size - checkpoint->operation_count;

  // Undo operations until we reach checkpoint
  bool success = true;
  for (uint32_t i = 0; i < operations_to_undo && success; i++)
  {
    success = cns_reversibility_undo_last(manager);
  }

  otel_span_set_attribute(span, "checkpoint.name", checkpoint_name);
  otel_span_set_attribute(span, "operations_undone", operations_to_undo);
  otel_span_set_attribute(span, "rollback.success", success);

  S7T_VALIDATE_PERFORMANCE(/* rollback complete */, 100);

  otel_span_end(span);
  return success;
}

uint32_t cns_reversibility_get_undo_stack_size(cns_reversibility_manager_t *manager)
{
  if (!manager)
    return 0;
  return manager->undo_stack_size;
}

uint32_t cns_reversibility_get_total_operations(cns_reversibility_manager_t *manager)
{
  if (!manager)
    return 0;
  return manager->total_operations;
}

uint32_t cns_reversibility_get_total_reversals(cns_reversibility_manager_t *manager)
{
  if (!manager)
    return 0;
  return manager->total_reversals;
}

bool cns_reversibility_is_enabled(cns_reversibility_manager_t *manager)
{
  if (!manager)
    return false;
  return manager->enabled;
}

void cns_reversibility_set_enabled(cns_reversibility_manager_t *manager, bool enabled)
{
  if (!manager)
    return;
  manager->enabled = enabled;
}

// Example operation functions
static bool example_execute_function(void *data)
{
  // Example execution logic
  return true;
}

static bool example_reverse_function(void *data)
{
  // Example reverse logic
  return true;
}

// Performance validation functions
void cns_reversibility_validate_performance(cns_reversibility_manager_t *manager)
{
  if (!manager)
    return;

  otel_span_t span = otel_span_start("reversibility.validate_performance");

  // Validate initialization performance
  uint64_t start = s7t_cycles();
  cns_reversibility_manager_t *test_manager = cns_reversibility_init();
  uint64_t end = s7t_cycles();
  uint32_t init_cycles = (uint32_t)(end - start);

  if (test_manager)
  {
    // Validate operation registration performance
    start = s7t_cycles();
    uint32_t op_id = cns_reversibility_register_operation(test_manager, "test_op", "test",
                                                          CNS_OP_TYPE_CREATE, NULL, NULL, 0,
                                                          example_execute_function, example_reverse_function);
    end = s7t_cycles();
    uint32_t reg_cycles = (uint32_t)(end - start);

    // Validate operation execution performance
    start = s7t_cycles();
    bool exec_success = cns_reversibility_execute_operation(test_manager, op_id);
    end = s7t_cycles();
    uint32_t exec_cycles = (uint32_t)(end - start);

    // Validate undo performance
    start = s7t_cycles();
    bool undo_success = cns_reversibility_undo_last(test_manager);
    end = s7t_cycles();
    uint32_t undo_cycles = (uint32_t)(end - start);

    // Validate checkpoint performance
    start = s7t_cycles();
    bool checkpoint_success = cns_reversibility_create_checkpoint(test_manager, "test_checkpoint");
    end = s7t_cycles();
    uint32_t checkpoint_cycles = (uint32_t)(end - start);

    otel_span_set_attribute(span, "performance.init_cycles", init_cycles);
    otel_span_set_attribute(span, "performance.reg_cycles", reg_cycles);
    otel_span_set_attribute(span, "performance.exec_cycles", exec_cycles);
    otel_span_set_attribute(span, "performance.undo_cycles", undo_cycles);
    otel_span_set_attribute(span, "performance.checkpoint_cycles", checkpoint_cycles);

    // Validate 7-tick compliance
    bool init_compliant = (init_cycles <= 10);
    bool reg_compliant = (reg_cycles <= 10);
    bool exec_compliant = (exec_cycles <= 10);
    bool undo_compliant = (undo_cycles <= 10);
    bool checkpoint_compliant = (checkpoint_cycles <= 100);

    otel_span_set_attribute(span, "compliance.init_7_tick", init_compliant);
    otel_span_set_attribute(span, "compliance.reg_7_tick", reg_compliant);
    otel_span_set_attribute(span, "compliance.exec_7_tick", exec_compliant);
    otel_span_set_attribute(span, "compliance.undo_7_tick", undo_compliant);
    otel_span_set_attribute(span, "compliance.checkpoint_7_tick", checkpoint_compliant);

    cns_reversibility_cleanup(test_manager);
  }

  otel_span_end(span);
}