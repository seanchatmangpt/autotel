# TTL Lexer State Machine Design

## Overview

This document describes the lexical analyzer (lexer) design for parsing TTL (Turtle) format. The lexer uses a state machine approach to tokenize TTL input into a stream of tokens for the parser.

## Token Types

### 1. IRI Tokens
- **IRI_REF**: Absolute IRI enclosed in angle brackets
  - Example: `<http://example.org/resource>`
  - Pattern: `<[^<>"{}|^`\\]\\x00-\\x20]*>`

- **PREFIXED_NAME**: Prefixed name with optional local part
  - Example: `foaf:Person`, `rdf:type`, `:hasName`
  - Pattern: `(PN_PREFIX)? ':' (PN_LOCAL)?`

### 2. Literal Tokens
- **STRING_LITERAL_QUOTE**: Single-line string with double quotes
  - Example: `"Hello World"`
  - Pattern: `"([^"\\]|\\.)*"`

- **STRING_LITERAL_SINGLE_QUOTE**: Single-line string with single quotes
  - Example: `'Hello World'`
  - Pattern: `'([^'\\]|\\.)*'`

- **STRING_LITERAL_LONG_QUOTE**: Multi-line string with triple double quotes
  - Example: `"""Multi\nline\nstring"""`
  - Pattern: `"""((.|\n)*?)"""`

- **STRING_LITERAL_LONG_SINGLE_QUOTE**: Multi-line string with triple single quotes
  - Example: `'''Multi\nline\nstring'''`
  - Pattern: `'''((.|\n)*?)'''`

- **INTEGER**: Integer literal
  - Example: `42`, `-123`, `+999`
  - Pattern: `[+-]?[0-9]+`

- **DECIMAL**: Decimal literal
  - Example: `3.14`, `-0.5`, `+12.34`
  - Pattern: `[+-]?[0-9]*\.[0-9]+`

- **DOUBLE**: Double precision float
  - Example: `1.23E4`, `-3.14e-10`
  - Pattern: `[+-]?([0-9]+\.[0-9]*[eE][+-]?[0-9]+|[0-9]*\.[0-9]+[eE][+-]?[0-9]+|[0-9]+[eE][+-]?[0-9]+)`

- **BOOLEAN**: Boolean literal
  - Example: `true`, `false`
  - Pattern: `true|false`

### 3. Node Tokens
- **BLANK_NODE_LABEL**: Labeled blank node
  - Example: `_:node1`, `_:b123`
  - Pattern: `_:[A-Za-z0-9_]+`

- **ANON**: Anonymous blank node
  - Example: `[]`
  - Pattern: `\[\]`

### 4. Keywords and Directives
- **PREFIX**: Prefix directive keyword
  - Example: `@prefix`
  - Pattern: `@prefix`

- **BASE**: Base directive keyword
  - Example: `@base`
  - Pattern: `@base`

- **A**: RDF type abbreviation
  - Example: `a`
  - Pattern: `a`

### 5. Punctuation and Delimiters
- **DOT**: Statement terminator `.`
- **SEMICOLON**: Predicate-object list separator `;`
- **COMMA**: Object list separator `,`
- **OPEN_PAREN**: Opening parenthesis `(`
- **CLOSE_PAREN**: Closing parenthesis `)`
- **OPEN_BRACKET**: Opening bracket `[`
- **CLOSE_BRACKET**: Closing bracket `]`
- **OPEN_BRACE**: Opening brace `{` (for quoted triples)
- **CLOSE_BRACE**: Closing brace `}` (for quoted triples)
- **DOUBLE_CARET**: Datatype indicator `^^`
- **AT**: Language tag indicator `@`

### 6. Special Tokens
- **COMMENT**: Comment (ignored by parser)
  - Example: `# This is a comment`
  - Pattern: `#[^\n\r]*`

- **WHITESPACE**: Whitespace (usually ignored)
  - Pattern: `[ \t\r\n]+`

- **EOF**: End of file marker

## State Machine States

### Primary States

1. **INITIAL**: Default state, ready to recognize any token
   - Transitions based on first character
   - Skips whitespace
   - Handles comment detection

2. **IN_IRI**: Inside an IRI reference
   - Entered on `<`
   - Accumulates characters until `>`
   - Handles escape sequences

3. **IN_STRING_DQ**: Inside double-quoted string
   - Entered on `"`
   - Handles escape sequences
   - Checks for triple quotes

4. **IN_STRING_SQ**: Inside single-quoted string
   - Entered on `'`
   - Handles escape sequences
   - Checks for triple quotes

5. **IN_STRING_LONG_DQ**: Inside triple double-quoted string
   - Entered on `"""`
   - Allows newlines
   - Handles escape sequences

6. **IN_STRING_LONG_SQ**: Inside triple single-quoted string
   - Entered on `'''`
   - Allows newlines
   - Handles escape sequences

7. **IN_NUMBER**: Recognizing numeric literal
   - Handles integers, decimals, doubles
   - Tracks decimal points and exponents

8. **IN_PREFIXED_NAME**: Recognizing prefixed name
   - Handles prefix part
   - Transitions at `:`
   - Handles local part

9. **IN_BLANK_NODE**: Recognizing blank node label
   - Entered on `_:`
   - Accumulates valid characters

10. **IN_COMMENT**: Inside a comment
    - Entered on `#`
    - Continues until newline

11. **IN_DIRECTIVE**: Recognizing @prefix or @base
    - Entered on `@`
    - Identifies directive type

12. **IN_LANGUAGE_TAG**: Recognizing language tag
    - Entered on `@` after string literal
    - Follows BCP47 rules

## State Transition Diagram

```
                    ┌─────────┐
                    │ INITIAL │
                    └────┬────┘
                         │
    ┌────────────────────┼────────────────────┐
    │                    │                    │
    ▼                    ▼                    ▼
┌───────┐          ┌──────────┐         ┌─────────┐
│IN_IRI │          │IN_STRING │         │IN_NUMBER│
└───┬───┘          └────┬─────┘         └────┬────┘
    │'>'                │'"'                 │[0-9]
    ▼                   ▼                    ▼
 [IRI_REF]         [STRING_LIT]          [NUMBER]

                    ┌──────────┐
                    │IN_PREFIX │
                    └────┬─────┘
                         │':'
                         ▼
                   ┌──────────┐
                   │IN_LOCAL  │
                   └──────────┘

                  ┌────────────┐
                  │IN_COMMENT  │
                  └────────────┘
```

## Token Structure

```typescript
interface Token {
  type: TokenType;        // Enum of all token types
  value: string;          // Raw token value
  line: number;           // Line number (1-based)
  column: number;         // Column number (1-based)
  startPos: number;       // Absolute position in input
  endPos: number;         // End position in input
  trivia?: {
    leadingWhitespace?: string;
    trailingWhitespace?: string;
    comments?: string[];
  };
}

enum TokenType {
  // IRI tokens
  IRI_REF,
  PREFIXED_NAME,
  
  // Literals
  STRING_LITERAL_QUOTE,
  STRING_LITERAL_SINGLE_QUOTE,
  STRING_LITERAL_LONG_QUOTE,
  STRING_LITERAL_LONG_SINGLE_QUOTE,
  INTEGER,
  DECIMAL,
  DOUBLE,
  BOOLEAN,
  
  // Nodes
  BLANK_NODE_LABEL,
  ANON,
  
  // Keywords
  PREFIX,
  BASE,
  A,
  
  // Punctuation
  DOT,
  SEMICOLON,
  COMMA,
  OPEN_PAREN,
  CLOSE_PAREN,
  OPEN_BRACKET,
  CLOSE_BRACKET,
  OPEN_BRACE,
  CLOSE_BRACE,
  DOUBLE_CARET,
  AT,
  
  // Special
  COMMENT,
  WHITESPACE,
  EOF,
  
  // Error
  INVALID
}
```

## Error Handling Strategy

### 1. Lexical Errors

- **Unterminated String**: Missing closing quote
  - Recovery: Assume string ends at line break
  - Error token with partial string value

- **Invalid IRI**: Malformed IRI characters
  - Recovery: Create error token, continue from next valid character
  - Report illegal characters in IRI

- **Invalid Escape Sequence**: Unrecognized escape in string
  - Recovery: Include literal characters
  - Warning for invalid escape

- **Incomplete Token**: EOF in middle of token
  - Recovery: Return partial token with error flag
  - Report unexpected EOF

### 2. Error Recovery

```typescript
interface LexerError {
  type: 'unterminated_string' | 'invalid_iri' | 'invalid_escape' | 'unexpected_char';
  message: string;
  line: number;
  column: number;
  context: string;  // Surrounding text
  suggestion?: string;  // Suggested fix
}
```

### 3. Error Reporting

- Maintain error buffer
- Continue lexing after errors
- Provide context for errors
- Suggest common fixes

## Unicode and Escape Sequence Support

### 1. Unicode Support

- **IRI Unicode**: Full Unicode support in IRIs
  - UTF-8 encoding
  - Percent-encoding for special characters

- **String Unicode**: Unicode in string literals
  - Direct UTF-8 characters
  - Unicode escape sequences: `\uXXXX`, `\UXXXXXXXX`

### 2. Escape Sequences

```
String Escapes:
\t - Tab
\n - Newline
\r - Carriage return
\b - Backspace
\f - Form feed
\" - Double quote
\' - Single quote
\\ - Backslash
\uXXXX - Unicode 4-hex
\UXXXXXXXX - Unicode 8-hex
```

### 3. Character Classes

```
PN_CHARS_BASE ::= [A-Z] | [a-z] | [#x00C0-#x00D6] | [#x00D8-#x00F6] | ...
PN_CHARS ::= PN_CHARS_BASE | '-' | [0-9] | #x00B7 | [#x0300-#x036F] | ...
```

## Performance Considerations

### 1. Optimization Strategies

- **Character Lookup Tables**: Fast character classification
  ```typescript
  const CHAR_CLASSES = new Uint8Array(256);
  // Precomputed character classes for ASCII
  ```

- **String Interning**: Reuse common string values
  - Intern keywords, common prefixes
  - Reduce memory allocation

- **Buffer Management**: Efficient token value building
  - Preallocate buffers
  - Reuse buffers between tokens

### 2. Streaming Support

- **Incremental Lexing**: Support chunk-based input
  - Save state between chunks
  - Handle tokens spanning chunks

- **Lookahead Optimization**: Minimize lookahead
  - Most tokens decidable with 1-2 character lookahead
  - Special handling for triple quotes

### 3. Memory Efficiency

- **Token Pooling**: Reuse token objects
- **Lazy String Creation**: Create strings only when needed
- **Compact Token Representation**: Minimize token size

## Implementation Guidelines

### 1. Lexer Interface

```typescript
interface Lexer {
  // Initialize with input
  init(input: string | Buffer): void;
  
  // Get next token
  nextToken(): Token;
  
  // Peek at next token without consuming
  peekToken(): Token;
  
  // Get all tokens (for debugging)
  tokenize(): Token[];
  
  // Error handling
  getErrors(): LexerError[];
  
  // State management
  saveState(): LexerState;
  restoreState(state: LexerState): void;
}
```

### 2. State Machine Implementation

```typescript
class LexerStateMachine {
  private state: LexerState = LexerState.INITIAL;
  private buffer: string = '';
  private position: number = 0;
  private line: number = 1;
  private column: number = 1;
  
  transition(input: char): void {
    switch (this.state) {
      case LexerState.INITIAL:
        this.handleInitial(input);
        break;
      case LexerState.IN_IRI:
        this.handleInIRI(input);
        break;
      // ... other states
    }
  }
}
```

### 3. Testing Strategy

- **Token Recognition Tests**: Each token type
- **Error Recovery Tests**: Malformed input
- **Unicode Tests**: Various Unicode scenarios
- **Performance Tests**: Large files, pathological cases
- **Streaming Tests**: Chunked input

## Future Enhancements

1. **Parallel Lexing**: Multi-threaded tokenization for large files
2. **Incremental Re-lexing**: Efficient updates for editing
3. **Custom Token Filters**: Application-specific token handling
4. **Syntax Highlighting Support**: Rich editor integration
5. **Error Recovery Heuristics**: Smart error correction suggestions

## References

- [W3C Turtle Specification](https://www.w3.org/TR/turtle/)
- [RDF 1.1 Concepts](https://www.w3.org/TR/rdf11-concepts/)
- [Unicode in RDF](https://www.w3.org/TR/rdf11-concepts/#section-IRIs)