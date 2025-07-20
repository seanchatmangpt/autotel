#include "aot_calculator.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/**
 * AOT Memory Calculator Implementation
 * 
 * 80/20 approach: Focus on core calculation logic without complex
 * SHACL/TTL parsing. Direct memory requirement calculation.
 */

// Maximum reasonable limits for validation
#define MAX_NODES       10000000    // 10M nodes
#define MAX_EDGES       50000000    // 50M edges  
#define MAX_PROPERTIES  20000000    // 20M properties
#define MAX_STRING_DATA (1024*1024*1024) // 1GB string data

void aot_init_default_config(aot_memory_config_t* config) {
    if (!config) return;
    
    config->alignment = AOT_DEFAULT_ALIGNMENT;
    config->safety_margin = AOT_DEFAULT_SAFETY_MARGIN;
    config->enable_padding = 1;
}

int aot_validate_components(const aot_component_sizes_t* components) {
    if (!components) return 0;
    
    // Check for reasonable limits
    if (components->node_count > MAX_NODES ||
        components->edge_count > MAX_EDGES ||
        components->property_count > MAX_PROPERTIES ||
        components->string_data_size > MAX_STRING_DATA) {
        return 0;
    }
    
    // Basic sanity checks
    if (components->edge_count > components->node_count * components->node_count) {
        return 0; // More edges than possible in complete graph
    }
    
    return 1;
}

static size_t calculate_section_size(size_t count, size_t item_size, 
                                   const aot_memory_config_t* config) {
    size_t base_size = count * item_size;
    if (config && config->enable_padding) {
        return aot_align_size(base_size, config->alignment);
    }
    return base_size;
}

size_t aot_calculate_memory(const aot_component_sizes_t* components,
                           const aot_memory_config_t* config,
                           aot_memory_layout_t* layout) {
    if (!components || !aot_validate_components(components)) {
        return 0;
    }
    
    // Use default config if none provided
    aot_memory_config_t default_config;
    if (!config) {
        aot_init_default_config(&default_config);
        config = &default_config;
    }
    
    aot_memory_layout_t temp_layout = {0};
    aot_memory_layout_t* working_layout = layout ? layout : &temp_layout;
    
    // Calculate individual section sizes
    working_layout->node_section_size = calculate_section_size(
        components->node_count, AOT_NODE_SIZE, config);
    
    working_layout->edge_section_size = calculate_section_size(
        components->edge_count, AOT_EDGE_SIZE, config);
    
    working_layout->property_section_size = calculate_section_size(
        components->property_count, AOT_PROPERTY_SIZE, config);
    
    working_layout->string_section_size = aot_align_size(
        components->string_data_size, config->alignment);
    
    // Calculate index overhead (15% of main data)
    size_t main_data_size = working_layout->node_section_size + 
                           working_layout->edge_section_size + 
                           working_layout->property_section_size;
    working_layout->index_section_size = aot_align_size(
        (size_t)(main_data_size * AOT_INDEX_OVERHEAD_RATIO), config->alignment);
    
    // Calculate padding overhead if enabled
    working_layout->padding_overhead = 0;
    if (config->enable_padding) {
        // Estimate padding between sections
        size_t sections = 5; // node, edge, property, string, index
        working_layout->padding_overhead = sections * config->alignment;
    }
    
    // Calculate total size
    working_layout->total_size = 
        working_layout->node_section_size +
        working_layout->edge_section_size +
        working_layout->property_section_size +
        working_layout->string_section_size +
        working_layout->index_section_size +
        working_layout->padding_overhead +
        config->safety_margin;
    
    // Align total size
    working_layout->total_size = aot_align_size(
        working_layout->total_size, config->alignment);
    
    return working_layout->total_size;
}

size_t aot_generate_static_arena_size(const aot_component_sizes_t* components,
                                     const aot_memory_config_t* config) {
    // Generate a static size with extra safety margin for compilation
    aot_memory_config_t static_config;
    if (config) {
        static_config = *config;
    } else {
        aot_init_default_config(&static_config);
    }
    
    // Increase safety margin for static allocation
    static_config.safety_margin *= 2;
    
    size_t calculated_size = aot_calculate_memory(components, &static_config, NULL);
    
    // Round up to next power of 2 for better memory management
    size_t arena_size = 1;
    while (arena_size < calculated_size) {
        arena_size <<= 1;
    }
    
    return arena_size;
}

// Utility function for debugging/monitoring
void aot_print_memory_layout(const aot_memory_layout_t* layout, 
                            const aot_component_sizes_t* components) {
    if (!layout || !components) return;
    
    printf("AOT Memory Layout:\n");
    printf("  Nodes:      %zu bytes (%zu items)\n", 
           layout->node_section_size, components->node_count);
    printf("  Edges:      %zu bytes (%zu items)\n", 
           layout->edge_section_size, components->edge_count);
    printf("  Properties: %zu bytes (%zu items)\n", 
           layout->property_section_size, components->property_count);
    printf("  Strings:    %zu bytes\n", layout->string_section_size);
    printf("  Indices:    %zu bytes\n", layout->index_section_size);
    printf("  Padding:    %zu bytes\n", layout->padding_overhead);
    printf("  TOTAL:      %zu bytes (%.2f MB)\n", 
           layout->total_size, layout->total_size / (1024.0 * 1024.0));
}

// Example usage for common CNS graph sizes
size_t aot_calculate_for_small_graph(void) {
    aot_component_sizes_t small = {
        .node_count = 1000,
        .edge_count = 5000,
        .property_count = 3000,
        .string_data_size = 64 * 1024,  // 64KB
        .index_overhead = 0
    };
    
    return aot_generate_static_arena_size(&small, NULL);
}

size_t aot_calculate_for_medium_graph(void) {
    aot_component_sizes_t medium = {
        .node_count = 10000,
        .edge_count = 50000,
        .property_count = 30000,
        .string_data_size = 512 * 1024, // 512KB
        .index_overhead = 0
    };
    
    return aot_generate_static_arena_size(&medium, NULL);
}

size_t aot_calculate_for_large_graph(void) {
    aot_component_sizes_t large = {
        .node_count = 100000,
        .edge_count = 500000,
        .property_count = 300000,
        .string_data_size = 2 * 1024 * 1024, // 2MB
        .index_overhead = 0
    };
    
    return aot_generate_static_arena_size(&large, NULL);
}