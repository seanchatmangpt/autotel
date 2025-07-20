/**
 * @file test_lexer.c
 * @brief TTL Lexer Test Suite
 * 
 * Comprehensive tests for the TTL lexer including:
 * - Basic token recognition
 * - Number parsing (integers and decimals)
 * - Time unit recognition
 * - Operator tokenization
 * - Error handling
 * - Edge cases
 */

#include "../include/ttl_lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* Test result tracking */
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

/* Test macros */
#define TEST(name) static void test_##name(void)
#define RUN_TEST(name) do { \
    printf("Running %s... ", #name); \
    fflush(stdout); \
    tests_run++; \
    test_##name(); \
    tests_passed++; \
    printf("PASSED\n"); \
} while(0)

#define ASSERT(cond) do { \
    if (!(cond)) { \
        printf("FAILED\n  Assertion failed: %s\n  at %s:%d\n", \
               #cond, __FILE__, __LINE__); \
        tests_failed++; \
        tests_passed--; \
        return; \
    } \
} while(0)

#define ASSERT_EQ(a, b) ASSERT((a) == (b))
#define ASSERT_STR_EQ(a, b) ASSERT(strcmp((a), (b)) == 0)

/* Test cases */

TEST(basic_integers) {
    ttl_lexer_t lexer;
    ttl_token_t token;
    
    const char *source = "42 123 0 999";
    ASSERT(ttl_lexer_init(&lexer, source, strlen(source)));
    
    /* First number: 42 */
    ASSERT(ttl_lexer_next_token(&lexer, &token));
    ASSERT_EQ(token.type, TOKEN_INTEGER);
    ASSERT_EQ(token.value.integer_value, 42);
    
    /* Second number: 123 */
    ASSERT(ttl_lexer_next_token(&lexer, &token));
    ASSERT_EQ(token.type, TOKEN_INTEGER);
    ASSERT_EQ(token.value.integer_value, 123);
    
    /* Third number: 0 */
    ASSERT(ttl_lexer_next_token(&lexer, &token));
    ASSERT_EQ(token.type, TOKEN_INTEGER);
    ASSERT_EQ(token.value.integer_value, 0);
    
    /* Fourth number: 999 */
    ASSERT(ttl_lexer_next_token(&lexer, &token));
    ASSERT_EQ(token.type, TOKEN_INTEGER);
    ASSERT_EQ(token.value.integer_value, 999);
    
    /* EOF */
    ASSERT(ttl_lexer_next_token(&lexer, &token));
    ASSERT_EQ(token.type, TOKEN_EOF);
    
    ttl_lexer_cleanup(&lexer);
}

TEST(basic_decimals) {
    ttl_lexer_t lexer;
    ttl_token_t token;
    
    const char *source = "3.14 0.5 123.456";
    ASSERT(ttl_lexer_init(&lexer, source, strlen(source)));
    
    /* First decimal: 3.14 */
    ASSERT(ttl_lexer_next_token(&lexer, &token));
    ASSERT_EQ(token.type, TOKEN_DECIMAL);
    ASSERT(token.value.decimal_value > 3.13 && token.value.decimal_value < 3.15);
    
    /* Second decimal: 0.5 */
    ASSERT(ttl_lexer_next_token(&lexer, &token));
    ASSERT_EQ(token.type, TOKEN_DECIMAL);
    ASSERT(token.value.decimal_value > 0.49 && token.value.decimal_value < 0.51);
    
    /* Third decimal: 123.456 */
    ASSERT(ttl_lexer_next_token(&lexer, &token));
    ASSERT_EQ(token.type, TOKEN_DECIMAL);
    ASSERT(token.value.decimal_value > 123.455 && token.value.decimal_value < 123.457);
    
    ttl_lexer_cleanup(&lexer);
}

TEST(time_units) {
    ttl_lexer_t lexer;
    ttl_token_t token;
    
    const char *source = "ns us ms s m h d w mo y";
    ASSERT(ttl_lexer_init(&lexer, source, strlen(source)));
    
    struct {
        ttl_token_type_t expected;
        const char *name;
    } units[] = {
        {TOKEN_NANOSECOND, "ns"},
        {TOKEN_MICROSECOND, "us"},
        {TOKEN_MILLISECOND, "ms"},
        {TOKEN_SECOND, "s"},
        {TOKEN_MINUTE, "m"},
        {TOKEN_HOUR, "h"},
        {TOKEN_DAY, "d"},
        {TOKEN_WEEK, "w"},
        {TOKEN_MONTH, "mo"},
        {TOKEN_YEAR, "y"}
    };
    
    for (size_t i = 0; i < sizeof(units) / sizeof(units[0]); i++) {
        ASSERT(ttl_lexer_next_token(&lexer, &token));
        ASSERT_EQ(token.type, units[i].expected);
    }
    
    ttl_lexer_cleanup(&lexer);
}

TEST(operators) {
    ttl_lexer_t lexer;
    ttl_token_t token;
    
    const char *source = "+ - * / ( )";
    ASSERT(ttl_lexer_init(&lexer, source, strlen(source)));
    
    /* Plus */
    ASSERT(ttl_lexer_next_token(&lexer, &token));
    ASSERT_EQ(token.type, TOKEN_PLUS);
    
    /* Minus */
    ASSERT(ttl_lexer_next_token(&lexer, &token));
    ASSERT_EQ(token.type, TOKEN_MINUS);
    
    /* Multiply */
    ASSERT(ttl_lexer_next_token(&lexer, &token));
    ASSERT_EQ(token.type, TOKEN_MULTIPLY);
    
    /* Divide */
    ASSERT(ttl_lexer_next_token(&lexer, &token));
    ASSERT_EQ(token.type, TOKEN_DIVIDE);
    
    /* Left paren */
    ASSERT(ttl_lexer_next_token(&lexer, &token));
    ASSERT_EQ(token.type, TOKEN_LPAREN);
    
    /* Right paren */
    ASSERT(ttl_lexer_next_token(&lexer, &token));
    ASSERT_EQ(token.type, TOKEN_RPAREN);
    
    ttl_lexer_cleanup(&lexer);
}

TEST(ttl_expressions) {
    ttl_lexer_t lexer;
    ttl_token_t token;
    
    const char *source = "5m 30s 1.5h 100ms";
    ASSERT(ttl_lexer_init(&lexer, source, strlen(source)));
    
    /* 5m */
    ASSERT(ttl_lexer_next_token(&lexer, &token));
    ASSERT_EQ(token.type, TOKEN_INTEGER);
    ASSERT_EQ(token.value.integer_value, 5);
    
    ASSERT(ttl_lexer_next_token(&lexer, &token));
    ASSERT_EQ(token.type, TOKEN_MINUTE);
    
    /* 30s */
    ASSERT(ttl_lexer_next_token(&lexer, &token));
    ASSERT_EQ(token.type, TOKEN_INTEGER);
    ASSERT_EQ(token.value.integer_value, 30);
    
    ASSERT(ttl_lexer_next_token(&lexer, &token));
    ASSERT_EQ(token.type, TOKEN_SECOND);
    
    /* 1.5h */
    ASSERT(ttl_lexer_next_token(&lexer, &token));
    ASSERT_EQ(token.type, TOKEN_DECIMAL);
    ASSERT(token.value.decimal_value > 1.49 && token.value.decimal_value < 1.51);
    
    ASSERT(ttl_lexer_next_token(&lexer, &token));
    ASSERT_EQ(token.type, TOKEN_HOUR);
    
    /* 100ms */
    ASSERT(ttl_lexer_next_token(&lexer, &token));
    ASSERT_EQ(token.type, TOKEN_INTEGER);
    ASSERT_EQ(token.value.integer_value, 100);
    
    ASSERT(ttl_lexer_next_token(&lexer, &token));
    ASSERT_EQ(token.type, TOKEN_MILLISECOND);
    
    ttl_lexer_cleanup(&lexer);
}

TEST(complex_expression) {
    ttl_lexer_t lexer;
    ttl_token_t token;
    
    const char *source = "(5m + 30s) * 2";
    ASSERT(ttl_lexer_init(&lexer, source, strlen(source)));
    
    /* ( */
    ASSERT(ttl_lexer_next_token(&lexer, &token));
    ASSERT_EQ(token.type, TOKEN_LPAREN);
    
    /* 5 */
    ASSERT(ttl_lexer_next_token(&lexer, &token));
    ASSERT_EQ(token.type, TOKEN_INTEGER);
    ASSERT_EQ(token.value.integer_value, 5);
    
    /* m */
    ASSERT(ttl_lexer_next_token(&lexer, &token));
    ASSERT_EQ(token.type, TOKEN_MINUTE);
    
    /* + */
    ASSERT(ttl_lexer_next_token(&lexer, &token));
    ASSERT_EQ(token.type, TOKEN_PLUS);
    
    /* 30 */
    ASSERT(ttl_lexer_next_token(&lexer, &token));
    ASSERT_EQ(token.type, TOKEN_INTEGER);
    ASSERT_EQ(token.value.integer_value, 30);
    
    /* s */
    ASSERT(ttl_lexer_next_token(&lexer, &token));
    ASSERT_EQ(token.type, TOKEN_SECOND);
    
    /* ) */
    ASSERT(ttl_lexer_next_token(&lexer, &token));
    ASSERT_EQ(token.type, TOKEN_RPAREN);
    
    /* * */
    ASSERT(ttl_lexer_next_token(&lexer, &token));
    ASSERT_EQ(token.type, TOKEN_MULTIPLY);
    
    /* 2 */
    ASSERT(ttl_lexer_next_token(&lexer, &token));
    ASSERT_EQ(token.type, TOKEN_INTEGER);
    ASSERT_EQ(token.value.integer_value, 2);
    
    ttl_lexer_cleanup(&lexer);
}

TEST(error_handling) {
    ttl_lexer_t lexer;
    ttl_token_t token;
    
    /* Unknown character */
    const char *source1 = "5m @ 30s";
    ASSERT(ttl_lexer_init(&lexer, source1, strlen(source1)));
    
    ASSERT(ttl_lexer_next_token(&lexer, &token)); /* 5 */
    ASSERT(ttl_lexer_next_token(&lexer, &token)); /* m */
    ASSERT(!ttl_lexer_next_token(&lexer, &token)); /* @ should fail */
    ASSERT_EQ(token.type, TOKEN_ERROR);
    ASSERT(ttl_lexer_get_error(&lexer) != NULL);
    
    ttl_lexer_cleanup(&lexer);
    
    /* Unknown time unit */
    const char *source2 = "5xyz";
    ASSERT(ttl_lexer_init(&lexer, source2, strlen(source2)));
    
    ASSERT(ttl_lexer_next_token(&lexer, &token)); /* 5 */
    ASSERT(!ttl_lexer_next_token(&lexer, &token)); /* xyz should fail */
    ASSERT_EQ(token.type, TOKEN_ERROR);
    
    ttl_lexer_cleanup(&lexer);
}

TEST(whitespace_handling) {
    ttl_lexer_t lexer;
    ttl_token_t token;
    
    const char *source = "  5   m\t+\n30  s  ";
    ASSERT(ttl_lexer_init(&lexer, source, strlen(source)));
    
    /* 5 */
    ASSERT(ttl_lexer_next_token(&lexer, &token));
    ASSERT_EQ(token.type, TOKEN_INTEGER);
    ASSERT_EQ(token.value.integer_value, 5);
    
    /* m */
    ASSERT(ttl_lexer_next_token(&lexer, &token));
    ASSERT_EQ(token.type, TOKEN_MINUTE);
    
    /* + */
    ASSERT(ttl_lexer_next_token(&lexer, &token));
    ASSERT_EQ(token.type, TOKEN_PLUS);
    
    /* 30 */
    ASSERT(ttl_lexer_next_token(&lexer, &token));
    ASSERT_EQ(token.type, TOKEN_INTEGER);
    ASSERT_EQ(token.value.integer_value, 30);
    
    /* s */
    ASSERT(ttl_lexer_next_token(&lexer, &token));
    ASSERT_EQ(token.type, TOKEN_SECOND);
    
    /* EOF */
    ASSERT(ttl_lexer_next_token(&lexer, &token));
    ASSERT_EQ(token.type, TOKEN_EOF);
    
    ttl_lexer_cleanup(&lexer);
}

TEST(peek_functionality) {
    ttl_lexer_t lexer;
    ttl_token_t token, peek_token;
    
    const char *source = "5m";
    ASSERT(ttl_lexer_init(&lexer, source, strlen(source)));
    
    /* Peek should not consume */
    ASSERT(ttl_lexer_peek_token(&lexer, &peek_token));
    ASSERT_EQ(peek_token.type, TOKEN_INTEGER);
    ASSERT_EQ(peek_token.value.integer_value, 5);
    
    /* Next should return same token */
    ASSERT(ttl_lexer_next_token(&lexer, &token));
    ASSERT_EQ(token.type, TOKEN_INTEGER);
    ASSERT_EQ(token.value.integer_value, 5);
    
    ttl_lexer_cleanup(&lexer);
}

TEST(time_conversion) {
    /* Test conversion function */
    ASSERT_EQ(ttl_convert_to_nanoseconds(1.0, TOKEN_NANOSECOND), 1ULL);
    ASSERT_EQ(ttl_convert_to_nanoseconds(1.0, TOKEN_MICROSECOND), 1000ULL);
    ASSERT_EQ(ttl_convert_to_nanoseconds(1.0, TOKEN_MILLISECOND), 1000000ULL);
    ASSERT_EQ(ttl_convert_to_nanoseconds(1.0, TOKEN_SECOND), 1000000000ULL);
    ASSERT_EQ(ttl_convert_to_nanoseconds(1.0, TOKEN_MINUTE), 60ULL * 1000000000ULL);
    ASSERT_EQ(ttl_convert_to_nanoseconds(1.0, TOKEN_HOUR), 3600ULL * 1000000000ULL);
    ASSERT_EQ(ttl_convert_to_nanoseconds(2.5, TOKEN_SECOND), 2500000000ULL);
}

/* Main test runner */
int main(void) {
    printf("=== TTL Lexer Test Suite ===\n\n");
    
    /* Run all tests */
    RUN_TEST(basic_integers);
    RUN_TEST(basic_decimals);
    RUN_TEST(time_units);
    RUN_TEST(operators);
    RUN_TEST(ttl_expressions);
    RUN_TEST(complex_expression);
    RUN_TEST(error_handling);
    RUN_TEST(whitespace_handling);
    RUN_TEST(peek_functionality);
    RUN_TEST(time_conversion);
    
    /* Summary */
    printf("\n=== Test Summary ===\n");
    printf("Total tests: %d\n", tests_run);
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    
    return tests_failed > 0 ? 1 : 0;
}