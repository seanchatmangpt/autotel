/**
 * @file hypothesis_engine.c
 * @brief 8H Hypothesis Engine - Intelligent Adversary for Invariance Testing
 *
 * Principle: An intelligent system requires an intelligent adversary.
 * Purpose: Create a feedback loop where the Weaver learns how to break the Trinity.
 */

#include "cns/cns_weaver.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>

// ============================================================================
// 8H HYPOTHESIS ENGINE STATE
// ============================================================================

typedef struct
{
  cns_hypothesis_engine_t *engine;
  uint64_t observation_cycles;
  uint64_t orientation_cycles;
  uint64_t decision_cycles;
  uint64_t action_cycles;
  uint64_t total_cycles;
} hypothesis_8h_state_t;

static hypothesis_8h_state_t hypothesis_state = {0};

// ============================================================================
// OBSERVE PHASE - Pattern Recognition
// ============================================================================

// Analyze permutation results for patterns
typedef struct
{
  uint32_t permutation_type;
  uint64_t total_deviation;
  uint64_t occurrence_count;
  uint64_t max_deviation;
  uint64_t min_deviation;
  double average_deviation;
} pattern_analysis_t;

// Observe patterns in permutation results
int hypothesis_observe_patterns(const cns_permutation_result_t *results,
                                uint32_t result_count,
                                pattern_analysis_t *patterns,
                                uint32_t *pattern_count)
{
  assert(results != NULL);
  assert(patterns != NULL);
  assert(pattern_count != NULL);

  *pattern_count = 0;

  // Analyze each permutation type
  for (uint32_t type = PERM_TEMPORAL; type <= PERM_COMPOSITE; type *= 2)
  {
    uint64_t total_deviation = 0;
    uint64_t occurrence_count = 0;
    uint64_t max_deviation = 0;
    uint64_t min_deviation = UINT64_MAX;

    for (uint32_t i = 0; i < result_count; i++)
    {
      if (results[i].config.type & type)
      {
        total_deviation += results[i].deviation_score;
        occurrence_count++;

        if (results[i].deviation_score > max_deviation)
        {
          max_deviation = results[i].deviation_score;
        }
        if (results[i].deviation_score < min_deviation)
        {
          min_deviation = results[i].deviation_score;
        }
      }
    }

    if (occurrence_count > 0)
    {
      patterns[*pattern_count].permutation_type = type;
      patterns[*pattern_count].total_deviation = total_deviation;
      patterns[*pattern_count].occurrence_count = occurrence_count;
      patterns[*pattern_count].max_deviation = max_deviation;
      patterns[*pattern_count].min_deviation = min_deviation;
      patterns[*pattern_count].average_deviation = (double)total_deviation / occurrence_count;

      (*pattern_count)++;
    }
  }

  hypothesis_state.observation_cycles++;
  return CNS_WEAVER_SUCCESS;
}

// ============================================================================
// ORIENT PHASE - Cross-Reference Analysis
// ============================================================================

// Cross-reference patterns with semantic information
typedef struct
{
  uint32_t operation_id;
  uint64_t sensitivity_score;
  uint32_t affected_permutation_types;
  const char *semantic_context;
} operation_sensitivity_t;

// Analyze operation sensitivity to different permutation types
int hypothesis_analyze_operation_sensitivity(const cns_permutation_result_t *results,
                                             uint32_t result_count,
                                             const cns_weave_op_t *sequence,
                                             uint32_t op_count,
                                             operation_sensitivity_t *sensitivities,
                                             uint32_t *sensitivity_count)
{
  assert(results != NULL);
  assert(sequence != NULL);
  assert(sensitivities != NULL);
  assert(sensitivity_count != NULL);

  *sensitivity_count = 0;

  // Analyze each operation type
  for (uint32_t op_id = 0x0001; op_id <= 0x0402; op_id++)
  {
    uint64_t total_sensitivity = 0;
    uint32_t occurrence_count = 0;
    uint32_t affected_types = 0;

    for (uint32_t i = 0; i < result_count; i++)
    {
      // Check if this operation appears in the sequence
      bool operation_present = false;
      for (uint32_t j = 0; j < op_count; j++)
      {
        if (sequence[j].operation_id == op_id)
        {
          operation_present = true;
          break;
        }
      }

      if (operation_present)
      {
        total_sensitivity += results[i].deviation_score;
        occurrence_count++;
        affected_types |= results[i].config.type;
      }
    }

    if (occurrence_count > 0)
    {
      sensitivities[*sensitivity_count].operation_id = op_id;
      sensitivities[*sensitivity_count].sensitivity_score = total_sensitivity / occurrence_count;
      sensitivities[*sensitivity_count].affected_permutation_types = affected_types;

      // Assign semantic context based on operation type
      if (op_id >= OP_8T_EXECUTE && op_id < OP_8T_EXECUTE + 0x100)
      {
        sensitivities[*sensitivity_count].semantic_context = "8T Physics Layer";
      }
      else if (op_id >= OP_8H_COGNITIVE_CYCLE && op_id < OP_8H_COGNITIVE_CYCLE + 0x100)
      {
        sensitivities[*sensitivity_count].semantic_context = "8H Cognitive Layer";
      }
      else if (op_id >= OP_8M_ALLOC && op_id < OP_8M_ALLOC + 0x100)
      {
        sensitivities[*sensitivity_count].semantic_context = "8M Memory Layer";
      }
      else if (op_id >= OP_SHACL_VALIDATE && op_id < OP_SHACL_VALIDATE + 0x100)
      {
        sensitivities[*sensitivity_count].semantic_context = "SHACL Validation";
      }
      else if (op_id >= OP_SPARQL_QUERY && op_id < OP_SPARQL_QUERY + 0x100)
      {
        sensitivities[*sensitivity_count].semantic_context = "SPARQL Query Processing";
      }
      else if (op_id >= OP_GRAPH_INIT && op_id < OP_GRAPH_INIT + 0x100)
      {
        sensitivities[*sensitivity_count].semantic_context = "Graph Operations";
      }
      else
      {
        sensitivities[*sensitivity_count].semantic_context = "Unknown Operation";
      }

      (*sensitivity_count)++;
    }
  }

  hypothesis_state.orientation_cycles++;
  return CNS_WEAVER_SUCCESS;
}

// ============================================================================
// DECIDE PHASE - Hypothesis Formation
// ============================================================================

// Form hypotheses based on observed patterns and sensitivities
int hypothesis_form_hypotheses(const pattern_analysis_t *patterns,
                               uint32_t pattern_count,
                               const operation_sensitivity_t *sensitivities,
                               uint32_t sensitivity_count,
                               cns_hypothesis_t *hypotheses,
                               uint32_t *hypothesis_count)
{
  assert(patterns != NULL);
  assert(sensitivities != NULL);
  assert(hypotheses != NULL);
  assert(hypothesis_count != NULL);

  *hypothesis_count = 0;

  // Find the most sensitive operation
  uint64_t max_sensitivity = 0;
  uint32_t most_sensitive_op = 0;

  for (uint32_t i = 0; i < sensitivity_count; i++)
  {
    if (sensitivities[i].sensitivity_score > max_sensitivity)
    {
      max_sensitivity = sensitivities[i].sensitivity_score;
      most_sensitive_op = sensitivities[i].operation_id;
    }
  }

  // Find the most problematic permutation type
  uint64_t max_pattern_deviation = 0;
  uint32_t most_problematic_type = 0;

  for (uint32_t i = 0; i < pattern_count; i++)
  {
    if (patterns[i].average_deviation > max_pattern_deviation)
    {
      max_pattern_deviation = patterns[i].average_deviation;
      most_problematic_type = patterns[i].permutation_type;
    }
  }

  // Generate hypotheses based on findings
  if (max_sensitivity > 0 && max_pattern_deviation > 0)
  {
    // Hypothesis 1: Specific operation is sensitive to specific permutation type
    hypotheses[*hypothesis_count].description = "Operation shows sensitivity to permutation type";
    hypotheses[*hypothesis_count].operation_id = most_sensitive_op;
    hypotheses[*hypothesis_count].permutation_type = most_problematic_type;
    hypotheses[*hypothesis_count].confidence_score = (max_sensitivity * 100) / 0xFFFFFFFFFFFFFFFFULL;
    hypotheses[*hypothesis_count].evidence_count = 1;
    (*hypothesis_count)++;

    // Hypothesis 2: Composite permutation targeting the sensitive operation
    if (*hypothesis_count < CNS_WEAVER_MAX_HYPOTHESES)
    {
      hypotheses[*hypothesis_count].description = "Composite permutation targeting sensitive operation";
      hypotheses[*hypothesis_count].operation_id = most_sensitive_op;
      hypotheses[*hypothesis_count].permutation_type = PERM_COMPOSITE;
      hypotheses[*hypothesis_count].confidence_score = (max_sensitivity * 80) / 0xFFFFFFFFFFFFFFFFULL;
      hypotheses[*hypothesis_count].evidence_count = 1;
      (*hypothesis_count)++;
    }
  }

  // Generate timing-specific hypotheses
  if (most_problematic_type & PERM_TEMPORAL)
  {
    if (*hypothesis_count < CNS_WEAVER_MAX_HYPOTHESES)
    {
      hypotheses[*hypothesis_count].description = "System shows timing sensitivity";
      hypotheses[*hypothesis_count].operation_id = 0; // Not operation-specific
      hypotheses[*hypothesis_count].permutation_type = PERM_TEMPORAL;
      hypotheses[*hypothesis_count].confidence_score = (max_pattern_deviation * 100) / 0xFFFFFFFFFFFFFFFFULL;
      hypotheses[*hypothesis_count].evidence_count = pattern_count;
      (*hypothesis_count)++;
    }
  }

  // Generate memory-specific hypotheses
  if (most_problematic_type & PERM_SPATIAL)
  {
    if (*hypothesis_count < CNS_WEAVER_MAX_HYPOTHESES)
    {
      hypotheses[*hypothesis_count].description = "System shows memory layout sensitivity";
      hypotheses[*hypothesis_count].operation_id = 0; // Not operation-specific
      hypotheses[*hypothesis_count].permutation_type = PERM_SPATIAL;
      hypotheses[*hypothesis_count].confidence_score = (max_pattern_deviation * 100) / 0xFFFFFFFFFFFFFFFFULL;
      hypotheses[*hypothesis_count].evidence_count = pattern_count;
      (*hypothesis_count)++;
    }
  }

  hypothesis_state.decision_cycles++;
  return CNS_WEAVER_SUCCESS;
}

// ============================================================================
// ACT PHASE - Test Case Generation
// ============================================================================

// Generate new test cases based on hypotheses
int hypothesis_generate_test_cases(const cns_hypothesis_t *hypotheses,
                                   uint32_t hypothesis_count,
                                   const cns_weave_op_t *original_sequence,
                                   uint32_t op_count,
                                   cns_weave_t *new_test_cases,
                                   uint32_t *test_case_count)
{
  assert(hypotheses != NULL);
  assert(original_sequence != NULL);
  assert(new_test_cases != NULL);
  assert(test_case_count != NULL);

  *test_case_count = 0;

  for (uint32_t i = 0; i < hypothesis_count && *test_case_count < 10; i++)
  {
    const cns_hypothesis_t *hypothesis = &hypotheses[i];

    // Generate a focused test case based on the hypothesis
    cns_weave_t *test_case = &new_test_cases[*test_case_count];

    // Copy original sequence
    test_case->canonical_sequence = malloc(op_count * sizeof(cns_weave_op_t));
    if (!test_case->canonical_sequence)
    {
      return CNS_WEAVER_ERROR_MEMORY;
    }

    memcpy(test_case->canonical_sequence, original_sequence, op_count * sizeof(cns_weave_op_t));

    // Configure test case based on hypothesis
    test_case->name = malloc(256);
    if (!test_case->name)
    {
      free(test_case->canonical_sequence);
      return CNS_WEAVER_ERROR_MEMORY;
    }

    snprintf((char *)test_case->name, 256, "hypothesis_test_%u_%s",
             i, hypothesis->description);

    test_case->op_count = op_count;
    test_case->permutations_to_run = 100;            // Focused testing
    test_case->seed = time(NULL) + i;                // Unique seed
    test_case->flags = hypothesis->permutation_type; // Use hypothesis permutation type

    // Store hypothesis metadata
    test_case->metadata[0] = hypothesis->operation_id;
    test_case->metadata[1] = hypothesis->permutation_type;
    test_case->metadata[2] = hypothesis->confidence_score;
    test_case->metadata[3] = hypothesis->evidence_count;

    (*test_case_count)++;
  }

  hypothesis_state.action_cycles++;
  return CNS_WEAVER_SUCCESS;
}

// ============================================================================
// 8H COGNITIVE CYCLE INTEGRATION
// ============================================================================

// Complete 8H cognitive cycle for hypothesis generation
int hypothesis_8h_cognitive_cycle(const cns_permutation_result_t *results,
                                  uint32_t result_count,
                                  const cns_weave_op_t *sequence,
                                  uint32_t op_count,
                                  cns_hypothesis_t *hypotheses,
                                  uint32_t *hypothesis_count)
{
  assert(results != NULL);
  assert(sequence != NULL);
  assert(hypotheses != NULL);
  assert(hypothesis_count != NULL);

  // Phase 1: OBSERVE - Pattern recognition
  pattern_analysis_t patterns[8];
  uint32_t pattern_count = 0;

  int result = hypothesis_observe_patterns(results, result_count, patterns, &pattern_count);
  if (result != CNS_WEAVER_SUCCESS)
  {
    return result;
  }

  // Phase 2: ORIENT - Cross-reference analysis
  operation_sensitivity_t sensitivities[64];
  uint32_t sensitivity_count = 0;

  result = hypothesis_analyze_operation_sensitivity(results, result_count, sequence, op_count,
                                                    sensitivities, &sensitivity_count);
  if (result != CNS_WEAVER_SUCCESS)
  {
    return result;
  }

  // Phase 3: DECIDE - Hypothesis formation
  result = hypothesis_form_hypotheses(patterns, pattern_count, sensitivities, sensitivity_count,
                                      hypotheses, hypothesis_count);
  if (result != CNS_WEAVER_SUCCESS)
  {
    return result;
  }

  // Phase 4: ACT - Test case generation (optional)
  // This would generate new test cases, but we'll skip it for now

  hypothesis_state.total_cycles++;

  printf("8H Hypothesis Engine completed cognitive cycle:\n");
  printf("  Patterns observed: %u\n", pattern_count);
  printf("  Operations analyzed: %u\n", sensitivity_count);
  printf("  Hypotheses generated: %u\n", *hypothesis_count);
  printf("  Total cycles: %llu\n", hypothesis_state.total_cycles);

  return CNS_WEAVER_SUCCESS;
}

// ============================================================================
// HYPOTHESIS ENGINE API
// ============================================================================

// Generate a new hypothesis based on observed deviations
int cns_weaver_generate_hypothesis(cns_weaver_state_t *state,
                                   cns_hypothesis_t *hypothesis)
{
  assert(state != NULL);
  assert(hypothesis != NULL);

  if (state->result_count == 0)
  {
    return CNS_WEAVER_ERROR_INVALID_ARGS;
  }

  // Run 8H cognitive cycle to generate hypotheses
  cns_hypothesis_t hypotheses[CNS_WEAVER_MAX_HYPOTHESES];
  uint32_t hypothesis_count = 0;

  int result = hypothesis_8h_cognitive_cycle(state->results, state->result_count,
                                             state->current_weave->canonical_sequence,
                                             state->current_weave->op_count,
                                             hypotheses, &hypothesis_count);
  if (result != CNS_WEAVER_SUCCESS)
  {
    return result;
  }

  if (hypothesis_count == 0)
  {
    return CNS_WEAVER_ERROR_INVALID_ARGS;
  }

  // Return the highest-confidence hypothesis
  uint64_t max_confidence = 0;
  uint32_t best_hypothesis = 0;

  for (uint32_t i = 0; i < hypothesis_count; i++)
  {
    if (hypotheses[i].confidence_score > max_confidence)
    {
      max_confidence = hypotheses[i].confidence_score;
      best_hypothesis = i;
    }
  }

  *hypothesis = hypotheses[best_hypothesis];

  return CNS_WEAVER_SUCCESS;
}

// ============================================================================
// STATISTICS AND REPORTING
// ============================================================================

// Get hypothesis engine statistics
void hypothesis_get_stats(hypothesis_8h_state_t *stats)
{
  if (stats)
  {
    *stats = hypothesis_state;
  }
}

// Print hypothesis engine statistics
void hypothesis_print_stats(void)
{
  printf("=== 8H Hypothesis Engine Statistics ===\n");
  printf("Observation cycles: %llu\n", hypothesis_state.observation_cycles);
  printf("Orientation cycles: %llu\n", hypothesis_state.orientation_cycles);
  printf("Decision cycles: %llu\n", hypothesis_state.decision_cycles);
  printf("Action cycles: %llu\n", hypothesis_state.action_cycles);
  printf("Total cognitive cycles: %llu\n", hypothesis_state.total_cycles);
  printf("=== End Statistics ===\n");
}

// Print generated hypotheses
void hypothesis_print_hypotheses(const cns_hypothesis_t *hypotheses,
                                 uint32_t hypothesis_count)
{
  printf("=== Generated Hypotheses ===\n");

  for (uint32_t i = 0; i < hypothesis_count; i++)
  {
    const cns_hypothesis_t *h = &hypotheses[i];

    printf("Hypothesis %u:\n", i + 1);
    printf("  Description: %s\n", h->description);
    printf("  Operation ID: 0x%04X\n", h->operation_id);
    printf("  Permutation Type: 0x%02X\n", h->permutation_type);
    printf("  Confidence: %llu%%\n", h->confidence_score);
    printf("  Evidence Count: %llu\n", h->evidence_count);
    printf("\n");
  }

  printf("=== End Hypotheses ===\n");
}

// ============================================================================
// INITIALIZATION
// ============================================================================

// Initialize hypothesis engine
int hypothesis_init(cns_hypothesis_engine_t *engine)
{
  assert(engine != NULL);

  memset(engine, 0, sizeof(cns_hypothesis_engine_t));
  engine->max_hypotheses = CNS_WEAVER_MAX_HYPOTHESES;

  // Initialize 8H state
  memset(&hypothesis_state, 0, sizeof(hypothesis_state));
  hypothesis_state.engine = engine;

  printf("8H Hypothesis Engine initialized\n");
  return CNS_WEAVER_SUCCESS;
}

// Clean up hypothesis engine
void hypothesis_cleanup(cns_hypothesis_engine_t *engine)
{
  if (engine && engine->hypotheses)
  {
    free(engine->hypotheses);
    engine->hypotheses = NULL;
  }

  printf("8H Hypothesis Engine cleaned up\n");
}