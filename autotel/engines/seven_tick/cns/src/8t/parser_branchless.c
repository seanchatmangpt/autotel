/**
 * @file parser_branchless.c
 * @brief Branchless parser with state machines for 8T substrate
 * 
 * Implements branch-free parsing using lookup tables, bit manipulation,
 * and SIMD operations to minimize CPU pipeline stalls and maintain
 * 8-tick performance constraints.
 */

#include "cns/8t/core.h"
#include "cns/parser.h"
#include <stdlib.h>
#include <string.h>
#include <immintrin.h>

// ============================================================================
// BRANCHLESS PARSER CONSTANTS
// ============================================================================

// Character classes for branchless classification
#define CNS_8T_CHAR_DIGIT       0x01
#define CNS_8T_CHAR_ALPHA       0x02
#define CNS_8T_CHAR_WHITESPACE  0x04
#define CNS_8T_CHAR_DELIMITER   0x08
#define CNS_8T_CHAR_OPERATOR    0x10
#define CNS_8T_CHAR_QUOTE       0x20
#define CNS_8T_CHAR_ESCAPE      0x40
#define CNS_8T_CHAR_NEWLINE     0x80

// Parser states (power of 2 for fast bit operations)
#define CNS_8T_STATE_INITIAL    0x00
#define CNS_8T_STATE_IDENTIFIER 0x01
#define CNS_8T_STATE_NUMBER     0x02
#define CNS_8T_STATE_STRING     0x04
#define CNS_8T_STATE_COMMENT    0x08
#define CNS_8T_STATE_OPERATOR   0x10
#define CNS_8T_STATE_ERROR      0x20

// Token types
#define CNS_8T_TOKEN_EOF        0x00
#define CNS_8T_TOKEN_IDENTIFIER 0x01
#define CNS_8T_TOKEN_NUMBER     0x02
#define CNS_8T_TOKEN_STRING     0x04
#define CNS_8T_TOKEN_OPERATOR   0x08
#define CNS_8T_TOKEN_DELIMITER  0x10
#define CNS_8T_TOKEN_KEYWORD    0x20
#define CNS_8T_TOKEN_ERROR      0x40

// ============================================================================
// BRANCHLESS PARSER STRUCTURES
// ============================================================================

// Character classification table (256 entries, cache-friendly)
static const uint8_t char_class_table[256] __attribute__((aligned(64))) = {
    // Control characters (0-31)
    [0 ... 31] = 0,
    ['\t'] = CNS_8T_CHAR_WHITESPACE,
    ['\n'] = CNS_8T_CHAR_WHITESPACE | CNS_8T_CHAR_NEWLINE,
    ['\r'] = CNS_8T_CHAR_WHITESPACE,
    
    // Printable characters (32-126)
    [' '] = CNS_8T_CHAR_WHITESPACE,
    ['!'] = CNS_8T_CHAR_OPERATOR,
    ['"'] = CNS_8T_CHAR_QUOTE,
    ['#'] = CNS_8T_CHAR_OPERATOR,
    ['$'] = CNS_8T_CHAR_OPERATOR,
    ['%'] = CNS_8T_CHAR_OPERATOR,
    ['&'] = CNS_8T_CHAR_OPERATOR,
    ['\''] = CNS_8T_CHAR_QUOTE,
    ['('] = CNS_8T_CHAR_DELIMITER,
    [')'] = CNS_8T_CHAR_DELIMITER,
    ['*'] = CNS_8T_CHAR_OPERATOR,
    ['+'] = CNS_8T_CHAR_OPERATOR,
    [','] = CNS_8T_CHAR_DELIMITER,
    ['-'] = CNS_8T_CHAR_OPERATOR,
    ['.'] = CNS_8T_CHAR_DELIMITER,
    ['/'] = CNS_8T_CHAR_OPERATOR,
    
    // Digits (48-57)
    ['0' ... '9'] = CNS_8T_CHAR_DIGIT,
    
    [':'] = CNS_8T_CHAR_DELIMITER,
    [';'] = CNS_8T_CHAR_DELIMITER,
    ['<'] = CNS_8T_CHAR_OPERATOR,
    ['='] = CNS_8T_CHAR_OPERATOR,
    ['>'] = CNS_8T_CHAR_OPERATOR,
    ['?'] = CNS_8T_CHAR_OPERATOR,
    ['@'] = CNS_8T_CHAR_OPERATOR,
    
    // Uppercase letters (65-90)
    ['A' ... 'Z'] = CNS_8T_CHAR_ALPHA,
    
    ['['] = CNS_8T_CHAR_DELIMITER,
    ['\\'] = CNS_8T_CHAR_ESCAPE,
    [']'] = CNS_8T_CHAR_DELIMITER,
    ['^'] = CNS_8T_CHAR_OPERATOR,
    ['_'] = CNS_8T_CHAR_ALPHA,
    ['`'] = CNS_8T_CHAR_QUOTE,
    
    // Lowercase letters (97-122)
    ['a' ... 'z'] = CNS_8T_CHAR_ALPHA,
    
    ['{'] = CNS_8T_CHAR_DELIMITER,
    ['|'] = CNS_8T_CHAR_OPERATOR,
    ['}'] = CNS_8T_CHAR_DELIMITER,
    ['~'] = CNS_8T_CHAR_OPERATOR,
    
    // Extended characters (128-255)
    [128 ... 255] = 0
};

// State transition table (branchless state machine)
static const uint8_t state_transition_table[8][8] __attribute__((aligned(64))) = {
    // Input classes: DIGIT, ALPHA, SPACE, DELIM, OPER, QUOTE, ESC, NL
    /* INITIAL */    {CNS_8T_STATE_NUMBER, CNS_8T_STATE_IDENTIFIER, CNS_8T_STATE_INITIAL, CNS_8T_STATE_INITIAL, CNS_8T_STATE_OPERATOR, CNS_8T_STATE_STRING, CNS_8T_STATE_ERROR, CNS_8T_STATE_INITIAL},
    /* IDENTIFIER */ {CNS_8T_STATE_IDENTIFIER, CNS_8T_STATE_IDENTIFIER, CNS_8T_STATE_INITIAL, CNS_8T_STATE_INITIAL, CNS_8T_STATE_OPERATOR, CNS_8T_STATE_STRING, CNS_8T_STATE_ERROR, CNS_8T_STATE_INITIAL},
    /* NUMBER */     {CNS_8T_STATE_NUMBER, CNS_8T_STATE_ERROR, CNS_8T_STATE_INITIAL, CNS_8T_STATE_INITIAL, CNS_8T_STATE_OPERATOR, CNS_8T_STATE_STRING, CNS_8T_STATE_ERROR, CNS_8T_STATE_INITIAL},
    /* STRING */     {CNS_8T_STATE_STRING, CNS_8T_STATE_STRING, CNS_8T_STATE_STRING, CNS_8T_STATE_STRING, CNS_8T_STATE_STRING, CNS_8T_STATE_INITIAL, CNS_8T_STATE_STRING, CNS_8T_STATE_STRING},
    /* COMMENT */    {CNS_8T_STATE_COMMENT, CNS_8T_STATE_COMMENT, CNS_8T_STATE_COMMENT, CNS_8T_STATE_COMMENT, CNS_8T_STATE_COMMENT, CNS_8T_STATE_COMMENT, CNS_8T_STATE_COMMENT, CNS_8T_STATE_INITIAL},
    /* OPERATOR */   {CNS_8T_STATE_NUMBER, CNS_8T_STATE_IDENTIFIER, CNS_8T_STATE_INITIAL, CNS_8T_STATE_INITIAL, CNS_8T_STATE_OPERATOR, CNS_8T_STATE_STRING, CNS_8T_STATE_ERROR, CNS_8T_STATE_INITIAL},
    /* ERROR */      {CNS_8T_STATE_ERROR, CNS_8T_STATE_ERROR, CNS_8T_STATE_ERROR, CNS_8T_STATE_ERROR, CNS_8T_STATE_ERROR, CNS_8T_STATE_ERROR, CNS_8T_STATE_ERROR, CNS_8T_STATE_ERROR},
    /* RESERVED */   {CNS_8T_STATE_ERROR, CNS_8T_STATE_ERROR, CNS_8T_STATE_ERROR, CNS_8T_STATE_ERROR, CNS_8T_STATE_ERROR, CNS_8T_STATE_ERROR, CNS_8T_STATE_ERROR, CNS_8T_STATE_ERROR}
};

// Token type mapping (branchless token classification)
static const uint8_t token_type_table[8] __attribute__((aligned(32))) = {
    CNS_8T_TOKEN_EOF,        // INITIAL
    CNS_8T_TOKEN_IDENTIFIER, // IDENTIFIER  
    CNS_8T_TOKEN_NUMBER,     // NUMBER
    CNS_8T_TOKEN_STRING,     // STRING
    CNS_8T_TOKEN_DELIMITER,  // COMMENT (becomes delimiter)
    CNS_8T_TOKEN_OPERATOR,   // OPERATOR
    CNS_8T_TOKEN_ERROR,      // ERROR
    CNS_8T_TOKEN_ERROR       // RESERVED
};

// Token structure optimized for cache alignment
typedef struct {
    uint32_t type;          // Token type
    uint32_t start_pos;     // Start position in input
    uint32_t length;        // Token length
    uint32_t line_number;   // Line number
    uint64_t hash;          // Fast hash for keyword lookup
    uint32_t flags;         // Token flags
    uint32_t padding;       // Padding to 32 bytes
} __attribute__((aligned(32))) cns_8t_token_t;

// Parser state structure (cache-line aligned)
typedef struct {
    // Hot data (first cache line)
    const char* input;          // Input text
    size_t input_length;        // Input length
    size_t position;            // Current position
    uint32_t current_state;     // Current parser state
    uint32_t line_number;       // Current line number
    uint32_t column_number;     // Current column number
    cns_8t_token_t* tokens;     // Token array
    uint32_t token_count;       // Number of tokens
    uint32_t token_capacity;    // Token array capacity
    uint32_t error_count;       // Number of errors
    
    // Cold data (second cache line)
    cns_8t_perf_metrics_t perf; // Performance metrics
    char error_message[32];     // Error message buffer
    uint32_t flags;             // Parser flags
    uint32_t padding[7];        // Padding to cache line
} __attribute__((aligned(64))) cns_8t_parser_state_t;

// ============================================================================
// BRANCHLESS UTILITY FUNCTIONS
// ============================================================================

/**
 * Branchless character classification
 */
static inline uint8_t cns_8t_classify_char(char c) {
    return char_class_table[(uint8_t)c];
}

/**
 * Branchless character class to index conversion
 */
static inline uint32_t cns_8t_char_class_to_index(uint8_t char_class) {
    // Use bit scanning to convert class bitmask to index
    static const uint8_t class_to_index[256] = {
        [CNS_8T_CHAR_DIGIT] = 0,
        [CNS_8T_CHAR_ALPHA] = 1,
        [CNS_8T_CHAR_WHITESPACE] = 2,
        [CNS_8T_CHAR_DELIMITER] = 3,
        [CNS_8T_CHAR_OPERATOR] = 4,
        [CNS_8T_CHAR_QUOTE] = 5,
        [CNS_8T_CHAR_ESCAPE] = 6,
        [CNS_8T_CHAR_NEWLINE] = 7
    };
    
    // Handle combined flags (whitespace + newline)
    if (char_class & CNS_8T_CHAR_NEWLINE) return 7;
    
    return class_to_index[char_class & 0x7F]; // Mask to handle single flags
}

/**
 * Fast hash function for keyword lookup (branchless)
 */
static inline uint64_t cns_8t_hash_token(const char* text, size_t length) {
    uint64_t hash = 14695981039346656037ULL; // FNV-1a offset basis
    
    // Process 8 bytes at a time using SIMD when possible
    size_t simd_len = length & ~7UL;
    for (size_t i = 0; i < simd_len; i += 8) {
        uint64_t chunk;
        memcpy(&chunk, &text[i], 8);
        hash ^= chunk;
        hash *= 1099511628211ULL; // FNV-1a prime
    }
    
    // Handle remaining bytes
    for (size_t i = simd_len; i < length; i++) {
        hash ^= (uint8_t)text[i];
        hash *= 1099511628211ULL;
    }
    
    return hash;
}

/**
 * SIMD-optimized character processing (process 32 chars at once)
 */
static inline void cns_8t_process_chars_simd(const char* input, size_t length,
                                             uint8_t* char_classes) {
    const size_t simd_len = length & ~31UL; // Process 32 chars at once
    
    for (size_t i = 0; i < simd_len; i += 32) {
        // Load 32 characters
        __m256i chars = _mm256_loadu_si256((const __m256i*)&input[i]);
        
        // Create character class masks
        __m256i digits = _mm256_and_si256(
            _mm256_cmpgt_epi8(chars, _mm256_set1_epi8('0' - 1)),
            _mm256_cmpgt_epi8(_mm256_set1_epi8('9' + 1), chars)
        );
        
        __m256i alpha_lower = _mm256_and_si256(
            _mm256_cmpgt_epi8(chars, _mm256_set1_epi8('a' - 1)),
            _mm256_cmpgt_epi8(_mm256_set1_epi8('z' + 1), chars)
        );
        
        __m256i alpha_upper = _mm256_and_si256(
            _mm256_cmpgt_epi8(chars, _mm256_set1_epi8('A' - 1)),
            _mm256_cmpgt_epi8(_mm256_set1_epi8('Z' + 1), chars)
        );
        
        __m256i whitespace = _mm256_or_si256(
            _mm256_cmpeq_epi8(chars, _mm256_set1_epi8(' ')),
            _mm256_or_si256(
                _mm256_cmpeq_epi8(chars, _mm256_set1_epi8('\t')),
                _mm256_cmpeq_epi8(chars, _mm256_set1_epi8('\n'))
            )
        );
        
        // Combine masks and store
        __m256i result = _mm256_or_si256(
            _mm256_and_si256(digits, _mm256_set1_epi8(CNS_8T_CHAR_DIGIT)),
            _mm256_or_si256(
                _mm256_and_si256(_mm256_or_si256(alpha_lower, alpha_upper), 
                                _mm256_set1_epi8(CNS_8T_CHAR_ALPHA)),
                _mm256_and_si256(whitespace, _mm256_set1_epi8(CNS_8T_CHAR_WHITESPACE))
            )
        );
        
        _mm256_storeu_si256((__m256i*)&char_classes[i], result);
        
        // Prefetch next cache line
        if (i + 64 < length) {
            __builtin_prefetch(&input[i + 64], 0, 3);
        }
    }
    
    // Handle remaining characters
    for (size_t i = simd_len; i < length; i++) {
        char_classes[i] = cns_8t_classify_char(input[i]);
    }
}

// ============================================================================
// BRANCHLESS PARSER IMPLEMENTATION
// ============================================================================

/**
 * Create branchless parser
 */
cns_8t_result_t cns_8t_parser_create(size_t initial_token_capacity,
                                     cns_parser_t** parser_out) {
    if (!parser_out || initial_token_capacity == 0) {
        return CNS_8T_ERROR_INVALID_PARAM;
    }
    
    cns_tick_t start_tick = cns_get_tick_count();
    
    // Allocate parser state
    cns_8t_parser_state_t* parser = (cns_8t_parser_state_t*)
        aligned_alloc(CNS_8T_ALIGNMENT, sizeof(cns_8t_parser_state_t));
    
    if (!parser) {
        return CNS_8T_ERROR_OUT_OF_MEMORY;
    }
    
    // Clear parser state
    memset(parser, 0, sizeof(cns_8t_parser_state_t));
    
    // Allocate token array
    parser->token_capacity = initial_token_capacity;
    parser->tokens = (cns_8t_token_t*)
        aligned_alloc(CNS_8T_ALIGNMENT, 
                     parser->token_capacity * sizeof(cns_8t_token_t));
    
    if (!parser->tokens) {
        free(parser);
        return CNS_8T_ERROR_OUT_OF_MEMORY;
    }
    
    // Initialize parser state
    parser->input = NULL;
    parser->input_length = 0;
    parser->position = 0;
    parser->current_state = CNS_8T_STATE_INITIAL;
    parser->line_number = 1;
    parser->column_number = 1;
    parser->token_count = 0;
    parser->error_count = 0;
    parser->flags = 0;
    
    // Initialize performance metrics
    memset(&parser->perf, 0, sizeof(parser->perf));
    parser->perf.start_tick = start_tick;
    
    *parser_out = (cns_parser_t*)parser;
    
    cns_tick_t elapsed = cns_get_tick_count() - start_tick;
    if (elapsed > CNS_8T_TICK_LIMIT) {
        return CNS_8T_ERROR_8T_VIOLATION;
    }
    
    return CNS_8T_OK;
}

/**
 * Branchless token parsing (main algorithm)
 */
cns_8t_result_t cns_8t_parser_parse(cns_parser_t* parser_handle,
                                    const char* input,
                                    size_t input_length) {
    cns_8t_parser_state_t* parser = (cns_8t_parser_state_t*)parser_handle;
    
    if (!parser || !input || input_length == 0) {
        return CNS_8T_ERROR_INVALID_PARAM;
    }
    
    CNS_8T_START_TIMING(&parser->perf);
    
    // Set input
    parser->input = input;
    parser->input_length = input_length;
    parser->position = 0;
    parser->token_count = 0;
    parser->error_count = 0;
    parser->current_state = CNS_8T_STATE_INITIAL;
    parser->line_number = 1;
    parser->column_number = 1;
    
    // Pre-classify all characters using SIMD
    uint8_t* char_classes = (uint8_t*)aligned_alloc(32, input_length);
    if (!char_classes) {
        CNS_8T_END_TIMING(&parser->perf);
        return CNS_8T_ERROR_OUT_OF_MEMORY;
    }
    
    cns_8t_process_chars_simd(input, input_length, char_classes);
    
    // Main parsing loop (branchless state machine)
    size_t token_start = 0;
    uint32_t prev_state = CNS_8T_STATE_INITIAL;
    
    for (size_t pos = 0; pos < input_length; pos++) {
        // Prefetch next character
        if (pos + 16 < input_length) {
            __builtin_prefetch(&input[pos + 16], 0, 3);
            __builtin_prefetch(&char_classes[pos + 16], 0, 3);
        }
        
        uint8_t char_class = char_classes[pos];
        uint32_t class_index = cns_8t_char_class_to_index(char_class);
        
        // Branchless state transition
        uint32_t state_index = 0;
        state_index += (parser->current_state == CNS_8T_STATE_INITIAL) ? 0 : 0;
        state_index += (parser->current_state == CNS_8T_STATE_IDENTIFIER) ? 1 : 0;
        state_index += (parser->current_state == CNS_8T_STATE_NUMBER) ? 2 : 0;
        state_index += (parser->current_state == CNS_8T_STATE_STRING) ? 3 : 0;
        state_index += (parser->current_state == CNS_8T_STATE_COMMENT) ? 4 : 0;
        state_index += (parser->current_state == CNS_8T_STATE_OPERATOR) ? 5 : 0;
        state_index += (parser->current_state == CNS_8T_STATE_ERROR) ? 6 : 0;
        
        uint32_t new_state = state_transition_table[state_index][class_index];
        
        // Check for token boundary (state change or end of input)
        uint32_t token_boundary = (new_state != parser->current_state) | 
                                 (pos == input_length - 1);
        
        // Emit token if we hit a boundary and have content
        if (token_boundary && (pos > token_start) && 
            (parser->current_state != CNS_8T_STATE_INITIAL)) {
            
            // Check token capacity
            if (parser->token_count >= parser->token_capacity) {
                // Expand token array
                uint32_t new_capacity = parser->token_capacity * 2;
                cns_8t_token_t* new_tokens = (cns_8t_token_t*)
                    aligned_alloc(CNS_8T_ALIGNMENT, 
                                 new_capacity * sizeof(cns_8t_token_t));
                
                if (!new_tokens) {
                    free(char_classes);
                    CNS_8T_END_TIMING(&parser->perf);
                    return CNS_8T_ERROR_OUT_OF_MEMORY;
                }
                
                memcpy(new_tokens, parser->tokens, 
                       parser->token_count * sizeof(cns_8t_token_t));
                
                free(parser->tokens);
                parser->tokens = new_tokens;
                parser->token_capacity = new_capacity;
            }
            
            // Create token
            cns_8t_token_t* token = &parser->tokens[parser->token_count];
            token->type = token_type_table[parser->current_state];
            token->start_pos = token_start;
            token->length = pos - token_start;
            token->line_number = parser->line_number;
            token->flags = 0;
            
            // Calculate hash for keyword lookup
            token->hash = cns_8t_hash_token(&input[token_start], token->length);
            
            parser->token_count++;
            parser->perf.operation_count++;
        }
        
        // Update state and position
        prev_state = parser->current_state;
        parser->current_state = new_state;
        
        // Update token start position on state change
        token_start = (new_state != prev_state) ? pos : token_start;
        
        // Update line/column tracking (branchless)
        uint32_t is_newline = (char_class & CNS_8T_CHAR_NEWLINE) != 0;
        parser->line_number += is_newline;
        parser->column_number = is_newline ? 1 : parser->column_number + 1;
        
        // Track errors
        parser->error_count += (new_state == CNS_8T_STATE_ERROR) ? 1 : 0;
    }
    
    free(char_classes);
    
    CNS_8T_END_TIMING(&parser->perf);
    
    return (parser->error_count > 0) ? CNS_8T_ERROR_PARSE_ERROR : CNS_8T_OK;
}

/**
 * Get token by index
 */
cns_8t_result_t cns_8t_parser_get_token(cns_parser_t* parser_handle,
                                        uint32_t token_index,
                                        cns_token_t* token_out) {
    cns_8t_parser_state_t* parser = (cns_8t_parser_state_t*)parser_handle;
    
    if (!parser || !token_out || token_index >= parser->token_count) {
        return CNS_8T_ERROR_INVALID_PARAM;
    }
    
    cns_8t_token_t* internal_token = &parser->tokens[token_index];
    
    // Convert internal token to public token format
    token_out->type = internal_token->type;
    token_out->start_pos = internal_token->start_pos;
    token_out->length = internal_token->length;
    token_out->line_number = internal_token->line_number;
    
    return CNS_8T_OK;
}

/**
 * Get total number of tokens
 */
uint32_t cns_8t_parser_get_token_count(cns_parser_t* parser_handle) {
    cns_8t_parser_state_t* parser = (cns_8t_parser_state_t*)parser_handle;
    return parser ? parser->token_count : 0;
}

/**
 * Reset parser state
 */
cns_8t_result_t cns_8t_parser_reset(cns_parser_t* parser_handle) {
    cns_8t_parser_state_t* parser = (cns_8t_parser_state_t*)parser_handle;
    
    if (!parser) {
        return CNS_8T_ERROR_INVALID_PARAM;
    }
    
    parser->input = NULL;
    parser->input_length = 0;
    parser->position = 0;
    parser->current_state = CNS_8T_STATE_INITIAL;
    parser->line_number = 1;
    parser->column_number = 1;
    parser->token_count = 0;
    parser->error_count = 0;
    
    // Reset performance metrics
    memset(&parser->perf, 0, sizeof(parser->perf));
    parser->perf.start_tick = cns_get_tick_count();
    
    return CNS_8T_OK;
}

/**
 * Destroy parser and free memory
 */
cns_8t_result_t cns_8t_parser_destroy(cns_parser_t* parser_handle) {
    cns_8t_parser_state_t* parser = (cns_8t_parser_state_t*)parser_handle;
    
    if (!parser) {
        return CNS_8T_ERROR_INVALID_PARAM;
    }
    
    // Free token array
    if (parser->tokens) {
        free(parser->tokens);
    }
    
    // Clear and free parser structure
    memset(parser, 0, sizeof(cns_8t_parser_state_t));
    free(parser);
    
    return CNS_8T_OK;
}

/**
 * Get parser performance metrics
 */
cns_8t_result_t cns_8t_parser_get_metrics(cns_parser_t* parser_handle,
                                          cns_8t_perf_metrics_t* metrics) {
    cns_8t_parser_state_t* parser = (cns_8t_parser_state_t*)parser_handle;
    
    if (!parser || !metrics) {
        return CNS_8T_ERROR_INVALID_PARAM;
    }
    
    *metrics = parser->perf;
    
    // Add parser-specific metrics
    metrics->memory_allocated = (parser->token_capacity * sizeof(cns_8t_token_t)) +
                               sizeof(cns_8t_parser_state_t);
    
    if (parser->input_length > 0 && parser->perf.end_tick > parser->perf.start_tick) {
        cns_tick_t elapsed = parser->perf.end_tick - parser->perf.start_tick;
        metrics->throughput_mbps = (float)(parser->input_length * 1000000.0) / 
                                  (elapsed * 1024.0 * 1024.0);
    }
    
    return CNS_8T_OK;
}