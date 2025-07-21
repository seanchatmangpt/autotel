#include "registry.h"
#include <stdio.h>  // For printf
#include <stdlib.h> // For malloc, free
#include <string.h> // For strcmp, strdup

// Simple in-memory registry using a linked list for demonstration.
// In a real system, this would be a more efficient hash table or similar.

typedef struct RegistryNode {
    registry_entry_t entry;
    char* actor_name; // Store name for lookup
    struct RegistryNode* next;
} RegistryNode;

static RegistryNode* registry_head = NULL;
static uint64_t next_actor_id = 1; // Simple ID generator

void registry_init() {
    printf("Registry: Initializing...\n");
    registry_head = NULL;
    next_actor_id = 1;
}

int registry_register_actor(const char* actor_name, uint64_t actor_id, void* memory_location) {
    // Check if name already exists
    RegistryNode* current = registry_head;
    while (current != NULL) {
        if (strcmp(current->actor_name, actor_name) == 0) {
            printf("Registry: Error - Actor name '%s' already registered.\n", actor_name);
            return 1; // Name already exists
        }
        current = current->next;
    }

    RegistryNode* new_node = (RegistryNode*)malloc(sizeof(RegistryNode));
    if (!new_node) {
        perror("Registry: Failed to allocate memory for new actor");
        return 1; // Allocation failed
    }

    new_node->actor_name = strdup(actor_name);
    if (!new_node->actor_name) {
        perror("Registry: Failed to allocate memory for actor name");
        free(new_node);
        return 1; // Allocation failed
    }

    new_node->entry.actor_id = (actor_id == 0) ? next_actor_id++ : actor_id; // Use provided ID or generate new
    new_node->entry.memory_location = memory_location;
    new_node->next = registry_head;
    registry_head = new_node;

    printf("Registry: Registered actor '%s' with ID %llu at %p\n", actor_name, new_node->entry.actor_id, new_node->entry.memory_location);
    // Conceptual connection to L5: Once an actor is registered and its memory location is known,
    // its behavior (defined in L5's conductor_manifest) can be associated with this physical instance.
    return 0;
}

const registry_entry_t* registry_resolve_name(const char* actor_name) {
    RegistryNode* current = registry_head;
    while (current != NULL) {
        if (strcmp(current->actor_name, actor_name) == 0) {
            printf("Registry: Resolved name '%s' to ID %llu.\n", actor_name, current->entry.actor_id);
            return &(current->entry);
        }
        current = current->next;
    }
    printf("Registry: Name '%s' not found.\n", actor_name);
    return NULL;
}

const registry_entry_t* registry_resolve_id(uint64_t actor_id) {
    RegistryNode* current = registry_head;
    while (current != NULL) {
        if (current->entry.actor_id == actor_id) {
            printf("Registry: Resolved ID %llu to name '%s'.\n", actor_id, current->actor_name);
            return &(current->entry);
        }
        current = current->next;
    }
    printf("Registry: ID %llu not found.\n", actor_id);
    return NULL;
}

void registry_shutdown() {
    printf("Registry: Shutting down...\n");
    RegistryNode* current = registry_head;
    while (current != NULL) {
        RegistryNode* next = current->next;
        free(current->actor_name);
        free(current);
        current = next;
    }
    registry_head = NULL;
}
