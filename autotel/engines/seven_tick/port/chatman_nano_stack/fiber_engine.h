#ifndef FIBER_ENGINE_H
#define FIBER_ENGINE_H

#include <stdint.h> // For uint64_t
#include "conductor_manifest.h" // For conductor_manifest_entry_t

// Structure to represent an actor's mailbox (simplified)
typedef struct {
    void* message; // Placeholder for a message
    uint64_t message_id; // Unique ID for the message
    int has_message; // Flag to indicate if there's a message
} actor_mailbox_t;

// Function to execute a fiber for a given actor
// actor_id: The ID of the actor
// actor_state: Pointer to the actor's internal state
// mailbox: Pointer to the actor's mailbox
// manifest_entry: The manifest entry for this actor's behavior
// Returns 0 on success, non-zero on error (e.g., tick budget exceeded)
int fiber_engine_execute_fiber(
    uint64_t actor_id,
    void* actor_state,
    actor_mailbox_t* mailbox,
    const conductor_manifest_entry_t* manifest_entry,
    meta_probe_collector_t* collector
);

#endif // FIBER_ENGINE_H
