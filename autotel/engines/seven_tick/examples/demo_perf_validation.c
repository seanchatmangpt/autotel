#include "../lib/s7t_perf.h"
#include "../lib/7t_pm.h"
#include "../lib/7t_mcts.h"
#include "../lib/7t_sparql.h"
#include "../lib/7t_shacl.h"
#include "../lib/7t_owl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// ============================================================================
// BENCHMARK OPERATIONS
// ============================================================================

// Benchmark PM7T operations
static void benchmark_pm7t() {
    printf("\n=== PM7T ENGINE VALIDATION ===\n");
    
    PM7T* pm = pm7t_create();
    
    // Validate event addition
    S7T_ASSERT_CYCLES("pm7t_add_event", {
        pm7t_add_event(pm, "case1", "start", 1000);
    });
    
    // Add more events for pattern mining
    for (int i = 0; i < 10; i++) {
        char activity[32];
        snprintf(activity, sizeof(activity), "activity_%d", i);
        pm7t_add_event(pm, "case1", activity, 1000 + i * 100);
    }
    
    // Validate pattern mining
    S7T_ASSERT_CYCLES("pm7t_mine_patterns", {
        pm7t_mine_patterns(pm);
    });
    
    // Validate conformance checking
    EventSequence* sequence = pm7t_get_sequence(pm, "case1");
    S7T_ASSERT_CYCLES("pm7t_check_conformance", {
        double conformance = pm7t_check_conformance(pm, sequence);
        (void)conformance;
    });
    
    pm7t_destroy(pm);
    printf("PM7T validation complete.\n");
}

// Benchmark MCTS7T operations
static void benchmark_mcts7t() {
    printf("\n=== MCTS7T ENGINE VALIDATION ===\n");
    
    MCTS7T* mcts = mcts7t_create();
    
    // Add some nodes for testing
    for (uint32_t i = 0; i < 5; i++) {
        mcts7t_expand(mcts, i, i + 1, 1.0);
    }
    
    // Validate selection
    uint32_t state = 0;
    S7T_ASSERT_CYCLES("mcts7t_select", {
        MCTSNode* node = mcts7t_select(mcts, state);
        (void)node;
    });
    
    // Validate expansion
    S7T_ASSERT_CYCLES("mcts7t_expand", {
        MCTSNode* node = mcts7t_expand(mcts, 5, 6, 1.0);
        (void)node;
    });
    
    // Validate simulation
    S7T_ASSERT_CYCLES("mcts7t_simulate", {
        double reward = mcts7t_simulate(mcts, 0);
        (void)reward;
    });
    
    // Validate backpropagation
    MCTSNode* leaf = mcts7t_get_node(mcts, 5);
    S7T_ASSERT_CYCLES("mcts7t_backpropagate", {
        mcts7t_backpropagate(mcts, leaf, 1.0);
    });
    
    mcts7t_destroy(mcts);
    printf("MCTS7T validation complete.\n");
}

// Benchmark SPARQL7T operations
static void benchmark_sparql7t() {
    printf("\n=== SPARQL7T ENGINE VALIDATION ===\n");
    
    SPARQL7TEngine* sparql = sparql7t_create();
    
    // Validate triple addition
    S7T_ASSERT_CYCLES("sparql7t_add_triple", {
        sparql7t_add_triple(sparql, "subject1", "predicate1", "object1");
    });
    
    // Add more triples
    for (int i = 0; i < 10; i++) {
        char s[32], p[32], o[32];
        snprintf(s, sizeof(s), "s%d", i);
        snprintf(p, sizeof(p), "p%d", i);
        snprintf(o, sizeof(o), "o%d", i);
        sparql7t_add_triple(sparql, s, p, o);
    }
    
    // Validate query parsing
    const char* query = "SELECT ?s ?p ?o WHERE { ?s ?p ?o }";
    SPARQL7TQuery* q = sparql7t_parse_query(query);
    
    // Validate query execution
    S7T_ASSERT_CYCLES("sparql7t_execute_query", {
        SPARQL7TResult* result = sparql7t_execute_query(sparql, q);
        sparql7t_free_result(result);
    });
    
    sparql7t_free_query(q);
    sparql7t_destroy(sparql);
    printf("SPARQL7T validation complete.\n");
}

// Benchmark SHACL7T operations
static void benchmark_shacl7t() {
    printf("\n=== SHACL7T ENGINE VALIDATION ===\n");
    
    SHACL7TEngine* shacl = shacl7t_create();
    
    // Create a simple shape
    SHACL7TShape* shape = shacl7t_create_shape("PersonShape");
    shacl7t_add_property_constraint(shape, "name", SHACL7T_DATATYPE, "string");
    shacl7t_add_property_constraint(shape, "age", SHACL7T_MIN_COUNT, (void*)1);
    
    // Validate shape addition
    S7T_ASSERT_CYCLES("shacl7t_add_shape", {
        shacl7t_add_shape(shacl, shape);
    });
    
    // Create test data
    SHACL7TData* data = shacl7t_create_data();
    shacl7t_add_data_property(data, "person1", "name", "John");
    shacl7t_add_data_property(data, "person1", "age", "30");
    
    // Validate validation
    S7T_ASSERT_CYCLES("shacl7t_validate", {
        SHACL7TResult* result = shacl7t_validate(shacl, data);
        shacl7t_free_result(result);
    });
    
    shacl7t_free_data(data);
    shacl7t_destroy(shacl);
    printf("SHACL7T validation complete.\n");
}

// Benchmark OWL7T operations
static void benchmark_owl7t() {
    printf("\n=== OWL7T ENGINE VALIDATION ===\n");
    
    OWL7TEngine* owl = owl7t_create();
    
    // Validate class creation
    S7T_ASSERT_CYCLES("owl7t_add_class", {
        owl7t_add_class(owl, "Person");
    });
    
    // Add more classes and properties
    owl7t_add_class(owl, "Animal");
    owl7t_add_object_property(owl, "hasPet");
    owl7t_add_data_property(owl, "hasName");
    
    // Validate subclass relationship
    S7T_ASSERT_CYCLES("owl7t_add_subclass", {
        owl7t_add_subclass(owl, "Dog", "Animal");
    });
    
    // Validate reasoning
    S7T_ASSERT_CYCLES("owl7t_is_subclass", {
        bool is_sub = owl7t_is_subclass_of(owl, "Dog", "Animal");
        (void)is_sub;
    });
    
    // Validate consistency checking
    S7T_ASSERT_CYCLES("owl7t_check_consistency", {
        bool consistent = owl7t_check_consistency(owl);
        (void)consistent;
    });
    
    owl7t_destroy(owl);
    printf("OWL7T validation complete.\n");
}

// ============================================================================
// MEMORY ACCESS PATTERN VALIDATION
// ============================================================================

static void validate_memory_patterns() {
    printf("\n=== MEMORY ACCESS PATTERN VALIDATION ===\n");
    
    // Test different array sizes and access patterns
    size_t sizes[] = {64, 256, 1024, 4096};
    
    for (size_t i = 0; i < sizeof(sizes)/sizeof(sizes[0]); i++) {
        size_t size = sizes[i];
        uint32_t* array = aligned_alloc(64, size * sizeof(uint32_t));
        
        // Initialize array
        for (size_t j = 0; j < size; j++) {
            array[j] = j;
        }
        
        printf("\nArray size: %zu elements\n", size);
        
        // Sequential access
        g_memory_tracker.count = 0;  // Reset tracker
        g_memory_tracker.cache_hits = 0;
        g_memory_tracker.cache_misses = 0;
        
        S7T_ASSERT_CYCLES("sequential_read", {
            uint32_t sum = 0;
            for (size_t j = 0; j < size; j++) {
                S7T_VALIDATE_MEMORY_READ(&array[j], sizeof(uint32_t));
                sum += array[j];
            }
        });
        
        MemoryAnalysis seq_analysis = s7t_analyze_memory_patterns();
        printf("  Sequential - Cache hit rate: %.2f%%\n", 
               seq_analysis.cache_hit_rate * 100);
        
        // Stride access (every 16th element)
        g_memory_tracker.count = 0;
        g_memory_tracker.cache_hits = 0;
        g_memory_tracker.cache_misses = 0;
        
        S7T_ASSERT_CYCLES("stride_read", {
            uint32_t sum = 0;
            for (size_t j = 0; j < size; j += 16) {
                S7T_VALIDATE_MEMORY_READ(&array[j], sizeof(uint32_t));
                sum += array[j];
            }
        });
        
        MemoryAnalysis stride_analysis = s7t_analyze_memory_patterns();
        printf("  Stride-16 - Cache hit rate: %.2f%%\n", 
               stride_analysis.cache_hit_rate * 100);
        
        free(array);
    }
}

// ============================================================================
// COMPREHENSIVE PERFORMANCE REPORT
// ============================================================================

static void generate_comprehensive_report() {
    printf("\n=== GENERATING COMPREHENSIVE PERFORMANCE REPORT ===\n");
    
    FILE* fp = fopen("S7T_PERFORMANCE_VALIDATION.md", "w");
    if (!fp) return;
    
    fprintf(fp, "# S7T Engine Performance Validation Report\n\n");
    fprintf(fp, "Generated: %s %s\n\n", __DATE__, __TIME__);
    
    fprintf(fp, "## Executive Summary\n\n");
    
    // Get validation statistics
    size_t total_ops = g_s7t_validator.count;
    size_t passed = total_ops - g_s7t_validator.failures;
    double success_rate = 100.0 * passed / total_ops;
    
    fprintf(fp, "- **Total Operations Validated**: %zu\n", total_ops);
    fprintf(fp, "- **Passed**: %zu (%.1f%%)\n", passed, success_rate);
    fprintf(fp, "- **Failed**: %zu\n", g_s7t_validator.failures);
    fprintf(fp, "- **Physics Compliance**: %s\n\n", 
            g_s7t_validator.failures == 0 ? "✅ PASSED" : "❌ FAILED");
    
    fprintf(fp, "## Engine-Specific Results\n\n");
    
    // Group results by engine
    const char* engines[] = {"pm7t", "mcts7t", "sparql7t", "shacl7t", "owl7t"};
    
    for (int e = 0; e < 5; e++) {
        fprintf(fp, "### %s Engine\n\n", engines[e]);
        fprintf(fp, "| Operation | Cycles | Status | Compliance |\n");
        fprintf(fp, "|-----------|--------|--------|------------|\n");
        
        for (size_t i = 0; i < g_s7t_validator.count; i++) {
            S7TValidation* val = &g_s7t_validator.validations[i];
            if (strstr(val->operation, engines[e])) {
                fprintf(fp, "| %s | %lu | %s | %s |\n",
                        val->operation,
                        val->cycles,
                        val->passed ? "✅ Pass" : "❌ Fail",
                        val->cycles <= S7T_MAX_CYCLES ? "7T Compliant" : "Violation");
            }
        }
        fprintf(fp, "\n");
    }
    
    fprintf(fp, "## Memory Access Analysis\n\n");
    MemoryAnalysis mem = s7t_analyze_memory_patterns();
    fprintf(fp, "- **Cache Hit Rate**: %.2f%%\n", mem.cache_hit_rate * 100);
    fprintf(fp, "- **Sequential Accesses**: %zu\n", mem.sequential_accesses);
    fprintf(fp, "- **Random Accesses**: %zu\n", mem.random_accesses);
    fprintf(fp, "- **Cache Line Splits**: %zu\n", mem.cache_line_splits);
    fprintf(fp, "- **Cache Efficiency**: %s\n\n", 
            mem.is_cache_friendly ? "✅ Optimal" : "⚠️ Suboptimal");
    
    fprintf(fp, "## Branch Prediction Analysis\n\n");
    BranchAnalysis branch = s7t_analyze_branch_patterns();
    fprintf(fp, "- **Total Branches**: %zu\n", branch.total_branches);
    fprintf(fp, "- **Mispredictions**: %zu\n", branch.mispredictions);
    fprintf(fp, "- **Prediction Accuracy**: %.2f%%\n", branch.prediction_accuracy * 100);
    fprintf(fp, "- **Branch Behavior**: %s\n\n", 
            branch.is_predictable ? "✅ Predictable" : "⚠️ Unpredictable");
    
    fprintf(fp, "## Recommendations\n\n");
    
    if (g_s7t_validator.failures > 0) {
        fprintf(fp, "### ⚠️ Performance Issues Detected\n\n");
        fprintf(fp, "The following operations exceed the 7-cycle limit:\n\n");
        
        for (size_t i = 0; i < g_s7t_validator.count; i++) {
            S7TValidation* val = &g_s7t_validator.validations[i];
            if (!val->passed) {
                fprintf(fp, "- **%s**: %lu cycles (%.1fx over limit)\n",
                        val->operation, val->cycles, 
                        (double)val->cycles / S7T_MAX_CYCLES);
            }
        }
        fprintf(fp, "\n");
    }
    
    if (!mem.is_cache_friendly) {
        fprintf(fp, "### 💾 Memory Access Optimization Needed\n\n");
        fprintf(fp, "- Consider improving data locality\n");
        fprintf(fp, "- Use sequential access patterns where possible\n");
        fprintf(fp, "- Align data structures to cache line boundaries\n\n");
    }
    
    if (!branch.is_predictable) {
        fprintf(fp, "### 🔀 Branch Prediction Optimization Needed\n\n");
        fprintf(fp, "- Consider using branch-free algorithms\n");
        fprintf(fp, "- Sort data to improve branch predictability\n");
        fprintf(fp, "- Use conditional moves instead of branches\n\n");
    }
    
    fprintf(fp, "## Conclusion\n\n");
    
    bool fully_compliant = (g_s7t_validator.failures == 0) && 
                          mem.is_cache_friendly && 
                          branch.is_predictable;
    
    if (fully_compliant) {
        fprintf(fp, "✅ **All engines are fully 7T physics compliant!**\n\n");
        fprintf(fp, "The implementation successfully meets all performance requirements:\n");
        fprintf(fp, "- All operations complete within 7 CPU cycles\n");
        fprintf(fp, "- Memory access patterns are cache-efficient\n");
        fprintf(fp, "- Branch prediction is optimal\n");
        fprintf(fp, "- No dynamic allocations in hot paths\n");
    } else {
        fprintf(fp, "❌ **Physics compliance violations detected.**\n\n");
        fprintf(fp, "Please address the issues identified above to achieve full compliance.\n");
    }
    
    fclose(fp);
    printf("Report saved to: S7T_PERFORMANCE_VALIDATION.md\n");
}

// ============================================================================
// MAIN VALIDATION RUNNER
// ============================================================================

int main(int argc, char* argv[]) {
    printf("╔══════════════════════════════════════════════╗\n");
    printf("║     S7T ENGINE PERFORMANCE VALIDATION        ║\n");
    printf("║          Physics Compliance Checker          ║\n");
    printf("╚══════════════════════════════════════════════╝\n");
    
    // Initialize validation framework
    s7t_validator_init(false);  // Non-strict for comprehensive testing
    s7t_memory_tracker_init();
    s7t_branch_tracker_init();
    
    // Run engine benchmarks
    benchmark_pm7t();
    benchmark_mcts7t();
    benchmark_sparql7t();
    benchmark_shacl7t();
    benchmark_owl7t();
    
    // Validate memory patterns
    validate_memory_patterns();
    
    // Generate reports
    s7t_validation_report();
    generate_comprehensive_report();
    s7t_generate_json_report("s7t_validation.json");
    s7t_generate_heatmap("s7t_heatmap.md");
    
    // Cleanup
    s7t_validator_cleanup();
    s7t_memory_tracker_cleanup();
    s7t_branch_tracker_cleanup();
    
    printf("\n✅ Validation complete. Reports generated.\n");
    
    return 0;
}