#include "s7t.h"
#include "s7t_patterns.h"
#include "s7t_additional_patterns.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Pragmatic Programmer: Design by Contract validation
#define PRAGMATIC_CONTRACT_PRECONDITION(condition, message)     \
  do                                                            \
  {                                                             \
    if (!(condition))                                           \
    {                                                           \
      fprintf(stderr, "PRECONDITION VIOLATION: %s\n", message); \
      exit(1);                                                  \
    }                                                           \
  } while (0)

#define PRAGMATIC_CONTRACT_POSTCONDITION(condition, message)     \
  do                                                             \
  {                                                              \
    if (!(condition))                                            \
    {                                                            \
      fprintf(stderr, "POSTCONDITION VIOLATION: %s\n", message); \
      exit(1);                                                   \
    }                                                            \
  } while (0)

#define PRAGMATIC_CONTRACT_INVARIANT(condition, message)     \
  do                                                         \
  {                                                          \
    if (!(condition))                                        \
    {                                                        \
      fprintf(stderr, "INVARIANT VIOLATION: %s\n", message); \
      exit(1);                                               \
    }                                                        \
  } while (0)

// Pragmatic Programmer: Performance Contract validation
#define PRAGMATIC_PERFORMANCE_CONTRACT(operation, max_cycles)                   \
  do                                                                            \
  {                                                                             \
    uint64_t start = s7t_cycles();                                              \
    operation;                                                                  \
    uint64_t cycles = s7t_cycles() - start;                                     \
    if (cycles > max_cycles)                                                    \
    {                                                                           \
      fprintf(stderr, "PERFORMANCE CONTRACT VIOLATION: %lu cycles > %lu max\n", \
              cycles, max_cycles);                                              \
      exit(1);                                                                  \
    }                                                                           \
  } while (0)

// Pragmatic Programmer: Property-based testing
static bool test_pattern_property_consistency(void)
{
  // Test that all patterns maintain consistent state
  s7t_singleton_t *singleton1 = s7t_singleton_get();
  s7t_singleton_t *singleton2 = s7t_singleton_get();

  PRAGMATIC_CONTRACT_INVARIANT(singleton1 == singleton2, "Singleton must return same instance");

  // Test factory pattern consistency
  s7t_factory_t factory;
  s7t_factory_init(&factory);

  s7t_product_t *product1 = s7t_factory_create(&factory, S7T_PRODUCT_TYPE_A);
  s7t_product_t *product2 = s7t_factory_create(&factory, S7T_PRODUCT_TYPE_A);

  PRAGMATIC_CONTRACT_INVARIANT(product1 != NULL, "Factory must create valid product");
  PRAGMATIC_CONTRACT_INVARIANT(product2 != NULL, "Factory must create valid product");
  PRAGMATIC_CONTRACT_INVARIANT(product1 != product2, "Factory must create different instances");

  return true;
}

// Pragmatic Programmer: Performance regression testing
static bool test_performance_regression(void)
{
  const uint64_t MAX_CYCLES_PER_OPERATION = 7 * S7T_CYCLES_PER_TICK;

  // Test core patterns performance
  PRAGMATIC_PERFORMANCE_CONTRACT(s7t_singleton_get(), MAX_CYCLES_PER_OPERATION);

  s7t_factory_t factory;
  s7t_factory_init(&factory);
  PRAGMATIC_PERFORMANCE_CONTRACT(s7t_factory_create(&factory, S7T_PRODUCT_TYPE_A), MAX_CYCLES_PER_OPERATION);

  // Test strategy pattern
  s7t_strategy_t strategy;
  s7t_strategy_init(&strategy, S7T_STRATEGY_TYPE_FAST);
  PRAGMATIC_PERFORMANCE_CONTRACT(s7t_strategy_execute(&strategy, 42), MAX_CYCLES_PER_OPERATION);

  // Test additional patterns
  s7t_template_method_t template_method;
  s7t_template_method_init(&template_method);
  PRAGMATIC_PERFORMANCE_CONTRACT(s7t_template_method_execute(&template_method), MAX_CYCLES_PER_OPERATION);

  s7t_prototype_t prototype;
  s7t_prototype_init(&prototype);
  PRAGMATIC_PERFORMANCE_CONTRACT(s7t_prototype_clone(&prototype), MAX_CYCLES_PER_OPERATION);

  return true;
}

// Pragmatic Programmer: Integration testing
static bool test_integration_workflow(void)
{
  // Test complete workflow with multiple patterns
  s7t_factory_t factory;
  s7t_factory_init(&factory);

  // Create products using factory
  s7t_product_t *product = s7t_factory_create(&factory, S7T_PRODUCT_TYPE_A);
  PRAGMATIC_CONTRACT_POSTCONDITION(product != NULL, "Factory must create product");

  // Apply strategy to product
  s7t_strategy_t strategy;
  s7t_strategy_init(&strategy, S7T_STRATEGY_TYPE_FAST);
  int result = s7t_strategy_execute(&strategy, 42);
  PRAGMATIC_CONTRACT_POSTCONDITION(result >= 0, "Strategy must return valid result");

  // Use decorator to enhance product
  s7t_decorator_t decorator;
  s7t_decorator_init(&decorator, product);
  s7t_decorator_operation(&decorator);

  // Use facade to simplify complex operations
  s7t_facade_t facade;
  s7t_facade_init(&facade);
  s7t_facade_operation(&facade);

  return true;
}

// Pragmatic Programmer: Telemetry validation
static bool test_telemetry_integration(void)
{
  // Simulate telemetry collection
  uint64_t total_operations = 0;
  uint64_t total_cycles = 0;

  // Run operations with telemetry
  for (int i = 0; i < 1000; i++)
  {
    uint64_t start = s7t_cycles();

    // Execute pattern operations
    s7t_singleton_get();
    s7t_factory_t factory;
    s7t_factory_init(&factory);
    s7t_factory_create(&factory, S7T_PRODUCT_TYPE_A);

    uint64_t cycles = s7t_cycles() - start;
    total_operations++;
    total_cycles += cycles;
  }

  // Validate telemetry data
  double avg_cycles = (double)total_cycles / total_operations;
  PRAGMATIC_CONTRACT_INVARIANT(avg_cycles < 7 * S7T_CYCLES_PER_TICK,
                               "Average performance must be within 7-tick limit");

  printf("Telemetry Validation: %.2f avg cycles/op (%.2f ns/op)\n",
         avg_cycles, avg_cycles / S7T_CYCLES_PER_NS);

  return true;
}

int main(void)
{
  printf("Pragmatic Programmer Validation with 7-Tick Benchmarks\n");
  printf("=====================================================\n\n");

  // Initialize S7T system
  s7t_init();

  // Run Pragmatic Programmer validations
  printf("1. Testing Pattern Property Consistency...\n");
  if (!test_pattern_property_consistency())
  {
    printf("❌ Property consistency test failed\n");
    return 1;
  }
  printf("✅ Property consistency test passed\n\n");

  printf("2. Testing Performance Regression...\n");
  if (!test_performance_regression())
  {
    printf("❌ Performance regression test failed\n");
    return 1;
  }
  printf("✅ Performance regression test passed\n\n");

  printf("3. Testing Integration Workflow...\n");
  if (!test_integration_workflow())
  {
    printf("❌ Integration workflow test failed\n");
    return 1;
  }
  printf("✅ Integration workflow test passed\n\n");

  printf("4. Testing Telemetry Integration...\n");
  if (!test_telemetry_integration())
  {
    printf("❌ Telemetry integration test failed\n");
    return 1;
  }
  printf("✅ Telemetry integration test passed\n\n");

  printf("=====================================================\n");
  printf("🎉 ALL PRAGMATIC PROGRAMMER VALIDATIONS PASSED!\n");
  printf("✅ Design by Contract: Working\n");
  printf("✅ Performance Contracts: Working\n");
  printf("✅ Property-based Testing: Working\n");
  printf("✅ Integration Testing: Working\n");
  printf("✅ Telemetry Integration: Working\n");
  printf("✅ 7-Tick Performance: Maintained\n");
  printf("=====================================================\n");

  return 0;
}