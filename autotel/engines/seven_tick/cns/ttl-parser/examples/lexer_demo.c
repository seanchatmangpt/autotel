/**
 * @file lexer_demo.c
 * @brief TTL Lexer Demonstration
 * 
 * Shows how to use the TTL lexer to tokenize various expressions.
 */

#include "../include/ttl_lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Print token information */
static void print_token(const ttl_token_t *token) {
    printf("Token: %-12s", ttl_token_type_name(token->type));
    
    if (token->type == TOKEN_INTEGER) {
        printf(" Value: %lld", (long long)token->value.integer_value);
    } else if (token->type == TOKEN_DECIMAL) {
        printf(" Value: %.3f", token->value.decimal_value);
    } else if (token->length > 0) {
        printf(" Text: '%.*s'", (int)token->length, token->start);
    }
    
    printf(" (Line %zu, Col %zu)\n", token->line, token->column);
}

/* Tokenize and display a TTL expression */
static void tokenize_expression(const char *expr) {
    ttl_lexer_t lexer;
    ttl_token_t token;
    
    printf("\nTokenizing: \"%s\"\n", expr);
    printf("----------------------------------------\n");
    
    if (!ttl_lexer_init(&lexer, expr, strlen(expr))) {
        fprintf(stderr, "Failed to initialize lexer\n");
        return;
    }
    
    while (ttl_lexer_next_token(&lexer, &token)) {
        if (token.type == TOKEN_EOF) {
            printf("Token: EOF\n");
            break;
        }
        
        if (token.type == TOKEN_ERROR) {
            printf("ERROR: %s\n", ttl_lexer_get_error(&lexer));
            break;
        }
        
        print_token(&token);
    }
    
    ttl_lexer_cleanup(&lexer);
}

/* Calculate total time in nanoseconds from a simple expression */
static void calculate_ttl(const char *expr) {
    ttl_lexer_t lexer;
    ttl_token_t token;
    uint64_t total_ns = 0;
    double current_value = 0;
    
    printf("\nCalculating TTL: \"%s\"\n", expr);
    
    if (!ttl_lexer_init(&lexer, expr, strlen(expr))) {
        fprintf(stderr, "Failed to initialize lexer\n");
        return;
    }
    
    while (ttl_lexer_next_token(&lexer, &token)) {
        if (token.type == TOKEN_EOF) {
            break;
        }
        
        if (token.type == TOKEN_ERROR) {
            printf("ERROR: %s\n", ttl_lexer_get_error(&lexer));
            break;
        }
        
        if (token.type == TOKEN_INTEGER) {
            current_value = (double)token.value.integer_value;
        } else if (token.type == TOKEN_DECIMAL) {
            current_value = token.value.decimal_value;
        } else if (token.type >= TOKEN_NANOSECOND && token.type <= TOKEN_YEAR) {
            uint64_t ns = ttl_convert_to_nanoseconds(current_value, token.type);
            total_ns += ns;
            printf("  %g %s = %llu nanoseconds\n", 
                   current_value, ttl_token_type_name(token.type), 
                   (unsigned long long)ns);
        }
    }
    
    /* Convert to human-readable format */
    printf("Total: %llu nanoseconds", (unsigned long long)total_ns);
    
    if (total_ns >= 1000000000ULL) {
        printf(" (%.3f seconds)", total_ns / 1000000000.0);
    } else if (total_ns >= 1000000ULL) {
        printf(" (%.3f milliseconds)", total_ns / 1000000.0);
    } else if (total_ns >= 1000ULL) {
        printf(" (%.3f microseconds)", total_ns / 1000.0);
    }
    printf("\n");
    
    ttl_lexer_cleanup(&lexer);
}

int main(void) {
    printf("=== TTL Lexer Demonstration ===\n");
    
    /* Basic examples */
    tokenize_expression("30s");
    tokenize_expression("5m 30s");
    tokenize_expression("1.5h");
    tokenize_expression("100ms + 50ms");
    tokenize_expression("(1h + 30m) * 2");
    
    /* Error examples */
    tokenize_expression("5 minutes");  /* Unknown unit */
    tokenize_expression("10s @ 5s");   /* Invalid character */
    
    /* Calculate some TTLs */
    printf("\n=== TTL Calculations ===\n");
    calculate_ttl("5m");
    calculate_ttl("5m 30s");
    calculate_ttl("1h 30m 45s");
    calculate_ttl("1.5d");
    calculate_ttl("100ms 250us 500ns");
    
    return 0;
}