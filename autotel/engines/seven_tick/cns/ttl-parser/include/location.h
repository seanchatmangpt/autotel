#ifndef TTL_LOCATION_H
#define TTL_LOCATION_H

#include <stddef.h>

// Source location structure
typedef struct {
    size_t line;      // Line number (1-based)
    size_t column;    // Column number (1-based)
    size_t offset;    // Byte offset in source
    size_t length;    // Length of the token/error
} ttl_location_t;

// Location utilities
static inline ttl_location_t ttl_location_make(size_t line, size_t column, size_t offset, size_t length) {
    return (ttl_location_t){
        .line = line,
        .column = column,
        .offset = offset,
        .length = length
    };
}

static inline ttl_location_t ttl_location_span(ttl_location_t start, ttl_location_t end) {
    return (ttl_location_t){
        .line = start.line,
        .column = start.column,
        .offset = start.offset,
        .length = (end.offset + end.length) - start.offset
    };
}

#endif // TTL_LOCATION_H