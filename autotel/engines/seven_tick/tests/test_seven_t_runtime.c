#include "7t_unit_test_framework.h"
#include "../runtime/src/seven_t_runtime.h"
#include <string.h>

// Test suite registration
TEST_SUITE_BEGIN(seven_t_runtime)

// Test data structures
typedef struct
{
  EngineState *engine;
  uint32_t test_subject;
  uint32_t test_predicate;
  uint32_t test_object;
} RuntimeTestData;

// Setup and teardown functions
static void setup_runtime_test(TestContext *context)
{
  RuntimeTestData *data = malloc(sizeof(RuntimeTestData));
  ASSERT_NOT_NULL(data);

  data->engine = s7t_create_engine();
  ASSERT_NOT_NULL(data->engine);

  // Create test triples
  data->test_subject = s7t_intern_string(data->engine, "test_subject");
  data->test_predicate = s7t_intern_string(data->engine, "test_predicate");
  data->test_object = s7t_intern_string(data->engine, "test_object");

  context->test_data = data;
}

static void teardown_runtime_test(TestContext *context)
{
  RuntimeTestData *data = (RuntimeTestData *)context->test_data;
  if (data)
  {
    if (data->engine)
    {
      s7t_destroy_engine(data->engine);
    }
    free(data);
  }
}

// ============================================================================
// ENGINE CREATION AND DESTRUCTION TESTS
// ============================================================================

TEST_CASE(engine_creation, "Test engine creation and basic initialization")
static void test_engine_creation(TestContext *context)
{
  EngineState *engine = s7t_create_engine();
  ASSERT_NOT_NULL(engine);

  // Check initial state
  ASSERT_EQUAL(0, engine->triple_count);
  ASSERT_EQUAL(0, engine->subject_count);
  ASSERT_EQUAL(0, engine->predicate_count);
  ASSERT_EQUAL(0, engine->object_count);
  ASSERT_NOT_NULL(engine->triples);
  ASSERT_NOT_NULL(engine->subject_map);
  ASSERT_NOT_NULL(engine->predicate_map);
  ASSERT_NOT_NULL(engine->object_map);

  s7t_destroy_engine(engine);
}

TEST_CASE(engine_destruction, "Test engine destruction and cleanup")
static void test_engine_destruction(TestContext *context)
{
  EngineState *engine = s7t_create_engine();
  ASSERT_NOT_NULL(engine);

  // Add some data
  uint32_t s = s7t_intern_string(engine, "subject");
  uint32_t p = s7t_intern_string(engine, "predicate");
  uint32_t o = s7t_intern_string(engine, "object");
  s7t_add_triple(engine, s, p, o);

  // Destroy should not crash
  s7t_destroy_engine(engine);

  // Test should pass if we get here
  ASSERT_TRUE(true);
}

// ============================================================================
// STRING INTERNING TESTS
// ============================================================================

TEST_CASE(string_interning_basic, "Test basic string interning functionality")
static void test_string_interning_basic(TestContext *context)
{
  EngineState *engine = s7t_create_engine();
  ASSERT_NOT_NULL(engine);

  // Test basic interning
  uint32_t id1 = s7t_intern_string(engine, "test_string");
  uint32_t id2 = s7t_intern_string(engine, "test_string");

  // Same string should get same ID
  ASSERT_EQUAL(id1, id2);

  // Different strings should get different IDs
  uint32_t id3 = s7t_intern_string(engine, "different_string");
  ASSERT_NOT_EQUAL(id1, id3);

  s7t_destroy_engine(engine);
}

TEST_CASE(string_interning_performance, "Test string interning performance")
static void test_string_interning_performance(TestContext *context)
{
  EngineState *engine = s7t_create_engine();
  ASSERT_NOT_NULL(engine);

  // Performance test: intern many strings
  ASSERT_PERFORMANCE({
        for (int i = 0; i < 1000; i++) {
            char buf[64];
            snprintf(buf, sizeof(buf), "string_%d", i);
            s7t_intern_string(engine, buf);
        } }, 100000); // 100K cycles for 1000 strings

  s7t_destroy_engine(engine);
}

TEST_CASE(string_interning_memory, "Test string interning memory usage")
static void test_string_interning_memory(TestContext *context)
{
  EngineState *engine = s7t_create_engine();
  ASSERT_NOT_NULL(engine);

  // Memory test: intern many strings
  ASSERT_MEMORY_USAGE({
        for (int i = 0; i < 1000; i++) {
            char buf[64];
            snprintf(buf, sizeof(buf), "string_%d", i);
            s7t_intern_string(engine, buf);
        } }, 1024 * 1024); // 1MB limit

  s7t_destroy_engine(engine);
}

// ============================================================================
// TRIPLE OPERATIONS TESTS
// ============================================================================

TEST_CASE(triple_addition_basic, "Test basic triple addition")
static void test_triple_addition_basic(TestContext *context)
{
  RuntimeTestData *data = (RuntimeTestData *)context->test_data;
  ASSERT_NOT_NULL(data);

  size_t initial_count = data->engine->triple_count;

  // Add a triple
  s7t_add_triple(data->engine, data->test_subject, data->test_predicate, data->test_object);

  // Check that count increased
  ASSERT_EQUAL(initial_count + 1, data->engine->triple_count);
}

TEST_CASE(triple_addition_duplicate, "Test duplicate triple addition")
static void test_triple_addition_duplicate(TestContext *context)
{
  RuntimeTestData *data = (RuntimeTestData *)context->test_data;
  ASSERT_NOT_NULL(data);

  size_t initial_count = data->engine->triple_count;

  // Add same triple twice
  s7t_add_triple(data->engine, data->test_subject, data->test_predicate, data->test_object);
  s7t_add_triple(data->engine, data->test_subject, data->test_predicate, data->test_object);

  // Check that count increased by 2 (duplicates are allowed)
  ASSERT_EQUAL(initial_count + 2, data->engine->triple_count);
}

TEST_CASE(triple_addition_performance, "Test triple addition performance")
static void test_triple_addition_performance(TestContext *context)
{
  RuntimeTestData *data = (RuntimeTestData *)context->test_data;
  ASSERT_NOT_NULL(data);

  // Performance test: add many triples
  ASSERT_PERFORMANCE({
        for (int i = 0; i < 1000; i++) {
            uint32_t s = s7t_intern_string(data->engine, "subject");
            uint32_t p = s7t_intern_string(data->engine, "predicate");
            uint32_t o = s7t_intern_string(data->engine, "object");
            s7t_add_triple(data->engine, s, p, o);
        } }, 1000000); // 1M cycles for 1000 triples
}

TEST_CASE(triple_addition_memory, "Test triple addition memory usage")
static void test_triple_addition_memory(TestContext *context)
{
  RuntimeTestData *data = (RuntimeTestData *)context->test_data;
  ASSERT_NOT_NULL(data);

  // Memory test: add many triples
  ASSERT_MEMORY_USAGE({
        for (int i = 0; i < 1000; i++) {
            uint32_t s = s7t_intern_string(data->engine, "subject");
            uint32_t p = s7t_intern_string(data->engine, "predicate");
            uint32_t o = s7t_intern_string(data->engine, "object");
            s7t_add_triple(data->engine, s, p, o);
        } }, 1024 * 1024); // 1MB limit
}

// ============================================================================
// PATTERN MATCHING TESTS
// ============================================================================

TEST_CASE(pattern_matching_basic, "Test basic pattern matching")
static void test_pattern_matching_basic(TestContext *context)
{
  RuntimeTestData *data = (RuntimeTestData *)context->test_data;
  ASSERT_NOT_NULL(data);

  // Add a triple
  s7t_add_triple(data->engine, data->test_subject, data->test_predicate, data->test_object);

  // Test pattern matching
  int result = s7t_ask_pattern(data->engine, data->test_subject, data->test_predicate, data->test_object);
  ASSERT_EQUAL(1, result); // Should find the triple

  // Test non-existent pattern
  uint32_t non_existent = s7t_intern_string(data->engine, "non_existent");
  result = s7t_ask_pattern(data->engine, non_existent, data->test_predicate, data->test_object);
  ASSERT_EQUAL(0, result); // Should not find anything
}

TEST_CASE(pattern_matching_performance, "Test pattern matching performance")
static void test_pattern_matching_performance(TestContext *context)
{
  RuntimeTestData *data = (RuntimeTestData *)context->test_data;
  ASSERT_NOT_NULL(data);

  // Add some triples
  for (int i = 0; i < 100; i++)
  {
    uint32_t s = s7t_intern_string(data->engine, "subject");
    uint32_t p = s7t_intern_string(data->engine, "predicate");
    uint32_t o = s7t_intern_string(data->engine, "object");
    s7t_add_triple(data->engine, s, p, o);
  }

  // Performance test: pattern matching
  ASSERT_PERFORMANCE({
        for (int i = 0; i < 1000; i++) {
            s7t_ask_pattern(data->engine, data->test_subject, data->test_predicate, data->test_object);
        } }, 100000); // 100K cycles for 1000 queries
}

TEST_CASE(pattern_matching_latency, "Test pattern matching latency")
static void test_pattern_matching_latency(TestContext *context)
{
  RuntimeTestData *data = (RuntimeTestData *)context->test_data;
  ASSERT_NOT_NULL(data);

  // Add a triple
  s7t_add_triple(data->engine, data->test_subject, data->test_predicate, data->test_object);

  // Latency test: single pattern match
  ASSERT_LATENCY({ s7t_ask_pattern(data->engine, data->test_subject, data->test_predicate, data->test_object); }, 1000); // 1 microsecond limit
}

// ============================================================================
// MATERIALIZATION TESTS
// ============================================================================

TEST_CASE(materialization_basic, "Test basic materialization")
static void test_materialization_basic(TestContext *context)
{
  RuntimeTestData *data = (RuntimeTestData *)context->test_data;
  ASSERT_NOT_NULL(data);

  // Add some triples
  for (int i = 0; i < 10; i++)
  {
    char buf[64];
    snprintf(buf, sizeof(buf), "subject_%d", i);
    uint32_t s = s7t_intern_string(data->engine, buf);
    s7t_add_triple(data->engine, s, data->test_predicate, data->test_object);
  }

  // Materialize subjects
  size_t count;
  uint32_t *results = s7t_materialize_subjects(data->engine, data->test_predicate, data->test_object, &count);

  ASSERT_NOT_NULL(results);
  ASSERT_EQUAL(10, count);

  free(results);
}

TEST_CASE(materialization_performance, "Test materialization performance")
static void test_materialization_performance(TestContext *context)
{
  RuntimeTestData *data = (RuntimeTestData *)context->test_data;
  ASSERT_NOT_NULL(data);

  // Add many triples
  for (int i = 0; i < 1000; i++)
  {
    char buf[64];
    snprintf(buf, sizeof(buf), "subject_%d", i);
    uint32_t s = s7t_intern_string(data->engine, buf);
    s7t_add_triple(data->engine, s, data->test_predicate, data->test_object);
  }

  // Performance test: materialization
  ASSERT_PERFORMANCE({
        size_t count;
        uint32_t* results = s7t_materialize_subjects(data->engine, data->test_predicate, data->test_object, &count);
        if (results) {
            free(results);
        } }, 1000000); // 1M cycles for materialization
}

// ============================================================================
// BIT VECTOR TESTS
// ============================================================================

TEST_CASE(bit_vector_creation, "Test bit vector creation")
static void test_bit_vector_creation(TestContext *context)
{
  RuntimeTestData *data = (RuntimeTestData *)context->test_data;
  ASSERT_NOT_NULL(data);

  // Add some triples
  s7t_add_triple(data->engine, data->test_subject, data->test_predicate, data->test_object);

  // Create bit vectors
  BitVector *pred_vec = s7t_get_subject_vector(data->engine, data->test_predicate, data->test_object);
  BitVector *obj_vec = s7t_get_object_vector(data->engine, data->test_predicate, data->test_subject);

  ASSERT_NOT_NULL(pred_vec);
  ASSERT_NOT_NULL(obj_vec);

  bitvec_destroy(pred_vec);
  bitvec_destroy(obj_vec);
}

TEST_CASE(bit_vector_operations, "Test bit vector operations")
static void test_bit_vector_operations(TestContext *context)
{
  RuntimeTestData *data = (RuntimeTestData *)context->test_data;
  ASSERT_NOT_NULL(data);

  // Add some triples
  s7t_add_triple(data->engine, data->test_subject, data->test_predicate, data->test_object);

  // Create bit vectors
  BitVector *pred_vec = s7t_get_subject_vector(data->engine, data->test_predicate, data->test_object);
  BitVector *obj_vec = s7t_get_object_vector(data->engine, data->test_predicate, data->test_subject);

  ASSERT_NOT_NULL(pred_vec);
  ASSERT_NOT_NULL(obj_vec);

  // Test bit vector operations
  BitVector *intersection = bitvec_and(pred_vec, obj_vec);
  ASSERT_NOT_NULL(intersection);

  size_t popcount = bitvec_popcount(intersection);
  ASSERT_GREATER_THAN(popcount, 0);

  bitvec_destroy(pred_vec);
  bitvec_destroy(obj_vec);
  bitvec_destroy(intersection);
}

TEST_CASE(bit_vector_performance, "Test bit vector performance")
static void test_bit_vector_performance(TestContext *context)
{
  RuntimeTestData *data = (RuntimeTestData *)context->test_data;
  ASSERT_NOT_NULL(data);

  // Add some triples
  for (int i = 0; i < 100; i++)
  {
    char buf[64];
    snprintf(buf, sizeof(buf), "subject_%d", i);
    uint32_t s = s7t_intern_string(data->engine, buf);
    s7t_add_triple(data->engine, s, data->test_predicate, data->test_object);
  }

  // Performance test: bit vector operations
  ASSERT_PERFORMANCE({
        BitVector* pred_vec = s7t_get_subject_vector(data->engine, data->test_predicate, data->test_object);
        BitVector* obj_vec = s7t_get_object_vector(data->engine, data->test_predicate, data->test_subject);
        
        if (pred_vec && obj_vec) {
            BitVector* intersection = bitvec_and(pred_vec, obj_vec);
            size_t count = bitvec_popcount(intersection);
            (void)count; // Prevent unused variable warning
            
            if (intersection) bitvec_destroy(intersection);
        }
        
        if (pred_vec) bitvec_destroy(pred_vec);
        if (obj_vec) bitvec_destroy(obj_vec); }, 100000); // 100K cycles for bit vector operations
}

// ============================================================================
// STRESS TESTS
// ============================================================================

TEST_CASE(stress_test_large_dataset, "Test with large dataset")
static void test_stress_test_large_dataset(TestContext *context)
{
  RuntimeTestData *data = (RuntimeTestData *)context->test_data;
  ASSERT_NOT_NULL(data);

  // Add large number of triples
  for (int i = 0; i < 10000; i++)
  {
    char buf[64];
    snprintf(buf, sizeof(buf), "subject_%d", i);
    uint32_t s = s7t_intern_string(data->engine, buf);

    snprintf(buf, sizeof(buf), "predicate_%d", i % 100);
    uint32_t p = s7t_intern_string(data->engine, buf);

    snprintf(buf, sizeof(buf), "object_%d", i % 1000);
    uint32_t o = s7t_intern_string(data->engine, buf);

    s7t_add_triple(data->engine, s, p, o);
  }

  // Verify we can still query
  ASSERT_EQUAL(10000, data->engine->triple_count);

  // Test pattern matching on large dataset
  uint32_t test_s = s7t_intern_string(data->engine, "subject_0");
  uint32_t test_p = s7t_intern_string(data->engine, "predicate_0");
  uint32_t test_o = s7t_intern_string(data->engine, "object_0");

  int result = s7t_ask_pattern(data->engine, test_s, test_p, test_o);
  ASSERT_EQUAL(1, result);
}

TEST_CASE(stress_test_memory_efficiency, "Test memory efficiency under load")
static void test_stress_test_memory_efficiency(TestContext *context)
{
  RuntimeTestData *data = (RuntimeTestData *)context->test_data;
  ASSERT_NOT_NULL(data);

  // Memory efficiency test: add many triples and check memory usage
  size_t initial_memory = get_memory_usage();

  for (int i = 0; i < 5000; i++)
  {
    char buf[64];
    snprintf(buf, sizeof(buf), "subject_%d", i);
    uint32_t s = s7t_intern_string(data->engine, buf);

    snprintf(buf, sizeof(buf), "predicate_%d", i % 50);
    uint32_t p = s7t_intern_string(data->engine, buf);

    snprintf(buf, sizeof(buf), "object_%d", i % 500);
    uint32_t o = s7t_intern_string(data->engine, buf);

    s7t_add_triple(data->engine, s, p, o);
  }

  size_t final_memory = get_memory_usage();
  size_t memory_used = final_memory - initial_memory;

  // Memory usage should be reasonable (less than 100MB for 5000 triples)
  ASSERT_LESS_THAN(memory_used, 100 * 1024 * 1024);
}

// ============================================================================
// EDGE CASE TESTS
// ============================================================================

TEST_CASE(edge_case_empty_engine, "Test operations on empty engine")
static void test_edge_case_empty_engine(TestContext *context)
{
  RuntimeTestData *data = (RuntimeTestData *)context->test_data;
  ASSERT_NOT_NULL(data);

  // Test pattern matching on empty engine
  int result = s7t_ask_pattern(data->engine, data->test_subject, data->test_predicate, data->test_object);
  ASSERT_EQUAL(0, result);

  // Test materialization on empty engine
  size_t count;
  uint32_t *results = s7t_materialize_subjects(data->engine, data->test_predicate, data->test_object, &count);
  ASSERT_EQUAL(0, count);
  // Note: results might be NULL or empty array depending on implementation
}

TEST_CASE(edge_case_null_strings, "Test handling of NULL strings")
static void test_edge_case_null_strings(TestContext *context)
{
  RuntimeTestData *data = (RuntimeTestData *)context->test_data;
  ASSERT_NOT_NULL(data);

  // Test interning NULL string (should handle gracefully)
  uint32_t null_id = s7t_intern_string(data->engine, NULL);
  // Implementation dependent - should either return 0 or handle gracefully

  // Test adding triple with NULL string IDs
  s7t_add_triple(data->engine, 0, 0, 0);
  // Should not crash
}

TEST_CASE(edge_case_duplicate_strings, "Test duplicate string handling")
static void test_edge_case_duplicate_strings(TestContext *context)
{
  RuntimeTestData *data = (RuntimeTestData *)context->test_data;
  ASSERT_NOT_NULL(data);

  // Test interning same string multiple times
  uint32_t id1 = s7t_intern_string(data->engine, "duplicate");
  uint32_t id2 = s7t_intern_string(data->engine, "duplicate");
  uint32_t id3 = s7t_intern_string(data->engine, "duplicate");

  // All should return same ID
  ASSERT_EQUAL(id1, id2);
  ASSERT_EQUAL(id2, id3);

  // Check that string count doesn't increase for duplicates
  size_t initial_count = data->engine->subject_count;
  s7t_intern_string(data->engine, "duplicate");
  ASSERT_EQUAL(initial_count, data->engine->subject_count);
}

TEST_SUITE_END(seven_t_runtime)

// Test suite registration function
void register_seven_t_runtime_tests(void)
{
  // Register all test cases
  TestCase test_cases[] = {
      // Engine creation and destruction
      test_case_engine_creation,
      test_case_engine_destruction,

      // String interning
      test_case_string_interning_basic,
      test_case_string_interning_performance,
      test_case_string_interning_memory,

      // Triple operations
      test_case_triple_addition_basic,
      test_case_triple_addition_duplicate,
      test_case_triple_addition_performance,
      test_case_triple_addition_memory,

      // Pattern matching
      test_case_pattern_matching_basic,
      test_case_pattern_matching_performance,
      test_case_pattern_matching_latency,

      // Materialization
      test_case_materialization_basic,
      test_case_materialization_performance,

      // Bit vectors
      test_case_bit_vector_creation,
      test_case_bit_vector_operations,
      test_case_bit_vector_performance,

      // Stress tests
      test_case_stress_test_large_dataset,
      test_case_stress_test_memory_efficiency,

      // Edge cases
      test_case_edge_case_empty_engine,
      test_case_edge_case_null_strings,
      test_case_edge_case_duplicate_strings};

  seven_t_runtime_registration.test_cases = test_cases;
  seven_t_runtime_registration.test_case_count = sizeof(test_cases) / sizeof(TestCase);
  seven_t_runtime_registration.suite_setup = setup_runtime_test;
  seven_t_runtime_registration.suite_teardown = teardown_runtime_test;
}

// Main test runner
int main(int argc, char *argv[])
{
  printf("=== 7T Runtime Unit Tests ===\n");
  printf("Framework Version: %s\n", SEVEN_TICK_TEST_VERSION);

  // Initialize test configuration
  test_config_init();
  test_config_set_verbose(true);

  // Register tests
  register_seven_t_runtime_tests();

  // Run test suite
  test_run_suite(&seven_t_runtime_registration);

  printf("\n=== Runtime Tests Complete ===\n");
  return 0;
}