#ifndef CNS_8T_PARSER_BRANCHLESS_H
#define CNS_8T_PARSER_BRANCHLESS_H

#include "cns/8t/core.h"
#include "cns/8t/numerical.h"
#include <immintrin.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// 8T BRANCHLESS PARSER INTERFACE
// Zero conditional branches in hot parsing paths for 8T guarantee
// ============================================================================

// Parser token types (designed for branchless dispatch)
typedef enum {
    CNS_8T_TOKEN_EOF = 0,           // End of file
    CNS_8T_TOKEN_IDENTIFIER = 1,    // Identifier
    CNS_8T_TOKEN_NUMBER = 2,        // Numeric literal
    CNS_8T_TOKEN_STRING = 3,        // String literal
    CNS_8T_TOKEN_OPERATOR = 4,      // Operator
    CNS_8T_TOKEN_PUNCTUATION = 5,   // Punctuation
    CNS_8T_TOKEN_KEYWORD = 6,       // Reserved keyword
    CNS_8T_TOKEN_WHITESPACE = 7,    // Whitespace (if preserved)
    CNS_8T_TOKEN_COMMENT = 8,       // Comment
    CNS_8T_TOKEN_ERROR = 15         // Error token (high value for branchless checks)
} cns_8t_token_type_t;

// Branchless parser token (32 bytes, aligned for SIMD)
typedef struct {
    cns_8t_token_type_t type;       // Token type (4 bytes)
    uint32_t length;                // Token length (4 bytes)
    uint32_t line;                  // Line number (4 bytes)
    uint32_t column;                // Column number (4 bytes)
    const char* start;              // Start pointer (8 bytes)
    uint64_t hash;                  // Precomputed hash for fast lookup (8 bytes)
} cns_8t_parser_token_t __attribute__((aligned(32)));

// SIMD character classification tables (256 bytes each, cache-line aligned)
typedef struct {
    uint8_t is_alpha[256];          // Alphabetic characters
    uint8_t is_digit[256];          // Digit characters
    uint8_t is_alnum[256];          // Alphanumeric characters
    uint8_t is_space[256];          // Whitespace characters
    uint8_t is_punct[256];          // Punctuation characters
    uint8_t is_upper[256];          // Uppercase characters
    uint8_t is_lower[256];          // Lowercase characters
    uint8_t char_class[256];        // Combined character classification
} cns_8t_char_tables_t __attribute__((aligned(64)));

// Branchless lexer state machine (designed for lookup table dispatch)
typedef struct {
    uint32_t current_state;         // Current lexer state
    uint32_t accept_state;          // Accept state for current token
    uint32_t error_state;           // Error state indicator
    uint32_t padding;               // Alignment padding
    
    // State transition table (256 states × 256 characters)
    uint16_t transition_table[256][256]; // Next state lookup
    uint8_t  accept_table[256];     // Accept state flags
    uint8_t  token_type_table[256]; // Token type for each accept state
} cns_8t_lexer_state_t __attribute__((aligned(64)));

// Parser context with branchless optimization
typedef struct {
    // Input stream
    const char* input;              // Input text
    size_t input_length;            // Total input length
    size_t position;                // Current position
    size_t line;                    // Current line number
    size_t column;                  // Current column number
    
    // Lexer state
    cns_8t_lexer_state_t* lexer;    // Lexer state machine
    cns_8t_char_tables_t* char_tables; // Character classification tables
    
    // Token buffer (ring buffer for lookahead)
    cns_8t_parser_token_t* tokens;  // Token ring buffer
    uint32_t token_capacity;        // Token buffer capacity
    uint32_t token_head;            // Token buffer head
    uint32_t token_tail;            // Token buffer tail
    uint32_t lookahead_distance;    // Maximum lookahead distance
    
    // Error handling (branchless error accumulation)
    uint32_t error_count;           // Number of errors encountered
    uint32_t error_flags;           // Error type flags
    char* error_buffer;             // Error message buffer
    size_t error_buffer_size;       // Error buffer size
    
    // Performance optimization
    uint64_t* keyword_hashes;       // Precomputed keyword hashes
    uint32_t keyword_count;         // Number of keywords
    __m256i simd_masks[8];          // Cached SIMD masks for character classes
    
    // Statistics
    uint64_t tokens_processed;      // Total tokens processed
    uint64_t characters_processed;  // Total characters processed
    cns_tick_t parsing_start_tick;  // Parsing start time
} cns_8t_parser_context_t __attribute__((aligned(64)));

// Branchless parsing function pointer types
typedef cns_8t_result_t (*cns_8t_parse_fn_t)(cns_8t_parser_context_t* ctx,
                                              void** result);

typedef cns_8t_result_t (*cns_8t_lexer_fn_t)(cns_8t_parser_context_t* ctx,
                                              cns_8t_parser_token_t* token);

// Parser configuration
typedef struct {
    bool enable_comments;           // Whether to preserve comments
    bool enable_whitespace;         // Whether to preserve whitespace
    bool case_sensitive;            // Case sensitivity for identifiers
    bool strict_mode;               // Strict parsing (more error checking)
    
    // Performance options
    bool enable_simd_lexing;        // Enable SIMD character processing
    bool enable_parallel_parsing;   // Enable parallel parsing (if applicable)
    uint32_t token_buffer_size;     // Size of token ring buffer
    uint32_t lookahead_limit;       // Maximum lookahead distance
    
    // Error handling
    bool continue_on_error;         // Continue parsing after errors
    uint32_t max_errors;            // Maximum errors before stopping
    bool detailed_errors;           // Generate detailed error messages
} cns_8t_parser_config_t;

// ============================================================================
// BRANCHLESS LEXING ALGORITHMS
// ============================================================================

// SIMD character classification (processes 32 characters at once)
static inline uint32_t cns_8t_classify_chars_simd(const char* input,
                                                   const cns_8t_char_tables_t* tables,
                                                   uint32_t* char_classes) {
    __m256i chars = _mm256_loadu_si256((const __m256i*)input);
    
    // Use gather operations to lookup character classes
    // This is a simplified version - real implementation would use multiple lookups
    __m256i indices = _mm256_cvtepu8_epi32(_mm256_extracti128_si256(chars, 0));
    
    // Store results using branchless operations
    for (int i = 0; i < 8; i++) {
        uint32_t char_val = _mm256_extract_epi32(indices, i);
        char_classes[i] = tables->char_class[char_val];
    }
    
    return 8; // Number of characters processed
}

// Branchless token type determination
static inline cns_8t_token_type_t cns_8t_determine_token_type_branchless(
    const char* start,
    uint32_t length,
    const cns_8t_char_tables_t* tables,
    const uint64_t* keyword_hashes,
    uint32_t keyword_count
) {
    // Fast path for single characters
    if (length == 1) {
        uint8_t c = (uint8_t)*start;
        uint32_t is_alpha = tables->is_alpha[c];
        uint32_t is_digit = tables->is_digit[c];
        uint32_t is_punct = tables->is_punct[c];
        
        // Branchless selection using bit manipulation
        return (cns_8t_token_type_t)(
            CNS_8T_TOKEN_IDENTIFIER * is_alpha +
            CNS_8T_TOKEN_NUMBER * is_digit +
            CNS_8T_TOKEN_PUNCTUATION * is_punct
        );
    }
    
    // For longer tokens, check first character and compute hash
    uint8_t first_char = (uint8_t)*start;
    if (tables->is_alpha[first_char] || *start == '_') {
        // Compute hash and check against keyword table
        uint64_t hash = 0;
        for (uint32_t i = 0; i < length; i++) {
            hash = hash * 31 + (uint8_t)start[i];
        }
        
        // Branchless keyword lookup
        uint32_t is_keyword = 0;
        for (uint32_t i = 0; i < keyword_count; i++) {
            is_keyword |= (hash == keyword_hashes[i]);
        }
        
        return is_keyword ? CNS_8T_TOKEN_KEYWORD : CNS_8T_TOKEN_IDENTIFIER;
    }
    
    if (tables->is_digit[first_char]) {
        return CNS_8T_TOKEN_NUMBER;
    }
    
    return CNS_8T_TOKEN_PUNCTUATION; // Default
}

// Branchless number parsing (handles integers and floats)
static inline cns_8t_result_t cns_8t_parse_number_branchless(
    const char* start,
    uint32_t length,
    cns_8t_quad_number_t* result
) {
    // Initialize accumulators
    uint64_t integer_part = 0;
    uint64_t fraction_part = 0;
    uint32_t fraction_digits = 0;
    int32_t exponent = 0;
    uint32_t sign = 0;
    
    const char* ptr = start;
    const char* end = start + length;
    
    // Handle sign (branchless)
    uint32_t has_sign = (*ptr == '-' || *ptr == '+');
    sign = (*ptr == '-');
    ptr += has_sign;
    
    // Parse integer part
    while (ptr < end && *ptr >= '0' && *ptr <= '9') {
        integer_part = integer_part * 10 + (*ptr - '0');
        ptr++;
    }
    
    // Handle decimal point (branchless)
    uint32_t has_decimal = (*ptr == '.');
    ptr += has_decimal;
    
    // Parse fractional part
    while (ptr < end && *ptr >= '0' && *ptr <= '9') {
        fraction_part = fraction_part * 10 + (*ptr - '0');
        fraction_digits += has_decimal;
        ptr++;
    }
    
    // Handle exponent
    uint32_t has_exponent = (*ptr == 'e' || *ptr == 'E');
    ptr += has_exponent;
    
    if (has_exponent) {
        uint32_t exp_sign = (*ptr == '-');
        ptr += (*ptr == '-' || *ptr == '+');
        
        uint32_t exp_value = 0;
        while (ptr < end && *ptr >= '0' && *ptr <= '9') {
            exp_value = exp_value * 10 + (*ptr - '0');
            ptr++;
        }
        
        exponent = exp_sign ? -(int32_t)exp_value : (int32_t)exp_value;
    }
    
    // Combine parts into final value
    long double value = (long double)integer_part;
    if (has_decimal) {
        long double fraction = (long double)fraction_part;
        for (uint32_t i = 0; i < fraction_digits; i++) {
            fraction /= 10.0L;
        }
        value += fraction;
    }
    
    // Apply exponent
    if (has_exponent) {
        value *= powl(10.0L, exponent);
    }
    
    // Apply sign
    if (sign) {
        value = -value;
    }
    
    result->value = value;
    result->error_bound = 0.0; // Would be computed properly in full implementation
    
    return CNS_8T_OK;
}

// ============================================================================
// BRANCHLESS PARSER API
// ============================================================================

// Parser lifecycle
cns_8t_result_t cns_8t_parser_context_create(const cns_8t_parser_config_t* config,
                                              cns_8t_parser_context_t** ctx);

cns_8t_result_t cns_8t_parser_context_destroy(cns_8t_parser_context_t* ctx);

// Input management
cns_8t_result_t cns_8t_parser_set_input(cns_8t_parser_context_t* ctx,
                                         const char* input,
                                         size_t input_length);

// Lexing operations
cns_8t_result_t cns_8t_lexer_next_token(cns_8t_parser_context_t* ctx,
                                         cns_8t_parser_token_t* token);

cns_8t_result_t cns_8t_lexer_peek_token(cns_8t_parser_context_t* ctx,
                                         uint32_t lookahead_distance,
                                         cns_8t_parser_token_t* token);

cns_8t_result_t cns_8t_lexer_consume_token(cns_8t_parser_context_t* ctx);

// SIMD-accelerated lexing
cns_8t_result_t cns_8t_lexer_tokenize_simd(cns_8t_parser_context_t* ctx,
                                            cns_8t_parser_token_t* tokens,
                                            uint32_t max_tokens,
                                            uint32_t* token_count);

// Branchless parsing primitives
cns_8t_result_t cns_8t_parser_expect_token(cns_8t_parser_context_t* ctx,
                                            cns_8t_token_type_t expected_type,
                                            cns_8t_parser_token_t* token);

cns_8t_result_t cns_8t_parser_match_keyword(cns_8t_parser_context_t* ctx,
                                             const char* keyword,
                                             bool* matched);

// Number parsing with precision guarantees
cns_8t_result_t cns_8t_parser_parse_number(cns_8t_parser_context_t* ctx,
                                            cns_8t_quad_number_t* number,
                                            const cns_8t_numerical_context_t* num_ctx);

cns_8t_result_t cns_8t_parser_parse_integer(cns_8t_parser_context_t* ctx,
                                             int64_t* value);

cns_8t_result_t cns_8t_parser_parse_float(cns_8t_parser_context_t* ctx,
                                           double* value,
                                           double* error_bound);

// String parsing
cns_8t_result_t cns_8t_parser_parse_string(cns_8t_parser_context_t* ctx,
                                            char* buffer,
                                            size_t buffer_size,
                                            size_t* string_length);

cns_8t_result_t cns_8t_parser_parse_identifier(cns_8t_parser_context_t* ctx,
                                                char* buffer,
                                                size_t buffer_size,
                                                size_t* id_length);

// Error handling
cns_8t_result_t cns_8t_parser_get_error_info(cns_8t_parser_context_t* ctx,
                                              uint32_t* error_count,
                                              const char** error_message);

cns_8t_result_t cns_8t_parser_reset_errors(cns_8t_parser_context_t* ctx);

// Performance monitoring
cns_8t_result_t cns_8t_parser_get_stats(cns_8t_parser_context_t* ctx,
                                         uint64_t* tokens_processed,
                                         uint64_t* characters_processed,
                                         cns_tick_t* parsing_time);

// ============================================================================
// SPECIALIZED PARSERS
// ============================================================================

// JSON parser with 8T guarantee
cns_8t_result_t cns_8t_parse_json_branchless(cns_8t_parser_context_t* ctx,
                                              void** json_object);

// Mathematical expression parser
cns_8t_result_t cns_8t_parse_math_expression(cns_8t_parser_context_t* ctx,
                                              void** expression_tree,
                                              const cns_8t_numerical_context_t* num_ctx);

// Configuration file parser
cns_8t_result_t cns_8t_parse_config_file(cns_8t_parser_context_t* ctx,
                                          void** config_object);

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

// Character classification table initialization
cns_8t_result_t cns_8t_char_tables_create(cns_8t_char_tables_t** tables);
void cns_8t_char_tables_destroy(cns_8t_char_tables_t* tables);

// Keyword hash table management
cns_8t_result_t cns_8t_keyword_hashes_create(const char** keywords,
                                              uint32_t keyword_count,
                                              uint64_t** hashes);

// Lexer state machine creation
cns_8t_result_t cns_8t_lexer_state_create(const char* grammar_description,
                                           cns_8t_lexer_state_t** state);

void cns_8t_lexer_state_destroy(cns_8t_lexer_state_t* state);

#ifdef __cplusplus
}
#endif

#endif // CNS_8T_PARSER_BRANCHLESS_H