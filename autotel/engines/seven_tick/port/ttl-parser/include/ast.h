#ifndef TTL_AST_H
#define TTL_AST_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct ttl_ast_node ttl_ast_node_t;
typedef struct ttl_ast_visitor ttl_ast_visitor_t;

// AST Node Types
typedef enum {
    // Document and statement nodes
    TTL_AST_DOCUMENT,
    TTL_AST_DIRECTIVE,
    TTL_AST_PREFIX_DIRECTIVE,
    TTL_AST_BASE_DIRECTIVE,
    TTL_AST_TRIPLE,
    
    // Subject/predicate/object nodes
    TTL_AST_SUBJECT,
    TTL_AST_PREDICATE,
    TTL_AST_OBJECT,
    
    // IRI and identifier nodes
    TTL_AST_IRI,
    TTL_AST_PREFIXED_NAME,
    TTL_AST_BLANK_NODE,
    TTL_AST_BLANK_NODE_LABEL,
    
    // Literal nodes
    TTL_AST_STRING_LITERAL,
    TTL_AST_NUMERIC_LITERAL,
    TTL_AST_BOOLEAN_LITERAL,
    TTL_AST_TYPED_LITERAL,
    TTL_AST_LANG_LITERAL,
    
    // Collection nodes
    TTL_AST_COLLECTION,
    TTL_AST_BLANK_NODE_PROPERTY_LIST,
    TTL_AST_PREDICATE_OBJECT_LIST,
    TTL_AST_OBJECT_LIST,
    
    // Special nodes
    TTL_AST_RDF_TYPE,  // 'a' shorthand
    TTL_AST_COMMENT,
    
    TTL_AST_NODE_TYPE_COUNT
} ttl_ast_node_type_t;

// Numeric literal types
typedef enum {
    TTL_NUMERIC_INTEGER,
    TTL_NUMERIC_DECIMAL,
    TTL_NUMERIC_DOUBLE
} ttl_numeric_type_t;

// String literal quote types
typedef enum {
    TTL_STRING_SINGLE_QUOTE,      // 'string'
    TTL_STRING_DOUBLE_QUOTE,      // "string"
    TTL_STRING_TRIPLE_SINGLE,     // '''string'''
    TTL_STRING_TRIPLE_DOUBLE      // """string"""
} ttl_string_quote_type_t;

// Source location information
typedef struct {
    uint32_t line;
    uint32_t column;
    uint32_t offset;
    uint32_t length;
} ttl_source_location_t;

// Base AST node structure
struct ttl_ast_node {
    ttl_ast_node_type_t type;
    ttl_source_location_t location;
    uint32_t ref_count;  // Reference counting for memory management
    
    // Parent node for tree traversal
    ttl_ast_node_t *parent;
    
    // Node-specific data (union for efficiency)
    union {
        // Document node
        struct {
            ttl_ast_node_t **statements;
            size_t statement_count;
            size_t statement_capacity;
        } document;
        
        // Directive nodes
        struct {
            ttl_ast_node_t *prefix;  // Can be NULL for base directive
            ttl_ast_node_t *iri;
        } directive;
        
        // Triple node
        struct {
            ttl_ast_node_t *subject;
            ttl_ast_node_t *predicate_object_list;
        } triple;
        
        // Predicate-object list
        struct {
            ttl_ast_node_t **items;
            size_t item_count;
            size_t item_capacity;
        } predicate_object_list;
        
        // Object list
        struct {
            ttl_ast_node_t **objects;
            size_t object_count;
            size_t object_capacity;
        } object_list;
        
        // IRI node
        struct {
            char *value;
            bool is_absolute;
        } iri;
        
        // Prefixed name node
        struct {
            char *prefix;
            char *local_name;
        } prefixed_name;
        
        // Blank node
        struct {
            char *label;  // NULL for anonymous blank nodes
            uint32_t id;  // Internal ID for anonymous blank nodes
        } blank_node;
        
        // String literal
        struct {
            char *value;
            ttl_string_quote_type_t quote_type;
            bool has_escapes;
        } string_literal;
        
        // Numeric literal
        struct {
            ttl_numeric_type_t numeric_type;
            union {
                int64_t integer_value;
                double double_value;
            };
            char *lexical_form;  // Original string representation
        } numeric_literal;
        
        // Boolean literal
        struct {
            bool value;
        } boolean_literal;
        
        // Typed literal
        struct {
            ttl_ast_node_t *value;
            ttl_ast_node_t *datatype;
        } typed_literal;
        
        // Language-tagged literal
        struct {
            ttl_ast_node_t *value;
            char *language_tag;
        } lang_literal;
        
        // Collection
        struct {
            ttl_ast_node_t **items;
            size_t item_count;
            size_t item_capacity;
        } collection;
        
        // Blank node property list
        struct {
            ttl_ast_node_t *predicate_object_list;
        } blank_node_property_list;
        
        // Comment
        struct {
            char *text;
            bool is_multiline;
        } comment;
    } data;
};

// Memory arena for efficient allocation
typedef struct ttl_ast_arena {
    struct ttl_ast_arena_block *blocks;
    struct ttl_ast_arena_block *current_block;
    size_t block_size;
    size_t total_allocated;
    size_t total_used;
} ttl_ast_arena_t;

// AST context for node creation
typedef struct {
    ttl_ast_arena_t *arena;
    bool use_arena;  // If false, use malloc/free
    uint32_t next_blank_node_id;
    
    // Statistics
    struct {
        uint32_t nodes_created;
        uint32_t nodes_destroyed;
        uint32_t max_depth;
    } stats;
} ttl_ast_context_t;

// Node creation functions
ttl_ast_context_t* ttl_ast_context_create(bool use_arena);
void ttl_ast_context_destroy(ttl_ast_context_t *ctx);

// Document and statement nodes
ttl_ast_node_t* ttl_ast_create_document(ttl_ast_context_t *ctx);
ttl_ast_node_t* ttl_ast_create_prefix_directive(ttl_ast_context_t *ctx, ttl_ast_node_t *prefix, ttl_ast_node_t *iri);
ttl_ast_node_t* ttl_ast_create_base_directive(ttl_ast_context_t *ctx, ttl_ast_node_t *iri);
ttl_ast_node_t* ttl_ast_create_triple(ttl_ast_context_t *ctx, ttl_ast_node_t *subject, ttl_ast_node_t *predicate_object_list);

// IRI and identifier nodes
ttl_ast_node_t* ttl_ast_create_iri(ttl_ast_context_t *ctx, const char *value);
ttl_ast_node_t* ttl_ast_create_prefixed_name(ttl_ast_context_t *ctx, const char *prefix, const char *local_name);
ttl_ast_node_t* ttl_ast_create_blank_node(ttl_ast_context_t *ctx, const char *label);
ttl_ast_node_t* ttl_ast_create_anonymous_blank_node(ttl_ast_context_t *ctx);

// Literal nodes
ttl_ast_node_t* ttl_ast_create_string_literal(ttl_ast_context_t *ctx, const char *value, ttl_string_quote_type_t quote_type);
ttl_ast_node_t* ttl_ast_create_integer_literal(ttl_ast_context_t *ctx, int64_t value, const char *lexical_form);
ttl_ast_node_t* ttl_ast_create_decimal_literal(ttl_ast_context_t *ctx, double value, const char *lexical_form);
ttl_ast_node_t* ttl_ast_create_double_literal(ttl_ast_context_t *ctx, double value, const char *lexical_form);
ttl_ast_node_t* ttl_ast_create_boolean_literal(ttl_ast_context_t *ctx, bool value);
ttl_ast_node_t* ttl_ast_create_typed_literal(ttl_ast_context_t *ctx, ttl_ast_node_t *value, ttl_ast_node_t *datatype);
ttl_ast_node_t* ttl_ast_create_lang_literal(ttl_ast_context_t *ctx, ttl_ast_node_t *value, const char *language_tag);

// Collection nodes
ttl_ast_node_t* ttl_ast_create_collection(ttl_ast_context_t *ctx);
ttl_ast_node_t* ttl_ast_create_blank_node_property_list(ttl_ast_context_t *ctx, ttl_ast_node_t *predicate_object_list);
ttl_ast_node_t* ttl_ast_create_predicate_object_list(ttl_ast_context_t *ctx);
ttl_ast_node_t* ttl_ast_create_object_list(ttl_ast_context_t *ctx);

// Special nodes
ttl_ast_node_t* ttl_ast_create_rdf_type(ttl_ast_context_t *ctx);

// Node manipulation
void ttl_ast_add_statement(ttl_ast_node_t *document, ttl_ast_node_t *statement);
void ttl_ast_add_predicate_object(ttl_ast_node_t *list, ttl_ast_node_t *predicate, ttl_ast_node_t *object_list);
void ttl_ast_add_object(ttl_ast_node_t *list, ttl_ast_node_t *object);
void ttl_ast_add_collection_item(ttl_ast_node_t *collection, ttl_ast_node_t *item);

// Reference counting
void ttl_ast_node_ref(ttl_ast_node_t *node);
void ttl_ast_node_unref(ttl_ast_node_t *node);

// Tree traversal helpers
ttl_ast_node_t* ttl_ast_get_parent(ttl_ast_node_t *node);
size_t ttl_ast_get_child_count(ttl_ast_node_t *node);
ttl_ast_node_t* ttl_ast_get_child(ttl_ast_node_t *node, size_t index);
int ttl_ast_get_depth(ttl_ast_node_t *node);

// Pretty printing
void ttl_ast_print(ttl_ast_node_t *node, FILE *out);
void ttl_ast_print_indented(ttl_ast_node_t *node, FILE *out, int indent);
char* ttl_ast_to_string(ttl_ast_node_t *node);

// Serialization
bool ttl_ast_serialize(ttl_ast_node_t *node, FILE *out);
ttl_ast_node_t* ttl_ast_deserialize(ttl_ast_context_t *ctx, FILE *in);

// Validation
bool ttl_ast_validate(ttl_ast_node_t *node);
const char* ttl_ast_node_type_to_string(ttl_ast_node_type_t type);

// Error handling
typedef struct {
    char message[256];
    ttl_source_location_t location;
    ttl_ast_node_t *node;
} ttl_ast_error_t;

bool ttl_ast_get_last_error(ttl_ast_error_t *error);

#ifdef __cplusplus
}
#endif

#endif // TTL_AST_H