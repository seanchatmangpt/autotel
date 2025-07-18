# Pattern: SIMD-Style Batch Pattern Matching

## Description
This pattern demonstrates how to use the 7T Engine's batch API to efficiently match multiple triple patterns in parallel, leveraging SIMD-style processing for maximum throughput.

## Code Example
```c
#include "c_src/sparql7t.h"
#include <assert.h>
#include <stdio.h>

int main() {
    S7TEngine* engine = s7t_create(1000, 100, 1000);
    assert(engine != NULL);

    // Add triples
    s7t_add_triple(engine, 1, 2, 3);
    s7t_add_triple(engine, 1, 2, 4);
    s7t_add_triple(engine, 2, 2, 3);

    // Prepare batch patterns
    TriplePattern patterns[4] = {
        {1, 2, 3},  // Should match
        {1, 2, 4},  // Should match
        {2, 2, 3},  // Should match
        {1, 2, 5}   // Should not match
    };
    int results[4];

    // Batch query
    s7t_ask_batch(engine, patterns, results, 4);

    printf("Batch results: [%d, %d, %d, %d]\n", results[0], results[1], results[2], results[3]);
    // Expected: [1, 1, 1, 0]

    s7t_destroy(engine);
    return 0;
}
```

## Tips
- Always use multiples of 4 for maximum SIMD efficiency.
- For large batches, process in chunks of 4.
- Batch operations are ideal for query workloads and validation tasks. 