# TTL Lexer Architecture Design

## Overview

The TTL (Turtle) lexer is designed as a high-performance, state-machine-based tokenizer for parsing RDF Turtle syntax. It uses a Deterministic Finite Automaton (DFA) approach for predictable performance and efficient memory usage.

## Core Architecture Components

### 1. Token Types Enumeration

```c
typedef enum {
    // Literals
    TOKEN_NUMBER,           // Integer or decimal numbers
    TOKEN_STRING,          // Quoted strings
    TOKEN_URI,             // <http://example.com>
    TOKEN_BLANK_NODE,      // _:node123
    TOKEN_LANG_TAG,        // @en, @fr-CA
    
    // Keywords
    TOKEN_PREFIX,          // @prefix
    TOKEN_BASE,            // @base
    TOKEN_A,               // 'a' (rdf:type shorthand)
    
    // Punctuation
    TOKEN_DOT,             // .
    TOKEN_COMMA,           // ,
    TOKEN_SEMICOLON,       // ;
    TOKEN_LBRACKET,        // [
    TOKEN_RBRACKET,        // ]
    TOKEN_LPAREN,          // (
    TOKEN_RPAREN,          // )
    
    // Operators
    TOKEN_DOUBLE_CARET,    // ^^
    
    // Identifiers
    TOKEN_PREFIXED_NAME,   // foaf:name
    TOKEN_LOCAL_NAME,      // :localName
    
    // Special
    TOKEN_EOF,             // End of file
    TOKEN_ERROR,           // Error token
    
    // Units (for TTL extensions)
    TOKEN_UNIT_MS,         // ms
    TOKEN_UNIT_US,         // μs
    TOKEN_UNIT_NS,         // ns
    TOKEN_UNIT_S,          // s
    TOKEN_UNIT_MIN,        // min
    TOKEN_UNIT_H,          // h
    TOKEN_UNIT_D,          // d
    
    // Whitespace (usually ignored)
    TOKEN_WHITESPACE,
    TOKEN_COMMENT,         // # comment
    
    TOKEN_COUNT
} ttl_token_type_t;
```

### 2. Token Structure

```c
typedef struct {
    ttl_token_type_t type;
    const char* start;      // Pointer to token start in source
    size_t length;          // Token length
    size_t line;           // Line number
    size_t column;         // Column number
    
    // Value storage (for parsed values)
    union {
        int64_t integer_value;
        double decimal_value;
        struct {
            const char* prefix;
            size_t prefix_len;
            const char* local;
            size_t local_len;
        } prefixed_name;
    } value;
} ttl_token_t;
```

### 3. Lexer State Machine Design

#### DFA States

```c
typedef enum {
    STATE_START,
    STATE_COMMENT,
    STATE_STRING,
    STATE_STRING_ESCAPE,
    STATE_LONG_STRING,
    STATE_URI,
    STATE_URI_ESCAPE,
    STATE_NUMBER,
    STATE_DECIMAL,
    STATE_PREFIXED_NAME,
    STATE_BLANK_NODE,
    STATE_LANG_TAG,
    STATE_DIRECTIVE,
    STATE_WHITESPACE,
    STATE_ERROR,
    STATE_EOF
} lexer_state_t;
```

#### State Transition Table

```c
// State transition function type
typedef lexer_state_t (*state_handler_t)(lexer_t* lexer, char current);

// State transition table
static const state_handler_t state_handlers[] = {
    [STATE_START]         = handle_start,
    [STATE_COMMENT]       = handle_comment,
    [STATE_STRING]        = handle_string,
    [STATE_STRING_ESCAPE] = handle_string_escape,
    [STATE_LONG_STRING]   = handle_long_string,
    [STATE_URI]           = handle_uri,
    [STATE_URI_ESCAPE]    = handle_uri_escape,
    [STATE_NUMBER]        = handle_number,
    [STATE_DECIMAL]       = handle_decimal,
    [STATE_PREFIXED_NAME] = handle_prefixed_name,
    [STATE_BLANK_NODE]    = handle_blank_node,
    [STATE_LANG_TAG]      = handle_lang_tag,
    [STATE_DIRECTIVE]     = handle_directive,
    [STATE_WHITESPACE]    = handle_whitespace
};
```

### 4. Lexer Context Structure

```c
typedef struct {
    // Input management
    const char* input;          // Input string
    size_t input_length;        // Total input length
    size_t current_pos;         // Current position
    size_t line;               // Current line number
    size_t column;             // Current column
    
    // Token management
    ttl_token_t current_token;  // Current token being built
    size_t token_start;         // Start position of current token
    
    // State management
    lexer_state_t state;        // Current lexer state
    lexer_state_t prev_state;   // Previous state (for error recovery)
    
    // Buffer management
    char* buffer;              // Dynamic buffer for token values
    size_t buffer_size;        // Buffer capacity
    size_t buffer_used;        // Buffer usage
    
    // Error handling
    char error_msg[256];       // Error message buffer
    bool has_error;            // Error flag
    
    // Performance counters
    size_t tokens_produced;    // Total tokens produced
    size_t bytes_processed;    // Total bytes processed
    
    // Configuration
    bool skip_whitespace;      // Skip whitespace tokens
    bool skip_comments;        // Skip comment tokens
} lexer_t;
```

## State Machine Diagram

```mermaid
stateDiagram-v2
    [*] --> START
    
    START --> COMMENT: #
    START --> STRING: "
    START --> LONG_STRING: """
    START --> URI: <
    START --> NUMBER: [0-9+-]
    START --> PREFIXED_NAME: [a-zA-Z]
    START --> BLANK_NODE: _:
    START --> DIRECTIVE: @
    START --> WHITESPACE: [ \t\n\r]
    
    COMMENT --> START: \n
    
    STRING --> STRING_ESCAPE: \
    STRING --> START: "
    STRING_ESCAPE --> STRING: any
    
    LONG_STRING --> START: """
    
    URI --> URI_ESCAPE: \
    URI --> START: >
    URI_ESCAPE --> URI: any
    
    NUMBER --> DECIMAL: .
    NUMBER --> START: [^0-9.]
    DECIMAL --> START: [^0-9]
    
    PREFIXED_NAME --> START: [^a-zA-Z0-9:_-]
    
    BLANK_NODE --> START: [^a-zA-Z0-9_]
    
    DIRECTIVE --> START: whitespace
    
    WHITESPACE --> START: [^ \t\n\r]
    
    START --> EOF: end
    EOF --> [*]
```

## Buffer Management Strategy

### 1. Zero-Copy Token Design

Tokens store pointers into the original input buffer when possible, avoiding unnecessary copying:

```c
typedef struct {
    const char* input_buffer;     // Original input
    size_t* line_starts;          // Array of line start positions
    size_t line_count;            // Number of lines
    size_t line_capacity;         // Line array capacity
} buffer_info_t;
```

### 2. Dynamic Value Buffer

For tokens requiring value transformation (escaped strings, normalized numbers):

```c
typedef struct {
    char* data;
    size_t size;
    size_t capacity;
    size_t grow_factor;  // Typically 2x
} dynamic_buffer_t;
```

### 3. Lookahead Buffer

Efficient lookahead mechanism for multi-character tokens:

```c
typedef struct {
    char buffer[16];     // Small lookahead buffer
    size_t count;        // Number of characters in buffer
    size_t pos;          // Current position in buffer
} lookahead_buffer_t;
```

## Error Handling Approach

### 1. Error Recovery Strategy

```c
typedef enum {
    ERROR_NONE,
    ERROR_UNEXPECTED_CHAR,
    ERROR_UNTERMINATED_STRING,
    ERROR_UNTERMINATED_URI,
    ERROR_INVALID_ESCAPE,
    ERROR_INVALID_NUMBER,
    ERROR_INVALID_PREFIX,
    ERROR_BUFFER_OVERFLOW,
    ERROR_INVALID_UTF8
} lexer_error_t;

typedef struct {
    lexer_error_t code;
    size_t position;
    size_t line;
    size_t column;
    char context[64];    // Surrounding context
    char expected[32];   // What was expected
} error_info_t;
```

### 2. Error Recovery Actions

```c
typedef enum {
    RECOVERY_SKIP_CHAR,      // Skip current character
    RECOVERY_SKIP_LINE,      // Skip to next line
    RECOVERY_SKIP_TO_DOT,    // Skip to next statement terminator
    RECOVERY_RESYNC_BRACKET, // Resync at bracket level
    RECOVERY_ABORT          // Unrecoverable error
} recovery_action_t;
```

## Performance Considerations

### 1. Branch Prediction Optimization

```c
// Hot path optimization using likely/unlikely hints
#define LIKELY(x)   __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

// Example usage in state handler
static inline lexer_state_t handle_start(lexer_t* lexer, char ch) {
    if (LIKELY(is_whitespace(ch))) {
        return STATE_WHITESPACE;
    }
    if (LIKELY(is_alpha(ch))) {
        return STATE_PREFIXED_NAME;
    }
    // Less common cases...
}
```

### 2. SIMD Character Classification

```c
// SIMD-accelerated character classification
typedef struct {
    __m128i whitespace_mask;
    __m128i alpha_lower_mask;
    __m128i alpha_upper_mask;
    __m128i digit_mask;
} simd_char_classifier_t;

bool is_whitespace_simd(const char* str, size_t len);
bool is_alphanumeric_simd(const char* str, size_t len);
```

### 3. Cache-Friendly Token Storage

```c
// Token pool for cache locality
typedef struct {
    ttl_token_t tokens[4096];  // Page-aligned token array
    size_t count;
    size_t capacity;
} token_pool_t;
```

### 4. Memory Pool Allocation

```c
typedef struct {
    void* base;           // Base memory address
    size_t size;          // Total size
    size_t used;          // Used bytes
    size_t alignment;     // Alignment requirement
} memory_pool_t;

// Fast allocation from pool
void* pool_alloc(memory_pool_t* pool, size_t size);
void pool_reset(memory_pool_t* pool);
```

## Lexer API Design

### 1. Initialization

```c
// Create lexer with configuration
lexer_t* lexer_create(const lexer_config_t* config);

// Initialize lexer with input
int lexer_init(lexer_t* lexer, const char* input, size_t length);

// Reset lexer for reuse
void lexer_reset(lexer_t* lexer);

// Destroy lexer
void lexer_destroy(lexer_t* lexer);
```

### 2. Token Operations

```c
// Get next token
ttl_token_t* lexer_next_token(lexer_t* lexer);

// Peek at next token without consuming
ttl_token_t* lexer_peek_token(lexer_t* lexer);

// Skip tokens of specific types
void lexer_skip_whitespace(lexer_t* lexer);
void lexer_skip_comments(lexer_t* lexer);
```

### 3. Error Handling

```c
// Check for errors
bool lexer_has_error(const lexer_t* lexer);

// Get error information
const error_info_t* lexer_get_error(const lexer_t* lexer);

// Attempt error recovery
bool lexer_recover(lexer_t* lexer, recovery_action_t action);
```

### 4. Performance Monitoring

```c
typedef struct {
    size_t tokens_per_second;
    size_t bytes_per_second;
    size_t cache_hits;
    size_t cache_misses;
    double avg_token_size;
} lexer_stats_t;

// Get performance statistics
void lexer_get_stats(const lexer_t* lexer, lexer_stats_t* stats);
```

## Thread Safety Considerations

The lexer is designed to be thread-safe for read operations:

1. **Immutable Input**: Input buffer is never modified
2. **Local State**: All mutable state is contained within lexer instance
3. **No Global State**: No static variables or global state
4. **Pool Safety**: Memory pools are per-lexer instance

For multi-threaded parsing:
```c
// Thread-local lexer pool
__thread lexer_t* thread_lexer = NULL;

// Get thread-local lexer
lexer_t* get_thread_lexer(void) {
    if (!thread_lexer) {
        thread_lexer = lexer_create(&default_config);
    }
    return thread_lexer;
}
```

## Integration Points

### 1. Parser Integration

```c
// Parser callback for token consumption
typedef void (*token_handler_t)(void* context, const ttl_token_t* token);

// Stream tokens to parser
void lexer_stream_tokens(lexer_t* lexer, token_handler_t handler, void* context);
```

### 2. Error Reporter Integration

```c
// Error reporter interface
typedef struct {
    void (*on_error)(void* context, const error_info_t* error);
    void (*on_warning)(void* context, const char* message);
    void* context;
} error_reporter_t;

// Set error reporter
void lexer_set_error_reporter(lexer_t* lexer, const error_reporter_t* reporter);
```

### 3. Memory Allocator Integration

```c
// Custom allocator interface
typedef struct {
    void* (*alloc)(size_t size);
    void* (*realloc)(void* ptr, size_t size);
    void (*free)(void* ptr);
} allocator_t;

// Set custom allocator
void lexer_set_allocator(lexer_t* lexer, const allocator_t* allocator);
```