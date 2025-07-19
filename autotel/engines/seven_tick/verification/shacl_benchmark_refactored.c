#include "7t_benchmark_framework.h"
#include "../runtime/src/seven_t_runtime.h"

// SHACL benchmark context
typedef struct
{
  EngineState *engine;
  uint32_t test_node;
  uint32_t person_class;
  uint32_t employee_class;
  uint32_t manager_class;
  uint32_t name_property;
  uint32_t email_property;
  uint32_t salary_property;
  uint32_t works_for_property;
  uint32_t manages_property;
} SHACLBenchmarkContext;

// Benchmark operations
void benchmark_class_membership_check(void *context)
{
  SHACLBenchmarkContext *ctx = (SHACLBenchmarkContext *)context;
  shacl_check_class(ctx->engine, ctx->test_node, ctx->person_class);
}

void benchmark_property_existence_check(void *context)
{
  SHACLBenchmarkContext *ctx = (SHACLBenchmarkContext *)context;
  shacl_check_min_count(ctx->engine, ctx->test_node, ctx->name_property, 1);
}

void benchmark_property_cardinality_check(void *context)
{
  SHACLBenchmarkContext *ctx = (SHACLBenchmarkContext *)context;
  shacl_check_max_count(ctx->engine, ctx->test_node, ctx->name_property, 1);
}

void benchmark_min_count_validation(void *context)
{
  SHACLBenchmarkContext *ctx = (SHACLBenchmarkContext *)context;
  shacl_check_min_count(ctx->engine, ctx->test_node, ctx->email_property, 1);
}

void benchmark_max_count_validation(void *context)
{
  SHACLBenchmarkContext *ctx = (SHACLBenchmarkContext *)context;
  shacl_check_max_count(ctx->engine, ctx->test_node, ctx->works_for_property, 1);
}

void benchmark_manager_validation(void *context)
{
  SHACLBenchmarkContext *ctx = (SHACLBenchmarkContext *)context;
  // Check if node is manager and has manages property
  shacl_check_class(ctx->engine, ctx->test_node, ctx->manager_class);
  shacl_check_min_count(ctx->engine, ctx->test_node, ctx->manages_property, 1);
}

// Setup test data
SHACLBenchmarkContext *setup_shacl_test_data(void)
{
  EngineState *engine = s7t_create_engine();
  if (!engine)
  {
    printf("❌ Failed to create engine\n");
    return NULL;
  }

  // Intern strings for classes
  uint32_t person_class = s7t_intern_string(engine, "ex:Person");
  uint32_t employee_class = s7t_intern_string(engine, "ex:Employee");
  uint32_t manager_class = s7t_intern_string(engine, "ex:Manager");

  // Intern strings for properties
  uint32_t name_property = s7t_intern_string(engine, "ex:hasName");
  uint32_t email_property = s7t_intern_string(engine, "ex:hasEmail");
  uint32_t salary_property = s7t_intern_string(engine, "ex:hasSalary");
  uint32_t works_for_property = s7t_intern_string(engine, "ex:worksFor");
  uint32_t manages_property = s7t_intern_string(engine, "ex:manages");

  // Intern strings for values
  uint32_t test_node = s7t_intern_string(engine, "ex:test_node");
  uint32_t name_value = s7t_intern_string(engine, "John Doe");
  uint32_t email_value = s7t_intern_string(engine, "john@company.com");
  uint32_t salary_value = s7t_intern_string(engine, "75000");
  uint32_t company_value = s7t_intern_string(engine, "ex:company");
  uint32_t employee_value = s7t_intern_string(engine, "ex:employee1");

  // Add class hierarchy
  s7t_add_triple(engine, employee_class, 0, person_class);
  s7t_add_triple(engine, manager_class, 0, employee_class);

  // Add test node as employee with all required properties
  s7t_add_triple(engine, test_node, 0, employee_class);
  s7t_add_triple(engine, test_node, name_property, name_value);
  s7t_add_triple(engine, test_node, email_property, email_value);
  s7t_add_triple(engine, test_node, salary_property, salary_value);
  s7t_add_triple(engine, test_node, works_for_property, company_value);

  // Add manager properties
  s7t_add_triple(engine, test_node, 0, manager_class);
  s7t_add_triple(engine, test_node, manages_property, employee_value);

  SHACLBenchmarkContext *ctx = malloc(sizeof(SHACLBenchmarkContext));
  ctx->engine = engine;
  ctx->test_node = test_node;
  ctx->person_class = person_class;
  ctx->employee_class = employee_class;
  ctx->manager_class = manager_class;
  ctx->name_property = name_property;
  ctx->email_property = email_property;
  ctx->salary_property = salary_property;
  ctx->works_for_property = works_for_property;
  ctx->manages_property = manages_property;

  return ctx;
}

void cleanup_shacl_test_data(SHACLBenchmarkContext *ctx)
{
  if (ctx)
  {
    s7t_destroy_engine(ctx->engine);
    free(ctx);
  }
}

// Validate correctness
bool validate_shacl_correctness(SHACLBenchmarkContext *ctx)
{
  // Test class membership
  bool test1 = shacl_check_class(ctx->engine, ctx->test_node, ctx->person_class);
  bool test2 = shacl_check_class(ctx->engine, ctx->test_node, ctx->employee_class);
  bool test3 = shacl_check_class(ctx->engine, ctx->test_node, ctx->manager_class);

  // Test property existence
  bool test4 = shacl_check_min_count(ctx->engine, ctx->test_node, ctx->name_property, 1);
  bool test5 = shacl_check_min_count(ctx->engine, ctx->test_node, ctx->email_property, 1);
  bool test6 = shacl_check_min_count(ctx->engine, ctx->test_node, ctx->salary_property, 1);

  // Test property cardinality
  bool test7 = shacl_check_max_count(ctx->engine, ctx->test_node, ctx->name_property, 1);
  bool test8 = shacl_check_max_count(ctx->engine, ctx->test_node, ctx->works_for_property, 1);

  // Test manager-specific validation
  bool test9 = shacl_check_min_count(ctx->engine, ctx->test_node, ctx->manages_property, 1);

  return test1 && test2 && test3 && test4 && test5 && test6 && test7 && test8 && test9;
}

// Main benchmark runner
void run_shacl_benchmarks(void)
{
  printf("🔒 SHACL Engine Benchmark Suite\n");
  printf("===============================\n\n");

  // Create benchmark suite
  BenchmarkSuite *suite = create_benchmark_suite("SHACL Engine");
  if (!suite)
  {
    printf("❌ Failed to create benchmark suite\n");
    return;
  }

  // Setup test data
  SHACLBenchmarkContext *ctx = setup_shacl_test_data();
  if (!ctx)
  {
    destroy_benchmark_suite(suite);
    return;
  }

  printf("✅ Test data loaded (%zu triples)\n\n", ctx->engine->triple_count);

  // Validate correctness first
  if (!validate_shacl_correctness(ctx))
  {
    printf("❌ SHACL correctness validation failed!\n");
    cleanup_shacl_test_data(ctx);
    destroy_benchmark_suite(suite);
    return;
  }
  printf("✅ Correctness validation passed\n\n");

  // Get benchmark configuration
  BenchmarkConfig config = get_default_config();
  config.verbose = true;

  // Run individual benchmarks
  BenchmarkResult result;

  // 1. Class membership check
  result = run_benchmark("Class Membership Check", "SHACL", &config,
                         benchmark_class_membership_check, ctx);
  add_benchmark_result(suite, result);

  // 2. Property existence check
  result = run_benchmark("Property Existence Check", "SHACL", &config,
                         benchmark_property_existence_check, ctx);
  add_benchmark_result(suite, result);

  // 3. Property cardinality check
  result = run_benchmark("Property Cardinality Check", "SHACL", &config,
                         benchmark_property_cardinality_check, ctx);
  add_benchmark_result(suite, result);

  // 4. Min count validation
  result = run_benchmark("Min Count Validation", "SHACL", &config,
                         benchmark_min_count_validation, ctx);
  add_benchmark_result(suite, result);

  // 5. Max count validation
  result = run_benchmark("Max Count Validation", "SHACL", &config,
                         benchmark_max_count_validation, ctx);
  add_benchmark_result(suite, result);

  // 6. Manager validation
  result = run_benchmark("Manager Validation", "SHACL", &config,
                         benchmark_manager_validation, ctx);
  add_benchmark_result(suite, result);

  // Print results
  print_benchmark_suite(suite);

  // Export results
  export_benchmark_results_csv(suite, "shacl_benchmark_results.csv");
  export_benchmark_results_json(suite, "shacl_benchmark_results.json");

  printf("📊 Results exported to:\n");
  printf("  - shacl_benchmark_results.csv\n");
  printf("  - shacl_benchmark_results.json\n\n");

  // Cleanup
  cleanup_shacl_test_data(ctx);
  destroy_benchmark_suite(suite);

  printf("✅ SHACL Benchmark Suite Complete\n");
  printf("=================================\n");
}

int main()
{
  run_shacl_benchmarks();
  return 0;
}