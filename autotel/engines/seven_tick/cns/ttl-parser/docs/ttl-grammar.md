# TTL/Turtle Grammar Specification

## Overview

This document provides the complete grammar specification for TTL (Turtle - Terse RDF Triple Language) based on the W3C RDF 1.1 Turtle specification. The grammar is expressed in EBNF notation and is suitable for implementing a TTL parser.

## Grammar Properties

- **Type**: LL(1) and LALR(1) parseable
- **Entry Point**: `turtleDoc`
- **Tokenization**: Longest match is chosen
- **Case Sensitivity**: 
  - Single-quoted keywords ('@base', '@prefix', 'a', 'true', 'false') are case-sensitive
  - Double-quoted keywords ("BASE", "PREFIX") are case-insensitive
  - Escape sequences (UCHAR, ECHAR) are case-sensitive

## Complete Grammar Rules

### Document Structure

```ebnf
[1]  turtleDoc          ::= statement*
[2]  statement          ::= directive | triples '.'
[3]  directive          ::= prefixID | base | sparqlPrefix | sparqlBase
[4]  prefixID           ::= '@prefix' PNAME_NS IRIREF '.'
[5]  base               ::= '@base' IRIREF '.'
[5s] sparqlBase         ::= "BASE" IRIREF
[6s] sparqlPrefix       ::= "PREFIX" PNAME_NS IRIREF
```

### Triple Patterns

```ebnf
[6]  triples            ::= subject predicateObjectList | blankNodePropertyList predicateObjectList?
[7]  predicateObjectList ::= verb objectList (';' (verb objectList)?)*
[8]  objectList         ::= object (',' object)*
[9]  verb               ::= predicate | 'a'
[10] subject            ::= iri | BlankNode | collection
[11] predicate          ::= iri
[12] object             ::= iri | BlankNode | collection | blankNodePropertyList | literal
```

### Literals

```ebnf
[13]  literal           ::= RDFLiteral | NumericLiteral | BooleanLiteral
[16]  NumericLiteral    ::= INTEGER | DECIMAL | DOUBLE
[128s] RDFLiteral      ::= String (LANGTAG | '^^' iri)?
[133s] BooleanLiteral  ::= 'true' | 'false'
```

### Collections and Blank Nodes

```ebnf
[14] blankNodePropertyList ::= '[' predicateObjectList ']'
[15] collection            ::= '(' object* ')'
[137s] BlankNode          ::= BLANK_NODE_LABEL | ANON
[141s] BLANK_NODE_LABEL   ::= '_:' (PN_CHARS_U | [0-9]) ((PN_CHARS | '.')* PN_CHARS)?
[162s] ANON               ::= '[' WS* ']'
```

### IRIs and Prefixed Names

```ebnf
[135s] iri                ::= IRIREF | PrefixedName
[136s] PrefixedName       ::= PNAME_LN | PNAME_NS
[139s] PNAME_NS           ::= PN_PREFIX? ':'
[140s] PNAME_LN           ::= PNAME_NS PN_LOCAL
[18]   IRIREF             ::= '<' ([^#x00-#x20<>"{}|^`\] | UCHAR)* '>'
```

### String Literals

```ebnf
[17] String                        ::= STRING_LITERAL_QUOTE | STRING_LITERAL_SINGLE_QUOTE | 
                                       STRING_LITERAL_LONG_SINGLE_QUOTE | STRING_LITERAL_LONG_QUOTE
[22] STRING_LITERAL_QUOTE          ::= '"' ([^#x22#x5C#xA#xD] | ECHAR | UCHAR)* '"'
[23] STRING_LITERAL_SINGLE_QUOTE   ::= "'" ([^#x27#x5C#xA#xD] | ECHAR | UCHAR)* "'"
[24] STRING_LITERAL_LONG_SINGLE_QUOTE ::= "'''" (("'" | "''")? ([^'\\] | ECHAR | UCHAR))* "'''"
[25] STRING_LITERAL_LONG_QUOTE     ::= '"""' (('"' | '""')? ([^"\\] | ECHAR | UCHAR))* '"""'
```

### Numeric Literals

```ebnf
[19] INTEGER ::= [+-]? [0-9]+
[20] DECIMAL ::= [+-]? [0-9]* '.' [0-9]+
[21] DOUBLE  ::= [+-]? ([0-9]+ '.' [0-9]* EXPONENT | '.' [0-9]+ EXPONENT | [0-9]+ EXPONENT)
[154s] EXPONENT ::= [eE] [+-]? [0-9]+
```

### Language Tags

```ebnf
[144s] LANGTAG ::= '@' [a-zA-Z]+ ('-' [a-zA-Z0-9]+)*
```

### Character Classes and Escape Sequences

```ebnf
[26]   UCHAR          ::= '\u' HEX HEX HEX HEX | '\U' HEX HEX HEX HEX HEX HEX HEX HEX
[159s] ECHAR          ::= '\' [tbnrf"'\\]
[171s] HEX            ::= [0-9] | [A-F] | [a-f]
```

### Prefixed Name Character Classes

```ebnf
[163s] PN_CHARS_BASE  ::= [A-Z] | [a-z] | [#x00C0-#x00D6] | [#x00D8-#x00F6] | [#x00F8-#x02FF] | 
                          [#x0370-#x037D] | [#x037F-#x1FFF] | [#x200C-#x200D] | [#x2070-#x218F] | 
                          [#x2C00-#x2FEF] | [#x3001-#xD7FF] | [#xF900-#xFDCF] | [#xFDF0-#xFFFD] | 
                          [#x10000-#xEFFFF]
[164s] PN_CHARS_U     ::= PN_CHARS_BASE | '_'
[166s] PN_CHARS       ::= PN_CHARS_U | '-' | [0-9] | #x00B7 | [#x0300-#x036F] | [#x203F-#x2040]
[167s] PN_PREFIX      ::= PN_CHARS_BASE ((PN_CHARS | '.')* PN_CHARS)?
[168s] PN_LOCAL       ::= (PN_CHARS_U | ':' | [0-9] | PLX) ((PN_CHARS | '.' | ':' | PLX)* (PN_CHARS | ':' | PLX))?
[169s] PLX            ::= PERCENT | PN_LOCAL_ESC
[170s] PERCENT        ::= '%' HEX HEX
[172s] PN_LOCAL_ESC   ::= '\' ('_' | '~' | '.' | '-' | '!' | '$' | '&' | "'" | '(' | ')' | 
                           '*' | '+' | ',' | ';' | '=' | '/' | '?' | '#' | '@' | '%')
```

### Whitespace and Comments

```ebnf
[161s] WS ::= #x20 | #x9 | #xD | #xA
# Comments start with '#' and continue to end of line
```

## Grammar Examples

### 1. Simple Triple
```turtle
@prefix ex: <http://example.org/> .
ex:subject ex:predicate ex:object .
```

Grammar trace:
- turtleDoc → statement (directive) → prefixID → '@prefix' PNAME_NS IRIREF '.'
- turtleDoc → statement (triples) → subject predicateObjectList '.'

### 2. Triple with Literal Object
```turtle
ex:person ex:name "Alice" .
ex:person ex:age 30 .
ex:person ex:active true .
```

Grammar trace:
- String literal: STRING_LITERAL_QUOTE
- Numeric literal: INTEGER
- Boolean literal: BooleanLiteral

### 3. Blank Node with Properties
```turtle
ex:person ex:knows [
    ex:name "Bob" ;
    ex:email "bob@example.com"
] .
```

Grammar trace:
- blankNodePropertyList → '[' predicateObjectList ']'

### 4. Collection (RDF List)
```turtle
ex:numbers ex:hasValues (1 2 3) .
```

Grammar trace:
- collection → '(' object* ')'

### 5. Language-Tagged String
```turtle
ex:greeting ex:text "Hello"@en .
```

Grammar trace:
- RDFLiteral → String LANGTAG

### 6. Typed Literal
```turtle
ex:date ex:value "2024-01-01"^^<http://www.w3.org/2001/XMLSchema#date> .
```

Grammar trace:
- RDFLiteral → String '^^' iri

### 7. Multi-line String
```turtle
ex:description ex:text """This is a
multi-line
string""" .
```

Grammar trace:
- STRING_LITERAL_LONG_QUOTE

### 8. Anonymous Blank Node
```turtle
ex:person ex:knows [] .
```

Grammar trace:
- ANON → '[' WS* ']'

## Special Cases and Edge Cases

### 1. Escape Sequences in Strings
- Use `\t`, `\n`, `\r`, `\b`, `\f` for control characters
- Use `\"` for quotes in double-quoted strings
- Use `\'` for quotes in single-quoted strings
- Use `\\` for literal backslash
- Use `\uXXXX` for 4-digit Unicode characters
- Use `\UXXXXXXXX` for 8-digit Unicode characters

### 2. Numeric Edge Cases
- Leading zeros are allowed: `007`
- Decimal point without leading digit: `.5`
- Exponent notation: `1.5e10`, `2E-5`
- Signed numbers: `+42`, `-3.14`

### 3. IRI Special Characters
- IRIs must be enclosed in angle brackets: `<http://example.org/>`
- Certain characters must be escaped using UCHAR sequences
- Space and control characters are not allowed

### 4. Prefix Name Rules
- Local names can contain dots, but cannot end with a dot
- Percent encoding allowed in local names: `ex:name%20with%20spaces`
- Special characters can be escaped with backslash

### 5. Comment Handling
- Comments start with `#` and continue to end of line
- Comments cannot appear inside IRIREFs or strings
- Comments are treated as whitespace

### 6. Whitespace Rules
- Whitespace includes space, tab, carriage return, and line feed
- No whitespace allowed between sign and number in numeric literals
- Arbitrary whitespace allowed inside ANON production: `[    ]`

## Parser Implementation Notes

1. **Tokenization**: Implement a tokenizer that recognizes the longest match
2. **Lookahead**: The grammar requires only 1 token lookahead (LL(1))
3. **Unicode Support**: Full Unicode support required for PN_CHARS_BASE
4. **Error Recovery**: Consider implementing error recovery for common mistakes
5. **Performance**: Cache parsed prefixes for efficient IRI expansion
6. **Memory**: Use efficient data structures for triple storage

## Grammar Validation

The grammar has been validated against the W3C test suite and conforms to:
- RDF 1.1 Turtle (W3C Recommendation 25 February 2014)
- SPARQL 1.1 Query Language (for shared productions)

## References

1. [W3C RDF 1.1 Turtle](https://www.w3.org/TR/turtle/)
2. [RDF 1.2 Turtle (Latest Draft)](https://w3c.github.io/rdf-turtle/spec/)
3. [SPARQL 1.1 Query Language](https://www.w3.org/TR/sparql11-query/)
4. [EBNF Notation](https://www.w3.org/TR/REC-xml/#sec-notation)