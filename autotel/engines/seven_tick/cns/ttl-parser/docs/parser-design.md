# TTL Recursive Descent Parser Design

## Overview

This document outlines the design for a recursive descent parser for TTL (Turtle) files. The parser follows a predictive parsing approach with single-token lookahead, implementing one function per grammar rule for clarity and maintainability.

## Architecture Overview

```
┌─────────────────┐
│   TTL Input     │
└────────┬────────┘
         │
    ┌────▼────┐
    │ Lexer   │ → Token Stream
    └────┬────┘
         │
  ┌──────▼──────┐
  │   Parser    │ → AST
  └──────┬──────┘
         │
   ┌─────▼─────┐
   │  AST API  │
   └───────────┘
```

## Parser Function Hierarchy

The parser functions directly mirror the grammar rules for clarity:

### Document Level
```c
// Entry point - Rule [1]
ASTNode* parseTurtleDoc(Parser* parser);

// Rule [2]
ASTNode* parseStatement(Parser* parser);

// Rule [3]
ASTNode* parseDirective(Parser* parser);
```

### Directive Parsing
```c
// Rule [4]
ASTNode* parsePrefixID(Parser* parser);

// Rule [5]
ASTNode* parseBase(Parser* parser);

// Rule [5s]
ASTNode* parseSparqlBase(Parser* parser);

// Rule [6s]
ASTNode* parseSparqlPrefix(Parser* parser);
```

### Triple Parsing
```c
// Rule [6]
ASTNode* parseTriples(Parser* parser);

// Rule [7]
ASTNode* parsePredicateObjectList(Parser* parser);

// Rule [8]
ASTNode* parseObjectList(Parser* parser);

// Rule [9]
ASTNode* parseVerb(Parser* parser);

// Rule [10]
ASTNode* parseSubject(Parser* parser);

// Rule [11]
ASTNode* parsePredicate(Parser* parser);

// Rule [12]
ASTNode* parseObject(Parser* parser);
```

### Literal Parsing
```c
// Rule [13]
ASTNode* parseLiteral(Parser* parser);

// Rule [16]
ASTNode* parseNumericLiteral(Parser* parser);

// Rule [128s]
ASTNode* parseRDFLiteral(Parser* parser);

// Rule [133s]
ASTNode* parseBooleanLiteral(Parser* parser);
```

### Collections and Blank Nodes
```c
// Rule [14]
ASTNode* parseBlankNodePropertyList(Parser* parser);

// Rule [15]
ASTNode* parseCollection(Parser* parser);

// Rule [137s]
ASTNode* parseBlankNode(Parser* parser);
```

### IRI Parsing
```c
// Rule [135s]
ASTNode* parseIRI(Parser* parser);

// Rule [136s]
ASTNode* parsePrefixedName(Parser* parser);
```

## AST Node Types

### Core Node Structure
```c
typedef enum {
    AST_DOCUMENT,
    AST_PREFIX_DECL,
    AST_BASE_DECL,
    AST_TRIPLE,
    AST_SUBJECT,
    AST_PREDICATE,
    AST_OBJECT,
    AST_IRI,
    AST_PREFIXED_NAME,
    AST_BLANK_NODE,
    AST_LITERAL,
    AST_COLLECTION,
    AST_BLANK_NODE_PROPERTY_LIST
} ASTNodeType;

typedef struct ASTNode {
    ASTNodeType type;
    union {
        DocumentNode document;
        PrefixDeclNode prefix;
        BaseDeclNode base;
        TripleNode triple;
        IRINode iri;
        PrefixedNameNode prefixed_name;
        BlankNodeNode blank_node;
        LiteralNode literal;
        CollectionNode collection;
        PropertyListNode property_list;
    } data;
    struct ASTNode* next;  // For lists
} ASTNode;
```

### Specific Node Types

```c
// Document node - holds all statements
typedef struct {
    ASTNode* statements;  // Linked list
    size_t statement_count;
} DocumentNode;

// Prefix declaration
typedef struct {
    char* prefix;
    char* iri;
} PrefixDeclNode;

// Base declaration
typedef struct {
    char* iri;
} BaseDeclNode;

// Triple node
typedef struct {
    ASTNode* subject;
    ASTNode* predicate_object_list;
} TripleNode;

// IRI node
typedef struct {
    char* value;
    bool is_absolute;
} IRINode;

// Prefixed name node
typedef struct {
    char* prefix;
    char* local_name;
} PrefixedNameNode;

// Blank node
typedef struct {
    char* label;  // NULL for anonymous
    bool is_anonymous;
} BlankNodeNode;

// Literal node
typedef struct {
    enum {
        LITERAL_STRING,
        LITERAL_INTEGER,
        LITERAL_DECIMAL,
        LITERAL_DOUBLE,
        LITERAL_BOOLEAN
    } literal_type;
    char* value;
    char* language_tag;  // Optional
    ASTNode* datatype_iri;  // Optional
} LiteralNode;

// Collection node (RDF list)
typedef struct {
    ASTNode* elements;  // Linked list
    size_t element_count;
} CollectionNode;

// Blank node property list
typedef struct {
    ASTNode* predicate_object_pairs;  // Linked list
} PropertyListNode;
```

## Error Recovery Strategy

### Synchronization Points

The parser implements error recovery at key synchronization points:

1. **Statement Level**: On error, skip to next '.' or directive keyword
2. **Triple Level**: Skip to ';' or '.' 
3. **Object List**: Skip to ',' or ';' or '.'
4. **Collection**: Skip to matching ')'
5. **Property List**: Skip to matching ']'

### Error Handling Functions

```c
typedef struct {
    int line;
    int column;
    char* message;
    ErrorSeverity severity;
} ParseError;

// Error recovery
void synchronizeTo(Parser* parser, TokenType* sync_tokens, int count);
void reportError(Parser* parser, const char* message);
bool recoverFromError(Parser* parser);

// Error collection
typedef struct {
    ParseError* errors;
    size_t error_count;
    size_t error_capacity;
    bool fatal_error;
} ErrorCollector;
```

### Recovery Implementation

```c
// Example: Recover at statement level
ASTNode* parseStatement(Parser* parser) {
    if (isDirectiveStart(parser->current_token)) {
        return parseDirective(parser);
    }
    
    ASTNode* triples = parseTriples(parser);
    if (!triples) {
        // Error in triple parsing
        TokenType sync[] = {TOKEN_DOT, TOKEN_AT_PREFIX, TOKEN_AT_BASE};
        synchronizeTo(parser, sync, 3);
        return NULL;
    }
    
    if (!consume(parser, TOKEN_DOT)) {
        reportError(parser, "Expected '.' after triples");
        synchronizeTo(parser, sync, 3);
        return NULL;
    }
    
    return triples;
}
```

## Memory Management Approach

### Arena Allocation

Use arena allocation for efficient memory management:

```c
typedef struct Arena {
    char* memory;
    size_t size;
    size_t used;
    struct Arena* next;
} Arena;

typedef struct {
    Arena* first;
    Arena* current;
    size_t default_size;
} ArenaAllocator;

// Allocation functions
void* arenaAlloc(ArenaAllocator* arena, size_t size);
char* arenaStrdup(ArenaAllocator* arena, const char* str);
void arenaReset(ArenaAllocator* arena);
void arenaDestroy(ArenaAllocator* arena);
```

### String Interning

Implement string interning for efficient IRI and prefix storage:

```c
typedef struct StringIntern {
    char** strings;
    size_t count;
    size_t capacity;
    ArenaAllocator* arena;
} StringIntern;

const char* internString(StringIntern* intern, const char* str);
```

### AST Lifecycle

```c
// Create parser with arena
Parser* createParser(Lexer* lexer) {
    Parser* parser = malloc(sizeof(Parser));
    parser->arena = createArena(64 * 1024);  // 64KB chunks
    parser->string_intern = createStringIntern(parser->arena);
    return parser;
}

// Parse and get AST
ASTNode* parse(Parser* parser) {
    return parseTurtleDoc(parser);
}

// Cleanup - single operation
void destroyParser(Parser* parser) {
    arenaDestroy(parser->arena);  // Frees all AST nodes
    free(parser);
}
```

## Parser API Design

### Public API

```c
// Main parser structure
typedef struct Parser Parser;

// Create/destroy
Parser* ttl_parser_create(const char* input, size_t length);
Parser* ttl_parser_create_from_file(FILE* file);
void ttl_parser_destroy(Parser* parser);

// Parsing
ASTNode* ttl_parser_parse(Parser* parser);

// Error handling
bool ttl_parser_has_errors(Parser* parser);
size_t ttl_parser_error_count(Parser* parser);
const ParseError* ttl_parser_get_error(Parser* parser, size_t index);

// Configuration
void ttl_parser_set_base_iri(Parser* parser, const char* base);
void ttl_parser_set_strict_mode(Parser* parser, bool strict);
```

### AST Visitor API

```c
// Visitor pattern for AST traversal
typedef struct {
    void* user_data;
    
    // Visit functions
    void (*visit_document)(void* data, DocumentNode* node);
    void (*visit_prefix_decl)(void* data, PrefixDeclNode* node);
    void (*visit_base_decl)(void* data, BaseDeclNode* node);
    void (*visit_triple)(void* data, TripleNode* node);
    void (*visit_iri)(void* data, IRINode* node);
    void (*visit_prefixed_name)(void* data, PrefixedNameNode* node);
    void (*visit_blank_node)(void* data, BlankNodeNode* node);
    void (*visit_literal)(void* data, LiteralNode* node);
    void (*visit_collection)(void* data, CollectionNode* node);
    void (*visit_property_list)(void* data, PropertyListNode* node);
} ASTVisitor;

void ttl_ast_accept(ASTNode* node, ASTVisitor* visitor);
```

### Streaming API

For large files, provide a streaming interface:

```c
// Streaming parser callbacks
typedef struct {
    void* user_data;
    
    // Called for each complete triple
    void (*on_triple)(void* data, 
                     const char* subject,
                     const char* predicate, 
                     const char* object);
    
    // Called for prefix declarations
    void (*on_prefix)(void* data,
                     const char* prefix,
                     const char* iri);
    
    // Called for base declaration
    void (*on_base)(void* data, const char* iri);
    
    // Error callback
    void (*on_error)(void* data, const ParseError* error);
} StreamingCallbacks;

// Streaming parser
typedef struct StreamingParser StreamingParser;

StreamingParser* ttl_streaming_parser_create(StreamingCallbacks* callbacks);
void ttl_streaming_parser_feed(StreamingParser* parser, 
                              const char* data, 
                              size_t length);
void ttl_streaming_parser_end(StreamingParser* parser);
void ttl_streaming_parser_destroy(StreamingParser* parser);
```

## Implementation Strategy

### Phase 1: Core Parser
1. Implement lexer with all token types
2. Implement basic parser functions
3. Create AST node types
4. Basic error reporting

### Phase 2: Full Grammar Support
1. Complete all grammar rules
2. Unicode support for identifiers
3. Escape sequence handling
4. Numeric literal parsing

### Phase 3: Error Recovery
1. Implement synchronization points
2. Add error recovery logic
3. Improve error messages
4. Add error context

### Phase 4: Optimization
1. Implement string interning
2. Add arena allocation
3. Optimize token lookahead
4. Add streaming support

### Phase 5: API Polish
1. Complete public API
2. Add visitor pattern
3. Create examples
4. Write documentation

## Performance Considerations

1. **Single Token Lookahead**: Grammar is LL(1), requiring only one token lookahead
2. **Arena Allocation**: Reduces allocation overhead and improves cache locality
3. **String Interning**: Reduces memory usage for repeated IRIs and prefixes
4. **Streaming Support**: Allows parsing of files larger than memory
5. **Error Recovery**: Continues parsing after errors for better diagnostics

## Testing Strategy

1. **Unit Tests**: Test each parser function individually
2. **Integration Tests**: Test complete TTL documents
3. **W3C Test Suite**: Validate against official test cases
4. **Fuzzing**: Test parser robustness with malformed input
5. **Performance Tests**: Benchmark parsing speed and memory usage

## Example Usage

```c
// Parse a TTL file
Parser* parser = ttl_parser_create_from_file(file);
ASTNode* ast = ttl_parser_parse(parser);

if (ttl_parser_has_errors(parser)) {
    for (size_t i = 0; i < ttl_parser_error_count(parser); i++) {
        const ParseError* error = ttl_parser_get_error(parser, i);
        fprintf(stderr, "Error at %d:%d: %s\n", 
                error->line, error->column, error->message);
    }
} else {
    // Process AST
    MyVisitor visitor = { .user_data = ... };
    ttl_ast_accept(ast, &visitor);
}

ttl_parser_destroy(parser);
```

## Next Steps

1. Implement the lexer according to the token specifications
2. Create the parser structure and basic infrastructure
3. Implement parser functions following the grammar rules
4. Build comprehensive test suite
5. Optimize for performance and memory usage