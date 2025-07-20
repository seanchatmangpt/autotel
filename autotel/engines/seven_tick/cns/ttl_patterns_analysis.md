# TTL Patterns Analysis - Deep Dive

## Real-World TTL Usage Analysis

### 1. Network Protocol Patterns

#### 1.1 TCP/IP TTL
```c
// Standard IP packet TTL
struct ip_header {
    uint8_t ttl;  // 0-255, typically 64 or 128
};

// Common defaults by OS:
// Linux: 64
// Windows: 128
// macOS: 64
// Router decrements: usually 1 per hop
```

#### 1.2 DNS TTL Patterns
```c
// DNS Resource Record
struct dns_rr {
    uint32_t ttl;  // In seconds, 0 to 2^32-1
};

// Common DNS TTL values:
// 300     (5 minutes)    - Dynamic records
// 3600    (1 hour)       - Standard records
// 86400   (1 day)        - Stable records
// 604800  (1 week)       - Very stable records
// 0                      - No caching allowed
```

### 2. Cache System Patterns

#### 2.1 Redis Patterns
```c
// Redis uses seconds for EXPIRE, milliseconds for PEXPIRE
// Patterns found in production:

// Simple expire
"EXPIRE key 3600"              // 1 hour
"EXPIRE session:123 7200"      // 2 hours

// Expire at specific time
"EXPIREAT key 1609459200"      // Unix timestamp

// Set with expiry
"SETEX cache:user:123 300 data"  // 5 minutes

// Millisecond precision
"PEXPIRE key 5500"             // 5.5 seconds
"PSETEX key 1500 value"        // 1.5 seconds
```

#### 2.2 Memcached Patterns
```c
// Memcached uses Unix timestamp or seconds from now
// Maximum: 30 days (2592000 seconds)

set key 0 300 5\r\n     // 300 seconds TTL
hello\r\n

set key 0 0 5\r\n       // No expiration
world\r\n

// Special case: values > 30 days treated as Unix timestamp
set key 0 1609459200 5\r\n  // Specific timestamp
```

### 3. HTTP Cache Patterns

#### 3.1 Cache-Control Headers
```c
// Common patterns in HTTP responses
"Cache-Control: max-age=3600"                    // 1 hour
"Cache-Control: max-age=31536000, immutable"     // 1 year (static assets)
"Cache-Control: max-age=0, must-revalidate"      // Always revalidate
"Cache-Control: no-cache, no-store"              // Never cache
"Cache-Control: s-maxage=3600"                   // Shared cache TTL
```

#### 3.2 Expires Header
```c
// Older HTTP/1.0 pattern
"Expires: Thu, 01 Dec 2024 16:00:00 GMT"        // Absolute time
"Expires: 0"                                     // Already expired
"Expires: -1"                                    // Already expired (IE compatible)
```

### 4. Configuration File Patterns

#### 4.1 Nginx Configuration
```nginx
# Various TTL patterns in nginx
proxy_cache_valid 200 302 10m;    # 10 minutes
proxy_cache_valid 404 1m;         # 1 minute
proxy_cache_valid any 1h;         # 1 hour

expires 1h;                       # 1 hour
expires 7d;                       # 7 days
expires epoch;                    # Already expired
expires max;                      # Far future (10 years)
expires -1;                       # Already expired
```

#### 4.2 Apache Configuration
```apache
# Apache mod_expires patterns
ExpiresDefault "access plus 1 month"
ExpiresByType image/jpg "access plus 1 year"
ExpiresByType text/css "modification plus 1 hour"

# Alternative format
ExpiresDefault A3600       # 1 hour after access
ExpiresDefault M86400      # 1 day after modification
```

### 5. Database-Specific Patterns

#### 5.1 PostgreSQL
```sql
-- Interval syntax
SELECT NOW() + INTERVAL '1 hour';
SELECT NOW() + INTERVAL '30 minutes';
SELECT NOW() + INTERVAL '7 days';
SELECT NOW() + INTERVAL '1 year 2 months 3 days 4 hours 5 minutes 6 seconds';

-- Short form
SELECT NOW() + '1h'::interval;
SELECT NOW() + '30m'::interval;
```

#### 5.2 MySQL
```sql
-- DATE_ADD syntax
SELECT DATE_ADD(NOW(), INTERVAL 1 HOUR);
SELECT DATE_ADD(NOW(), INTERVAL 30 MINUTE);
SELECT DATE_ADD(NOW(), INTERVAL 7 DAY);

-- Compound intervals
SELECT DATE_ADD(NOW(), INTERVAL '1:30' HOUR_MINUTE);
SELECT DATE_ADD(NOW(), INTERVAL '7 12:30:15' DAY_SECOND);
```

### 6. Special Edge Cases Found

#### 6.1 Ambiguous Zero Values
```c
// Zero can mean different things:
ttl = 0;  // Could mean:
          // - Expire immediately
          // - Never expire
          // - Use default TTL
          // - Invalid/not set
```

#### 6.2 Overflow Scenarios
```c
// 32-bit seconds overflow (Year 2038 problem)
uint32_t ttl = 2147483647;  // Max signed 32-bit: ~68 years

// Some systems use special values
#define TTL_FOREVER UINT32_MAX    // 4294967295
#define TTL_DEFAULT 0
#define TTL_IMMEDIATE -1
```

#### 6.3 Unit Ambiguity
```c
// Real-world confusion cases:
"1m"    // Minutes in most systems
"1M"    // Months in some systems, Megabytes in others
"1s"    // Seconds
"1S"    // Usually seconds, sometimes invalid

// Microseconds representation varies:
"1us"   // Common
"1µs"   // Unicode micro symbol
"1μs"   // Greek mu
```

### 7. Performance Patterns

#### 7.1 Caching Strategies
```c
// Layered TTL approach
#define L1_CACHE_TTL 60         // 1 minute (hot cache)
#define L2_CACHE_TTL 3600       // 1 hour (warm cache)
#define L3_CACHE_TTL 86400      // 1 day (cold cache)

// Jittered TTL to prevent thundering herd
int ttl_with_jitter = base_ttl + (rand() % (base_ttl / 10));
```

#### 7.2 TTL Bucketing
```c
// Round TTL to common values for better cache efficiency
int round_ttl(int ttl) {
    if (ttl <= 300) return 300;         // 5 min
    if (ttl <= 900) return 900;         // 15 min
    if (ttl <= 3600) return 3600;       // 1 hour
    if (ttl <= 14400) return 14400;     // 4 hours
    if (ttl <= 86400) return 86400;     // 1 day
    return 604800;                       // 1 week
}
```

### 8. Internationalization Considerations

#### 8.1 Locale-Specific Formats
```c
// Some systems support localized units
"1 jour"     // French: 1 day
"1 Tag"      // German: 1 day
"1日"        // Japanese: 1 day
"1 день"     // Russian: 1 day
```

#### 8.2 Number Format Variations
```c
// Decimal separators vary by locale
"1.5h"       // US: 1.5 hours
"1,5h"       // EU: 1.5 hours

// Thousand separators
"1000s"      // US: 1000 seconds
"1.000s"     // EU: 1000 seconds
"1 000s"     // Some locales: 1000 seconds
```

### 9. Security Considerations

#### 9.1 TTL Injection Attacks
```c
// Malicious TTL values to watch for:
"-1; DROP TABLE users; --"
"0 OR 1=1"
"../../../etc/passwd"
"<script>alert('XSS')</script>"
```

#### 9.2 Resource Exhaustion
```c
// Extremely large TTL values
"999999999999999999d"   // Overflow attempt
"1e308s"                // Float overflow
"0x7FFFFFFFFFFFFFFF"    // Max int64
```

### 10. Modern Trends

#### 10.1 Millisecond Precision
```c
// Increasing use of millisecond precision
"500ms"      // Half second
"1500ms"     // 1.5 seconds
"250ms"      // Quarter second
```

#### 10.2 Human-Readable Formats
```c
// More systems accepting natural language
"in 5 minutes"
"after 1 hour"
"tomorrow at 3pm"
"next Monday"
```

## Lexer Design Recommendations

Based on this analysis, the lexer should:

1. **Handle multiple numeric bases** (decimal, hex, octal, binary)
2. **Support floating-point** for sub-second precision
3. **Be case-insensitive** for units (with option to be strict)
4. **Recognize compound expressions** (e.g., "1h30m")
5. **Validate reasonable bounds** (prevent overflow)
6. **Support common aliases** (s/sec/second/seconds)
7. **Handle special values** (-1, 0, infinity)
8. **Provide clear error messages** for ambiguous inputs
9. **Consider context** (network vs cache vs database)
10. **Optimize for common cases** (cache frequent conversions)

## Testing Priority

High-priority test cases based on real-world usage:
1. Common time units (s, m, h, d)
2. Zero and negative handling
3. Maximum value boundaries
4. Millisecond precision
5. Compound expressions
6. Case sensitivity
7. Whitespace handling
8. Invalid unit detection
9. Overflow prevention
10. Special value recognition