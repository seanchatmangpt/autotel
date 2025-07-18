#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../runtime/src/seven_t_runtime.h"

int main()
{
  printf("Creating engine...\n");
  EngineState *engine = s7t_create_engine();
  if (!engine)
  {
    printf("FAILED: Could not create engine\n");
    return 1;
  }
  printf("PASSED\n");

  printf("Testing hash table with 100 triples...\n");
  for (int i = 0; i < 100; i++)
  {
    printf("Adding triple %d...\n", i);
    char subj[32], obj[32];
    snprintf(subj, sizeof(subj), "subject_%d", i);
    snprintf(obj, sizeof(obj), "object_%d", i);

    uint32_t s = s7t_intern_string(engine, subj);
    uint32_t p = s7t_intern_string(engine, "predicate");
    uint32_t o = s7t_intern_string(engine, obj);

    printf("  String IDs: s=%u, p=%u, o=%u\n", s, p, o);

    s7t_add_triple(engine, s, p, o);
    printf("  Triple %d added successfully\n", i);
  }
  printf("PASSED: Added %zu triples\n", engine->triple_count);

  printf("Testing query...\n");
  uint32_t pred = s7t_intern_string(engine, "predicate");
  uint32_t obj = s7t_intern_string(engine, "object_5");

  BitVector *subjects = s7t_get_subject_vector(engine, pred, obj);
  if (subjects)
  {
    printf("PASSED: Found %zu subjects\n", subjects->count);
    bitvec_destroy(subjects);
  }
  else
  {
    printf("FAILED: No subjects found\n");
  }

  printf("Destroying engine...\n");
  s7t_destroy_engine(engine);
  printf("PASSED: Engine destroyed\n");

  return 0;
}