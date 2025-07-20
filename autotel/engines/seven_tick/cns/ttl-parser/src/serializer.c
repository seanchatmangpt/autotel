/**
 * @file serializer.c
 * @brief Main serializer implementation and API
 * @author CNS Seven-Tick Engine
 * @date 2024
 * 
 * This file provides the main serializer API and coordinates between
 * the different format-specific serializers.
 */

#include "serializer.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

/**
 * Generic serializer structure
 */
struct ttl_serializer {
    ttl_serializer_format_t format;
    ttl_serializer_options_t options;
    ttl_serializer_stats_t stats;
    void* format_context; // Format-specific context
    bool has_error;
    char error_message[256];
};

/**
 * Forward declarations for format-specific functions
 */
extern ttl_serializer_t* ttl_create_ntriples_serializer(const ttl_serializer_options_t* options);
extern bool ttl_serialize_ntriples_ast(ttl_serializer_t* serializer, ttl_ast_node_t* root);

extern ttl_serializer_t* ttl_create_jsonld_serializer(const ttl_serializer_options_t* options);
extern bool ttl_serialize_jsonld_ast(ttl_serializer_t* serializer, ttl_ast_node_t* root);

extern ttl_serializer_t* ttl_create_rdfxml_serializer(const ttl_serializer_options_t* options);
extern bool ttl_serialize_rdfxml_ast(ttl_serializer_t* serializer, ttl_ast_node_t* root);

/**
 * Get default options for format
 */
ttl_serializer_options_t ttl_serializer_default_options(ttl_serializer_format_t format) {
    ttl_serializer_options_t options = {0};
    
    switch (format) {
        case TTL_FORMAT_NTRIPLES:
            options.pretty_print = false;     // N-Triples is naturally line-based
            options.use_prefixes = false;     // N-Triples doesn't support prefixes
            options.escape_unicode = true;    // Always escape for compatibility
            options.include_comments = false; // N-Triples doesn't support comments
            break;
            
        case TTL_FORMAT_JSONLD:
            options.pretty_print = true;      // JSON-LD benefits from formatting
            options.use_prefixes = true;      // Use compact form where possible
            options.escape_unicode = false;   // JSON handles UTF-8 natively
            options.include_comments = false; // JSON-LD doesn't support comments
            break;
            
        case TTL_FORMAT_RDFXML:
            options.pretty_print = true;      // XML benefits from formatting
            options.use_prefixes = true;      // Use namespace prefixes
            options.escape_unicode = true;    // XML escaping required
            options.include_comments = false; // Basic implementation doesn't support
            break;
            
        default:
            // Safe defaults
            options.pretty_print = true;
            options.use_prefixes = true;
            options.escape_unicode = true;
            options.include_comments = false;
            break;
    }
    
    options.base_iri = NULL;
    options.output = NULL; // Will default to stdout
    
    return options;
}

/**
 * Create serializer for specific format
 */
ttl_serializer_t* ttl_serializer_create(ttl_serializer_format_t format, 
                                        const ttl_serializer_options_t* options) {
    if (format >= TTL_FORMAT_COUNT) return NULL;
    
    ttl_serializer_t* serializer = calloc(1, sizeof(ttl_serializer_t));
    if (!serializer) return NULL;
    
    serializer->format = format;
    
    if (options) {
        serializer->options = *options;
    } else {
        serializer->options = ttl_serializer_default_options(format);
    }
    
    // Default to stdout if no output specified
    if (!serializer->options.output) {
        serializer->options.output = stdout;
    }
    
    // Create format-specific context
    switch (format) {
        case TTL_FORMAT_NTRIPLES:
            serializer->format_context = ttl_create_ntriples_serializer(&serializer->options);
            break;
        case TTL_FORMAT_JSONLD:
            serializer->format_context = ttl_create_jsonld_serializer(&serializer->options);
            break;
        case TTL_FORMAT_RDFXML:
            serializer->format_context = ttl_create_rdfxml_serializer(&serializer->options);
            break;
        default:
            free(serializer);
            return NULL;
    }
    
    if (!serializer->format_context) {
        free(serializer);
        return NULL;
    }
    
    return serializer;
}

/**
 * Destroy serializer
 */
void ttl_serializer_destroy(ttl_serializer_t* serializer) {
    if (!serializer) return;
    
    if (serializer->format_context) {
        free(serializer->format_context);
    }
    
    free(serializer);
}

/**
 * Serialize AST to output stream
 */
bool ttl_serializer_serialize(ttl_serializer_t* serializer, ttl_ast_node_t* root) {
    if (!serializer || !root) return false;
    
    clock_t start_time = clock();
    bool success = false;
    
    switch (serializer->format) {
        case TTL_FORMAT_NTRIPLES:
            success = ttl_serialize_ntriples_ast(serializer->format_context, root);
            break;
        case TTL_FORMAT_JSONLD:
            success = ttl_serialize_jsonld_ast(serializer->format_context, root);
            break;
        case TTL_FORMAT_RDFXML:
            success = ttl_serialize_rdfxml_ast(serializer->format_context, root);
            break;
        default:
            return false;
    }
    
    if (success) {
        double time_ms = ((double)(clock() - start_time)) / CLOCKS_PER_SEC * 1000.0;
        serializer->stats.serialization_time_ms = time_ms;
    }
    
    return success;
}

/**
 * Serialize AST to string
 */
bool ttl_serializer_serialize_to_string(ttl_serializer_t* serializer, 
                                        ttl_ast_node_t* root, 
                                        char** output) {
    if (!serializer || !root || !output) return false;
    
    // Create temporary file for capturing output
    FILE* temp = tmpfile();
    if (!temp) return false;
    
    // Save original output and redirect to temp file
    FILE* original_output = serializer->options.output;
    serializer->options.output = temp;
    
    // Serialize to temp file
    bool success = ttl_serializer_serialize(serializer, root);
    
    if (success) {
        // Get file size
        fseek(temp, 0, SEEK_END);
        long size = ftell(temp);
        fseek(temp, 0, SEEK_SET);
        
        // Allocate buffer and read content
        *output = malloc(size + 1);
        if (*output) {
            size_t read_size = fread(*output, 1, size, temp);
            (*output)[read_size] = '\0';
        } else {
            success = false;
        }
    }
    
    // Restore original output
    serializer->options.output = original_output;
    fclose(temp);
    
    return success;
}

/**
 * Get serialization statistics
 */
void ttl_serializer_get_stats(const ttl_serializer_t* serializer, 
                             ttl_serializer_stats_t* stats) {
    if (!serializer || !stats) return;
    
    *stats = serializer->stats;
}

/**
 * Escape string for target format
 */
char* ttl_serializer_escape_string(const char* input, ttl_serializer_format_t format) {
    if (!input) return NULL;
    
    size_t len = strlen(input);
    size_t max_len = len * 6 + 1; // Worst case for all formats
    char* escaped = malloc(max_len);
    if (!escaped) return NULL;
    
    char* dest = escaped;
    
    switch (format) {
        case TTL_FORMAT_NTRIPLES:
            // N-Triples escaping: newline, carriage return, tab, quote, backslash
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
            break;
            
        case TTL_FORMAT_JSONLD:
            // JSON escaping
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
            break;
            
        case TTL_FORMAT_RDFXML:
            // XML escaping
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
            break;
            
        default:
            // Safe fallback - copy as-is
            strcpy(escaped, input);
            return escaped;
    }
    
    *dest = '\0';
    return escaped;
}

/**
 * Get format name
 */
const char* ttl_serializer_format_name(ttl_serializer_format_t format) {
    switch (format) {
        case TTL_FORMAT_NTRIPLES: return "N-Triples";
        case TTL_FORMAT_JSONLD:   return "JSON-LD";
        case TTL_FORMAT_RDFXML:   return "RDF/XML";
        default:                  return "Unknown";
    }
}

/**
 * Get MIME type for format
 */
const char* ttl_serializer_mime_type(ttl_serializer_format_t format) {
    switch (format) {
        case TTL_FORMAT_NTRIPLES: return "application/n-triples";
        case TTL_FORMAT_JSONLD:   return "application/ld+json";
        case TTL_FORMAT_RDFXML:   return "application/rdf+xml";
        default:                  return "application/octet-stream";
    }
}

/**
 * Get file extension for format
 */
const char* ttl_serializer_file_extension(ttl_serializer_format_t format) {
    switch (format) {
        case TTL_FORMAT_NTRIPLES: return "nt";
        case TTL_FORMAT_JSONLD:   return "jsonld";
        case TTL_FORMAT_RDFXML:   return "rdf";
        default:                  return "txt";
    }
}