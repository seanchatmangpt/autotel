# TTL Lexer Formal Grammar Specification

## EBNF Grammar Definition

```ebnf
(* TTL Lexer Grammar in Extended Backus-Naur Form *)

(* Top-level rules *)
ttl_expression    ::= simple_ttl | compound_ttl | special_ttl
simple_ttl        ::= numeric_value [whitespace] [time_unit]
compound_ttl      ::= ttl_component {ttl_component}
special_ttl       ::= special_keyword | special_number

(* TTL Components *)
ttl_component     ::= numeric_value [whitespace] time_unit [whitespace]

(* Numeric Values *)
numeric_value     ::= integer_literal | float_literal
integer_literal   ::= decimal_int | hex_int | octal_int | binary_int
float_literal     ::= decimal_float | scientific_float

(* Integer Formats *)
decimal_int       ::= [sign] decimal_digits [int_suffix]
hex_int          ::= "0" ("x" | "X") hex_digits [int_suffix]
octal_int        ::= "0" octal_digits [int_suffix]
binary_int       ::= "0" ("b" | "B") binary_digits [int_suffix]

(* Float Formats *)
decimal_float    ::= [sign] decimal_digits "." decimal_digits [float_suffix]
scientific_float ::= [sign] decimal_digits ["." decimal_digits] 
                    ("e" | "E") [sign] decimal_digits [float_suffix]

(* Digit Sequences *)
decimal_digits   ::= decimal_digit {decimal_digit}
hex_digits       ::= hex_digit {hex_digit}
octal_digits     ::= octal_digit {octal_digit}
binary_digits    ::= binary_digit {binary_digit}

(* Individual Digits *)
decimal_digit    ::= "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9"
hex_digit        ::= decimal_digit | "a" | "b" | "c" | "d" | "e" | "f" 
                                   | "A" | "B" | "C" | "D" | "E" | "F"
octal_digit      ::= "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7"
binary_digit     ::= "0" | "1"

(* Suffixes *)
int_suffix       ::= ["u" | "U"] ["l" | "L" ["l" | "L"]] 
                   | ["l" | "L" ["l" | "L"]] ["u" | "U"]
float_suffix     ::= "f" | "F" | "l" | "L"

(* Time Units *)
time_unit        ::= nanosec_unit | microsec_unit | millisec_unit 
                   | second_unit | minute_unit | hour_unit 
                   | day_unit | week_unit | month_unit | year_unit

nanosec_unit     ::= "ns" | "nsec" | "nanosec" | "nanosecond" | "nanoseconds"
microsec_unit    ::= "us" | "µs" | "μs" | "usec" | "microsec" 
                   | "microsecond" | "microseconds"
millisec_unit    ::= "ms" | "msec" | "millisec" | "millisecond" | "milliseconds"
second_unit      ::= "s" | "sec" | "second" | "seconds"
minute_unit      ::= "m" | "min" | "minute" | "minutes"
hour_unit        ::= "h" | "hr" | "hrs" | "hour" | "hours"
day_unit         ::= "d" | "day" | "days"
week_unit        ::= "w" | "wk" | "week" | "weeks"
month_unit       ::= "M" | "mon" | "month" | "months"
year_unit        ::= "y" | "yr" | "year" | "years"

(* Special Values *)
special_keyword  ::= "never" | "infinite" | "infinity" | "forever" 
                   | "permanent" | "now" | "immediate" | "expired"
special_number   ::= "-1" | "0"

(* Utilities *)
sign             ::= "+" | "-"
whitespace       ::= " " | "\t" | "\n" | "\r"
```

## Regex Patterns for Implementation

```c
// Core numeric patterns
#define DECIMAL_INT_PATTERN     "^[+-]?[0-9]+[uU]?[lL]{0,2}[uU]?$"
#define HEX_INT_PATTERN         "^0[xX][0-9a-fA-F]+[uU]?[lL]{0,2}[uU]?$"
#define OCTAL_INT_PATTERN       "^0[0-7]+[uU]?[lL]{0,2}[uU]?$"
#define BINARY_INT_PATTERN      "^0[bB][01]+[uU]?[lL]{0,2}[uU]?$"
#define FLOAT_PATTERN           "^[+-]?[0-9]+\\.[0-9]+[fFlL]?$"
#define SCIENTIFIC_PATTERN      "^[+-]?[0-9]+(\\.[0-9]+)?[eE][+-]?[0-9]+[fFlL]?$"

// Time unit patterns (case-insensitive)
#define NANOSEC_PATTERN         "^(ns|nsec|nanosec|nanoseconds?)$"
#define MICROSEC_PATTERN        "^(us|µs|μs|usec|microsec|microseconds?)$"
#define MILLISEC_PATTERN        "^(ms|msec|millisec|milliseconds?)$"
#define SECOND_PATTERN          "^(s|sec|seconds?)$"
#define MINUTE_PATTERN          "^(m|min|minutes?)$"
#define HOUR_PATTERN            "^(h|hrs?|hours?)$"
#define DAY_PATTERN             "^(d|days?)$"
#define WEEK_PATTERN            "^(w|wk|weeks?)$"
#define MONTH_PATTERN           "^(M|mon|months?)$"
#define YEAR_PATTERN            "^(y|yr|years?)$"

// Compound TTL pattern
#define COMPOUND_TTL_PATTERN    "^([0-9]+(\\.[0-9]+)?[a-zA-Zµμ]+\\s*)+$"

// Special value patterns
#define SPECIAL_KEYWORD_PATTERN "^(never|infinite|infinity|forever|permanent|now|immediate|expired)$"
#define SPECIAL_NUMBER_PATTERN  "^(-1|0)$"
```

## Token Types and Attributes

```c
typedef enum {
    TOKEN_EOF,
    TOKEN_NUMBER,
    TOKEN_TIME_UNIT,
    TOKEN_SPECIAL,
    TOKEN_ERROR
} ttl_token_type_t;

typedef enum {
    UNIT_NANOSECOND,
    UNIT_MICROSECOND,
    UNIT_MILLISECOND,
    UNIT_SECOND,
    UNIT_MINUTE,
    UNIT_HOUR,
    UNIT_DAY,
    UNIT_WEEK,
    UNIT_MONTH,
    UNIT_YEAR,
    UNIT_NONE
} ttl_time_unit_t;

typedef enum {
    SPECIAL_NEVER,
    SPECIAL_INFINITE,
    SPECIAL_NOW,
    SPECIAL_EXPIRED,
    SPECIAL_NONE
} ttl_special_t;

typedef struct {
    ttl_token_type_t type;
    union {
        struct {
            double value;
            int is_float;
            int has_suffix;
            char suffix[4];  // ULL, L, F, etc.
        } number;
        struct {
            ttl_time_unit_t unit;
            char text[20];   // Original text
        } time_unit;
        struct {
            ttl_special_t special;
            char text[20];
        } special;
    } data;
    size_t position;
    size_t length;
} ttl_token_t;
```

## State Machine Definition

```c
typedef enum {
    STATE_START,
    STATE_SIGN,
    STATE_ZERO,
    STATE_DECIMAL,
    STATE_HEX_PREFIX,
    STATE_HEX,
    STATE_OCTAL,
    STATE_BINARY_PREFIX,
    STATE_BINARY,
    STATE_FLOAT_DOT,
    STATE_FLOAT_DECIMAL,
    STATE_EXPONENT,
    STATE_EXPONENT_SIGN,
    STATE_EXPONENT_DIGIT,
    STATE_SUFFIX,
    STATE_WHITESPACE,
    STATE_UNIT,
    STATE_DONE,
    STATE_ERROR
} lexer_state_t;
```

## Conversion Factors

```c
// Base unit: nanoseconds
static const int64_t time_unit_factors[] = {
    [UNIT_NANOSECOND]  = 1LL,
    [UNIT_MICROSECOND] = 1000LL,
    [UNIT_MILLISECOND] = 1000000LL,
    [UNIT_SECOND]      = 1000000000LL,
    [UNIT_MINUTE]      = 60000000000LL,
    [UNIT_HOUR]        = 3600000000000LL,
    [UNIT_DAY]         = 86400000000000LL,
    [UNIT_WEEK]        = 604800000000000LL,
    [UNIT_MONTH]       = 2592000000000000LL,  // 30 days approximation
    [UNIT_YEAR]        = 31536000000000000LL   // 365 days approximation
};
```

## Precedence and Associativity

For compound expressions:
1. Left-to-right parsing
2. No operator precedence (sequential addition)
3. Units must decrease in magnitude (1h30m valid, 30m1h invalid)

## Error Codes

```c
typedef enum {
    TTL_OK = 0,
    TTL_ERROR_INVALID_NUMBER,
    TTL_ERROR_INVALID_UNIT,
    TTL_ERROR_OVERFLOW,
    TTL_ERROR_UNDERFLOW,
    TTL_ERROR_INVALID_COMPOUND,
    TTL_ERROR_DUPLICATE_UNIT,
    TTL_ERROR_UNIT_ORDER,
    TTL_ERROR_EMPTY_INPUT,
    TTL_ERROR_MEMORY,
    TTL_ERROR_UNKNOWN
} ttl_error_t;
```

## Validation Rules

1. **Numeric Range**: Values must fit in int64_t when converted to base unit
2. **Unit Order**: In compounds, units must be in descending order of magnitude
3. **No Duplicates**: Each unit type can appear at most once
4. **Positive Values**: TTL values must be >= 0 (except special -1)
5. **Reasonable Bounds**: Warn if TTL > 100 years or < 1 nanosecond

## Implementation Guidelines

1. **Two-pass parsing**:
   - First pass: Tokenize into numbers and units
   - Second pass: Validate and compute final value

2. **Error Recovery**:
   - Continue parsing after errors to report all issues
   - Provide suggestions for common mistakes

3. **Performance**:
   - Use perfect hash for unit lookups
   - Pre-compile regex patterns
   - Cache conversion results

4. **Flexibility**:
   - Case-insensitive unit matching by default
   - Optional strict mode for case sensitivity
   - Configurable special value handling

## Example Parse Trees

### Simple TTL: "30s"
```
ttl_expression
└── simple_ttl
    ├── numeric_value
    │   └── decimal_int: 30
    └── time_unit
        └── second_unit: "s"
```

### Compound TTL: "1h30m45s"
```
ttl_expression
└── compound_ttl
    ├── ttl_component
    │   ├── numeric_value
    │   │   └── decimal_int: 1
    │   └── time_unit
    │       └── hour_unit: "h"
    ├── ttl_component
    │   ├── numeric_value
    │   │   └── decimal_int: 30
    │   └── time_unit
    │       └── minute_unit: "m"
    └── ttl_component
        ├── numeric_value
        │   └── decimal_int: 45
        └── time_unit
            └── second_unit: "s"
```

### Special TTL: "never"
```
ttl_expression
└── special_ttl
    └── special_keyword: "never"
```

This grammar provides a complete, unambiguous specification for parsing TTL values in C programs.