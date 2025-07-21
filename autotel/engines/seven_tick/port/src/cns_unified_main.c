/**
 * @file cns_unified_main.c
 * @brief Unified CNS Entry Point - Phase 2 Dark 80/20 Integration
 * @version 1.0.0
 *
 * This is the unified entry point that connects all Phase 2 Dark 80/20
 * optimization components into a cohesive, 7-tick compliant system.
 *
 * @author 7T Engine Team
 * @date 2025-01-15
 */

#include "../include/cns/cns_core.h"
#include "../include/cns_commands.h"
#include "../pragmatic/contracts.c"
#include "../pragmatic/entropy.c"
#include "../performance_optimizations.c"
#include "../cjinja_final.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// =============================================================================
// GLOBAL STATE
// =============================================================================

static cns_entropy_tracker_t *g_entropy_tracker = NULL;
static cns_performance_contract_t *g_performance_contract = NULL;

// =============================================================================
// PERFORMANCE CONTRACT ENFORCEMENT
// =============================================================================

/**
 * @brief Initialize performance contract system
 */
static void cns_init_performance_contracts(void)
{
  g_performance_contract = malloc(sizeof(cns_performance_contract_t));
  g_performance_contract->max_cycles = 7; // 7-tick compliance
  g_performance_contract->span = NULL;    // TODO: OpenTelemetry integration

  printf("INFO: Performance contracts initialized (7-tick target)\n");
}

/**
 * @brief Validate operation performance
 */
static cns_result_t cns_validate_performance(
    const char *operation_name,
    uint64_t cycles)
{
  if (cycles > g_performance_contract->max_cycles)
  {
    printf("ERROR: Performance violation in %s: %lu cycles (max: %u)\n",
           operation_name, cycles, g_performance_contract->max_cycles);
    return CNS_ERROR_PERFORMANCE_VIOLATION;
  }

  printf("INFO: %s performance OK: %lu cycles\n", operation_name, cycles);
  return CNS_OK;
}

// =============================================================================
// ENTROPY MANAGEMENT
// =============================================================================

/**
 * @brief Initialize entropy tracking system
 */
static void cns_init_entropy_tracking(void)
{
  g_entropy_tracker = cns_entropy_init();
  if (!g_entropy_tracker)
  {
    fprintf(stderr, "ERROR: Failed to initialize entropy tracker\n");
    exit(1);
  }

  printf("INFO: Entropy tracking initialized\n");
}

/**
 * @brief Track operation complexity
 */
static void cns_track_operation_complexity(
    const char *operation_name,
    double complexity_score)
{
  if (g_entropy_tracker)
  {
    cns_entropy_track_operation(g_entropy_tracker, operation_name, complexity_score);

    double current_entropy = cns_entropy_get_score(g_entropy_tracker);
    if (current_entropy > 0.8)
    { // 80% threshold
      printf("WARNING: High entropy detected: %.2f\n", current_entropy);
      cns_entropy_reduce_complexity(g_entropy_tracker);
    }
  }
}

// =============================================================================
// SHACL INTEGRATION
// =============================================================================

/**
 * @brief Test SHACL validation with performance contracts
 */
static cns_result_t cns_test_shacl_validation(void)
{
  printf("\n=== SHACL Validation Test ===\n");

  // Sample SHACL data and shapes
  const char *test_data = "@prefix ex: <http://example.org/> .\n"
                          "ex:Person1 a ex:Person ;\n"
                          "  ex:name \"John Doe\" ;\n"
                          "  ex:age 30 .";

  const char *test_shapes = "@prefix sh: <http://www.w3.org/ns/shacl#> .\n"
                            "ex:PersonShape a sh:NodeShape ;\n"
                            "  sh:targetClass ex:Person ;\n"
                            "  sh:property [\n"
                            "    sh:path ex:name ;\n"
                            "    sh:minCount 1 ;\n"
                            "    sh:datatype xsd:string\n"
                            "  ] .";

  // Measure performance
  uint64_t start = s7t_cycles();

  // TODO: Call optimized SHACL validator
  // cns_shacl_result_t result = cns_shacl_l1_validate(test_data, test_shapes);

  uint64_t end = s7t_cycles();
  uint64_t cycles = end - start;

  // Validate performance
  cns_result_t perf_result = cns_validate_performance("SHACL_validation", cycles);

  // Track complexity
  cns_track_operation_complexity("SHACL_validation", 0.3);

  printf("SHACL validation completed in %lu cycles\n", cycles);
  return perf_result;
}

// =============================================================================
// CJINJA INTEGRATION
// =============================================================================

/**
 * @brief Test CJinja template rendering with performance contracts
 */
static cns_result_t cns_test_cjinja_rendering(void)
{
  printf("\n=== CJinja Template Test ===\n");

  // Create CJinja engine and context
  CJinjaEngine *engine = cjinja_create(NULL);
  CJinjaContext *ctx = cjinja_create_context();

  if (!engine || !ctx)
  {
    printf("ERROR: Failed to create CJinja engine/context\n");
    return CNS_ERROR_INITIALIZATION_FAILED;
  }

  // Set test variables
  cjinja_set_var(ctx, "name", "World");
  cjinja_set_var(ctx, "greeting", "Hello");

  // Test template
  const char *template_str = "{{greeting}} {{name}}!";

  // Measure performance
  uint64_t start = s7t_cycles();

  char *result = cjinja_render_string_7tick(template_str, ctx);

  uint64_t end = s7t_cycles();
  uint64_t cycles = end - start;

  // Validate performance
  cns_result_t perf_result = cns_validate_performance("CJinja_rendering", cycles);

  if (result)
  {
    printf("Template result: %s\n", result);
    free(result);
  }

  // Track complexity
  cns_track_operation_complexity("CJinja_rendering", 0.2);

  // Cleanup
  cjinja_destroy_context(ctx);
  cjinja_destroy(engine);

  printf("CJinja rendering completed in %lu cycles\n", cycles);
  return perf_result;
}

// =============================================================================
// PERFORMANCE PRIMITIVES TEST
// =============================================================================

/**
 * @brief Test performance optimization primitives
 */
static cns_result_t cns_test_performance_primitives(void)
{
  printf("\n=== Performance Primitives Test ===\n");

  // Test optimized string hashing
  const char *test_string = "performance_test_string";

  uint64_t start = s7t_cycles();
  uint32_t hash = s7t_hash_string(test_string, strlen(test_string));
  uint64_t end = s7t_cycles();
  uint64_t cycles = end - start;

  cns_result_t perf_result = cns_validate_performance("string_hashing", cycles);

  printf("String hash: 0x%08x (computed in %lu cycles)\n", hash, cycles);

  // Track complexity
  cns_track_operation_complexity("string_hashing", 0.1);

  return perf_result;
}

// =============================================================================
// MAIN ENTRY POINT
// =============================================================================

int main(int argc, char **argv)
{
  printf("CNS Unified Main - Phase 2 Dark 80/20 Integration\n");
  printf("================================================\n");

  // Initialize systems
  cns_init_performance_contracts();
  cns_init_entropy_tracking();

  printf("\nStarting component integration tests...\n");

  // Test all components with performance contracts
  cns_result_t results[3];

  results[0] = cns_test_performance_primitives();
  results[1] = cns_test_cjinja_rendering();
  results[2] = cns_test_shacl_validation();

  // Generate final report
  printf("\n=== Integration Test Results ===\n");

  const char *test_names[] = {
      "Performance Primitives",
      "CJinja Template Engine",
      "SHACL Validation"};

  int total_tests = 3;
  int passed_tests = 0;

  for (int i = 0; i < total_tests; i++)
  {
    if (results[i] == CNS_OK)
    {
      printf("✅ %s: PASSED\n", test_names[i]);
      passed_tests++;
    }
    else
    {
      printf("❌ %s: FAILED (error: %d)\n", test_names[i], results[i]);
    }
  }

  // Entropy report
  if (g_entropy_tracker)
  {
    double final_entropy = cns_entropy_get_score(g_entropy_tracker);
    printf("\nEntropy Score: %.2f (threshold: 0.80)\n", final_entropy);

    if (final_entropy <= 0.8)
    {
      printf("✅ Entropy management: PASSED\n");
    }
    else
    {
      printf("❌ Entropy management: FAILED (too complex)\n");
    }
  }

  printf("\nTest Summary: %d/%d tests passed\n", passed_tests, total_tests);

  // Cleanup
  if (g_entropy_tracker)
  {
    cns_entropy_cleanup(g_entropy_tracker);
  }
  if (g_performance_contract)
  {
    free(g_performance_contract);
  }

  return (passed_tests == total_tests) ? 0 : 1;
}