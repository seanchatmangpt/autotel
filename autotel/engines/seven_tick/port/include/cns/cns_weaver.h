/**
 * @file cns_weaver.h
 * @brief CNS Permutation Weaver (PW7) - Physics Engine for Invariance Proof
 * 
 * Core Axiom: The testing framework must adhere to the same physical laws as the system it tests.
 * Purpose: Prove that the Trinity's logical output is invariant under permutations of its physical execution.
 */

#ifndef CNS_WEAVER_H
#define CNS_WEAVER_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

// ============================================================================
// 8M-Compliant Core Primitives
// ============================================================================

// 8-byte quantum alignment for all structures
#define CNS_CACHE_ALIGN __attribute__((aligned(8)))
#define CNS_CACHE_LINE_ALIGN __attribute__((aligned(64)))

// 8T-compliant operation wrapper
#define CNS_8T_EXECUTE(op, code) \
    do { \
        (op)->tick_start = __builtin_readcyclecounter(); \
        code \
        (op)->tick_end = __builtin_readcyclecounter(); \
        assert((op)->tick_end - (op)->tick_start <= 8); \
    } while(0)

// ============================================================================
// Weaver Core Data Structures
// ============================================================================

// A sequence of operations to be executed
typedef struct CNS_CACHE_ALIGN {
    uint32_t operation_id;     // Maps to a function in the Trinity
    void* context;             // 8M-aligned context pointer
    uint64_t args[6];          // 8B-aligned arguments (48 bytes total)
    uint64_t quantum_padding;  // Ensures 8M compliance
} cns_weave_op_t;

// Permutation strategy definition
typedef enum {
    PERM_TEMPORAL = 0,     // Timing jitter and delays
    PERM_OPERATIONAL = 1,  // Operation reordering
    PERM_SPATIAL = 2,      // Memory layout manipulation
    PERM_LOGICAL = 3,      // Cognitive step reordering
    PERM_COMPOSITE = 4     // Combined strategies
} cns_permutation_type_t;

// Permutation parameters
typedef struct CNS_CACHE_ALIGN {
    cns_permutation_type_t type;
    uint32_t intensity;         // 0-1000 scale of perturbation strength
    uint32_t jitter_cycles;     // Temporal jitter range (1-100 cycles)
    uint32_t reorder_window;    // Operational reordering window size
    uint64_t spatial_seed;      // Memory layout randomization seed
} cns_permutation_params_t;

// A complete test case definition
typedef struct CNS_CACHE_LINE_ALIGN {
    const char* name;                           // Test case identifier
    cns_weave_op_t* canonical_sequence;        // Reference sequence
    uint32_t op_count;                         // Number of operations
    uint32_t permutations_to_run;              // Number of permutation tests
    cns_permutation_params_t* strategies;      // Permutation strategies to apply
    uint32_t strategy_count;                   // Number of strategies
    uint64_t hypothesis_seed;                  // 8H hypothesis generation seed
    uint64_t quantum_padding[2];               // Cache line alignment
} cns_weave_t;

// Gatekeeper report structure (the oracle)
typedef struct CNS_CACHE_LINE_ALIGN {
    uint64_t total_operations;          // Total operations executed
    uint64_t total_cycles;              // Total CPU cycles consumed
    uint64_t p50_cycles;                // 50th percentile cycle count
    uint64_t p95_cycles;                // 95th percentile cycle count
    uint64_t p99_cycles;                // 99th percentile cycle count
    uint64_t throughput_mops;           // Millions of operations per second
    uint64_t memory_allocated;          // Total memory allocated
    uint64_t cache_misses;              // L1 cache miss count
    uint64_t simd_operations;           // SIMD operations executed
    uint64_t validation_passes;         // SHACL validation passes
    uint64_t validation_failures;       // SHACL validation failures
    uint64_t cognitive_cycles;          // 8H cognitive cycles completed
    uint64_t trinity_hash;              // Final Trinity state hash
    uint64_t quantum_padding[2];        // Cache line alignment
} cns_gatekeeper_report_t;

// Weaver execution context
typedef struct CNS_CACHE_LINE_ALIGN {
    cns_weave_t* test_case;             // Current test case
    cns_gatekeeper_report_t canonical;  // Canonical run report
    cns_gatekeeper_report_t permuted;   // Permuted run report
    uint64_t permutation_count;         // Current permutation number
    uint64_t deviation_score;           // Cumulative deviation metric
    bool invariance_proven;             // Whether invariance is proven
    uint64_t quantum_padding[3];        // Cache line alignment
} cns_weaver_context_t;

// ============================================================================
// Core Weaver API
// ============================================================================

// Initialize the Weaver context
cns_weaver_context_t* cns_weaver_init(cns_weave_t* test_case);

// Execute a canonical run and capture Gatekeeper report
bool cns_weaver_run_canonical(cns_weaver_context_t* ctx);

// Execute a permuted run with specified strategy
bool cns_weaver_run_permuted(cns_weaver_context_t* ctx, 
                            cns_permutation_params_t* strategy);

// Validate invariance by comparing reports
bool cns_weaver_validate_invariance(cns_weaver_context_t* ctx);

// Run the complete permutation test suite
bool cns_weaver_run(cns_weave_t* test_case);

// ============================================================================
// Trinity Probe Interface
// ============================================================================

// Operation function pointer type
typedef bool (*cns_operation_fn_t)(void* context, uint64_t* args);

// Operation registry entry
typedef struct CNS_CACHE_ALIGN {
    uint32_t operation_id;
    const char* name;
    cns_operation_fn_t function;
    uint64_t quantum_padding;
} cns_operation_registry_t;

// Register an operation with the Weaver
bool cns_weaver_register_operation(uint32_t id, const char* name, 
                                  cns_operation_fn_t function);

// Execute a single operation with cycle-level precision
bool cns_weaver_execute_operation(cns_weave_op_t* op, 
                                 cns_gatekeeper_report_t* report);

// ============================================================================
// Permutation Engine Interface
// ============================================================================

// Generate temporal permutation (timing jitter)
bool cns_weaver_permute_temporal(cns_weave_op_t* sequence, uint32_t count,
                                cns_permutation_params_t* params);

// Generate operational permutation (reordering)
bool cns_weaver_permute_operational(cns_weave_op_t* sequence, uint32_t count,
                                   cns_permutation_params_t* params);

// Generate spatial permutation (memory layout)
bool cns_weaver_permute_spatial(cns_weave_op_t* sequence, uint32_t count,
                               cns_permutation_params_t* params);

// Generate composite permutation (combined strategies)
bool cns_weaver_permute_composite(cns_weave_op_t* sequence, uint32_t count,
                                 cns_permutation_params_t* params);

// ============================================================================
// 8H Hypothesis Engine Interface
// ============================================================================

// Hypothesis about system behavior
typedef struct CNS_CACHE_ALIGN {
    uint64_t hypothesis_id;             // Unique hypothesis identifier
    const char* description;            // Human-readable description
    cns_permutation_params_t strategy;  // Generated permutation strategy
    uint64_t confidence_score;          // 0-1000 confidence in hypothesis
    uint64_t quantum_padding;
} cns_hypothesis_t;

// Generate new hypothesis based on observed deviations
cns_hypothesis_t* cns_weaver_generate_hypothesis(cns_weaver_context_t* ctx);

// Validate hypothesis by running targeted permutation
bool cns_weaver_validate_hypothesis(cns_weaver_context_t* ctx, 
                                   cns_hypothesis_t* hypothesis);

// ============================================================================
// Declarative Test Definition Macros
// ============================================================================

// Define a permutation test case
#define CNS_PERMUTATION_DEFINE(name, sequence) \
    cns_weave_t name##_weave = { \
        .name = #name, \
        .canonical_sequence = sequence, \
        .op_count = sizeof(sequence)/sizeof(cns_weave_op_t), \
        .permutations_to_run = 1000, \
        .strategies = NULL, \
        .strategy_count = 0, \
        .hypothesis_seed = 0x123456789ABCDEFULL \
    }

// Define operation with 8M-compliant arguments
#define CNS_WEAVE_OP(id, ctx, ...) \
    (cns_weave_op_t){ \
        .operation_id = id, \
        .context = ctx, \
        .args = {__VA_ARGS__, 0, 0, 0, 0, 0}, \
        .quantum_padding = 0 \
    }

// ============================================================================
// Validation and Reporting
// ============================================================================

// Compare two Gatekeeper reports for invariance
bool cns_weaver_reports_identical(const cns_gatekeeper_report_t* a,
                                 const cns_gatekeeper_report_t* b);

// Calculate deviation score between reports
uint64_t cns_weaver_calculate_deviation(const cns_gatekeeper_report_t* canonical,
                                       const cns_gatekeeper_report_t* permuted);

// Print detailed comparison report
void cns_weaver_print_comparison(const cns_gatekeeper_report_t* canonical,
                                const cns_gatekeeper_report_t* permuted);

// ============================================================================
// Performance Contracts
// ============================================================================

// Ensure Weaver operations are 8T-compliant
#define CNS_WEAVER_8T_COMPLIANT(code) \
    CNS_8T_EXECUTE(&weaver_op, code)

// Validate 8M alignment at compile time
#define CNS_WEAVER_8M_ALIGNED(type) \
    static_assert(sizeof(type) % 8 == 0, "Type must be 8M-aligned")

// Compile-time validation of core structures
CNS_WEAVER_8M_ALIGNED(cns_weave_op_t);
CNS_WEAVER_8M_ALIGNED(cns_weave_t);
CNS_WEAVER_8M_ALIGNED(cns_gatekeeper_report_t);
CNS_WEAVER_8M_ALIGNED(cns_weaver_context_t);

#endif // CNS_WEAVER_H 