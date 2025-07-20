/**
 * @file serializer.h
 * @brief RDF serialization interfaces for TTL parser
 * @author CNS Seven-Tick Engine
 * @date 2024
 * 
 * This file defines the serialization interfaces for converting TTL AST
 * to various RDF formats using the visitor pattern with 80/20 optimization.
 */

#ifndef TTL_SERIALIZER_H
#define TTL_SERIALIZER_H

#include "ast.h"
#include "visitor.h"
#include <stdio.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Serialization formats (prioritized by usage)
 */
typedef enum {
    TTL_FORMAT_NTRIPLES,    // 80% use case - simple, widely supported
    TTL_FORMAT_JSONLD,      // 15% use case - web-friendly
    TTL_FORMAT_RDFXML,      // 5% use case - legacy support
    TTL_FORMAT_COUNT
} ttl_serializer_format_t;

/**
 * Serialization options
 */
typedef struct {
    bool pretty_print;           // Format output for readability
    bool use_prefixes;          // Use namespace prefixes where possible
    bool escape_unicode;        // Escape non-ASCII characters
    bool include_comments;      // Preserve comments (format dependent)
    const char* base_iri;       // Base IRI for relative IRIs
    FILE* output;               // Output stream
} ttl_serializer_options_t;

/**
 * Serialization statistics
 */
typedef struct {
    size_t triples_serialized;  // Number of triples output
    size_t bytes_written;       // Total bytes written
    size_t prefixes_used;       // Number of prefixes used
    double serialization_time_ms; // Time taken in milliseconds
} ttl_serializer_stats_t;

/**
 * Serializer context
 */
typedef struct ttl_serializer ttl_serializer_t;

/**
 * Create serializer for specific format
 * @param format Target format
 * @param options Serialization options (NULL for defaults)
 * @return New serializer instance
 */
ttl_serializer_t* ttl_serializer_create(ttl_serializer_format_t format, 
                                        const ttl_serializer_options_t* options);

/**
 * Destroy serializer
 * @param serializer Serializer to destroy
 */
void ttl_serializer_destroy(ttl_serializer_t* serializer);

/**
 * Serialize AST to output stream
 * @param serializer Serializer instance
 * @param root AST root node
 * @return True if successful, false on error
 */
bool ttl_serializer_serialize(ttl_serializer_t* serializer, ttl_ast_node_t* root);

/**
 * Serialize AST to string
 * @param serializer Serializer instance  
 * @param root AST root node
 * @param output Output string (caller must free)
 * @return True if successful, false on error
 */
bool ttl_serializer_serialize_to_string(ttl_serializer_t* serializer, 
                                        ttl_ast_node_t* root, 
                                        char** output);

/**
 * Get serialization statistics
 * @param serializer Serializer instance
 * @param stats Output statistics
 */
void ttl_serializer_get_stats(const ttl_serializer_t* serializer, 
                             ttl_serializer_stats_t* stats);

/**
 * Get default options for format
 * @param format Target format
 * @return Default options
 */
ttl_serializer_options_t ttl_serializer_default_options(ttl_serializer_format_t format);

/**
 * Format-specific quick serialization functions
 */

/**
 * Serialize to N-Triples format
 * @param root AST root node
 * @param output Output stream
 * @return True if successful
 */
bool ttl_serialize_ntriples(ttl_ast_node_t* root, FILE* output);

/**
 * Serialize to JSON-LD format
 * @param root AST root node
 * @param output Output stream
 * @param pretty_print Format for readability
 * @return True if successful
 */
bool ttl_serialize_jsonld(ttl_ast_node_t* root, FILE* output, bool pretty_print);

/**
 * Serialize to RDF/XML format
 * @param root AST root node
 * @param output Output stream
 * @param use_prefixes Use namespace prefixes
 * @return True if successful
 */
bool ttl_serialize_rdfxml(ttl_ast_node_t* root, FILE* output, bool use_prefixes);

/**
 * Utility functions
 */

/**
 * Escape string for target format
 * @param input Input string
 * @param format Target format
 * @return Escaped string (caller must free)
 */
char* ttl_serializer_escape_string(const char* input, ttl_serializer_format_t format);

/**
 * Get format name
 * @param format Format enum
 * @return Human-readable format name
 */
const char* ttl_serializer_format_name(ttl_serializer_format_t format);

/**
 * Get MIME type for format
 * @param format Format enum
 * @return MIME type string
 */
const char* ttl_serializer_mime_type(ttl_serializer_format_t format);

/**
 * Get file extension for format
 * @param format Format enum
 * @return File extension (without dot)
 */
const char* ttl_serializer_file_extension(ttl_serializer_format_t format);

#ifdef __cplusplus
}
#endif

#endif /* TTL_SERIALIZER_H */