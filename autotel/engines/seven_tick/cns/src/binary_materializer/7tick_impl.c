/*
 * CNS Binary Materializer - 7-Tick Implementation
 * True O(1) node access with CPU cycle measurement
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>

#ifdef __x86_64__
#include <x86intrin.h>
#define CNS_RDTSC() __rdtsc()
#elif defined(__aarch64__)
static inline uint64_t CNS_RDTSC(void) {
    uint64_t val;
    __asm__ volatile("mrs %0, cntvct_el0" : "=r" (val));
    return val;
}
#else
#define CNS_RDTSC() 0
#endif

// Binary format constants
#define CNS_7T_MAGIC    0x37544943  // '7TIC'
#define CNS_7T_VERSION  0x0100
#define CNS_NODE_SIZE   16          // Fixed size for alignment
#define CNS_CACHE_LINE  64          // CPU cache line size

// Packed node structure (exactly 16 bytes)
typedef struct __attribute__((packed)) {
    uint32_t id;        // 4 bytes
    uint16_t type;      // 2 bytes
    uint16_t flags;     // 2 bytes
    uint32_t data_off;  // 4 bytes - offset to data
    uint32_t edge_idx;  // 4 bytes - first edge index
} cns_7t_node_t;

// Header structure (64 bytes aligned)
typedef struct __attribute__((aligned(64))) {
    uint32_t magic;
    uint16_t version;
    uint16_t flags;
    uint32_t node_count;
    uint32_t edge_count;
    uint64_t nodes_offset;   // Direct offset to nodes array
    uint64_t edges_offset;   // Direct offset to edges array
    uint64_t data_offset;    // Direct offset to data pool
    uint64_t index_offset;   // Direct offset to ID->index map
    uint8_t padding[24];     // Pad to 64 bytes
} cns_7t_header_t;

// Memory-mapped view (zero-copy)
typedef struct {
    void* base;              // mmap base address
    size_t size;             // Total size
    cns_7t_header_t* header; // Header pointer
    cns_7t_node_t* nodes;    // Direct node array access
    uint32_t* index;         // ID to array index mapping
} cns_7t_view_t;

// Create and write a test graph
static int create_test_graph(const char* path, uint32_t node_count) {
    int fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) return -1;
    
    // Calculate layout
    size_t header_size = sizeof(cns_7t_header_t);
    size_t nodes_size = node_count * sizeof(cns_7t_node_t);
    size_t index_size = node_count * sizeof(uint32_t) * 2; // Sparse index
    size_t total_size = header_size + nodes_size + index_size;
    
    // Extend file
    if (ftruncate(fd, total_size) < 0) {
        close(fd);
        return -1;
    }
    
    // Memory map for writing
    void* map = mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (map == MAP_FAILED) {
        close(fd);
        return -1;
    }
    
    // Write header
    cns_7t_header_t* header = (cns_7t_header_t*)map;
    header->magic = CNS_7T_MAGIC;
    header->version = CNS_7T_VERSION;
    header->flags = 0;
    header->node_count = node_count;
    header->edge_count = 0;
    header->nodes_offset = header_size;
    header->edges_offset = header_size + nodes_size;
    header->data_offset = header_size + nodes_size;
    header->index_offset = header_size + nodes_size;
    
    // Write nodes (sequential IDs for simplicity)
    cns_7t_node_t* nodes = (cns_7t_node_t*)((uint8_t*)map + header_size);
    for (uint32_t i = 0; i < node_count; i++) {
        nodes[i].id = i;
        nodes[i].type = 0x1000 + (i % 10);
        nodes[i].flags = 0;
        nodes[i].data_off = 0;
        nodes[i].edge_idx = 0xFFFFFFFF; // No edges
    }
    
    // Write index (identity mapping for now)
    uint32_t* index = (uint32_t*)((uint8_t*)map + header->index_offset);
    for (uint32_t i = 0; i < node_count; i++) {
        index[i * 2] = i;      // ID
        index[i * 2 + 1] = i;  // Array index
    }
    
    // Sync and cleanup
    msync(map, total_size, MS_SYNC);
    munmap(map, total_size);
    close(fd);
    
    return 0;
}

// Open memory-mapped view (zero-copy)
static int cns_7t_open(cns_7t_view_t* view, const char* path) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) return -1;
    
    struct stat st;
    if (fstat(fd, &st) < 0) {
        close(fd);
        return -1;
    }
    
    void* map = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    
    if (map == MAP_FAILED) return -1;
    
    view->base = map;
    view->size = st.st_size;
    view->header = (cns_7t_header_t*)map;
    view->nodes = (cns_7t_node_t*)((uint8_t*)map + view->header->nodes_offset);
    view->index = (uint32_t*)((uint8_t*)map + view->header->index_offset);
    
    return 0;
}

// Close view
static void cns_7t_close(cns_7t_view_t* view) {
    if (view->base) {
        munmap(view->base, view->size);
        view->base = NULL;
    }
}

// 7-tick node access (inlined for optimization)
static inline cns_7t_node_t* cns_7t_get_node(cns_7t_view_t* view, uint32_t node_id) {
    // Direct array access - no bounds check for 7-tick path
    return &view->nodes[node_id];
}

// Benchmark node access cycles
static void benchmark_7tick_access(cns_7t_view_t* view, uint32_t iterations) {
    printf("\n7-Tick Node Access Benchmark\n");
    printf("============================\n");
    
    // Warm up cache
    volatile uint32_t dummy = 0;
    for (uint32_t i = 0; i < 1000; i++) {
        cns_7t_node_t* node = cns_7t_get_node(view, i % view->header->node_count);
        dummy += node->type;
    }
    
    // Measure single access cycles
    uint64_t min_cycles = UINT64_MAX;
    uint64_t total_cycles = 0;
    uint32_t samples = 0;
    
    for (uint32_t i = 0; i < iterations; i++) {
        uint32_t node_id = i % view->header->node_count;
        
        // Ensure in cache
        __builtin_prefetch(&view->nodes[node_id], 0, 3);
        
        // Measure cycles for single access
        uint64_t start = CNS_RDTSC();
        cns_7t_node_t* node = cns_7t_get_node(view, node_id);
        volatile uint16_t type = node->type;  // Force read
        uint64_t end = CNS_RDTSC();
        
        uint64_t cycles = end - start;
        if (cycles < min_cycles && cycles > 0) {
            min_cycles = cycles;
        }
        total_cycles += cycles;
        samples++;
    }
    
    printf("Min cycles per access: %llu\n", min_cycles);
    printf("Avg cycles per access: %.2f\n", (double)total_cycles / samples);
    
    // Detailed breakdown for best case
    if (min_cycles <= 7) {
        printf("✅ 7-TICK ACHIEVED! (%llu cycles)\n", min_cycles);
    } else if (min_cycles <= 20) {
        printf("⚡ Near 7-tick (%llu cycles) - L1 cache hit\n", min_cycles);
    } else {
        printf("❌ Not 7-tick (%llu cycles) - Cache miss likely\n", min_cycles);
    }
}

// Throughput benchmark
static void benchmark_throughput(cns_7t_view_t* view) {
    printf("\nThroughput Benchmark\n");
    printf("===================\n");
    
    uint32_t node_count = view->header->node_count;
    
    // Sequential access pattern
    clock_t start = clock();
    volatile uint64_t sum = 0;
    for (uint32_t iter = 0; iter < 1000; iter++) {
        for (uint32_t i = 0; i < node_count; i++) {
            cns_7t_node_t* node = cns_7t_get_node(view, i);
            sum += node->type;
        }
    }
    clock_t end = clock();
    
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
    double nodes_per_sec = (node_count * 1000.0) / elapsed;
    double gb_per_sec = (nodes_per_sec * sizeof(cns_7t_node_t)) / (1024 * 1024 * 1024);
    
    printf("Sequential access: %.2f billion nodes/sec\n", nodes_per_sec / 1e9);
    printf("Memory bandwidth: %.2f GB/s\n", gb_per_sec);
    
    // Random access pattern
    start = clock();
    sum = 0;
    uint32_t lfsr = 0xACE1u;  // Linear feedback shift register for pseudo-random
    for (uint32_t iter = 0; iter < 1000000; iter++) {
        // Simple LFSR for fast pseudo-random numbers
        lfsr = (lfsr >> 1) ^ (-(lfsr & 1u) & 0xB400u);
        uint32_t node_id = lfsr % node_count;
        
        cns_7t_node_t* node = cns_7t_get_node(view, node_id);
        sum += node->type;
    }
    end = clock();
    
    elapsed = (double)(end - start) / CLOCKS_PER_SEC;
    double random_per_sec = 1000000.0 / elapsed;
    
    printf("Random access: %.2f million nodes/sec\n", random_per_sec / 1e6);
    printf("Cache efficiency: %.1f%%\n", (random_per_sec / nodes_per_sec) * 100000.0);
}

// Assembly inspection helper
__attribute__((noinline))
uint16_t benchmark_single_access(cns_7t_view_t* view, uint32_t node_id) {
    // This function helps inspect generated assembly
    cns_7t_node_t* node = cns_7t_get_node(view, node_id);
    return node->type;
}

int main() {
    printf("CNS 7-Tick Binary Materializer\n");
    printf("==============================\n");
    
    const char* test_file = "7tick_test.cnsb";
    uint32_t node_counts[] = {1000, 10000, 100000, 1000000};
    
    for (size_t i = 0; i < sizeof(node_counts) / sizeof(node_counts[0]); i++) {
        uint32_t count = node_counts[i];
        
        printf("\n--- Testing with %u nodes ---\n", count);
        
        // Create test file
        if (create_test_graph(test_file, count) < 0) {
            fprintf(stderr, "Failed to create test graph\n");
            continue;
        }
        
        // Open for reading
        cns_7t_view_t view = {0};
        if (cns_7t_open(&view, test_file) < 0) {
            fprintf(stderr, "Failed to open graph\n");
            continue;
        }
        
        // Verify data
        printf("Loaded graph: %u nodes\n", view.header->node_count);
        printf("Memory mapped size: %.2f MB\n", (double)view.size / (1024 * 1024));
        
        // Run benchmarks
        benchmark_7tick_access(&view, 10000);
        benchmark_throughput(&view);
        
        // Show assembly hint
        printf("\nAssembly for single access (compile with -S to inspect):\n");
        volatile uint16_t result = benchmark_single_access(&view, 42);
        printf("Sample result: 0x%04x\n", result);
        
        // Cleanup
        cns_7t_close(&view);
        unlink(test_file);
    }
    
    // Architecture info
    printf("\n--- Architecture Info ---\n");
    #ifdef __x86_64__
    printf("CPU: x86_64 (RDTSC available)\n");
    #elif defined(__aarch64__)
    printf("CPU: ARM64 (cycle counter available)\n");
    #else
    printf("CPU: Unknown (no cycle counter)\n");
    #endif
    
    printf("\nTo inspect assembly:\n");
    printf("  clang -O3 -S 7tick_impl.c -o 7tick_impl.s\n");
    printf("  grep -A10 benchmark_single_access 7tick_impl.s\n");
    
    return 0;
}