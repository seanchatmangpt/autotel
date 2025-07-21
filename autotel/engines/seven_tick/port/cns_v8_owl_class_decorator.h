/**
 * CNS v8 OWL Class Decorator Implementation
 * Gap 1 Solution: Automatic DSPy→OWL mapping with @owl_class decorator
 * 
 * This header provides the actual implementation of the @owl_class decorator
 * that makes DSPy signatures automatically become OWL entities in C memory.
 */

#ifndef CNS_V8_OWL_CLASS_DECORATOR_H
#define CNS_V8_OWL_CLASS_DECORATOR_H

#include "cns_v8_dspy_owl_native_bridge.h"
#include <stdint.h>
#include <stdbool.h>

// ========================================
// CORE DECORATOR INFRASTRUCTURE
// ========================================

// Metadata for OWL class generation
typedef struct {
    char iri[256];                          // Full OWL class IRI
    char namespace_prefix[32];              // e.g., "dspy", "cns", "owl"
    char local_name[64];                    // e.g., "PatternRecognition"
    uint32_t field_mappings[16];            // DSPy field → OWL property mappings
    uint8_t automatic_properties;           // Auto-generate properties flag
    uint8_t inherit_owl_thing;              // Inherit from owl:Thing
    uint8_t enable_shacl_validation;        // Auto-generate SHACL shapes
} owl_class_metadata_t;

// Python-C bridge for decorator registration
typedef struct {
    char dspy_signature_name[128];          // Python class name
    owl_class_metadata_t owl_metadata;      // OWL generation parameters
    native_dspy_owl_entity_t* native_entity; // Generated C entity
    
    // Automatic field discovery
    struct {
        char python_field_names[16][64];    // Original DSPy field names
        char owl_property_iris[16][256];    // Generated OWL property IRIs
        uint8_t field_count;
        bool auto_generated;
    } field_mapping;
    
    // Runtime state
    bool is_registered;
    uint64_t creation_timestamp;
    uint32_t usage_count;
    
} owl_class_decorator_registration_t;

// Global registry for decorated DSPy signatures
typedef struct {
    owl_class_decorator_registration_t registrations[256];
    uint8_t registration_count;
    uint32_t registration_bitmap;           // Which slots are occupied
    
    // Automatic IRI generation
    struct {
        char base_namespace[128];           // "http://dspy.ai/ontology#"
        uint32_t auto_iri_counter;          // For unique IRI generation
        bool preserve_python_names;        // Use Python names in OWL
    } iri_generator;
    
    // Performance tracking
    struct {
        uint64_t decorations_processed;
        uint64_t owl_entities_generated;
        uint64_t properties_auto_created;
        cns_cycle_t avg_decoration_cycles;
    } metrics;
    
} owl_class_decorator_registry_t;

// ========================================
// DECORATOR API FUNCTIONS
// ========================================

// Initialize the decorator registry
int cns_v8_owl_decorator_init(
    owl_class_decorator_registry_t* registry,
    const char* base_namespace
);

// Register a DSPy signature with OWL metadata (called from Python)
int cns_v8_register_owl_class(
    owl_class_decorator_registry_t* registry,
    const char* python_signature_json,      // DSPy signature definition
    const owl_class_metadata_t* metadata,   // OWL class parameters
    native_dspy_owl_entity_t** out_entity   // Generated native entity
);

// Automatic OWL property generation from DSPy fields
int cns_v8_generate_owl_properties(
    const char* dspy_fields_json,           // DSPy field definitions
    const owl_class_metadata_t* metadata,   // Class metadata
    native_owl_field_t* properties,         // Output property array
    uint8_t* property_count                 // Number of properties generated
);

// Automatic SHACL shape generation for validation
int cns_v8_generate_shacl_shapes(
    const native_dspy_owl_entity_t* entity, // OWL entity
    native_shacl_state_t* shacl_state       // Output SHACL shapes
);

// Lookup decorated entity by Python signature name
native_dspy_owl_entity_t* cns_v8_find_owl_entity_by_name(
    const owl_class_decorator_registry_t* registry,
    const char* python_signature_name
);

// ========================================
// AUTOMATIC MAPPING FUNCTIONS
// ========================================

// Convert Python DSPy field types to OWL datatypes
uint8_t dspy_type_to_owl_datatype(const char* dspy_type);

// Generate OWL property IRI from DSPy field name
void generate_owl_property_iri(
    const char* field_name,
    const owl_class_metadata_t* metadata,
    char* property_iri,
    size_t iri_size
);

// Automatic constraint inference from DSPy field descriptions
uint8_t infer_shacl_constraints_from_description(
    const char* field_description,
    const char* field_type
);

// ========================================
// PYTHON INTEGRATION LAYER
// ========================================

// C functions callable from Python via ctypes
extern "C" {
    // Register decorator from Python
    int cns_v8_python_register_owl_class(
        const char* json_payload             // Complete registration data
    );
    
    // Query registered entities from Python
    const char* cns_v8_python_list_owl_entities(void);
    
    // Validate DSPy output against registered OWL entity
    bool cns_v8_python_validate_output(
        const char* signature_name,
        const char* dspy_output_json
    );
    
    // Get OWL/SHACL representation for debugging
    const char* cns_v8_python_export_owl_ttl(
        const char* signature_name
    );
}

// ========================================
// CONCRETE DECORATOR EXAMPLES
// ========================================

// Example 1: Pattern Recognition Signature
/*
Python code with decorator:

@owl_class("http://dspy.ai/ontology#PatternRecognition")
class PatternRecognition(dspy.Signature):
    """Classify RDF triples into 80/20 pattern categories"""
    triple = dspy.InputField(desc="RDF triple in N-Triples format")
    pattern_type = dspy.OutputField(desc="One of: type_decl, label, property, hierarchy, other")

Generated C representation:
*/
static const owl_class_metadata_t pattern_recognition_metadata = {
    .iri = "http://dspy.ai/ontology#PatternRecognition",
    .namespace_prefix = "dspy",
    .local_name = "PatternRecognition", 
    .automatic_properties = 1,
    .inherit_owl_thing = 1,
    .enable_shacl_validation = 1
};

// Example 2: Question Answering Signature
/*
Python code:

@owl_class("http://dspy.ai/ontology#QuestionAnswering",
           namespace="qa",
           auto_properties=True,
           shacl_validation=True)
class QuestionAnswering(dspy.Signature):
    question = dspy.InputField(desc="Question to answer")
    context = dspy.InputField(desc="Relevant context")
    answer = dspy.OutputField(desc="Generated answer")

Generated OWL properties:
- qa:hasQuestion (xsd:string, sh:minCount 1)
- qa:hasContext (xsd:string, sh:minCount 0)  
- qa:hasAnswer (xsd:string, sh:minCount 1)
*/

// ========================================
// REAL-TIME PROCESSING INTEGRATION
// ========================================

// Process decorated DSPy signature in 7-tick constraint
int cns_v8_process_decorated_signature_realtime(
    const owl_class_decorator_registry_t* registry,
    const char* signature_name,
    const char* dspy_input_json,
    char* dspy_output_json,
    size_t output_size,
    cns_cycle_t max_cycles
);

// Validate output against OWL/SHACL constraints in real-time
bool cns_v8_validate_decorated_output_realtime(
    const native_dspy_owl_entity_t* entity,
    const char* dspy_output_json,
    cns_cycle_t max_cycles,
    char* violation_details,
    size_t details_size
);

// Update entity based on usage patterns and feedback
void cns_v8_evolve_decorated_entity(
    native_dspy_owl_entity_t* entity,
    const cns_v8_usage_metrics_t* usage_stats,
    const cns_v8_validation_feedback_t* feedback
);

// ========================================
// PERFORMANCE MONITORING
// ========================================

typedef struct {
    uint64_t decorations_registered;
    uint64_t owl_entities_created;
    uint64_t properties_auto_generated;
    uint64_t shacl_shapes_created;
    
    cns_cycle_t avg_registration_cycles;
    cns_cycle_t avg_property_generation_cycles;
    cns_cycle_t avg_validation_cycles;
    
    double automation_success_rate;
    uint32_t manual_interventions_needed;
    
} owl_decorator_metrics_t;

void cns_v8_get_owl_decorator_metrics(
    const owl_class_decorator_registry_t* registry,
    owl_decorator_metrics_t* metrics
);

// ========================================
// CONFIGURATION AND CLEANUP
// ========================================

// Load decorator configuration from file
int cns_v8_load_decorator_config(
    owl_class_decorator_registry_t* registry,
    const char* config_file_path
);

// Export registered entities as OWL ontology
int cns_v8_export_decorated_ontology(
    const owl_class_decorator_registry_t* registry,
    char* turtle_output,
    size_t buffer_size
);

// Cleanup all decorator resources
void cns_v8_owl_decorator_cleanup(
    owl_class_decorator_registry_t* registry
);

#endif // CNS_V8_OWL_CLASS_DECORATOR_H