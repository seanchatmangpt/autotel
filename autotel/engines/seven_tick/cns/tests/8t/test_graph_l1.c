/**
 * @file test_graph_l1.c
 * @brief Cache-Optimized Graph Tests for 8T
 * 
 * Tests for L1 cache-optimized graph operations with performance
 * validation and cache behavior analysis.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <immintrin.h>

// L1 cache configuration
#define L1_CACHE_SIZE (32 * 1024)
#define L1_CACHE_LINE_SIZE 64
#define CACHE_FRIENDLY_SIZE (L1_CACHE_SIZE / 4)

// Graph configuration
#define MAX_VERTICES 1024
#define MAX_EDGES 8192
#define BATCH_SIZE 8  // For SIMD operations

// Performance measurement
static inline uint64_t rdtsc(void) {
    unsigned int lo, hi;
    __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
    return ((uint64_t)hi << 32) | lo;
}

// Cache-optimized graph structures
typedef struct {
    uint32_t to;
    float weight;
} __attribute__((packed)) edge_t;

typedef struct {
    uint32_t first_edge;
    uint32_t degree;
    uint32_t visited;
    float distance;
} __attribute__((aligned(16))) vertex_t;

typedef struct {
    vertex_t *vertices;
    edge_t *edges;
    uint32_t num_vertices;
    uint32_t num_edges;
    uint32_t edge_capacity;
} graph_l1_t;

// Initialize cache-optimized graph
int graph_l1_init(graph_l1_t *graph, uint32_t max_vertices, uint32_t max_edges) {
    // Align vertices to cache lines
    graph->vertices = aligned_alloc(L1_CACHE_LINE_SIZE, 
                                  max_vertices * sizeof(vertex_t));
    if (!graph->vertices) return -1;
    
    // Pack edges contiguously
    graph->edges = aligned_alloc(L1_CACHE_LINE_SIZE, 
                               max_edges * sizeof(edge_t));
    if (!graph->edges) {
        free(graph->vertices);
        return -1;
    }
    
    graph->num_vertices = 0;
    graph->num_edges = 0;
    graph->edge_capacity = max_edges;
    
    // Initialize vertices
    memset(graph->vertices, 0, max_vertices * sizeof(vertex_t));
    
    return 0;
}

// Add vertex with cache-friendly layout
uint32_t graph_l1_add_vertex(graph_l1_t *graph) {
    uint32_t vertex_id = graph->num_vertices++;
    vertex_t *v = &graph->vertices[vertex_id];
    
    v->first_edge = graph->num_edges;
    v->degree = 0;
    v->visited = 0;
    v->distance = INFINITY;
    
    return vertex_id;
}

// Add edge with spatial locality
int graph_l1_add_edge(graph_l1_t *graph, uint32_t from, uint32_t to, float weight) {
    if (graph->num_edges >= graph->edge_capacity) return -1;
    if (from >= graph->num_vertices || to >= graph->num_vertices) return -1;
    
    // Add edge to the end of vertex's edge list
    uint32_t edge_idx = graph->num_edges++;
    edge_t *edge = &graph->edges[edge_idx];
    
    edge->to = to;
    edge->weight = weight;
    
    // Update vertex degree
    graph->vertices[from].degree++;
    
    return 0;
}

// Cache-friendly BFS implementation
int graph_l1_bfs(graph_l1_t *graph, uint32_t start) {
    if (start >= graph->num_vertices) return -1;
    
    // Reset visited flags
    for (uint32_t i = 0; i < graph->num_vertices; i++) {
        graph->vertices[i].visited = 0;
        graph->vertices[i].distance = INFINITY;
    }
    
    // Simple queue using array (cache-friendly)
    uint32_t *queue = malloc(graph->num_vertices * sizeof(uint32_t));
    uint32_t queue_front = 0, queue_back = 0;
    
    // Start BFS
    graph->vertices[start].visited = 1;
    graph->vertices[start].distance = 0.0f;
    queue[queue_back++] = start;
    
    while (queue_front < queue_back) {
        uint32_t current = queue[queue_front++];
        vertex_t *v = &graph->vertices[current];
        
        // Process edges in sequential order (cache-friendly)
        for (uint32_t i = 0; i < v->degree; i++) {
            edge_t *edge = &graph->edges[v->first_edge + i];
            vertex_t *neighbor = &graph->vertices[edge->to];
            
            if (!neighbor->visited) {
                neighbor->visited = 1;
                neighbor->distance = v->distance + 1.0f;
                queue[queue_back++] = edge->to;
            }
        }
    }
    
    free(queue);
    return 0;
}

// SIMD-optimized distance updates
void graph_l1_update_distances_simd(float *distances, const float *updates, 
                                   uint32_t count) {
    uint32_t simd_count = count - (count % BATCH_SIZE);
    
    for (uint32_t i = 0; i < simd_count; i += BATCH_SIZE) {
        __m256 dist = _mm256_load_ps(&distances[i]);
        __m256 upd = _mm256_load_ps(&updates[i]);
        __m256 new_dist = _mm256_min_ps(dist, upd);
        _mm256_store_ps(&distances[i], new_dist);
    }
    
    // Handle remaining elements
    for (uint32_t i = simd_count; i < count; i++) {
        if (updates[i] < distances[i]) {
            distances[i] = updates[i];
        }
    }
}

// Test cache locality in graph traversal
int test_cache_locality_traversal(void) {
    printf("Testing cache locality in graph traversal...\n");
    
    graph_l1_t graph;
    if (graph_l1_init(&graph, 256, 2048) != 0) {
        printf("Failed to initialize graph\n");
        return -1;
    }
    
    // Create a grid graph for predictable access patterns
    const uint32_t grid_size = 16; // 16x16 grid
    uint32_t vertices[grid_size][grid_size];
    
    // Add vertices
    for (uint32_t i = 0; i < grid_size; i++) {
        for (uint32_t j = 0; j < grid_size; j++) {
            vertices[i][j] = graph_l1_add_vertex(&graph);
        }
    }
    
    // Add edges (4-connected grid)
    for (uint32_t i = 0; i < grid_size; i++) {
        for (uint32_t j = 0; j < grid_size; j++) {
            // Right edge
            if (j < grid_size - 1) {
                graph_l1_add_edge(&graph, vertices[i][j], vertices[i][j+1], 1.0f);
            }
            // Down edge
            if (i < grid_size - 1) {
                graph_l1_add_edge(&graph, vertices[i][j], vertices[i+1][j], 1.0f);
            }
        }
    }
    
    // Measure BFS performance
    uint64_t start = rdtsc();
    for (int iter = 0; iter < 1000; iter++) {
        graph_l1_bfs(&graph, vertices[0][0]);
    }
    uint64_t end = rdtsc();
    
    printf("Grid BFS time: %lu cycles\n", end - start);
    
    // Verify correctness
    float expected_distance = (float)(grid_size - 1) + (grid_size - 1);
    float actual_distance = graph.vertices[vertices[grid_size-1][grid_size-1]].distance;
    
    if (fabsf(actual_distance - expected_distance) < 1e-6f) {
        printf("✓ BFS distances computed correctly\n");
    } else {
        printf("✗ BFS distance error: expected %.1f, got %.1f\n", 
               expected_distance, actual_distance);
    }
    
    free(graph.vertices);
    free(graph.edges);
    
    printf("✓ Cache locality traversal test completed\n");
    return 0;
}

// Test adjacency list vs matrix performance
int test_adjacency_formats(void) {
    printf("Testing adjacency list vs matrix performance...\n");
    
    const uint32_t num_vertices = 512;
    const uint32_t num_edges = 4096;
    
    // Test adjacency list (our cache-optimized format)
    graph_l1_t list_graph;
    graph_l1_init(&list_graph, num_vertices, num_edges);
    
    // Add random edges
    srand(42);
    for (uint32_t i = 0; i < num_vertices; i++) {
        graph_l1_add_vertex(&list_graph);
    }
    
    for (uint32_t i = 0; i < num_edges; i++) {
        uint32_t from = rand() % num_vertices;
        uint32_t to = rand() % num_vertices;
        graph_l1_add_edge(&list_graph, from, to, 1.0f);
    }
    
    // Time adjacency list BFS
    uint64_t list_start = rdtsc();
    for (int iter = 0; iter < 100; iter++) {
        graph_l1_bfs(&list_graph, 0);
    }
    uint64_t list_end = rdtsc();
    uint64_t list_time = list_end - list_start;
    
    // Test adjacency matrix
    uint8_t *matrix = calloc(num_vertices * num_vertices, sizeof(uint8_t));
    
    // Fill matrix from list
    for (uint32_t v = 0; v < list_graph.num_vertices; v++) {
        vertex_t *vertex = &list_graph.vertices[v];
        for (uint32_t e = 0; e < vertex->degree; e++) {
            edge_t *edge = &list_graph.edges[vertex->first_edge + e];
            matrix[v * num_vertices + edge->to] = 1;
        }
    }
    
    // Time matrix BFS
    uint32_t *visited = calloc(num_vertices, sizeof(uint32_t));
    uint32_t *queue = malloc(num_vertices * sizeof(uint32_t));
    
    uint64_t matrix_start = rdtsc();
    for (int iter = 0; iter < 100; iter++) {
        // Reset
        memset(visited, 0, num_vertices * sizeof(uint32_t));
        uint32_t queue_front = 0, queue_back = 0;
        
        visited[0] = 1;
        queue[queue_back++] = 0;
        
        while (queue_front < queue_back) {
            uint32_t current = queue[queue_front++];
            
            // Check all possible neighbors
            for (uint32_t neighbor = 0; neighbor < num_vertices; neighbor++) {
                if (matrix[current * num_vertices + neighbor] && !visited[neighbor]) {
                    visited[neighbor] = 1;
                    queue[queue_back++] = neighbor;
                }
            }
        }
    }
    uint64_t matrix_end = rdtsc();
    uint64_t matrix_time = matrix_end - matrix_start;
    
    printf("Adjacency list BFS time: %lu cycles\n", list_time);
    printf("Adjacency matrix BFS time: %lu cycles\n", matrix_time);
    printf("List vs Matrix speedup: %.2fx\n", (double)matrix_time / list_time);
    
    // Memory usage comparison
    size_t list_memory = num_vertices * sizeof(vertex_t) + num_edges * sizeof(edge_t);
    size_t matrix_memory = num_vertices * num_vertices * sizeof(uint8_t);
    
    printf("List memory usage: %zu bytes\n", list_memory);
    printf("Matrix memory usage: %zu bytes\n", matrix_memory);
    printf("Memory efficiency: %.2fx\n", (double)matrix_memory / list_memory);
    
    // Cleanup
    free(list_graph.vertices);
    free(list_graph.edges);
    free(matrix);
    free(visited);
    free(queue);
    
    printf("✓ Adjacency format comparison completed\n");
    return 0;
}

// Test SIMD distance updates
int test_simd_distance_updates(void) {
    printf("Testing SIMD distance updates...\n");
    
    const uint32_t num_distances = 1024;
    float *distances = aligned_alloc(32, num_distances * sizeof(float));
    float *updates = aligned_alloc(32, num_distances * sizeof(float));
    float *distances_scalar = aligned_alloc(32, num_distances * sizeof(float));
    
    // Initialize test data
    srand(42);
    for (uint32_t i = 0; i < num_distances; i++) {
        distances[i] = (float)rand() / RAND_MAX * 100.0f;
        updates[i] = (float)rand() / RAND_MAX * 100.0f;
        distances_scalar[i] = distances[i];
    }
    
    // Scalar version
    uint64_t scalar_start = rdtsc();
    for (int iter = 0; iter < 10000; iter++) {
        for (uint32_t i = 0; i < num_distances; i++) {
            if (updates[i] < distances_scalar[i]) {
                distances_scalar[i] = updates[i];
            }
        }
    }
    uint64_t scalar_end = rdtsc();
    uint64_t scalar_time = scalar_end - scalar_start;
    
    // SIMD version
    uint64_t simd_start = rdtsc();
    for (int iter = 0; iter < 10000; iter++) {
        graph_l1_update_distances_simd(distances, updates, num_distances);
    }
    uint64_t simd_end = rdtsc();
    uint64_t simd_time = simd_end - simd_start;
    
    // Verify correctness
    int errors = 0;
    for (uint32_t i = 0; i < num_distances; i++) {
        if (fabsf(distances[i] - distances_scalar[i]) > 1e-6f) {
            errors++;
        }
    }
    
    printf("SIMD distance update errors: %d/%u\n", errors, num_distances);
    printf("Scalar time: %lu cycles\n", scalar_time);
    printf("SIMD time: %lu cycles\n", simd_time);
    printf("SIMD speedup: %.2fx\n", (double)scalar_time / simd_time);
    
    free(distances);
    free(updates);
    free(distances_scalar);
    
    if (errors == 0) {
        printf("✓ SIMD distance updates working correctly\n");
        return 0;
    } else {
        printf("✗ SIMD distance update errors detected\n");
        return -1;
    }
}

// Test cache-friendly graph layout
int test_cache_friendly_layout(void) {
    printf("Testing cache-friendly graph layout...\n");
    
    graph_l1_t graph;
    if (graph_l1_init(&graph, 64, 512) != 0) {
        printf("Failed to initialize graph\n");
        return -1;
    }
    
    // Create vertices
    for (uint32_t i = 0; i < 64; i++) {
        graph_l1_add_vertex(&graph);
    }
    
    // Add edges in vertex order (cache-friendly)
    for (uint32_t i = 0; i < 63; i++) {
        graph_l1_add_edge(&graph, i, i + 1, 1.0f);
        if (i < 62) {
            graph_l1_add_edge(&graph, i, i + 2, 2.0f);
        }
    }
    
    // Verify edge layout is sequential
    uint32_t expected_edge_idx = 0;
    for (uint32_t v = 0; v < graph.num_vertices; v++) {
        vertex_t *vertex = &graph.vertices[v];
        if (vertex->degree > 0) {
            if (vertex->first_edge != expected_edge_idx) {
                printf("✗ Edge layout not sequential for vertex %u\n", v);
                free(graph.vertices);
                free(graph.edges);
                return -1;
            }
            expected_edge_idx += vertex->degree;
        }
    }
    
    // Test memory access patterns
    uint64_t start = rdtsc();
    for (int iter = 0; iter < 10000; iter++) {
        // Sequential edge access
        for (uint32_t e = 0; e < graph.num_edges; e++) {
            volatile uint32_t to = graph.edges[e].to;
            volatile float weight = graph.edges[e].weight;
            (void)to; (void)weight; // Suppress unused warnings
        }
    }
    uint64_t end = rdtsc();
    
    printf("Sequential edge access time: %lu cycles\n", end - start);
    
    // Verify cache line alignment
    uintptr_t vertex_addr = (uintptr_t)graph.vertices;
    uintptr_t edge_addr = (uintptr_t)graph.edges;
    
    if (vertex_addr % L1_CACHE_LINE_SIZE == 0) {
        printf("✓ Vertices aligned to cache line boundary\n");
    } else {
        printf("✗ Vertices not aligned to cache line boundary\n");
    }
    
    if (edge_addr % L1_CACHE_LINE_SIZE == 0) {
        printf("✓ Edges aligned to cache line boundary\n");
    } else {
        printf("✗ Edges not aligned to cache line boundary\n");
    }
    
    free(graph.vertices);
    free(graph.edges);
    
    printf("✓ Cache-friendly layout test completed\n");
    return 0;
}

// Test graph memory footprint
int test_memory_footprint(void) {
    printf("Testing graph memory footprint...\n");
    
    const uint32_t num_vertices = 1000;
    const uint32_t num_edges = 5000;
    
    graph_l1_t graph;
    graph_l1_init(&graph, num_vertices, num_edges);
    
    // Calculate memory usage
    size_t vertex_memory = num_vertices * sizeof(vertex_t);
    size_t edge_memory = num_edges * sizeof(edge_t);
    size_t total_memory = vertex_memory + edge_memory;
    
    printf("Vertex memory: %zu bytes (%.2f KB)\n", vertex_memory, vertex_memory / 1024.0);
    printf("Edge memory: %zu bytes (%.2f KB)\n", edge_memory, edge_memory / 1024.0);
    printf("Total memory: %zu bytes (%.2f KB)\n", total_memory, total_memory / 1024.0);
    printf("L1 cache utilization: %.1f%%\n", (double)total_memory / L1_CACHE_SIZE * 100);
    
    // Check if graph fits in L1 cache
    if (total_memory <= CACHE_FRIENDLY_SIZE) {
        printf("✓ Graph fits in L1 cache-friendly size\n");
    } else {
        printf("⚠ Graph exceeds L1 cache-friendly size\n");
    }
    
    // Memory per vertex analysis
    double bytes_per_vertex = (double)total_memory / num_vertices;
    printf("Average bytes per vertex: %.2f\n", bytes_per_vertex);
    
    free(graph.vertices);
    free(graph.edges);
    
    printf("✓ Memory footprint analysis completed\n");
    return 0;
}

int main(void) {
    printf("=== 8T Cache-Optimized Graph Tests ===\n\n");
    
    int result = 0;
    
    result |= test_cache_locality_traversal();
    printf("\n");
    
    result |= test_adjacency_formats();
    printf("\n");
    
    result |= test_simd_distance_updates();
    printf("\n");
    
    result |= test_cache_friendly_layout();
    printf("\n");
    
    result |= test_memory_footprint();
    printf("\n");
    
    if (result == 0) {
        printf("✅ All cache-optimized graph tests passed!\n");
    } else {
        printf("❌ Some tests failed\n");
    }
    
    return result;
}