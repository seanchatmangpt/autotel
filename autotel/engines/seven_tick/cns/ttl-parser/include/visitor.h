#ifndef TTL_VISITOR_H
#define TTL_VISITOR_H

#include "ast.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declaration
typedef struct ttl_ast_visitor ttl_ast_visitor_t;

// Visitor function types
typedef bool (*ttl_visit_func_t)(ttl_ast_visitor_t *visitor, ttl_ast_node_t *node);
typedef void (*ttl_visit_exit_func_t)(ttl_ast_visitor_t *visitor, ttl_ast_node_t *node);

// Visitor traversal order
typedef enum {
    TTL_VISITOR_PRE_ORDER,   // Visit node before children
    TTL_VISITOR_POST_ORDER,  // Visit node after children
    TTL_VISITOR_IN_ORDER     // Visit node between children (for binary nodes)
} ttl_visitor_order_t;

// Visitor control flags
typedef enum {
    TTL_VISITOR_CONTINUE = 0,     // Continue traversal normally
    TTL_VISITOR_SKIP_CHILDREN,    // Skip children of current node
    TTL_VISITOR_STOP             // Stop traversal entirely
} ttl_visitor_control_t;

// Visitor interface
struct ttl_ast_visitor {
    // User data
    void *user_data;
    
    // Traversal order
    ttl_visitor_order_t order;
    
    // Control flags (set by visit functions)
    ttl_visitor_control_t control;
    
    // Current traversal state
    struct {
        int depth;
        size_t nodes_visited;
        ttl_ast_node_t *current_node;
        ttl_ast_node_t *parent_node;
    } state;
    
    // Visit functions for each node type
    ttl_visit_func_t visit_document;
    ttl_visit_func_t visit_prefix_directive;
    ttl_visit_func_t visit_base_directive;
    ttl_visit_func_t visit_triple;
    ttl_visit_func_t visit_subject;
    ttl_visit_func_t visit_predicate;
    ttl_visit_func_t visit_object;
    ttl_visit_func_t visit_iri;
    ttl_visit_func_t visit_prefixed_name;
    ttl_visit_func_t visit_blank_node;
    ttl_visit_func_t visit_blank_node_label;
    ttl_visit_func_t visit_string_literal;
    ttl_visit_func_t visit_numeric_literal;
    ttl_visit_func_t visit_boolean_literal;
    ttl_visit_func_t visit_typed_literal;
    ttl_visit_func_t visit_lang_literal;
    ttl_visit_func_t visit_collection;
    ttl_visit_func_t visit_blank_node_property_list;
    ttl_visit_func_t visit_predicate_object_list;
    ttl_visit_func_t visit_object_list;
    ttl_visit_func_t visit_rdf_type;
    ttl_visit_func_t visit_comment;
    
    // Exit functions (called after visiting children)
    ttl_visit_exit_func_t exit_document;
    ttl_visit_exit_func_t exit_prefix_directive;
    ttl_visit_exit_func_t exit_base_directive;
    ttl_visit_exit_func_t exit_triple;
    ttl_visit_exit_func_t exit_collection;
    ttl_visit_exit_func_t exit_blank_node_property_list;
    ttl_visit_exit_func_t exit_predicate_object_list;
    ttl_visit_exit_func_t exit_object_list;
    
    // Generic visit functions (called for all nodes)
    ttl_visit_func_t visit_enter;  // Called before node-specific function
    ttl_visit_exit_func_t visit_exit;   // Called after node-specific function
};

// Visitor creation and management
ttl_ast_visitor_t* ttl_visitor_create(void);
void ttl_visitor_destroy(ttl_ast_visitor_t *visitor);
void ttl_visitor_reset(ttl_ast_visitor_t *visitor);

// Set visitor functions
void ttl_visitor_set_user_data(ttl_ast_visitor_t *visitor, void *user_data);
void ttl_visitor_set_order(ttl_ast_visitor_t *visitor, ttl_visitor_order_t order);

// Main traversal function
bool ttl_ast_accept(ttl_ast_node_t *node, ttl_ast_visitor_t *visitor);

// Helper visitors for common tasks

// Triple extraction visitor
typedef struct {
    void (*on_triple)(void *user_data, 
                     ttl_ast_node_t *subject,
                     ttl_ast_node_t *predicate, 
                     ttl_ast_node_t *object);
    void *user_data;
} ttl_triple_visitor_t;

ttl_ast_visitor_t* ttl_create_triple_visitor(ttl_triple_visitor_t *config);

// Namespace collection visitor
typedef struct {
    void (*on_prefix)(void *user_data,
                     const char *prefix,
                     const char *namespace_iri);
    void (*on_base)(void *user_data,
                   const char *base_iri);
    void *user_data;
} ttl_namespace_visitor_t;

ttl_ast_visitor_t* ttl_create_namespace_visitor(ttl_namespace_visitor_t *config);

// Statistics visitor
typedef struct {
    size_t total_nodes;
    size_t nodes_by_type[TTL_AST_NODE_TYPE_COUNT];
    size_t max_depth;
    size_t triple_count;
    size_t literal_count;
    size_t iri_count;
    size_t blank_node_count;
    size_t collection_count;
} ttl_ast_stats_t;

ttl_ast_visitor_t* ttl_create_stats_visitor(ttl_ast_stats_t *stats);

// Validation visitor
typedef struct {
    bool (*on_error)(void *user_data,
                    ttl_ast_node_t *node,
                    const char *error_message);
    void *user_data;
    bool strict_mode;  // If true, check for additional constraints
} ttl_validation_visitor_t;

ttl_ast_visitor_t* ttl_create_validation_visitor(ttl_validation_visitor_t *config);

// Transformation visitor (for AST rewriting)
typedef struct {
    ttl_ast_node_t* (*transform_node)(void *user_data,
                                    ttl_ast_node_t *node,
                                    ttl_ast_context_t *ctx);
    void *user_data;
    ttl_ast_context_t *context;  // Context for creating new nodes
} ttl_transform_visitor_t;

ttl_ast_visitor_t* ttl_create_transform_visitor(ttl_transform_visitor_t *config);

// Pretty printer visitor
typedef struct {
    FILE *output;
    int indent_size;
    bool use_color;
    bool show_location;
    bool compact_mode;
} ttl_printer_visitor_t;

ttl_ast_visitor_t* ttl_create_printer_visitor(ttl_printer_visitor_t *config);

// JSON serialization visitor
typedef struct {
    FILE *output;
    bool pretty_print;
    int indent_level;
    bool include_location;
} ttl_json_visitor_t;

ttl_ast_visitor_t* ttl_create_json_visitor(ttl_json_visitor_t *config);

// Path-based node finder
typedef struct {
    const char *path;  // XPath-like syntax: "/document/triple[0]/subject"
    ttl_ast_node_t *result;
} ttl_path_finder_t;

ttl_ast_visitor_t* ttl_create_path_finder(ttl_path_finder_t *config);

// Utility functions for visitor patterns

// Walk all nodes of specific type
typedef void (*ttl_node_callback_t)(ttl_ast_node_t *node, void *user_data);

void ttl_ast_walk_nodes_of_type(ttl_ast_node_t *root, 
                               ttl_ast_node_type_t type,
                               ttl_node_callback_t callback,
                               void *user_data);

// Find first node matching predicate
typedef bool (*ttl_node_predicate_t)(ttl_ast_node_t *node, void *user_data);

ttl_ast_node_t* ttl_ast_find_first(ttl_ast_node_t *root,
                                  ttl_node_predicate_t predicate,
                                  void *user_data);

// Find all nodes matching predicate
typedef struct {
    ttl_ast_node_t **nodes;
    size_t count;
    size_t capacity;
} ttl_node_list_t;

ttl_node_list_t* ttl_ast_find_all(ttl_ast_node_t *root,
                                 ttl_node_predicate_t predicate,
                                 void *user_data);

void ttl_node_list_free(ttl_node_list_t *list);

// Common predicates
bool ttl_predicate_is_literal(ttl_ast_node_t *node, void *user_data);
bool ttl_predicate_is_resource(ttl_ast_node_t *node, void *user_data);
bool ttl_predicate_is_blank_node(ttl_ast_node_t *node, void *user_data);
bool ttl_predicate_has_language(ttl_ast_node_t *node, void *user_data);
bool ttl_predicate_has_datatype(ttl_ast_node_t *node, void *user_data);

#ifdef __cplusplus
}
#endif

#endif // TTL_VISITOR_H