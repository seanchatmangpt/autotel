#ifndef CNS_SHACL_H
#define CNS_SHACL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "cns/engines/sparql.h"

// Branch prediction hints for optimal performance
#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

// SHACL constraint types
typedef enum
{
  CNS_SHACL_MIN_COUNT,
  CNS_SHACL_MAX_COUNT,
  CNS_SHACL_CLASS,
  CNS_SHACL_DATATYPE,
  CNS_SHACL_PATTERN,
  CNS_SHACL_RANGE
} CNSShaclConstraintType;

// SHACL constraint structure
typedef struct
{
  CNSShaclConstraintType type;
  uint32_t property_id;
  uint32_t value;     // For min_count, max_count, etc.
  char *string_value; // For pattern, datatype, etc.
} CNSShaclConstraint;

// SHACL shape structure
typedef struct
{
  uint32_t shape_id;
  uint32_t target_class;
  CNSShaclConstraint *constraints;
  size_t constraint_count;
  size_t constraint_capacity;
} CNSShaclShape;

// CNS SHACL engine structure
typedef struct
{
  CNSSparqlEngine *sparql_engine;
  CNSShaclShape *shapes;
  size_t shape_count;
  size_t shape_capacity;
  size_t memory_usage;
} CNSShaclEngine;

// Function declarations

// Engine creation and destruction
CNSShaclEngine *cns_shacl_create(CNSSparqlEngine *sparql_engine);
void cns_shacl_destroy(CNSShaclEngine *engine);

// Shape management
int cns_shacl_define_shape(CNSShaclEngine *engine, uint32_t shape_id, uint32_t target_class);
int cns_shacl_add_constraint(CNSShaclEngine *engine, uint32_t shape_id, CNSShaclConstraint *constraint);

// Validation operations (7-tick optimized)
bool cns_shacl_validate_node(CNSShaclEngine *engine, uint32_t node_id);
bool cns_shacl_check_min_count(CNSShaclEngine *engine, uint32_t node_id, uint32_t property_id, uint32_t min_count);
bool cns_shacl_check_max_count(CNSShaclEngine *engine, uint32_t node_id, uint32_t property_id, uint32_t max_count);
bool cns_shacl_check_class(CNSShaclEngine *engine, uint32_t node_id, uint32_t class_id);

// Performance monitoring
uint64_t cns_shacl_get_cycles(void);
void cns_shacl_measure_validation_cycles(CNSShaclEngine *engine, uint32_t node_id);

// Memory management
size_t cns_shacl_get_memory_usage(CNSShaclEngine *engine);

#endif // CNS_SHACL_H