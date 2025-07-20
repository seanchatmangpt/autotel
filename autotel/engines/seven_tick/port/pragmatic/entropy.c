/*  ─────────────────────────────────────────────────────────────
    cns/src/pragmatic/entropy.c  –  Entropy Management (v2.0)
    Software entropy management and technical debt tracking
    ───────────────────────────────────────────────────────────── */

#include "cns/pragmatic/entropy.h"
#include "cns/telemetry/otel.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/*═══════════════════════════════════════════════════════════════
  Entropy Management System
  ═══════════════════════════════════════════════════════════════*/

static cns_entropy_system_t g_entropy_system = {0};

// Initialize entropy management system
CNSResult cns_entropy_init(cns_entropy_system_t *system)
{
  if (!system)
  {
    return CNS_ERR_INVALID_ARG;
  }

  memset(system, 0, sizeof(cns_entropy_system_t));
  system->initialized = true;
  system->start_time = time(NULL);
  system->total_operations = 0;
  system->entropy_score = 0.0;

  // Initialize entropy sources
  system->sources = malloc(sizeof(cns_entropy_source_t) * CNS_MAX_ENTROPY_SOURCES);
  if (!system->sources)
  {
    return CNS_ERR_RESOURCE;
  }

  memset(system->sources, 0, sizeof(cns_entropy_source_t) * CNS_MAX_ENTROPY_SOURCES);

  printf("✅ Entropy management system initialized\n");
  return CNS_OK;
}

// Shutdown entropy management system
void cns_entropy_shutdown(cns_entropy_system_t *system)
{
  if (!system || !system->initialized)
  {
    return;
  }

  // Generate final entropy report
  cns_entropy_generate_report(system);

  // Cleanup resources
  if (system->sources)
  {
    free(system->sources);
    system->sources = NULL;
  }

  system->initialized = false;
  printf("✅ Entropy management system shutdown\n");
}

// Register entropy source
CNSResult cns_entropy_register_source(cns_entropy_system_t *system,
                                      const char *name,
                                      cns_entropy_type_t type,
                                      double initial_score)
{
  if (!system || !system->initialized || !name)
  {
    return CNS_ERR_INVALID_ARG;
  }

  if (system->source_count >= CNS_MAX_ENTROPY_SOURCES)
  {
    return CNS_ERR_RESOURCE;
  }

  cns_entropy_source_t *source = &system->sources[system->source_count];
  strncpy(source->name, name, sizeof(source->name) - 1);
  source->type = type;
  source->score = initial_score;
  source->last_updated = time(NULL);
  source->operation_count = 0;

  system->source_count++;
  system->entropy_score += initial_score;

  printf("📊 Registered entropy source: %s (score: %.2f)\n", name, initial_score);
  return CNS_OK;
}

// Update entropy score for a source
CNSResult cns_entropy_update_score(cns_entropy_system_t *system,
                                   const char *name,
                                   double delta_score)
{
  if (!system || !system->initialized || !name)
  {
    return CNS_ERR_INVALID_ARG;
  }

  for (int i = 0; i < system->source_count; i++)
  {
    if (strcmp(system->sources[i].name, name) == 0)
    {
      cns_entropy_source_t *source = &system->sources[i];
      double old_score = source->score;
      source->score += delta_score;
      source->last_updated = time(NULL);
      source->operation_count++;

      system->entropy_score += delta_score;
      system->total_operations++;

      // Telemetry: Record entropy change
      if (delta_score > 0)
      {
        printf("⚠️  Entropy increased: %s (%.2f → %.2f)\n",
               name, old_score, source->score);
      }
      else if (delta_score < 0)
      {
        printf("✅ Entropy decreased: %s (%.2f → %.2f)\n",
               name, old_score, source->score);
      }

      return CNS_OK;
    }
  }

  return CNS_ERR_NOT_FOUND;
}

// Get current entropy score
double cns_entropy_get_score(const cns_entropy_system_t *system)
{
  if (!system || !system->initialized)
  {
    return 0.0;
  }
  return system->entropy_score;
}

// Check if entropy is within acceptable limits
bool cns_entropy_is_acceptable(const cns_entropy_system_t *system)
{
  if (!system || !system->initialized)
  {
    return true;
  }
  return system->entropy_score < CNS_ENTROPY_THRESHOLD;
}

// Generate entropy report
void cns_entropy_generate_report(const cns_entropy_system_t *system)
{
  if (!system || !system->initialized)
  {
    return;
  }

  printf("\n📊 ENTROPY MANAGEMENT REPORT\n");
  printf("================================\n");
  printf("Total Entropy Score: %.2f\n", system->entropy_score);
  printf("Acceptable Threshold: %.2f\n", CNS_ENTROPY_THRESHOLD);
  printf("Status: %s\n", cns_entropy_is_acceptable(system) ? "✅ ACCEPTABLE" : "⚠️  HIGH");
  printf("Total Operations: %lu\n", system->total_operations);
  printf("Runtime: %ld seconds\n", time(NULL) - system->start_time);

  printf("\nEntropy Sources:\n");
  printf("----------------\n");
  for (int i = 0; i < system->source_count; i++)
  {
    const cns_entropy_source_t *source = &system->sources[i];
    printf("  %s: %.2f (ops: %lu)\n",
           source->name, source->score, source->operation_count);
  }

  printf("\nRecommendations:\n");
  printf("----------------\n");
  if (system->entropy_score > CNS_ENTROPY_THRESHOLD)
  {
    printf("⚠️  High entropy detected! Consider:\n");
    printf("   - Refactoring complex code\n");
    printf("   - Reducing technical debt\n");
    printf("   - Improving code organization\n");
    printf("   - Adding more tests\n");
  }
  else
  {
    printf("✅ Entropy is well-managed\n");
    printf("   - Continue current practices\n");
    printf("   - Monitor for new entropy sources\n");
  }
  printf("================================\n\n");
}

// Monitor code complexity
CNSResult cns_entropy_monitor_complexity(cns_entropy_system_t *system,
                                         const char *function_name,
                                         int cyclomatic_complexity)
{
  if (!system || !function_name)
  {
    return CNS_ERR_INVALID_ARG;
  }

  double entropy_delta = 0.0;

  if (cyclomatic_complexity > CNS_COMPLEXITY_THRESHOLD)
  {
    entropy_delta = (cyclomatic_complexity - CNS_COMPLEXITY_THRESHOLD) * 0.1;
    printf("⚠️  High complexity in %s: %d (threshold: %d)\n",
           function_name, cyclomatic_complexity, CNS_COMPLEXITY_THRESHOLD);
  }

  return cns_entropy_update_score(system, "code_complexity", entropy_delta);
}

// Monitor performance violations
CNSResult cns_entropy_monitor_performance(cns_entropy_system_t *system,
                                          const char *operation_name,
                                          uint64_t actual_cycles,
                                          uint64_t threshold_cycles)
{
  if (!system || !operation_name)
  {
    return CNS_ERR_INVALID_ARG;
  }

  double entropy_delta = 0.0;

  if (actual_cycles > threshold_cycles)
  {
    double violation_ratio = (double)actual_cycles / threshold_cycles;
    entropy_delta = (violation_ratio - 1.0) * 0.5;
    printf("⚠️  Performance violation in %s: %lu cycles (threshold: %lu)\n",
           operation_name, actual_cycles, threshold_cycles);
  }

  return cns_entropy_update_score(system, "performance_violations", entropy_delta);
}

// Monitor technical debt
CNSResult cns_entropy_monitor_technical_debt(cns_entropy_system_t *system,
                                             const char *debt_type,
                                             int debt_severity)
{
  if (!system || !debt_type)
  {
    return CNS_ERR_INVALID_ARG;
  }

  double entropy_delta = debt_severity * 0.2;

  if (debt_severity > 0)
  {
    printf("⚠️  Technical debt detected: %s (severity: %d)\n", debt_type, debt_severity);
  }

  return cns_entropy_update_score(system, "technical_debt", entropy_delta);
}

// Reduce entropy through refactoring
CNSResult cns_entropy_refactor(cns_entropy_system_t *system,
                               const char *component_name,
                               double improvement_score)
{
  if (!system || !component_name)
  {
    return CNS_ERR_INVALID_ARG;
  }

  double entropy_delta = -improvement_score; // Negative for improvement

  printf("✅ Refactoring %s: entropy reduced by %.2f\n", component_name, improvement_score);

  return cns_entropy_update_score(system, "refactoring_improvements", entropy_delta);
}

// Get entropy system instance
cns_entropy_system_t *cns_entropy_get_system(void)
{
  if (!g_entropy_system.initialized)
  {
    cns_entropy_init(&g_entropy_system);
  }
  return &g_entropy_system;
}