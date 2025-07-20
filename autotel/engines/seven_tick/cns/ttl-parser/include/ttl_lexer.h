/**
 * @file ttl_lexer.h
 * @brief TTL (Time-To-Live) Lexer - Header file
 * 
 * This lexer tokenizes TTL expressions with support for:
 * - Time units: ns, us, ms, s, m, h, d, w, mo, y
 * - Numeric values: integers and decimals
 * - Operators: +, -, *, /
 * - Whitespace handling
 * - Error reporting
 */

#ifndef TTL_LEXER_H
#define TTL_LEXER_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Token types */
typedef enum {
    /* Special tokens */
    TOKEN_EOF = 0,
    TOKEN_ERROR,
    
    /* Numeric tokens */
    TOKEN_INTEGER,
    TOKEN_DECIMAL,
    
    /* Time unit tokens */
    TOKEN_NANOSECOND,    /* ns */
    TOKEN_MICROSECOND,   /* us */
    TOKEN_MILLISECOND,   /* ms */
    TOKEN_SECOND,        /* s */
    TOKEN_MINUTE,        /* m */
    TOKEN_HOUR,          /* h */
    TOKEN_DAY,           /* d */
    TOKEN_WEEK,          /* w */
    TOKEN_MONTH,         /* mo */
    TOKEN_YEAR,          /* y */
    
    /* Operator tokens */
    TOKEN_PLUS,          /* + */
    TOKEN_MINUS,         /* - */
    TOKEN_MULTIPLY,      /* * */
    TOKEN_DIVIDE,        /* / */
    
    /* Grouping tokens */
    TOKEN_LPAREN,        /* ( */
    TOKEN_RPAREN,        /* ) */
    
    /* Other */
    TOKEN_WHITESPACE
} ttl_token_type_t;

/* Token structure */
typedef struct {
    ttl_token_type_t type;
    const char *start;      /* Pointer to token start in source */
    size_t length;          /* Token length */
    
    /* Token value (for numbers) */
    union {
        int64_t integer_value;
        double decimal_value;
    } value;
    
    /* Position information for error reporting */
    size_t line;
    size_t column;
} ttl_token_t;

/* Lexer state */
typedef struct {
    const char *source;     /* Source string */
    const char *current;    /* Current position */
    const char *start;      /* Start of current token */
    size_t line;           /* Current line number */
    size_t column;         /* Current column number */
    
    /* Error handling */
    bool has_error;
    char error_msg[256];
    
    /* Buffer management */
    size_t source_length;
    
    /* Performance optimization: lookahead cache */
    char lookahead[4];
    bool lookahead_valid;
} ttl_lexer_t;

/* Time unit conversion factors (to nanoseconds) */
typedef struct {
    const char *unit;
    uint64_t factor;
} ttl_unit_factor_t;

/* API Functions */

/**
 * Initialize a TTL lexer with source string
 * @param lexer Pointer to lexer structure
 * @param source Source string to tokenize
 * @param length Length of source string
 * @return true on success, false on error
 */
bool ttl_lexer_init(ttl_lexer_t *lexer, const char *source, size_t length);

/**
 * Get the next token from the lexer
 * @param lexer Pointer to lexer structure
 * @param token Pointer to token structure to fill
 * @return true on success, false on error
 */
bool ttl_lexer_next_token(ttl_lexer_t *lexer, ttl_token_t *token);

/**
 * Peek at the next token without consuming it
 * @param lexer Pointer to lexer structure
 * @param token Pointer to token structure to fill
 * @return true on success, false on error
 */
bool ttl_lexer_peek_token(ttl_lexer_t *lexer, ttl_token_t *token);

/**
 * Reset lexer to beginning of source
 * @param lexer Pointer to lexer structure
 */
void ttl_lexer_reset(ttl_lexer_t *lexer);

/**
 * Clean up lexer resources
 * @param lexer Pointer to lexer structure
 */
void ttl_lexer_cleanup(ttl_lexer_t *lexer);

/**
 * Get human-readable name for token type
 * @param type Token type
 * @return String representation of token type
 */
const char *ttl_token_type_name(ttl_token_type_t type);

/**
 * Convert time value to nanoseconds
 * @param value Numeric value
 * @param unit Unit token type
 * @return Value in nanoseconds, or 0 on error
 */
uint64_t ttl_convert_to_nanoseconds(double value, ttl_token_type_t unit);

/**
 * Get error message from lexer
 * @param lexer Pointer to lexer structure
 * @return Error message string or NULL if no error
 */
const char *ttl_lexer_get_error(const ttl_lexer_t *lexer);

#ifdef __cplusplus
}
#endif

#endif /* TTL_LEXER_H */