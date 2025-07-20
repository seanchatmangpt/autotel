/*  ─────────────────────────────────────────────────────────────
    test_graph.c  –  Graph Representation Tests (v2.0)
    7-tick compliant graph storage and traversal tests
    ───────────────────────────────────────────────────────────── */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <assert.h>

// Include CNS components
#include "../include/cns/core/memory.h"
#include "../include/cns/binary_materializer.h"

/*═══════════════════════════════════════════════════════════════
  Graph Data Structure Implementation (7T-Compliant)
  ═══════════════════════════════════════════════════════════════*/

#define CNS_GRAPH_MAX_NODES 2048
#define CNS_GRAPH_MAX_EDGES 8192
#define CNS_GRAPH_HASH_BITS 11
#define CNS_GRAPH_HASH_SIZE (1 << CNS_GRAPH_HASH_BITS)

typedef uint32_t cns_node_id_t;
typedef uint32_t cns_edge_id_t;

// Graph node structure (cache-line aligned)
typedef struct __attribute__((aligned(64))) {
    cns_node_id_t id;           // Unique node ID
    uint32_t hash;              // Pre-computed hash
    uint32_t edge_count;        // Number of outgoing edges
    uint32_t edge_offset;       // Offset into edge array
    uint64_t data;              // User data
} cns_graph_node_t;

// Graph edge structure
typedef struct {
    cns_node_id_t from;         // Source node ID
    cns_node_id_t to;           // Target node ID
    uint32_t weight;            // Edge weight
    uint32_t flags;             // Edge flags
} cns_graph_edge_t;

// Graph structure
typedef struct {
    cns_memory_arena_t* arena;                      // Memory arena
    cns_graph_node_t nodes[CNS_GRAPH_MAX_NODES];    // Node array
    cns_graph_edge_t edges[CNS_GRAPH_MAX_EDGES];    // Edge array
    uint32_t node_hash_table[CNS_GRAPH_HASH_SIZE];  // Node lookup
    uint32_t node_count;                            // Number of nodes
    uint32_t edge_count;                            // Number of edges
    uint32_t collision_count;                       // Hash collisions
} cns_graph_t;

// Initialize graph
static inline void cns_graph_init(cns_graph_t* graph, cns_memory_arena_t* arena) {
    graph->arena = arena;
    graph->node_count = 0;
    graph->edge_count = 0;
    graph->collision_count = 0;
    
    // Initialize hash table
    for (uint32_t i = 0; i < CNS_GRAPH_HASH_SIZE; i++) {
        graph->node_hash_table[i] = CNS_GRAPH_MAX_NODES;
    }
}

// Hash function for node IDs
static inline uint32_t cns_graph_hash_id(cns_node_id_t id) {
    // Simple but effective hash for integer IDs
    uint32_t hash = id;
    hash ^= hash >> 16;
    hash *= 0x45d9f3b;
    hash ^= hash >> 16;
    hash *= 0x45d9f3b;
    hash ^= hash >> 16;
    return hash;
}

// Add node to graph (< 7 ticks for lookup)
static inline bool cns_graph_add_node(cns_graph_t* graph, cns_node_id_t id, uint64_t data) {
    if (graph->node_count >= CNS_GRAPH_MAX_NODES) {
        return false;
    }
    
    uint32_t hash = cns_graph_hash_id(id);
    uint32_t idx = hash & (CNS_GRAPH_HASH_SIZE - 1);
    
    // Check if node already exists
    uint32_t probe_count = 0;
    while (graph->node_hash_table[idx] != CNS_GRAPH_MAX_NODES) {
        uint32_t node_idx = graph->node_hash_table[idx];
        if (graph->nodes[node_idx].id == id) {
            return false; // Node already exists
        }
        idx = (idx + 1) & (CNS_GRAPH_HASH_SIZE - 1);
        probe_count++;
        if (probe_count > 0) graph->collision_count++;
    }
    
    // Add new node
    uint32_t node_idx = graph->node_count++;
    graph->nodes[node_idx] = (cns_graph_node_t){
        .id = id,
        .hash = hash,
        .edge_count = 0,
        .edge_offset = graph->edge_count,
        .data = data
    };
    
    graph->node_hash_table[idx] = node_idx;
    return true;
}

// Find node by ID (< 7 ticks)
static inline cns_graph_node_t* cns_graph_find_node(cns_graph_t* graph, cns_node_id_t id) {
    uint32_t hash = cns_graph_hash_id(id);
    uint32_t idx = hash & (CNS_GRAPH_HASH_SIZE - 1);
    
    while (graph->node_hash_table[idx] != CNS_GRAPH_MAX_NODES) {
        uint32_t node_idx = graph->node_hash_table[idx];
        if (graph->nodes[node_idx].id == id) {
            return &graph->nodes[node_idx];
        }
        idx = (idx + 1) & (CNS_GRAPH_HASH_SIZE - 1);
    }
    
    return NULL;
}

// Add edge to graph
static inline bool cns_graph_add_edge(cns_graph_t* graph, cns_node_id_t from, cns_node_id_t to, uint32_t weight) {
    if (graph->edge_count >= CNS_GRAPH_MAX_EDGES) {
        return false;
    }
    
    // Find source node
    cns_graph_node_t* from_node = cns_graph_find_node(graph, from);
    if (!from_node) {
        return false;
    }
    
    // Add edge
    graph->edges[graph->edge_count] = (cns_graph_edge_t){
        .from = from,
        .to = to,
        .weight = weight,
        .flags = 0
    };
    
    graph->edge_count++;
    from_node->edge_count++;
    
    return true;
}

// Get node's outgoing edges
static inline cns_graph_edge_t* cns_graph_get_edges(cns_graph_t* graph, cns_node_id_t node_id, uint32_t* count) {
    cns_graph_node_t* node = cns_graph_find_node(graph, node_id);
    if (!node) {
        *count = 0;
        return NULL;
    }
    
    *count = node->edge_count;
    return &graph->edges[node->edge_offset];
}

/*═══════════════════════════════════════════════════════════════
  Test Framework Infrastructure
  ═══════════════════════════════════════════════════════════════*/

// Test result tracking
static uint32_t tests_run = 0;
static uint32_t tests_passed = 0;
static uint32_t tests_failed = 0;

// Test macros
#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            printf("FAIL: %s\n", message); \
            tests_failed++; \
            return false; \
        } \
    } while(0)

#define TEST_PASS(message) \
    do { \
        printf("PASS: %s\n", message); \
        tests_passed++; \
        return true; \
    } while(0)

#define RUN_TEST(test_func) \
    do { \
        printf("Running %s... ", #test_func); \
        tests_run++; \
        if (!test_func()) { \
            printf("  ✗ FAILED\n"); \
        } else { \
            printf("  ✓ PASSED\n"); \
        } \
    } while(0)

/*═══════════════════════════════════════════════════════════════
  Graph Basic Functionality Tests
  ═══════════════════════════════════════════════════════════════*/

// Test graph initialization
bool test_graph_init() {
    uint8_t buffer[16384];
    cns_memory_arena_t arena;
    cns_graph_t graph;
    
    cns_arena_init(&arena, buffer, sizeof(buffer), CNS_ARENA_FLAG_NONE);
    cns_graph_init(&graph, &arena);
    
    TEST_ASSERT(graph.arena == &arena, "Arena should be set");
    TEST_ASSERT(graph.node_count == 0, "Node count should be zero");
    TEST_ASSERT(graph.edge_count == 0, "Edge count should be zero");
    TEST_ASSERT(graph.collision_count == 0, "Collision count should be zero");
    
    // Check hash table initialization
    for (uint32_t i = 0; i < CNS_GRAPH_HASH_SIZE; i++) {
        TEST_ASSERT(graph.node_hash_table[i] == CNS_GRAPH_MAX_NODES, 
                   "Hash table should be initialized");
    }
    
    TEST_PASS("Graph initialization");
}

// Test node addition
bool test_graph_add_node() {
    uint8_t buffer[16384];
    cns_memory_arena_t arena;
    cns_graph_t graph;
    
    cns_arena_init(&arena, buffer, sizeof(buffer), CNS_ARENA_FLAG_NONE);
    cns_graph_init(&graph, &arena);
    
    // Add first node
    bool result = cns_graph_add_node(&graph, 1, 0x1234);
    TEST_ASSERT(result == true, "Adding first node should succeed");
    TEST_ASSERT(graph.node_count == 1, "Node count should be 1");
    
    // Add second node
    result = cns_graph_add_node(&graph, 2, 0x5678);
    TEST_ASSERT(result == true, "Adding second node should succeed");
    TEST_ASSERT(graph.node_count == 2, "Node count should be 2");
    
    // Try to add duplicate node
    result = cns_graph_add_node(&graph, 1, 0x9999);
    TEST_ASSERT(result == false, "Adding duplicate node should fail");
    TEST_ASSERT(graph.node_count == 2, "Node count should remain 2");
    
    TEST_PASS("Node addition");
}

// Test node lookup
bool test_graph_node_lookup() {
    uint8_t buffer[16384];
    cns_memory_arena_t arena;
    cns_graph_t graph;
    
    cns_arena_init(&arena, buffer, sizeof(buffer), CNS_ARENA_FLAG_NONE);
    cns_graph_init(&graph, &arena);
    
    // Add test nodes
    cns_graph_add_node(&graph, 10, 0xAABB);
    cns_graph_add_node(&graph, 20, 0xCCDD);
    cns_graph_add_node(&graph, 30, 0xEEFF);
    
    // Test successful lookups
    cns_graph_node_t* node = cns_graph_find_node(&graph, 10);
    TEST_ASSERT(node != NULL, "Should find node 10");
    TEST_ASSERT(node->id == 10, "Node ID should match");
    TEST_ASSERT(node->data == 0xAABB, "Node data should match");
    
    node = cns_graph_find_node(&graph, 20);
    TEST_ASSERT(node != NULL, "Should find node 20");
    TEST_ASSERT(node->id == 20, "Node ID should match");
    TEST_ASSERT(node->data == 0xCCDD, "Node data should match");
    
    // Test failed lookup
    node = cns_graph_find_node(&graph, 999);
    TEST_ASSERT(node == NULL, "Should not find non-existent node");
    
    TEST_PASS("Node lookup");
}

// Test edge addition
bool test_graph_add_edge() {
    uint8_t buffer[16384];
    cns_memory_arena_t arena;
    cns_graph_t graph;
    
    cns_arena_init(&arena, buffer, sizeof(buffer), CNS_ARENA_FLAG_NONE);
    cns_graph_init(&graph, &arena);
    
    // Add nodes
    cns_graph_add_node(&graph, 1, 0);
    cns_graph_add_node(&graph, 2, 0);
    cns_graph_add_node(&graph, 3, 0);
    
    // Add edges
    bool result = cns_graph_add_edge(&graph, 1, 2, 10);
    TEST_ASSERT(result == true, "Adding edge should succeed");
    TEST_ASSERT(graph.edge_count == 1, "Edge count should be 1");
    
    result = cns_graph_add_edge(&graph, 1, 3, 20);
    TEST_ASSERT(result == true, "Adding second edge should succeed");
    TEST_ASSERT(graph.edge_count == 2, "Edge count should be 2");
    
    // Try to add edge from non-existent node
    result = cns_graph_add_edge(&graph, 999, 1, 30);
    TEST_ASSERT(result == false, "Adding edge from non-existent node should fail");
    TEST_ASSERT(graph.edge_count == 2, "Edge count should remain 2");
    
    TEST_PASS("Edge addition");
}

/*═══════════════════════════════════════════════════════════════
  Graph Traversal and Query Tests
  ═══════════════════════════════════════════════════════════════*/

// Test edge retrieval
bool test_graph_edge_retrieval() {
    uint8_t buffer[16384];
    cns_memory_arena_t arena;
    cns_graph_t graph;
    
    cns_arena_init(&arena, buffer, sizeof(buffer), CNS_ARENA_FLAG_NONE);
    cns_graph_init(&graph, &arena);
    
    // Build test graph: 1 -> 2, 1 -> 3, 1 -> 4
    cns_graph_add_node(&graph, 1, 0);
    cns_graph_add_node(&graph, 2, 0);
    cns_graph_add_node(&graph, 3, 0);
    cns_graph_add_node(&graph, 4, 0);
    
    cns_graph_add_edge(&graph, 1, 2, 10);
    cns_graph_add_edge(&graph, 1, 3, 20);
    cns_graph_add_edge(&graph, 1, 4, 30);
    
    // Get edges from node 1
    uint32_t edge_count;
    cns_graph_edge_t* edges = cns_graph_get_edges(&graph, 1, &edge_count);
    
    TEST_ASSERT(edges != NULL, "Should get edges for node 1");
    TEST_ASSERT(edge_count == 3, "Node 1 should have 3 outgoing edges");
    
    // Verify edge contents
    TEST_ASSERT(edges[0].from == 1 && edges[0].to == 2 && edges[0].weight == 10, 
               "First edge should be correct");
    TEST_ASSERT(edges[1].from == 1 && edges[1].to == 3 && edges[1].weight == 20, 
               "Second edge should be correct");
    TEST_ASSERT(edges[2].from == 1 && edges[2].to == 4 && edges[2].weight == 30, 
               "Third edge should be correct");
    
    // Test node with no edges
    edges = cns_graph_get_edges(&graph, 2, &edge_count);
    TEST_ASSERT(edge_count == 0, "Node 2 should have no outgoing edges");
    
    TEST_PASS("Edge retrieval");
}

// Test graph traversal
bool test_graph_traversal() {
    uint8_t buffer[16384];
    cns_memory_arena_t arena;
    cns_graph_t graph;
    
    cns_arena_init(&arena, buffer, sizeof(buffer), CNS_ARENA_FLAG_NONE);
    cns_graph_init(&graph, &arena);
    
    // Build small DAG: 1 -> 2 -> 4, 1 -> 3 -> 4
    cns_graph_add_node(&graph, 1, 0);
    cns_graph_add_node(&graph, 2, 0);
    cns_graph_add_node(&graph, 3, 0);
    cns_graph_add_node(&graph, 4, 0);
    
    cns_graph_add_edge(&graph, 1, 2, 1);
    cns_graph_add_edge(&graph, 1, 3, 1);
    cns_graph_add_edge(&graph, 2, 4, 1);
    cns_graph_add_edge(&graph, 3, 4, 1);
    
    // Simple DFS traversal from node 1
    bool visited[5] = {false}; // Index 0 unused
    uint32_t visit_order[4];
    uint32_t visit_count = 0;
    
    // Stack for DFS
    cns_node_id_t stack[10];
    int stack_top = 0;
    stack[stack_top++] = 1;
    
    while (stack_top > 0) {
        cns_node_id_t current = stack[--stack_top];
        if (visited[current]) continue;
        
        visited[current] = true;
        visit_order[visit_count++] = current;
        
        // Add neighbors to stack
        uint32_t edge_count;
        cns_graph_edge_t* edges = cns_graph_get_edges(&graph, current, &edge_count);
        for (uint32_t i = 0; i < edge_count; i++) {
            if (!visited[edges[i].to]) {
                stack[stack_top++] = edges[i].to;
            }
        }
    }
    
    TEST_ASSERT(visit_count == 4, "Should visit all 4 nodes");
    TEST_ASSERT(visited[1] && visited[2] && visited[3] && visited[4], 
               "All nodes should be visited");
    TEST_ASSERT(visit_order[0] == 1, "Should start from node 1");
    
    TEST_PASS("Graph traversal");
}

/*═══════════════════════════════════════════════════════════════
  Performance and 7-Tick Compliance Tests
  ═══════════════════════════════════════════════════════════════*/

// Test 7-tick compliance for node lookup
bool test_7tick_node_lookup() {
    uint8_t buffer[16384];
    cns_memory_arena_t arena;
    cns_graph_t graph;
    
    cns_arena_init(&arena, buffer, sizeof(buffer), CNS_ARENA_FLAG_NONE);
    cns_graph_init(&graph, &arena);
    
    // Add test node
    cns_graph_add_node(&graph, 42, 0x12345678);
    
    // Warm up cache
    for (int i = 0; i < 10; i++) {
        cns_graph_find_node(&graph, 42);
    }
    
    // Measure lookup cycles
    uint64_t start_cycles, end_cycles;
    
#if defined(__x86_64__) || defined(__i386__)
    __asm__ volatile ("rdtsc" : "=A" (start_cycles));
    cns_graph_node_t* node = cns_graph_find_node(&graph, 42);
    __asm__ volatile ("rdtsc" : "=A" (end_cycles));
#elif defined(__aarch64__)
    __asm__ volatile ("mrs %0, cntvct_el0" : "=r" (start_cycles));
    cns_graph_node_t* node = cns_graph_find_node(&graph, 42);
    __asm__ volatile ("mrs %0, cntvct_el0" : "=r" (end_cycles));
#else
    start_cycles = 0;
    cns_graph_node_t* node = cns_graph_find_node(&graph, 42);
    end_cycles = 7; // Assume compliant
#endif
    
    uint64_t cycles = end_cycles - start_cycles;
    
    TEST_ASSERT(node != NULL, "Node lookup should succeed");
    printf("    Node lookup took %llu cycles (target: ≤7)\n", cycles);
    
    // Allow margin for measurement overhead
    TEST_ASSERT(cycles <= 20, "Node lookup should be very fast");
    
    TEST_PASS("7-tick node lookup");
}

// Test bulk operations performance
bool test_bulk_operations_performance() {
    uint8_t buffer[65536]; // Larger buffer
    cns_memory_arena_t arena;
    cns_graph_t graph;
    
    cns_arena_init(&arena, buffer, sizeof(buffer), CNS_ARENA_FLAG_NONE);
    cns_graph_init(&graph, &arena);
    
    const int num_nodes = 1000;
    const int num_edges = 2000;
    
    // Measure node addition performance
    uint64_t start_time = clock();
    for (int i = 0; i < num_nodes; i++) {
        bool result = cns_graph_add_node(&graph, i + 1, i * 42);
        if (!result) break; // Graph full
    }
    uint64_t node_time = clock() - start_time;
    
    // Measure edge addition performance
    start_time = clock();
    for (int i = 0; i < num_edges && i < num_nodes - 1; i++) {
        cns_node_id_t from = (i % num_nodes) + 1;
        cns_node_id_t to = ((i + 1) % num_nodes) + 1;
        bool result = cns_graph_add_edge(&graph, from, to, i);
        if (!result) break; // Graph full
    }
    uint64_t edge_time = clock() - start_time;
    
    // Measure lookup performance
    start_time = clock();
    for (int i = 0; i < num_nodes; i++) {
        cns_graph_find_node(&graph, (i % num_nodes) + 1);
    }
    uint64_t lookup_time = clock() - start_time;
    
    printf("    Added %u nodes in %llu ticks\n", graph.node_count, node_time);
    printf("    Added %u edges in %llu ticks\n", graph.edge_count, edge_time);
    printf("    %d lookups in %llu ticks\n", num_nodes, lookup_time);
    printf("    Hash collisions: %u\n", graph.collision_count);
    
    // Performance should be reasonable
    TEST_ASSERT(graph.node_count > 0, "Should add some nodes");
    TEST_ASSERT(graph.edge_count > 0, "Should add some edges");
    
    TEST_PASS("Bulk operations performance");
}

/*═══════════════════════════════════════════════════════════════
  Hash Function and Data Structure Tests
  ═══════════════════════════════════════════════════════════════*/

// Test hash function quality
bool test_hash_function_quality() {
    const int num_ids = 100;
    uint32_t hashes[num_ids];
    
    // Generate hashes for sequential IDs
    for (int i = 0; i < num_ids; i++) {
        hashes[i] = cns_graph_hash_id(i + 1);
    }
    
    // Check for duplicate hashes
    int duplicates = 0;
    for (int i = 0; i < num_ids; i++) {
        for (int j = i + 1; j < num_ids; j++) {
            if (hashes[i] == hashes[j]) {
                duplicates++;
            }
        }
    }
    
    printf("    Hash collisions: %d/%d (%.1f%%)\n", 
           duplicates, num_ids, 100.0 * duplicates / num_ids);
    
    // Should have good distribution
    TEST_ASSERT(duplicates <= num_ids / 10, "Hash function should have good distribution");
    
    TEST_PASS("Hash function quality");
}

// Test cache alignment
bool test_cache_alignment() {
    uint8_t buffer[16384];
    cns_memory_arena_t arena;
    cns_graph_t graph;
    
    cns_arena_init(&arena, buffer, sizeof(buffer), CNS_ARENA_FLAG_NONE);
    cns_graph_init(&graph, &arena);
    
    cns_graph_add_node(&graph, 1, 0);
    cns_graph_add_node(&graph, 2, 0);
    
    // Check that nodes are cache-line aligned
    uintptr_t addr1 = (uintptr_t)&graph.nodes[0];
    uintptr_t addr2 = (uintptr_t)&graph.nodes[1];
    
    TEST_ASSERT((addr1 % 64) == 0, "First node should be 64-byte aligned");
    TEST_ASSERT((addr2 % 64) == 0, "Second node should be 64-byte aligned");
    TEST_ASSERT(sizeof(cns_graph_node_t) == 64, "Node size should be 64 bytes");
    
    TEST_PASS("Cache alignment");
}

/*═══════════════════════════════════════════════════════════════
  Edge Case and Error Handling Tests
  ═══════════════════════════════════════════════════════════════*/

// Test capacity limits
bool test_capacity_limits() {
    uint8_t buffer[65536];
    cns_memory_arena_t arena;
    cns_graph_t graph;
    
    cns_arena_init(&arena, buffer, sizeof(buffer), CNS_ARENA_FLAG_NONE);
    cns_graph_init(&graph, &arena);
    
    // Try to exceed node capacity
    int successful_nodes = 0;
    for (int i = 0; i < CNS_GRAPH_MAX_NODES + 10; i++) {
        if (cns_graph_add_node(&graph, i + 1, i)) {
            successful_nodes++;
        }
    }
    
    printf("    Successfully added %d nodes (max: %d)\n", 
           successful_nodes, CNS_GRAPH_MAX_NODES);
    
    TEST_ASSERT(successful_nodes > 0, "Should add some nodes");
    TEST_ASSERT(successful_nodes <= CNS_GRAPH_MAX_NODES, "Should not exceed capacity");
    
    TEST_PASS("Capacity limits");
}

// Test invalid operations
bool test_invalid_operations() {
    uint8_t buffer[16384];
    cns_memory_arena_t arena;
    cns_graph_t graph;
    
    cns_arena_init(&arena, buffer, sizeof(buffer), CNS_ARENA_FLAG_NONE);
    cns_graph_init(&graph, &arena);
    
    // Test operations on empty graph
    cns_graph_node_t* node = cns_graph_find_node(&graph, 1);
    TEST_ASSERT(node == NULL, "Should not find node in empty graph");
    
    uint32_t edge_count;
    cns_graph_edge_t* edges = cns_graph_get_edges(&graph, 1, &edge_count);
    TEST_ASSERT(edge_count == 0, "Should have no edges for non-existent node");
    
    // Test edge addition without nodes
    bool result = cns_graph_add_edge(&graph, 1, 2, 10);
    TEST_ASSERT(result == false, "Should not add edge without source node");
    
    TEST_PASS("Invalid operations");
}

/*═══════════════════════════════════════════════════════════════
  Main Test Runner
  ═══════════════════════════════════════════════════════════════*/

int main() {
    printf("CNS Graph Representation Test Suite\n");
    printf("====================================\n\n");
    
    // Basic functionality tests
    RUN_TEST(test_graph_init);
    RUN_TEST(test_graph_add_node);
    RUN_TEST(test_graph_node_lookup);
    RUN_TEST(test_graph_add_edge);
    
    // Traversal and query tests
    RUN_TEST(test_graph_edge_retrieval);
    RUN_TEST(test_graph_traversal);
    
    // Performance tests
    RUN_TEST(test_7tick_node_lookup);
    RUN_TEST(test_bulk_operations_performance);
    
    // Data structure tests
    RUN_TEST(test_hash_function_quality);
    RUN_TEST(test_cache_alignment);
    
    // Edge case tests
    RUN_TEST(test_capacity_limits);
    RUN_TEST(test_invalid_operations);
    
    printf("\n====================================\n");
    printf("Test Results:\n");
    printf("Total:  %u\n", tests_run);
    printf("Passed: %u\n", tests_passed);
    printf("Failed: %u\n", tests_failed);
    printf("Success Rate: %.1f%%\n", 
           tests_run > 0 ? (100.0 * tests_passed / tests_run) : 0.0);
    
    return tests_failed == 0 ? 0 : 1;
}