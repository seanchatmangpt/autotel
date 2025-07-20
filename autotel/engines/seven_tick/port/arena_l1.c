#include "cns/8t/8t.h"
#include <string.h>
#include <assert.h>
#include <stdlib.h>

// ============================================================================
// L1-OPTIMIZED ARENA ALLOCATOR IMPLEMENTATION
// ============================================================================

void cns_8t_arena_init(cns_8t_arena_t* arena, void* memory, size_t size) {
    assert(arena != NULL);
    assert(memory != NULL);
    assert(size > 0);
    assert(((uintptr_t)memory & (CNS_8T_ALIGNMENT - 1)) == 0); // Check alignment
    
    arena->beg = (char*)memory;
    arena->end = arena->beg + size;
    arena->prefetch_ptr = arena->beg + CNS_8T_CACHE_LINE_SIZE;
    arena->cache_line_mask = CNS_8T_CACHE_LINE_SIZE - 1;
    arena->l1_budget = CNS_8T_L1_CACHE_SIZE / 4; // Use 1/4 of L1 cache
    arena->allocation_count = 0;
    arena->cache_hits = 0;
}

void* cns_8t_arena_alloc(cns_8t_arena_t* arena, size_t size, size_t align) {
    assert(arena != NULL);
    assert(size > 0);
    assert((align & (align - 1)) == 0); // Check align is power of 2
    
    // Prefetch next cache line for better L1 performance
    if (arena->prefetch_ptr < arena->end) {
        __builtin_prefetch(arena->prefetch_ptr, 1, 3); // Write, high locality
    }
    
    // Calculate alignment padding
    uintptr_t current = (uintptr_t)arena->beg;
    size_t padding = (align - (current & (align - 1))) & (align - 1);
    
    // L1 cache line optimization: avoid crossing cache line boundaries unnecessarily
    if ((padding + size) > CNS_8T_CACHE_LINE_SIZE && 
        (current & arena->cache_line_mask) + padding > CNS_8T_CACHE_LINE_SIZE) {
        // Align to next cache line boundary
        padding = CNS_8T_CACHE_LINE_SIZE - (current & arena->cache_line_mask);
    }
    
    // Check if allocation fits
    if (arena->beg + padding + size > arena->end) {
        return NULL; // Out of memory
    }
    
    // Perform allocation
    void* result = arena->beg + padding;
    arena->beg += padding + size;
    
    // Update prefetch pointer
    arena->prefetch_ptr = arena->beg + CNS_8T_CACHE_LINE_SIZE;
    
    // Update statistics
    arena->allocation_count++;
    
    // Check if we're still within L1 cache budget
    size_t allocated_size = arena->beg - (char*)result + size;
    if (allocated_size <= arena->l1_budget) {
        arena->cache_hits++;
    }
    
    return result;
}

void cns_8t_arena_reset(cns_8t_arena_t* arena) {
    assert(arena != NULL);
    
    // Reset to initial state, preserving configuration
    char* original_start = arena->end - (arena->end - arena->beg);
    arena->beg = original_start;
    arena->prefetch_ptr = arena->beg + CNS_8T_CACHE_LINE_SIZE;
    arena->allocation_count = 0;
    arena->cache_hits = 0;
}

// ============================================================================
// L1 CACHE OPTIMIZATION UTILITIES
// ============================================================================

// Check if two pointers are in the same cache line
static inline bool same_cache_line(const void* a, const void* b) {
    uintptr_t addr_a = (uintptr_t)a;
    uintptr_t addr_b = (uintptr_t)b;
    return (addr_a >> 6) == (addr_b >> 6); // Divide by 64 (cache line size)
}

// Get cache line address
static inline void* get_cache_line_start(const void* ptr) {
    uintptr_t addr = (uintptr_t)ptr;
    return (void*)(addr & ~(CNS_8T_CACHE_LINE_SIZE - 1));
}

// Optimized memcpy for cache line aligned data
void cns_8t_memcpy_cache_optimized(void* dest, const void* src, size_t size) {
    assert(dest != NULL);
    assert(src != NULL);
    
    // Platform-specific optimized copy
#ifdef CNS_8T_X86_64
    // If both are cache line aligned and size is multiple of cache line
    if (((uintptr_t)dest & (CNS_8T_CACHE_LINE_SIZE - 1)) == 0 &&
        ((uintptr_t)src & (CNS_8T_CACHE_LINE_SIZE - 1)) == 0 &&
        (size & (CNS_8T_CACHE_LINE_SIZE - 1)) == 0) {
        
        // Use non-temporal stores for large copies to avoid cache pollution
        if (size >= 512) {
            char* d = (char*)dest;
            const char* s = (const char*)src;
            
            for (size_t i = 0; i < size; i += CNS_8T_CACHE_LINE_SIZE) {
                __m256d v0 = _mm256_load_pd((const double*)(s + i));
                __m256d v1 = _mm256_load_pd((const double*)(s + i + 32));
                
                _mm256_stream_pd((double*)(d + i), v0);
                _mm256_stream_pd((double*)(d + i + 32), v1);
            }
            
            _mm_sfence(); // Ensure all stores complete
            return;
        }
    }
#elif defined(CNS_8T_ARM64)
    // ARM NEON optimized copy for large aligned data
    if (size >= 512 && ((uintptr_t)dest & 15) == 0 && ((uintptr_t)src & 15) == 0) {
        char* d = (char*)dest;
        const char* s = (const char*)src;
        
        for (size_t i = 0; i < size; i += 64) {
            float64x2_t v0 = vld1q_f64((const double*)(s + i));
            float64x2_t v1 = vld1q_f64((const double*)(s + i + 16));
            float64x2_t v2 = vld1q_f64((const double*)(s + i + 32));
            float64x2_t v3 = vld1q_f64((const double*)(s + i + 48));
            
            vst1q_f64((double*)(d + i), v0);
            vst1q_f64((double*)(d + i + 16), v1);
            vst1q_f64((double*)(d + i + 32), v2);
            vst1q_f64((double*)(d + i + 48), v3);
        }
        return;
    }
#endif
    
    // Fallback to standard memcpy
    memcpy(dest, src, size);
}

// Get L1 cache efficiency for arena
double cns_8t_arena_get_l1_efficiency(const cns_8t_arena_t* arena) {
    assert(arena != NULL);
    
    if (arena->allocation_count == 0) {
        return 1.0; // Perfect efficiency for no allocations
    }
    
    return (double)arena->cache_hits / arena->allocation_count;
}