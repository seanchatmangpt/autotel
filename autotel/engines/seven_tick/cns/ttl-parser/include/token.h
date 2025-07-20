/**
 * @file token.h
 * @brief Token structure and types for TTL lexer
 * @author CNS Seven-Tick Engine
 * @date 2024
 * 
 * This file defines the token types and structures used by the TTL lexer.
 * Based on the W3C Turtle specification.
 */

#ifndef TTL_TOKEN_H
#define TTL_TOKEN_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Token types enumeration
 * Covers all TTL/Turtle token types including IRIs, literals, keywords, and punctuation
 */
typedef enum {
    /* IRI tokens */
    TOKEN_IRI_REF,                      /* <http://example.org/resource> */
    TOKEN_PREFIXED_NAME,                /* foaf:Person, :hasName */
    
    /* Literal tokens */
    TOKEN_STRING_LITERAL_QUOTE,         /* "Hello World" */
    TOKEN_STRING_LITERAL_SINGLE_QUOTE,  /* 'Hello World' */
    TOKEN_STRING_LITERAL_LONG_QUOTE,    /* """Multi-line""" */
    TOKEN_STRING_LITERAL_LONG_SINGLE_QUOTE, /* '''Multi-line''' */
    TOKEN_INTEGER,                      /* 42, -123, +999 */
    TOKEN_DECIMAL,                      /* 3.14, -0.5 */
    TOKEN_DOUBLE,                       /* 1.23E4, -3.14e-10 */
    TOKEN_BOOLEAN,                      /* true, false */
    
    /* Node tokens */
    TOKEN_BLANK_NODE_LABEL,             /* _:node1, _:b123 */
    TOKEN_ANON,                         /* [] */
    
    /* Keywords and directives */
    TOKEN_PREFIX,                       /* @prefix */
    TOKEN_BASE,                         /* @base */
    TOKEN_A,                            /* a (rdf:type abbreviation) */
    
    /* Punctuation and delimiters */
    TOKEN_DOT,                          /* . */
    TOKEN_SEMICOLON,                    /* ; */
    TOKEN_COMMA,                        /* , */
    TOKEN_OPEN_PAREN,                   /* ( */
    TOKEN_CLOSE_PAREN,                  /* ) */
    TOKEN_OPEN_BRACKET,                 /* [ */
    TOKEN_CLOSE_BRACKET,                /* ] */
    TOKEN_OPEN_BRACE,                   /* { */
    TOKEN_CLOSE_BRACE,                  /* } */
    TOKEN_DOUBLE_CARET,                 /* ^^ */
    TOKEN_AT,                           /* @ */
    
    /* Special tokens */
    TOKEN_COMMENT,                      /* # This is a comment */
    TOKEN_WHITESPACE,                   /* Spaces, tabs, newlines */
    TOKEN_EOF,                          /* End of file */
    
    /* Error token */
    TOKEN_INVALID                       /* Invalid/error token */
} TokenType;

/* Type alias for consistency with other headers */
typedef TokenType ttl_token_type_t;

/**
 * Trivia information (whitespace and comments)
 */
typedef struct {
    char* leading_whitespace;    /* Whitespace before token */
    char* trailing_whitespace;   /* Whitespace after token */
    char** comments;             /* Array of comment strings */
    size_t comment_count;        /* Number of comments */
} TokenTrivia;

/**
 * Token structure
 * Represents a single lexical token with position information
 */
typedef struct {
    TokenType type;              /* Token type */
    char* value;                 /* Raw token value (null-terminated) */
    size_t length;               /* Length of value */
    
    /* Position information */
    size_t line;                 /* Line number (1-based) */
    size_t column;               /* Column number (1-based) */
    size_t start_pos;            /* Absolute start position in input */
    size_t end_pos;              /* Absolute end position in input */
    
    /* Optional trivia */
    TokenTrivia* trivia;         /* Associated whitespace/comments (may be NULL) */
    
    /* Error information */
    bool has_error;              /* True if token represents an error */
    char* error_message;         /* Error description (NULL if no error) */
} Token;

/**
 * Create a new token
 * @param type Token type
 * @param value Token value (will be copied)
 * @param length Length of value
 * @return Newly allocated token (must be freed with token_free)
 */
Token* token_create(TokenType type, const char* value, size_t length);

/**
 * Create an error token
 * @param value Partial token value
 * @param length Length of value
 * @param error_message Error description
 * @return Newly allocated error token
 */
Token* token_create_error(const char* value, size_t length, const char* error_message);

/**
 * Free a token and its resources
 * @param token Token to free
 */
void token_free(Token* token);

/**
 * Duplicate a token
 * @param token Token to duplicate
 * @return New token with copied data
 */
Token* token_duplicate(const Token* token);

/**
 * Get string representation of token type
 * @param type Token type
 * @return String name of token type
 */
const char* token_type_to_string(TokenType type);

/**
 * Check if token is a literal type
 * @param type Token type
 * @return True if token is a literal
 */
bool token_is_literal(TokenType type);

/**
 * Check if token is a keyword
 * @param type Token type
 * @return True if token is a keyword
 */
bool token_is_keyword(TokenType type);

/**
 * Check if token is punctuation
 * @param type Token type
 * @return True if token is punctuation
 */
bool token_is_punctuation(TokenType type);

/**
 * Create trivia structure
 * @return Newly allocated trivia (must be freed with token_trivia_free)
 */
TokenTrivia* token_trivia_create(void);

/**
 * Free trivia structure
 * @param trivia Trivia to free
 */
void token_trivia_free(TokenTrivia* trivia);

/**
 * Add comment to trivia
 * @param trivia Trivia structure
 * @param comment Comment string (will be copied)
 */
void token_trivia_add_comment(TokenTrivia* trivia, const char* comment);

#ifdef __cplusplus
}
#endif

#endif /* TTL_TOKEN_H */