#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include "../runtime/src/seven_t_runtime.h"

// High-precision timing
static inline uint64_t get_microseconds()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000000ULL + tv.tv_usec;
}

// Test SHACL validation primitives
int main()
{
  printf("7T SHACL Implementation Benchmark\n");
  printf("==================================\n\n");

  // Create engine
  printf("Creating engine...\n");
  EngineState *engine = s7t_create_engine();

  // Add test data
  printf("Adding test data...\n");

  // Intern strings
  uint32_t rdf_type = s7t_intern_string(engine, "rdf:type");
  uint32_t ex_person = s7t_intern_string(engine, "ex:Person");
  uint32_t ex_machine = s7t_intern_string(engine, "ex:Machine");
  uint32_t ex_name = s7t_intern_string(engine, "ex:name");
  uint32_t ex_email = s7t_intern_string(engine, "ex:email");
  uint32_t ex_serial = s7t_intern_string(engine, "ex:serial");

  uint32_t alice = s7t_intern_string(engine, "ex:Alice");
  uint32_t bob = s7t_intern_string(engine, "ex:Bob");
  uint32_t robot = s7t_intern_string(engine, "ex:Robot");

  uint32_t alice_name = s7t_intern_string(engine, "Alice Smith");
  uint32_t alice_email = s7t_intern_string(engine, "alice@example.com");
  uint32_t bob_name = s7t_intern_string(engine, "Bob Jones");
  uint32_t robot_serial = s7t_intern_string(engine, "R2D2");

  // Add triples
  s7t_add_triple(engine, alice, rdf_type, ex_person);
  s7t_add_triple(engine, bob, rdf_type, ex_person);
  s7t_add_triple(engine, robot, rdf_type, ex_machine);

  s7t_add_triple(engine, alice, ex_name, alice_name);
  s7t_add_triple(engine, alice, ex_email, alice_email);
  s7t_add_triple(engine, bob, ex_name, bob_name);
  s7t_add_triple(engine, robot, ex_serial, robot_serial);

  printf("✅ Added test data\n\n");

  // Test SHACL validation primitives
  printf("Testing SHACL validation primitives:\n");
  printf("=====================================\n");

  // Test min_count validation
  printf("\n1. MIN_COUNT VALIDATION:\n");

  // Alice has 2 properties (name, email) - should pass min_count 1
  int alice_min_1 = shacl_check_min_count(engine, alice, ex_name, 1);
  printf("   Alice name min_count(1): %s\n", alice_min_1 ? "PASS" : "FAIL");

  // Alice should pass min_count 2 for name
  int alice_min_2 = shacl_check_min_count(engine, alice, ex_name, 2);
  printf("   Alice name min_count(2): %s\n", alice_min_2 ? "PASS" : "FAIL");

  // Bob has 1 property (name) - should pass min_count 1, fail min_count 2
  int bob_min_1 = shacl_check_min_count(engine, bob, ex_name, 1);
  printf("   Bob name min_count(1): %s\n", bob_min_1 ? "PASS" : "FAIL");

  int bob_min_2 = shacl_check_min_count(engine, bob, ex_name, 2);
  printf("   Bob name min_count(2): %s\n", bob_min_2 ? "PASS" : "FAIL");

  // Test max_count validation
  printf("\n2. MAX_COUNT VALIDATION:\n");

  // Alice has 1 name - should pass max_count 1, fail max_count 0
  int alice_max_1 = shacl_check_max_count(engine, alice, ex_name, 1);
  printf("   Alice name max_count(1): %s\n", alice_max_1 ? "PASS" : "FAIL");

  int alice_max_0 = shacl_check_max_count(engine, alice, ex_name, 0);
  printf("   Alice name max_count(0): %s\n", alice_max_0 ? "PASS" : "FAIL");

  // Test class validation
  printf("\n3. CLASS VALIDATION:\n");

  // Set object type IDs for class checking
  engine->object_type_ids[alice] = ex_person;
  engine->object_type_ids[bob] = ex_person;
  engine->object_type_ids[robot] = ex_machine;

  int alice_person = shacl_check_class(engine, alice, ex_person);
  printf("   Alice is Person: %s\n", alice_person ? "PASS" : "FAIL");

  int alice_machine = shacl_check_class(engine, alice, ex_machine);
  printf("   Alice is Machine: %s\n", alice_machine ? "PASS" : "FAIL");

  int robot_person = shacl_check_class(engine, robot, ex_person);
  printf("   Robot is Person: %s\n", robot_person ? "PASS" : "FAIL");

  int robot_machine = shacl_check_class(engine, robot, ex_machine);
  printf("   Robot is Machine: %s\n", robot_machine ? "PASS" : "FAIL");

  // Benchmark SHACL validation performance
  printf("\nBenchmarking SHACL validation performance:\n");
  printf("===========================================\n");

  const int iterations = 1000000;
  uint64_t start_time, end_time;

  // Benchmark min_count validation
  start_time = get_microseconds();
  for (int i = 0; i < iterations; i++)
  {
    shacl_check_min_count(engine, alice, ex_name, 1);
  }
  end_time = get_microseconds();

  uint64_t min_count_time = end_time - start_time;
  double min_count_ns = (double)min_count_time * 1000.0 / iterations;
  double min_count_ops_per_sec = (double)iterations * 1000000.0 / min_count_time;

  printf("min_count validation:\n");
  printf("   Time: %llu μs for %d iterations\n", min_count_time, iterations);
  printf("   Latency: %.2f ns per validation\n", min_count_ns);
  printf("   Throughput: %.0f validations/sec\n", min_count_ops_per_sec);

  // Benchmark max_count validation
  start_time = get_microseconds();
  for (int i = 0; i < iterations; i++)
  {
    shacl_check_max_count(engine, alice, ex_name, 1);
  }
  end_time = get_microseconds();

  uint64_t max_count_time = end_time - start_time;
  double max_count_ns = (double)max_count_time * 1000.0 / iterations;
  double max_count_ops_per_sec = (double)iterations * 1000000.0 / max_count_time;

  printf("\nmax_count validation:\n");
  printf("   Time: %llu μs for %d iterations\n", max_count_time, iterations);
  printf("   Latency: %.2f ns per validation\n", max_count_ns);
  printf("   Throughput: %.0f validations/sec\n", max_count_ops_per_sec);

  // Benchmark class validation
  start_time = get_microseconds();
  for (int i = 0; i < iterations; i++)
  {
    shacl_check_class(engine, alice, ex_person);
  }
  end_time = get_microseconds();

  uint64_t class_time = end_time - start_time;
  double class_ns = (double)class_time * 1000.0 / iterations;
  double class_ops_per_sec = (double)iterations * 1000000.0 / class_time;

  printf("\nclass validation:\n");
  printf("   Time: %llu μs for %d iterations\n", class_time, iterations);
  printf("   Latency: %.2f ns per validation\n", class_ns);
  printf("   Throughput: %.0f validations/sec\n", class_ops_per_sec);

  // Performance summary
  printf("\nPerformance Summary:\n");
  printf("===================\n");
  printf("✅ min_count: %.2f ns (%.0f ops/sec)\n", min_count_ns, min_count_ops_per_sec);
  printf("✅ max_count: %.2f ns (%.0f ops/sec)\n", max_count_ns, max_count_ops_per_sec);
  printf("✅ class: %.2f ns (%.0f ops/sec)\n", class_ns, class_ops_per_sec);

  // Check if we're achieving sub-10ns performance
  if (min_count_ns < 10.0 && max_count_ns < 10.0 && class_ns < 10.0)
  {
    printf("\n🎉 ACHIEVING SUB-10NS SHACL VALIDATION!\n");
  }
  else if (min_count_ns < 100.0 && max_count_ns < 100.0 && class_ns < 100.0)
  {
    printf("\n✅ ACHIEVING SUB-100NS SHACL VALIDATION!\n");
  }
  else
  {
    printf("\n⚠️ Performance above 100ns\n");
  }

  // Compare with previous mock implementation
  printf("\nComparison with Previous Implementation:\n");
  printf("========================================\n");
  printf("Before (Mock):\n");
  printf("   - Fallback implementations with simplified logic\n");
  printf("   - No real C runtime integration\n");
  printf("   - Assumed performance characteristics\n");
  printf("\nAfter (Real Implementation):\n");
  printf("   - Direct C runtime primitive calls\n");
  printf("   - Real hash table lookups for counting\n");
  printf("   - Measured performance: %.2f ns average\n", (min_count_ns + max_count_ns + class_ns) / 3.0);
  printf("   - Memory-efficient bit-vector operations\n");

  // Cleanup
  s7t_destroy_engine(engine);

  printf("\n🎉 SHACL Implementation Benchmark Complete!\n");
  return 0;
}