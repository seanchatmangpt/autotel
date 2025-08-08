#include "registry.h"
#include <stdio.h>  // For printf
#include <stdlib.h> // For malloc, free (though mostly removed)
#include <string.h> // For strcmp, strdup (though mostly removed)

// Fixed-size array for the registry
static registry_entry_t registry_entries[MAX_ACTORS];
static uint32_t registry_count = 0;

// A default invalid entry to return for failed lookups
static const registry_entry_t INVALID_REGISTRY_ENTRY = {
    .actor_id = 0xFFFFFFFFFFFFFFFFULL, // Invalid ID
    .memory_location = NULL,
    .name_hash = 0xFFFFFFFFFFFFFFFFULL // Invalid hash
};

void registry_init() {
    printf("Registry: Initializing (fixed-size array, assuming zero-init or pre-filled)...\n");
    registry_count = 0;
}

int registry_register_actor(uint64_t name_hash, uint64_t actor_id, void* memory_location) {
    // In a 7-tick system, actor_id would likely be directly mapped to an index
    // or a pre-calculated hash. For this simulation, we'll use actor_id as index.
    // This assumes actor_id is within [0, MAX_ACTORS-1].
    if (actor_id >= MAX_ACTORS) {
        fprintf(stderr, "Registry: Error - Actor ID %llu out of bounds (MAX_ACTORS=%u).\n", actor_id, MAX_ACTORS);
        return 1; // Indicate error
    }

    // Direct placement into the array
    registry_entries[actor_id].actor_id = actor_id;
    registry_entries[actor_id].memory_location = memory_location;
    registry_entries[actor_id].name_hash = name_hash;

    // Increment count if this is a new entry (conceptual, for tracking)
    // In a true fixed-time system, this might be pre-calculated.
    if (registry_entries[actor_id].actor_id == INVALID_REGISTRY_ENTRY.actor_id) {
        registry_count++;
    }

    printf("Registry: Registered actor with ID %llu, Name Hash 0x%llX at %p\n", actor_id, name_hash, memory_location);
    return 0;
}

const registry_entry_t* registry_resolve_name_hash(uint64_t name_hash) {
    // Direct array lookup using name_hash as an index (or a portion of it)
    // This assumes a perfect hash or a collision-free mapping for active actors.
    // For a fixed-size array, we can use a bitwise AND to get an index.
    uint32_t index = name_hash & (MAX_ACTORS - 1); // Assumes MAX_ACTORS is a power of 2

    // In a true 7-tick system, the validity check might be implicit or handled by hardware.
    // For simulation, we return the entry directly.
    return &registry_entries[index];
}

const registry_entry_t* registry_resolve_id(uint64_t actor_id) {
    // Direct array lookup (assuming actor_id is the index)
    if (actor_id < MAX_ACTORS) {
        printf("Registry: Resolved ID %llu to Name Hash 0x%llX.\n", actor_id, registry_entries[actor_id].name_hash);
        return &registry_entries[actor_id];
    }
    printf("Registry: ID %llu out of bounds.\n", actor_id);
    return &INVALID_REGISTRY_ENTRY;
}

void registry_shutdown() {
    printf("Registry: Shutting down (resetting count)...\n");
    registry_count = 0; // Simply reset the count, no dynamic memory to free
}
