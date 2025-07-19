#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../runtime/src/seven_t_runtime.h"

int main()
{
  printf("SPARQL Implementation Debug\n");
  printf("===========================\n\n");

  // Create engine
  EngineState *engine = s7t_create_engine();
  if (!engine)
  {
    printf("❌ Failed to create engine\n");
    return 1;
  }

  // Add test data
  uint32_t s1 = s7t_intern_string(engine, "ex:alice");
  uint32_t s2 = s7t_intern_string(engine, "ex:bob");
  uint32_t p1 = s7t_intern_string(engine, "ex:knows");
  uint32_t o1 = s7t_intern_string(engine, "ex:charlie");

  printf("String IDs: s1=%u, s2=%u, p1=%u, o1=%u\n", s1, s2, p1, o1);

  // Add triple: Alice knows Charlie
  s7t_add_triple(engine, s1, p1, o1);
  printf("Added triple: (%u, %u, %u)\n", s1, p1, o1);

  // Test pattern matching
  printf("\nTesting pattern matching:\n");

  // Should be true: Alice knows Charlie
  int result1 = s7t_ask_pattern(engine, s1, p1, o1);
  printf("s7t_ask_pattern(%u, %u, %u) = %d (should be 1)\n", s1, p1, o1, result1);

  // Should be false: Bob knows Charlie
  int result2 = s7t_ask_pattern(engine, s2, p1, o1);
  printf("s7t_ask_pattern(%u, %u, %u) = %d (should be 0)\n", s2, p1, o1, result2);

  // Should be false: Alice knows Bob
  int result3 = s7t_ask_pattern(engine, s1, p1, s2);
  printf("s7t_ask_pattern(%u, %u, %u) = %d (should be 0)\n", s1, p1, s2, result3);

  // Debug bit vectors
  printf("\nDebugging bit vectors:\n");
  BitVector *pred_vec = engine->predicate_vectors[p1];
  BitVector *obj_vec = engine->object_vectors[o1];

  if (pred_vec)
  {
    printf("Predicate vector for p1=%u exists\n", p1);
    printf("Predicate vector capacity: %zu\n", pred_vec->capacity);

    // Check if subject s1 is set in predicate vector
    size_t chunk = s1 / 64;
    uint64_t bit = 1ULL << (s1 % 64);
    uint64_t p_word = pred_vec->bits[chunk];
    printf("Subject %u: chunk=%zu, bit=0x%lx, word=0x%lx, result=%d\n",
           s1, chunk, bit, p_word, (p_word & bit) != 0);
  }
  else
  {
    printf("❌ Predicate vector for p1=%u is NULL\n", p1);
  }

  if (obj_vec)
  {
    printf("Object vector for o1=%u exists\n", o1);
    printf("Object vector capacity: %zu\n", obj_vec->capacity);

    // Check if subject s1 is set in object vector
    size_t chunk = s1 / 64;
    uint64_t bit = 1ULL << (s1 % 64);
    uint64_t o_word = obj_vec->bits[chunk];
    printf("Subject %u: chunk=%zu, bit=0x%lx, word=0x%lx, result=%d\n",
           s1, chunk, bit, o_word, (o_word & bit) != 0);
  }
  else
  {
    printf("❌ Object vector for o1=%u is NULL\n", o1);
  }

  // Cleanup
  s7t_destroy_engine(engine);

  return 0;
}