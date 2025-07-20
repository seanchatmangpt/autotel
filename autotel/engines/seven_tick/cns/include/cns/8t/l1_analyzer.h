#ifndef CNS_8T_L1_ANALYZER_H
#define CNS_8T_L1_ANALYZER_H

#include "cns/8t/core.h"
#include "cns/8t/arena_l1.h"
#include "cns/8t/graph_l1.h"
#include <immintrin.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// 8T L1 CACHE ANALYSIS TOOLS
// Real-time L1 cache performance analysis and optimization
// ============================================================================

// L1 cache architecture parameters
#define CNS_8T_L1_CACHE_SIZE_BYTES  (32 * 1024)    // 32KB L1 data cache
#define CNS_8T_L1_LINE_SIZE_BYTES   64              // 64-byte cache lines
#define CNS_8T_L1_ASSOCIATIVITY     8               // 8-way set associative
#define CNS_8T_L1_NUM_SETS          (CNS_8T_L1_CACHE_SIZE_BYTES / (CNS_8T_L1_LINE_SIZE_BYTES * CNS_8T_L1_ASSOCIATIVITY))

// Cache access patterns
typedef enum {
    CNS_8T_ACCESS_SEQUENTIAL,       // Sequential memory access
    CNS_8T_ACCESS_RANDOM,           // Random memory access
    CNS_8T_ACCESS_STRIDE,           // Strided access pattern
    CNS_8T_ACCESS_HOTSPOT,          // Hot spot access pattern
    CNS_8T_ACCESS_SCATTER_GATHER    // Scatter-gather pattern
} cns_8t_access_pattern_t;

// Cache miss types
typedef enum {
    CNS_8T_MISS_COMPULSORY,         // Cold start miss
    CNS_8T_MISS_CAPACITY,           // Working set exceeds cache size
    CNS_8T_MISS_CONFLICT,           // Cache line conflict
    CNS_8T_MISS_COHERENCE           // Cache coherence miss (SMP)
} cns_8t_cache_miss_type_t;

// L1 cache line state
typedef struct {
    uint64_t address;               // Cache line address
    uint32_t set_index;             // Cache set index
    uint32_t tag;                   // Cache tag
    uint8_t  way_index;             // Way within set
    uint8_t  state;                 // Cache line state (valid, dirty, etc.)
    uint16_t access_count;          // Number of accesses to this line
    cns_tick_t last_access_tick;    // Last access time
} cns_8t_cache_line_state_t;

// L1 cache set state (tracks all ways in a set)
typedef struct {
    cns_8t_cache_line_state_t ways[CNS_8T_L1_ASSOCIATIVITY]; // All ways in set
    uint32_t lru_counter[CNS_8T_L1_ASSOCIATIVITY];           // LRU replacement data
    uint64_t access_bitmap;         // Recent access pattern bitmap
    uint32_t conflict_count;        // Number of conflicts in this set
} cns_8t_cache_set_state_t;

// Complete L1 cache simulation state
typedef struct {
    cns_8t_cache_set_state_t sets[CNS_8T_L1_NUM_SETS]; // All cache sets
    
    // Global cache statistics
    uint64_t total_accesses;        // Total memory accesses
    uint64_t total_hits;            // Total cache hits
    uint64_t total_misses;          // Total cache misses
    uint64_t compulsory_misses;     // Cold start misses
    uint64_t capacity_misses;       // Capacity-related misses
    uint64_t conflict_misses;       // Conflict-related misses
    
    // Access pattern analysis
    cns_8t_access_pattern_t dominant_pattern; // Dominant access pattern
    uint64_t sequential_accesses;   // Sequential access count
    uint64_t random_accesses;       // Random access count
    uint64_t stride_accesses;       // Strided access count
    
    // Temporal locality metrics
    double temporal_locality_score; // 0.0 = no reuse, 1.0 = perfect reuse
    uint64_t reuse_distance_sum;    // Sum of reuse distances
    uint32_t reuse_distance_count;  // Number of reuse distance measurements
    
    // Spatial locality metrics
    double spatial_locality_score;  // 0.0 = no spatial locality, 1.0 = perfect
    uint64_t cache_line_utilization_sum; // Sum of cache line utilizations
    uint32_t cache_line_loads;      // Number of cache line loads
    
    // Performance impact
    cns_tick_t miss_penalty_cycles; // Average miss penalty in cycles
    double performance_degradation; // Performance loss due to cache misses
} cns_8t_l1_cache_simulator_t __attribute__((aligned(64)));

// Memory access event for analysis
typedef struct {
    uint64_t address;               // Memory address accessed
    uint32_t size;                  // Access size in bytes
    bool     is_write;              // Whether this is a write access
    cns_tick_t timestamp;           // When the access occurred
    uint32_t instruction_pointer;   // Instruction pointer (if available)
    uint32_t thread_id;             // Thread ID (for SMP analysis)
} cns_8t_memory_access_event_t;

// L1 cache performance profile
typedef struct {
    // Hit rate metrics
    double overall_hit_rate;        // Overall L1 hit rate
    double read_hit_rate;           // Read hit rate
    double write_hit_rate;          // Write hit rate
    
    // Miss rate breakdown
    double compulsory_miss_rate;    // Compulsory miss rate
    double capacity_miss_rate;      // Capacity miss rate
    double conflict_miss_rate;      // Conflict miss rate
    
    // Working set analysis
    size_t working_set_size;        // Estimated working set size
    double working_set_fit_ratio;   // How well working set fits in L1
    uint32_t hot_cache_lines;       // Number of frequently accessed lines
    
    // Access pattern characteristics
    cns_8t_access_pattern_t primary_pattern;   // Primary access pattern
    cns_8t_access_pattern_t secondary_pattern; // Secondary access pattern
    double pattern_consistency;     // How consistent the access pattern is
    
    // Locality metrics
    double temporal_locality;       // Temporal locality score
    double spatial_locality;        // Spatial locality score
    double cache_line_utilization;  // Average cache line utilization
    
    // Performance impact
    double cycles_per_access;       // Average cycles per memory access
    double bandwidth_utilization;   // Memory bandwidth utilization
    double performance_potential;   // Potential performance with perfect cache
    
    // Optimization opportunities
    uint32_t prefetch_opportunities; // Number of prefetch opportunities
    uint32_t alignment_issues;       // Memory alignment issues
    uint32_t false_sharing_events;   // False sharing occurrences
    uint32_t cache_line_splits;      // Accesses that cross cache lines
} cns_8t_l1_performance_profile_t;

// L1 optimization recommendations
typedef struct {
    // Data structure recommendations
    bool recommend_reordering;       // Recommend data structure reordering
    bool recommend_padding;          // Recommend adding padding
    bool recommend_alignment;        // Recommend better alignment
    size_t recommended_alignment;    // Recommended alignment value
    
    // Access pattern recommendations
    bool recommend_prefetch;         // Recommend software prefetch
    uint32_t prefetch_distance;      // Recommended prefetch distance
    bool recommend_blocking;         // Recommend loop blocking/tiling
    uint32_t recommended_block_size; // Recommended block size
    
    // Algorithm recommendations
    bool recommend_cache_oblivious;  // Use cache-oblivious algorithms
    bool recommend_streaming;        // Use streaming optimizations
    bool recommend_temporal_blocking; // Use temporal blocking
    
    // Hardware recommendations
    bool recommend_huge_pages;       // Use huge pages
    bool recommend_numa_binding;     // Bind to specific NUMA nodes
    bool recommend_thread_affinity;  // Set CPU affinity
    
    // Implementation recommendations
    const char* optimization_summary; // Human-readable summary
    double estimated_speedup;         // Estimated performance improvement
    uint32_t implementation_difficulty; // 1=easy, 5=very difficult
} cns_8t_l1_optimization_recommendations_t;

// Real-time L1 analyzer
typedef struct {
    // Cache simulation
    cns_8t_l1_cache_simulator_t* simulator;
    
    // Event tracking
    cns_8t_memory_access_event_t* event_buffer;
    uint32_t event_buffer_size;
    uint32_t event_buffer_head;
    uint32_t event_buffer_tail;
    volatile uint32_t event_count;
    
    // Analysis configuration
    bool enable_real_time_analysis;  // Perform analysis in real-time
    bool enable_pattern_detection;   // Detect access patterns
    bool enable_optimization_hints;  // Generate optimization hints
    uint32_t analysis_window_size;   // Size of analysis window
    
    // Hardware performance counter integration
    bool use_hardware_counters;     // Use CPU performance counters
    uint64_t* hw_counter_values;    // Hardware counter values
    const char** hw_counter_names;  // Hardware counter names
    uint32_t hw_counter_count;      // Number of hardware counters
    
    // Analysis results
    cns_8t_l1_performance_profile_t current_profile;
    cns_8t_l1_optimization_recommendations_t recommendations;
    
    // Historical tracking
    cns_8t_l1_performance_profile_t* historical_profiles;
    uint32_t historical_count;
    uint32_t historical_capacity;
    
    // State
    bool analyzer_active;           // Whether analyzer is running
    cns_tick_t analysis_start_tick; // When analysis started
    uint32_t analysis_iterations;   // Number of analysis iterations
} cns_8t_l1_analyzer_t __attribute__((aligned(64)));

// ============================================================================
// CACHE SIMULATION FUNCTIONS
// ============================================================================

// L1 cache simulator operations
cns_8t_result_t cns_8t_l1_simulator_create(cns_8t_l1_cache_simulator_t** simulator);
cns_8t_result_t cns_8t_l1_simulator_destroy(cns_8t_l1_cache_simulator_t* simulator);
cns_8t_result_t cns_8t_l1_simulator_reset(cns_8t_l1_cache_simulator_t* simulator);

// Memory access simulation
cns_8t_result_t cns_8t_l1_simulator_access(cns_8t_l1_cache_simulator_t* simulator,
                                            uint64_t address,
                                            uint32_t size,
                                            bool is_write,
                                            bool* cache_hit);

// Batch access simulation for performance
cns_8t_result_t cns_8t_l1_simulator_access_batch(cns_8t_l1_cache_simulator_t* simulator,
                                                  const cns_8t_memory_access_event_t* events,
                                                  uint32_t event_count,
                                                  bool* hit_results);

// Cache state inspection
cns_8t_result_t cns_8t_l1_simulator_get_cache_line(cns_8t_l1_cache_simulator_t* simulator,
                                                    uint64_t address,
                                                    cns_8t_cache_line_state_t* line_state);

cns_8t_result_t cns_8t_l1_simulator_get_set_state(cns_8t_l1_cache_simulator_t* simulator,
                                                   uint32_t set_index,
                                                   cns_8t_cache_set_state_t* set_state);

// ============================================================================
// ACCESS PATTERN ANALYSIS
// ============================================================================

// Detect access patterns in memory accesses
cns_8t_result_t cns_8t_analyze_access_pattern(const cns_8t_memory_access_event_t* events,
                                               uint32_t event_count,
                                               cns_8t_access_pattern_t* pattern,
                                               double* pattern_confidence);

// Analyze temporal locality
cns_8t_result_t cns_8t_analyze_temporal_locality(const cns_8t_memory_access_event_t* events,
                                                  uint32_t event_count,
                                                  double* temporal_locality_score,
                                                  uint64_t* average_reuse_distance);

// Analyze spatial locality
cns_8t_result_t cns_8t_analyze_spatial_locality(const cns_8t_memory_access_event_t* events,
                                                 uint32_t event_count,
                                                 double* spatial_locality_score,
                                                 double* cache_line_utilization);

// Working set size estimation
cns_8t_result_t cns_8t_estimate_working_set_size(const cns_8t_memory_access_event_t* events,
                                                  uint32_t event_count,
                                                  size_t* working_set_size,
                                                  double* confidence);

// ============================================================================
// PERFORMANCE PROFILING
// ============================================================================

// Generate comprehensive L1 performance profile
cns_8t_result_t cns_8t_l1_generate_performance_profile(
    const cns_8t_l1_cache_simulator_t* simulator,
    const cns_8t_memory_access_event_t* events,
    uint32_t event_count,
    cns_8t_l1_performance_profile_t* profile
);

// Analyze performance impact of cache misses
cns_8t_result_t cns_8t_l1_analyze_performance_impact(
    const cns_8t_l1_performance_profile_t* profile,
    cns_tick_t base_execution_time,
    double* performance_degradation,
    cns_tick_t* cycles_lost_to_misses
);

// Compare two performance profiles
cns_8t_result_t cns_8t_l1_compare_profiles(
    const cns_8t_l1_performance_profile_t* profile1,
    const cns_8t_l1_performance_profile_t* profile2,
    double* improvement_factor,
    const char** comparison_summary
);

// ============================================================================
// OPTIMIZATION ANALYSIS
// ============================================================================

// Generate optimization recommendations
cns_8t_result_t cns_8t_l1_generate_optimization_recommendations(
    const cns_8t_l1_performance_profile_t* profile,
    const cns_8t_memory_access_event_t* events,
    uint32_t event_count,
    cns_8t_l1_optimization_recommendations_t* recommendations
);

// Analyze data structure layout efficiency
cns_8t_result_t cns_8t_l1_analyze_data_layout(
    const void* data_structure,
    size_t structure_size,
    const cns_8t_memory_access_event_t* events,
    uint32_t event_count,
    double* layout_efficiency,
    uint32_t* suggested_reordering
);

// Analyze false sharing potential
cns_8t_result_t cns_8t_l1_analyze_false_sharing(
    const cns_8t_memory_access_event_t* events,
    uint32_t event_count,
    uint32_t thread_count,
    uint32_t* false_sharing_events,
    uint64_t* conflicting_addresses
);

// Prefetch opportunity analysis
cns_8t_result_t cns_8t_l1_analyze_prefetch_opportunities(
    const cns_8t_memory_access_event_t* events,
    uint32_t event_count,
    uint32_t* prefetch_opportunities,
    uint32_t* optimal_prefetch_distance
);

// ============================================================================
// REAL-TIME ANALYZER API
// ============================================================================

// Analyzer lifecycle
cns_8t_result_t cns_8t_l1_analyzer_create(uint32_t event_buffer_size,
                                           cns_8t_l1_analyzer_t** analyzer);

cns_8t_result_t cns_8t_l1_analyzer_destroy(cns_8t_l1_analyzer_t* analyzer);

// Analysis control
cns_8t_result_t cns_8t_l1_analyzer_start(cns_8t_l1_analyzer_t* analyzer);
cns_8t_result_t cns_8t_l1_analyzer_stop(cns_8t_l1_analyzer_t* analyzer);
cns_8t_result_t cns_8t_l1_analyzer_reset(cns_8t_l1_analyzer_t* analyzer);

// Event recording
cns_8t_result_t cns_8t_l1_analyzer_record_access(cns_8t_l1_analyzer_t* analyzer,
                                                  uint64_t address,
                                                  uint32_t size,
                                                  bool is_write);

cns_8t_result_t cns_8t_l1_analyzer_record_batch(cns_8t_l1_analyzer_t* analyzer,
                                                 const cns_8t_memory_access_event_t* events,
                                                 uint32_t event_count);

// Analysis retrieval
cns_8t_result_t cns_8t_l1_analyzer_get_current_profile(cns_8t_l1_analyzer_t* analyzer,
                                                        cns_8t_l1_performance_profile_t* profile);

cns_8t_result_t cns_8t_l1_analyzer_get_recommendations(cns_8t_l1_analyzer_t* analyzer,
                                                        cns_8t_l1_optimization_recommendations_t* recommendations);

// Configuration
cns_8t_result_t cns_8t_l1_analyzer_set_real_time_mode(cns_8t_l1_analyzer_t* analyzer,
                                                       bool enable);

cns_8t_result_t cns_8t_l1_analyzer_set_analysis_window(cns_8t_l1_analyzer_t* analyzer,
                                                        uint32_t window_size);

// ============================================================================
// INTEGRATION WITH OTHER 8T COMPONENTS
// ============================================================================

// Analyze L1 cache behavior of arena allocator
cns_8t_result_t cns_8t_analyze_arena_l1_behavior(cns_8t_arena_l1_t* arena,
                                                  cns_8t_l1_analyzer_t* analyzer,
                                                  uint32_t analysis_duration_ms);

// Analyze L1 cache behavior of graph operations
cns_8t_result_t cns_8t_analyze_graph_l1_behavior(cns_8t_graph_l1_t* graph,
                                                  cns_8t_l1_analyzer_t* analyzer,
                                                  const char* operation_name);

// Generate cache-optimized memory layout
cns_8t_result_t cns_8t_optimize_layout_for_l1(void* data,
                                               size_t data_size,
                                               const cns_8t_l1_performance_profile_t* profile,
                                               void** optimized_data);

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

// Convert address to cache set and tag
static inline void cns_8t_address_to_cache_coords(uint64_t address,
                                                   uint32_t* set_index,
                                                   uint32_t* tag) {
    // Assuming 64-byte cache lines and 64 sets
    *set_index = (address >> 6) & (CNS_8T_L1_NUM_SETS - 1);
    *tag = address >> (6 + __builtin_ctz(CNS_8T_L1_NUM_SETS));
}

// Check if two addresses are in the same cache line
static inline bool cns_8t_same_cache_line(uint64_t addr1, uint64_t addr2) {
    return (addr1 >> 6) == (addr2 >> 6);
}

// Calculate cache line utilization for an access pattern
static inline double cns_8t_calculate_cache_line_utilization(
    const uint64_t* addresses,
    uint32_t count
) {
    if (count == 0) return 0.0;
    
    uint32_t unique_lines = 0;
    uint64_t last_line = UINT64_MAX;
    
    for (uint32_t i = 0; i < count; i++) {
        uint64_t line = addresses[i] >> 6;
        if (line != last_line) {
            unique_lines++;
            last_line = line;
        }
    }
    
    return (double)count / unique_lines;
}

#ifdef __cplusplus
}
#endif

#endif // CNS_8T_L1_ANALYZER_H