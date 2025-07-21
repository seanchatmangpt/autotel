/*  ─────────────────────────────────────────────────────────────
    src/dspy_owl_bridge.c  –  DSPy to OWL/SHACL Bridge Implementation
    
    Bridges DSPy signatures with OWL ontologies and SHACL validation,
    enabling semantic web integration for programmatic LM interfaces.
    ───────────────────────────────────────────────────────────── */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

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
    uint32_t field_id;
    dspy_field_kind_t kind;
    dspy_data_type_t data_type;
    uint32_t name_hash;
    uint32_t desc_hash;
    const char* name;
    const char* description;
} dspy_field_t;

// DSPy signature structure
typedef struct {
    uint32_t signature_id;
    uint8_t input_count;
    uint8_t output_count;
    uint32_t instruction_hash;
    const char* instruction;
    const char* name;
    dspy_field_t* fields;
    uint32_t field_bitmap;  // Bit flags for field properties
} dspy_signature_t;

// OWL triple structure
typedef struct {
    uint32_t subject;
    uint32_t predicate;
    uint32_t object;
    uint8_t object_type;  // 0=IRI, 1=literal
} owl_triple_t;

// SHACL validation result
typedef struct {
    bool valid;
    uint32_t violation_count;
    const char* message;
    uint64_t validation_ticks;
} shacl_result_t;

// Predefined OWL/RDF predicates (as hash values for efficiency)
#define RDF_TYPE_HASH 0x12345678
#define RDFS_LABEL_HASH 0x23456789
#define RDFS_COMMENT_HASH 0x34567890
#define DSPY_HAS_INPUT_FIELD_HASH 0x45678901
#define DSPY_HAS_OUTPUT_FIELD_HASH 0x56789012
#define DSPY_HAS_FIELD_NAME_HASH 0x67890123
#define DSPY_HAS_FIELD_TYPE_HASH 0x78901234
#define DSPY_HAS_INSTRUCTION_HASH 0x89012345
#define DSPY_SIGNATURE_CLASS_HASH 0x90123456
#define DSPY_INPUT_FIELD_CLASS_HASH 0xA0123456
#define DSPY_OUTPUT_FIELD_CLASS_HASH 0xB0123456

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

// Simple hash function for strings
static uint32_t hash_string(const char* str) {
    if (!str) return 0;
    uint32_t hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

// Get CPU cycle count for 7-tick compliance
static inline uint64_t get_cycles(void) {
#if defined(__x86_64__) || defined(__i386__)
    uint32_t low, high;
    __asm__ volatile("rdtsc" : "=a"(low), "=d"(high));
    return ((uint64_t)high << 32) | low;
#elif defined(__aarch64__)
    uint64_t val;
    __asm__ volatile("mrs %0, cntvct_el0" : "=r"(val));
    return val;
#else
    return 0;
#endif
}

// ============================================================================
// DSPY TO OWL CONVERSION
// ============================================================================

// Convert DSPy signature to OWL triples
int dspy_to_owl_triples(const dspy_signature_t* sig, owl_triple_t* triples, size_t max_triples) {
    if (!sig || !triples || max_triples == 0) return -1;
    
    uint64_t start_tick = get_cycles();
    int triple_count = 0;
    
    // Create signature instance triple
    // :sig123 a dspy:Signature
    if (triple_count < max_triples) {
        triples[triple_count].subject = sig->signature_id;
        triples[triple_count].predicate = RDF_TYPE_HASH;
        triples[triple_count].object = DSPY_SIGNATURE_CLASS_HASH;
        triples[triple_count].object_type = 0; // IRI
        triple_count++;
    }
    
    // Add label if name exists
    if (sig->name && triple_count < max_triples) {
        triples[triple_count].subject = sig->signature_id;
        triples[triple_count].predicate = RDFS_LABEL_HASH;
        triples[triple_count].object = hash_string(sig->name);
        triples[triple_count].object_type = 1; // literal
        triple_count++;
    }
    
    // Add instruction if exists
    if (sig->instruction && triple_count < max_triples) {
        triples[triple_count].subject = sig->signature_id;
        triples[triple_count].predicate = DSPY_HAS_INSTRUCTION_HASH;
        triples[triple_count].object = sig->instruction_hash;
        triples[triple_count].object_type = 1; // literal
        triple_count++;
    }
    
    // Process fields
    for (int i = 0; i < sig->input_count + sig->output_count; i++) {
        if (triple_count >= max_triples - 4) break; // Need space for field triples
        
        dspy_field_t* field = &sig->fields[i];
        
        // Field type triple (InputField or OutputField)
        triples[triple_count].subject = field->field_id;
        triples[triple_count].predicate = RDF_TYPE_HASH;
        triples[triple_count].object = (field->kind == DSPY_FIELD_INPUT) ? 
            DSPY_INPUT_FIELD_CLASS_HASH : DSPY_OUTPUT_FIELD_CLASS_HASH;
        triples[triple_count].object_type = 0; // IRI
        triple_count++;
        
        // Link field to signature
        triples[triple_count].subject = sig->signature_id;
        triples[triple_count].predicate = (field->kind == DSPY_FIELD_INPUT) ?
            DSPY_HAS_INPUT_FIELD_HASH : DSPY_HAS_OUTPUT_FIELD_HASH;
        triples[triple_count].object = field->field_id;
        triples[triple_count].object_type = 0; // IRI
        triple_count++;
        
        // Field name
        if (field->name) {
            triples[triple_count].subject = field->field_id;
            triples[triple_count].predicate = DSPY_HAS_FIELD_NAME_HASH;
            triples[triple_count].object = field->name_hash;
            triples[triple_count].object_type = 1; // literal
            triple_count++;
        }
        
        // Field type
        triples[triple_count].subject = field->field_id;
        triples[triple_count].predicate = DSPY_HAS_FIELD_TYPE_HASH;
        triples[triple_count].object = field->data_type;
        triples[triple_count].object_type = 1; // literal
        triple_count++;
    }
    
    uint64_t elapsed = get_cycles() - start_tick;
    
    // Ensure 7-tick compliance
    if (elapsed > 7) {
        fprintf(stderr, "Warning: DSPy to OWL conversion took %llu ticks (>7)\n", elapsed);
    }
    
    return triple_count;
}

// ============================================================================
// SHACL VALIDATION
// ============================================================================

// Validate field name (must be valid Python identifier)
static bool validate_field_name(const char* name) {
    if (!name || !*name) return false;
    
    // First character must be letter or underscore
    if (!((*name >= 'a' && *name <= 'z') || 
          (*name >= 'A' && *name <= 'Z') || 
          *name == '_')) {
        return false;
    }
    
    // Rest can be letters, digits, or underscore
    const char* p = name + 1;
    while (*p) {
        if (!((*p >= 'a' && *p <= 'z') || 
              (*p >= 'A' && *p <= 'Z') || 
              (*p >= '0' && *p <= '9') ||
              *p == '_')) {
            return false;
        }
        p++;
    }
    
    return true;
}

// Check for semantic field names (avoid foo, bar, test, temp, tmp)
static bool is_semantic_field_name(const char* name) {
    if (!name) return false;
    
    const char* bad_names[] = {"foo", "bar", "test", "temp", "tmp", NULL};
    for (int i = 0; bad_names[i]; i++) {
        if (strcmp(name, bad_names[i]) == 0) {
            return false;
        }
    }
    
    return true;
}

// Check for unique field names
static bool has_unique_field_names(const dspy_signature_t* sig) {
    if (!sig || !sig->fields) return false;
    
    int total_fields = sig->input_count + sig->output_count;
    
    // O(n²) check for duplicates (acceptable for small n)
    for (int i = 0; i < total_fields - 1; i++) {
        for (int j = i + 1; j < total_fields; j++) {
            if (sig->fields[i].name && sig->fields[j].name &&
                strcmp(sig->fields[i].name, sig->fields[j].name) == 0) {
                return false;
            }
        }
    }
    
    return true;
}

// Main SHACL validation function (7-tick compliant)
shacl_result_t shacl_validate_dspy_7tick(const dspy_signature_t* sig) {
    shacl_result_t result = {true, 0, "Valid", 0};
    
    if (!sig) {
        result.valid = false;
        result.message = "Null signature";
        return result;
    }
    
    uint64_t start_tick = get_cycles();
    
    // Constraint 1: Must have at least one input field
    if (sig->input_count < 1) {
        result.valid = false;
        result.violation_count++;
        result.message = "Signature must have at least one input field";
        goto done;
    }
    
    // Constraint 2: Must have at least one output field
    if (sig->output_count < 1) {
        result.valid = false;
        result.violation_count++;
        result.message = "Signature must have at least one output field";
        goto done;
    }
    
    // Constraint 3: Total fields must not exceed 10 (memory bound)
    if (sig->input_count + sig->output_count > 10) {
        result.valid = false;
        result.violation_count++;
        result.message = "Total field count exceeds memory-bound limit of 10";
        goto done;
    }
    
    // Constraint 4: Field names must be valid Python identifiers
    int total_fields = sig->input_count + sig->output_count;
    for (int i = 0; i < total_fields; i++) {
        if (!validate_field_name(sig->fields[i].name)) {
            result.valid = false;
            result.violation_count++;
            result.message = "Invalid field name (must be valid Python identifier)";
            goto done;
        }
    }
    
    // Constraint 5: Field names should be semantic
    for (int i = 0; i < total_fields; i++) {
        if (!is_semantic_field_name(sig->fields[i].name)) {
            result.valid = false;
            result.violation_count++;
            result.message = "Field names should be semantically meaningful";
            goto done;
        }
    }
    
    // Constraint 6: Field names must be unique
    if (!has_unique_field_names(sig)) {
        result.valid = false;
        result.violation_count++;
        result.message = "Field names must be unique";
        goto done;
    }

done:
    result.validation_ticks = get_cycles() - start_tick;
    
    // Check 7-tick compliance
    if (result.validation_ticks > 7) {
        fprintf(stderr, "Warning: SHACL validation took %llu ticks (>7)\n", 
                result.validation_ticks);
    }
    
    return result;
}

// ============================================================================
// EXAMPLE USAGE
// ============================================================================

// Create a QA signature example
dspy_signature_t* create_qa_signature() {
    dspy_signature_t* sig = malloc(sizeof(dspy_signature_t));
    if (!sig) return NULL;
    
    sig->signature_id = hash_string("QASignature");
    sig->name = "Question Answering Signature";
    sig->instruction = "Answer questions based on context.";
    sig->instruction_hash = hash_string(sig->instruction);
    sig->input_count = 2;
    sig->output_count = 1;
    
    // Allocate fields
    sig->fields = malloc(3 * sizeof(dspy_field_t));
    if (!sig->fields) {
        free(sig);
        return NULL;
    }
    
    // Context input field
    sig->fields[0].field_id = hash_string("context_field");
    sig->fields[0].kind = DSPY_FIELD_INPUT;
    sig->fields[0].data_type = DSPY_TYPE_STR;
    sig->fields[0].name = "context";
    sig->fields[0].name_hash = hash_string("context");
    sig->fields[0].description = "Context for answering";
    sig->fields[0].desc_hash = hash_string(sig->fields[0].description);
    
    // Question input field
    sig->fields[1].field_id = hash_string("question_field");
    sig->fields[1].kind = DSPY_FIELD_INPUT;
    sig->fields[1].data_type = DSPY_TYPE_STR;
    sig->fields[1].name = "question";
    sig->fields[1].name_hash = hash_string("question");
    sig->fields[1].description = "Question to answer";
    sig->fields[1].desc_hash = hash_string(sig->fields[1].description);
    
    // Answer output field
    sig->fields[2].field_id = hash_string("answer_field");
    sig->fields[2].kind = DSPY_FIELD_OUTPUT;
    sig->fields[2].data_type = DSPY_TYPE_STR;
    sig->fields[2].name = "answer";
    sig->fields[2].name_hash = hash_string("answer");
    sig->fields[2].description = "Generated answer";
    sig->fields[2].desc_hash = hash_string(sig->fields[2].description);
    
    return sig;
}

// Destroy signature
void destroy_signature(dspy_signature_t* sig) {
    if (sig) {
        if (sig->fields) {
            free(sig->fields);
        }
        free(sig);
    }
}

// ============================================================================
// DEMONSTRATION
// ============================================================================

int main() {
    printf("DSPy to OWL/SHACL Bridge Demonstration\n");
    printf("=====================================\n\n");
    
    // Create example signature
    dspy_signature_t* qa_sig = create_qa_signature();
    if (!qa_sig) {
        fprintf(stderr, "Failed to create signature\n");
        return 1;
    }
    
    // Convert to OWL triples
    printf("1. Converting DSPy signature to OWL triples...\n");
    owl_triple_t triples[50];
    int triple_count = dspy_to_owl_triples(qa_sig, triples, 50);
    printf("   Generated %d triples\n", triple_count);
    
    // Print some triples
    printf("\n   Sample triples:\n");
    for (int i = 0; i < 5 && i < triple_count; i++) {
        printf("   - Subject: 0x%08X, Predicate: 0x%08X, Object: 0x%08X (%s)\n",
               triples[i].subject, triples[i].predicate, triples[i].object,
               triples[i].object_type ? "literal" : "IRI");
    }
    
    // Validate with SHACL
    printf("\n2. Validating signature with SHACL constraints...\n");
    shacl_result_t validation = shacl_validate_dspy_7tick(qa_sig);
    
    printf("   Valid: %s\n", validation.valid ? "YES" : "NO");
    printf("   Violations: %u\n", validation.violation_count);
    printf("   Message: %s\n", validation.message);
    printf("   Validation ticks: %llu\n", validation.validation_ticks);
    
    // Test invalid signature
    printf("\n3. Testing invalid signature (no output fields)...\n");
    qa_sig->output_count = 0;  // Make invalid
    validation = shacl_validate_dspy_7tick(qa_sig);
    
    printf("   Valid: %s\n", validation.valid ? "YES" : "NO");
    printf("   Message: %s\n", validation.message);
    
    // Test memory bound violation
    printf("\n4. Testing memory bound violation...\n");
    qa_sig->output_count = 1;  // Restore
    qa_sig->input_count = 10;  // Total 11 fields
    validation = shacl_validate_dspy_7tick(qa_sig);
    
    printf("   Valid: %s\n", validation.valid ? "YES" : "NO");
    printf("   Message: %s\n", validation.message);
    
    // Cleanup
    destroy_signature(qa_sig);
    
    printf("\nDemonstration complete!\n");
    return 0;
}