/**
 * @file trinity_probe.c
 * @brief Trinity Probe - Deterministic Interface for CNS v8 Substrate
 *
 * Purpose: To measure invariance, one must have a perfectly controlled way to interact with the system under test.
 * The probe provides cycle-level precision and telemetry capture for every operation.
 */

#include "cns/cns_weaver.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>

// ============================================================================
// FUNCTION POINTER TABLE FOR CNS v8 OPERATIONS
// ============================================================================

typedef int (*cns_operation_fn_t)(void *context, uint64_t *args);

// Function pointer table mapping operation IDs to actual functions
static cns_operation_fn_t operation_table[0xFFFF] = {0};

// Operation names for debugging
static const char *operation_names[0xFFFF] = {0};

// ============================================================================
// CYCLE-LEVEL TIMING AND TELEMETRY
// ============================================================================

// probe_telemetry_t is now defined in cns_weaver.h

// Get current cycle count with maximum precision
static inline uint64_t probe_get_cycles(void)
{
#if defined(__x86_64__) || defined(__i386__)
  uint32_t hi, lo;
  __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
  return ((uint64_t)lo) | (((uint64_t)hi) << 32);
#elif defined(__aarch64__)
  uint64_t cycles;
  __asm__ volatile("mrs %0, cntvct_el0" : "=r"(cycles));
  return cycles;
#else
  // Fallback to clock_gettime for other architectures
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
#endif
}

// Introduce controlled delay (for temporal permutations)
static inline void probe_delay_cycles(uint64_t cycles)
{
  uint64_t start = probe_get_cycles();
  while (probe_get_cycles() - start < cycles)
  {
    // Busy wait for precise cycle counting
    __asm__ volatile("nop");
  }
}

// ============================================================================
// TRINITY PROBE CORE
// ============================================================================

// Execute a single operation with full telemetry
static int probe_execute_operation(const cns_weave_op_t *op,
                                   probe_telemetry_t *telemetry,
                                   uint64_t delay_cycles)
{
  assert(op != NULL);
  assert(telemetry != NULL);

  // Record start time
  telemetry->start_ticks = probe_get_cycles();
  telemetry->operation_id = op->operation_id;

  // Introduce controlled delay if specified (for temporal permutations)
  if (delay_cycles > 0)
  {
    probe_delay_cycles(delay_cycles);
  }

  // Execute the operation
  cns_operation_fn_t fn = operation_table[op->operation_id];
  if (fn == NULL)
  {
    fprintf(stderr, "ERROR: Unknown operation ID 0x%04X\n", op->operation_id);
    return CNS_WEAVER_ERROR_EXECUTION;
  }

  int result = fn(op->context, (uint64_t *)op->args);
  telemetry->result = result;

  // Record end time
  telemetry->end_ticks = probe_get_cycles();

  // Calculate execution time
  uint64_t execution_cycles = telemetry->end_ticks - telemetry->start_ticks;

  // Store telemetry data
  telemetry->telemetry_data[0] = execution_cycles;
  telemetry->telemetry_data[1] = op->metadata;
  telemetry->telemetry_data[2] = (uint64_t)op->context;
  telemetry->telemetry_data[3] = op->args[0];
  telemetry->telemetry_data[4] = op->args[1];
  telemetry->telemetry_data[5] = op->args[2];
  telemetry->telemetry_data[6] = op->args[3];
  telemetry->telemetry_data[7] = op->args[4];

  return result;
}

// Execute a complete sequence of operations
int probe_execute_sequence(const cns_weave_op_t *sequence,
                           uint32_t op_count,
                           probe_telemetry_t *telemetry_buffer,
                           uint64_t *delays)
{
  assert(sequence != NULL);
  assert(telemetry_buffer != NULL);

  for (uint32_t i = 0; i < op_count; i++)
  {
    uint64_t delay = (delays != NULL) ? delays[i] : 0;

    int result = probe_execute_operation(&sequence[i],
                                         &telemetry_buffer[i],
                                         delay);

    if (result != CNS_WEAVER_SUCCESS)
    {
      fprintf(stderr, "ERROR: Operation %d (0x%04X) failed with code %d\n",
              i, sequence[i].operation_id, result);
      return result;
    }
  }

  return CNS_WEAVER_SUCCESS;
}

// ============================================================================
// OPERATION REGISTRATION SYSTEM
// ============================================================================

// Register an operation function
int probe_register_operation(uint32_t operation_id,
                             cns_operation_fn_t function,
                             const char *name)
{
  if (operation_id >= 0xFFFF)
  {
    return CNS_WEAVER_ERROR_INVALID_ARGS;
  }

  operation_table[operation_id] = function;
  operation_names[operation_id] = name;

  return CNS_WEAVER_SUCCESS;
}

// Get operation name for debugging
const char *probe_get_operation_name(uint32_t operation_id)
{
  if (operation_id >= 0xFFFF)
  {
    return "UNKNOWN";
  }
  return operation_names[operation_id] ? operation_names[operation_id] : "UNREGISTERED";
}

// ============================================================================
// GATEKEEPER INTEGRATION
// ============================================================================

// Collect gatekeeper metrics after sequence execution
int probe_collect_gatekeeper_metrics(const probe_telemetry_t *telemetry,
                                     uint32_t telemetry_count,
                                     gatekeeper_metrics_t *metrics)
{
  assert(telemetry != NULL);
  assert(metrics != NULL);

  // Initialize metrics
  memset(metrics, 0, sizeof(gatekeeper_metrics_t));

  // Aggregate telemetry data
  for (uint32_t i = 0; i < telemetry_count; i++)
  {
    metrics->total_ticks += telemetry[i].telemetry_data[0]; // execution_cycles
    metrics->operations_completed++;

    // Extract operation-specific metrics from telemetry
    uint64_t op_metadata = telemetry[i].telemetry_data[1];

    // Parse metadata based on operation type
    uint32_t op_id = telemetry[i].operation_id;
    if (op_id >= OP_8T_EXECUTE && op_id < OP_8T_EXECUTE + 0x100)
    {
      metrics->physics_operations++;
    }
    else if (op_id >= OP_8H_COGNITIVE_CYCLE && op_id < OP_8H_COGNITIVE_CYCLE + 0x100)
    {
      metrics->cognitive_cycle_count++;
    }
    else if (op_id >= OP_8M_ALLOC && op_id < OP_8M_ALLOC + 0x100)
    {
      metrics->memory_quanta_used += op_metadata;
    }
    else if (op_id >= OP_SHACL_VALIDATE && op_id < OP_SHACL_VALIDATE + 0x100)
    {
      metrics->shacl_validations++;
    }
    else if (op_id >= OP_SPARQL_QUERY && op_id < OP_SPARQL_QUERY + 0x100)
    {
      metrics->sparql_queries++;
    }
    else if (op_id >= OP_GRAPH_INIT && op_id < OP_GRAPH_INIT + 0x100)
    {
      metrics->graph_operations++;
    }
  }

  // Calculate deterministic checksum
  metrics->checksum = 0;
  for (uint32_t i = 0; i < telemetry_count; i++)
  {
    metrics->checksum ^= telemetry[i].operation_id;
    metrics->checksum ^= telemetry[i].result;
    metrics->checksum ^= telemetry[i].telemetry_data[0]; // execution_cycles
  }

  // Add other metrics to checksum
  metrics->checksum ^= metrics->total_ticks;
  metrics->checksum ^= metrics->operations_completed;
  metrics->checksum ^= metrics->physics_operations;
  metrics->checksum ^= metrics->cognitive_cycle_count;
  metrics->checksum ^= metrics->memory_quanta_used;

  return CNS_WEAVER_SUCCESS;
}

// ============================================================================
// PERMUTATION SUPPORT
// ============================================================================

// Generate temporal delays for permutation testing
int probe_generate_temporal_delays(uint32_t op_count,
                                   uint32_t intensity,
                                   uint64_t seed,
                                   uint64_t *delays)
{
  assert(delays != NULL);

  // Simple linear congruential generator for reproducible randomness
  uint64_t state = seed;

  for (uint32_t i = 0; i < op_count; i++)
  {
    // Generate delay between 0 and intensity cycles
    state = state * 1103515245ULL + 12345ULL;
    delays[i] = (state % (intensity + 1));
  }

  return CNS_WEAVER_SUCCESS;
}

// Reorder operations for logical permutation testing
int probe_reorder_operations(const cns_weave_op_t *original_sequence,
                             uint32_t op_count,
                             uint32_t intensity,
                             uint64_t seed,
                             cns_weave_op_t *reordered_sequence)
{
  assert(original_sequence != NULL);
  assert(reordered_sequence != NULL);

  // Copy original sequence
  memcpy(reordered_sequence, original_sequence, op_count * sizeof(cns_weave_op_t));

  // Simple random reordering based on intensity
  uint64_t state = seed;

  for (uint32_t i = 0; i < op_count && intensity > 0; i++)
  {
    state = state * 1103515245ULL + 12345ULL;

    // Probability of swapping based on intensity
    if ((state % 100) < intensity)
    {
      uint32_t j = state % op_count;
      if (i != j)
      {
        // Swap operations
        cns_weave_op_t temp = reordered_sequence[i];
        reordered_sequence[i] = reordered_sequence[j];
        reordered_sequence[j] = temp;
      }
    }
  }

  return CNS_WEAVER_SUCCESS;
}

// ============================================================================
// DEBUGGING AND DIAGNOSTICS
// ============================================================================

// Print telemetry for debugging
void probe_print_telemetry(const probe_telemetry_t *telemetry, uint32_t count)
{
  printf("=== Trinity Probe Telemetry ===\n");
  printf("Operations executed: %u\n", count);
  printf("\n");

  for (uint32_t i = 0; i < count; i++)
  {
    const char *op_name = probe_get_operation_name(telemetry[i].operation_id);
    uint64_t cycles = telemetry[i].telemetry_data[0];

    printf("Op %2u: 0x%04llX (%s) - %llu cycles, result=%lld\n",
           i, telemetry[i].operation_id, op_name, cycles, telemetry[i].result);
  }

  printf("=== End Telemetry ===\n");
}

// Print gatekeeper metrics
void probe_print_gatekeeper_metrics(const gatekeeper_metrics_t *metrics)
{
  printf("=== Gatekeeper Metrics ===\n");
  printf("Total ticks: %llu\n", metrics->total_ticks);
  printf("Operations completed: %llu\n", metrics->operations_completed);
  printf("Physics operations: %llu\n", metrics->physics_operations);
  printf("Cognitive cycles: %llu\n", metrics->cognitive_cycle_count);
  printf("Memory quanta: %llu\n", metrics->memory_quanta_used);
  printf("SHACL validations: %llu\n", metrics->shacl_validations);
  printf("SPARQL queries: %llu\n", metrics->sparql_queries);
  printf("Graph operations: %llu\n", metrics->graph_operations);
  printf("Checksum: 0x%016llX\n", metrics->checksum);
  printf("=== End Metrics ===\n");
}

// ============================================================================
// INITIALIZATION
// ============================================================================

// Initialize the probe system
int probe_init(void)
{
  // Clear operation table
  memset(operation_table, 0, sizeof(operation_table));
  memset(operation_names, 0, sizeof(operation_names));

  printf("Trinity Probe initialized\n");
  return CNS_WEAVER_SUCCESS;
}

// Clean up probe resources
void probe_cleanup(void)
{
  // Nothing to clean up for now
  printf("Trinity Probe cleaned up\n");
}