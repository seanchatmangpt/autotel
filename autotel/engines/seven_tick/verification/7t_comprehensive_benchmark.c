#include "7t_benchmark_framework.h"
#include "../runtime/src/seven_t_runtime.h"
#include "../compiler/src/cjinja.h"
#include <string.h>

// Comprehensive benchmark context
typedef struct {
    EngineState* engine;
    CJinjaEngine* cjinja_engine;
    CJinjaContext* cjinja_ctx;
    uint32_t test_data[100];
    uint32_t test_strings[50];
} ComprehensiveBenchmarkContext;

// SPARQL benchmark operations
void benchmark_sparql_pattern_match(void* context) {
    ComprehensiveBenchmarkContext* ctx = (ComprehensiveBenchmarkContext*)context;
    s7t_ask_pattern(ctx->engine, ctx->test_data[0], ctx->test_data[1], ctx->test_data[2]);
}

void benchmark_sparql_bit_vector(void* context) {
    ComprehensiveBenchmarkContext* ctx = (ComprehensiveBenchmarkContext*)context;
    BitVector* result = s7t_get_subject_vector(ctx->engine, ctx->test_data[1], ctx->test_data[2]);
    if (result) {
        bitvec_destroy(result);
    }
}

// SHACL benchmark operations
void benchmark_shacl_class_check(void* context) {
    ComprehensiveBenchmarkContext* ctx = (ComprehensiveBenchmarkContext*)context;
    shacl_check_class(ctx->engine, ctx->test_data[0], ctx->test_data[3]);
}

void benchmark_shacl_property_check(void* context) {
    ComprehensiveBenchmarkContext* ctx = (ComprehensiveBenchmarkContext*)context;
    shacl_check_min_count(ctx->engine, ctx->test_data[0], ctx->test_data[4], 1);
}

// CJinja benchmark operations
void benchmark_cjinja_variable_substitution(void* context) {
    ComprehensiveBenchmarkContext* ctx = (ComprehensiveBenchmarkContext*)context;
    char* result = cjinja_render_string("Hello {{ name }}!", ctx->cjinja_ctx);
    if (result) {
        free(result);
    }
}

void benchmark_cjinja_conditional(void* context) {
    ComprehensiveBenchmarkContext* ctx = (ComprehensiveBenchmarkContext*)context;
    char* result = cjinja_render_string("{% if enabled %}Yes{% else %}No{% endif %}", ctx->cjinja_ctx);
    if (result) {
        free(result);
    }
}

// Memory benchmark operations
void benchmark_memory_allocation(void* context) {
    ComprehensiveBenchmarkContext* ctx = (ComprehensiveBenchmarkContext*)context;
    void* ptr = malloc(64);
    if (ptr) {
        memset(ptr, 0x42, 64);
        free(ptr);
    }
}

void benchmark_cache_access(void* context) {
    ComprehensiveBenchmarkContext* ctx = (ComprehensiveBenchmarkContext*)context;
    // Simulate cache-friendly access pattern
    for (int i = 0; i < 64; i++) {
        ctx->test_data[i] = i;
    }
}

// Setup comprehensive test data
ComprehensiveBenchmarkContext* setup_comprehensive_test_data(void) {
    ComprehensiveBenchmarkContext* ctx = malloc(sizeof(ComprehensiveBenchmarkContext));
    if (!ctx) {
        printf("❌ Failed to allocate benchmark context\n");
        return NULL;
    }
    
    // Setup 7T engine
    ctx->engine = s7t_create_engine();
    if (!ctx->engine) {
        printf("❌ Failed to create 7T engine\n");
        free(ctx);
        return NULL;
    }
    
    // Setup CJinja engine
    ctx->cjinja_engine = cjinja_create(NULL);
    if (!ctx->cjinja_engine) {
        printf("❌ Failed to create CJinja engine\n");
        s7t_destroy_engine(ctx->engine);
        free(ctx);
        return NULL;
    }
    
    // Setup CJinja context
    ctx->cjinja_ctx = cjinja_create_context();
    if (!ctx->cjinja_ctx) {
        printf("❌ Failed to create CJinja context\n");
        cjinja_destroy(ctx->cjinja_engine);
        s7t_destroy_engine(ctx->engine);
        free(ctx);
        return NULL;
    }
    
    // Add CJinja variables
    cjinja_set_var(ctx->cjinja_ctx, "name", "7T Engine");
    cjinja_set_bool(ctx->cjinja_ctx, "enabled", 1);
    char* items[] = {"item1", "item2", "item3"};
    cjinja_set_array(ctx->cjinja_ctx, "items", items, 3);
    
    // Setup test data for SPARQL/SHACL
    ctx->test_strings[0] = s7t_intern_string(ctx->engine, "ex:alice");
    ctx->test_strings[1] = s7t_intern_string(ctx->engine, "ex:bob");
    ctx->test_strings[2] = s7t_intern_string(ctx->engine, "ex:charlie");
    ctx->test_strings[3] = s7t_intern_string(ctx->engine, "ex:knows");
    ctx->test_strings[4] = s7t_intern_string(ctx->engine, "ex:likes");
    ctx->test_strings[5] = s7t_intern_string(ctx->engine, "ex:Person");
    ctx->test_strings[6] = s7t_intern_string(ctx->engine, "ex:hasName");
    ctx->test_strings[7] = s7t_intern_string(ctx->engine, "ex:hasEmail");
    
    // Add test triples
    s7t_add_triple(ctx->engine, ctx->test_strings[0], ctx->test_strings[3], ctx->test_strings[2]);
    s7t_add_triple(ctx->engine, ctx->test_strings[0], ctx->test_strings[4], ctx->test_strings[1]);
    s7t_add_triple(ctx->engine, ctx->test_strings[0], 0, ctx->test_strings[5]);
    s7t_add_triple(ctx->engine, ctx->test_strings[0], ctx->test_strings[6], ctx->test_strings[1]);
    s7t_add_triple(ctx->engine, ctx->test_strings[0], ctx->test_strings[7], ctx->test_strings[2]);
    
    // Setup test data array
    ctx->test_data[0] = ctx->test_strings[0]; // alice
    ctx->test_data[1] = ctx->test_strings[3]; // knows
    ctx->test_data[2] = ctx->test_strings[2]; // charlie
    ctx->test_data[3] = ctx->test_strings[5]; // Person
    ctx->test_data[4] = ctx->test_strings[6]; // hasName
    
    return ctx;
}

void cleanup_comprehensive_test_data(ComprehensiveBenchmarkContext* ctx) {
    if (ctx) {
        if (ctx->cjinja_ctx) {
            cjinja_destroy_context(ctx->cjinja_ctx);
        }
        if (ctx->cjinja_engine) {
            cjinja_destroy(ctx->cjinja_engine);
        }
        if (ctx->engine) {
            s7t_destroy_engine(ctx->engine);
        }
        free(ctx);
    }
}

// Validate correctness
bool validate_comprehensive_correctness(ComprehensiveBenchmarkContext* ctx) {
    // Test SPARQL
    bool sparql_test = s7t_ask_pattern(ctx->engine, ctx->test_strings[0], ctx->test_strings[3], ctx->test_strings[2]);
    
    // Test SHACL
    bool shacl_test = shacl_check_class(ctx->engine, ctx->test_strings[0], ctx->test_strings[5]);
    
    // Test CJinja
    char* cjinja_result = cjinja_render_string("{{ name }}", ctx->cjinja_ctx);
    bool cjinja_test = cjinja_result && strcmp(cjinja_result, "7T Engine") == 0;
    if (cjinja_result) {
        free(cjinja_result);
    }
    
    return sparql_test && shacl_test && cjinja_test;
}

// Main comprehensive benchmark runner
void run_comprehensive_benchmarks(void) {
    printf("🚀 7T Engine Comprehensive Benchmark Suite\n");
    printf("==========================================\n\n");
    
    // Create benchmark suite
    BenchmarkSuite* suite = create_benchmark_suite("7T Engine Comprehensive");
    if (!suite) {
        printf("❌ Failed to create benchmark suite\n");
        return;
    }
    
    // Setup test data
    ComprehensiveBenchmarkContext* ctx = setup_comprehensive_test_data();
    if (!ctx) {
        destroy_benchmark_suite(suite);
        return;
    }
    
    printf("✅ Test data loaded (%zu triples)\n\n", ctx->engine->triple_count);
    
    // Validate correctness first
    if (!validate_comprehensive_correctness(ctx)) {
        printf("❌ Comprehensive correctness validation failed!\n");
        cleanup_comprehensive_test_data(ctx);
        destroy_benchmark_suite(suite);
        return;
    }
    printf("✅ Correctness validation passed\n\n");
    
    // Get benchmark configuration
    BenchmarkConfig config = get_default_config();
    config.verbose = true;
    
    // Run SPARQL benchmarks
    printf("🔍 Running SPARQL benchmarks...\n");
    BenchmarkResult result;
    
    result = run_benchmark("Pattern Matching", "SPARQL", &config, 
                          benchmark_sparql_pattern_match, ctx);
    add_benchmark_result(suite, result);
    
    result = run_benchmark("Bit Vector Operations", "SPARQL", &config, 
                          benchmark_sparql_bit_vector, ctx);
    add_benchmark_result(suite, result);
    
    // Run SHACL benchmarks
    printf("🔍 Running SHACL benchmarks...\n");
    
    result = run_benchmark("Class Membership Check", "SHACL", &config, 
                          benchmark_shacl_class_check, ctx);
    add_benchmark_result(suite, result);
    
    result = run_benchmark("Property Existence Check", "SHACL", &config, 
                          benchmark_shacl_property_check, ctx);
    add_benchmark_result(suite, result);
    
    // Run CJinja benchmarks
    printf("🔍 Running CJinja benchmarks...\n");
    
    result = run_benchmark("Variable Substitution", "CJinja", &config, 
                          benchmark_cjinja_variable_substitution, ctx);
    add_benchmark_result(suite, result);
    
    result = run_benchmark("Conditional Rendering", "CJinja", &config, 
                          benchmark_cjinja_conditional, ctx);
    add_benchmark_result(suite, result);
    
    // Run Memory benchmarks
    printf("🔍 Running Memory benchmarks...\n");
    
    result = run_benchmark("Memory Allocation", "Memory", &config, 
                          benchmark_memory_allocation, ctx);
    add_benchmark_result(suite, result);
    
    result = run_benchmark("Cache Access", "Memory", &config, 
                          benchmark_cache_access, ctx);
    add_benchmark_result(suite, result);
    
    // Print comprehensive results
    print_benchmark_suite(suite);
    
    // Export results
    export_benchmark_results_csv(suite, "7t_comprehensive_benchmark_results.csv");
    export_benchmark_results_json(suite, "7t_comprehensive_benchmark_results.json");
    
    printf("📊 Results exported to:\n");
    printf("  - 7t_comprehensive_benchmark_results.csv\n");
    printf("  - 7t_comprehensive_benchmark_results.json\n\n");
    
    // Performance summary
    printf("🎯 Performance Summary:\n");
    printf("======================\n");
    
    int seven_tick_count = 0;
    int total_count = suite->result_count;
    
    for (int i = 0; i < suite->result_count; i++) {
        if (suite->results[i].target_achieved) {
            seven_tick_count++;
            printf("  🎉 %s (%s): %.1f ns - 7-TICK ACHIEVED!\n", 
                   suite->results[i].operation_name, 
                   suite->results[i].component_name,
                   suite->results[i].avg_time_ns);
        } else {
            printf("  ⚠️ %s (%s): %.1f ns - %s\n", 
                   suite->results[i].operation_name, 
                   suite->results[i].component_name,
                   suite->results[i].avg_time_ns,
                   suite->results[i].performance_tier);
        }
    }
    
    printf("\n🎉 Overall: %d/%d operations achieved 7-tick performance (%.1f%%)\n", 
           seven_tick_count, total_count, (double)seven_tick_count / total_count * 100);
    
    // Cleanup
    cleanup_comprehensive_test_data(ctx);
    destroy_benchmark_suite(suite);
    
    printf("\n✅ 7T Engine Comprehensive Benchmark Suite Complete\n");
    printf("==================================================\n");
}

int main() {
    run_comprehensive_benchmarks();
    return 0;
}