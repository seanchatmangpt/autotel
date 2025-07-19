#include "../lib/s7t_perf.h"
#include "../lib/7t_pm.h"
#include "../lib/7t_mcts.h"
#include "../lib/7t_sparql.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// ============================================================================
// TEST OPERATIONS FOR VALIDATION
// ============================================================================

// Fast operation - should pass 7-cycle limit
static void fast_hash_operation(void* ctx) {
    uint32_t* value = (uint32_t*)ctx;
    // Simple hash that completes in <7 cycles
    *value = (*value * 0x9e3779b9) ^ (*value >> 16);
}

// Slow operation - will fail 7-cycle limit
static void slow_operation(void* ctx) {
    uint32_t* value = (uint32_t*)ctx;
    // Deliberately slow operation
    for (int i = 0; i < 10; i++) {
        *value = (*value * 0x9e3779b9) ^ (*value >> 16);
    }
}

// Cache-friendly sequential access
static void cache_friendly_access(void* ctx) {
    uint32_t* array = (uint32_t*)ctx;
    uint32_t sum = 0;
    
    // Sequential access pattern
    for (int i = 0; i < 16; i++) {
        S7T_VALIDATE_MEMORY_READ(&array[i], sizeof(uint32_t));
        sum += array[i];
    }
}

// Cache-unfriendly random access
static void cache_unfriendly_access(void* ctx) {
    uint32_t* array = (uint32_t*)ctx;
    uint32_t sum = 0;
    
    // Random access pattern (cache-unfriendly)
    int indices[] = {15, 0, 7, 3, 11, 1, 14, 5, 9, 2, 13, 4, 10, 6, 12, 8};
    for (int i = 0; i < 16; i++) {
        S7T_VALIDATE_MEMORY_READ(&array[indices[i]], sizeof(uint32_t));
        sum += array[indices[i]];
    }
}

// Branch-free operation
static void branch_free_operation(void* ctx) {
    uint32_t* values = (uint32_t*)ctx;
    
    // Branch-free conditional using bit manipulation
    uint32_t a = values[0];
    uint32_t b = values[1];
    uint32_t mask = -(a > b);  // All 1s if true, all 0s if false
    values[2] = (a & mask) | (b & ~mask);  // Branch-free max
}

// Branchy operation
static void branchy_operation(void* ctx) {
    uint32_t* values = (uint32_t*)ctx;
    
    // Multiple unpredictable branches
    for (int i = 0; i < 10; i++) {
        S7T_VALIDATE_BRANCH("loop_branch", i < 10, true);
        
        if (values[i] & 1) {
            S7T_VALIDATE_BRANCH("odd_branch", true, false);
            values[i] *= 3;
        } else {
            S7T_VALIDATE_BRANCH("even_branch", true, false);
            values[i] /= 2;
        }
    }
}

// No allocation operation
static void no_alloc_operation(void* ctx) {
    uint32_t* buffer = (uint32_t*)ctx;
    
    // Process data without any allocation
    for (int i = 0; i < 10; i++) {
        buffer[i] = buffer[i] * 2 + 1;
    }
}

// Allocation operation (will fail no-alloc test)
static void alloc_operation(void* ctx) {
    // This will trigger allocation tracking
    uint32_t* temp = malloc(sizeof(uint32_t) * 10);
    if (temp) {
        memset(temp, 0, sizeof(uint32_t) * 10);
        free(temp);
    }
}

// ============================================================================
// VALIDATION TEST SUITE
// ============================================================================

static void test_cycle_validation() {
    printf("\n=== CYCLE VALIDATION TESTS ===\n");
    
    uint32_t value = 12345;
    
    // Test fast operation (should pass)
    printf("Testing fast hash operation...\n");
    S7T_ASSERT_CYCLES("fast_hash", {
        fast_hash_operation(&value);
    });
    
    // Test slow operation (should fail)
    printf("Testing slow operation...\n");
    S7T_ASSERT_CYCLES("slow_operation", {
        slow_operation(&value);
    });
    
    // Test PM7T operations
    PM7T* pm = pm7t_create();
    printf("Testing PM7T add_event...\n");
    S7T_ASSERT_CYCLES("pm7t_add_event", {
        pm7t_add_event(pm, "case1", "activity1", 1000);
    });
    
    printf("Testing PM7T mine_patterns...\n");
    S7T_ASSERT_CYCLES("pm7t_mine_patterns", {
        pm7t_mine_patterns(pm);
    });
    
    pm7t_destroy(pm);
    
    // Test MCTS7T operations
    MCTS7T* mcts = mcts7t_create();
    printf("Testing MCTS7T select...\n");
    
    uint32_t state = 0;
    S7T_ASSERT_CYCLES("mcts7t_select", {
        mcts7t_select(mcts, state);
    });
    
    mcts7t_destroy(mcts);
}

static void test_memory_patterns() {
    printf("\n=== MEMORY PATTERN VALIDATION ===\n");
    
    // Initialize test array
    uint32_t* test_array = aligned_alloc(64, sizeof(uint32_t) * 1024);
    for (int i = 0; i < 1024; i++) {
        test_array[i] = i;
    }
    
    // Test cache-friendly access
    printf("Testing cache-friendly sequential access...\n");
    S7T_ASSERT_CYCLES("sequential_access", {
        cache_friendly_access(test_array);
    });
    
    // Test cache-unfriendly access
    printf("Testing cache-unfriendly random access...\n");
    S7T_ASSERT_CYCLES("random_access", {
        cache_unfriendly_access(test_array);
    });
    
    // Analyze memory patterns
    MemoryAnalysis analysis = s7t_analyze_memory_patterns();
    printf("Cache Hit Rate: %.2f%%\n", analysis.cache_hit_rate * 100);
    printf("Sequential Accesses: %zu\n", analysis.sequential_accesses);
    printf("Random Accesses: %zu\n", analysis.random_accesses);
    printf("Cache-Friendly: %s\n", analysis.is_cache_friendly ? "YES" : "NO");
    
    free(test_array);
}

static void test_branch_prediction() {
    printf("\n=== BRANCH PREDICTION VALIDATION ===\n");
    
    uint32_t values[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    
    // Test branch-free operation
    printf("Testing branch-free operation...\n");
    S7T_ASSERT_CYCLES("branch_free", {
        branch_free_operation(values);
    });
    
    // Test branchy operation
    printf("Testing branchy operation...\n");
    S7T_ASSERT_CYCLES("branchy", {
        branchy_operation(values);
    });
    
    // Analyze branch patterns
    BranchAnalysis analysis = s7t_analyze_branch_patterns();
    printf("Total Branches: %zu\n", analysis.total_branches);
    printf("Mispredictions: %zu\n", analysis.mispredictions);
    printf("Prediction Accuracy: %.2f%%\n", analysis.prediction_accuracy * 100);
    printf("Branch Predictable: %s\n", analysis.is_predictable ? "YES" : "NO");
}

static void test_alignment_validation() {
    printf("\n=== ALIGNMENT VALIDATION ===\n");
    
    // Test aligned allocation
    void* aligned_ptr = aligned_alloc(64, 1024);
    S7T_VALIDATE_ALIGNMENT(aligned_ptr, 64);
    printf("64-byte aligned allocation: PASS\n");
    
    // Test potentially misaligned
    char buffer[1024];
    char* ptr = buffer + 1;  // Misaligned
    
    if (S7T_IS_ALIGNED(ptr, 8)) {
        printf("8-byte alignment check: PASS\n");
    } else {
        printf("8-byte alignment check: FAIL (expected)\n");
    }
    
    free(aligned_ptr);
}

static void test_no_allocation_zones() {
    printf("\n=== NO-ALLOCATION ZONE VALIDATION ===\n");
    
    uint32_t buffer[100];
    
    // Test operation without allocation
    printf("Testing no-allocation operation...\n");
    S7T_NO_ALLOC_ZONE_START();
    no_alloc_operation(buffer);
    S7T_NO_ALLOC_ZONE_END();
    printf("No-allocation test: PASS\n");
    
    // Note: We can't actually test the failing case without
    // causing the validator to abort in strict mode
}

static void test_cache_simulation() {
    printf("\n=== CACHE SIMULATION ===\n");
    
    // Create 32KB L1 cache with 8-way associativity
    CacheSimulator* cache = s7t_cache_simulator_create(32, 8);
    
    // Simulate sequential access (good locality)
    printf("Simulating sequential access pattern...\n");
    char data[4096];
    for (int i = 0; i < 4096; i += 64) {
        s7t_cache_access(cache, &data[i]);
    }
    
    CacheStats stats = s7t_get_cache_stats(cache);
    printf("Sequential - Hit Rate: %.2f%%, Misses: %zu\n", 
           stats.hit_rate * 100, stats.misses);
    
    // Reset and simulate random access (poor locality)
    s7t_cache_simulator_destroy(cache);
    cache = s7t_cache_simulator_create(32, 8);
    
    printf("Simulating random access pattern...\n");
    for (int i = 0; i < 100; i++) {
        int offset = (rand() % 64) * 64;
        s7t_cache_access(cache, &data[offset]);
    }
    
    stats = s7t_get_cache_stats(cache);
    printf("Random - Hit Rate: %.2f%%, Misses: %zu\n", 
           stats.hit_rate * 100, stats.misses);
    
    s7t_cache_simulator_destroy(cache);
}

static void test_performance_monitoring() {
    printf("\n=== PERFORMANCE MONITORING ===\n");
    
    PerfMonitor* monitor = s7t_perf_monitor_create(1000);
    
    // Record multiple samples
    uint32_t value = 12345;
    for (int i = 0; i < 100; i++) {
        uint64_t start = s7t_rdtscp();
        fast_hash_operation(&value);
        uint64_t end = s7t_rdtscp();
        s7t_perf_monitor_record(monitor, end - start);
    }
    
    PerfStats stats = s7t_perf_monitor_stats(monitor);
    printf("Performance Statistics:\n");
    printf("  Min: %lu cycles\n", stats.min);
    printf("  Max: %lu cycles\n", stats.max);
    printf("  Avg: %.2f cycles\n", stats.avg);
    printf("  StdDev: %.2f cycles\n", stats.std_dev);
    printf("  P50: %.2f cycles\n", stats.p50);
    printf("  P95: %.2f cycles\n", stats.p95);
    printf("  P99: %.2f cycles\n", stats.p99);
    
    s7t_perf_monitor_destroy(monitor);
}

static void test_comprehensive_validation() {
    printf("\n=== COMPREHENSIVE VALIDATION SUITE ===\n");
    
    S7TValidationSuite* suite = s7t_create_validation_suite();
    
    // Test a simple operation
    uint32_t test_value = 42;
    S7TValidationReport report = s7t_run_validation(suite, 
                                                   fast_hash_operation, 
                                                   &test_value);
    
    printf("Validation Report:\n");
    printf("  Total Tests: %zu\n", report.total_tests);
    printf("  Passed: %zu\n", report.passed);
    printf("  Failed: %zu\n", report.failed);
    printf("  Avg Cycles: %.2f\n", report.avg_cycles);
    printf("  Cache Hit Rate: %.2f%%\n", report.cache_hit_rate * 100);
    printf("  Branch Prediction: %.2f%%\n", report.branch_prediction_rate * 100);
    printf("  Physics Compliant: %s\n", report.physics_compliant ? "YES" : "NO");
    
    s7t_destroy_validation_suite(suite);
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

int main(int argc, char* argv[]) {
    printf("S7T Performance Validation Framework\n");
    printf("====================================\n");
    
    // Initialize validation system
    s7t_validator_init(false);  // Non-strict mode for testing
    s7t_memory_tracker_init();
    s7t_branch_tracker_init();
    
    // Run all validation tests
    test_cycle_validation();
    test_memory_patterns();
    test_branch_prediction();
    test_alignment_validation();
    test_no_allocation_zones();
    test_cache_simulation();
    test_performance_monitoring();
    test_comprehensive_validation();
    
    // Generate reports
    printf("\n=== GENERATING REPORTS ===\n");
    
    s7t_validation_report();
    s7t_generate_validation_report("validation_report.md");
    s7t_generate_json_report("validation_report.json");
    s7t_generate_heatmap("validation_heatmap.md");
    
    printf("\nReports generated:\n");
    printf("  - validation_report.md\n");
    printf("  - validation_report.json\n");
    printf("  - validation_heatmap.md\n");
    
    // Cleanup
    s7t_validator_cleanup();
    s7t_memory_tracker_cleanup();
    s7t_branch_tracker_cleanup();
    
    return 0;
}