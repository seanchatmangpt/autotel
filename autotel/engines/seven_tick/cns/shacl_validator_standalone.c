/*  ─────────────────────────────────────────────────────────────
    shacl_validator_standalone.c  –  Standalone SHACL Validator Implementation
    
    Self-contained 80/20 SHACL validator focused on memory bound validation.
    ───────────────────────────────────────────────────────────── */

#include "shacl_validator_standalone.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ============================================================================
// INTERNAL HELPER FUNCTIONS
// ============================================================================

// High-precision cycle counting for 7T compliance
static inline shacl_tick_t get_cycles(void) {
    #if defined(__x86_64__) || defined(__i386__)
        uint32_t low, high;
        __asm__ volatile ("rdtsc" : "=a" (low), "=d" (high));
        return ((uint64_t)high << 32) | low;
    #elif defined(__aarch64__)
        uint64_t val;
        __asm__ volatile ("mrs %0, cntvct_el0" : "=r" (val));
        return val;
    #else
        return 0; // Fallback for unsupported architectures
    #endif
}

// Simple hash function for string IRI lookups
static uint32_t hash_string(const char *str) {
    if (!str) return 0;
    uint32_t hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    return hash;
}

// String duplication helper
static char* duplicate_string(const char *str) {
    if (!str) return NULL;
    size_t len = strlen(str);
    char *dup = malloc(len + 1);
    if (dup) {
        strcpy(dup, str);
    }
    return dup;
}

// Find shape by IRI (linear search for simplicity)
static shacl_shape_t* find_shape_by_iri(shacl_validator_t *validator, const char *shape_iri) {
    if (!validator || !shape_iri) {
        return NULL;
    }
    
    for (uint32_t i = 0; i < validator->shape_count; i++) {
        if (validator->shapes[i].shape_iri && 
            strcmp(validator->shapes[i].shape_iri, shape_iri) == 0) {
            return &validator->shapes[i];
        }
    }
    
    return NULL;
}

// ============================================================================
// VALIDATOR LIFECYCLE FUNCTIONS
// ============================================================================

shacl_validator_t* shacl_validator_create(size_t max_memory_per_node, size_t max_memory_per_graph) {
    shacl_tick_t start_tick = get_cycles();
    
    // Allocate validator structure
    shacl_validator_t *validator = malloc(sizeof(shacl_validator_t));
    if (!validator) {
        return NULL;
    }
    
    // Initialize validator
    memset(validator, 0, sizeof(shacl_validator_t));
    validator->max_memory_per_node = max_memory_per_node;
    validator->max_memory_per_graph = max_memory_per_graph;
    validator->strict_memory_mode = true;
    validator->magic = SHACL_VALIDATOR_MAGIC;
    
    // Initialize shapes array
    validator->shape_capacity = 16; // Start with 16 shapes
    validator->shapes = malloc(validator->shape_capacity * sizeof(shacl_shape_t));
    if (!validator->shapes) {
        free(validator);
        return NULL;
    }
    memset(validator->shapes, 0, validator->shape_capacity * sizeof(shacl_shape_t));
    
    // Update memory tracking
    validator->memory.current_usage = sizeof(shacl_validator_t) +
                                      validator->shape_capacity * sizeof(shacl_shape_t);
    validator->memory.max_allowed = max_memory_per_graph;
    validator->memory.bounded = (validator->memory.current_usage <= validator->memory.max_allowed);
    
    shacl_tick_t end_tick = get_cycles();
    shacl_tick_t elapsed = end_tick - start_tick;
    
    // Ensure 7T compliance
    if (elapsed > SHACL_MAX_TICKS_PER_NODE) {
        fprintf(stderr, "SHACL validator creation took %llu ticks (>%d)\n", 
                elapsed, SHACL_MAX_TICKS_PER_NODE);
    }
    
    return validator;
}

shacl_validator_t* shacl_validator_create_default(void) {
    return shacl_validator_create(SHACL_MAX_MEMORY_PER_NODE, SHACL_MAX_MEMORY_PER_GRAPH);
}

void shacl_validator_destroy(shacl_validator_t *validator) {
    if (!validator || validator->magic != SHACL_VALIDATOR_MAGIC) {
        return;
    }
    
    // Free shapes and constraints
    for (uint32_t i = 0; i < validator->shape_count; i++) {
        shacl_shape_t *shape = &validator->shapes[i];
        
        // Free constraint list
        shacl_constraint_t *constraint = shape->constraints;
        while (constraint) {
            shacl_constraint_t *next = constraint->next;
            free(constraint->property_path);
            free(constraint->message);
            if (constraint->type == SHACL_CONSTRAINT_CLASS ||
                constraint->type == SHACL_CONSTRAINT_DATATYPE) {
                free(constraint->value.string_value);
            }
            free(constraint);
            constraint = next;
        }
        
        // Free shape strings
        free(shape->shape_iri);
        free(shape->target_class);
    }
    
    free(validator->shapes);
    validator->magic = 0;
    free(validator);
}

shacl_result_t shacl_validator_clear(shacl_validator_t *validator) {
    if (!validator || validator->magic != SHACL_VALIDATOR_MAGIC) {
        return SHACL_ERROR_INVALID_ARG;
    }
    
    // Free existing shapes
    for (uint32_t i = 0; i < validator->shape_count; i++) {
        shacl_shape_t *shape = &validator->shapes[i];
        
        // Free constraint list
        shacl_constraint_t *constraint = shape->constraints;
        while (constraint) {
            shacl_constraint_t *next = constraint->next;
            free(constraint->property_path);
            free(constraint->message);
            if (constraint->type == SHACL_CONSTRAINT_CLASS ||
                constraint->type == SHACL_CONSTRAINT_DATATYPE) {
                free(constraint->value.string_value);
            }
            free(constraint);
            constraint = next;
        }
        
        // Free shape strings
        free(shape->shape_iri);
        free(shape->target_class);
    }
    
    // Reset counters
    validator->shape_count = 0;
    validator->validations_performed = 0;
    validator->memory_violations = 0;
    validator->total_ticks = 0;
    
    // Clear shapes array
    memset(validator->shapes, 0, validator->shape_capacity * sizeof(shacl_shape_t));
    
    // Reset memory tracking
    validator->memory.current_usage = sizeof(shacl_validator_t) +
                                      validator->shape_capacity * sizeof(shacl_shape_t);
    validator->memory.peak_usage = validator->memory.current_usage;
    validator->memory.violation_count = 0;
    validator->memory.bounded = true;
    
    return SHACL_OK;
}

// ============================================================================
// SHAPE MANAGEMENT FUNCTIONS
// ============================================================================

shacl_result_t shacl_load_shape(shacl_validator_t *validator, const char *shape_iri, const char *target_class) {
    if (!validator || !shape_iri || validator->magic != SHACL_VALIDATOR_MAGIC) {
        return SHACL_ERROR_INVALID_ARG;
    }
    
    shacl_tick_t start_tick = get_cycles();
    
    // Check if shape already exists
    if (find_shape_by_iri(validator, shape_iri) != NULL) {
        return SHACL_ERROR_INVALID_ARG; // Shape already exists
    }
    
    // Check capacity
    if (validator->shape_count >= validator->shape_capacity) {
        // Expand array
        uint32_t new_capacity = validator->shape_capacity * 2;
        shacl_shape_t *new_shapes = realloc(validator->shapes, new_capacity * sizeof(shacl_shape_t));
        if (!new_shapes) {
            return SHACL_ERROR_MEMORY;
        }
        validator->shapes = new_shapes;
        validator->shape_capacity = new_capacity;
        
        // Zero new memory
        memset(&validator->shapes[validator->shape_count], 0, 
               (new_capacity - validator->shape_count) * sizeof(shacl_shape_t));
    }
    
    // Add shape to array
    shacl_shape_t *shape = &validator->shapes[validator->shape_count];
    memset(shape, 0, sizeof(shacl_shape_t));
    
    // Set shape properties
    shape->shape_iri = duplicate_string(shape_iri);
    if (!shape->shape_iri) {
        return SHACL_ERROR_MEMORY;
    }
    
    if (target_class) {
        shape->target_class = duplicate_string(target_class);
        if (!shape->target_class) {
            free(shape->shape_iri);
            return SHACL_ERROR_MEMORY;
        }
    }
    
    shape->active = true;
    shape->shape_id = validator->shape_count;
    
    // Initialize memory tracking for shape
    shape->memory.current_usage = sizeof(shacl_shape_t) + strlen(shape_iri) + 1;
    if (target_class) {
        shape->memory.current_usage += strlen(target_class) + 1;
    }
    shape->memory.max_allowed = validator->max_memory_per_node;
    shape->memory.bounded = true;
    
    validator->shape_count++;
    
    // Update memory usage
    validator->memory.current_usage += shape->memory.current_usage;
    if (validator->memory.current_usage > validator->memory.peak_usage) {
        validator->memory.peak_usage = validator->memory.current_usage;
    }
    
    shacl_tick_t end_tick = get_cycles();
    shacl_tick_t elapsed = end_tick - start_tick;
    
    // Check 7T compliance
    if (elapsed > SHACL_MAX_TICKS_PER_NODE) {
        fprintf(stderr, "Shape loading took %llu ticks (>%d)\n", 
                elapsed, SHACL_MAX_TICKS_PER_NODE);
    }
    
    return SHACL_OK;
}

shacl_result_t shacl_add_constraint(shacl_validator_t *validator, const char *shape_iri,
                                   shacl_constraint_type_t type, const char *property_path,
                                   const shacl_constraint_value_t *value) {
    if (!validator || !shape_iri || !value || validator->magic != SHACL_VALIDATOR_MAGIC) {
        return SHACL_ERROR_INVALID_ARG;
    }
    
    shacl_tick_t start_tick = get_cycles();
    
    // Find shape
    shacl_shape_t *shape = find_shape_by_iri(validator, shape_iri);
    if (!shape) {
        return SHACL_ERROR_NOT_FOUND;
    }
    
    // Check constraint limit
    if (shape->constraint_count >= SHACL_MAX_CONSTRAINTS) {
        return SHACL_ERROR_CAPACITY;
    }
    
    // Allocate constraint
    shacl_constraint_t *constraint = malloc(sizeof(shacl_constraint_t));
    if (!constraint) {
        return SHACL_ERROR_MEMORY;
    }
    
    // Initialize constraint
    memset(constraint, 0, sizeof(shacl_constraint_t));
    constraint->type = type;
    constraint->value = *value;
    
    if (property_path) {
        constraint->property_path = duplicate_string(property_path);
        if (!constraint->property_path) {
            free(constraint);
            return SHACL_ERROR_MEMORY;
        }
    }
    
    // Handle string values that need duplication
    if ((type == SHACL_CONSTRAINT_CLASS || type == SHACL_CONSTRAINT_DATATYPE) && 
        value->string_value) {
        constraint->value.string_value = duplicate_string(value->string_value);
        if (!constraint->value.string_value) {
            free(constraint->property_path);
            free(constraint);
            return SHACL_ERROR_MEMORY;
        }
    }
    
    // Initialize memory tracking for constraint
    constraint->memory.current_usage = sizeof(shacl_constraint_t);
    if (property_path) {
        constraint->memory.current_usage += strlen(property_path) + 1;
    }
    constraint->memory.max_allowed = validator->max_memory_per_node / 10; // 10% per constraint
    constraint->memory.bounded = true;
    
    // Add to shape's constraint list
    constraint->next = shape->constraints;
    shape->constraints = constraint;
    shape->constraint_count++;
    
    // Update memory usage
    shape->memory.current_usage += constraint->memory.current_usage;
    validator->memory.current_usage += constraint->memory.current_usage;
    
    shacl_tick_t end_tick = get_cycles();
    shacl_tick_t elapsed = end_tick - start_tick;
    
    // Check 7T compliance
    if (elapsed > SHACL_MAX_TICKS_PER_CONSTRAINT) {
        fprintf(stderr, "Constraint addition took %llu ticks (>%d)\n", 
                elapsed, SHACL_MAX_TICKS_PER_CONSTRAINT);
    }
    
    return SHACL_OK;
}

shacl_result_t shacl_add_memory_constraint(shacl_validator_t *validator, const char *shape_iri,
                                          const char *property_path, size_t memory_limit) {
    shacl_constraint_value_t value;
    value.memory_limit = memory_limit;
    
    return shacl_add_constraint(validator, shape_iri, SHACL_CONSTRAINT_MEMORY_BOUND,
                                property_path, &value);
}

const shacl_shape_t* shacl_get_shape(const shacl_validator_t *validator, const char *shape_iri) {
    if (!validator || !shape_iri || validator->magic != SHACL_VALIDATOR_MAGIC) {
        return NULL;
    }
    
    return find_shape_by_iri((shacl_validator_t*)validator, shape_iri);
}

// ============================================================================
// CONSTRAINT EVALUATION FUNCTIONS
// ============================================================================

bool shacl_eval_memory_bound(const shacl_graph_t *graph, const char *focus_node,
                            size_t memory_limit, shacl_memory_footprint_t *footprint) {
    if (!focus_node || !footprint) {
        return false;
    }
    
    // Calculate memory usage for this node (simplified)
    footprint->current_usage = strlen(focus_node) + 64; // Base cost
    if (graph && graph->data) {
        footprint->current_usage += graph->size / 100; // Assume 1% of graph per node
    }
    
    footprint->max_allowed = memory_limit;
    footprint->bounded = (footprint->current_usage <= memory_limit);
    
    if (!footprint->bounded) {
        footprint->violation_count++;
    }
    
    return footprint->bounded;
}

bool shacl_eval_count_constraint(const shacl_graph_t *graph, const char *focus_node,
                                const char *property_path, uint32_t min_count, uint32_t max_count,
                                uint32_t *actual_count) {
    if (!focus_node || !actual_count) {
        return false;
    }
    
    // Simplified count calculation
    *actual_count = 1; // Assume one value for simplicity
    
    if (min_count > 0 && *actual_count < min_count) {
        return false;
    }
    
    if (max_count > 0 && *actual_count > max_count) {
        return false;
    }
    
    return true;
}

bool shacl_eval_node_kind_constraint(const shacl_graph_t *graph, const char *value,
                                    shacl_node_kind_t node_kind) {
    if (!value) {
        return false;
    }
    
    // Simplified node kind checking based on value format
    switch (node_kind) {
        case SHACL_NODE_KIND_IRI:
            return (strstr(value, "http://") != NULL || strstr(value, "https://") != NULL);
        case SHACL_NODE_KIND_BLANK_NODE:
            return (value[0] == '_' && value[1] == ':');
        case SHACL_NODE_KIND_LITERAL:
            return !(value[0] == '_' || strstr(value, "://") != NULL);
        case SHACL_NODE_KIND_ANY:
            return true;
        default:
            return false;
    }
}

shacl_result_t shacl_eval_constraint(shacl_validator_t *validator, const shacl_graph_t *graph,
                                     const char *focus_node, const char *property_path,
                                     const shacl_constraint_t *constraint, bool *conforms,
                                     shacl_memory_footprint_t *memory) {
    if (!validator || !focus_node || !constraint || !conforms || !memory) {
        return SHACL_ERROR_INVALID_ARG;
    }
    
    shacl_tick_t start_tick = get_cycles();
    
    // Initialize
    *conforms = true;
    memset(memory, 0, sizeof(shacl_memory_footprint_t));
    
    // Evaluate based on constraint type
    switch (constraint->type) {
        case SHACL_CONSTRAINT_MEMORY_BOUND:
            *conforms = shacl_eval_memory_bound(graph, focus_node,
                                               constraint->value.memory_limit, memory);
            if (!*conforms) {
                validator->memory_violations++;
            }
            break;
            
        case SHACL_CONSTRAINT_MIN_COUNT:
        case SHACL_CONSTRAINT_MAX_COUNT: {
            uint32_t actual_count;
            *conforms = shacl_eval_count_constraint(graph, focus_node, property_path,
                                                   constraint->type == SHACL_CONSTRAINT_MIN_COUNT ? constraint->value.count_value : 0,
                                                   constraint->type == SHACL_CONSTRAINT_MAX_COUNT ? constraint->value.count_value : UINT32_MAX,
                                                   &actual_count);
            break;
        }
        
        case SHACL_CONSTRAINT_CLASS:
            // For class constraint, assume conforming for simplicity
            *conforms = true;
            break;
            
        case SHACL_CONSTRAINT_DATATYPE:
            // For datatype constraint, assume conforming for simplicity
            *conforms = true;
            break;
            
        case SHACL_CONSTRAINT_NODE_KIND:
            *conforms = shacl_eval_node_kind_constraint(graph, focus_node, 
                                                       constraint->value.node_kind);
            break;
            
        default:
            // Unsupported constraint types are assumed to conform
            *conforms = true;
            break;
    }
    
    shacl_tick_t end_tick = get_cycles();
    shacl_tick_t elapsed = end_tick - start_tick;
    
    if (elapsed > SHACL_MAX_TICKS_PER_CONSTRAINT) {
        fprintf(stderr, "Constraint evaluation took %llu ticks (>%d)\n", 
                elapsed, SHACL_MAX_TICKS_PER_CONSTRAINT);
    }
    
    return SHACL_OK;
}

// ============================================================================
// VALIDATION FUNCTIONS
// ============================================================================

shacl_result_t shacl_validate_node_shape(shacl_validator_t *validator, const shacl_graph_t *graph,
                                         const char *node_iri, const shacl_shape_t *shape,
                                         shacl_validation_report_t *report) {
    if (!validator || !graph || !node_iri || !shape || !report) {
        return SHACL_ERROR_INVALID_ARG;
    }
    
    // Iterate through all constraints in the shape
    const shacl_constraint_t *constraint = shape->constraints;
    while (constraint) {
        bool conforms = false;
        shacl_memory_footprint_t memory = {0};
        
        // Evaluate constraint
        shacl_result_t result = shacl_eval_constraint(validator, graph, node_iri, 
                                                     constraint->property_path, constraint, 
                                                     &conforms, &memory);
        
        if (result != SHACL_OK) {
            return result;
        }
        
        // Add result if constraint fails
        if (!conforms) {
            shacl_add_result(report, node_iri, constraint->property_path, NULL,
                            constraint->type, SHACL_RESULT_VIOLATION,
                            "Constraint violation", &memory);
        }
        
        constraint = constraint->next;
    }
    
    return SHACL_OK;
}

shacl_result_t shacl_validate_node(shacl_validator_t *validator, const shacl_graph_t *graph,
                                  const char *node_iri, shacl_validation_report_t *report) {
    if (!validator || !graph || !node_iri || !report || 
        validator->magic != SHACL_VALIDATOR_MAGIC) {
        return SHACL_ERROR_INVALID_ARG;
    }
    
    shacl_tick_t start_tick = get_cycles();
    
    // Find applicable shapes for this node
    for (uint32_t i = 0; i < validator->shape_count; i++) {
        shacl_shape_t *shape = &validator->shapes[i];
        if (!shape->active) continue;
        
        // Check if node matches target class (simplified)
        bool matches_target = true; // Assume match for simplicity
        
        if (matches_target) {
            // Validate against this shape
            shacl_result_t result = shacl_validate_node_shape(validator, graph, node_iri, shape, report);
            if (result != SHACL_OK) {
                return result;
            }
        }
    }
    
    validator->validations_performed++;
    report->nodes_validated++;
    
    shacl_tick_t end_tick = get_cycles();
    shacl_tick_t elapsed = end_tick - start_tick;
    
    if (elapsed > SHACL_MAX_TICKS_PER_NODE) {
        fprintf(stderr, "Node validation took %llu ticks (>%d)\n", 
                elapsed, SHACL_MAX_TICKS_PER_NODE);
    }
    
    validator->total_ticks += elapsed;
    report->validation_ticks += elapsed;
    
    return SHACL_OK;
}

// ============================================================================
// VALIDATION REPORT FUNCTIONS
// ============================================================================

shacl_validation_report_t* shacl_create_report(shacl_validator_t *validator) {
    if (!validator || validator->magic != SHACL_VALIDATOR_MAGIC) {
        return NULL;
    }
    
    shacl_validation_report_t *report = malloc(sizeof(shacl_validation_report_t));
    if (!report) {
        return NULL;
    }
    
    memset(report, 0, sizeof(shacl_validation_report_t));
    report->conforms = true; // Assume conforming until proven otherwise
    
    return report;
}

shacl_result_t shacl_add_result(shacl_validation_report_t *report, const char *focus_node,
                               const char *property_path, const char *value,
                               shacl_constraint_type_t constraint_type, shacl_result_level_t level,
                               const char *message, const shacl_memory_footprint_t *memory) {
    if (!report) {
        return SHACL_ERROR_INVALID_ARG;
    }
    
    // For simplicity, we'll just update counters
    // In a full implementation, we'd create result objects
    
    report->result_count++;
    
    if (level == SHACL_RESULT_VIOLATION) {
        report->violation_count++;
        report->conforms = false;
    }
    
    if (constraint_type == SHACL_CONSTRAINT_MEMORY_BOUND) {
        report->memory_violation_count++;
    }
    
    if (memory) {
        report->total_memory.current_usage += memory->current_usage;
        if (memory->current_usage > report->total_memory.peak_usage) {
            report->total_memory.peak_usage = memory->current_usage;
        }
        report->total_memory.violation_count += memory->violation_count;
    }
    
    return SHACL_OK;
}

shacl_result_t shacl_finalize_report(shacl_validation_report_t *report) {
    if (!report) {
        return SHACL_ERROR_INVALID_ARG;
    }
    
    // Update final conformance status
    report->conforms = (report->violation_count == 0);
    
    // Update memory bounds
    report->total_memory.bounded = (report->total_memory.violation_count == 0);
    
    return SHACL_OK;
}

void shacl_destroy_report(shacl_validation_report_t *report) {
    if (report) {
        // In a full implementation, we'd free the result list
        free(report);
    }
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

const char* shacl_constraint_type_string(shacl_constraint_type_t type) {
    switch (type) {
        case SHACL_CONSTRAINT_MEMORY_BOUND: return "memory-bound";
        case SHACL_CONSTRAINT_MIN_COUNT: return "min-count";
        case SHACL_CONSTRAINT_MAX_COUNT: return "max-count";
        case SHACL_CONSTRAINT_CLASS: return "class";
        case SHACL_CONSTRAINT_DATATYPE: return "datatype";
        case SHACL_CONSTRAINT_NODE_KIND: return "node-kind";
        default: return "unknown";
    }
}

const char* shacl_result_level_string(shacl_result_level_t level) {
    switch (level) {
        case SHACL_RESULT_INFO: return "info";
        case SHACL_RESULT_WARNING: return "warning";
        case SHACL_RESULT_VIOLATION: return "violation";
        case SHACL_RESULT_MEMORY_VIOLATION: return "memory-violation";
        default: return "unknown";
    }
}

const char* shacl_node_kind_string(shacl_node_kind_t node_kind) {
    switch (node_kind) {
        case SHACL_NODE_KIND_IRI: return "IRI";
        case SHACL_NODE_KIND_BLANK_NODE: return "BlankNode";
        case SHACL_NODE_KIND_LITERAL: return "Literal";
        case SHACL_NODE_KIND_ANY: return "Any";
        default: return "unknown";
    }
}

shacl_result_t shacl_print_report(const shacl_validation_report_t *report, FILE *output) {
    if (!report || !output) {
        return SHACL_ERROR_INVALID_ARG;
    }
    
    fprintf(output, "SHACL Validation Report:\n");
    fprintf(output, "  Conforms: %s\n", report->conforms ? "true" : "false");
    fprintf(output, "  Total Results: %u\n", report->result_count);
    fprintf(output, "  Violations: %u\n", report->violation_count);
    fprintf(output, "  Memory Violations: %u\n", report->memory_violation_count);
    fprintf(output, "  Nodes Validated: %u\n", report->nodes_validated);
    fprintf(output, "  Validation Time: %llu ticks\n", report->validation_ticks);
    fprintf(output, "  Total Memory Usage: %zu bytes\n", report->total_memory.current_usage);
    fprintf(output, "  Peak Memory Usage: %zu bytes\n", report->total_memory.peak_usage);
    fprintf(output, "  Memory Bounded: %s\n", report->total_memory.bounded ? "true" : "false");
    
    return SHACL_OK;
}

size_t shacl_validator_memory_usage(const shacl_validator_t *validator) {
    if (!validator || validator->magic != SHACL_VALIDATOR_MAGIC) {
        return 0;
    }
    
    return validator->memory.current_usage;
}

bool shacl_validator_valid(const shacl_validator_t *validator) {
    return (validator && validator->magic == SHACL_VALIDATOR_MAGIC);
}

shacl_result_t shacl_validate_validator(const shacl_validator_t *validator) {
    if (!validator) {
        return SHACL_ERROR_INVALID_ARG;
    }
    
    if (validator->magic != SHACL_VALIDATOR_MAGIC) {
        return SHACL_ERROR_CORRUPTION;
    }
    
    if (!validator->shapes) {
        return SHACL_ERROR_CORRUPTION;
    }
    
    if (validator->shape_count > validator->shape_capacity) {
        return SHACL_ERROR_CORRUPTION;
    }
    
    return SHACL_OK;
}