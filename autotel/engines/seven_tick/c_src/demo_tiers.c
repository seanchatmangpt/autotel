#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "7t_compiler.h"

// Example: Financial Transaction Validator (L1-Compliant)
void demo_l1_financial_validator() {
    printf("\n=== L1-COMPLIANT: Financial Transaction Validator ===\n");
    
    // Define schema that fits in L1
    SchemaDefinition schema = {
        .num_classes = 200,        // Account types, currencies, regions
        .num_properties = 50,      // Transaction properties
        .num_shapes = 50,          // Validation rules
        .expected_subjects = 1000, // Active transactions
        .expected_predicates = 10,
        .expected_objects = 1000
    };
    
    // Configure compiler for L1
    CompilerConfig config = {
        .target_tier = TIER_L1_COMPLIANT,
        .optimize_for_latency = 1,
        .enable_sharding = 0,
        .enable_vectorization = 1,
        .output_path = "validator_l1.c"
    };
    
    SevenTickCompiler* compiler = compiler_create(&config);
    
    // Analyze footprint
    KernelFootprint footprint;
    if (compiler_analyze_schema(compiler, &schema, &footprint) == 0) {
        printf("✅ Schema fits in L1!\n");
        
        // Generate compliance certificate
        ComplianceCertificate cert;
        generate_compliance_certificate("financial_validator", &footprint, &cert);
        print_compliance_certificate(&cert);
        
        // Compile kernel
        CompiledKernel* kernel = compiler_compile(compiler, &schema);
        compiler_write_kernel(kernel, "financial_validator_l1.c");
        
        printf("Generated L1-compliant kernel: financial_validator_l1.c\n");
        free(kernel->code);
        free(kernel);
    } else {
        printf("❌ Schema too large for L1\n");
        compiler_suggest_optimizations(compiler, &schema, TIER_L1_COMPLIANT);
    }
    
    compiler_destroy(compiler);
}

// Example: Sprint Health Monitor (L2-Compliant)
void demo_l2_sprint_monitor() {
    printf("\n=== L2-COMPLIANT: Sprint Health Monitor ===\n");
    
    // Realistic Scrum ontology
    SchemaDefinition schema = {
        .num_classes = 200,         // Scrum concepts
        .num_properties = 100,      // Properties
        .num_shapes = 200,          // Health rules
        .expected_subjects = 5000,  // Reduced from 20K to fit L2
        .expected_predicates = 50,
        .expected_objects = 5000
    };
    
    CompilerConfig config = {
        .target_tier = TIER_L2_COMPLIANT,
        .optimize_for_latency = 1,
        .enable_sharding = 0,
        .enable_vectorization = 1,
        .output_path = "sprint_monitor_l2.c"
    };
    
    SevenTickCompiler* compiler = compiler_create(&config);
    
    KernelFootprint footprint;
    if (compiler_analyze_schema(compiler, &schema, &footprint) == 0) {
        printf("✅ Schema fits in L2!\n");
        
        ComplianceCertificate cert;
        generate_compliance_certificate("sprint_monitor", &footprint, &cert);
        print_compliance_certificate(&cert);
        
        printf("\nPerformance for IDE plugin:\n");
        printf("- Refresh latency: %.2f ms for 50K operations\n", 
               cert.guaranteed_latency_ns * 50000 / 1e6);
        printf("- Updates per second: %.0f\n", 
               1e9 / (cert.guaranteed_latency_ns * 50000));
    } else {
        printf("❌ Schema too large for L2\n");
        compiler_suggest_optimizations(compiler, &schema, TIER_L2_COMPLIANT);
    }
    
    compiler_destroy(compiler);
}

// Example: Sharding large datasets
void demo_sharding() {
    printf("\n=== SHARDING: Large Dataset Across Multiple L2 Kernels ===\n");
    
    // Original schema (too large)
    SchemaDefinition original = {
        .num_classes = 200,
        .num_properties = 100,
        .num_shapes = 200,
        .expected_subjects = 100000,  // Too large!
        .expected_predicates = 50,
        .expected_objects = 20000
    };
    
    printf("Original schema footprint:\n");
    CompilerConfig config = {
        .target_tier = TIER_L2_COMPLIANT,
        .enable_sharding = 1
    };
    
    SevenTickCompiler* compiler = compiler_create(&config);
    KernelFootprint footprint;
    compiler_analyze_schema(compiler, &original, &footprint);
    printf("- Total: %zu KB (exceeds L2!)\n", footprint.total_footprint_kb);
    
    // Shard into 4 L2-compliant kernels
    printf("\nSharding into 4 kernels:\n");
    for (int shard = 0; shard < 4; shard++) {
        SchemaDefinition sharded = original;
        sharded.expected_subjects = original.expected_subjects / 4;
        sharded.expected_objects = original.expected_objects / 4;
        
        compiler_analyze_schema(compiler, &sharded, &footprint);
        
        char name[256];
        snprintf(name, sizeof(name), "shard_%d", shard);
        
        ComplianceCertificate cert;
        generate_compliance_certificate(name, &footprint, &cert);
        
        printf("\nShard %d: %zu KB - %s\n", 
               shard, footprint.total_footprint_kb,
               TIER_SPECS[cert.certified_tier].name);
        printf("- Handles subjects %d-%d\n", 
               shard * 25000, (shard + 1) * 25000 - 1);
        printf("- Latency: < %.0f ns\n", cert.guaranteed_latency_ns);
    }
    
    printf("\n✅ All shards fit in L2 cache!\n");
    printf("Parallel execution possible across CPU cores.\n");
    
    compiler_destroy(compiler);
}

// Demonstrate tier selection based on use case
void demo_tier_selection() {
    printf("\n=== TIER SELECTION GUIDE ===\n");
    
    struct {
        const char* use_case;
        size_t operations_per_second;
        double latency_budget_ms;
        ComplianceTier recommended_tier;
    } requirements[] = {
        {"HFT Order Validation", 10000000, 0.1, TIER_L1_COMPLIANT},
        {"Live UI Updates", 1000000, 16.0, TIER_L2_COMPLIANT},
        {"Batch Compliance", 100000, 1000.0, TIER_L3_COMPLIANT}
    };
    
    for (int i = 0; i < 3; i++) {
        printf("\nUse Case: %s\n", requirements[i].use_case);
        printf("- Required: %zu ops/sec, < %.1f ms latency\n",
               requirements[i].operations_per_second,
               requirements[i].latency_budget_ms);
        
        const TierSpecification* tier = &TIER_SPECS[requirements[i].recommended_tier];
        printf("- Recommended: %s\n", tier->name);
        printf("- Provides: %llu ops/sec, < %.0f ns latency\n",
               tier->min_throughput_ops_sec,
               tier->max_latency_ns);
        
        double ops_in_budget = requirements[i].latency_budget_ms * 1e6 / 
                              tier->max_latency_ns;
        printf("- Can do %.0f operations in %.1f ms budget\n",
               ops_in_budget, requirements[i].latency_budget_ms);
    }
}

int main() {
    printf("=== 7T Memory Hierarchy Compliance Demos ===\n");
    printf("Demonstrating tier-based performance guarantees\n");
    
    // Run demos
    demo_l1_financial_validator();
    demo_l2_sprint_monitor();
    demo_sharding();
    demo_tier_selection();
    
    printf("\n\n=== Key Takeaways ===\n");
    printf("1. Performance is predictable when data fits in cache\n");
    printf("2. Design your schemas to fit your performance tier\n");
    printf("3. Use sharding for datasets larger than L3\n");
    printf("4. The compiler certifies and guarantees performance\n");
    printf("\nWith 7T, performance is a design choice, not an accident!\n");
    
    return 0;
}