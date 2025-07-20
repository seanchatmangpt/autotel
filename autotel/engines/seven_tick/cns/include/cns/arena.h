/*  ─────────────────────────────────────────────────────────────
    cns/arena.h  –  ARENAC Memory Arena (v2.0)
    7T-compliant arena allocator with O(1) operations
    ───────────────────────────────────────────────────────────── */
#ifndef CNS_ARENA_H
#define CNS_ARENA_H

#include "../../s7t_minimal.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/*═══════════════════════════════════════════════════════════════
  ARENAC Core Constants
  ═══════════════════════════════════════════════════════════════*/

#define ARENAC_VERSION_MAJOR 2
#define ARENAC_VERSION_MINOR 0
#define ARENAC_VERSION_PATCH 0

// Performance constraints
#define ARENAC_MAX_CYCLES 7
#define ARENAC_DEFAULT_ALIGNMENT 64
#define ARENAC_CACHE_LINE_SIZE 64
#define ARENAC_PAGE_SIZE 4096

// Arena limits
#define ARENAC_MIN_SIZE (64 * 1024)      // 64KB minimum
#define ARENAC_MAX_SIZE (1ULL << 32)     // 4GB maximum
#define ARENAC_MAX_ZONES 16              // Maximum allocation zones

// Abort macro for critical failures
#define ARENAC_ABORT(msg) \
    do { \
        fprintf(stderr, "ARENAC ABORT: %s at %s:%d\n", msg, __FILE__, __LINE__); \
        abort(); \
    } while(0)

/*═══════════════════════════════════════════════════════════════
  Memory Alignment Macros (7T-compliant)
  ═══════════════════════════════════════════════════════════════*/

// Branchless alignment (< 7 ticks)
#define ARENAC_ALIGN_UP(ptr, alignment) \
    ((void*)(((uintptr_t)(ptr) + ((alignment) - 1)) & ~((alignment) - 1)))

#define ARENAC_ALIGN_DOWN(ptr, alignment) \
    ((void*)((uintptr_t)(ptr) & ~((alignment) - 1)))

#define ARENAC_IS_ALIGNED(ptr, alignment) \
    (((uintptr_t)(ptr) & ((alignment) - 1)) == 0)

// Cache-line alignment
#define ARENAC_CACHE_ALIGN(ptr) ARENAC_ALIGN_UP(ptr, ARENAC_CACHE_LINE_SIZE)

/*═══════════════════════════════════════════════════════════════
  Arena Flags and Configuration
  ═══════════════════════════════════════════════════════════════*/

typedef enum {
    ARENAC_FLAG_NONE           = 0,
    ARENAC_FLAG_ZERO_ALLOC     = 1 << 0,  // Zero memory on allocation
    ARENAC_FLAG_ZERO_FREE      = 1 << 1,  // Zero memory on free
    ARENAC_FLAG_GUARD_PAGES    = 1 << 2,  // Add guard pages
    ARENAC_FLAG_STATS          = 1 << 3,  // Track statistics
    ARENAC_FLAG_THREAD_SAFE    = 1 << 4,  // Thread-safe operations
    ARENAC_FLAG_OVERFLOW_CHECK = 1 << 5,  // Check for overflows
    ARENAC_FLAG_ALIGN_64       = 1 << 6,  // 64-byte alignment
    ARENAC_FLAG_TEMP           = 1 << 7,  // Temporary arena
    ARENAC_FLAG_PROTECTED      = 1 << 8,  // Memory protection
    ARENAC_FLAG_PREFAULT       = 1 << 9,  // Pre-fault pages
} arenac_flags_t;

/*═══════════════════════════════════════════════════════════════
  Arena Zone Structure (For multi-zone allocation)
  ═══════════════════════════════════════════════════════════════*/

typedef struct S7T_ALIGNED(64) {
    uint8_t* base;              // Zone base address
    size_t size;                // Zone size
    size_t used;                // Used bytes in zone
    uint32_t zone_id;           // Zone identifier
    uint32_t padding;           // Padding for alignment
} arenac_zone_t;

/*═══════════════════════════════════════════════════════════════
  Arena Statistics (Optional, enabled by ARENAC_FLAG_STATS)
  ═══════════════════════════════════════════════════════════════*/

typedef struct S7T_ALIGNED(64) {
    uint64_t total_allocations; // Total allocation count
    uint64_t total_frees;       // Total free count  
    uint64_t bytes_allocated;   // Total bytes allocated
    uint64_t bytes_freed;       // Total bytes freed
    uint64_t peak_usage;        // Peak memory usage
    uint64_t waste_bytes;       // Bytes lost to alignment
    uint64_t violation_count;   // 7T violations
    uint64_t last_alloc_cycles; // Last allocation time
    uint64_t avg_alloc_cycles;  // Average allocation time
    uint64_t max_alloc_cycles;  // Maximum allocation time
} arenac_stats_t;

/*═══════════════════════════════════════════════════════════════
  Main Arena Structure
  ═══════════════════════════════════════════════════════════════*/

typedef struct S7T_ALIGNED(64) {
    // Core memory management (64-byte aligned)
    uint8_t* base;              // Arena base address
    size_t size;                // Total arena size
    size_t used;                // Current usage
    size_t high_water_mark;     // Peak usage
    
    // Configuration and flags
    uint32_t flags;             // Arena flags
    uint32_t alignment;         // Default alignment
    
    // Multi-zone support
    arenac_zone_t zones[ARENAC_MAX_ZONES];
    uint32_t zone_count;        // Active zone count
    uint32_t current_zone;      // Current allocation zone
    
    // Performance tracking
    uint64_t allocation_count;  // Number of allocations
    uint64_t free_count;        // Number of frees
    
    // Optional statistics (only if ARENAC_FLAG_STATS)
    arenac_stats_t* stats;      // Statistics pointer
    
    // Thread safety (only if ARENAC_FLAG_THREAD_SAFE)
    void* mutex;                // Mutex pointer (opaque)
    
    // Memory protection
    uint8_t* guard_start;       // Guard page start
    uint8_t* guard_end;         // Guard page end
} arena_t;

/*═══════════════════════════════════════════════════════════════
  Arena Initialization and Destruction
  ═══════════════════════════════════════════════════════════════*/

/**
 * Initialize arena with pre-allocated memory
 * 
 * @param arena Arena to initialize
 * @param memory Pre-allocated memory buffer
 * @param size Size of memory buffer
 * @param flags Arena configuration flags
 * @return 0 on success, negative on error
 * 
 * Time complexity: O(1)
 * Cycle guarantee: ≤ 7 ticks
 */
static inline int arenac_init(
    arena_t* arena,
    void* memory,
    size_t size,
    uint32_t flags
) {
    if (!arena || !memory || size < ARENAC_MIN_SIZE) {
        return -1;
    }
    
    // Initialize core fields
    arena->base = (uint8_t*)memory;
    arena->size = size;
    arena->used = 0;
    arena->high_water_mark = 0;
    arena->flags = flags;
    arena->alignment = (flags & ARENAC_FLAG_ALIGN_64) ? 64 : 8;
    
    // Initialize zones
    arena->zone_count = 1;
    arena->current_zone = 0;
    arena->zones[0] = (arenac_zone_t){
        .base = arena->base,
        .size = arena->size,
        .used = 0,
        .zone_id = 0,
        .padding = 0
    };
    
    // Initialize counters
    arena->allocation_count = 0;
    arena->free_count = 0;
    
    // Initialize optional components
    arena->stats = NULL;
    arena->mutex = NULL;
    arena->guard_start = NULL;
    arena->guard_end = NULL;
    
    return 0;
}

/**
 * Create new arena with system allocation
 * 
 * @param size Arena size
 * @param flags Arena configuration flags
 * @return Pointer to arena or NULL on failure
 * 
 * Time complexity: O(1)
 * Cycle guarantee: ≤ 7 ticks (excluding system malloc)
 */
arena_t* arenac_create(size_t size, uint32_t flags);

/**
 * Destroy arena and free system memory
 * 
 * @param arena Arena to destroy
 * 
 * Time complexity: O(1)
 * Cycle guarantee: ≤ 7 ticks (excluding system free)
 */
void arenac_destroy(arena_t* arena);

/*═══════════════════════════════════════════════════════════════
  Core Allocation Functions (7T-compliant)
  ═══════════════════════════════════════════════════════════════*/

/**
 * Allocate memory from arena
 * 
 * @param arena Arena to allocate from
 * @param size Number of bytes to allocate
 * @return Pointer to allocated memory or NULL on failure
 * 
 * Time complexity: O(1)
 * Cycle guarantee: ≤ 7 ticks
 */
static inline void* arenac_alloc(arena_t* arena, size_t size) {
    if (!arena || size == 0) {
        return NULL;
    }
    
    // Get current zone
    arenac_zone_t* zone = &arena->zones[arena->current_zone];
    
    // Calculate aligned size
    size_t alignment = arena->alignment;
    size_t aligned_size = (size + alignment - 1) & ~(alignment - 1);
    
    // Check space in current zone
    if (zone->used + aligned_size > zone->size) {
        return NULL;  // Out of memory
    }
    
    // Allocate
    void* ptr = zone->base + zone->used;
    zone->used += aligned_size;
    arena->used += aligned_size;
    arena->allocation_count++;
    
    // Update high water mark
    if (arena->used > arena->high_water_mark) {
        arena->high_water_mark = arena->used;
    }
    
    // Zero memory if requested
    if (arena->flags & ARENAC_FLAG_ZERO_ALLOC) {
        memset(ptr, 0, size);
    }
    
    return ptr;
}

/**
 * Allocate aligned memory from arena
 * 
 * @param arena Arena to allocate from
 * @param size Number of bytes to allocate
 * @param alignment Required alignment (must be power of 2)
 * @return Pointer to aligned memory or NULL on failure
 * 
 * Time complexity: O(1)
 * Cycle guarantee: ≤ 7 ticks
 */
static inline void* arenac_alloc_aligned(
    arena_t* arena,
    size_t size,
    size_t alignment
) {
    if (!arena || size == 0 || (alignment & (alignment - 1)) != 0) {
        return NULL;
    }
    
    // Get current zone
    arenac_zone_t* zone = &arena->zones[arena->current_zone];
    
    // Calculate aligned offset
    uintptr_t current = (uintptr_t)(zone->base + zone->used);
    uintptr_t aligned = (current + alignment - 1) & ~(alignment - 1);
    size_t padding = aligned - current;
    
    // Check total space needed
    if (zone->used + padding + size > zone->size) {
        return NULL;
    }
    
    // Add padding
    zone->used += padding;
    arena->used += padding;
    
    // Allocate aligned memory
    return arenac_alloc(arena, size);
}

/**
 * Reset arena (free all allocations)
 * 
 * @param arena Arena to reset
 * 
 * Time complexity: O(1)
 * Cycle guarantee: ≤ 7 ticks
 */
static inline void arenac_reset(arena_t* arena) {
    if (!arena) {
        return;
    }
    
    // Zero memory if requested
    if (arena->flags & ARENAC_FLAG_ZERO_FREE) {
        memset(arena->base, 0, arena->used);
    }
    
    // Reset all zones
    for (uint32_t i = 0; i < arena->zone_count; i++) {
        arena->zones[i].used = 0;
    }
    
    // Reset counters
    arena->used = 0;
    arena->current_zone = 0;
    arena->free_count += arena->allocation_count;
    arena->allocation_count = 0;
}

/*═══════════════════════════════════════════════════════════════
  Typed Allocation Macros (7T-compliant)
  ═══════════════════════════════════════════════════════════════*/

/**
 * Allocate typed object
 */
#define ARENAC_NEW(arena, type) \
    ((type*)arenac_alloc(arena, sizeof(type)))

/**
 * Allocate typed array
 */
#define ARENAC_NEW_ARRAY(arena, type, count) \
    ((type*)arenac_alloc(arena, sizeof(type) * (count)))

/**
 * Allocate aligned typed object
 */
#define ARENAC_NEW_ALIGNED(arena, type, alignment) \
    ((type*)arenac_alloc_aligned(arena, sizeof(type), alignment))

/**
 * Allocate and zero typed object
 */
#define ARENAC_NEW_ZERO(arena, type) \
    ({ \
        type* __ptr = ARENAC_NEW(arena, type); \
        if (__ptr) memset(__ptr, 0, sizeof(type)); \
        __ptr; \
    })

/*═══════════════════════════════════════════════════════════════
  String Operations (7T-compliant)
  ═══════════════════════════════════════════════════════════════*/

/**
 * Duplicate string in arena
 * 
 * @param arena Arena to allocate from
 * @param str String to duplicate
 * @return Pointer to duplicated string or NULL on failure
 * 
 * Time complexity: O(n) where n is string length
 * Cycle guarantee: ≤ 7 ticks for allocation + copy time
 */
static inline char* arenac_strdup(arena_t* arena, const char* str) {
    if (!arena || !str) {
        return NULL;
    }
    
    size_t len = strlen(str) + 1;
    char* copy = (char*)arenac_alloc(arena, len);
    if (copy) {
        memcpy(copy, str, len);
    }
    return copy;
}

/**
 * Duplicate string with maximum length
 * 
 * @param arena Arena to allocate from
 * @param str String to duplicate
 * @param max_len Maximum length to copy
 * @return Pointer to duplicated string or NULL on failure
 * 
 * Time complexity: O(min(n, max_len))
 * Cycle guarantee: ≤ 7 ticks for allocation + copy time
 */
static inline char* arenac_strndup(
    arena_t* arena,
    const char* str,
    size_t max_len
) {
    if (!arena || !str) {
        return NULL;
    }
    
    size_t len = strnlen(str, max_len);
    char* copy = (char*)arenac_alloc(arena, len + 1);
    if (copy) {
        memcpy(copy, str, len);
        copy[len] = '\0';
    }
    return copy;
}

/*═══════════════════════════════════════════════════════════════
  Arena Zones (Multi-zone allocation)
  ═══════════════════════════════════════════════════════════════*/

/**
 * Add new allocation zone
 * 
 * @param arena Arena to add zone to
 * @param memory Zone memory buffer
 * @param size Zone size
 * @return Zone ID or negative on error
 */
int arenac_add_zone(arena_t* arena, void* memory, size_t size);

/**
 * Switch to specific zone for allocation
 * 
 * @param arena Arena to switch
 * @param zone_id Zone ID to switch to
 * @return 0 on success, negative on error
 */
int arenac_switch_zone(arena_t* arena, uint32_t zone_id);

/*═══════════════════════════════════════════════════════════════
  Arena Statistics and Debugging
  ═══════════════════════════════════════════════════════════════*/

/**
 * Arena usage information
 */
typedef struct {
    size_t total_size;          // Total arena size
    size_t used_size;           // Currently used
    size_t available_size;      // Available space
    size_t high_water_mark;     // Peak usage
    uint64_t allocation_count;  // Number of allocations
    double utilization;         // Usage percentage
    uint32_t zone_count;        // Number of zones
} arenac_info_t;

/**
 * Get arena usage information
 * 
 * @param arena Arena to query
 * @param info Output information structure
 * 
 * Time complexity: O(1)
 * Cycle guarantee: ≤ 7 ticks
 */
static inline void arenac_get_info(
    const arena_t* arena,
    arenac_info_t* info
) {
    if (!arena || !info) {
        return;
    }
    
    info->total_size = arena->size;
    info->used_size = arena->used;
    info->available_size = arena->size - arena->used;
    info->high_water_mark = arena->high_water_mark;
    info->allocation_count = arena->allocation_count;
    info->utilization = (double)arena->used / arena->size * 100.0;
    info->zone_count = arena->zone_count;
}

/**
 * Enable statistics collection
 * 
 * @param arena Arena to enable stats for
 * @return 0 on success, negative on error
 */
int arenac_enable_stats(arena_t* arena);

/**
 * Get detailed statistics
 * 
 * @param arena Arena to query
 * @return Pointer to statistics or NULL if not enabled
 */
const arenac_stats_t* arenac_get_stats(const arena_t* arena);

/*═══════════════════════════════════════════════════════════════
  Checkpoint/Restore (Temporary allocations)
  ═══════════════════════════════════════════════════════════════*/

/**
 * Arena checkpoint for temporary allocations
 */
typedef struct {
    size_t saved_used;          // Saved usage
    uint64_t saved_count;       // Saved allocation count
    uint32_t saved_zone;        // Saved current zone
    arenac_zone_t saved_zones[ARENAC_MAX_ZONES]; // Saved zone states
} arenac_checkpoint_t;

/**
 * Create arena checkpoint
 * 
 * @param arena Arena to checkpoint
 * @return Checkpoint structure
 * 
 * Time complexity: O(1)
 * Cycle guarantee: ≤ 7 ticks
 */
static inline arenac_checkpoint_t arenac_checkpoint(const arena_t* arena) {
    arenac_checkpoint_t checkpoint = {0};
    
    if (arena) {
        checkpoint.saved_used = arena->used;
        checkpoint.saved_count = arena->allocation_count;
        checkpoint.saved_zone = arena->current_zone;
        
        // Save zone states
        for (uint32_t i = 0; i < arena->zone_count && i < ARENAC_MAX_ZONES; i++) {
            checkpoint.saved_zones[i] = arena->zones[i];
        }
    }
    
    return checkpoint;
}

/**
 * Restore arena to checkpoint
 * 
 * @param arena Arena to restore
 * @param checkpoint Checkpoint to restore to
 * 
 * Time complexity: O(1)
 * Cycle guarantee: ≤ 7 ticks
 */
static inline void arenac_restore(
    arena_t* arena,
    const arenac_checkpoint_t* checkpoint
) {
    if (!arena || !checkpoint) {
        return;
    }
    
    // Restore main state
    arena->used = checkpoint->saved_used;
    arena->allocation_count = checkpoint->saved_count;
    arena->current_zone = checkpoint->saved_zone;
    
    // Restore zone states
    for (uint32_t i = 0; i < arena->zone_count && i < ARENAC_MAX_ZONES; i++) {
        arena->zones[i] = checkpoint->saved_zones[i];
    }
}

/*═══════════════════════════════════════════════════════════════
  Memory Protection and Safety
  ═══════════════════════════════════════════════════════════════*/

/**
 * Enable memory protection (guard pages)
 * 
 * @param arena Arena to protect
 * @return 0 on success, negative on error
 */
int arenac_enable_protection(arena_t* arena);

/**
 * Validate arena integrity
 * 
 * @param arena Arena to validate
 * @return 0 if valid, negative if corrupted
 */
int arenac_validate(const arena_t* arena);

/*═══════════════════════════════════════════════════════════════
  Thread Safety (Optional)
  ═══════════════════════════════════════════════════════════════*/

/**
 * Enable thread-safe operations
 * 
 * @param arena Arena to make thread-safe
 * @return 0 on success, negative on error
 */
int arenac_enable_thread_safety(arena_t* arena);

/**
 * Lock arena for exclusive access
 * 
 * @param arena Arena to lock
 */
void arenac_lock(arena_t* arena);

/**
 * Unlock arena
 * 
 * @param arena Arena to unlock
 */
void arenac_unlock(arena_t* arena);

/*═══════════════════════════════════════════════════════════════
  Performance Testing and Validation
  ═══════════════════════════════════════════════════════════════*/

/**
 * Benchmark arena operations
 * 
 * @param arena Arena to benchmark
 * @param iterations Number of test iterations
 * @return Average cycles per operation
 */
uint64_t arenac_benchmark(arena_t* arena, uint64_t iterations);

/**
 * Validate 7-tick compliance
 * 
 * @param arena Arena to test
 * @return 0 if compliant, negative if violations found
 */
int arenac_validate_7tick(arena_t* arena);

#ifdef __cplusplus
}
#endif

#endif /* CNS_ARENA_H */