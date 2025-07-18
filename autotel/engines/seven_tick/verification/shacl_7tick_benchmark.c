#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include <sys/time.h>
#include <x86intrin.h> // For __rdtsc() on x86
#include "../runtime/src/seven_t_runtime.h"

// High-precision timing using CPU cycles
static inline uint64_t get_cycles()
{
  return __rdtsc();
}

// High-precision timing for microsecond measurements
static inline uint64_t get_microseconds()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000000ULL + tv.tv_usec;
}

// 7-Tick SHACL benchmark - testing sub-10ns performance
int main()
{
  printf("7T SHACL 7-Tick Performance Benchmark\n");
  printf("=====================================\n\n");

  // Step 1: Create engine
  printf("Creating engine...\n");
  EngineState *engine = s7t_create_engine();

  // Step 2: Add test data for SHACL validation
  printf("Adding test data for SHACL validation...\n");

  // Define predicates and classes
  uint32_t pred_type = s7t_intern_string(engine, "<http://www.w3.org/1999/02/22-rdf-syntax-ns#type>");
  uint32_t pred_name = s7t_intern_string(engine, "<http://example.org/name>");
  uint32_t pred_email = s7t_intern_string(engine, "<http://example.org/email>");
  uint32_t pred_phone = s7t_intern_string(engine, "<http://example.org/phone>");
  uint32_t pred_age = s7t_intern_string(engine, "<http://example.org/age>");

  uint32_t class_Person = s7t_intern_string(engine, "<http://example.org/Person>");
  uint32_t class_Employee = s7t_intern_string(engine, "<http://example.org/Employee>");

  // Add test data - 1000 people with various properties
  for (int i = 0; i < 1000; i++)
  {
    char subject[64], name[64], email[64], phone[64], age[16];

    sprintf(subject, "<http://example.org/person_%d>", i);
    sprintf(name, "\"Person %d\"", i);
    sprintf(email, "\"person%d@example.com\"", i);
    sprintf(phone, "\"+1-555-%04d\"", i);
    sprintf(age, "\"%d\"", 20 + (i % 60));

    uint32_t s = s7t_intern_string(engine, subject);
    uint32_t n = s7t_intern_string(engine, name);
    uint32_t e = s7t_intern_string(engine, email);
    uint32_t p = s7t_intern_string(engine, phone);
    uint32_t a = s7t_intern_string(engine, age);

    // All people have type Person
    s7t_add_triple(engine, s, pred_type, class_Person);

    // All people have name
    s7t_add_triple(engine, s, pred_name, n);

    // 80% have email
    if (i % 5 != 0)
    {
      s7t_add_triple(engine, s, pred_email, e);
    }

    // 60% have phone
    if (i % 5 < 3)
    {
      s7t_add_triple(engine, s, pred_phone, p);
    }

    // 40% have age
    if (i % 5 < 2)
    {
      s7t_add_triple(engine, s, pred_age, a);
    }

    // 20% are employees
    if (i % 5 == 0)
    {
      s7t_add_triple(engine, s, pred_type, class_Employee);
    }
  }

  printf("Added %d people with various properties\n", 1000);
  printf("  - All have name\n");
  printf("  - 80%% have email\n");
  printf("  - 60%% have phone\n");
  printf("  - 40%% have age\n");
  printf("  - 20%% are employees\n\n");

  // Step 3: Benchmark SHACL primitives for 7-tick performance
  printf("Benchmarking SHACL primitives for 7-tick performance...\n");

  // Warm up the cache
  for (int i = 0; i < 1000; i++)
  {
    shacl_check_min_count(engine, i, pred_name, 1);
  }

  // Benchmark 1: Property existence checking (shacl_check_min_count)
  printf("\n1. Property Existence Check (shacl_check_min_count)\n");

  uint64_t start_cycles = get_cycles();
  uint64_t start_time = get_microseconds();

  int validations = 0;
  for (int i = 0; i < 100000; i++)
  {
    uint32_t subject_id = i % 1000;
    uint32_t predicate_id = (i % 4 == 0) ? pred_name : (i % 4 == 1) ? pred_email
                                                   : (i % 4 == 2)   ? pred_phone
                                                                    : pred_age;

    int result = shacl_check_min_count(engine, subject_id, predicate_id, 1);
    validations++;

    // Prevent compiler optimization
    if (result)
    {
      // Do nothing, just prevent optimization
    }
  }

  uint64_t end_cycles = get_cycles();
  uint64_t end_time = get_microseconds();

  uint64_t elapsed_cycles = end_cycles - start_cycles;
  uint64_t elapsed_time = end_time - start_time;

  double cycles_per_check = (double)elapsed_cycles / validations;
  double ns_per_check = (elapsed_time * 1000.0) / validations;

  printf("  Total checks: %d\n", validations);
  printf("  Total cycles: %lu\n", elapsed_cycles);
  printf("  Total time: %.3f ms\n", elapsed_time / 1000.0);
  printf("  Cycles per check: %.1f\n", cycles_per_check);
  printf("  Nanoseconds per check: %.1f\n", ns_per_check);

  if (cycles_per_check <= 7)
  {
    printf("  🎉 ACHIEVING 7-TICK PERFORMANCE! (%.1f cycles)\n", cycles_per_check);
  }
  else if (cycles_per_check <= 70)
  {
    printf("  ✅ Achieving sub-70-cycle performance! (%.1f cycles)\n", cycles_per_check);
  }
  else
  {
    printf("  ⚠️  Performance above 70 cycles (%.1f cycles)\n", cycles_per_check);
  }

  if (ns_per_check < 10)
  {
    printf("  ✅ Achieving sub-10ns performance! (%.1f ns)\n", ns_per_check);
  }
  else if (ns_per_check < 100)
  {
    printf("  ✅ Achieving sub-100ns performance! (%.1f ns)\n", ns_per_check);
  }
  else
  {
    printf("  ⚠️  Performance above 100ns (%.1f ns)\n", ns_per_check);
  }

  // Benchmark 2: Property counting (s7t_get_objects)
  printf("\n2. Property Value Counting (s7t_get_objects)\n");

  start_cycles = get_cycles();
  start_time = get_microseconds();

  int total_count = 0;
  for (int i = 0; i < 10000; i++)
  {
    uint32_t subject_id = i % 1000;
    uint32_t predicate_id = (i % 4 == 0) ? pred_name : (i % 4 == 1) ? pred_email
                                                   : (i % 4 == 2)   ? pred_phone
                                                                    : pred_age;

    size_t count = 0;
    uint32_t *objects = s7t_get_objects(engine, predicate_id, subject_id, &count);
    total_count += count;
  }

  end_cycles = get_cycles();
  end_time = get_microseconds();

  elapsed_cycles = end_cycles - start_cycles;
  elapsed_time = end_time - start_time;

  double cycles_per_count = (double)elapsed_cycles / 10000;
  double ns_per_count = (elapsed_time * 1000.0) / 10000;

  printf("  Total counts: 10,000\n");
  printf("  Total properties found: %d\n", total_count);
  printf("  Total cycles: %lu\n", elapsed_cycles);
  printf("  Total time: %.3f ms\n", elapsed_time / 1000.0);
  printf("  Cycles per count: %.1f\n", cycles_per_count);
  printf("  Nanoseconds per count: %.1f\n", ns_per_count);

  if (cycles_per_count <= 7)
  {
    printf("  🎉 ACHIEVING 7-TICK PERFORMANCE! (%.1f cycles)\n", cycles_per_count);
  }
  else if (cycles_per_count <= 70)
  {
    printf("  ✅ Achieving sub-70-cycle performance! (%.1f cycles)\n", cycles_per_count);
  }
  else
  {
    printf("  ⚠️  Performance above 70 cycles (%.1f cycles)\n", cycles_per_count);
  }

  if (ns_per_count < 10)
  {
    printf("  ✅ Achieving sub-10ns performance! (%.1f ns)\n", ns_per_count);
  }
  else if (ns_per_count < 100)
  {
    printf("  ✅ Achieving sub-100ns performance! (%.1f ns)\n", ns_per_count);
  }
  else
  {
    printf("  ⚠️  Performance above 100ns (%.1f ns)\n", ns_per_count);
  }

  // Benchmark 3: Class checking (shacl_check_class)
  printf("\n3. Class Membership Check (shacl_check_class)\n");

  start_cycles = get_cycles();
  start_time = get_microseconds();

  int class_checks = 0;
  for (int i = 0; i < 100000; i++)
  {
    uint32_t subject_id = i % 1000;
    uint32_t class_id = (i % 2 == 0) ? class_Person : class_Employee;

    int result = shacl_check_class(engine, subject_id, class_id);
    class_checks++;

    // Prevent compiler optimization
    if (result)
    {
      // Do nothing, just prevent optimization
    }
  }

  end_cycles = get_cycles();
  end_time = get_microseconds();

  elapsed_cycles = end_cycles - start_cycles;
  elapsed_time = end_time - start_time;

  double cycles_per_class_check = (double)elapsed_cycles / class_checks;
  double ns_per_class_check = (elapsed_time * 1000.0) / class_checks;

  printf("  Total checks: %d\n", class_checks);
  printf("  Total cycles: %lu\n", elapsed_cycles);
  printf("  Total time: %.3f ms\n", elapsed_time / 1000.0);
  printf("  Cycles per check: %.1f\n", cycles_per_class_check);
  printf("  Nanoseconds per check: %.1f\n", ns_per_class_check);

  if (cycles_per_class_check <= 7)
  {
    printf("  🎉 ACHIEVING 7-TICK PERFORMANCE! (%.1f cycles)\n", cycles_per_class_check);
  }
  else if (cycles_per_class_check <= 70)
  {
    printf("  ✅ Achieving sub-70-cycle performance! (%.1f cycles)\n", cycles_per_class_check);
  }
  else
  {
    printf("  ⚠️  Performance above 70 cycles (%.1f cycles)\n", cycles_per_class_check);
  }

  if (ns_per_class_check < 10)
  {
    printf("  ✅ Achieving sub-10ns performance! (%.1f ns)\n", ns_per_class_check);
  }
  else if (ns_per_class_check < 100)
  {
    printf("  ✅ Achieving sub-100ns performance! (%.1f ns)\n", ns_per_class_check);
  }
  else
  {
    printf("  ⚠️  Performance above 100ns (%.1f ns)\n", ns_per_class_check);
  }

  // Benchmark 4: Full SHACL validation simulation
  printf("\n4. Full SHACL Validation Simulation\n");

  start_cycles = get_cycles();
  start_time = get_microseconds();

  int valid_count = 0;
  for (int i = 0; i < 1000; i++)
  {
    uint32_t subject_id = i;

    // Simulate Person shape validation: must have name and email
    int has_name = shacl_check_min_count(engine, subject_id, pred_name, 1);
    int has_email = shacl_check_min_count(engine, subject_id, pred_email, 1);

    // Simulate Employee shape validation: must have name, email, and phone
    int has_phone = shacl_check_min_count(engine, subject_id, pred_phone, 1);
    int is_employee = shacl_check_class(engine, subject_id, class_Employee);

    // Validation logic
    int person_valid = has_name && has_email;
    int employee_valid = is_employee ? (has_name && has_email && has_phone) : 1;

    if (person_valid && employee_valid)
    {
      valid_count++;
    }
  }

  end_cycles = get_cycles();
  end_time = get_microseconds();

  elapsed_cycles = end_cycles - start_cycles;
  elapsed_time = end_time - start_time;

  double cycles_per_validation = (double)elapsed_cycles / 1000;
  double ns_per_validation = (elapsed_time * 1000.0) / 1000;

  printf("  Total validations: 1,000\n");
  printf("  Valid entities: %d\n", valid_count);
  printf("  Total cycles: %lu\n", elapsed_cycles);
  printf("  Total time: %.3f ms\n", elapsed_time / 1000.0);
  printf("  Cycles per validation: %.1f\n", cycles_per_validation);
  printf("  Nanoseconds per validation: %.1f\n", ns_per_validation);

  if (cycles_per_validation <= 7)
  {
    printf("  🎉 ACHIEVING 7-TICK PERFORMANCE! (%.1f cycles)\n", cycles_per_validation);
  }
  else if (cycles_per_validation <= 70)
  {
    printf("  ✅ Achieving sub-70-cycle performance! (%.1f cycles)\n", cycles_per_validation);
  }
  else
  {
    printf("  ⚠️  Performance above 70 cycles (%.1f cycles)\n", cycles_per_validation);
  }

  if (ns_per_validation < 10)
  {
    printf("  ✅ Achieving sub-10ns performance! (%.1f ns)\n", ns_per_validation);
  }
  else if (ns_per_validation < 100)
  {
    printf("  ✅ Achieving sub-100ns performance! (%.1f ns)\n", ns_per_validation);
  }
  else
  {
    printf("  ⚠️  Performance above 100ns (%.1f ns)\n", ns_per_validation);
  }

  // Summary
  printf("\n7T SHACL Performance Summary");
  printf("Property existence check: %.1f cycles (%.1f ns)", cycles_per_check, ns_per_check);
  printf("Property value counting:  %.1f cycles (%.1f ns)", cycles_per_count, ns_per_count);
  printf("Class membership check:   %.1f cycles (%.1f ns)", cycles_per_class_check, ns_per_class_check);
  printf("Full validation:          %.1f cycles (%.1f ns)", cycles_per_validation, ns_per_validation);

  // Performance assessment
  if (cycles_per_check <= 7 && cycles_per_count <= 7 &&
      cycles_per_class_check <= 7 && cycles_per_validation <= 7)
  {
    printf("\n🎉 ACHIEVING 7-TICK PERFORMANCE ACROSS ALL OPERATIONS!");
    printf("   All SHACL operations under 7 CPU cycles!");
  }
  else if (cycles_per_check <= 70 && cycles_per_count <= 70 &&
           cycles_per_class_check <= 70 && cycles_per_validation <= 70)
  {
    printf("\n✅ Achieving sub-70-cycle performance!");
  }
  else
  {
    printf("\n⚠️  Some operations above 70 cycles");
  }

  if (ns_per_check < 10 && ns_per_count < 10 &&
      ns_per_class_check < 10 && ns_per_validation < 10)
  {
    printf("\n🎉 ACHIEVING SUB-10NS PERFORMANCE ACROSS ALL OPERATIONS!");
    printf("   All SHACL operations under 10 nanoseconds!");
  }
  else if (ns_per_check < 100 && ns_per_count < 100 &&
           ns_per_class_check < 100 && ns_per_validation < 100)
  {
    printf("\n✅ Achieving sub-100ns performance!");
  }
  else
  {
    printf("\n⚠️  Some operations above 100ns");
  }

  // Cleanup
  s7t_destroy_engine(engine);

  printf("\n7T SHACL 7-tick benchmark completed!\n");
  return 0;
}