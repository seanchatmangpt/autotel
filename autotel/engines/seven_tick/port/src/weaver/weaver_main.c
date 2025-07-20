/**
 * @file weaver_main.c
 * @brief CNS Permutation Weaver (PW7) - Main Orchestrator
 *
 * Purpose: Coordinate all weaver components and provide high-level API for invariance testing.
 */

#include "cns/cns_weaver.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>

// ============================================================================
// WEAVER MAIN STATE
// ============================================================================

static cns_weaver_state_t weaver_state = {0};
static cns_hypothesis_engine_t hypothesis_engine = {0};

// ============================================================================
// CORE WEAVER API IMPLEMENTATION
// ============================================================================

// Initialize the weaver
int cns_weaver_init(cns_weaver_state_t *state)
{
  assert(state != NULL);

  memset(state, 0, sizeof(cns_weaver_state_t));

  // Initialize subsystems
  int result = probe_init();
  if (result != CNS_WEAVER_SUCCESS)
  {
    return result;
  }

  result = permutation_init(time(NULL));
  if (result != CNS_WEAVER_SUCCESS)
  {
    return result;
  }

  result = oracle_init();
  if (result != CNS_WEAVER_SUCCESS)
  {
    return result;
  }

  result = hypothesis_init(&hypothesis_engine);
  if (result != CNS_WEAVER_SUCCESS)
  {
    return result;
  }

  printf("CNS Permutation Weaver (PW7) initialized successfully\n");
  return CNS_WEAVER_SUCCESS;
}

// Run a permutation test
int cns_weaver_run(cns_weave_t *weave)
{
  assert(weave != NULL);

  printf("=== Running Permutation Test: %s ===\n", weave->name);
  printf("Operations: %u\n", weave->op_count);
  printf("Permutations: %u\n", weave->permutations_to_run);
  printf("Seed: 0x%016llX\n", weave->seed);

  // Initialize weaver state for this test
  weaver_state.current_weave = weave;
  weaver_state.total_permutations = 0;
  weaver_state.successful_permutations = 0;
  weaver_state.failed_permutations = 0;
  weaver_state.total_execution_time = 0;

  // Allocate result buffer
  weaver_state.results = malloc(weave->permutations_to_run * sizeof(cns_permutation_result_t));
  if (!weaver_state.results)
  {
    return CNS_WEAVER_ERROR_MEMORY;
  }
  weaver_state.result_count = 0;

  // Generate permutation configurations
  cns_permutation_config_t *configs = malloc(weave->permutations_to_run * sizeof(cns_permutation_config_t));
  if (!configs)
  {
    free(weaver_state.results);
    return CNS_WEAVER_ERROR_MEMORY;
  }

  int result = permutation_generate_sequence(weave->permutations_to_run,
                                             PERM_COMPOSITE, // Start with composite permutations
                                             50,             // Medium intensity
                                             weave->seed,
                                             configs);
  if (result != CNS_WEAVER_SUCCESS)
  {
    free(configs);
    free(weaver_state.results);
    return result;
  }

  // Run batch validation
  result = oracle_run_batch_validation(weave, configs, weave->permutations_to_run, weaver_state.results);
  if (result != CNS_WEAVER_SUCCESS)
  {
    free(configs);
    free(weaver_state.results);
    return result;
  }

  // Update statistics
  for (uint32_t i = 0; i < weave->permutations_to_run; i++)
  {
    bool success = weaver_state.results[i].is_invariant;
    uint64_t execution_time = weaver_state.results[i].execution_time;

    permutation_update_stats(success, execution_time);

    if (success)
    {
      weaver_state.successful_permutations++;
    }
    else
    {
      weaver_state.failed_permutations++;
    }
    weaver_state.total_execution_time += execution_time;
  }

  weaver_state.total_permutations = weave->permutations_to_run;

  // Generate hypotheses if there were failures
  if (weaver_state.failed_permutations > 0)
  {
    printf("\n=== Generating Hypotheses ===\n");

    cns_hypothesis_t hypotheses[CNS_WEAVER_MAX_HYPOTHESES];
    uint32_t hypothesis_count = 0;

    result = hypothesis_8h_cognitive_cycle(weaver_state.results, weaver_state.total_permutations,
                                           weave->canonical_sequence, weave->op_count,
                                           hypotheses, &hypothesis_count);
    if (result == CNS_WEAVER_SUCCESS && hypothesis_count > 0)
    {
      hypothesis_print_hypotheses(hypotheses, hypothesis_count);
    }
  }

  // Print final results
  cns_weaver_print_results(&weaver_state);

  free(configs);
  return CNS_WEAVER_SUCCESS;
}

// Print weaver results
void cns_weaver_print_results(const cns_weaver_state_t *state)
{
  assert(state != NULL);

  printf("\n=== CNS Permutation Weaver Results ===\n");
  printf("Test: %s\n", state->current_weave ? state->current_weave->name : "Unknown");
  printf("Total permutations: %llu\n", state->total_permutations);
  printf("Successful: %llu\n", state->successful_permutations);
  printf("Failed: %llu\n", state->failed_permutations);
  printf("Success rate: %.2f%%\n",
         state->total_permutations > 0 ? (double)state->successful_permutations / state->total_permutations * 100.0 : 0.0);
  printf("Total execution time: %llu cycles\n", state->total_execution_time);
  printf("Average execution time: %llu cycles\n",
         state->total_permutations > 0 ? state->total_execution_time / state->total_permutations : 0);

  if (state->failed_permutations > 0)
  {
    printf("\nINVARIANCE VIOLATIONS DETECTED!\n");
    printf("The system is NOT invariant under physical permutations.\n");
    printf("This indicates that the Trinity's logic is not perfectly orthogonal to its physical execution.\n");
  }
  else
  {
    printf("\nINVARIANCE PROVEN!\n");
    printf("The system is invariant under all tested physical permutations.\n");
    printf("The Trinity's logic is perfectly orthogonal to its physical execution.\n");
  }

  printf("=== End Results ===\n");
}

// Clean up weaver resources
void cns_weaver_cleanup(cns_weaver_state_t *state)
{
  if (state)
  {
    if (state->results)
    {
      free(state->results);
      state->results = NULL;
    }
    memset(state, 0, sizeof(cns_weaver_state_t));
  }

  // Clean up subsystems
  probe_cleanup();
  permutation_cleanup();
  oracle_cleanup();
  hypothesis_cleanup(&hypothesis_engine);

  printf("CNS Permutation Weaver cleaned up\n");
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

// Generate permutation configuration
cns_permutation_config_t cns_weaver_generate_permutation(uint64_t seed,
                                                         cns_permutation_type_t type,
                                                         uint32_t intensity)
{
  return permutation_generate_config(seed, type, intensity);
}

// ============================================================================
// DEMONSTRATION AND TESTING
// ============================================================================

// Example operation implementations for testing
static int test_8t_operation(void *context, uint64_t *args)
{
  // Simulate 8T operation
  uint64_t result = args[0] ^ args[1];
  return CNS_WEAVER_SUCCESS;
}

static int test_8h_operation(void *context, uint64_t *args)
{
  // Simulate 8H cognitive cycle
  uint64_t state = args[0];
  for (int i = 0; i < 8; i++)
  {
    state = state ^ (0x1111111111111111ULL << i);
  }
  return CNS_WEAVER_SUCCESS;
}

static int test_8m_operation(void *context, uint64_t *args)
{
  // Simulate 8M memory allocation
  size_t size = args[0];
  void *ptr = malloc(size);
  if (ptr)
  {
    free(ptr);
    return CNS_WEAVER_SUCCESS;
  }
  return CNS_WEAVER_ERROR_MEMORY;
}

// Example test sequence
static cns_weave_op_t example_sequence[] = {
    CNS_OP(OP_8T_EXECUTE, NULL, 0x1234567890ABCDEFULL, 0xFEDCBA0987654321ULL),
    CNS_OP(OP_8H_COGNITIVE_CYCLE, NULL, 0xAAAAAAAAAAAAAAAAULL),
    CNS_OP(OP_8M_ALLOC, NULL, 1024),
    CNS_OP(OP_8T_EXECUTE, NULL, 0x5555555555555555ULL, 0xAAAAAAAAAAAAAAAAULL),
    CNS_OP(OP_8H_COGNITIVE_CYCLE, NULL, 0xBBBBBBBBBBBBBBBBULL)};

// Example test case
CNS_PERMUTATION_DEFINE(example_invariance_test, example_sequence);

// Main function for demonstration
int main(int argc, char *argv[])
{
  printf("=== CNS Permutation Weaver (PW7) - Fifth Epoch Invariance Testing ===\n\n");

  // Initialize weaver
  int result = cns_weaver_init(&weaver_state);
  if (result != CNS_WEAVER_SUCCESS)
  {
    printf("Failed to initialize weaver: %d\n", result);
    return 1;
  }

  // Register test operations
  probe_register_operation(OP_8T_EXECUTE, test_8t_operation, "8T_EXECUTE");
  probe_register_operation(OP_8H_COGNITIVE_CYCLE, test_8h_operation, "8H_COGNITIVE_CYCLE");
  probe_register_operation(OP_8M_ALLOC, test_8m_operation, "8M_ALLOC");

  // Run example test
  printf("Running example invariance test...\n");
  result = cns_weaver_run(&example_invariance_test_weave);
  if (result != CNS_WEAVER_SUCCESS)
  {
    printf("Test failed: %d\n", result);
    cns_weaver_cleanup(&weaver_state);
    return 1;
  }

  // Print subsystem statistics
  printf("\n=== Subsystem Statistics ===\n");
  permutation_print_stats();
  hypothesis_print_stats();

  // Clean up
  cns_weaver_cleanup(&weaver_state);

  printf("\n=== Fifth Epoch Invariance Testing Complete ===\n");
  return 0;
}