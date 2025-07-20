# TTL/Turtle Edge Cases and Complex Examples

This document provides comprehensive edge cases and complex examples for TTL parser implementation, focusing on challenging scenarios that parsers must handle correctly.

## 1. Blank Node Cycles

### Simple Cycle
```turtle
# Two blank nodes referencing each other
_:node1 :knows _:node2 .
_:node2 :knows _:node1 .
```

### Complex Cycle with Properties
```turtle
# Cannot use [] syntax for cycles - must use labeled blank nodes
_:alice a :Person ;
    :name "Alice" ;
    :friend _:bob ;
    :worksWith _:bob .

_:bob a :Person ;
    :name "Bob" ;
    :friend _:alice ;
    :manages _:alice .
```

### Multi-Node Cycle
```turtle
_:a :next _:b .
_:b :next _:c .
_:c :next _:a .
```

## 2. Nested Collections

### Simple Nested Collection
```turtle
:subject :hasLists ((1 2) (3 4) (5 6)) .
```

### Deeply Nested Collections
```turtle
:data :contains (
    (
        ("a" "b")
        ("c" "d")
    )
    (
        ("e" "f")
        ("g" "h")
    )
) .
```

### Collections with Blank Nodes
```turtle
:list :items (
    [ :name "Item 1" ]
    [ :name "Item 2" ]
    [ :name "Item 3" ]
) .
```

### Empty Collections
```turtle
# Empty collection is rdf:nil
:subject :emptyList () .

# Nested empty collections
:subject :nestedEmpty (() (())) .
```

## 3. Multiple References to Same Blank Node

### Shared Blank Node
```turtle
# _:shared is referenced multiple times
_:shared a :SharedResource ;
    :value 42 .

:user1 :uses _:shared .
:user2 :uses _:shared .
:user3 :references _:shared .
```

### Cannot Use Anonymous Syntax
```turtle
# WRONG - this creates different blank nodes
# :user1 :uses [ :value 42 ] .
# :user2 :uses [ :value 42 ] .

# CORRECT - use labeled blank node
_:resource :value 42 .
:user1 :uses _:resource .
:user2 :uses _:resource .
```

## 4. Complex String Literals

### Escaped Characters in Different String Types
```turtle
# Single quoted string with escapes
:s1 :p 'Line 1\nLine 2\tTabbed' .

# Double quoted string with escapes
:s2 :p "Quote: \" Backslash: \\ Tab: \t" .

# Long string with quotes
:s3 :p """Triple "quotes" inside""" .
:s4 :p '''Triple 'quotes' inside''' .
```

### Unicode Escapes
```turtle
# 4-digit Unicode
:s1 :text "Caf\u00E9" .  # Café

# 8-digit Unicode
:s2 :emoji "\U0001F600" .  # 😀

# Mixed escapes
:s3 :mixed "Tab:\t Quote:\" Unicode:\u00A9 Emoji:\U0001F44D" .
```

### Edge Cases in Long Strings
```turtle
# Almost-terminating quotes
:s1 :text """Text with "" two quotes""" .
:s2 :text '''Text with '' two quotes''' .

# Escaped quotes at end
:s3 :text """Text ending with quote\"""" .
:s4 :text '''Text ending with quote\''''''' .
```

## 5. Numeric Edge Cases

### Various Numeric Formats
```turtle
# Integers with signs
:n1 :value +42 .
:n2 :value -42 .
:n3 :value 007 .  # Leading zeros allowed

# Decimals
:d1 :value .5 .    # No leading digit
:d2 :value 5. .    # No trailing digits
:d3 :value +.5 .   # Positive decimal without leading digit
:d4 :value -.5 .   # Negative decimal without leading digit

# Doubles with exponents
:e1 :value 1e10 .
:e2 :value 1E10 .
:e3 :value 1.5e-10 .
:e4 :value .5e10 .
:e5 :value 5.e10 .
```

## 6. IRI and Prefixed Name Edge Cases

### Special Characters in IRIs
```turtle
# Unicode in IRIs (must be percent-encoded)
<http://example.org/caf%C3%A9> a :Thing .

# Escaped characters in IRIs
<http://example.org/\u0020space> a :Thing .
```

### Prefixed Name Edge Cases
```turtle
@prefix ex: <http://example.org/> .
@prefix : <http://default.org/> .

# Percent-encoding in local names
ex:name%20with%20spaces a :Thing .

# Escaped special characters
ex:name\-with\-dashes a :Thing .
ex:name\.with\.dots a :Thing .

# Colon in local name
ex:part:one:two a :Thing .

# Local name starting with number (requires PLX)
ex:123name a :Thing .

# Empty prefix
:localName a :Thing .
```

### Dots in Local Names
```turtle
# Dots allowed in middle but not at end
ex:valid.name.here a :Thing .
# ex:invalid.name. a :Thing .  # ERROR - ends with dot
```

## 7. Predicate Lists and Object Lists

### Complex Semicolon Usage
```turtle
:subject
    :p1 :o1 ;
    :p2 :o2, :o3, :o4 ;
    :p3 [
        :nested1 "value1" ;
        :nested2 "value2"
    ] ;
    :p4 :o5 ;
    .  # Final dot
```

### Empty Predicate After Semicolon
```turtle
:subject 
    :p1 :o1 ;
    # Empty predicate is allowed
    ;
    :p2 :o2 .
```

### Trailing Comma
```turtle
:subject :predicate 
    :object1,
    :object2,
    # Trailing comma is allowed before semicolon
    ;
    :p2 :o3 .
```

## 8. Language Tags and Datatypes

### Language Tag Variations
```turtle
# Simple language tag
:s1 :text "Hello"@en .

# Language tag with region
:s2 :text "Hello"@en-US .

# Language tag with script
:s3 :text "Hello"@zh-Hans .

# Complex language tag
:s4 :text "Hello"@en-US-x-private .
```

### Datatype Edge Cases
```turtle
# Various XML Schema datatypes
:s1 :value "42"^^<http://www.w3.org/2001/XMLSchema#integer> .
:s2 :value "2024-01-01"^^<http://www.w3.org/2001/XMLSchema#date> .
:s3 :value "true"^^<http://www.w3.org/2001/XMLSchema#boolean> .

# Custom datatypes
:s4 :value "custom"^^<http://example.org/MyType> .
```

## 9. Comment Edge Cases

### Comments in Various Positions
```turtle
# Comment at start
@prefix ex: <http://example.org/> . # Comment after directive
ex:s # Comment in middle
    ex:p # Another comment
    ex:o # Comment before dot
    . # Comment after dot
```

### Comments Don't Break Strings or IRIs
```turtle
# Hash in string is not a comment
:s :p "This # is not a comment" .

# Hash in IRI is not a comment
<http://example.org/page#section> a :Thing .
```

## 10. Whitespace Handling

### Significant Whitespace in ANON
```turtle
# Various amounts of whitespace in anonymous blank nodes
:s1 :p [] .
:s2 :p [  ] .
:s3 :p [
    
    ] .
:s4 :p [ # comment inside
    ] .
```

### No Whitespace in Numbers
```turtle
# CORRECT
:s :value -42 .

# WRONG - space between sign and number
# :s :value - 42 .
```

## 11. Base URI Resolution

### Base URI Changes
```turtle
@base <http://example.org/> .
<path1> a :Thing .  # http://example.org/path1

@base <http://other.org/> .
<path2> a :Thing .  # http://other.org/path2
```

### Relative IRI Resolution
```turtle
@base <http://example.org/dir/> .
<file> a :Thing .           # http://example.org/dir/file
<../parent> a :Thing .      # http://example.org/parent
<subdir/file> a :Thing .    # http://example.org/dir/subdir/file
<#fragment> a :Thing .      # http://example.org/dir/#fragment
```

## 12. Mixed Directive Styles

### Mixing Turtle and SPARQL-style Directives
```turtle
@prefix ex: <http://example.org/> .
PREFIX other: <http://other.org/>
@base <http://base.org/> .
BASE <http://newbase.org/>

# All directives are active
ex:s1 other:p <relative> .
```

## 13. Collections as Subjects

### Collection in Subject Position
```turtle
# List as subject (creates blank node)
(1 2 3) :sum 6 .

# Nested collection as subject
((1 2) (3 4)) :type :NestedList .
```

## 14. Literal Subjects (Invalid)

```turtle
# These are INVALID in standard Turtle
# "literal" :p :o .  # ERROR - literals cannot be subjects
# 42 :p :o .         # ERROR - literals cannot be subjects
# true :p :o .       # ERROR - literals cannot be subjects
```

## 15. Parser Stress Tests

### Maximum Nesting
```turtle
# Deeply nested structures
:s :p [
    :p [
        :p [
            :p [
                :p [
                    :p "Deep value"
                ]
            ]
        ]
    ]
] .
```

### Large Collections
```turtle
# Collection with many elements
:s :numbers (1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20) .
```

### Complex Mixed Structure
```turtle
@prefix ex: <http://example.org/> .

_:complex a ex:ComplexNode ;
    ex:hasCollection (
        [ ex:name "Item 1" ; ex:sublist (1 2 3) ]
        [ ex:name "Item 2" ; ex:ref _:complex ]
        "Plain string"
        42
        true
    ) ;
    ex:hasProperties [
        a ex:Anonymous ;
        ex:nestedAnon [
            ex:deepValue (
                ((1 2) (3 4))
                ((5 6) (7 8))
            )
        ]
    ] ;
    ex:selfRef _:complex .
```

## Parser Implementation Recommendations

1. **Blank Node Management**: Maintain a symbol table for labeled blank nodes to handle cycles and multiple references
2. **Collection Parsing**: Use a stack-based approach for nested collections
3. **String Parsing**: Implement proper escape sequence handling for all string types
4. **Error Recovery**: Provide meaningful error messages for common mistakes
5. **Memory Efficiency**: Stream large files rather than loading entirely into memory
6. **Unicode Support**: Ensure full Unicode support in all contexts
7. **IRI Resolution**: Implement proper relative IRI resolution according to RFC 3986

## Testing Strategy

1. **Positive Tests**: Verify all valid constructs parse correctly
2. **Negative Tests**: Ensure invalid syntax is properly rejected
3. **Round-trip Tests**: Parse and serialize to verify correctness
4. **Performance Tests**: Test with large files and complex structures
5. **Conformance Tests**: Use W3C test suite for compliance validation