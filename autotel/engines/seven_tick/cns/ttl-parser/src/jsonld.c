/**
 * @file jsonld.c
 * @brief JSON-LD serialization implementation
 * @author CNS Seven-Tick Engine
 * @date 2024
 * 
 * JSON-LD is a web-friendly RDF serialization format.
 * Basic implementation without complex framing or contexts.
 */

#include "serializer.h"
#include "visitor.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

/**
 * JSON-LD serializer context
 */
typedef struct {
    FILE* output;
    ttl_serializer_options_t options;
    ttl_serializer_stats_t stats;
    
    // JSON output state
    int indent_level;
    bool first_item;
    bool has_error;
    char error_message[256];
    
    // Simple context for common prefixes
    bool wrote_context;
    
    // Timing
    clock_t start_time;
} jsonld_context_t;

/**
 * Escape string for JSON format
 */
static char* escape_json_string(const char* input) {
    if (!input) return NULL;
    
    size_t len = strlen(input);
    size_t escaped_len = len * 6 + 1; // Worst case for unicode escapes
    char* escaped = malloc(escaped_len);
    if (!escaped) return NULL;
    
    char* dest = escaped;
    for (const char* src = input; *src; src++) {
        switch (*src) {
            case '"':  *dest++ = '\\'; *dest++ = '"'; break;
            case '\\': *dest++ = '\\'; *dest++ = '\\'; break;
            case '\b': *dest++ = '\\'; *dest++ = 'b'; break;
            case '\f': *dest++ = '\\'; *dest++ = 'f'; break;
            case '\n': *dest++ = '\\'; *dest++ = 'n'; break;
            case '\r': *dest++ = '\\'; *dest++ = 'r'; break;
            case '\t': *dest++ = '\\'; *dest++ = 't'; break;
            default:
                if ((unsigned char)*src < 0x20) {
                    sprintf(dest, "\\u%04x", (unsigned char)*src);
                    dest += 6;
                } else {
                    *dest++ = *src;
                }
                break;
        }
    }
    *dest = '\0';
    
    return escaped;
}

/**
 * Write indentation
 */
static bool write_indent(jsonld_context_t* ctx) {
    if (!ctx->options.pretty_print) return true;
    
    for (int i = 0; i < ctx->indent_level; i++) {
        if (fprintf(ctx->output, "  ") < 0) {
            ctx->has_error = true;
            strcpy(ctx->error_message, "Failed to write indentation");
            return false;
        }
    }
    return true;
}

/**
 * Write newline (if pretty printing)
 */
static bool write_newline(jsonld_context_t* ctx) {
    if (!ctx->options.pretty_print) return true;
    
    if (fprintf(ctx->output, "\n") < 0) {
        ctx->has_error = true;
        strcpy(ctx->error_message, "Failed to write newline");
        return false;
    }
    return true;
}

/**
 * Write JSON-LD context
 */
static bool write_context(jsonld_context_t* ctx) {
    if (ctx->wrote_context) return true;
    
    if (!write_indent(ctx)) return false;
    if (fprintf(ctx->output, "\"@context\": {") < 0) return false;
    if (!write_newline(ctx)) return false;
    
    ctx->indent_level++;
    
    // Write common prefixes
    if (!write_indent(ctx)) return false;
    if (fprintf(ctx->output, "\"rdf\": \"http://www.w3.org/1999/02/22-rdf-syntax-ns#\",") < 0) return false;
    if (!write_newline(ctx)) return false;
    
    if (!write_indent(ctx)) return false;
    if (fprintf(ctx->output, "\"rdfs\": \"http://www.w3.org/2000/01/rdf-schema#\",") < 0) return false;
    if (!write_newline(ctx)) return false;
    
    if (!write_indent(ctx)) return false;
    if (fprintf(ctx->output, "\"xsd\": \"http://www.w3.org/2001/XMLSchema#\"") < 0) return false;
    if (!write_newline(ctx)) return false;
    
    ctx->indent_level--;
    
    if (!write_indent(ctx)) return false;
    if (fprintf(ctx->output, "},") < 0) return false;
    if (!write_newline(ctx)) return false;
    
    ctx->wrote_context = true;
    return true;
}

/**
 * Convert node to JSON-LD value representation
 */
static bool node_to_jsonld_value(jsonld_context_t* ctx, ttl_ast_node_t* node) {
    if (!node) return false;
    
    switch (node->type) {
        case TTL_AST_IRI: {
            const char* iri = node->data.iri.value;
            char* escaped = escape_json_string(iri);
            if (!escaped) return false;
            
            int result = fprintf(ctx->output, "{\"@id\": \"%s\"}", escaped);
            free(escaped);
            return result >= 0;
        }
        
        case TTL_AST_PREFIXED_NAME: {
            const char* prefix = node->data.prefixed_name.prefix;
            const char* local = node->data.prefixed_name.local_name;
            
            // Use compact form if it's a known prefix
            if (strcmp(prefix, "rdf") == 0 || strcmp(prefix, "rdfs") == 0 || 
                strcmp(prefix, "xsd") == 0) {
                return fprintf(ctx->output, "{\"@id\": \"%s:%s\"}", prefix, local) >= 0;
            } else {
                // Expand unknown prefixes
                return fprintf(ctx->output, "{\"@id\": \"%s%s\"}", prefix, local) >= 0;
            }
        }
        
        case TTL_AST_BLANK_NODE: {
            const char* label = node->data.blank_node.label;
            if (label) {
                return fprintf(ctx->output, "{\"@id\": \"_:%s\"}", label) >= 0;
            } else {
                return fprintf(ctx->output, "{\"@id\": \"_:genid%u\"}", 
                              node->data.blank_node.id) >= 0;
            }
        }
        
        case TTL_AST_STRING_LITERAL: {
            const char* value = node->data.string_literal.value;
            char* escaped = escape_json_string(value);
            if (!escaped) return false;
            
            int result = fprintf(ctx->output, "\"%s\"", escaped);
            free(escaped);
            return result >= 0;
        }
        
        case TTL_AST_TYPED_LITERAL: {
            ttl_ast_node_t* value = node->data.typed_literal.value;
            ttl_ast_node_t* datatype = node->data.typed_literal.datatype;
            
            if (fprintf(ctx->output, "{\"@value\": ") < 0) return false;
            if (!node_to_jsonld_value(ctx, value)) return false;
            if (fprintf(ctx->output, ", \"@type\": ") < 0) return false;
            
            // Serialize datatype
            if (datatype->type == TTL_AST_IRI) {
                char* escaped = escape_json_string(datatype->data.iri.value);
                if (!escaped) return false;
                int result = fprintf(ctx->output, "\"%s\"}", escaped);
                free(escaped);
                return result >= 0;
            } else if (datatype->type == TTL_AST_PREFIXED_NAME) {
                const char* prefix = datatype->data.prefixed_name.prefix;
                const char* local = datatype->data.prefixed_name.local_name;
                return fprintf(ctx->output, "\"%s:%s\"}", prefix, local) >= 0;
            }
            return false;
        }
        
        case TTL_AST_LANG_LITERAL: {
            ttl_ast_node_t* value = node->data.lang_literal.value;
            const char* lang = node->data.lang_literal.language_tag;
            
            if (fprintf(ctx->output, "{\"@value\": ") < 0) return false;
            if (!node_to_jsonld_value(ctx, value)) return false;
            if (fprintf(ctx->output, ", \"@language\": \"%s\"}", lang) < 0) return false;
            
            return true;
        }
        
        case TTL_AST_NUMERIC_LITERAL: {
            const char* lexical = node->data.numeric_literal.lexical_form;
            const char* datatype_iri;
            
            switch (node->data.numeric_literal.numeric_type) {
                case TTL_NUMERIC_INTEGER:
                    datatype_iri = "xsd:integer";
                    break;
                case TTL_NUMERIC_DECIMAL:
                    datatype_iri = "xsd:decimal";
                    break;
                case TTL_NUMERIC_DOUBLE:
                    datatype_iri = "xsd:double";
                    break;
                default:
                    return false;
            }
            
            if (lexical) {
                return fprintf(ctx->output, "{\"@value\": \"%s\", \"@type\": \"%s\"}", 
                              lexical, datatype_iri) >= 0;
            } else {
                // Generate value
                if (node->data.numeric_literal.numeric_type == TTL_NUMERIC_INTEGER) {
                    return fprintf(ctx->output, "{\"@value\": \"%lld\", \"@type\": \"%s\"}", 
                                  (long long)node->data.numeric_literal.integer_value, 
                                  datatype_iri) >= 0;
                } else {
                    return fprintf(ctx->output, "{\"@value\": \"%g\", \"@type\": \"%s\"}", 
                                  node->data.numeric_literal.double_value, 
                                  datatype_iri) >= 0;
                }
            }
        }
        
        case TTL_AST_BOOLEAN_LITERAL: {
            const char* value = node->data.boolean_literal.value ? "true" : "false";
            return fprintf(ctx->output, "{\"@value\": \"%s\", \"@type\": \"xsd:boolean\"}", 
                          value) >= 0;
        }
        
        case TTL_AST_RDF_TYPE:
            return fprintf(ctx->output, "{\"@id\": \"rdf:type\"}") >= 0;
        
        default:
            return false;
    }
}

/**
 * Triple visitor for JSON-LD serialization
 */
static bool visit_triple(ttl_ast_visitor_t* visitor, ttl_ast_node_t* node) {
    if (node->type != TTL_AST_TRIPLE) return true;
    
    jsonld_context_t* ctx = (jsonld_context_t*)visitor->user_data;
    
    ttl_ast_node_t* subject = node->data.triple.subject;
    ttl_ast_node_t* pred_obj_list = node->data.triple.predicate_object_list;
    
    if (!subject || !pred_obj_list) return true;
    
    // For each subject, create a JSON object
    if (!ctx->first_item) {
        if (fprintf(ctx->output, ",") < 0) return false;
        if (!write_newline(ctx)) return false;
    } else {
        ctx->first_item = false;
    }
    
    if (!write_indent(ctx)) return false;
    if (fprintf(ctx->output, "{") < 0) return false;
    if (!write_newline(ctx)) return false;
    
    ctx->indent_level++;
    
    // Write subject ID
    if (!write_indent(ctx)) return false;
    if (fprintf(ctx->output, "\"@id\": ") < 0) return false;
    
    if (subject->type == TTL_AST_IRI) {
        char* escaped = escape_json_string(subject->data.iri.value);
        if (!escaped) return false;
        int result = fprintf(ctx->output, "\"%s\",", escaped);
        free(escaped);
        if (result < 0) return false;
    } else if (subject->type == TTL_AST_BLANK_NODE) {
        const char* label = subject->data.blank_node.label;
        if (label) {
            if (fprintf(ctx->output, "\"_:%s\",", label) < 0) return false;
        } else {
            if (fprintf(ctx->output, "\"_:genid%u\",", 
                       subject->data.blank_node.id) < 0) return false;
        }
    }
    
    if (!write_newline(ctx)) return false;
    
    // Process predicate-object list
    if (pred_obj_list->type == TTL_AST_PREDICATE_OBJECT_LIST) {
        size_t count = pred_obj_list->data.predicate_object_list.item_count;
        ttl_ast_node_t** items = pred_obj_list->data.predicate_object_list.items;
        
        for (size_t i = 0; i < count; i += 2) {
            ttl_ast_node_t* predicate = items[i];
            ttl_ast_node_t* object_list = items[i + 1];
            
            if (!write_indent(ctx)) return false;
            
            // Write predicate as property name
            if (predicate->type == TTL_AST_IRI) {
                char* escaped = escape_json_string(predicate->data.iri.value);
                if (!escaped) return false;
                int result = fprintf(ctx->output, "\"%s\": ", escaped);
                free(escaped);
                if (result < 0) return false;
            } else if (predicate->type == TTL_AST_PREFIXED_NAME) {
                const char* prefix = predicate->data.prefixed_name.prefix;
                const char* local = predicate->data.prefixed_name.local_name;
                if (fprintf(ctx->output, "\"%s:%s\": ", prefix, local) < 0) return false;
            } else if (predicate->type == TTL_AST_RDF_TYPE) {
                if (fprintf(ctx->output, "\"@type\": ") < 0) return false;
            }
            
            // Write object(s)
            if (object_list->type == TTL_AST_OBJECT_LIST) {
                size_t obj_count = object_list->data.object_list.object_count;
                ttl_ast_node_t** objects = object_list->data.object_list.objects;
                
                if (obj_count == 1) {
                    // Single object
                    if (!node_to_jsonld_value(ctx, objects[0])) return false;
                } else {
                    // Multiple objects - use array
                    if (fprintf(ctx->output, "[") < 0) return false;
                    for (size_t j = 0; j < obj_count; j++) {
                        if (j > 0) {
                            if (fprintf(ctx->output, ", ") < 0) return false;
                        }
                        if (!node_to_jsonld_value(ctx, objects[j])) return false;
                    }
                    if (fprintf(ctx->output, "]") < 0) return false;
                }
            }
            
            if (i + 2 < count) {
                if (fprintf(ctx->output, ",") < 0) return false;
            }
            if (!write_newline(ctx)) return false;
        }
    }
    
    ctx->indent_level--;
    if (!write_indent(ctx)) return false;
    if (fprintf(ctx->output, "}") < 0) return false;
    
    ctx->stats.triples_serialized++;
    
    return true;
}

/**
 * Create JSON-LD serializer
 */
ttl_serializer_t* ttl_create_jsonld_serializer(const ttl_serializer_options_t* options) {
    jsonld_context_t* ctx = calloc(1, sizeof(jsonld_context_t));
    if (!ctx) return NULL;
    
    if (options) {
        ctx->options = *options;
    } else {
        ctx->options = ttl_serializer_default_options(TTL_FORMAT_JSONLD);
    }
    
    ctx->output = ctx->options.output ? ctx->options.output : stdout;
    ctx->first_item = true;
    ctx->start_time = clock();
    
    return (ttl_serializer_t*)ctx;
}

/**
 * Serialize AST to JSON-LD format
 */
bool ttl_serialize_jsonld_ast(ttl_serializer_t* serializer, ttl_ast_node_t* root) {
    if (!serializer || !root) return false;
    
    jsonld_context_t* ctx = (jsonld_context_t*)serializer;
    
    // Start JSON-LD document
    if (fprintf(ctx->output, "{") < 0) return false;
    if (!write_newline(ctx)) return false;
    
    ctx->indent_level++;
    
    // Write context
    if (!write_context(ctx)) return false;
    
    // Start graph array
    if (!write_indent(ctx)) return false;
    if (fprintf(ctx->output, "\"@graph\": [") < 0) return false;
    if (!write_newline(ctx)) return false;
    
    ctx->indent_level++;
    
    // Create visitor for triple extraction
    ttl_ast_visitor_t* visitor = ttl_visitor_create();
    if (!visitor) return false;
    
    visitor->user_data = ctx;
    visitor->visit_triple = visit_triple;
    visitor->order = TTL_VISITOR_PRE_ORDER;
    
    // Traverse AST and serialize triples
    bool success = ttl_ast_accept(root, visitor);
    
    ctx->indent_level--;
    if (!write_newline(ctx)) return false;
    if (!write_indent(ctx)) return false;
    if (fprintf(ctx->output, "]") < 0) return false;
    if (!write_newline(ctx)) return false;
    
    ctx->indent_level--;
    if (fprintf(ctx->output, "}") < 0) return false;
    if (!write_newline(ctx)) return false;
    
    if (success && !ctx->has_error) {
        ctx->stats.serialization_time_ms = 
            ((double)(clock() - ctx->start_time)) / CLOCKS_PER_SEC * 1000.0;
    }
    
    ttl_visitor_destroy(visitor);
    return success && !ctx->has_error;
}

/**
 * Quick JSON-LD serialization function
 */
bool ttl_serialize_jsonld(ttl_ast_node_t* root, FILE* output, bool pretty_print) {
    ttl_serializer_options_t options = ttl_serializer_default_options(TTL_FORMAT_JSONLD);
    options.output = output;
    options.pretty_print = pretty_print;
    
    ttl_serializer_t* serializer = ttl_create_jsonld_serializer(&options);
    if (!serializer) return false;
    
    bool success = ttl_serialize_jsonld_ast(serializer, root);
    free(serializer);
    
    return success;
}