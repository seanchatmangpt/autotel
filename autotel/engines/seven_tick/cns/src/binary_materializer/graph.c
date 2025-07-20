// graph.c - Complete binary materializer in one file
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

// Constants
#define MAX_EDGES_PER_NODE 100
#define HASH_SIZE 16384

// Node structure with inline edge storage
typedef struct {
    uint32_t id;
    uint32_t edge_count;
    uint32_t edges[MAX_EDGES_PER_NODE];
} node_t;

// Graph structure with hash-based node lookup
typedef struct {
    node_t** nodes;
    uint32_t* hash_table;
    uint32_t node_count;
    uint32_t capacity;
} graph_t;

// Hash function for node lookup
static inline uint32_t hash_id(uint32_t id) {
    return (id * 2654435761U) & (HASH_SIZE - 1);
}

// Create new graph
graph_t* create_graph(uint32_t initial_capacity) {
    graph_t* g = malloc(sizeof(graph_t));
    g->capacity = initial_capacity;
    g->node_count = 0;
    g->nodes = calloc(initial_capacity, sizeof(node_t*));
    g->hash_table = calloc(HASH_SIZE, sizeof(uint32_t));
    return g;
}

// Add node to graph
void add_node(graph_t* g, uint32_t id) {
    if (g->node_count >= g->capacity) {
        g->capacity *= 2;
        g->nodes = realloc(g->nodes, g->capacity * sizeof(node_t*));
    }
    
    node_t* node = malloc(sizeof(node_t));
    node->id = id;
    node->edge_count = 0;
    
    uint32_t idx = g->node_count++;
    g->nodes[idx] = node;
    
    // Add to hash table for fast lookup
    uint32_t hash = hash_id(id);
    g->hash_table[hash] = idx + 1; // +1 to distinguish from 0
}

// Find node by ID
node_t* find_node(graph_t* g, uint32_t id) {
    uint32_t hash = hash_id(id);
    uint32_t idx = g->hash_table[hash];
    if (idx > 0 && idx <= g->node_count) {
        node_t* node = g->nodes[idx - 1];
        if (node->id == id) return node;
    }
    
    // Linear search fallback
    for (uint32_t i = 0; i < g->node_count; i++) {
        if (g->nodes[i]->id == id) {
            g->hash_table[hash] = i + 1;
            return g->nodes[i];
        }
    }
    return NULL;
}

// Add edge between nodes
void add_edge(graph_t* g, uint32_t from, uint32_t to) {
    node_t* from_node = find_node(g, from);
    if (!from_node || from_node->edge_count >= MAX_EDGES_PER_NODE) return;
    
    from_node->edges[from_node->edge_count++] = to;
}

// Serialize graph to binary file
void serialize_graph(graph_t* g, const char* filename) {
    FILE* f = fopen(filename, "wb");
    if (!f) return;
    
    // Write header
    fwrite(&g->node_count, sizeof(uint32_t), 1, f);
    
    // Write nodes
    for (uint32_t i = 0; i < g->node_count; i++) {
        node_t* node = g->nodes[i];
        fwrite(&node->id, sizeof(uint32_t), 1, f);
        fwrite(&node->edge_count, sizeof(uint32_t), 1, f);
        fwrite(node->edges, sizeof(uint32_t), node->edge_count, f);
    }
    
    fclose(f);
}

// Deserialize graph from binary file
graph_t* deserialize_graph(const char* filename) {
    FILE* f = fopen(filename, "rb");
    if (!f) return NULL;
    
    uint32_t node_count;
    fread(&node_count, sizeof(uint32_t), 1, f);
    
    graph_t* g = create_graph(node_count);
    
    // Read nodes
    for (uint32_t i = 0; i < node_count; i++) {
        node_t* node = malloc(sizeof(node_t));
        fread(&node->id, sizeof(uint32_t), 1, f);
        fread(&node->edge_count, sizeof(uint32_t), 1, f);
        fread(node->edges, sizeof(uint32_t), node->edge_count, f);
        
        g->nodes[g->node_count++] = node;
        uint32_t hash = hash_id(node->id);
        g->hash_table[hash] = i + 1;
    }
    
    fclose(f);
    return g;
}

// Free graph memory
void free_graph(graph_t* g) {
    for (uint32_t i = 0; i < g->node_count; i++) {
        free(g->nodes[i]);
    }
    free(g->nodes);
    free(g->hash_table);
    free(g);
}

// Get current time in microseconds
double get_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

// Benchmark function
void benchmark_operations(graph_t* g, int iterations) {
    double start, end;
    
    // Benchmark node lookup
    start = get_time();
    for (int i = 0; i < iterations; i++) {
        uint32_t id = rand() % g->node_count;
        find_node(g, id);
    }
    end = get_time();
    double lookup_time = end - start;
    double lookup_ops = iterations / lookup_time;
    
    // Benchmark edge traversal
    start = get_time();
    int edge_count = 0;
    for (int i = 0; i < iterations; i++) {
        node_t* node = g->nodes[rand() % g->node_count];
        edge_count += node->edge_count;
    }
    end = get_time();
    double traverse_time = end - start;
    double traverse_ops = iterations / traverse_time;
    
    printf("Performance Results:\n");
    printf("  Node Lookups: %.0f ops/sec\n", lookup_ops);
    printf("  Edge Traversals: %.0f ops/sec\n", traverse_ops);
    printf("  Average: %.0f ops/sec\n", (lookup_ops + traverse_ops) / 2);
}

// Demo usage
int main() {
    printf("Binary Materializer Demo\n");
    printf("========================\n\n");
    
    // Create graph with 10K nodes
    printf("Creating graph with 10,000 nodes...\n");
    graph_t* g = create_graph(10000);
    
    for (uint32_t i = 0; i < 10000; i++) {
        add_node(g, i);
    }
    
    // Add random edges (average 10 per node)
    printf("Adding ~100,000 edges...\n");
    srand(time(NULL));
    for (uint32_t i = 0; i < 10000; i++) {
        int edge_count = 5 + rand() % 10;
        for (int j = 0; j < edge_count; j++) {
            add_edge(g, i, rand() % 10000);
        }
    }
    
    // Serialize to binary
    printf("Serializing to graph.bin...\n");
    double start = get_time();
    serialize_graph(g, "graph.bin");
    double serialize_time = get_time() - start;
    printf("  Serialization time: %.3f seconds\n", serialize_time);
    
    // Free original graph
    free_graph(g);
    
    // Deserialize from binary
    printf("\nDeserializing from graph.bin...\n");
    start = get_time();
    g = deserialize_graph("graph.bin");
    double deserialize_time = get_time() - start;
    printf("  Deserialization time: %.3f seconds\n", deserialize_time);
    
    // Verify graph
    printf("\nGraph Statistics:\n");
    printf("  Nodes: %u\n", g->node_count);
    uint64_t total_edges = 0;
    for (uint32_t i = 0; i < g->node_count; i++) {
        total_edges += g->nodes[i]->edge_count;
    }
    printf("  Total Edges: %llu\n", total_edges);
    
    // Run benchmarks
    printf("\nRunning performance benchmarks...\n");
    benchmark_operations(g, 1000000);
    
    // Example queries
    printf("\nExample Queries:\n");
    
    // Find specific node
    node_t* node = find_node(g, 42);
    if (node) {
        printf("  Node 42 has %u edges\n", node->edge_count);
        printf("  First 5 edges: ");
        for (uint32_t i = 0; i < 5 && i < node->edge_count; i++) {
            printf("%u ", node->edges[i]);
        }
        printf("\n");
    }
    
    // Path finding example (BFS)
    printf("\nFinding path from node 0 to node 100...\n");
    uint32_t* visited = calloc(g->node_count, sizeof(uint32_t));
    uint32_t* queue = malloc(g->node_count * sizeof(uint32_t));
    uint32_t front = 0, rear = 0;
    
    queue[rear++] = 0;
    visited[0] = 1;
    int found = 0;
    
    while (front < rear && !found) {
        uint32_t current = queue[front++];
        node_t* n = find_node(g, current);
        
        for (uint32_t i = 0; i < n->edge_count && !found; i++) {
            uint32_t next = n->edges[i];
            if (next == 100) {
                printf("  Path found! Length: %u\n", front);
                found = 1;
            }
            if (!visited[next]) {
                visited[next] = 1;
                queue[rear++] = next;
            }
        }
    }
    
    if (!found) printf("  No path found\n");
    
    free(visited);
    free(queue);
    
    // Calculate file size
    FILE* f = fopen("graph.bin", "rb");
    if (f) {
        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        fclose(f);
        printf("\nBinary file size: %.2f MB\n", size / (1024.0 * 1024.0));
        printf("Compression ratio: %.2fx vs naive storage\n", 
               (double)(g->node_count * sizeof(node_t)) / size);
    }
    
    // Cleanup
    free_graph(g);
    
    printf("\nDemo complete!\n");
    return 0;
}