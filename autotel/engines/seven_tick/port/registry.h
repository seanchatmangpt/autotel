#ifndef REGISTRY_H
#define REGISTRY_H

#include <stdint.h> // For uint64_t

// Define a structure for an actor's identity in the registry
typedef struct {
    uint64_t actor_id;      // Unique numerical ID for the actor
    void* memory_location;  // Pointer to the actor's memory location (e.g., state, mailbox)
    // Add other relevant fields like process ID (pid) if applicable
} registry_entry_t;

// Initialize the registry
void registry_init();

// Register an actor with its name, ID, and memory location
// Returns 0 on success, non-zero on failure (e.g., name already exists)
int registry_register_actor(const char* actor_name, uint64_t actor_id, void* memory_location);

// Resolve an actor's name to its registry entry
// Returns a pointer to the registry_entry_t on success, NULL if not found
const registry_entry_t* registry_resolve_name(const char* actor_name);

// Resolve an actor's ID to its registry entry
// Returns a pointer to the registry_entry_t on success, NULL if not found
const registry_entry_t* registry_resolve_id(uint64_t actor_id);

// Shutdown the registry
void registry_shutdown();

#endif // REGISTRY_H
