/**
 * @file parser.h
 * @brief TTL/Turtle recursive descent parser interface
 * @author CNS Seven-Tick Engine
 * @date 2024
 * 
 * This file defines the parser interface for parsing TTL (Turtle) documents
 * using a recursive descent approach with single-token lookahead.
 */

#ifndef TTL_PARSER_H
#define TTL_PARSER_H

#include "ast.h"
#include "lexer.h"
#include "token.h"
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct Parser Parser;
typedef struct ParseError ParseError;
typedef struct ParserOptions ParserOptions;
typedef struct ParserStats ParserStats;

/**
 * Error severity levels
 */
typedef enum {
    ERROR_SEVERITY_WARNING,    /* Non-fatal warning */
    ERROR_SEVERITY_ERROR,      /* Recoverable error */
    ERROR_SEVERITY_FATAL       /* Fatal error, parsing stopped */
} ErrorSeverity;

/**
 * Parse error structure
 */
struct ParseError {
    int line;                  /* Line number where error occurred */
    int column;                /* Column number where error occurred */
    char* message;             /* Error message */
    char* context;             /* Surrounding context */
    char* suggestion;          /* Suggested fix (optional) */
    ErrorSeverity severity;    /* Error severity */
    ParseError* next;          /* Next error in linked list */
};

/**
 * Parser options
 */
struct ParserOptions {
    bool strict_mode;          /* W3C strict compliance */
    bool error_recovery;       /* Enable error recovery */
    bool track_comments;       /* Include comments in AST */
    bool validate_iris;        /* Validate IRI syntax */
    bool normalize_literals;   /* Normalize numeric literals */
    size_t max_errors;         /* Maximum errors before stopping */
    const char* base_iri;      /* Default base IRI */
};

/**
 * Parser statistics
 */
struct ParserStats {
    size_t statements_parsed;  /* Number of statements parsed */
    size_t triples_parsed;     /* Number of triples parsed */
    size_t errors_recovered;   /* Number of errors recovered from */
    size_t max_depth;          /* Maximum parse tree depth */
    size_t tokens_consumed;    /* Total tokens consumed */
    double parse_time_ms;      /* Parse time in milliseconds */
};

/**
 * Create parser from string input
 * @param input Input string (UTF-8 encoded)
 * @param length Input length (0 for null-terminated)
 * @param options Parser options (NULL for defaults)
 * @return New parser instance (must be freed with ttl_parser_destroy)
 */
Parser* ttl_parser_create(const char* input, size_t length, const ParserOptions* options);

/**
 * Create parser from file
 * @param file File handle
 * @param options Parser options (NULL for defaults)
 * @return New parser instance (must be freed with ttl_parser_destroy)
 */
Parser* ttl_parser_create_from_file(FILE* file, const ParserOptions* options);

/**
 * Create parser with existing lexer
 * @param lexer Lexer instance (parser takes ownership)
 * @param options Parser options (NULL for defaults)
 * @return New parser instance
 */
Parser* ttl_parser_create_with_lexer(Lexer* lexer, const ParserOptions* options);

/**
 * Destroy parser instance
 * @param parser Parser to destroy
 */
void ttl_parser_destroy(Parser* parser);

/**
 * Parse document and return AST
 * @param parser Parser instance
 * @return AST root node (NULL on fatal error)
 */
ttl_ast_node_t* ttl_parser_parse(Parser* parser);

/**
 * Check if parser has errors
 * @param parser Parser instance
 * @return True if errors occurred
 */
bool ttl_parser_has_errors(const Parser* parser);

/**
 * Get error count
 * @param parser Parser instance
 * @return Number of errors
 */
size_t ttl_parser_error_count(const Parser* parser);

/**
 * Get error by index
 * @param parser Parser instance
 * @param index Error index
 * @return Error at index (NULL if out of bounds)
 */
const ParseError* ttl_parser_get_error(const Parser* parser, size_t index);

/**
 * Get all errors
 * @param parser Parser instance
 * @return Linked list of errors (NULL if no errors)
 */
const ParseError* ttl_parser_get_errors(const Parser* parser);

/**
 * Set base IRI
 * @param parser Parser instance
 * @param base_iri Base IRI
 */
void ttl_parser_set_base_iri(Parser* parser, const char* base_iri);

/**
 * Set strict mode
 * @param parser Parser instance
 * @param strict Enable strict W3C compliance
 */
void ttl_parser_set_strict_mode(Parser* parser, bool strict);

/**
 * Get parser statistics
 * @param parser Parser instance
 * @param stats Output statistics structure
 */
void ttl_parser_get_stats(const Parser* parser, ParserStats* stats);

/**
 * Get default parser options
 * @return Default options structure
 */
ParserOptions ttl_parser_default_options(void);

/**
 * Streaming parser callbacks
 */
typedef struct {
    void* user_data;
    
    /* Called for each complete triple */
    void (*on_triple)(void* data, 
                     const char* subject,
                     const char* predicate, 
                     const char* object);
    
    /* Called for prefix declarations */
    void (*on_prefix)(void* data,
                     const char* prefix,
                     const char* iri);
    
    /* Called for base declaration */
    void (*on_base)(void* data, const char* iri);
    
    /* Error callback */
    void (*on_error)(void* data, const ParseError* error);
} StreamingCallbacks;

/* Streaming parser */
typedef struct StreamingParser StreamingParser;

/**
 * Create streaming parser
 * @param callbacks Callback functions
 * @param options Parser options (NULL for defaults)
 * @return New streaming parser
 */
StreamingParser* ttl_streaming_parser_create(const StreamingCallbacks* callbacks, 
                                             const ParserOptions* options);

/**
 * Feed data to streaming parser
 * @param parser Streaming parser
 * @param data Input data
 * @param length Data length
 * @return True if successful, false on fatal error
 */
bool ttl_streaming_parser_feed(StreamingParser* parser, 
                              const char* data, 
                              size_t length);

/**
 * Signal end of input
 * @param parser Streaming parser
 */
void ttl_streaming_parser_end(StreamingParser* parser);

/**
 * Destroy streaming parser
 * @param parser Streaming parser to destroy
 */
void ttl_streaming_parser_destroy(StreamingParser* parser);

/**
 * Utility functions
 */

/**
 * Validate TTL syntax without building AST
 * @param input Input string
 * @param length Input length
 * @param errors Output error array (optional)
 * @param error_count Output error count (optional)
 * @return True if valid TTL
 */
bool ttl_validate_syntax(const char* input, size_t length,
                        ParseError** errors, size_t* error_count);

/**
 * Parse single statement
 * @param input Statement string
 * @param length Input length
 * @param base_iri Base IRI (optional)
 * @return Parsed statement AST (NULL on error)
 */
ttl_ast_node_t* ttl_parse_statement(const char* input, size_t length,
                                    const char* base_iri);

#ifdef __cplusplus
}
#endif

#endif /* TTL_PARSER_H */