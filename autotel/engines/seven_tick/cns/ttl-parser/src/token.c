/**
 * @file token.c
 * @brief Token utilities implementation
 * @author CNS Seven-Tick Engine
 * @date 2024
 */

#include "../include/token.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/**
 * Token type string representations
 */
static const char* TOKEN_TYPE_NAMES[] = {
    [TOKEN_IRI_REF] = "IRI_REF",
    [TOKEN_PREFIXED_NAME] = "PREFIXED_NAME",
    [TOKEN_STRING_LITERAL_QUOTE] = "STRING_LITERAL_QUOTE",
    [TOKEN_STRING_LITERAL_SINGLE_QUOTE] = "STRING_LITERAL_SINGLE_QUOTE",
    [TOKEN_STRING_LITERAL_LONG_QUOTE] = "STRING_LITERAL_LONG_QUOTE",
    [TOKEN_STRING_LITERAL_LONG_SINGLE_QUOTE] = "STRING_LITERAL_LONG_SINGLE_QUOTE",
    [TOKEN_INTEGER] = "INTEGER",
    [TOKEN_DECIMAL] = "DECIMAL",
    [TOKEN_DOUBLE] = "DOUBLE",
    [TOKEN_BOOLEAN] = "BOOLEAN",
    [TOKEN_BLANK_NODE_LABEL] = "BLANK_NODE_LABEL",
    [TOKEN_ANON] = "ANON",
    [TOKEN_PREFIX] = "PREFIX",
    [TOKEN_BASE] = "BASE",
    [TOKEN_A] = "A",
    [TOKEN_DOT] = "DOT",
    [TOKEN_SEMICOLON] = "SEMICOLON",
    [TOKEN_COMMA] = "COMMA",
    [TOKEN_OPEN_PAREN] = "OPEN_PAREN",
    [TOKEN_CLOSE_PAREN] = "CLOSE_PAREN",
    [TOKEN_OPEN_BRACKET] = "OPEN_BRACKET",
    [TOKEN_CLOSE_BRACKET] = "CLOSE_BRACKET",
    [TOKEN_OPEN_BRACE] = "OPEN_BRACE",
    [TOKEN_CLOSE_BRACE] = "CLOSE_BRACE",
    [TOKEN_DOUBLE_CARET] = "DOUBLE_CARET",
    [TOKEN_AT] = "AT",
    [TOKEN_COMMENT] = "COMMENT",
    [TOKEN_WHITESPACE] = "WHITESPACE",
    [TOKEN_EOF] = "EOF",
    [TOKEN_INVALID] = "INVALID"
};

Token* token_create(TokenType type, const char* value, size_t length) {
    Token* token = (Token*)calloc(1, sizeof(Token));
    if (!token) return NULL;
    
    token->type = type;
    token->length = length;
    token->has_error = false;
    
    if (value && length > 0) {
        token->value = (char*)malloc(length + 1);
        if (!token->value) {
            free(token);
            return NULL;
        }
        memcpy(token->value, value, length);
        token->value[length] = '\0';
    }
    
    return token;
}

Token* token_create_error(const char* value, size_t length, const char* error_message) {
    Token* token = token_create(TOKEN_INVALID, value, length);
    if (!token) return NULL;
    
    token->has_error = true;
    
    if (error_message) {
        size_t msg_len = strlen(error_message);
        token->error_message = (char*)malloc(msg_len + 1);
        if (token->error_message) {
            strcpy(token->error_message, error_message);
        }
    }
    
    return token;
}

void token_free(Token* token) {
    if (!token) return;
    
    if (token->value) {
        free(token->value);
    }
    
    if (token->error_message) {
        free(token->error_message);
    }
    
    if (token->trivia) {
        token_trivia_free(token->trivia);
    }
    
    free(token);
}

Token* token_duplicate(const Token* token) {
    if (!token) return NULL;
    
    Token* dup = token_create(token->type, token->value, token->length);
    if (!dup) return NULL;
    
    dup->line = token->line;
    dup->column = token->column;
    dup->start_pos = token->start_pos;
    dup->end_pos = token->end_pos;
    dup->has_error = token->has_error;
    
    if (token->error_message) {
        size_t msg_len = strlen(token->error_message);
        dup->error_message = (char*)malloc(msg_len + 1);
        if (dup->error_message) {
            strcpy(dup->error_message, token->error_message);
        }
    }
    
    if (token->trivia) {
        dup->trivia = token_trivia_create();
        if (dup->trivia) {
            if (token->trivia->leading_whitespace) {
                size_t len = strlen(token->trivia->leading_whitespace);
                dup->trivia->leading_whitespace = (char*)malloc(len + 1);
                if (dup->trivia->leading_whitespace) {
                    strcpy(dup->trivia->leading_whitespace, token->trivia->leading_whitespace);
                }
            }
            
            if (token->trivia->trailing_whitespace) {
                size_t len = strlen(token->trivia->trailing_whitespace);
                dup->trivia->trailing_whitespace = (char*)malloc(len + 1);
                if (dup->trivia->trailing_whitespace) {
                    strcpy(dup->trivia->trailing_whitespace, token->trivia->trailing_whitespace);
                }
            }
            
            for (size_t i = 0; i < token->trivia->comment_count; i++) {
                token_trivia_add_comment(dup->trivia, token->trivia->comments[i]);
            }
        }
    }
    
    return dup;
}

const char* token_type_to_string(TokenType type) {
    if (type >= 0 && type < sizeof(TOKEN_TYPE_NAMES) / sizeof(TOKEN_TYPE_NAMES[0])) {
        return TOKEN_TYPE_NAMES[type];
    }
    return "UNKNOWN";
}

bool token_is_literal(TokenType type) {
    switch (type) {
        case TOKEN_STRING_LITERAL_QUOTE:
        case TOKEN_STRING_LITERAL_SINGLE_QUOTE:
        case TOKEN_STRING_LITERAL_LONG_QUOTE:
        case TOKEN_STRING_LITERAL_LONG_SINGLE_QUOTE:
        case TOKEN_INTEGER:
        case TOKEN_DECIMAL:
        case TOKEN_DOUBLE:
        case TOKEN_BOOLEAN:
            return true;
        default:
            return false;
    }
}

bool token_is_keyword(TokenType type) {
    switch (type) {
        case TOKEN_PREFIX:
        case TOKEN_BASE:
        case TOKEN_A:
            return true;
        default:
            return false;
    }
}

bool token_is_punctuation(TokenType type) {
    switch (type) {
        case TOKEN_DOT:
        case TOKEN_SEMICOLON:
        case TOKEN_COMMA:
        case TOKEN_OPEN_PAREN:
        case TOKEN_CLOSE_PAREN:
        case TOKEN_OPEN_BRACKET:
        case TOKEN_CLOSE_BRACKET:
        case TOKEN_OPEN_BRACE:
        case TOKEN_CLOSE_BRACE:
        case TOKEN_DOUBLE_CARET:
        case TOKEN_AT:
            return true;
        default:
            return false;
    }
}

TokenTrivia* token_trivia_create(void) {
    TokenTrivia* trivia = (TokenTrivia*)calloc(1, sizeof(TokenTrivia));
    return trivia;
}

void token_trivia_free(TokenTrivia* trivia) {
    if (!trivia) return;
    
    if (trivia->leading_whitespace) {
        free(trivia->leading_whitespace);
    }
    
    if (trivia->trailing_whitespace) {
        free(trivia->trailing_whitespace);
    }
    
    if (trivia->comments) {
        for (size_t i = 0; i < trivia->comment_count; i++) {
            if (trivia->comments[i]) {
                free(trivia->comments[i]);
            }
        }
        free(trivia->comments);
    }
    
    free(trivia);
}

void token_trivia_add_comment(TokenTrivia* trivia, const char* comment) {
    if (!trivia || !comment) return;
    
    size_t new_count = trivia->comment_count + 1;
    char** new_comments = (char**)realloc(trivia->comments, new_count * sizeof(char*));
    if (!new_comments) return;
    
    trivia->comments = new_comments;
    
    size_t len = strlen(comment);
    trivia->comments[trivia->comment_count] = (char*)malloc(len + 1);
    if (trivia->comments[trivia->comment_count]) {
        strcpy(trivia->comments[trivia->comment_count], comment);
        trivia->comment_count = new_count;
    }
}