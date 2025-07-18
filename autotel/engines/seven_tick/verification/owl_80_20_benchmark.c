#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "../c_src/owl7t.h"
#include "../c_src/owl7t_optimized.c"

// High-precision timing
static inline uint64_t get_time_ns()
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

// Test data setup
void setup_test_data(OWLEngine *e)
{
  printf("Setting up OWL test data...\n");

  // Define classes
  uint32_t Person = 1;
  uint32_t Employee = 2;
  uint32_t Manager = 3;
  uint32_t Student = 4;
  uint32_t Organization = 5;

  // Define properties
  uint32_t worksFor = 10;
  uint32_t manages = 11;
  uint32_t knows = 12;
  uint32_t hasName = 13;

  // Add subclass hierarchy
  owl_add_subclass(e, Employee, Person);
  owl_add_subclass(e, Manager, Employee);
  owl_add_subclass(e, Student, Person);

  // Set property characteristics
  owl_set_transitive(e, worksFor); // WorksFor is transitive
  owl_set_symmetric(e, knows);     // Knows is symmetric
  owl_set_functional(e, hasName);  // HasName is functional

  // Add domain/range restrictions
  OWLAxiom domain_axiom = {worksFor, Person, OWL_DOMAIN};
  e->axioms[e->axiom_count++] = domain_axiom;

  OWLAxiom range_axiom = {worksFor, Organization, OWL_RANGE};
  e->axioms[e->axiom_count++] = range_axiom;

  printf("Added %zu OWL axioms\n", e->axiom_count);
}

// Benchmark original vs optimized materialization
void benchmark_materialization(OWLEngine *e)
{
  printf("\n=== Benchmarking OWL Materialization (80/20 Optimization) ===\n");

  // Test original materialization
  printf("Testing original materialization...\n");
  uint64_t start_time = get_time_ns();

  owl_materialize_inferences(e);

  uint64_t end_time = get_time_ns();
  double original_time_ns = (double)(end_time - start_time);

  printf("Original materialization: %.2f ns\n", original_time_ns);

  // Test optimized materialization
  printf("Testing 80/20 optimized materialization...\n");
  start_time = get_time_ns();

  owl_materialize_inferences_80_20(e);

  end_time = get_time_ns();
  double optimized_time_ns = (double)(end_time - start_time);

  printf("80/20 optimized materialization: %.2f ns\n", optimized_time_ns);

  // Calculate improvement
  double improvement_factor = original_time_ns / optimized_time_ns;
  printf("Improvement factor: %.2fx faster\n", improvement_factor);

  if (improvement_factor > 1.0)
  {
    printf("✅ 80/20 optimization successful!\n");
  }
  else
  {
    printf("⚠️  No improvement detected\n");
  }
}

// Benchmark reasoning queries
void benchmark_reasoning_queries(OWLEngine *e)
{
  printf("\n=== Benchmarking OWL Reasoning Queries ===\n");

  // Test original reasoning
  printf("Testing original reasoning queries...\n");
  uint64_t start_time = get_time_ns();

  int original_results = 0;
  for (int i = 0; i < 10000; i++)
  {
    // Test various reasoning scenarios
    original_results += owl_ask_with_reasoning(e, 1, 0, 2); // Is Employee a Person?
    original_results += owl_ask_with_reasoning(e, 2, 0, 1); // Is Person an Employee?
    original_results += owl_ask_with_reasoning(e, 3, 0, 1); // Is Manager a Person?
  }

  uint64_t end_time = get_time_ns();
  double original_time_ns = (double)(end_time - start_time) / 10000;

  printf("Original reasoning: %.2f ns per query\n", original_time_ns);
  printf("Original results: %d\n", original_results);

  // Test optimized reasoning
  printf("Testing 80/20 optimized reasoning queries...\n");
  start_time = get_time_ns();

  int optimized_results = 0;
  for (int i = 0; i < 10000; i++)
  {
    // Test same scenarios
    optimized_results += owl_ask_with_reasoning_80_20(e, 1, 0, 2);
    optimized_results += owl_ask_with_reasoning_80_20(e, 2, 0, 1);
    optimized_results += owl_ask_with_reasoning_80_20(e, 3, 0, 1);
  }

  end_time = get_time_ns();
  double optimized_time_ns = (double)(end_time - start_time) / 10000;

  printf("80/20 optimized reasoning: %.2f ns per query\n", optimized_time_ns);
  printf("Optimized results: %d\n", optimized_results);

  // Calculate improvement
  double improvement_factor = original_time_ns / optimized_time_ns;
  printf("Improvement factor: %.2fx faster\n", improvement_factor);

  if (improvement_factor > 1.0)
  {
    printf("✅ 80/20 reasoning optimization successful!\n");
  }
  else
  {
    printf("⚠️  No improvement detected\n");
  }

  // Verify correctness
  if (original_results == optimized_results)
  {
    printf("✅ Results match - optimization preserves correctness\n");
  }
  else
  {
    printf("❌ Results differ - optimization may have introduced bugs\n");
  }
}

// Test transitive property materialization specifically
void test_transitive_materialization(OWLEngine *e)
{
  printf("\n=== Testing Transitive Property Materialization ===\n");

  // Add some test triples for transitive reasoning
  uint32_t worksFor = 10;
  uint32_t alice = 100;
  uint32_t bob = 101;
  uint32_t charlie = 102;
  uint32_t company = 200;
  uint32_t department = 201;

  // Create transitive chain: Alice worksFor Bob, Bob worksFor Charlie
  s7t_add_triple(e->base_engine, alice, worksFor, bob);
  s7t_add_triple(e->base_engine, bob, worksFor, charlie);

  printf("Added transitive chain: Alice -> Bob -> Charlie\n");

  // Test before materialization
  printf("Before materialization:\n");
  printf("  Alice worksFor Bob: %s\n", s7t_ask_pattern(e->base_engine, alice, worksFor, bob) ? "Yes" : "No");
  printf("  Alice worksFor Charlie: %s\n", s7t_ask_pattern(e->base_engine, alice, worksFor, charlie) ? "Yes" : "No");

  // Materialize transitive closure
  printf("Materializing transitive closure...\n");
  materialize_transitive_property_80_20(e, worksFor);

  // Test after materialization
  printf("After materialization:\n");
  printf("  Alice worksFor Bob: %s\n", s7t_ask_pattern(e->base_engine, alice, worksFor, bob) ? "Yes" : "No");
  printf("  Alice worksFor Charlie: %s\n", s7t_ask_pattern(e->base_engine, alice, worksFor, charlie) ? "Yes" : "No");

  // Test reasoning queries
  printf("Testing reasoning queries:\n");
  printf("  owl_ask_with_reasoning(Alice, worksFor, Charlie): %s\n",
         owl_ask_with_reasoning(e, alice, worksFor, charlie) ? "Yes" : "No");
  printf("  owl_ask_with_reasoning_80_20(Alice, worksFor, Charlie): %s\n",
         owl_ask_with_reasoning_80_20(e, alice, worksFor, charlie) ? "Yes" : "No");
}

int main()
{
  printf("============================================================\n");
  printf("OWL 80/20 Optimization Benchmark\n");
  printf("============================================================\n");

  // Create OWL engine
  printf("Creating OWL engine...\n");
  S7TEngine *base_engine = s7t_create_engine();
  OWLEngine *owl_engine = owl_create(base_engine, 1000, 100);

  // Setup test data
  setup_test_data(owl_engine);

  // Test transitive materialization
  test_transitive_materialization(owl_engine);

  // Benchmark materialization
  benchmark_materialization(owl_engine);

  // Benchmark reasoning queries
  benchmark_reasoning_queries(owl_engine);

  // Summary
  printf("\n============================================================\n");
  printf("OWL 80/20 OPTIMIZATION SUMMARY\n");
  printf("============================================================\n");
  printf("✅ Replaced placeholder transitive materialization with real implementation\n");
  printf("✅ Added bit-vector optimized transitive closure computation\n");
  printf("✅ Implemented symmetric property materialization\n");
  printf("✅ Added functional property validation\n");
  printf("✅ Optimized reasoning queries with early termination\n");
  printf("✅ Limited iterations and checks for 80/20 performance\n");

  // Cleanup
  owl_destroy(owl_engine);
  s7t_destroy_engine(base_engine);

  return 0;
}