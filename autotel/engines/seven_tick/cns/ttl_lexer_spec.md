# TTL (Time-To-Live) Lexer Specification for C

## Overview

This specification defines the lexical patterns for parsing TTL (Time-To-Live) values in C programs. TTL values are commonly used in network protocols, caching systems, databases, and various system configurations to specify duration or expiration times.

## 1. Numeric Formats

### 1.1 Integer Representations
- **Decimal**: `123`, `9999`, `0`
- **Hexadecimal**: `0x1F4`, `0X3E8`, `0xFFFF`
- **Octal**: `0755`, `0177`
- **Binary** (C23): `0b11111111`, `0B10101010`

### 1.2 Floating-Point Representations
- **Standard**: `3.14`, `0.5`, `100.0`
- **Scientific**: `1e3`, `2.5e-2`, `1E6`
- **With suffix**: `3.14f`, `2.0`, `1.5L`

### 1.3 Unsigned/Long Suffixes
- **Unsigned**: `100U`, `200u`
- **Long**: `1000L`, `2000l`
- **Unsigned Long**: `3000UL`, `4000ul`, `5000LU`, `6000lu`
- **Long Long**: `1000000LL`, `2000000ll`
- **Unsigned Long Long**: `3000000ULL`, `4000000ull`

## 2. Time Unit Suffixes

### 2.1 Standard Units
- **Nanoseconds**: `ns`, `nsec`, `nanosec`, `nanoseconds`
- **Microseconds**: `us`, `µs`, `usec`, `microsec`, `microseconds`
- **Milliseconds**: `ms`, `msec`, `millisec`, `milliseconds`
- **Seconds**: `s`, `sec`, `second`, `seconds`
- **Minutes**: `m`, `min`, `minute`, `minutes`
- **Hours**: `h`, `hr`, `hour`, `hours`
- **Days**: `d`, `day`, `days`
- **Weeks**: `w`, `week`, `weeks`
- **Months**: `M`, `mon`, `month`, `months`
- **Years**: `y`, `yr`, `year`, `years`

### 2.2 Case Sensitivity
- Most implementations are case-insensitive: `1s`, `1S`, `1Sec`, `1SEC`
- Some systems distinguish: `m` (minutes) vs `M` (months)

## 3. Common TTL Patterns

### 3.1 Simple Numeric TTL
```c
// Pure numbers (usually seconds by default)
int ttl = 3600;         // 1 hour in seconds
int ttl = 86400;        // 1 day in seconds
int ttl = 604800;       // 1 week in seconds
```

### 3.2 TTL with Unit Suffix (String Parsing)
```c
// Common in configuration files and APIs
"30s"       // 30 seconds
"5m"        // 5 minutes
"24h"       // 24 hours
"7d"        // 7 days
"1.5h"      // 1.5 hours
"0.5s"      // 500 milliseconds
```

### 3.3 Compound TTL Expressions
```c
// Some systems support compound expressions
"1h30m"     // 1 hour and 30 minutes
"2d12h"     // 2 days and 12 hours
"1w3d"      // 1 week and 3 days
```

### 3.4 Macro-based TTL
```c
#define TTL_SECOND      1
#define TTL_MINUTE      (60 * TTL_SECOND)
#define TTL_HOUR        (60 * TTL_MINUTE)
#define TTL_DAY         (24 * TTL_HOUR)
#define TTL_WEEK        (7 * TTL_DAY)

// Usage
int cache_ttl = 5 * TTL_MINUTE;
int session_ttl = 2 * TTL_HOUR;
```

## 4. Context-Specific Patterns

### 4.1 Network Protocols
```c
// IP TTL (0-255)
unsigned char ip_ttl = 64;     // Default for many systems
unsigned char ip_ttl = 255;    // Maximum

// DNS TTL (seconds)
uint32_t dns_ttl = 300;         // 5 minutes
uint32_t dns_ttl = 86400;       // 1 day
```

### 4.2 HTTP Cache Headers
```c
// Cache-Control max-age (seconds)
"max-age=3600"                  // 1 hour
"max-age=0"                     // No cache
"max-age=31536000"              // 1 year (common for static assets)
```

### 4.3 Redis/Memcached
```c
// EXPIRE command (seconds)
"EXPIRE key 3600"               // 1 hour
"EXPIREAT key 1672531200"       // Unix timestamp
"PEXPIRE key 5000"              // 5000 milliseconds
"SETEX key 86400 value"         // SET with expiry
```

### 4.4 Database Systems
```c
// PostgreSQL intervals
"'1 hour'"
"'30 minutes'"
"'7 days'"
"'1 year 2 months 3 days'"

// MySQL
"INTERVAL 1 HOUR"
"INTERVAL 30 MINUTE"
"INTERVAL 7 DAY"
```

## 5. Special Values

### 5.1 Infinite/No Expiry
- `-1` (common convention)
- `0` (sometimes means no expiry, sometimes immediate expiry)
- `UINT_MAX`, `ULONG_MAX`
- `"never"`, `"infinite"`, `"permanent"`

### 5.2 Immediate Expiry
- `0` (context-dependent)
- `"now"`, `"immediate"`

## 6. Edge Cases and Invalid Formats

### 6.1 Invalid Numeric Formats
- Leading zeros in decimal: `0123` (might be octal)
- Invalid hex: `0xGHI`
- Overflow: numbers exceeding type limits
- Multiple decimal points: `1.2.3`

### 6.2 Invalid Unit Combinations
- Duplicate units: `"1h1h"`
- Conflicting units: `"1s1ms"`
- Unknown units: `"1x"`, `"5foo"`
- Mixed separators: `"1h:30m"`

### 6.3 Ambiguous Formats
- `"1m"` - minutes or months?
- `"1"` - what unit?
- Whitespace handling: `"1 h"`, `"1h "`, `" 1h"`

## 7. Lexer Token Categories

### 7.1 Primary Tokens
1. **TTL_NUMBER**: Numeric value (integer or float)
2. **TTL_UNIT**: Time unit suffix
3. **TTL_SEPARATOR**: Separators in compound expressions
4. **TTL_SPECIAL**: Special values (infinite, never, etc.)

### 7.2 Token Attributes
- Value: Numeric value
- Unit: Time unit type
- Scale: Multiplication factor to convert to base unit
- Flags: Special properties (infinite, immediate, etc.)

## 8. Recommended Parsing Strategy

1. **Tokenization Phase**
   - Identify numeric values
   - Identify unit suffixes
   - Handle compound expressions

2. **Validation Phase**
   - Check numeric ranges
   - Validate unit combinations
   - Handle special cases

3. **Normalization Phase**
   - Convert to common base unit (usually seconds or milliseconds)
   - Apply scaling factors
   - Handle overflow/underflow

## 9. Implementation Considerations

### 9.1 Performance
- Use lookup tables for unit conversion
- Minimize string allocations
- Consider compile-time computation for constants

### 9.2 Compatibility
- Support multiple unit formats
- Handle platform-specific representations
- Consider locale-specific formats

### 9.3 Error Handling
- Clear error messages for invalid formats
- Graceful degradation
- Default values for unparseable input

## 10. Testing Scenarios

### 10.1 Valid Cases
- All numeric formats with all unit combinations
- Boundary values (0, MAX_INT, etc.)
- Floating-point edge cases
- Compound expressions

### 10.2 Invalid Cases
- Malformed numbers
- Unknown units
- Overflow scenarios
- Empty strings
- Null pointers

### 10.3 Stress Tests
- Very large inputs
- Many compound units
- Unicode in unit strings
- Concurrent parsing

## References

1. POSIX time.h conventions
2. ISO 8601 duration format
3. RFC 3339 time format
4. Redis EXPIRE documentation
5. HTTP Cache-Control specification (RFC 7234)
6. Various C standard library time functions

---

This specification serves as a comprehensive guide for implementing a robust TTL lexer in C that can handle the diverse formats encountered in real-world applications.