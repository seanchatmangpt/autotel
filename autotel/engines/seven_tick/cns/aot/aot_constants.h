#ifndef AOT_CONSTANTS_H
#define AOT_CONSTANTS_H

/**
 * AOT Static Arena Size Constants
 * 
 * Pre-calculated memory arena sizes for common CNS graph configurations.
 * These constants can be used for compile-time memory allocation.
 */

#include <stddef.h>

// Static arena sizes for different graph scales (in bytes)
#define AOT_ARENA_SMALL    (512 * 1024)        // 512KB - up to 1K nodes
#define AOT_ARENA_MEDIUM   (2 * 1024 * 1024)   // 2MB - up to 10K nodes  
#define AOT_ARENA_LARGE    (16 * 1024 * 1024)  // 16MB - up to 100K nodes
#define AOT_ARENA_XLARGE   (64 * 1024 * 1024)  // 64MB - up to 1M nodes

// 7-tick optimized sizes (power of 2 aligned for cache efficiency)
#define AOT_7TICK_SMALL    (256 * 1024)        // 256KB
#define AOT_7TICK_MEDIUM   (1024 * 1024)       // 1MB
#define AOT_7TICK_LARGE    (8 * 1024 * 1024)   // 8MB
#define AOT_7TICK_XLARGE   (32 * 1024 * 1024)  // 32MB

// Component size estimates for typical CNS ontologies
#define AOT_TYPICAL_NODE_COUNT_SMALL     1000
#define AOT_TYPICAL_EDGE_COUNT_SMALL     5000
#define AOT_TYPICAL_PROPERTY_COUNT_SMALL 3000

#define AOT_TYPICAL_NODE_COUNT_MEDIUM    10000
#define AOT_TYPICAL_EDGE_COUNT_MEDIUM    50000
#define AOT_TYPICAL_PROPERTY_COUNT_MEDIUM 30000

#define AOT_TYPICAL_NODE_COUNT_LARGE     100000
#define AOT_TYPICAL_EDGE_COUNT_LARGE     500000
#define AOT_TYPICAL_PROPERTY_COUNT_LARGE 300000

// Default arena selection macro
#define AOT_SELECT_ARENA_SIZE(node_count) \
    ((node_count) <= 1000 ? AOT_7TICK_SMALL : \
     (node_count) <= 10000 ? AOT_7TICK_MEDIUM : \
     (node_count) <= 100000 ? AOT_7TICK_LARGE : \
     AOT_7TICK_XLARGE)

// Static arena declaration helpers
#define AOT_DECLARE_SMALL_ARENA()  static char aot_arena[AOT_7TICK_SMALL]
#define AOT_DECLARE_MEDIUM_ARENA() static char aot_arena[AOT_7TICK_MEDIUM] 
#define AOT_DECLARE_LARGE_ARENA()  static char aot_arena[AOT_7TICK_LARGE]
#define AOT_DECLARE_XLARGE_ARENA() static char aot_arena[AOT_7TICK_XLARGE]

// Dynamic arena declaration
#define AOT_DECLARE_DYNAMIC_ARENA(size) static char aot_arena[size]

#endif // AOT_CONSTANTS_H