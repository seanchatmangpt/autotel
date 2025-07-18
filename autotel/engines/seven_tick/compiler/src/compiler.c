#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include "qop.h"
#include "cjinja.h"

// Simple RDF parser structures
typedef struct {
    char* subject;
    char* predicate;
    char* object;
} ParsedTriple;

typedef struct {
    ParsedTriple* triples;
    size_t count;
    size_t capacity;
} ParsedDocument;

// Forward declarations
ParsedDocument* parse_ttl_file(const char* filename);
void free_parsed_document(ParsedDocument* doc);
char* generate_kernel_code(ParsedDocument* ontology, ParsedDocument* shapes, 
                          ParsedDocument* queries, CostModel* cost_model);
int compile_kernel(const char* c_code, const char* output_path);

int main(int argc, char** argv) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <ontology.ttl> <shapes.ttl> <queries.sparql> <output.so>\n", argv[0]);
        return 1;
    }
    
    #ifndef NDEBUG
    printf("7T Compiler - AOT Hardening System\n");
    printf("==================================\n");
    #endif
    
    // Parse specification files
    #ifndef NDEBUG
    printf("Parsing ontology: %s\n", argv[1]);
    #endif
    ParsedDocument* ontology = parse_ttl_file(argv[1]);
    
    #ifndef NDEBUG
    printf("Parsing shapes: %s\n", argv[2]);
    #endif
    ParsedDocument* shapes = parse_ttl_file(argv[2]);
    
    #ifndef NDEBUG
    printf("Parsing queries: %s\n", argv[3]);
    #endif
    ParsedDocument* queries = parse_ttl_file(argv[3]);
    
    // Create cost model
    #ifndef NDEBUG
    printf("Building cost model...\n");
    #endif
    CostModel* cost_model = create_cost_model(NULL);
    
    // Generate kernel code
    #ifndef NDEBUG
    printf("Generating optimized kernel...\n");
    #endif
    char* kernel_code = generate_kernel_code(ontology, shapes, queries, cost_model);
    
    // Write intermediate C file for debugging
    FILE* c_file = fopen("/tmp/seven_t_kernel.c", "w");
    fprintf(c_file, "%s", kernel_code);
    fclose(c_file);
    
    // Compile to shared library
    #ifndef NDEBUG
    printf("Compiling kernel to: %s\n", argv[4]);
    #endif
    int result = compile_kernel(kernel_code, argv[4]);
    
    // Cleanup
    free_parsed_document(ontology);
    free_parsed_document(shapes);
    free_parsed_document(queries);
    destroy_cost_model(cost_model);
    free(kernel_code);
    
    #ifndef NDEBUG
    if (result == 0) {
        printf("Compilation successful!\n");
    } else {
        printf("Compilation failed!\n");
    }
    #endif
    
    return result;
}
// Simple TTL parser for MVP
ParsedDocument* parse_ttl_file(const char* filename) {
    ParsedDocument* doc = malloc(sizeof(ParsedDocument));
    doc->capacity = 1024;
    doc->count = 0;
    doc->triples = malloc(doc->capacity * sizeof(ParsedTriple));
    
    FILE* f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "Cannot open file: %s\n", filename);
        return doc;
    }
    
    char line[4096];
    while (fgets(line, sizeof(line), f)) {
        // Skip comments and empty lines
        if (line[0] == '#' || line[0] == '@' || line[0] == '\n') continue;
        
        // Very simple triple parsing (for MVP)
        char* subject = strtok(line, " \t");
        if (!subject) continue;
        
        char* predicate = strtok(NULL, " \t");
        if (!predicate) continue;
        
        char* object = strtok(NULL, " \t\n");
        if (!object) continue;
        
        // Remove trailing dot
        size_t obj_len = strlen(object);
        if (obj_len > 0 && object[obj_len-1] == '.') {
            object[obj_len-1] = '\0';
        }
        
        // Grow array if needed
        if (doc->count >= doc->capacity) {
            doc->capacity *= 2;
            doc->triples = realloc(doc->triples, doc->capacity * sizeof(ParsedTriple));
        }
        
        // Store triple
        doc->triples[doc->count].subject = strdup(subject);
        doc->triples[doc->count].predicate = strdup(predicate);
        doc->triples[doc->count].object = strdup(object);
        doc->count++;
    }
    
    fclose(f);
    return doc;
}

void free_parsed_document(ParsedDocument* doc) {
    for (size_t i = 0; i < doc->count; i++) {
        free(doc->triples[i].subject);
        free(doc->triples[i].predicate);
        free(doc->triples[i].object);
    }
    free(doc->triples);
    free(doc);
}
// Code generation
char* generate_kernel_code(ParsedDocument* ontology, ParsedDocument* shapes, 
                          ParsedDocument* queries, CostModel* cost_model) {
    
    // Create template context
    CJinjaContext* ctx = cjinja_create_context();
    
    // For MVP, generate a simple kernel that demonstrates the concept
    const char* kernel_template = 
        "#include <stdint.h>\n"
        "#include <stddef.h>\n"
        "#include <stdlib.h>\n"
        "#include \"seven_t_runtime.h\"\n\n"
        "// Forward declaration\n"
        "uint32_t s7t_intern_string(EngineState* engine, const char* str);\n\n"
        "// Auto-generated 7T kernel\n"
        "// Ontology triples: {{ ontology_count }}\n"
        "// Shape constraints: {{ shape_count }}\n\n"
        "// Query 1: Find patients with appointments\n"
        "typedef struct {\n"
        "    uint32_t patient_id;\n"
        "    uint32_t appointment_id;\n"
        "    uint32_t doctor_id;\n"
        "} QueryResult;\n\n"
        "QueryResult* execute_query_1(EngineState* engine, size_t* result_count) {\n"
        "    // Look up predicates and classes by string\n"
        "    uint32_t pred_type = s7t_intern_string(engine, \"<http://www.w3.org/1999/02/22-rdf-syntax-ns#type>\");\n"
        "    uint32_t pred_hasAppointment = s7t_intern_string(engine, \"<http://example.org/sprint_health#hasAppointment>\");\n"
        "    uint32_t pred_attendedBy = s7t_intern_string(engine, \"<http://example.org/sprint_health#attendedBy>\");\n"
        "    uint32_t class_Patient = s7t_intern_string(engine, \"<http://example.org/sprint_health#Patient>\");\n"
        "    \n"
        "    // Get all patients\n"
        "    BitVector* patients = s7t_get_subject_vector(engine, pred_type, class_Patient);\n"
        "    \n"
        "    // Allocate results\n"
        "    QueryResult* results = malloc(patients->count * sizeof(QueryResult));\n"
        "    *result_count = 0;\n"
        "    \n"
        "    // Iterate through patients\n"
        "    for (size_t i = 0; i <= engine->max_subject_id; i++) {\n"
        "        if (!bitvec_test(patients, i)) continue;\n"
        "        \n"
        "        // Check SHACL constraint: minCount 1 for hasAppointment\n"
        "        if (!shacl_check_min_count(engine, i, pred_hasAppointment, 1)) continue;\n"
        "        \n"
        "        // Get appointments\n"
        "        size_t appt_count;\n"
        "        uint32_t* appointments = s7t_get_objects(engine, pred_hasAppointment, i, &appt_count);\n"
        "        \n"
        "        for (size_t j = 0; j < appt_count; j++) {\n"
        "            // Get doctor\n"
        "            size_t doc_count;\n"
        "            uint32_t* doctors = s7t_get_objects(engine, pred_attendedBy, appointments[j], &doc_count);\n"
        "            \n"
        "            if (doc_count > 0) {\n"
        "                results[*result_count].patient_id = i;\n"
        "                results[*result_count].appointment_id = appointments[j];\n"
        "                results[*result_count].doctor_id = doctors[0];\n"
        "                (*result_count)++;\n"
        "            }\n"
        "        }\n"
        "    }\n"
        "    \n"
        "    bitvec_destroy(patients);\n"
        "    return results;\n"
        "}\n\n"
        "// SHACL validation function\n"
        "int validate_patient_shape(EngineState* engine, uint32_t patient_id) {\n"
        "    uint32_t pred_hasName = s7t_intern_string(engine, \"<http://example.org/sprint_health#hasName>\");\n"
        "    uint32_t pred_hasAppointment = s7t_intern_string(engine, \"<http://example.org/sprint_health#hasAppointment>\");\n"
        "    \n"
        "    // Check minCount 1 for hasName\n"
        "    if (!shacl_check_min_count(engine, patient_id, pred_hasName, 1)) return 0;\n"
        "    \n"
        "    // Check maxCount 1 for hasName\n"
        "    if (!shacl_check_max_count(engine, patient_id, pred_hasName, 1)) return 0;\n"
        "    \n"
        "    // Check minCount 1 for hasAppointment\n"
        "    if (!shacl_check_min_count(engine, patient_id, pred_hasAppointment, 1)) return 0;\n"
        "    \n"
        "    return 1;\n"
        "}\n";
    
    // Set template variables
    char count_str[32];
    snprintf(count_str, sizeof(count_str), "%zu", ontology->count);
    cjinja_set_var(ctx, "ontology_count", count_str);
    
    snprintf(count_str, sizeof(count_str), "%zu", shapes->count);
    cjinja_set_var(ctx, "shape_count", count_str);
    
    // Render template
    char* result = cjinja_render_string(kernel_template, ctx);
    cjinja_destroy_context(ctx);
    
    return result;
}
// Compile C code to shared library
int compile_kernel(const char* c_code, const char* output_path) {
    // Write to temporary file
    const char* temp_c = "/tmp/seven_t_kernel.c";
    FILE* f = fopen(temp_c, "w");
    if (!f) return -1;
    
    fprintf(f, "%s", c_code);
    fclose(f);
    
    // Build compile command
    char cmd[4096];
    snprintf(cmd, sizeof(cmd), 
            "cc -O3 -march=native -fPIC -shared "
            "-I/Users/sac/autotel/autotel/engines/seven_tick/runtime/src "
            "-L/Users/sac/autotel/autotel/engines/seven_tick/lib "
            "-l7t_runtime "
            "%s -o %s 2>&1", 
            temp_c, output_path);
    
    #ifndef NDEBUG
    printf("Compile command: %s\n", cmd);
    #endif
    
    // Execute compilation
    int result = system(cmd);
    
    return result;
}
