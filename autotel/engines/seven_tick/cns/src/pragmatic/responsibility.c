/*  ─────────────────────────────────────────────────────────────
    cns/src/pragmatic/responsibility.c  –  Responsibility Management (v2.0)
    Taking responsibility for code quality and system behavior
    ───────────────────────────────────────────────────────────── */

#include "cns/pragmatic/responsibility.h"
#include "cns/telemetry/otel.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/*═══════════════════════════════════════════════════════════════
  Responsibility Management System
  ═══════════════════════════════════════════════════════════════*/

static cns_responsibility_system_t g_responsibility_system = {0};

// Initialize responsibility management system
CNSResult cns_responsibility_init(cns_responsibility_system_t *system)
{
  if (!system)
  {
    return CNS_ERR_INVALID_ARG;
  }

  memset(system, 0, sizeof(cns_responsibility_system_t));
  system->initialized = true;
  system->start_time = time(NULL);
  system->total_decisions = 0;
  system->total_issues_resolved = 0;

  // Initialize responsibility areas
  system->areas = malloc(sizeof(cns_responsibility_area_t) * CNS_MAX_RESPONSIBILITY_AREAS);
  if (!system->areas)
  {
    return CNS_ERR_RESOURCE;
  }

  memset(system->areas, 0, sizeof(cns_responsibility_area_t) * CNS_MAX_RESPONSIBILITY_AREAS);

  printf("✅ Responsibility management system initialized\n");
  return CNS_OK;
}

// Shutdown responsibility management system
void cns_responsibility_shutdown(cns_responsibility_system_t *system)
{
  if (!system || !system->initialized)
  {
    return;
  }

  // Generate final responsibility report
  cns_responsibility_generate_report(system);

  // Cleanup resources
  if (system->areas)
  {
    free(system->areas);
    system->areas = NULL;
  }

  system->initialized = false;
  printf("✅ Responsibility management system shutdown\n");
}

// Register responsibility area
CNSResult cns_responsibility_register_area(cns_responsibility_system_t *system,
                                           const char *name,
                                           cns_responsibility_type_t type,
                                           double initial_ownership)
{
  if (!system || !system->initialized || !name)
  {
    return CNS_ERR_INVALID_ARG;
  }

  if (system->area_count >= CNS_MAX_RESPONSIBILITY_AREAS)
  {
    return CNS_ERR_RESOURCE;
  }

  cns_responsibility_area_t *area = &system->areas[system->area_count];
  strncpy(area->name, name, sizeof(area->name) - 1);
  area->type = type;
  area->ownership = initial_ownership;
  area->last_updated = time(NULL);
  area->decisions_made = 0;
  area->issues_resolved = 0;

  system->area_count++;

  printf("🎯 Registered responsibility area: %s (ownership: %.1f%%)\n", name, initial_ownership);
  return CNS_OK;
}

// Take responsibility for a decision
CNSResult cns_responsibility_take_decision(cns_responsibility_system_t *system,
                                           const char *area_name,
                                           const char *decision,
                                           cns_decision_impact_t impact)
{
  if (!system || !system->initialized || !area_name || !decision)
  {
    return CNS_ERR_INVALID_ARG;
  }

  for (int i = 0; i < system->area_count; i++)
  {
    if (strcmp(system->areas[i].name, area_name) == 0)
    {
      cns_responsibility_area_t *area = &system->areas[i];
      area->decisions_made++;
      area->last_updated = time(NULL);

      system->total_decisions++;

      printf("🎯 Decision taken in %s: %s (impact: %s)\n",
             area_name, decision,
             impact == CNS_DECISION_IMPACT_LOW ? "LOW" : impact == CNS_DECISION_IMPACT_MEDIUM ? "MEDIUM"
                                                                                              : "HIGH");

      return CNS_OK;
    }
  }

  return CNS_ERR_NOT_FOUND;
}

// Resolve an issue
CNSResult cns_responsibility_resolve_issue(cns_responsibility_system_t *system,
                                           const char *area_name,
                                           const char *issue,
                                           cns_issue_severity_t severity)
{
  if (!system || !system->initialized || !area_name || !issue)
  {
    return CNS_ERR_INVALID_ARG;
  }

  for (int i = 0; i < system->area_count; i++)
  {
    if (strcmp(system->areas[i].name, area_name) == 0)
    {
      cns_responsibility_area_t *area = &system->areas[i];
      area->issues_resolved++;
      area->last_updated = time(NULL);

      system->total_issues_resolved++;

      printf("🔧 Issue resolved in %s: %s (severity: %s)\n",
             area_name, issue,
             severity == CNS_ISSUE_SEVERITY_LOW ? "LOW" : severity == CNS_ISSUE_SEVERITY_MEDIUM ? "MEDIUM"
                                                                                                : "HIGH");

      return CNS_OK;
    }
  }

  return CNS_ERR_NOT_FOUND;
}

// Update ownership level
CNSResult cns_responsibility_update_ownership(cns_responsibility_system_t *system,
                                              const char *area_name,
                                              double ownership_delta)
{
  if (!system || !system->initialized || !area_name)
  {
    return CNS_ERR_INVALID_ARG;
  }

  for (int i = 0; i < system->area_count; i++)
  {
    if (strcmp(system->areas[i].name, area_name) == 0)
    {
      cns_responsibility_area_t *area = &system->areas[i];
      double old_ownership = area->ownership;
      area->ownership += ownership_delta;
      area->last_updated = time(NULL);

      // Clamp ownership to 0-100 range
      if (area->ownership < 0.0)
        area->ownership = 0.0;
      if (area->ownership > 100.0)
        area->ownership = 100.0;

      printf("🎯 Ownership updated in %s: %.1f%% → %.1f%%\n",
             area_name, old_ownership, area->ownership);

      return CNS_OK;
    }
  }

  return CNS_ERR_NOT_FOUND;
}

// Get responsibility score
double cns_responsibility_get_score(const cns_responsibility_system_t *system)
{
  if (!system || !system->initialized)
  {
    return 0.0;
  }

  double total_score = 0.0;
  for (int i = 0; i < system->area_count; i++)
  {
    total_score += system->areas[i].ownership;
  }

  return system->area_count > 0 ? total_score / system->area_count : 0.0;
}

// Check if responsibility is adequate
bool cns_responsibility_is_adequate(const cns_responsibility_system_t *system)
{
  if (!system || !system->initialized)
  {
    return false;
  }
  return cns_responsibility_get_score(system) >= CNS_RESPONSIBILITY_THRESHOLD;
}

// Generate responsibility report
void cns_responsibility_generate_report(const cns_responsibility_system_t *system)
{
  if (!system || !system->initialized)
  {
    return;
  }

  printf("\n🎯 RESPONSIBILITY MANAGEMENT REPORT\n");
  printf("===================================\n");
  printf("Average Ownership: %.1f%%\n", cns_responsibility_get_score(system));
  printf("Responsibility Threshold: %.1f%%\n", CNS_RESPONSIBILITY_THRESHOLD);
  printf("Status: %s\n", cns_responsibility_is_adequate(system) ? "✅ ADEQUATE" : "⚠️  NEEDS IMPROVEMENT");
  printf("Total Decisions: %lu\n", system->total_decisions);
  printf("Total Issues Resolved: %lu\n", system->total_issues_resolved);
  printf("Runtime: %ld seconds\n", time(NULL) - system->start_time);

  printf("\nResponsibility Areas:\n");
  printf("--------------------\n");
  for (int i = 0; i < system->area_count; i++)
  {
    const cns_responsibility_area_t *area = &system->areas[i];
    printf("  %s: %.1f%% (decisions: %lu, issues: %lu)\n",
           area->name, area->ownership, area->decisions_made, area->issues_resolved);
  }

  printf("\nRecommendations:\n");
  printf("----------------\n");
  if (cns_responsibility_get_score(system) < CNS_RESPONSIBILITY_THRESHOLD)
  {
    printf("⚠️  More responsibility needed! Consider:\n");
    printf("   - Taking ownership of code quality\n");
    printf("   - Making decisions proactively\n");
    printf("   - Resolving issues promptly\n");
    printf("   - Mentoring team members\n");
    printf("   - Contributing to system architecture\n");
  }
  else
  {
    printf("✅ Responsibility is well-managed\n");
    printf("   - Continue taking ownership\n");
    printf("   - Share knowledge with others\n");
    printf("   - Lead by example\n");
  }
  printf("===================================\n\n");
}

// Accept responsibility for a problem
CNSResult cns_responsibility_accept_problem(cns_responsibility_system_t *system,
                                            const char *area_name,
                                            const char *problem,
                                            const char *solution)
{
  if (!system || !area_name || !problem)
  {
    return CNS_ERR_INVALID_ARG;
  }

  printf("🎯 Accepting responsibility for: %s\n", problem);
  printf("   Area: %s\n", area_name);
  if (solution)
  {
    printf("   Solution: %s\n", solution);
  }

  // Update ownership positively for accepting responsibility
  cns_responsibility_update_ownership(system, area_name, 5.0);

  return CNS_OK;
}

// Delegate responsibility appropriately
CNSResult cns_responsibility_delegate(cns_responsibility_system_t *system,
                                      const char *from_area,
                                      const char *to_area,
                                      const char *task,
                                      double ownership_transfer)
{
  if (!system || !from_area || !to_area || !task)
  {
    return CNS_ERR_INVALID_ARG;
  }

  printf("🎯 Delegating responsibility: %s\n", task);
  printf("   From: %s\n", from_area);
  printf("   To: %s\n", to_area);
  printf("   Ownership transfer: %.1f%%\n", ownership_transfer);

  // Transfer ownership
  cns_responsibility_update_ownership(system, from_area, -ownership_transfer);
  cns_responsibility_update_ownership(system, to_area, ownership_transfer);

  return CNS_OK;
}

// Get responsibility system instance
cns_responsibility_system_t *cns_responsibility_get_system(void)
{
  if (!g_responsibility_system.initialized)
  {
    cns_responsibility_init(&g_responsibility_system);
  }
  return &g_responsibility_system;
}