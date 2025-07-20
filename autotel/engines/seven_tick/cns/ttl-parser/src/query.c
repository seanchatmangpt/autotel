/**
 * @file query.c
 * @brief Simple triple query engine implementation
 * @author CNS Seven-Tick Engine
 * @date 2024
 * 
 * Implements 80/20 query functionality focusing on basic triple pattern matching.
 * Uses the AST visitor pattern to efficiently scan for matching triples.
 */

#include "query.h"
#include "visitor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <regex.h>

/* Internal structures for query processing */
typedef struct {
    ttl_query_engine_t *engine;
    ttl_query_pattern_t *pattern;
    ttl_query_result_t *result;
    size_t matches_found;
} query_visitor_context_t;

/* Helper functions */
static char* ttl_string_duplicate(const char *str);
static bool ttl_pattern_element_matches(const ttl_pattern_element_t *element, ttl_ast_node_t *node);
static char* ttl_ast_node_to_string(ttl_ast_node_t *node);
static bool ttl_is_variable(const char *str);
static ttl_pattern_element_t ttl_parse_pattern_element(const char *str);
static void ttl_collect_triples_visitor(ttl_ast_node_t *node, void *user_data);
static bool ttl_query_triple_visitor(ttl_ast_visitor_t *visitor, ttl_ast_node_t *node);
static bool ttl_apply_filters(ttl_query_pattern_t *pattern, ttl_query_binding_t *bindings, size_t binding_count);
static void ttl_add_binding_to_result(ttl_query_result_t *result, ttl_query_binding_t *bindings, size_t binding_count);

/* String utility functions */
static char* ttl_string_duplicate(const char *str) {
    if (!str) return NULL;
    
    size_t len = strlen(str);
    char *dup = malloc(len + 1);
    if (dup) {
        strcpy(dup, str);
    }
    return dup;
}

__attribute__((unused))
static bool ttl_is_variable(const char *str) {
    return str && str[0] == '?';
}

static ttl_pattern_element_t ttl_parse_pattern_element(const char *str) {
    ttl_pattern_element_t element = {0};
    
    if (!str || strlen(str) == 0) {
        element.type = TTL_PATTERN_WILDCARD;
    } else if (str[0] == '?') {
        element.type = TTL_PATTERN_VARIABLE;
        element.data.variable_name = ttl_string_duplicate(str + 1); // Skip '?'
    } else if (str[0] == '*') {
        element.type = TTL_PATTERN_WILDCARD;
    } else {
        element.type = TTL_PATTERN_FIXED;
        element.data.fixed_value = ttl_string_duplicate(str);
    }
    
    return element;
}

/* AST node string conversion */
static char* ttl_ast_node_to_string(ttl_ast_node_t *node) {
    if (!node) return NULL;
    
    switch (node->type) {
        case TTL_AST_IRI:
            return ttl_string_duplicate(node->data.iri.value);
            
        case TTL_AST_PREFIXED_NAME: {
            size_t prefix_len = node->data.prefixed_name.prefix ? strlen(node->data.prefixed_name.prefix) : 0;
            size_t local_len = node->data.prefixed_name.local_name ? strlen(node->data.prefixed_name.local_name) : 0;
            char *result = malloc(prefix_len + local_len + 2); // +2 for ':' and null terminator
            if (result) {
                snprintf(result, prefix_len + local_len + 2, "%s:%s",
                        node->data.prefixed_name.prefix ? node->data.prefixed_name.prefix : "",
                        node->data.prefixed_name.local_name ? node->data.prefixed_name.local_name : "");
            }
            return result;
        }
        
        case TTL_AST_STRING_LITERAL:
            return ttl_string_duplicate(node->data.string_literal.value);
            
        case TTL_AST_NUMERIC_LITERAL:
            return ttl_string_duplicate(node->data.numeric_literal.lexical_form);
            
        case TTL_AST_BOOLEAN_LITERAL: {
            return ttl_string_duplicate(node->data.boolean_literal.value ? "true" : "false");
        }
        
        case TTL_AST_BLANK_NODE: {
            if (node->data.blank_node.label) {
                size_t len = strlen(node->data.blank_node.label) + 3; // +3 for "_:" and null terminator
                char *result = malloc(len);
                if (result) {
                    snprintf(result, len, "_:%s", node->data.blank_node.label);
                }
                return result;
            } else {
                char *result = malloc(16);
                if (result) {
                    snprintf(result, 16, "_:b%u", node->data.blank_node.id);
                }
                return result;
            }
        }
        
        case TTL_AST_RDF_TYPE:
            return ttl_string_duplicate("a");
            
        default:
            return NULL;
    }
}

/* Pattern matching functions */
static bool ttl_pattern_element_matches(const ttl_pattern_element_t *element, ttl_ast_node_t *node) {
    if (!element || !node) return false;
    
    switch (element->type) {
        case TTL_PATTERN_WILDCARD:
            return true;
            
        case TTL_PATTERN_VARIABLE:
            return true; // Variables always match, binding happens later
            
        case TTL_PATTERN_FIXED: {
            char *node_str = ttl_ast_node_to_string(node);
            if (!node_str) return false;
            
            bool matches = strcmp(element->data.fixed_value, node_str) == 0;
            free(node_str);
            return matches;
        }
        
        default:
            return false;
    }
}

/* Triple collection visitor for indexing */
static void ttl_collect_triples_visitor(ttl_ast_node_t *node, void *user_data) {
    if (node->type != TTL_AST_TRIPLE) return;
    
    ttl_query_engine_t *engine = (ttl_query_engine_t*)user_data;
    
    // Resize array if needed
    if (engine->triple_count >= 1000) { // Simple fixed limit for 80/20 implementation
        return;
    }
    
    engine->triples[engine->triple_count++] = node;
}

/* Query visitor for pattern matching */
static bool ttl_query_triple_visitor(ttl_ast_visitor_t *visitor, ttl_ast_node_t *node) {
    if (node->type != TTL_AST_TRIPLE) return true;
    
    query_visitor_context_t *ctx = (query_visitor_context_t*)visitor->user_data;
    ttl_query_pattern_t *pattern = ctx->pattern;
    
    // Extract subject, predicate, object from triple
    ttl_ast_node_t *subject = node->data.triple.subject;
    if (!subject) return true;
    
    // Navigate to predicate-object list
    ttl_ast_node_t *po_list = node->data.triple.predicate_object_list;
    if (!po_list || po_list->type != TTL_AST_PREDICATE_OBJECT_LIST) return true;
    
    // For simplicity in 80/20 implementation, handle first predicate-object pair
    if (po_list->data.predicate_object_list.item_count == 0) return true;
    
    ttl_ast_node_t *po_item = po_list->data.predicate_object_list.items[0];
    if (!po_item) return true;
    
    // Find predicate and object (simplified AST navigation for 80/20)
    // This would need more robust AST navigation in full implementation
    ttl_ast_node_t *predicate = NULL;
    ttl_ast_node_t *object = NULL;
    
    // For now, assume simple structure and extract first predicate/object
    // In full implementation, this would properly traverse the AST structure
    if (po_item->type == TTL_AST_PREDICATE) {
        predicate = po_item;
        // Find associated object - simplified for 80/20
        if (po_list->data.predicate_object_list.item_count > 1) {
            object = po_list->data.predicate_object_list.items[1];
        }
    }
    
    if (!predicate || !object) return true;
    
    // Check if pattern matches
    if (!ttl_pattern_element_matches(&pattern->subject, subject) ||
        !ttl_pattern_element_matches(&pattern->predicate, predicate) ||
        !ttl_pattern_element_matches(&pattern->object, object)) {
        return true;
    }
    
    // Create bindings for variables
    ttl_query_binding_t bindings[3]; // Max 3 variables (s, p, o)
    size_t binding_count = 0;
    
    if (pattern->subject.type == TTL_PATTERN_VARIABLE) {
        bindings[binding_count].variable_name = ttl_string_duplicate(pattern->subject.data.variable_name);
        bindings[binding_count].value = subject;
        bindings[binding_count].string_value = ttl_ast_node_to_string(subject);
        binding_count++;
    }
    
    if (pattern->predicate.type == TTL_PATTERN_VARIABLE) {
        bindings[binding_count].variable_name = ttl_string_duplicate(pattern->predicate.data.variable_name);
        bindings[binding_count].value = predicate;
        bindings[binding_count].string_value = ttl_ast_node_to_string(predicate);
        binding_count++;
    }
    
    if (pattern->object.type == TTL_PATTERN_VARIABLE) {
        bindings[binding_count].variable_name = ttl_string_duplicate(pattern->object.data.variable_name);
        bindings[binding_count].value = object;
        bindings[binding_count].string_value = ttl_ast_node_to_string(object);
        binding_count++;
    }
    
    // Apply filters if any
    if (pattern->filter_count == 0 || ttl_apply_filters(pattern, bindings, binding_count)) {
        ttl_add_binding_to_result(ctx->result, bindings, binding_count);
        ctx->matches_found++;
    }
    
    // Clean up temporary bindings
    for (size_t i = 0; i < binding_count; i++) {
        free(bindings[i].variable_name);
        free(bindings[i].string_value);
    }
    
    return true;
}

/* Filter application */
static bool ttl_apply_filters(ttl_query_pattern_t *pattern, ttl_query_binding_t *bindings, size_t binding_count) {
    // Simplified filter implementation for 80/20
    for (size_t f = 0; f < pattern->filter_count; f++) {
        ttl_query_filter_t *filter = &pattern->filters[f];
        
        // Find target variable in bindings
        ttl_query_binding_t *target_binding = NULL;
        for (size_t b = 0; b < binding_count; b++) {
            if (strcmp(bindings[b].variable_name, filter->target_variable) == 0) {
                target_binding = &bindings[b];
                break;
            }
        }
        
        if (!target_binding) continue;
        
        switch (filter->type) {
            case TTL_FILTER_STRING_LITERAL:
                if (strcmp(target_binding->string_value, filter->data.string_literal.value) != 0) {
                    return false;
                }
                break;
                
            case TTL_FILTER_STRING_REGEX: {
                regex_t regex;
                int flags = REG_EXTENDED;
                if (filter->data.string_regex.case_insensitive) {
                    flags |= REG_ICASE;
                }
                
                if (regcomp(&regex, filter->data.string_regex.pattern, flags) == 0) {
                    bool matches = regexec(&regex, target_binding->string_value, 0, NULL, 0) == 0;
                    regfree(&regex);
                    if (!matches) return false;
                } else {
                    return false; // Regex compilation failed
                }
                break;
            }
            
            case TTL_FILTER_NUMERIC_RANGE: {
                // Simple numeric filter - convert string to double
                double value = strtod(target_binding->string_value, NULL);
                if (value < filter->data.numeric_range.min_value || 
                    value > filter->data.numeric_range.max_value) {
                    return false;
                }
                break;
            }
            
            default:
                // Other filter types not implemented in 80/20 version
                break;
        }
    }
    
    return true;
}

/* Add binding to result set */
static void ttl_add_binding_to_result(ttl_query_result_t *result, ttl_query_binding_t *bindings, size_t binding_count) {
    // Resize rows array if needed
    if (result->row_count >= result->binding_capacity) {
        size_t new_capacity = result->binding_capacity == 0 ? 16 : result->binding_capacity * 2;
        ttl_query_binding_t **new_rows = realloc(result->rows, new_capacity * sizeof(ttl_query_binding_t*));
        if (!new_rows) return;
        
        result->rows = new_rows;
        result->binding_capacity = new_capacity;
    }
    
    // Allocate bindings for this row
    ttl_query_binding_t *row_bindings = malloc(binding_count * sizeof(ttl_query_binding_t));
    if (!row_bindings) return;
    
    // Copy bindings
    for (size_t i = 0; i < binding_count; i++) {
        row_bindings[i].variable_name = ttl_string_duplicate(bindings[i].variable_name);
        row_bindings[i].value = bindings[i].value;
        row_bindings[i].string_value = ttl_string_duplicate(bindings[i].string_value);
    }
    
    result->rows[result->row_count] = row_bindings;
    if (result->row_count == 0) {
        result->binding_count = binding_count;
        result->bindings = row_bindings; // Point to first row for compatibility
    }
    result->row_count++;
}

/* Public API Implementation */

ttl_query_engine_t* ttl_query_engine_create(ttl_ast_node_t *document, ttl_ast_context_t *context) {
    if (!document) return NULL;
    
    ttl_query_engine_t *engine = calloc(1, sizeof(ttl_query_engine_t));
    if (!engine) return NULL;
    
    engine->document = document;
    engine->context = context;
    
    // Allocate space for triple index (simplified for 80/20)
    engine->triples = malloc(1000 * sizeof(ttl_ast_node_t*));
    if (!engine->triples) {
        free(engine);
        return NULL;
    }
    
    // Index all triples in the document
    ttl_ast_walk_nodes_of_type(document, TTL_AST_TRIPLE, ttl_collect_triples_visitor, engine);
    
    return engine;
}

void ttl_query_engine_destroy(ttl_query_engine_t *engine) {
    if (!engine) return;
    
    free(engine->triples);
    free(engine);
}

ttl_query_pattern_t* ttl_query_pattern_create(const char *subject_pattern,
                                              const char *predicate_pattern,
                                              const char *object_pattern) {
    ttl_query_pattern_t *pattern = calloc(1, sizeof(ttl_query_pattern_t));
    if (!pattern) return NULL;
    
    pattern->subject = ttl_parse_pattern_element(subject_pattern);
    pattern->predicate = ttl_parse_pattern_element(predicate_pattern);
    pattern->object = ttl_parse_pattern_element(object_pattern);
    
    return pattern;
}

void ttl_query_pattern_destroy(ttl_query_pattern_t *pattern) {
    if (!pattern) return;
    
    if (pattern->subject.type == TTL_PATTERN_VARIABLE) {
        free(pattern->subject.data.variable_name);
    } else if (pattern->subject.type == TTL_PATTERN_FIXED) {
        free(pattern->subject.data.fixed_value);
    }
    
    if (pattern->predicate.type == TTL_PATTERN_VARIABLE) {
        free(pattern->predicate.data.variable_name);
    } else if (pattern->predicate.type == TTL_PATTERN_FIXED) {
        free(pattern->predicate.data.fixed_value);
    }
    
    if (pattern->object.type == TTL_PATTERN_VARIABLE) {
        free(pattern->object.data.variable_name);
    } else if (pattern->object.type == TTL_PATTERN_FIXED) {
        free(pattern->object.data.fixed_value);
    }
    
    free(pattern->filters);
    free(pattern);
}

ttl_query_result_t* ttl_query_execute(ttl_query_engine_t *engine, ttl_query_pattern_t *pattern) {
    if (!engine || !pattern) return NULL;
    
    ttl_query_result_t *result = calloc(1, sizeof(ttl_query_result_t));
    if (!result) return NULL;
    
    // Create visitor context
    query_visitor_context_t ctx = {
        .engine = engine,
        .pattern = pattern,
        .result = result,
        .matches_found = 0
    };
    
    // Create and configure visitor
    ttl_ast_visitor_t *visitor = ttl_visitor_create();
    if (!visitor) {
        free(result);
        return NULL;
    }
    
    visitor->user_data = &ctx;
    visitor->visit_triple = ttl_query_triple_visitor;
    
    // Execute query
    ttl_ast_accept(engine->document, visitor);
    
    // Update statistics
    engine->stats.queries_executed++;
    engine->stats.patterns_matched += ctx.matches_found;
    engine->stats.total_results += result->row_count;
    
    ttl_visitor_destroy(visitor);
    return result;
}

ttl_query_result_t* ttl_query_execute_simple(ttl_query_engine_t *engine, const char *query_string) {
    if (!engine || !query_string) return NULL;
    
    // Simple parser for basic triple patterns like "?s rdf:type foaf:Person"
    char *query_copy = ttl_string_duplicate(query_string);
    if (!query_copy) return NULL;
    
    char *subject = NULL, *predicate = NULL, *object = NULL;
    char *token = strtok(query_copy, " \t\n");
    
    if (token) {
        subject = ttl_string_duplicate(token);
        token = strtok(NULL, " \t\n");
    }
    if (token) {
        predicate = ttl_string_duplicate(token);
        token = strtok(NULL, " \t\n");
    }
    if (token) {
        object = ttl_string_duplicate(token);
    }
    
    free(query_copy);
    
    if (!subject || !predicate || !object) {
        free(subject);
        free(predicate);
        free(object);
        return NULL;
    }
    
    ttl_query_pattern_t *pattern = ttl_query_pattern_create(subject, predicate, object);
    free(subject);
    free(predicate);
    free(object);
    
    if (!pattern) return NULL;
    
    ttl_query_result_t *result = ttl_query_execute(engine, pattern);
    ttl_query_pattern_destroy(pattern);
    
    return result;
}

size_t ttl_query_result_count(const ttl_query_result_t *result) {
    return result ? result->row_count : 0;
}

void ttl_query_result_get_variables(const ttl_query_result_t *result, 
                                   char ***variable_names, 
                                   size_t *count) {
    if (!result || !variable_names || !count) return;
    
    *variable_names = NULL;
    *count = 0;
    
    if (result->row_count == 0) return;
    
    // Get variables from first row
    ttl_query_binding_t *first_row = result->rows[0];
    if (!first_row) return;
    
    char **names = malloc(result->binding_count * sizeof(char*));
    if (!names) return;
    
    for (size_t i = 0; i < result->binding_count; i++) {
        names[i] = ttl_string_duplicate(first_row[i].variable_name);
    }
    
    *variable_names = names;
    *count = result->binding_count;
}

const ttl_query_binding_t* ttl_query_result_get_binding(const ttl_query_result_t *result,
                                                       const char *variable_name) {
    if (!result || !variable_name || result->current_row >= result->row_count) return NULL;
    
    ttl_query_binding_t *current_bindings = result->rows[result->current_row];
    if (!current_bindings) return NULL;
    
    for (size_t i = 0; i < result->binding_count; i++) {
        if (strcmp(current_bindings[i].variable_name, variable_name) == 0) {
            return &current_bindings[i];
        }
    }
    
    return NULL;
}

bool ttl_query_result_next(ttl_query_result_t *result) {
    if (!result) return false;
    
    if (result->current_row + 1 < result->row_count) {
        result->current_row++;
        return true;
    }
    
    return false;
}

void ttl_query_result_reset(ttl_query_result_t *result) {
    if (result) {
        result->current_row = 0;
    }
}

void ttl_query_result_destroy(ttl_query_result_t *result) {
    if (!result) return;
    
    // Free all rows
    for (size_t r = 0; r < result->row_count; r++) {
        ttl_query_binding_t *row = result->rows[r];
        if (row) {
            for (size_t b = 0; b < result->binding_count; b++) {
                free(row[b].variable_name);
                free(row[b].string_value);
            }
            free(row);
        }
    }
    
    free(result->rows);
    free(result);
}

/* Utility functions */

ttl_query_result_t* ttl_query_find_by_predicate(ttl_query_engine_t *engine, const char *predicate) {
    char query[256];
    snprintf(query, sizeof(query), "?s %s ?o", predicate);
    return ttl_query_execute_simple(engine, query);
}

ttl_query_result_t* ttl_query_find_by_subject(ttl_query_engine_t *engine, const char *subject) {
    char query[256];
    snprintf(query, sizeof(query), "%s ?p ?o", subject);
    return ttl_query_execute_simple(engine, query);
}

ttl_query_result_t* ttl_query_find_instances(ttl_query_engine_t *engine, const char *type_iri) {
    char query[256];
    snprintf(query, sizeof(query), "?s a %s", type_iri);
    return ttl_query_execute_simple(engine, query);
}

void ttl_query_engine_get_stats(const ttl_query_engine_t *engine,
                               size_t *queries_executed,
                               size_t *patterns_matched,
                               size_t *total_results) {
    if (!engine) return;
    
    if (queries_executed) *queries_executed = engine->stats.queries_executed;
    if (patterns_matched) *patterns_matched = engine->stats.patterns_matched;
    if (total_results) *total_results = engine->stats.total_results;
}

void ttl_query_result_print(const ttl_query_result_t *result, FILE *output) {
    if (!result || !output) return;
    
    if (result->row_count == 0) {
        fprintf(output, "No results found.\n");
        return;
    }
    
    // Print header
    ttl_query_binding_t *first_row = result->rows[0];
    for (size_t i = 0; i < result->binding_count; i++) {
        fprintf(output, "?%s", first_row[i].variable_name);
        if (i < result->binding_count - 1) fprintf(output, "\t");
    }
    fprintf(output, "\n");
    
    // Print separator
    for (size_t i = 0; i < result->binding_count; i++) {
        fprintf(output, "---");
        if (i < result->binding_count - 1) fprintf(output, "\t");
    }
    fprintf(output, "\n");
    
    // Print rows
    for (size_t r = 0; r < result->row_count; r++) {
        ttl_query_binding_t *row = result->rows[r];
        for (size_t b = 0; b < result->binding_count; b++) {
            fprintf(output, "%s", row[b].string_value ? row[b].string_value : "");
            if (b < result->binding_count - 1) fprintf(output, "\t");
        }
        fprintf(output, "\n");
    }
    
    fprintf(output, "\nTotal results: %zu\n", result->row_count);
}

void ttl_query_result_print_json(const ttl_query_result_t *result, FILE *output) {
    if (!result || !output) return;
    
    fprintf(output, "{\n");
    fprintf(output, "  \"results\": {\n");
    fprintf(output, "    \"bindings\": [\n");
    
    for (size_t r = 0; r < result->row_count; r++) {
        ttl_query_binding_t *row = result->rows[r];
        fprintf(output, "      {");
        
        for (size_t b = 0; b < result->binding_count; b++) {
            fprintf(output, "\n        \"%s\": {", row[b].variable_name);
            fprintf(output, "\"type\": \"literal\", ");
            fprintf(output, "\"value\": \"%s\"}", row[b].string_value ? row[b].string_value : "");
            if (b < result->binding_count - 1) fprintf(output, ",");
        }
        
        fprintf(output, "\n      }");
        if (r < result->row_count - 1) fprintf(output, ",");
        fprintf(output, "\n");
    }
    
    fprintf(output, "    ]\n");
    fprintf(output, "  },\n");
    fprintf(output, "  \"count\": %zu\n", result->row_count);
    fprintf(output, "}\n");
}