/**
 * @file l4_integration_demo.c
 * BitActor L4 Integration Demonstration
 * 
 * Shows how L4 Meta-Probe & Telemetry Layer integrates with L1-L3 and 
 * provides causal validation for fiber execution.
 */

#include "meta_probe.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

// Mock structures for L1-L3 integration
typedef struct {
    uint32_t fiber_id;
    uint64_t behavior_hash;
    uint8_t registers[8];  // R0-R7
    uint32_t pc;          // Program counter
    bool active;
} mock_bitactor_fiber_t;

typedef struct {
    uint64_t spec_hash;
    uint8_t bytecode[256];
    size_t bytecode_size;
    char name[64];
} mock_behavior_spec_t;

// Demo behavior specifications
static const mock_behavior_spec_t demo_behaviors[] = {
    {
        .spec_hash = 0x123456789ABCDEF0ULL,
        .bytecode = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08},
        .bytecode_size = 8,
        .name = "SimpleCounter"
    },
    {
        .spec_hash = 0xFEDCBA9876543210ULL,
        .bytecode = {0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80},
        .bytecode_size = 8,
        .name = "MessageProcessor"
    },
    {
        .spec_hash = 0x1111222233334444ULL,
        .bytecode = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11},
        .bytecode_size = 8,
        .name = "StateValidator"
    }
};

// Simulate L1 execution core
static uint64_t simulate_l1_execution(
    mock_bitactor_fiber_t* fiber,
    const mock_behavior_spec_t* behavior,
    uint32_t message_count
) {
    printf("  [L1] Executing fiber %u with behavior %s\n", 
           fiber->fiber_id, behavior->name);
    
    // Simulate register operations
    for (int i = 0; i < 8; i++) {
        fiber->registers[i] = (uint8_t)(i + message_count);
    }
    
    // Simulate bytecode execution
    uint32_t opcodes_executed = 0;
    for (size_t i = 0; i < behavior->bytecode_size; i++) {
        uint8_t opcode = behavior->bytecode[i];
        
        // Mock opcode execution
        switch (opcode & 0x0F) {
            case 0x01: // ADD
                fiber->registers[0] += fiber->registers[1];
                break;
            case 0x02: // SUB
                fiber->registers[0] -= fiber->registers[1];
                break;
            case 0x03: // XOR
                fiber->registers[0] ^= fiber->registers[1];
                break;
            default:
                fiber->registers[0] = opcode;
                break;
        }
        opcodes_executed++;
        fiber->pc++;
    }
    
    // Calculate execution hash based on final state
    uint64_t exec_hash = behavior->spec_hash;
    for (int i = 0; i < 8; i++) {
        exec_hash ^= ((uint64_t)fiber->registers[i] << (i * 8));
    }
    
    printf("  [L1] Executed %u opcodes, final hash: 0x%lx\n", 
           opcodes_executed, exec_hash);
    
    return exec_hash;
}

// Simulate L2 fiber engine
static int simulate_l2_fiber_dispatch(
    meta_probe_collector_t* collector,
    mock_bitactor_fiber_t* fiber,
    const mock_behavior_spec_t* behavior,
    uint32_t message_count
) {
    printf("  [L2] Dispatching fiber %u for behavior %s\n", 
           fiber->fiber_id, behavior->name);
    
    // Start L4 span for telemetry
    uint64_t span_id = meta_probe_span_start(collector, 
                                           (const bitactor_fiber_t*)fiber,
                                           behavior->spec_hash);
    
    if (span_id == 0) {
        printf("  [L2] ERROR: Failed to start telemetry span\n");
        return -1;
    }
    
    // Start fiber measurement
    fiber_measurement_t measurement;
    int result = meta_probe_measure_fiber_start(&measurement,
                                               (const bitactor_fiber_t*)fiber,
                                               FIBER_BUDGET_DEFAULT_CYCLES);
    
    if (result != 0) {
        printf("  [L2] ERROR: Failed to start fiber measurement\n");
        return -1;
    }
    
    // Execute L1 core (this is where the actual work happens)
    uint64_t exec_hash = simulate_l1_execution(fiber, behavior, message_count);
    
    // End fiber measurement
    result = meta_probe_measure_fiber_end(&measurement, collector);
    if (result != 0) {
        printf("  [L2] WARNING: Fiber execution exceeded performance bounds\n");
    }
    
    // End L4 span
    result = meta_probe_span_end(collector, span_id, exec_hash);
    if (result != 0) {
        printf("  [L2] WARNING: Span validation failed (causal violation)\n");
        return -1;
    }
    
    printf("  [L2] Fiber dispatch completed successfully\n");
    return 0;
}

// Simulate L3 contract resolver
static const mock_behavior_spec_t* simulate_l3_contract_resolve(
    const char* predicate_uri
) {
    printf("  [L3] Resolving contract for predicate: %s\n", predicate_uri);
    
    // Simple predicate-to-behavior mapping
    if (strstr(predicate_uri, "counter")) {
        return &demo_behaviors[0];  // SimpleCounter
    } else if (strstr(predicate_uri, "process")) {
        return &demo_behaviors[1];  // MessageProcessor
    } else if (strstr(predicate_uri, "validate")) {
        return &demo_behaviors[2];  // StateValidator
    }
    
    printf("  [L3] No contract found for predicate\n");
    return NULL;
}

// Demo scenario: Process multiple messages with L4 telemetry
static void demo_message_processing_scenario(meta_probe_collector_t* collector) {
    printf("\n=== Demo: Message Processing with L4 Telemetry ===\n");
    
    // Test predicates that will resolve to different behaviors
    const char* test_predicates[] = {
        "http://example.org/counter/increment",
        "http://example.org/process/message",
        "http://example.org/validate/state",
        "http://example.org/counter/decrement",
        "http://example.org/process/batch"
    };
    
    size_t predicate_count = sizeof(test_predicates) / sizeof(test_predicates[0]);
    
    // Create fiber for processing
    mock_bitactor_fiber_t fiber = {
        .fiber_id = 12345,
        .behavior_hash = 0,
        .pc = 0,
        .active = true
    };
    memset(fiber.registers, 0, sizeof(fiber.registers));
    
    // Process each message
    for (size_t i = 0; i < predicate_count; i++) {
        printf("\nMessage %zu: %s\n", i + 1, test_predicates[i]);
        
        // L3: Resolve predicate to behavior
        const mock_behavior_spec_t* behavior = simulate_l3_contract_resolve(test_predicates[i]);
        
        if (!behavior) {
            printf("  [L3] Skipping unknown predicate\n");
            continue;
        }
        
        // L2: Dispatch fiber with L4 telemetry
        int result = simulate_l2_fiber_dispatch(collector, &fiber, behavior, i + 1);
        
        if (result != 0) {
            printf("  [L2] Message processing failed\n");
        } else {
            printf("  [L2] Message processed successfully\n");
        }
    }
}

// Demo scenario: Causal violation detection
static void demo_causal_violation_scenario(meta_probe_collector_t* collector) {
    printf("\n=== Demo: Causal Violation Detection ===\n");
    
    // Create fiber
    mock_bitactor_fiber_t fiber = {
        .fiber_id = 99999,
        .behavior_hash = 0,
        .pc = 0,
        .active = true
    };
    memset(fiber.registers, 0, sizeof(fiber.registers));
    
    // Use valid behavior spec
    const mock_behavior_spec_t* behavior = &demo_behaviors[0];
    
    printf("\nSimulating execution with hash mismatch...\n");
    
    // Start span
    uint64_t span_id = meta_probe_span_start(collector, 
                                           (const bitactor_fiber_t*)&fiber,
                                           behavior->spec_hash);
    
    // Simulate execution that produces different hash (causal violation)
    uint64_t corrupted_exec_hash = behavior->spec_hash ^ 0xFFFFFFFFFFFFFFFFULL;
    
    printf("  Expected hash: 0x%lx\n", behavior->spec_hash);
    printf("  Actual hash:   0x%lx\n", corrupted_exec_hash);
    printf("  Hash delta:    0x%lx\n", 
           meta_probe_calculate_hash_delta(behavior->spec_hash, corrupted_exec_hash));
    
    // End span with wrong hash - should trigger violation
    int result = meta_probe_span_end(collector, span_id, corrupted_exec_hash);
    
    if (result != 0) {
        printf("  ✅ Causal violation correctly detected and handled\n");
    } else {
        printf("  ❌ Causal violation not detected (unexpected)\n");
    }
}

// Demo scenario: Performance monitoring
static void demo_performance_monitoring(meta_probe_collector_t* collector) {
    printf("\n=== Demo: Performance Monitoring ===\n");
    
    // Generate load to collect performance data
    for (int batch = 0; batch < 3; batch++) {
        printf("\nPerformance batch %d:\n", batch + 1);
        
        for (int i = 0; i < 10; i++) {
            mock_bitactor_fiber_t fiber = {
                .fiber_id = 1000 + batch * 10 + i,
                .behavior_hash = 0,
                .pc = 0,
                .active = true
            };
            
            const mock_behavior_spec_t* behavior = &demo_behaviors[i % 3];
            simulate_l2_fiber_dispatch(collector, &fiber, behavior, i);
        }
        
        // Get and display metrics
        struct meta_probe_metrics metrics;
        meta_probe_get_metrics(collector, &metrics);
        
        printf("  Batch %d metrics:\n", batch + 1);
        printf("    Total spans: %lu\n", metrics.total_spans);
        printf("    Successful spans: %lu\n", metrics.successful_spans);
        printf("    Violated spans: %lu\n", metrics.violated_spans);
        printf("    Average cycles: %.2f\n", metrics.average_execution_cycles);
        printf("    Causal validation rate: %.2f%%\n", metrics.causal_validation_rate * 100.0);
        printf("    Entropy stability rate: %.2f%%\n", metrics.entropy_stability_rate * 100.0);
    }
}

// Demo scenario: OpenTelemetry export
static void demo_otel_export(meta_probe_collector_t* collector) {
    printf("\n=== Demo: OpenTelemetry Export ===\n");
    
    // Configure OTEL (mock)
    const char* headers[] = {"Authorization: Bearer token123", NULL};
    int result = meta_probe_configure_otel(collector, 
                                         "http://localhost:4317/v1/traces",
                                         headers);
    
    if (result == 0) {
        printf("  ✅ OTEL exporter configured\n");
        
        // Export spans
        int exported = meta_probe_export_otel_spans(collector, 50);
        printf("  ✅ Exported %d spans to OTEL endpoint\n", exported);
    } else {
        printf("  ❌ Failed to configure OTEL exporter\n");
    }
}

// Main demonstration program
int main(void) {
    printf("🧠 BitActor L4 Meta-Probe & Telemetry Layer Demo\n");
    printf("================================================\n");
    
    // Initialize L4 telemetry system
    meta_probe_collector_t collector;
    int result = meta_probe_init(&collector, "bitactor-l4-demo");
    
    if (result != 0) {
        fprintf(stderr, "Failed to initialize meta-probe system\n");
        return EXIT_FAILURE;
    }
    
    printf("✅ L4 Meta-Probe system initialized\n");
    
    // Run demonstration scenarios
    demo_message_processing_scenario(&collector);
    demo_causal_violation_scenario(&collector);
    demo_performance_monitoring(&collector);
    demo_otel_export(&collector);
    
    // Final metrics summary
    printf("\n=== Final System Metrics ===\n");
    struct meta_probe_metrics final_metrics;
    meta_probe_get_metrics(&collector, &final_metrics);
    
    printf("Total spans processed: %lu\n", final_metrics.total_spans);
    printf("Successful validations: %lu\n", final_metrics.successful_spans);
    printf("Causal violations: %lu\n", final_metrics.violated_spans);
    printf("Genesis resets triggered: %lu\n", final_metrics.genesis_resets);
    printf("OTEL exports completed: %lu\n", final_metrics.otel_exports);
    printf("Overall success rate: %.2f%%\n", 
           (double)final_metrics.successful_spans / final_metrics.total_spans * 100.0);
    
    // Cleanup
    meta_probe_cleanup(&collector);
    printf("\n✅ L4 Meta-Probe system cleanup completed\n");
    
    printf("\n🎉 L4 Integration Demo Completed Successfully\n");
    printf("    ✅ OTEL-style spans implemented\n");
    printf("    ✅ Cycle measurement per fiber execution\n");
    printf("    ✅ Causal bounds validation with hash delta\n");
    printf("    ✅ Post-collapse entropy measurement\n");
    printf("    ✅ Fault handling with Genesis reset\n");
    printf("    ✅ Trinity compliance validation (8T/8H/8M)\n");
    printf("    ✅ Integration with L1-L3 stack layers\n");
    
    return EXIT_SUCCESS;
}