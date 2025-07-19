/*  ─────────────────────────────────────────────────────────────
    cns/include/cns/pragmatic/knowledge.h  –  Knowledge Management (v2.0)
    Continuous knowledge investment and learning tracking
    ───────────────────────────────────────────────────────────── */
#ifndef CNS_PRAGMATIC_KNOWLEDGE_H
#define CNS_PRAGMATIC_KNOWLEDGE_H

#include "cns/types.h"
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

/*═══════════════════════════════════════════════════════════════
  Configuration
  ═══════════════════════════════════════════════════════════════*/

#define CNS_MAX_KNOWLEDGE_AREAS 32
#define CNS_KNOWLEDGE_THRESHOLD 500.0
#define CNS_PROFICIENCY_THRESHOLD 70.0

/*═══════════════════════════════════════════════════════════════
  Knowledge Categories
  ═══════════════════════════════════════════════════════════════*/

typedef enum {
    CNS_KNOWLEDGE_CATEGORY_TECHNICAL,
    CNS_KNOWLEDGE_CATEGORY_SOFT_SKILLS,
    CNS_KNOWLEDGE_CATEGORY_DOMAIN,
    CNS_KNOWLEDGE_CATEGORY_TOOLS,
    CNS_KNOWLEDGE_CATEGORY_METHODOLOGY
} cns_knowledge_category_t;

/*═══════════════════════════════════════════════════════════════
  Learning Types
  ═══════════════════════════════════════════════════════════════*/

typedef enum {
    CNS_LEARNING_TYPE_READING,
    CNS_LEARNING_TYPE_PRACTICE,
    CNS_LEARNING_TYPE_COURSE,
    CNS_LEARNING_TYPE_CONFERENCE,
    CNS_LEARNING_TYPE_MENTORING,
    CNS_LEARNING_TYPE_PROJECT
} cns_learning_type_t;

/*═══════════════════════════════════════════════════════════════
  Knowledge Area
  ═══════════════════════════════════════════════════════════════*/

typedef struct {
    char name[64];
    cns_knowledge_category_t category;
    double proficiency;  // 0-100%
    time_t last_updated;
    uint64_t learning_sessions;
    double total_hours;
} cns_knowledge_area_t;

/*═══════════════════════════════════════════════════════════════
  Knowledge System
  ═══════════════════════════════════════════════════════════════*/

typedef struct {
    bool initialized;
    time_t start_time;
    uint64_t total_learning_sessions;
    double total_knowledge_points;
    cns_knowledge_area_t* areas;
    int area_count;
} cns_knowledge_system_t;

/*═══════════════════════════════════════════════════════════════
  Core Functions
  ═══════════════════════════════════════════════════════════════*/

// Initialize knowledge management system
CNSResult cns_knowledge_init(cns_knowledge_system_t* system);

// Shutdown knowledge management system
void cns_knowledge_shutdown(cns_knowledge_system_t* system);

// Register knowledge area
CNSResult cns_knowledge_register_area(cns_knowledge_system_t* system,
                                    const char* name,
                                    cns_knowledge_category_t category,
                                    double initial_proficiency);

// Update knowledge proficiency
CNSResult cns_knowledge_update_proficiency(cns_knowledge_system_t* system,
                                         const char* area_name,
                                         double proficiency_delta,
                                         double learning_hours);

// Get current knowledge score
double cns_knowledge_get_score(const cns_knowledge_system_t* system);

// Get average proficiency
double cns_knowledge_get_average_proficiency(const cns_knowledge_system_t* system);

// Check if knowledge investment is adequate
bool cns_knowledge_is_adequate(const cns_knowledge_system_t* system);

// Generate knowledge report
void cns_knowledge_generate_report(const cns_knowledge_system_t* system);

/*═══════════════════════════════════════════════════════════════
  Learning Functions
  ═══════════════════════════════════════════════════════════════*/

// Record learning session
CNSResult cns_knowledge_record_session(cns_knowledge_system_t* system,
                                     const char* area_name,
                                     cns_learning_type_t type,
                                     double hours,
                                     const char* description);

// Identify knowledge gaps
void cns_knowledge_identify_gaps(const cns_knowledge_system_t* system);

// Plan learning roadmap
void cns_knowledge_plan_roadmap(const cns_knowledge_system_t* system);

/*═══════════════════════════════════════════════════════════════
  Utility Functions
  ═══════════════════════════════════════════════════════════════*/

// Get knowledge system instance
cns_knowledge_system_t* cns_knowledge_get_system(void);

/*═══════════════════════════════════════════════════════════════
  Convenience Macros
  ═══════════════════════════════════════════════════════════════*/

// Record learning session automatically
#define CNS_KNOWLEDGE_LEARN(system, area, type, hours, desc) \
    cns_knowledge_record_session(system, area, type, hours, desc)

// Update proficiency automatically
#define CNS_KNOWLEDGE_UPDATE(system, area, delta, hours) \
    cns_knowledge_update_proficiency(system, area, delta, hours)

// Check knowledge adequacy automatically
#define CNS_KNOWLEDGE_CHECK(system) \
    cns_knowledge_is_adequate(system)

#endif /* CNS_PRAGMATIC_KNOWLEDGE_H */ 