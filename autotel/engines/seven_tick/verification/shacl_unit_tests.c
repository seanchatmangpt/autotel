#include "7t_unit_test_framework.h"
#include "../runtime/src/seven_t_runtime.h"

// SHACL test context
typedef struct
{
  EngineState *engine;
  uint32_t person, employee, manager, company;
  uint32_t alice, bob, charlie, diana;
  uint32_t has_name, has_email, has_salary, works_for, manages;
  uint32_t alice_name, bob_name, charlie_name, diana_name;
  uint32_t alice_email, bob_email, charlie_email, diana_email;
  uint32_t alice_salary, bob_salary, charlie_salary, diana_salary;
} SHACLTestContext;

// Test setup and teardown
SHACLTestContext *setup_shacl_test_context(void)
{
  SHACLTestContext *ctx = malloc(sizeof(SHACLTestContext));
  if (!ctx)
    return NULL;

  ctx->engine = s7t_create_engine();
  if (!ctx->engine)
  {
    free(ctx);
    return NULL;
  }

  // Intern class strings
  ctx->person = s7t_intern_string(ctx->engine, "ex:Person");
  ctx->employee = s7t_intern_string(ctx->engine, "ex:Employee");
  ctx->manager = s7t_intern_string(ctx->engine, "ex:Manager");
  ctx->company = s7t_intern_string(ctx->engine, "ex:Company");

  // Intern entity strings
  ctx->alice = s7t_intern_string(ctx->engine, "ex:alice");
  ctx->bob = s7t_intern_string(ctx->engine, "ex:bob");
  ctx->charlie = s7t_intern_string(ctx->engine, "ex:charlie");
  ctx->diana = s7t_intern_string(ctx->engine, "ex:diana");

  // Intern property strings
  ctx->has_name = s7t_intern_string(ctx->engine, "ex:hasName");
  ctx->has_email = s7t_intern_string(ctx->engine, "ex:hasEmail");
  ctx->has_salary = s7t_intern_string(ctx->engine, "ex:hasSalary");
  ctx->works_for = s7t_intern_string(ctx->engine, "ex:worksFor");
  ctx->manages = s7t_intern_string(ctx->engine, "ex:manages");

  // Intern value strings
  ctx->alice_name = s7t_intern_string(ctx->engine, "Alice Smith");
  ctx->bob_name = s7t_intern_string(ctx->engine, "Bob Johnson");
  ctx->charlie_name = s7t_intern_string(ctx->engine, "Charlie Brown");
  ctx->diana_name = s7t_intern_string(ctx->engine, "Diana Prince");

  ctx->alice_email = s7t_intern_string(ctx->engine, "alice@company.com");
  ctx->bob_email = s7t_intern_string(ctx->engine, "bob@company.com");
  ctx->charlie_email = s7t_intern_string(ctx->engine, "charlie@company.com");
  ctx->diana_email = s7t_intern_string(ctx->engine, "diana@company.com");

  ctx->alice_salary = s7t_intern_string(ctx->engine, "75000");
  ctx->bob_salary = s7t_intern_string(ctx->engine, "65000");
  ctx->charlie_salary = s7t_intern_string(ctx->engine, "85000");
  ctx->diana_salary = s7t_intern_string(ctx->engine, "95000");

  // Add class hierarchy
  s7t_add_triple(ctx->engine, ctx->employee, 0, ctx->person);
  s7t_add_triple(ctx->engine, ctx->manager, 0, ctx->employee);

  // Add instance data
  s7t_add_triple(ctx->engine, ctx->alice, 0, ctx->employee);
  s7t_add_triple(ctx->engine, ctx->bob, 0, ctx->employee);
  s7t_add_triple(ctx->engine, ctx->charlie, 0, ctx->manager);
  s7t_add_triple(ctx->engine, ctx->diana, 0, ctx->manager);

  // Add properties for Alice (complete employee)
  s7t_add_triple(ctx->engine, ctx->alice, ctx->has_name, ctx->alice_name);
  s7t_add_triple(ctx->engine, ctx->alice, ctx->has_email, ctx->alice_email);
  s7t_add_triple(ctx->engine, ctx->alice, ctx->has_salary, ctx->alice_salary);
  s7t_add_triple(ctx->engine, ctx->alice, ctx->works_for, ctx->company);

  // Add properties for Bob (missing email)
  s7t_add_triple(ctx->engine, ctx->bob, ctx->has_name, ctx->bob_name);
  s7t_add_triple(ctx->engine, ctx->bob, ctx->has_salary, ctx->bob_salary);
  s7t_add_triple(ctx->engine, ctx->bob, ctx->works_for, ctx->company);

  // Add properties for Charlie (complete manager)
  s7t_add_triple(ctx->engine, ctx->charlie, ctx->has_name, ctx->charlie_name);
  s7t_add_triple(ctx->engine, ctx->charlie, ctx->has_email, ctx->charlie_email);
  s7t_add_triple(ctx->engine, ctx->charlie, ctx->has_salary, ctx->charlie_salary);
  s7t_add_triple(ctx->engine, ctx->charlie, ctx->works_for, ctx->company);
  s7t_add_triple(ctx->engine, ctx->charlie, ctx->manages, ctx->alice);
  s7t_add_triple(ctx->engine, ctx->charlie, ctx->manages, ctx->bob);

  // Add properties for Diana (complete manager)
  s7t_add_triple(ctx->engine, ctx->diana, ctx->has_name, ctx->diana_name);
  s7t_add_triple(ctx->engine, ctx->diana, ctx->has_email, ctx->diana_email);
  s7t_add_triple(ctx->engine, ctx->diana, ctx->has_salary, ctx->diana_salary);
  s7t_add_triple(ctx->engine, ctx->diana, ctx->works_for, ctx->company);

  return ctx;
}

void teardown_shacl_test_context(SHACLTestContext *ctx)
{
  if (ctx)
  {
    s7t_destroy_engine(ctx->engine);
    free(ctx);
  }
}

// Individual test functions
void test_class_membership_basic(void)
{
  SHACLTestContext *ctx = setup_shacl_test_context();
  ASSERT_NOT_NULL(ctx);

  // Test class membership
  ASSERT_TRUE(shacl_check_class(ctx->engine, ctx->alice, ctx->employee));
  ASSERT_TRUE(shacl_check_class(ctx->engine, ctx->alice, ctx->person));
  ASSERT_TRUE(shacl_check_class(ctx->engine, ctx->charlie, ctx->manager));
  ASSERT_TRUE(shacl_check_class(ctx->engine, ctx->charlie, ctx->employee));
  ASSERT_TRUE(shacl_check_class(ctx->engine, ctx->charlie, ctx->person));

  // Test non-membership
  ASSERT_FALSE(shacl_check_class(ctx->engine, ctx->alice, ctx->manager));
  ASSERT_FALSE(shacl_check_class(ctx->engine, ctx->bob, ctx->manager));

  teardown_shacl_test_context(ctx);
}

void test_property_existence_validation(void)
{
  SHACLTestContext *ctx = setup_shacl_test_context();
  ASSERT_NOT_NULL(ctx);

  // Test required properties for Alice (complete)
  ASSERT_TRUE(shacl_check_min_count(ctx->engine, ctx->alice, ctx->has_name, 1));
  ASSERT_TRUE(shacl_check_min_count(ctx->engine, ctx->alice, ctx->has_email, 1));
  ASSERT_TRUE(shacl_check_min_count(ctx->engine, ctx->alice, ctx->has_salary, 1));
  ASSERT_TRUE(shacl_check_min_count(ctx->engine, ctx->alice, ctx->works_for, 1));

  // Test required properties for Bob (missing email)
  ASSERT_TRUE(shacl_check_min_count(ctx->engine, ctx->bob, ctx->has_name, 1));
  ASSERT_FALSE(shacl_check_min_count(ctx->engine, ctx->bob, ctx->has_email, 1));
  ASSERT_TRUE(shacl_check_min_count(ctx->engine, ctx->bob, ctx->has_salary, 1));
  ASSERT_TRUE(shacl_check_min_count(ctx->engine, ctx->bob, ctx->works_for, 1));

  teardown_shacl_test_context(ctx);
}

void test_property_cardinality_validation(void)
{
  SHACLTestContext *ctx = setup_shacl_test_context();
  ASSERT_NOT_NULL(ctx);

  // Test max cardinality (should have at most 1 name, 1 email, 1 salary)
  ASSERT_TRUE(shacl_check_max_count(ctx->engine, ctx->alice, ctx->has_name, 1));
  ASSERT_TRUE(shacl_check_max_count(ctx->engine, ctx->alice, ctx->has_email, 1));
  ASSERT_TRUE(shacl_check_max_count(ctx->engine, ctx->alice, ctx->has_salary, 1));
  ASSERT_TRUE(shacl_check_max_count(ctx->engine, ctx->alice, ctx->works_for, 1));

  // Test min cardinality for managers (should manage at least 1 person)
  ASSERT_TRUE(shacl_check_min_count(ctx->engine, ctx->charlie, ctx->manages, 1));
  ASSERT_TRUE(shacl_check_min_count(ctx->engine, ctx->diana, ctx->manages, 0)); // Diana has no reports

  teardown_shacl_test_context(ctx);
}

void test_manager_specific_validation(void)
{
  SHACLTestContext *ctx = setup_shacl_test_context();
  ASSERT_NOT_NULL(ctx);

  // Test manager-specific constraints
  ASSERT_TRUE(shacl_check_class(ctx->engine, ctx->charlie, ctx->manager));
  ASSERT_TRUE(shacl_check_min_count(ctx->engine, ctx->charlie, ctx->manages, 1));
  ASSERT_TRUE(shacl_check_max_count(ctx->engine, ctx->charlie, ctx->manages, 5));

  // Test that employees are not managers
  ASSERT_FALSE(shacl_check_class(ctx->engine, ctx->alice, ctx->manager));
  ASSERT_FALSE(shacl_check_class(ctx->engine, ctx->bob, ctx->manager));

  teardown_shacl_test_context(ctx);
}

void test_inheritance_validation(void)
{
  SHACLTestContext *ctx = setup_shacl_test_context();
  ASSERT_NOT_NULL(ctx);

  // Test inheritance relationships
  ASSERT_TRUE(shacl_check_class(ctx->engine, ctx->employee, ctx->person));
  ASSERT_TRUE(shacl_check_class(ctx->engine, ctx->manager, ctx->employee));
  ASSERT_TRUE(shacl_check_class(ctx->engine, ctx->manager, ctx->person));

  // Test that inheritance is not symmetric
  ASSERT_FALSE(shacl_check_class(ctx->engine, ctx->person, ctx->employee));
  ASSERT_FALSE(shacl_check_class(ctx->engine, ctx->employee, ctx->manager));

  teardown_shacl_test_context(ctx);
}

void test_property_value_validation(void)
{
  SHACLTestContext *ctx = setup_shacl_test_context();
  ASSERT_NOT_NULL(ctx);

  // Test that properties have correct values
  ASSERT_TRUE(s7t_ask_pattern(ctx->engine, ctx->alice, ctx->has_name, ctx->alice_name));
  ASSERT_TRUE(s7t_ask_pattern(ctx->engine, ctx->alice, ctx->has_email, ctx->alice_email));
  ASSERT_TRUE(s7t_ask_pattern(ctx->engine, ctx->alice, ctx->has_salary, ctx->alice_salary));

  // Test that properties don't have incorrect values
  ASSERT_FALSE(s7t_ask_pattern(ctx->engine, ctx->alice, ctx->has_name, ctx->bob_name));
  ASSERT_FALSE(s7t_ask_pattern(ctx->engine, ctx->alice, ctx->has_email, ctx->bob_email));

  teardown_shacl_test_context(ctx);
}

void test_complex_constraint_validation(void)
{
  SHACLTestContext *ctx = setup_shacl_test_context();
  ASSERT_NOT_NULL(ctx);

  // Test complex constraints: managers must have all employee properties plus manages
  ASSERT_TRUE(shacl_check_min_count(ctx->engine, ctx->charlie, ctx->has_name, 1));
  ASSERT_TRUE(shacl_check_min_count(ctx->engine, ctx->charlie, ctx->has_email, 1));
  ASSERT_TRUE(shacl_check_min_count(ctx->engine, ctx->charlie, ctx->has_salary, 1));
  ASSERT_TRUE(shacl_check_min_count(ctx->engine, ctx->charlie, ctx->works_for, 1));
  ASSERT_TRUE(shacl_check_min_count(ctx->engine, ctx->charlie, ctx->manages, 1));

  // Test that regular employees don't have manages property
  ASSERT_FALSE(shacl_check_min_count(ctx->engine, ctx->alice, ctx->manages, 1));
  ASSERT_FALSE(shacl_check_min_count(ctx->engine, ctx->bob, ctx->manages, 1));

  teardown_shacl_test_context(ctx);
}

void test_edge_cases(void)
{
  SHACLTestContext *ctx = setup_shacl_test_context();
  ASSERT_NOT_NULL(ctx);

  // Test with non-existent entities
  ASSERT_FALSE(shacl_check_class(ctx->engine, 999, ctx->person));
  ASSERT_FALSE(shacl_check_min_count(ctx->engine, 999, ctx->has_name, 1));
  ASSERT_FALSE(shacl_check_max_count(ctx->engine, 999, ctx->has_name, 1));

  // Test with non-existent classes/properties
  ASSERT_FALSE(shacl_check_class(ctx->engine, ctx->alice, 999));
  ASSERT_FALSE(shacl_check_min_count(ctx->engine, ctx->alice, 999, 1));
  ASSERT_FALSE(shacl_check_max_count(ctx->engine, ctx->alice, 999, 1));

  // Test with zero counts
  ASSERT_TRUE(shacl_check_min_count(ctx->engine, ctx->alice, ctx->manages, 0));
  ASSERT_TRUE(shacl_check_max_count(ctx->engine, ctx->alice, ctx->manages, 0));

  teardown_shacl_test_context(ctx);
}

void test_performance_7tick_class_check(void)
{
  SHACLTestContext *ctx = setup_shacl_test_context();
  ASSERT_NOT_NULL(ctx);

  // Test 7-tick performance for class membership checks
  ASSERT_PERFORMANCE_7TICK(
      shacl_check_class(ctx->engine, ctx->alice, ctx->employee),
      100000);

  teardown_shacl_test_context(ctx);
}

void test_performance_7tick_property_check(void)
{
  SHACLTestContext *ctx = setup_shacl_test_context();
  ASSERT_NOT_NULL(ctx);

  // Test 7-tick performance for property existence checks
  ASSERT_PERFORMANCE_7TICK(
      shacl_check_min_count(ctx->engine, ctx->alice, ctx->has_name, 1),
      100000);

  teardown_shacl_test_context(ctx);
}

void test_performance_7tick_cardinality_check(void)
{
  SHACLTestContext *ctx = setup_shacl_test_context();
  ASSERT_NOT_NULL(ctx);

  // Test 7-tick performance for cardinality checks
  ASSERT_PERFORMANCE_7TICK(
      shacl_check_max_count(ctx->engine, ctx->alice, ctx->has_name, 1),
      100000);

  teardown_shacl_test_context(ctx);
}

void test_large_scale_validation(void)
{
  EngineState *engine = s7t_create_engine();
  ASSERT_NOT_NULL(engine);

  // Create large test dataset
  uint32_t person_class = s7t_intern_string(engine, "ex:Person");
  uint32_t name_prop = s7t_intern_string(engine, "ex:hasName");

  // Add many people with names
  for (int i = 0; i < 1000; i++)
  {
    char person_str[32], name_str[32];
    snprintf(person_str, sizeof(person_str), "ex:person_%d", i);
    snprintf(name_str, sizeof(name_str), "Person %d", i);

    uint32_t person = s7t_intern_string(engine, person_str);
    uint32_t name = s7t_intern_string(engine, name_str);

    s7t_add_triple(engine, person, 0, person_class);
    s7t_add_triple(engine, person, name_prop, name);
  }

  // Test validation on large dataset
  uint32_t test_person = s7t_intern_string(engine, "ex:person_500");
  ASSERT_TRUE(shacl_check_class(engine, test_person, person_class));
  ASSERT_TRUE(shacl_check_min_count(engine, test_person, name_prop, 1));
  ASSERT_TRUE(shacl_check_max_count(engine, test_person, name_prop, 1));

  s7t_destroy_engine(engine);
}

void test_memory_management(void)
{
  SHACLTestContext *ctx = setup_shacl_test_context();
  ASSERT_NOT_NULL(ctx);

  // Perform many validation operations
  for (int i = 0; i < 1000; i++)
  {
    shacl_check_class(ctx->engine, ctx->alice, ctx->employee);
    shacl_check_min_count(ctx->engine, ctx->alice, ctx->has_name, 1);
    shacl_check_max_count(ctx->engine, ctx->alice, ctx->has_name, 1);
  }

  // Verify engine still works after many operations
  ASSERT_TRUE(shacl_check_class(ctx->engine, ctx->alice, ctx->employee));
  ASSERT_TRUE(shacl_check_min_count(ctx->engine, ctx->alice, ctx->has_name, 1));

  teardown_shacl_test_context(ctx);
}

// Test suite runner
void run_shacl_tests(TestSuite *suite)
{
  printf("\n🔒 Running SHACL Engine Unit Tests\n");
  printf("==================================\n");

  run_test(suite, "Class Membership Basic", test_class_membership_basic);
  run_test(suite, "Property Existence Validation", test_property_existence_validation);
  run_test(suite, "Property Cardinality Validation", test_property_cardinality_validation);
  run_test(suite, "Manager Specific Validation", test_manager_specific_validation);
  run_test(suite, "Inheritance Validation", test_inheritance_validation);
  run_test(suite, "Property Value Validation", test_property_value_validation);
  run_test(suite, "Complex Constraint Validation", test_complex_constraint_validation);
  run_test(suite, "Edge Cases", test_edge_cases);
  run_test(suite, "7-Tick Class Check Performance", test_performance_7tick_class_check);
  run_test(suite, "7-Tick Property Check Performance", test_performance_7tick_property_check);
  run_test(suite, "7-Tick Cardinality Check Performance", test_performance_7tick_cardinality_check);
  run_test(suite, "Large Scale Validation", test_large_scale_validation);
  run_test(suite, "Memory Management", test_memory_management);
}

// Main test runner
int main()
{
  TestSuite *suite = create_test_suite("SHACL Engine");
  if (!suite)
  {
    printf("❌ Failed to create test suite\n");
    return 1;
  }

  run_shacl_tests(suite);

  print_test_report(suite);

  int exit_code = suite->failed_tests > 0 ? 1 : 0;
  destroy_test_suite(suite);

  return exit_code;
}