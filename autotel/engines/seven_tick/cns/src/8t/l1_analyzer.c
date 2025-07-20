/*
 * 8T L1 Cache Usage Analyzer
 * Real-time L1 cache line analysis for 8-tick optimization
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <immintrin.h>
#include <assert.h>
#include <time.h>
#include <sys/mman.h>

#include "cns/8t/core.h"
#include "cns/8t/analyzer.h"

// ============================================================================
// L1 CACHE ANALYSIS TYPES
// ============================================================================

#define CNS_8T_L1_CACHE_SIZE    32768    // 32KB L1 data cache (typical)
#define CNS_8T_L1_LINE_SIZE     64       // 64-byte cache lines
#define CNS_8T_L1_LINES         (CNS_8T_L1_CACHE_SIZE / CNS_8T_L1_LINE_SIZE)
#define CNS_8T_L1_ASSOCIATIVITY 8        // 8-way set associative (typical)
#define CNS_8T_L1_SETS          (CNS_8T_L1_LINES / CNS_8T_L1_ASSOCIATIVITY)

// L1 cache line state
typedef enum {
    CNS_8T_L1_INVALID,     // Cache line is invalid
    CNS_8T_L1_SHARED,      // Cache line is shared (read-only)
    CNS_8T_L1_EXCLUSIVE,   // Cache line is exclusive (read-write)
    CNS_8T_L1_MODIFIED     // Cache line is modified (dirty)
} cns_8t_l1_state_t;

// L1 cache line metadata
typedef struct {
    uint64_t address;           // Physical address (cache line aligned)
    uint32_t access_count;      // Number of accesses
    uint32_t miss_count;        // Number of misses
    cns_8t_l1_state_t state;    // Current cache line state
    cns_tick_t last_access;     // Last access timestamp
    uint16_t age;               // Age counter for LRU
    uint8_t access_pattern;     // Sequential, random, etc.
    uint8_t reserved;
} cns_8t_l1_line_info_t __attribute__((aligned(32)));

// L1 set information (8 cache lines per set)
typedef struct {
    cns_8t_l1_line_info_t lines[CNS_8T_L1_ASSOCIATIVITY];
    uint32_t hit_count;         // Hits in this set
    uint32_t miss_count;        // Misses in this set
    uint16_t lru_counter;       // LRU replacement counter
    uint16_t reserved;
} cns_8t_l1_set_info_t __attribute__((aligned(64)));

// Memory access pattern analysis
typedef struct {
    uint64_t address;           // Memory address accessed
    cns_tick_t timestamp;       // When the access occurred
    uint8_t access_type;        // Read, write, prefetch
    uint8_t cache_hit;          // Was it a cache hit?
    uint16_t latency_ticks;     // Access latency in ticks
} cns_8t_memory_access_t __attribute__((aligned(16)));

// Access pattern ring buffer
#define CNS_8T_ACCESS_BUFFER_SIZE 8192
typedef struct {
    cns_8t_memory_access_t accesses[CNS_8T_ACCESS_BUFFER_SIZE];
    uint32_t head;              // Next write position
    uint32_t tail;              // Start of valid data
    uint32_t count;             // Number of valid entries
    uint32_t total_accesses;    // Total accesses recorded
} cns_8t_access_buffer_t __attribute__((aligned(64)));

// L1 cache analyzer context
typedef struct {
    // Cache model state
    cns_8t_l1_set_info_t sets[CNS_8T_L1_SETS];
    
    // Access tracking
    cns_8t_access_buffer_t access_buffer;
    
    // Performance metrics
    uint64_t total_accesses;
    uint64_t total_hits;
    uint64_t total_misses;
    uint64_t sequential_accesses;
    uint64_t random_accesses;
    uint64_t prefetch_hits;
    uint64_t write_backs;
    
    // Timing analysis
    cns_tick_t start_time;
    cns_tick_t end_time;
    uint32_t tick_samples;
    uint32_t violations_8t;     // Operations exceeding 8 ticks
    
    // Configuration
    bool enable_prefetch_sim;   // Simulate hardware prefetcher
    bool strict_8t_mode;        // Strict 8-tick enforcement
    uint32_t prefetch_distance; // Prefetch lookahead
    
    // Memory layout analysis
    uint64_t hot_regions[16];   // Frequently accessed memory regions
    uint32_t hot_region_count;
    
} cns_8t_l1_analyzer_t __attribute__((aligned(64)));

// ============================================================================
// L1 CACHE SIMULATION
// ============================================================================

static inline uint32_t cns_8t_l1_get_set_index(uint64_t address) {
    // Extract set index from address (assuming 64-byte lines)
    return (address >> 6) & (CNS_8T_L1_SETS - 1);
}

static inline uint64_t cns_8t_l1_get_tag(uint64_t address) {
    // Extract tag from address
    return address & ~((1ULL << 6) - 1);  // Clear line offset bits
}

static cns_8t_l1_line_info_t* 
cns_8t_l1_find_line(cns_8t_l1_set_info_t* set, uint64_t tag) {
    for (int i = 0; i < CNS_8T_L1_ASSOCIATIVITY; i++) {
        if (set->lines[i].state != CNS_8T_L1_INVALID && 
            set->lines[i].address == tag) {
            return &set->lines[i];
        }
    }
    return NULL;
}

static cns_8t_l1_line_info_t* 
cns_8t_l1_find_lru_line(cns_8t_l1_set_info_t* set) {
    cns_8t_l1_line_info_t* lru_line = &set->lines[0];
    uint16_t oldest_age = lru_line->age;
    
    for (int i = 1; i < CNS_8T_L1_ASSOCIATIVITY; i++) {
        if (set->lines[i].state == CNS_8T_L1_INVALID) {
            return &set->lines[i];  // Found empty line
        }
        if (set->lines[i].age > oldest_age) {
            oldest_age = set->lines[i].age;
            lru_line = &set->lines[i];
        }
    }
    
    return lru_line;
}

static void cns_8t_l1_update_lru(cns_8t_l1_set_info_t* set, cns_8t_l1_line_info_t* accessed_line) {
    // Update age for all lines in set
    for (int i = 0; i < CNS_8T_L1_ASSOCIATIVITY; i++) {
        if (&set->lines[i] != accessed_line) {
            set->lines[i].age++;
        }
    }
    accessed_line->age = 0;  // Most recently used
}

static bool cns_8t_l1_simulate_access(cns_8t_l1_analyzer_t* analyzer, 
                                       uint64_t address, 
                                       bool is_write,
                                       cns_tick_t* latency_out) {
    uint32_t set_index = cns_8t_l1_get_set_index(address);
    uint64_t tag = cns_8t_l1_get_tag(address);
    cns_8t_l1_set_info_t* set = &analyzer->sets[set_index];
    
    // Look for existing cache line
    cns_8t_l1_line_info_t* line = cns_8t_l1_find_line(set, tag);
    
    if (line) {
        // Cache hit!
        line->access_count++;
        line->last_access = cns_get_tick_count();
        cns_8t_l1_update_lru(set, line);
        
        set->hit_count++;
        analyzer->total_hits++;
        
        if (is_write && line->state == CNS_8T_L1_SHARED) {
            line->state = CNS_8T_L1_MODIFIED;
        }
        
        *latency_out = 1;  // L1 hit = 1 tick
        return true;
    }
    
    // Cache miss - need to load from L2/memory
    set->miss_count++;
    analyzer->total_misses++;
    
    // Find LRU line for replacement
    cns_8t_l1_line_info_t* victim = cns_8t_l1_find_lru_line(set);
    
    // If victim is dirty, simulate writeback
    if (victim->state == CNS_8T_L1_MODIFIED) {
        analyzer->write_backs++;
    }
    
    // Install new line
    victim->address = tag;
    victim->access_count = 1;
    victim->miss_count = 1;
    victim->state = is_write ? CNS_8T_L1_MODIFIED : CNS_8T_L1_EXCLUSIVE;
    victim->last_access = cns_get_tick_count();
    victim->age = 0;
    
    cns_8t_l1_update_lru(set, victim);
    
    // L2 hit typically takes ~10 ticks, memory ~300 ticks
    // For simplicity, assume L2 hit
    *latency_out = 10;
    
    return false;  // Cache miss
}

// ============================================================================
// ACCESS PATTERN ANALYSIS
// ============================================================================

static void cns_8t_record_access(cns_8t_l1_analyzer_t* analyzer,
                                  uint64_t address,
                                  uint8_t access_type,
                                  bool cache_hit,
                                  uint16_t latency_ticks) {
    cns_8t_access_buffer_t* buffer = &analyzer->access_buffer;
    
    // Add to ring buffer
    uint32_t index = buffer->head;
    buffer->accesses[index].address = address;
    buffer->accesses[index].timestamp = cns_get_tick_count();
    buffer->accesses[index].access_type = access_type;
    buffer->accesses[index].cache_hit = cache_hit ? 1 : 0;
    buffer->accesses[index].latency_ticks = latency_ticks;
    
    buffer->head = (buffer->head + 1) % CNS_8T_ACCESS_BUFFER_SIZE;
    if (buffer->count < CNS_8T_ACCESS_BUFFER_SIZE) {
        buffer->count++;
    } else {
        buffer->tail = (buffer->tail + 1) % CNS_8T_ACCESS_BUFFER_SIZE;
    }
    buffer->total_accesses++;
}

static void cns_8t_analyze_access_patterns(cns_8t_l1_analyzer_t* analyzer) {
    cns_8t_access_buffer_t* buffer = &analyzer->access_buffer;
    
    if (buffer->count < 2) return;
    
    // Analyze stride patterns
    uint64_t sequential_count = 0;
    uint64_t random_count = 0;
    
    for (uint32_t i = 1; i < buffer->count; i++) {
        uint32_t curr_idx = (buffer->tail + i) % CNS_8T_ACCESS_BUFFER_SIZE;
        uint32_t prev_idx = (buffer->tail + i - 1) % CNS_8T_ACCESS_BUFFER_SIZE;
        
        uint64_t curr_addr = buffer->accesses[curr_idx].address;
        uint64_t prev_addr = buffer->accesses[prev_idx].address;
        
        int64_t stride = (int64_t)curr_addr - (int64_t)prev_addr;
        
        if (abs(stride) <= 128) {  // Within 2 cache lines
            sequential_count++;
        } else {
            random_count++;
        }
    }
    
    analyzer->sequential_accesses += sequential_count;
    analyzer->random_accesses += random_count;
}

// ============================================================================
// HARDWARE PREFETCHER SIMULATION
// ============================================================================

static void cns_8t_simulate_prefetcher(cns_8t_l1_analyzer_t* analyzer,
                                        uint64_t address) {
    if (!analyzer->enable_prefetch_sim) return;
    
    // Simple next-line prefetcher simulation
    uint64_t prefetch_addr = address + CNS_8T_L1_LINE_SIZE;
    
    cns_tick_t prefetch_latency;
    bool was_hit = cns_8t_l1_simulate_access(analyzer, prefetch_addr, false, &prefetch_latency);
    
    if (was_hit) {
        analyzer->prefetch_hits++;
    }
}

// ============================================================================
// ANALYZER API
// ============================================================================

cns_8t_result_t cns_8t_l1_analyzer_create(cns_8t_l1_analyzer_t** analyzer_out) {
    if (!analyzer_out) return CNS_8T_ERROR_INVALID_PARAM;
    
    cns_8t_l1_analyzer_t* analyzer = aligned_alloc(64, sizeof(cns_8t_l1_analyzer_t));
    if (!analyzer) return CNS_8T_ERROR_MEMORY;
    
    memset(analyzer, 0, sizeof(*analyzer));
    
    // Initialize cache sets
    for (int i = 0; i < CNS_8T_L1_SETS; i++) {
        for (int j = 0; j < CNS_8T_L1_ASSOCIATIVITY; j++) {
            analyzer->sets[i].lines[j].state = CNS_8T_L1_INVALID;
        }
    }
    
    // Default configuration
    analyzer->enable_prefetch_sim = true;
    analyzer->strict_8t_mode = true;
    analyzer->prefetch_distance = 2;
    analyzer->start_time = cns_get_tick_count();
    
    *analyzer_out = analyzer;
    return CNS_8T_OK;
}

void cns_8t_l1_analyzer_destroy(cns_8t_l1_analyzer_t* analyzer) {
    if (!analyzer) return;
    free(analyzer);
}

cns_8t_result_t cns_8t_l1_analyze_memory_access(cns_8t_l1_analyzer_t* analyzer,
                                                 uint64_t address,
                                                 size_t size,
                                                 bool is_write,
                                                 cns_tick_t* total_latency_out) {
    if (!analyzer || !total_latency_out) return CNS_8T_ERROR_INVALID_PARAM;
    
    cns_tick_t start_tick = cns_get_tick_count();
    cns_tick_t total_latency = 0;
    
    analyzer->total_accesses++;
    
    // Handle accesses that span multiple cache lines
    uint64_t line_start = address & ~(CNS_8T_L1_LINE_SIZE - 1);
    uint64_t line_end = (address + size - 1) & ~(CNS_8T_L1_LINE_SIZE - 1);
    
    for (uint64_t line_addr = line_start; line_addr <= line_end; line_addr += CNS_8T_L1_LINE_SIZE) {
        cns_tick_t line_latency;
        bool cache_hit = cns_8t_l1_simulate_access(analyzer, line_addr, is_write, &line_latency);
        total_latency += line_latency;
        
        // Record access for pattern analysis
        cns_8t_record_access(analyzer, line_addr, is_write ? 1 : 0, cache_hit, line_latency);
        
        // Simulate hardware prefetcher
        cns_8t_simulate_prefetcher(analyzer, line_addr);
    }
    
    cns_tick_t end_tick = cns_get_tick_count();
    cns_tick_t operation_ticks = end_tick - start_tick;
    
    // Check 8-tick violation
    if (analyzer->strict_8t_mode && operation_ticks > CNS_8T_TICK_LIMIT) {
        analyzer->violations_8t++;
    }
    
    *total_latency_out = total_latency;
    return CNS_8T_OK;
}

cns_8t_result_t cns_8t_l1_analyze_data_structure(cns_8t_l1_analyzer_t* analyzer,
                                                  const void* data_ptr,
                                                  size_t data_size,
                                                  const char* structure_name) {
    if (!analyzer || !data_ptr || !structure_name) return CNS_8T_ERROR_INVALID_PARAM;
    
    printf("\nAnalyzing data structure: %s\n", structure_name);
    printf("Address: %p, Size: %zu bytes\n", data_ptr, data_size);
    
    uint64_t base_addr = (uint64_t)data_ptr;
    size_t cache_lines_used = (data_size + CNS_8T_L1_LINE_SIZE - 1) / CNS_8T_L1_LINE_SIZE;
    
    printf("Cache lines required: %zu\n", cache_lines_used);
    printf("Memory efficiency: %.1f%% (used bytes / cache line bytes)\n",
           (double)data_size / (cache_lines_used * CNS_8T_L1_LINE_SIZE) * 100.0);
    
    // Simulate sequential access to the entire structure
    cns_tick_t total_latency = 0;
    
    for (size_t offset = 0; offset < data_size; offset += sizeof(uint64_t)) {
        cns_tick_t access_latency;
        cns_8t_result_t result = cns_8t_l1_analyze_memory_access(
            analyzer,
            base_addr + offset,
            sizeof(uint64_t),
            false,  // read access
            &access_latency
        );
        
        if (result != CNS_8T_OK) return result;
        total_latency += access_latency;
    }
    
    size_t num_accesses = (data_size + sizeof(uint64_t) - 1) / sizeof(uint64_t);
    double avg_latency = (double)total_latency / num_accesses;
    
    printf("Average access latency: %.2f ticks\n", avg_latency);
    
    if (avg_latency <= 8.0) {
        printf("✅ Structure is 8T-compatible (%.2f ticks)\n", avg_latency);
    } else {
        printf("❌ Structure violates 8T constraint (%.2f ticks)\n", avg_latency);
    }
    
    return CNS_8T_OK;
}

cns_8t_result_t cns_8t_l1_get_cache_statistics(const cns_8t_l1_analyzer_t* analyzer,
                                                cns_8t_perf_metrics_t* stats_out) {
    if (!analyzer || !stats_out) return CNS_8T_ERROR_INVALID_PARAM;
    
    memset(stats_out, 0, sizeof(*stats_out));
    
    stats_out->start_tick = analyzer->start_time;
    stats_out->end_tick = cns_get_tick_count();
    stats_out->cache_hits = analyzer->total_hits;
    stats_out->cache_misses = analyzer->total_misses;
    stats_out->l1_cache_misses = analyzer->total_misses;
    
    // Calculate hit rate
    if (analyzer->total_accesses > 0) {
        double hit_rate = (double)analyzer->total_hits / analyzer->total_accesses;
        printf("\nL1 Cache Statistics:\n");
        printf("===================\n");
        printf("Total accesses: %lu\n", analyzer->total_accesses);
        printf("Cache hits: %lu\n", analyzer->total_hits);
        printf("Cache misses: %lu\n", analyzer->total_misses);
        printf("Hit rate: %.2f%%\n", hit_rate * 100.0);
        printf("Sequential accesses: %lu\n", analyzer->sequential_accesses);
        printf("Random accesses: %lu\n", analyzer->random_accesses);
        printf("Prefetch hits: %lu\n", analyzer->prefetch_hits);
        printf("Write backs: %lu\n", analyzer->write_backs);
        printf("8T violations: %u\n", analyzer->violations_8t);
    }
    
    return CNS_8T_OK;
}

// ============================================================================
// OPTIMIZATION RECOMMENDATIONS
// ============================================================================

cns_8t_result_t cns_8t_l1_generate_optimization_report(const cns_8t_l1_analyzer_t* analyzer) {
    if (!analyzer) return CNS_8T_ERROR_INVALID_PARAM;
    
    printf("\n8T L1 Cache Optimization Report\n");
    printf("===============================\n");
    
    double hit_rate = 0.0;
    if (analyzer->total_accesses > 0) {
        hit_rate = (double)analyzer->total_hits / analyzer->total_accesses;
    }
    
    printf("Overall Performance:\n");
    if (hit_rate >= 0.95) {
        printf("✅ Excellent L1 cache performance (%.1f%% hit rate)\n", hit_rate * 100.0);
    } else if (hit_rate >= 0.85) {
        printf("⚡ Good L1 cache performance (%.1f%% hit rate)\n", hit_rate * 100.0);
    } else {
        printf("❌ Poor L1 cache performance (%.1f%% hit rate)\n", hit_rate * 100.0);
    }
    
    printf("\nRecommendations:\n");
    
    if (hit_rate < 0.90) {
        printf("• Consider data structure reorganization for better spatial locality\n");
        printf("• Reduce working set size to fit in L1 cache (%dKB)\n", CNS_8T_L1_CACHE_SIZE / 1024);
    }
    
    if (analyzer->random_accesses > analyzer->sequential_accesses) {
        printf("• Access patterns are primarily random - consider:\n");
        printf("  - Hash table optimization\n");
        printf("  - Prefetching strategies\n");
        printf("  - Memory layout reorganization\n");
    }
    
    if (analyzer->violations_8t > 0) {
        printf("• %u operations exceeded 8-tick limit:\n", analyzer->violations_8t);
        printf("  - Review critical path algorithms\n");
        printf("  - Consider SIMD optimizations\n");
        printf("  - Ensure hot data fits in L1 cache\n");
    }
    
    if (analyzer->prefetch_hits < analyzer->total_misses / 10) {
        printf("• Hardware prefetcher effectiveness is low:\n");
        printf("  - Consider software prefetching\n");
        printf("  - Improve access predictability\n");
    }
    
    printf("\nMemory Layout Optimization:\n");
    printf("• Use 64-byte alignment for frequently accessed structures\n");
    printf("• Group related data together (structure of arrays vs array of structures)\n");
    printf("• Consider cache-oblivious algorithms for large datasets\n");
    
    return CNS_8T_OK;
}

// ============================================================================
// BENCHMARK FUNCTIONS
// ============================================================================

cns_8t_result_t cns_8t_l1_benchmark_access_patterns(void) {
    printf("\n8T L1 Cache Access Pattern Benchmark\n");
    printf("====================================\n");
    
    cns_8t_l1_analyzer_t* analyzer;
    cns_8t_result_t result = cns_8t_l1_analyzer_create(&analyzer);
    if (result != CNS_8T_OK) return result;
    
    // Test data
    const size_t test_size = 64 * 1024;  // 64KB test data
    uint8_t* test_data = aligned_alloc(64, test_size);
    if (!test_data) {
        cns_8t_l1_analyzer_destroy(analyzer);
        return CNS_8T_ERROR_MEMORY;
    }
    
    // Initialize test data
    for (size_t i = 0; i < test_size; i++) {
        test_data[i] = (uint8_t)(i & 0xFF);
    }
    
    printf("\nTesting sequential access pattern...\n");
    cns_tick_t seq_start = cns_get_tick_count();
    for (size_t i = 0; i < test_size; i += 8) {
        cns_tick_t latency;
        cns_8t_l1_analyze_memory_access(analyzer, (uint64_t)&test_data[i], 8, false, &latency);
    }
    cns_tick_t seq_end = cns_get_tick_count();
    
    printf("Sequential access completed in %lu ticks\n", seq_end - seq_start);
    
    // Reset analyzer for random access test
    cns_8t_l1_analyzer_destroy(analyzer);
    result = cns_8t_l1_analyzer_create(&analyzer);
    if (result != CNS_8T_OK) {
        free(test_data);
        return result;
    }
    
    printf("\nTesting random access pattern...\n");
    cns_tick_t rand_start = cns_get_tick_count();
    uint32_t lfsr = 0xACE1u;
    for (int i = 0; i < 1000; i++) {
        lfsr = (lfsr >> 1) ^ (-(lfsr & 1u) & 0xB400u);
        size_t offset = (lfsr % (test_size / 8)) * 8;
        
        cns_tick_t latency;
        cns_8t_l1_analyze_memory_access(analyzer, (uint64_t)&test_data[offset], 8, false, &latency);
    }
    cns_tick_t rand_end = cns_get_tick_count();
    
    printf("Random access completed in %lu ticks\n", rand_end - rand_start);
    
    // Generate performance report
    cns_8t_perf_metrics_t stats;
    cns_8t_l1_get_cache_statistics(analyzer, &stats);
    cns_8t_l1_generate_optimization_report(analyzer);
    
    free(test_data);
    cns_8t_l1_analyzer_destroy(analyzer);
    
    return CNS_8T_OK;
}

// Demo main function
#ifdef CNS_8T_L1_ANALYZER_STANDALONE
int main() {
    printf("CNS 8T L1 Cache Usage Analyzer\n");
    printf("==============================\n");
    
    // Run access pattern benchmark
    cns_8t_result_t result = cns_8t_l1_benchmark_access_patterns();
    if (result != CNS_8T_OK) {
        fprintf(stderr, "Benchmark failed with error: %d\n", result);
        return 1;
    }
    
    // Test with some common data structures
    cns_8t_l1_analyzer_t* analyzer;
    result = cns_8t_l1_analyzer_create(&analyzer);
    if (result == CNS_8T_OK) {
        // Analyze different data structure layouts
        struct test_struct_aos {
            uint32_t id;
            double value;
            char name[16];
        } aos_array[1000];
        
        cns_8t_l1_analyze_data_structure(analyzer, aos_array, sizeof(aos_array), "Array of Structures");
        
        struct test_struct_soa {
            uint32_t ids[1000];
            double values[1000];
            char names[1000][16];
        } soa_data;
        
        cns_8t_l1_analyze_data_structure(analyzer, &soa_data, sizeof(soa_data), "Structure of Arrays");
        
        cns_8t_l1_analyzer_destroy(analyzer);
    }
    
    return 0;
}
#endif