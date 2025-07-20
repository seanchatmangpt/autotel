# TTL (Time-To-Live) Lexer

A high-performance C lexer for parsing TTL (Time-To-Live) expressions with support for various time units and arithmetic operations.

## Features

- **Time Units Supported**:
  - `ns` - nanoseconds
  - `us` - microseconds  
  - `ms` - milliseconds
  - `s` - seconds
  - `m` - minutes
  - `h` - hours
  - `d` - days
  - `w` - weeks
  - `mo` - months (30 days)
  - `y` - years (365 days)

- **Numeric Support**:
  - Integers: `42`, `100`, `0`
  - Decimals: `3.14`, `0.5`, `1.5`

- **Operators**:
  - Addition: `+`
  - Subtraction: `-`
  - Multiplication: `*`
  - Division: `/`
  - Grouping: `(` and `)`

- **Performance Features**:
  - Zero-allocation design
  - State machine implementation
  - Optimized character classification
  - Lookahead caching
  - Memory-efficient buffer management

## Building

```bash
# Build all targets
make

# Run tests
make test

# Run demo
make demo

# Debug build
make debug

# Performance optimized build
make perf
```

## Usage Example

```c
#include "ttl_lexer.h"

// Initialize lexer
ttl_lexer_t lexer;
ttl_token_t token;
const char *expr = "5m 30s";

ttl_lexer_init(&lexer, expr, strlen(expr));

// Tokenize expression
while (ttl_lexer_next_token(&lexer, &token)) {
    if (token.type == TOKEN_EOF) break;
    
    printf("Token: %s\n", ttl_token_type_name(token.type));
    
    if (token.type == TOKEN_INTEGER) {
        printf("  Value: %lld\n", token.value.integer_value);
    }
}

// Cleanup
ttl_lexer_cleanup(&lexer);
```

## Token Types

The lexer produces the following token types:

- `TOKEN_INTEGER` - Integer numbers
- `TOKEN_DECIMAL` - Decimal numbers
- `TOKEN_NANOSECOND` through `TOKEN_YEAR` - Time units
- `TOKEN_PLUS`, `TOKEN_MINUS`, `TOKEN_MULTIPLY`, `TOKEN_DIVIDE` - Operators
- `TOKEN_LPAREN`, `TOKEN_RPAREN` - Parentheses
- `TOKEN_EOF` - End of input
- `TOKEN_ERROR` - Lexical error

## API Functions

### Core Functions

- `ttl_lexer_init()` - Initialize lexer with source string
- `ttl_lexer_next_token()` - Get next token
- `ttl_lexer_peek_token()` - Look ahead without consuming
- `ttl_lexer_reset()` - Reset to beginning
- `ttl_lexer_cleanup()` - Clean up resources

### Utility Functions

- `ttl_token_type_name()` - Get human-readable token name
- `ttl_convert_to_nanoseconds()` - Convert time value to nanoseconds
- `ttl_lexer_get_error()` - Get error message

## Performance

The lexer is designed for high performance:

- **Zero allocations** during tokenization
- **Single-pass** scanning
- **Optimized** character classification
- **Cache-friendly** data structures
- **Minimal branching** in hot paths

## Error Handling

The lexer provides detailed error reporting:

```c
if (!ttl_lexer_next_token(&lexer, &token)) {
    printf("Error: %s\n", ttl_lexer_get_error(&lexer));
    printf("At line %zu, column %zu\n", token.line, token.column);
}
```

## Examples

### Simple TTL Expression
```
Input: "30s"
Tokens: INTEGER(30), SECOND, EOF
```

### Complex Expression
```
Input: "(5m + 30s) * 2"
Tokens: LPAREN, INTEGER(5), MINUTE, PLUS, INTEGER(30), SECOND, RPAREN, MULTIPLY, INTEGER(2), EOF
```

### Decimal Values
```
Input: "1.5h"
Tokens: DECIMAL(1.5), HOUR, EOF
```

## Testing

The lexer includes a comprehensive test suite:

```bash
make test
```

Tests cover:
- Basic integer and decimal parsing
- All time unit recognition
- Operator tokenization
- Complex expressions
- Error handling
- Edge cases
- Whitespace handling

## Integration

To integrate the TTL lexer into your project:

1. Copy `include/ttl_lexer.h` and `src/ttl_lexer.c`
2. Include the header: `#include "ttl_lexer.h"`
3. Compile and link the source file

## License

This lexer is part of the CNS (Cognitive Narrative System) project.