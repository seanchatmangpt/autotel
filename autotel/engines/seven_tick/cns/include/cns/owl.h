#ifndef CNS_OWL_H
#define CNS_OWL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <time.h>

// ============================================================================
// CNS OWL ENGINE - 7T COMPLIANT ONTOLOGY REASONING
// ============================================================================

// OWL Axiom Types (optimized for 7T)
typedef enum
{
  OWL_SUBCLASS_OF = 1,        // rdfs:subClassOf
  OWL_EQUIVALENT_CLASS = 2,   // owl:equivalentClass
  OWL_DISJOINT_WITH = 3,      // owl:disjointWith
  OWL_TRANSITIVE = 4,         // owl:TransitiveProperty
  OWL_SYMMETRIC = 5,          // owl:SymmetricProperty
  OWL_FUNCTIONAL = 6,         // owl:FunctionalProperty
  OWL_INVERSE_FUNCTIONAL = 7, // owl:InverseFunctionalProperty
  OWL_DOMAIN = 8,             // rdfs:domain
  OWL_RANGE = 9,              // rdfs:range
  OWL_INVERSE_OF = 10,        // owl:inverseOf
  OWL_SAME_AS = 11,           // owl:sameAs
  OWL_DIFFERENT_FROM = 12     // owl:differentFrom
} OWLAxiomType;

// OWL Axiom Structure (optimized for cache locality)
typedef struct
{
  uint32_t subject_id;   // Subject entity ID
  uint32_t predicate_id; // Predicate/relation ID
  uint32_t object_id;    // Object entity ID
  uint8_t axiom_type;    // OWLAxiomType
  uint8_t materialized;  // Materialization flag
  uint16_t tick_cost;    // CPU cycles for reasoning
} OWLAxiom;

// OWL Engine Structure (7T optimized)
typedef struct
{
  // Core data structures
  OWLAxiom *axioms;      // Array of OWL axioms
  size_t axiom_count;    // Number of axioms
  size_t axiom_capacity; // Capacity of axioms array

  // Materialized inferences (bit-vector based for 7T compliance)
  uint64_t *class_hierarchy;    // Class hierarchy matrix
  uint64_t *property_matrix;    // Property characteristics matrix
  uint64_t *transitive_closure; // Transitive closure matrix

  // Entity mappings (ID-based for 7T compliance)
  uint32_t *class_ids;    // Class entity IDs
  uint32_t *property_ids; // Property entity IDs
  size_t class_count;     // Number of classes
  size_t property_count;  // Number of properties

  // Performance metrics
  uint64_t reasoning_cycles;       // Total cycles spent on reasoning
  uint64_t materialization_cycles; // Cycles for materialization
  uint32_t inference_count;        // Number of inferences made

  // 80/20 optimization flags
  bool use_80_20_materialization; // Use optimized materialization
  bool use_80_20_reasoning;       // Use optimized reasoning
  bool precompute_closures;       // Precompute transitive closures
} CNSOWLEngine;

// ============================================================================
// CNS OWL API FUNCTIONS
// ============================================================================

// Engine lifecycle
CNSOWLEngine *cns_owl_create(size_t initial_capacity);
void cns_owl_destroy(CNSOWLEngine *engine);

// Axiom management
int cns_owl_add_axiom(CNSOWLEngine *engine, uint32_t subject, uint32_t predicate,
                      uint32_t object, OWLAxiomType type);
int cns_owl_add_subclass(CNSOWLEngine *engine, uint32_t child, uint32_t parent);
int cns_owl_add_equivalent_class(CNSOWLEngine *engine, uint32_t class1, uint32_t class2);
int cns_owl_add_disjoint_class(CNSOWLEngine *engine, uint32_t class1, uint32_t class2);

// Property characteristics
int cns_owl_set_transitive(CNSOWLEngine *engine, uint32_t property);
int cns_owl_set_symmetric(CNSOWLEngine *engine, uint32_t property);
int cns_owl_set_functional(CNSOWLEngine *engine, uint32_t property);
int cns_owl_set_inverse_functional(CNSOWLEngine *engine, uint32_t property);

// Domain and range
int cns_owl_set_domain(CNSOWLEngine *engine, uint32_t property, uint32_t domain);
int cns_owl_set_range(CNSOWLEngine *engine, uint32_t property, uint32_t range);

// Reasoning queries (7T compliant - ≤7 cycles)
bool cns_owl_is_subclass_of(CNSOWLEngine *engine, uint32_t child, uint32_t parent);
bool cns_owl_is_equivalent_class(CNSOWLEngine *engine, uint32_t class1, uint32_t class2);
bool cns_owl_is_disjoint_with(CNSOWLEngine *engine, uint32_t class1, uint32_t class2);
bool cns_owl_has_property_characteristic(CNSOWLEngine *engine, uint32_t property, OWLAxiomType characteristic);

// Transitive reasoning
bool cns_owl_transitive_query(CNSOWLEngine *engine, uint32_t subject, uint32_t property, uint32_t object);
int cns_owl_materialize_transitive_closure(CNSOWLEngine *engine, uint32_t property);

// Materialization (80/20 optimized)
int cns_owl_materialize_inferences(CNSOWLEngine *engine);
int cns_owl_materialize_inferences_80_20(CNSOWLEngine *engine);

// Performance monitoring
uint64_t cns_owl_get_reasoning_cycles(CNSOWLEngine *engine);
uint64_t cns_owl_get_materialization_cycles(CNSOWLEngine *engine);
uint32_t cns_owl_get_inference_count(CNSOWLEngine *engine);

// 80/20 optimization control
void cns_owl_enable_80_20_optimizations(CNSOWLEngine *engine, bool enable);
void cns_owl_set_precompute_closures(CNSOWLEngine *engine, bool enable);

// ============================================================================
// 7T COMPLIANCE MACROS
// ============================================================================

// Ensure reasoning operations complete in ≤7 cycles
#define CNS_OWL_7T_REASONING(engine, operation)                                                         \
  do                                                                                                    \
  {                                                                                                     \
    uint64_t start_cycles = cns_get_cycles();                                                           \
    operation;                                                                                          \
    uint64_t end_cycles = cns_get_cycles();                                                             \
    uint64_t cycles = end_cycles - start_cycles;                                                        \
    if (cycles > 7)                                                                                     \
    {                                                                                                   \
      cns_log_warning("OWL reasoning exceeded 7-cycle limit: %llu cycles", (unsigned long long)cycles); \
    }                                                                                                   \
  } while (0)

// ============================================================================
// INTERNAL HELPER FUNCTIONS
// ============================================================================

// Bit-vector operations for 7T compliance
static inline uint64_t cns_owl_get_bit(uint64_t *matrix, size_t row, size_t col)
{
  return (matrix[row] >> col) & 1ULL;
}

static inline void cns_owl_set_bit(uint64_t *matrix, size_t row, size_t col)
{
  matrix[row] |= (1ULL << col);
}

static inline void cns_owl_clear_bit(uint64_t *matrix, size_t row, size_t col)
{
  matrix[row] &= ~(1ULL << col);
}

// Cycle counting (platform-specific)
static inline uint64_t cns_get_cycles(void)
{
#if defined(__x86_64__) || defined(__i386__)
  uint32_t lo, hi;
  __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
  return ((uint64_t)hi << 32) | lo;
#else
  // Fallback for other architectures
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
#endif
}

// Logging (placeholder - integrate with CNS logging)
static inline void cns_log_warning(const char *format, ...)
{
  // TODO: Integrate with CNS logging system
  (void)format;
}

#endif // CNS_OWL_H