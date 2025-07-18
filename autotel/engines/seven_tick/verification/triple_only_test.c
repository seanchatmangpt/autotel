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

  printf("Adding triples...\n");
  for (int i = 0; i < 100; i++)
  {
    char subj[32], obj[32];
    snprintf(subj, sizeof(subj), "subject_%d", i);
    snprintf(obj, sizeof(obj), "object_%d", i);

    uint32_t s = s7t_intern_string(engine, subj);
    uint32_t p = s7t_intern_string(engine, "predicate");
    uint32_t o = s7t_intern_string(engine, obj);

    s7t_add_triple(engine, s, p, o);
  }
  printf("PASSED: Added %zu triples\n", engine->triple_count);

  printf("Destroying engine...\n");
  s7t_destroy_engine(engine);
  printf("PASSED: Engine destroyed\n");

  return 0;
}