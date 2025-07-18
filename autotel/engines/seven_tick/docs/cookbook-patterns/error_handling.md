# Pattern: Robust Error Handling

## Description
This pattern demonstrates how to implement comprehensive error handling in the 7T Engine, including input validation, memory error recovery, and graceful degradation for production systems.

## Code Example
```c
#include "c_src/sparql7t.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

// Error codes
#define S7T_SUCCESS 0
#define S7T_ERROR_NULL_POINTER -1
#define S7T_ERROR_INVALID_PARAMETER -2
#define S7T_ERROR_MEMORY_ALLOCATION -3
#define S7T_ERROR_ENGINE_FULL -4

// Safe engine creation with error handling
S7TEngine* safe_create_engine(size_t max_s, size_t max_p, size_t max_o) {
    if (max_s == 0 || max_p == 0 || max_o == 0) {
        fprintf(stderr, "Invalid engine parameters: %zu, %zu, %zu\n", max_s, max_p, max_o);
        return NULL;
    }
    
    S7TEngine* engine = s7t_create(max_s, max_p, max_o);
    if (!engine) {
        fprintf(stderr, "Failed to create engine: %s\n", strerror(errno));
        return NULL;
    }
    
    return engine;
}

// Safe triple addition with validation
int safe_add_triple(S7TEngine* engine, uint32_t s, uint32_t p, uint32_t o) {
    if (!engine) {
        fprintf(stderr, "Engine is NULL\n");
        return S7T_ERROR_NULL_POINTER;
    }
    
    if (s >= engine->max_subjects || p >= engine->max_predicates || o >= engine->max_objects) {
        fprintf(stderr, "Invalid triple parameters: s=%u, p=%u, o=%u\n", s, p, o);
        return S7T_ERROR_INVALID_PARAMETER;
    }
    
    s7t_add_triple(engine, s, p, o);
    return S7T_SUCCESS;
}

// Safe pattern matching with error checking
int safe_ask_pattern(S7TEngine* engine, uint32_t s, uint32_t p, uint32_t o) {
    if (!engine) {
        fprintf(stderr, "Engine is NULL\n");
        return S7T_ERROR_NULL_POINTER;
    }
    
    if (s >= engine->max_subjects || p >= engine->max_predicates || o >= engine->max_objects) {
        fprintf(stderr, "Invalid pattern parameters: s=%u, p=%u, o=%u\n", s, p, o);
        return S7T_ERROR_INVALID_PARAMETER;
    }
    
    return s7t_ask_pattern(engine, s, p, o);
}

// Batch operation with error handling
int safe_batch_operation(S7TEngine* engine, TriplePattern* patterns, int* results, size_t count) {
    if (!engine || !patterns || !results) {
        fprintf(stderr, "Invalid parameters for batch operation\n");
        return S7T_ERROR_NULL_POINTER;
    }
    
    if (count == 0) {
        fprintf(stderr, "Batch count is zero\n");
        return S7T_ERROR_INVALID_PARAMETER;
    }
    
    // Validate all patterns before processing
    for (size_t i = 0; i < count; i++) {
        if (patterns[i].s >= engine->max_subjects || 
            patterns[i].p >= engine->max_predicates || 
            patterns[i].o >= engine->max_objects) {
            fprintf(stderr, "Invalid pattern at index %zu: s=%u, p=%u, o=%u\n", 
                   i, patterns[i].s, patterns[i].p, patterns[i].o);
            return S7T_ERROR_INVALID_PARAMETER;
        }
    }
    
    s7t_ask_batch(engine, patterns, results, count);
    return S7T_SUCCESS;
}

int main() {
    // Create engine with error handling
    S7TEngine* engine = safe_create_engine(1000, 100, 1000);
    if (!engine) {
        fprintf(stderr, "Failed to create engine\n");
        return 1;
    }
    
    // Add triples with error checking
    int result = safe_add_triple(engine, 1, 2, 3);
    if (result != S7T_SUCCESS) {
        fprintf(stderr, "Failed to add triple: %d\n", result);
        s7t_destroy(engine);
        return 1;
    }
    
    // Query with error checking
    result = safe_ask_pattern(engine, 1, 2, 3);
    if (result < 0) {
        fprintf(stderr, "Query failed: %d\n", result);
    } else {
        printf("Pattern found: %d\n", result);
    }
    
    // Batch operation with error handling
    TriplePattern patterns[2] = {{1, 2, 3}, {1, 2, 4}};
    int results[2];
    result = safe_batch_operation(engine, patterns, results, 2);
    if (result == S7T_SUCCESS) {
        printf("Batch results: [%d, %d]\n", results[0], results[1]);
    }
    
    s7t_destroy(engine);
    return 0;
}
```

## Tips
- Always validate parameters before operations.
- Use consistent error codes across your application.
- Log errors with sufficient context for debugging.
- Implement graceful degradation for non-critical failures. 