#ifndef SEVEN_T_RUNTIME_OPTIMIZED_H
#define SEVEN_T_RUNTIME_OPTIMIZED_H

#include <stdint.h>
#include <stddef.h>
#include "seven_t_runtime.h"

// 80/20 optimization: Ultra-fast inline functions for 7T performance
// These eliminate function call overhead and use direct memory access

// Optimized hash function for better distribution
static inline uint32_t hash_ps_optimized(uint32_t predicate, uint32_t subject)
{
  // Use a better hash function for fewer collisions
  uint32_t hash = predicate ^ (subject << 16) ^ (subject >> 16);
  hash = ((hash << 13) ^ hash) ^ ((hash >> 17) ^ hash);
  hash = ((hash << 5) ^ hash) + 0x6ed9eb1;
  return hash;
}

// Ultra-fast property existence check (inline, no function call overhead)
static inline int shacl_has_property_fast(EngineState *engine, uint32_t subject_id, uint32_t predicate_id)
{
  // Direct memory access to avoid function calls
  PSOHashTable *table = (PSOHashTable *)engine->ps_to_o_index;
  uint32_t hash = hash_ps_optimized(predicate_id, subject_id);
  uint32_t index = hash % HASH_TABLE_SIZE;

  // Single probe for common case (80/20 rule)
  if (table->entries[index].predicate == predicate_id &&
      table->entries[index].subject == subject_id)
  {
    return table->entries[index].count > 0;
  }

  // Fallback to full search only if needed
  for (int i = 1; i < 4; i++)
  { // Limit to 3 additional probes
    index = (hash + i) % HASH_TABLE_SIZE;
    if (table->entries[index].predicate == predicate_id &&
        table->entries[index].subject == subject_id)
    {
      return table->entries[index].count > 0;
    }
    if (table->entries[index].predicate == 0 && table->entries[index].subject == 0)
    {
      break; // Not found
    }
  }

  return 0;
}

// Ultra-fast property count check (inline)
static inline uint32_t shacl_count_property_fast(EngineState *engine, uint32_t subject_id, uint32_t predicate_id)
{
  PSOHashTable *table = (PSOHashTable *)engine->ps_to_o_index;
  uint32_t hash = hash_ps_optimized(predicate_id, subject_id);
  uint32_t index = hash % HASH_TABLE_SIZE;

  // Single probe for common case
  if (table->entries[index].predicate == predicate_id &&
      table->entries[index].subject == subject_id)
  {
    return table->entries[index].count;
  }

  // Fallback to limited search
  for (int i = 1; i < 4; i++)
  {
    index = (hash + i) % HASH_TABLE_SIZE;
    if (table->entries[index].predicate == predicate_id &&
        table->entries[index].subject == subject_id)
    {
      return table->entries[index].count;
    }
    if (table->entries[index].predicate == 0 && table->entries[index].subject == 0)
    {
      break;
    }
  }

  return 0;
}

// Ultra-fast min_count check (inline)
static inline int shacl_min_count_fast(EngineState *engine, uint32_t subject_id, uint32_t predicate_id, uint32_t min_count)
{
  if (min_count == 0)
    return 1; // Always true for min_count 0

  uint32_t count = shacl_count_property_fast(engine, subject_id, predicate_id);
  return count >= min_count;
}

// Ultra-fast max_count check (inline)
static inline int shacl_max_count_fast(EngineState *engine, uint32_t subject_id, uint32_t predicate_id, uint32_t max_count)
{
  if (max_count == 0)
    return shacl_count_property_fast(engine, subject_id, predicate_id) == 0;

  uint32_t count = shacl_count_property_fast(engine, subject_id, predicate_id);
  return count <= max_count;
}

// Ultra-fast class check (inline, direct array access)
static inline int shacl_class_fast(EngineState *engine, uint32_t subject_id, uint32_t class_id)
{
  // Direct array access - no bounds checking for speed
  return engine->object_type_ids[subject_id] == class_id;
}

// Ultra-fast pattern matching (inline)
static inline int s7t_ask_pattern_fast(EngineState *engine, uint32_t s, uint32_t p, uint32_t o)
{
  // For o == 0 (wildcard), just check if property exists
  if (o == 0)
  {
    return shacl_has_property_fast(engine, s, p);
  }

  // For specific object, use optimized lookup
  PSOHashTable *table = (PSOHashTable *)engine->ps_to_o_index;
  uint32_t hash = hash_ps_optimized(p, s);
  uint32_t index = hash % HASH_TABLE_SIZE;

  // Single probe for common case
  if (table->entries[index].predicate == p && table->entries[index].subject == s)
  {
    // Check if object exists in the objects array
    for (size_t i = 0; i < table->entries[index].count; i++)
    {
      if (table->entries[index].objects[i] == o)
      {
        return 1;
      }
    }
    return 0;
  }

  // Fallback to limited search
  for (int i = 1; i < 4; i++)
  {
    index = (hash + i) % HASH_TABLE_SIZE;
    if (table->entries[index].predicate == p && table->entries[index].subject == s)
    {
      for (size_t j = 0; j < table->entries[index].count; j++)
      {
        if (table->entries[index].objects[j] == o)
        {
          return 1;
        }
      }
      return 0;
    }
    if (table->entries[index].predicate == 0 && table->entries[index].subject == 0)
    {
      break;
    }
  }

  return 0;
}

// Ultra-fast complete SHACL validation (inline, single function)
static inline int shacl_validate_fast(EngineState *engine, uint32_t subject_id, uint32_t class_id,
                                      uint32_t *properties, uint32_t property_count,
                                      uint32_t min_count, uint32_t max_count)
{
  // Check class first (fastest check)
  if (!shacl_class_fast(engine, subject_id, class_id))
  {
    return 0;
  }

  // Check required properties
  for (uint32_t i = 0; i < property_count; i++)
  {
    if (!shacl_has_property_fast(engine, subject_id, properties[i]))
    {
      return 0;
    }
  }

  // Check min_count constraints
  if (min_count > 0)
  {
    for (uint32_t i = 0; i < property_count; i++)
    {
      if (!shacl_min_count_fast(engine, subject_id, properties[i], min_count))
      {
        return 0;
      }
    }
  }

  // Check max_count constraints
  if (max_count > 0)
  {
    for (uint32_t i = 0; i < property_count; i++)
    {
      if (!shacl_max_count_fast(engine, subject_id, properties[i], max_count))
      {
        return 0;
      }
    }
  }

  return 1;
}

#endif // SEVEN_T_RUNTIME_OPTIMIZED_H