/*  ─────────────────────────────────────────────────────────────
    cns/src/pragmatic/orthogonality.c  –  Orthogonality Implementation (v1.0)
    Independent, modular components with zero coupling
    ───────────────────────────────────────────────────────────── */

#include "cns/pragmatic/orthogonality.h"
#include "cns/telemetry/otel.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/*═══════════════════════════════════════════════════════════════
  Internal Functions
  ═══════════════════════════════════════════════════════════════*/

static bool has_dependency_cycle_recursive(
    cns_orthogonality_manager_t *manager,
    uint32_t component_id,
    bool *visited,
    bool *rec_stack);

static double calculate_component_coupling_score(
    cns_orthogonal_component_t *component,
    cns_orthogonality_manager_t *manager);

/*═══════════════════════════════════════════════════════════════
  Core Implementation
  ═══════════════════════════════════════════════════════════════*/

cns_orthogonality_manager_t *cns_orthogonality_init(void)
{
  otel_span_t span = otel_span_start("orthogonality.init");

  cns_orthogonality_manager_t *manager = malloc(sizeof(cns_orthogonality_manager_t));
  if (!manager)
  {
    otel_span_set_attribute(span, "error", "allocation_failed");
    otel_span_end(span);
    return NULL;
  }

  memset(manager, 0, sizeof(cns_orthogonality_manager_t));
  manager->component_count = 0;
  manager->overall_orthogonality_score = 1.0; // Start with perfect orthogonality
  manager->validation_passed = true;

  otel_span_set_attribute(span, "manager.components", 0);
  otel_span_set_attribute(span, "manager.score", 1.0);
  otel_span_end(span);

  return manager;
}

cns_result_t cns_orthogonality_register_component(
    cns_orthogonality_manager_t *manager,
    const char *name,
    cns_orthogonal_type_t type)
{
  otel_span_t span = otel_span_start("orthogonality.register_component");

  if (!manager || !name)
  {
    otel_span_set_attribute(span, "error", "invalid_parameters");
    otel_span_end(span);
    return CNS_ERROR_INVALID_PARAMETERS;
  }

  if (manager->component_count >= CNS_MAX_ORTHOGONAL_COMPONENTS)
  {
    otel_span_set_attribute(span, "error", "max_components_reached");
    otel_span_end(span);
    return CNS_ERROR_LIMIT_EXCEEDED;
  }

  cns_orthogonal_component_t *component = &manager->components[manager->component_count];
  component->component_id = manager->component_count;
  strncpy(component->name, name, sizeof(component->name) - 1);
  component->name[sizeof(component->name) - 1] = '\0';
  component->type = type;
  component->dependency_count = 0;
  component->coupling_score = 0.0;
  component->is_orthogonal = (type == CNS_ORTHOGONAL_TYPE_INDEPENDENT);

  manager->component_count++;

  otel_span_set_attribute(span, "component.id", component->component_id);
  otel_span_set_attribute(span, "component.name", name);
  otel_span_set_attribute(span, "component.type", type);
  otel_span_set_attribute(span, "manager.total_components", manager->component_count);
  otel_span_end(span);

  return CNS_SUCCESS;
}

cns_result_t cns_orthogonality_add_dependency(
    cns_orthogonality_manager_t *manager,
    uint32_t component_id,
    uint32_t dependency_id)
{
  otel_span_t span = otel_span_start("orthogonality.add_dependency");

  if (!manager)
  {
    otel_span_set_attribute(span, "error", "invalid_manager");
    otel_span_end(span);
    return CNS_ERROR_INVALID_PARAMETERS;
  }

  if (component_id >= manager->component_count ||
      dependency_id >= manager->component_count)
  {
    otel_span_set_attribute(span, "error", "invalid_component_id");
    otel_span_end(span);
    return CNS_ERROR_INVALID_PARAMETERS;
  }

  if (component_id == dependency_id)
  {
    otel_span_set_attribute(span, "error", "self_dependency");
    otel_span_end(span);
    return CNS_ERROR_INVALID_PARAMETERS;
  }

  cns_orthogonal_component_t *component = &manager->components[component_id];

  if (component->dependency_count >= CNS_MAX_DEPENDENCIES_PER_COMPONENT)
  {
    otel_span_set_attribute(span, "error", "max_dependencies_reached");
    otel_span_end(span);
    return CNS_ERROR_LIMIT_EXCEEDED;
  }

  // Check if dependency already exists
  for (uint32_t i = 0; i < component->dependency_count; i++)
  {
    if (component->dependencies[i] == dependency_id)
    {
      otel_span_set_attribute(span, "warning", "dependency_already_exists");
      otel_span_end(span);
      return CNS_SUCCESS; // Already exists, not an error
    }
  }

  component->dependencies[component->dependency_count] = dependency_id;
  component->dependency_count++;

  // Update component type based on dependencies
  if (component->dependency_count == 1)
  {
    component->type = CNS_ORTHOGONAL_TYPE_WEAK_COUPLED;
  }
  else if (component->dependency_count > 3)
  {
    component->type = CNS_ORTHOGONAL_TYPE_STRONG_COUPLED;
  }

  component->is_orthogonal = (component->type == CNS_ORTHOGONAL_TYPE_INDEPENDENT);

  otel_span_set_attribute(span, "component.id", component_id);
  otel_span_set_attribute(span, "dependency.id", dependency_id);
  otel_span_set_attribute(span, "component.dependency_count", component->dependency_count);
  otel_span_set_attribute(span, "component.type", component->type);
  otel_span_end(span);

  return CNS_SUCCESS;
}

cns_result_t cns_orthogonality_calculate_scores(
    cns_orthogonality_manager_t *manager)
{
  otel_span_t span = otel_span_start("orthogonality.calculate_scores");

  if (!manager)
  {
    otel_span_set_attribute(span, "error", "invalid_manager");
    otel_span_end(span);
    return CNS_ERROR_INVALID_PARAMETERS;
  }

  double total_score = 0.0;
  uint32_t valid_components = 0;

  for (uint32_t i = 0; i < manager->component_count; i++)
  {
    cns_orthogonal_component_t *component = &manager->components[i];

    // Calculate coupling score for this component
    component->coupling_score = calculate_component_coupling_score(component, manager);

    // Determine if component is orthogonal
    component->is_orthogonal = (component->coupling_score <= CNS_ORTHOGONALITY_THRESHOLD);

    if (component->is_orthogonal)
    {
      total_score += 1.0;
    }
    else
    {
      total_score += (1.0 - component->coupling_score);
    }
    valid_components++;
  }

  if (valid_components > 0)
  {
    manager->overall_orthogonality_score = total_score / valid_components;
  }
  else
  {
    manager->overall_orthogonality_score = 1.0;
  }

  otel_span_set_attribute(span, "overall_score", manager->overall_orthogonality_score);
  otel_span_set_attribute(span, "valid_components", valid_components);
  otel_span_end(span);

  return CNS_SUCCESS;
}

cns_result_t cns_orthogonality_validate(
    cns_orthogonality_manager_t *manager)
{
  otel_span_t span = otel_span_start("orthogonality.validate");

  if (!manager)
  {
    otel_span_set_attribute(span, "error", "invalid_manager");
    otel_span_end(span);
    return CNS_ERROR_INVALID_PARAMETERS;
  }

  // Check for circular dependencies
  if (cns_orthogonality_has_circular_dependencies(manager))
  {
    manager->validation_passed = false;
    otel_span_set_attribute(span, "validation.failed", "circular_dependencies");
    otel_span_end(span);
    return CNS_ERROR_VALIDATION_FAILED;
  }

  // Recalculate scores
  cns_result_t result = cns_orthogonality_calculate_scores(manager);
  if (result != CNS_SUCCESS)
  {
    manager->validation_passed = false;
    otel_span_set_attribute(span, "validation.failed", "score_calculation_failed");
    otel_span_end(span);
    return result;
  }

  // Check overall orthogonality score
  if (manager->overall_orthogonality_score < 0.8)
  {
    manager->validation_passed = false;
    otel_span_set_attribute(span, "validation.failed", "low_orthogonality_score");
    otel_span_set_attribute(span, "score", manager->overall_orthogonality_score);
    otel_span_end(span);
    return CNS_ERROR_VALIDATION_FAILED;
  }

  manager->validation_passed = true;
  otel_span_set_attribute(span, "validation.passed", true);
  otel_span_set_attribute(span, "score", manager->overall_orthogonality_score);
  otel_span_end(span);

  return CNS_SUCCESS;
}

cns_result_t cns_orthogonality_get_report(
    cns_orthogonality_manager_t *manager,
    char *report_buffer,
    size_t buffer_size)
{
  otel_span_t span = otel_span_start("orthogonality.get_report");

  if (!manager || !report_buffer)
  {
    otel_span_set_attribute(span, "error", "invalid_parameters");
    otel_span_end(span);
    return CNS_ERROR_INVALID_PARAMETERS;
  }

  int written = snprintf(report_buffer, buffer_size,
                         "=== ORTHOGONALITY REPORT ===\n"
                         "Overall Score: %.3f\n"
                         "Validation: %s\n"
                         "Components: %u\n\n",
                         manager->overall_orthogonality_score,
                         manager->validation_passed ? "PASSED" : "FAILED",
                         manager->component_count);

  if (written < 0 || (size_t)written >= buffer_size)
  {
    otel_span_set_attribute(span, "error", "buffer_overflow");
    otel_span_end(span);
    return CNS_ERROR_BUFFER_OVERFLOW;
  }

  size_t offset = written;

  for (uint32_t i = 0; i < manager->component_count; i++)
  {
    cns_orthogonal_component_t *component = &manager->components[i];

    written = snprintf(report_buffer + offset, buffer_size - offset,
                       "Component %u: %s\n"
                       "  Type: %u\n"
                       "  Dependencies: %u\n"
                       "  Coupling Score: %.3f\n"
                       "  Orthogonal: %s\n\n",
                       component->component_id,
                       component->name,
                       component->type,
                       component->dependency_count,
                       component->coupling_score,
                       component->is_orthogonal ? "YES" : "NO");

    if (written < 0 || (size_t)written >= buffer_size - offset)
    {
      otel_span_set_attribute(span, "error", "buffer_overflow");
      otel_span_end(span);
      return CNS_ERROR_BUFFER_OVERFLOW;
    }

    offset += written;
  }

  otel_span_set_attribute(span, "report.size", offset);
  otel_span_end(span);

  return CNS_SUCCESS;
}

bool cns_orthogonality_is_component_orthogonal(
    cns_orthogonality_manager_t *manager,
    uint32_t component_id)
{
  if (!manager || component_id >= manager->component_count)
  {
    return false;
  }

  return manager->components[component_id].is_orthogonal;
}

double cns_orthogonality_get_overall_score(
    cns_orthogonality_manager_t *manager)
{
  if (!manager)
  {
    return 0.0;
  }

  return manager->overall_orthogonality_score;
}

void cns_orthogonality_cleanup(cns_orthogonality_manager_t *manager)
{
  if (manager)
  {
    free(manager);
  }
}

/*═══════════════════════════════════════════════════════════════
  Utility Functions
  ═══════════════════════════════════════════════════════════════*/

double cns_orthogonality_calculate_coupling(
    cns_orthogonal_component_t *component1,
    cns_orthogonal_component_t *component2)
{
  if (!component1 || !component2)
  {
    return 1.0; // Maximum coupling for invalid components
  }

  // Check if component2 is a dependency of component1
  for (uint32_t i = 0; i < component1->dependency_count; i++)
  {
    if (component1->dependencies[i] == component2->component_id)
    {
      return 0.5; // Direct dependency coupling
    }
  }

  // Check if component1 is a dependency of component2
  for (uint32_t i = 0; i < component2->dependency_count; i++)
  {
    if (component2->dependencies[i] == component1->component_id)
    {
      return 0.5; // Reverse dependency coupling
    }
  }

  return 0.0; // No coupling
}

bool cns_orthogonality_has_circular_dependencies(
    cns_orthogonality_manager_t *manager)
{
  if (!manager)
  {
    return false;
  }

  bool *visited = calloc(manager->component_count, sizeof(bool));
  bool *rec_stack = calloc(manager->component_count, sizeof(bool));

  if (!visited || !rec_stack)
  {
    free(visited);
    free(rec_stack);
    return false;
  }

  bool has_cycle = false;

  for (uint32_t i = 0; i < manager->component_count; i++)
  {
    if (!visited[i])
    {
      if (has_dependency_cycle_recursive(manager, i, visited, rec_stack))
      {
        has_cycle = true;
        break;
      }
    }
  }

  free(visited);
  free(rec_stack);

  return has_cycle;
}

cns_result_t cns_orthogonality_suggest_decoupling(
    cns_orthogonality_manager_t *manager,
    uint32_t component_id,
    char *suggestion_buffer,
    size_t buffer_size)
{
  otel_span_t span = otel_span_start("orthogonality.suggest_decoupling");

  if (!manager || !suggestion_buffer || component_id >= manager->component_count)
  {
    otel_span_set_attribute(span, "error", "invalid_parameters");
    otel_span_end(span);
    return CNS_ERROR_INVALID_PARAMETERS;
  }

  cns_orthogonal_component_t *component = &manager->components[component_id];

  int written = snprintf(suggestion_buffer, buffer_size,
                         "Decoupling suggestions for component '%s':\n",
                         component->name);

  if (written < 0 || (size_t)written >= buffer_size)
  {
    otel_span_set_attribute(span, "error", "buffer_overflow");
    otel_span_end(span);
    return CNS_ERROR_BUFFER_OVERFLOW;
  }

  size_t offset = written;

  if (component->dependency_count == 0)
  {
    written = snprintf(suggestion_buffer + offset, buffer_size - offset,
                       "- Component is already orthogonal (no dependencies)\n");
  }
  else
  {
    written = snprintf(suggestion_buffer + offset, buffer_size - offset,
                       "- Consider removing %u dependencies to improve orthogonality\n"
                       "- Dependencies: ",
                       component->dependency_count);

    if (written < 0 || (size_t)written >= buffer_size - offset)
    {
      otel_span_set_attribute(span, "error", "buffer_overflow");
      otel_span_end(span);
      return CNS_ERROR_BUFFER_OVERFLOW;
    }

    offset += written;

    for (uint32_t i = 0; i < component->dependency_count; i++)
    {
      uint32_t dep_id = component->dependencies[i];
      if (dep_id < manager->component_count)
      {
        written = snprintf(suggestion_buffer + offset, buffer_size - offset,
                           "%s%s",
                           i > 0 ? ", " : "",
                           manager->components[dep_id].name);

        if (written < 0 || (size_t)written >= buffer_size - offset)
        {
          otel_span_set_attribute(span, "error", "buffer_overflow");
          otel_span_end(span);
          return CNS_ERROR_BUFFER_OVERFLOW;
        }

        offset += written;
      }
    }

    written = snprintf(suggestion_buffer + offset, buffer_size - offset, "\n");
    if (written < 0 || (size_t)written >= buffer_size - offset)
    {
      otel_span_set_attribute(span, "error", "buffer_overflow");
      otel_span_end(span);
      return CNS_ERROR_BUFFER_OVERFLOW;
    }

    offset += written;
  }

  otel_span_set_attribute(span, "suggestion.size", offset);
  otel_span_end(span);

  return CNS_SUCCESS;
}

/*═══════════════════════════════════════════════════════════════
  Internal Helper Functions
  ═══════════════════════════════════════════════════════════════*/

static bool has_dependency_cycle_recursive(
    cns_orthogonality_manager_t *manager,
    uint32_t component_id,
    bool *visited,
    bool *rec_stack)
{
  if (!visited[component_id])
  {
    visited[component_id] = true;
    rec_stack[component_id] = true;

    cns_orthogonal_component_t *component = &manager->components[component_id];

    for (uint32_t i = 0; i < component->dependency_count; i++)
    {
      uint32_t dep_id = component->dependencies[i];

      if (!visited[dep_id] &&
          has_dependency_cycle_recursive(manager, dep_id, visited, rec_stack))
      {
        return true;
      }
      else if (rec_stack[dep_id])
      {
        return true;
      }
    }
  }

  rec_stack[component_id] = false;
  return false;
}

static double calculate_component_coupling_score(
    cns_orthogonal_component_t *component,
    cns_orthogonality_manager_t *manager)
{
  if (!component || !manager)
  {
    return 1.0;
  }

  if (component->dependency_count == 0)
  {
    return 0.0; // Perfect orthogonality
  }

  // Calculate coupling based on number of dependencies
  double base_coupling = (double)component->dependency_count / CNS_MAX_DEPENDENCIES_PER_COMPONENT;

  // Penalize strong coupling types
  double type_penalty = 0.0;
  switch (component->type)
  {
  case CNS_ORTHOGONAL_TYPE_INDEPENDENT:
    type_penalty = 0.0;
    break;
  case CNS_ORTHOGONAL_TYPE_WEAK_COUPLED:
    type_penalty = 0.1;
    break;
  case CNS_ORTHOGONAL_TYPE_STRONG_COUPLED:
    type_penalty = 0.3;
    break;
  case CNS_ORTHOGONAL_TYPE_TIGHTLY_COUPLED:
    type_penalty = 0.5;
    break;
  }

  return base_coupling + type_penalty;
}