/**
 * @file rdfxml.c
 * @brief RDF/XML serialization implementation
 * @author CNS Seven-Tick Engine
 * @date 2024
 * 
 * RDF/XML is the legacy RDF serialization format.
 * Simple implementation focusing on correctness over optimization.
 */

#include "serializer.h"
#include "visitor.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

/**
 * RDF/XML serializer context
 */
typedef struct {
    FILE* output;
    ttl_serializer_options_t options;
    ttl_serializer_stats_t stats;
    
    // XML output state
    int indent_level;
    bool wrote_header;
    bool has_error;
    char error_message[256];
    
    // Namespace management
    bool wrote_namespaces;
    
    // Timing
    clock_t start_time;
} rdfxml_context_t;

/**
 * Escape string for XML
 */
static char* escape_xml_string(const char* input) {
    if (!input) return NULL;
    
    size_t len = strlen(input);
    size_t escaped_len = len * 6 + 1; // Worst case for &quot; etc.
    char* escaped = malloc(escaped_len);
    if (!escaped) return NULL;
    
    char* dest = escaped;
    for (const char* src = input; *src; src++) {
        switch (*src) {
            case '<':  strcpy(dest, "&lt;"); dest += 4; break;
            case '>':  strcpy(dest, "&gt;"); dest += 4; break;
            case '&':  strcpy(dest, "&amp;"); dest += 5; break;
            case '"':  strcpy(dest, "&quot;"); dest += 6; break;
            case '\'': strcpy(dest, "&apos;"); dest += 6; break;
            default:   *dest++ = *src; break;
        }
    }
    *dest = '\0';
    
    return escaped;
}

/**
 * Write indentation
 */
static bool write_indent(rdfxml_context_t* ctx) {
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
static bool write_newline(rdfxml_context_t* ctx) {
    if (!ctx->options.pretty_print) return true;
    
    if (fprintf(ctx->output, "\n") < 0) {
        ctx->has_error = true;
        strcpy(ctx->error_message, "Failed to write newline");
        return false;
    }
    return true;
}

/**
 * Write XML header and root element
 */
static bool write_header(rdfxml_context_t* ctx) {
    if (ctx->wrote_header) return true;
    
    // XML declaration
    if (fprintf(ctx->output, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>") < 0) return false;
    if (!write_newline(ctx)) return false;
    
    // RDF root element with namespaces
    if (!write_indent(ctx)) return false;
    if (fprintf(ctx->output, "<rdf:RDF") < 0) return false;
    
    // Standard namespaces
    if (fprintf(ctx->output, 
                " xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\"") < 0) return false;
    if (fprintf(ctx->output, 
                " xmlns:rdfs=\"http://www.w3.org/2000/01/rdf-schema#\"") < 0) return false;
    if (fprintf(ctx->output, 
                " xmlns:xsd=\"http://www.w3.org/2001/XMLSchema#\"") < 0) return false;
    
    if (fprintf(ctx->output, ">") < 0) return false;
    if (!write_newline(ctx)) return false;
    
    ctx->indent_level++;
    ctx->wrote_header = true;
    return true;
}

/**
 * Write XML footer
 */
static bool write_footer(rdfxml_context_t* ctx) {
    ctx->indent_level--;
    if (!write_indent(ctx)) return false;
    if (fprintf(ctx->output, "</rdf:RDF>") < 0) return false;
    if (!write_newline(ctx)) return false;
    return true;
}

/**
 * Get QName for IRI (simple implementation)
 */
static const char* get_qname(const char* iri) {
    // Simple mapping for common IRIs
    if (strstr(iri, "http://www.w3.org/1999/02/22-rdf-syntax-ns#")) {
        const char* local = strrchr(iri, '#');
        if (local) return local + 1; // Return local part, prefix will be "rdf:"
    }
    if (strstr(iri, "http://www.w3.org/2000/01/rdf-schema#")) {
        const char* local = strrchr(iri, '#');
        if (local) return local + 1; // Return local part, prefix will be "rdfs:"
    }
    if (strstr(iri, "http://www.w3.org/2001/XMLSchema#")) {
        const char* local = strrchr(iri, '#');
        if (local) return local + 1; // Return local part, prefix will be "xsd:"
    }
    return NULL;
}

/**
 * Get namespace prefix for IRI
 */
static const char* get_prefix(const char* iri) {
    if (strstr(iri, "http://www.w3.org/1999/02/22-rdf-syntax-ns#")) {
        return "rdf";
    }
    if (strstr(iri, "http://www.w3.org/2000/01/rdf-schema#")) {
        return "rdfs";
    }
    if (strstr(iri, "http://www.w3.org/2001/XMLSchema#")) {
        return "xsd";
    }
    return NULL;
}

/**
 * Serialize resource reference
 */
static bool serialize_resource_ref(rdfxml_context_t* ctx, ttl_ast_node_t* node) {
    if (!node) return false;
    
    switch (node->type) {
        case TTL_AST_IRI: {
            const char* iri = node->data.iri.value;
            char* escaped = escape_xml_string(iri);
            if (!escaped) return false;
            
            int result = fprintf(ctx->output, "rdf:resource=\"%s\"", escaped);
            free(escaped);
            return result >= 0;
        }
        
        case TTL_AST_PREFIXED_NAME: {
            const char* prefix = node->data.prefixed_name.prefix;
            const char* local = node->data.prefixed_name.local_name;
            
            // Expand to full IRI for resource attribute
            if (strcmp(prefix, "rdf") == 0) {
                return fprintf(ctx->output, 
                              "rdf:resource=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#%s\"", 
                              local) >= 0;
            } else if (strcmp(prefix, "rdfs") == 0) {
                return fprintf(ctx->output, 
                              "rdf:resource=\"http://www.w3.org/2000/01/rdf-schema#%s\"", 
                              local) >= 0;
            } else if (strcmp(prefix, "xsd") == 0) {
                return fprintf(ctx->output, 
                              "rdf:resource=\"http://www.w3.org/2001/XMLSchema#%s\"", 
                              local) >= 0;
            } else {
                return fprintf(ctx->output, "rdf:resource=\"%s%s\"", prefix, local) >= 0;
            }
        }
        
        case TTL_AST_BLANK_NODE: {
            const char* label = node->data.blank_node.label;
            if (label) {
                return fprintf(ctx->output, "rdf:nodeID=\"%s\"", label) >= 0;
            } else {
                return fprintf(ctx->output, "rdf:nodeID=\"genid%u\"", 
                              node->data.blank_node.id) >= 0;
            }
        }
        
        case TTL_AST_RDF_TYPE:
            return fprintf(ctx->output, 
                          "rdf:resource=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#type\"") >= 0;
        
        default:
            return false;
    }
}

/**
 * Serialize literal value
 */
static bool serialize_literal_content(rdfxml_context_t* ctx, ttl_ast_node_t* node) {
    switch (node->type) {
        case TTL_AST_STRING_LITERAL: {
            const char* value = node->data.string_literal.value;
            char* escaped = escape_xml_string(value);
            if (!escaped) return false;
            
            int result = fprintf(ctx->output, "%s", escaped);
            free(escaped);
            return result >= 0;
        }
        
        case TTL_AST_TYPED_LITERAL: {
            ttl_ast_node_t* value = node->data.typed_literal.value;
            ttl_ast_node_t* datatype = node->data.typed_literal.datatype;
            
            // Write datatype attribute first
            if (datatype->type == TTL_AST_IRI) {
                char* escaped = escape_xml_string(datatype->data.iri.value);
                if (!escaped) return false;
                int result = fprintf(ctx->output, " rdf:datatype=\"%s\"", escaped);
                free(escaped);
                if (result < 0) return false;
            } else if (datatype->type == TTL_AST_PREFIXED_NAME) {
                const char* prefix = datatype->data.prefixed_name.prefix;
                const char* local = datatype->data.prefixed_name.local_name;
                
                if (strcmp(prefix, "xsd") == 0) {
                    if (fprintf(ctx->output, 
                               " rdf:datatype=\"http://www.w3.org/2001/XMLSchema#%s\"", 
                               local) < 0) return false;
                } else {
                    if (fprintf(ctx->output, " rdf:datatype=\"%s%s\"", 
                               prefix, local) < 0) return false;
                }
            }
            
            if (fprintf(ctx->output, ">") < 0) return false;
            
            // Write value content
            return serialize_literal_content(ctx, value);
        }
        
        case TTL_AST_LANG_LITERAL: {
            ttl_ast_node_t* value = node->data.lang_literal.value;
            const char* lang = node->data.lang_literal.language_tag;
            
            // Write language attribute
            if (fprintf(ctx->output, " xml:lang=\"%s\">", lang) < 0) return false;
            
            // Write value content
            return serialize_literal_content(ctx, value);
        }
        
        case TTL_AST_NUMERIC_LITERAL: {
            const char* lexical = node->data.numeric_literal.lexical_form;
            const char* datatype_iri;
            
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
                default:
                    return false;
            }
            
            if (fprintf(ctx->output, " rdf:datatype=\"%s\">", datatype_iri) < 0) return false;
            
            if (lexical) {
                char* escaped = escape_xml_string(lexical);
                if (!escaped) return false;
                int result = fprintf(ctx->output, "%s", escaped);
                free(escaped);
                return result >= 0;
            } else {
                if (node->data.numeric_literal.numeric_type == TTL_NUMERIC_INTEGER) {
                    return fprintf(ctx->output, "%lld", 
                                  (long long)node->data.numeric_literal.integer_value) >= 0;
                } else {
                    return fprintf(ctx->output, "%g", 
                                  node->data.numeric_literal.double_value) >= 0;
                }
            }
        }
        
        case TTL_AST_BOOLEAN_LITERAL: {
            const char* value = node->data.boolean_literal.value ? "true" : "false";
            if (fprintf(ctx->output, 
                       " rdf:datatype=\"http://www.w3.org/2001/XMLSchema#boolean\">%s", 
                       value) < 0) return false;
            return true;
        }
        
        default:
            return false;
    }
}

/**
 * Check if node is a literal
 */
static bool is_literal(ttl_ast_node_t* node) {
    if (!node) return false;
    
    switch (node->type) {
        case TTL_AST_STRING_LITERAL:
        case TTL_AST_TYPED_LITERAL:
        case TTL_AST_LANG_LITERAL:
        case TTL_AST_NUMERIC_LITERAL:
        case TTL_AST_BOOLEAN_LITERAL:
            return true;
        default:
            return false;
    }
}

/**
 * Triple visitor for RDF/XML serialization
 */
static bool visit_triple(ttl_ast_visitor_t* visitor, ttl_ast_node_t* node) {
    if (node->type != TTL_AST_TRIPLE) return true;
    
    rdfxml_context_t* ctx = (rdfxml_context_t*)visitor->user_data;
    
    ttl_ast_node_t* subject = node->data.triple.subject;
    ttl_ast_node_t* pred_obj_list = node->data.triple.predicate_object_list;
    
    if (!subject || !pred_obj_list) return true;
    
    // Start rdf:Description element for subject
    if (!write_indent(ctx)) return false;
    if (fprintf(ctx->output, "<rdf:Description") < 0) return false;
    
    // Write subject reference
    if (subject->type == TTL_AST_IRI) {
        char* escaped = escape_xml_string(subject->data.iri.value);
        if (!escaped) return false;
        int result = fprintf(ctx->output, " rdf:about=\"%s\"", escaped);
        free(escaped);
        if (result < 0) return false;
    } else if (subject->type == TTL_AST_BLANK_NODE) {
        const char* label = subject->data.blank_node.label;
        if (label) {
            if (fprintf(ctx->output, " rdf:nodeID=\"%s\"", label) < 0) return false;
        } else {
            if (fprintf(ctx->output, " rdf:nodeID=\"genid%u\"", 
                       subject->data.blank_node.id) < 0) return false;
        }
    }
    
    if (fprintf(ctx->output, ">") < 0) return false;
    if (!write_newline(ctx)) return false;
    
    ctx->indent_level++;
    
    // Process predicate-object list
    if (pred_obj_list->type == TTL_AST_PREDICATE_OBJECT_LIST) {
        size_t count = pred_obj_list->data.predicate_object_list.item_count;
        ttl_ast_node_t** items = pred_obj_list->data.predicate_object_list.items;
        
        for (size_t i = 0; i < count; i += 2) {
            ttl_ast_node_t* predicate = items[i];
            ttl_ast_node_t* object_list = items[i + 1];
            
            if (object_list->type == TTL_AST_OBJECT_LIST) {
                size_t obj_count = object_list->data.object_list.object_count;
                ttl_ast_node_t** objects = object_list->data.object_list.objects;
                
                for (size_t j = 0; j < obj_count; j++) {
                    ttl_ast_node_t* object = objects[j];
                    
                    if (!write_indent(ctx)) return false;
                    
                    // Write predicate element
                    if (predicate->type == TTL_AST_IRI) {
                        const char* iri = predicate->data.iri.value;
                        const char* qname = get_qname(iri);
                        const char* prefix = get_prefix(iri);
                        
                        if (qname && prefix) {
                            if (fprintf(ctx->output, "<%s:%s", prefix, qname) < 0) return false;
                        } else {
                            // Use full IRI as element name (not ideal but works)
                            char* escaped = escape_xml_string(iri);
                            if (!escaped) return false;
                            int result = fprintf(ctx->output, "<rdf:_property rdf:about=\"%s\"", escaped);
                            free(escaped);
                            if (result < 0) return false;
                        }
                    } else if (predicate->type == TTL_AST_PREFIXED_NAME) {
                        const char* prefix = predicate->data.prefixed_name.prefix;
                        const char* local = predicate->data.prefixed_name.local_name;
                        if (fprintf(ctx->output, "<%s:%s", prefix, local) < 0) return false;
                    } else if (predicate->type == TTL_AST_RDF_TYPE) {
                        if (fprintf(ctx->output, "<rdf:type") < 0) return false;
                    }
                    
                    // Handle object
                    if (is_literal(object)) {
                        // Literal object - write as element content
                        if (!serialize_literal_content(ctx, object)) return false;
                        
                        // Close predicate element
                        if (predicate->type == TTL_AST_IRI) {
                            const char* iri = predicate->data.iri.value;
                            const char* qname = get_qname(iri);
                            const char* prefix = get_prefix(iri);
                            
                            if (qname && prefix) {
                                if (fprintf(ctx->output, "</%s:%s>", prefix, qname) < 0) return false;
                            } else {
                                if (fprintf(ctx->output, "</rdf:_property>") < 0) return false;
                            }
                        } else if (predicate->type == TTL_AST_PREFIXED_NAME) {
                            const char* prefix = predicate->data.prefixed_name.prefix;
                            const char* local = predicate->data.prefixed_name.local_name;
                            if (fprintf(ctx->output, "</%s:%s>", prefix, local) < 0) return false;
                        } else if (predicate->type == TTL_AST_RDF_TYPE) {
                            if (fprintf(ctx->output, "</rdf:type>") < 0) return false;
                        }
                    } else {
                        // Resource object - write as attribute
                        if (fprintf(ctx->output, " ") < 0) return false;
                        if (!serialize_resource_ref(ctx, object)) return false;
                        if (fprintf(ctx->output, "/>") < 0) return false;
                    }
                    
                    if (!write_newline(ctx)) return false;
                    ctx->stats.triples_serialized++;
                }
            }
        }
    }
    
    ctx->indent_level--;
    if (!write_indent(ctx)) return false;
    if (fprintf(ctx->output, "</rdf:Description>") < 0) return false;
    if (!write_newline(ctx)) return false;
    
    return true;
}

/**
 * Create RDF/XML serializer
 */
ttl_serializer_t* ttl_create_rdfxml_serializer(const ttl_serializer_options_t* options) {
    rdfxml_context_t* ctx = calloc(1, sizeof(rdfxml_context_t));
    if (!ctx) return NULL;
    
    if (options) {
        ctx->options = *options;
    } else {
        ctx->options = ttl_serializer_default_options(TTL_FORMAT_RDFXML);
    }
    
    ctx->output = ctx->options.output ? ctx->options.output : stdout;
    ctx->start_time = clock();
    
    return (ttl_serializer_t*)ctx;
}

/**
 * Serialize AST to RDF/XML format
 */
bool ttl_serialize_rdfxml_ast(ttl_serializer_t* serializer, ttl_ast_node_t* root) {
    if (!serializer || !root) return false;
    
    rdfxml_context_t* ctx = (rdfxml_context_t*)serializer;
    
    // Write XML header
    if (!write_header(ctx)) return false;
    
    // Create visitor for triple extraction
    ttl_ast_visitor_t* visitor = ttl_visitor_create();
    if (!visitor) return false;
    
    visitor->user_data = ctx;
    visitor->visit_triple = visit_triple;
    visitor->order = TTL_VISITOR_PRE_ORDER;
    
    // Traverse AST and serialize triples
    bool success = ttl_ast_accept(root, visitor);
    
    // Write XML footer
    if (success && !ctx->has_error) {
        success = write_footer(ctx);
        ctx->stats.serialization_time_ms = 
            ((double)(clock() - ctx->start_time)) / CLOCKS_PER_SEC * 1000.0;
    }
    
    ttl_visitor_destroy(visitor);
    return success && !ctx->has_error;
}

/**
 * Quick RDF/XML serialization function
 */
bool ttl_serialize_rdfxml(ttl_ast_node_t* root, FILE* output, bool use_prefixes) {
    ttl_serializer_options_t options = ttl_serializer_default_options(TTL_FORMAT_RDFXML);
    options.output = output;
    options.use_prefixes = use_prefixes;
    
    ttl_serializer_t* serializer = ttl_create_rdfxml_serializer(&options);
    if (!serializer) return false;
    
    bool success = ttl_serialize_rdfxml_ast(serializer, root);
    free(serializer);
    
    return success;
}