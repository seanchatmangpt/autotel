/**
 * CNS v8 ML Turtle Loop Demo
 * Demonstrates ML-based pattern prediction and dynamic optimization
 */

#include "cns_v8_turtle_loop_ml_optimizer.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

// Sample turtle data with realistic pattern distribution
const char* sample_turtle_data = 
    "@prefix rdf: <http://www.w3.org/1999/02/22-rdf-syntax-ns#> .\n"
    "@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> .\n"
    "@prefix owl: <http://www.w3.org/2002/07/owl#> .\n"
    "@prefix ex: <http://example.org/> .\n"
    "\n"
    "# Type declarations (30% - vital few)\n"
    "ex:Person a owl:Class .\n"
    "ex:john a ex:Person .\n"
    "ex:mary a ex:Person .\n"
    "ex:Company a owl:Class .\n"
    "ex:acme a ex:Company .\n"
    "\n"
    "# Labels (20% - vital few)\n"
    "ex:john rdfs:label \"John Doe\" .\n"
    "ex:mary rdfs:label \"Mary Smith\" .\n"
    "ex:acme rdfs:label \"ACME Corporation\" .\n"
    "\n"
    "# Properties (20% - vital few)\n"
    "ex:john ex:age \"30\" .\n"
    "ex:john ex:worksFor ex:acme .\n"
    "ex:mary ex:age \"28\" .\n"
    "ex:mary ex:worksFor ex:acme .\n"
    "\n"
    "# Hierarchy (10% - still important)\n"
    "ex:Employee rdfs:subClassOf ex:Person .\n"
    "ex:Manager rdfs:subClassOf ex:Employee .\n"
    "\n"
    "# Other patterns (20% - trivial many)\n"
    "ex:john ex:hobby \"reading\" .\n"
    "ex:john ex:hobby \"cycling\" .\n"
    "ex:mary ex:hobby \"painting\" .\n"
    "ex:acme ex:founded \"1985\" .\n"
    "ex:acme ex:industry \"technology\" .\n";

// Generate synthetic turtle data with controlled pattern distribution
char* generate_synthetic_turtle(size_t triple_count) {
    size_t buffer_size = triple_count * 100; // Estimate ~100 chars per triple
    char* buffer = malloc(buffer_size);
    char* ptr = buffer;
    
    // Write prefixes
    ptr += sprintf(ptr, "@prefix ex: <http://example.org/> .\n");
    ptr += sprintf(ptr, "@prefix rdf: <http://www.w3.org/1999/02/22-rdf-syntax-ns#> .\n");
    ptr += sprintf(ptr, "@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> .\n\n");
    
    // Generate triples following 80/20 distribution
    for (size_t i = 0; i < triple_count; i++) {
        int pattern_choice = rand() % 100;
        
        if (pattern_choice < 30) {
            // Type declaration (30%)
            ptr += sprintf(ptr, "ex:entity%zu a ex:Class%d .\n", 
                          i, rand() % 10);
        } else if (pattern_choice < 50) {
            // Label (20%)
            ptr += sprintf(ptr, "ex:entity%zu rdfs:label \"Entity %zu\" .\n", 
                          i, i);
        } else if (pattern_choice < 70) {
            // Property (20%)
            ptr += sprintf(ptr, "ex:entity%zu ex:property%d \"%d\" .\n", 
                          i, rand() % 20, rand() % 1000);
        } else if (pattern_choice < 80) {
            // Hierarchy (10%)
            ptr += sprintf(ptr, "ex:class%zu rdfs:subClassOf ex:class%d .\n", 
                          i, rand() % 10);
        } else {
            // Other (20%)
            ptr += sprintf(ptr, "ex:entity%zu ex:misc%d \"value%zu\" .\n", 
                          i, rand() % 50, i);
        }
    }
    
    return buffer;
}

// Demonstrate ML optimization features
void demonstrate_ml_optimization() {
    printf("🧠 CNS v8 ML-Enhanced Turtle Loop Demo\n");
    printf("=====================================\n\n");
    
    // Initialize base turtle loop
    cns_v8_turtle_loop_t base_loop;
    if (cns_v8_turtle_loop_init(&base_loop, 16384) != 0) {
        fprintf(stderr, "Failed to initialize base turtle loop\n");
        return;
    }
    
    // Initialize ML-enhanced loop
    cns_v8_ml_turtle_loop_t ml_loop;
    if (cns_v8_ml_turtle_loop_init(&ml_loop, &base_loop, 0.85) != 0) {
        fprintf(stderr, "Failed to initialize ML turtle loop\n");
        cns_v8_turtle_loop_cleanup(&base_loop);
        return;
    }
    
    printf("✅ Initialized ML-enhanced turtle loop\n");
    printf("   Target Pareto efficiency: 0.85\n");
    printf("   Initial 80/20 split: %.1f/%.1f\n\n", 
           ml_loop.optimizer.current_split * 100,
           (1.0 - ml_loop.optimizer.current_split) * 100);
    
    // Process sample data to train the predictor
    printf("📊 Phase 1: Training on sample data\n");
    printf("   Processing %zu bytes of turtle data...\n", strlen(sample_turtle_data));
    
    char output_buffer[4096];
    size_t output_size = sizeof(output_buffer);
    
    int result = cns_v8_ml_process_turtle(&ml_loop, sample_turtle_data, 
                                         strlen(sample_turtle_data),
                                         output_buffer, &output_size);
    
    if (result == 0) {
        printf("   ✅ Processing successful\n");
        
        // Get initial metrics
        cns_v8_ml_metrics_t metrics;
        cns_v8_get_ml_metrics(&ml_loop, &metrics);
        
        printf("\n📈 Initial Metrics:\n");
        printf("   Pattern Distribution:\n");
        printf("     - Type declarations: %.1f%% (vital)\n", 
               metrics.base_metrics.pattern_coverage[0]);
        printf("     - Labels: %.1f%% (vital)\n", 
               metrics.base_metrics.pattern_coverage[1]);
        printf("     - Properties: %.1f%% (vital)\n", 
               metrics.base_metrics.pattern_coverage[2]);
        printf("     - Hierarchy: %.1f%% (important)\n", 
               metrics.base_metrics.pattern_coverage[3]);
        printf("     - Other: %.1f%% (trivial)\n", 
               metrics.base_metrics.pattern_coverage[4]);
        printf("   Pareto Efficiency: %.3f\n", 
               metrics.base_metrics.pareto_efficiency);
        printf("   Avg cycles/triple: %.1f\n\n", 
               metrics.base_metrics.avg_cycles_per_triple);
    }
    
    // Generate larger dataset for testing
    printf("📊 Phase 2: Testing with synthetic data\n");
    size_t test_triple_count = 1000;
    char* synthetic_data = generate_synthetic_turtle(test_triple_count);
    
    printf("   Generated %zu triples for testing\n", test_triple_count);
    
    // Test different optimization strategies
    optimization_strategy_t strategies[] = {
        OPT_STRATEGY_CONSERVATIVE,
        OPT_STRATEGY_AGGRESSIVE,
        OPT_STRATEGY_ADAPTIVE,
        OPT_STRATEGY_QUANTUM
    };
    
    const char* strategy_names[] = {
        "Conservative (80/20)",
        "Aggressive (90/10)",
        "Adaptive (ML-driven)",
        "Quantum (8-aligned)"
    };
    
    printf("\n🔄 Testing optimization strategies:\n");
    
    for (int s = 0; s < 4; s++) {
        printf("\n   Strategy: %s\n", strategy_names[s]);
        cns_v8_set_optimization_strategy(&ml_loop, strategies[s]);
        
        // Reset metrics
        ml_loop.metrics.total_predictions = 0;
        ml_loop.metrics.accurate_predictions = 0;
        ml_loop.metrics.vital_few_identified = 0;
        ml_loop.metrics.trivial_many_skipped = 0;
        
        // Process with current strategy
        output_size = sizeof(output_buffer);
        uint64_t start_time = clock();
        
        result = cns_v8_ml_process_turtle(&ml_loop, synthetic_data,
                                         strlen(synthetic_data),
                                         output_buffer, &output_size);
        
        uint64_t elapsed = clock() - start_time;
        
        if (result == 0) {
            cns_v8_ml_metrics_t metrics;
            cns_v8_get_ml_metrics(&ml_loop, &metrics);
            
            printf("   Results:\n");
            printf("     - Prediction accuracy: %.1f%%\n", 
                   metrics.prediction_accuracy * 100);
            printf("     - Pareto efficiency: %.3f %s\n", 
                   metrics.optimization_efficiency,
                   metrics.optimization_efficiency >= 0.85 ? "✅" : "❌");
            printf("     - Vital patterns found: %llu\n", 
                   metrics.vital_few_identified);
            printf("     - Trivial patterns skipped: %llu\n", 
                   metrics.trivial_many_skipped);
            printf("     - ML overhead: %.1f%%\n", 
                   metrics.ml_overhead_percent);
            printf("     - Processing time: %llu μs\n", 
                   elapsed / (CLOCKS_PER_SEC / 1000000));
            
            // Show feedback improvement
            if (s == 2) { // Adaptive strategy
                printf("     - Feedback improvement rate: %.3f\n",
                       metrics.feedback_improvement_rate);
            }
        }
    }
    
    // Demonstrate pattern prediction
    printf("\n🔮 Phase 3: Pattern Prediction Demo\n");
    
    // Create a sequence with clear patterns
    triple_pattern_t pattern_sequence[] = {
        PATTERN_TYPE_DECL, PATTERN_TYPE_DECL, PATTERN_LABEL,
        PATTERN_TYPE_DECL, PATTERN_TYPE_DECL, PATTERN_LABEL,
        PATTERN_TYPE_DECL, PATTERN_TYPE_DECL, PATTERN_LABEL,
        PATTERN_PROPERTY, PATTERN_PROPERTY, PATTERN_HIERARCHY
    };
    
    printf("   Training on pattern sequence...\n");
    cns_v8_train_pattern_predictor(&ml_loop, pattern_sequence, 12);
    
    printf("   Making predictions:\n");
    for (int i = 0; i < 5; i++) {
        double confidence;
        triple_pattern_t predicted = cns_v8_predict_next_pattern(
            &ml_loop, &pattern_sequence[i], 3, &confidence);
        
        const char* pattern_names[] = {
            "TYPE_DECL", "LABEL", "PROPERTY", "HIERARCHY", "OTHER"
        };
        
        int pred_idx = __builtin_ctz(predicted);
        printf("     After seeing 3 patterns starting at %d: predict %s (confidence: %.2f)\n",
               i, pattern_names[pred_idx < 5 ? pred_idx : 4], confidence);
    }
    
    // Final performance summary
    printf("\n📊 Final Performance Summary\n");
    printf("================================\n");
    
    cns_v8_ml_metrics_t final_metrics;
    cns_v8_get_ml_metrics(&ml_loop, &final_metrics);
    
    printf("✅ Pareto Efficiency: %.3f (target: 0.85)\n", 
           final_metrics.optimization_efficiency);
    printf("✅ Pattern Prediction Accuracy: %.1f%%\n", 
           final_metrics.prediction_accuracy * 100);
    printf("✅ 7-tick Compliance: %s\n", 
           final_metrics.base_metrics.avg_cycles_per_triple <= 7 ? "YES" : "NO");
    printf("✅ Vital Few Coverage: %.1f%% of patterns\n",
           (final_metrics.base_metrics.pattern_coverage[0] +
            final_metrics.base_metrics.pattern_coverage[1] +
            final_metrics.base_metrics.pattern_coverage[2]));
    
    // Cleanup
    free(synthetic_data);
    cns_v8_ml_turtle_loop_cleanup(&ml_loop);
    cns_v8_turtle_loop_cleanup(&base_loop);
    
    printf("\n✅ Demo completed successfully!\n");
}

// Performance benchmark
void benchmark_ml_optimization() {
    printf("\n⚡ Performance Benchmark\n");
    printf("========================\n");
    
    cns_v8_turtle_loop_t base_loop;
    cns_v8_ml_turtle_loop_t ml_loop;
    
    cns_v8_turtle_loop_init(&base_loop, 16384);
    cns_v8_ml_turtle_loop_init(&ml_loop, &base_loop, 0.85);
    
    // Test with increasing data sizes
    size_t test_sizes[] = {100, 500, 1000, 5000, 10000};
    
    printf("\nTriples | Base (μs) | ML-Opt (μs) | Speedup | Efficiency\n");
    printf("--------|-----------|-------------|---------|------------\n");
    
    for (int i = 0; i < 5; i++) {
        char* data = generate_synthetic_turtle(test_sizes[i]);
        char output[65536];
        size_t out_size = sizeof(output);
        
        // Benchmark base implementation
        uint64_t base_start = clock();
        cns_v8_process_turtle(&base_loop, data, strlen(data), output, &out_size);
        uint64_t base_time = clock() - base_start;
        
        // Benchmark ML-optimized implementation
        out_size = sizeof(output);
        uint64_t ml_start = clock();
        cns_v8_ml_process_turtle(&ml_loop, data, strlen(data), output, &out_size);
        uint64_t ml_time = clock() - ml_start;
        
        // Get efficiency
        cns_v8_ml_metrics_t metrics;
        cns_v8_get_ml_metrics(&ml_loop, &metrics);
        
        double speedup = (double)base_time / ml_time;
        
        printf("%-7zu | %-9llu | %-11llu | %-7.2fx | %.3f\n",
               test_sizes[i],
               base_time / (CLOCKS_PER_SEC / 1000000),
               ml_time / (CLOCKS_PER_SEC / 1000000),
               speedup,
               metrics.optimization_efficiency);
        
        free(data);
    }
    
    cns_v8_ml_turtle_loop_cleanup(&ml_loop);
    cns_v8_turtle_loop_cleanup(&base_loop);
}

int main(int argc, char** argv) {
    // Seed random number generator
    srand(time(NULL));
    
    // Run demonstration
    demonstrate_ml_optimization();
    
    // Run benchmark
    benchmark_ml_optimization();
    
    return 0;
}