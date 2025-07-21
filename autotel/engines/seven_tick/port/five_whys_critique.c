/**
 * Five Whys Critique Implementation
 * Demonstrates why linear root cause analysis fails in complex systems
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

// Structure representing a "5 Whys" analysis
typedef struct {
    char* problem;
    char* why[5];
    char* root_cause;
    char* proposed_solution;
} five_whys_t;

// Structure representing actual system state
typedef struct {
    float parsing_efficiency;
    float validation_overhead;
    float memory_alignment;
    float cache_hit_rate;
    float pattern_distribution[5];  // 80/20 patterns
    float network_latency;
    float cpu_utilization;
    float feedback_strength;       // System adaptation
} system_state_t;

// Simulate "5 Whys" analysis
five_whys_t* perform_five_whys(const char* problem) {
    five_whys_t* analysis = calloc(1, sizeof(five_whys_t));
    
    analysis->problem = strdup(problem);
    
    // Linear questioning (picks one path arbitrarily)
    if (strstr(problem, "slow")) {
        analysis->why[0] = "Because parsing takes too long";
        analysis->why[1] = "Because we parse every triple";
        analysis->why[2] = "Because we need to validate them";
        analysis->why[3] = "Because data might be invalid";
        analysis->why[4] = "Because users make mistakes";
        analysis->root_cause = "User input quality";
        analysis->proposed_solution = "Add input validation";
    } else if (strstr(problem, "crash")) {
        analysis->why[0] = "Because memory ran out";
        analysis->why[1] = "Because of memory leaks";
        analysis->why[2] = "Because resources aren't freed";
        analysis->why[3] = "Because of poor coding practices";
        analysis->why[4] = "Because of lack of code reviews";
        analysis->root_cause = "Development process";
        analysis->proposed_solution = "Implement code reviews";
    }
    
    return analysis;
}

// Simulate real complex system behavior
void simulate_system_iteration(system_state_t* state, bool intervention_applied) {
    // Complex systems have multiple interacting factors
    
    // Parsing efficiency affects memory pressure
    float memory_pressure = 1.0f - state->parsing_efficiency;
    
    // Memory pressure affects cache performance
    state->cache_hit_rate *= (1.0f - memory_pressure * 0.1f);
    
    // Cache misses affect parsing (feedback loop!)
    state->parsing_efficiency *= (0.5f + 0.5f * state->cache_hit_rate);
    
    // Network effects
    state->network_latency *= (1.0f + (1.0f - state->cpu_utilization) * 0.05f);
    
    // Pattern distribution evolves (80/20 is dynamic)
    for (int i = 0; i < 5; i++) {
        state->pattern_distribution[i] += (rand() % 100 - 50) / 1000.0f;
        if (state->pattern_distribution[i] < 0) state->pattern_distribution[i] = 0;
    }
    
    // Normalize patterns
    float sum = 0;
    for (int i = 0; i < 5; i++) sum += state->pattern_distribution[i];
    for (int i = 0; i < 5; i++) state->pattern_distribution[i] /= sum;
    
    // System adapts (emergence)
    state->feedback_strength *= 1.01f;  // System learns to compensate
    
    // If "5 Whys solution" was applied (input validation)
    if (intervention_applied) {
        // It helps a tiny bit with validation
        state->validation_overhead *= 0.95f;
        
        // But creates new problems!
        state->parsing_efficiency *= 0.9f;  // More validation = slower parsing
        state->cpu_utilization *= 1.1f;     // More CPU usage
    }
}

// Calculate actual system performance
float calculate_system_performance(const system_state_t* state) {
    // Performance is emergent from multiple factors
    float performance = 1.0f;
    
    performance *= state->parsing_efficiency;
    performance *= (2.0f - state->validation_overhead);  // Less validation is better
    performance *= state->cache_hit_rate;
    performance *= (2.0f - state->network_latency);
    performance *= (2.0f - state->cpu_utilization);
    
    // 80/20 bonus for optimized patterns
    float pattern_optimization = 0.0f;
    // First 4 patterns should cover 80%
    for (int i = 0; i < 4; i++) {
        pattern_optimization += state->pattern_distribution[i];
    }
    performance *= (0.5f + pattern_optimization);
    
    // Feedback effects (can amplify or dampen)
    performance *= state->feedback_strength;
    
    return performance;
}

// Find actual high-impact factors using measurement
typedef struct {
    const char* factor;
    float impact;
} impact_measurement_t;

void measure_factor_impacts(system_state_t* baseline, impact_measurement_t* impacts) {
    // Measure impact of each factor by perturbation
    system_state_t test_state;
    float baseline_perf = calculate_system_performance(baseline);
    
    // Test parsing efficiency
    memcpy(&test_state, baseline, sizeof(system_state_t));
    test_state.parsing_efficiency *= 1.2f;
    impacts[0].factor = "parsing_efficiency";
    impacts[0].impact = calculate_system_performance(&test_state) - baseline_perf;
    
    // Test validation overhead
    memcpy(&test_state, baseline, sizeof(system_state_t));
    test_state.validation_overhead *= 0.8f;
    impacts[1].factor = "validation_overhead";
    impacts[1].impact = calculate_system_performance(&test_state) - baseline_perf;
    
    // Test cache hit rate
    memcpy(&test_state, baseline, sizeof(system_state_t));
    test_state.cache_hit_rate *= 1.2f;
    impacts[2].factor = "cache_hit_rate";
    impacts[2].impact = calculate_system_performance(&test_state) - baseline_perf;
    
    // Test memory alignment
    memcpy(&test_state, baseline, sizeof(system_state_t));
    test_state.memory_alignment = 8.0f;  // Perfect alignment
    impacts[3].factor = "memory_alignment";
    impacts[3].impact = calculate_system_performance(&test_state) - baseline_perf;
    
    // Sort by impact
    for (int i = 0; i < 4; i++) {
        for (int j = i + 1; j < 4; j++) {
            if (impacts[j].impact > impacts[i].impact) {
                impact_measurement_t temp = impacts[i];
                impacts[i] = impacts[j];
                impacts[j] = temp;
            }
        }
    }
}

int main() {
    printf("=== Five Whys Failure Demonstration ===\n\n");
    
    // Initialize system
    system_state_t system = {
        .parsing_efficiency = 0.7f,
        .validation_overhead = 1.3f,
        .memory_alignment = 4.0f,
        .cache_hit_rate = 0.6f,
        .pattern_distribution = {0.3f, 0.2f, 0.2f, 0.1f, 0.2f},
        .network_latency = 1.0f,
        .cpu_utilization = 0.8f,
        .feedback_strength = 1.0f
    };
    
    // Problem statement
    const char* problem = "System is slow";
    
    // Perform "5 Whys" analysis
    printf("1. Five Whys Analysis:\n");
    printf("   Problem: %s\n", problem);
    five_whys_t* five_whys = perform_five_whys(problem);
    
    for (int i = 0; i < 5; i++) {
        printf("   Why %d: %s\n", i + 1, five_whys->why[i]);
    }
    printf("   'Root Cause': %s\n", five_whys->root_cause);
    printf("   'Solution': %s\n\n", five_whys->proposed_solution);
    
    // Measure actual impacts
    printf("2. Actual Impact Measurements:\n");
    impact_measurement_t impacts[4];
    measure_factor_impacts(&system, impacts);
    
    float total_impact = 0;
    for (int i = 0; i < 4; i++) {
        printf("   %s: %.3f impact\n", impacts[i].factor, impacts[i].impact);
        total_impact += impacts[i].impact;
    }
    
    // Show 80/20
    float cumulative = 0;
    printf("\n3. 80/20 Analysis:\n");
    for (int i = 0; i < 4; i++) {
        cumulative += impacts[i].impact;
        float percent = (cumulative / total_impact) * 100;
        printf("   Top %d factors: %.1f%% of impact\n", i + 1, percent);
        if (percent >= 80) {
            printf("   → 80%% impact from %d/%d factors\n", i + 1, 4);
            break;
        }
    }
    
    // Simulate applying "5 Whys solution"
    printf("\n4. Applying '5 Whys' Solution (Input Validation):\n");
    float initial_performance = calculate_system_performance(&system);
    printf("   Initial performance: %.3f\n", initial_performance);
    
    // Run simulation
    for (int i = 1; i <= 10; i++) {
        simulate_system_iteration(&system, true);
        float performance = calculate_system_performance(&system);
        printf("   Iteration %2d performance: %.3f %s\n", 
               i, performance,
               performance < initial_performance ? "↓ WORSE!" : "↑");
    }
    
    // Show why it failed
    printf("\n5. Why '5 Whys' Failed:\n");
    printf("   ✗ Assumed linear causation (A→B→C→D→E)\n");
    printf("   ✗ Ignored feedback loops (cache→parsing→memory→cache)\n");
    printf("   ✗ Missed emergent behavior (system adaptation)\n");
    printf("   ✗ Picked arbitrary causal path (confirmation bias)\n");
    printf("   ✗ Ignored measurement data (empirical impacts)\n");
    printf("   ✗ Static analysis (patterns evolve over time)\n");
    
    // Show correct approach
    printf("\n6. Correct Approach (Empirical 80/20):\n");
    printf("   ✓ Measure all factor impacts\n");
    printf("   ✓ Identify vital few (parsing + cache)\n");
    printf("   ✓ Consider network effects\n");
    printf("   ✓ Monitor dynamic changes\n");
    printf("   ✓ Optimize intersection of high-impact factors\n");
    
    // Demonstrate correct intervention
    printf("\n7. Applying Correct Solution (Optimize parsing + cache):\n");
    system.parsing_efficiency = 0.9f;
    system.cache_hit_rate = 0.9f;
    system.memory_alignment = 8.0f;  // Bonus: align memory
    
    float optimized_performance = calculate_system_performance(&system);
    printf("   Optimized performance: %.3f (%.1fx improvement)\n", 
           optimized_performance,
           optimized_performance / initial_performance);
    
    // Pattern distribution info
    printf("\n8. Dynamic Pattern Distribution:\n");
    const char* patterns[] = {"Type decl", "Labels", "Properties", "Hierarchy", "Other"};
    for (int i = 0; i < 5; i++) {
        printf("   %s: %.1f%%\n", patterns[i], system.pattern_distribution[i] * 100);
    }
    
    free(five_whys->problem);
    free(five_whys);
    
    return 0;
}