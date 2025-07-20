# TTL Lexer Data Structures

## Core Data Structures

### 1. Lexer Configuration

```c
typedef struct {
    // Feature flags
    bool skip_whitespace;        // Auto-skip whitespace tokens
    bool skip_comments;          // Auto-skip comment tokens
    bool strict_mode;            // Strict Turtle compliance
    bool allow_extensions;       // Allow TTL timing extensions
    
    // Performance tuning
    size_t initial_buffer_size;  // Initial token buffer size (default: 4KB)
    size_t max_buffer_size;      // Maximum buffer size (default: 1MB)
    size_t lookahead_size;       // Lookahead buffer size (default: 16)
    
    // Memory management
    allocator_t* allocator;      // Custom allocator (optional)
    bool use_memory_pool;        // Use memory pooling
    size_t pool_size;           // Memory pool size
    
    // Error handling
    error_reporter_t* reporter;  // Error reporter (optional)
    recovery_mode_t recovery;    // Error recovery mode
    size_t max_errors;          // Maximum errors before abort
    
    // Timing constraints (7-tick)
    bool enable_timing;         // Enable timing constraints
    uint64_t tick_budget;       // Tick budget (default: 7)
    
    // Debugging
    bool enable_metrics;        // Collect performance metrics
    bool enable_trace;          // Enable state tracing
} lexer_config_t;
```

### 2. Token Storage

```c
// Token pool for efficient allocation
typedef struct token_node {
    ttl_token_t token;
    struct token_node* next;
} token_node_t;

typedef struct {
    token_node_t* free_list;     // Free token nodes
    token_node_t* active_list;   // Active tokens
    size_t pool_size;            // Total nodes in pool
    size_t active_count;         // Active token count
    
    // Memory blocks for pool
    void* memory_blocks[16];     // Up to 16 memory blocks
    size_t block_count;          // Number of allocated blocks
    size_t block_size;           // Size of each block
} token_pool_t;

// Token buffer for parser interface
typedef struct {
    ttl_token_t* tokens;         // Token array
    size_t capacity;             // Array capacity
    size_t count;                // Current token count
    size_t read_pos;             // Parser read position
    size_t write_pos;            // Lexer write position
    
    // Ring buffer support
    bool is_ring_buffer;         // Use as ring buffer
    size_t mask;                 // Capacity - 1 (for power of 2)
} token_buffer_t;
```

### 3. Input Management

```c
// Input source abstraction
typedef enum {
    INPUT_STRING,                // In-memory string
    INPUT_FILE,                  // File handle
    INPUT_STREAM,                // Generic stream
    INPUT_MMAP                   // Memory-mapped file
} input_type_t;

typedef struct {
    input_type_t type;
    
    union {
        struct {
            const char* data;
            size_t length;
        } string;
        
        struct {
            FILE* handle;
            char* buffer;
            size_t buffer_size;
            size_t buffer_pos;
            size_t buffer_fill;
        } file;
        
        struct {
            void* handle;
            size_t (*read)(void* handle, char* buf, size_t size);
            char* buffer;
            size_t buffer_size;
            size_t buffer_pos;
            size_t buffer_fill;
        } stream;
        
        struct {
            void* base;
            size_t length;
            int fd;
        } mmap;
    } source;
    
    // Position tracking
    size_t byte_offset;          // Absolute byte offset
    size_t line;                 // Current line (1-based)
    size_t column;               // Current column (1-based)
    
    // Line information for error reporting
    size_t* line_starts;         // Array of line start offsets
    size_t line_count;           // Number of lines
    size_t line_capacity;        // Line array capacity
} input_source_t;
```

### 4. Character Buffer Management

```c
// Efficient character buffer for token values
typedef struct {
    char* data;                  // Buffer data
    size_t size;                 // Current size
    size_t capacity;             // Total capacity
    
    // Growth strategy
    size_t initial_capacity;     // Initial allocation
    float growth_factor;         // Growth multiplier (e.g., 1.5)
    size_t max_capacity;         // Maximum allowed size
    
    // Memory source
    memory_pool_t* pool;         // Optional memory pool
    allocator_t* allocator;      // Custom allocator
} char_buffer_t;

// Operations
bool buffer_init(char_buffer_t* buf, size_t initial_capacity);
bool buffer_append(char_buffer_t* buf, char ch);
bool buffer_append_n(char_buffer_t* buf, const char* data, size_t len);
bool buffer_append_utf8(char_buffer_t* buf, uint32_t codepoint);
void buffer_clear(char_buffer_t* buf);
void buffer_destroy(char_buffer_t* buf);
```

### 5. State Machine Context

```c
// Per-state context data
typedef struct {
    // String state context
    struct {
        bool is_long_string;     // Triple-quoted string
        size_t quote_count;      // Quotes seen (for """)
        bool in_escape;          // Currently in escape sequence
        uint32_t unicode_value;  // Building unicode value
        size_t unicode_digits;   // Unicode digits remaining
    } string;
    
    // Number state context
    struct {
        bool has_sign;           // Has leading +/-
        bool has_decimal;        // Has decimal point
        bool has_exponent;       // Has exponent
        bool exp_has_sign;       // Exponent has sign
        int64_t int_value;       // Integer value (if applicable)
        double float_value;      // Float value
    } number;
    
    // Name state context
    struct {
        const char* prefix_start;
        size_t prefix_length;
        const char* local_start;
        size_t local_length;
        bool has_colon;          // Seen ':' separator
    } name;
    
    // Directive state context
    struct {
        directive_type_t type;   // @prefix, @base, etc.
        size_t chars_matched;    // Characters matched so far
    } directive;
} state_context_t;
```

### 6. Performance Metrics

```c
// Detailed performance tracking
typedef struct {
    // Timing metrics
    uint64_t start_time;         // Lexing start time
    uint64_t total_time;         // Total lexing time
    uint64_t state_times[STATE_COUNT]; // Time per state
    
    // Volume metrics
    size_t bytes_processed;      // Total bytes processed
    size_t tokens_produced;      // Total tokens produced
    size_t chars_per_token;      // Average chars per token
    
    // State metrics
    size_t state_visits[STATE_COUNT];      // Visit count per state
    size_t state_transitions[STATE_COUNT][STATE_COUNT]; // Transition matrix
    
    // Cache performance
    size_t cache_hits;           // Lookahead cache hits
    size_t cache_misses;         // Lookahead cache misses
    
    // Memory metrics
    size_t peak_memory;          // Peak memory usage
    size_t allocations;          // Total allocations
    size_t deallocations;        // Total deallocations
    
    // Error metrics
    size_t errors_encountered;   // Total errors
    size_t errors_recovered;     // Successfully recovered
    
    // SIMD utilization
    size_t simd_ops;             // SIMD operations performed
    size_t scalar_ops;           // Scalar operations performed
} lexer_metrics_t;
```

### 7. SIMD Support Structures

```c
// SIMD character classification tables
typedef struct {
    // SSE/AVX character class masks
    __m128i whitespace_mask;     // ' ', '\t', '\n', '\r'
    __m128i alpha_lower_bound;   // 'a' - 1
    __m128i alpha_lower_upper;   // 'z' + 1
    __m128i alpha_upper_bound;   // 'A' - 1
    __m128i alpha_upper_upper;   // 'Z' + 1
    __m128i digit_lower;         // '0' - 1
    __m128i digit_upper;         // '9' + 1
    
    // Combined masks for common operations
    __m128i alnum_mask;          // Alphanumeric check
    __m128i name_char_mask;      // Valid name characters
    __m128i punct_mask;          // Punctuation characters
} simd_tables_t;

// SIMD scan results
typedef struct {
    size_t match_pos;            // First match position
    uint32_t match_mask;         // Bit mask of matches
    bool found;                  // Match found flag
} simd_scan_result_t;
```

### 8. Memory Pool Implementation

```c
// Memory pool for zero-allocation lexing
typedef struct memory_block {
    void* base;                  // Block base address
    size_t size;                 // Block size
    size_t used;                 // Used bytes
    struct memory_block* next;   // Next block in chain
} memory_block_t;

typedef struct {
    memory_block_t* current;     // Current active block
    memory_block_t* blocks;      // All blocks (linked list)
    size_t block_size;           // Standard block size
    size_t total_allocated;      // Total memory allocated
    size_t total_used;           // Total memory used
    
    // Allocation strategy
    alignment_t alignment;       // Memory alignment
    bool allow_growth;           // Allow new block allocation
    size_t max_memory;           // Maximum memory limit
    
    // Statistics
    size_t allocation_count;     // Number of allocations
    size_t reset_count;          // Number of resets
} memory_pool_t;

// Pool operations
void* pool_alloc(memory_pool_t* pool, size_t size);
void* pool_alloc_aligned(memory_pool_t* pool, size_t size, size_t align);
void pool_reset(memory_pool_t* pool);
void pool_destroy(memory_pool_t* pool);
```

### 9. Error Context

```c
// Rich error information
typedef struct {
    lexer_error_t code;          // Error code
    const char* message;         // Error message
    
    // Location information
    size_t byte_offset;          // Byte position in input
    size_t line;                 // Line number (1-based)
    size_t column;               // Column number (1-based)
    
    // Context window
    char before[32];             // Text before error
    char at[32];                 // Text at error
    char after[32];              // Text after error
    
    // Expected vs actual
    char expected[64];           // What was expected
    char found[64];              // What was found
    
    // Recovery information
    recovery_action_t recovery;   // Suggested recovery
    size_t recovery_pos;         // Recovery position
    
    // Stack trace (for debugging)
    lexer_state_t state_stack[8]; // State history
    size_t stack_depth;          // Stack depth
} error_context_t;
```

### 10. Thread-Local Storage

```c
// Thread-local lexer state for parallel processing
typedef struct {
    lexer_t* lexer;              // Thread-local lexer instance
    memory_pool_t* pool;         // Thread-local memory pool
    token_buffer_t* buffer;      // Thread-local token buffer
    simd_tables_t* simd_tables;  // Shared SIMD tables (read-only)
    
    // Performance counters
    lexer_metrics_t metrics;     // Thread-local metrics
    
    // Error handling
    error_context_t* errors;     // Error buffer
    size_t error_capacity;       // Error buffer capacity
    size_t error_count;          // Current error count
} thread_lexer_t;

// Thread-local storage key
extern __thread thread_lexer_t* tls_lexer;

// Thread-safe operations
thread_lexer_t* get_thread_lexer(void);
void release_thread_lexer(thread_lexer_t* tl);
```

## Memory Layout Optimization

### Cache-Aligned Structures

```c
// Ensure cache line alignment for hot data
#define CACHE_LINE_SIZE 64

typedef struct __attribute__((aligned(CACHE_LINE_SIZE))) {
    // Hot path data (first cache line)
    const char* input;           // 8 bytes
    size_t current_pos;          // 8 bytes
    size_t input_length;         // 8 bytes
    lexer_state_t state;         // 4 bytes
    uint32_t flags;              // 4 bytes
    ttl_token_t* current_token;  // 8 bytes
    char_buffer_t* buffer;       // 8 bytes
    state_context_t* context;    // 8 bytes
    // Total: 56 bytes (fits in one cache line)
    
    char padding[8];             // Pad to 64 bytes
    
    // Cold path data (subsequent cache lines)
    lexer_config_t config;
    lexer_metrics_t metrics;
    error_reporter_t* reporter;
    memory_pool_t* pool;
} lexer_cache_aligned_t;
```

### Structure Packing

```c
// Minimize structure size for token arrays
#pragma pack(push, 1)
typedef struct {
    uint16_t type;               // Token type (2 bytes)
    uint32_t offset;             // Start offset (4 bytes)
    uint16_t length;             // Token length (2 bytes)
    uint16_t line;               // Line number (2 bytes)
    uint16_t column;             // Column number (2 bytes)
    // Total: 12 bytes per token
} compact_token_t;
#pragma pack(pop)

// Convert between full and compact tokens
void token_compact(const ttl_token_t* full, compact_token_t* compact);
void token_expand(const compact_token_t* compact, ttl_token_t* full);
```

## Usage Patterns

### 1. Basic Lexing

```c
// Initialize lexer
lexer_config_t config = {
    .skip_whitespace = true,
    .skip_comments = true,
    .strict_mode = true,
    .enable_metrics = true
};

lexer_t* lexer = lexer_create(&config);
lexer_init(lexer, input_text, strlen(input_text));

// Process tokens
ttl_token_t* token;
while ((token = lexer_next_token(lexer)) != NULL) {
    if (token->type == TOKEN_EOF) break;
    if (token->type == TOKEN_ERROR) {
        handle_error(lexer);
        continue;
    }
    process_token(token);
}

// Get metrics
lexer_metrics_t metrics;
lexer_get_metrics(lexer, &metrics);
printf("Processed %zu tokens in %llu ms\n", 
       metrics.tokens_produced,
       metrics.total_time / 1000000);

lexer_destroy(lexer);
```

### 2. Streaming Large Files

```c
// Memory-mapped file for large TTL files
int fd = open("large.ttl", O_RDONLY);
struct stat st;
fstat(fd, &st);

void* mmap_base = mmap(NULL, st.st_size, PROT_READ, 
                       MAP_PRIVATE, fd, 0);

input_source_t input = {
    .type = INPUT_MMAP,
    .source.mmap = {
        .base = mmap_base,
        .length = st.st_size,
        .fd = fd
    }
};

lexer_t* lexer = lexer_create(&config);
lexer_init_source(lexer, &input);

// Process with callback to avoid token buffer
lexer_stream_tokens(lexer, token_callback, user_context);

munmap(mmap_base, st.st_size);
close(fd);
```

### 3. Parallel Processing

```c
// Split input for parallel lexing
typedef struct {
    const char* input;
    size_t start;
    size_t end;
    token_buffer_t* output;
} work_item_t;

void* worker_thread(void* arg) {
    work_item_t* work = (work_item_t*)arg;
    thread_lexer_t* tl = get_thread_lexer();
    
    // Process chunk
    lexer_init(tl->lexer, work->input + work->start, 
               work->end - work->start);
    
    ttl_token_t* token;
    while ((token = lexer_next_token(tl->lexer)) != NULL) {
        if (token->type == TOKEN_EOF) break;
        token_buffer_append(work->output, token);
    }
    
    release_thread_lexer(tl);
    return NULL;
}
```