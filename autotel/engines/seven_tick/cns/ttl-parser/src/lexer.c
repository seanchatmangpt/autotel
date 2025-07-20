/**
 * @file lexer.c
 * @brief TTL/Turtle lexical analyzer implementation
 * @author CNS Seven-Tick Engine
 * @date 2024
 */

#include "lexer.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>

/* Internal state machine states */
typedef enum {
    STATE_INITIAL,
    STATE_IN_IRI,
    STATE_IN_STRING_DQ,
    STATE_IN_STRING_SQ,
    STATE_IN_STRING_LONG_DQ,
    STATE_IN_STRING_LONG_SQ,
    STATE_IN_NUMBER,
    STATE_IN_PREFIXED_NAME,
    STATE_IN_BLANK_NODE,
    STATE_IN_COMMENT,
    STATE_IN_DIRECTIVE,
    STATE_IN_LANGUAGE_TAG
} LexerInternalState;

/* Character classification lookup tables for performance */
static uint8_t CHAR_CLASSES[256];
static bool LOOKUP_TABLES_INITIALIZED = false;

/* Character class flags */
#define CHAR_WHITESPACE  0x01
#define CHAR_DIGIT       0x02
#define CHAR_ALPHA       0x04
#define CHAR_PN_CHARS    0x08
#define CHAR_IRI_SAFE    0x10
#define CHAR_HEX         0x20

/* Lexer structure */
struct Lexer {
    /* Input buffer */
    const char* input;
    size_t input_length;
    size_t position;
    
    /* Position tracking */
    size_t line;
    size_t column;
    size_t token_start_line;
    size_t token_start_column;
    size_t token_start_pos;
    
    /* State machine */
    LexerInternalState state;
    
    /* Token building */
    char* buffer;
    size_t buffer_size;
    size_t buffer_pos;
    
    /* Lookahead buffer */
    Token** lookahead;
    size_t lookahead_size;
    size_t lookahead_pos;
    size_t lookahead_count;
    
    /* Error handling */
    LexerError* errors;
    LexerError* last_error;
    size_t error_count;
    
    /* Options */
    LexerOptions options;
    
    /* Streaming support */
    bool streaming;
    bool input_ended;
    char* partial_token;
    size_t partial_length;
};

/* Forward declarations */
static void init_lookup_tables(void);
static bool is_whitespace(char ch);
static bool is_digit(char ch);
static bool is_alpha(char ch);
static bool is_hex_digit(char ch);
static bool is_iri_char(char ch);
static bool is_pn_chars_start(char ch);
static bool is_pn_chars_cont(char ch);
static Token* make_token(Lexer* lexer, TokenType type);
static Token* make_error_token(Lexer* lexer, const char* message);
static void add_error(Lexer* lexer, LexerErrorType type, const char* message);
static void buffer_append(Lexer* lexer, char ch);
static void buffer_clear(Lexer* lexer);
static char peek_char(Lexer* lexer, size_t offset);
static char next_char(Lexer* lexer);
static void skip_whitespace(Lexer* lexer);
static Token* scan_iri(Lexer* lexer);
static Token* scan_string(Lexer* lexer, char quote);
static Token* scan_long_string(Lexer* lexer, char quote);
static Token* scan_number(Lexer* lexer);
static Token* scan_prefixed_name(Lexer* lexer);
static Token* scan_blank_node(Lexer* lexer);
static Token* scan_directive(Lexer* lexer);
static Token* scan_comment(Lexer* lexer);
static Token* scan_punctuation(Lexer* lexer);
static Token* scan_keyword_or_boolean(Lexer* lexer);

/* Initialize character lookup tables */
static void init_lookup_tables(void) {
    if (LOOKUP_TABLES_INITIALIZED) return;
    
    memset(CHAR_CLASSES, 0, sizeof(CHAR_CLASSES));
    
    /* Whitespace */
    CHAR_CLASSES[' '] |= CHAR_WHITESPACE;
    CHAR_CLASSES['\t'] |= CHAR_WHITESPACE;
    CHAR_CLASSES['\r'] |= CHAR_WHITESPACE;
    CHAR_CLASSES['\n'] |= CHAR_WHITESPACE;
    
    /* Digits */
    for (int i = '0'; i <= '9'; i++) {
        CHAR_CLASSES[i] |= CHAR_DIGIT | CHAR_HEX;
    }
    
    /* Letters */
    for (int i = 'A'; i <= 'Z'; i++) {
        CHAR_CLASSES[i] |= CHAR_ALPHA | CHAR_PN_CHARS;
        if (i <= 'F') CHAR_CLASSES[i] |= CHAR_HEX;
    }
    for (int i = 'a'; i <= 'z'; i++) {
        CHAR_CLASSES[i] |= CHAR_ALPHA | CHAR_PN_CHARS;
        if (i <= 'f') CHAR_CLASSES[i] |= CHAR_HEX;
    }
    
    /* Additional PN_CHARS */
    CHAR_CLASSES['-'] |= CHAR_PN_CHARS;
    CHAR_CLASSES['_'] |= CHAR_PN_CHARS;
    
    /* IRI safe characters */
    const char* iri_safe = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._~:/?#[]@!$&'()*+,;=";
    for (const char* p = iri_safe; *p; p++) {
        CHAR_CLASSES[(unsigned char)*p] |= CHAR_IRI_SAFE;
    }
    
    LOOKUP_TABLES_INITIALIZED = true;
}

/* Character classification helpers */
static inline bool is_whitespace(char ch) {
    return (CHAR_CLASSES[(unsigned char)ch] & CHAR_WHITESPACE) != 0;
}

static inline bool is_digit(char ch) {
    return (CHAR_CLASSES[(unsigned char)ch] & CHAR_DIGIT) != 0;
}

static inline bool is_alpha(char ch) {
    return (CHAR_CLASSES[(unsigned char)ch] & CHAR_ALPHA) != 0;
}

static inline bool is_hex_digit(char ch) {
    return (CHAR_CLASSES[(unsigned char)ch] & CHAR_HEX) != 0;
}

__attribute__((unused))
static inline bool is_iri_char(char ch) {
    return (CHAR_CLASSES[(unsigned char)ch] & CHAR_IRI_SAFE) != 0;
}

static inline bool is_pn_chars_start(char ch) {
    return is_alpha(ch) || ch == '_';
}

static inline bool is_pn_chars_cont(char ch) {
    return (CHAR_CLASSES[(unsigned char)ch] & CHAR_PN_CHARS) != 0 || is_digit(ch);
}

/* Lexer creation and destruction */
Lexer* lexer_create(const LexerOptions* options) {
    init_lookup_tables();
    
    Lexer* lexer = (Lexer*)calloc(1, sizeof(Lexer));
    if (!lexer) return NULL;
    
    /* Set options */
    if (options) {
        lexer->options = *options;
    } else {
        lexer->options = lexer_default_options();
    }
    
    /* Allocate buffers */
    lexer->buffer_size = lexer->options.buffer_size;
    lexer->buffer = (char*)malloc(lexer->buffer_size);
    if (!lexer->buffer) {
        free(lexer);
        return NULL;
    }
    
    /* Allocate lookahead buffer */
    lexer->lookahead_size = 4;
    lexer->lookahead = (Token**)calloc(lexer->lookahead_size, sizeof(Token*));
    if (!lexer->lookahead) {
        free(lexer->buffer);
        free(lexer);
        return NULL;
    }
    
    /* Initialize state */
    lexer->state = STATE_INITIAL;
    lexer->line = 1;
    lexer->column = 1;
    
    return lexer;
}

void lexer_destroy(Lexer* lexer) {
    if (!lexer) return;
    
    /* Free buffers */
    if (lexer->buffer) free(lexer->buffer);
    if (lexer->partial_token) free(lexer->partial_token);
    
    /* Free lookahead tokens */
    if (lexer->lookahead) {
        for (size_t i = 0; i < lexer->lookahead_size; i++) {
            if (lexer->lookahead[i]) {
                token_free(lexer->lookahead[i]);
            }
        }
        free(lexer->lookahead);
    }
    
    /* Free errors */
    LexerError* error = lexer->errors;
    while (error) {
        LexerError* next = error->next;
        if (error->message) free(error->message);
        if (error->context) free(error->context);
        if (error->suggestion) free(error->suggestion);
        free(error);
        error = next;
    }
    
    free(lexer);
}

/* Initialize lexer with input */
void lexer_init(Lexer* lexer, const char* input, size_t length) {
    assert(lexer != NULL);
    assert(input != NULL);
    
    lexer->input = input;
    lexer->input_length = length ? length : strlen(input);
    lexer->position = 0;
    lexer->line = 1;
    lexer->column = 1;
    lexer->state = STATE_INITIAL;
    lexer->streaming = false;
    lexer->input_ended = true;
    
    /* Clear buffers */
    buffer_clear(lexer);
    lexer->lookahead_pos = 0;
    lexer->lookahead_count = 0;
    
    /* Clear errors */
    lexer_clear_errors(lexer);
}

void lexer_init_buffer(Lexer* lexer, const uint8_t* buffer, size_t length) {
    lexer_init(lexer, (const char*)buffer, length);
}

/* Buffer management */
static void buffer_append(Lexer* lexer, char ch) {
    if (lexer->buffer_pos >= lexer->buffer_size - 1) {
        /* Resize buffer */
        size_t new_size = lexer->buffer_size * 2;
        char* new_buffer = (char*)realloc(lexer->buffer, new_size);
        if (!new_buffer) return; /* TODO: Handle allocation failure */
        lexer->buffer = new_buffer;
        lexer->buffer_size = new_size;
    }
    lexer->buffer[lexer->buffer_pos++] = ch;
}

static void buffer_clear(Lexer* lexer) {
    lexer->buffer_pos = 0;
}

/* Input handling */
static char peek_char(Lexer* lexer, size_t offset) {
    size_t pos = lexer->position + offset;
    if (pos >= lexer->input_length) return '\0';
    return lexer->input[pos];
}

static char next_char(Lexer* lexer) {
    if (lexer->position >= lexer->input_length) return '\0';
    
    char ch = lexer->input[lexer->position++];
    
    /* Update line/column */
    if (ch == '\n') {
        lexer->line++;
        lexer->column = 1;
    } else if (ch != '\r') {
        lexer->column++;
    }
    
    return ch;
}

/* Skip whitespace */
static void skip_whitespace(Lexer* lexer) {
    while (lexer->position < lexer->input_length) {
        char ch = peek_char(lexer, 0);
        if (!is_whitespace(ch)) break;
        next_char(lexer);
    }
}

/* Create token */
static Token* make_token(Lexer* lexer, TokenType type) {
    lexer->buffer[lexer->buffer_pos] = '\0';
    Token* token = token_create(type, lexer->buffer, lexer->buffer_pos);
    if (!token) return NULL;
    
    token->line = lexer->token_start_line;
    token->column = lexer->token_start_column;
    token->start_pos = lexer->token_start_pos;
    token->end_pos = lexer->position;
    
    buffer_clear(lexer);
    return token;
}

/* Create error token */
static Token* make_error_token(Lexer* lexer, const char* message) {
    lexer->buffer[lexer->buffer_pos] = '\0';
    Token* token = token_create_error(lexer->buffer, lexer->buffer_pos, message);
    if (!token) return NULL;
    
    token->line = lexer->token_start_line;
    token->column = lexer->token_start_column;
    token->start_pos = lexer->token_start_pos;
    token->end_pos = lexer->position;
    
    buffer_clear(lexer);
    return token;
}

/* Add error */
static void add_error(Lexer* lexer, LexerErrorType type, const char* message) {
    if (!lexer->options.error_recovery) return;
    
    LexerError* error = (LexerError*)calloc(1, sizeof(LexerError));
    if (!error) return;
    
    error->type = type;
    error->line = lexer->line;
    error->column = lexer->column;
    error->position = lexer->position;
    
    if (message) {
        error->message = strdup(message);
    }
    
    /* Add to error list */
    if (lexer->last_error) {
        lexer->last_error->next = error;
    } else {
        lexer->errors = error;
    }
    lexer->last_error = error;
    lexer->error_count++;
}

/* Scan IRI */
static Token* scan_iri(Lexer* lexer) {
    /* Skip opening < */
    next_char(lexer);
    
    while (lexer->position < lexer->input_length) {
        char ch = peek_char(lexer, 0);
        
        if (ch == '>') {
            next_char(lexer);
            return make_token(lexer, TOKEN_IRI_REF);
        }
        
        if (ch == '\0' || ch == '\n' || ch == '\r') {
            add_error(lexer, LEXER_ERROR_UNTERMINATED_STRING, "Unterminated IRI");
            return make_error_token(lexer, "Unterminated IRI");
        }
        
        if (ch == '\\') {
            /* Handle escape */
            next_char(lexer);
            char esc = next_char(lexer);
            if (esc == 'u' || esc == 'U') {
                /* Unicode escape */
                int digits = (esc == 'u') ? 4 : 8;
                for (int i = 0; i < digits; i++) {
                    char hex = peek_char(lexer, 0);
                    if (!is_hex_digit(hex)) {
                        add_error(lexer, LEXER_ERROR_INVALID_ESCAPE, "Invalid Unicode escape in IRI");
                        break;
                    }
                    buffer_append(lexer, next_char(lexer));
                }
            } else {
                buffer_append(lexer, esc);
            }
        } else {
            buffer_append(lexer, next_char(lexer));
        }
    }
    
    add_error(lexer, LEXER_ERROR_INCOMPLETE_TOKEN, "Unexpected end of input in IRI");
    return make_error_token(lexer, "Incomplete IRI");
}

/* Scan string literal */
static Token* scan_string(Lexer* lexer, char quote) {
    /* Check for long string */
    if (peek_char(lexer, 0) == quote && peek_char(lexer, 1) == quote) {
        next_char(lexer); /* Second quote */
        next_char(lexer); /* Third quote */
        return scan_long_string(lexer, quote);
    }
    
    TokenType type = (quote == '"') ? TOKEN_STRING_LITERAL_QUOTE : TOKEN_STRING_LITERAL_SINGLE_QUOTE;
    
    while (lexer->position < lexer->input_length) {
        char ch = peek_char(lexer, 0);
        
        if (ch == quote) {
            next_char(lexer);
            return make_token(lexer, type);
        }
        
        if (ch == '\0' || ch == '\n' || ch == '\r') {
            add_error(lexer, LEXER_ERROR_UNTERMINATED_STRING, "Unterminated string literal");
            return make_error_token(lexer, "Unterminated string");
        }
        
        if (ch == '\\') {
            /* Handle escape */
            next_char(lexer);
            char esc = next_char(lexer);
            switch (esc) {
                case 't': buffer_append(lexer, '\t'); break;
                case 'n': buffer_append(lexer, '\n'); break;
                case 'r': buffer_append(lexer, '\r'); break;
                case 'b': buffer_append(lexer, '\b'); break;
                case 'f': buffer_append(lexer, '\f'); break;
                case '"': buffer_append(lexer, '"'); break;
                case '\'': buffer_append(lexer, '\''); break;
                case '\\': buffer_append(lexer, '\\'); break;
                case 'u':
                case 'U': {
                    /* Unicode escape */
                    int digits = (esc == 'u') ? 4 : 8;
                    for (int i = 0; i < digits; i++) {
                        char hex = peek_char(lexer, 0);
                        if (!is_hex_digit(hex)) {
                            add_error(lexer, LEXER_ERROR_INVALID_ESCAPE, "Invalid Unicode escape");
                            break;
                        }
                        buffer_append(lexer, next_char(lexer));
                    }
                    break;
                }
                default:
                    add_error(lexer, LEXER_ERROR_INVALID_ESCAPE, "Invalid escape sequence");
                    buffer_append(lexer, esc);
            }
        } else {
            buffer_append(lexer, next_char(lexer));
        }
    }
    
    add_error(lexer, LEXER_ERROR_INCOMPLETE_TOKEN, "Unexpected end of input in string");
    return make_error_token(lexer, "Incomplete string");
}

/* Scan long string */
static Token* scan_long_string(Lexer* lexer, char quote) {
    TokenType type = (quote == '"') ? TOKEN_STRING_LITERAL_LONG_QUOTE : TOKEN_STRING_LITERAL_LONG_SINGLE_QUOTE;
    
    while (lexer->position < lexer->input_length) {
        char ch = peek_char(lexer, 0);
        
        if (ch == quote && peek_char(lexer, 1) == quote && peek_char(lexer, 2) == quote) {
            next_char(lexer); /* First quote */
            next_char(lexer); /* Second quote */
            next_char(lexer); /* Third quote */
            return make_token(lexer, type);
        }
        
        if (ch == '\\') {
            /* Handle escape (same as regular string) */
            next_char(lexer);
            char esc = next_char(lexer);
            switch (esc) {
                case 't': buffer_append(lexer, '\t'); break;
                case 'n': buffer_append(lexer, '\n'); break;
                case 'r': buffer_append(lexer, '\r'); break;
                case 'b': buffer_append(lexer, '\b'); break;
                case 'f': buffer_append(lexer, '\f'); break;
                case '"': buffer_append(lexer, '"'); break;
                case '\'': buffer_append(lexer, '\''); break;
                case '\\': buffer_append(lexer, '\\'); break;
                case 'u':
                case 'U': {
                    /* Unicode escape */
                    int digits = (esc == 'u') ? 4 : 8;
                    for (int i = 0; i < digits; i++) {
                        char hex = peek_char(lexer, 0);
                        if (!is_hex_digit(hex)) {
                            add_error(lexer, LEXER_ERROR_INVALID_ESCAPE, "Invalid Unicode escape");
                            break;
                        }
                        buffer_append(lexer, next_char(lexer));
                    }
                    break;
                }
                default:
                    buffer_append(lexer, esc);
            }
        } else {
            buffer_append(lexer, next_char(lexer));
        }
    }
    
    add_error(lexer, LEXER_ERROR_INCOMPLETE_TOKEN, "Unexpected end of input in long string");
    return make_error_token(lexer, "Incomplete long string");
}

/* Scan number */
static Token* scan_number(Lexer* lexer) {
    // bool has_sign = false; // Unused - keeping for future numeric validation
    bool has_decimal = false;
    bool has_exponent = false;
    
    /* Optional sign */
    char ch = peek_char(lexer, 0);
    if (ch == '+' || ch == '-') {
        buffer_append(lexer, next_char(lexer));
        // has_sign = true;
    }
    
    /* Integer part */
    if (!is_digit(peek_char(lexer, 0))) {
        add_error(lexer, LEXER_ERROR_INVALID_NUMBER, "Invalid number format");
        return make_error_token(lexer, "Invalid number");
    }
    
    while (is_digit(peek_char(lexer, 0))) {
        buffer_append(lexer, next_char(lexer));
    }
    
    /* Decimal part */
    if (peek_char(lexer, 0) == '.') {
        has_decimal = true;
        buffer_append(lexer, next_char(lexer));
        
        if (!is_digit(peek_char(lexer, 0))) {
            add_error(lexer, LEXER_ERROR_INVALID_NUMBER, "Invalid decimal number");
            return make_error_token(lexer, "Invalid decimal");
        }
        
        while (is_digit(peek_char(lexer, 0))) {
            buffer_append(lexer, next_char(lexer));
        }
    }
    
    /* Exponent part */
    ch = peek_char(lexer, 0);
    if (ch == 'e' || ch == 'E') {
        has_exponent = true;
        buffer_append(lexer, next_char(lexer));
        
        ch = peek_char(lexer, 0);
        if (ch == '+' || ch == '-') {
            buffer_append(lexer, next_char(lexer));
        }
        
        if (!is_digit(peek_char(lexer, 0))) {
            add_error(lexer, LEXER_ERROR_INVALID_NUMBER, "Invalid exponent");
            return make_error_token(lexer, "Invalid exponent");
        }
        
        while (is_digit(peek_char(lexer, 0))) {
            buffer_append(lexer, next_char(lexer));
        }
    }
    
    /* Determine token type */
    TokenType type;
    if (has_exponent) {
        type = TOKEN_DOUBLE;
    } else if (has_decimal) {
        type = TOKEN_DECIMAL;
    } else {
        type = TOKEN_INTEGER;
    }
    
    return make_token(lexer, type);
}

/* Scan prefixed name */
__attribute__((unused))
static Token* scan_prefixed_name(Lexer* lexer) {
    /* Scan prefix part (optional) */
    while (lexer->position < lexer->input_length) {
        char ch = peek_char(lexer, 0);
        
        if (ch == ':') {
            buffer_append(lexer, next_char(lexer));
            break;
        }
        
        if (!is_pn_chars_cont(ch)) {
            /* Not a valid prefixed name */
            return NULL;
        }
        
        buffer_append(lexer, next_char(lexer));
    }
    
    /* Scan local part (optional) */
    while (is_pn_chars_cont(peek_char(lexer, 0))) {
        buffer_append(lexer, next_char(lexer));
    }
    
    return make_token(lexer, TOKEN_PREFIXED_NAME);
}

/* Scan blank node */
static Token* scan_blank_node(Lexer* lexer) {
    /* Skip _: */
    next_char(lexer); /* _ */
    next_char(lexer); /* : */
    
    if (!is_pn_chars_start(peek_char(lexer, 0))) {
        add_error(lexer, LEXER_ERROR_INVALID_IRI, "Invalid blank node label");
        return make_error_token(lexer, "Invalid blank node");
    }
    
    while (is_pn_chars_cont(peek_char(lexer, 0))) {
        buffer_append(lexer, next_char(lexer));
    }
    
    return make_token(lexer, TOKEN_BLANK_NODE_LABEL);
}

/* Scan directive */
static Token* scan_directive(Lexer* lexer) {
    /* CORE FIX: Reset buffer position to ensure clean scan */
    lexer->buffer_pos = 0;
    
    /* Skip @ and record it */
    buffer_append(lexer, next_char(lexer));
    
    /* Read directive name */
    while (is_alpha(peek_char(lexer, 0))) {
        buffer_append(lexer, next_char(lexer));
    }
    
    /* Null terminate buffer */
    lexer->buffer[lexer->buffer_pos] = '\0';
    
    /* CORE FIX: Include @ in comparison */
    if (strcmp(lexer->buffer, "@prefix") == 0) {
        return make_token(lexer, TOKEN_PREFIX);
    } else if (strcmp(lexer->buffer, "@base") == 0) {
        return make_token(lexer, TOKEN_BASE);
    } else {
        /* CORE FIX: If not a known directive, just return @ as TOKEN_AT */
        /* Reset position to just after @ */
        lexer->position = lexer->token_start_pos + 1;
        lexer->buffer_pos = 1;
        lexer->buffer[0] = '@';
        lexer->buffer[1] = '\0';
        return make_token(lexer, TOKEN_AT);
    }
}

/* Scan comment */
static Token* scan_comment(Lexer* lexer) {
    /* Skip # */
    next_char(lexer);
    
    /* Read until end of line */
    while (lexer->position < lexer->input_length) {
        char ch = peek_char(lexer, 0);
        if (ch == '\n' || ch == '\r') break;
        buffer_append(lexer, next_char(lexer));
    }
    
    return make_token(lexer, TOKEN_COMMENT);
}

/* Scan keyword or boolean */
static Token* scan_keyword_or_boolean(Lexer* lexer) {
    while (is_alpha(peek_char(lexer, 0))) {
        buffer_append(lexer, next_char(lexer));
    }
    
    lexer->buffer[lexer->buffer_pos] = '\0';
    
    if (strcmp(lexer->buffer, "a") == 0) {
        return make_token(lexer, TOKEN_A);
    } else if (strcmp(lexer->buffer, "true") == 0 || strcmp(lexer->buffer, "false") == 0) {
        return make_token(lexer, TOKEN_BOOLEAN);
    } else {
        /* Check if it's a prefixed name */
        if (peek_char(lexer, 0) == ':') {
            buffer_append(lexer, next_char(lexer));
            while (is_pn_chars_cont(peek_char(lexer, 0))) {
                buffer_append(lexer, next_char(lexer));
            }
            return make_token(lexer, TOKEN_PREFIXED_NAME);
        }
        
        /* Otherwise it's an error */
        add_error(lexer, LEXER_ERROR_UNEXPECTED_CHAR, "Unexpected identifier");
        return make_error_token(lexer, "Unexpected identifier");
    }
}

/* Scan punctuation */
static Token* scan_punctuation(Lexer* lexer) {
    char ch = next_char(lexer);
    buffer_append(lexer, ch);
    
    switch (ch) {
        case '.': return make_token(lexer, TOKEN_DOT);
        case ';': return make_token(lexer, TOKEN_SEMICOLON);
        case ',': return make_token(lexer, TOKEN_COMMA);
        case '(': return make_token(lexer, TOKEN_OPEN_PAREN);
        case ')': return make_token(lexer, TOKEN_CLOSE_PAREN);
        case '[': {
            /* Check for anonymous blank node */
            if (peek_char(lexer, 0) == ']') {
                next_char(lexer);
                buffer_clear(lexer);
                return make_token(lexer, TOKEN_ANON);
            }
            return make_token(lexer, TOKEN_OPEN_BRACKET);
        }
        case ']': return make_token(lexer, TOKEN_CLOSE_BRACKET);
        case '{': return make_token(lexer, TOKEN_OPEN_BRACE);
        case '}': return make_token(lexer, TOKEN_CLOSE_BRACE);
        case '^': {
            if (peek_char(lexer, 0) == '^') {
                buffer_append(lexer, next_char(lexer));
                return make_token(lexer, TOKEN_DOUBLE_CARET);
            }
            break;
        }
        case '@': return make_token(lexer, TOKEN_AT);
    }
    
    add_error(lexer, LEXER_ERROR_UNEXPECTED_CHAR, "Unexpected character");
    return make_error_token(lexer, "Unexpected character");
}

/* Main tokenization function */
Token* lexer_next_token(Lexer* lexer) {
    assert(lexer != NULL);
    
    /* Check lookahead buffer first */
    if (lexer->lookahead_count > 0) {
        Token* token = lexer->lookahead[lexer->lookahead_pos];
        lexer->lookahead[lexer->lookahead_pos] = NULL;
        lexer->lookahead_pos = (lexer->lookahead_pos + 1) % lexer->lookahead_size;
        lexer->lookahead_count--;
        return token;
    }
    
    /* Skip whitespace if not tracking trivia */
    if (!lexer->options.track_trivia) {
        skip_whitespace(lexer);
    }
    
    /* Check for EOF */
    if (lexer->position >= lexer->input_length) {
        return token_create(TOKEN_EOF, NULL, 0);
    }
    
    /* Record token start position */
    lexer->token_start_pos = lexer->position;
    lexer->token_start_line = lexer->line;
    lexer->token_start_column = lexer->column;
    
    /* Get first character */
    char ch = peek_char(lexer, 0);
    
    /* Handle different token types */
    switch (ch) {
        case '<':
            return scan_iri(lexer);
            
        case '"':
            next_char(lexer);
            return scan_string(lexer, '"');
            
        case '\'':
            next_char(lexer);
            return scan_string(lexer, '\'');
            
        case '#':
            return scan_comment(lexer);
            
        case '@':
            return scan_directive(lexer);
            
        case '_':
            if (peek_char(lexer, 1) == ':') {
                return scan_blank_node(lexer);
            }
            break;
            
        case ':':
            /* Prefixed name with empty prefix */
            buffer_append(lexer, next_char(lexer));
            while (is_pn_chars_cont(peek_char(lexer, 0))) {
                buffer_append(lexer, next_char(lexer));
            }
            return make_token(lexer, TOKEN_PREFIXED_NAME);
            
        case '+':
        case '-':
            /* Could be number or just punctuation */
            if (is_digit(peek_char(lexer, 1))) {
                return scan_number(lexer);
            }
            break;
            
        case '.':
        case ';':
        case ',':
        case '(':
        case ')':
        case '[':
        case ']':
        case '{':
        case '}':
        case '^':
            return scan_punctuation(lexer);
            
        default:
            if (is_digit(ch)) {
                return scan_number(lexer);
            } else if (is_alpha(ch)) {
                return scan_keyword_or_boolean(lexer);
            }
            break;
    }
    
    /* Unexpected character */
    buffer_append(lexer, next_char(lexer));
    add_error(lexer, LEXER_ERROR_UNEXPECTED_CHAR, "Unexpected character");
    return make_error_token(lexer, "Unexpected character");
}

/* Peek at next token */
const Token* lexer_peek_token(Lexer* lexer, size_t lookahead) {
    assert(lexer != NULL);
    
    /* Ensure we have enough tokens in lookahead buffer */
    while (lexer->lookahead_count <= lookahead) {
        if (lexer->lookahead_count >= lexer->lookahead_size) {
            /* Resize lookahead buffer */
            size_t new_size = lexer->lookahead_size * 2;
            Token** new_buffer = (Token**)realloc(lexer->lookahead, 
                                                  new_size * sizeof(Token*));
            if (!new_buffer) return NULL;
            
            /* Initialize new slots */
            for (size_t i = lexer->lookahead_size; i < new_size; i++) {
                new_buffer[i] = NULL;
            }
            
            lexer->lookahead = new_buffer;
            lexer->lookahead_size = new_size;
        }
        
        /* Get next token */
        Token* token = lexer_next_token(lexer);
        if (!token) return NULL;
        
        /* Add to lookahead buffer */
        size_t idx = (lexer->lookahead_pos + lexer->lookahead_count) % lexer->lookahead_size;
        lexer->lookahead[idx] = token;
        lexer->lookahead_count++;
    }
    
    /* Return requested token */
    size_t idx = (lexer->lookahead_pos + lookahead) % lexer->lookahead_size;
    return lexer->lookahead[idx];
}

/* Tokenize entire input */
bool lexer_tokenize_all(Lexer* lexer, Token*** tokens, size_t* count) {
    assert(lexer != NULL);
    assert(tokens != NULL);
    assert(count != NULL);
    
    size_t capacity = 128;
    size_t size = 0;
    Token** token_array = (Token**)malloc(capacity * sizeof(Token*));
    if (!token_array) return false;
    
    while (true) {
        Token* token = lexer_next_token(lexer);
        if (!token) {
            /* Free allocated tokens on error */
            for (size_t i = 0; i < size; i++) {
                token_free(token_array[i]);
            }
            free(token_array);
            return false;
        }
        
        /* Resize array if needed */
        if (size >= capacity) {
            capacity *= 2;
            Token** new_array = (Token**)realloc(token_array, capacity * sizeof(Token*));
            if (!new_array) {
                /* Free allocated tokens on error */
                for (size_t i = 0; i < size; i++) {
                    token_free(token_array[i]);
                }
                free(token_array);
                token_free(token);
                return false;
            }
            token_array = new_array;
        }
        
        token_array[size++] = token;
        
        if (token->type == TOKEN_EOF) {
            break;
        }
    }
    
    *tokens = token_array;
    *count = size;
    return true;
}

/* Error handling */
const LexerError* lexer_get_errors(const Lexer* lexer) {
    assert(lexer != NULL);
    return lexer->errors;
}

size_t lexer_error_count(const Lexer* lexer) {
    assert(lexer != NULL);
    return lexer->error_count;
}

void lexer_clear_errors(Lexer* lexer) {
    assert(lexer != NULL);
    
    LexerError* error = lexer->errors;
    while (error) {
        LexerError* next = error->next;
        if (error->message) free(error->message);
        if (error->context) free(error->context);
        if (error->suggestion) free(error->suggestion);
        free(error);
        error = next;
    }
    
    lexer->errors = NULL;
    lexer->last_error = NULL;
    lexer->error_count = 0;
}

/* State management */
LexerState* lexer_save_state(const Lexer* lexer) {
    assert(lexer != NULL);
    
    LexerState* state = (LexerState*)calloc(1, sizeof(LexerState));
    if (!state) return NULL;
    
    state->position = lexer->position;
    state->line = lexer->line;
    state->column = lexer->column;
    state->state = lexer->state;
    
    if (lexer->partial_token && lexer->partial_length > 0) {
        state->partial_token = (char*)malloc(lexer->partial_length + 1);
        if (state->partial_token) {
            memcpy(state->partial_token, lexer->partial_token, lexer->partial_length);
            state->partial_token[lexer->partial_length] = '\0';
            state->partial_length = lexer->partial_length;
        }
    }
    
    return state;
}

void lexer_restore_state(Lexer* lexer, const LexerState* state) {
    assert(lexer != NULL);
    assert(state != NULL);
    
    lexer->position = state->position;
    lexer->line = state->line;
    lexer->column = state->column;
    lexer->state = state->state;
    
    if (lexer->partial_token) {
        free(lexer->partial_token);
        lexer->partial_token = NULL;
    }
    
    if (state->partial_token && state->partial_length > 0) {
        lexer->partial_token = (char*)malloc(state->partial_length + 1);
        if (lexer->partial_token) {
            memcpy(lexer->partial_token, state->partial_token, state->partial_length);
            lexer->partial_token[state->partial_length] = '\0';
            lexer->partial_length = state->partial_length;
        }
    }
}

void lexer_state_free(LexerState* state) {
    if (!state) return;
    
    if (state->partial_token) {
        free(state->partial_token);
    }
    
    free(state);
}

/* Position info */
void lexer_get_position(const Lexer* lexer, size_t* line, size_t* column, size_t* position) {
    assert(lexer != NULL);
    
    if (line) *line = lexer->line;
    if (column) *column = lexer->column;
    if (position) *position = lexer->position;
}

/* Default options */
LexerOptions lexer_default_options(void) {
    LexerOptions options = {
        .track_trivia = false,
        .unicode_normalization = false,
        .strict_mode = true,
        .error_recovery = true,
        .buffer_size = 1024,
        .max_token_length = 65536
    };
    return options;
}

/* Utilities */
bool lexer_validate_iri(const char* iri, size_t length) {
    if (!iri || length == 0) return false;
    
    for (size_t i = 0; i < length; i++) {
        char ch = iri[i];
        if (ch < 0x20 || ch == '<' || ch == '>' || ch == '"' || 
            ch == '{' || ch == '}' || ch == '|' || ch == '^' || 
            ch == '`' || ch == '\\') {
            return false;
        }
    }
    
    return true;
}

bool lexer_unescape_string(const char* escaped, size_t length, 
                          char* output, size_t* output_length) {
    if (!escaped || !output || !output_length) return false;
    
    size_t out_pos = 0;
    
    for (size_t i = 0; i < length; i++) {
        if (escaped[i] == '\\' && i + 1 < length) {
            i++;
            switch (escaped[i]) {
                case 't': output[out_pos++] = '\t'; break;
                case 'n': output[out_pos++] = '\n'; break;
                case 'r': output[out_pos++] = '\r'; break;
                case 'b': output[out_pos++] = '\b'; break;
                case 'f': output[out_pos++] = '\f'; break;
                case '"': output[out_pos++] = '"'; break;
                case '\'': output[out_pos++] = '\''; break;
                case '\\': output[out_pos++] = '\\'; break;
                case 'u':
                case 'U': {
                    /* Unicode escape */
                    int digits = (escaped[i] == 'u') ? 4 : 8;
                    if (i + digits >= length) return false;
                    
                    uint32_t codepoint = 0;
                    for (int j = 0; j < digits; j++) {
                        i++;
                        char ch = escaped[i];
                        if (!is_hex_digit(ch)) return false;
                        
                        codepoint = codepoint * 16;
                        if (ch >= '0' && ch <= '9') codepoint += ch - '0';
                        else if (ch >= 'A' && ch <= 'F') codepoint += ch - 'A' + 10;
                        else if (ch >= 'a' && ch <= 'f') codepoint += ch - 'a' + 10;
                    }
                    
                    /* Convert to UTF-8 */
                    if (codepoint <= 0x7F) {
                        output[out_pos++] = (char)codepoint;
                    } else if (codepoint <= 0x7FF) {
                        output[out_pos++] = 0xC0 | (codepoint >> 6);
                        output[out_pos++] = 0x80 | (codepoint & 0x3F);
                    } else if (codepoint <= 0xFFFF) {
                        output[out_pos++] = 0xE0 | (codepoint >> 12);
                        output[out_pos++] = 0x80 | ((codepoint >> 6) & 0x3F);
                        output[out_pos++] = 0x80 | (codepoint & 0x3F);
                    } else if (codepoint <= 0x10FFFF) {
                        output[out_pos++] = 0xF0 | (codepoint >> 18);
                        output[out_pos++] = 0x80 | ((codepoint >> 12) & 0x3F);
                        output[out_pos++] = 0x80 | ((codepoint >> 6) & 0x3F);
                        output[out_pos++] = 0x80 | (codepoint & 0x3F);
                    } else {
                        return false;
                    }
                    break;
                }
                default:
                    output[out_pos++] = escaped[i];
            }
        } else {
            output[out_pos++] = escaped[i];
        }
    }
    
    *output_length = out_pos;
    return true;
}

bool lexer_is_pn_chars_base(uint32_t ch) {
    /* Basic ASCII range */
    if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z')) {
        return true;
    }
    
    /* Extended Unicode ranges per W3C spec */
    if ((ch >= 0x00C0 && ch <= 0x00D6) ||
        (ch >= 0x00D8 && ch <= 0x00F6) ||
        (ch >= 0x00F8 && ch <= 0x02FF) ||
        (ch >= 0x0370 && ch <= 0x037D) ||
        (ch >= 0x037F && ch <= 0x1FFF) ||
        (ch >= 0x200C && ch <= 0x200D) ||
        (ch >= 0x2070 && ch <= 0x218F) ||
        (ch >= 0x2C00 && ch <= 0x2FEF) ||
        (ch >= 0x3001 && ch <= 0xD7FF) ||
        (ch >= 0xF900 && ch <= 0xFDCF) ||
        (ch >= 0xFDF0 && ch <= 0xFFFD) ||
        (ch >= 0x10000 && ch <= 0xEFFFF)) {
        return true;
    }
    
    return false;
}

bool lexer_is_pn_chars(uint32_t ch) {
    if (lexer_is_pn_chars_base(ch)) return true;
    
    if (ch == '-' || ch == '_' || (ch >= '0' && ch <= '9')) return true;
    
    /* Additional Unicode ranges */
    if (ch == 0x00B7 ||
        (ch >= 0x0300 && ch <= 0x036F) ||
        (ch >= 0x203F && ch <= 0x2040)) {
        return true;
    }
    
    return false;
}

/* Streaming support */
bool lexer_feed(Lexer* lexer, const char* input, size_t length) {
    (void)length; // TODO: Use when implementing streaming
    assert(lexer != NULL);
    assert(input != NULL);
    
    /* TODO: Implement streaming support */
    return false;
}

void lexer_end_input(Lexer* lexer) {
    assert(lexer != NULL);
    lexer->input_ended = true;
}