#ifndef AOT_CALCULATOR_H
#define AOT_CALCULATOR_H

#include <stddef.h>
#include <stdint.h>

/**
 * AOT Memory Calculator for CNS Binary Materializer
 * 
 * Calculates total memory requirements for Ahead-of-Time allocation
 * of graph structures and components.
 */

// Component memory requirements (in bytes)
typedef struct {
    size_t node_count;
    size_t edge_count;
    size_t property_count;
    size_t string_data_size;
    size_t index_overhead;
} aot_component_sizes_t;

// Memory layout configuration
typedef struct {
    size_t alignment;           // Memory alignment requirement (default: 8)
    size_t safety_margin;       // Extra bytes for safety (default: 1024)
    uint8_t enable_padding;     // Add padding between sections (default: 1)
} aot_memory_config_t;

// Calculated memory requirements
typedef struct {
    size_t total_size;          // Total memory arena size
    size_t node_section_size;   // Memory for nodes
    size_t edge_section_size;   // Memory for edges
    size_t property_section_size; // Memory for properties
    size_t string_section_size; // Memory for string data
    size_t index_section_size;  // Memory for indices
    size_t padding_overhead;    // Total padding bytes
} aot_memory_layout_t;

// Default configuration
#define AOT_DEFAULT_ALIGNMENT    8
#define AOT_DEFAULT_SAFETY_MARGIN 1024
#define AOT_NODE_SIZE           64    // Base size per node
#define AOT_EDGE_SIZE           32    // Base size per edge
#define AOT_PROPERTY_SIZE       24    // Base size per property
#define AOT_INDEX_OVERHEAD_RATIO 0.15 // 15% overhead for indices

/**
 * Calculate total memory requirements for AOT allocation
 * 
 * @param components Component size requirements
 * @param config Memory layout configuration (NULL for defaults)
 * @param layout Output memory layout (can be NULL)
 * @return Total memory size in bytes, or 0 on error
 */
size_t aot_calculate_memory(const aot_component_sizes_t* components,
                           const aot_memory_config_t* config,
                           aot_memory_layout_t* layout);

/**
 * Generate static arena size constant for compilation
 * 
 * @param components Component size requirements
 * @param config Memory layout configuration (NULL for defaults)
 * @return Static arena size constant
 */
size_t aot_generate_static_arena_size(const aot_component_sizes_t* components,
                                     const aot_memory_config_t* config);

/**
 * Initialize default memory configuration
 * 
 * @param config Configuration structure to initialize
 */
void aot_init_default_config(aot_memory_config_t* config);

/**
 * Validate component sizes for reasonableness
 * 
 * @param components Component sizes to validate
 * @return 1 if valid, 0 if invalid
 */
int aot_validate_components(const aot_component_sizes_t* components);

/**
 * Calculate aligned size for given size and alignment
 * 
 * @param size Original size
 * @param alignment Alignment requirement
 * @return Aligned size
 */
static inline size_t aot_align_size(size_t size, size_t alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}

/**
 * Example calculations for common graph sizes
 */
size_t aot_calculate_for_small_graph(void);
size_t aot_calculate_for_medium_graph(void);
size_t aot_calculate_for_large_graph(void);

/**
 * Debug utility for printing memory layout
 */
void aot_print_memory_layout(const aot_memory_layout_t* layout, 
                            const aot_component_sizes_t* components);

#endif // AOT_CALCULATOR_H