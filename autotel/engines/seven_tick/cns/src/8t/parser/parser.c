#include "cns/8t/interfaces.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// 8T Parser Implementation - 80/20 state machine parser

typedef struct {
    parser_state_t state;
    char* buffer;
    size_t buffer_size;
    size_t buffer_pos;
} parser_context_t;

static parser_context_t* g_parser_ctx = NULL;

static int parser_init(void) {
    if (g_parser_ctx != NULL) {
        return -1;
    }
    
    g_parser_ctx = calloc(1, sizeof(parser_context_t));
    if (g_parser_ctx == NULL) {
        return -1;
    }
    
    g_parser_ctx->buffer_size = 4096;
    g_parser_ctx->buffer = malloc(g_parser_ctx->buffer_size);
    if (g_parser_ctx->buffer == NULL) {
        free(g_parser_ctx);
        g_parser_ctx = NULL;
        return -1;
    }
    
    g_parser_ctx->state = STATE_INIT;
    return 0;
}

static void parser_cleanup(void) {
    if (g_parser_ctx != NULL) {
        free(g_parser_ctx->buffer);
        free(g_parser_ctx);
        g_parser_ctx = NULL;
    }
}

static int parser_parse(const char* input, size_t len, triple_t* output) {
    if (g_parser_ctx == NULL || input == NULL || output == NULL) {
        return -1;
    }
    
    // 80/20: Simple state machine for parsing triples
    // Format: <subject> <predicate> <object> .
    
    g_parser_ctx->state = STATE_SUBJECT;
    g_parser_ctx->buffer_pos = 0;
    
    for (size_t i = 0; i < len; i++) {
        char c = input[i];
        
        switch (g_parser_ctx->state) {
            case STATE_SUBJECT:
                if (c == '>') {
                    g_parser_ctx->buffer[g_parser_ctx->buffer_pos] = '\\0';
                    output->subject = (uint32_t)strtoul(g_parser_ctx->buffer, NULL, 10);
                    g_parser_ctx->state = STATE_PREDICATE;
                    g_parser_ctx->buffer_pos = 0;
                } else if (c != '<' && c != ' ') {
                    g_parser_ctx->buffer[g_parser_ctx->buffer_pos++] = c;
                }
                break;
                
            case STATE_PREDICATE:
                if (c == '>') {
                    g_parser_ctx->buffer[g_parser_ctx->buffer_pos] = '\\0';
                    output->predicate = (uint32_t)strtoul(g_parser_ctx->buffer, NULL, 10);
                    g_parser_ctx->state = STATE_OBJECT;
                    g_parser_ctx->buffer_pos = 0;
                } else if (c != '<' && c != ' ') {
                    g_parser_ctx->buffer[g_parser_ctx->buffer_pos++] = c;
                }
                break;
                
            case STATE_OBJECT:
                if (c == '>' || c == '.') {
                    g_parser_ctx->buffer[g_parser_ctx->buffer_pos] = '\\0';
                    output->object = (uint32_t)strtoul(g_parser_ctx->buffer, NULL, 10);
                    g_parser_ctx->state = STATE_COMPLETE;
                    return 0;
                } else if (c != '<' && c != ' ' && c != '"') {
                    g_parser_ctx->buffer[g_parser_ctx->buffer_pos++] = c;
                }
                break;
                
            default:
                break;
        }
        
        // Buffer overflow protection
        if (g_parser_ctx->buffer_pos >= g_parser_ctx->buffer_size - 1) {
            return -1;
        }
    }
    
    return (g_parser_ctx->state == STATE_COMPLETE) ? 0 : -1;
}

// Export parser interface
static parser_interface_t t8_parser = {
    .init = parser_init,
    .parse = parser_parse,
    .cleanup = parser_cleanup
};

parser_interface_t* t8_get_parser(void) {
    return &t8_parser;
}