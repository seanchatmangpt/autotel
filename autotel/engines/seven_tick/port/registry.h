#ifndef REGISTRY_H
#define REGISTRY_H

#include <stdint.h> // For uint64_t

#define MAX_ACTORS 64 // Fixed size for the registry, must be power of 2 for potential bitwise ops

// Define a structure for an actor's identity in the registry
typedef struct {
    uint64_t actor_id;      // Unique numerical ID for the actor
    void* memory_location;  // Pointer to the actor's memory location (e.g., state, mailbox)
    uint64_t name_hash;     // Precomputed hash of the actor's name for direct lookup
} registry_entry_t;

// Initialize the registry
void registry_init();

// Register an actor with its name hash, ID, and memory location
// In a 7-tick system, this would be part of AOT compilation or a fixed-time setup.
// Returns 0 on success, non-zero on failure (e.g., index out of bounds).
int registry_register_actor(uint64_t name_hash, uint64_t actor_id, void* memory_location);

// Resolve an actor's name hash to its registry entry
// Returns a pointer to the registry_entry_t on success, NULL if not found (or a default invalid entry).
const registry_entry_t* registry_resolve_name_hash(uint64_t name_hash);

// Resolve an actor's ID to its registry entry
// Returns a pointer to the registry_entry_t on success, NULL if not found (or a default invalid entry).
const registry_entry_t* registry_resolve_id(uint64_t actor_id);

// Shutdown the registry (resets count, no freeing of static memory)
void registry_shutdown();

#endif // REGISTRY_H
