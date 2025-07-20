/**
 * @file ntriples.c
 * @brief N-Triples serialization implementation
 * @author CNS Seven-Tick Engine
 * @date 2024
 * 
 * N-Triples is the simplest RDF serialization format.
 * Format: <subject> <predicate> <object> .
 * Each triple on one line, IRIs in angle brackets, literals quoted.
 */

#include "serializer.h"
#include "visitor.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

/**
 * N-Triples serializer context
 */
typedef struct {
    FILE* output;
    ttl_serializer_options_t options;
    ttl_serializer_stats_t stats;
    
    // Current triple being serialized
    char* current_subject;
    char* current_predicate;
    
    // Error state
    bool has_error;
    char error_message[256];
    
    // Timing
    clock_t start_time;
} ntriples_context_t;

/**
 * Escape string for N-Triples format
 * N-Triples uses simple escaping: \n \r \t \" \\
 */
static char* escape_ntriples_string(const char* input) {
    if (!input) return NULL;
    
    size_t len = strlen(input);
    size_t escaped_len = len * 2 + 1; // Worst case
    char* escaped = malloc(escaped_len);
    if (!escaped) return NULL;
    
    char* dest = escaped;
    for (const char* src = input; *src; src++) {
        switch (*src) {
            case '\n': *dest++ = '\\'; *dest++ = 'n'; break;
            case '\r': *dest++ = '\\'; *dest++ = 'r'; break;
            case '\t': *dest++ = '\\'; *dest++ = 't'; break;
            case '"':  *dest++ = '\\'; *dest++ = '"'; break;
            case '\\': *dest++ = '\\'; *dest++ = '\\'; break;
            default:   *dest++ = *src; break;
        }
    }
    *dest = '\0';
    
    return escaped;
}

/**
 * Serialize IRI node to N-Triples format
 */
static bool serialize_iri(ntriples_context_t* ctx, ttl_ast_node_t* node) {
    if (node->type != TTL_AST_IRI) return false;
    
    const char* iri = node->data.iri.value;
    if (!iri) return false;
    
    // N-Triples requires absolute IRIs in angle brackets
    if (fprintf(ctx->output, "<%s>", iri) < 0) {
        ctx->has_error = true;
        snprintf(ctx->error_message, sizeof(ctx->error_message), 
                "Failed to write IRI: %s", iri);
        return false;
    }
    
    return true;
}

/**
 * Serialize prefixed name to N-Triples format
 * Note: N-Triples doesn't support prefixes, so we need to expand them
 */
static bool serialize_prefixed_name(ntriples_context_t* ctx, ttl_ast_node_t* node) {
    if (node->type != TTL_AST_PREFIXED_NAME) return false;
    
    // For 80/20 implementation, we'll use a simple fallback
    // In a full implementation, we'd maintain a prefix mapping
    const char* prefix = node->data.prefixed_name.prefix;
    const char* local = node->data.prefixed_name.local_name;
    
    if (!prefix || !local) return false;
    
    // Simple expansion for common prefixes
    const char* namespace_iri = NULL;
    if (strcmp(prefix, "rdf") == 0) {
        namespace_iri = "http://www.w3.org/1999/02/22-rdf-syntax-ns#";
    } else if (strcmp(prefix, "rdfs") == 0) {
        namespace_iri = "http://www.w3.org/2000/01/rdf-schema#";
    } else if (strcmp(prefix, "xsd") == 0) {
        namespace_iri = "http://www.w3.org/2001/XMLSchema#";
    } else {
        // Fallback: use prefix as-is (not ideal but works for 80/20)
        namespace_iri = prefix;
    }
    
    if (fprintf(ctx->output, "<%s%s>", namespace_iri, local) < 0) {
        ctx->has_error = true;
        snprintf(ctx->error_message, sizeof(ctx->error_message), 
                "Failed to write prefixed name: %s:%s", prefix, local);
        return false;
    }
    
    return true;
}

/**
 * Serialize blank node to N-Triples format
 */
static bool serialize_blank_node(ntriples_context_t* ctx, ttl_ast_node_t* node) {
    if (node->type != TTL_AST_BLANK_NODE) return false;
    
    const char* label = node->data.blank_node.label;
    if (label) {
        // Named blank node: _:label
        if (fprintf(ctx->output, "_:%s", label) < 0) {
            ctx->has_error = true;
            snprintf(ctx->error_message, sizeof(ctx->error_message), 
                    "Failed to write blank node: %s", label);
            return false;
        }
    } else {
        // Anonymous blank node: _:genid<id>
        uint32_t id = node->data.blank_node.id;
        if (fprintf(ctx->output, "_:genid%u", id) < 0) {
            ctx->has_error = true;
            snprintf(ctx->error_message, sizeof(ctx->error_message), 
                    "Failed to write anonymous blank node: %u", id);
            return false;
        }
    }
    
    return true;
}

/**
 * Serialize string literal to N-Triples format
 */
static bool serialize_string_literal(ntriples_context_t* ctx, ttl_ast_node_t* node) {
    if (node->type != TTL_AST_STRING_LITERAL) return false;
    
    const char* value = node->data.string_literal.value;
    if (!value) return false;
    
    char* escaped = escape_ntriples_string(value);
    if (!escaped) {
        ctx->has_error = true;
        strcpy(ctx->error_message, "Failed to escape string literal");
        return false;
    }
    
    // N-Triples uses double quotes for strings
    int result = fprintf(ctx->output, "\"%s\"", escaped);
    free(escaped);
    
    if (result < 0) {
        ctx->has_error = true;
        strcpy(ctx->error_message, "Failed to write string literal");
        return false;
    }
    
    return true;
}

/**
 * Serialize typed literal to N-Triples format
 */
static bool serialize_typed_literal(ntriples_context_t* ctx, ttl_ast_node_t* node) {
    if (node->type != TTL_AST_TYPED_LITERAL) return false;
    
    // Serialize the value
    ttl_ast_node_t* value = node->data.typed_literal.value;
    if (!serialize_string_literal(ctx, value)) {
        return false;
    }
    
    // Add datatype
    if (fprintf(ctx->output, "^^") < 0) {
        ctx->has_error = true;
        strcpy(ctx->error_message, "Failed to write datatype separator");
        return false;
    }
    
    // Serialize datatype IRI
    ttl_ast_node_t* datatype = node->data.typed_literal.datatype;
    if (datatype->type == TTL_AST_IRI) {
        return serialize_iri(ctx, datatype);
    } else if (datatype->type == TTL_AST_PREFIXED_NAME) {
        return serialize_prefixed_name(ctx, datatype);
    }
    
    return false;
}

/**
 * Serialize language literal to N-Triples format
 */
static bool serialize_lang_literal(ntriples_context_t* ctx, ttl_ast_node_t* node) {
    if (node->type != TTL_AST_LANG_LITERAL) return false;
    
    // Serialize the value
    ttl_ast_node_t* value = node->data.lang_literal.value;
    if (!serialize_string_literal(ctx, value)) {
        return false;
    }
    
    // Add language tag
    const char* lang_tag = node->data.lang_literal.language_tag;
    if (!lang_tag) return false;
    
    if (fprintf(ctx->output, "@%s", lang_tag) < 0) {
        ctx->has_error = true;
        snprintf(ctx->error_message, sizeof(ctx->error_message), 
                "Failed to write language tag: %s", lang_tag);
        return false;
    }
    
    return true;
}

/**
 * Serialize numeric literal to N-Triples format
 */
static bool serialize_numeric_literal(ntriples_context_t* ctx, ttl_ast_node_t* node) {
    if (node->type != TTL_AST_NUMERIC_LITERAL) return false;
    
    const char* lexical = node->data.numeric_literal.lexical_form;
    if (lexical) {
        // Use original lexical form if available
        if (fprintf(ctx->output, "\"%s\"", lexical) < 0) {
            ctx->has_error = true;
            strcpy(ctx->error_message, "Failed to write numeric literal");
            return false;
        }
    } else {
        // Generate from numeric value
        ttl_numeric_type_t num_type = node->data.numeric_literal.numeric_type;
        switch (num_type) {
            case TTL_NUMERIC_INTEGER:
                if (fprintf(ctx->output, "\"%lld\"", 
                           (long long)node->data.numeric_literal.integer_value) < 0) {
                    ctx->has_error = true;
                    strcpy(ctx->error_message, "Failed to write integer literal");
                    return false;
                }
                break;
            case TTL_NUMERIC_DECIMAL:
            case TTL_NUMERIC_DOUBLE:
                if (fprintf(ctx->output, "\"%g\"", 
                           node->data.numeric_literal.double_value) < 0) {
                    ctx->has_error = true;
                    strcpy(ctx->error_message, "Failed to write decimal literal");
                    return false;
                }
                break;
        }
    }
    
    // Add appropriate datatype
    const char* datatype_iri = NULL;
    switch (node->data.numeric_literal.numeric_type) {
        case TTL_NUMERIC_INTEGER:
            datatype_iri = "http://www.w3.org/2001/XMLSchema#integer";
            break;
        case TTL_NUMERIC_DECIMAL:
            datatype_iri = "http://www.w3.org/2001/XMLSchema#decimal";
            break;
        case TTL_NUMERIC_DOUBLE:
            datatype_iri = "http://www.w3.org/2001/XMLSchema#double";
            break;
    }
    
    if (fprintf(ctx->output, "^^<%s>", datatype_iri) < 0) {
        ctx->has_error = true;
        strcpy(ctx->error_message, "Failed to write numeric datatype");
        return false;
    }
    
    return true;
}

/**
 * Serialize boolean literal to N-Triples format
 */
static bool serialize_boolean_literal(ntriples_context_t* ctx, ttl_ast_node_t* node) {
    if (node->type != TTL_AST_BOOLEAN_LITERAL) return false;
    
    const char* value = node->data.boolean_literal.value ? "true" : "false";
    
    if (fprintf(ctx->output, "\"%s\"^^<http://www.w3.org/2001/XMLSchema#boolean>", 
                value) < 0) {
        ctx->has_error = true;
        strcpy(ctx->error_message, "Failed to write boolean literal");
        return false;
    }
    
    return true;
}

/**
 * Serialize any resource (subject/predicate/object)
 */
static bool serialize_resource(ntriples_context_t* ctx, ttl_ast_node_t* node) {
    if (!node) return false;
    
    switch (node->type) {
        case TTL_AST_IRI:
            return serialize_iri(ctx, node);
        case TTL_AST_PREFIXED_NAME:
            return serialize_prefixed_name(ctx, node);
        case TTL_AST_BLANK_NODE:
            return serialize_blank_node(ctx, node);
        case TTL_AST_STRING_LITERAL:
            return serialize_string_literal(ctx, node);
        case TTL_AST_TYPED_LITERAL:
            return serialize_typed_literal(ctx, node);
        case TTL_AST_LANG_LITERAL:
            return serialize_lang_literal(ctx, node);
        case TTL_AST_NUMERIC_LITERAL:
            return serialize_numeric_literal(ctx, node);
        case TTL_AST_BOOLEAN_LITERAL:
            return serialize_boolean_literal(ctx, node);
        case TTL_AST_RDF_TYPE:
            // 'a' shorthand expands to rdf:type
            return fprintf(ctx->output, 
                          "<http://www.w3.org/1999/02/22-rdf-syntax-ns#type>") >= 0;
        default:
            return false;
    }
}

/**
 * Triple visitor function for N-Triples serialization
 */
static bool visit_triple(ttl_ast_visitor_t* visitor, ttl_ast_node_t* node) {
    if (node->type != TTL_AST_TRIPLE) return true;
    
    ntriples_context_t* ctx = (ntriples_context_t*)visitor->user_data;
    
    ttl_ast_node_t* subject = node->data.triple.subject;
    ttl_ast_node_t* pred_obj_list = node->data.triple.predicate_object_list;
    
    if (!subject || !pred_obj_list) return true;
    
    // For N-Triples, we need to flatten predicate-object lists into individual triples
    if (pred_obj_list->type == TTL_AST_PREDICATE_OBJECT_LIST) {
        size_t count = pred_obj_list->data.predicate_object_list.item_count;
        ttl_ast_node_t** items = pred_obj_list->data.predicate_object_list.items;
        
        for (size_t i = 0; i < count; i += 2) { // predicate, object list pairs
            ttl_ast_node_t* predicate = items[i];
            ttl_ast_node_t* object_list = items[i + 1];
            
            if (object_list->type == TTL_AST_OBJECT_LIST) {
                size_t obj_count = object_list->data.object_list.object_count;
                ttl_ast_node_t** objects = object_list->data.object_list.objects;
                
                for (size_t j = 0; j < obj_count; j++) {
                    // Write one complete triple
                    if (!serialize_resource(ctx, subject)) return false;
                    if (fprintf(ctx->output, " ") < 0) return false;
                    
                    if (!serialize_resource(ctx, predicate)) return false;
                    if (fprintf(ctx->output, " ") < 0) return false;
                    
                    if (!serialize_resource(ctx, objects[j])) return false;
                    if (fprintf(ctx->output, " .\n") < 0) return false;
                    
                    ctx->stats.triples_serialized++;
                }
            }
        }
    }
    
    return true;
}

/**
 * Create N-Triples serializer
 */
ttl_serializer_t* ttl_create_ntriples_serializer(const ttl_serializer_options_t* options) {
    ntriples_context_t* ctx = calloc(1, sizeof(ntriples_context_t));
    if (!ctx) return NULL;
    
    if (options) {
        ctx->options = *options;
    } else {
        ctx->options = ttl_serializer_default_options(TTL_FORMAT_NTRIPLES);
    }
    
    ctx->output = ctx->options.output ? ctx->options.output : stdout;
    ctx->start_time = clock();
    
    return (ttl_serializer_t*)ctx;
}

/**
 * Serialize AST to N-Triples format
 */
bool ttl_serialize_ntriples_ast(ttl_serializer_t* serializer, ttl_ast_node_t* root) {
    if (!serializer || !root) return false;
    
    ntriples_context_t* ctx = (ntriples_context_t*)serializer;
    
    // Create visitor for triple extraction
    ttl_ast_visitor_t* visitor = ttl_visitor_create();
    if (!visitor) return false;
    
    visitor->user_data = ctx;
    visitor->visit_triple = visit_triple;
    visitor->order = TTL_VISITOR_PRE_ORDER;
    
    // Traverse AST and serialize triples
    bool success = ttl_ast_accept(root, visitor);
    
    if (success && !ctx->has_error) {
        ctx->stats.serialization_time_ms = 
            ((double)(clock() - ctx->start_time)) / CLOCKS_PER_SEC * 1000.0;
    }
    
    ttl_visitor_destroy(visitor);
    return success && !ctx->has_error;
}

/**
 * Quick N-Triples serialization function
 */
bool ttl_serialize_ntriples(ttl_ast_node_t* root, FILE* output) {
    ttl_serializer_options_t options = ttl_serializer_default_options(TTL_FORMAT_NTRIPLES);
    options.output = output;
    
    ttl_serializer_t* serializer = ttl_create_ntriples_serializer(&options);
    if (!serializer) return false;
    
    bool success = ttl_serialize_ntriples_ast(serializer, root);
    free(serializer);
    
    return success;
}