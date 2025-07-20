/*  ─────────────────────────────────────────────────────────────
    cns/include/cns/pragmatic/entropy.h  –  Entropy Management (v2.0)
    Software entropy management and technical debt tracking
    ───────────────────────────────────────────────────────────── */
#ifndef CNS_PRAGMATIC_ENTROPY_H
#define CNS_PRAGMATIC_ENTROPY_H

#include "cns/types.h"
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

/*═══════════════════════════════════════════════════════════════
  Configuration
  ═══════════════════════════════════════════════════════════════*/

#define CNS_MAX_ENTROPY_SOURCES 32
#define CNS_ENTROPY_THRESHOLD 10.0
#define CNS_COMPLEXITY_THRESHOLD 10

/*═══════════════════════════════════════════════════════════════
  Entropy Types
  ═══════════════════════════════════════════════════════════════*/

typedef enum
{
  CNS_ENTROPY_TYPE_COMPLEXITY,
  CNS_ENTROPY_TYPE_PERFORMANCE,
  CNS_ENTROPY_TYPE_TECHNICAL_DEBT,
  CNS_ENTROPY_TYPE_MAINTENANCE,
  CNS_ENTROPY_TYPE_QUALITY
} cns_entropy_type_t;

/*═══════════════════════════════════════════════════════════════
  Entropy Source
  ═══════════════════════════════════════════════════════════════*/

typedef struct
{
  char name[64];
  cns_entropy_type_t type;
  double score;
  time_t last_updated;
  uint64_t operation_count;
} cns_entropy_source_t;

/*═══════════════════════════════════════════════════════════════
  Entropy System
  ═══════════════════════════════════════════════════════════════*/

typedef struct
{
  bool initialized;
  time_t start_time;
  uint64_t total_operations;
  double entropy_score;
  cns_entropy_source_t *sources;
  int source_count;
} cns_entropy_system_t;

/*═══════════════════════════════════════════════════════════════
  Core Functions
  ═══════════════════════════════════════════════════════════════*/

// Initialize entropy management system
CNSResult cns_entropy_init(cns_entropy_system_t *system);

// Shutdown entropy management system
void cns_entropy_shutdown(cns_entropy_system_t *system);

// Register entropy source
CNSResult cns_entropy_register_source(cns_entropy_system_t *system,
                                      const char *name,
                                      cns_entropy_type_t type,
                                      double initial_score);

// Update entropy score for a source
CNSResult cns_entropy_update_score(cns_entropy_system_t *system,
                                   const char *name,
                                   double delta_score);

// Get current entropy score
double cns_entropy_get_score(const cns_entropy_system_t *system);

// Check if entropy is within acceptable limits
bool cns_entropy_is_acceptable(const cns_entropy_system_t *system);

// Generate entropy report
void cns_entropy_generate_report(const cns_entropy_system_t *system);

/*═══════════════════════════════════════════════════════════════
  Monitoring Functions
  ═══════════════════════════════════════════════════════════════*/

// Monitor code complexity
CNSResult cns_entropy_monitor_complexity(cns_entropy_system_t *system,
                                         const char *function_name,
                                         int cyclomatic_complexity);

// Monitor performance violations
CNSResult cns_entropy_monitor_performance(cns_entropy_system_t *system,
                                          const char *operation_name,
                                          uint64_t actual_cycles,
                                          uint64_t threshold_cycles);

// Monitor technical debt
CNSResult cns_entropy_monitor_technical_debt(cns_entropy_system_t *system,
                                             const char *debt_type,
                                             int debt_severity);

// Reduce entropy through refactoring
CNSResult cns_entropy_refactor(cns_entropy_system_t *system,
                               const char *component_name,
                               double improvement_score);

/*═══════════════════════════════════════════════════════════════
  Utility Functions
  ═══════════════════════════════════════════════════════════════*/

// Get entropy system instance
cns_entropy_system_t *cns_entropy_get_system(void);

/*═══════════════════════════════════════════════════════════════
  Convenience Macros
  ═══════════════════════════════════════════════════════════════*/

// Monitor entropy automatically
#define CNS_ENTROPY_MONITOR(system, name, value, threshold) \
  cns_entropy_monitor_performance(system, name, value, threshold)

// Track complexity automatically
#define CNS_ENTROPY_COMPLEXITY(system, func, complexity) \
  cns_entropy_monitor_complexity(system, func, complexity)

// Track technical debt automatically
#define CNS_ENTROPY_DEBT(system, type, severity) \
  cns_entropy_monitor_technical_debt(system, type, severity)

// Record refactoring improvement
#define CNS_ENTROPY_REFACTOR(system, component, improvement) \
  cns_entropy_refactor(system, component, improvement)

#endif /* CNS_PRAGMATIC_ENTROPY_H */