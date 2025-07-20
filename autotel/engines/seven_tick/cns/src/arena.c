/*  ─────────────────────────────────────────────────────────────
    src/arena.c  –  ARENAC Implementation (v2.0)
    7T-compliant arena allocator implementation
    ───────────────────────────────────────────────────────────── */

#define _DEFAULT_SOURCE
#define _BSD_SOURCE

#include "cns/arena.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/mman.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

// Portable MAP_ANONYMOUS definition
#ifndef MAP_ANONYMOUS
  #ifdef MAP_ANON
    #define MAP_ANONYMOUS MAP_ANON
  #else
    #define MAP_ANONYMOUS 0x20
  #endif
#endif

/*═══════════════════════════════════════════════════════════════
  Internal Utilities
  ═══════════════════════════════════════════════════════════════*/

// Get page size (cached)
static size_t get_page_size(void) {
    static size_t page_size = 0;
    if (page_size == 0) {
        page_size = (size_t)sysconf(_SC_PAGESIZE);
        if (page_size == 0) {
            page_size = ARENAC_PAGE_SIZE;
        }
    }
    return page_size;
}

// Align size to page boundary
static size_t align_to_page(size_t size) {
    size_t page_size = get_page_size();
    return (size + page_size - 1) & ~(page_size - 1);
}

/*═══════════════════════════════════════════════════════════════
  Arena Creation and Destruction
  ═══════════════════════════════════════════════════════════════*/

arena_t* arenac_create(size_t size, uint32_t flags) {
    // Validate parameters
    if (size < ARENAC_MIN_SIZE || size > ARENAC_MAX_SIZE) {
        return NULL;
    }
    
    // Allocate arena structure
    arena_t* arena = (arena_t*)malloc(sizeof(arena_t));
    if (!arena) {
        return NULL;
    }
    
    // Align size to page boundary if using guard pages
    size_t aligned_size = size;
    if (flags & ARENAC_FLAG_GUARD_PAGES) {
        aligned_size = align_to_page(size);
    }
    
    // Allocate memory buffer
    void* memory = NULL;
    
    if (flags & ARENAC_FLAG_GUARD_PAGES) {
        // Use mmap for guard page support
        size_t total_size = aligned_size + 2 * get_page_size();
        void* mapping = mmap(NULL, total_size, 
                           PROT_READ | PROT_WRITE,
                           MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        
        if (mapping == MAP_FAILED) {
            free(arena);
            return NULL;
        }
        
        // Set up guard pages
        uint8_t* base = (uint8_t*)mapping;
        if (mprotect(base, get_page_size(), PROT_NONE) != 0 ||
            mprotect(base + get_page_size() + aligned_size, get_page_size(), PROT_NONE) != 0) {
            munmap(mapping, total_size);
            free(arena);
            return NULL;
        }
        
        memory = base + get_page_size();
        arena->guard_start = base;
        arena->guard_end = base + get_page_size() + aligned_size;
    } else {
        // Use regular malloc
        memory = malloc(aligned_size);
        if (!memory) {
            free(arena);
            return NULL;
        }
        arena->guard_start = NULL;
        arena->guard_end = NULL;
    }
    
    // Initialize arena
    if (arenac_init(arena, memory, aligned_size, flags) != 0) {
        if (flags & ARENAC_FLAG_GUARD_PAGES) {
            munmap(arena->guard_start, aligned_size + 2 * get_page_size());
        } else {
            free(memory);
        }
        free(arena);
        return NULL;
    }
    
    // Initialize optional components
    if (flags & ARENAC_FLAG_STATS) {
        if (arenac_enable_stats(arena) != 0) {
            arenac_destroy(arena);
            return NULL;
        }
    }
    
    if (flags & ARENAC_FLAG_THREAD_SAFE) {
        if (arenac_enable_thread_safety(arena) != 0) {
            arenac_destroy(arena);
            return NULL;
        }
    }
    
    return arena;
}

void arenac_destroy(arena_t* arena) {
    if (!arena) {
        return;
    }
    
    // Clean up thread safety
    if (arena->mutex) {
        pthread_mutex_destroy((pthread_mutex_t*)arena->mutex);
        free(arena->mutex);
    }
    
    // Clean up statistics
    if (arena->stats) {
        free(arena->stats);
    }
    
    // Free memory
    if (arena->guard_start) {
        // Memory was allocated with mmap
        size_t total_size = arena->size + 2 * get_page_size();
        munmap(arena->guard_start, total_size);
    } else {
        // Memory was allocated with malloc
        free(arena->base);
    }
    
    // Free arena structure
    free(arena);
}

/*═══════════════════════════════════════════════════════════════
  Zone Management
  ═══════════════════════════════════════════════════════════════*/

int arenac_add_zone(arena_t* arena, void* memory, size_t size) {
    if (!arena || !memory || size == 0) {
        return -1;
    }
    
    if (arena->zone_count >= ARENAC_MAX_ZONES) {
        return -2;  // Too many zones
    }
    
    uint32_t zone_id = arena->zone_count;
    arena->zones[zone_id] = (arenac_zone_t){
        .base = (uint8_t*)memory,
        .size = size,
        .used = 0,
        .zone_id = zone_id,
        .padding = 0
    };
    
    arena->zone_count++;
    arena->size += size;
    
    return (int)zone_id;
}

int arenac_switch_zone(arena_t* arena, uint32_t zone_id) {
    if (!arena || zone_id >= arena->zone_count) {
        return -1;
    }
    
    arena->current_zone = zone_id;
    return 0;
}

/*═══════════════════════════════════════════════════════════════
  Statistics
  ═══════════════════════════════════════════════════════════════*/

int arenac_enable_stats(arena_t* arena) {
    if (!arena) {
        return -1;
    }
    
    if (arena->stats) {
        return 0;  // Already enabled
    }
    
    arena->stats = (arenac_stats_t*)calloc(1, sizeof(arenac_stats_t));
    if (!arena->stats) {
        return -2;
    }
    
    arena->flags |= ARENAC_FLAG_STATS;
    return 0;
}

const arenac_stats_t* arenac_get_stats(const arena_t* arena) {
    if (!arena || !(arena->flags & ARENAC_FLAG_STATS)) {
        return NULL;
    }
    
    return arena->stats;
}

/*═══════════════════════════════════════════════════════════════
  Memory Protection
  ═══════════════════════════════════════════════════════════════*/

int arenac_enable_protection(arena_t* arena) {
    if (!arena) {
        return -1;
    }
    
    if (arena->flags & ARENAC_FLAG_GUARD_PAGES) {
        return 0;  // Already protected
    }
    
    // Cannot enable protection on existing arena without guard pages
    return -2;
}

int arenac_validate(const arena_t* arena) {
    if (!arena) {
        return -1;
    }
    
    // Check basic structure
    if (!arena->base || arena->size == 0) {
        return -2;
    }
    
    // Check usage bounds
    if (arena->used > arena->size) {
        return -3;
    }
    
    // Check zone consistency
    size_t total_zone_size = 0;
    for (uint32_t i = 0; i < arena->zone_count; i++) {
        const arenac_zone_t* zone = &arena->zones[i];
        if (!zone->base || zone->used > zone->size) {
            return -4;
        }
        total_zone_size += zone->size;
    }
    
    if (total_zone_size != arena->size) {
        return -5;
    }
    
    // Check current zone
    if (arena->current_zone >= arena->zone_count) {
        return -6;
    }
    
    return 0;
}

/*═══════════════════════════════════════════════════════════════
  Thread Safety
  ═══════════════════════════════════════════════════════════════*/

int arenac_enable_thread_safety(arena_t* arena) {
    if (!arena) {
        return -1;
    }
    
    if (arena->mutex) {
        return 0;  // Already thread-safe
    }
    
    pthread_mutex_t* mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    if (!mutex) {
        return -2;
    }
    
    if (pthread_mutex_init(mutex, NULL) != 0) {
        free(mutex);
        return -3;
    }
    
    arena->mutex = mutex;
    arena->flags |= ARENAC_FLAG_THREAD_SAFE;
    
    return 0;
}

void arenac_lock(arena_t* arena) {
    if (arena && arena->mutex) {
        pthread_mutex_lock((pthread_mutex_t*)arena->mutex);
    }
}

void arenac_unlock(arena_t* arena) {
    if (arena && arena->mutex) {
        pthread_mutex_unlock((pthread_mutex_t*)arena->mutex);
    }
}

/*═══════════════════════════════════════════════════════════════
  Performance Testing
  ═══════════════════════════════════════════════════════════════*/

// Simple cycle counter for testing
static inline uint64_t get_cycles(void) {
#if defined(__x86_64__) || defined(__i386__)
    uint32_t lo, hi;
    __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
#elif defined(__aarch64__)
    uint64_t val;
    __asm__ volatile("mrs %0, cntvct_el0" : "=r"(val));
    return val;
#else
    return 0;  // Fallback
#endif
}

uint64_t arenac_benchmark(arena_t* arena, uint64_t iterations) {
    if (!arena || iterations == 0) {
        return 0;
    }
    
    uint64_t total_cycles = 0;
    const size_t test_size = 64;  // Test allocation size
    
    for (uint64_t i = 0; i < iterations; i++) {
        // Save checkpoint
        arenac_checkpoint_t checkpoint = arenac_checkpoint(arena);
        
        // Measure allocation
        uint64_t start = get_cycles();
        void* ptr = arenac_alloc(arena, test_size);
        uint64_t end = get_cycles();
        
        if (ptr) {
            total_cycles += (end - start);
        }
        
        // Restore checkpoint
        arenac_restore(arena, &checkpoint);
    }
    
    return iterations > 0 ? total_cycles / iterations : 0;
}

int arenac_validate_7tick(arena_t* arena) {
    if (!arena) {
        return -1;
    }
    
    const uint64_t max_cycles = 7;  // 7-tick constraint
    const uint64_t test_iterations = 1000;
    
    // Test allocation performance
    uint64_t avg_cycles = arenac_benchmark(arena, test_iterations);
    
    // Check if within 7-tick constraint
    if (avg_cycles > max_cycles) {
        return -2;  // Performance violation
    }
    
    // Test checkpoint/restore performance
    arenac_checkpoint_t checkpoint;
    uint64_t start = get_cycles();
    checkpoint = arenac_checkpoint(arena);
    arenac_restore(arena, &checkpoint);
    uint64_t cycles = get_cycles() - start;
    
    if (cycles > max_cycles) {
        return -3;  // Checkpoint/restore violation
    }
    
    return 0;  // All tests passed
}