#include "arena_codegen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Test the arena code generator
int main(void) {
    printf("ARENAC Arena Code Generator Test\n");
    printf("================================\n\n");
    
    // Create output file
    FILE* output = fopen("generated_arena.h", "w");
    if (!output) {
        fprintf(stderr, "Failed to create output file\n");
        return 1;
    }
    
    // Configure arena generation
    arena_codegen_config_t config = arena_codegen_default_config();
    config.total_size = 32 * 1024 * 1024;  // 32MB
    config.arena_name = "game_arena";
    config.type_name = "game_arena_t";
    config.include_debug_info = 1;
    
    // Create code generation context
    arena_codegen_ctx_t* ctx = arena_codegen_create(output, &config);
    if (!ctx) {
        fprintf(stderr, "Failed to create codegen context\n");
        fclose(output);
        return 1;
    }
    
    // Set up typed template with zones
    arena_codegen_set_template(ctx, ARENA_TEMPLATE_TYPED);
    
    // Add various typed zones
    arena_zone_config_t entity_zone = arena_codegen_default_zone("entities", "Entity", 64, 1000);
    arena_zone_config_t component_zone = arena_codegen_default_zone("components", "Component", 32, 5000);
    arena_zone_config_t buffer_zone = arena_codegen_default_zone("buffers", "uint8_t", 1, 1024 * 1024);
    arena_zone_config_t string_zone = arena_codegen_default_zone("strings", "char", 1, 256 * 1024);
    
    arena_codegen_add_zone(ctx, &entity_zone);
    arena_codegen_add_zone(ctx, &component_zone);
    arena_codegen_add_zone(ctx, &buffer_zone);
    arena_codegen_add_zone(ctx, &string_zone);
    
    // Generate complete arena code
    int result = arena_codegen_generate_complete(ctx);
    
    if (result == ARENA_CODEGEN_OK) {
        printf("✓ Successfully generated arena code\n");
        printf("  Output file: generated_arena.h\n");
        printf("  Total size: %zu bytes (%.2f MB)\n", 
               config.total_size, (double)config.total_size / (1024.0 * 1024.0));
        printf("  Zones: %zu\n", ctx->zone_count);
        printf("  Template: TYPED\n");
        
        // Calculate zone sizes
        printf("\n  Zone breakdown:\n");
        for (size_t i = 0; i < ctx->zone_count; i++) {
            const arena_zone_config_t* zone = &ctx->zones[i];
            size_t zone_size = arena_codegen_calculate_zone_size(zone);
            printf("    - %s: %zu bytes (%s[%zu])\n", 
                   zone->zone_name, zone_size, zone->type_name, zone->count);
        }
        
    } else {
        printf("✗ Failed to generate arena code: %s\n", 
               arena_codegen_error_string((arena_codegen_error_t)result));
    }
    
    // Clean up
    arena_codegen_destroy(ctx);
    fclose(output);
    
    // Test simple template generation
    printf("\n\nGenerating simple template...\n");
    
    FILE* simple_output = fopen("simple_arena.h", "w");
    if (simple_output) {
        arena_codegen_config_t simple_config = arena_codegen_default_config();
        simple_config.total_size = 8 * 1024 * 1024;  // 8MB
        simple_config.arena_name = "simple_arena";
        simple_config.include_debug_info = 0;
        
        arena_codegen_ctx_t* simple_ctx = arena_codegen_create(simple_output, &simple_config);
        if (simple_ctx) {
            arena_codegen_set_template(simple_ctx, ARENA_TEMPLATE_SIMPLE);
            
            if (arena_codegen_generate_complete(simple_ctx) == ARENA_CODEGEN_OK) {
                printf("✓ Successfully generated simple arena code\n");
                printf("  Output file: simple_arena.h\n");
                printf("  Size: %zu bytes (%.2f MB)\n", 
                       simple_config.total_size, (double)simple_config.total_size / (1024.0 * 1024.0));
            } else {
                printf("✗ Failed to generate simple arena code\n");
            }
            
            arena_codegen_destroy(simple_ctx);
        }
        fclose(simple_output);
    }
    
    printf("\nArena code generation test completed.\n");
    
    return result == ARENA_CODEGEN_OK ? 0 : 1;
}