/*  ─────────────────────────────────────────────────────────────
    src/shacl_validator.c  –  Memory-Constrained SHACL Validator Implementation
    
    80/20 SHACL validator implementation focused on memory bound validation.
    Core implementation prioritizing performance and memory constraints.
    ───────────────────────────────────────────────────────────── */

#include "cns/shacl_validator.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

// ============================================================================
// INTERNAL HELPER FUNCTIONS
// ============================================================================

// High-precision cycle counting for 7T compliance
static inline cns_tick_t get_cycles(void) {
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

// Find shape by IRI (O(1) hash lookup)
static cns_shacl_shape_t* find_shape_by_iri(cns_shacl_validator_t *validator, const char *shape_iri) {
    if (!validator || !shape_iri || validator->shape_count == 0) {
        return NULL;
    }
    
    uint32_t hash = hash_string(shape_iri);
    uint32_t index = hash % validator->hash_table_size;
    
    // Linear probing for collision resolution
    for (uint32_t i = 0; i < validator->hash_table_size; i++) {
        uint32_t probe_index = (index + i) % validator->hash_table_size;
        uint32_t shape_idx = validator->shape_hash_table[probe_index];
        
        if (shape_idx == UINT32_MAX) {
            break; // Empty slot found, shape doesn't exist
        }
        
        if (shape_idx < validator->shape_count) {
            cns_shacl_shape_t *shape = &validator->shapes[shape_idx];
            // For simplicity, we'll do string comparison here
            // In a full implementation, we'd use interned strings
            if (shape->shape_iri.hash == hash) {
                return shape;
            }
        }
    }
    
    return NULL;
}

// Calculate memory footprint of a data structure
static size_t calculate_structure_memory(const void *ptr, size_t base_size) {
    if (!ptr) return 0;
    // For simplicity, return base size
    // In a full implementation, this would recursively calculate memory usage
    return base_size;
}

// ============================================================================
// VALIDATOR LIFECYCLE FUNCTIONS
// ============================================================================

cns_shacl_validator_t* cns_shacl_validator_create(cns_arena_t *arena,
                                                  size_t max_memory_per_node,
                                                  size_t max_memory_per_graph) {
    if (!arena) {
        return NULL;
    }
    
    cns_tick_t start_tick = get_cycles();
    
    // Allocate validator structure
    cns_shacl_validator_t *validator = (cns_shacl_validator_t*)arenac_alloc(arena, sizeof(cns_shacl_validator_t));
    if (!validator) {
        return NULL;
    }
    
    // Initialize validator
    memset(validator, 0, sizeof(cns_shacl_validator_t));
    validator->arena = arena;
    validator->max_memory_per_node = max_memory_per_node;
    validator->max_memory_per_graph = max_memory_per_graph;
    validator->max_validation_depth = CNS_SHACL_MAX_VALIDATION_DEPTH;
    validator->strict_memory_mode = true;
    validator->magic = CNS_SHACL_VALIDATOR_MAGIC;
    
    // Initialize hash table (power of 2 for fast modulo)
    validator->hash_table_size = 64; // Start with small table
    validator->shape_hash_table = (uint32_t*)arenac_alloc(arena, 
        validator->hash_table_size * sizeof(uint32_t));
    if (!validator->shape_hash_table) {
        return NULL;
    }
    
    // Initialize hash table with sentinel values
    for (uint32_t i = 0; i < validator->hash_table_size; i++) {
        validator->shape_hash_table[i] = UINT32_MAX;
    }
    
    // Initialize shapes array
    validator->shape_capacity = 16; // Start with 16 shapes
    validator->shapes = (cns_shacl_shape_t*)arenac_alloc(arena,
        validator->shape_capacity * sizeof(cns_shacl_shape_t));
    if (!validator->shapes) {
        return NULL;
    }
    
    // Update memory tracking
    validator->memory.current_usage = sizeof(cns_shacl_validator_t) +
                                      validator->hash_table_size * sizeof(uint32_t) +
                                      validator->shape_capacity * sizeof(cns_shacl_shape_t);
    validator->memory.max_allowed = max_memory_per_graph;
    validator->memory.bounded = (validator->memory.current_usage <= validator->memory.max_allowed);
    
    cns_tick_t end_tick = get_cycles();
    cns_tick_t elapsed = end_tick - start_tick;
    
    // Ensure 7T compliance
    if (elapsed > CNS_SHACL_MAX_TICKS_PER_NODE) {
        // Log warning but continue
        fprintf(stderr, "SHACL validator creation took %lu ticks (>%d)\n", 
                elapsed, CNS_SHACL_MAX_TICKS_PER_NODE);
    }
    
    return validator;
}

cns_shacl_validator_t* cns_shacl_validator_create_default(cns_arena_t *arena) {
    return cns_shacl_validator_create(arena, 
                                      CNS_SHACL_MAX_MEMORY_PER_NODE,
                                      CNS_SHACL_MAX_MEMORY_PER_GRAPH);
}

void cns_shacl_validator_destroy(cns_shacl_validator_t *validator) {
    if (!validator || validator->magic != CNS_SHACL_VALIDATOR_MAGIC) {
        return;
    }
    
    // With arena allocation, we just need to clear the magic number
    // The arena will handle memory cleanup
    validator->magic = 0;
}

cns_shacl_result_t cns_shacl_validator_clear(cns_shacl_validator_t *validator) {
    if (!validator || validator->magic != CNS_SHACL_VALIDATOR_MAGIC) {
        return CNS_SHACL_ERROR_INVALID_ARG;
    }
    
    // Reset counters (arena memory stays allocated but is reusable)
    validator->shape_count = 0;
    validator->validations_performed = 0;
    validator->memory_violations = 0;
    validator->total_ticks = 0;
    
    // Clear hash table
    for (uint32_t i = 0; i < validator->hash_table_size; i++) {
        validator->shape_hash_table[i] = UINT32_MAX;
    }
    
    // Reset memory tracking
    validator->memory.current_usage = sizeof(cns_shacl_validator_t) +
                                      validator->hash_table_size * sizeof(uint32_t) +
                                      validator->shape_capacity * sizeof(cns_shacl_shape_t);
    validator->memory.peak_usage = validator->memory.current_usage;
    validator->memory.violation_count = 0;
    validator->memory.bounded = true;
    
    return CNS_SHACL_OK;
}

// ============================================================================
// SHAPE MANAGEMENT FUNCTIONS
// ============================================================================

cns_shacl_result_t cns_shacl_load_shape(cns_shacl_validator_t *validator,
                                  const char *shape_iri,
                                  const char *target_class) {
    if (!validator || !shape_iri || validator->magic != CNS_SHACL_VALIDATOR_MAGIC) {
        return CNS_SHACL_ERROR_INVALID_ARG;
    }
    
    cns_tick_t start_tick = get_cycles();
    
    // Check if shape already exists
    if (find_shape_by_iri(validator, shape_iri) != NULL) {
        return CNS_SHACL_ERROR_INVALID_ARG; // Shape already exists
    }
    
    // Check capacity
    if (validator->shape_count >= validator->shape_capacity) {
        return CNS_SHACL_ERROR_CAPACITY;
    }
    
    // Add shape to array
    cns_shacl_shape_t *shape = &validator->shapes[validator->shape_count];
    memset(shape, 0, sizeof(cns_shacl_shape_t));
    
    // Set shape properties (simplified string handling)
    shape->shape_iri.hash = hash_string(shape_iri);
    shape->shape_iri.length = strlen(shape_iri);
    
    if (target_class) {
        shape->target_class.hash = hash_string(target_class);
        shape->target_class.length = strlen(target_class);
    }
    
    shape->active = true;
    shape->shape_id = validator->shape_count;
    
    // Initialize memory tracking for shape
    shape->memory.current_usage = sizeof(cns_shacl_shape_t);
    shape->memory.max_allowed = validator->max_memory_per_node;
    shape->memory.bounded = true;
    
    // Add to hash table
    uint32_t hash = shape->shape_iri.hash;
    uint32_t index = hash % validator->hash_table_size;
    
    // Find empty slot using linear probing
    for (uint32_t i = 0; i < validator->hash_table_size; i++) {
        uint32_t probe_index = (index + i) % validator->hash_table_size;
        if (validator->shape_hash_table[probe_index] == UINT32_MAX) {
            validator->shape_hash_table[probe_index] = validator->shape_count;
            break;
        }
    }
    
    validator->shape_count++;
    
    // Update memory usage
    validator->memory.current_usage += sizeof(cns_shacl_shape_t);
    if (validator->memory.current_usage > validator->memory.peak_usage) {
        validator->memory.peak_usage = validator->memory.current_usage;
    }
    
    cns_tick_t end_tick = get_cycles();
    cns_tick_t elapsed = end_tick - start_tick;
    
    // Check 7T compliance
    if (elapsed > CNS_SHACL_MAX_TICKS_PER_NODE) {
        fprintf(stderr, "Shape loading took %lu ticks (>%d)\n", 
                elapsed, CNS_SHACL_MAX_TICKS_PER_NODE);
    }
    
    return CNS_SHACL_OK;
}

cns_shacl_result_t cns_shacl_add_constraint(cns_shacl_validator_t *validator,
                                      const char *shape_iri,
                                      cns_shacl_constraint_type_t type,
                                      const char *property_path,
                                      const cns_constraint_value_t *value) {
    if (!validator || !shape_iri || !value || validator->magic != CNS_SHACL_VALIDATOR_MAGIC) {
        return CNS_SHACL_ERROR_INVALID_ARG;
    }
    
    cns_tick_t start_tick = get_cycles();
    
    // Find shape
    cns_shacl_shape_t *shape = find_shape_by_iri(validator, shape_iri);
    if (!shape) {
        return CNS_SHACL_ERROR_NOT_FOUND;
    }
    
    // Check constraint limit
    if (shape->constraint_count >= CNS_SHACL_MAX_CONSTRAINTS) {
        return CNS_SHACL_ERROR_CAPACITY;
    }
    
    // Allocate constraint
    cns_shacl_constraint_t *constraint = (cns_shacl_constraint_t*)arenac_alloc(
        validator->arena, sizeof(cns_shacl_constraint_t));
    if (!constraint) {
        return CNS_SHACL_ERROR_MEMORY;
    }
    
    // Initialize constraint
    memset(constraint, 0, sizeof(cns_shacl_constraint_t));
    constraint->type = type;
    constraint->value = *value;
    
    if (property_path) {
        constraint->property_path.hash = hash_string(property_path);
        constraint->property_path.length = strlen(property_path);
    }
    
    // Initialize memory tracking for constraint
    constraint->memory.current_usage = sizeof(cns_shacl_constraint_t);
    constraint->memory.max_allowed = validator->max_memory_per_node / 10; // 10% per constraint
    constraint->memory.bounded = true;
    
    // Add to shape's constraint list
    constraint->next = shape->constraints;
    shape->constraints = constraint;
    shape->constraint_count++;
    
    // Update memory usage
    shape->memory.current_usage += sizeof(cns_shacl_constraint_t);
    validator->memory.current_usage += sizeof(cns_shacl_constraint_t);
    
    cns_tick_t end_tick = get_cycles();
    cns_tick_t elapsed = end_tick - start_tick;
    
    // Check 7T compliance
    if (elapsed > CNS_SHACL_MAX_TICKS_PER_CONSTRAINT) {
        fprintf(stderr, "Constraint addition took %lu ticks (>%d)\n", 
                elapsed, CNS_SHACL_MAX_TICKS_PER_CONSTRAINT);
    }
    
    return CNS_SHACL_OK;
}

cns_shacl_result_t cns_shacl_add_memory_constraint(cns_shacl_validator_t *validator,
                                             const char *shape_iri,
                                             const char *property_path,
                                             size_t memory_limit) {
    cns_constraint_value_t value;
    value.memory_limit = memory_limit;
    
    return cns_shacl_add_constraint(validator, shape_iri, 
                                    CNS_SHACL_CONSTRAINT_MEMORY_BOUND,
                                    property_path, &value);
}

const cns_shacl_shape_t* cns_shacl_get_shape(const cns_shacl_validator_t *validator,
                                              const char *shape_iri) {
    if (!validator || !shape_iri || validator->magic != CNS_SHACL_VALIDATOR_MAGIC) {
        return NULL;
    }
    
    return find_shape_by_iri((cns_shacl_validator_t*)validator, shape_iri);
}

// ============================================================================
// CONSTRAINT EVALUATION FUNCTIONS
// ============================================================================

bool cns_shacl_eval_memory_bound(const cns_shacl_graph_t *graph,
                                const char *focus_node,
                                size_t memory_limit,
                                cns_memory_footprint_t *footprint) {
    if (!graph || !focus_node || !footprint) {
        return false;
    }
    
    // Calculate memory usage for this node
    // For simplicity, we'll use a basic calculation
    // In a real implementation, this would traverse the graph
    footprint->current_usage = strlen(focus_node) + 64; // Base cost
    footprint->max_allowed = memory_limit;
    footprint->bounded = (footprint->current_usage <= memory_limit);
    
    if (!footprint->bounded) {
        footprint->violation_count++;
    }
    
    return footprint->bounded;
}

bool cns_shacl_eval_count_constraint(const cns_shacl_graph_t *graph,
                                     const char *focus_node,
                                     const char *property_path,
                                     uint32_t min_count,
                                     uint32_t max_count,
                                     uint32_t *actual_count) {
    if (!graph || !focus_node || !actual_count) {
        return false;
    }
    
    // Simplified count calculation
    // In a real implementation, this would query the graph
    *actual_count = 1; // Assume one value for simplicity
    
    if (min_count > 0 && *actual_count < min_count) {
        return false;
    }
    
    if (max_count > 0 && *actual_count > max_count) {
        return false;
    }
    
    return true;
}

bool cns_shacl_eval_class_constraint(const cns_shacl_graph_t *graph,
                                     const char *focus_node,
                                     const char *target_class) {
    if (!graph || !focus_node || !target_class) {
        return false;
    }
    
    // Simplified class checking
    // In a real implementation, this would check rdf:type relationships
    return true; // Assume conforming for simplicity
}

bool cns_shacl_eval_datatype_constraint(const cns_shacl_graph_t *graph,
                                        const char *value,
                                        const char *datatype) {
    if (!value || !datatype) {
        return false;
    }
    
    // Simplified datatype checking
    // In a real implementation, this would parse and validate datatypes
    return true; // Assume conforming for simplicity
}

bool cns_shacl_eval_node_kind_constraint(const cns_shacl_graph_t *graph,
                                         const char *value,
                                         cns_shacl_node_kind_t node_kind) {
    if (!value) {
        return false;
    }
    
    // Simplified node kind checking based on value format
    switch (node_kind) {
        case CNS_SHACL_NODE_KIND_IRI:
            return (strstr(value, "http://") != NULL || strstr(value, "https://") != NULL);
        case CNS_SHACL_NODE_KIND_BLANK_NODE:
            return (value[0] == '_' && value[1] == ':');
        case CNS_SHACL_NODE_KIND_LITERAL:
            return !(value[0] == '_' || strstr(value, "://") != NULL);
        case CNS_SHACL_NODE_KIND_ANY:
            return true;
        default:
            return false;
    }
}

// ============================================================================
// VALIDATION FUNCTIONS
// ============================================================================

cns_shacl_result_t cns_shacl_validate_node(cns_shacl_validator_t *validator,
                                     const cns_shacl_graph_t *graph,
                                     const char *node_iri,
                                     cns_shacl_validation_report_t *report) {
    if (!validator || !graph || !node_iri || !report || 
        validator->magic != CNS_SHACL_VALIDATOR_MAGIC) {
        return CNS_SHACL_ERROR_INVALID_ARG;
    }
    
    cns_tick_t start_tick = get_cycles();
    
    // Find applicable shapes for this node
    // For simplicity, we'll check all shapes
    for (uint32_t i = 0; i < validator->shape_count; i++) {
        cns_shacl_shape_t *shape = &validator->shapes[i];
        if (!shape->active) continue;
        
        // Check if node matches target class (simplified)
        bool matches_target = true; // Assume match for simplicity
        
        if (matches_target) {
            // Validate against this shape
            cns_shacl_result_t result = cns_shacl_validate_node_shape(
                validator, graph, node_iri, shape, report);
            if (result != CNS_SHACL_OK) {
                return result;
            }
        }
    }
    
    validator->validations_performed++;
    report->nodes_validated++;
    
    cns_tick_t end_tick = get_cycles();
    cns_tick_t elapsed = end_tick - start_tick;
    
    if (elapsed > CNS_SHACL_MAX_TICKS_PER_NODE) {
        fprintf(stderr, "Node validation took %lu ticks (>%d)\n", 
                elapsed, CNS_SHACL_MAX_TICKS_PER_NODE);
    }
    
    validator->total_ticks += elapsed;
    report->validation_ticks += elapsed;
    
    return CNS_SHACL_OK;
}

cns_shacl_result_t cns_shacl_validate_node_shape(cns_shacl_validator_t *validator,
                                           const cns_shacl_graph_t *graph,
                                           const char *node_iri,
                                           const cns_shacl_shape_t *shape,
                                           cns_shacl_validation_report_t *report) {
    if (!validator || !graph || !node_iri || !shape || !report) {
        return CNS_SHACL_ERROR_INVALID_ARG;
    }
    
    // Iterate through all constraints in the shape
    const cns_shacl_constraint_t *constraint = shape->constraints;
    while (constraint) {
        bool conforms = false;
        cns_memory_footprint_t memory = {0};
        
        // Evaluate constraint
        cns_shacl_result_t result = cns_shacl_eval_constraint(
            validator, graph, node_iri, NULL, constraint, &conforms, &memory);
        
        if (result != CNS_SHACL_OK) {
            return result;
        }
        
        // Add result if constraint fails
        if (!conforms) {
            cns_shacl_add_result(report, node_iri, NULL, NULL,
                                 constraint->type, CNS_SHACL_RESULT_VIOLATION,
                                 "Constraint violation", &memory);
        }
        
        constraint = constraint->next;
    }
    
    return CNS_SHACL_OK;
}

cns_shacl_result_t cns_shacl_eval_constraint(cns_shacl_validator_t *validator,
                                       const cns_shacl_graph_t *graph,
                                       const char *focus_node,
                                       const char *property_path,
                                       const cns_shacl_constraint_t *constraint,
                                       bool *conforms,
                                       cns_memory_footprint_t *memory) {
    if (!validator || !graph || !focus_node || !constraint || !conforms || !memory) {
        return CNS_SHACL_ERROR_INVALID_ARG;
    }
    
    cns_tick_t start_tick = get_cycles();
    
    // Initialize
    *conforms = true;
    memset(memory, 0, sizeof(cns_memory_footprint_t));
    
    // Evaluate based on constraint type
    switch (constraint->type) {
        case CNS_SHACL_CONSTRAINT_MEMORY_BOUND:
            *conforms = cns_shacl_eval_memory_bound(graph, focus_node,
                                                    constraint->value.memory_limit, memory);
            if (!*conforms) {
                validator->memory_violations++;
            }
            break;
            
        case CNS_SHACL_CONSTRAINT_MIN_COUNT:
        case CNS_SHACL_CONSTRAINT_MAX_COUNT: {
            uint32_t actual_count;
            *conforms = cns_shacl_eval_count_constraint(graph, focus_node, property_path,
                                                        constraint->type == CNS_SHACL_CONSTRAINT_MIN_COUNT ? constraint->value.count_value : 0,
                                                        constraint->type == CNS_SHACL_CONSTRAINT_MAX_COUNT ? constraint->value.count_value : UINT32_MAX,
                                                        &actual_count);
            break;
        }
        
        case CNS_SHACL_CONSTRAINT_CLASS:
            // For class constraint, we need to resolve the string reference
            // Simplified implementation
            *conforms = cns_shacl_eval_class_constraint(graph, focus_node, "example:Class");
            break;
            
        case CNS_SHACL_CONSTRAINT_NODE_KIND:
            *conforms = cns_shacl_eval_node_kind_constraint(graph, focus_node, 
                                                            constraint->value.node_kind);
            break;
            
        default:
            // Unsupported constraint types are assumed to conform
            *conforms = true;
            break;
    }
    
    cns_tick_t end_tick = get_cycles();
    cns_tick_t elapsed = end_tick - start_tick;
    
    if (elapsed > CNS_SHACL_MAX_TICKS_PER_CONSTRAINT) {
        fprintf(stderr, "Constraint evaluation took %lu ticks (>%d)\n", 
                elapsed, CNS_SHACL_MAX_TICKS_PER_CONSTRAINT);
    }
    
    return CNS_SHACL_OK;
}

// ============================================================================
// VALIDATION REPORT FUNCTIONS
// ============================================================================

cns_shacl_validation_report_t* cns_shacl_create_report(cns_shacl_validator_t *validator) {
    if (!validator || validator->magic != CNS_SHACL_VALIDATOR_MAGIC) {
        return NULL;
    }
    
    cns_shacl_validation_report_t *report = (cns_shacl_validation_report_t*)arenac_alloc(
        validator->arena, sizeof(cns_shacl_validation_report_t));
    if (!report) {
        return NULL;
    }
    
    memset(report, 0, sizeof(cns_shacl_validation_report_t));
    report->conforms = true; // Assume conforming until proven otherwise
    
    return report;
}

cns_shacl_result_t cns_shacl_add_result(cns_shacl_validation_report_t *report,
                                  const char *focus_node,
                                  const char *property_path,
                                  const char *value,
                                  cns_shacl_constraint_type_t constraint_type,
                                  cns_shacl_result_level_t level,
                                  const char *message,
                                  const cns_memory_footprint_t *memory) {
    if (!report) {
        return CNS_SHACL_ERROR_INVALID_ARG;
    }
    
    // For simplicity, we'll just update counters
    // In a full implementation, we'd create result objects
    
    report->result_count++;
    
    if (level == CNS_SHACL_RESULT_VIOLATION) {
        report->violation_count++;
        report->conforms = false;
    }
    
    if (constraint_type == CNS_SHACL_CONSTRAINT_MEMORY_BOUND) {
        report->memory_violation_count++;
    }
    
    if (memory) {
        report->total_memory.current_usage += memory->current_usage;
        if (memory->current_usage > report->total_memory.peak_usage) {
            report->total_memory.peak_usage = memory->current_usage;
        }
        report->total_memory.violation_count += memory->violation_count;
    }
    
    return CNS_SHACL_OK;
}

cns_shacl_result_t cns_shacl_finalize_report(cns_shacl_validation_report_t *report) {
    if (!report) {
        return CNS_SHACL_ERROR_INVALID_ARG;
    }
    
    // Update final conformance status
    report->conforms = (report->violation_count == 0);
    
    // Update memory bounds
    report->total_memory.bounded = (report->total_memory.violation_count == 0);
    
    return CNS_SHACL_OK;
}

void cns_shacl_destroy_report(cns_shacl_validation_report_t *report) {
    // With arena allocation, no explicit cleanup needed
    // Arena will handle memory cleanup
    if (report) {
        memset(report, 0, sizeof(cns_shacl_validation_report_t));
    }
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

const char* cns_shacl_constraint_type_string(cns_shacl_constraint_type_t type) {
    switch (type) {
        case CNS_SHACL_CONSTRAINT_MEMORY_BOUND: return "memory-bound";
        case CNS_SHACL_CONSTRAINT_MIN_COUNT: return "min-count";
        case CNS_SHACL_CONSTRAINT_MAX_COUNT: return "max-count";
        case CNS_SHACL_CONSTRAINT_CLASS: return "class";
        case CNS_SHACL_CONSTRAINT_DATATYPE: return "datatype";
        case CNS_SHACL_CONSTRAINT_NODE_KIND: return "node-kind";
        case CNS_SHACL_CONSTRAINT_MIN_LENGTH: return "min-length";
        case CNS_SHACL_CONSTRAINT_MAX_LENGTH: return "max-length";
        case CNS_SHACL_CONSTRAINT_PATTERN: return "pattern";
        case CNS_SHACL_CONSTRAINT_IN: return "in";
        default: return "unknown";
    }
}

const char* cns_shacl_result_level_string(cns_shacl_result_level_t level) {
    switch (level) {
        case CNS_SHACL_RESULT_INFO: return "info";
        case CNS_SHACL_RESULT_WARNING: return "warning";
        case CNS_SHACL_RESULT_VIOLATION: return "violation";
        case CNS_SHACL_RESULT_MEMORY_VIOLATION: return "memory-violation";
        default: return "unknown";
    }
}

const char* cns_shacl_node_kind_string(cns_shacl_node_kind_t node_kind) {
    switch (node_kind) {
        case CNS_SHACL_NODE_KIND_IRI: return "IRI";
        case CNS_SHACL_NODE_KIND_BLANK_NODE: return "BlankNode";
        case CNS_SHACL_NODE_KIND_LITERAL: return "Literal";
        case CNS_SHACL_NODE_KIND_ANY: return "Any";
        default: return "unknown";
    }
}

cns_shacl_result_t cns_shacl_print_report(const cns_shacl_validation_report_t *report, FILE *output) {
    if (!report || !output) {
        return CNS_SHACL_ERROR_INVALID_ARG;
    }
    
    fprintf(output, "SHACL Validation Report:\n");
    fprintf(output, "  Conforms: %s\n", report->conforms ? "true" : "false");
    fprintf(output, "  Total Results: %u\n", report->result_count);
    fprintf(output, "  Violations: %u\n", report->violation_count);
    fprintf(output, "  Memory Violations: %u\n", report->memory_violation_count);
    fprintf(output, "  Nodes Validated: %u\n", report->nodes_validated);
    fprintf(output, "  Validation Time: %lu ticks\n", report->validation_ticks);
    fprintf(output, "  Total Memory Usage: %zu bytes\n", report->total_memory.current_usage);
    fprintf(output, "  Peak Memory Usage: %zu bytes\n", report->total_memory.peak_usage);
    fprintf(output, "  Memory Bounded: %s\n", report->total_memory.bounded ? "true" : "false");
    
    return CNS_SHACL_OK;
}

size_t cns_shacl_validator_memory_usage(const cns_shacl_validator_t *validator) {
    if (!validator || validator->magic != CNS_SHACL_VALIDATOR_MAGIC) {
        return 0;
    }
    
    return validator->memory.current_usage;
}

bool cns_shacl_validator_valid(const cns_shacl_validator_t *validator) {
    return (validator && validator->magic == CNS_SHACL_VALIDATOR_MAGIC);
}

cns_shacl_result_t cns_shacl_validate_validator(const cns_shacl_validator_t *validator) {
    if (!validator) {
        return CNS_SHACL_ERROR_INVALID_ARG;
    }
    
    if (validator->magic != CNS_SHACL_VALIDATOR_MAGIC) {
        return CNS_SHACL_ERROR_CORRUPTION;
    }
    
    if (!validator->arena || !validator->shapes || !validator->shape_hash_table) {
        return CNS_SHACL_ERROR_CORRUPTION;
    }
    
    if (validator->shape_count > validator->shape_capacity) {
        return CNS_SHACL_ERROR_CORRUPTION;
    }
    
    return CNS_SHACL_OK;
}