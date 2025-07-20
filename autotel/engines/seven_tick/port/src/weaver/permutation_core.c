/**
 * @file permutation_core.c
 * @brief Permutation Engine - Structured Chaos for Invariance Testing
 *
 * Principle: Chaos must be structured.
 * Purpose: Implement intelligent permutation algorithms that stress the specific physics of the 8T/8H/8M layers.
 */

#include "cns/cns_weaver.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>

// ============================================================================
// PERMUTATION ENGINE STATE
// ============================================================================

typedef struct
{
  uint64_t seed;
  uint64_t permutation_count;
  uint64_t successful_permutations;
  uint64_t failed_permutations;
  uint64_t total_execution_time;
} permutation_engine_state_t;

static permutation_engine_state_t engine_state = {0};

// ============================================================================
// RANDOM NUMBER GENERATION
// ============================================================================

// Linear congruential generator for reproducible randomness
static uint64_t lcg_next(uint64_t *state)
{
  *state = *state * 1103515245ULL + 12345ULL;
  return *state;
}

// Generate random value in range [0, max)
static uint64_t random_range(uint64_t *state, uint64_t max)
{
  return lcg_next(state) % max;
}

// ============================================================================
// TEMPORAL PERMUTATIONS
// ============================================================================

// Generate temporal jitter for operations
int permutation_generate_temporal_jitter(uint32_t op_count,
                                         uint32_t intensity,
                                         uint64_t seed,
                                         uint64_t *delays)
{
  assert(delays != NULL);

  uint64_t state = seed;

  for (uint32_t i = 0; i < op_count; i++)
  {
    // Generate delay between 1 and intensity cycles
    // This tests for hidden race conditions and timing dependencies
    delays[i] = random_range(&state, intensity) + 1;

    // Occasionally add larger delays for stress testing
    if (random_range(&state, 100) < 10)
    {
      delays[i] += random_range(&state, intensity * 5);
    }
  }

  return CNS_WEAVER_SUCCESS;
}

// Generate operation-specific timing variations
int permutation_generate_operation_timing(const cns_weave_op_t *sequence,
                                          uint32_t op_count,
                                          uint32_t intensity,
                                          uint64_t seed,
                                          uint64_t *delays)
{
  assert(sequence != NULL);
  assert(delays != NULL);

  uint64_t state = seed;

  for (uint32_t i = 0; i < op_count; i++)
  {
    uint32_t op_id = sequence[i].operation_id;

    // Different operation types get different timing characteristics
    if (op_id >= OP_8T_EXECUTE && op_id < OP_8T_EXECUTE + 0x100)
    {
      // 8T operations: minimal jitter (1-5 cycles)
      delays[i] = random_range(&state, 5) + 1;
    }
    else if (op_id >= OP_8H_COGNITIVE_CYCLE && op_id < OP_8H_COGNITIVE_CYCLE + 0x100)
    {
      // 8H operations: moderate jitter (1-10 cycles)
      delays[i] = random_range(&state, 10) + 1;
    }
    else if (op_id >= OP_8M_ALLOC && op_id < OP_8M_ALLOC + 0x100)
    {
      // 8M operations: variable jitter based on allocation size
      uint64_t alloc_size = sequence[i].args[0];
      delays[i] = random_range(&state, (alloc_size / 64) + 1) + 1;
    }
    else
    {
      // Other operations: standard jitter
      delays[i] = random_range(&state, intensity) + 1;
    }
  }

  return CNS_WEAVER_SUCCESS;
}

// ============================================================================
// SPATIAL PERMUTATIONS (MEMORY LAYOUT)
// ============================================================================

// Memory layout permutation context
typedef struct
{
  void *original_memory;
  void *permuted_memory;
  size_t total_size;
  size_t *fragment_sizes;
  uint32_t fragment_count;
} spatial_permutation_context_t;

// Create fragmented memory layout
int permutation_create_spatial_fragmentation(size_t total_size,
                                             uint32_t fragment_count,
                                             uint64_t seed,
                                             spatial_permutation_context_t *context)
{
  assert(context != NULL);
  assert(fragment_count > 0);

  context->total_size = total_size;
  context->fragment_count = fragment_count;

  // Allocate memory for fragment sizes
  context->fragment_sizes = malloc(fragment_count * sizeof(size_t));
  if (!context->fragment_sizes)
  {
    return CNS_WEAVER_ERROR_MEMORY;
  }

  // Generate random fragment sizes
  uint64_t state = seed;
  size_t remaining_size = total_size;

  for (uint32_t i = 0; i < fragment_count - 1; i++)
  {
    size_t max_fragment = remaining_size / (fragment_count - i);
    size_t fragment_size = random_range(&state, max_fragment) + 64; // Minimum 64 bytes

    context->fragment_sizes[i] = fragment_size;
    remaining_size -= fragment_size;
  }

  // Last fragment gets remaining size
  context->fragment_sizes[fragment_count - 1] = remaining_size;

  // Allocate fragmented memory
  context->permuted_memory = malloc(total_size + fragment_count * 64); // Extra space for fragmentation
  if (!context->permuted_memory)
  {
    free(context->fragment_sizes);
    return CNS_WEAVER_ERROR_MEMORY;
  }

  return CNS_WEAVER_SUCCESS;
}

// Apply spatial permutation to memory allocation
int permutation_apply_spatial_permutation(void *original_memory,
                                          size_t size,
                                          spatial_permutation_context_t *context,
                                          void **permuted_memory)
{
  (void)original_memory; // Unused parameter
  (void)size;            // Unused parameter
  assert(context != NULL);
  assert(permuted_memory != NULL);

  // For now, just return the fragmented memory
  // In a full implementation, this would copy data with fragmentation
  *permuted_memory = context->permuted_memory;

  return CNS_WEAVER_SUCCESS;
}

// Clean up spatial permutation context
void permutation_cleanup_spatial(spatial_permutation_context_t *context)
{
  if (context)
  {
    if (context->fragment_sizes)
    {
      free(context->fragment_sizes);
    }
    if (context->permuted_memory)
    {
      free(context->permuted_memory);
    }
    memset(context, 0, sizeof(*context));
  }
}

// ============================================================================
// LOGICAL PERMUTATIONS (OPERATION REORDERING)
// ============================================================================

// Dependency analysis for operation reordering
typedef struct
{
  uint32_t operation_id;
  uint32_t dependencies[8]; // Operations this depends on
  uint32_t dependents[8];   // Operations that depend on this
  uint32_t dep_count;
  uint32_t dependent_count;
  bool can_reorder;
} operation_dependency_t;

// Analyze dependencies in a sequence
int permutation_analyze_dependencies(const cns_weave_op_t *sequence,
                                     uint32_t op_count,
                                     operation_dependency_t *dependencies)
{
  assert(sequence != NULL);
  assert(dependencies != NULL);

  // Initialize dependency structures
  for (uint32_t i = 0; i < op_count; i++)
  {
    dependencies[i].operation_id = sequence[i].operation_id;
    dependencies[i].dep_count = 0;
    dependencies[i].dependent_count = 0;
    dependencies[i].can_reorder = true;
  }

  // Analyze dependencies based on operation types
  for (uint32_t i = 0; i < op_count; i++)
  {
    uint32_t op_id = sequence[i].operation_id;

    // 8M allocations must come before their use
    if (op_id >= OP_8M_ALLOC && op_id < OP_8M_ALLOC + 0x100)
    {
      // Find operations that use this allocation
      for (uint32_t j = i + 1; j < op_count; j++)
      {
        if (sequence[j].context == (void *)sequence[i].args[0])
        { // Simplified check
          dependencies[j].dependencies[dependencies[j].dep_count++] = i;
          dependencies[i].dependents[dependencies[i].dependent_count++] = j;
        }
      }
    }

    // Graph operations may have dependencies
    if (op_id >= OP_GRAPH_INIT && op_id < OP_GRAPH_INIT + 0x100)
    {
      if (op_id == OP_GRAPH_ADD_TRIPLE)
      {
        // Add triple depends on graph init
        for (uint32_t j = 0; j < i; j++)
        {
          if (sequence[j].operation_id == OP_GRAPH_INIT)
          {
            dependencies[i].dependencies[dependencies[i].dep_count++] = j;
            dependencies[j].dependents[dependencies[j].dependent_count++] = i;
            break;
          }
        }
      }
    }
  }

  return CNS_WEAVER_SUCCESS;
}

// Generate reordered sequence respecting dependencies
int permutation_generate_logical_reordering(const cns_weave_op_t *original_sequence,
                                            uint32_t op_count,
                                            uint32_t intensity,
                                            uint64_t seed,
                                            cns_weave_op_t *reordered_sequence)
{
  assert(original_sequence != NULL);
  assert(reordered_sequence != NULL);

  // Copy original sequence
  memcpy(reordered_sequence, original_sequence, op_count * sizeof(cns_weave_op_t));

  // Analyze dependencies
  operation_dependency_t *dependencies = malloc(op_count * sizeof(operation_dependency_t));
  if (!dependencies)
  {
    return CNS_WEAVER_ERROR_MEMORY;
  }

  int result = permutation_analyze_dependencies(original_sequence, op_count, dependencies);
  if (result != CNS_WEAVER_SUCCESS)
  {
    free(dependencies);
    return result;
  }

  // Perform reordering respecting dependencies
  uint64_t state = seed;

  for (uint32_t i = 0; i < op_count && intensity > 0; i++)
  {
    // Only reorder operations that can be reordered
    if (!dependencies[i].can_reorder)
    {
      continue;
    }

    // Probability of swapping based on intensity
    if (random_range(&state, 100) < intensity)
    {
      // Find a valid swap candidate
      for (uint32_t attempts = 0; attempts < 10; attempts++)
      {
        uint32_t j = random_range(&state, op_count);

        if (i != j && dependencies[j].can_reorder)
        {
          // Check if swap would violate dependencies
          bool valid_swap = true;

          // Check if j depends on i
          for (uint32_t k = 0; k < dependencies[j].dep_count; k++)
          {
            if (dependencies[j].dependencies[k] == i)
            {
              valid_swap = false;
              break;
            }
          }

          // Check if i depends on j
          for (uint32_t k = 0; k < dependencies[i].dep_count; k++)
          {
            if (dependencies[i].dependencies[k] == j)
            {
              valid_swap = false;
              break;
            }
          }

          if (valid_swap)
          {
            // Perform the swap
            cns_weave_op_t temp = reordered_sequence[i];
            reordered_sequence[i] = reordered_sequence[j];
            reordered_sequence[j] = temp;
            break;
          }
        }
      }
    }
  }

  free(dependencies);
  return CNS_WEAVER_SUCCESS;
}

// ============================================================================
// CONCURRENCY PERMUTATIONS
// ============================================================================

// Simulate concurrent execution by interleaving operations
int permutation_generate_concurrency_interleaving(const cns_weave_op_t *sequence_a,
                                                  uint32_t count_a,
                                                  const cns_weave_op_t *sequence_b,
                                                  uint32_t count_b,
                                                  uint64_t seed,
                                                  cns_weave_op_t *interleaved_sequence)
{
  assert(sequence_a != NULL);
  assert(sequence_b != NULL);
  assert(interleaved_sequence != NULL);

  uint64_t state = seed;
  uint32_t a_index = 0;
  uint32_t b_index = 0;
  uint32_t interleaved_index = 0;

  while (a_index < count_a || b_index < count_b)
  {
    // Decide which sequence to take from next
    bool take_from_a = false;

    if (a_index >= count_a)
    {
      take_from_a = false;
    }
    else if (b_index >= count_b)
    {
      take_from_a = true;
    }
    else
    {
      // Random choice
      take_from_a = (random_range(&state, 2) == 0);
    }

    // Copy operation from chosen sequence
    if (take_from_a)
    {
      interleaved_sequence[interleaved_index++] = sequence_a[a_index++];
    }
    else
    {
      interleaved_sequence[interleaved_index++] = sequence_b[b_index++];
    }
  }

  return CNS_WEAVER_SUCCESS;
}

// ============================================================================
// COMPOSITE PERMUTATIONS
// ============================================================================

// Apply multiple permutation types simultaneously
int permutation_apply_composite_permutation(const cns_weave_op_t *original_sequence,
                                            uint32_t op_count,
                                            cns_permutation_config_t *config,
                                            cns_weave_op_t *permuted_sequence,
                                            uint64_t *temporal_delays)
{
  assert(original_sequence != NULL);
  assert(config != NULL);
  assert(permuted_sequence != NULL);
  assert(temporal_delays != NULL);

  (void)config->seed; // Unused variable

  // Apply logical reordering if requested
  if (config->type & PERM_LOGICAL)
  {
    int result = permutation_generate_logical_reordering(original_sequence,
                                                         op_count,
                                                         config->intensity,
                                                         config->seed,
                                                         permuted_sequence);
    if (result != CNS_WEAVER_SUCCESS)
    {
      return result;
    }
  }
  else
  {
    // Copy original sequence
    memcpy(permuted_sequence, original_sequence, op_count * sizeof(cns_weave_op_t));
  }

  // Apply temporal permutations
  if (config->type & PERM_TEMPORAL)
  {
    int result = permutation_generate_operation_timing(permuted_sequence,
                                                       op_count,
                                                       config->intensity,
                                                       config->seed,
                                                       temporal_delays);
    if (result != CNS_WEAVER_SUCCESS)
    {
      return result;
    }
  }
  else
  {
    // No temporal delays
    memset(temporal_delays, 0, op_count * sizeof(uint64_t));
  }

  return CNS_WEAVER_SUCCESS;
}

// ============================================================================
// PERMUTATION GENERATION API
// ============================================================================

// Generate permutation configuration
cns_permutation_config_t permutation_generate_config(uint64_t seed,
                                                     cns_permutation_type_t type,
                                                     uint32_t intensity)
{
  cns_permutation_config_t config = {0};
  config.type = type;
  config.intensity = intensity;
  config.seed = seed;

  // Store type-specific metadata
  config.metadata[0] = seed;
  config.metadata[1] = type;
  config.metadata[2] = intensity;
  config.metadata[3] = time(NULL);

  return config;
}

// Generate a sequence of permutations for testing
int permutation_generate_sequence(uint32_t count,
                                  cns_permutation_type_t base_type,
                                  uint32_t base_intensity,
                                  uint64_t seed,
                                  cns_permutation_config_t *configs)
{
  assert(configs != NULL);

  uint64_t state = seed;

  for (uint32_t i = 0; i < count; i++)
  {
    // Vary the permutation type and intensity
    cns_permutation_type_t type = base_type;
    uint32_t intensity = base_intensity;

    // Occasionally add other permutation types
    if (random_range(&state, 100) < 20)
    {
      type |= PERM_TEMPORAL;
    }
    if (random_range(&state, 100) < 20)
    {
      type |= PERM_LOGICAL;
    }

    // Vary intensity
    intensity += random_range(&state, 20) - 10;
    if (intensity > 100)
      intensity = 100;
    if (intensity < 1)
      intensity = 1;

    configs[i] = permutation_generate_config(state, type, intensity);
    state = lcg_next(&state);
  }

  return CNS_WEAVER_SUCCESS;
}

// ============================================================================
// STATISTICS AND REPORTING
// ============================================================================

// Update engine statistics
void permutation_update_stats(bool success, uint64_t execution_time)
{
  engine_state.permutation_count++;
  engine_state.total_execution_time += execution_time;

  if (success)
  {
    engine_state.successful_permutations++;
  }
  else
  {
    engine_state.failed_permutations++;
  }
}

// Get engine statistics
void permutation_get_stats(permutation_engine_state_t *stats)
{
  if (stats)
  {
    *stats = engine_state;
  }
}

// Print permutation statistics
void permutation_print_stats(void)
{
  printf("=== Permutation Engine Statistics ===\n");
  printf("Total permutations: %llu\n", engine_state.permutation_count);
  printf("Successful: %llu\n", engine_state.successful_permutations);
  printf("Failed: %llu\n", engine_state.failed_permutations);
  printf("Success rate: %.2f%%\n",
         engine_state.permutation_count > 0 ? (double)engine_state.successful_permutations / engine_state.permutation_count * 100.0 : 0.0);
  printf("Total execution time: %llu cycles\n", engine_state.total_execution_time);
  printf("Average execution time: %llu cycles\n",
         engine_state.permutation_count > 0 ? engine_state.total_execution_time / engine_state.permutation_count : 0);
  printf("=== End Statistics ===\n");
}

// ============================================================================
// INITIALIZATION
// ============================================================================

// Initialize permutation engine
int permutation_init(uint64_t seed)
{
  engine_state.seed = seed;
  engine_state.permutation_count = 0;
  engine_state.successful_permutations = 0;
  engine_state.failed_permutations = 0;
  engine_state.total_execution_time = 0;

  printf("Permutation Engine initialized with seed 0x%016llX\n", seed);
  return CNS_WEAVER_SUCCESS;
}

// Clean up permutation engine
void permutation_cleanup(void)
{
  printf("Permutation Engine cleaned up\n");
}