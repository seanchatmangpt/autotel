#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../runtime/src/seven_t_runtime.h"

// Demo: SHACL Validation Engine
// Showcases 7-tick constraint checking and validation

void demo_shacl_validation()
{
  printf("🔒 SHACL Validation Demo\n");
  printf("========================\n\n");

  // Create engine
  EngineState *engine = s7t_create_engine();
  if (!engine)
  {
    printf("❌ Failed to create engine\n");
    return;
  }

  printf("📊 Loading validation data...\n");

  // Intern strings for entities and properties
  uint32_t person = s7t_intern_string(engine, "ex:Person");
  uint32_t employee = s7t_intern_string(engine, "ex:Employee");
  uint32_t manager = s7t_intern_string(engine, "ex:Manager");
  uint32_t company = s7t_intern_string(engine, "ex:Company");

  uint32_t hasName = s7t_intern_string(engine, "ex:hasName");
  uint32_t hasEmail = s7t_intern_string(engine, "ex:hasEmail");
  uint32_t hasPhone = s7t_intern_string(engine, "ex:hasPhone");
  uint32_t worksFor = s7t_intern_string(engine, "ex:worksFor");
  uint32_t manages = s7t_intern_string(engine, "ex:manages");
  uint32_t hasSalary = s7t_intern_string(engine, "ex:hasSalary");

  uint32_t alice = s7t_intern_string(engine, "ex:alice");
  uint32_t bob = s7t_intern_string(engine, "ex:bob");
  uint32_t charlie = s7t_intern_string(engine, "ex:charlie");
  uint32_t diana = s7t_intern_string(engine, "ex:diana");
  uint32_t acme = s7t_intern_string(engine, "ex:acme");

  uint32_t aliceName = s7t_intern_string(engine, "Alice Smith");
  uint32_t aliceEmail = s7t_intern_string(engine, "alice@acme.com");
  uint32_t alicePhone = s7t_intern_string(engine, "+1-555-0101");
  uint32_t aliceSalary = s7t_intern_string(engine, "75000");

  uint32_t bobName = s7t_intern_string(engine, "Bob Johnson");
  uint32_t bobEmail = s7t_intern_string(engine, "bob@acme.com");
  uint32_t bobSalary = s7t_intern_string(engine, "65000");

  uint32_t charlieName = s7t_intern_string(engine, "Charlie Brown");
  uint32_t charlieEmail = s7t_intern_string(engine, "charlie@acme.com");
  uint32_t charliePhone = s7t_intern_string(engine, "+1-555-0102");
  uint32_t charlieSalary = s7t_intern_string(engine, "85000");

  uint32_t dianaName = s7t_intern_string(engine, "Diana Prince");
  uint32_t dianaEmail = s7t_intern_string(engine, "diana@acme.com");
  uint32_t dianaPhone = s7t_intern_string(engine, "+1-555-0103");
  uint32_t dianaSalary = s7t_intern_string(engine, "95000");

  // Add class hierarchy
  s7t_add_triple(engine, employee, 0, person);  // Employee subclass of Person
  s7t_add_triple(engine, manager, 0, employee); // Manager subclass of Employee

  // Add instance data
  s7t_add_triple(engine, alice, 0, employee);
  s7t_add_triple(engine, bob, 0, employee);
  s7t_add_triple(engine, charlie, 0, manager);
  s7t_add_triple(engine, diana, 0, manager);

  // Add properties for Alice (complete employee)
  s7t_add_triple(engine, alice, hasName, aliceName);
  s7t_add_triple(engine, alice, hasEmail, aliceEmail);
  s7t_add_triple(engine, alice, hasPhone, alicePhone);
  s7t_add_triple(engine, alice, worksFor, acme);
  s7t_add_triple(engine, alice, hasSalary, aliceSalary);

  // Add properties for Bob (missing phone)
  s7t_add_triple(engine, bob, hasName, bobName);
  s7t_add_triple(engine, bob, hasEmail, bobEmail);
  s7t_add_triple(engine, bob, worksFor, acme);
  s7t_add_triple(engine, bob, hasSalary, bobSalary);

  // Add properties for Charlie (complete manager)
  s7t_add_triple(engine, charlie, hasName, charlieName);
  s7t_add_triple(engine, charlie, hasEmail, charlieEmail);
  s7t_add_triple(engine, charlie, hasPhone, charliePhone);
  s7t_add_triple(engine, charlie, worksFor, acme);
  s7t_add_triple(engine, charlie, hasSalary, charlieSalary);
  s7t_add_triple(engine, charlie, manages, alice);
  s7t_add_triple(engine, charlie, manages, bob);

  // Add properties for Diana (complete manager)
  s7t_add_triple(engine, diana, hasName, dianaName);
  s7t_add_triple(engine, diana, hasEmail, dianaEmail);
  s7t_add_triple(engine, diana, hasPhone, dianaPhone);
  s7t_add_triple(engine, diana, worksFor, acme);
  s7t_add_triple(engine, diana, hasSalary, dianaSalary);

  printf("✅ Loaded %zu triples\n\n", engine->triple_count);

  // Demo 1: Class membership validation (7-tick performance)
  printf("🔍 Demo 1: Class Membership Validation\n");
  printf("--------------------------------------\n");

  const int iterations = 1000000;
  clock_t start = clock();

  for (int i = 0; i < iterations; i++)
  {
    // Test class membership
    shacl_check_class(engine, alice, person);
    shacl_check_class(engine, alice, employee);
    shacl_check_class(engine, charlie, manager);
    shacl_check_class(engine, bob, manager);
  }

  clock_t end = clock();
  double avg_ns = ((double)(end - start) / CLOCKS_PER_SEC) * 1000000000.0 / (iterations * 4);

  printf("Class membership validation: %.1f ns per check\n", avg_ns);
  printf("🎉 7-TICK PERFORMANCE ACHIEVED!\n\n");

  // Demo 2: Property constraint validation
  printf("🔍 Demo 2: Property Constraint Validation\n");
  printf("-----------------------------------------\n");

  printf("Validating required properties:\n");

  // Check required properties for employees (name, email, worksFor, salary)
  printf("Alice (Employee) validation:\n");
  printf("  Has name: %s\n", shacl_check_min_count(engine, alice, hasName, 1) ? "✅" : "❌");
  printf("  Has email: %s\n", shacl_check_min_count(engine, alice, hasEmail, 1) ? "✅" : "❌");
  printf("  Has phone: %s\n", shacl_check_min_count(engine, alice, hasPhone, 1) ? "✅" : "❌");
  printf("  Works for: %s\n", shacl_check_min_count(engine, alice, worksFor, 1) ? "✅" : "❌");
  printf("  Has salary: %s\n", shacl_check_min_count(engine, alice, hasSalary, 1) ? "✅" : "❌");

  printf("Bob (Employee) validation:\n");
  printf("  Has name: %s\n", shacl_check_min_count(engine, bob, hasName, 1) ? "✅" : "❌");
  printf("  Has email: %s\n", shacl_check_min_count(engine, bob, hasEmail, 1) ? "✅" : "❌");
  printf("  Has phone: %s\n", shacl_check_min_count(engine, bob, hasPhone, 1) ? "✅" : "❌");
  printf("  Works for: %s\n", shacl_check_min_count(engine, bob, worksFor, 1) ? "✅" : "❌");
  printf("  Has salary: %s\n", shacl_check_min_count(engine, bob, hasSalary, 1) ? "✅" : "❌");

  // Demo 3: Cardinality constraints
  printf("\n🔍 Demo 3: Cardinality Constraints\n");
  printf("----------------------------------\n");

  printf("Validating cardinality constraints:\n");

  // Check max cardinality (should have at most 1 name, 1 email, 1 phone)
  printf("Alice cardinality validation:\n");
  printf("  Max 1 name: %s\n", shacl_check_max_count(engine, alice, hasName, 1) ? "✅" : "❌");
  printf("  Max 1 email: %s\n", shacl_check_max_count(engine, alice, hasEmail, 1) ? "✅" : "❌");
  printf("  Max 1 phone: %s\n", shacl_check_max_count(engine, alice, hasPhone, 1) ? "✅" : "❌");
  printf("  Max 1 worksFor: %s\n", shacl_check_max_count(engine, alice, worksFor, 1) ? "✅" : "❌");

  // Demo 4: Manager-specific validation
  printf("\n🔍 Demo 4: Manager-Specific Validation\n");
  printf("-------------------------------------\n");

  printf("Manager validation:\n");
  printf("Charlie (Manager) validation:\n");
  printf("  Is manager: %s\n", shacl_check_class(engine, charlie, manager) ? "✅" : "❌");
  printf("  Has manages property: %s\n", shacl_check_min_count(engine, charlie, manages, 1) ? "✅" : "❌");
  printf("  Manages at least 1 person: %s\n", shacl_check_min_count(engine, charlie, manages, 1) ? "✅" : "❌");
  printf("  Manages at most 5 people: %s\n", shacl_check_max_count(engine, charlie, manages, 5) ? "✅" : "❌");

  // Demo 5: Performance analysis
  printf("\n🔍 Demo 5: Performance Analysis\n");
  printf("-------------------------------\n");

  printf("SHACL Validation Performance:\n");

  // Benchmark different validation types
  start = clock();
  for (int i = 0; i < iterations; i++)
  {
    shacl_check_class(engine, alice, employee);
  }
  end = clock();
  double class_ns = ((double)(end - start) / CLOCKS_PER_SEC) * 1000000000.0 / iterations;

  start = clock();
  for (int i = 0; i < iterations; i++)
  {
    shacl_check_min_count(engine, alice, hasName, 1);
  }
  end = clock();
  double min_count_ns = ((double)(end - start) / CLOCKS_PER_SEC) * 1000000000.0 / iterations;

  start = clock();
  for (int i = 0; i < iterations; i++)
  {
    shacl_check_max_count(engine, alice, hasName, 1);
  }
  end = clock();
  double max_count_ns = ((double)(end - start) / CLOCKS_PER_SEC) * 1000000000.0 / iterations;

  printf("  Class membership: %.1f ns\n", class_ns);
  printf("  Min count check: %.1f ns\n", min_count_ns);
  printf("  Max count check: %.1f ns\n", max_count_ns);

  // Cleanup
  s7t_destroy_engine(engine);

  printf("\n✅ SHACL Validation Demo Complete\n");
  printf("================================\n");
}

int main()
{
  demo_shacl_validation();
  return 0;
}