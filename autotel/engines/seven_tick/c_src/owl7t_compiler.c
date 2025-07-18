#include "owl7t_compiler.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <ctype.h>

// Simple parser state
typedef struct {
    const char* data;
    size_t pos;
    size_t len;
} Parser;

// URI to ID mapping (simple hash table)
#define URI_BUCKETS 1024
typedef struct URIEntry {
    char* uri;
    uint32_t id;
    struct URIEntry* next;
} URIEntry;

typedef struct {
    URIEntry* buckets[URI_BUCKETS];
    uint32_t next_id;
} URIIndex;

// Get hardware ticks for cost estimation
static inline uint64_t get_ticks(void) {
#if defined(__aarch64__)
    uint64_t val;
    __asm__ __volatile__ ("mrs %0, cntvct_el0" : "=r" (val));
    return val;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000ULL + ts.tv_nsec;
#endif
}

// Simple string hash
static uint32_t hash_string(const char* str) {
    uint32_t hash = 5381;
    int c;
    while ((c = *str++))
        hash = ((hash << 5) + hash) + c;
    return hash;
}

// Get or create URI ID
static uint32_t get_uri_id(URIIndex* index, const char* uri) {
    uint32_t bucket = hash_string(uri) % URI_BUCKETS;
    
    // Look for existing
    URIEntry* entry = index->buckets[bucket];
    while (entry) {
        if (strcmp(entry->uri, uri) == 0)
            return entry->id;
        entry = entry->next;
    }
    
    // Create new
    entry = malloc(sizeof(URIEntry));
    entry->uri = strdup(uri);
    entry->id = index->next_id++;
    entry->next = index->buckets[bucket];
    index->buckets[bucket] = entry;
    
    return entry->id;
}

// Estimate tick cost of axiom
static uint8_t estimate_tick_cost(OWL7T_AxiomType type) {
    switch (type) {
        case OWL7T_SUBCLASS:
        case OWL7T_EQUIVALENT_CLASS:
            return 3;  // Load + AND + test
        
        case OWL7T_DOMAIN:
        case OWL7T_RANGE:
            return 4;  // Load + shift + AND + test
            
        case OWL7T_FUNCTIONAL:
        case OWL7T_INVERSE_FUNCTIONAL:
            return 5;  // Load + popcount + compare
            
        case OWL7T_TRANSITIVE:
            return 7;  // Maximum for single-hop closure
            
        default:
            return 8;  // Over budget
    }
}

// Parse simple Turtle triple (80/20 - handles basic patterns only)
static bool parse_triple(Parser* p, char* subject, char* predicate, char* object) {
    // Skip whitespace and comments
    while (p->pos < p->len && (p->data[p->pos] == ' ' || 
                                p->data[p->pos] == '\t' || 
                                p->data[p->pos] == '\n' ||
                                p->data[p->pos] == '#')) {
        if (p->data[p->pos] == '#') {
            // Skip to end of line
            while (p->pos < p->len && p->data[p->pos] != '\n')
                p->pos++;
        } else {
            p->pos++;
        }
    }
    
    if (p->pos >= p->len)
        return false;
    
    // Parse subject
    size_t start = p->pos;
    if (p->data[p->pos] == '<') {
        p->pos++;
        start++;
        while (p->pos < p->len && p->data[p->pos] != '>')
            p->pos++;
        if (p->pos >= p->len) return false;
        strncpy(subject, &p->data[start], p->pos - start);
        subject[p->pos - start] = '\0';
        p->pos++; // Skip '>'
    } else if (p->data[p->pos] == ':') {
        // Prefixed URI
        p->pos++;
        start = p->pos;
        while (p->pos < p->len && p->data[p->pos] != ' ' && p->data[p->pos] != '\t')
            p->pos++;
        strncpy(subject, &p->data[start-1], p->pos - start + 1);
        subject[p->pos - start + 1] = '\0';
    } else {
        return false;
    }
    
    // Skip whitespace
    while (p->pos < p->len && (p->data[p->pos] == ' ' || p->data[p->pos] == '\t'))
        p->pos++;
    
    // Parse predicate (similar logic)
    start = p->pos;
    if (p->data[p->pos] == '<') {
        p->pos++;
        start++;
        while (p->pos < p->len && p->data[p->pos] != '>')
            p->pos++;
        strncpy(predicate, &p->data[start], p->pos - start);
        predicate[p->pos - start] = '\0';
        p->pos++;
    } else if (strncmp(&p->data[p->pos], "a ", 2) == 0) {
        strcpy(predicate, "rdf:type");
        p->pos += 2;
    } else {
        // Handle prefixed predicates
        start = p->pos;
        while (p->pos < p->len && p->data[p->pos] != ' ' && p->data[p->pos] != '\t')
            p->pos++;
        strncpy(predicate, &p->data[start], p->pos - start);
        predicate[p->pos - start] = '\0';
    }
    
    // Skip whitespace
    while (p->pos < p->len && (p->data[p->pos] == ' ' || p->data[p->pos] == '\t'))
        p->pos++;
    
    // Parse object
    start = p->pos;
    if (p->data[p->pos] == '<') {
        p->pos++;
        start++;
        while (p->pos < p->len && p->data[p->pos] != '>')
            p->pos++;
        strncpy(object, &p->data[start], p->pos - start);
        object[p->pos - start] = '\0';
        p->pos++;
    } else if (p->data[p->pos] == ':' || isalpha(p->data[p->pos])) {
        // Prefixed URI or keyword
        while (p->pos < p->len && p->data[p->pos] != ' ' && 
               p->data[p->pos] != '\t' && p->data[p->pos] != '.' && 
               p->data[p->pos] != ';')
            p->pos++;
        strncpy(object, &p->data[start], p->pos - start);
        object[p->pos - start] = '\0';
    }
    
    // Skip to end of statement
    while (p->pos < p->len && p->data[p->pos] != '.')
        p->pos++;
    if (p->pos < p->len)
        p->pos++; // Skip '.'
    
    return true;
}

// Main compilation function
OWL7T_CompileResult* owl7t_compile(const char* ontology_path, const char* output_dir) {
    // Read file
    FILE* f = fopen(ontology_path, "r");
    if (!f) abort();
    
    fseek(f, 0, SEEK_END);
    size_t file_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char* data = malloc(file_size + 1);
    fread(data, 1, file_size, f);
    data[file_size] = '\0';
    fclose(f);
    
    // Initialize result
    OWL7T_CompileResult* result = calloc(1, sizeof(OWL7T_CompileResult));
    result->axioms = malloc(1000 * sizeof(CompiledAxiom));  // Pre-alloc
    
    // URI index
    URIIndex uri_index = {0};
    
    // Parse triples
    Parser parser = { .data = data, .pos = 0, .len = file_size };
    char subject[256], predicate[256], object[256];
    
    while (parse_triple(&parser, subject, predicate, object)) {
        CompiledAxiom axiom = {0};
        
        // Classify axiom type
        if (strcmp(predicate, "rdfs:subClassOf") == 0) {
            axiom.type = OWL7T_SUBCLASS;
            axiom.subject_id = get_uri_id(&uri_index, subject);
            axiom.object_id = get_uri_id(&uri_index, object);
        } else if (strcmp(predicate, "owl:equivalentClass") == 0) {
            axiom.type = OWL7T_EQUIVALENT_CLASS;
            axiom.subject_id = get_uri_id(&uri_index, subject);
            axiom.object_id = get_uri_id(&uri_index, object);
        } else if (strcmp(predicate, "rdfs:domain") == 0) {
            axiom.type = OWL7T_DOMAIN;
            axiom.subject_id = get_uri_id(&uri_index, subject);
            axiom.object_id = get_uri_id(&uri_index, object);
        } else if (strcmp(predicate, "rdfs:range") == 0) {
            axiom.type = OWL7T_RANGE;
            axiom.subject_id = get_uri_id(&uri_index, subject);
            axiom.object_id = get_uri_id(&uri_index, object);
        } else if (strcmp(predicate, "rdf:type") == 0) {
            if (strcmp(object, "owl:FunctionalProperty") == 0) {
                axiom.type = OWL7T_FUNCTIONAL;
                axiom.subject_id = get_uri_id(&uri_index, subject);
            } else if (strcmp(object, "owl:TransitiveProperty") == 0) {
                axiom.type = OWL7T_TRANSITIVE;
                axiom.subject_id = get_uri_id(&uri_index, subject);
            }
        } else {
            continue;  // Skip unsupported
        }
        
        // Estimate tick cost
        axiom.tick_cost = estimate_tick_cost(axiom.type);
        
        // Accept or reject based on tick budget
        if (axiom.tick_cost <= 7) {
            // Pre-compute mask
            axiom.mask = 1ULL << (axiom.object_id % 64);
            result->axioms[result->axiom_count++] = axiom;
            result->tick_compliant_count++;
        } else {
            result->rejected_count++;
            fprintf(stderr, "WARNING: Axiom %s %s %s exceeds 7-tick budget (cost: %d)\n",
                    subject, predicate, object, axiom.tick_cost);
        }
    }
    
    // Update counts
    result->class_count = uri_index.next_id;
    result->property_count = uri_index.next_id;  // Simplified
    
    // Allocate mask tables
    result->class_masks = calloc(result->class_count, sizeof(uint64_t));
    result->property_masks = calloc(result->property_count, sizeof(uint64_t));
    
    // Materialize transitive closures (simplified)
    for (size_t i = 0; i < result->axiom_count; i++) {
        CompiledAxiom* ax = &result->axioms[i];
        if (ax->type == OWL7T_SUBCLASS) {
            result->class_masks[ax->subject_id] |= ax->mask;
        }
    }
    
    // Compute transitive closure (Floyd-Warshall simplified)
    for (uint32_t k = 0; k < result->class_count; k++) {
        for (uint32_t i = 0; i < result->class_count; i++) {
            if (result->class_masks[i] & (1ULL << k)) {
                result->class_masks[i] |= result->class_masks[k];
            }
        }
    }
    
    free(data);
    
    // Generate report
    printf("OWL-7T Compilation Report:\n");
    printf("  Total axioms: %zu\n", result->axiom_count);
    printf("  Tick-compliant: %u\n", result->tick_compliant_count);
    printf("  Materialized: %u\n", result->materialized_count);
    printf("  Rejected: %u\n", result->rejected_count);
    
    return result;
}

// The 7-tick runtime checks
bool owl7t_check_subclass(const OWL7T_CompileResult* result, uint32_t child, uint32_t parent) {
    uint64_t start = get_ticks();
    
    // --- THE 7 TICKS BEGIN HERE ---
    uint64_t child_mask = result->class_masks[child];      // Tick 1-2: load
    uint64_t parent_bit = 1ULL << (parent % 64);          // Tick 3: shift
    bool is_subclass = (child_mask & parent_bit) != 0;    // Tick 4-5: AND + test
    // --- THE 7 TICKS END HERE ---
    
    uint64_t elapsed = get_ticks() - start;
    // Note: on ARM this will be timer ticks, not CPU cycles
    
    return is_subclass;
}

bool owl7t_check_property(const OWL7T_CompileResult* result, uint32_t prop, uint32_t characteristic) {
    uint64_t start = get_ticks();
    
    // --- THE 7 TICKS BEGIN HERE ---
    uint64_t prop_mask = result->property_masks[prop];     // Tick 1-2: load
    bool has_characteristic = (prop_mask & characteristic) != 0;  // Tick 3-4: AND + test
    // --- THE 7 TICKS END HERE ---
    
    uint64_t elapsed = get_ticks() - start;
    
    return has_characteristic;
}

// Generate C code
int owl7t_generate_c_code(const OWL7T_CompileResult* result, const char* output_dir) {
    char header_path[256], impl_path[256];
    snprintf(header_path, sizeof(header_path), "%s/owl7t_generated.h", output_dir);
    snprintf(impl_path, sizeof(impl_path), "%s/owl7t_generated.c", output_dir);
    
    // Write header
    FILE* h = fopen(header_path, "w");
    if (!h) return -1;
    
    fprintf(h, "// Auto-generated by OWL-7T compiler\n");
    fprintf(h, "#ifndef OWL7T_GENERATED_H\n");
    fprintf(h, "#define OWL7T_GENERATED_H\n\n");
    fprintf(h, "#include <stdint.h>\n#include <stdbool.h>\n\n");
    
    fprintf(h, "// Class hierarchy masks (%zu classes)\n", result->class_count);
    fprintf(h, "extern const uint64_t OWL7T_CLASS_MASKS[%zu];\n\n", result->class_count);
    
    fprintf(h, "// Property characteristic masks (%zu properties)\n", result->property_count);
    fprintf(h, "extern const uint64_t OWL7T_PROPERTY_MASKS[%zu];\n\n", result->property_count);
    
    fprintf(h, "// 7-tick inference functions\n");
    fprintf(h, "static inline bool owl7t_is_subclass_of(uint32_t child, uint32_t parent) {\n");
    fprintf(h, "    uint64_t child_mask = OWL7T_CLASS_MASKS[child];\n");
    fprintf(h, "    uint64_t parent_bit = 1ULL << (parent %% 64);\n");
    fprintf(h, "    return (child_mask & parent_bit) != 0;\n");
    fprintf(h, "}\n\n");
    
    fprintf(h, "#endif // OWL7T_GENERATED_H\n");
    fclose(h);
    
    // Write implementation
    FILE* c = fopen(impl_path, "w");
    if (!c) return -1;
    
    fprintf(c, "// Auto-generated by OWL-7T compiler\n");
    fprintf(c, "#include \"owl7t_generated.h\"\n\n");
    
    fprintf(c, "const uint64_t OWL7T_CLASS_MASKS[%zu] = {\n", result->class_count);
    for (size_t i = 0; i < result->class_count; i++) {
        fprintf(c, "    0x%016llxULL,\n", result->class_masks[i]);
    }
    fprintf(c, "};\n\n");
    
    fprintf(c, "const uint64_t OWL7T_PROPERTY_MASKS[%zu] = {\n", result->property_count);
    for (size_t i = 0; i < result->property_count; i++) {
        fprintf(c, "    0x%016llxULL,\n", result->property_masks[i]);
    }
    fprintf(c, "};\n");
    
    fclose(c);
    
    printf("Generated: %s, %s\n", header_path, impl_path);
    return 0;
}

void owl7t_free_result(OWL7T_CompileResult* result) {
    if (!result) return;
    free(result->axioms);
    free(result->class_masks);
    free(result->property_masks);
    free(result);
}
