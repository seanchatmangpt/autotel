/*
 * First Principles C Tutorial: Memory Management
 *
 * This tutorial validates understanding of memory management concepts
 * that are critical for the 7T Engine's performance and reliability.
 *
 * Key Concepts:
 * - Stack vs Heap memory
 * - Memory allocation patterns
 * - Memory safety and validation
 * - Performance implications
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Test utilities
#define TEST_ASSERT(condition, message) \
  do                                    \
  {                                     \
    if (!(condition))                   \
    {                                   \
      printf("❌ FAIL: %s\n", message); \
      return 0;                         \
    }                                   \
    else                                \
    {                                   \
      printf("✅ PASS: %s\n", message); \
    }                                   \
  } while (0)

#define TEST_SECTION(name) printf("\n=== %s ===\n", name)

// Lesson 1: Stack vs Heap Memory
int lesson_stack_vs_heap()
{
  TEST_SECTION("Stack vs Heap Memory");

  // Stack allocation (automatic)
  int stack_array[100];
  stack_array[0] = 42;
  stack_array[99] = 99;

  TEST_ASSERT(stack_array[0] == 42, "Stack array write/read");
  TEST_ASSERT(stack_array[99] == 99, "Stack array bounds");

  // Heap allocation (manual)
  int *heap_array = malloc(100 * sizeof(int));
  TEST_ASSERT(heap_array != NULL, "Heap allocation success");

  heap_array[0] = 42;
  heap_array[99] = 99;

  TEST_ASSERT(heap_array[0] == 42, "Heap array write/read");
  TEST_ASSERT(heap_array[99] == 99, "Heap array bounds");

  // Memory cleanup
  free(heap_array);

  // Stack automatically cleaned up when function returns
  return 1;
}

// Lesson 2: Memory Allocation Patterns
int lesson_allocation_patterns()
{
  TEST_SECTION("Memory Allocation Patterns");

  // Pattern 1: Single allocation
  char *single = malloc(100);
  TEST_ASSERT(single != NULL, "Single allocation");
  strcpy(single, "Hello World");
  TEST_ASSERT(strcmp(single, "Hello World") == 0, "Single allocation usage");
  free(single);

  // Pattern 2: Multiple allocations
  char **multiple = malloc(5 * sizeof(char *));
  TEST_ASSERT(multiple != NULL, "Multiple allocation array");

  for (int i = 0; i < 5; i++)
  {
    multiple[i] = malloc(20);
    TEST_ASSERT(multiple[i] != NULL, "Individual allocation");
    sprintf(multiple[i], "String %d", i);
  }

  TEST_ASSERT(strcmp(multiple[0], "String 0") == 0, "Multiple allocation usage");
  TEST_ASSERT(strcmp(multiple[4], "String 4") == 0, "Multiple allocation bounds");

  // Cleanup multiple allocations
  for (int i = 0; i < 5; i++)
  {
    free(multiple[i]);
  }
  free(multiple);

  // Pattern 3: Reallocation
  char *realloc_test = malloc(10);
  strcpy(realloc_test, "Hello");

  char *expanded = realloc(realloc_test, 20);
  TEST_ASSERT(expanded != NULL, "Reallocation success");
  strcat(expanded, " World");
  TEST_ASSERT(strcmp(expanded, "Hello World") == 0, "Reallocation usage");

  free(expanded);

  return 1;
}

// Lesson 3: Memory Safety Validation
int lesson_memory_safety()
{
  TEST_SECTION("Memory Safety Validation");

  // Safety 1: NULL pointer checks
  char *ptr = NULL;
  TEST_ASSERT(ptr == NULL, "NULL pointer validation");

  // Safety 2: Allocation failure handling
  char *large_alloc = malloc(SIZE_MAX);
  if (large_alloc == NULL)
  {
    printf("  Expected: Large allocation failed gracefully\n");
  }

  // Safety 3: Bounds checking
  int *bounds_test = malloc(10 * sizeof(int));
  TEST_ASSERT(bounds_test != NULL, "Bounds test allocation");

  // Valid access
  bounds_test[0] = 1;
  bounds_test[9] = 10;
  TEST_ASSERT(bounds_test[0] == 1, "Valid lower bound access");
  TEST_ASSERT(bounds_test[9] == 10, "Valid upper bound access");

  // Note: We don't test invalid bounds as it's undefined behavior
  printf("  Note: Invalid bounds access would cause undefined behavior\n");

  free(bounds_test);

  // Safety 4: Double free prevention
  char *double_free_test = malloc(10);
  TEST_ASSERT(double_free_test != NULL, "Double free test allocation");
  free(double_free_test);
  printf("  Note: Double free would cause undefined behavior\n");

  return 1;
}

// Lesson 4: Performance Implications
int lesson_performance_implications()
{
  TEST_SECTION("Performance Implications");

  // Performance 1: Stack allocation speed
  clock_t start = clock();
  for (int i = 0; i < 10000; i++)
  {
    int stack_array[100];
    stack_array[0] = i;
  }
  clock_t end = clock();
  double stack_time = ((double)(end - start)) / CLOCKS_PER_SEC;

  printf("  Stack allocation time: %.6f seconds\n", stack_time);
  TEST_ASSERT(stack_time < 0.001, "Stack allocation is fast");

  // Performance 2: Heap allocation speed
  start = clock();
  for (int i = 0; i < 10000; i++)
  {
    int *heap_array = malloc(100 * sizeof(int));
    heap_array[0] = i;
    free(heap_array);
  }
  end = clock();
  double heap_time = ((double)(end - start)) / CLOCKS_PER_SEC;

  printf("  Heap allocation time: %.6f seconds\n", heap_time);
  TEST_ASSERT(heap_time > stack_time, "Heap allocation is slower than stack");

  // Performance 3: Memory locality
  int *local_array = malloc(1000 * sizeof(int));
  TEST_ASSERT(local_array != NULL, "Local array allocation");

  // Sequential access (cache-friendly)
  start = clock();
  for (int i = 0; i < 1000; i++)
  {
    local_array[i] = i;
  }
  end = clock();
  double sequential_time = ((double)(end - start)) / CLOCKS_PER_SEC;

  // Random access (cache-unfriendly)
  start = clock();
  for (int i = 0; i < 1000; i++)
  {
    int index = (i * 7) % 1000; // Pseudo-random access
    local_array[index] = i;
  }
  end = clock();
  double random_time = ((double)(end - start)) / CLOCKS_PER_SEC;

  printf("  Sequential access time: %.6f seconds\n", sequential_time);
  printf("  Random access time: %.6f seconds\n", random_time);
  TEST_ASSERT(sequential_time <= random_time, "Sequential access is faster or equal");

  free(local_array);

  return 1;
}

// Lesson 5: Memory Patterns for 7T Engine
int lesson_7t_engine_patterns()
{
  TEST_SECTION("7T Engine Memory Patterns");

  // Pattern 1: Fixed-size pools (like CJinja context)
  typedef struct
  {
    char *variables[100];
    int values[100];
    size_t count;
  } ContextPool;

  ContextPool *pool = malloc(sizeof(ContextPool));
  TEST_ASSERT(pool != NULL, "Context pool allocation");
  pool->count = 0;

  // Add to pool
  pool->variables[pool->count] = strdup("name");
  pool->values[pool->count] = 42;
  pool->count++;

  TEST_ASSERT(pool->count == 1, "Pool addition");
  TEST_ASSERT(strcmp(pool->variables[0], "name") == 0, "Pool variable storage");
  TEST_ASSERT(pool->values[0] == 42, "Pool value storage");

  // Cleanup pool
  free(pool->variables[0]);
  free(pool);

  // Pattern 2: Dynamic arrays (like SPARQL triples)
  typedef struct
  {
    int *subjects;
    int *predicates;
    int *objects;
    size_t count;
    size_t capacity;
  } TripleArray;

  TripleArray *triples = malloc(sizeof(TripleArray));
  TEST_ASSERT(triples != NULL, "Triple array allocation");

  triples->capacity = 10;
  triples->count = 0;
  triples->subjects = malloc(triples->capacity * sizeof(int));
  triples->predicates = malloc(triples->capacity * sizeof(int));
  triples->objects = malloc(triples->capacity * sizeof(int));

  TEST_ASSERT(triples->subjects != NULL, "Subjects array allocation");
  TEST_ASSERT(triples->predicates != NULL, "Predicates array allocation");
  TEST_ASSERT(triples->objects != NULL, "Objects array allocation");

  // Add triple
  triples->subjects[triples->count] = 1;
  triples->predicates[triples->count] = 2;
  triples->objects[triples->count] = 3;
  triples->count++;

  TEST_ASSERT(triples->count == 1, "Triple addition");
  TEST_ASSERT(triples->subjects[0] == 1, "Triple subject storage");
  TEST_ASSERT(triples->predicates[0] == 2, "Triple predicate storage");
  TEST_ASSERT(triples->objects[0] == 3, "Triple object storage");

  // Cleanup triple array
  free(triples->subjects);
  free(triples->predicates);
  free(triples->objects);
  free(triples);

  return 1;
}

// Main tutorial runner
int main()
{
  printf("First Principles C Tutorial: Memory Management\n");
  printf("==============================================\n");
  printf("Validating core memory concepts for 7T Engine\n\n");

  int total_lessons = 5;
  int passed_lessons = 0;

  // Run all lessons
  if (lesson_stack_vs_heap())
    passed_lessons++;
  if (lesson_allocation_patterns())
    passed_lessons++;
  if (lesson_memory_safety())
    passed_lessons++;
  if (lesson_performance_implications())
    passed_lessons++;
  if (lesson_7t_engine_patterns())
    passed_lessons++;

  // Summary
  printf("\n=== Tutorial Summary ===\n");
  printf("Total lessons: %d\n", total_lessons);
  printf("Passed: %d\n", passed_lessons);
  printf("Failed: %d\n", total_lessons - passed_lessons);
  printf("Success rate: %.1f%%\n", (passed_lessons * 100.0) / total_lessons);

  if (passed_lessons == total_lessons)
  {
    printf("\n🎉 All memory management concepts validated!\n");
    printf("Ready for 7T Engine development.\n");
    return 0;
  }
  else
  {
    printf("\n❌ Some concepts need review.\n");
    return 1;
  }
}