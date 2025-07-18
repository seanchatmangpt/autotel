#include <stdio.h>
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
  printf("PASSED: Engine created\n");

  printf("Adding single triple...\n");
  uint32_t s = s7t_intern_string(engine, "subject");
  uint32_t p = s7t_intern_string(engine, "predicate");
  uint32_t o = s7t_intern_string(engine, "object");

  printf("String IDs: s=%u, p=%u, o=%u\n", s, p, o);

  s7t_add_triple(engine, s, p, o);
  printf("PASSED: Triple added\n");

  printf("Destroying engine...\n");
  s7t_destroy_engine(engine);
  printf("PASSED: Engine destroyed\n");

  return 0;
}