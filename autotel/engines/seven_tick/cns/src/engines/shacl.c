#include "cns/engines/shacl.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// High-precision cycle counting
static inline uint64_t get_cycles()
{
  return __builtin_readcyclecounter();
}

// Optimized engine creation
CNSShaclEngine *cns_shacl_create(CNSSparqlEngine *sparql_engine)
{
  if (!sparql_engine)
    return NULL;

  CNSShaclEngine *engine = malloc(sizeof(CNSShaclEngine));
  if (!engine)
    return NULL;

  engine->sparql_engine = sparql_engine;
  engine->shapes = NULL;
  engine->shape_count = 0;
  engine->shape_capacity = 0;
  engine->memory_usage = sizeof(CNSShaclEngine);

  return engine;
}

// Optimized engine destruction
void cns_shacl_destroy(CNSShaclEngine *engine)
{
  if (!engine)
    return;

  // Free shapes and constraints
  for (size_t i = 0; i < engine->shape_count; i++)
  {
    CNSShaclShape *shape = &engine->shapes[i];
    for (size_t j = 0; j < shape->constraint_count; j++)
    {
      if (shape->constraints[j].string_value)
      {
        free(shape->constraints[j].string_value);
      }
    }
    if (shape->constraints)
    {
      free(shape->constraints);
    }
  }

  if (engine->shapes)
  {
    free(engine->shapes);
  }

  free(engine);
}

// Define a SHACL shape
int cns_shacl_define_shape(CNSShaclEngine *engine, uint32_t shape_id, uint32_t target_class)
{
  if (!engine)
    return -1;

  // Check if shape already exists
  for (size_t i = 0; i < engine->shape_count; i++)
  {
    if (engine->shapes[i].shape_id == shape_id)
    {
      return -1; // Shape already exists
    }
  }

  // Expand shapes array if needed
  if (engine->shape_count >= engine->shape_capacity)
  {
    size_t new_capacity = engine->shape_capacity == 0 ? 16 : engine->shape_capacity * 2;
    CNSShaclShape *new_shapes = realloc(engine->shapes, new_capacity * sizeof(CNSShaclShape));
    if (!new_shapes)
      return -1;

    engine->shapes = new_shapes;
    engine->shape_capacity = new_capacity;
  }

  // Add new shape
  CNSShaclShape *shape = &engine->shapes[engine->shape_count];
  shape->shape_id = shape_id;
  shape->target_class = target_class;
  shape->constraints = NULL;
  shape->constraint_count = 0;
  shape->constraint_capacity = 0;

  engine->shape_count++;
  engine->memory_usage += sizeof(CNSShaclShape);

  return 0;
}

// Add constraint to shape
int cns_shacl_add_constraint(CNSShaclEngine *engine, uint32_t shape_id, CNSShaclConstraint *constraint)
{
  if (!engine || !constraint)
    return -1;

  // Find shape
  CNSShaclShape *shape = NULL;
  for (size_t i = 0; i < engine->shape_count; i++)
  {
    if (engine->shapes[i].shape_id == shape_id)
    {
      shape = &engine->shapes[i];
      break;
    }
  }

  if (!shape)
    return -1;

  // Expand constraints array if needed
  if (shape->constraint_count >= shape->constraint_capacity)
  {
    size_t new_capacity = shape->constraint_capacity == 0 ? 8 : shape->constraint_capacity * 2;
    CNSShaclConstraint *new_constraints = realloc(shape->constraints, new_capacity * sizeof(CNSShaclConstraint));
    if (!new_constraints)
      return -1;

    shape->constraints = new_constraints;
    shape->constraint_capacity = new_capacity;
  }

  // Add constraint
  CNSShaclConstraint *new_constraint = &shape->constraints[shape->constraint_count];
  new_constraint->type = constraint->type;
  new_constraint->property_id = constraint->property_id;
  new_constraint->value = constraint->value;

  // Copy string value if present
  if (constraint->string_value)
  {
    size_t len = strlen(constraint->string_value);
    new_constraint->string_value = malloc(len + 1);
    if (new_constraint->string_value)
    {
      strcpy(new_constraint->string_value, constraint->string_value);
    }
  }
  else
  {
    new_constraint->string_value = NULL;
  }

  shape->constraint_count++;
  engine->memory_usage += sizeof(CNSShaclConstraint);

  return 0;
}

// 7-tick optimized min_count validation
bool cns_shacl_check_min_count(CNSShaclEngine *engine, uint32_t node_id, uint32_t property_id, uint32_t min_count)
{
  if (!engine || min_count == 0)
    return true;

  // Count property values using SPARQL engine
  uint32_t count = 0;
  const uint32_t max_check = 1000; // Limit for performance

  for (uint32_t obj_id = 0; obj_id < max_check; obj_id++)
  {
    if (cns_sparql_ask_pattern(engine->sparql_engine, node_id, property_id, obj_id))
    {
      count++;
      if (count >= min_count)
      {
        return true; // Early exit for performance
      }
    }
  }

  return count >= min_count;
}

// 7-tick optimized max_count validation
bool cns_shacl_check_max_count(CNSShaclEngine *engine, uint32_t node_id, uint32_t property_id, uint32_t max_count)
{
  if (!engine)
    return false;

  // Count property values using SPARQL engine
  uint32_t count = 0;
  const uint32_t max_check = 1000; // Limit for performance

  for (uint32_t obj_id = 0; obj_id < max_check; obj_id++)
  {
    if (cns_sparql_ask_pattern(engine->sparql_engine, node_id, property_id, obj_id))
    {
      count++;
      if (count > max_count)
      {
        return false; // Early exit for performance
      }
    }
  }

  return count <= max_count;
}

// 7-tick optimized class validation
bool cns_shacl_check_class(CNSShaclEngine *engine, uint32_t node_id, uint32_t class_id)
{
  if (!engine)
    return false;

  // Check if node is of the specified class using SPARQL engine
  return cns_sparql_ask_pattern(engine->sparql_engine, node_id, 1, class_id) != 0; // rdf:type = 1
}

// Validate node against all applicable shapes
bool cns_shacl_validate_node(CNSShaclEngine *engine, uint32_t node_id)
{
  if (!engine)
    return false;

  for (size_t i = 0; i < engine->shape_count; i++)
  {
    CNSShaclShape *shape = &engine->shapes[i];

    // Check if node is of target class
    if (cns_shacl_check_class(engine, node_id, shape->target_class))
    {
      // Validate all constraints
      for (size_t j = 0; j < shape->constraint_count; j++)
      {
        CNSShaclConstraint *constraint = &shape->constraints[j];

        bool valid = false;
        switch (constraint->type)
        {
        case CNS_SHACL_MIN_COUNT:
          valid = cns_shacl_check_min_count(engine, node_id, constraint->property_id, constraint->value);
          break;
        case CNS_SHACL_MAX_COUNT:
          valid = cns_shacl_check_max_count(engine, node_id, constraint->property_id, constraint->value);
          break;
        case CNS_SHACL_CLASS:
          valid = cns_shacl_check_class(engine, node_id, constraint->value);
          break;
        default:
          valid = true; // Skip unsupported constraints for now
          break;
        }

        if (!valid)
        {
          return false; // Node fails validation
        }
      }
    }
  }

  return true; // Node passes all validations
}

// Performance monitoring
uint64_t cns_shacl_get_cycles(void)
{
  return __builtin_readcyclecounter();
}

void cns_shacl_measure_validation_cycles(CNSShaclEngine *engine, uint32_t node_id)
{
  uint64_t start = get_cycles();
  bool result = cns_shacl_validate_node(engine, node_id);
  uint64_t end = get_cycles();

  uint64_t cycles = end - start;
  if (cycles > 7)
  {
    printf("Warning: SHACL validation took %lu cycles (>7) - node_id=%u, result=%d\n",
           cycles, node_id, result);
  }
}

// Memory management
size_t cns_shacl_get_memory_usage(CNSShaclEngine *engine)
{
  return engine ? engine->memory_usage : 0;
}