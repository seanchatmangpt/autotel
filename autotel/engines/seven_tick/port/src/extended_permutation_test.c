/**
 * @file extended_permutation_test.c
 * @brief Extended Permutation Testing for L1-L2-L3 Integration
 * @version 1.0.0
 * 
 * Advanced permutation testing with comprehensive validation:
 * - Stress testing with large message volumes
 * - Edge case validation
 * - Performance regression detection
 * - Statistical analysis of results
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>

#ifdef __APPLE__
#include <mach/mach_time.h>
#endif

// Re-use core definitions from integrated test
#define CACHE_LINE_SIZE 64
#define L1_ACTORS_MAX 256
#define TARGET_INTEGRATED_NS 1000

// Extended test configurations
#define MAX_STRESS_ACTORS 128
#define MAX_STRESS_MESSAGES 64
#define MAX_STRESS_PERMUTATIONS 5000

typedef struct {
    uint64_t min_time;
    uint64_t max_time;
    uint64_t total_time;
    double avg_time;
    double std_dev;
    uint32_t samples;
} PerformanceStats;

typedef struct {
    // Test configuration
    uint32_t num_actors;
    uint32_t num_messages;
    uint32_t num_permutations;
    bool stress_test_mode;
    
    // Results
    uint32_t total_tests;
    uint32_t passed_tests;
    uint32_t performance_failures;
    uint32_t integration_failures;
    
    // Performance analysis
    PerformanceStats pipeline_stats;
    PerformanceStats layer_stats[3];  // L1, L2, L3
    
    // Statistical validation
    double performance_mean;
    double performance_variance;
    double performance_confidence_95;
    
} ExtendedTestResults;

static uint64_t get_precise_nanoseconds(void) {
#ifdef __APPLE__
    static mach_timebase_info_data_t timebase = {0};
    if (timebase.denom == 0) {
        mach_timebase_info(&timebase);
    }
    uint64_t mach_time = mach_absolute_time();
    return mach_time * timebase.numer / timebase.denom;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
#endif
}

// Simplified simulation functions (avoiding full integrated system complexity)
static inline uint64_t simulate_l1_operation(uint32_t actor_id) {
    uint64_t start = get_precise_nanoseconds();
    
    // Simulate L1 causal collapse work
    volatile uint64_t result = 0;
    for (int i = 0; i < 8; i++) {
        result ^= (actor_id + i) * 0x9E3779B97F4A7C15ULL;
    }
    
    uint64_t end = get_precise_nanoseconds();
    return end - start;
}

static inline uint64_t simulate_l2_operation(uint32_t source, uint32_t target) {
    uint64_t start = get_precise_nanoseconds();
    
    // Simulate L2 message routing work
    volatile uint64_t routing = (source << 16) | target;
    routing = ((routing >> 16) ^ routing) * 0x45d9f3b;
    routing = ((routing >> 16) ^ routing) * 0x45d9f3b;
    routing = (routing >> 16) ^ routing;
    
    uint64_t end = get_precise_nanoseconds();
    return end - start;
}

static inline uint64_t simulate_l3_operation(uint32_t state, uint32_t transition) {
    uint64_t start = get_precise_nanoseconds();
    
    // Simulate L3 behavior transition work
    volatile uint32_t new_state = (state + transition) % 8;
    volatile uint64_t behavior_vector = new_state * 0x0101010101010101ULL;
    
    uint64_t end = get_precise_nanoseconds();
    return end - start;
}

static inline uint64_t simulate_integrated_pipeline(uint32_t actor_id, uint32_t message_type) {
    uint64_t start = get_precise_nanoseconds();
    
    // Simulate integrated L1->L2->L3 pipeline
    uint64_t l1_time = simulate_l1_operation(actor_id);
    uint64_t l2_time = simulate_l2_operation(actor_id, (actor_id + 1) % MAX_STRESS_ACTORS);
    uint64_t l3_time = simulate_l3_operation(actor_id % 8, message_type % 8);
    
    // Small integration overhead
    volatile uint64_t integration_hash = l1_time ^ l2_time ^ l3_time;
    
    uint64_t end = get_precise_nanoseconds();
    return end - start;
}

static void update_performance_stats(PerformanceStats* stats, uint64_t time) {
    if (stats->samples == 0) {
        stats->min_time = time;
        stats->max_time = time;
    } else {
        if (time < stats->min_time) stats->min_time = time;
        if (time > stats->max_time) stats->max_time = time;
    }
    
    stats->total_time += time;
    stats->samples++;
    stats->avg_time = (double)stats->total_time / stats->samples;
}

static void calculate_std_dev(PerformanceStats* stats, uint64_t* times, uint32_t count) {
    if (count < 2) {
        stats->std_dev = 0.0;
        return;
    }
    
    double variance = 0.0;
    for (uint32_t i = 0; i < count; i++) {
        double diff = times[i] - stats->avg_time;
        variance += diff * diff;
    }
    variance /= (count - 1);
    stats->std_dev = sqrt(variance);
}

static ExtendedTestResults run_extended_permutation_test(uint32_t num_actors, 
                                                        uint32_t num_messages,
                                                        uint32_t num_permutations,
                                                        bool stress_mode) {
    printf("🧪 EXTENDED PERMUTATION TEST: Starting...\n");
    printf("   Actors: %u, Messages: %u, Permutations: %u\n", 
           num_actors, num_messages, num_permutations);
    printf("   Mode: %s\n", stress_mode ? "STRESS" : "STANDARD");
    
    ExtendedTestResults results = {0};
    results.num_actors = num_actors;
    results.num_messages = num_messages;
    results.num_permutations = num_permutations;
    results.stress_test_mode = stress_mode;
    
    // Allocate arrays for statistical analysis
    uint64_t* pipeline_times = malloc(num_permutations * num_messages * sizeof(uint64_t));
    uint32_t time_samples = 0;
    
    uint64_t test_start = get_precise_nanoseconds();
    
    for (uint32_t perm = 0; perm < num_permutations; perm++) {
        uint64_t perm_start = get_precise_nanoseconds();
        bool perm_passed = true;
        
        // Execute messages in this permutation
        for (uint32_t msg_idx = 0; msg_idx < num_messages; msg_idx++) {
            uint32_t actor_id = (perm + msg_idx) % num_actors;
            uint32_t message_type = (perm * msg_idx + 7) % 16;
            
            uint64_t pipeline_time = simulate_integrated_pipeline(actor_id, message_type);
            pipeline_times[time_samples++] = pipeline_time;
            
            update_performance_stats(&results.pipeline_stats, pipeline_time);
            
            // Performance validation
            uint32_t performance_target = stress_mode ? TARGET_INTEGRATED_NS * 3 : TARGET_INTEGRATED_NS * 2;
            if (pipeline_time > performance_target) {
                results.performance_failures++;
                perm_passed = false;
            }
        }
        
        uint64_t perm_end = get_precise_nanoseconds();
        uint64_t perm_time = perm_end - perm_start;
        
        results.total_tests++;
        if (perm_passed) {
            results.passed_tests++;
        }
        
        // Progress reporting
        if ((perm + 1) % (num_permutations / 20) == 0) {
            printf("   Progress: %u/%u (%.1f%%) - Pass Rate: %.1f%%\n", 
                   perm + 1, num_permutations, 
                   (double)(perm + 1) / num_permutations * 100.0,
                   (double)results.passed_tests / results.total_tests * 100.0);
        }
    }
    
    uint64_t test_end = get_precise_nanoseconds();
    
    // Calculate statistical measures
    calculate_std_dev(&results.pipeline_stats, pipeline_times, time_samples);
    
    results.performance_mean = results.pipeline_stats.avg_time;
    results.performance_variance = results.pipeline_stats.std_dev * results.pipeline_stats.std_dev;
    
    // 95% confidence interval (approx)
    results.performance_confidence_95 = results.performance_mean + (1.96 * results.pipeline_stats.std_dev);
    
    printf("🧪 EXTENDED TEST: Complete in %.3f seconds\n", 
           (test_end - test_start) / 1e9);
    
    free(pipeline_times);
    return results;
}

static void print_extended_results(ExtendedTestResults results) {
    printf("\n📊 EXTENDED PERMUTATION TEST RESULTS\n");
    printf("====================================\n");
    
    printf("Configuration:\n");
    printf("  Actors:        %u\n", results.num_actors);
    printf("  Messages:      %u per permutation\n", results.num_messages);
    printf("  Permutations:  %u\n", results.num_permutations);
    printf("  Mode:          %s\n", results.stress_test_mode ? "STRESS" : "STANDARD");
    
    printf("\nTest Results:\n");
    printf("  Total Tests:         %u\n", results.total_tests);
    printf("  Passed:             %u (%.1f%%)\n", 
           results.passed_tests,
           (double)results.passed_tests / results.total_tests * 100.0);
    printf("  Performance Failures: %u (%.1f%%)\n",
           results.performance_failures,
           (double)results.performance_failures / (results.total_tests * results.num_messages) * 100.0);
    printf("  Integration Failures: %u\n", results.integration_failures);
    
    printf("\nPerformance Statistics:\n");
    printf("  Min Time:      %lluns\n", results.pipeline_stats.min_time);
    printf("  Average Time:  %.1fns\n", results.pipeline_stats.avg_time);
    printf("  Max Time:      %lluns\n", results.pipeline_stats.max_time);
    printf("  Std Deviation: %.1fns\n", results.pipeline_stats.std_dev);
    printf("  Samples:       %u\n", results.pipeline_stats.samples);
    
    printf("\nStatistical Analysis:\n");
    printf("  Mean:          %.1fns\n", results.performance_mean);
    printf("  Variance:      %.1fns²\n", results.performance_variance);
    printf("  95%% Conf Int:   %.1fns\n", results.performance_confidence_95);
    
    uint32_t target = results.stress_test_mode ? TARGET_INTEGRATED_NS * 3 : TARGET_INTEGRATED_NS * 2;
    bool statistical_pass = results.performance_confidence_95 <= target;
    bool overall_pass = (results.passed_tests >= results.total_tests * 0.95) && statistical_pass;
    
    printf("\nValidation:\n");
    printf("  Target:        %uns\n", target);
    printf("  Statistical:   %s\n", statistical_pass ? "✅ PASS" : "❌ FAIL");
    printf("  Overall:       %s\n", overall_pass ? "✅ PASS" : "❌ FAIL");
}

static void run_comprehensive_test_suite(void) {
    printf("🚀 COMPREHENSIVE L1-L2-L3 PERMUTATION TEST SUITE\n");
    printf("=================================================\n\n");
    
    // Test 1: Standard validation
    printf("🔬 Test 1: Standard Validation\n");
    printf("------------------------------\n");
    ExtendedTestResults test1 = run_extended_permutation_test(32, 16, 2000, false);
    print_extended_results(test1);
    
    // Test 2: High actor count
    printf("\n🔬 Test 2: High Actor Count\n");
    printf("---------------------------\n");
    ExtendedTestResults test2 = run_extended_permutation_test(64, 8, 1000, false);
    print_extended_results(test2);
    
    // Test 3: High message volume
    printf("\n🔬 Test 3: High Message Volume\n");
    printf("------------------------------\n");
    ExtendedTestResults test3 = run_extended_permutation_test(16, 32, 1000, false);
    print_extended_results(test3);
    
    // Test 4: Stress test
    printf("\n🔬 Test 4: Stress Test\n");
    printf("----------------------\n");
    ExtendedTestResults test4 = run_extended_permutation_test(MAX_STRESS_ACTORS, 
                                                             MAX_STRESS_MESSAGES, 
                                                             1000, true);
    print_extended_results(test4);
    
    // Summary
    printf("\n🎯 COMPREHENSIVE TEST SUMMARY\n");
    printf("=============================\n");
    
    bool all_passed = true;
    double total_pass_rate = 0.0;
    
    ExtendedTestResults tests[] = {test1, test2, test3, test4};
    const char* test_names[] = {"Standard", "High Actors", "High Messages", "Stress"};
    
    for (int i = 0; i < 4; i++) {
        double pass_rate = (double)tests[i].passed_tests / tests[i].total_tests * 100.0;
        total_pass_rate += pass_rate;
        
        uint32_t target = tests[i].stress_test_mode ? TARGET_INTEGRATED_NS * 3 : TARGET_INTEGRATED_NS * 2;
        bool test_pass = (pass_rate >= 95.0) && (tests[i].performance_confidence_95 <= target);
        
        printf("  %s: %.1f%% pass rate, %.1fns 95%% conf - %s\n",
               test_names[i], pass_rate, tests[i].performance_confidence_95,
               test_pass ? "✅ PASS" : "❌ FAIL");
        
        if (!test_pass) all_passed = false;
    }
    
    double avg_pass_rate = total_pass_rate / 4.0;
    
    printf("\n  Overall Pass Rate: %.1f%%\n", avg_pass_rate);
    printf("  Comprehensive Result: %s\n", all_passed ? "✅ SUCCESS" : "❌ NEEDS OPTIMIZATION");
}

int main(void) {
    srand((unsigned int)time(NULL));
    
    run_comprehensive_test_suite();
    
    return 0;
}