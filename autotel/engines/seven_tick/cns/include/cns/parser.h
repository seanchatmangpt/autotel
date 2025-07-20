#ifndef CNS_PARSER_H
#define CNS_PARSER_H

#include "cns/types.h"
#include "cns/arena.h"
#include "cns/interner.h"
#include "cns/graph.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// CNS PARSER - HIGH-PERFORMANCE TTL/RDF PARSER FOR 7T SUBSTRATE
// ============================================================================

// TTL/RDF parser designed for 7T substrate with deterministic performance
// guarantees. All parsing operations maintain 7-tick performance through
// incremental parsing, efficient tokenization, and arena allocation.

// ============================================================================
// TOKEN DEFINITIONS
// ============================================================================

// Token types for TTL/RDF parsing
typedef enum {
    CNS_TOKEN_EOF = 0,           // End of file
    CNS_TOKEN_IRI,               // IRI reference <http://example.org>
    CNS_TOKEN_PREFIXED_NAME,     // Prefixed name prefix:localname
    CNS_TOKEN_BLANK_NODE,        // Blank node _:label
    CNS_TOKEN_LITERAL,           // String literal "value"
    CNS_TOKEN_LITERAL_LANG,      // Language-tagged literal "value"@lang
    CNS_TOKEN_LITERAL_TYPE,      // Typed literal "value"^^<type>
    CNS_TOKEN_INTEGER,           // Integer literal 123
    CNS_TOKEN_DECIMAL,           // Decimal literal 123.45
    CNS_TOKEN_DOUBLE,            // Double literal 1.23e4
    CNS_TOKEN_BOOLEAN,           // Boolean literal true/false
    CNS_TOKEN_PREFIX,            // @prefix directive
    CNS_TOKEN_BASE,              // @base directive
    CNS_TOKEN_DOT,               // Statement terminator .
    CNS_TOKEN_SEMICOLON,         // Property separator ;
    CNS_TOKEN_COMMA,             // Object separator ,
    CNS_TOKEN_LBRACKET,          // Left bracket [
    CNS_TOKEN_RBRACKET,          // Right bracket ]
    CNS_TOKEN_LPAREN,            // Left parenthesis (
    CNS_TOKEN_RPAREN,            // Right parenthesis )
    CNS_TOKEN_A,                 // 'a' keyword (rdf:type shorthand)
    CNS_TOKEN_ANON,              // Anonymous blank node []
    CNS_TOKEN_COLLECTION,        // Collection ()
    CNS_TOKEN_WHITESPACE,        // Whitespace (usually skipped)
    CNS_TOKEN_COMMENT,           // Comment # ... (usually skipped)
    CNS_TOKEN_ERROR,             // Parse error
    CNS_TOKEN_COUNT              // Number of token types
} cns_token_type_t;

// Token structure - optimized for fast processing
typedef struct cns_token {
    cns_token_type_t type;       // Token type
    cns_string_ref_t value;      // Token value (interned string)
    const char *start;           // Start position in input
    const char *end;             // End position in input
    uint32_t line;               // Line number
    uint32_t column;             // Column number
    uint32_t flags;              // Token flags
} cns_token_t;

// Token flags
#define CNS_TOKEN_FLAG_ESCAPED     (1 << 0)  // Contains escape sequences
#define CNS_TOKEN_FLAG_MULTILINE   (1 << 1)  // Multiline token
#define CNS_TOKEN_FLAG_LONG_STRING (1 << 2)  // Long string literal """..."""

// ============================================================================
// AST NODE DEFINITIONS
// ============================================================================

// AST node types for structured parsing
typedef enum {
    CNS_AST_DOCUMENT,            // Document root
    CNS_AST_PREFIX_DECL,         // Prefix declaration
    CNS_AST_BASE_DECL,           // Base declaration
    CNS_AST_TRIPLE,              // Triple statement
    CNS_AST_SUBJECT,             // Subject term
    CNS_AST_PREDICATE,           // Predicate term
    CNS_AST_OBJECT,              // Object term
    CNS_AST_BLANK_NODE_PROPERTY_LIST, // Blank node with properties
    CNS_AST_COLLECTION,          // RDF collection
    CNS_AST_IRI,                 // IRI reference
    CNS_AST_PREFIXED_NAME,       // Prefixed name
    CNS_AST_LITERAL,             // Literal value
    CNS_AST_BLANK_NODE,          // Blank node
} cns_ast_node_type_t;

// AST node structure - arena-allocated for fast cleanup
typedef struct cns_ast_node {
    cns_ast_node_type_t type;    // Node type
    cns_token_t token;           // Associated token
    struct cns_ast_node *parent; // Parent node
    struct cns_ast_node *first_child; // First child node
    struct cns_ast_node *next_sibling; // Next sibling node
    uint32_t child_count;        // Number of children
    uint32_t flags;              // Node flags
    void *data;                  // Node-specific data
} cns_ast_node_t;

// ============================================================================
// PARSER STATE AND STRUCTURES
// ============================================================================

// Parser state for tracking current position and context
typedef struct {
    const char *input;           // Input text
    const char *current;         // Current parse position
    const char *end;             // End of input
    size_t length;               // Input length
    uint32_t line;               // Current line number
    uint32_t column;             // Current column number
    uint32_t error_count;        // Number of parse errors
    cns_token_t lookahead;       // Lookahead token
    bool has_lookahead;          // True if lookahead is valid
} cns_parser_state_t;

// Parser error information
typedef struct {
    cns_result_t code;           // Error code
    uint32_t line;               // Error line number
    uint32_t column;             // Error column number
    const char *message;         // Error message
    const char *position;        // Error position in input
    uint32_t length;             // Error token length
} cns_parser_error_t;

// Parser statistics for monitoring
typedef struct {
    uint64_t tokens_parsed;      // Total tokens parsed
    uint64_t triples_parsed;     // Total triples parsed
    uint64_t prefixes_defined;   // Number of prefix definitions
    uint64_t errors_encountered; // Number of parse errors
    size_t input_bytes;          // Total input bytes processed
    cns_tick_t total_parse_ticks; // Total parsing time
    cns_tick_t tokenize_ticks;   // Time spent tokenizing
    cns_tick_t ast_ticks;        // Time spent building AST
    cns_tick_t graph_ticks;      // Time spent building graph
} cns_parser_stats_t;

// Main parser structure
struct cns_parser {
    // Memory management
    cns_arena_t *token_arena;    // Arena for tokens
    cns_arena_t *ast_arena;      // Arena for AST nodes
    cns_interner_t *interner;    // String interner
    
    // Target graph
    cns_graph_t *graph;          // Target RDF graph
    
    // Parser state
    cns_parser_state_t state;    // Current parser state
    
    // Prefix mappings
    cns_string_ref_t *prefixes;  // Prefix namespace mappings
    cns_string_ref_t *prefix_iris; // Prefix IRI mappings
    size_t prefix_count;         // Number of defined prefixes
    size_t prefix_capacity;      // Prefix array capacity
    
    // Base IRI
    cns_string_ref_t base_iri;   // Current base IRI
    
    // Blank node management
    uint32_t blank_node_counter; // Counter for anonymous blank nodes
    
    // Error handling
    cns_parser_error_t *errors;  // Array of parse errors
    size_t error_count;          // Number of errors
    size_t error_capacity;       // Error array capacity
    
    // Performance tracking
    cns_parser_stats_t stats;    // Parser statistics
    
    // Configuration
    uint32_t flags;              // Parser flags
    bool strict_mode;            // Strict parsing mode
    bool build_ast;              // Build AST during parsing
    bool incremental_mode;       // Incremental parsing mode
    
    // Thread safety (if enabled)
    void *mutex;                 // Mutex for thread safety
    uint32_t magic;              // Magic number for validation
};

// Parser flags
#define CNS_PARSER_FLAG_STRICT         (1 << 0)  // Strict RDF validation
#define CNS_PARSER_FLAG_BUILD_AST      (1 << 1)  // Build AST tree
#define CNS_PARSER_FLAG_INCREMENTAL    (1 << 2)  // Incremental parsing
#define CNS_PARSER_FLAG_SKIP_COMMENTS  (1 << 3)  // Skip comment tokens
#define CNS_PARSER_FLAG_SKIP_WHITESPACE (1 << 4) // Skip whitespace tokens
#define CNS_PARSER_FLAG_THREAD_SAFE    (1 << 5)  // Thread-safe operations
#define CNS_PARSER_FLAG_VALIDATE_IRIS  (1 << 6)  // Validate IRI syntax
#define CNS_PARSER_FLAG_COLLECT_ERRORS (1 << 7)  // Collect parse errors

// Magic number for parser validation
#define CNS_PARSER_MAGIC 0x50415253  // 'PARS'

// ============================================================================
// PARSER LIFECYCLE FUNCTIONS - O(1) OPERATIONS
// ============================================================================

// Create a new TTL/RDF parser with specified configuration
// PERFORMANCE: O(1) - completes within 7 CPU ticks
cns_parser_t* cns_parser_create(const cns_parser_config_t *config);

// Create parser with default configuration
// PERFORMANCE: O(1) - optimized default settings
cns_parser_t* cns_parser_create_default(cns_arena_t *arena, 
                                       cns_interner_t *interner,
                                       cns_graph_t *graph);

// Destroy parser and free all memory
// PERFORMANCE: O(1) when using arenas - just resets arena pointers
void cns_parser_destroy(cns_parser_t *parser);

// Reset parser state for new input
// PERFORMANCE: O(1) - resets state and counters
cns_result_t cns_parser_reset(cns_parser_t *parser);

// Clone parser configuration (not state)
// PERFORMANCE: O(1) - copies configuration only
cns_parser_t* cns_parser_clone_config(const cns_parser_t *parser);

// ============================================================================
// TOKENIZATION FUNCTIONS - 7T GUARANTEED
// ============================================================================

// Get next token from input
// PERFORMANCE: O(1) average - single character scanning with state machine
cns_result_t cns_parser_next_token(cns_parser_t *parser, cns_token_t *token);

// Peek at next token without consuming it
// PERFORMANCE: O(1) - uses lookahead buffer
cns_result_t cns_parser_peek_token(cns_parser_t *parser, cns_token_t *token);

// Consume current lookahead token
// PERFORMANCE: O(1) - moves lookahead to current
cns_result_t cns_parser_consume_token(cns_parser_t *parser);

// Skip tokens of specific type
// PERFORMANCE: O(k) where k is number of tokens skipped
cns_result_t cns_parser_skip_tokens(cns_parser_t *parser, cns_token_type_t type);

// ============================================================================
// PARSING FUNCTIONS - 7T PERFORMANCE
// ============================================================================

// Parse complete TTL document
// PERFORMANCE: O(n) where n is input size, but 7T per token
cns_result_t cns_parser_parse_document(cns_parser_t *parser, const char *input);

// Parse TTL document from string with length
// PERFORMANCE: O(n) where n is input size
cns_result_t cns_parser_parse_string(cns_parser_t *parser, 
                                    const char *input, 
                                    size_t length);

// Parse TTL document from file
// PERFORMANCE: O(n) where n is file size
cns_result_t cns_parser_parse_file(cns_parser_t *parser, const char *filename);

// Incremental parsing - parse next statement
// PERFORMANCE: O(1) for single statement, 7T guaranteed
cns_result_t cns_parser_parse_statement(cns_parser_t *parser);

// Parse single triple
// PERFORMANCE: O(1) - 7T guaranteed
cns_result_t cns_parser_parse_triple(cns_parser_t *parser, cns_triple_t *triple);

// ============================================================================
// PREFIX AND BASE MANAGEMENT
// ============================================================================

// Define prefix mapping
// PERFORMANCE: O(1) - hash table insertion
cns_result_t cns_parser_define_prefix(cns_parser_t *parser,
                                     const char *prefix,
                                     const char *iri);

// Resolve prefixed name to full IRI
// PERFORMANCE: O(1) - hash table lookup
cns_result_t cns_parser_resolve_prefix(const cns_parser_t *parser,
                                      const char *prefixed_name,
                                      cns_string_ref_t *resolved_iri);

// Set base IRI
// PERFORMANCE: O(1) - string interning
cns_result_t cns_parser_set_base(cns_parser_t *parser, const char *base_iri);

// Resolve relative IRI against base
// PERFORMANCE: O(1) - string concatenation and interning
cns_result_t cns_parser_resolve_relative(const cns_parser_t *parser,
                                        const char *relative_iri,
                                        cns_string_ref_t *absolute_iri);

// Get defined prefixes
// PERFORMANCE: O(1) - returns arrays
cns_result_t cns_parser_get_prefixes(const cns_parser_t *parser,
                                    cns_string_ref_t **prefixes,
                                    cns_string_ref_t **iris,
                                    size_t *count);

// ============================================================================
// AST CONSTRUCTION FUNCTIONS
// ============================================================================

// Create AST node
// PERFORMANCE: O(1) - arena allocation
cns_ast_node_t* cns_parser_create_ast_node(cns_parser_t *parser,
                                          cns_ast_node_type_t type,
                                          const cns_token_t *token);

// Add child to AST node
// PERFORMANCE: O(1) - linked list insertion
cns_result_t cns_parser_add_ast_child(cns_ast_node_t *parent, cns_ast_node_t *child);

// Get AST root node
// PERFORMANCE: O(1) - returns cached root
cns_ast_node_t* cns_parser_get_ast_root(const cns_parser_t *parser);

// Walk AST with visitor function
// PERFORMANCE: O(n) where n is number of nodes
cns_result_t cns_parser_walk_ast(const cns_ast_node_t *root,
                                cns_result_t (*visitor)(const cns_ast_node_t *, void *),
                                void *user_data);

// ============================================================================
// ERROR HANDLING AND REPORTING
// ============================================================================

// Add parse error
// PERFORMANCE: O(1) - adds to error array
cns_result_t cns_parser_add_error(cns_parser_t *parser,
                                 cns_result_t code,
                                 const char *message);

// Get parse errors
// PERFORMANCE: O(1) - returns error array
cns_result_t cns_parser_get_errors(const cns_parser_t *parser,
                                  const cns_parser_error_t **errors,
                                  size_t *count);

// Clear parse errors
// PERFORMANCE: O(1) - resets error count
cns_result_t cns_parser_clear_errors(cns_parser_t *parser);

// Format error message
// PERFORMANCE: O(1) - string formatting
cns_result_t cns_parser_format_error(const cns_parser_error_t *error,
                                    char *buffer,
                                    size_t buffer_size);

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

// Check if character is whitespace
// PERFORMANCE: O(1) - lookup table
static inline bool cns_parser_is_whitespace(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

// Check if character is valid for IRI
// PERFORMANCE: O(1) - character classification
bool cns_parser_is_iri_char(char c);

// Check if character is valid for local name
// PERFORMANCE: O(1) - character classification
bool cns_parser_is_localname_char(char c);

// Unescape string literal
// PERFORMANCE: O(n) where n is string length
cns_result_t cns_parser_unescape_string(cns_parser_t *parser,
                                       const char *input,
                                       size_t length,
                                       cns_string_ref_t *result);

// Validate IRI syntax
// PERFORMANCE: O(n) where n is IRI length
bool cns_parser_validate_iri(const char *iri);

// ============================================================================
// PARSER INFORMATION AND STATISTICS
// ============================================================================

// Get parser statistics
// PERFORMANCE: O(1) - returns cached statistics
cns_result_t cns_parser_get_stats(const cns_parser_t *parser, cns_parser_stats_t *stats);

// Get current parse position
// PERFORMANCE: O(1) - returns state information
cns_result_t cns_parser_get_position(const cns_parser_t *parser,
                                    uint32_t *line,
                                    uint32_t *column,
                                    const char **position);

// Get parser flags
// PERFORMANCE: O(1) - returns cached flags
uint32_t cns_parser_get_flags(const cns_parser_t *parser);

// Set parser flags
// PERFORMANCE: O(1) - updates flags
cns_result_t cns_parser_set_flags(cns_parser_t *parser, uint32_t flags);

// ============================================================================
// TOKEN UTILITY FUNCTIONS
// ============================================================================

// Convert token type to string
// PERFORMANCE: O(1) - lookup table
const char* cns_token_type_string(cns_token_type_t type);

// Check if token is literal
// PERFORMANCE: O(1) - simple comparison
static inline bool cns_token_is_literal(cns_token_type_t type) {
    return type >= CNS_TOKEN_LITERAL && type <= CNS_TOKEN_BOOLEAN;
}

// Check if token is term (subject/predicate/object)
// PERFORMANCE: O(1) - simple comparison
static inline bool cns_token_is_term(cns_token_type_t type) {
    return type == CNS_TOKEN_IRI || 
           type == CNS_TOKEN_PREFIXED_NAME ||
           type == CNS_TOKEN_BLANK_NODE ||
           cns_token_is_literal(type);
}

// Get token length
// PERFORMANCE: O(1) - pointer arithmetic
static inline size_t cns_token_length(const cns_token_t *token) {
    return token->end - token->start;
}

// ============================================================================
// STREAMING PARSER INTERFACE
// ============================================================================

// Streaming parser for large files
typedef struct cns_streaming_parser cns_streaming_parser_t;

// Create streaming parser
// PERFORMANCE: O(1) - initializes streaming state
cns_streaming_parser_t* cns_streaming_parser_create(cns_parser_t *parser,
                                                   size_t buffer_size);

// Feed data to streaming parser
// PERFORMANCE: O(n) where n is data size
cns_result_t cns_streaming_parser_feed(cns_streaming_parser_t *stream,
                                      const char *data,
                                      size_t length);

// Finish streaming parsing
// PERFORMANCE: O(1) - processes remaining buffer
cns_result_t cns_streaming_parser_finish(cns_streaming_parser_t *stream);

// Destroy streaming parser
// PERFORMANCE: O(1) - frees streaming state
void cns_streaming_parser_destroy(cns_streaming_parser_t *stream);

// ============================================================================
// DEBUG AND VALIDATION FUNCTIONS
// ============================================================================

// Validate parser state
// PERFORMANCE: O(1) - checks internal consistency
cns_result_t cns_parser_validate(const cns_parser_t *parser);

// Print parser statistics
// PERFORMANCE: O(1) - prints cached statistics
cns_result_t cns_parser_print_stats(const cns_parser_t *parser, FILE *output);

// Dump tokens from input
// PERFORMANCE: O(n) - tokenizes and prints all tokens
cns_result_t cns_parser_dump_tokens(cns_parser_t *parser, 
                                   const char *input, 
                                   FILE *output);

// Print AST tree
// PERFORMANCE: O(n) where n is number of nodes
cns_result_t cns_parser_print_ast(const cns_ast_node_t *root, FILE *output);

// ============================================================================
// PERFORMANCE MONITORING INTEGRATION
// ============================================================================

// Performance callback for parser events
typedef void (*cns_parser_perf_callback_t)(const cns_parser_t *parser,
                                          const char *operation,
                                          size_t tokens_processed,
                                          cns_tick_t ticks,
                                          void *user_data);

// Set performance monitoring callback
// PERFORMANCE: O(1) - stores callback pointer
cns_result_t cns_parser_set_perf_callback(cns_parser_t *parser,
                                         cns_parser_perf_callback_t callback,
                                         void *user_data);

// Clear performance monitoring callback
// PERFORMANCE: O(1) - clears callback pointer
cns_result_t cns_parser_clear_perf_callback(cns_parser_t *parser);

// ============================================================================
// THREAD SAFETY FUNCTIONS
// ============================================================================

// Enable thread safety for parser
// PERFORMANCE: O(1) - initializes mutex
cns_result_t cns_parser_enable_thread_safety(cns_parser_t *parser);

// Disable thread safety
// PERFORMANCE: O(1) - destroys mutex
cns_result_t cns_parser_disable_thread_safety(cns_parser_t *parser);

// Lock parser for exclusive access
// PERFORMANCE: O(1) - mutex lock
cns_result_t cns_parser_lock(cns_parser_t *parser);

// Unlock parser
// PERFORMANCE: O(1) - mutex unlock
cns_result_t cns_parser_unlock(cns_parser_t *parser);

// ============================================================================
// UTILITY MACROS FOR COMMON PATTERNS
// ============================================================================

// Parse and check for errors
#define CNS_PARSER_PARSE_CHECK(parser, input) \
    do { \
        cns_result_t result = cns_parser_parse_document((parser), (input)); \
        if (CNS_7T_UNLIKELY(result != CNS_OK)) return result; \
    } while(0)

// Get next token and check type
#define CNS_PARSER_EXPECT_TOKEN(parser, expected_type, token_var) \
    do { \
        cns_result_t result = cns_parser_next_token((parser), &(token_var)); \
        if (CNS_7T_UNLIKELY(result != CNS_OK)) return result; \
        if (CNS_7T_UNLIKELY((token_var).type != (expected_type))) \
            return CNS_ERROR_PARSER; \
    } while(0)

// Add error and return
#define CNS_PARSER_ERROR_RETURN(parser, code, message) \
    do { \
        cns_parser_add_error((parser), (code), (message)); \
        return (code); \
    } while(0)

#ifdef __cplusplus
}
#endif

#endif // CNS_PARSER_H