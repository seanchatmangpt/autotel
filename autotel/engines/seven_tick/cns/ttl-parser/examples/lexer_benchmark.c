/**
 * @file lexer_benchmark.c
 * @brief TTL Lexer Performance Benchmark
 * 
 * Measures the performance of the TTL lexer on various inputs.
 */

#include "../include/ttl_lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Benchmark configuration */
#define ITERATIONS 1000000
#define WARMUP_ITERATIONS 1000

/* Time measurement */
static double get_time_diff(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
}

/* Benchmark a single expression */
static void benchmark_expression(const char *name, const char *expr) {
    ttl_lexer_t lexer;
    ttl_token_t token;
    struct timespec start, end;
    
    /* Warmup */
    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
        ttl_lexer_init(&lexer, expr, strlen(expr));
        while (ttl_lexer_next_token(&lexer, &token)) {
            if (token.type == TOKEN_EOF) break;
        }
        ttl_lexer_cleanup(&lexer);
    }
    
    /* Benchmark */
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    for (int i = 0; i < ITERATIONS; i++) {
        ttl_lexer_init(&lexer, expr, strlen(expr));
        while (ttl_lexer_next_token(&lexer, &token)) {
            if (token.type == TOKEN_EOF) break;
        }
        ttl_lexer_cleanup(&lexer);
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    double elapsed = get_time_diff(start, end);
    double ops_per_sec = ITERATIONS / elapsed;
    double ns_per_op = elapsed * 1e9 / ITERATIONS;
    
    printf("%-30s: %8.2f ns/op | %10.0f ops/sec\n", 
           name, ns_per_op, ops_per_sec);
}

/* Count tokens in expression */
static int count_tokens(const char *expr) {
    ttl_lexer_t lexer;
    ttl_token_t token;
    int count = 0;
    
    ttl_lexer_init(&lexer, expr, strlen(expr));
    while (ttl_lexer_next_token(&lexer, &token)) {
        if (token.type == TOKEN_EOF) break;
        count++;
    }
    ttl_lexer_cleanup(&lexer);
    
    return count;
}

int main(void) {
    printf("=== TTL Lexer Performance Benchmark ===\n");
    printf("Iterations: %d\n", ITERATIONS);
    printf("----------------------------------------\n\n");
    
    /* Test expressions */
    struct {
        const char *name;
        const char *expr;
    } tests[] = {
        {"Simple integer", "42"},
        {"Simple decimal", "3.14"},
        {"Single TTL", "30s"},
        {"Two TTL values", "5m 30s"},
        {"Complex expression", "(5m + 30s) * 2"},
        {"Long expression", "1h 30m 45s 500ms 250us 100ns"},
        {"Arithmetic", "100 + 200 - 50 * 2 / 10"},
        {"Nested parentheses", "((1h + 30m) * 2) - (15m / 3)"},
    };
    
    /* Print token counts */
    printf("Token counts:\n");
    for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
        int tokens = count_tokens(tests[i].expr);
        printf("  %-30s: %d tokens\n", tests[i].name, tokens);
    }
    printf("\n");
    
    /* Run benchmarks */
    printf("Performance results:\n");
    for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
        benchmark_expression(tests[i].name, tests[i].expr);
    }
    
    /* Memory usage */
    printf("\n");
    printf("Memory usage:\n");
    printf("  ttl_lexer_t size: %zu bytes\n", sizeof(ttl_lexer_t));
    printf("  ttl_token_t size: %zu bytes\n", sizeof(ttl_token_t));
    
    return 0;
}