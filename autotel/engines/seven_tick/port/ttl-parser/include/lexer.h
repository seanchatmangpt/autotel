/**
 * @file lexer.h
 * @brief TTL/Turtle lexical analyzer interface
 * @author CNS Seven-Tick Engine
 * @date 2024
 * 
 * This file defines the lexer interface for tokenizing TTL (Turtle) input.
 * The lexer uses a state machine approach for efficient tokenization.
 */

#ifndef TTL_LEXER_H
#define TTL_LEXER_H

#include "token.h"
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct Lexer Lexer;
typedef struct LexerState LexerState;
typedef struct LexerError LexerError;

/**
 * Lexer error types
 */
typedef enum {
    LEXER_ERROR_UNTERMINATED_STRING,
    LEXER_ERROR_INVALID_IRI,
    LEXER_ERROR_INVALID_ESCAPE,
    LEXER_ERROR_UNEXPECTED_CHAR,
    LEXER_ERROR_INCOMPLETE_TOKEN,
    LEXER_ERROR_INVALID_NUMBER,
    LEXER_ERROR_INVALID_UNICODE
} LexerErrorType;

/**
 * Lexer error structure
 */
struct LexerError {
    LexerErrorType type;        /* Error type */
    char* message;               /* Error message */
    size_t line;                 /* Line where error occurred */
    size_t column;               /* Column where error occurred */
    size_t position;             /* Absolute position in input */
    char* context;               /* Surrounding text for context */
    char* suggestion;            /* Suggested fix (optional) */
    struct LexerError* next;     /* Next error in linked list */
};

/**
 * Lexer state for incremental parsing
 */
struct LexerState {
    size_t position;             /* Current position in input */
    size_t line;                 /* Current line number */
    size_t column;               /* Current column number */
    int state;                   /* Current state machine state */
    char* partial_token;         /* Partial token for chunked input */
    size_t partial_length;       /* Length of partial token */
};

/**
 * Lexer options/configuration
 */
typedef struct {
    bool track_trivia;           /* Track whitespace and comments */
    bool unicode_normalization;  /* Normalize Unicode (NFC) */
    bool strict_mode;            /* Strict W3C compliance */
    bool error_recovery;         /* Enable error recovery */
    size_t buffer_size;          /* Internal buffer size */
    size_t max_token_length;     /* Maximum token length */
} LexerOptions;

/**
 * Create a new lexer instance
 * @param options Lexer configuration (NULL for defaults)
 * @return New lexer instance (must be freed with lexer_destroy)
 */
Lexer* lexer_create(const LexerOptions* options);

/**
 * Destroy a lexer instance
 * @param lexer Lexer to destroy
 */
void lexer_destroy(Lexer* lexer);

/**
 * Initialize lexer with input string
 * @param lexer Lexer instance
 * @param input Input string (UTF-8 encoded)
 * @param length Length of input (0 for null-terminated)
 */
void lexer_init(Lexer* lexer, const char* input, size_t length);

/**
 * Initialize lexer with input buffer
 * @param lexer Lexer instance
 * @param buffer Input buffer
 * @param length Buffer length
 */
void lexer_init_buffer(Lexer* lexer, const uint8_t* buffer, size_t length);

/**
 * Get next token
 * @param lexer Lexer instance
 * @return Next token (caller owns, must free with token_free)
 *         Returns EOF token at end of input
 */
Token* lexer_next_token(Lexer* lexer);

/**
 * Peek at next token without consuming
 * @param lexer Lexer instance
 * @param lookahead How many tokens to look ahead (0 = next token)
 * @return Token at lookahead position (do not free, owned by lexer)
 */
const Token* lexer_peek_token(Lexer* lexer, size_t lookahead);

/**
 * Tokenize entire input
 * @param lexer Lexer instance
 * @param tokens Output array (will be allocated)
 * @param count Output token count
 * @return True on success, false on allocation failure
 */
bool lexer_tokenize_all(Lexer* lexer, Token*** tokens, size_t* count);

/**
 * Get lexer errors
 * @param lexer Lexer instance
 * @return Linked list of errors (NULL if no errors)
 */
const LexerError* lexer_get_errors(const Lexer* lexer);

/**
 * Get error count
 * @param lexer Lexer instance
 * @return Number of errors encountered
 */
size_t lexer_error_count(const Lexer* lexer);

/**
 * Clear errors
 * @param lexer Lexer instance
 */
void lexer_clear_errors(Lexer* lexer);

/**
 * Save lexer state for incremental parsing
 * @param lexer Lexer instance
 * @return Lexer state (must be freed with lexer_state_free)
 */
LexerState* lexer_save_state(const Lexer* lexer);

/**
 * Restore lexer state
 * @param lexer Lexer instance
 * @param state State to restore
 */
void lexer_restore_state(Lexer* lexer, const LexerState* state);

/**
 * Free lexer state
 * @param state State to free
 */
void lexer_state_free(LexerState* state);

/**
 * Feed additional input for streaming
 * @param lexer Lexer instance
 * @param input Additional input
 * @param length Input length
 * @return True if more input can be accepted
 */
bool lexer_feed(Lexer* lexer, const char* input, size_t length);

/**
 * Signal end of input for streaming
 * @param lexer Lexer instance
 */
void lexer_end_input(Lexer* lexer);

/**
 * Get current position info
 * @param lexer Lexer instance
 * @param line Output line number (optional)
 * @param column Output column number (optional)
 * @param position Output absolute position (optional)
 */
void lexer_get_position(const Lexer* lexer, size_t* line, size_t* column, size_t* position);

/**
 * Create default lexer options
 * @return Default options
 */
LexerOptions lexer_default_options(void);

/**
 * Utility: Validate IRI
 * @param iri IRI string
 * @param length IRI length
 * @return True if valid IRI
 */
bool lexer_validate_iri(const char* iri, size_t length);

/**
 * Utility: Unescape string
 * @param escaped Escaped string
 * @param length String length
 * @param output Output buffer (must be at least length bytes)
 * @param output_length Actual output length
 * @return True on success
 */
bool lexer_unescape_string(const char* escaped, size_t length, 
                          char* output, size_t* output_length);

/**
 * Utility: Check if character is valid PN_CHARS_BASE
 * @param ch Character to check
 * @return True if valid
 */
bool lexer_is_pn_chars_base(uint32_t ch);

/**
 * Utility: Check if character is valid PN_CHARS
 * @param ch Character to check
 * @return True if valid
 */
bool lexer_is_pn_chars(uint32_t ch);

#ifdef __cplusplus
}
#endif

#endif /* TTL_LEXER_H */