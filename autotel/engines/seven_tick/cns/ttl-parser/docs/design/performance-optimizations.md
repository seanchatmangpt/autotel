# TTL Lexer Performance Optimizations

## Overview

This document outlines the performance optimization strategies for achieving 7-tick lexing performance on TTL/Turtle files. The optimizations are organized by impact level and implementation complexity.

## 1. Branch Prediction Optimizations

### Hot Path Identification

```c
// Profile-guided optimization hints
#define LIKELY(x)   __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

// Most common state transitions
static inline lexer_state_t handle_start_optimized(lexer_t* lexer, char ch) {
    // Order by frequency from profiling data
    if (LIKELY(ch >= 'a' && ch <= 'z')) {
        return STATE_PREFIXED_NAME;
    }
    if (LIKELY(ch == ' ' || ch == '\t' || ch == '\n')) {
        return STATE_WHITESPACE;
    }
    if (LIKELY(ch >= '0' && ch <= '9')) {
        return STATE_NUMBER;
    }
    
    // Less common cases
    switch (ch) {
        case '"': return STATE_STRING;
        case '<': return STATE_URI;
        case '#': return STATE_COMMENT;
        case '.': emit_token(lexer, TOKEN_DOT); return STATE_START;
        // ... other cases
        default: return STATE_ERROR;
    }
}
```

### Jump Table Optimization

```c
// Direct jump table for single-byte tokens
static const struct {
    ttl_token_type_t type;
    lexer_state_t next_state;
} direct_tokens[256] = {
    ['.'] = { TOKEN_DOT, STATE_START },
    [','] = { TOKEN_COMMA, STATE_START },
    [';'] = { TOKEN_SEMICOLON, STATE_START },
    ['['] = { TOKEN_LBRACKET, STATE_START },
    [']'] = { TOKEN_RBRACKET, STATE_START },
    ['('] = { TOKEN_LPAREN, STATE_START },
    [')'] = { TOKEN_RPAREN, STATE_START },
    // ... initialize rest to TOKEN_NONE
};

// Fast path for punctuation
if (direct_tokens[ch].type != TOKEN_NONE) {
    emit_token(lexer, direct_tokens[ch].type);
    return direct_tokens[ch].next_state;
}
```

## 2. SIMD Acceleration

### Character Classification

```c
// SIMD-accelerated whitespace detection
static inline size_t skip_whitespace_simd(const char* input, size_t len) {
    const __m128i space = _mm_set1_epi8(' ');
    const __m128i tab = _mm_set1_epi8('\t');
    const __m128i newline = _mm_set1_epi8('\n');
    const __m128i cr = _mm_set1_epi8('\r');
    
    size_t pos = 0;
    
    // Process 16 bytes at a time
    while (pos + 16 <= len) {
        __m128i chunk = _mm_loadu_si128((__m128i*)(input + pos));
        
        // Check for any whitespace
        __m128i is_space = _mm_cmpeq_epi8(chunk, space);
        __m128i is_tab = _mm_cmpeq_epi8(chunk, tab);
        __m128i is_newline = _mm_cmpeq_epi8(chunk, newline);
        __m128i is_cr = _mm_cmpeq_epi8(chunk, cr);
        
        __m128i is_ws = _mm_or_si128(
            _mm_or_si128(is_space, is_tab),
            _mm_or_si128(is_newline, is_cr)
        );
        
        int mask = _mm_movemask_epi8(is_ws);
        
        if (mask != 0xFFFF) {
            // Found non-whitespace
            pos += __builtin_ctz(~mask);
            break;
        }
        
        pos += 16;
    }
    
    // Handle remainder
    while (pos < len && is_whitespace(input[pos])) {
        pos++;
    }
    
    return pos;
}
```

### Identifier Scanning

```c
// SIMD-accelerated identifier scanning
static size_t scan_identifier_simd(const char* input, size_t len) {
    // Constants for character range checks
    const __m128i lower_a = _mm_set1_epi8('a' - 1);
    const __m128i upper_z = _mm_set1_epi8('z' + 1);
    const __m128i lower_A = _mm_set1_epi8('A' - 1);
    const __m128i upper_Z = _mm_set1_epi8('Z' + 1);
    const __m128i lower_0 = _mm_set1_epi8('0' - 1);
    const __m128i upper_9 = _mm_set1_epi8('9' + 1);
    const __m128i underscore = _mm_set1_epi8('_');
    const __m128i hyphen = _mm_set1_epi8('-');
    
    size_t pos = 0;
    
    while (pos + 16 <= len) {
        __m128i chunk = _mm_loadu_si128((__m128i*)(input + pos));
        
        // Check lowercase
        __m128i gt_lower_a = _mm_cmpgt_epi8(chunk, lower_a);
        __m128i lt_upper_z = _mm_cmplt_epi8(chunk, upper_z);
        __m128i is_lower = _mm_and_si128(gt_lower_a, lt_upper_z);
        
        // Check uppercase
        __m128i gt_lower_A = _mm_cmpgt_epi8(chunk, lower_A);
        __m128i lt_upper_Z = _mm_cmplt_epi8(chunk, upper_Z);
        __m128i is_upper = _mm_and_si128(gt_lower_A, lt_upper_Z);
        
        // Check digits
        __m128i gt_lower_0 = _mm_cmpgt_epi8(chunk, lower_0);
        __m128i lt_upper_9 = _mm_cmplt_epi8(chunk, upper_9);
        __m128i is_digit = _mm_and_si128(gt_lower_0, lt_upper_9);
        
        // Check special characters
        __m128i is_underscore = _mm_cmpeq_epi8(chunk, underscore);
        __m128i is_hyphen = _mm_cmpeq_epi8(chunk, hyphen);
        
        // Combine all valid characters
        __m128i valid = _mm_or_si128(
            _mm_or_si128(is_lower, is_upper),
            _mm_or_si128(is_digit, _mm_or_si128(is_underscore, is_hyphen))
        );
        
        int mask = _mm_movemask_epi8(valid);
        
        if (mask != 0xFFFF) {
            // Found invalid character
            pos += __builtin_ctz(~mask & 0xFFFF);
            break;
        }
        
        pos += 16;
    }
    
    // Handle remainder with scalar code
    while (pos < len && is_name_char(input[pos])) {
        pos++;
    }
    
    return pos;
}
```

### String Scanning with Escape Detection

```c
// SIMD string scanning with escape detection
static size_t scan_string_simd(const char* input, size_t len) {
    const __m128i quote = _mm_set1_epi8('"');
    const __m128i backslash = _mm_set1_epi8('\\');
    const __m128i newline = _mm_set1_epi8('\n');
    
    size_t pos = 0;
    bool escaped = false;
    
    while (pos + 16 <= len) {
        if (escaped) {
            // Skip one character after backslash
            pos++;
            escaped = false;
            continue;
        }
        
        __m128i chunk = _mm_loadu_si128((__m128i*)(input + pos));
        
        // Check for quotes, backslashes, and newlines
        __m128i is_quote = _mm_cmpeq_epi8(chunk, quote);
        __m128i is_backslash = _mm_cmpeq_epi8(chunk, backslash);
        __m128i is_newline = _mm_cmpeq_epi8(chunk, newline);
        
        // Combine special characters
        __m128i special = _mm_or_si128(
            _mm_or_si128(is_quote, is_backslash),
            is_newline
        );
        
        int mask = _mm_movemask_epi8(special);
        
        if (mask != 0) {
            // Found special character
            int offset = __builtin_ctz(mask);
            pos += offset;
            
            if (input[pos] == '\\') {
                escaped = true;
                pos++;
            } else if (input[pos] == '"') {
                return pos;  // End of string
            } else if (input[pos] == '\n') {
                return -1;   // Error: newline in string
            }
        } else {
            pos += 16;
        }
    }
    
    // Handle remainder
    while (pos < len) {
        if (escaped) {
            escaped = false;
            pos++;
            continue;
        }
        
        if (input[pos] == '\\') {
            escaped = true;
        } else if (input[pos] == '"') {
            return pos;
        } else if (input[pos] == '\n') {
            return -1;
        }
        pos++;
    }
    
    return -1;  // Unterminated string
}
```

## 3. Memory Access Optimizations

### Prefetching

```c
// Prefetch next cache lines during processing
static inline void prefetch_input(const char* input, size_t pos, size_t len) {
    // Prefetch 2 cache lines ahead
    if (pos + 128 < len) {
        __builtin_prefetch(input + pos + 64, 0, 3);   // L1 cache
        __builtin_prefetch(input + pos + 128, 0, 2);  // L2 cache
    }
}

// Use in main loop
while (lexer->current_pos < lexer->input_length) {
    prefetch_input(lexer->input, lexer->current_pos, lexer->input_length);
    
    char ch = lexer->input[lexer->current_pos];
    lexer->state = state_handlers[lexer->state](lexer, ch);
    lexer->current_pos++;
}
```

### Cache-Friendly Token Buffer

```c
// Align token buffer to cache line boundaries
typedef struct __attribute__((aligned(64))) {
    ttl_token_t tokens[1024];    // Power of 2 for fast modulo
    uint32_t write_pos;          // Write position
    uint32_t read_pos;           // Read position
    uint32_t mask;               // Size - 1 for fast modulo
    
    // Padding to separate read/write positions
    char padding[64 - 2 * sizeof(uint32_t) - sizeof(uint32_t)];
    
    // Statistics in separate cache line
    struct {
        size_t total_tokens;
        size_t buffer_overflows;
    } stats;
} cache_aligned_token_buffer_t;

// Fast token insertion
static inline void buffer_put_token(cache_aligned_token_buffer_t* buf,
                                   const ttl_token_t* token) {
    uint32_t pos = buf->write_pos & buf->mask;
    buf->tokens[pos] = *token;
    
    // Ensure write is visible before updating position
    __atomic_store_n(&buf->write_pos, buf->write_pos + 1, __ATOMIC_RELEASE);
}
```

## 4. Compiler Optimizations

### Function Inlining

```c
// Force inline for hot path functions
#define FORCE_INLINE __attribute__((always_inline)) inline

FORCE_INLINE static char current_char(const lexer_t* lexer) {
    return lexer->input[lexer->current_pos];
}

FORCE_INLINE static void advance(lexer_t* lexer, size_t n) {
    lexer->current_pos += n;
    lexer->column += n;
}

FORCE_INLINE static bool is_at_end(const lexer_t* lexer) {
    return lexer->current_pos >= lexer->input_length;
}

// Template-like optimization for common patterns
#define SCAN_WHILE(lexer, condition) \
    do { \
        size_t start = (lexer)->current_pos; \
        while ((lexer)->current_pos < (lexer)->input_length && \
               (condition)) { \
            (lexer)->current_pos++; \
        } \
        return (lexer)->current_pos - start; \
    } while (0)

// Usage
FORCE_INLINE static size_t scan_digits(lexer_t* lexer) {
    SCAN_WHILE(lexer, is_digit(current_char(lexer)));
}
```

### Loop Unrolling

```c
// Manually unrolled loops for small fixed iterations
static bool match_keyword_prefix(const char* input, size_t pos) {
    // Check "@prefix" - unrolled
    return input[pos + 0] == '@' &&
           input[pos + 1] == 'p' &&
           input[pos + 2] == 'r' &&
           input[pos + 3] == 'e' &&
           input[pos + 4] == 'f' &&
           input[pos + 5] == 'i' &&
           input[pos + 6] == 'x';
}

// Compiler hint for loop unrolling
#pragma GCC unroll 4
for (int i = 0; i < unicode_digits; i++) {
    char ch = input[pos + i];
    if (!is_hex_digit(ch)) return false;
    value = (value << 4) | hex_value(ch);
}
```

### Profile-Guided Optimization

```c
// Build with PGO
// Step 1: Compile with profiling
// gcc -fprofile-generate -O2 lexer.c -o lexer_profile

// Step 2: Run with representative data
// ./lexer_profile large_dataset.ttl

// Step 3: Compile with profile data
// gcc -fprofile-use -O3 lexer.c -o lexer_optimized

// Code annotations for expected values
if (__builtin_expect(token->type == TOKEN_PREFIXED_NAME, 1)) {
    // Common case: prefixed names
    process_prefixed_name(token);
} else {
    // Rare cases
    process_other_token(token);
}
```

## 5. Algorithm Optimizations

### Perfect Hash for Keywords

```c
// Perfect hash function for TTL keywords
static inline uint32_t keyword_hash(const char* str, size_t len) {
    // Minimal perfect hash for: @prefix, @base, a
    if (len == 1) return 0;  // 'a'
    if (len == 5) return 1;  // @base
    if (len == 7) return 2;  // @prefix
    return UINT32_MAX;
}

static const struct {
    const char* keyword;
    ttl_token_type_t type;
} keyword_table[] = {
    {"a", TOKEN_A},
    {"@base", TOKEN_BASE},
    {"@prefix", TOKEN_PREFIX}
};

static ttl_token_type_t lookup_keyword(const char* str, size_t len) {
    uint32_t idx = keyword_hash(str, len);
    if (idx < 3 && memcmp(str, keyword_table[idx].keyword, len) == 0) {
        return keyword_table[idx].type;
    }
    return TOKEN_NONE;
}
```

### Fast Number Parsing

```c
// Optimized integer parsing without division
static int64_t parse_integer_fast(const char* str, size_t len) {
    int64_t value = 0;
    size_t i = 0;
    
    // Handle sign
    int sign = 1;
    if (str[0] == '-') {
        sign = -1;
        i = 1;
    } else if (str[0] == '+') {
        i = 1;
    }
    
    // Unroll common cases
    switch (len - i) {
        case 1:
            value = str[i] - '0';
            break;
        case 2:
            value = (str[i] - '0') * 10 + (str[i+1] - '0');
            break;
        case 3:
            value = (str[i] - '0') * 100 + 
                    (str[i+1] - '0') * 10 + 
                    (str[i+2] - '0');
            break;
        default:
            // General case with SWAR (SIMD Within A Register)
            while (i + 8 <= len) {
                uint64_t chunk = *(uint64_t*)(str + i);
                // Process 8 digits at once using SWAR techniques
                // ... (complex but fast implementation)
                i += 8;
            }
            // Handle remainder
            while (i < len) {
                value = value * 10 + (str[i] - '0');
                i++;
            }
    }
    
    return value * sign;
}
```

## 6. 7-Tick Optimizations

### Tick Budget Management

```c
typedef struct {
    uint64_t start_tsc;          // Start timestamp counter
    uint64_t tick_budget;        // Total ticks (7)
    uint64_t ticks_per_phase[7]; // Ticks per processing phase
    uint64_t current_tick;       // Current tick count
} tick_manager_t;

// Check tick budget during processing
#define CHECK_TICK_BUDGET(lexer) \
    do { \
        uint64_t elapsed = rdtsc() - (lexer)->tick_mgr.start_tsc; \
        if (elapsed > (lexer)->tick_mgr.tick_budget) { \
            return handle_tick_overflow(lexer); \
        } \
    } while (0)

// Adaptive processing based on remaining ticks
static void adjust_processing_strategy(lexer_t* lexer) {
    uint64_t remaining_ticks = lexer->tick_mgr.tick_budget - 
                               lexer->tick_mgr.current_tick;
    
    if (remaining_ticks < 2) {
        // Switch to fast path only
        lexer->flags |= LEXER_FAST_MODE;
        lexer->flags &= ~LEXER_VALIDATION;
    } else if (remaining_ticks < 4) {
        // Disable non-essential features
        lexer->flags &= ~LEXER_COLLECT_METRICS;
    }
}
```

### Batched Token Processing

```c
// Process multiple tokens in one tick
static size_t process_token_batch(lexer_t* lexer, size_t max_tokens) {
    size_t count = 0;
    uint64_t tick_start = rdtsc();
    
    while (count < max_tokens && !is_at_end(lexer)) {
        // Fast token recognition
        char ch = current_char(lexer);
        
        // Direct dispatch for common cases
        if (ch >= 'a' && ch <= 'z') {
            count += process_identifier_batch(lexer);
        } else if (ch == ' ' || ch == '\t' || ch == '\n') {
            count += skip_whitespace_batch(lexer);
        } else {
            // Regular processing
            process_single_token(lexer);
            count++;
        }
        
        // Check if we've used our tick
        if (rdtsc() - tick_start > CYCLES_PER_TICK) {
            break;
        }
    }
    
    return count;
}
```

## 7. Platform-Specific Optimizations

### ARM NEON Optimizations

```c
#ifdef __ARM_NEON
// NEON-accelerated character classification
static size_t skip_whitespace_neon(const char* input, size_t len) {
    const uint8x16_t space = vdupq_n_u8(' ');
    const uint8x16_t tab = vdupq_n_u8('\t');
    const uint8x16_t newline = vdupq_n_u8('\n');
    const uint8x16_t cr = vdupq_n_u8('\r');
    
    size_t pos = 0;
    
    while (pos + 16 <= len) {
        uint8x16_t chunk = vld1q_u8((const uint8_t*)(input + pos));
        
        uint8x16_t is_space = vceqq_u8(chunk, space);
        uint8x16_t is_tab = vceqq_u8(chunk, tab);
        uint8x16_t is_newline = vceqq_u8(chunk, newline);
        uint8x16_t is_cr = vceqq_u8(chunk, cr);
        
        uint8x16_t is_ws = vorrq_u8(
            vorrq_u8(is_space, is_tab),
            vorrq_u8(is_newline, is_cr)
        );
        
        // Check if all bytes are whitespace
        if (vminvq_u8(is_ws) != 0xFF) {
            // Found non-whitespace
            // ... find exact position
            break;
        }
        
        pos += 16;
    }
    
    return pos;
}
#endif
```

### AVX2 Optimizations (x86-64)

```c
#ifdef __AVX2__
// AVX2 - process 32 bytes at once
static size_t scan_identifier_avx2(const char* input, size_t len) {
    // Similar to SSE but processing 32 bytes
    const __m256i lower_bound = _mm256_set1_epi8('a' - 1);
    const __m256i upper_bound = _mm256_set1_epi8('z' + 1);
    // ... implementation
}
#endif
```

## Performance Monitoring

### Inline Performance Counters

```c
// Lightweight performance monitoring
typedef struct {
    uint64_t cycles[STATE_COUNT];
    uint64_t transitions;
    uint64_t cache_misses;
} perf_counters_t;

// Update counters with minimal overhead
#define PERF_STATE_ENTER(lexer, state) \
    do { \
        if ((lexer)->flags & LEXER_COLLECT_METRICS) { \
            (lexer)->perf.cycles[state] -= rdtsc(); \
        } \
    } while (0)

#define PERF_STATE_EXIT(lexer, state) \
    do { \
        if ((lexer)->flags & LEXER_COLLECT_METRICS) { \
            (lexer)->perf.cycles[state] += rdtsc(); \
            (lexer)->perf.transitions++; \
        } \
    } while (0)
```

## Benchmark Results

Expected performance improvements:

| Optimization | Improvement | Implementation Effort |
|--------------|-------------|----------------------|
| SIMD Character Classification | 3-5x | Medium |
| Branch Prediction Hints | 10-20% | Low |
| Perfect Hash Keywords | 2x | Low |
| Cache-Aligned Structures | 15-25% | Low |
| Prefetching | 10-15% | Low |
| Fast Number Parsing | 2-3x | Medium |
| Profile-Guided Optimization | 10-30% | Low |
| Batched Processing | 20-40% | Medium |

Combined optimizations target:
- **Throughput**: 1GB/s+ on modern CPUs
- **Latency**: < 7 CPU ticks per token average
- **Memory**: < 1KB per concurrent lexer instance