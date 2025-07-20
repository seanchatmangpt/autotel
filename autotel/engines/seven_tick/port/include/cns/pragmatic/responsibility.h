/*  ─────────────────────────────────────────────────────────────
    cns/include/cns/pragmatic/responsibility.h  –  Responsibility Management (v2.0)
    Taking responsibility for code quality and system behavior
    ───────────────────────────────────────────────────────────── */
#ifndef CNS_PRAGMATIC_RESPONSIBILITY_H
#define CNS_PRAGMATIC_RESPONSIBILITY_H

#include "cns/types.h"
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

/*═══════════════════════════════════════════════════════════════
  Configuration
  ═══════════════════════════════════════════════════════════════*/

#define CNS_MAX_RESPONSIBILITY_AREAS 32
#define CNS_RESPONSIBILITY_THRESHOLD 70.0

/*═══════════════════════════════════════════════════════════════
  Responsibility Types
  ═══════════════════════════════════════════════════════════════*/

typedef enum
{
  CNS_RESPONSIBILITY_TYPE_CODE_QUALITY,
  CNS_RESPONSIBILITY_TYPE_SYSTEM_ARCHITECTURE,
  CNS_RESPONSIBILITY_TYPE_PERFORMANCE,
  CNS_RESPONSIBILITY_TYPE_SECURITY,
  CNS_RESPONSIBILITY_TYPE_TEAM_LEADERSHIP,
  CNS_RESPONSIBILITY_TYPE_PROJECT_MANAGEMENT
} cns_responsibility_type_t;

/*═══════════════════════════════════════════════════════════════
  Decision Impact Levels
  ═══════════════════════════════════════════════════════════════*/

typedef enum
{
  CNS_DECISION_IMPACT_LOW,
  CNS_DECISION_IMPACT_MEDIUM,
  CNS_DECISION_IMPACT_HIGH
} cns_decision_impact_t;

/*═══════════════════════════════════════════════════════════════
  Issue Severity Levels
  ═══════════════════════════════════════════════════════════════*/

typedef enum
{
  CNS_ISSUE_SEVERITY_LOW,
  CNS_ISSUE_SEVERITY_MEDIUM,
  CNS_ISSUE_SEVERITY_HIGH
} cns_issue_severity_t;

/*═══════════════════════════════════════════════════════════════
  Responsibility Area
  ═══════════════════════════════════════════════════════════════*/

typedef struct
{
  char name[64];
  cns_responsibility_type_t type;
  double ownership; // 0-100%
  time_t last_updated;
  uint64_t decisions_made;
  uint64_t issues_resolved;
} cns_responsibility_area_t;

/*═══════════════════════════════════════════════════════════════
  Responsibility System
  ═══════════════════════════════════════════════════════════════*/

typedef struct
{
  bool initialized;
  time_t start_time;
  uint64_t total_decisions;
  uint64_t total_issues_resolved;
  cns_responsibility_area_t *areas;
  int area_count;
} cns_responsibility_system_t;

/*═══════════════════════════════════════════════════════════════
  Core Functions
  ═══════════════════════════════════════════════════════════════*/

// Initialize responsibility management system
CNSResult cns_responsibility_init(cns_responsibility_system_t *system);

// Shutdown responsibility management system
void cns_responsibility_shutdown(cns_responsibility_system_t *system);

// Register responsibility area
CNSResult cns_responsibility_register_area(cns_responsibility_system_t *system,
                                           const char *name,
                                           cns_responsibility_type_t type,
                                           double initial_ownership);

// Take responsibility for a decision
CNSResult cns_responsibility_take_decision(cns_responsibility_system_t *system,
                                           const char *area_name,
                                           const char *decision,
                                           cns_decision_impact_t impact);

// Resolve an issue
CNSResult cns_responsibility_resolve_issue(cns_responsibility_system_t *system,
                                           const char *area_name,
                                           const char *issue,
                                           cns_issue_severity_t severity);

// Update ownership level
CNSResult cns_responsibility_update_ownership(cns_responsibility_system_t *system,
                                              const char *area_name,
                                              double ownership_delta);

// Get responsibility score
double cns_responsibility_get_score(const cns_responsibility_system_t *system);

// Check if responsibility is adequate
bool cns_responsibility_is_adequate(const cns_responsibility_system_t *system);

// Generate responsibility report
void cns_responsibility_generate_report(const cns_responsibility_system_t *system);

/*═══════════════════════════════════════════════════════════════
  Advanced Functions
  ═══════════════════════════════════════════════════════════════*/

// Accept responsibility for a problem
CNSResult cns_responsibility_accept_problem(cns_responsibility_system_t *system,
                                            const char *area_name,
                                            const char *problem,
                                            const char *solution);

// Delegate responsibility appropriately
CNSResult cns_responsibility_delegate(cns_responsibility_system_t *system,
                                      const char *from_area,
                                      const char *to_area,
                                      const char *task,
                                      double ownership_transfer);

/*═══════════════════════════════════════════════════════════════
  Utility Functions
  ═══════════════════════════════════════════════════════════════*/

// Get responsibility system instance
cns_responsibility_system_t *cns_responsibility_get_system(void);

/*═══════════════════════════════════════════════════════════════
  Convenience Macros
  ═══════════════════════════════════════════════════════════════*/

// Take decision automatically
#define CNS_RESPONSIBILITY_DECIDE(system, area, decision, impact) \
  cns_responsibility_take_decision(system, area, decision, impact)

// Resolve issue automatically
#define CNS_RESPONSIBILITY_RESOLVE(system, area, issue, severity) \
  cns_responsibility_resolve_issue(system, area, issue, severity)

// Update ownership automatically
#define CNS_RESPONSIBILITY_OWN(system, area, delta) \
  cns_responsibility_update_ownership(system, area, delta)

// Accept problem automatically
#define CNS_RESPONSIBILITY_ACCEPT(system, area, problem, solution) \
  cns_responsibility_accept_problem(system, area, problem, solution)

#endif /* CNS_PRAGMATIC_RESPONSIBILITY_H */