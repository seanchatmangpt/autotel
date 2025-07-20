#ifndef CNS_SEMANTIC_STACK_H
#define CNS_SEMANTIC_STACK_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// CNS Semantic Stack - Integrated TTL/OWL/SHACL/SPARQL
// ============================================================================

// Forward declarations
typedef struct CNSSemanticStack CNSSemanticStack;
typedef struct SPARQLResultSet SPARQLResultSet;
typedef struct SHACLValidationResult SHACLValidationResult;

// Result structures
struct SPARQLResultSet {
    uint32_t *results;
    size_t result_count;
    size_t result_capacity;
};

struct SHACLValidationResult {
    uint32_t violation_count;
    uint32_t hop_count;
    uint64_t reasoning_cycles;
};

// ============================================================================
// Core API Functions
// ============================================================================

// Lifecycle management
CNSSemanticStack* cns_semantic_stack_create(size_t max_entities);
void cns_semantic_stack_destroy(CNSSemanticStack *stack);

// TTL parsing
int cns_semantic_stack_load_ttl(CNSSemanticStack *stack, const char *filename);
int cns_semantic_stack_parse_ttl_string(CNSSemanticStack *stack, const char *ttl_content);

// OWL reasoning
int cns_semantic_stack_add_subclass(CNSSemanticStack *stack, 
                                   const char *child_iri, 
                                   const char *parent_iri);
int cns_semantic_stack_add_equivalent_class(CNSSemanticStack *stack,
                                           const char *class1_iri,
                                           const char *class2_iri);
int cns_semantic_stack_set_transitive_property(CNSSemanticStack *stack,
                                              const char *property_iri);
int cns_semantic_stack_materialize_inferences(CNSSemanticStack *stack);

// SHACL validation
int cns_semantic_stack_define_shape(CNSSemanticStack *stack,
                                   const char *shape_iri,
                                   const char *target_class_iri);
int cns_semantic_stack_add_min_count_constraint(CNSSemanticStack *stack,
                                               const char *shape_iri,
                                               const char *property_iri,
                                               uint32_t min_count);
SHACLValidationResult* cns_semantic_stack_validate_node(CNSSemanticStack *stack,
                                                       const char *node_iri);
void cns_semantic_stack_free_validation_result(SHACLValidationResult *result);

// SPARQL queries
SPARQLResultSet* cns_semantic_stack_query(CNSSemanticStack *stack,
                                         const char *sparql_query);
void cns_semantic_stack_free_result_set(SPARQLResultSet *results);

// String resolution (for result interpretation)
const char* cns_semantic_stack_get_string(CNSSemanticStack *stack, uint32_t id);

// Performance optimization
void cns_semantic_stack_enable_dark_patterns(CNSSemanticStack *stack, bool enable);
void cns_semantic_stack_enable_ahi_perception(CNSSemanticStack *stack, bool enable);
void cns_semantic_stack_enable_8hop_cycles(CNSSemanticStack *stack, bool enable);

// AOT compilation
int cns_semantic_stack_compile_ontology(CNSSemanticStack *stack,
                                       const char *ttl_file,
                                       const char *output_file);

// Performance metrics
typedef struct {
    uint64_t ttl_parse_cycles;
    uint64_t owl_reasoning_cycles;
    uint64_t shacl_validation_cycles;
    uint64_t sparql_query_cycles;
    uint64_t total_cycles;
} CNSSemanticMetrics;

void cns_semantic_stack_get_metrics(CNSSemanticStack *stack, CNSSemanticMetrics *metrics);
void cns_semantic_stack_print_metrics(CNSSemanticStack *stack);

// ============================================================================
// Integration Helpers
// ============================================================================

// Binary materialization integration
int cns_semantic_stack_export_graph(CNSSemanticStack *stack, const char *filename);
int cns_semantic_stack_import_graph(CNSSemanticStack *stack, const char *filename);

// 7-tick compliance checking
bool cns_semantic_stack_check_7tick_compliance(CNSSemanticStack *stack);

#ifdef __cplusplus
}
#endif

#endif // CNS_SEMANTIC_STACK_H