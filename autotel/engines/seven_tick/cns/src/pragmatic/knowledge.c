/*  ─────────────────────────────────────────────────────────────
    cns/src/pragmatic/knowledge.c  –  Knowledge Management (v2.0)
    Continuous knowledge investment and learning tracking
    ───────────────────────────────────────────────────────────── */

#include "cns/pragmatic/knowledge.h"
#include "cns/telemetry/otel.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/*═══════════════════════════════════════════════════════════════
  Knowledge Management System
  ═══════════════════════════════════════════════════════════════*/

static cns_knowledge_system_t g_knowledge_system = {0};

// Initialize knowledge management system
CNSResult cns_knowledge_init(cns_knowledge_system_t *system)
{
  if (!system)
  {
    return CNS_ERR_INVALID_ARG;
  }

  memset(system, 0, sizeof(cns_knowledge_system_t));
  system->initialized = true;
  system->start_time = time(NULL);
  system->total_learning_sessions = 0;
  system->total_knowledge_points = 0;

  // Initialize knowledge areas
  system->areas = malloc(sizeof(cns_knowledge_area_t) * CNS_MAX_KNOWLEDGE_AREAS);
  if (!system->areas)
  {
    return CNS_ERR_RESOURCE;
  }

  memset(system->areas, 0, sizeof(cns_knowledge_area_t) * CNS_MAX_KNOWLEDGE_AREAS);

  printf("✅ Knowledge management system initialized\n");
  return CNS_OK;
}

// Shutdown knowledge management system
void cns_knowledge_shutdown(cns_knowledge_system_t *system)
{
  if (!system || !system->initialized)
  {
    return;
  }

  // Generate final knowledge report
  cns_knowledge_generate_report(system);

  // Cleanup resources
  if (system->areas)
  {
    free(system->areas);
    system->areas = NULL;
  }

  system->initialized = false;
  printf("✅ Knowledge management system shutdown\n");
}

// Register knowledge area
CNSResult cns_knowledge_register_area(cns_knowledge_system_t *system,
                                      const char *name,
                                      cns_knowledge_category_t category,
                                      double initial_proficiency)
{
  if (!system || !system->initialized || !name)
  {
    return CNS_ERR_INVALID_ARG;
  }

  if (system->area_count >= CNS_MAX_KNOWLEDGE_AREAS)
  {
    return CNS_ERR_RESOURCE;
  }

  cns_knowledge_area_t *area = &system->areas[system->area_count];
  strncpy(area->name, name, sizeof(area->name) - 1);
  area->category = category;
  area->proficiency = initial_proficiency;
  area->last_updated = time(NULL);
  area->learning_sessions = 0;
  area->total_hours = 0.0;

  system->area_count++;
  system->total_knowledge_points += initial_proficiency;

  printf("📚 Registered knowledge area: %s (proficiency: %.1f%%)\n", name, initial_proficiency);
  return CNS_OK;
}

// Update knowledge proficiency
CNSResult cns_knowledge_update_proficiency(cns_knowledge_system_t *system,
                                           const char *area_name,
                                           double proficiency_delta,
                                           double learning_hours)
{
  if (!system || !system->initialized || !area_name)
  {
    return CNS_ERR_INVALID_ARG;
  }

  for (int i = 0; i < system->area_count; i++)
  {
    if (strcmp(system->areas[i].name, area_name) == 0)
    {
      cns_knowledge_area_t *area = &system->areas[i];
      double old_proficiency = area->proficiency;
      area->proficiency += proficiency_delta;
      area->last_updated = time(NULL);
      area->learning_sessions++;
      area->total_hours += learning_hours;

      // Clamp proficiency to 0-100 range
      if (area->proficiency < 0.0)
        area->proficiency = 0.0;
      if (area->proficiency > 100.0)
        area->proficiency = 100.0;

      system->total_knowledge_points += proficiency_delta;
      system->total_learning_sessions++;

      // Telemetry: Record knowledge improvement
      if (proficiency_delta > 0)
      {
        printf("📈 Knowledge improved: %s (%.1f%% → %.1f%%, +%.1f hours)\n",
               area_name, old_proficiency, area->proficiency, learning_hours);
      }
      else if (proficiency_delta < 0)
      {
        printf("📉 Knowledge declined: %s (%.1f%% → %.1f%%)\n",
               area_name, old_proficiency, area->proficiency);
      }

      return CNS_OK;
    }
  }

  return CNS_ERR_NOT_FOUND;
}

// Get current knowledge score
double cns_knowledge_get_score(const cns_knowledge_system_t *system)
{
  if (!system || !system->initialized)
  {
    return 0.0;
  }
  return system->total_knowledge_points;
}

// Get average proficiency
double cns_knowledge_get_average_proficiency(const cns_knowledge_system_t *system)
{
  if (!system || !system->initialized || system->area_count == 0)
  {
    return 0.0;
  }
  return system->total_knowledge_points / system->area_count;
}

// Check if knowledge investment is adequate
bool cns_knowledge_is_adequate(const cns_knowledge_system_t *system)
{
  if (!system || !system->initialized)
  {
    return false;
  }
  return system->total_knowledge_points >= CNS_KNOWLEDGE_THRESHOLD;
}

// Generate knowledge report
void cns_knowledge_generate_report(const cns_knowledge_system_t *system)
{
  if (!system || !system->initialized)
  {
    return;
  }

  printf("\n📚 KNOWLEDGE MANAGEMENT REPORT\n");
  printf("================================\n");
  printf("Total Knowledge Points: %.1f\n", system->total_knowledge_points);
  printf("Average Proficiency: %.1f%%\n", cns_knowledge_get_average_proficiency(system));
  printf("Adequacy Threshold: %.1f\n", CNS_KNOWLEDGE_THRESHOLD);
  printf("Status: %s\n", cns_knowledge_is_adequate(system) ? "✅ ADEQUATE" : "⚠️  NEEDS IMPROVEMENT");
  printf("Learning Sessions: %lu\n", system->total_learning_sessions);
  printf("Runtime: %ld seconds\n", time(NULL) - system->start_time);

  printf("\nKnowledge Areas:\n");
  printf("----------------\n");
  for (int i = 0; i < system->area_count; i++)
  {
    const cns_knowledge_area_t *area = &system->areas[i];
    printf("  %s: %.1f%% (sessions: %lu, hours: %.1f)\n",
           area->name, area->proficiency, area->learning_sessions, area->total_hours);
  }

  printf("\nRecommendations:\n");
  printf("----------------\n");
  if (system->total_knowledge_points < CNS_KNOWLEDGE_THRESHOLD)
  {
    printf("⚠️  Knowledge investment needed! Consider:\n");
    printf("   - Reading technical books\n");
    printf("   - Taking online courses\n");
    printf("   - Attending conferences\n");
    printf("   - Practicing new technologies\n");
    printf("   - Contributing to open source\n");
  }
  else
  {
    printf("✅ Knowledge investment is adequate\n");
    printf("   - Continue learning new areas\n");
    printf("   - Share knowledge with others\n");
    printf("   - Mentor junior developers\n");
  }
  printf("================================\n\n");
}

// Record learning session
CNSResult cns_knowledge_record_session(cns_knowledge_system_t *system,
                                       const char *area_name,
                                       cns_learning_type_t type,
                                       double hours,
                                       const char *description)
{
  if (!system || !area_name)
  {
    return CNS_ERR_INVALID_ARG;
  }

  // Calculate proficiency gain based on learning type and hours
  double proficiency_gain = 0.0;
  switch (type)
  {
  case CNS_LEARNING_TYPE_READING:
    proficiency_gain = hours * 2.0; // Reading is efficient
    break;
  case CNS_LEARNING_TYPE_PRACTICE:
    proficiency_gain = hours * 3.0; // Practice is very effective
    break;
  case CNS_LEARNING_TYPE_COURSE:
    proficiency_gain = hours * 2.5; // Courses provide structured learning
    break;
  case CNS_LEARNING_TYPE_CONFERENCE:
    proficiency_gain = hours * 1.5; // Conferences provide exposure
    break;
  case CNS_LEARNING_TYPE_MENTORING:
    proficiency_gain = hours * 2.0; // Mentoring is effective
    break;
  default:
    proficiency_gain = hours * 1.0; // Default rate
    break;
  }

  printf("📖 Learning session: %s (%s, %.1f hours)\n", area_name, description, hours);

  return cns_knowledge_update_proficiency(system, area_name, proficiency_gain, hours);
}

// Identify knowledge gaps
void cns_knowledge_identify_gaps(const cns_knowledge_system_t *system)
{
  if (!system || !system->initialized)
  {
    return;
  }

  printf("\n🔍 KNOWLEDGE GAP ANALYSIS\n");
  printf("=========================\n");

  for (int i = 0; i < system->area_count; i++)
  {
    const cns_knowledge_area_t *area = &system->areas[i];

    if (area->proficiency < CNS_PROFICIENCY_THRESHOLD)
    {
      printf("⚠️  %s: %.1f%% (target: %.1f%%)\n",
             area->name, area->proficiency, CNS_PROFICIENCY_THRESHOLD);

      // Suggest learning activities
      printf("    Suggested activities:\n");
      if (area->category == CNS_KNOWLEDGE_CATEGORY_TECHNICAL)
      {
        printf("    - Read technical documentation\n");
        printf("    - Practice with sample projects\n");
        printf("    - Take online courses\n");
      }
      else if (area->category == CNS_KNOWLEDGE_CATEGORY_SOFT_SKILLS)
      {
        printf("    - Attend communication workshops\n");
        printf("    - Practice public speaking\n");
        printf("    - Read leadership books\n");
      }
      else if (area->category == CNS_KNOWLEDGE_CATEGORY_DOMAIN)
      {
        printf("    - Study domain-specific literature\n");
        printf("    - Work with domain experts\n");
        printf("    - Attend industry conferences\n");
      }
    }
  }
  printf("=========================\n\n");
}

// Plan learning roadmap
void cns_knowledge_plan_roadmap(const cns_knowledge_system_t *system)
{
  if (!system || !system->initialized)
  {
    return;
  }

  printf("\n🗺️  LEARNING ROADMAP\n");
  printf("===================\n");

  // Sort areas by priority (lowest proficiency first)
  cns_knowledge_area_t *sorted_areas = malloc(sizeof(cns_knowledge_area_t) * system->area_count);
  if (!sorted_areas)
  {
    return;
  }

  memcpy(sorted_areas, system->areas, sizeof(cns_knowledge_area_t) * system->area_count);

  // Simple bubble sort by proficiency (ascending)
  for (int i = 0; i < system->area_count - 1; i++)
  {
    for (int j = 0; j < system->area_count - i - 1; j++)
    {
      if (sorted_areas[j].proficiency > sorted_areas[j + 1].proficiency)
      {
        cns_knowledge_area_t temp = sorted_areas[j];
        sorted_areas[j] = sorted_areas[j + 1];
        sorted_areas[j + 1] = temp;
      }
    }
  }

  printf("Priority order for learning:\n");
  for (int i = 0; i < system->area_count; i++)
  {
    const cns_knowledge_area_t *area = &sorted_areas[i];
    double gap = CNS_PROFICIENCY_THRESHOLD - area->proficiency;
    if (gap > 0)
    {
      printf("  %d. %s (gap: %.1f%%)\n", i + 1, area->name, gap);
    }
  }

  free(sorted_areas);
  printf("===================\n\n");
}

// Get knowledge system instance
cns_knowledge_system_t *cns_knowledge_get_system(void)
{
  if (!g_knowledge_system.initialized)
  {
    cns_knowledge_init(&g_knowledge_system);
  }
  return &g_knowledge_system;
}