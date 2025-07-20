#include "../include/visitor.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

// Create visitor
ttl_ast_visitor_t* ttl_visitor_create(void) {
    ttl_ast_visitor_t *visitor = calloc(1, sizeof(ttl_ast_visitor_t));
    if (!visitor) return NULL;
    
    visitor->order = TTL_VISITOR_PRE_ORDER;
    visitor->control = TTL_VISITOR_CONTINUE;
    
    return visitor;
}

// Destroy visitor
void ttl_visitor_destroy(ttl_ast_visitor_t *visitor) {
    free(visitor);
}

// Reset visitor state
void ttl_visitor_reset(ttl_ast_visitor_t *visitor) {
    if (!visitor) return;
    
    visitor->control = TTL_VISITOR_CONTINUE;
    visitor->state.depth = 0;
    visitor->state.nodes_visited = 0;
    visitor->state.current_node = NULL;
    visitor->state.parent_node = NULL;
}

// Set user data
void ttl_visitor_set_user_data(ttl_ast_visitor_t *visitor, void *user_data) {
    if (visitor) {
        visitor->user_data = user_data;
    }
}

// Set traversal order
void ttl_visitor_set_order(ttl_ast_visitor_t *visitor, ttl_visitor_order_t order) {
    if (visitor) {
        visitor->order = order;
    }
}

// Internal traversal helper
static bool visit_node(ttl_ast_node_t *node, ttl_ast_visitor_t *visitor) {
    if (!node || !visitor) return false;
    
    // Save parent state
    ttl_ast_node_t *saved_parent = visitor->state.parent_node;
    ttl_ast_node_t *saved_current = visitor->state.current_node;
    
    // Update state
    visitor->state.parent_node = visitor->state.current_node;
    visitor->state.current_node = node;
    visitor->state.nodes_visited++;
    
    // Call generic enter function
    if (visitor->visit_enter) {
        if (!visitor->visit_enter(visitor, node)) {
            visitor->control = TTL_VISITOR_STOP;
            goto cleanup;
        }
    }
    
    // Pre-order visit
    if (visitor->order == TTL_VISITOR_PRE_ORDER || visitor->order == TTL_VISITOR_IN_ORDER) {
        ttl_visit_func_t visit_func = NULL;
        
        // Select appropriate visit function
        switch (node->type) {
            case TTL_AST_DOCUMENT:
                visit_func = visitor->visit_document;
                break;
            case TTL_AST_PREFIX_DIRECTIVE:
                visit_func = visitor->visit_prefix_directive;
                break;
            case TTL_AST_BASE_DIRECTIVE:
                visit_func = visitor->visit_base_directive;
                break;
            case TTL_AST_TRIPLE:
                visit_func = visitor->visit_triple;
                break;
            case TTL_AST_IRI:
                visit_func = visitor->visit_iri;
                break;
            case TTL_AST_PREFIXED_NAME:
                visit_func = visitor->visit_prefixed_name;
                break;
            case TTL_AST_BLANK_NODE:
                visit_func = visitor->visit_blank_node;
                break;
            case TTL_AST_BLANK_NODE_LABEL:
                visit_func = visitor->visit_blank_node_label;
                break;
            case TTL_AST_STRING_LITERAL:
                visit_func = visitor->visit_string_literal;
                break;
            case TTL_AST_NUMERIC_LITERAL:
                visit_func = visitor->visit_numeric_literal;
                break;
            case TTL_AST_BOOLEAN_LITERAL:
                visit_func = visitor->visit_boolean_literal;
                break;
            case TTL_AST_TYPED_LITERAL:
                visit_func = visitor->visit_typed_literal;
                break;
            case TTL_AST_LANG_LITERAL:
                visit_func = visitor->visit_lang_literal;
                break;
            case TTL_AST_COLLECTION:
                visit_func = visitor->visit_collection;
                break;
            case TTL_AST_BLANK_NODE_PROPERTY_LIST:
                visit_func = visitor->visit_blank_node_property_list;
                break;
            case TTL_AST_PREDICATE_OBJECT_LIST:
                visit_func = visitor->visit_predicate_object_list;
                break;
            case TTL_AST_OBJECT_LIST:
                visit_func = visitor->visit_object_list;
                break;
            case TTL_AST_RDF_TYPE:
                visit_func = visitor->visit_rdf_type;
                break;
            case TTL_AST_COMMENT:
                visit_func = visitor->visit_comment;
                break;
            default:
                break;
        }
        
        // Call node-specific visit function
        if (visit_func) {
            if (!visit_func(visitor, node)) {
                visitor->control = TTL_VISITOR_STOP;
                goto cleanup;
            }
        }
    }
    
    // Visit children if not skipped
    if (visitor->control != TTL_VISITOR_SKIP_CHILDREN) {
        visitor->state.depth++;
        
        size_t child_count = ttl_ast_get_child_count(node);
        for (size_t i = 0; i < child_count; i++) {
            ttl_ast_node_t *child = ttl_ast_get_child(node, i);
            if (child && !visit_node(child, visitor)) {
                if (visitor->control == TTL_VISITOR_STOP) {
                    visitor->state.depth--;
                    goto cleanup;
                }
            }
            
            // Reset skip flag after each child
            if (visitor->control == TTL_VISITOR_SKIP_CHILDREN) {
                visitor->control = TTL_VISITOR_CONTINUE;
            }
        }
        
        visitor->state.depth--;
    }
    
    // Post-order visit
    if (visitor->order == TTL_VISITOR_POST_ORDER) {
        ttl_visit_func_t visit_func = NULL;
        
        // Use same visit functions as pre-order
        switch (node->type) {
            case TTL_AST_DOCUMENT:
                visit_func = visitor->visit_document;
                break;
            case TTL_AST_PREFIX_DIRECTIVE:
                visit_func = visitor->visit_prefix_directive;
                break;
            // ... (same as above)
            default:
                break;
        }
        
        if (visit_func && !visit_func(visitor, node)) {
            visitor->control = TTL_VISITOR_STOP;
        }
    }
    
    // Call exit functions
    ttl_visit_exit_func_t exit_func = NULL;
    switch (node->type) {
        case TTL_AST_DOCUMENT:
            exit_func = visitor->exit_document;
            break;
        case TTL_AST_PREFIX_DIRECTIVE:
            exit_func = visitor->exit_prefix_directive;
            break;
        case TTL_AST_BASE_DIRECTIVE:
            exit_func = visitor->exit_base_directive;
            break;
        case TTL_AST_TRIPLE:
            exit_func = visitor->exit_triple;
            break;
        case TTL_AST_COLLECTION:
            exit_func = visitor->exit_collection;
            break;
        case TTL_AST_BLANK_NODE_PROPERTY_LIST:
            exit_func = visitor->exit_blank_node_property_list;
            break;
        case TTL_AST_PREDICATE_OBJECT_LIST:
            exit_func = visitor->exit_predicate_object_list;
            break;
        case TTL_AST_OBJECT_LIST:
            exit_func = visitor->exit_object_list;
            break;
        default:
            break;
    }
    
    if (exit_func) {
        exit_func(visitor, node);
    }
    
    // Call generic exit function
    if (visitor->visit_exit) {
        visitor->visit_exit(visitor, node);
    }
    
cleanup:
    // Restore state
    visitor->state.parent_node = saved_parent;
    visitor->state.current_node = saved_current;
    
    return visitor->control != TTL_VISITOR_STOP;
}

// Main accept function
bool ttl_ast_accept(ttl_ast_node_t *node, ttl_ast_visitor_t *visitor) {
    if (!node || !visitor) return false;
    
    ttl_visitor_reset(visitor);
    return visit_node(node, visitor);
}

// Triple visitor implementation
typedef struct {
    ttl_triple_visitor_t *config;
    ttl_ast_node_t *current_subject;
    ttl_ast_node_t *current_predicate;
} triple_visitor_data_t;

static bool visit_triple_for_extraction(ttl_ast_visitor_t *visitor, ttl_ast_node_t *node) {
    triple_visitor_data_t *data = visitor->user_data;
    
    if (node->type == TTL_AST_TRIPLE) {
        data->current_subject = node->data.triple.subject;
    }
    
    return true;
}

static bool visit_predicate_object_list_for_extraction(ttl_ast_visitor_t *visitor, ttl_ast_node_t *node) {
    triple_visitor_data_t *data = visitor->user_data;
    
    if (node->type == TTL_AST_PREDICATE_OBJECT_LIST) {
        // Process predicate-object pairs
        for (size_t i = 0; i < node->data.predicate_object_list.item_count; i += 2) {
            ttl_ast_node_t *predicate = node->data.predicate_object_list.items[i];
            ttl_ast_node_t *object_list = node->data.predicate_object_list.items[i + 1];
            
            data->current_predicate = predicate;
            
            // Process each object in the object list
            if (object_list && object_list->type == TTL_AST_OBJECT_LIST) {
                for (size_t j = 0; j < object_list->data.object_list.object_count; j++) {
                    ttl_ast_node_t *object = object_list->data.object_list.objects[j];
                    
                    // Call the callback
                    if (data->config->on_triple) {
                        data->config->on_triple(data->config->user_data,
                                              data->current_subject,
                                              data->current_predicate,
                                              object);
                    }
                }
            }
        }
    }
    
    return true;
}

ttl_ast_visitor_t* ttl_create_triple_visitor(ttl_triple_visitor_t *config) {
    if (!config) return NULL;
    
    ttl_ast_visitor_t *visitor = ttl_visitor_create();
    if (!visitor) return NULL;
    
    triple_visitor_data_t *data = calloc(1, sizeof(triple_visitor_data_t));
    if (!data) {
        ttl_visitor_destroy(visitor);
        return NULL;
    }
    
    data->config = config;
    visitor->user_data = data;
    visitor->visit_triple = visit_triple_for_extraction;
    visitor->visit_predicate_object_list = visit_predicate_object_list_for_extraction;
    
    return visitor;
}

// Namespace visitor implementation
static bool visit_prefix_for_namespace(ttl_ast_visitor_t *visitor, ttl_ast_node_t *node) {
    ttl_namespace_visitor_t *config = visitor->user_data;
    
    if (node->type == TTL_AST_PREFIX_DIRECTIVE && config->on_prefix) {
        ttl_ast_node_t *prefix_node = node->data.directive.prefix;
        ttl_ast_node_t *iri_node = node->data.directive.iri;
        
        const char *prefix = "";
        if (prefix_node && prefix_node->type == TTL_AST_PREFIXED_NAME) {
            prefix = prefix_node->data.prefixed_name.prefix ? 
                    prefix_node->data.prefixed_name.prefix : "";
        }
        
        const char *iri = "";
        if (iri_node && iri_node->type == TTL_AST_IRI) {
            iri = iri_node->data.iri.value;
        }
        
        config->on_prefix(config->user_data, prefix, iri);
    }
    
    return true;
}

static bool visit_base_for_namespace(ttl_ast_visitor_t *visitor, ttl_ast_node_t *node) {
    ttl_namespace_visitor_t *config = visitor->user_data;
    
    if (node->type == TTL_AST_BASE_DIRECTIVE && config->on_base) {
        ttl_ast_node_t *iri_node = node->data.directive.iri;
        
        const char *iri = "";
        if (iri_node && iri_node->type == TTL_AST_IRI) {
            iri = iri_node->data.iri.value;
        }
        
        config->on_base(config->user_data, iri);
    }
    
    return true;
}

ttl_ast_visitor_t* ttl_create_namespace_visitor(ttl_namespace_visitor_t *config) {
    if (!config) return NULL;
    
    ttl_ast_visitor_t *visitor = ttl_visitor_create();
    if (!visitor) return NULL;
    
    visitor->user_data = config;
    visitor->visit_prefix_directive = visit_prefix_for_namespace;
    visitor->visit_base_directive = visit_base_for_namespace;
    
    return visitor;
}

// Statistics visitor implementation
static bool visit_for_stats(ttl_ast_visitor_t *visitor, ttl_ast_node_t *node) {
    ttl_ast_stats_t *stats = visitor->user_data;
    
    stats->total_nodes++;
    stats->nodes_by_type[node->type]++;
    
    int depth = ttl_ast_get_depth(node);
    if ((size_t)depth > stats->max_depth) {
        stats->max_depth = depth;
    }
    
    switch (node->type) {
        case TTL_AST_TRIPLE:
            stats->triple_count++;
            break;
        case TTL_AST_STRING_LITERAL:
        case TTL_AST_NUMERIC_LITERAL:
        case TTL_AST_BOOLEAN_LITERAL:
        case TTL_AST_TYPED_LITERAL:
        case TTL_AST_LANG_LITERAL:
            stats->literal_count++;
            break;
        case TTL_AST_IRI:
        case TTL_AST_PREFIXED_NAME:
            stats->iri_count++;
            break;
        case TTL_AST_BLANK_NODE:
        case TTL_AST_BLANK_NODE_LABEL:
            stats->blank_node_count++;
            break;
        case TTL_AST_COLLECTION:
            stats->collection_count++;
            break;
        default:
            break;
    }
    
    return true;
}

ttl_ast_visitor_t* ttl_create_stats_visitor(ttl_ast_stats_t *stats) {
    if (!stats) return NULL;
    
    memset(stats, 0, sizeof(ttl_ast_stats_t));
    
    ttl_ast_visitor_t *visitor = ttl_visitor_create();
    if (!visitor) return NULL;
    
    visitor->user_data = stats;
    visitor->visit_enter = visit_for_stats;
    
    return visitor;
}

// Validation visitor implementation
typedef struct {
    ttl_validation_visitor_t *config;
    bool has_errors;
} validation_data_t;

static bool validate_node(ttl_ast_visitor_t *visitor, ttl_ast_node_t *node) {
    validation_data_t *data = visitor->user_data;
    const char *error = NULL;
    
    // Basic validation checks
    switch (node->type) {
        case TTL_AST_IRI:
            if (!node->data.iri.value || strlen(node->data.iri.value) == 0) {
                error = "IRI cannot be empty";
            }
            break;
            
        case TTL_AST_PREFIXED_NAME:
            if (!node->data.prefixed_name.local_name) {
                error = "Prefixed name must have local name";
            }
            break;
            
        case TTL_AST_TRIPLE:
            if (!node->data.triple.subject || !node->data.triple.predicate_object_list) {
                error = "Triple must have subject and predicate-object list";
            }
            break;
            
        case TTL_AST_LANG_LITERAL:
            if (!node->data.lang_literal.language_tag || 
                strlen(node->data.lang_literal.language_tag) == 0) {
                error = "Language-tagged literal must have language tag";
            }
            break;
            
        default:
            break;
    }
    
    if (error && data->config->on_error) {
        data->has_errors = true;
        if (!data->config->on_error(data->config->user_data, node, error)) {
            visitor->control = TTL_VISITOR_STOP;
            return false;
        }
    }
    
    return true;
}

ttl_ast_visitor_t* ttl_create_validation_visitor(ttl_validation_visitor_t *config) {
    if (!config) return NULL;
    
    ttl_ast_visitor_t *visitor = ttl_visitor_create();
    if (!visitor) return NULL;
    
    validation_data_t *data = calloc(1, sizeof(validation_data_t));
    if (!data) {
        ttl_visitor_destroy(visitor);
        return NULL;
    }
    
    data->config = config;
    visitor->user_data = data;
    visitor->visit_enter = validate_node;
    
    return visitor;
}

// Pretty printer visitor implementation
static void print_indent(FILE *out, int depth, int indent_size) {
    for (int i = 0; i < depth * indent_size; i++) {
        fputc(' ', out);
    }
}

static bool print_node(ttl_ast_visitor_t *visitor, ttl_ast_node_t *node) {
    ttl_printer_visitor_t *config = visitor->user_data;
    FILE *out = config->output ? config->output : stdout;
    
    if (!config->compact_mode) {
        print_indent(out, visitor->state.depth, config->indent_size);
    }
    
    // Print node type
    fprintf(out, "%s", ttl_ast_node_type_to_string(node->type));
    
    // Print node content
    switch (node->type) {
        case TTL_AST_IRI:
            fprintf(out, " <%s>", node->data.iri.value);
            break;
            
        case TTL_AST_PREFIXED_NAME:
            fprintf(out, " %s:%s", 
                node->data.prefixed_name.prefix ? node->data.prefixed_name.prefix : "",
                node->data.prefixed_name.local_name);
            break;
            
        case TTL_AST_STRING_LITERAL:
            fprintf(out, " \"%s\"", node->data.string_literal.value);
            break;
            
        case TTL_AST_NUMERIC_LITERAL:
            fprintf(out, " %s", node->data.numeric_literal.lexical_form);
            break;
            
        case TTL_AST_BOOLEAN_LITERAL:
            fprintf(out, " %s", node->data.boolean_literal.value ? "true" : "false");
            break;
            
        default:
            break;
    }
    
    // Print location if requested
    if (config->show_location) {
        fprintf(out, " [%u:%u]", node->location.line, node->location.column);
    }
    
    if (!config->compact_mode) {
        fprintf(out, "\n");
    } else {
        fprintf(out, " ");
    }
    
    return true;
}

ttl_ast_visitor_t* ttl_create_printer_visitor(ttl_printer_visitor_t *config) {
    if (!config) return NULL;
    
    ttl_ast_visitor_t *visitor = ttl_visitor_create();
    if (!visitor) return NULL;
    
    visitor->user_data = config;
    visitor->visit_enter = print_node;
    
    return visitor;
}

// Helper callback for walk_nodes_of_type
static bool walk_nodes_visit_callback(ttl_ast_visitor_t *v, ttl_ast_node_t *node) {
    typedef struct {
        ttl_ast_node_type_t type;
        ttl_node_callback_t callback;
        void *user_data;
    } walk_data_t;
    
    walk_data_t *d = v->user_data;
    if (node->type == d->type) {
        d->callback(node, d->user_data);
    }
    return true;
}

// Utility functions
void ttl_ast_walk_nodes_of_type(ttl_ast_node_t *root,
                               ttl_ast_node_type_t type,
                               ttl_node_callback_t callback,
                               void *user_data) {
    if (!root || !callback) return;
    
    typedef struct {
        ttl_ast_node_type_t type;
        ttl_node_callback_t callback;
        void *user_data;
    } walk_data_t;
    
    walk_data_t data = { type, callback, user_data };
    
    ttl_ast_visitor_t *visitor = ttl_visitor_create();
    if (!visitor) return;
    
    visitor->user_data = &data;
    visitor->visit_enter = walk_nodes_visit_callback;
    
    ttl_ast_accept(root, visitor);
    ttl_visitor_destroy(visitor);
}

// Find first node matching predicate
typedef struct {
    ttl_node_predicate_t predicate;
    void *user_data;
    ttl_ast_node_t *result;
} find_first_data_t;

static bool find_first_visitor(ttl_ast_visitor_t *visitor, ttl_ast_node_t *node) {
    find_first_data_t *data = visitor->user_data;
    
    if (data->predicate(node, data->user_data)) {
        data->result = node;
        visitor->control = TTL_VISITOR_STOP;
        return false;
    }
    
    return true;
}

ttl_ast_node_t* ttl_ast_find_first(ttl_ast_node_t *root,
                                  ttl_node_predicate_t predicate,
                                  void *user_data) {
    if (!root || !predicate) return NULL;
    
    find_first_data_t data = { predicate, user_data, NULL };
    
    ttl_ast_visitor_t *visitor = ttl_visitor_create();
    if (!visitor) return NULL;
    
    visitor->user_data = &data;
    visitor->visit_enter = find_first_visitor;
    
    ttl_ast_accept(root, visitor);
    ttl_visitor_destroy(visitor);
    
    return data.result;
}

// Find all nodes matching predicate
typedef struct {
    ttl_node_predicate_t predicate;
    void *user_data;
    ttl_node_list_t *list;
} find_all_data_t;

static bool find_all_visitor(ttl_ast_visitor_t *visitor, ttl_ast_node_t *node) {
    find_all_data_t *data = visitor->user_data;
    
    if (data->predicate(node, data->user_data)) {
        // Grow array if needed
        if (data->list->count >= data->list->capacity) {
            size_t new_capacity = data->list->capacity * 2;
            ttl_ast_node_t **new_nodes = realloc(data->list->nodes,
                sizeof(ttl_ast_node_t*) * new_capacity);
            if (!new_nodes) return false;
            
            data->list->nodes = new_nodes;
            data->list->capacity = new_capacity;
        }
        
        data->list->nodes[data->list->count++] = node;
    }
    
    return true;
}

ttl_node_list_t* ttl_ast_find_all(ttl_ast_node_t *root,
                                 ttl_node_predicate_t predicate,
                                 void *user_data) {
    if (!root || !predicate) return NULL;
    
    ttl_node_list_t *list = calloc(1, sizeof(ttl_node_list_t));
    if (!list) return NULL;
    
    list->capacity = 16;
    list->nodes = calloc(list->capacity, sizeof(ttl_ast_node_t*));
    if (!list->nodes) {
        free(list);
        return NULL;
    }
    
    find_all_data_t data = { predicate, user_data, list };
    
    ttl_ast_visitor_t *visitor = ttl_visitor_create();
    if (!visitor) {
        ttl_node_list_free(list);
        return NULL;
    }
    
    visitor->user_data = &data;
    visitor->visit_enter = find_all_visitor;
    
    ttl_ast_accept(root, visitor);
    ttl_visitor_destroy(visitor);
    
    return list;
}

void ttl_node_list_free(ttl_node_list_t *list) {
    if (list) {
        free(list->nodes);
        free(list);
    }
}

// Common predicates
bool ttl_predicate_is_literal(ttl_ast_node_t *node, void *user_data) {
    (void)user_data;
    return node->type == TTL_AST_STRING_LITERAL ||
           node->type == TTL_AST_NUMERIC_LITERAL ||
           node->type == TTL_AST_BOOLEAN_LITERAL ||
           node->type == TTL_AST_TYPED_LITERAL ||
           node->type == TTL_AST_LANG_LITERAL;
}

bool ttl_predicate_is_resource(ttl_ast_node_t *node, void *user_data) {
    (void)user_data;
    return node->type == TTL_AST_IRI ||
           node->type == TTL_AST_PREFIXED_NAME;
}

bool ttl_predicate_is_blank_node(ttl_ast_node_t *node, void *user_data) {
    (void)user_data;
    return node->type == TTL_AST_BLANK_NODE ||
           node->type == TTL_AST_BLANK_NODE_LABEL;
}

bool ttl_predicate_has_language(ttl_ast_node_t *node, void *user_data) {
    (void)user_data;
    return node->type == TTL_AST_LANG_LITERAL;
}

bool ttl_predicate_has_datatype(ttl_ast_node_t *node, void *user_data) {
    (void)user_data;
    return node->type == TTL_AST_TYPED_LITERAL;
}