/*
 * Test program for CNS 8T L1-optimized RDF graph
 * Demonstrates the 16-byte aligned triple structure with SIMD operations
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

// Include the implementation directly for testing
#include "graph_l1_rdf.c"

// Test configuration
#define TEST_TRIPLE_COUNT 10000
#define TEST_QUERY_COUNT 1000

// Helper to measure time in nanoseconds
static inline uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

// Test basic insertion and lookup
void test_basic_operations(void) {
    printf("\n=== Basic Operations Test ===\n");
    
    cns_8t_graph_rdf_t* graph = cns_8t_graph_rdf_create(1000, true);
    if (!graph) {
        printf("Failed to create graph\n");
        return;
    }
    
    // Insert some triples
    printf("Inserting test triples...\n");
    bool result;
    
    result = cns_8t_graph_rdf_insert_triple(graph, 1, 2, 3, CNS_8T_RDF_TYPE_IRI, 0);
    printf("  Insert (1,2,3): %s\n", result ? "SUCCESS" : "FAILED");
    
    result = cns_8t_graph_rdf_insert_triple(graph, 1, 2, 4, CNS_8T_RDF_TYPE_LITERAL, 0);
    printf("  Insert (1,2,4): %s\n", result ? "SUCCESS" : "FAILED");
    
    result = cns_8t_graph_rdf_insert_triple(graph, 2, 3, 4, CNS_8T_RDF_TYPE_BLANK, 0);
    printf("  Insert (2,3,4): %s\n", result ? "SUCCESS" : "FAILED");
    
    // Test duplicate detection
    result = cns_8t_graph_rdf_insert_triple(graph, 1, 2, 3, CNS_8T_RDF_TYPE_IRI, 0);
    printf("  Insert duplicate (1,2,3): %s (should fail)\n", result ? "FAILED" : "SUCCESS");
    
    // Test pattern matching
    printf("\nTesting pattern matching...\n");
    uint32_t results[10];
    size_t found;
    
    // Find all triples with subject = 1
    found = cns_8t_graph_rdf_find_pattern_simd(graph, 1, CNS_8T_RDF_WILDCARD, 
                                               CNS_8T_RDF_WILDCARD, results, 10);
    printf("  Pattern (1,*,*): found %zu triples\n", found);
    
    // Find all triples with predicate = 2
    found = cns_8t_graph_rdf_find_pattern_simd(graph, CNS_8T_RDF_WILDCARD, 2, 
                                               CNS_8T_RDF_WILDCARD, results, 10);
    printf("  Pattern (*,2,*): found %zu triples\n", found);
    
    // Find specific triple
    found = cns_8t_graph_rdf_find_pattern_simd(graph, 2, 3, 4, results, 10);
    printf("  Pattern (2,3,4): found %zu triples\n", found);
    
    // Get statistics
    uint64_t hits, misses, simd_ops;
    cns_8t_graph_rdf_get_stats(graph, &hits, &misses, &simd_ops);
    printf("\nStatistics:\n");
    printf("  Cache hits: %llu\n", hits);
    printf("  Cache misses: %llu\n", misses);
    printf("  SIMD operations: %llu\n", simd_ops);
    
    cns_8t_graph_rdf_destroy(graph);
}

// Test SIMD batch insertion performance
void test_simd_batch_performance(void) {
    printf("\n=== SIMD Batch Performance Test ===\n");
    
    cns_8t_graph_rdf_t* graph = cns_8t_graph_rdf_create(TEST_TRIPLE_COUNT * 2, true);
    if (!graph) {
        printf("Failed to create graph\n");
        return;
    }
    
    // Allocate aligned arrays for batch data
    uint32_t* subjects = aligned_alloc_16(TEST_TRIPLE_COUNT * sizeof(uint32_t));
    uint32_t* predicates = aligned_alloc_16(TEST_TRIPLE_COUNT * sizeof(uint32_t));
    uint32_t* objects = aligned_alloc_16(TEST_TRIPLE_COUNT * sizeof(uint32_t));
    uint16_t* types = aligned_alloc_16(TEST_TRIPLE_COUNT * sizeof(uint16_t));
    uint16_t* graphs = aligned_alloc_16(TEST_TRIPLE_COUNT * sizeof(uint16_t));
    
    // Generate test data
    printf("Generating %d test triples...\n", TEST_TRIPLE_COUNT);
    for (size_t i = 0; i < TEST_TRIPLE_COUNT; i++) {
        subjects[i] = (i / 100) + 1;      // 100 triples per subject
        predicates[i] = (i % 10) + 1;     // 10 different predicates
        objects[i] = i + 1000;            // Unique objects
        types[i] = CNS_8T_RDF_TYPE_IRI;
        graphs[i] = 0;
    }
    
    // Measure batch insertion time
    uint64_t start_ns = get_time_ns();
    
#ifdef __x86_64__
    size_t inserted = cns_8t_graph_rdf_insert_batch_simd(
        graph, subjects, predicates, objects, types, graphs, TEST_TRIPLE_COUNT
    );
#else
    size_t inserted = 0;
    for (size_t i = 0; i < TEST_TRIPLE_COUNT; i++) {
        if (cns_8t_graph_rdf_insert_triple(graph, subjects[i], predicates[i],
                                          objects[i], types[i], graphs[i])) {
            inserted++;
        }
    }
#endif
    
    uint64_t end_ns = get_time_ns();
    double elapsed_ms = (end_ns - start_ns) / 1000000.0;
    
    printf("Inserted %zu triples in %.3f ms\n", inserted, elapsed_ms);
    printf("Rate: %.0f triples/second\n", (inserted * 1000.0) / elapsed_ms);
    printf("Average: %.2f nanoseconds/triple\n", 
           (double)(end_ns - start_ns) / inserted);
    
    // Test query performance
    printf("\nTesting query performance...\n");
    uint32_t* results = malloc(TEST_TRIPLE_COUNT * sizeof(uint32_t));
    
    // Query 1: Find all triples for subject 50
    start_ns = get_time_ns();
    size_t found = cns_8t_graph_rdf_find_pattern_simd(
        graph, 50, CNS_8T_RDF_WILDCARD, CNS_8T_RDF_WILDCARD, 
        results, TEST_TRIPLE_COUNT
    );
    end_ns = get_time_ns();
    
    printf("Query (50,*,*): found %zu triples in %.3f µs\n", 
           found, (end_ns - start_ns) / 1000.0);
    
    // Query 2: Find all triples with predicate 5
    start_ns = get_time_ns();
    found = cns_8t_graph_rdf_find_pattern_simd(
        graph, CNS_8T_RDF_WILDCARD, 5, CNS_8T_RDF_WILDCARD, 
        results, TEST_TRIPLE_COUNT
    );
    end_ns = get_time_ns();
    
    printf("Query (*,5,*): found %zu triples in %.3f µs\n", 
           found, (end_ns - start_ns) / 1000.0);
    
    // Get final statistics
    uint64_t hits, misses, simd_ops;
    cns_8t_graph_rdf_get_stats(graph, &hits, &misses, &simd_ops);
    printf("\nFinal Statistics:\n");
    printf("  Cache hits: %llu\n", hits);
    printf("  Cache misses: %llu\n", misses);
    printf("  SIMD operations: %llu\n", simd_ops);
    printf("  Hit ratio: %.2f%%\n", 100.0 * hits / (hits + misses));
    
    // Cleanup
    free(results);
    aligned_free(subjects);
    aligned_free(predicates);
    aligned_free(objects);
    aligned_free(types);
    aligned_free(graphs);
    cns_8t_graph_rdf_destroy(graph);
}

// Test cache optimization
void test_cache_optimization(void) {
    printf("\n=== Cache Optimization Test ===\n");
    
    cns_8t_graph_rdf_t* graph = cns_8t_graph_rdf_create(1000, true);
    if (!graph) {
        printf("Failed to create graph\n");
        return;
    }
    
    // Insert triples with poor locality (random access pattern)
    printf("Inserting triples with random pattern...\n");
    for (int i = 0; i < 100; i++) {
        uint32_t s = rand() % 20 + 1;
        uint32_t p = rand() % 5 + 1;
        uint32_t o = rand() % 100 + 1000;
        cns_8t_graph_rdf_insert_triple(graph, s, p, o, CNS_8T_RDF_TYPE_IRI, 0);
    }
    
    // Test prefetching
    printf("Testing prefetch effectiveness...\n");
    
    // Without prefetch
    uint32_t results[100];
    uint64_t start_ns = get_time_ns();
    for (int i = 1; i <= 20; i++) {
        cns_8t_graph_rdf_find_pattern_simd(
            graph, i, CNS_8T_RDF_WILDCARD, CNS_8T_RDF_WILDCARD, 
            results, 100
        );
    }
    uint64_t no_prefetch_ns = get_time_ns() - start_ns;
    
    // With prefetch
    start_ns = get_time_ns();
    cns_8t_graph_rdf_prefetch_range(graph, 0, 100);
    for (int i = 1; i <= 20; i++) {
        cns_8t_graph_rdf_find_pattern_simd(
            graph, i, CNS_8T_RDF_WILDCARD, CNS_8T_RDF_WILDCARD, 
            results, 100
        );
    }
    uint64_t with_prefetch_ns = get_time_ns() - start_ns;
    
    printf("Query time without prefetch: %.3f µs\n", no_prefetch_ns / 1000.0);
    printf("Query time with prefetch: %.3f µs\n", with_prefetch_ns / 1000.0);
    printf("Improvement: %.1f%%\n", 
           100.0 * (1.0 - (double)with_prefetch_ns / no_prefetch_ns));
    
    cns_8t_graph_rdf_destroy(graph);
}

// Test memory alignment
void test_memory_alignment(void) {
    printf("\n=== Memory Alignment Test ===\n");
    
    cns_8t_graph_rdf_t* graph = cns_8t_graph_rdf_create(100, true);
    if (!graph) {
        printf("Failed to create graph\n");
        return;
    }
    
    printf("Triple size: %zu bytes\n", sizeof(cns_8t_triple_rdf_t));
    printf("Triple alignment requirement: 16 bytes\n");
    printf("Triple array address: %p\n", (void*)graph->triples);
    printf("Triple array aligned: %s\n", 
           ((uintptr_t)graph->triples % 16 == 0) ? "YES" : "NO");
    
    printf("\nHash entry size: %zu bytes\n", sizeof(hash_entry_t));
    printf("Hash table address: %p\n", (void*)graph->hash_table);
    printf("Hash table aligned: %s\n", 
           ((uintptr_t)graph->hash_table % 16 == 0) ? "YES" : "NO");
    
    // Verify structure packing
    cns_8t_triple_rdf_t test_triple;
    printf("\nTriple field offsets:\n");
    printf("  subject_id: %zu\n", offsetof(cns_8t_triple_rdf_t, subject_id));
    printf("  predicate_id: %zu\n", offsetof(cns_8t_triple_rdf_t, predicate_id));
    printf("  object_id: %zu\n", offsetof(cns_8t_triple_rdf_t, object_id));
    printf("  type_flags: %zu\n", offsetof(cns_8t_triple_rdf_t, type_flags));
    printf("  graph_id: %zu\n", offsetof(cns_8t_triple_rdf_t, graph_id));
    
    cns_8t_graph_rdf_destroy(graph);
}

int main(void) {
    printf("CNS 8T L1-Optimized RDF Graph Test Suite\n");
    printf("=========================================\n");
    
    // Detect CPU features
    printf("\nCPU Features:\n");
#ifdef __x86_64__
    printf("  Architecture: x86-64\n");
    printf("  SIMD: AVX2 (assumed)\n");
#elif defined(__aarch64__)
    printf("  Architecture: ARM64\n");
    printf("  SIMD: NEON\n");
#else
    printf("  Architecture: Generic\n");
    printf("  SIMD: Disabled\n");
#endif
    
    // Run tests
    test_memory_alignment();
    test_basic_operations();
    test_simd_batch_performance();
    test_cache_optimization();
    
    printf("\n=== All Tests Complete ===\n");
    return 0;
}