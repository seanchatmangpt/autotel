/*  ─────────────────────────────────────────────────────────────
    include/cns/dspy_owl_bridge.h  –  DSPy to OWL/SHACL Bridge API
    
    Header file for DSPy signature to OWL ontology and SHACL validation
    bridge, enabling semantic web integration for programmatic LM interfaces.
    ───────────────────────────────────────────────────────────── */

#ifndef CNS_DSPY_OWL_BRIDGE_H
#define CNS_DSPY_OWL_BRIDGE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// TYPE DEFINITIONS
// ============================================================================

// DSPy field types
typedef enum {
    DSPY_FIELD_INPUT = 0,
    DSPY_FIELD_OUTPUT = 1
} dspy_field_kind_t;

// DSPy data types
typedef enum {
    DSPY_TYPE_STR = 0,
    DSPY_TYPE_INT = 1,
    DSPY_TYPE_FLOAT = 2,
    DSPY_TYPE_BOOL = 3,
    DSPY_TYPE_LIST = 4,
    DSPY_TYPE_DICT = 5
} dspy_data_type_t;

// DSPy field structure
typedef struct {
    uint32_t field_id;              // Unique field identifier
    dspy_field_kind_t kind;         // Input or output field
    dspy_data_type_t data_type;     // Data type of the field
    uint32_t name_hash;             // Hash of field name
    uint32_t desc_hash;             // Hash of field description
    const char* name;               // Field name (Python identifier)
    const char* description;        // Field description
} dspy_field_t;

// DSPy signature structure
typedef struct {
    uint32_t signature_id;          // Unique signature identifier
    uint8_t input_count;            // Number of input fields
    uint8_t output_count;           // Number of output fields
    uint32_t instruction_hash;      // Hash of instruction text
    const char* instruction;        // Optional instruction text
    const char* name;               // Signature name
    dspy_field_t* fields;           // Array of fields
    uint32_t field_bitmap;          // Bit flags for field properties
} dspy_signature_t;

// OWL triple structure
typedef struct {
    uint32_t subject;               // Subject IRI hash
    uint32_t predicate;             // Predicate IRI hash
    uint32_t object;                // Object IRI hash or literal value
    uint8_t object_type;            // 0=IRI, 1=literal
} owl_triple_t;

// SHACL validation result
typedef struct {
    bool valid;                     // Overall validity
    uint32_t violation_count;       // Number of violations found
    const char* message;            // Validation message
    uint64_t validation_ticks;      // CPU ticks for validation
} shacl_result_t;

// SHACL constraint types for DSPy
typedef enum {
    SHACL_DSPY_MIN_INPUT = 0,       // Minimum input fields
    SHACL_DSPY_MIN_OUTPUT = 1,      // Minimum output fields
    SHACL_DSPY_MAX_FIELDS = 2,      // Maximum total fields
    SHACL_DSPY_FIELD_NAME = 3,      // Field name validation
    SHACL_DSPY_SEMANTIC_NAME = 4,   // Semantic field names
    SHACL_DSPY_UNIQUE_NAMES = 5,    // Unique field names
    SHACL_DSPY_TYPE_CONSISTENCY = 6 // Type consistency across signatures
} shacl_dspy_constraint_t;

// ============================================================================
// PREDEFINED CONSTANTS
// ============================================================================

// Maximum limits for 7-tick compliance
#define DSPY_MAX_FIELDS 10          // Maximum fields per signature
#define DSPY_MAX_NAME_LENGTH 64     // Maximum field name length
#define DSPY_MAX_DESC_LENGTH 256    // Maximum description length
#define DSPY_MAX_TRIPLES 50         // Maximum OWL triples per signature

// Predefined OWL/RDF predicates (as hash values for efficiency)
#define RDF_TYPE_HASH 0x12345678
#define RDFS_LABEL_HASH 0x23456789
#define RDFS_COMMENT_HASH 0x34567890
#define DSPY_HAS_INPUT_FIELD_HASH 0x45678901
#define DSPY_HAS_OUTPUT_FIELD_HASH 0x56789012
#define DSPY_HAS_FIELD_NAME_HASH 0x67890123
#define DSPY_HAS_FIELD_TYPE_HASH 0x78901234
#define DSPY_HAS_DESCRIPTION_HASH 0x89012345
#define DSPY_HAS_INSTRUCTION_HASH 0x90123456

// OWL class hashes
#define DSPY_SIGNATURE_CLASS_HASH 0xA0123456
#define DSPY_FIELD_CLASS_HASH 0xA1234567
#define DSPY_INPUT_FIELD_CLASS_HASH 0xA2345678
#define DSPY_OUTPUT_FIELD_CLASS_HASH 0xA3456789
#define DSPY_MODULE_CLASS_HASH 0xA4567890

// ============================================================================
// FUNCTION DECLARATIONS
// ============================================================================

// Signature creation and destruction
dspy_signature_t* dspy_signature_create(const char* name, const char* instruction);
void dspy_signature_destroy(dspy_signature_t* sig);

// Field management
int dspy_signature_add_input(dspy_signature_t* sig, const char* name, 
                            dspy_data_type_t type, const char* description);
int dspy_signature_add_output(dspy_signature_t* sig, const char* name,
                             dspy_data_type_t type, const char* description);
dspy_field_t* dspy_signature_get_field(const dspy_signature_t* sig, const char* name);

// OWL conversion
int dspy_to_owl_triples(const dspy_signature_t* sig, owl_triple_t* triples, 
                       size_t max_triples);
int dspy_to_turtle(const dspy_signature_t* sig, char* buffer, size_t buffer_size);

// SHACL validation
shacl_result_t shacl_validate_dspy_7tick(const dspy_signature_t* sig);
bool shacl_validate_field_name(const char* name);
bool shacl_validate_semantic_name(const char* name);
bool shacl_validate_unique_names(const dspy_signature_t* sig);
bool shacl_validate_memory_bound(const dspy_signature_t* sig);

// Utility functions
uint32_t dspy_hash_string(const char* str);
const char* dspy_type_to_string(dspy_data_type_t type);
const char* dspy_field_kind_to_string(dspy_field_kind_t kind);

// Predefined signature templates
dspy_signature_t* dspy_create_qa_signature(void);
dspy_signature_t* dspy_create_cot_signature(void);
dspy_signature_t* dspy_create_rag_signature(void);

// Advanced features
int dspy_signature_compose(const dspy_signature_t* sig1, 
                          const dspy_signature_t* sig2,
                          dspy_signature_t* result);
bool dspy_signatures_compatible(const dspy_signature_t* sig1,
                               const dspy_signature_t* sig2);

// Performance metrics
typedef struct {
    uint64_t conversion_ticks;      // Ticks for OWL conversion
    uint64_t validation_ticks;      // Ticks for SHACL validation
    uint32_t triple_count;          // Number of triples generated
    size_t memory_usage;            // Memory used in bytes
} dspy_performance_metrics_t;

dspy_performance_metrics_t dspy_get_performance_metrics(void);
void dspy_reset_performance_metrics(void);

#ifdef __cplusplus
}
#endif

#endif // CNS_DSPY_OWL_BRIDGE_H