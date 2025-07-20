#include "../include/ast.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

// Arena block structure for memory allocation
typedef struct ttl_ast_arena_block {
    struct ttl_ast_arena_block *next;
    size_t size;
    size_t used;
    char data[];
} ttl_ast_arena_block_t;

// Thread-local error storage
static __thread ttl_ast_error_t last_error;

// Helper to set error
static void set_error(const char *message, ttl_ast_node_t *node) {
    strncpy(last_error.message, message, sizeof(last_error.message) - 1);
    last_error.message[sizeof(last_error.message) - 1] = '\0';
    last_error.node = node;
    if (node) {
        last_error.location = node->location;
    }
}

// Arena allocation
static void* arena_alloc(ttl_ast_arena_t *arena, size_t size) {
    // Align to 8 bytes
    size = (size + 7) & ~7;
    
    // Check if current block has enough space
    if (!arena->current_block || arena->current_block->used + size > arena->current_block->size) {
        // Allocate new block
        size_t block_size = arena->block_size;
        if (size > block_size) {
            block_size = size + sizeof(ttl_ast_arena_block_t);
        }
        
        ttl_ast_arena_block_t *new_block = malloc(block_size);
        if (!new_block) return NULL;
        
        new_block->next = arena->blocks;
        new_block->size = block_size - sizeof(ttl_ast_arena_block_t);
        new_block->used = 0;
        
        arena->blocks = new_block;
        arena->current_block = new_block;
        arena->total_allocated += block_size;
    }
    
    void *ptr = arena->current_block->data + arena->current_block->used;
    arena->current_block->used += size;
    arena->total_used += size;
    
    return ptr;
}

// Create AST context
ttl_ast_context_t* ttl_ast_context_create(bool use_arena) {
    ttl_ast_context_t *ctx = calloc(1, sizeof(ttl_ast_context_t));
    if (!ctx) return NULL;
    
    ctx->use_arena = use_arena;
    ctx->next_blank_node_id = 1;
    
    if (use_arena) {
        ctx->arena = calloc(1, sizeof(ttl_ast_arena_t));
        if (!ctx->arena) {
            free(ctx);
            return NULL;
        }
        ctx->arena->block_size = 64 * 1024; // 64KB blocks
    }
    
    return ctx;
}

// Destroy AST context
void ttl_ast_context_destroy(ttl_ast_context_t *ctx) {
    if (!ctx) return;
    
    if (ctx->arena) {
        // Free all arena blocks
        ttl_ast_arena_block_t *block = ctx->arena->blocks;
        while (block) {
            ttl_ast_arena_block_t *next = block->next;
            free(block);
            block = next;
        }
        free(ctx->arena);
    }
    
    free(ctx);
}

// Allocate node
static ttl_ast_node_t* alloc_node(ttl_ast_context_t *ctx, ttl_ast_node_type_t type) {
    ttl_ast_node_t *node;
    
    if (ctx->use_arena && ctx->arena) {
        node = arena_alloc(ctx->arena, sizeof(ttl_ast_node_t));
    } else {
        node = calloc(1, sizeof(ttl_ast_node_t));
    }
    
    if (!node) return NULL;
    
    node->type = type;
    node->ref_count = 1;
    ctx->stats.nodes_created++;
    
    return node;
}

// Free node (only when not using arena)
// Note: Currently unused but kept for future arena-based memory management
static void free_node(ttl_ast_node_t *node, ttl_ast_context_t *ctx) __attribute__((unused));
static void free_node(ttl_ast_node_t *node, ttl_ast_context_t *ctx) {
    if (!node || ctx->use_arena) return;
    
    // Free node-specific data
    switch (node->type) {
        case TTL_AST_DOCUMENT:
            free(node->data.document.statements);
            break;
            
        case TTL_AST_PREDICATE_OBJECT_LIST:
            free(node->data.predicate_object_list.items);
            break;
            
        case TTL_AST_OBJECT_LIST:
            free(node->data.object_list.objects);
            break;
            
        case TTL_AST_IRI:
            free(node->data.iri.value);
            break;
            
        case TTL_AST_PREFIXED_NAME:
            free(node->data.prefixed_name.prefix);
            free(node->data.prefixed_name.local_name);
            break;
            
        case TTL_AST_BLANK_NODE:
        case TTL_AST_BLANK_NODE_LABEL:
            free(node->data.blank_node.label);
            break;
            
        case TTL_AST_STRING_LITERAL:
            free(node->data.string_literal.value);
            break;
            
        case TTL_AST_NUMERIC_LITERAL:
            free(node->data.numeric_literal.lexical_form);
            break;
            
        case TTL_AST_LANG_LITERAL:
            free(node->data.lang_literal.language_tag);
            break;
            
        case TTL_AST_COLLECTION:
            free(node->data.collection.items);
            break;
            
        case TTL_AST_COMMENT:
            free(node->data.comment.text);
            break;
            
        default:
            break;
    }
    
    free(node);
    if (ctx) ctx->stats.nodes_destroyed++;
}

// String duplication helper
static char* duplicate_string(ttl_ast_context_t *ctx, const char *str) {
    if (!str) return NULL;
    
    size_t len = strlen(str) + 1;
    char *copy;
    
    if (ctx->use_arena && ctx->arena) {
        copy = arena_alloc(ctx->arena, len);
    } else {
        copy = malloc(len);
    }
    
    if (copy) {
        memcpy(copy, str, len);
    }
    
    return copy;
}

// Create document node
ttl_ast_node_t* ttl_ast_create_document(ttl_ast_context_t *ctx) {
    ttl_ast_node_t *node = alloc_node(ctx, TTL_AST_DOCUMENT);
    if (!node) return NULL;
    
    node->data.document.statement_capacity = 16;
    if (ctx->use_arena && ctx->arena) {
        node->data.document.statements = arena_alloc(ctx->arena, 
            sizeof(ttl_ast_node_t*) * node->data.document.statement_capacity);
    } else {
        node->data.document.statements = calloc(node->data.document.statement_capacity, 
            sizeof(ttl_ast_node_t*));
    }
    
    return node;
}

// Create prefix directive
ttl_ast_node_t* ttl_ast_create_prefix_directive(ttl_ast_context_t *ctx, 
                                               ttl_ast_node_t *prefix, 
                                               ttl_ast_node_t *iri) {
    ttl_ast_node_t *node = alloc_node(ctx, TTL_AST_PREFIX_DIRECTIVE);
    if (!node) return NULL;
    
    node->data.directive.prefix = prefix;
    node->data.directive.iri = iri;
    
    if (prefix) {
        prefix->parent = node;
        ttl_ast_node_ref(prefix);
    }
    if (iri) {
        iri->parent = node;
        ttl_ast_node_ref(iri);
    }
    
    return node;
}

// Create base directive
ttl_ast_node_t* ttl_ast_create_base_directive(ttl_ast_context_t *ctx, ttl_ast_node_t *iri) {
    ttl_ast_node_t *node = alloc_node(ctx, TTL_AST_BASE_DIRECTIVE);
    if (!node) return NULL;
    
    node->data.directive.iri = iri;
    if (iri) {
        iri->parent = node;
        ttl_ast_node_ref(iri);
    }
    
    return node;
}

// Create triple
ttl_ast_node_t* ttl_ast_create_triple(ttl_ast_context_t *ctx,
                                     ttl_ast_node_t *subject,
                                     ttl_ast_node_t *predicate_object_list) {
    ttl_ast_node_t *node = alloc_node(ctx, TTL_AST_TRIPLE);
    if (!node) return NULL;
    
    node->data.triple.subject = subject;
    node->data.triple.predicate_object_list = predicate_object_list;
    
    if (subject) {
        subject->parent = node;
        ttl_ast_node_ref(subject);
    }
    if (predicate_object_list) {
        predicate_object_list->parent = node;
        ttl_ast_node_ref(predicate_object_list);
    }
    
    return node;
}

// Create IRI
ttl_ast_node_t* ttl_ast_create_iri(ttl_ast_context_t *ctx, const char *value) {
    ttl_ast_node_t *node = alloc_node(ctx, TTL_AST_IRI);
    if (!node) return NULL;
    
    node->data.iri.value = duplicate_string(ctx, value);
    node->data.iri.is_absolute = (value && (strstr(value, "://") != NULL));
    
    return node;
}

// Create prefixed name
ttl_ast_node_t* ttl_ast_create_prefixed_name(ttl_ast_context_t *ctx,
                                            const char *prefix,
                                            const char *local_name) {
    ttl_ast_node_t *node = alloc_node(ctx, TTL_AST_PREFIXED_NAME);
    if (!node) return NULL;
    
    node->data.prefixed_name.prefix = duplicate_string(ctx, prefix);
    node->data.prefixed_name.local_name = duplicate_string(ctx, local_name);
    
    return node;
}

// Create blank node
ttl_ast_node_t* ttl_ast_create_blank_node(ttl_ast_context_t *ctx, const char *label) {
    ttl_ast_node_t *node = alloc_node(ctx, TTL_AST_BLANK_NODE_LABEL);
    if (!node) return NULL;
    
    node->data.blank_node.label = duplicate_string(ctx, label);
    
    return node;
}

// Create anonymous blank node
ttl_ast_node_t* ttl_ast_create_anonymous_blank_node(ttl_ast_context_t *ctx) {
    ttl_ast_node_t *node = alloc_node(ctx, TTL_AST_BLANK_NODE);
    if (!node) return NULL;
    
    node->data.blank_node.id = ctx->next_blank_node_id++;
    
    return node;
}

// Create string literal
ttl_ast_node_t* ttl_ast_create_string_literal(ttl_ast_context_t *ctx,
                                             const char *value,
                                             ttl_string_quote_type_t quote_type) {
    ttl_ast_node_t *node = alloc_node(ctx, TTL_AST_STRING_LITERAL);
    if (!node) return NULL;
    
    node->data.string_literal.value = duplicate_string(ctx, value);
    node->data.string_literal.quote_type = quote_type;
    node->data.string_literal.has_escapes = (value && strchr(value, '\\') != NULL);
    
    return node;
}

// Create integer literal
ttl_ast_node_t* ttl_ast_create_integer_literal(ttl_ast_context_t *ctx,
                                              int64_t value,
                                              const char *lexical_form) {
    ttl_ast_node_t *node = alloc_node(ctx, TTL_AST_NUMERIC_LITERAL);
    if (!node) return NULL;
    
    node->data.numeric_literal.numeric_type = TTL_NUMERIC_INTEGER;
    node->data.numeric_literal.integer_value = value;
    node->data.numeric_literal.lexical_form = duplicate_string(ctx, lexical_form);
    
    return node;
}

// Create decimal literal
ttl_ast_node_t* ttl_ast_create_decimal_literal(ttl_ast_context_t *ctx,
                                              double value,
                                              const char *lexical_form) {
    ttl_ast_node_t *node = alloc_node(ctx, TTL_AST_NUMERIC_LITERAL);
    if (!node) return NULL;
    
    node->data.numeric_literal.numeric_type = TTL_NUMERIC_DECIMAL;
    node->data.numeric_literal.double_value = value;
    node->data.numeric_literal.lexical_form = duplicate_string(ctx, lexical_form);
    
    return node;
}

// Create double literal
ttl_ast_node_t* ttl_ast_create_double_literal(ttl_ast_context_t *ctx,
                                             double value,
                                             const char *lexical_form) {
    ttl_ast_node_t *node = alloc_node(ctx, TTL_AST_NUMERIC_LITERAL);
    if (!node) return NULL;
    
    node->data.numeric_literal.numeric_type = TTL_NUMERIC_DOUBLE;
    node->data.numeric_literal.double_value = value;
    node->data.numeric_literal.lexical_form = duplicate_string(ctx, lexical_form);
    
    return node;
}

// Create boolean literal
ttl_ast_node_t* ttl_ast_create_boolean_literal(ttl_ast_context_t *ctx, bool value) {
    ttl_ast_node_t *node = alloc_node(ctx, TTL_AST_BOOLEAN_LITERAL);
    if (!node) return NULL;
    
    node->data.boolean_literal.value = value;
    
    return node;
}

// Create typed literal
ttl_ast_node_t* ttl_ast_create_typed_literal(ttl_ast_context_t *ctx,
                                            ttl_ast_node_t *value,
                                            ttl_ast_node_t *datatype) {
    ttl_ast_node_t *node = alloc_node(ctx, TTL_AST_TYPED_LITERAL);
    if (!node) return NULL;
    
    node->data.typed_literal.value = value;
    node->data.typed_literal.datatype = datatype;
    
    if (value) {
        value->parent = node;
        ttl_ast_node_ref(value);
    }
    if (datatype) {
        datatype->parent = node;
        ttl_ast_node_ref(datatype);
    }
    
    return node;
}

// Create language-tagged literal
ttl_ast_node_t* ttl_ast_create_lang_literal(ttl_ast_context_t *ctx,
                                           ttl_ast_node_t *value,
                                           const char *language_tag) {
    ttl_ast_node_t *node = alloc_node(ctx, TTL_AST_LANG_LITERAL);
    if (!node) return NULL;
    
    node->data.lang_literal.value = value;
    node->data.lang_literal.language_tag = duplicate_string(ctx, language_tag);
    
    if (value) {
        value->parent = node;
        ttl_ast_node_ref(value);
    }
    
    return node;
}

// Create collection
ttl_ast_node_t* ttl_ast_create_collection(ttl_ast_context_t *ctx) {
    ttl_ast_node_t *node = alloc_node(ctx, TTL_AST_COLLECTION);
    if (!node) return NULL;
    
    node->data.collection.item_capacity = 8;
    if (ctx->use_arena && ctx->arena) {
        node->data.collection.items = arena_alloc(ctx->arena,
            sizeof(ttl_ast_node_t*) * node->data.collection.item_capacity);
    } else {
        node->data.collection.items = calloc(node->data.collection.item_capacity,
            sizeof(ttl_ast_node_t*));
    }
    
    return node;
}

// Create blank node property list
ttl_ast_node_t* ttl_ast_create_blank_node_property_list(ttl_ast_context_t *ctx,
                                                       ttl_ast_node_t *predicate_object_list) {
    ttl_ast_node_t *node = alloc_node(ctx, TTL_AST_BLANK_NODE_PROPERTY_LIST);
    if (!node) return NULL;
    
    node->data.blank_node_property_list.predicate_object_list = predicate_object_list;
    
    if (predicate_object_list) {
        predicate_object_list->parent = node;
        ttl_ast_node_ref(predicate_object_list);
    }
    
    return node;
}

// Create predicate-object list
ttl_ast_node_t* ttl_ast_create_predicate_object_list(ttl_ast_context_t *ctx) {
    ttl_ast_node_t *node = alloc_node(ctx, TTL_AST_PREDICATE_OBJECT_LIST);
    if (!node) return NULL;
    
    node->data.predicate_object_list.item_capacity = 8;
    if (ctx->use_arena && ctx->arena) {
        node->data.predicate_object_list.items = arena_alloc(ctx->arena,
            sizeof(ttl_ast_node_t*) * node->data.predicate_object_list.item_capacity);
    } else {
        node->data.predicate_object_list.items = calloc(
            node->data.predicate_object_list.item_capacity,
            sizeof(ttl_ast_node_t*));
    }
    
    return node;
}

// Create object list
ttl_ast_node_t* ttl_ast_create_object_list(ttl_ast_context_t *ctx) {
    ttl_ast_node_t *node = alloc_node(ctx, TTL_AST_OBJECT_LIST);
    if (!node) return NULL;
    
    node->data.object_list.object_capacity = 4;
    if (ctx->use_arena && ctx->arena) {
        node->data.object_list.objects = arena_alloc(ctx->arena,
            sizeof(ttl_ast_node_t*) * node->data.object_list.object_capacity);
    } else {
        node->data.object_list.objects = calloc(node->data.object_list.object_capacity,
            sizeof(ttl_ast_node_t*));
    }
    
    return node;
}

// Create RDF type shorthand
ttl_ast_node_t* ttl_ast_create_rdf_type(ttl_ast_context_t *ctx) {
    ttl_ast_node_t *node = alloc_node(ctx, TTL_AST_RDF_TYPE);
    return node;
}

// Add statement to document
void ttl_ast_add_statement(ttl_ast_node_t *document, ttl_ast_node_t *statement) {
    assert(document && document->type == TTL_AST_DOCUMENT);
    assert(statement);
    
    // Grow array if needed
    if (document->data.document.statement_count >= document->data.document.statement_capacity) {
        size_t new_capacity = document->data.document.statement_capacity * 2;
        ttl_ast_node_t **new_statements = realloc(document->data.document.statements,
            sizeof(ttl_ast_node_t*) * new_capacity);
        if (!new_statements) return;
        
        document->data.document.statements = new_statements;
        document->data.document.statement_capacity = new_capacity;
    }
    
    document->data.document.statements[document->data.document.statement_count++] = statement;
    statement->parent = document;
    ttl_ast_node_ref(statement);
}

// Add predicate-object pair to list
void ttl_ast_add_predicate_object(ttl_ast_node_t *list,
                                 ttl_ast_node_t *predicate,
                                 ttl_ast_node_t *object_list) {
    assert(list && list->type == TTL_AST_PREDICATE_OBJECT_LIST);
    assert(predicate && object_list);
    
    // Grow array if needed (stores pairs, so need 2 slots)
    if (list->data.predicate_object_list.item_count + 2 > 
        list->data.predicate_object_list.item_capacity) {
        size_t new_capacity = list->data.predicate_object_list.item_capacity * 2;
        ttl_ast_node_t **new_items = realloc(list->data.predicate_object_list.items,
            sizeof(ttl_ast_node_t*) * new_capacity);
        if (!new_items) return;
        
        list->data.predicate_object_list.items = new_items;
        list->data.predicate_object_list.item_capacity = new_capacity;
    }
    
    list->data.predicate_object_list.items[list->data.predicate_object_list.item_count++] = predicate;
    list->data.predicate_object_list.items[list->data.predicate_object_list.item_count++] = object_list;
    
    predicate->parent = list;
    object_list->parent = list;
    ttl_ast_node_ref(predicate);
    ttl_ast_node_ref(object_list);
}

// Add object to object list
void ttl_ast_add_object(ttl_ast_node_t *list, ttl_ast_node_t *object) {
    assert(list && list->type == TTL_AST_OBJECT_LIST);
    assert(object);
    
    // Grow array if needed
    if (list->data.object_list.object_count >= list->data.object_list.object_capacity) {
        size_t new_capacity = list->data.object_list.object_capacity * 2;
        ttl_ast_node_t **new_objects = realloc(list->data.object_list.objects,
            sizeof(ttl_ast_node_t*) * new_capacity);
        if (!new_objects) return;
        
        list->data.object_list.objects = new_objects;
        list->data.object_list.object_capacity = new_capacity;
    }
    
    list->data.object_list.objects[list->data.object_list.object_count++] = object;
    object->parent = list;
    ttl_ast_node_ref(object);
}

// Add item to collection
void ttl_ast_add_collection_item(ttl_ast_node_t *collection, ttl_ast_node_t *item) {
    assert(collection && collection->type == TTL_AST_COLLECTION);
    assert(item);
    
    // Grow array if needed
    if (collection->data.collection.item_count >= collection->data.collection.item_capacity) {
        size_t new_capacity = collection->data.collection.item_capacity * 2;
        ttl_ast_node_t **new_items = realloc(collection->data.collection.items,
            sizeof(ttl_ast_node_t*) * new_capacity);
        if (!new_items) return;
        
        collection->data.collection.items = new_items;
        collection->data.collection.item_capacity = new_capacity;
    }
    
    collection->data.collection.items[collection->data.collection.item_count++] = item;
    item->parent = collection;
    ttl_ast_node_ref(item);
}

// Reference counting
void ttl_ast_node_ref(ttl_ast_node_t *node) {
    if (node) {
        node->ref_count++;
    }
}

void ttl_ast_node_unref(ttl_ast_node_t *node) {
    if (!node || --node->ref_count > 0) return;
    
    // Unref children based on node type
    switch (node->type) {
        case TTL_AST_DOCUMENT:
            for (size_t i = 0; i < node->data.document.statement_count; i++) {
                ttl_ast_node_unref(node->data.document.statements[i]);
            }
            break;
            
        case TTL_AST_PREFIX_DIRECTIVE:
        case TTL_AST_BASE_DIRECTIVE:
            ttl_ast_node_unref(node->data.directive.prefix);
            ttl_ast_node_unref(node->data.directive.iri);
            break;
            
        case TTL_AST_TRIPLE:
            ttl_ast_node_unref(node->data.triple.subject);
            ttl_ast_node_unref(node->data.triple.predicate_object_list);
            break;
            
        case TTL_AST_PREDICATE_OBJECT_LIST:
            for (size_t i = 0; i < node->data.predicate_object_list.item_count; i++) {
                ttl_ast_node_unref(node->data.predicate_object_list.items[i]);
            }
            break;
            
        case TTL_AST_OBJECT_LIST:
            for (size_t i = 0; i < node->data.object_list.object_count; i++) {
                ttl_ast_node_unref(node->data.object_list.objects[i]);
            }
            break;
            
        case TTL_AST_TYPED_LITERAL:
            ttl_ast_node_unref(node->data.typed_literal.value);
            ttl_ast_node_unref(node->data.typed_literal.datatype);
            break;
            
        case TTL_AST_LANG_LITERAL:
            ttl_ast_node_unref(node->data.lang_literal.value);
            break;
            
        case TTL_AST_COLLECTION:
            for (size_t i = 0; i < node->data.collection.item_count; i++) {
                ttl_ast_node_unref(node->data.collection.items[i]);
            }
            break;
            
        case TTL_AST_BLANK_NODE_PROPERTY_LIST:
            ttl_ast_node_unref(node->data.blank_node_property_list.predicate_object_list);
            break;
            
        default:
            break;
    }
    
    // Free the node (context is not available here, so we can't use free_node)
    // This is a limitation of the current design - we'd need to store context in node
    // For now, if using arena, nodes won't be freed individually
}

// Get parent node
ttl_ast_node_t* ttl_ast_get_parent(ttl_ast_node_t *node) {
    return node ? node->parent : NULL;
}

// Get child count
size_t ttl_ast_get_child_count(ttl_ast_node_t *node) {
    if (!node) return 0;
    
    switch (node->type) {
        case TTL_AST_DOCUMENT:
            return node->data.document.statement_count;
            
        case TTL_AST_PREFIX_DIRECTIVE:
            return (node->data.directive.prefix ? 1 : 0) + 1; // prefix (optional) + iri
            
        case TTL_AST_BASE_DIRECTIVE:
            return 1; // iri
            
        case TTL_AST_TRIPLE:
            return 2; // subject + predicate_object_list
            
        case TTL_AST_PREDICATE_OBJECT_LIST:
            return node->data.predicate_object_list.item_count;
            
        case TTL_AST_OBJECT_LIST:
            return node->data.object_list.object_count;
            
        case TTL_AST_TYPED_LITERAL:
            return 2; // value + datatype
            
        case TTL_AST_LANG_LITERAL:
            return 1; // value
            
        case TTL_AST_COLLECTION:
            return node->data.collection.item_count;
            
        case TTL_AST_BLANK_NODE_PROPERTY_LIST:
            return 1; // predicate_object_list
            
        default:
            return 0;
    }
}

// Get child by index
ttl_ast_node_t* ttl_ast_get_child(ttl_ast_node_t *node, size_t index) {
    if (!node) return NULL;
    
    switch (node->type) {
        case TTL_AST_DOCUMENT:
            if (index < node->data.document.statement_count) {
                return node->data.document.statements[index];
            }
            break;
            
        case TTL_AST_PREFIX_DIRECTIVE:
            if (node->data.directive.prefix) {
                if (index == 0) return node->data.directive.prefix;
                if (index == 1) return node->data.directive.iri;
            } else {
                if (index == 0) return node->data.directive.iri;
            }
            break;
            
        case TTL_AST_BASE_DIRECTIVE:
            if (index == 0) return node->data.directive.iri;
            break;
            
        case TTL_AST_TRIPLE:
            if (index == 0) return node->data.triple.subject;
            if (index == 1) return node->data.triple.predicate_object_list;
            break;
            
        case TTL_AST_PREDICATE_OBJECT_LIST:
            if (index < node->data.predicate_object_list.item_count) {
                return node->data.predicate_object_list.items[index];
            }
            break;
            
        case TTL_AST_OBJECT_LIST:
            if (index < node->data.object_list.object_count) {
                return node->data.object_list.objects[index];
            }
            break;
            
        case TTL_AST_TYPED_LITERAL:
            if (index == 0) return node->data.typed_literal.value;
            if (index == 1) return node->data.typed_literal.datatype;
            break;
            
        case TTL_AST_LANG_LITERAL:
            if (index == 0) return node->data.lang_literal.value;
            break;
            
        case TTL_AST_COLLECTION:
            if (index < node->data.collection.item_count) {
                return node->data.collection.items[index];
            }
            break;
            
        case TTL_AST_BLANK_NODE_PROPERTY_LIST:
            if (index == 0) return node->data.blank_node_property_list.predicate_object_list;
            break;
            
        default:
            break;
    }
    
    return NULL;
}

// Get node depth
int ttl_ast_get_depth(ttl_ast_node_t *node) {
    int depth = 0;
    while (node && node->parent) {
        depth++;
        node = node->parent;
    }
    return depth;
}

// Node type to string
const char* ttl_ast_node_type_to_string(ttl_ast_node_type_t type) {
    static const char *type_names[] = {
        "DOCUMENT",
        "DIRECTIVE",
        "PREFIX_DIRECTIVE",
        "BASE_DIRECTIVE",
        "TRIPLE",
        "SUBJECT",
        "PREDICATE",
        "OBJECT",
        "IRI",
        "PREFIXED_NAME",
        "BLANK_NODE",
        "BLANK_NODE_LABEL",
        "STRING_LITERAL",
        "NUMERIC_LITERAL",
        "BOOLEAN_LITERAL",
        "TYPED_LITERAL",
        "LANG_LITERAL",
        "COLLECTION",
        "BLANK_NODE_PROPERTY_LIST",
        "PREDICATE_OBJECT_LIST",
        "OBJECT_LIST",
        "RDF_TYPE",
        "COMMENT"
    };
    
    if (type < TTL_AST_NODE_TYPE_COUNT) {
        return type_names[type];
    }
    return "UNKNOWN";
}

// Pretty print helpers
static void print_indent(FILE *out, int indent) {
    for (int i = 0; i < indent; i++) {
        fprintf(out, "  ");
    }
}

// Pretty print node
void ttl_ast_print_indented(ttl_ast_node_t *node, FILE *out, int indent) {
    if (!node || !out) return;
    
    print_indent(out, indent);
    fprintf(out, "%s", ttl_ast_node_type_to_string(node->type));
    
    // Print node-specific data
    switch (node->type) {
        case TTL_AST_IRI:
            fprintf(out, " <%s>", node->data.iri.value);
            break;
            
        case TTL_AST_PREFIXED_NAME:
            fprintf(out, " %s:%s", 
                node->data.prefixed_name.prefix ? node->data.prefixed_name.prefix : "",
                node->data.prefixed_name.local_name);
            break;
            
        case TTL_AST_BLANK_NODE_LABEL:
            fprintf(out, " _:%s", node->data.blank_node.label);
            break;
            
        case TTL_AST_BLANK_NODE:
            fprintf(out, " _:b%u", node->data.blank_node.id);
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
            
        case TTL_AST_LANG_LITERAL:
            fprintf(out, " @%s", node->data.lang_literal.language_tag);
            break;
            
        default:
            break;
    }
    
    fprintf(out, "\n");
    
    // Print children
    size_t child_count = ttl_ast_get_child_count(node);
    for (size_t i = 0; i < child_count; i++) {
        ttl_ast_print_indented(ttl_ast_get_child(node, i), out, indent + 1);
    }
}

// Print node (convenience wrapper)
void ttl_ast_print(ttl_ast_node_t *node, FILE *out) {
    ttl_ast_print_indented(node, out, 0);
}

// Convert to string
char* ttl_ast_to_string(ttl_ast_node_t *node) {
    // This is a simplified implementation
    // In production, you'd want to serialize to a buffer
    static char buffer[1024];
    
    switch (node->type) {
        case TTL_AST_IRI:
            snprintf(buffer, sizeof(buffer), "<%s>", node->data.iri.value);
            break;
            
        case TTL_AST_PREFIXED_NAME:
            snprintf(buffer, sizeof(buffer), "%s:%s",
                node->data.prefixed_name.prefix ? node->data.prefixed_name.prefix : "",
                node->data.prefixed_name.local_name);
            break;
            
        case TTL_AST_STRING_LITERAL:
            snprintf(buffer, sizeof(buffer), "\"%s\"", node->data.string_literal.value);
            break;
            
        default:
            snprintf(buffer, sizeof(buffer), "[%s]", ttl_ast_node_type_to_string(node->type));
            break;
    }
    
    return buffer;
}

// Validate node
bool ttl_ast_validate(ttl_ast_node_t *node) {
    if (!node) return false;
    
    // Basic validation - check required fields
    switch (node->type) {
        case TTL_AST_IRI:
            if (!node->data.iri.value) {
                set_error("IRI node missing value", node);
                return false;
            }
            break;
            
        case TTL_AST_PREFIXED_NAME:
            if (!node->data.prefixed_name.local_name) {
                set_error("Prefixed name missing local name", node);
                return false;
            }
            break;
            
        case TTL_AST_TRIPLE:
            if (!node->data.triple.subject || !node->data.triple.predicate_object_list) {
                set_error("Triple missing subject or predicate-object list", node);
                return false;
            }
            break;
            
        default:
            break;
    }
    
    // Validate children
    size_t child_count = ttl_ast_get_child_count(node);
    for (size_t i = 0; i < child_count; i++) {
        if (!ttl_ast_validate(ttl_ast_get_child(node, i))) {
            return false;
        }
    }
    
    return true;
}

// Get last error
bool ttl_ast_get_last_error(ttl_ast_error_t *error) {
    if (!error) return false;
    
    if (last_error.message[0] == '\0') {
        return false;
    }
    
    *error = last_error;
    return true;
}