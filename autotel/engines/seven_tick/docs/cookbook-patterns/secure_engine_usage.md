# Pattern: Secure Engine Usage and Access Control

## Description
This pattern demonstrates best practices for secure usage of the 7T Engine, including input validation, memory safety, and implementing basic access control for API operations.

## Code Example
```c
#include "c_src/sparql7t.h"
#include <assert.h>
#include <stdio.h>
#include <stdint.h>

// Example: Validate input parameters
int safe_add_triple(S7TEngine* engine, uint32_t s, uint32_t p, uint32_t o) {
    if (!engine) return 0;
    if (s >= engine->max_subjects || p >= engine->max_predicates || o >= engine->max_objects) {
        fprintf(stderr, "Invalid triple parameters: %u, %u, %u\n", s, p, o);
        return 0;
    }
    s7t_add_triple(engine, s, p, o);
    return 1;
}

// Example: Simple permission check
#define PERMISSION_WRITE 0x01

typedef struct {
    uint32_t user_id;
    uint32_t permissions;
} UserSession;

int check_permission(UserSession* session, uint32_t required) {
    return session && (session->permissions & required) == required;
}

int main() {
    S7TEngine* engine = s7t_create(1000, 100, 1000);
    assert(engine != NULL);
    UserSession session = { .user_id = 42, .permissions = PERMISSION_WRITE };

    // Secure triple addition
    if (check_permission(&session, PERMISSION_WRITE)) {
        safe_add_triple(engine, 1, 2, 3);
    } else {
        fprintf(stderr, "Permission denied\n");
    }

    s7t_destroy(engine);
    return 0;
}
```

## Tips
- Always validate all input parameters.
- Use permission checks for all write operations.
- Free all resources with `s7t_destroy` to prevent leaks.
- For advanced security, see the `SECURITY.md` guide. 