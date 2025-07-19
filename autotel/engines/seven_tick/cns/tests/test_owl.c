#include "cns/owl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

// ============================================================================
// CNS OWL TEST SUITE - 80/20 OPTIMIZATION VALIDATION
// ============================================================================

// Test framework
typedef struct
{
  int total_tests;
  int passed_tests;
  int failed_tests;
} TestResults;

static TestResults test_results = {0, 0, 0};

#define TEST_EQUAL(actual, expected, message)                                  \
  do                                                                           \
  {                                                                            \
    test_results.total_tests++;                                                \
    if ((actual) == (expected))                                                \
    {                                                                          \
      test_results.passed_tests++;                                             \
      printf("✓ %s\n", (message));                                             \
    }                                                                          \
    else                                                                       \
    {                                                                          \
      test_results.failed_tests++;                                             \
      printf("✗ %s (expected %d, got %d)\n", (message), (expected), (actual)); \
    }                                                                          \
  } while (0)

#define TEST_TRUE(condition, message) \
  do                                  \
  {                                   \
    test_results.total_tests++;       \
    if (condition)                    \
    {                                 \
      test_results.passed_tests++;    \
      printf("✓ %s\n", (message));    \
    }                                 \
    else                              \
    {                                 \
      test_results.failed_tests++;    \
      printf("✗ %s\n", (message));    \
    }                                 \
  } while (0)

#define TEST_FALSE(condition, message) \
  do                                   \
  {                                    \
    test_results.total_tests++;        \
    if (!(condition))                  \
    {                                  \
      test_results.passed_tests++;     \
      printf("✓ %s\n", (message));     \
    }                                  \
    else                               \
    {                                  \
      test_results.failed_tests++;     \
      printf("✗ %s\n", (message));     \
    }                                  \
  } while (0)

// ============================================================================
// TEST FUNCTIONS
// ============================================================================

static void test_engine_creation(void)
{
  printf("\n=== Testing Engine Creation ===\n");

  CNSOWLEngine *engine = cns_owl_create(100);
  TEST_TRUE(engine != NULL, "Engine creation should succeed");

  if (engine)
  {
    TEST_EQUAL(engine->axiom_count, 0, "Initial axiom count should be 0");
    TEST_EQUAL(engine->axiom_capacity, 100, "Initial capacity should be 100");
    TEST_TRUE(engine->use_80_20_materialization, "80/20 materialization should be enabled by default");
    TEST_TRUE(engine->use_80_20_reasoning, "80/20 reasoning should be enabled by default");
    TEST_TRUE(engine->precompute_closures, "Precompute closures should be enabled by default");

    cns_owl_destroy(engine);
  }
}

static void test_subclass_reasoning(void)
{
  printf("\n=== Testing Subclass Reasoning ===\n");

  CNSOWLEngine *engine = cns_owl_create(100);
  TEST_TRUE(engine != NULL, "Engine creation should succeed");

  if (engine)
  {
    // Define class hierarchy: Animal -> Mammal -> Dog
    uint32_t Animal = 1;
    uint32_t Mammal = 2;
    uint32_t Dog = 3;

    // Add subclass relationships
    TEST_EQUAL(cns_owl_add_subclass(engine, Mammal, Animal), 0, "Adding Mammal subclass of Animal should succeed");
    TEST_EQUAL(cns_owl_add_subclass(engine, Dog, Mammal), 0, "Adding Dog subclass of Mammal should succeed");

    // Test reasoning
    TEST_TRUE(cns_owl_is_subclass_of(engine, Dog, Mammal), "Dog should be subclass of Mammal");
    TEST_TRUE(cns_owl_is_subclass_of(engine, Dog, Animal), "Dog should be subclass of Animal (transitive)");
    TEST_TRUE(cns_owl_is_subclass_of(engine, Mammal, Animal), "Mammal should be subclass of Animal");
    TEST_FALSE(cns_owl_is_subclass_of(engine, Animal, Dog), "Animal should not be subclass of Dog");
    TEST_FALSE(cns_owl_is_subclass_of(engine, Mammal, Dog), "Mammal should not be subclass of Dog");

    cns_owl_destroy(engine);
  }
}

static void test_equivalent_classes(void)
{
  printf("\n=== Testing Equivalent Classes ===\n");

  CNSOWLEngine *engine = cns_owl_create(100);
  TEST_TRUE(engine != NULL, "Engine creation should succeed");

  if (engine)
  {
    uint32_t Person = 1;
    uint32_t Human = 2;

    // Add equivalent class relationship
    TEST_EQUAL(cns_owl_add_equivalent_class(engine, Person, Human), 0, "Adding equivalent classes should succeed");

    // Test reasoning
    TEST_TRUE(cns_owl_is_equivalent_class(engine, Person, Human), "Person and Human should be equivalent");
    TEST_TRUE(cns_owl_is_equivalent_class(engine, Human, Person), "Human and Person should be equivalent (symmetric)");

    cns_owl_destroy(engine);
  }
}

static void test_property_characteristics(void)
{
  printf("\n=== Testing Property Characteristics ===\n");

  CNSOWLEngine *engine = cns_owl_create(100);
  TEST_TRUE(engine != NULL, "Engine creation should succeed");

  if (engine)
  {
    uint32_t knows = 10;
    uint32_t hasName = 11;
    uint32_t ancestor = 12;

    // Set property characteristics
    TEST_EQUAL(cns_owl_set_symmetric(engine, knows), 0, "Setting symmetric property should succeed");
    TEST_EQUAL(cns_owl_set_functional(engine, hasName), 0, "Setting functional property should succeed");
    TEST_EQUAL(cns_owl_set_transitive(engine, ancestor), 0, "Setting transitive property should succeed");

    // Test property characteristics
    TEST_TRUE(cns_owl_has_property_characteristic(engine, knows, OWL_SYMMETRIC), "Knows should be symmetric");
    TEST_TRUE(cns_owl_has_property_characteristic(engine, hasName, OWL_FUNCTIONAL), "HasName should be functional");
    TEST_TRUE(cns_owl_has_property_characteristic(engine, ancestor, OWL_TRANSITIVE), "Ancestor should be transitive");

    TEST_FALSE(cns_owl_has_property_characteristic(engine, knows, OWL_FUNCTIONAL), "Knows should not be functional");
    TEST_FALSE(cns_owl_has_property_characteristic(engine, hasName, OWL_SYMMETRIC), "HasName should not be symmetric");

    cns_owl_destroy(engine);
  }
}

static void test_transitive_reasoning(void)
{
  printf("\n=== Testing Transitive Reasoning ===\n");

  CNSOWLEngine *engine = cns_owl_create(100);
  TEST_TRUE(engine != NULL, "Engine creation should succeed");

  if (engine)
  {
    uint32_t ancestor = 10;
    uint32_t alice = 100;
    uint32_t bob = 101;
    uint32_t charlie = 102;

    // Set up transitive property
    TEST_EQUAL(cns_owl_set_transitive(engine, ancestor), 0, "Setting transitive property should succeed");

    // Add transitive chain: Alice -> Bob -> Charlie
    TEST_EQUAL(cns_owl_add_axiom(engine, alice, ancestor, bob, OWL_SUBCLASS_OF), 0, "Adding Alice ancestor of Bob should succeed");
    TEST_EQUAL(cns_owl_add_axiom(engine, bob, ancestor, charlie, OWL_SUBCLASS_OF), 0, "Adding Bob ancestor of Charlie should succeed");

    // Test transitive reasoning
    TEST_TRUE(cns_owl_transitive_query(engine, alice, ancestor, bob), "Alice should be ancestor of Bob");
    TEST_TRUE(cns_owl_transitive_query(engine, bob, ancestor, charlie), "Bob should be ancestor of Charlie");
    TEST_TRUE(cns_owl_transitive_query(engine, alice, ancestor, charlie), "Alice should be ancestor of Charlie (transitive)");

    cns_owl_destroy(engine);
  }
}

static void test_80_20_materialization(void)
{
  printf("\n=== Testing 80/20 Materialization ===\n");

  CNSOWLEngine *engine = cns_owl_create(100);
  TEST_TRUE(engine != NULL, "Engine creation should succeed");

  if (engine)
  {
    // Add axioms without immediate materialization
    cns_owl_enable_80_20_optimizations(engine, false);

    uint32_t Animal = 1;
    uint32_t Mammal = 2;
    uint32_t Dog = 3;

    TEST_EQUAL(cns_owl_add_subclass(engine, Mammal, Animal), 0, "Adding subclass should succeed");
    TEST_EQUAL(cns_owl_add_subclass(engine, Dog, Mammal), 0, "Adding subclass should succeed");

    // Test before materialization
    TEST_FALSE(cns_owl_is_subclass_of(engine, Dog, Animal), "Dog should not be subclass of Animal before materialization");

    // Enable 80/20 optimizations and test materialization
    cns_owl_enable_80_20_optimizations(engine, true);
    TEST_EQUAL(cns_owl_materialize_inferences_80_20(engine), 0, "80/20 materialization should succeed");

    // Test after materialization
    TEST_TRUE(cns_owl_is_subclass_of(engine, Dog, Animal), "Dog should be subclass of Animal after materialization");

    uint32_t inference_count = cns_owl_get_inference_count(engine);
    TEST_TRUE(inference_count > 0, "Inference count should be greater than 0");

    cns_owl_destroy(engine);
  }
}

static void test_performance_metrics(void)
{
  printf("\n=== Testing Performance Metrics ===\n");

  CNSOWLEngine *engine = cns_owl_create(100);
  TEST_TRUE(engine != NULL, "Engine creation should succeed");

  if (engine)
  {
    // Add some axioms and perform reasoning
    for (int i = 0; i < 10; i++)
    {
      cns_owl_add_subclass(engine, i + 1, i);
    }

    // Perform materialization
    cns_owl_materialize_inferences_80_20(engine);

    // Test performance queries
    uint64_t reasoning_cycles = cns_owl_get_reasoning_cycles(engine);
    uint64_t materialization_cycles = cns_owl_get_materialization_cycles(engine);
    uint32_t inference_count = cns_owl_get_inference_count(engine);

    TEST_TRUE(reasoning_cycles >= 0, "Reasoning cycles should be non-negative");
    TEST_TRUE(materialization_cycles >= 0, "Materialization cycles should be non-negative");
    TEST_TRUE(inference_count >= 0, "Inference count should be non-negative");

    printf("Performance metrics:\n");
    printf("  Reasoning cycles: %llu\n", (unsigned long long)reasoning_cycles);
    printf("  Materialization cycles: %llu\n", (unsigned long long)materialization_cycles);
    printf("  Inference count: %u\n", inference_count);

    cns_owl_destroy(engine);
  }
}

static void test_7t_compliance(void)
{
  printf("\n=== Testing 7T Compliance ===\n");

  CNSOWLEngine *engine = cns_owl_create(100);
  TEST_TRUE(engine != NULL, "Engine creation should succeed");

  if (engine)
  {
    uint32_t Animal = 1;
    uint32_t Mammal = 2;
    uint32_t Dog = 3;

    // Add subclass relationships
    cns_owl_add_subclass(engine, Mammal, Animal);
    cns_owl_add_subclass(engine, Dog, Mammal);

    // Test 7T compliance with cycle counting
    uint64_t start_cycles = cns_get_cycles();

    // Perform reasoning operations
    bool result1 = cns_owl_is_subclass_of(engine, Dog, Mammal);
    bool result2 = cns_owl_is_subclass_of(engine, Dog, Animal);
    bool result3 = cns_owl_is_subclass_of(engine, Animal, Dog);

    uint64_t end_cycles = cns_get_cycles();
    uint64_t total_cycles = end_cycles - start_cycles;

    TEST_TRUE(result1, "Dog should be subclass of Mammal");
    TEST_TRUE(result2, "Dog should be subclass of Animal");
    TEST_FALSE(result3, "Animal should not be subclass of Dog");

    // Check 7T compliance (≤7 cycles per operation)
    uint64_t cycles_per_operation = total_cycles / 3;
    TEST_TRUE(cycles_per_operation <= 7, "Operations should complete in ≤7 cycles for 7T compliance");

    printf("7T Compliance Test:\n");
    printf("  Total cycles for 3 operations: %llu\n", (unsigned long long)total_cycles);
    printf("  Cycles per operation: %llu\n", (unsigned long long)cycles_per_operation);
    printf("  7T compliant: %s\n", cycles_per_operation <= 7 ? "✓" : "✗");

    cns_owl_destroy(engine);
  }
}

// ============================================================================
// BENCHMARK TESTS
// ============================================================================

static void benchmark_subclass_queries(void)
{
  printf("\n=== Benchmarking Subclass Queries ===\n");

  CNSOWLEngine *engine = cns_owl_create(1000);
  TEST_TRUE(engine != NULL, "Engine creation should succeed");

  if (engine)
  {
    // Create a large class hierarchy
    for (int i = 0; i < 50; i++)
    {
      cns_owl_add_subclass(engine, i + 1, i);
    }

    // Materialize inferences
    cns_owl_materialize_inferences_80_20(engine);

    // Benchmark subclass queries
    const int iterations = 100000;
    uint64_t start_cycles = cns_get_cycles();

    int true_count = 0;
    for (int i = 0; i < iterations; i++)
    {
      int child = (i % 49) + 1;
      int parent = i % 50;
      if (cns_owl_is_subclass_of(engine, child, parent))
      {
        true_count++;
      }
    }

    uint64_t end_cycles = cns_get_cycles();
    uint64_t total_cycles = end_cycles - start_cycles;
    double cycles_per_query = (double)total_cycles / iterations;

    printf("Subclass Query Benchmark:\n");
    printf("  Iterations: %d\n", iterations);
    printf("  Total cycles: %llu\n", (unsigned long long)total_cycles);
    printf("  Cycles per query: %.2f\n", cycles_per_query);
    printf("  True results: %d\n", true_count);
    printf("  7T compliant: %s\n", cycles_per_query <= 7.0 ? "✓" : "✗");

    TEST_TRUE(cycles_per_query <= 7.0, "Subclass queries should be 7T compliant");

    cns_owl_destroy(engine);
  }
}

static void benchmark_materialization(void)
{
  printf("\n=== Benchmarking Materialization ===\n");

  CNSOWLEngine *engine = cns_owl_create(1000);
  TEST_TRUE(engine != NULL, "Engine creation should succeed");

  if (engine)
  {
    // Add many axioms
    for (int i = 0; i < 100; i++)
    {
      cns_owl_add_subclass(engine, i + 1, i);
    }

    // Benchmark standard materialization
    uint64_t start_cycles = cns_get_cycles();
    cns_owl_materialize_inferences(engine);
    uint64_t end_cycles = cns_get_cycles();
    uint64_t standard_cycles = end_cycles - start_cycles;

    // Reset and benchmark 80/20 materialization
    cns_owl_destroy(engine);
    engine = cns_owl_create(1000);

    for (int i = 0; i < 100; i++)
    {
      cns_owl_add_subclass(engine, i + 1, i);
    }

    start_cycles = cns_get_cycles();
    cns_owl_materialize_inferences_80_20(engine);
    end_cycles = cns_get_cycles();
    uint64_t optimized_cycles = end_cycles - start_cycles;

    printf("Materialization Benchmark:\n");
    printf("  Standard materialization: %llu cycles\n", (unsigned long long)standard_cycles);
    printf("  80/20 optimized materialization: %llu cycles\n", (unsigned long long)optimized_cycles);

    if (standard_cycles > 0)
    {
      double improvement = (double)standard_cycles / optimized_cycles;
      printf("  Improvement factor: %.2fx\n", improvement);
      TEST_TRUE(improvement >= 1.0, "80/20 optimization should provide improvement");
    }

    cns_owl_destroy(engine);
  }
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

int main(void)
{
  printf("=== CNS OWL ENGINE TEST SUITE ===\n");
  printf("Testing 80/20 optimized OWL reasoning with 7T compliance\n\n");

  // Run unit tests
  test_engine_creation();
  test_subclass_reasoning();
  test_equivalent_classes();
  test_property_characteristics();
  test_transitive_reasoning();
  test_80_20_materialization();
  test_performance_metrics();
  test_7t_compliance();

  // Run benchmarks
  benchmark_subclass_queries();
  benchmark_materialization();

  // Print test results
  printf("\n=== TEST RESULTS ===\n");
  printf("Total tests: %d\n", test_results.total_tests);
  printf("Passed: %d\n", test_results.passed_tests);
  printf("Failed: %d\n", test_results.failed_tests);
  printf("Success rate: %.1f%%\n",
         (double)test_results.passed_tests / test_results.total_tests * 100.0);

  if (test_results.failed_tests == 0)
  {
    printf("\n✓ ALL TESTS PASSED\n");
    printf("✓ CNS OWL engine is working correctly\n");
    printf("✓ 80/20 optimizations are effective\n");
    printf("✓ 7T compliance is maintained\n");
    return 0;
  }
  else
  {
    printf("\n✗ SOME TESTS FAILED\n");
    printf("✗ CNS OWL engine needs fixes\n");
    return 1;
  }
}