/*
 * CNS 8M Memory Quantum System
 * 
 * Implements the 8-Memory contract that quantizes all memory to perfect 8-byte alignment.
 * This provides optimal CPU cache performance, eliminates fragmentation, and enables
 * efficient SIMD operations by ensuring all data structures are multiples of 8 bytes.
 * 
 * Key Features:
 * - All allocations align to 8-byte boundaries
 * - All data structures are 8-byte multiples
 * - Zero memory fragmentation
 * - Perfect cache-line utilization (64 bytes = 8 quantum units)
 * - Hardware-optimized memory access patterns
 * - Integration with 7-tick SIMD operations
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdalign.h>
#include <assert.h>
#include <time.h>
#ifdef __x86_64__
#include <immintrin.h>
#elif defined(__aarch64__) || defined(__arm64__)
#include <arm_neon.h>
#else
#error "Unsupported architecture - requires x86_64 or ARM64"
#endif

// 8M Quantum Constants
#define QUANTUM_SIZE        8           // Base quantum unit (8 bytes)
#define QUANTUM_MASK        0x7         // Mask for alignment check
#define QUANTUM_SHIFT       3           // Log2(8) for fast division
#define CACHE_LINE_SIZE     64          // Standard cache line
#define CACHE_LINE_QUANTA   8           // Cache line in quantum units

// Memory alignment macros
#define ALIGN_UP_8(x)       (((x) + 7) & ~7)
#define ALIGN_DOWN_8(x)     ((x) & ~7)
#define IS_ALIGNED_8(x)     (((uintptr_t)(x) & 7) == 0)
#define QUANTA_COUNT(x)     (((x) + 7) >> 3)

// Compile-time assertions for structure alignment
_Static_assert(sizeof(void*) == 8, "8M requires 64-bit architecture");
// Platform must support 8-byte alignment (all modern 64-bit systems do)

// 8M-aligned memory block header
typedef struct {
    uint64_t size_quanta;    // Size in 8-byte quantum units
    uint64_t magic;          // Magic number for validation
} __attribute__((aligned(8))) quantum_header_t;

// 8M-aligned node structure (24 bytes = 3 quanta)
typedef struct {
    uint64_t id;             // 8 bytes
    uint32_t type;           // 4 bytes
    uint32_t flags;          // 4 bytes
    uint64_t data_offset;    // 8 bytes - offset in quantum pool
} __attribute__((aligned(8))) quantum_node_t;

// Use public type names for API
typedef quantum_node_t cns_8m_node_t;

_Static_assert(sizeof(quantum_node_t) == 24, "Node must be exactly 3 quanta");
_Static_assert(sizeof(quantum_node_t) % QUANTUM_SIZE == 0, "Node must be quantum-aligned");

// 8M-aligned edge structure (32 bytes = 4 quanta)
typedef struct {
    uint64_t source;         // 8 bytes
    uint64_t target;         // 8 bytes
    uint32_t type;           // 4 bytes
    float weight;            // 4 bytes (more efficient than double)
    uint64_t data_offset;    // 8 bytes - offset in quantum pool
} __attribute__((aligned(8))) quantum_edge_t;

// Use public type names for API
typedef quantum_edge_t cns_8m_edge_t;

_Static_assert(sizeof(quantum_edge_t) == 32, "Edge must be exactly 4 quanta");
_Static_assert(sizeof(quantum_edge_t) % QUANTUM_SIZE == 0, "Edge must be quantum-aligned");

// Quantum memory pool for zero-fragmentation allocation
typedef struct {  // Align to cache line
    uint8_t* base;           // Base memory pointer
    uint64_t size_quanta;    // Total size in quantum units
    uint64_t used_quanta;    // Used quantum units
    uint64_t free_list;      // Head of free list (quantum offset)
    uint64_t allocation_count;
    uint64_t fragmentation_prevented;
    uint64_t cache_hits;
    uint64_t simd_operations;
} __attribute__((aligned(64))) quantum_pool_t;

_Static_assert(sizeof(quantum_pool_t) == 64, "Pool header must fit cache line");

// Quantum allocator statistics
typedef struct {
    uint64_t total_allocated;
    uint64_t total_freed;
    uint64_t alignment_corrections;
    uint64_t cache_line_aligned;
    uint64_t simd_ready_blocks;
    uint64_t fragmentation_saved;
    uint64_t peak_usage;
    uint64_t quantum_efficiency;  // Percentage * 100
    uint64_t simd_operations;     // Count of SIMD operations
} __attribute__((aligned(64))) quantum_stats_t;

// Use public type names for API
typedef quantum_stats_t cns_8m_stats_t;

// Global quantum pools
static quantum_pool_t* node_pool = NULL;
static quantum_pool_t* edge_pool = NULL;
static quantum_pool_t* data_pool = NULL;
static quantum_stats_t stats = {0};

// Magic numbers for validation
#define QUANTUM_MAGIC       0x384D454D4F5259ULL  // "8MEMORY"
#define POOL_MAGIC         0x384D504F4F4C00ULL  // "8MPOOL"

// Initialize quantum memory pool
static quantum_pool_t* quantum_pool_create(size_t initial_size) {
    // Ensure size is cache-line aligned
    size_t aligned_size = ALIGN_UP_8(initial_size);
    size_t cache_lines = (aligned_size + CACHE_LINE_SIZE - 1) / CACHE_LINE_SIZE;
    aligned_size = cache_lines * CACHE_LINE_SIZE;
    
    // Allocate aligned memory
    void* mem = NULL;
    int ret = posix_memalign(&mem, CACHE_LINE_SIZE, aligned_size);
    if (ret != 0 || !mem) {
        return NULL;
    }
    
    // Initialize pool header in first cache line
    quantum_pool_t* pool = (quantum_pool_t*)mem;
    pool->base = (uint8_t*)mem + CACHE_LINE_SIZE;  // Data starts after header
    pool->size_quanta = (aligned_size - CACHE_LINE_SIZE) / QUANTUM_SIZE;
    pool->used_quanta = 0;
    pool->free_list = 0;  // No free blocks initially
    pool->allocation_count = 0;
    pool->fragmentation_prevented = 0;
    pool->cache_hits = 0;
    pool->simd_operations = 0;
    
    // Clear the data area using SIMD
#ifdef __x86_64__
    __m256i zero = _mm256_setzero_si256();
    __m256i* ptr = (__m256i*)pool->base;
    size_t vectors = pool->size_quanta * QUANTUM_SIZE / 32;
    for (size_t i = 0; i < vectors; i++) {
        _mm256_store_si256(ptr + i, zero);
    }
#elif defined(__aarch64__) || defined(__arm64__)
    uint8x16_t zero = vdupq_n_u8(0);
    uint8x16_t* ptr = (uint8x16_t*)pool->base;
    size_t vectors = pool->size_quanta * QUANTUM_SIZE / 16;
    for (size_t i = 0; i < vectors; i++) {
        vst1q_u8((uint8_t*)(ptr + i), zero);
    }
#else
    memset(pool->base, 0, pool->size_quanta * QUANTUM_SIZE);
#endif
    
    stats.cache_line_aligned++;
    return pool;
}

// Allocate quantum-aligned memory from pool
static void* quantum_alloc(quantum_pool_t* pool, size_t size) {
    if (!pool || size == 0) return NULL;
    
    // Calculate required quanta
    size_t quanta_needed = QUANTA_COUNT(size);
    
    // Check if we have space
    if (pool->used_quanta + quanta_needed > pool->size_quanta) {
        return NULL;  // Pool exhausted
    }
    
    // Allocate from pool
    uint64_t offset = pool->used_quanta * QUANTUM_SIZE;
    pool->used_quanta += quanta_needed;
    pool->allocation_count++;
    
    // Update statistics
    stats.total_allocated += quanta_needed * QUANTUM_SIZE;
    if (pool->used_quanta > stats.peak_usage) {
        stats.peak_usage = pool->used_quanta;
    }
    
    // Check if allocation is cache-line aligned
    if ((offset & (CACHE_LINE_SIZE - 1)) == 0) {
        stats.cache_line_aligned++;
    }
    
    // Return aligned pointer
    void* ptr = pool->base + offset;
    assert(IS_ALIGNED_8(ptr));
    return ptr;
}

// Custom aligned allocation with validation
void* cns_8m_alloc(size_t size) {
    if (size == 0) return NULL;
    
    // Add header space
    size_t total_size = sizeof(quantum_header_t) + ALIGN_UP_8(size);
    
    // Allocate aligned memory
    void* mem = NULL;
    int ret = posix_memalign(&mem, QUANTUM_SIZE, total_size);
    if (ret != 0 || !mem) {
        return NULL;
    }
    
    // Initialize header
    quantum_header_t* header = (quantum_header_t*)mem;
    header->size_quanta = QUANTA_COUNT(size);
    header->magic = QUANTUM_MAGIC;
    
    // Update statistics
    stats.total_allocated += total_size;
    if (total_size >= CACHE_LINE_SIZE) {
        stats.cache_line_aligned++;
    }
    stats.simd_ready_blocks++;
    
    // Return user pointer (after header)
    return (uint8_t*)mem + sizeof(quantum_header_t);
}

// Free quantum-aligned memory with validation
void cns_8m_free(void* ptr) {
    if (!ptr) return;
    
    // Get header
    quantum_header_t* header = (quantum_header_t*)((uint8_t*)ptr - sizeof(quantum_header_t));
    
    // Validate magic
    if (header->magic != QUANTUM_MAGIC) {
        fprintf(stderr, "8M: Invalid magic in free - memory corruption!\n");
        abort();
    }
    
    // Update statistics
    stats.total_freed += header->size_quanta * QUANTUM_SIZE;
    
    // Clear magic and free
    header->magic = 0;
    free(header);
}

// Reallocate with quantum alignment preservation
void* cns_8m_realloc(void* ptr, size_t new_size) {
    if (!ptr) return cns_8m_alloc(new_size);
    if (new_size == 0) {
        cns_8m_free(ptr);
        return NULL;
    }
    
    // Get old size from header
    quantum_header_t* header = (quantum_header_t*)((uint8_t*)ptr - sizeof(quantum_header_t));
    if (header->magic != QUANTUM_MAGIC) {
        fprintf(stderr, "8M: Invalid magic in realloc - memory corruption!\n");
        abort();
    }
    
    size_t old_size = header->size_quanta * QUANTUM_SIZE;
    
    // Allocate new block
    void* new_ptr = cns_8m_alloc(new_size);
    if (!new_ptr) return NULL;
    
    // Copy data
    size_t copy_size = (old_size < new_size) ? old_size : new_size;
    memcpy(new_ptr, ptr, copy_size);
    
    // Free old block
    cns_8m_free(ptr);
    
    return new_ptr;
}

// Validate alignment of a pointer
int cns_8m_validate_alignment(const void* ptr) {
    if (!ptr) return 0;
    
    if (!IS_ALIGNED_8(ptr)) {
        stats.alignment_corrections++;
        return 0;
    }
    
    return 1;
}

// Initialize the 8M memory system
int cns_8m_init(size_t node_pool_size, size_t edge_pool_size, size_t data_pool_size) {
    // Create quantum pools
    node_pool = quantum_pool_create(node_pool_size);
    if (!node_pool) return -1;
    
    edge_pool = quantum_pool_create(edge_pool_size);
    if (!edge_pool) {
        free(node_pool);
        return -1;
    }
    
    data_pool = quantum_pool_create(data_pool_size);
    if (!data_pool) {
        free(node_pool);
        free(edge_pool);
        return -1;
    }
    
    printf("8M Memory System Initialized:\n");
    printf("  Node Pool: %zu quanta (%zu KB)\n", 
           (size_t)node_pool->size_quanta, (size_t)(node_pool->size_quanta * QUANTUM_SIZE / 1024));
    printf("  Edge Pool: %zu quanta (%zu KB)\n", 
           (size_t)edge_pool->size_quanta, (size_t)(edge_pool->size_quanta * QUANTUM_SIZE / 1024));
    printf("  Data Pool: %zu quanta (%zu KB)\n", 
           (size_t)data_pool->size_quanta, (size_t)(data_pool->size_quanta * QUANTUM_SIZE / 1024));
    printf("  All pools aligned to %d-byte cache lines\n", CACHE_LINE_SIZE);
    
    return 0;
}

// Allocate quantum-aligned node
cns_8m_node_t* cns_8m_alloc_node(void) {
    cns_8m_node_t* node = (cns_8m_node_t*)quantum_alloc(node_pool, sizeof(cns_8m_node_t));
    if (node) {
        // Verify alignment
        assert(IS_ALIGNED_8(node));
        assert(sizeof(cns_8m_node_t) == 3 * QUANTUM_SIZE);
    }
    return node;
}

// Allocate quantum-aligned edge
cns_8m_edge_t* cns_8m_alloc_edge(void) {
    cns_8m_edge_t* edge = (cns_8m_edge_t*)quantum_alloc(edge_pool, sizeof(cns_8m_edge_t));
    if (edge) {
        // Verify alignment
        assert(IS_ALIGNED_8(edge));
        assert(sizeof(cns_8m_edge_t) == 4 * QUANTUM_SIZE);
    }
    return edge;
}

// Allocate quantum-aligned data
void* cns_8m_alloc_data(size_t size) {
    return quantum_alloc(data_pool, size);
}

// SIMD-optimized memory copy (8-byte aligned)
void cns_8m_memcpy(void* dst, const void* src, size_t size) {
    assert(IS_ALIGNED_8(dst));
    assert(IS_ALIGNED_8(src));
    
#ifdef __x86_64__
    // Use AVX2 for large copies on x86_64
    if (size >= 32) {
        __m256i* d = (__m256i*)dst;
        const __m256i* s = (const __m256i*)src;
        size_t vectors = size / 32;
        
        for (size_t i = 0; i < vectors; i++) {
            _mm256_store_si256(d + i, _mm256_load_si256(s + i));
        }
        
        stats.simd_operations += vectors;
        
        // Handle remainder
        size_t remainder = size & 31;
        if (remainder) {
            memcpy((uint8_t*)dst + vectors * 32, (const uint8_t*)src + vectors * 32, remainder);
        }
    } else {
        memcpy(dst, src, size);
    }
#elif defined(__aarch64__) || defined(__arm64__)
    // Use NEON for large copies on ARM64
    if (size >= 16) {
        uint8x16_t* d = (uint8x16_t*)dst;
        const uint8x16_t* s = (const uint8x16_t*)src;
        size_t vectors = size / 16;
        
        for (size_t i = 0; i < vectors; i++) {
            vst1q_u8((uint8_t*)(d + i), vld1q_u8((const uint8_t*)(s + i)));
        }
        
        stats.simd_operations += vectors;
        
        // Handle remainder
        size_t remainder = size & 15;
        if (remainder) {
            memcpy((uint8_t*)dst + vectors * 16, (const uint8_t*)src + vectors * 16, remainder);
        }
    } else {
        memcpy(dst, src, size);
    }
#else
    memcpy(dst, src, size);
#endif
}

// SIMD-optimized memory clear (8-byte aligned)
void cns_8m_memset(void* dst, int value, size_t size) {
    assert(IS_ALIGNED_8(dst));
    
#ifdef __x86_64__
    // Use AVX2 for large clears on x86_64
    if (size >= 32) {
        __m256i val = _mm256_set1_epi8((char)value);
        __m256i* d = (__m256i*)dst;
        size_t vectors = size / 32;
        
        for (size_t i = 0; i < vectors; i++) {
            _mm256_store_si256(d + i, val);
        }
        
        stats.simd_operations += vectors;
        
        // Handle remainder
        size_t remainder = size & 31;
        if (remainder) {
            memset((uint8_t*)dst + vectors * 32, value, remainder);
        }
    } else {
        memset(dst, value, size);
    }
#elif defined(__aarch64__) || defined(__arm64__)
    // Use NEON for large clears on ARM64
    if (size >= 16) {
        uint8x16_t val = vdupq_n_u8((uint8_t)value);
        uint8x16_t* d = (uint8x16_t*)dst;
        size_t vectors = size / 16;
        
        for (size_t i = 0; i < vectors; i++) {
            vst1q_u8((uint8_t*)(d + i), val);
        }
        
        stats.simd_operations += vectors;
        
        // Handle remainder
        size_t remainder = size & 15;
        if (remainder) {
            memset((uint8_t*)dst + vectors * 16, value, remainder);
        }
    } else {
        memset(dst, value, size);
    }
#else
    memset(dst, value, size);
#endif
}

// Get memory statistics
void cns_8m_get_stats(cns_8m_stats_t* out_stats) {
    if (out_stats) {
        *out_stats = stats;
        
        // Calculate efficiency
        if (stats.total_allocated > 0) {
            uint64_t useful = stats.total_allocated - stats.alignment_corrections * QUANTUM_SIZE;
            stats.quantum_efficiency = (useful * 100) / stats.total_allocated;
        }
    }
}

// Print memory statistics
void cns_8m_print_stats(void) {
    printf("\n8M Memory Statistics:\n");
    printf("  Total Allocated: %zu KB\n", (size_t)(stats.total_allocated / 1024));
    printf("  Total Freed: %zu KB\n", (size_t)(stats.total_freed / 1024));
    printf("  Peak Usage: %zu quanta\n", (size_t)stats.peak_usage);
    printf("  Cache-Line Aligned: %zu blocks\n", (size_t)stats.cache_line_aligned);
    printf("  SIMD Operations: %zu\n", (size_t)stats.simd_operations);
    printf("  Alignment Corrections: %zu\n", (size_t)stats.alignment_corrections);
    printf("  Fragmentation Prevented: %zu bytes\n", (size_t)stats.fragmentation_saved);
    printf("  Quantum Efficiency: %zu%%\n", (size_t)stats.quantum_efficiency);
    
    if (node_pool) {
        printf("\nNode Pool Usage: %zu/%zu quanta (%.1f%%)\n",
               (size_t)node_pool->used_quanta, (size_t)node_pool->size_quanta,
               100.0 * node_pool->used_quanta / node_pool->size_quanta);
    }
    
    if (edge_pool) {
        printf("Edge Pool Usage: %zu/%zu quanta (%.1f%%)\n",
               (size_t)edge_pool->used_quanta, (size_t)edge_pool->size_quanta,
               100.0 * edge_pool->used_quanta / edge_pool->size_quanta);
    }
    
    if (data_pool) {
        printf("Data Pool Usage: %zu/%zu quanta (%.1f%%)\n",
               (size_t)data_pool->used_quanta, (size_t)data_pool->size_quanta,
               100.0 * data_pool->used_quanta / data_pool->size_quanta);
    }
}

// Cleanup the 8M memory system
void cns_8m_cleanup(void) {
    if (node_pool) {
        free((void*)((uint8_t*)node_pool - CACHE_LINE_SIZE));
        node_pool = NULL;
    }
    
    if (edge_pool) {
        free((void*)((uint8_t*)edge_pool - CACHE_LINE_SIZE));
        edge_pool = NULL;
    }
    
    if (data_pool) {
        free((void*)((uint8_t*)data_pool - CACHE_LINE_SIZE));
        data_pool = NULL;
    }
    
    cns_8m_print_stats();
}

// Benchmark the 8M memory system
void cns_8m_benchmark(void) {
    printf("\n8M Memory System Benchmark:\n");
    printf("================================\n");
    
    // Test alignment validation
    void* test_ptr = cns_8m_alloc(1000);
    printf("Allocation alignment: %s\n", 
           cns_8m_validate_alignment(test_ptr) ? "VALID" : "INVALID");
    
    // Test quantum calculations
    printf("\nQuantum calculations:\n");
    for (size_t size = 1; size <= 64; size *= 2) {
        printf("  Size %2zu bytes = %zu quanta\n", size, QUANTA_COUNT(size));
    }
    
    // Test structure sizes
    printf("\nStructure validation:\n");
    printf("  quantum_node_t: %zu bytes = %zu quanta %s\n",
           sizeof(quantum_node_t), sizeof(quantum_node_t) / QUANTUM_SIZE,
           sizeof(quantum_node_t) % QUANTUM_SIZE == 0 ? "✓" : "✗");
    printf("  quantum_edge_t: %zu bytes = %zu quanta %s\n",
           sizeof(quantum_edge_t), sizeof(quantum_edge_t) / QUANTUM_SIZE,
           sizeof(quantum_edge_t) % QUANTUM_SIZE == 0 ? "✓" : "✗");
    printf("  quantum_pool_t: %zu bytes = %zu cache lines %s\n",
           sizeof(quantum_pool_t), sizeof(quantum_pool_t) / CACHE_LINE_SIZE,
           sizeof(quantum_pool_t) == CACHE_LINE_SIZE ? "✓" : "✗");
    
    // Benchmark allocations
    const int iterations = 100000;
    clock_t start = clock();
    
    for (int i = 0; i < iterations; i++) {
        quantum_node_t* node = cns_8m_alloc_node();
        if (!node) break;
        node->id = i;
        node->type = i % 256;
        node->flags = 0;
    }
    
    clock_t end = clock();
    double seconds = (double)(end - start) / CLOCKS_PER_SEC;
    printf("\nPerformance:\n");
    printf("  Allocated %d nodes in %.3f seconds\n", iterations, seconds);
    printf("  Rate: %.0f allocations/second\n", iterations / seconds);
    printf("  Average: %.2f nanoseconds/allocation\n", seconds * 1e9 / iterations);
    
    cns_8m_free(test_ptr);
}

// Integration test with existing CNS structures
void cns_8m_integration_test(void) {
    printf("\n8M Integration Test:\n");
    printf("====================\n");
    
    // Initialize system
    cns_8m_init(1024 * 1024, 2 * 1024 * 1024, 4 * 1024 * 1024);
    
    // Create quantum-aligned graph
    const int node_count = 1000;
    const int edges_per_node = 10;
    
    printf("Creating graph with %d nodes and ~%d edges\n", 
           node_count, node_count * edges_per_node);
    
    // Allocate nodes
    quantum_node_t** nodes = cns_8m_alloc(node_count * sizeof(quantum_node_t*));
    for (int i = 0; i < node_count; i++) {
        nodes[i] = cns_8m_alloc_node();
        nodes[i]->id = i;
        nodes[i]->type = i % 16;
        nodes[i]->flags = 0;
        nodes[i]->data_offset = 0;
    }
    
    // Allocate edges
    int edge_count = 0;
    for (int i = 0; i < node_count; i++) {
        for (int j = 0; j < edges_per_node; j++) {
            quantum_edge_t* edge = cns_8m_alloc_edge();
            if (!edge) break;
            
            edge->source = i;
            edge->target = (i + j + 1) % node_count;
            edge->type = j;
            edge->weight = 1.0f / (j + 1);
            edge->data_offset = 0;
            edge_count++;
        }
    }
    
    printf("Created %d edges\n", edge_count);
    
    // Test SIMD operations
    void* src_data = cns_8m_alloc_data(CACHE_LINE_SIZE * 100);
    void* dst_data = cns_8m_alloc_data(CACHE_LINE_SIZE * 100);
    
    if (src_data && dst_data) {
        // Fill source with pattern
        cns_8m_memset(src_data, 0xAB, CACHE_LINE_SIZE * 100);
        
        // SIMD copy
        cns_8m_memcpy(dst_data, src_data, CACHE_LINE_SIZE * 100);
        
        // Verify
        if (memcmp(src_data, dst_data, CACHE_LINE_SIZE * 100) == 0) {
            printf("SIMD copy verified ✓\n");
        }
    }
    
    // Cleanup
    cns_8m_free(nodes);
    cns_8m_cleanup();
}

// Main entry point for testing
// Main entry point moved to test file