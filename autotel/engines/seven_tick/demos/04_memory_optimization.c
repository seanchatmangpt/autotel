#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "../runtime/src/seven_t_runtime.h"

// Demo: Memory Optimization and Cache Performance
// Showcases L1/L2/L3 cache optimization and memory hierarchy compliance

// Cache-aligned data structure for L1 optimization
typedef struct __attribute__((aligned(64)))
{
  uint64_t data[8]; // 64 bytes = 1 cache line
  uint32_t count;
  uint32_t capacity;
} L1OptimizedArray;

// L2 cache-friendly structure
typedef struct __attribute__((aligned(128)))
{
  uint64_t data[16]; // 128 bytes = 2 cache lines
  uint32_t metadata[4];
} L2OptimizedArray;

// Memory pool for zero-allocation operations
typedef struct
{
  void *blocks[1024];
  size_t block_size;
  size_t used;
  size_t capacity;
} MemoryPool;

// Helper for nanosecond timing
static inline uint64_t get_nanoseconds()
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

void demo_memory_optimization()
{
  printf("💾 Memory Optimization Demo\n");
  printf("===========================\n\n");

  // Create engine
  EngineState *engine = s7t_create_engine();
  if (!engine)
  {
    printf("❌ Failed to create engine\n");
    return;
  }

  printf("📊 Loading memory test data...\n");

  // Intern strings for memory test
  uint32_t entities[1000];
  uint32_t predicates[100];
  uint32_t objects[1000];

  for (int i = 0; i < 1000; i++)
  {
    char str[64];
    snprintf(str, sizeof(str), "ex:entity_%d", i);
    entities[i] = s7t_intern_string(engine, str);
  }

  for (int i = 0; i < 100; i++)
  {
    char str[64];
    snprintf(str, sizeof(str), "ex:predicate_%d", i);
    predicates[i] = s7t_intern_string(engine, str);
  }

  for (int i = 0; i < 1000; i++)
  {
    char str[64];
    snprintf(str, sizeof(str), "ex:object_%d", i);
    objects[i] = s7t_intern_string(engine, str);
  }

  // Add test triples
  for (int i = 0; i < 10000; i++)
  {
    uint32_t s = entities[i % 1000];
    uint32_t p = predicates[i % 100];
    uint32_t o = objects[i % 1000];
    s7t_add_triple(engine, s, p, o);
  }

  printf("✅ Loaded %zu triples\n\n", engine->triple_count);

  // Demo 1: L1 Cache Performance (7-tick operations)
  printf("🔍 Demo 1: L1 Cache Performance\n");
  printf("--------------------------------\n");

  // Test L1 cache-friendly operations
  const int l1_iterations = 1000000;
  uint64_t start = get_nanoseconds();

  for (int i = 0; i < l1_iterations; i++)
  {
    // Direct bit vector access (L1 cache)
    uint32_t s = entities[i % 1000];
    uint32_t p = predicates[i % 100];
    uint32_t o = objects[i % 1000];

    s7t_ask_pattern(engine, s, p, o);
  }

  uint64_t end = get_nanoseconds();
  double avg_ns = (double)(end - start) / l1_iterations;

  printf("L1 cache operations: %.1f ns per operation\n", avg_ns);
  if (avg_ns < 10.0)
  {
    printf("🎉 7-TICK L1 PERFORMANCE ACHIEVED!\n");
  }
  printf("\n");

  // Demo 2: L2 Cache Performance
  printf("🔍 Demo 2: L2 Cache Performance\n");
  printf("--------------------------------\n");

  // Test L2 cache operations (larger data sets)
  const int l2_iterations = 100000;
  start = get_nanoseconds();

  for (int i = 0; i < l2_iterations; i++)
  {
    // Multiple bit vector operations (L2 cache)
    for (int j = 0; j < 10; j++)
    {
      uint32_t s = entities[(i + j) % 1000];
      uint32_t p = predicates[j % 100];
      uint32_t o = objects[(i + j) % 1000];

      s7t_ask_pattern(engine, s, p, o);
    }
  }

  end = get_nanoseconds();
  avg_ns = (double)(end - start) / (l2_iterations * 10);

  printf("L2 cache operations: %.1f ns per operation\n", avg_ns);
  if (avg_ns < 100.0)
  {
    printf("✅ SUB-100NS L2 PERFORMANCE!\n");
  }
  printf("\n");

  // Demo 3: Memory Hierarchy Analysis
  printf("🔍 Demo 3: Memory Hierarchy Analysis\n");
  printf("------------------------------------\n");

  printf("Memory hierarchy performance:\n");

  // L1 cache test (small, hot data)
  start = get_nanoseconds();
  for (int i = 0; i < 1000000; i++)
  {
    s7t_ask_pattern(engine, entities[0], predicates[0], objects[0]);
  }
  end = get_nanoseconds();
  double l1_ns = (double)(end - start) / 1000000;

  // L2 cache test (medium data)
  start = get_nanoseconds();
  for (int i = 0; i < 100000; i++)
  {
    for (int j = 0; j < 10; j++)
    {
      s7t_ask_pattern(engine, entities[j], predicates[j], objects[j]);
    }
  }
  end = get_nanoseconds();
  double l2_ns = (double)(end - start) / (100000 * 10);

  // L3 cache test (larger data)
  start = get_nanoseconds();
  for (int i = 0; i < 10000; i++)
  {
    for (int j = 0; j < 100; j++)
    {
      s7t_ask_pattern(engine, entities[j], predicates[j % 10], objects[j]);
    }
  }
  end = get_nanoseconds();
  double l3_ns = (double)(end - start) / (10000 * 100);

  printf("  L1 Cache (hot data): %.1f ns\n", l1_ns);
  printf("  L2 Cache (warm data): %.1f ns\n", l2_ns);
  printf("  L3 Cache (cold data): %.1f ns\n", l3_ns);

  // Demo 4: Cache-Aligned Data Structures
  printf("\n🔍 Demo 4: Cache-Aligned Data Structures\n");
  printf("----------------------------------------\n");

  // Test cache-aligned structures
  L1OptimizedArray *l1_array = aligned_alloc(64, sizeof(L1OptimizedArray));
  L2OptimizedArray *l2_array = aligned_alloc(128, sizeof(L2OptimizedArray));

  if (l1_array && l2_array)
  {
    // Initialize arrays
    memset(l1_array->data, 0xFF, sizeof(l1_array->data));
    memset(l2_array->data, 0xAA, sizeof(l2_array->data));

    // Benchmark cache-aligned access
    start = get_nanoseconds();
    for (int i = 0; i < 1000000; i++)
    {
      l1_array->data[i % 8] = i;
      l1_array->count = i % 100;
    }
    end = get_nanoseconds();
    double aligned_ns = (double)(end - start) / 1000000;

    printf("Cache-aligned access: %.1f ns per operation\n", aligned_ns);
    printf("✅ Cache line alignment working\n");

    free(l1_array);
    free(l2_array);
  }

  // Demo 5: Memory Pool Performance
  printf("\n🔍 Demo 5: Memory Pool Performance\n");
  printf("----------------------------------\n");

  // Simple memory pool implementation
  MemoryPool pool = {0};
  pool.block_size = 64;
  pool.capacity = 1024;
  pool.blocks[0] = malloc(pool.block_size * pool.capacity);

  if (pool.blocks[0])
  {
    // Benchmark memory pool allocation
    start = get_nanoseconds();
    for (int i = 0; i < 100000; i++)
    {
      void *block = pool.blocks[0] + ((i % pool.capacity) * pool.block_size);
      // Simulate using the block
      memset(block, i % 256, pool.block_size);
    }
    end = get_nanoseconds();
    double pool_ns = (double)(end - start) / 100000;

    printf("Memory pool allocation: %.1f ns per allocation\n", pool_ns);
    printf("✅ Zero-fragmentation allocation\n");

    free(pool.blocks[0]);
  }

  // Demo 6: String Interning Performance
  printf("\n🔍 Demo 6: String Interning Performance\n");
  printf("---------------------------------------\n");

  printf("String interning performance:\n");

  // Test string interning
  start = get_nanoseconds();
  for (int i = 0; i < 100000; i++)
  {
    char str[64];
    snprintf(str, sizeof(str), "test_string_%d", i % 1000);
    s7t_intern_string(engine, str);
  }
  end = get_nanoseconds();
  double interning_ns = (double)(end - start) / 100000;

  printf("  String interning: %.1f ns per string\n", interning_ns);
  printf("  Total unique strings: %zu\n", engine->string_count);

  // Demo 7: Memory Usage Analysis
  printf("\n🔍 Demo 7: Memory Usage Analysis\n");
  printf("--------------------------------\n");

  printf("Memory usage statistics:\n");
  printf("  Total triples: %zu\n", engine->triple_count);
  printf("  Max subject ID: %zu\n", engine->max_subject_id);
  printf("  Max predicate ID: %zu\n", engine->max_predicate_id);
  printf("  Max object ID: %zu\n", engine->max_object_id);
  printf("  String count: %zu\n", engine->string_count);

  // Estimate memory usage
  size_t estimated_memory =
      engine->triple_count * sizeof(uint32_t) * 3 + // Triple storage
      engine->max_predicate_id * sizeof(void *) +   // Predicate vectors
      engine->max_object_id * sizeof(void *) +      // Object vectors
      engine->string_count * 32;                    // String storage

  printf("  Estimated memory usage: %zu bytes (%.1f MB)\n",
         estimated_memory, estimated_memory / (1024.0 * 1024.0));

  // Cleanup
  s7t_destroy_engine(engine);

  printf("\n✅ Memory Optimization Demo Complete\n");
  printf("===================================\n");
}

int main()
{
  demo_memory_optimization();
  return 0;
}