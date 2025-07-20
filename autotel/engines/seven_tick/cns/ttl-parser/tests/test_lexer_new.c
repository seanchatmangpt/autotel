#include "test_utils.h"
#include "../src/lexer.h"

// Test token creation and basic functionality
int test_token_creation() {
    Token token;
    init_token(&token, TOKEN_PREFIX, "@prefix", 1, 1);
    
    TEST_ASSERT_EQ(token.type, TOKEN_PREFIX);
    TEST_ASSERT_STR_EQ(token.value, "@prefix");
    TEST_ASSERT_EQ(token.line, 1);
    TEST_ASSERT_EQ(token.column, 1);
    
    free_token(&token);
    return 1;
}

// Test basic lexer initialization
int test_lexer_init() {
    const char* input = "@prefix ex: <http://example.org/> .";
    
    Lexer lexer;
    int result = init_lexer(&lexer, input);
    TEST_ASSERT(result == 1);
    TEST_ASSERT_NOT_NULL(lexer.input);
    TEST_ASSERT_EQ(lexer.position, 0);
    TEST_ASSERT_EQ(lexer.line, 1);
    TEST_ASSERT_EQ(lexer.column, 1);
    
    free_lexer(&lexer);
    return 1;
}

// Test prefix directive tokenization
int test_tokenize_prefix() {
    const char* input = "@prefix ex: <http://example.org/> .";
    
    Lexer lexer;
    init_lexer(&lexer, input);
    
    Token token;
    
    // @prefix
    int result = next_token(&lexer, &token);
    TEST_ASSERT(result == 1);
    TEST_ASSERT_EQ(token.type, TOKEN_PREFIX);
    TEST_ASSERT_STR_EQ(token.value, "@prefix");
    free_token(&token);
    
    // ex:
    result = next_token(&lexer, &token);
    TEST_ASSERT(result == 1);
    TEST_ASSERT_EQ(token.type, TOKEN_PREFIXED_NAME);
    TEST_ASSERT_STR_EQ(token.value, "ex:");
    free_token(&token);
    
    // <http://example.org/>
    result = next_token(&lexer, &token);
    TEST_ASSERT(result == 1);
    TEST_ASSERT_EQ(token.type, TOKEN_URI);
    TEST_ASSERT_STR_EQ(token.value, "http://example.org/");
    free_token(&token);
    
    // .
    result = next_token(&lexer, &token);
    TEST_ASSERT(result == 1);
    TEST_ASSERT_EQ(token.type, TOKEN_DOT);
    TEST_ASSERT_STR_EQ(token.value, ".");
    free_token(&token);
    
    // EOF
    result = next_token(&lexer, &token);
    TEST_ASSERT(result == 1);
    TEST_ASSERT_EQ(token.type, TOKEN_EOF);
    free_token(&token);
    
    free_lexer(&lexer);
    return 1;
}

// Test URI tokenization
int test_tokenize_uri() {
    const char* input = "<http://example.org/resource>";
    
    Lexer lexer;
    init_lexer(&lexer, input);
    
    Token token;
    int result = next_token(&lexer, &token);
    TEST_ASSERT(result == 1);
    TEST_ASSERT_EQ(token.type, TOKEN_URI);
    TEST_ASSERT_STR_EQ(token.value, "http://example.org/resource");
    
    free_token(&token);
    free_lexer(&lexer);
    return 1;
}

// Test string literal tokenization
int test_tokenize_string() {
    const char* input = "\"Hello, World!\"";
    
    Lexer lexer;
    init_lexer(&lexer, input);
    
    Token token;
    int result = next_token(&lexer, &token);
    TEST_ASSERT(result == 1);
    TEST_ASSERT_EQ(token.type, TOKEN_STRING);
    TEST_ASSERT_STR_EQ(token.value, "Hello, World!");
    
    free_token(&token);
    free_lexer(&lexer);
    return 1;
}

// Test string with escapes
int test_tokenize_string_escapes() {
    const char* input = "\"Line 1\\nLine 2\\tTabbed\"";
    
    Lexer lexer;
    init_lexer(&lexer, input);
    
    Token token;
    int result = next_token(&lexer, &token);
    TEST_ASSERT(result == 1);
    TEST_ASSERT_EQ(token.type, TOKEN_STRING);
    TEST_ASSERT_STR_EQ(token.value, "Line 1\nLine 2\tTabbed");
    
    free_token(&token);
    free_lexer(&lexer);
    return 1;
}

// Test integer tokenization
int test_tokenize_integer() {
    const char* input = "42 -17 0 999999";
    
    Lexer lexer;
    init_lexer(&lexer, input);
    
    Token token;
    
    // 42
    int result = next_token(&lexer, &token);
    TEST_ASSERT(result == 1);
    TEST_ASSERT_EQ(token.type, TOKEN_INTEGER);
    TEST_ASSERT_STR_EQ(token.value, "42");
    free_token(&token);
    
    // -17
    result = next_token(&lexer, &token);
    TEST_ASSERT(result == 1);
    TEST_ASSERT_EQ(token.type, TOKEN_INTEGER);
    TEST_ASSERT_STR_EQ(token.value, "-17");
    free_token(&token);
    
    // 0
    result = next_token(&lexer, &token);
    TEST_ASSERT(result == 1);
    TEST_ASSERT_EQ(token.type, TOKEN_INTEGER);
    TEST_ASSERT_STR_EQ(token.value, "0");
    free_token(&token);
    
    // 999999
    result = next_token(&lexer, &token);
    TEST_ASSERT(result == 1);
    TEST_ASSERT_EQ(token.type, TOKEN_INTEGER);
    TEST_ASSERT_STR_EQ(token.value, "999999");
    free_token(&token);
    
    free_lexer(&lexer);
    return 1;
}

// Test decimal tokenization
int test_tokenize_decimal() {
    const char* input = "3.14 -2.5 0.001 123.456789";
    
    Lexer lexer;
    init_lexer(&lexer, input);
    
    Token token;
    
    // 3.14
    int result = next_token(&lexer, &token);
    TEST_ASSERT(result == 1);
    TEST_ASSERT_EQ(token.type, TOKEN_DECIMAL);
    TEST_ASSERT_STR_EQ(token.value, "3.14");
    free_token(&token);
    
    // -2.5
    result = next_token(&lexer, &token);
    TEST_ASSERT(result == 1);
    TEST_ASSERT_EQ(token.type, TOKEN_DECIMAL);
    TEST_ASSERT_STR_EQ(token.value, "-2.5");
    free_token(&token);
    
    free_lexer(&lexer);
    return 1;
}

// Test blank node tokenization
int test_tokenize_blank_node() {
    const char* input = "_:blank1 []";
    
    Lexer lexer;
    init_lexer(&lexer, input);
    
    Token token;
    
    // _:blank1
    int result = next_token(&lexer, &token);
    TEST_ASSERT(result == 1);
    TEST_ASSERT_EQ(token.type, TOKEN_BLANK_NODE);
    TEST_ASSERT_STR_EQ(token.value, "_:blank1");
    free_token(&token);
    
    // []
    result = next_token(&lexer, &token);
    TEST_ASSERT(result == 1);
    TEST_ASSERT_EQ(token.type, TOKEN_ANON);
    TEST_ASSERT_STR_EQ(token.value, "[]");
    free_token(&token);
    
    free_lexer(&lexer);
    return 1;
}

// Test punctuation tokenization
int test_tokenize_punctuation() {
    const char* input = ". ; , ( ) [ ]";
    
    Lexer lexer;
    init_lexer(&lexer, input);
    
    Token token;
    TokenType expected[] = {
        TOKEN_DOT, TOKEN_SEMICOLON, TOKEN_COMMA,
        TOKEN_LPAREN, TOKEN_RPAREN, TOKEN_LBRACKET, TOKEN_RBRACKET
    };
    
    for (int i = 0; i < 7; i++) {
        int result = next_token(&lexer, &token);
        TEST_ASSERT(result == 1);
        TEST_ASSERT_EQ(token.type, expected[i]);
        free_token(&token);
    }
    
    free_lexer(&lexer);
    return 1;
}

// Test 'a' keyword (rdf:type shorthand)
int test_tokenize_a_keyword() {
    const char* input = "ex:alice a ex:Person";
    
    Lexer lexer;
    init_lexer(&lexer, input);
    
    Token token;
    
    // ex:alice
    next_token(&lexer, &token);
    TEST_ASSERT_EQ(token.type, TOKEN_PREFIXED_NAME);
    free_token(&token);
    
    // a
    next_token(&lexer, &token);
    TEST_ASSERT_EQ(token.type, TOKEN_A);
    TEST_ASSERT_STR_EQ(token.value, "a");
    free_token(&token);
    
    // ex:Person
    next_token(&lexer, &token);
    TEST_ASSERT_EQ(token.type, TOKEN_PREFIXED_NAME);
    free_token(&token);
    
    free_lexer(&lexer);
    return 1;
}

// Test comment handling
int test_tokenize_comments() {
    const char* input = 
        "# This is a comment\n"
        "@prefix ex: <http://example.org/> . # Another comment\n"
        "ex:subject ex:predicate ex:object .";
    
    Lexer lexer;
    init_lexer(&lexer, input);
    
    Token token;
    
    // Should skip comments and get @prefix
    int result = next_token(&lexer, &token);
    TEST_ASSERT(result == 1);
    TEST_ASSERT_EQ(token.type, TOKEN_PREFIX);
    free_token(&token);
    
    // Continue parsing after comments
    next_token(&lexer, &token); // ex:
    TEST_ASSERT_EQ(token.type, TOKEN_PREFIXED_NAME);
    free_token(&token);
    
    free_lexer(&lexer);
    return 1;
}

// Test whitespace handling
int test_tokenize_whitespace() {
    const char* input = "  \t\n @prefix   \t  ex:  \n  <http://example.org/>  \t .  ";
    
    Lexer lexer;
    init_lexer(&lexer, input);
    
    Token token;
    
    // Should skip all whitespace and get @prefix
    int result = next_token(&lexer, &token);
    TEST_ASSERT(result == 1);
    TEST_ASSERT_EQ(token.type, TOKEN_PREFIX);
    free_token(&token);
    
    // ex:
    result = next_token(&lexer, &token);
    TEST_ASSERT(result == 1);
    TEST_ASSERT_EQ(token.type, TOKEN_PREFIXED_NAME);
    free_token(&token);
    
    free_lexer(&lexer);
    return 1;
}

// Test error on invalid input
int test_tokenize_error() {
    const char* input = "@prefix ex: <unclosed URI";
    
    Lexer lexer;
    init_lexer(&lexer, input);
    
    Token token;
    
    // @prefix should work
    next_token(&lexer, &token);
    TEST_ASSERT_EQ(token.type, TOKEN_PREFIX);
    free_token(&token);
    
    // ex: should work
    next_token(&lexer, &token);
    TEST_ASSERT_EQ(token.type, TOKEN_PREFIXED_NAME);
    free_token(&token);
    
    // Unclosed URI should cause error
    int result = next_token(&lexer, &token);
    TEST_ASSERT(result == 0); // Should fail
    TEST_ASSERT(has_error(&lexer.error));
    
    free_lexer(&lexer);
    return 1;
}

// Test line and column tracking
int test_position_tracking() {
    const char* input = "@prefix ex:\n<http://example.org/>\n.";
    
    Lexer lexer;
    init_lexer(&lexer, input);
    
    Token token;
    
    // @prefix at line 1, column 1
    next_token(&lexer, &token);
    TEST_ASSERT_EQ(token.line, 1);
    TEST_ASSERT_EQ(token.column, 1);
    free_token(&token);
    
    // ex: at line 1, column 9
    next_token(&lexer, &token);
    TEST_ASSERT_EQ(token.line, 1);
    TEST_ASSERT_EQ(token.column, 9);
    free_token(&token);
    
    // URI at line 2, column 1
    next_token(&lexer, &token);
    TEST_ASSERT_EQ(token.line, 2);
    TEST_ASSERT_EQ(token.column, 1);
    free_token(&token);
    
    // . at line 3, column 1
    next_token(&lexer, &token);
    TEST_ASSERT_EQ(token.line, 3);
    TEST_ASSERT_EQ(token.column, 1);
    free_token(&token);
    
    free_lexer(&lexer);
    return 1;
}

// Test base directive
int test_tokenize_base() {
    const char* input = "@base <http://example.org/>";
    
    Lexer lexer;
    init_lexer(&lexer, input);
    
    Token token;
    
    // @base
    int result = next_token(&lexer, &token);
    TEST_ASSERT(result == 1);
    TEST_ASSERT_EQ(token.type, TOKEN_BASE);
    TEST_ASSERT_STR_EQ(token.value, "@base");
    free_token(&token);
    
    // URI
    result = next_token(&lexer, &token);
    TEST_ASSERT(result == 1);
    TEST_ASSERT_EQ(token.type, TOKEN_URI);
    free_token(&token);
    
    free_lexer(&lexer);
    return 1;
}

// Test Unicode handling
int test_tokenize_unicode() {
    const char* input = "\"Hello 世界 🌍\"";
    
    Lexer lexer;
    init_lexer(&lexer, input);
    
    Token token;
    int result = next_token(&lexer, &token);
    TEST_ASSERT(result == 1);
    TEST_ASSERT_EQ(token.type, TOKEN_STRING);
    TEST_ASSERT_STR_EQ(token.value, "Hello 世界 🌍");
    
    free_token(&token);
    free_lexer(&lexer);
    return 1;
}

// Test peek functionality
int test_lexer_peek() {
    const char* input = "ex:subject ex:predicate";
    
    Lexer lexer;
    init_lexer(&lexer, input);
    
    Token token1, token2;
    
    // Peek should not advance position
    int result = peek_token(&lexer, &token1);
    TEST_ASSERT(result == 1);
    TEST_ASSERT_EQ(token1.type, TOKEN_PREFIXED_NAME);
    
    // Next should return same token
    result = next_token(&lexer, &token2);
    TEST_ASSERT(result == 1);
    TEST_ASSERT_EQ(token2.type, TOKEN_PREFIXED_NAME);
    TEST_ASSERT_STR_EQ(token1.value, token2.value);
    
    free_token(&token1);
    free_token(&token2);
    free_lexer(&lexer);
    return 1;
}

// Performance test - large input
int test_lexer_performance() {
    printf("    Generating large TTL input...\n");
    
    // Generate large input
    const int num_statements = 1000;
    size_t input_size = num_statements * 100;
    char* input = TEST_MALLOC(input_size);
    TEST_ASSERT_NOT_NULL(input);
    
    strcpy(input, "@prefix ex: <http://example.org/> .\n");
    char* pos = input + strlen(input);
    
    for (int i = 0; i < num_statements; i++) {
        sprintf(pos, "ex:subject%d ex:predicate%d \"Object %d\" .\n", i, i, i);
        pos += strlen(pos);
    }
    
    printf("    Tokenizing large input (%.1f KB)...\n", strlen(input) / 1024.0);
    
    clock_t start = clock();
    
    Lexer lexer;
    init_lexer(&lexer, input);
    
    Token token;
    int token_count = 0;
    
    while (next_token(&lexer, &token) && token.type != TOKEN_EOF) {
        token_count++;
        free_token(&token);
    }
    
    clock_t end = clock();
    double elapsed = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    printf("    Tokenized %d tokens in %.3f seconds (%.0f tokens/sec)\n",
           token_count, elapsed, token_count / elapsed);
    
    free_lexer(&lexer);
    TEST_FREE(input);
    
    return 1;
}

// Main test suite runner
void run_lexer_tests() {
    RUN_TEST_SUITE("Lexer", () => {
        RUN_TEST(test_token_creation);
        RUN_TEST(test_lexer_init);
        RUN_TEST(test_tokenize_prefix);
        RUN_TEST(test_tokenize_uri);
        RUN_TEST(test_tokenize_string);
        RUN_TEST(test_tokenize_string_escapes);
        RUN_TEST(test_tokenize_integer);
        RUN_TEST(test_tokenize_decimal);
        RUN_TEST(test_tokenize_blank_node);
        RUN_TEST(test_tokenize_punctuation);
        RUN_TEST(test_tokenize_a_keyword);
        RUN_TEST(test_tokenize_comments);
        RUN_TEST(test_tokenize_whitespace);
        RUN_TEST(test_tokenize_error);
        RUN_TEST(test_position_tracking);
        RUN_TEST(test_tokenize_base);
        RUN_TEST(test_tokenize_unicode);
        RUN_TEST(test_lexer_peek);
        RUN_TEST(test_lexer_performance);
    });
}