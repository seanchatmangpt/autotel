/*  ─────────────────────────────────────────────────────────────
    cns/core/memory.h  –  Memory Management (v2.0)
    Arena allocator with zero-copy support
    ───────────────────────────────────────────────────────────── */
#ifndef CNS_CORE_MEMORY_H
#define CNS_CORE_MEMORY_H

#include "../../../include/s7t.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/*═══════════════════════════════════════════════════════════════
  Memory Arena Structure
  ═══════════════════════════════════════════════════════════════*/

typedef struct S7T_ALIGNED(64) {
    uint8_t* base;              // Base memory address
    size_t size;                // Total arena size
    size_t used;                // Current usage
    size_t peak_used;           // Peak usage
    uint32_t allocation_count;  // Number of allocations
    uint32_t flags;             // Arena flags
} cns_memory_arena_t;

/*═══════════════════════════════════════════════════════════════
  Arena Flags
  ═══════════════════════════════════════════════════════════════*/

typedef enum {
    CNS_ARENA_FLAG_NONE       = 0,
    CNS_ARENA_FLAG_ZERO       = 1 << 0,  // Zero memory on alloc
    CNS_ARENA_FLAG_ALIGN_64   = 1 << 1,  // 64-byte alignment
    CNS_ARENA_FLAG_TEMP       = 1 << 2,  // Temporary arena
    CNS_ARENA_FLAG_PROTECTED  = 1 << 3,  // Protected memory
} cns_arena_flags_t;

/*═══════════════════════════════════════════════════════════════
  Arena Management
  ═══════════════════════════════════════════════════════════════*/

// Initialize arena with pre-allocated memory
S7T_ALWAYS_INLINE void cns_arena_init(
    cns_memory_arena_t* arena,
    void* memory,
    size_t size,
    uint32_t flags
) {
    arena->base = (uint8_t*)memory;
    arena->size = size;
    arena->used = 0;
    arena->peak_used = 0;
    arena->allocation_count = 0;
    arena->flags = flags;
}

// Reset arena (free all allocations)
S7T_ALWAYS_INLINE void cns_arena_reset(cns_memory_arena_t* arena) {
    arena->used = 0;
    arena->allocation_count = 0;
}

// Allocate from arena (< 7 ticks)
S7T_ALWAYS_INLINE void* cns_arena_alloc(
    cns_memory_arena_t* arena,
    size_t size
) {
    // Align to 8 bytes by default
    size_t alignment = (arena->flags & CNS_ARENA_FLAG_ALIGN_64) ? 64 : 8;
    size_t aligned_size = (size + alignment - 1) & ~(alignment - 1);
    
    // Check available space
    if (arena->used + aligned_size > arena->size) {
        return NULL;
    }
    
    // Allocate
    void* ptr = arena->base + arena->used;
    arena->used += aligned_size;
    arena->allocation_count++;
    
    // Update peak usage
    if (arena->used > arena->peak_used) {
        arena->peak_used = arena->used;
    }
    
    // Zero memory if requested
    if (arena->flags & CNS_ARENA_FLAG_ZERO) {
        memset(ptr, 0, size);
    }
    
    return ptr;
}

// Allocate aligned memory
S7T_ALWAYS_INLINE void* cns_arena_alloc_aligned(
    cns_memory_arena_t* arena,
    size_t size,
    size_t alignment
) {
    // Calculate aligned offset
    uintptr_t current = (uintptr_t)(arena->base + arena->used);
    uintptr_t aligned = (current + alignment - 1) & ~(alignment - 1);
    size_t padding = aligned - current;
    
    // Check space
    if (arena->used + padding + size > arena->size) {
        return NULL;
    }
    
    // Allocate with padding
    arena->used += padding;
    return cns_arena_alloc(arena, size);
}

/*═══════════════════════════════════════════════════════════════
  Typed Allocation Helpers
  ═══════════════════════════════════════════════════════════════*/

// Allocate typed object
#define CNS_ARENA_NEW(arena, type) \
    ((type*)cns_arena_alloc(arena, sizeof(type)))

// Allocate typed array
#define CNS_ARENA_NEW_ARRAY(arena, type, count) \
    ((type*)cns_arena_alloc(arena, sizeof(type) * (count)))

// Allocate and zero
#define CNS_ARENA_NEW_ZERO(arena, type) \
    ((type*)cns_arena_alloc_zero(arena, sizeof(type)))

/*═══════════════════════════════════════════════════════════════
  Zero-Copy String Operations
  ═══════════════════════════════════════════════════════════════*/

// Copy string to arena (returns arena pointer)
S7T_ALWAYS_INLINE char* cns_arena_strdup(
    cns_memory_arena_t* arena,
    const char* str
) {
    size_t len = strlen(str) + 1;
    char* copy = (char*)cns_arena_alloc(arena, len);
    if (copy) {
        memcpy(copy, str, len);
    }
    return copy;
}

// Copy string with max length
S7T_ALWAYS_INLINE char* cns_arena_strndup(
    cns_memory_arena_t* arena,
    const char* str,
    size_t max_len
) {
    size_t len = strnlen(str, max_len);
    char* copy = (char*)cns_arena_alloc(arena, len + 1);
    if (copy) {
        memcpy(copy, str, len);
        copy[len] = '\0';
    }
    return copy;
}

/*═══════════════════════════════════════════════════════════════
  Arena Statistics
  ═══════════════════════════════════════════════════════════════*/

typedef struct {
    size_t total_size;          // Total arena size
    size_t used_size;           // Current usage
    size_t peak_size;           // Peak usage
    size_t available_size;      // Available space
    uint32_t allocation_count;  // Number of allocations
    double utilization;         // Usage percentage
} cns_arena_stats_t;

// Get arena statistics
S7T_ALWAYS_INLINE void cns_arena_get_stats(
    const cns_memory_arena_t* arena,
    cns_arena_stats_t* stats
) {
    stats->total_size = arena->size;
    stats->used_size = arena->used;
    stats->peak_size = arena->peak_used;
    stats->available_size = arena->size - arena->used;
    stats->allocation_count = arena->allocation_count;
    stats->utilization = (double)arena->used / arena->size * 100.0;
}

/*═══════════════════════════════════════════════════════════════
  Temporary Arena Scope
  ═══════════════════════════════════════════════════════════════*/

// Temporary arena checkpoint
typedef struct {
    size_t saved_used;
    uint32_t saved_count;
} cns_arena_checkpoint_t;

// Save arena state
S7T_ALWAYS_INLINE cns_arena_checkpoint_t cns_arena_checkpoint(
    const cns_memory_arena_t* arena
) {
    return (cns_arena_checkpoint_t){
        .saved_used = arena->used,
        .saved_count = arena->allocation_count
    };
}

// Restore arena state
S7T_ALWAYS_INLINE void cns_arena_restore(
    cns_memory_arena_t* arena,
    const cns_arena_checkpoint_t* checkpoint
) {
    arena->used = checkpoint->saved_used;
    arena->allocation_count = checkpoint->saved_count;
}

/*═══════════════════════════════════════════════════════════════
  Stack Allocator (For temporary allocations)
  ═══════════════════════════════════════════════════════════════*/

typedef struct {
    uint8_t buffer[4096];       // Stack buffer
    cns_memory_arena_t arena;   // Arena interface
} cns_stack_allocator_t;

// Initialize stack allocator
S7T_ALWAYS_INLINE void cns_stack_init(cns_stack_allocator_t* stack) {
    cns_arena_init(&stack->arena, stack->buffer, sizeof(stack->buffer), 
                   CNS_ARENA_FLAG_TEMP | CNS_ARENA_FLAG_ZERO);
}

#endif /* CNS_CORE_MEMORY_H */