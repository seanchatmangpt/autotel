/*  ─────────────────────────────────────────────────────────────
    src/8t/arena_l1_full.c  –  Full L1-Optimized Arena Implementation
    Complete implementation matching the advanced arena_l1.h header
    ───────────────────────────────────────────────────────────────*/

#define _GNU_SOURCE  // For aligned_alloc, madvise, numa support
#include "cns/8t/arena_l1.h"
#include "cns/8t/core.h"
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>

#ifdef __linux__
#include <numa.h>
#include <numaif.h>
#endif

/*═══════════════════════════════════════════════════════════════
  Internal Constants and Helpers
  ═══════════════════════════════════════════════════════════════*/

// Magic numbers for corruption detection
#define ARENA_MAGIC_LIVE    0xCAFEBABEDEADBEEFULL
#define ARENA_MAGIC_FREE    0xFEEDFACEBAADC0DEULL

// Memory allocation flags
#define MAP_HUGE_2MB    (21 << MAP_HUGE_SHIFT)

// Get current timestamp in cycles
static inline uint64_t get_cycles(void) {
    uint32_t lo, hi;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

// Get NUMA node for current CPU
static inline int get_numa_node(void) {
#ifdef __linux__
    if (numa_available() >= 0) {
        return numa_node_of_cpu(sched_getcpu());
    }
#endif
    return 0;
}

/*═══════════════════════════════════════════════════════════════
  Arena Creation and Initialization
  ═══════════════════════════════════════════════════════════════*/

cns_8t_result_t cns_8t_arena_l1_create(const cns_8t_arena_l1_config_t* config,
                                        cns_8t_arena_l1_t** arena) {
    if (!config || !arena) {
        return CNS_8T_ERROR_INVALID_ARGUMENT;
    }
    
    // Start timing
    uint64_t start_cycles = get_cycles();
    
    // Allocate arena structure (cache-aligned)
    cns_8t_arena_l1_t* a = aligned_alloc(64, sizeof(cns_8t_arena_l1_t));
    if (!a) {
        return CNS_8T_ERROR_OUT_OF_MEMORY;
    }
    
    // Clear arena structure
    memset(a, 0, sizeof(cns_8t_arena_l1_t));
    
    // Copy and validate configuration
    a->config = *config;
    
    // Set defaults
    if (a->config.l1_working_set_size == 0) {
        a->config.l1_working_set_size = CNS_8T_L1_CACHE_SIZE;
    }
    if (a->config.l1_working_set_size > CNS_8T_L1_CACHE_SIZE) {
        a->config.l1_working_set_size = CNS_8T_L1_CACHE_SIZE;
    }
    if (a->config.prefetch_distance == 0) {
        a->config.prefetch_distance = 2;
    }
    if (a->config.alignment_requirement == 0) {
        a->config.alignment_requirement = 64;
    }
    if (a->config.color_stride == 0) {
        a->config.color_stride = 256;  // 4 cache lines
    }
    
    // Calculate number of sub-arenas
    a->num_arenas = CNS_8T_L1_CACHE_SIZE / (4 * 1024);  // 8 arenas of 4KB each
    if (a->num_arenas == 0) a->num_arenas = 1;
    if (a->num_arenas > 16) a->num_arenas = 16;
    
    // Calculate memory requirements
    size_t metadata_size = a->num_arenas * sizeof(cns_8t_arena_metadata_t);
    size_t arena_size = a->config.l1_working_set_size * 2;  // Double for headroom
    size_t total_size = metadata_size + arena_size;
    
    // Align to page/huge page boundary
    size_t page_size = sysconf(_SC_PAGESIZE);
    if (a->config.use_huge_pages) {
        page_size = 2 * 1024 * 1024;  // 2MB huge pages
    }
    total_size = (total_size + page_size - 1) & ~(page_size - 1);
    
    // Allocate memory with appropriate flags
    int mmap_flags = MAP_PRIVATE | MAP_ANONYMOUS;
    int mmap_prot = PROT_READ | PROT_WRITE;
    
#ifdef MAP_HUGETLB
    if (a->config.use_huge_pages) {
        mmap_flags |= MAP_HUGETLB | MAP_HUGE_2MB;
    }
#endif
    
    // NUMA-aware allocation
    void* memory = NULL;
    if (a->config.enable_numa_locality) {
#ifdef __linux__
        if (numa_available() >= 0) {
            int node = get_numa_node();
            memory = numa_alloc_onnode(total_size, node);
            if (!memory) {
                // Fallback to regular mmap
                memory = mmap(NULL, total_size, mmap_prot, mmap_flags, -1, 0);
            }
        } else
#endif
        {
            memory = mmap(NULL, total_size, mmap_prot, mmap_flags, -1, 0);
        }
    } else {
        memory = mmap(NULL, total_size, mmap_prot, mmap_flags, -1, 0);
    }
    
    if (memory == MAP_FAILED) {
        free(a);
        return CNS_8T_ERROR_SYSTEM;
    }
    
    // Set up memory regions
    a->memory_base = memory;
    a->memory_size = total_size;
    
    // Set up arena metadata array
    a->arenas = (cns_8t_arena_metadata_t*)memory;
    
    // Initialize each sub-arena
    uint8_t* arena_base = (uint8_t*)memory + metadata_size;
    size_t per_arena_size = arena_size / a->num_arenas;
    
    for (uint32_t i = 0; i < a->num_arenas; i++) {
        cns_8t_arena_metadata_t* meta = &a->arenas[i];
        
        // Clear metadata (important for cache line alignment)
        memset(meta, 0, sizeof(cns_8t_arena_metadata_t));
        
        // Initialize hot path data
        meta->base_ptr = arena_base + (i * per_arena_size);
        meta->total_size = per_arena_size;
        meta->used_size = 0;
        meta->next_offset = 0;
        meta->arena_id = i;
        meta->numa_node = get_numa_node();
        meta->alignment_shift = __builtin_ctz(a->config.alignment_requirement);
        
        // Initialize free lists
        for (int j = 0; j < 4; j++) {
            meta->free_lists[j] = NULL;
        }
        
        // Initialize performance counters
        meta->allocations = 0;
        meta->deallocations = 0;
        meta->cache_hits = 0;
        meta->cache_misses = 0;
        meta->access_pattern_bitmap = 0;
    }
    
    // Initialize L1 optimization state
    a->last_cache_set = 0;
    a->allocation_sequence = 0;
    a->prefetch_head = 0;
    a->prefetch_tail = 0;
    
    // Initialize SIMD context
    a->simd_zero = _mm256_setzero_si256();
    a->simd_pattern = _mm256_set1_epi8(0xAA);
    
    // Apply memory hints
    if (a->config.optimize_for_streaming) {
        madvise(memory, total_size, MADV_SEQUENTIAL);
    } else {
        madvise(memory, total_size, MADV_WILLNEED);
    }
    
    // Prefault pages if using huge pages
    if (a->config.use_huge_pages) {
        volatile char* touch = (volatile char*)memory;
        for (size_t i = 0; i < total_size; i += page_size) {
            touch[i] = 0;
        }
    }
    
    // Initialize performance tracking
    a->perf.total_cycles = get_cycles() - start_cycles;
    a->perf.total_ops = 0;
    a->perf.successful_ops = 0;
    a->perf.cache_hits = 0;
    a->perf.cache_misses = 0;
    a->l1_utilization_cycles = 0;
    a->total_cycles = 0;
    
    *arena = a;
    return CNS_8T_SUCCESS;
}

/*═══════════════════════════════════════════════════════════════
  Fast Allocation with L1 Optimization
  ═══════════════════════════════════════════════════════════════*/

cns_8t_result_t cns_8t_arena_l1_alloc_fast(cns_8t_arena_l1_t* arena,
                                            size_t size,
                                            void** ptr) {
    if (!arena || !ptr || size == 0) {
        return CNS_8T_ERROR_INVALID_ARGUMENT;
    }
    
    uint64_t start_cycles = get_cycles();
    
    // Determine size class using branchless algorithm
    uint32_t size_class = cns_8t_arena_l1_size_class_branchless(size);
    
    // Add header size
    size_t total_size = size + sizeof(cns_8t_arena_block_t);
    
    // Align to requirement
    total_size = cns_8t_arena_l1_align_branchless(total_size, 
                                                   arena->config.alignment_requirement);
    
    // Get current arena metadata
    cns_8t_arena_metadata_t* meta = &arena->arenas[arena->active_arena];
    
    // Try free list first (hot path)
    if (size_class < 4 && meta->free_lists[size_class]) {
        cns_8t_arena_block_t* block = meta->free_lists[size_class];
        meta->free_lists[size_class] = block->next;
        
        // Update statistics
        meta->allocations++;
        meta->cache_hits++;
        arena->perf.cache_hits++;
        
        // Prefetch the block
        if (arena->config.enable_prefetch) {
            __builtin_prefetch(block, 1, arena->config.temporal_locality_hint);
            __builtin_prefetch((uint8_t*)block + 64, 1, arena->config.temporal_locality_hint);
        }
        
        // Return user pointer
        *ptr = (uint8_t*)block + sizeof(cns_8t_arena_block_t);
        
        // Track cycles
        arena->perf.total_cycles += get_cycles() - start_cycles;
        arena->perf.successful_ops++;
        
        return CNS_8T_SUCCESS;
    }
    
    // Check if allocation fits
    if (meta->next_offset + total_size > meta->total_size) {
        // Try next arena
        uint32_t start_arena = arena->active_arena;
        do {
            arena->active_arena = (arena->active_arena + 1) % arena->num_arenas;
            meta = &arena->arenas[arena->active_arena];
            
            if (meta->next_offset + total_size <= meta->total_size) {
                break;
            }
        } while (arena->active_arena != start_arena);
        
        // Still doesn't fit?
        if (meta->next_offset + total_size > meta->total_size) {
            arena->perf.total_cycles += get_cycles() - start_cycles;
            return CNS_8T_ERROR_OUT_OF_MEMORY;
        }
    }
    
    // Calculate allocation address with cache coloring
    uint32_t offset = meta->next_offset;
    
    if (arena->config.enable_cache_coloring) {
        // Apply cache coloring to reduce conflicts
        uint32_t color = (arena->last_cache_set * CNS_8T_L1_LINE_SIZE) % 
                        arena->config.color_stride;
        offset = (offset + color) & ~(arena->config.alignment_requirement - 1);
        arena->last_cache_set = (arena->last_cache_set + 1) & (CNS_8T_L1_SETS - 1);
    }
    
    cns_8t_arena_block_t* block = (cns_8t_arena_block_t*)
        ((uint8_t*)meta->base_ptr + offset);
    
    // Initialize block header
    block->next = NULL;
    block->size = total_size;
    block->arena_id = meta->arena_id;
    block->block_class = size_class;
    block->flags = 0;
    block->magic = ARENA_MAGIC_LIVE;
    
    // Update arena state
    meta->next_offset = offset + total_size;
    meta->used_size += total_size;
    meta->allocations++;
    arena->allocation_sequence++;
    
    // Prefetch future allocation areas
    if (arena->config.enable_prefetch) {
        for (uint32_t i = 1; i <= arena->config.prefetch_distance; i++) {
            void* prefetch_addr = (uint8_t*)meta->base_ptr + meta->next_offset + 
                                 (i * CNS_8T_L1_LINE_SIZE);
            if (prefetch_addr < (uint8_t*)meta->base_ptr + meta->total_size) {
                __builtin_prefetch(prefetch_addr, 1, 3);
            }
        }
    }
    
    // Update prefetch queue
    if (arena->prefetch_tail < 16) {
        arena->prefetch_queue[arena->prefetch_tail++] = meta->next_offset;
    }
    
    // Clear memory if requested
    void* user_ptr = (uint8_t*)block + sizeof(cns_8t_arena_block_t);
    if (arena->config.optimize_for_streaming) {
        cns_8t_arena_l1_simd_memset(user_ptr, 0, size);
    }
    
    // Update access pattern
    meta->access_pattern_bitmap = (meta->access_pattern_bitmap << 1) | 1;
    
    // Update performance metrics
    uint64_t elapsed = get_cycles() - start_cycles;
    arena->perf.total_cycles += elapsed;
    arena->perf.total_ops++;
    arena->perf.successful_ops++;
    
    // Track L1 utilization
    if (elapsed <= 8) {  // 8 cycles or less indicates L1 hit
        arena->l1_utilization_cycles++;
    }
    arena->total_cycles++;
    
    *ptr = user_ptr;
    return CNS_8T_SUCCESS;
}

/*═══════════════════════════════════════════════════════════════
  Core Allocation Interface
  ═══════════════════════════════════════════════════════════════*/

cns_8t_result_t cns_8t_arena_l1_alloc(cns_8t_arena_l1_t* arena,
                                       size_t size,
                                       void** ptr) {
    return cns_8t_arena_l1_alloc_fast(arena, size, ptr);
}

/*═══════════════════════════════════════════════════════════════
  Aligned Allocation with Hints
  ═══════════════════════════════════════════════════════════════*/

cns_8t_result_t cns_8t_arena_l1_alloc_aligned(cns_8t_arena_l1_t* arena,
                                               const cns_8t_arena_alloc_request_t* req,
                                               void** ptr) {
    if (!arena || !req || !ptr) {
        return CNS_8T_ERROR_INVALID_ARGUMENT;
    }
    
    // Validate alignment
    if (!cns_8t_arena_l1_is_power_of_2(req->alignment)) {
        return CNS_8T_ERROR_INVALID_ARGUMENT;
    }
    
    uint64_t start_cycles = get_cycles();
    
    // Calculate allocation size with alignment
    size_t header_size = sizeof(cns_8t_arena_block_t);
    size_t total_size = cns_8t_arena_l1_align_branchless(
        header_size + req->size + req->alignment, req->alignment);
    
    // Allocate raw memory
    void* raw_ptr;
    cns_8t_result_t result = cns_8t_arena_l1_alloc_fast(arena, total_size, &raw_ptr);
    if (result != CNS_8T_SUCCESS) {
        return result;
    }
    
    // Calculate aligned user pointer
    uintptr_t raw_addr = (uintptr_t)raw_ptr;
    uintptr_t aligned_addr = cns_8t_arena_l1_align_branchless(raw_addr, req->alignment);
    
    // Apply access pattern hints
    if (req->temporal_locality && arena->config.enable_prefetch) {
        // High temporal locality - prefetch into L1
        __builtin_prefetch((void*)aligned_addr, 1, 3);
        for (size_t i = 64; i < req->size; i += 64) {
            __builtin_prefetch((void*)(aligned_addr + i), 1, 3);
        }
    }
    
    if (req->spatial_locality) {
        // High spatial locality - prefetch neighboring lines
        for (int i = -2; i <= 2; i++) {
            if (i != 0) {
                __builtin_prefetch((void*)(aligned_addr + i * 64), 0, 2);
            }
        }
    }
    
    // Update cache priority in block header
    cns_8t_arena_block_t* block = (cns_8t_arena_block_t*)
        ((uint8_t*)raw_ptr - sizeof(cns_8t_arena_block_t));
    block->flags |= (req->cache_priority << 4);
    
    arena->perf.total_cycles += get_cycles() - start_cycles;
    
    *ptr = (void*)aligned_addr;
    return CNS_8T_SUCCESS;
}

/*═══════════════════════════════════════════════════════════════
  Batch Allocation
  ═══════════════════════════════════════════════════════════════*/

cns_8t_result_t cns_8t_arena_l1_alloc_batch(cns_8t_arena_l1_t* arena,
                                             const size_t* sizes,
                                             uint32_t count,
                                             void** ptrs) {
    if (!arena || !sizes || !ptrs || count == 0) {
        return CNS_8T_ERROR_INVALID_ARGUMENT;
    }
    
    uint64_t start_cycles = get_cycles();
    
    // Calculate total size needed
    size_t total_size = 0;
    size_t aligned_sizes[count];
    
    for (uint32_t i = 0; i < count; i++) {
        aligned_sizes[i] = cns_8t_arena_l1_align_branchless(
            sizes[i] + sizeof(cns_8t_arena_block_t),
            arena->config.alignment_requirement);
        total_size += aligned_sizes[i];
    }
    
    // Find arena with enough space
    cns_8t_arena_metadata_t* meta = NULL;
    for (uint32_t i = 0; i < arena->num_arenas; i++) {
        meta = &arena->arenas[(arena->active_arena + i) % arena->num_arenas];
        if (meta->next_offset + total_size <= meta->total_size) {
            arena->active_arena = (arena->active_arena + i) % arena->num_arenas;
            break;
        }
    }
    
    if (!meta || meta->next_offset + total_size > meta->total_size) {
        return CNS_8T_ERROR_OUT_OF_MEMORY;
    }
    
    // Allocate all blocks contiguously for cache efficiency
    uint8_t* current = (uint8_t*)meta->base_ptr + meta->next_offset;
    
    for (uint32_t i = 0; i < count; i++) {
        cns_8t_arena_block_t* block = (cns_8t_arena_block_t*)current;
        
        // Initialize block
        block->next = NULL;
        block->size = aligned_sizes[i];
        block->arena_id = meta->arena_id;
        block->block_class = cns_8t_arena_l1_size_class_branchless(sizes[i]);
        block->flags = 0;
        block->magic = ARENA_MAGIC_LIVE;
        
        // Return user pointer
        ptrs[i] = current + sizeof(cns_8t_arena_block_t);
        
        // Prefetch next block
        if (i < count - 1 && arena->config.enable_prefetch) {
            __builtin_prefetch(current + aligned_sizes[i], 1, 3);
        }
        
        current += aligned_sizes[i];
    }
    
    // Update arena state
    meta->next_offset += total_size;
    meta->used_size += total_size;
    meta->allocations += count;
    arena->allocation_sequence += count;
    
    // Prefetch future areas
    if (arena->config.enable_prefetch) {
        void* next_area = (uint8_t*)meta->base_ptr + meta->next_offset;
        for (uint32_t i = 0; i < arena->config.prefetch_distance; i++) {
            __builtin_prefetch((uint8_t*)next_area + i * 64, 1, 3);
        }
    }
    
    arena->perf.total_cycles += get_cycles() - start_cycles;
    arena->perf.total_ops += count;
    arena->perf.successful_ops += count;
    
    return CNS_8T_SUCCESS;
}

/*═══════════════════════════════════════════════════════════════
  Memory Operations
  ═══════════════════════════════════════════════════════════════*/

cns_8t_result_t cns_8t_arena_l1_clear_simd(void* ptr, size_t size) {
    if (!ptr || size == 0) {
        return CNS_8T_ERROR_INVALID_ARGUMENT;
    }
    
    cns_8t_arena_l1_simd_memset(ptr, 0, size);
    return CNS_8T_SUCCESS;
}

cns_8t_result_t cns_8t_arena_l1_copy_optimized(void* dst, const void* src, size_t size) {
    if (!dst || !src || size == 0) {
        return CNS_8T_ERROR_INVALID_ARGUMENT;
    }
    
    cns_8t_arena_l1_simd_memcpy(dst, src, size);
    return CNS_8T_SUCCESS;
}

/*═══════════════════════════════════════════════════════════════
  Free Operation
  ═══════════════════════════════════════════════════════════════*/

cns_8t_result_t cns_8t_arena_l1_free(cns_8t_arena_l1_t* arena, void* ptr) {
    if (!arena || !ptr) {
        return CNS_8T_ERROR_INVALID_ARGUMENT;
    }
    
    uint64_t start_cycles = get_cycles();
    
    // Get block header
    cns_8t_arena_block_t* block = (cns_8t_arena_block_t*)
        ((uint8_t*)ptr - sizeof(cns_8t_arena_block_t));
    
    // Validate magic
    if (block->magic != ARENA_MAGIC_LIVE) {
        return CNS_8T_ERROR_CORRUPTION;
    }
    
    // Validate arena ID
    if (block->arena_id >= arena->num_arenas) {
        return CNS_8T_ERROR_CORRUPTION;
    }
    
    // Mark as free
    block->magic = ARENA_MAGIC_FREE;
    
    // Get arena metadata
    cns_8t_arena_metadata_t* meta = &arena->arenas[block->arena_id];
    
    // Add to free list if appropriate size
    if (block->block_class < 4) {
        block->next = meta->free_lists[block->block_class];
        meta->free_lists[block->block_class] = block;
        meta->deallocations++;
        
        // Prefetch block for reuse
        if (arena->config.enable_prefetch) {
            __builtin_prefetch(block, 1, 2);
        }
    }
    
    arena->perf.total_cycles += get_cycles() - start_cycles;
    arena->perf.total_ops++;
    
    return CNS_8T_SUCCESS;
}

/*═══════════════════════════════════════════════════════════════
  Reallocation
  ═══════════════════════════════════════════════════════════════*/

cns_8t_result_t cns_8t_arena_l1_realloc(cns_8t_arena_l1_t* arena,
                                         void* ptr,
                                         size_t new_size,
                                         void** new_ptr) {
    if (!arena || !new_ptr) {
        return CNS_8T_ERROR_INVALID_ARGUMENT;
    }
    
    // Handle NULL ptr as malloc
    if (!ptr) {
        return cns_8t_arena_l1_alloc(arena, new_size, new_ptr);
    }
    
    // Handle zero size as free
    if (new_size == 0) {
        cns_8t_arena_l1_free(arena, ptr);
        *new_ptr = NULL;
        return CNS_8T_SUCCESS;
    }
    
    // Get current block
    cns_8t_arena_block_t* block = (cns_8t_arena_block_t*)
        ((uint8_t*)ptr - sizeof(cns_8t_arena_block_t));
    
    // Validate block
    if (block->magic != ARENA_MAGIC_LIVE) {
        return CNS_8T_ERROR_CORRUPTION;
    }
    
    size_t old_size = block->size - sizeof(cns_8t_arena_block_t);
    
    // If shrinking, just return same pointer
    if (new_size <= old_size) {
        *new_ptr = ptr;
        return CNS_8T_SUCCESS;
    }
    
    // Need to allocate new block
    void* new_block;
    cns_8t_result_t result = cns_8t_arena_l1_alloc(arena, new_size, &new_block);
    if (result != CNS_8T_SUCCESS) {
        return result;
    }
    
    // Copy old data
    cns_8t_arena_l1_simd_memcpy(new_block, ptr, old_size);
    
    // Free old block
    cns_8t_arena_l1_free(arena, ptr);
    
    *new_ptr = new_block;
    return CNS_8T_SUCCESS;
}

/*═══════════════════════════════════════════════════════════════
  Arena Reset
  ═══════════════════════════════════════════════════════════════*/

cns_8t_result_t cns_8t_arena_l1_reset(cns_8t_arena_l1_t* arena) {
    if (!arena) {
        return CNS_8T_ERROR_INVALID_ARGUMENT;
    }
    
    // Reset all sub-arenas
    for (uint32_t i = 0; i < arena->num_arenas; i++) {
        cns_8t_arena_metadata_t* meta = &arena->arenas[i];
        
        // Reset allocation state
        meta->used_size = 0;
        meta->next_offset = 0;
        
        // Clear free lists
        for (int j = 0; j < 4; j++) {
            meta->free_lists[j] = NULL;
        }
        
        // Keep cumulative statistics
        meta->access_pattern_bitmap = 0;
    }
    
    // Reset arena state
    arena->active_arena = 0;
    arena->allocation_sequence = 0;
    arena->last_cache_set = 0;
    
    // Clear prefetch queue
    arena->prefetch_head = 0;
    arena->prefetch_tail = 0;
    
    return CNS_8T_SUCCESS;
}

/*═══════════════════════════════════════════════════════════════
  Arena Destruction
  ═══════════════════════════════════════════════════════════════*/

cns_8t_result_t cns_8t_arena_l1_destroy(cns_8t_arena_l1_t* arena) {
    if (!arena) {
        return CNS_8T_ERROR_INVALID_ARGUMENT;
    }
    
    // Unmap memory
    if (arena->memory_base) {
        if (arena->config.enable_numa_locality) {
#ifdef __linux__
            if (numa_available() >= 0) {
                numa_free(arena->memory_base, arena->memory_size);
            } else
#endif
            {
                munmap(arena->memory_base, arena->memory_size);
            }
        } else {
            munmap(arena->memory_base, arena->memory_size);
        }
    }
    
    // Free arena structure
    free(arena);
    
    return CNS_8T_SUCCESS;
}

/*═══════════════════════════════════════════════════════════════
  Cache Analysis
  ═══════════════════════════════════════════════════════════════*/

cns_8t_result_t cns_8t_arena_l1_analyze_cache(cns_8t_arena_l1_t* arena,
                                               cns_8t_l1_analysis_t* analysis) {
    if (!arena || !analysis) {
        return CNS_8T_ERROR_INVALID_ARGUMENT;
    }
    
    memset(analysis, 0, sizeof(cns_8t_l1_analysis_t));
    
    // Calculate L1 hit rate
    if (arena->total_cycles > 0) {
        analysis->l1_hit_rate = (double)arena->l1_utilization_cycles / arena->total_cycles;
    }
    
    // Calculate working set efficiency
    size_t total_used = 0;
    for (uint32_t i = 0; i < arena->num_arenas; i++) {
        total_used += arena->arenas[i].used_size;
    }
    
    if (arena->config.l1_working_set_size > 0) {
        analysis->working_set_efficiency = 
            (double)total_used / arena->config.l1_working_set_size;
    }
    
    // Analyze cache conflicts
    uint32_t conflicts = 0;
    for (uint32_t i = 0; i < arena->num_arenas; i++) {
        uint64_t pattern = arena->arenas[i].access_pattern_bitmap;
        // Count consecutive 1s (potential conflicts)
        uint32_t consecutive = 0;
        while (pattern) {
            if (pattern & 1) {
                consecutive++;
                if (consecutive > 4) conflicts++;
            } else {
                consecutive = 0;
            }
            pattern >>= 1;
        }
    }
    analysis->cache_conflicts = conflicts;
    
    // Check false sharing risk
    analysis->false_sharing_risk = 0;
    if (arena->config.alignment_requirement < CNS_8T_L1_LINE_SIZE) {
        analysis->false_sharing_risk = 10;  // High risk
    } else if (arena->config.alignment_requirement == CNS_8T_L1_LINE_SIZE) {
        analysis->false_sharing_risk = 5;   // Medium risk
    }
    
    // Recommendations
    analysis->recommend_reordering = (conflicts > 10);
    analysis->recommend_prefetch = !arena->config.enable_prefetch && 
                                  (analysis->l1_hit_rate < 0.8);
    analysis->recommend_alignment = (arena->config.alignment_requirement < 64);
    analysis->optimal_block_size = CNS_8T_ARENA_SMALL_SIZE;
    
    return CNS_8T_SUCCESS;
}

/*═══════════════════════════════════════════════════════════════
  Layout Optimization
  ═══════════════════════════════════════════════════════════════*/

cns_8t_result_t cns_8t_arena_l1_optimize_layout(cns_8t_arena_l1_t* arena) {
    if (!arena) {
        return CNS_8T_ERROR_INVALID_ARGUMENT;
    }
    
    // Analyze current layout
    cns_8t_l1_analysis_t analysis;
    cns_8t_arena_l1_analyze_cache(arena, &analysis);
    
    // Apply optimizations based on analysis
    if (analysis.recommend_prefetch) {
        arena->config.enable_prefetch = true;
        arena->config.prefetch_distance = 3;
    }
    
    if (analysis.recommend_alignment) {
        arena->config.alignment_requirement = 64;
    }
    
    if (analysis.cache_conflicts > 20) {
        // Enable more aggressive cache coloring
        arena->config.enable_cache_coloring = true;
        arena->config.color_stride = 512;  // 8 cache lines
    }
    
    // Rebalance arenas if needed
    if (analysis.working_set_efficiency < 0.5) {
        // Consolidate to fewer arenas
        uint32_t new_count = arena->num_arenas / 2;
        if (new_count < 1) new_count = 1;
        
        // Reset and use fewer arenas
        cns_8t_arena_l1_reset(arena);
        arena->num_arenas = new_count;
    }
    
    return CNS_8T_SUCCESS;
}

/*═══════════════════════════════════════════════════════════════
  Prefetch Control
  ═══════════════════════════════════════════════════════════════*/

cns_8t_result_t cns_8t_arena_l1_prefetch_next(cns_8t_arena_l1_t* arena,
                                               uint32_t prefetch_count) {
    if (!arena || prefetch_count == 0) {
        return CNS_8T_ERROR_INVALID_ARGUMENT;
    }
    
    cns_8t_arena_metadata_t* meta = &arena->arenas[arena->active_arena];
    uint8_t* prefetch_addr = (uint8_t*)meta->base_ptr + meta->next_offset;
    
    // Prefetch upcoming allocation areas
    for (uint32_t i = 0; i < prefetch_count; i++) {
        if (prefetch_addr + i * 64 < (uint8_t*)meta->base_ptr + meta->total_size) {
            __builtin_prefetch(prefetch_addr + i * 64, 1, 3);
        }
    }
    
    // Also prefetch from prefetch queue
    while (arena->prefetch_head < arena->prefetch_tail && prefetch_count > 0) {
        uint32_t offset = arena->prefetch_queue[arena->prefetch_head++];
        void* addr = (uint8_t*)meta->base_ptr + offset;
        __builtin_prefetch(addr, 0, 2);
        prefetch_count--;
    }
    
    return CNS_8T_SUCCESS;
}

/*═══════════════════════════════════════════════════════════════
  Cache Coloring
  ═══════════════════════════════════════════════════════════════*/

cns_8t_result_t cns_8t_arena_l1_apply_coloring(cns_8t_arena_l1_t* arena,
                                                void* ptr,
                                                size_t size) {
    if (!arena || !ptr || size == 0) {
        return CNS_8T_ERROR_INVALID_ARGUMENT;
    }
    
    // Calculate cache set for this allocation
    uintptr_t addr = (uintptr_t)ptr;
    uint32_t cache_set = (addr / CNS_8T_L1_LINE_SIZE) % CNS_8T_L1_SETS;
    
    // Track cache set usage
    arena->last_cache_set = cache_set;
    
    // Prefetch with appropriate hints based on cache set
    if (arena->config.enable_prefetch) {
        // Prefetch this and neighboring cache lines
        __builtin_prefetch(ptr, 1, 3);
        if (size > 64) {
            __builtin_prefetch((uint8_t*)ptr + 64, 1, 2);
        }
        if (size > 128) {
            __builtin_prefetch((uint8_t*)ptr + 128, 1, 1);
        }
    }
    
    return CNS_8T_SUCCESS;
}

/*═══════════════════════════════════════════════════════════════
  Performance Monitoring
  ═══════════════════════════════════════════════════════════════*/

cns_8t_result_t cns_8t_arena_l1_get_stats(cns_8t_arena_l1_t* arena,
                                           cns_8t_perf_metrics_t* stats) {
    if (!arena || !stats) {
        return CNS_8T_ERROR_INVALID_ARGUMENT;
    }
    
    *stats = arena->perf;
    
    // Add per-arena statistics
    for (uint32_t i = 0; i < arena->num_arenas; i++) {
        cns_8t_arena_metadata_t* meta = &arena->arenas[i];
        stats->cache_hits += meta->cache_hits;
        stats->cache_misses += meta->cache_misses;
    }
    
    return CNS_8T_SUCCESS;
}

cns_8t_result_t cns_8t_arena_l1_get_l1_metrics(cns_8t_arena_l1_t* arena,
                                                cns_8t_l1_analysis_t* metrics) {
    return cns_8t_arena_l1_analyze_cache(arena, metrics);
}

/*═══════════════════════════════════════════════════════════════
  Configuration
  ═══════════════════════════════════════════════════════════════*/

cns_8t_result_t cns_8t_arena_l1_set_config(cns_8t_arena_l1_t* arena,
                                            const cns_8t_arena_l1_config_t* config) {
    if (!arena || !config) {
        return CNS_8T_ERROR_INVALID_ARGUMENT;
    }
    
    // Update configuration (some fields can't be changed after creation)
    arena->config.prefetch_distance = config->prefetch_distance;
    arena->config.enable_cache_coloring = config->enable_cache_coloring;
    arena->config.color_stride = config->color_stride;
    arena->config.enable_prefetch = config->enable_prefetch;
    arena->config.optimize_for_streaming = config->optimize_for_streaming;
    arena->config.temporal_locality_hint = config->temporal_locality_hint;
    
    return CNS_8T_SUCCESS;
}

cns_8t_result_t cns_8t_arena_l1_tune_for_workload(cns_8t_arena_l1_t* arena,
                                                   const char* workload_profile) {
    if (!arena || !workload_profile) {
        return CNS_8T_ERROR_INVALID_ARGUMENT;
    }
    
    // Apply workload-specific tuning
    if (strcmp(workload_profile, "streaming") == 0) {
        arena->config.optimize_for_streaming = true;
        arena->config.prefetch_distance = 4;
        arena->config.temporal_locality_hint = 0;  // No reuse
    } else if (strcmp(workload_profile, "random") == 0) {
        arena->config.enable_cache_coloring = true;
        arena->config.color_stride = 1024;
        arena->config.prefetch_distance = 1;
    } else if (strcmp(workload_profile, "sequential") == 0) {
        arena->config.optimize_for_streaming = true;
        arena->config.enable_prefetch = true;
        arena->config.prefetch_distance = 3;
        arena->config.temporal_locality_hint = 2;
    } else if (strcmp(workload_profile, "graph") == 0) {
        arena->config.enable_cache_coloring = true;
        arena->config.alignment_requirement = 64;
        arena->config.prefetch_distance = 2;
        arena->config.temporal_locality_hint = 3;  // High reuse
    }
    
    return CNS_8T_SUCCESS;
}

/*═══════════════════════════════════════════════════════════════
  Debug Support
  ═══════════════════════════════════════════════════════════════*/

#ifdef CNS_8T_DEBUG

cns_8t_result_t cns_8t_arena_l1_validate(cns_8t_arena_l1_t* arena) {
    if (!arena) {
        return CNS_8T_ERROR_INVALID_ARGUMENT;
    }
    
    // Validate each sub-arena
    for (uint32_t i = 0; i < arena->num_arenas; i++) {
        cns_8t_arena_metadata_t* meta = &arena->arenas[i];
        
        // Check bounds
        if (meta->used_size > meta->total_size) {
            return CNS_8T_ERROR_CORRUPTION;
        }
        if (meta->next_offset > meta->total_size) {
            return CNS_8T_ERROR_CORRUPTION;
        }
        
        // Validate free lists
        for (int j = 0; j < 4; j++) {
            cns_8t_arena_block_t* block = meta->free_lists[j];
            int count = 0;
            while (block) {
                if (block->magic != ARENA_MAGIC_FREE) {
                    return CNS_8T_ERROR_CORRUPTION;
                }
                if (block->arena_id != i) {
                    return CNS_8T_ERROR_CORRUPTION;
                }
                if (++count > 1000000) {  // Detect cycles
                    return CNS_8T_ERROR_CORRUPTION;
                }
                block = block->next;
            }
        }
    }
    
    return CNS_8T_SUCCESS;
}

cns_8t_result_t cns_8t_arena_l1_dump_state(cns_8t_arena_l1_t* arena,
                                            char* buffer,
                                            size_t buffer_size) {
    if (!arena || !buffer || buffer_size == 0) {
        return CNS_8T_ERROR_INVALID_ARGUMENT;
    }
    
    size_t offset = 0;
    
    // Header
    offset += snprintf(buffer + offset, buffer_size - offset,
                      "=== L1-Optimized Arena State ===\n");
    offset += snprintf(buffer + offset, buffer_size - offset,
                      "Arenas: %u, Active: %u\n", 
                      arena->num_arenas, arena->active_arena);
    offset += snprintf(buffer + offset, buffer_size - offset,
                      "L1 Hit Rate: %.2f%%\n",
                      arena->total_cycles > 0 ? 
                      100.0 * arena->l1_utilization_cycles / arena->total_cycles : 0.0);
    
    // Per-arena statistics
    for (uint32_t i = 0; i < arena->num_arenas; i++) {
        cns_8t_arena_metadata_t* meta = &arena->arenas[i];
        offset += snprintf(buffer + offset, buffer_size - offset,
                          "\nArena %u: %zu/%zu bytes (%.1f%%)\n",
                          i, meta->used_size, meta->total_size,
                          100.0 * meta->used_size / meta->total_size);
        offset += snprintf(buffer + offset, buffer_size - offset,
                          "  Allocations: %u, Cache Hits: %u\n",
                          meta->allocations, meta->cache_hits);
        
        // Free list counts
        for (int j = 0; j < 4; j++) {
            int count = 0;
            cns_8t_arena_block_t* block = meta->free_lists[j];
            while (block) {
                count++;
                block = block->next;
            }
            if (count > 0) {
                offset += snprintf(buffer + offset, buffer_size - offset,
                                  "  Free list %d: %d blocks\n", j, count);
            }
        }
    }
    
    return CNS_8T_SUCCESS;
}

cns_8t_result_t cns_8t_arena_l1_check_corruption(cns_8t_arena_l1_t* arena) {
    return cns_8t_arena_l1_validate(arena);
}

#endif // CNS_8T_DEBUG