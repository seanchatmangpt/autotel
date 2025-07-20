#include <stdio.h>
#include <string.h>
#include <assert.h>

// Example data structures that match the generated zones
// Must be declared BEFORE including the generated arena code
typedef struct {
    int id;
    float x, y, z;
    int type;
    char name[32];
} Entity;

typedef struct {
    int entity_id;
    int component_type;
    union {
        float transform[4][4];
        struct { float r, g, b, a; } color;
        struct { float speed, direction; } movement;
    } data;
} Component;

// Include the generated arena code AFTER type declarations
#include "generated_arena.h"

// Example usage of the generated arena
int main(void) {
    printf("ARENAC Generated Arena Usage Example\n");
    printf("===================================\n\n");
    
    // The arena is already initialized as a static variable
    printf("Arena initialized:\n");
    printf("  Name: %s\n", game_arena.name);
    printf("  Total size: %zu bytes (%.2f MB)\n", 
           game_arena.end - game_arena.beg,
           (double)(game_arena.end - game_arena.beg) / (1024.0 * 1024.0));
    printf("  Available: %zu bytes\n", game_arena_available(&game_arena));
    printf("\n");
    
    // Test typed zone access - entities
    printf("Testing entity zone access:\n");
    Entity* first_entity = game_arena_get_entities(0);
    Entity* last_entity = game_arena_get_entities(999);
    Entity* invalid_entity = game_arena_get_entities(1000);
    
    printf("  First entity: %p\n", (void*)first_entity);
    printf("  Last entity: %p\n", (void*)last_entity);
    printf("  Invalid entity (index 1000): %p\n", (void*)invalid_entity);
    
    // Initialize some entities
    if (first_entity) {
        first_entity->id = 1;
        first_entity->x = 10.0f;
        first_entity->y = 20.0f;
        first_entity->z = 30.0f;
        first_entity->type = 1;
        strncpy(first_entity->name, "Player", sizeof(first_entity->name) - 1);
        first_entity->name[sizeof(first_entity->name) - 1] = '\0';
        
        printf("  Initialized first entity: ID=%d, pos=(%.1f,%.1f,%.1f), name='%s'\n",
               first_entity->id, first_entity->x, first_entity->y, first_entity->z, first_entity->name);
    }
    
    // Test component zone access
    printf("\nTesting component zone access:\n");
    Component* first_component = game_arena_get_components(0);
    if (first_component) {
        first_component->entity_id = 1;
        first_component->component_type = 1; // Transform component
        // Initialize transform matrix as identity
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                first_component->data.transform[i][j] = (i == j) ? 1.0f : 0.0f;
            }
        }
        printf("  Initialized first component for entity %d\n", first_component->entity_id);
    }
    
    // Test general arena allocation from the remaining space
    printf("\nTesting general arena allocation:\n");
    
    // Calculate remaining space after typed zones
    size_t typed_zones_size = game_arena_entities_SIZE + game_arena_components_SIZE + 
                             game_arena_buffers_SIZE + game_arena_strings_SIZE;
    printf("  Typed zones total: %zu bytes\n", typed_zones_size);
    printf("  Available for general allocation: %zu bytes\n", 
           game_arena_available(&game_arena) - typed_zones_size);
    
    // Allocate some general-purpose memory
    void* general_mem1 = game_arena_alloc(&game_arena, 1024, 16);
    void* general_mem2 = game_arena_alloc(&game_arena, 2048, 32);
    
    printf("  Allocated 1024 bytes at: %p\n", general_mem1);
    printf("  Allocated 2048 bytes at: %p\n", general_mem2);
    printf("  Current usage: %zu bytes\n", game_arena_used(&game_arena));
    printf("  Peak usage: %zu bytes\n", game_arena_peak_usage(&game_arena));
    
    // Test buffer zone access
    printf("\nTesting buffer zone access:\n");
    uint8_t* buffer_start = game_arena_get_buffers(0);
    uint8_t* buffer_mid = game_arena_get_buffers(512 * 1024);  // 512KB into the buffer
    printf("  Buffer start: %p\n", (void*)buffer_start);
    printf("  Buffer middle: %p\n", (void*)buffer_mid);
    
    if (buffer_start) {
        // Write some test data
        for (int i = 0; i < 10; i++) {
            buffer_start[i] = (uint8_t)(i * 42);
        }
        printf("  Wrote test pattern to buffer\n");
    }
    
    // Test string zone access
    printf("\nTesting string zone access:\n");
    char* string_start = game_arena_get_strings(0);
    char* string_mid = game_arena_get_strings(128 * 1024);  // 128KB into strings
    printf("  String area start: %p\n", (void*)string_start);
    printf("  String area middle: %p\n", (void*)string_mid);
    
    if (string_start) {
        strncpy(string_start, "Hello, ARENAC!", 256 * 1024 - 1);
        printf("  Stored string: '%.20s'\n", string_start);
    }
    
    // Reset arena and test
    printf("\nTesting arena reset:\n");
    size_t usage_before = game_arena_used(&game_arena);
    game_arena_reset(&game_arena);
    size_t usage_after = game_arena_used(&game_arena);
    
    printf("  Usage before reset: %zu bytes\n", usage_before);
    printf("  Usage after reset: %zu bytes\n", usage_after);
    printf("  Available after reset: %zu bytes\n", game_arena_available(&game_arena));
    
    printf("\nArena usage example completed successfully!\n");
    
    return 0;
}