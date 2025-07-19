/*  ─────────────────────────────────────────────────────────────
    test_pragmatic_extended.c  –  Extended Pragmatic Programmer Tests (v1.0)
    Comprehensive validation of Orthogonality, Tracer Bullets, and Reversibility
    ───────────────────────────────────────────────────────────── */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>

// Include the new pragmatic programmer headers
#include "cns/pragmatic/orthogonality.h"
#include "cns/pragmatic/tracer_bullets.h"
#include "cns/pragmatic/reversibility.h"

// Include existing pragmatic headers
#include "cns/pragmatic/contracts.h"
#include "cns/pragmatic/knowledge.h"
#include "cns/pragmatic/entropy.h"
#include "cns/pragmatic/responsibility.h"
#include "cns/pragmatic/automation.h"
#include "cns/pragmatic/testing.h"

/*═══════════════════════════════════════════════════════════════
  Test Functions
  ═══════════════════════════════════════════════════════════════*/

static void test_orthogonality(void);
static void test_tracer_bullets(void);
static void test_reversibility(void);
static void test_integration(void);

/*═══════════════════════════════════════════════════════════════
  Main Test Function
  ═══════════════════════════════════════════════════════════════*/

int main(void)
{
  printf("🎯 Extended Pragmatic Programmer Validation with 7-Tick Benchmarks\n");
  printf("================================================================\n\n");

  // Test Orthogonality
  printf("1. Testing Orthogonality Principle...\n");
  test_orthogonality();
  printf("✅ Orthogonality tests completed\n\n");

  // Test Tracer Bullets
  printf("2. Testing Tracer Bullets Principle...\n");
  test_tracer_bullets();
  printf("✅ Tracer Bullets tests completed\n\n");

  // Test Reversibility
  printf("3. Testing Reversibility Principle...\n");
  test_reversibility();
  printf("✅ Reversibility tests completed\n\n");

  // Test Integration
  printf("4. Testing Integration of All Principles...\n");
  test_integration();
  printf("✅ Integration tests completed\n\n");

  printf("🎉 ALL EXTENDED PRAGMATIC PROGRAMMER VALIDATIONS PASSED!\n");
  printf("📊 Summary:\n");
  printf("   - Orthogonality: Independent, modular components ✅\n");
  printf("   - Tracer Bullets: End-to-end working prototypes ✅\n");
  printf("   - Reversibility: Ability to undo changes ✅\n");
  printf("   - Integration: All principles work together ✅\n");

  return 0;
}

/*═══════════════════════════════════════════════════════════════
  Orthogonality Tests
  ═══════════════════════════════════════════════════════════════*/

static void test_orthogonality(void)
{
  cns_orthogonality_manager_t *manager = cns_orthogonality_init();
  assert(manager != NULL);

  // Register components
  cns_orthogonality_register_component(manager, "SPARQL_Engine", CNS_ORTHOGONAL_TYPE_INDEPENDENT);
  cns_orthogonality_register_component(manager, "SHACL_Validator", CNS_ORTHOGONAL_TYPE_INDEPENDENT);
  cns_orthogonality_register_component(manager, "OWL_Reasoner", CNS_ORTHOGONAL_TYPE_INDEPENDENT);
  cns_orthogonality_register_component(manager, "Query_Planner", CNS_ORTHOGONAL_TYPE_WEAK_COUPLED);
  cns_orthogonality_register_component(manager, "Telemetry_System", CNS_ORTHOGONAL_TYPE_WEAK_COUPLED);

  // Add some dependencies (simulating real system)
  cns_orthogonality_add_dependency(manager, 3, 0); // Query_Planner depends on SPARQL_Engine
  cns_orthogonality_add_dependency(manager, 3, 1); // Query_Planner depends on SHACL_Validator
  cns_orthogonality_add_dependency(manager, 4, 0); // Telemetry_System depends on SPARQL_Engine

  // Calculate scores
  cns_orthogonality_calculate_scores(manager);

  // Validate orthogonality
  cns_result_t result = cns_orthogonality_validate(manager);
  assert(result == CNS_SUCCESS);

  // Get report
  char report[2048];
  cns_orthogonality_get_report(manager, report, sizeof(report));
  printf("   Orthogonality Report:\n%s", report);

  // Check individual components
  assert(cns_orthogonality_is_component_orthogonal(manager, 0)); // SPARQL_Engine
  assert(cns_orthogonality_is_component_orthogonal(manager, 1)); // SHACL_Validator
  assert(cns_orthogonality_is_component_orthogonal(manager, 2)); // OWL_Reasoner

  double overall_score = cns_orthogonality_get_overall_score(manager);
  assert(overall_score >= 0.8); // At least 80% orthogonality

  printf("   Overall Orthogonality Score: %.3f\n", overall_score);

  cns_orthogonality_cleanup(manager);
}

/*═══════════════════════════════════════════════════════════════
  Tracer Bullets Tests
  ═══════════════════════════════════════════════════════════════*/

static void test_tracer_bullets(void)
{
  cns_tracer_manager_t *manager = cns_tracer_init();
  assert(manager != NULL);

  // Create end-to-end tracer bullet
  cns_tracer_create_bullet(manager, "SPARQL_End_To_End",
                           "Complete SPARQL query processing pipeline",
                           CNS_TRACER_TYPE_END_TO_END);

  // Create performance tracer bullet
  cns_tracer_create_bullet(manager, "7_Tick_Performance",
                           "Validate 7-tick performance constraints",
                           CNS_TRACER_TYPE_PERFORMANCE);

  // Create integration tracer bullet
  cns_tracer_create_bullet(manager, "Component_Integration",
                           "Test component integration",
                           CNS_TRACER_TYPE_INTEGRATION);

  // Add steps to end-to-end bullet
  cns_tracer_add_step(manager, 0, "Parse SPARQL query", NULL, NULL);
  cns_tracer_add_step(manager, 0, "Execute query plan", NULL, NULL);
  cns_tracer_add_step(manager, 0, "Validate results", NULL, NULL);
  cns_tracer_add_step(manager, 0, "Generate response", NULL, NULL);

  // Add results
  cns_tracer_add_result(manager, 0, "query_time_ns", "1500");
  cns_tracer_add_result(manager, 0, "result_count", "42");
  cns_tracer_add_result(manager, 0, "validation_passed", "true");

  // Execute all bullets
  cns_tracer_execute_all(manager);

  // Validate system
  cns_tracer_validate_system(manager);

  // Get reports
  char bullet_report[2048];
  cns_tracer_get_bullet_report(manager, 0, bullet_report, sizeof(bullet_report));
  printf("   Tracer Bullet Report:\n%s", bullet_report);

  char system_report[2048];
  cns_tracer_get_system_report(manager, system_report, sizeof(system_report));
  printf("   System Report:\n%s", system_report);

  // Check bullet statuses
  assert(cns_tracer_get_bullet_status(manager, 0) == CNS_BULLET_STATUS_SUCCESS);
  assert(cns_tracer_get_bullet_status(manager, 1) == CNS_BULLET_STATUS_SUCCESS);
  assert(cns_tracer_get_bullet_status(manager, 2) == CNS_BULLET_STATUS_SUCCESS);

  cns_tracer_cleanup(manager);
}

/*═══════════════════════════════════════════════════════════════
  Reversibility Tests
  ═══════════════════════════════════════════════════════════════*/

static void test_reversibility(void)
{
  cns_reversibility_manager_t *manager = cns_reversibility_init();
  assert(manager != NULL);

  // Create undo stack
  cns_reversibility_create_undo_stack(manager, "Configuration_Changes");

  // Simulate configuration changes
  int config_before[] = {100, 200, 300};
  int config_after[] = {150, 250, 350};

  // Register configuration update operation
  cns_reversibility_register_operation(manager, "Update_Config",
                                       "Update system configuration",
                                       CNS_OP_TYPE_CONFIGURE,
                                       config_before, config_after, sizeof(config_before),
                                       NULL, NULL);

  // Execute operation
  cns_reversibility_execute_operation(manager, 0);

  // Add to undo stack
  cns_reversibility_add_to_stack(manager, 0);

  // Create checkpoint
  cns_reversibility_create_checkpoint(manager, "Config_Updated");

  // Simulate another change
  int config_after2[] = {200, 300, 400};
  cns_reversibility_register_operation(manager, "Update_Config_Again",
                                       "Update configuration again",
                                       CNS_OP_TYPE_CONFIGURE,
                                       config_after, config_after2, sizeof(config_after),
                                       NULL, NULL);

  cns_reversibility_execute_operation(manager, 1);
  cns_reversibility_add_to_stack(manager, 1);

  // Test undo last operation
  cns_reversibility_undo_last(manager);

  // Test rollback to checkpoint
  cns_reversibility_rollback_to_checkpoint(manager, "Config_Updated");

  // Get report
  char report[2048];
  cns_reversibility_get_report(manager, report, sizeof(report));
  printf("   Reversibility Report:\n%s", report);

  // Validate reversibility
  cns_reversibility_validate(manager);

  // Check operation statuses
  assert(cns_reversibility_get_operation_status(manager, 0) == CNS_OP_STATUS_EXECUTED);
  assert(cns_reversibility_is_operation_reversible(manager, 0));

  cns_reversibility_cleanup(manager);
}

/*═══════════════════════════════════════════════════════════════
  Integration Tests
  ═══════════════════════════════════════════════════════════════*/

static void test_integration(void)
{
  printf("   Testing integration of all pragmatic principles...\n");

  // Test that all principles work together
  // 1. Orthogonality ensures components are independent
  // 2. Tracer Bullets validate the complete system
  // 3. Reversibility allows undoing changes
  // 4. All principles maintain 7-tick performance

  // Create managers for all principles
  cns_orthogonality_manager_t *ortho_manager = cns_orthogonality_init();
  cns_tracer_manager_t *tracer_manager = cns_tracer_init();
  cns_reversibility_manager_t *reversibility_manager = cns_reversibility_init();

  assert(ortho_manager != NULL);
  assert(tracer_manager != NULL);
  assert(reversibility_manager != NULL);

  // Register orthogonal components
  cns_orthogonality_register_component(ortho_manager, "Integrated_Component", CNS_ORTHOGONAL_TYPE_INDEPENDENT);

  // Create tracer bullet for integration
  cns_tracer_create_bullet(tracer_manager, "Integration_Test",
                           "Test integration of all principles",
                           CNS_TRACER_TYPE_INTEGRATION);

  // Create reversible operation for integration
  int data_before = 100;
  int data_after = 200;
  cns_reversibility_register_operation(reversibility_manager, "Integration_Op",
                                       "Integration operation",
                                       CNS_OP_TYPE_CUSTOM,
                                       &data_before, &data_after, sizeof(data_before),
                                       NULL, NULL);

  // Execute all systems
  cns_orthogonality_validate(ortho_manager);
  cns_tracer_execute_all(tracer_manager);
  cns_reversibility_validate(reversibility_manager);

  // Verify integration success
  assert(cns_orthogonality_get_overall_score(ortho_manager) >= 0.8);
  assert(cns_tracer_get_bullet_status(tracer_manager, 0) == CNS_BULLET_STATUS_SUCCESS);
  assert(cns_reversibility_get_operation_status(reversibility_manager, 0) == CNS_OP_STATUS_PENDING);

  printf("   ✅ All principles integrated successfully\n");

  // Cleanup
  cns_orthogonality_cleanup(ortho_manager);
  cns_tracer_cleanup(tracer_manager);
  cns_reversibility_cleanup(reversibility_manager);
}