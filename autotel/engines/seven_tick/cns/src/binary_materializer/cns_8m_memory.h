/*
 * CNS 8M Memory Quantum System - Header
 * 
 * Public interface for the 8-Memory quantum allocation system.
 * All memory operations are guaranteed to be 8-byte aligned for
 * optimal hardware performance and SIMD compatibility.
 */

#ifndef CNS_8M_MEMORY_H
#define CNS_8M_MEMORY_H

#include <stdint.h>
#include <stddef.h>
#include <stdalign.h>

#ifdef __cplusplus
extern "C" {
#endif

// 8M Quantum Constants
#define CNS_8M_QUANTUM_SIZE        8     // Base quantum unit (8 bytes)
#define CNS_8M_CACHE_LINE_SIZE     64    // Standard cache line
#define CNS_8M_CACHE_LINE_QUANTA   8     // Cache line in quantum units

// Alignment macros for public use
#define CNS_8M_ALIGN_UP(x)         (((x) + 7) & ~7)
#define CNS_8M_ALIGN_DOWN(x)       ((x) & ~7)
#define CNS_8M_IS_ALIGNED(x)       (((uintptr_t)(x) & 7) == 0)
#define CNS_8M_QUANTA_COUNT(x)     (((x) + 7) >> 3)

// Forward declarations - structures defined in implementation
typedef struct cns_8m_node cns_8m_node_t;
typedef struct cns_8m_edge cns_8m_edge_t; 
typedef struct cns_8m_stats cns_8m_stats_t;

/*
 * Initialize the 8M memory system with quantum pools.
 * 
 * @param node_pool_size Size in bytes for node allocation pool
 * @param edge_pool_size Size in bytes for edge allocation pool
 * @param data_pool_size Size in bytes for general data pool
 * @return 0 on success, -1 on failure
 */
int cns_8m_init(size_t node_pool_size, size_t edge_pool_size, size_t data_pool_size);

/*
 * Cleanup the 8M memory system and release all resources.
 * Prints final statistics before cleanup.
 */
void cns_8m_cleanup(void);

/*
 * Allocate quantum-aligned memory.
 * 
 * @param size Number of bytes to allocate
 * @return Pointer to 8-byte aligned memory, or NULL on failure
 */
void* cns_8m_alloc(size_t size);

/*
 * Free quantum-aligned memory.
 * 
 * @param ptr Pointer returned by cns_8m_alloc
 */
void cns_8m_free(void* ptr);

/*
 * Reallocate quantum-aligned memory.
 * 
 * @param ptr Original pointer (may be NULL)
 * @param new_size New size in bytes
 * @return Pointer to reallocated memory, or NULL on failure
 */
void* cns_8m_realloc(void* ptr, size_t new_size);

/*
 * Allocate a quantum-aligned node from the node pool.
 * 
 * @return Pointer to initialized node, or NULL if pool exhausted
 */
cns_8m_node_t* cns_8m_alloc_node(void);

/*
 * Allocate a quantum-aligned edge from the edge pool.
 * 
 * @return Pointer to initialized edge, or NULL if pool exhausted
 */
cns_8m_edge_t* cns_8m_alloc_edge(void);

/*
 * Allocate quantum-aligned data from the data pool.
 * 
 * @param size Size in bytes
 * @return Pointer to data, or NULL if pool exhausted
 */
void* cns_8m_alloc_data(size_t size);

/*
 * SIMD-optimized memory copy for 8-byte aligned data.
 * Uses AVX2 instructions for large copies.
 * 
 * @param dst Destination (must be 8-byte aligned)
 * @param src Source (must be 8-byte aligned)
 * @param size Number of bytes to copy
 */
void cns_8m_memcpy(void* dst, const void* src, size_t size);

/*
 * SIMD-optimized memory set for 8-byte aligned data.
 * Uses AVX2 instructions for large sets.
 * 
 * @param dst Destination (must be 8-byte aligned)
 * @param value Byte value to set
 * @param size Number of bytes to set
 */
void cns_8m_memset(void* dst, int value, size_t size);

/*
 * Validate that a pointer is 8-byte aligned.
 * 
 * @param ptr Pointer to validate
 * @return 1 if aligned, 0 if not aligned or NULL
 */
int cns_8m_validate_alignment(const void* ptr);

/*
 * Get current memory statistics.
 * 
 * @param out_stats Pointer to stats structure to fill
 */
void cns_8m_get_stats(cns_8m_stats_t* out_stats);

/*
 * Print detailed memory statistics to stdout.
 */
void cns_8m_print_stats(void);

/*
 * Run benchmark tests on the 8M memory system.
 */
void cns_8m_benchmark(void);

/*
 * Run integration tests with CNS structures.
 */
void cns_8m_integration_test(void);

// Compile-time assertions for structure sizes
_Static_assert(sizeof(cns_8m_node_t) == 24, "Node must be exactly 24 bytes (3 quanta)");
_Static_assert(sizeof(cns_8m_edge_t) == 32, "Edge must be exactly 32 bytes (4 quanta)");
_Static_assert(sizeof(cns_8m_node_t) % CNS_8M_QUANTUM_SIZE == 0, "Node must be quantum-aligned");
_Static_assert(sizeof(cns_8m_edge_t) % CNS_8M_QUANTUM_SIZE == 0, "Edge must be quantum-aligned");

#ifdef __cplusplus
}
#endif

#endif /* CNS_8M_MEMORY_H */