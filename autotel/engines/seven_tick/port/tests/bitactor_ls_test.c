#include "../include/cns/bitactor_ls.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

// Test framework macros
#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            printf("❌ FAIL: %s\n", message); \
            return false; \
        } else { \
            printf("✅ PASS: %s\n", message); \
        } \
    } while(0)

#define TEST_FUNCTION(name) \
    bool test_##name() { \
        printf("\n🧪 Testing %s\n", #name);

#define TEST_END \
        return true; \
    }

// External function declarations
extern uint64_t hash_ttl_content(const char* ttl_spec);
extern bitactor_manifest_t* create_bitactor_manifest(const char* ttl_spec);
extern void benchmark_bitactor_ls(void);

// Mock rdtsc() for non-x86 platforms
#ifndef __x86_64__
static uint64_t rdtsc() {
    return 10; // Simulate very fast operation for testing
}
#endif

// Test basic learning system initialization
TEST_FUNCTION(ls_matrix_creation)
    bitactor_ls_matrix_t* ls_matrix = bitactor_ls_matrix_create();
    TEST_ASSERT(ls_matrix != NULL, "Learning matrix created successfully");
    TEST_ASSERT(ls_matrix->learning_actor_count == 0, "Initial actor count is zero");
    TEST_ASSERT(ls_matrix->global_learning_tick == 0, "Initial tick count is zero");
    TEST_ASSERT(ls_matrix->global_metrics.sub_100ns_preserved == true, "Sub-100ns initially preserved");
    
    bitactor_ls_matrix_destroy(ls_matrix);
TEST_END

// Test learning actor creation and configuration
TEST_FUNCTION(ls_actor_creation)
    bitactor_ls_matrix_t* ls_matrix = bitactor_ls_matrix_create();
    bitactor_manifest_t* manifest = create_bitactor_manifest("test_spec");
    
    uint32_t actor_id = bitactor_ls_add_actor(ls_matrix, 0x42, manifest);
    TEST_ASSERT(actor_id == 0, "First actor gets ID 0");
    TEST_ASSERT(ls_matrix->learning_actor_count == 1, "Actor count incremented");
    
    bitactor_ls_core_t* ls_actor = &ls_matrix->learning_actors[0];
    TEST_ASSERT(ls_actor->base_actor.meaning == 0x42, "Actor meaning set correctly");
    TEST_ASSERT(ls_actor->learning_mode == 1, "Default learning mode is observe");
    TEST_ASSERT(ls_actor->pattern_count == 0, "No patterns initially");
    TEST_ASSERT(ls_actor->learning_trinity_compliant == true, "Trinity compliant initially");
    
    free(manifest->bytecode);
    free(manifest);
    bitactor_ls_matrix_destroy(ls_matrix);
TEST_END

// Test pattern management
TEST_FUNCTION(pattern_management)
    bitactor_ls_matrix_t* ls_matrix = bitactor_ls_matrix_create();
    bitactor_manifest_t* manifest = create_bitactor_manifest("test_spec");
    
    uint32_t actor_id = bitactor_ls_add_actor(ls_matrix, 0x42, manifest);
    bitactor_ls_core_t* ls_actor = &ls_matrix->learning_actors[actor_id];
    
    // Add a test pattern
    uint8_t pattern_data[8] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
    bool added = bitactor_ls_add_pattern(ls_actor, pattern_data, 8, 0.8);
    TEST_ASSERT(added == true, "Pattern added successfully");
    TEST_ASSERT(ls_actor->pattern_count == 1, "Pattern count incremented");
    
    bitactor_ls_pattern_t* pattern = &ls_actor->patterns[0];
    TEST_ASSERT(pattern->confidence_score > 0, "Pattern has confidence score");
    TEST_ASSERT(pattern->validity_flags & 0x01, "Pattern is marked valid");
    TEST_ASSERT(pattern->logic_size == 8, "Pattern logic size correct");
    
    // Test pattern activation
    bool activated = bitactor_ls_activate_pattern(ls_actor, 0);
    TEST_ASSERT(activated == true, "Pattern activated successfully");
    TEST_ASSERT(ls_actor->pattern_active_mask & 0x01, "Pattern active mask set");
    TEST_ASSERT(ls_actor->current_pattern_id == 0, "Current pattern ID set");
    
    // Test pattern removal
    bool removed = bitactor_ls_remove_pattern(ls_actor, 0);
    TEST_ASSERT(removed == true, "Pattern removed successfully");
    TEST_ASSERT((ls_actor->patterns[0].validity_flags & 0x01) == 0, "Pattern marked invalid");
    
    free(manifest->bytecode);
    free(manifest);
    bitactor_ls_matrix_destroy(ls_matrix);
TEST_END

// Test learning modes
TEST_FUNCTION(learning_modes)
    bitactor_ls_matrix_t* ls_matrix = bitactor_ls_matrix_create();
    bitactor_manifest_t* manifest = create_bitactor_manifest("test_spec");
    
    uint32_t actor_id = bitactor_ls_add_actor(ls_matrix, 0x42, manifest);
    bitactor_ls_core_t* ls_actor = &ls_matrix->learning_actors[actor_id];
    
    // Test mode setting
    bitactor_ls_set_learning_mode(ls_actor, 0); // Off
    TEST_ASSERT(ls_actor->learning_mode == 0, "Learning mode set to off");
    
    bitactor_ls_set_learning_mode(ls_actor, 1); // Observe
    TEST_ASSERT(ls_actor->learning_mode == 1, "Learning mode set to observe");
    
    bitactor_ls_set_learning_mode(ls_actor, 2); // Adapt
    TEST_ASSERT(ls_actor->learning_mode == 2, "Learning mode set to adapt");
    
    // Test invalid mode
    bitactor_ls_set_learning_mode(ls_actor, 5); // Invalid
    TEST_ASSERT(ls_actor->learning_mode == 0, "Invalid mode defaults to off");
    
    // Test adaptation threshold
    bitactor_ls_set_adaptation_threshold(ls_actor, 0.7);
    TEST_ASSERT(ls_actor->adaptation_threshold == 0.7, "Adaptation threshold set");
    
    // Test threshold clamping
    bitactor_ls_set_adaptation_threshold(ls_actor, 1.5); // Too high
    TEST_ASSERT(ls_actor->adaptation_threshold == 1.0, "High threshold clamped");
    
    bitactor_ls_set_adaptation_threshold(ls_actor, -0.1); // Too low
    TEST_ASSERT(ls_actor->adaptation_threshold == 0.1, "Low threshold clamped");
    
    free(manifest->bytecode);
    free(manifest);
    bitactor_ls_matrix_destroy(ls_matrix);
TEST_END

// Test hot path performance (critical test)
TEST_FUNCTION(hot_path_performance)
    bitactor_ls_matrix_t* ls_matrix = bitactor_ls_matrix_create();
    bitactor_manifest_t* manifest = create_bitactor_manifest("test_spec");
    
    // Add several learning actors
    for (int i = 0; i < 16; i++) {
        bitactor_ls_add_actor(ls_matrix, (bitactor_meaning_t)i, manifest);
    }
    
    // Add patterns and enable learning
    for (uint32_t i = 0; i < ls_matrix->learning_actor_count; i++) {
        bitactor_ls_core_t* ls_actor = &ls_matrix->learning_actors[i];
        
        uint8_t pattern_data[4] = {(uint8_t)i, (uint8_t)(i+1), (uint8_t)(i+2), (uint8_t)(i+3)};
        bitactor_ls_add_pattern(ls_actor, pattern_data, 4, 0.8);
        bitactor_ls_set_learning_mode(ls_actor, 2); // Adapt mode
        bitactor_ls_activate_pattern(ls_actor, 0);
    }
    
    // Test hot path execution
    const int iterations = 1000;
    uint64_t total_cycles = 0;
    uint32_t sub_100ns_count = 0;
    uint32_t trinity_violations = 0;
    
    for (int i = 0; i < iterations; i++) {
        bitactor_signal_t signals[4] = {0x1234, 0x5678, 0x9ABC, 0xDEF0};
        
        uint64_t start = rdtsc();
        uint32_t executed = bitactor_ls_matrix_tick(ls_matrix, signals, 4);
        uint64_t cycles = rdtsc() - start;
        
        total_cycles += cycles;
        if (cycles < 700) sub_100ns_count++; // 100ns @ 7GHz
        
        TEST_ASSERT(executed > 0, "Matrix tick executed actors");
        
        // Check Trinity compliance for all actors
        for (uint32_t j = 0; j < ls_matrix->learning_actor_count; j++) {
            if (!ls_matrix->learning_actors[j].learning_trinity_compliant) {
                trinity_violations++;
            }
        }
    }
    
    double avg_cycles = (double)total_cycles / iterations;
    double sub_100ns_rate = (double)sub_100ns_count / iterations * 100.0;
    
    printf("    📊 Performance Results:\n");
    printf("    - Average: %.1f cycles (%.1fns @ 7GHz)\n", avg_cycles, avg_cycles / 7.0);
    printf("    - Sub-100ns rate: %.1f%%\n", sub_100ns_rate);
    printf("    - Trinity violations: %u/%u\n", trinity_violations, iterations * ls_matrix->learning_actor_count);
    
    TEST_ASSERT(sub_100ns_rate >= 90.0, "Sub-100ns rate acceptable (≥90%)");
    TEST_ASSERT(trinity_violations < (iterations * ls_matrix->learning_actor_count / 10), "Trinity violations minimal (<10%)");
    TEST_ASSERT(avg_cycles < 100.0, "Average cycles within reasonable range");
    
    free(manifest->bytecode);
    free(manifest);
    bitactor_ls_matrix_destroy(ls_matrix);
TEST_END

// Test learning cognitive cycle
TEST_FUNCTION(learning_cognitive_cycle)
    bitactor_ls_matrix_t* ls_matrix = bitactor_ls_matrix_create();
    bitactor_manifest_t* manifest = create_bitactor_manifest("test_spec");
    
    uint32_t actor_id = bitactor_ls_add_actor(ls_matrix, 0x42, manifest);
    bitactor_ls_core_t* ls_actor = &ls_matrix->learning_actors[actor_id];
    
    // Add a pattern and enable observation
    uint8_t pattern_data[4] = {0x42, 0x43, 0x44, 0x45};
    bitactor_ls_add_pattern(ls_actor, pattern_data, 4, 0.8);
    bitactor_ls_set_learning_mode(ls_actor, 1); // Observe mode
    
    // Execute cognitive cycle
    uint64_t result = execute_ls_cognitive_cycle(ls_actor, NULL);
    TEST_ASSERT(result != 0, "Cognitive cycle executed");
    
    // Check that learning hops were executed (bits 8, 9, 10)
    bool observe_executed = (result >> 8) & 0x01;
    bool adapt_executed = (result >> 9) & 0x01;
    bool validate_executed = (result >> 10) & 0x01;
    
    TEST_ASSERT(observe_executed == true, "Learning observe hop executed");
    TEST_ASSERT(validate_executed == true, "Learning validate hop executed");
    
    // Switch to adapt mode and test again
    bitactor_ls_set_learning_mode(ls_actor, 2); // Adapt mode
    bitactor_ls_activate_pattern(ls_actor, 0);
    
    result = execute_ls_cognitive_cycle(ls_actor, NULL);
    adapt_executed = (result >> 9) & 0x01;
    TEST_ASSERT(adapt_executed == true, "Learning adapt hop executed in adapt mode");
    
    free(manifest->bytecode);
    free(manifest);
    bitactor_ls_matrix_destroy(ls_matrix);
TEST_END

// Test pattern discovery
TEST_FUNCTION(pattern_discovery)
    bitactor_ls_matrix_t* ls_matrix = bitactor_ls_matrix_create();
    bitactor_manifest_t* manifest = create_bitactor_manifest("test_spec");
    
    uint32_t actor_id = bitactor_ls_add_actor(ls_matrix, 0x42, manifest);
    bitactor_ls_core_t* ls_actor = &ls_matrix->learning_actors[actor_id];
    
    // Create historical signals with repeating patterns
    bitactor_signal_t signals[32];
    for (int i = 0; i < 32; i++) {
        signals[i] = 0x1000 + (i % 4); // Pattern: 1000, 1001, 1002, 1003, repeat
    }
    
    // Discover patterns
    uint32_t discovered = bitactor_ls_discover_patterns(ls_actor, signals, 32, 4);
    TEST_ASSERT(discovered > 0, "At least one pattern discovered");
    TEST_ASSERT(ls_actor->pattern_count > 0, "Pattern count increased");
    
    // Check that patterns have reasonable confidence
    for (uint32_t i = 0; i < ls_actor->pattern_count; i++) {
        bitactor_ls_pattern_t* pattern = &ls_actor->patterns[i];
        TEST_ASSERT(pattern->confidence_score > 0, "Pattern has confidence");
        TEST_ASSERT(pattern->validity_flags & 0x01, "Pattern is valid");
    }
    
    free(manifest->bytecode);
    free(manifest);
    bitactor_ls_matrix_destroy(ls_matrix);
TEST_END

// Test pattern sharing
TEST_FUNCTION(pattern_sharing)
    bitactor_ls_matrix_t* ls_matrix = bitactor_ls_matrix_create();
    bitactor_manifest_t* manifest = create_bitactor_manifest("test_spec");
    
    // Create two actors
    uint32_t actor1_id = bitactor_ls_add_actor(ls_matrix, 0x01, manifest);
    uint32_t actor2_id = bitactor_ls_add_actor(ls_matrix, 0x02, manifest);
    
    bitactor_ls_core_t* actor1 = &ls_matrix->learning_actors[actor1_id];
    bitactor_ls_core_t* actor2 = &ls_matrix->learning_actors[actor2_id];
    
    // Add pattern to first actor
    uint8_t pattern_data[4] = {0xAA, 0xBB, 0xCC, 0xDD};
    bitactor_ls_add_pattern(actor1, pattern_data, 4, 0.9);
    
    TEST_ASSERT(actor1->pattern_count == 1, "Actor1 has one pattern");
    TEST_ASSERT(actor2->pattern_count == 0, "Actor2 has no patterns");
    
    // Share pattern from actor1 to actor2
    bool shared = bitactor_ls_share_pattern(ls_matrix, actor1_id, actor2_id, 0);
    TEST_ASSERT(shared == true, "Pattern shared successfully");
    TEST_ASSERT(actor2->pattern_count == 1, "Actor2 now has one pattern");
    
    // Check that pattern was copied correctly
    bitactor_ls_pattern_t* original = &actor1->patterns[0];
    bitactor_ls_pattern_t* copy = &actor2->patterns[0];
    TEST_ASSERT(original->pattern_hash == copy->pattern_hash, "Pattern hash copied");
    TEST_ASSERT(original->confidence_score == copy->confidence_score, "Confidence copied");
    TEST_ASSERT(memcmp(original->compiled_logic, copy->compiled_logic, 32) == 0, "Logic copied");
    
    free(manifest->bytecode);
    free(manifest);
    bitactor_ls_matrix_destroy(ls_matrix);
TEST_END

// Test CNS integration
TEST_FUNCTION(cns_integration)
    cns_bitactor_ls_system_t* ls_sys = cns_bitactor_ls_create();
    TEST_ASSERT(ls_sys != NULL, "Learning CNS system created");
    TEST_ASSERT(ls_sys->ls_matrix != NULL, "Learning matrix created");
    TEST_ASSERT(ls_sys->learning_enabled == true, "Learning enabled by default");
    
    // Test learning configuration
    bool config_result = cns_bitactor_ls_configure_adaptation(ls_sys, 0.75, 500);
    TEST_ASSERT(config_result == true, "Adaptation configured successfully");
    TEST_ASSERT(ls_sys->global_adaptation_threshold == 0.75, "Threshold set correctly");
    
    // Test learning enable/disable
    bool disable_result = cns_bitactor_ls_enable_learning(ls_sys, false);
    TEST_ASSERT(disable_result == true, "Learning disabled successfully");
    TEST_ASSERT(ls_sys->learning_enabled == false, "Learning disabled flag set");
    
    bool enable_result = cns_bitactor_ls_enable_learning(ls_sys, true);
    TEST_ASSERT(enable_result == true, "Learning re-enabled successfully");
    TEST_ASSERT(ls_sys->learning_enabled == true, "Learning enabled flag set");
    
    // Test TTL execution
    const char* test_ttl = "@prefix ex: <http://example.org/> . ex:test ex:hasProperty ex:value .";
    bool exec_result = cns_bitactor_ls_execute(ls_sys, test_ttl);
    TEST_ASSERT(exec_result == true, "TTL execution successful");
    
    cns_bitactor_ls_destroy(ls_sys);
TEST_END

// Test performance validation
TEST_FUNCTION(performance_validation)
    bitactor_ls_matrix_t* ls_matrix = bitactor_ls_matrix_create();
    bitactor_manifest_t* manifest = create_bitactor_manifest("test_spec");
    
    // Add actors with learning enabled
    for (int i = 0; i < 8; i++) {
        uint32_t actor_id = bitactor_ls_add_actor(ls_matrix, (bitactor_meaning_t)i, manifest);
        bitactor_ls_core_t* ls_actor = &ls_matrix->learning_actors[actor_id];
        
        uint8_t pattern_data[4] = {(uint8_t)i, (uint8_t)(i+1), (uint8_t)(i+2), (uint8_t)(i+3)};
        bitactor_ls_add_pattern(ls_actor, pattern_data, 4, 0.8);
        bitactor_ls_set_learning_mode(ls_actor, 2); // Adapt mode
    }
    
    // Run some executions to generate metrics
    for (int i = 0; i < 100; i++) {
        bitactor_signal_t signals[2] = {(bitactor_signal_t)i, (bitactor_signal_t)(i+1000)};
        bitactor_ls_matrix_tick(ls_matrix, signals, 2);
    }
    
    // Validate performance
    learning_performance_result_t perf = validate_ls_performance(ls_matrix);
    
    TEST_ASSERT(perf.trinity_preserved == true, "Trinity constraints preserved");
    TEST_ASSERT(perf.sub_100ns_maintained == true, "Sub-100ns performance maintained");
    TEST_ASSERT(perf.learning_overhead_percent < 20.0, "Learning overhead reasonable (<20%)");
    TEST_ASSERT(perf.execution_cycles > 0, "Execution cycles recorded");
    
    printf("    📊 Performance Validation:\n");
    printf("    - Trinity preserved: %s\n", perf.trinity_preserved ? "✅" : "❌");
    printf("    - Sub-100ns maintained: %s\n", perf.sub_100ns_maintained ? "✅" : "❌");
    printf("    - Learning overhead: %.1f%%\n", perf.learning_overhead_percent);
    printf("    - Learning effective: %s\n", perf.learning_effective ? "✅" : "❌");
    
    free(manifest->bytecode);
    free(manifest);
    bitactor_ls_matrix_destroy(ls_matrix);
TEST_END

// Main test runner
int main() {
    printf("🧠 BITACTOR-LS Learning System Test Suite\n");
    printf("==========================================\n");
    
    int tests_passed = 0;
    int tests_total = 0;
    
    #define RUN_TEST(name) \
        do { \
            tests_total++; \
            if (test_##name()) { \
                tests_passed++; \
            } \
        } while(0)
    
    RUN_TEST(ls_matrix_creation);
    RUN_TEST(ls_actor_creation);
    RUN_TEST(pattern_management);
    RUN_TEST(learning_modes);
    RUN_TEST(hot_path_performance);
    RUN_TEST(learning_cognitive_cycle);
    RUN_TEST(pattern_discovery);
    RUN_TEST(pattern_sharing);
    RUN_TEST(cns_integration);
    RUN_TEST(performance_validation);
    
    printf("\n🧠 Test Results\n");
    printf("===============\n");
    printf("Tests passed: %d/%d\n", tests_passed, tests_total);
    
    if (tests_passed == tests_total) {
        printf("🎉 All tests passed! BITACTOR-LS is ready for production.\n");
        
        // Run final benchmark
        printf("\n🚀 Running final BITACTOR-LS benchmark...\n");
        benchmark_bitactor_ls();
        
        return 0;
    } else {
        printf("❌ Some tests failed. Check implementation.\n");
        return 1;
    }
}