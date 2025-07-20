#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include "cns/telemetry/otel.h"
#include "s7t_patterns.h"

// ============================================================================
// DESIGN BY CONTRACT IMPLEMENTATION FOR CNS
// ============================================================================

// Contract validation macros with telemetry
#define S7T_CONTRACT_PRE(span, condition, message)                         \
  do                                                                       \
  {                                                                        \
    if (!(condition))                                                      \
    {                                                                      \
      otel_span_set_attribute(span, "contract.violation", "precondition"); \
      otel_span_set_attribute(span, "contract.message", message);          \
      otel_span_set_attribute(span, "contract.condition", #condition);     \
      return S7T_ERROR_INVALID_INPUT;                                      \
    }                                                                      \
  } while (0)

#define S7T_CONTRACT_POST(span, condition, message)                         \
  do                                                                        \
  {                                                                         \
    if (!(condition))                                                       \
    {                                                                       \
      otel_span_set_attribute(span, "contract.violation", "postcondition"); \
      otel_span_set_attribute(span, "contract.message", message);           \
      otel_span_set_attribute(span, "contract.condition", #condition);      \
      return S7T_ERROR_INVALID_OUTPUT;                                      \
    }                                                                       \
  } while (0)

#define S7T_CONTRACT_INV(span, condition, message)                      \
  do                                                                    \
  {                                                                     \
    if (!(condition))                                                   \
    {                                                                   \
      otel_span_set_attribute(span, "contract.violation", "invariant"); \
      otel_span_set_attribute(span, "contract.message", message);       \
      otel_span_set_attribute(span, "contract.condition", #condition);  \
      return S7T_ERROR_INVALID_STATE;                                   \
    }                                                                   \
  } while (0)

// Performance contract validation
#define S7T_CONTRACT_PERFORMANCE(span, operation, max_ns)                        \
  do                                                                             \
  {                                                                              \
    uint64_t start = get_microseconds();                                         \
    operation;                                                                   \
    uint64_t end = get_microseconds();                                           \
    double ns_per_op = (end - start) * 1000.0;                                   \
    if (ns_per_op >= max_ns)                                                     \
    {                                                                            \
      otel_span_set_attribute(span, "performance.violation", "above_threshold"); \
      otel_span_set_attribute(span, "performance.actual_ns", ns_per_op);         \
      otel_span_set_attribute(span, "performance.threshold_ns", max_ns);         \
      return S7T_ERROR_PERFORMANCE_VIOLATION;                                    \
    }                                                                            \
    otel_span_set_attribute(span, "performance.actual_ns", ns_per_op);           \
  } while (0)

// Error codes
typedef enum
{
  S7T_ERROR_SUCCESS = 0,
  S7T_ERROR_INVALID_INPUT = -1,
  S7T_ERROR_INVALID_OUTPUT = -2,
  S7T_ERROR_INVALID_STATE = -3,
  S7T_ERROR_PERFORMANCE_VIOLATION = -4
} S7T_ErrorCode;

// Performance measurement
static inline uint64_t get_microseconds(void)
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000000ULL + tv.tv_usec;
}

// ============================================================================
// CONTRACT-VALIDATED PATTERN IMPLEMENTATIONS
// ============================================================================

// Strategy pattern with contracts
uint32_t s7t_strategy_execute_with_contracts(S7T_StrategyContext *ctx)
{
  otel_span_t span = otel_span_start("strategy.execute.contracts");

  // Preconditions
  S7T_CONTRACT_PRE(span, ctx != NULL, "Context cannot be NULL");
  S7T_CONTRACT_PRE(span, ctx->strategy_id < S7T_STRATEGY_COUNT, "Invalid strategy ID");
  S7T_CONTRACT_PRE(span, ctx->flags < 0x1000, "Invalid flags value");

  // Invariants
  S7T_CONTRACT_INV(span, ctx->data != NULL, "Data array cannot be NULL");

  // Performance contract
  uint32_t result;
  S7T_CONTRACT_PERFORMANCE(span,
                           result = strategies[ctx->strategy_id](ctx->data),
                           10.0 // 7-tick threshold
  );

  // Postconditions
  S7T_CONTRACT_POST(span, result >= 0, "Result must be non-negative");
  S7T_CONTRACT_POST(span, result < 0xFFFFFFFF, "Result must be within valid range");

  // Success telemetry
  otel_span_set_attribute(span, "strategy.id", ctx->strategy_id);
  otel_span_set_attribute(span, "strategy.result", result);
  otel_span_set_attribute(span, "contract.status", "success");

  otel_span_end(span);
  return result;
}

// Factory pattern with contracts
S7T_Object *s7t_factory_create_with_contracts(S7T_FactoryContext *ctx)
{
  otel_span_t span = otel_span_start("factory.create.contracts");

  // Preconditions
  S7T_CONTRACT_PRE(span, ctx != NULL, "Context cannot be NULL");
  S7T_CONTRACT_PRE(span, ctx->object_type < S7T_OBJECT_TYPE_COUNT, "Invalid object type");
  S7T_CONTRACT_PRE(span, ctx->flags < 0x1000, "Invalid flags value");

  // Invariants
  S7T_CONTRACT_INV(span, ctx->data != NULL, "Data array cannot be NULL");

  // Performance contract
  S7T_Object *result;
  S7T_CONTRACT_PERFORMANCE(span,
                           result = factories[ctx->object_type](ctx->data),
                           10.0 // 7-tick threshold
  );

  // Postconditions
  S7T_CONTRACT_POST(span, result != NULL, "Factory must return valid object");
  S7T_CONTRACT_POST(span, result->object_id < 0xFFFFFFFF, "Object ID must be valid");

  // Success telemetry
  otel_span_set_attribute(span, "factory.type", ctx->object_type);
  otel_span_set_attribute(span, "factory.object_id", result->object_id);
  otel_span_set_attribute(span, "contract.status", "success");

  otel_span_end(span);
  return result;
}

// Singleton pattern with contracts
S7T_Singleton *s7t_singleton_get_with_contracts(void)
{
  otel_span_t span = otel_span_start("singleton.get.contracts");

  // Preconditions (none for singleton get)

  // Invariants
  S7T_CONTRACT_INV(span, &singleton_instance != NULL, "Singleton instance must exist");

  // Performance contract
  S7T_Singleton *result;
  S7T_CONTRACT_PERFORMANCE(span,
                           result = &singleton_instance,
                           5.0 // Singleton should be very fast
  );

  // Postconditions
  S7T_CONTRACT_POST(span, result != NULL, "Singleton must return valid instance");
  S7T_CONTRACT_POST(span, result->data < 0xFFFFFFFF, "Singleton data must be valid");

  // Success telemetry
  otel_span_set_attribute(span, "singleton.data", result->data);
  otel_span_set_attribute(span, "singleton.flags", result->flags);
  otel_span_set_attribute(span, "contract.status", "success");

  otel_span_end(span);
  return result;
}

// Decorator pattern with contracts
uint32_t s7t_decorator_operation_with_contracts(S7T_DecoratorContext *ctx)
{
  otel_span_t span = otel_span_start("decorator.operation.contracts");

  // Preconditions
  S7T_CONTRACT_PRE(span, ctx != NULL, "Context cannot be NULL");
  S7T_CONTRACT_PRE(span, ctx->base_object_id < S7T_OBJECT_COUNT, "Invalid base object ID");
  S7T_CONTRACT_PRE(span, ctx->decorations < 0x100, "Invalid decorations value");

  // Invariants
  S7T_CONTRACT_INV(span, ctx->data != NULL, "Data array cannot be NULL");

  // Performance contract
  uint32_t result;
  S7T_CONTRACT_PERFORMANCE(span,
                           result = decorators[ctx->decorations](ctx->data),
                           10.0 // 7-tick threshold
  );

  // Postconditions
  S7T_CONTRACT_POST(span, result >= 0, "Result must be non-negative");
  S7T_CONTRACT_POST(span, (ctx->decorations & S7T_DECORATION_VALIDATION) == 0 || result < 1000,
                    "Validation decoration must ensure result < 1000");

  // Success telemetry
  otel_span_set_attribute(span, "decorator.base_id", ctx->base_object_id);
  otel_span_set_attribute(span, "decorator.decorations", ctx->decorations);
  otel_span_set_attribute(span, "decorator.result", result);
  otel_span_set_attribute(span, "contract.status", "success");

  otel_span_end(span);
  return result;
}

// ============================================================================
// CONTRACT TESTING FRAMEWORK
// ============================================================================

// Test contract violations
void test_contract_violations(void)
{
  printf("Testing contract violations...\n");

  // Test NULL context
  uint32_t result = s7t_strategy_execute_with_contracts(NULL);
  assert(result == S7T_ERROR_INVALID_INPUT);

  // Test invalid strategy ID
  S7T_StrategyContext invalid_ctx = {
      .strategy_id = 999, // Invalid ID
      .data = {1, 2, 3, 4},
      .flags = 0};
  result = s7t_strategy_execute_with_contracts(&invalid_ctx);
  assert(result == S7T_ERROR_INVALID_INPUT);

  // Test invalid flags
  S7T_StrategyContext invalid_flags_ctx = {
      .strategy_id = S7T_STRATEGY_FAST,
      .data = {1, 2, 3, 4},
      .flags = 0xFFFF // Invalid flags
  };
  result = s7t_strategy_execute_with_contracts(&invalid_flags_ctx);
  assert(result == S7T_ERROR_INVALID_INPUT);

  printf("Contract violation tests passed!\n");
}

// Test contract compliance
void test_contract_compliance(void)
{
  printf("Testing contract compliance...\n");

  // Test valid strategy execution
  S7T_StrategyContext valid_ctx = {
      .strategy_id = S7T_STRATEGY_FAST,
      .data = {1, 2, 3, 4},
      .flags = 0};

  uint32_t result = s7t_strategy_execute_with_contracts(&valid_ctx);
  assert(result >= 0);
  assert(result < 0xFFFFFFFF);

  // Test valid factory creation
  S7T_FactoryContext factory_ctx = {
      .object_type = S7T_OBJECT_TYPE_A,
      .data = {1, 2, 3, 4},
      .flags = 0};

  S7T_Object *obj = s7t_factory_create_with_contracts(&factory_ctx);
  assert(obj != NULL);
  assert(obj->object_id < 0xFFFFFFFF);

  // Test valid singleton access
  S7T_Singleton *singleton = s7t_singleton_get_with_contracts();
  assert(singleton != NULL);
  assert(singleton->data < 0xFFFFFFFF);

  printf("Contract compliance tests passed!\n");
}

// Test performance contracts
void test_performance_contracts(void)
{
  printf("Testing performance contracts...\n");

  const uint64_t iterations = 100000;

  // Test strategy performance
  S7T_StrategyContext ctx = {
      .strategy_id = S7T_STRATEGY_FAST,
      .data = {1, 2, 3, 4},
      .flags = 0};

  uint64_t start = get_microseconds();
  for (uint64_t i = 0; i < iterations; i++)
  {
    uint32_t result = s7t_strategy_execute_with_contracts(&ctx);
    assert(result >= 0);
  }
  uint64_t end = get_microseconds();

  double avg_ns = (end - start) * 1000.0 / iterations;
  printf("Strategy average performance: %.2f ns/op\n", avg_ns);
  assert(avg_ns < 10.0); // 7-tick threshold

  printf("Performance contract tests passed!\n");
}

// ============================================================================
// CONTRACT MONITORING
// ============================================================================

// Contract monitoring statistics
typedef struct
{
  uint64_t total_calls;
  uint64_t precondition_violations;
  uint64_t postcondition_violations;
  uint64_t invariant_violations;
  uint64_t performance_violations;
  uint64_t total_time_ns;
} S7T_ContractStats;

static S7T_ContractStats contract_stats = {0};

// Update contract statistics
void s7t_contract_update_stats(S7T_ErrorCode error, uint64_t time_ns)
{
  contract_stats.total_calls++;
  contract_stats.total_time_ns += time_ns;

  switch (error)
  {
  case S7T_ERROR_INVALID_INPUT:
    contract_stats.precondition_violations++;
    break;
  case S7T_ERROR_INVALID_OUTPUT:
    contract_stats.postcondition_violations++;
    break;
  case S7T_ERROR_INVALID_STATE:
    contract_stats.invariant_violations++;
    break;
  case S7T_ERROR_PERFORMANCE_VIOLATION:
    contract_stats.performance_violations++;
    break;
  default:
    break;
  }
}

// Print contract statistics
void s7t_contract_print_stats(void)
{
  printf("\n=== CONTRACT STATISTICS ===\n");
  printf("Total calls: %llu\n", (unsigned long long)contract_stats.total_calls);
  printf("Precondition violations: %llu (%.2f%%)\n",
         (unsigned long long)contract_stats.precondition_violations,
         (double)contract_stats.precondition_violations / contract_stats.total_calls * 100);
  printf("Postcondition violations: %llu (%.2f%%)\n",
         (unsigned long long)contract_stats.postcondition_violations,
         (double)contract_stats.postcondition_violations / contract_stats.total_calls * 100);
  printf("Invariant violations: %llu (%.2f%%)\n",
         (unsigned long long)contract_stats.invariant_violations,
         (double)contract_stats.invariant_violations / contract_stats.total_calls * 100);
  printf("Performance violations: %llu (%.2f%%)\n",
         (unsigned long long)contract_stats.performance_violations,
         (double)contract_stats.performance_violations / contract_stats.total_calls * 100);

  if (contract_stats.total_calls > 0)
  {
    double avg_ns = (double)contract_stats.total_time_ns / contract_stats.total_calls;
    printf("Average performance: %.2f ns/op\n", avg_ns);
  }

  printf("==========================\n\n");
}

// ============================================================================
// MAIN TEST FUNCTION
// ============================================================================

int main(void)
{
  printf("CNS Design by Contract Implementation\n");
  printf("=====================================\n\n");

  // Test contract violations
  test_contract_violations();

  // Test contract compliance
  test_contract_compliance();

  // Test performance contracts
  test_performance_contracts();

  // Print final statistics
  s7t_contract_print_stats();

  printf("All contract tests passed! ✅\n");
  return 0;
}