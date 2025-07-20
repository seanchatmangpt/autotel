# TTL Lexer State Machine Design

## State Machine Architecture

The TTL lexer uses a table-driven DFA (Deterministic Finite Automaton) for maximum performance and predictability. Each state has a dedicated handler function that processes input characters and determines state transitions.

## State Definitions and Transitions

### 1. START State

The initial state that classifies the first character of each token.

```c
static lexer_state_t handle_start(lexer_t* lexer, char ch) {
    // Fast path for common cases
    switch (ch) {
        // Whitespace
        case ' ': case '\t': case '\n': case '\r':
            lexer->token_start = lexer->current_pos;
            return STATE_WHITESPACE;
        
        // Comments
        case '#':
            lexer->token_start = lexer->current_pos;
            return STATE_COMMENT;
        
        // Strings
        case '"':
            lexer->token_start = lexer->current_pos;
            // Check for long string
            if (peek_ahead(lexer, 1) == '"' && peek_ahead(lexer, 2) == '"') {
                advance(lexer, 2);
                return STATE_LONG_STRING;
            }
            return STATE_STRING;
        
        // URIs
        case '<':
            lexer->token_start = lexer->current_pos;
            return STATE_URI;
        
        // Numbers
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
        case '+': case '-':
            lexer->token_start = lexer->current_pos;
            return STATE_NUMBER;
        
        // Punctuation (single character tokens)
        case '.':
            emit_token(lexer, TOKEN_DOT);
            return STATE_START;
        case ',':
            emit_token(lexer, TOKEN_COMMA);
            return STATE_START;
        case ';':
            emit_token(lexer, TOKEN_SEMICOLON);
            return STATE_START;
        case '[':
            emit_token(lexer, TOKEN_LBRACKET);
            return STATE_START;
        case ']':
            emit_token(lexer, TOKEN_RBRACKET);
            return STATE_START;
        case '(':
            emit_token(lexer, TOKEN_LPAREN);
            return STATE_START;
        case ')':
            emit_token(lexer, TOKEN_RPAREN);
            return STATE_START;
        
        // Special sequences
        case '^':
            if (peek_ahead(lexer, 1) == '^') {
                advance(lexer, 1);
                emit_token(lexer, TOKEN_DOUBLE_CARET);
                return STATE_START;
            }
            return STATE_ERROR;
        
        // Directives
        case '@':
            lexer->token_start = lexer->current_pos;
            return STATE_DIRECTIVE;
        
        // Blank nodes
        case '_':
            if (peek_ahead(lexer, 1) == ':') {
                lexer->token_start = lexer->current_pos;
                advance(lexer, 1);
                return STATE_BLANK_NODE;
            }
            return STATE_ERROR;
        
        // End of input
        case '\0':
            return STATE_EOF;
        
        default:
            // Prefixed names and keywords
            if (is_alpha(ch)) {
                lexer->token_start = lexer->current_pos;
                return STATE_PREFIXED_NAME;
            }
            return STATE_ERROR;
    }
}
```

### 2. String States

Handles quoted string literals with escape sequences.

```c
static lexer_state_t handle_string(lexer_t* lexer, char ch) {
    switch (ch) {
        case '"':
            // End of string
            emit_string_token(lexer, TOKEN_STRING);
            return STATE_START;
        
        case '\\':
            // Escape sequence
            return STATE_STRING_ESCAPE;
        
        case '\n':
            // Unescaped newline in string is an error
            set_error(lexer, ERROR_UNTERMINATED_STRING);
            return STATE_ERROR;
        
        case '\0':
            // EOF in string
            set_error(lexer, ERROR_UNTERMINATED_STRING);
            return STATE_ERROR;
        
        default:
            // Accumulate character
            buffer_append(lexer, ch);
            return STATE_STRING;
    }
}

static lexer_state_t handle_string_escape(lexer_t* lexer, char ch) {
    char escaped;
    switch (ch) {
        case 't':  escaped = '\t'; break;
        case 'n':  escaped = '\n'; break;
        case 'r':  escaped = '\r'; break;
        case '"':  escaped = '"';  break;
        case '\\': escaped = '\\'; break;
        case 'u':
            // Unicode escape \uXXXX
            return handle_unicode_escape(lexer, 4);
        case 'U':
            // Unicode escape \UXXXXXXXX
            return handle_unicode_escape(lexer, 8);
        default:
            set_error(lexer, ERROR_INVALID_ESCAPE);
            return STATE_ERROR;
    }
    buffer_append(lexer, escaped);
    return STATE_STRING;
}
```

### 3. Number States

Handles integers and decimals with optional signs and exponents.

```c
static lexer_state_t handle_number(lexer_t* lexer, char ch) {
    if (is_digit(ch)) {
        return STATE_NUMBER;
    }
    
    if (ch == '.') {
        // Check if followed by digit
        if (is_digit(peek_ahead(lexer, 1))) {
            return STATE_DECIMAL;
        }
        // Otherwise, emit integer and let '.' be handled separately
        emit_number_token(lexer, TOKEN_NUMBER, false);
        backup(lexer);
        return STATE_START;
    }
    
    if (ch == 'e' || ch == 'E') {
        // Scientific notation
        return handle_exponent(lexer);
    }
    
    // End of number
    emit_number_token(lexer, TOKEN_NUMBER, false);
    backup(lexer);
    return STATE_START;
}

static lexer_state_t handle_decimal(lexer_t* lexer, char ch) {
    if (is_digit(ch)) {
        return STATE_DECIMAL;
    }
    
    if (ch == 'e' || ch == 'E') {
        return handle_exponent(lexer);
    }
    
    // End of decimal
    emit_number_token(lexer, TOKEN_NUMBER, true);
    backup(lexer);
    return STATE_START;
}
```

### 4. Prefixed Name State

Handles prefixed names like `foaf:name` and local names like `:localName`.

```c
static lexer_state_t handle_prefixed_name(lexer_t* lexer, char ch) {
    // First character already verified as alpha
    
    if (is_alnum(ch) || ch == '_' || ch == '-') {
        return STATE_PREFIXED_NAME;
    }
    
    if (ch == ':') {
        // Found prefix separator
        size_t prefix_len = lexer->current_pos - lexer->token_start;
        
        // Check for special case 'a' (rdf:type)
        if (prefix_len == 1 && lexer->input[lexer->token_start] == 'a') {
            // Check if followed by whitespace (not part of prefixed name)
            if (is_whitespace(peek_ahead(lexer, 1))) {
                emit_token(lexer, TOKEN_A);
                return STATE_START;
            }
        }
        
        // Continue parsing local part
        return handle_local_name(lexer);
    }
    
    // End of identifier - check if it's a keyword
    if (is_keyword(lexer)) {
        emit_keyword_token(lexer);
    } else {
        // Bare identifier (not valid in strict Turtle)
        set_error(lexer, ERROR_INVALID_PREFIX);
        return STATE_ERROR;
    }
    
    backup(lexer);
    return STATE_START;
}
```

## State Transition Table

A compact representation of the state machine for validation and documentation:

```c
// State transition table for character classes
typedef enum {
    CHAR_CLASS_WHITESPACE,
    CHAR_CLASS_ALPHA,
    CHAR_CLASS_DIGIT,
    CHAR_CLASS_PUNCT,
    CHAR_CLASS_SPECIAL,
    CHAR_CLASS_OTHER,
    CHAR_CLASS_EOF
} char_class_t;

// Transition table: [current_state][char_class] -> next_state
static const lexer_state_t transition_table[STATE_COUNT][CHAR_CLASS_COUNT] = {
    [STATE_START] = {
        [CHAR_CLASS_WHITESPACE] = STATE_WHITESPACE,
        [CHAR_CLASS_ALPHA]      = STATE_PREFIXED_NAME,
        [CHAR_CLASS_DIGIT]      = STATE_NUMBER,
        [CHAR_CLASS_PUNCT]      = STATE_START,  // Single char tokens
        [CHAR_CLASS_SPECIAL]    = STATE_START,  // Requires lookahead
        [CHAR_CLASS_OTHER]      = STATE_ERROR,
        [CHAR_CLASS_EOF]        = STATE_EOF
    },
    [STATE_STRING] = {
        [CHAR_CLASS_WHITESPACE] = STATE_STRING,
        [CHAR_CLASS_ALPHA]      = STATE_STRING,
        [CHAR_CLASS_DIGIT]      = STATE_STRING,
        [CHAR_CLASS_PUNCT]      = STATE_STRING,
        [CHAR_CLASS_SPECIAL]    = STATE_STRING_ESCAPE,  // backslash
        [CHAR_CLASS_OTHER]      = STATE_STRING,
        [CHAR_CLASS_EOF]        = STATE_ERROR
    },
    // ... more states
};
```

## Optimization Techniques

### 1. Character Classification with Lookup Tables

```c
// 256-byte lookup table for fast character classification
static const uint8_t char_class_table[256] = {
    [' ']  = CHAR_CLASS_WHITESPACE,
    ['\t'] = CHAR_CLASS_WHITESPACE,
    ['\n'] = CHAR_CLASS_WHITESPACE,
    ['\r'] = CHAR_CLASS_WHITESPACE,
    
    ['a'] = CHAR_CLASS_ALPHA, ['b'] = CHAR_CLASS_ALPHA, // ... etc
    ['A'] = CHAR_CLASS_ALPHA, ['B'] = CHAR_CLASS_ALPHA, // ... etc
    
    ['0'] = CHAR_CLASS_DIGIT, ['1'] = CHAR_CLASS_DIGIT, // ... etc
    
    ['.'] = CHAR_CLASS_PUNCT, [','] = CHAR_CLASS_PUNCT,
    [';'] = CHAR_CLASS_PUNCT, ['['] = CHAR_CLASS_PUNCT,
    // ... etc
};

static inline char_class_t classify_char(char ch) {
    return char_class_table[(unsigned char)ch];
}
```

### 2. SIMD String Scanning

```c
// SIMD-accelerated string scanning for common patterns
static size_t scan_identifier_simd(const char* input, size_t max_len) {
    __m128i input_vec;
    __m128i alpha_lower = _mm_set1_epi8('a' - 1);
    __m128i alpha_upper = _mm_set1_epi8('z' + 1);
    __m128i digit_lower = _mm_set1_epi8('0' - 1);
    __m128i digit_upper = _mm_set1_epi8('9' + 1);
    
    size_t pos = 0;
    while (pos + 16 <= max_len) {
        input_vec = _mm_loadu_si128((__m128i*)(input + pos));
        
        // Check if all characters are alphanumeric
        __m128i is_alpha = _mm_and_si128(
            _mm_cmpgt_epi8(input_vec, alpha_lower),
            _mm_cmplt_epi8(input_vec, alpha_upper)
        );
        __m128i is_digit = _mm_and_si128(
            _mm_cmpgt_epi8(input_vec, digit_lower),
            _mm_cmplt_epi8(input_vec, digit_upper)
        );
        __m128i is_alnum = _mm_or_si128(is_alpha, is_digit);
        
        int mask = _mm_movemask_epi8(is_alnum);
        if (mask != 0xFFFF) {
            // Found non-alphanumeric character
            pos += __builtin_ctz(~mask);
            break;
        }
        
        pos += 16;
    }
    
    // Handle remainder
    while (pos < max_len && is_alnum(input[pos])) {
        pos++;
    }
    
    return pos;
}
```

### 3. Branch-Free Token Classification

```c
// Branch-free token type determination for punctuation
static ttl_token_type_t classify_punct_branchless(char ch) {
    // Use multiplication and bit manipulation to avoid branches
    static const struct {
        char ch;
        ttl_token_type_t type;
    } punct_map[] = {
        {'.', TOKEN_DOT},
        {',', TOKEN_COMMA},
        {';', TOKEN_SEMICOLON},
        {'[', TOKEN_LBRACKET},
        {']', TOKEN_RBRACKET},
        {'(', TOKEN_LPAREN},
        {')', TOKEN_RPAREN},
    };
    
    uint64_t hash = ch * 0x45d9f3b;  // Simple hash
    size_t index = (hash >> 56) & 0x7;
    
    // Verify and return (branch will be predicted)
    return (punct_map[index].ch == ch) ? 
           punct_map[index].type : TOKEN_ERROR;
}
```

## Error Recovery Strategies

### 1. Synchronization Points

The lexer identifies synchronization points for error recovery:

```c
static const char sync_chars[] = {'.', ';', ']', '}'};

static bool is_sync_point(char ch) {
    for (size_t i = 0; i < sizeof(sync_chars); i++) {
        if (ch == sync_chars[i]) return true;
    }
    return false;
}

static lexer_state_t recover_to_sync_point(lexer_t* lexer) {
    while (lexer->current_pos < lexer->input_length) {
        char ch = current_char(lexer);
        if (is_sync_point(ch)) {
            // Skip the sync character
            advance(lexer, 1);
            return STATE_START;
        }
        advance(lexer, 1);
    }
    return STATE_EOF;
}
```

### 2. Context-Aware Recovery

Different recovery strategies based on the current context:

```c
static lexer_state_t handle_error_recovery(lexer_t* lexer) {
    switch (lexer->state) {
        case STATE_STRING:
        case STATE_LONG_STRING:
            // Look for closing quote
            return recover_string(lexer);
        
        case STATE_URI:
            // Look for closing >
            return recover_uri(lexer);
        
        case STATE_COMMENT:
            // Skip to end of line
            return skip_to_eol(lexer);
        
        default:
            // General recovery: skip to sync point
            return recover_to_sync_point(lexer);
    }
}
```

## Performance Metrics

### 1. State Transition Counters

```c
typedef struct {
    size_t transitions[STATE_COUNT][STATE_COUNT];
    size_t state_visits[STATE_COUNT];
    size_t total_transitions;
} state_metrics_t;

static void record_transition(lexer_t* lexer, 
                            lexer_state_t from, 
                            lexer_state_t to) {
    lexer->metrics.transitions[from][to]++;
    lexer->metrics.state_visits[to]++;
    lexer->metrics.total_transitions++;
}
```

### 2. Hot Path Analysis

```c
// Identify hot paths for optimization
static void analyze_hot_paths(const state_metrics_t* metrics) {
    typedef struct {
        lexer_state_t from;
        lexer_state_t to;
        size_t count;
    } transition_t;
    
    transition_t hot_transitions[10];
    find_top_transitions(metrics, hot_transitions, 10);
    
    printf("Hot transitions:\n");
    for (int i = 0; i < 10; i++) {
        printf("  %s -> %s: %zu (%.2f%%)\n",
               state_names[hot_transitions[i].from],
               state_names[hot_transitions[i].to],
               hot_transitions[i].count,
               100.0 * hot_transitions[i].count / metrics->total_transitions);
    }
}
```

## Integration with 7-Tick Timing

The state machine integrates with 7-tick timing constraints:

```c
typedef struct {
    uint64_t tick_budget;      // 7 ticks total
    uint64_t ticks_used;       // Current tick count
    size_t chars_per_tick;     // Target characters per tick
} timing_constraint_t;

static bool check_timing_constraint(lexer_t* lexer) {
    if (lexer->timing.ticks_used >= lexer->timing.tick_budget) {
        // Exceeded tick budget - need to yield
        lexer->needs_yield = true;
        return false;
    }
    
    // Check if we're on track
    size_t expected_chars = lexer->timing.chars_per_tick * 
                           lexer->timing.ticks_used;
    if (lexer->bytes_processed < expected_chars) {
        // Behind schedule - try to catch up
        lexer->fast_mode = true;
    }
    
    return true;
}
```

## State Machine Validation

Compile-time validation of state machine consistency:

```c
// Static assertions to validate state machine
_Static_assert(STATE_COUNT == sizeof(state_handlers) / sizeof(state_handlers[0]),
               "State handler array size mismatch");

_Static_assert(sizeof(transition_table) == 
               STATE_COUNT * CHAR_CLASS_COUNT * sizeof(lexer_state_t),
               "Transition table size mismatch");

// Runtime validation
static bool validate_state_machine(void) {
    // Check that all states have handlers
    for (int i = 0; i < STATE_COUNT; i++) {
        if (state_handlers[i] == NULL) {
            fprintf(stderr, "Missing handler for state %d\n", i);
            return false;
        }
    }
    
    // Check transition table consistency
    for (int s = 0; s < STATE_COUNT; s++) {
        for (int c = 0; c < CHAR_CLASS_COUNT; c++) {
            lexer_state_t next = transition_table[s][c];
            if (next >= STATE_COUNT) {
                fprintf(stderr, "Invalid transition [%d][%d] -> %d\n",
                        s, c, next);
                return false;
            }
        }
    }
    
    return true;
}
```