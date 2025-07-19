#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/cns/optimization_helpers.h"

// ============================================================================
// EXAMPLE 1: SIMPLE CACHE-OPTIMIZED DATA STRUCTURE
// ============================================================================

// Cache-optimized structure using helper macros
typedef struct {
    uint32_t data[16];           // 64 bytes - fits in L1 cache
    uint32_t count;
    CNS_CACHE_PAD(sizeof(uint32_t) * 17); // Pad to cache line
} CacheOptimizedArray;

// ============================================================================
// EXAMPLE 2: SIMD-VECTORIZED ARRAY OPERATIONS
// ============================================================================

// Vectorized array operations using helper utilities
void vectorized_array_add(const uint32_t* a, const uint32_t* b, uint32_t* result, size_t count) {
    printf("Vectorized Array Addition (Vector Width: %d)\n", CNS_VECTOR_WIDTH);
    
    // Use SIMD batch processing helper macros
    CNS_SIMD_BATCH_START(count);
    
    // Process vectorized batch
    for (size_t i = 0; i < cns_simd_count; i += CNS_VECTOR_WIDTH) {
        // Load vectors using helper macros
        CNS_VECTOR_TYPE va = CNS_VECTOR_LOAD((CNS_VECTOR_TYPE*)&a[i]);
        CNS_VECTOR_TYPE vb = CNS_VECTOR_LOAD((CNS_VECTOR_TYPE*)&b[i]);
        
        // Process each element in the vector
        for (int j = 0; j < CNS_VECTOR_WIDTH; j++) {
            uint32_t val_a = CNS_VECTOR_EXTRACT(va, j);
            uint32_t val_b = CNS_VECTOR_EXTRACT(vb, j);
            result[i + j] = val_a + val_b;
        }
        
        // Prefetch next batch using helper function
        cns_prefetch_read(&a[i + CNS_VECTOR_WIDTH]);
        cns_prefetch_read(&b[i + CNS_VECTOR_WIDTH]);
    }
    
    // Handle remaining elements using helper macro
    CNS_SIMD_BATCH_END(count) {
        result[cns_i] = a[cns_i] + b[cns_i];
    }
}

// ============================================================================
// EXAMPLE 3: PERFORMANCE-MONITORED OPERATIONS
// ============================================================================

// Performance-monitored function using helper utilities
void performance_monitored_operation(CNSPerformanceCounters* counters, size_t iterations) {
    printf("Performance-Monitored Operation (%zu iterations)\n", iterations);
    
    for (size_t i = 0; i < iterations; i++) {
        // Performance measurement using helper macros
        CNS_PERFORMANCE_START(counters);
        
        // Simulate some work
        volatile uint64_t dummy = i * i;
        (void)dummy;
        
        // Record performance using helper macro
        CNS_PERFORMANCE_END(counters, i % 2); // Alternate cache hits/misses
    }
    
    // Get performance statistics using helper function
    uint64_t total_ops, cache_hits, cache_misses;
    double avg_time_ns;
    cns_performance_get_stats(counters, &total_ops, &cache_hits, &cache_misses, &avg_time_ns);
    
    printf("Performance Results:\n");
    printf("  Total Operations: %lu\n", total_ops);
    printf("  Cache Hit Rate: %.1f%%\n", 
           total_ops > 0 ? (double)cache_hits / total_ops * 100 : 0.0);
    printf("  Average Latency: %.1f ns\n", avg_time_ns);
    printf("  Throughput: %.1f ops/sec\n", 
           avg_time_ns > 0 ? 1000000000.0 / avg_time_ns : 0.0);
}

// ============================================================================
// EXAMPLE 4: LOCK-FREE RING BUFFER OPERATIONS
// ============================================================================

// Lock-free operations using helper utilities
void lock_free_operations_example(size_t operations) {
    printf("Lock-Free Ring Buffer Operations (%zu operations)\n", operations);
    
    // Create ring buffer using helper function
    CNSLockFreeRing* ring = cns_ring_create(1024);
    if (!ring) {
        printf("Failed to create ring buffer\n");
        return;
    }
    
    // Push operations using helper function
    uint64_t push_count = 0;
    for (size_t i = 0; i < operations; i++) {
        if (cns_ring_push(ring, i)) {
            push_count++;
        }
    }
    
    // Pop operations using helper function
    uint64_t pop_count = 0;
    uint64_t value;
    while (cns_ring_pop(ring, &value)) {
        pop_count++;
    }
    
    printf("Ring Buffer Results:\n");
    printf("  Pushed: %lu operations\n", push_count);
    printf("  Popped: %lu operations\n", pop_count);
    printf("  Success Rate: %.1f%%\n", 
           operations > 0 ? (double)push_count / operations * 100 : 0.0);
    
    // Clean up using helper function
    cns_ring_destroy(ring);
}

// ============================================================================
// EXAMPLE 5: MEMORY POOL ALLOCATION
// ============================================================================

// Memory pool usage using helper utilities
void memory_pool_example(size_t allocations) {
    printf("Memory Pool Allocation (%zu allocations)\n", allocations);
    
    // Create memory pool using helper function
    CNSMemoryPool* pool = cns_memory_pool_create(1024 * 1024, 64); // 1MB, 64-byte blocks
    if (!pool) {
        printf("Failed to create memory pool\n");
        return;
    }
    
    // Allocate blocks using helper function
    uint64_t alloc_count = 0;
    for (size_t i = 0; i < allocations; i++) {
        if (cns_memory_pool_alloc(pool)) {
            alloc_count++;
        }
    }
    
    printf("Memory Pool Results:\n");
    printf("  Allocated: %lu blocks\n", alloc_count);
    printf("  Success Rate: %.1f%%\n", 
           allocations > 0 ? (double)alloc_count / allocations * 100 : 0.0);
    printf("  Memory Used: %zu bytes\n", pool->used);
    
    // Clean up using helper function
    cns_memory_pool_destroy(pool);
}

// ============================================================================
// EXAMPLE 6: BIT VECTOR OPERATIONS
// ============================================================================

// Bit vector operations using helper utilities
void bit_vector_example(size_t operations) {
    printf("Bit Vector Operations (%zu operations)\n", operations);
    
    // Allocate bit vector using helper function
    size_t word_count = (operations + 63) / 64;
    uint64_t* bit_vector = cns_cache_aligned_alloc(word_count * sizeof(uint64_t));
    memset(bit_vector, 0, word_count * sizeof(uint64_t));
    
    // Set bits using helper function
    for (size_t i = 0; i < operations; i++) {
        cns_bit_vector_set(bit_vector, i);
    }
    
    // Test bits using helper function
    uint64_t test_count = 0;
    for (size_t i = 0; i < operations; i++) {
        if (cns_bit_vector_test(bit_vector, i)) {
            test_count++;
        }
    }
    
    // Get population count using helper function
    size_t pop_count = cns_bit_vector_popcount(bit_vector, word_count);
    
    printf("Bit Vector Results:\n");
    printf("  Set Operations: %zu\n", operations);
    printf("  Test Operations: %zu\n", operations);
    printf("  Test Success Rate: %.1f%%\n", 
           operations > 0 ? (double)test_count / operations * 100 : 0.0);
    printf("  Population Count: %zu\n", pop_count);
    
    free(bit_vector);
}

// ============================================================================
// EXAMPLE 7: CACHE MANAGER OPERATIONS
// ============================================================================

// Cache manager operations using helper utilities
void cache_manager_example(size_t operations) {
    printf("Cache Manager Operations (%zu operations)\n", operations);
    
    // Create cache manager using helper function
    CNSCacheManager* manager = cns_cache_manager_create(3); // L1, L2, L3
    if (!manager) {
        printf("Failed to create cache manager\n");
        return;
    }
    
    // Simulate cache operations
    uint64_t l1_hits = 0, l2_hits = 0, misses = 0;
    
    for (size_t i = 0; i < operations; i++) {
        uint64_t key = i % 1000;
        uint64_t value = i;
        
        // Check L1 cache using helper function
        if (cns_cache_l1_check(&manager->tiers[0], key, value)) {
            l1_hits++;
        }
        // Check L2 cache using helper function
        else if (cns_cache_l2_check(&manager->tiers[1], key, value)) {
            l2_hits++;
            // Promote to L1 using helper function
            cns_cache_l1_update(&manager->tiers[0], key, value);
        }
        else {
            misses++;
            // Update L2 cache using helper function
            cns_cache_l2_update(&manager->tiers[1], key, value);
        }
    }
    
    printf("Cache Manager Results:\n");
    printf("  L1 Cache Hits: %lu (%.1f%%)\n", l1_hits, 
           operations > 0 ? (double)l1_hits / operations * 100 : 0.0);
    printf("  L2 Cache Hits: %lu (%.1f%%)\n", l2_hits,
           operations > 0 ? (double)l2_hits / operations * 100 : 0.0);
    printf("  Cache Misses: %lu (%.1f%%)\n", misses,
           operations > 0 ? (double)misses / operations * 100 : 0.0);
    printf("  Overall Hit Rate: %.1f%%\n",
           operations > 0 ? (double)(l1_hits + l2_hits) / operations * 100 : 0.0);
    
    // Clean up using helper function
    cns_cache_manager_destroy(manager);
}

// ============================================================================
// MAIN EXAMPLE FUNCTION
// ============================================================================

int main() {
    printf("CNS Optimization Helpers Usage Examples\n");
    printf("=======================================\n\n");
    
    // Print hardware information using helper macros
    printf("Hardware Information:\n");
    printf("Vector Width: %d\n", CNS_VECTOR_WIDTH);
    printf("Vector Alignment: %d bytes\n", CNS_VECTOR_ALIGN);
    printf("Cache Line Size: %d bytes\n", CNS_CACHE_LINE_SIZE);
    
#ifdef CNS_HAS_AVX512
    printf("SIMD: AVX-512 (8-wide)\n");
#elif defined(CNS_HAS_AVX2)
    printf("SIMD: AVX-2 (4-wide)\n");
#elif defined(CNS_HAS_NEON)
    printf("SIMD: ARM NEON (4-wide)\n");
#else
    printf("SIMD: Scalar (1-wide)\n");
#endif
    printf("\n");
    
    // Example 1: Vectorized array operations
    printf("Example 1: Vectorized Array Operations\n");
    printf("--------------------------------------\n");
    const size_t array_size = 1000;
    uint32_t* a = malloc(array_size * sizeof(uint32_t));
    uint32_t* b = malloc(array_size * sizeof(uint32_t));
    uint32_t* result = malloc(array_size * sizeof(uint32_t));
    
    for (size_t i = 0; i < array_size; i++) {
        a[i] = i;
        b[i] = i * 2;
    }
    
    vectorized_array_add(a, b, result, array_size);
    
    // Verify results
    int correct = 1;
    for (size_t i = 0; i < array_size; i++) {
        if (result[i] != a[i] + b[i]) {
            correct = 0;
            break;
        }
    }
    printf("Result Verification: %s\n", correct ? "PASSED" : "FAILED");
    printf("\n");
    
    free(a);
    free(b);
    free(result);
    
    // Example 2: Performance monitoring
    printf("Example 2: Performance Monitoring\n");
    printf("---------------------------------\n");
    CNSPerformanceCounters* counters = cns_performance_counters_create();
    performance_monitored_operation(counters, 10000);
    cns_performance_counters_destroy(counters);
    printf("\n");
    
    // Example 3: Lock-free operations
    printf("Example 3: Lock-Free Operations\n");
    printf("-------------------------------\n");
    lock_free_operations_example(1000);
    printf("\n");
    
    // Example 4: Memory pool allocation
    printf("Example 4: Memory Pool Allocation\n");
    printf("---------------------------------\n");
    memory_pool_example(1000);
    printf("\n");
    
    // Example 5: Bit vector operations
    printf("Example 5: Bit Vector Operations\n");
    printf("--------------------------------\n");
    bit_vector_example(1000);
    printf("\n");
    
    // Example 6: Cache manager operations
    printf("Example 6: Cache Manager Operations\n");
    printf("-----------------------------------\n");
    cache_manager_example(1000);
    printf("\n");
    
    printf("All examples completed successfully!\n");
    printf("The helper utilities make it easy to implement advanced optimizations.\n");
    
    return 0;
} 