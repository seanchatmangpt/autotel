/**
 * @file parser.c
 * @brief TTL/Turtle recursive descent parser implementation
 * @author CNS Seven-Tick Engine
 * @date 2024
 * 
 * This file implements a recursive descent parser for TTL (Turtle) documents
 * following the W3C specification with error recovery and streaming support.
 */

#include "parser.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>

/* Parser state structure */
struct Parser {
    Lexer* lexer;                    /* Lexer instance */
    Token* current_token;            /* Current lookahead token */
    Token* previous_token;           /* Previous token for error reporting */
    ttl_ast_context_t* ast_context;  /* AST node creation context */
    ParserOptions options;           /* Parser configuration */
    ParserStats stats;               /* Parser statistics */
    
    /* Error handling */
    ParseError* errors;              /* Linked list of errors */
    ParseError* last_error;          /* Last error in list */
    size_t error_count;              /* Total error count */
    bool fatal_error;                /* Fatal error occurred */
    
    /* Parser state */
    char* base_iri;                  /* Current base IRI */
    struct {
        char* prefix;
        char* iri;
    }* prefixes;                     /* Prefix mappings */
    size_t prefix_count;
    size_t prefix_capacity;
    
    /* Performance tracking */
    clock_t start_time;
    
    /* Memory management */
    struct {
        void* memory;
        size_t size;
        size_t used;
    } string_pool;                   /* String interning pool */
};

/* Forward declarations for recursive descent functions */
static ttl_ast_node_t* parseTurtleDoc(Parser* parser);
static ttl_ast_node_t* parseStatement(Parser* parser);
static ttl_ast_node_t* parseDirective(Parser* parser);
static ttl_ast_node_t* parsePrefixID(Parser* parser);
static ttl_ast_node_t* parseBase(Parser* parser);
// TODO: Implement SPARQL directives
// static ttl_ast_node_t* parseSparqlBase(Parser* parser);
// static ttl_ast_node_t* parseSparqlPrefix(Parser* parser);
static ttl_ast_node_t* parseTriples(Parser* parser);
static ttl_ast_node_t* parsePredicateObjectList(Parser* parser);
static ttl_ast_node_t* parseObjectList(Parser* parser);
static ttl_ast_node_t* parseVerb(Parser* parser);
static ttl_ast_node_t* parseSubject(Parser* parser);
static ttl_ast_node_t* parsePredicate(Parser* parser);
static ttl_ast_node_t* parseObject(Parser* parser);
static ttl_ast_node_t* parseLiteral(Parser* parser);
static ttl_ast_node_t* parseNumericLiteral(Parser* parser);
static ttl_ast_node_t* parseRDFLiteral(Parser* parser);
static ttl_ast_node_t* parseBooleanLiteral(Parser* parser);
static ttl_ast_node_t* parseBlankNodePropertyList(Parser* parser);
static ttl_ast_node_t* parseCollection(Parser* parser);
static ttl_ast_node_t* parseBlankNode(Parser* parser);
static ttl_ast_node_t* parseIRI(Parser* parser);
static ttl_ast_node_t* parsePrefixedName(Parser* parser);

/* Helper functions */
static void advance(Parser* parser);
static bool consume(Parser* parser, TokenType expected);
static bool match(Parser* parser, TokenType type);
static bool check(Parser* parser, TokenType type);
static bool isAtEnd(Parser* parser);
static Token* peek(Parser* parser);
static Token* previous(Parser* parser);

/* Error handling */
static void reportError(Parser* parser, const char* message);
static void reportErrorAt(Parser* parser, Token* token, const char* message);
static ParseError* createError(const char* message, Token* token, ErrorSeverity severity);
static void addError(Parser* parser, ParseError* error);
static void synchronize(Parser* parser);
static void synchronizeTo(Parser* parser, const TokenType* sync_tokens, int count);
static bool recoverFromError(Parser* parser);

/* Utility functions */
static char* copyString(Parser* parser, const char* str);
static void addPrefix(Parser* parser, const char* prefix, const char* iri);
static const char* lookupPrefix(Parser* parser, const char* prefix);
static char* resolveIRI(Parser* parser, const char* iri);
static bool isDirectiveStart(Token* token);

/* Memory management */
static void* parserAlloc(Parser* parser, size_t size);
static void initStringPool(Parser* parser);
static void destroyStringPool(Parser* parser);

/**
 * Create parser from string input
 */
Parser* ttl_parser_create(const char* input, size_t length, const ParserOptions* options) {
    Parser* parser = (Parser*)calloc(1, sizeof(Parser));
    if (!parser) return NULL;
    
    /* Create lexer */
    parser->lexer = lexer_create(NULL);
    if (!parser->lexer) {
        free(parser);
        return NULL;
    }
    
    /* Initialize lexer with input */
    if (length == 0) length = strlen(input);
    lexer_init(parser->lexer, input, length);
    
    /* Set options */
    if (options) {
        parser->options = *options;
    } else {
        parser->options = ttl_parser_default_options();
    }
    
    /* Initialize parser state */
    parser->ast_context = ttl_ast_context_create(true);
    parser->start_time = clock();
    initStringPool(parser);
    
    /* Set base IRI if provided */
    if (parser->options.base_iri) {
        parser->base_iri = copyString(parser, parser->options.base_iri);
    }
    
    /* Allocate prefix array */
    parser->prefix_capacity = 16;
    parser->prefixes = calloc(parser->prefix_capacity, sizeof(parser->prefixes[0]));
    
    /* Get first token */
    advance(parser);
    
    return parser;
}

/**
 * Create parser from file
 */
Parser* ttl_parser_create_from_file(FILE* file, const ParserOptions* options) {
    if (!file) return NULL;
    
    /* Read file into buffer */
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (file_size < 0) return NULL;
    
    char* buffer = (char*)malloc(file_size + 1);
    if (!buffer) return NULL;
    
    size_t read_size = fread(buffer, 1, file_size, file);
    buffer[read_size] = '\0';
    
    Parser* parser = ttl_parser_create(buffer, read_size, options);
    free(buffer);
    
    return parser;
}

/**
 * Create parser with existing lexer
 */
Parser* ttl_parser_create_with_lexer(Lexer* lexer, const ParserOptions* options) {
    if (!lexer) return NULL;
    
    Parser* parser = (Parser*)calloc(1, sizeof(Parser));
    if (!parser) return NULL;
    
    parser->lexer = lexer;
    
    /* Set options */
    if (options) {
        parser->options = *options;
    } else {
        parser->options = ttl_parser_default_options();
    }
    
    /* Initialize parser state */
    parser->ast_context = ttl_ast_context_create(true);
    parser->start_time = clock();
    initStringPool(parser);
    
    /* Set base IRI if provided */
    if (parser->options.base_iri) {
        parser->base_iri = copyString(parser, parser->options.base_iri);
    }
    
    /* Allocate prefix array */
    parser->prefix_capacity = 16;
    parser->prefixes = calloc(parser->prefix_capacity, sizeof(parser->prefixes[0]));
    
    /* Get first token */
    advance(parser);
    
    return parser;
}

/**
 * Destroy parser instance
 */
void ttl_parser_destroy(Parser* parser) {
    if (!parser) return;
    
    /* Free lexer if we own it */
    if (parser->lexer) {
        lexer_destroy(parser->lexer);
    }
    
    /* Free current tokens */
    if (parser->current_token) {
        token_free(parser->current_token);
    }
    if (parser->previous_token) {
        token_free(parser->previous_token);
    }
    
    /* Free AST context */
    if (parser->ast_context) {
        ttl_ast_context_destroy(parser->ast_context);
    }
    
    /* Free errors */
    ParseError* error = parser->errors;
    while (error) {
        ParseError* next = error->next;
        free(error->message);
        free(error->context);
        free(error->suggestion);
        free(error);
        error = next;
    }
    
    /* Free prefixes */
    for (size_t i = 0; i < parser->prefix_count; i++) {
        free(parser->prefixes[i].prefix);
        free(parser->prefixes[i].iri);
    }
    free(parser->prefixes);
    
    /* Free base IRI */
    free(parser->base_iri);
    
    /* Free string pool */
    destroyStringPool(parser);
    
    free(parser);
}

/**
 * Parse document and return AST
 */
ttl_ast_node_t* ttl_parser_parse(Parser* parser) {
    if (!parser) return NULL;
    
    /* Parse the document */
    ttl_ast_node_t* ast = parseTurtleDoc(parser);
    
    /* Calculate parse time */
    clock_t end_time = clock();
    parser->stats.parse_time_ms = ((double)(end_time - parser->start_time) / CLOCKS_PER_SEC) * 1000.0;
    
    return ast;
}

/**
 * Entry point - Rule [1]: turtleDoc ::= statement*
 */
static ttl_ast_node_t* parseTurtleDoc(Parser* parser) {
    ttl_ast_node_t* document = ttl_ast_create_document(parser->ast_context);
    if (!document) {
        reportError(parser, "Failed to create document node");
        return NULL;
    }
    
    /* Parse statements until EOF */
    while (!isAtEnd(parser)) {
        /* Skip whitespace and comments if not tracking */
        if (match(parser, TOKEN_WHITESPACE) || match(parser, TOKEN_COMMENT)) {
            continue;
        }
        
        ttl_ast_node_t* statement = parseStatement(parser);
        if (statement) {
            ttl_ast_add_statement(document, statement);
            parser->stats.statements_parsed++;
        } else if (parser->fatal_error) {
            /* Fatal error, stop parsing */
            break;
        } else if (parser->options.error_recovery) {
            /* Try to recover and continue */
            if (!recoverFromError(parser)) {
                /* CORE FIX: If recovery fails, force advance to prevent infinite loop */
                if (!isAtEnd(parser)) {
                    advance(parser);
                }
            }
        } else {
            /* No error recovery, stop */
            break;
        }
    }
    
    return document;
}

/**
 * Rule [2]: statement ::= directive | triples '.'
 */
static ttl_ast_node_t* parseStatement(Parser* parser) {
    /* Check for directive */
    if (isDirectiveStart(parser->current_token)) {
        return parseDirective(parser);
    }
    
    /* Parse triples */
    ttl_ast_node_t* triples = parseTriples(parser);
    if (!triples) {
        /* Error in triple parsing */
        if (parser->options.error_recovery) {
            /* CORE FIX: Always advance before synchronizing */
            if (!isAtEnd(parser)) advance(parser);
            TokenType sync[] = {TOKEN_DOT, TOKEN_PREFIX, TOKEN_BASE};
            synchronizeTo(parser, sync, 3);
        }
        return NULL;
    }
    
    /* Expect '.' after triples */
    if (!consume(parser, TOKEN_DOT)) {
        reportError(parser, "Expected '.' after triples");
        if (parser->options.error_recovery) {
            /* CORE FIX: Always advance before synchronizing */
            if (!isAtEnd(parser)) advance(parser);
            TokenType sync[] = {TOKEN_DOT, TOKEN_PREFIX, TOKEN_BASE};
            synchronizeTo(parser, sync, 3);
        }
        return NULL;
    }
    
    parser->stats.triples_parsed++;
    return triples;
}

/**
 * Rule [3]: directive ::= prefixID | base | sparqlPrefix | sparqlBase
 */
static ttl_ast_node_t* parseDirective(Parser* parser) {
    if (match(parser, TOKEN_PREFIX)) {
        return parsePrefixID(parser);
    } else if (match(parser, TOKEN_BASE)) {
        return parseBase(parser);
    }
    /* TODO: Implement SPARQL-style directives */
    
    reportError(parser, "Invalid directive");
    return NULL;
}

/**
 * Rule [4]: prefixID ::= '@prefix' PNAME_NS IRIREF '.'
 */
static ttl_ast_node_t* parsePrefixID(Parser* parser) {
    /* '@prefix' already consumed */
    
    /* CORE FIX: Parse prefix name - can be PREFIXED_NAME (ex:) or just identifier */
    char* prefix = NULL;
    
    if (check(parser, TOKEN_PREFIXED_NAME)) {
        /* Standard prefix like "ex:" */
        char* full_prefix = copyString(parser, parser->current_token->value);
        advance(parser);
        
        /* Extract prefix part (remove colon) */
        char* colon = strchr(full_prefix, ':');
        if (colon) {
            *colon = '\0';  /* Remove colon */
            prefix = copyString(parser, full_prefix);
        } else {
            prefix = full_prefix;
        }
        free(full_prefix);
    } else {
        /* CORE FIX: Handle malformed prefix - skip until IRI */
        reportError(parser, "Expected prefix name after @prefix");
        return NULL;
    }
    
    /* Parse IRI */
    ttl_ast_node_t* iri = parseIRI(parser);
    if (!iri) {
        free(prefix);
        return NULL;
    }
    
    /* Expect '.' */
    if (!consume(parser, TOKEN_DOT)) {
        reportError(parser, "Expected '.' after prefix declaration");
        free(prefix);
        return NULL;
    }
    
    /* CORE FIX: Store prefix mapping with proper IRI extraction */
    const char* iri_value = NULL;
    if (iri && iri->type == TTL_AST_IRI) {
        iri_value = iri->data.iri.value;
    }
    
    if (iri_value) {
        addPrefix(parser, prefix, iri_value);
    }
    
    /* Create AST node */
    ttl_ast_node_t* prefix_node = ttl_ast_create_prefixed_name(parser->ast_context, prefix, "");
    ttl_ast_node_t* directive = ttl_ast_create_prefix_directive(parser->ast_context, prefix_node, iri);
    
    free(prefix);
    return directive;
}

/**
 * Rule [5]: base ::= '@base' IRIREF '.'
 */
static ttl_ast_node_t* parseBase(Parser* parser) {
    /* '@base' already consumed */
    
    /* Parse IRI */
    ttl_ast_node_t* iri = parseIRI(parser);
    if (!iri) {
        return NULL;
    }
    
    /* Expect '.' */
    if (!consume(parser, TOKEN_DOT)) {
        reportError(parser, "Expected '.' after base declaration");
        return NULL;
    }
    
    /* Update base IRI */
    free(parser->base_iri);
    parser->base_iri = copyString(parser, ((ttl_ast_node_t*)iri)->data.iri.value);
    
    /* Create AST node */
    return ttl_ast_create_base_directive(parser->ast_context, iri);
}

/**
 * Rule [6]: triples ::= subject predicateObjectList | blankNodePropertyList predicateObjectList?
 */
static ttl_ast_node_t* parseTriples(Parser* parser) {
    ttl_ast_node_t* subject = NULL;
    ttl_ast_node_t* predicate_object_list = NULL;
    
    /* Check for blank node property list */
    if (check(parser, TOKEN_OPEN_BRACKET)) {
        subject = parseBlankNodePropertyList(parser);
        if (!subject) return NULL;
        
        /* Optional predicateObjectList */
        if (!check(parser, TOKEN_DOT) && !isAtEnd(parser)) {
            predicate_object_list = parsePredicateObjectList(parser);
            if (!predicate_object_list && !parser->options.error_recovery) {
                return NULL;
            }
        }
    } else {
        /* Parse regular subject */
        subject = parseSubject(parser);
        if (!subject) return NULL;
        
        /* Parse predicateObjectList */
        predicate_object_list = parsePredicateObjectList(parser);
        if (!predicate_object_list) return NULL;
    }
    
    /* Create triple node */
    return ttl_ast_create_triple(parser->ast_context, subject, predicate_object_list);
}

/**
 * Rule [7]: predicateObjectList ::= verb objectList (';' (verb objectList)?)*
 */
static ttl_ast_node_t* parsePredicateObjectList(Parser* parser) {
    ttl_ast_node_t* list = ttl_ast_create_predicate_object_list(parser->ast_context);
    if (!list) return NULL;
    
    /* Parse first verb objectList */
    ttl_ast_node_t* verb = parseVerb(parser);
    if (!verb) return NULL;
    
    ttl_ast_node_t* object_list = parseObjectList(parser);
    if (!object_list) return NULL;
    
    ttl_ast_add_predicate_object(list, verb, object_list);
    
    /* Parse additional verb objectList pairs */
    while (match(parser, TOKEN_SEMICOLON)) {
        /* Skip trailing semicolons */
        if (check(parser, TOKEN_DOT) || check(parser, TOKEN_CLOSE_BRACKET) || 
            check(parser, TOKEN_CLOSE_PAREN) || isAtEnd(parser)) {
            break;
        }
        
        verb = parseVerb(parser);
        if (!verb) {
            if (parser->options.error_recovery) {
                continue;
            }
            return NULL;
        }
        
        object_list = parseObjectList(parser);
        if (!object_list) {
            if (parser->options.error_recovery) {
                continue;
            }
            return NULL;
        }
        
        ttl_ast_add_predicate_object(list, verb, object_list);
    }
    
    return list;
}

/**
 * Rule [8]: objectList ::= object (',' object)*
 */
static ttl_ast_node_t* parseObjectList(Parser* parser) {
    ttl_ast_node_t* list = ttl_ast_create_object_list(parser->ast_context);
    if (!list) return NULL;
    
    /* Parse first object */
    ttl_ast_node_t* object = parseObject(parser);
    if (!object) return NULL;
    
    ttl_ast_add_object(list, object);
    
    /* Parse additional objects */
    while (match(parser, TOKEN_COMMA)) {
        object = parseObject(parser);
        if (!object) {
            if (parser->options.error_recovery) {
                continue;
            }
            return NULL;
        }
        ttl_ast_add_object(list, object);
    }
    
    return list;
}

/**
 * Rule [9]: verb ::= predicate | 'a'
 */
static ttl_ast_node_t* parseVerb(Parser* parser) {
    if (match(parser, TOKEN_A)) {
        /* 'a' is shorthand for rdf:type */
        return ttl_ast_create_rdf_type(parser->ast_context);
    }
    
    return parsePredicate(parser);
}

/**
 * Rule [10]: subject ::= iri | BlankNode | collection
 */
static ttl_ast_node_t* parseSubject(Parser* parser) {
    if (check(parser, TOKEN_IRI_REF) || check(parser, TOKEN_PREFIXED_NAME)) {
        return parseIRI(parser);
    } else if (check(parser, TOKEN_BLANK_NODE_LABEL) || check(parser, TOKEN_ANON)) {
        return parseBlankNode(parser);
    } else if (check(parser, TOKEN_OPEN_PAREN)) {
        return parseCollection(parser);
    }
    
    reportError(parser, "Expected subject (IRI, blank node, or collection)");
    return NULL;
}

/**
 * Rule [11]: predicate ::= iri
 */
static ttl_ast_node_t* parsePredicate(Parser* parser) {
    return parseIRI(parser);
}

/**
 * Rule [12]: object ::= iri | BlankNode | collection | blankNodePropertyList | literal
 */
static ttl_ast_node_t* parseObject(Parser* parser) {
    if (check(parser, TOKEN_IRI_REF) || check(parser, TOKEN_PREFIXED_NAME)) {
        return parseIRI(parser);
    } else if (check(parser, TOKEN_BLANK_NODE_LABEL) || check(parser, TOKEN_ANON)) {
        return parseBlankNode(parser);
    } else if (check(parser, TOKEN_OPEN_PAREN)) {
        return parseCollection(parser);
    } else if (check(parser, TOKEN_OPEN_BRACKET)) {
        return parseBlankNodePropertyList(parser);
    } else if (check(parser, TOKEN_STRING_LITERAL_QUOTE) || 
               check(parser, TOKEN_STRING_LITERAL_SINGLE_QUOTE) ||
               check(parser, TOKEN_STRING_LITERAL_LONG_QUOTE) ||
               check(parser, TOKEN_STRING_LITERAL_LONG_SINGLE_QUOTE) ||
               check(parser, TOKEN_INTEGER) ||
               check(parser, TOKEN_DECIMAL) ||
               check(parser, TOKEN_DOUBLE) ||
               check(parser, TOKEN_BOOLEAN)) {
        return parseLiteral(parser);
    }
    
    reportError(parser, "Expected object");
    return NULL;
}

/**
 * Rule [13]: literal ::= RDFLiteral | NumericLiteral | BooleanLiteral
 */
static ttl_ast_node_t* parseLiteral(Parser* parser) {
    if (check(parser, TOKEN_STRING_LITERAL_QUOTE) || 
        check(parser, TOKEN_STRING_LITERAL_SINGLE_QUOTE) ||
        check(parser, TOKEN_STRING_LITERAL_LONG_QUOTE) ||
        check(parser, TOKEN_STRING_LITERAL_LONG_SINGLE_QUOTE)) {
        return parseRDFLiteral(parser);
    } else if (check(parser, TOKEN_INTEGER) || 
               check(parser, TOKEN_DECIMAL) || 
               check(parser, TOKEN_DOUBLE)) {
        return parseNumericLiteral(parser);
    } else if (check(parser, TOKEN_BOOLEAN)) {
        return parseBooleanLiteral(parser);
    }
    
    reportError(parser, "Expected literal");
    return NULL;
}

/**
 * Rule [16]: NumericLiteral ::= INTEGER | DECIMAL | DOUBLE
 */
static ttl_ast_node_t* parseNumericLiteral(Parser* parser) {
    Token* token = parser->current_token;
    
    if (match(parser, TOKEN_INTEGER)) {
        int64_t value = strtoll(token->value, NULL, 10);
        return ttl_ast_create_integer_literal(parser->ast_context, value, token->value);
    } else if (match(parser, TOKEN_DECIMAL)) {
        double value = strtod(token->value, NULL);
        return ttl_ast_create_decimal_literal(parser->ast_context, value, token->value);
    } else if (match(parser, TOKEN_DOUBLE)) {
        double value = strtod(token->value, NULL);
        return ttl_ast_create_double_literal(parser->ast_context, value, token->value);
    }
    
    reportError(parser, "Expected numeric literal");
    return NULL;
}

/**
 * Rule [128s]: RDFLiteral ::= String (LANGTAG | '^^' iri)?
 */
static ttl_ast_node_t* parseRDFLiteral(Parser* parser) {
    /* Parse string literal */
    ttl_string_quote_type_t quote_type;
    if (check(parser, TOKEN_STRING_LITERAL_QUOTE)) {
        quote_type = TTL_STRING_DOUBLE_QUOTE;
    } else if (check(parser, TOKEN_STRING_LITERAL_SINGLE_QUOTE)) {
        quote_type = TTL_STRING_SINGLE_QUOTE;
    } else if (check(parser, TOKEN_STRING_LITERAL_LONG_QUOTE)) {
        quote_type = TTL_STRING_TRIPLE_DOUBLE;
    } else if (check(parser, TOKEN_STRING_LITERAL_LONG_SINGLE_QUOTE)) {
        quote_type = TTL_STRING_TRIPLE_SINGLE;
    } else {
        reportError(parser, "Expected string literal");
        return NULL;
    }
    
    char* value = copyString(parser, parser->current_token->value);
    advance(parser);
    
    ttl_ast_node_t* string_node = ttl_ast_create_string_literal(parser->ast_context, value, quote_type);
    free(value);
    
    /* Check for language tag or datatype */
    if (match(parser, TOKEN_AT)) {
        /* Language tag */
        if (!check(parser, TOKEN_PREFIXED_NAME)) {
            reportError(parser, "Expected language tag after '@'");
            return NULL;
        }
        
        char* lang_tag = copyString(parser, parser->current_token->value);
        advance(parser);
        
        ttl_ast_node_t* lang_literal = ttl_ast_create_lang_literal(parser->ast_context, string_node, lang_tag);
        free(lang_tag);
        
        return lang_literal;
    } else if (match(parser, TOKEN_DOUBLE_CARET)) {
        /* Datatype IRI */
        ttl_ast_node_t* datatype = parseIRI(parser);
        if (!datatype) return NULL;
        
        return ttl_ast_create_typed_literal(parser->ast_context, string_node, datatype);
    }
    
    return string_node;
}

/**
 * Rule [133s]: BooleanLiteral ::= 'true' | 'false'
 */
static ttl_ast_node_t* parseBooleanLiteral(Parser* parser) {
    if (!check(parser, TOKEN_BOOLEAN)) {
        reportError(parser, "Expected boolean literal");
        return NULL;
    }
    
    bool value = strcmp(parser->current_token->value, "true") == 0;
    advance(parser);
    
    return ttl_ast_create_boolean_literal(parser->ast_context, value);
}

/**
 * Rule [14]: blankNodePropertyList ::= '[' predicateObjectList ']'
 */
static ttl_ast_node_t* parseBlankNodePropertyList(Parser* parser) {
    if (!consume(parser, TOKEN_OPEN_BRACKET)) {
        reportError(parser, "Expected '['");
        return NULL;
    }
    
    /* Empty property list */
    if (check(parser, TOKEN_CLOSE_BRACKET)) {
        advance(parser);
        return ttl_ast_create_blank_node_property_list(parser->ast_context, NULL);
    }
    
    /* Parse predicate object list */
    ttl_ast_node_t* predicate_object_list = parsePredicateObjectList(parser);
    if (!predicate_object_list) {
        if (parser->options.error_recovery) {
            /* Skip to closing bracket */
            TokenType sync[] = {TOKEN_CLOSE_BRACKET};
            synchronizeTo(parser, sync, 1);
        }
        return NULL;
    }
    
    if (!consume(parser, TOKEN_CLOSE_BRACKET)) {
        reportError(parser, "Expected ']' after property list");
        return NULL;
    }
    
    return ttl_ast_create_blank_node_property_list(parser->ast_context, predicate_object_list);
}

/**
 * Rule [15]: collection ::= '(' object* ')'
 */
static ttl_ast_node_t* parseCollection(Parser* parser) {
    if (!consume(parser, TOKEN_OPEN_PAREN)) {
        reportError(parser, "Expected '('");
        return NULL;
    }
    
    ttl_ast_node_t* collection = ttl_ast_create_collection(parser->ast_context);
    if (!collection) return NULL;
    
    /* Parse objects until ')' */
    while (!check(parser, TOKEN_CLOSE_PAREN) && !isAtEnd(parser)) {
        ttl_ast_node_t* object = parseObject(parser);
        if (!object) {
            if (parser->options.error_recovery) {
                /* Skip invalid object and continue */
                advance(parser);
                continue;
            }
            return NULL;
        }
        ttl_ast_add_collection_item(collection, object);
    }
    
    if (!consume(parser, TOKEN_CLOSE_PAREN)) {
        reportError(parser, "Expected ')' after collection");
        return NULL;
    }
    
    return collection;
}

/**
 * Rule [137s]: BlankNode ::= BLANK_NODE_LABEL | ANON
 */
static ttl_ast_node_t* parseBlankNode(Parser* parser) {
    if (match(parser, TOKEN_BLANK_NODE_LABEL)) {
        /* Extract label (skip '_:' prefix) */
        const char* label = previous(parser)->value + 2;
        return ttl_ast_create_blank_node(parser->ast_context, label);
    } else if (match(parser, TOKEN_ANON)) {
        /* Anonymous blank node */
        return ttl_ast_create_anonymous_blank_node(parser->ast_context);
    }
    
    reportError(parser, "Expected blank node");
    return NULL;
}

/**
 * Rule [135s]: iri ::= IRIREF | PrefixedName
 */
static ttl_ast_node_t* parseIRI(Parser* parser) {
    if (match(parser, TOKEN_IRI_REF)) {
        /* Extract IRI (remove angle brackets) */
        char* value = previous(parser)->value;
        size_t len = strlen(value);
        if (len >= 2 && value[0] == '<' && value[len-1] == '>') {
            value[len-1] = '\0';
            value++;
        }
        
        /* Resolve relative IRIs */
        char* resolved = resolveIRI(parser, value);
        ttl_ast_node_t* iri = ttl_ast_create_iri(parser->ast_context, resolved);
        free(resolved);
        
        return iri;
    } else if (check(parser, TOKEN_PREFIXED_NAME)) {
        return parsePrefixedName(parser);
    }
    
    reportError(parser, "Expected IRI or prefixed name");
    return NULL;
}

/**
 * Rule [136s]: PrefixedName ::= PNAME_LN | PNAME_NS
 */
static ttl_ast_node_t* parsePrefixedName(Parser* parser) {
    if (!match(parser, TOKEN_PREFIXED_NAME)) {
        reportError(parser, "Expected prefixed name");
        return NULL;
    }
    
    char* prefixed_name = previous(parser)->value;
    
    /* Split into prefix and local name */
    char* colon = strchr(prefixed_name, ':');
    if (!colon) {
        reportError(parser, "Invalid prefixed name format");
        return NULL;
    }
    
    size_t prefix_len = colon - prefixed_name;
    char* prefix = (char*)malloc(prefix_len + 1);
    strncpy(prefix, prefixed_name, prefix_len);
    prefix[prefix_len] = '\0';
    
    char* local_name = colon + 1;
    
    /* Validate prefix exists */
    const char* prefix_iri = lookupPrefix(parser, prefix);
    if (!prefix_iri && prefix_len > 0) {
        reportError(parser, "Undefined prefix");
        free(prefix);
        return NULL;
    }
    
    ttl_ast_node_t* node = ttl_ast_create_prefixed_name(parser->ast_context, prefix, local_name);
    free(prefix);
    
    return node;
}

/* Helper function implementations */

static void advance(Parser* parser) {
    if (parser->previous_token) {
        token_free(parser->previous_token);
    }
    parser->previous_token = parser->current_token;
    parser->current_token = lexer_next_token(parser->lexer);
    parser->stats.tokens_consumed++;
}

static bool consume(Parser* parser, TokenType expected) {
    if (check(parser, expected)) {
        advance(parser);
        return true;
    }
    return false;
}

static bool match(Parser* parser, TokenType type) {
    if (check(parser, type)) {
        advance(parser);
        return true;
    }
    return false;
}

static bool check(Parser* parser, TokenType type) {
    if (isAtEnd(parser)) return false;
    return parser->current_token->type == type;
}

static bool isAtEnd(Parser* parser) {
    return parser->current_token->type == TOKEN_EOF;
}

__attribute__((unused))
static Token* peek(Parser* parser) {
    return parser->current_token;
}

static Token* previous(Parser* parser) {
    return parser->previous_token;
}

/* Error handling implementations */

static void reportError(Parser* parser, const char* message) {
    reportErrorAt(parser, parser->current_token, message);
}

static void reportErrorAt(Parser* parser, Token* token, const char* message) {
    if (parser->error_count >= parser->options.max_errors) {
        parser->fatal_error = true;
        return;
    }
    
    ParseError* error = createError(message, token, ERROR_SEVERITY_ERROR);
    addError(parser, error);
}

static ParseError* createError(const char* message, Token* token, ErrorSeverity severity) {
    ParseError* error = (ParseError*)calloc(1, sizeof(ParseError));
    if (!error) return NULL;
    
    error->message = strdup(message);
    error->line = token->line;
    error->column = token->column;
    error->severity = severity;
    
    /* TODO: Add context and suggestions */
    
    return error;
}

static void addError(Parser* parser, ParseError* error) {
    if (!error) return;
    
    if (parser->last_error) {
        parser->last_error->next = error;
    } else {
        parser->errors = error;
    }
    parser->last_error = error;
    parser->error_count++;
}

static void synchronize(Parser* parser) {
    /* Default synchronization - skip to next statement */
    TokenType sync[] = {TOKEN_DOT, TOKEN_PREFIX, TOKEN_BASE, TOKEN_EOF};
    synchronizeTo(parser, sync, 4);
}

static void synchronizeTo(Parser* parser, const TokenType* sync_tokens, int count) {
    /* CORE FIX: Ensure we always advance at least one token to prevent infinite loops */
    if (!isAtEnd(parser)) {
        advance(parser);  /* Always advance at least once */
    }
    
    while (!isAtEnd(parser)) {
        /* Check if we've reached a synchronization point */
        for (int i = 0; i < count; i++) {
            if (check(parser, sync_tokens[i])) {
                return;
            }
        }
        advance(parser);
    }
}

static bool recoverFromError(Parser* parser) {
    /* CORE FIX: Always advance position to prevent infinite loops */
    if (!isAtEnd(parser)) {
        advance(parser);  /* Ensure we make progress */
    }
    synchronize(parser);
    parser->stats.errors_recovered++;
    return !isAtEnd(parser);
}

/* Utility function implementations */

static char* copyString(Parser* parser, const char* str) {
    (void)parser; /* TODO: Use parser's string pool */
    if (!str) return NULL;
    return strdup(str);  /* TODO: Use string pool */
}

static void addPrefix(Parser* parser, const char* prefix, const char* iri) {
    /* Check if prefix already exists */
    for (size_t i = 0; i < parser->prefix_count; i++) {
        if (strcmp(parser->prefixes[i].prefix, prefix) == 0) {
            /* Update existing prefix */
            free(parser->prefixes[i].iri);
            parser->prefixes[i].iri = copyString(parser, iri);
            return;
        }
    }
    
    /* Expand array if needed */
    if (parser->prefix_count >= parser->prefix_capacity) {
        parser->prefix_capacity *= 2;
        parser->prefixes = realloc(parser->prefixes, 
                                  parser->prefix_capacity * sizeof(parser->prefixes[0]));
    }
    
    /* Add new prefix */
    parser->prefixes[parser->prefix_count].prefix = copyString(parser, prefix);
    parser->prefixes[parser->prefix_count].iri = copyString(parser, iri);
    parser->prefix_count++;
}

static const char* lookupPrefix(Parser* parser, const char* prefix) {
    for (size_t i = 0; i < parser->prefix_count; i++) {
        if (strcmp(parser->prefixes[i].prefix, prefix) == 0) {
            return parser->prefixes[i].iri;
        }
    }
    return NULL;
}

static char* resolveIRI(Parser* parser, const char* iri) {
    /* TODO: Implement proper IRI resolution with base IRI */
    return copyString(parser, iri);
}

static bool isDirectiveStart(Token* token) {
    return token->type == TOKEN_PREFIX || 
           token->type == TOKEN_BASE;
    /* TODO: Add SPARQL-style directives */
}

/* Memory management implementations */

__attribute__((unused))
static void* parserAlloc(Parser* parser, size_t size) {
    (void)parser; /* TODO: Use parser's arena allocator */
    /* TODO: Implement arena allocation */
    return malloc(size);
}

static void initStringPool(Parser* parser) {
    parser->string_pool.size = 64 * 1024;  /* 64KB initial size */
    parser->string_pool.memory = malloc(parser->string_pool.size);
    parser->string_pool.used = 0;
}

static void destroyStringPool(Parser* parser) {
    free(parser->string_pool.memory);
}

/* Public API implementations */

bool ttl_parser_has_errors(const Parser* parser) {
    return parser && parser->error_count > 0;
}

size_t ttl_parser_error_count(const Parser* parser) {
    return parser ? parser->error_count : 0;
}

const ParseError* ttl_parser_get_error(const Parser* parser, size_t index) {
    if (!parser || index >= parser->error_count) return NULL;
    
    ParseError* error = parser->errors;
    for (size_t i = 0; i < index && error; i++) {
        error = error->next;
    }
    
    return error;
}

const ParseError* ttl_parser_get_errors(const Parser* parser) {
    return parser ? parser->errors : NULL;
}

void ttl_parser_set_base_iri(Parser* parser, const char* base_iri) {
    if (!parser) return;
    
    free(parser->base_iri);
    parser->base_iri = base_iri ? strdup(base_iri) : NULL;
}

void ttl_parser_set_strict_mode(Parser* parser, bool strict) {
    if (parser) {
        parser->options.strict_mode = strict;
    }
}

void ttl_parser_get_stats(const Parser* parser, ParserStats* stats) {
    if (parser && stats) {
        *stats = parser->stats;
    }
}

ParserOptions ttl_parser_default_options(void) {
    ParserOptions options = {
        .strict_mode = false,
        .error_recovery = true,
        .track_comments = false,
        .validate_iris = true,
        .normalize_literals = true,
        .max_errors = 100,
        .base_iri = NULL
    };
    return options;
}

/* Streaming parser implementation */

struct StreamingParser {
    Parser* parser;
    StreamingCallbacks callbacks;
    bool in_triple;
    char* current_subject;
    char* current_predicate;
};

StreamingParser* ttl_streaming_parser_create(const StreamingCallbacks* callbacks, 
                                             const ParserOptions* options) {
    if (!callbacks) return NULL;
    
    StreamingParser* sp = (StreamingParser*)calloc(1, sizeof(StreamingParser));
    if (!sp) return NULL;
    
    sp->callbacks = *callbacks;
    
    /* Create parser with empty input initially */
    sp->parser = ttl_parser_create("", 0, options);
    if (!sp->parser) {
        free(sp);
        return NULL;
    }
    
    return sp;
}

bool ttl_streaming_parser_feed(StreamingParser* parser, 
                              const char* data, 
                              size_t length) {
    if (!parser || !data) return false;
    
    /* Feed data to lexer */
    return lexer_feed(parser->parser->lexer, data, length);
}

void ttl_streaming_parser_end(StreamingParser* parser) {
    if (!parser) return;
    
    lexer_end_input(parser->parser->lexer);
    
    /* Parse any remaining data */
    ttl_parser_parse(parser->parser);
}

void ttl_streaming_parser_destroy(StreamingParser* parser) {
    if (!parser) return;
    
    ttl_parser_destroy(parser->parser);
    free(parser->current_subject);
    free(parser->current_predicate);
    free(parser);
}

/* Utility functions */

bool ttl_validate_syntax(const char* input, size_t length,
                        ParseError** errors, size_t* error_count) {
    Parser* parser = ttl_parser_create(input, length, NULL);
    if (!parser) return false;
    
    ttl_ast_node_t* ast = ttl_parser_parse(parser);
    bool valid = ast != NULL && !ttl_parser_has_errors(parser);
    
    if (errors && error_count) {
        *error_count = ttl_parser_error_count(parser);
        if (*error_count > 0) {
            /* Copy errors */
            *errors = (ParseError*)calloc(*error_count, sizeof(ParseError));
            const ParseError* src = ttl_parser_get_errors(parser);
            for (size_t i = 0; i < *error_count && src; i++) {
                (*errors)[i] = *src;
                (*errors)[i].message = strdup(src->message);
                (*errors)[i].next = (i + 1 < *error_count) ? &(*errors)[i + 1] : NULL;
                src = src->next;
            }
        }
    }
    
    ttl_parser_destroy(parser);
    return valid;
}

ttl_ast_node_t* ttl_parse_statement(const char* input, size_t length,
                                    const char* base_iri) {
    ParserOptions options = ttl_parser_default_options();
    options.base_iri = base_iri;
    
    Parser* parser = ttl_parser_create(input, length, &options);
    if (!parser) return NULL;
    
    ttl_ast_node_t* statement = parseStatement(parser);
    
    ttl_parser_destroy(parser);
    return statement;
}