#ifndef ARENA_CODEGEN_H
#define ARENA_CODEGEN_H

#include <stddef.h>
#include <stdio.h>
#include <stdint.h>

// Arena code generation configuration
typedef struct {
    size_t total_size;           // Total arena size in bytes
    size_t alignment;            // Memory alignment (default: 16)
    const char* arena_name;      // Generated arena variable name
    const char* type_name;       // Generated arena type name
    int use_static_storage;      // Use static vs dynamic allocation
    int include_guards;          // Include header guards
    int include_debug_info;      // Include debugging information
} arena_codegen_config_t;

// Template types for different arena patterns
typedef enum {
    ARENA_TEMPLATE_SIMPLE,       // Basic single arena
    ARENA_TEMPLATE_HIERARCHICAL, // Parent-child arena structure
    ARENA_TEMPLATE_POOLED,       // Multiple fixed-size pools
    ARENA_TEMPLATE_TYPED,        // Type-aware allocation zones
    ARENA_TEMPLATE_SLAB          // Slab allocator pattern
} arena_template_type_t;

// Arena zone configuration for typed templates
typedef struct {
    const char* zone_name;
    const char* type_name;
    size_t type_size;
    size_t count;
    size_t alignment;
} arena_zone_config_t;

// Main code generation context
typedef struct {
    FILE* output;
    arena_codegen_config_t config;
    arena_template_type_t template_type;
    
    // Template-specific data
    arena_zone_config_t* zones;
    size_t zone_count;
    
    // Generation state
    int indent_level;
    char* buffer;
    size_t buffer_size;
    size_t buffer_pos;
} arena_codegen_ctx_t;

// Core API functions
arena_codegen_ctx_t* arena_codegen_create(FILE* output, const arena_codegen_config_t* config);
void arena_codegen_destroy(arena_codegen_ctx_t* ctx);

// Template configuration
int arena_codegen_set_template(arena_codegen_ctx_t* ctx, arena_template_type_t type);
int arena_codegen_add_zone(arena_codegen_ctx_t* ctx, const arena_zone_config_t* zone);

// Code generation functions
int arena_codegen_generate_header(arena_codegen_ctx_t* ctx);
int arena_codegen_generate_declarations(arena_codegen_ctx_t* ctx);
int arena_codegen_generate_initialization(arena_codegen_ctx_t* ctx);
int arena_codegen_generate_accessors(arena_codegen_ctx_t* ctx);
int arena_codegen_generate_complete(arena_codegen_ctx_t* ctx);

// Utility functions
void arena_codegen_emit(arena_codegen_ctx_t* ctx, const char* format, ...);
void arena_codegen_emit_line(arena_codegen_ctx_t* ctx, const char* format, ...);
void arena_codegen_indent(arena_codegen_ctx_t* ctx);
void arena_codegen_dedent(arena_codegen_ctx_t* ctx);

// Size calculation helpers
size_t arena_codegen_calculate_zone_size(const arena_zone_config_t* zone);
size_t arena_codegen_calculate_total_size(const arena_zone_config_t* zones, size_t count);
size_t arena_codegen_align_size(size_t size, size_t alignment);

// Template-specific generators
int arena_codegen_generate_simple_template(arena_codegen_ctx_t* ctx);
int arena_codegen_generate_hierarchical_template(arena_codegen_ctx_t* ctx);
int arena_codegen_generate_pooled_template(arena_codegen_ctx_t* ctx);
int arena_codegen_generate_typed_template(arena_codegen_ctx_t* ctx);
int arena_codegen_generate_slab_template(arena_codegen_ctx_t* ctx);

// Default configurations
arena_codegen_config_t arena_codegen_default_config(void);
arena_zone_config_t arena_codegen_default_zone(const char* name, const char* type, size_t size, size_t count);

// Validation functions
int arena_codegen_validate_config(const arena_codegen_config_t* config);
int arena_codegen_validate_zone(const arena_zone_config_t* zone);

// Error handling
typedef enum {
    ARENA_CODEGEN_OK = 0,
    ARENA_CODEGEN_ERROR_NULL_POINTER,
    ARENA_CODEGEN_ERROR_INVALID_CONFIG,
    ARENA_CODEGEN_ERROR_INVALID_ZONE,
    ARENA_CODEGEN_ERROR_OUTPUT_FAILED,
    ARENA_CODEGEN_ERROR_BUFFER_OVERFLOW,
    ARENA_CODEGEN_ERROR_UNSUPPORTED_TEMPLATE
} arena_codegen_error_t;

const char* arena_codegen_error_string(arena_codegen_error_t error);

#endif // ARENA_CODEGEN_H