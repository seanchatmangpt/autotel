/**
 * @file ttl_lexer.c
 * @brief TTL (Time-To-Live) Lexer - Implementation
 * 
 * High-performance lexer for parsing TTL expressions.
 * Uses state machine approach with optimized character classification.
 */

#include "../include/ttl_lexer.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>

/* Note: time_units table removed - using direct conversion in ttl_convert_to_nanoseconds */

/* Character classification macros for performance */
#define IS_DIGIT(c) ((c) >= '0' && (c) <= '9')
#define IS_ALPHA(c) (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z'))
#define IS_WHITESPACE(c) ((c) == ' ' || (c) == '\t' || (c) == '\n' || (c) == '\r')

/* Internal functions */
static void advance(ttl_lexer_t *lexer);
static char peek(const ttl_lexer_t *lexer);
static char peek_next(const ttl_lexer_t *lexer);
static void skip_whitespace(ttl_lexer_t *lexer);
static bool scan_number(ttl_lexer_t *lexer, ttl_token_t *token);
static bool scan_unit(ttl_lexer_t *lexer, ttl_token_t *token);
static void set_error(ttl_lexer_t *lexer, const char *format, ...);

/* Initialize lexer */
bool ttl_lexer_init(ttl_lexer_t *lexer, const char *source, size_t length) {
    if (!lexer || !source) {
        return false;
    }
    
    memset(lexer, 0, sizeof(ttl_lexer_t));
    
    lexer->source = source;
    lexer->current = source;
    lexer->start = source;
    lexer->source_length = length;
    lexer->line = 1;
    lexer->column = 1;
    lexer->has_error = false;
    lexer->lookahead_valid = false;
    
    return true;
}

/* Get next token */
bool ttl_lexer_next_token(ttl_lexer_t *lexer, ttl_token_t *token) {
    if (!lexer || !token || lexer->has_error) {
        return false;
    }
    
    /* Skip whitespace */
    skip_whitespace(lexer);
    
    /* Mark token start */
    lexer->start = lexer->current;
    token->start = lexer->start;
    token->line = lexer->line;
    token->column = lexer->column;
    
    /* Check for EOF */
    if (lexer->current >= lexer->source + lexer->source_length || *lexer->current == '\0') {
        token->type = TOKEN_EOF;
        token->length = 0;
        return true;
    }
    
    char c = *lexer->current;
    
    /* Numbers */
    if (IS_DIGIT(c) || (c == '.' && IS_DIGIT(peek(lexer)))) {
        return scan_number(lexer, token);
    }
    
    /* Time units */
    if (IS_ALPHA(c)) {
        return scan_unit(lexer, token);
    }
    
    /* Single character tokens */
    advance(lexer);
    token->length = 1;
    
    switch (c) {
        case '+':
            token->type = TOKEN_PLUS;
            return true;
        case '-':
            token->type = TOKEN_MINUS;
            return true;
        case '*':
            token->type = TOKEN_MULTIPLY;
            return true;
        case '/':
            token->type = TOKEN_DIVIDE;
            return true;
        case '(':
            token->type = TOKEN_LPAREN;
            return true;
        case ')':
            token->type = TOKEN_RPAREN;
            return true;
        default:
            token->type = TOKEN_ERROR;
            set_error(lexer, "Unexpected character: '%c'", c);
            return false;
    }
}

/* Peek at next token without consuming */
bool ttl_lexer_peek_token(ttl_lexer_t *lexer, ttl_token_t *token) {
    if (!lexer || !token) {
        return false;
    }
    
    /* Save current state */
    const char *saved_current = lexer->current;
    const char *saved_start = lexer->start;
    size_t saved_line = lexer->line;
    size_t saved_column = lexer->column;
    
    /* Get next token */
    bool result = ttl_lexer_next_token(lexer, token);
    
    /* Restore state */
    lexer->current = saved_current;
    lexer->start = saved_start;
    lexer->line = saved_line;
    lexer->column = saved_column;
    
    return result;
}

/* Reset lexer */
void ttl_lexer_reset(ttl_lexer_t *lexer) {
    if (!lexer) {
        return;
    }
    
    lexer->current = lexer->source;
    lexer->start = lexer->source;
    lexer->line = 1;
    lexer->column = 1;
    lexer->has_error = false;
    lexer->lookahead_valid = false;
}

/* Cleanup lexer */
void ttl_lexer_cleanup(ttl_lexer_t *lexer) {
    if (!lexer) {
        return;
    }
    
    /* Currently no dynamic allocations to free */
    memset(lexer, 0, sizeof(ttl_lexer_t));
}

/* Get token type name */
const char *ttl_token_type_name(ttl_token_type_t type) {
    static const char *names[] = {
        "EOF", "ERROR", "INTEGER", "DECIMAL",
        "NANOSECOND", "MICROSECOND", "MILLISECOND", "SECOND",
        "MINUTE", "HOUR", "DAY", "WEEK", "MONTH", "YEAR",
        "PLUS", "MINUS", "MULTIPLY", "DIVIDE",
        "LPAREN", "RPAREN", "WHITESPACE"
    };
    
    if (type >= 0 && type < sizeof(names) / sizeof(names[0])) {
        return names[type];
    }
    return "UNKNOWN";
}

/* Convert time value to nanoseconds */
uint64_t ttl_convert_to_nanoseconds(double value, ttl_token_type_t unit) {
    switch (unit) {
        case TOKEN_NANOSECOND:
            return (uint64_t)value;
        case TOKEN_MICROSECOND:
            return (uint64_t)(value * 1000.0);
        case TOKEN_MILLISECOND:
            return (uint64_t)(value * 1000000.0);
        case TOKEN_SECOND:
            return (uint64_t)(value * 1000000000.0);
        case TOKEN_MINUTE:
            return (uint64_t)(value * 60.0 * 1000000000.0);
        case TOKEN_HOUR:
            return (uint64_t)(value * 3600.0 * 1000000000.0);
        case TOKEN_DAY:
            return (uint64_t)(value * 86400.0 * 1000000000.0);
        case TOKEN_WEEK:
            return (uint64_t)(value * 604800.0 * 1000000000.0);
        case TOKEN_MONTH:
            return (uint64_t)(value * 2592000.0 * 1000000000.0);
        case TOKEN_YEAR:
            return (uint64_t)(value * 31536000.0 * 1000000000.0);
        default:
            return 0;
    }
}

/* Get error message */
const char *ttl_lexer_get_error(const ttl_lexer_t *lexer) {
    if (!lexer || !lexer->has_error) {
        return NULL;
    }
    return lexer->error_msg;
}

/* Internal: Advance to next character */
static void advance(ttl_lexer_t *lexer) {
    if (*lexer->current == '\n') {
        lexer->line++;
        lexer->column = 1;
    } else {
        lexer->column++;
    }
    lexer->current++;
    lexer->lookahead_valid = false;
}

/* Internal: Peek at current character */
static char peek(const ttl_lexer_t *lexer) {
    if (lexer->current >= lexer->source + lexer->source_length) {
        return '\0';
    }
    return *lexer->current;
}

/* Internal: Peek at next character */
static char peek_next(const ttl_lexer_t *lexer) {
    if (lexer->current + 1 >= lexer->source + lexer->source_length) {
        return '\0';
    }
    return *(lexer->current + 1);
}

/* Internal: Match and consume expected character */
/* Currently unused but kept for future extensions
static bool match(ttl_lexer_t *lexer, char expected) {
    if (peek(lexer) != expected) {
        return false;
    }
    advance(lexer);
    return true;
}
*/

/* Internal: Skip whitespace */
static void skip_whitespace(ttl_lexer_t *lexer) {
    while (IS_WHITESPACE(peek(lexer))) {
        advance(lexer);
    }
}

/* Internal: Scan number (integer or decimal) */
static bool scan_number(ttl_lexer_t *lexer, ttl_token_t *token) {
    bool is_decimal = false;
    
    /* Scan integer part */
    while (IS_DIGIT(peek(lexer))) {
        advance(lexer);
    }
    
    /* Check for decimal point */
    if (peek(lexer) == '.' && IS_DIGIT(peek_next(lexer))) {
        is_decimal = true;
        advance(lexer); /* consume '.' */
        
        /* Scan fractional part */
        while (IS_DIGIT(peek(lexer))) {
            advance(lexer);
        }
    }
    
    /* Extract number string */
    token->length = lexer->current - lexer->start;
    char num_str[64];
    if (token->length >= sizeof(num_str)) {
        set_error(lexer, "Number too long");
        token->type = TOKEN_ERROR;
        return false;
    }
    
    memcpy(num_str, lexer->start, token->length);
    num_str[token->length] = '\0';
    
    /* Parse number */
    if (is_decimal) {
        token->type = TOKEN_DECIMAL;
        token->value.decimal_value = strtod(num_str, NULL);
    } else {
        token->type = TOKEN_INTEGER;
        token->value.integer_value = strtoll(num_str, NULL, 10);
    }
    
    return true;
}

/* Internal: Scan time unit */
static bool scan_unit(ttl_lexer_t *lexer, ttl_token_t *token) {
    /* Collect unit characters */
    while (IS_ALPHA(peek(lexer))) {
        advance(lexer);
    }
    
    token->length = lexer->current - lexer->start;
    
    /* Match against known units */
    if (token->length == 2 && memcmp(token->start, "ns", 2) == 0) {
        token->type = TOKEN_NANOSECOND;
    } else if (token->length == 2 && memcmp(token->start, "us", 2) == 0) {
        token->type = TOKEN_MICROSECOND;
    } else if (token->length == 2 && memcmp(token->start, "ms", 2) == 0) {
        token->type = TOKEN_MILLISECOND;
    } else if (token->length == 1 && *token->start == 's') {
        token->type = TOKEN_SECOND;
    } else if (token->length == 1 && *token->start == 'm') {
        token->type = TOKEN_MINUTE;
    } else if (token->length == 1 && *token->start == 'h') {
        token->type = TOKEN_HOUR;
    } else if (token->length == 1 && *token->start == 'd') {
        token->type = TOKEN_DAY;
    } else if (token->length == 1 && *token->start == 'w') {
        token->type = TOKEN_WEEK;
    } else if (token->length == 2 && memcmp(token->start, "mo", 2) == 0) {
        token->type = TOKEN_MONTH;
    } else if (token->length == 1 && *token->start == 'y') {
        token->type = TOKEN_YEAR;
    } else {
        token->type = TOKEN_ERROR;
        char unit_str[16];
        size_t len = token->length < sizeof(unit_str) - 1 ? token->length : sizeof(unit_str) - 1;
        memcpy(unit_str, token->start, len);
        unit_str[len] = '\0';
        set_error(lexer, "Unknown time unit: '%s'", unit_str);
        return false;
    }
    
    return true;
}

/* Internal: Set error message */
static void set_error(ttl_lexer_t *lexer, const char *format, ...) {
    lexer->has_error = true;
    
    va_list args;
    va_start(args, format);
    vsnprintf(lexer->error_msg, sizeof(lexer->error_msg), format, args);
    va_end(args);
}

/* Optional: State machine visualization for documentation */
#ifdef TTL_LEXER_DEBUG
void ttl_lexer_dump_state(const ttl_lexer_t *lexer) {
    printf("Lexer State:\n");
    printf("  Position: line %zu, column %zu\n", lexer->line, lexer->column);
    printf("  Current: '%c' (0x%02x)\n", 
           peek(lexer) ? peek(lexer) : ' ', 
           (unsigned char)peek(lexer));
    printf("  Has Error: %s\n", lexer->has_error ? "yes" : "no");
    if (lexer->has_error) {
        printf("  Error: %s\n", lexer->error_msg);
    }
}
#endif