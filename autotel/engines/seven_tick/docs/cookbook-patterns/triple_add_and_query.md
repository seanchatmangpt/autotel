# Pattern: Add and Query Triples

## Description
This pattern demonstrates how to add triples to the 7T Engine and query them efficiently using the core API. It covers both single and multiple object scenarios for a given (subject, predicate) pair.

## Code Example
```c
#include "c_src/sparql7t.h"
#include <assert.h>
#include <stdio.h>

int main() {
    // Create engine with capacity for 1000 subjects, 100 predicates, 1000 objects
    S7TEngine* engine = s7t_create(1000, 100, 1000);
    assert(engine != NULL);

    // Add triples
    s7t_add_triple(engine, 1, 2, 3);  // (subject=1, predicate=2, object=3)
    s7t_add_triple(engine, 1, 2, 4);  // (subject=1, predicate=2, object=4)
    s7t_add_triple(engine, 2, 2, 3);  // (subject=2, predicate=2, object=3)

    // Query triples
    int found1 = s7t_ask_pattern(engine, 1, 2, 3);  // Should be 1
    int found2 = s7t_ask_pattern(engine, 1, 2, 4);  // Should be 1
    int found3 = s7t_ask_pattern(engine, 1, 2, 5);  // Should be 0

    printf("(1,2,3): %d\n", found1);
    printf("(1,2,4): %d\n", found2);
    printf("(1,2,5): %d\n", found3);

    // Cleanup
    s7t_destroy(engine);
    return 0;
}
```

## Tips
- Always call `s7t_destroy(engine)` to free memory.
- Use integer IDs for subjects, predicates, and objects (string interning recommended).
- For batch queries, see the `batch_pattern_matching.md` pattern. 