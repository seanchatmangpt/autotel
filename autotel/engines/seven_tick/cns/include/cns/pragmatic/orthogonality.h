/*  ─────────────────────────────────────────────────────────────
    cns/include/cns/pragmatic/orthogonality.h  –  Orthogonality (v1.0)
    Independent, modular components with zero coupling
    ───────────────────────────────────────────────────────────── */
#ifndef CNS_PRAGMATIC_ORTHOGONALITY_H
#define CNS_PRAGMATIC_ORTHOGONALITY_H

#include "cns/types.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*═══════════════════════════════════════════════════════════════
  Configuration
  ═══════════════════════════════════════════════════════════════*/

#define CNS_MAX_ORTHOGONAL_COMPONENTS 64
#define CNS_MAX_DEPENDENCIES_PER_COMPONENT 8
#define CNS_ORTHOGONALITY_THRESHOLD 0.1 // Max coupling allowed

/*═══════════════════════════════════════════════════════════════
  Orthogonality Types
  ═══════════════════════════════════════════════════════════════*/

typedef enum
{
  CNS_ORTHOGONAL_TYPE_INDEPENDENT,    // No dependencies
  CNS_ORTHOGONAL_TYPE_WEAK_COUPLED,   // Minimal dependencies
  CNS_ORTHOGONAL_TYPE_STRONG_COUPLED, // High dependencies
  CNS_ORTHOGONAL_TYPE_TIGHTLY_COUPLED // Avoid this
} cns_orthogonal_type_t;

/*═══════════════════════════════════════════════════════════════
  Component Structure
  ═══════════════════════════════════════════════════════════════*/

typedef struct
{
  uint32_t component_id;
  char name[64];
  cns_orthogonal_type_t type;
  uint32_t dependency_count;
  uint32_t dependencies[CNS_MAX_DEPENDENCIES_PER_COMPONENT];
  double coupling_score;
  bool is_orthogonal;
} cns_orthogonal_component_t;

/*═══════════════════════════════════════════════════════════════
  Orthogonality Manager
  ═══════════════════════════════════════════════════════════════*/

typedef struct
{
  cns_orthogonal_component_t components[CNS_MAX_ORTHOGONAL_COMPONENTS];
  uint32_t component_count;
  double overall_orthogonality_score;
  bool validation_passed;
} cns_orthogonality_manager_t;

/*═══════════════════════════════════════════════════════════════
  Core Functions
  ═══════════════════════════════════════════════════════════════*/

/**
 * Initialize orthogonality manager
 */
cns_orthogonality_manager_t *cns_orthogonality_init(void);

/**
 * Register a component for orthogonality analysis
 */
cns_result_t cns_orthogonality_register_component(
    cns_orthogonality_manager_t *manager,
    const char *name,
    cns_orthogonal_type_t type);

/**
 * Add dependency between components
 */
cns_result_t cns_orthogonality_add_dependency(
    cns_orthogonality_manager_t *manager,
    uint32_t component_id,
    uint32_t dependency_id);

/**
 * Calculate orthogonality scores
 */
cns_result_t cns_orthogonality_calculate_scores(
    cns_orthogonality_manager_t *manager);

/**
 * Validate orthogonality constraints
 */
cns_result_t cns_orthogonality_validate(
    cns_orthogonality_manager_t *manager);

/**
 * Get component orthogonality report
 */
cns_result_t cns_orthogonality_get_report(
    cns_orthogonality_manager_t *manager,
    char *report_buffer,
    size_t buffer_size);

/**
 * Check if component is orthogonal
 */
bool cns_orthogonality_is_component_orthogonal(
    cns_orthogonality_manager_t *manager,
    uint32_t component_id);

/**
 * Get overall orthogonality score
 */
double cns_orthogonality_get_overall_score(
    cns_orthogonality_manager_t *manager);

/**
 * Cleanup orthogonality manager
 */
void cns_orthogonality_cleanup(cns_orthogonality_manager_t *manager);

/*═══════════════════════════════════════════════════════════════
  Utility Functions
  ═══════════════════════════════════════════════════════════════*/

/**
 * Calculate coupling score between two components
 */
double cns_orthogonality_calculate_coupling(
    cns_orthogonal_component_t *component1,
    cns_orthogonal_component_t *component2);

/**
 * Check for circular dependencies
 */
bool cns_orthogonality_has_circular_dependencies(
    cns_orthogonality_manager_t *manager);

/**
 * Suggest component decoupling
 */
cns_result_t cns_orthogonality_suggest_decoupling(
    cns_orthogonality_manager_t *manager,
    uint32_t component_id,
    char *suggestion_buffer,
    size_t buffer_size);

#endif /* CNS_PRAGMATIC_ORTHOGONALITY_H */