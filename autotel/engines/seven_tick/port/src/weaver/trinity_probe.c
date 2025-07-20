/**
 * @file trinity_probe.c
 * @brief Trinity Probe - Deterministic Interface to CNS v8 Substrate
 * 
 * Purpose: To measure invariance, one must have a perfectly controlled way to interact 
 * with the system under test. The probe provides cycle-level precision and controlled
 * perturbation capabilities.
 */

#include "cns/cns_weaver.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>

// ============================================================================
// Operation Registry
// ============================================================================

#define MAX_REGISTERED_OPERATIONS 256

static cns_operation_registry_t operation_registry[MAX_REGISTERED_OPERATIONS];
static uint32_t registered_operation_count = 0;

// ============================================================================
// Trinity Probe Implementation
// ============================================================================

bool cns_weaver_register_operation(uint32_t id, const char* name, 
                                  cns_operation_fn_t function) {
    if (registered_operation_count >= MAX_REGISTERED_OPERATIONS) {
        return false;
    }
    
    // Check for duplicate registration
    for (uint32_t i = 0; i < registered_operation_count; i++) {
        if (operation_registry[i].operation_id == id) {
            return false; // Already registered
        }
    }
    
    // Register the operation
    operation_registry[registered_operation_count].operation_id = id;
    operation_registry[registered_operation_count].name = name;
    operation_registry[registered_operation_count].function = function;
    operation_registry[registered_operation_count].quantum_padding = 0;
    
    registered_operation_count++;
    return true;
}

// Find operation by ID
static cns_operation_registry_t* find_operation(uint32_t id) {
    for (uint32_t i = 0; i < registered_operation_count; i++) {
        if (operation_registry[i].operation_id == id) {
            return &operation_registry[i];
        }
    }
    return NULL;
}

// ============================================================================
// Cycle-Level Precision Control
// ============================================================================

// Introduce controlled delay with cycle-level precision
static void cns_probe_delay_cycles(uint32_t cycles) {
    uint64_t start = __builtin_readcyclecounter();
    uint64_t target = start + cycles;
    
    // Busy wait for precise cycle count
    while (__builtin_readcyclecounter() < target) {
        // Compiler barrier to prevent optimization
        __asm__ volatile("" : : : "memory");
    }
}

// Introduce random jitter within specified range
static void cns_probe_jitter_cycles(uint32_t max_jitter) {
    if (max_jitter == 0) return;
    
    // Simple but effective random jitter
    uint32_t jitter = (rand() % max_jitter) + 1;
    cns_probe_delay_cycles(jitter);
}

// ============================================================================
// Gatekeeper Report Collection
// ============================================================================

// Initialize a Gatekeeper report
static void cns_probe_init_report(cns_gatekeeper_report_t* report) {
    memset(report, 0, sizeof(cns_gatekeeper_report_t));
}

// Update report with operation metrics
static void cns_probe_update_report(cns_gatekeeper_report_t* report,
                                   uint64_t cycles,
                                   uint64_t memory_delta,
                                   uint64_t cache_misses,
                                   uint64_t simd_ops,
                                   uint64_t validations,
                                   uint64_t cognitive_cycles) {
    report->total_operations++;
    report->total_cycles += cycles;
    report->memory_allocated += memory_delta;
    report->cache_misses += cache_misses;
    report->simd_operations += simd_ops;
    report->validation_passes += validations;
    report->cognitive_cycles += cognitive_cycles;
    
    // Update percentiles (simplified implementation)
    if (cycles > report->p99_cycles) {
        report->p99_cycles = cycles;
    }
    if (cycles > report->p95_cycles) {
        report->p95_cycles = cycles;
    }
    if (cycles > report->p50_cycles) {
        report->p50_cycles = cycles;
    }
    
    // Calculate throughput (simplified)
    if (report->total_cycles > 0) {
        report->throughput_mops = (report->total_operations * 3000000) / report->total_cycles; // Assume 3GHz
    }
}

// ============================================================================
// Core Operation Execution
// ============================================================================

bool cns_weaver_execute_operation(cns_weave_op_t* op, 
                                 cns_gatekeeper_report_t* report) {
    assert(op != NULL);
    assert(report != NULL);
    
    // Find the operation in registry
    cns_operation_registry_t* registry_entry = find_operation(op->operation_id);
    if (!registry_entry) {
        printf("ERROR: Operation ID %u not found in registry\n", op->operation_id);
        return false;
    }
    
    // Prepare operation context
    struct {
        uint64_t tick_start;
        uint64_t tick_end;
        uint64_t tick_budget;
        uint64_t simd_vector[8];
        uint64_t operation_hash;
        uint64_t determinism_proof;
    } weaver_op = {0};
    
    weaver_op.tick_budget = 8;
    
    // Execute operation with 8T compliance
    bool success = false;
    uint64_t start_cycles = __builtin_readcyclecounter();
    
    CNS_8T_EXECUTE(&weaver_op, {
        // Call the actual operation function
        success = registry_entry->function(op->context, op->args);
        
        // Update operation hash for determinism proof
        weaver_op.operation_hash = op->operation_id ^ 
                                  (uint64_t)(uintptr_t)op->context ^
                                  op->args[0] ^ op->args[1] ^ op->args[2];
    });
    
    uint64_t end_cycles = __builtin_readcyclecounter();
    uint64_t operation_cycles = end_cycles - start_cycles;
    
    // Update Gatekeeper report
    cns_probe_update_report(report,
                           operation_cycles,
                           64, // Assume 64 bytes per operation (simplified)
                           0,  // Cache misses (would be measured in real implementation)
                           weaver_op.simd_vector[0] > 0 ? 1 : 0, // SIMD operations
                           success ? 1 : 0, // Validation passes
                           1); // Cognitive cycles
    
    // Update Trinity hash in report
    report->trinity_hash ^= weaver_op.operation_hash;
    
    return success;
}

// ============================================================================
// Perturbation Injection
// ============================================================================

// Inject temporal perturbation between operations
void cns_probe_inject_temporal_perturbation(cns_permutation_params_t* params) {
    if (!params || params->type != PERM_TEMPORAL) {
        return;
    }
    
    // Calculate jitter based on intensity
    uint32_t max_jitter = (params->intensity * params->jitter_cycles) / 1000;
    cns_probe_jitter_cycles(max_jitter);
}

// Inject spatial perturbation (memory layout manipulation)
void cns_probe_inject_spatial_perturbation(cns_permutation_params_t* params) {
    if (!params || params->type != PERM_SPATIAL) {
        return;
    }
    
    // Allocate and deallocate memory to fragment the heap
    // This simulates cache fragmentation effects
    for (int i = 0; i < (params->intensity / 100); i++) {
        void* ptr = malloc(64); // 64-byte allocation (cache line size)
        if (ptr) {
            memset(ptr, 0xAA, 64);
            free(ptr);
        }
    }
}

// ============================================================================
// Probe Initialization and Cleanup
// ============================================================================

// Initialize the Trinity Probe
bool cns_probe_init(void) {
    // Initialize random seed for jitter generation
    srand(time(NULL));
    
    // Clear operation registry
    memset(operation_registry, 0, sizeof(operation_registry));
    registered_operation_count = 0;
    
    printf("Trinity Probe initialized with cycle-level precision\n");
    return true;
}

// Cleanup the Trinity Probe
void cns_probe_cleanup(void) {
    printf("Trinity Probe cleanup complete\n");
}

// ============================================================================
// Built-in Test Operations
// ============================================================================

// Test operation: Memory allocation
static bool test_op_allocate_memory(void* context, uint64_t* args) {
    size_t size = (size_t)args[0];
    void** result_ptr = (void**)args[1];
    
    if (result_ptr) {
        *result_ptr = aligned_alloc(8, size);
        return *result_ptr != NULL;
    }
    return false;
}

// Test operation: Memory deallocation
static bool test_op_free_memory(void* context, uint64_t* args) {
    void* ptr = (void*)args[0];
    
    if (ptr) {
        free(ptr);
        return true;
    }
    return false;
}

// Test operation: SIMD computation
static bool test_op_simd_compute(void* context, uint64_t* args) {
    uint64_t* data = (uint64_t*)args[0];
    uint32_t count = (uint32_t)args[1];
    
    // Simple SIMD-like computation
    for (uint32_t i = 0; i < count; i++) {
        data[i] = data[i] * 2 + 1;
    }
    
    return true;
}

// Test operation: Hash computation
static bool test_op_hash_compute(void* context, uint64_t* args) {
    uint64_t input = args[0];
    uint64_t* result = (uint64_t*)args[1];
    
    // Simple hash function
    uint64_t hash = input;
    hash ^= hash >> 33;
    hash *= 0xff51afd7ed558ccdULL;
    hash ^= hash >> 33;
    hash *= 0xc4ceb9fe1a85ec53ULL;
    hash ^= hash >> 33;
    
    if (result) {
        *result = hash;
    }
    
    return true;
}

// Test operation: Validation check
static bool test_op_validate_data(void* context, uint64_t* args) {
    uint64_t data = args[0];
    uint64_t expected = args[1];
    
    return data == expected;
}

// ============================================================================
// Operation Registration
// ============================================================================

// Register built-in test operations
bool cns_probe_register_test_operations(void) {
    bool success = true;
    
    success &= cns_weaver_register_operation(1, "allocate_memory", test_op_allocate_memory);
    success &= cns_weaver_register_operation(2, "free_memory", test_op_free_memory);
    success &= cns_weaver_register_operation(3, "simd_compute", test_op_simd_compute);
    success &= cns_weaver_register_operation(4, "hash_compute", test_op_hash_compute);
    success &= cns_weaver_register_operation(5, "validate_data", test_op_validate_data);
    
    if (success) {
        printf("Registered %u test operations with Trinity Probe\n", registered_operation_count);
    } else {
        printf("ERROR: Failed to register some test operations\n");
    }
    
    return success;
}

// ============================================================================
// Probe Self-Test
// ============================================================================

// Run self-test to validate probe functionality
bool cns_probe_self_test(void) {
    printf("Running Trinity Probe self-test...\n");
    
    // Initialize probe
    if (!cns_probe_init()) {
        printf("FAILED: Probe initialization\n");
        return false;
    }
    
    // Register test operations
    if (!cns_probe_register_test_operations()) {
        printf("FAILED: Operation registration\n");
        return false;
    }
    
    // Test operation execution
    cns_gatekeeper_report_t report;
    cns_probe_init_report(&report);
    
    // Test memory allocation
    void* test_ptr = NULL;
    cns_weave_op_t alloc_op = CNS_WEAVE_OP(1, NULL, 64, (uint64_t)&test_ptr);
    
    if (!cns_weaver_execute_operation(&alloc_op, &report)) {
        printf("FAILED: Memory allocation operation\n");
        return false;
    }
    
    if (!test_ptr) {
        printf("FAILED: Memory allocation result\n");
        return false;
    }
    
    // Test hash computation
    uint64_t hash_result = 0;
    cns_weave_op_t hash_op = CNS_WEAVE_OP(4, NULL, 0x123456789ABCDEFULL, (uint64_t)&hash_result);
    
    if (!cns_weaver_execute_operation(&hash_op, &report)) {
        printf("FAILED: Hash computation operation\n");
        return false;
    }
    
    if (hash_result == 0) {
        printf("FAILED: Hash computation result\n");
        return false;
    }
    
    // Test memory deallocation
    cns_weave_op_t free_op = CNS_WEAVE_OP(2, NULL, (uint64_t)test_ptr);
    
    if (!cns_weaver_execute_operation(&free_op, &report)) {
        printf("FAILED: Memory deallocation operation\n");
        return false;
    }
    
    // Validate report
    if (report.total_operations != 3) {
        printf("FAILED: Report operation count (%lu != 3)\n", report.total_operations);
        return false;
    }
    
    if (report.total_cycles == 0) {
        printf("FAILED: Report cycle count is zero\n");
        return false;
    }
    
    printf("PASSED: Trinity Probe self-test\n");
    printf("  Operations executed: %lu\n", report.total_operations);
    printf("  Total cycles: %lu\n", report.total_cycles);
    printf("  Trinity hash: 0x%016lX\n", report.trinity_hash);
    
    cns_probe_cleanup();
    return true;
} 