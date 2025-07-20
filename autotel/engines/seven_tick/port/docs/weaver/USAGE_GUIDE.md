# CNS Permutation Weaver - Usage Guide

## Quick Start

### 1. Build the Weaver

```bash
cd port
make -f Makefile.weaver
```

### 2. Run the Example Test

```bash
./bin/cns_weaver
```

This will run the built-in example test that demonstrates the Weaver's capabilities.

## Defining Permutation Tests

### Basic Test Structure

A permutation test consists of two parts:
1. A canonical sequence of operations
2. A test definition that specifies how many permutations to run

```c
#include "cns/cns_weaver.h"

// Define your operation sequence
cns_weave_op_t my_sequence[] = {
    CNS_OP(OP_8T_EXECUTE, NULL, 0x1234567890ABCDEFULL, 0xFEDCBA0987654321ULL),
    CNS_OP(OP_8H_COGNITIVE_CYCLE, NULL, 0xAAAAAAAAAAAAAAAAULL),
    CNS_OP(OP_8M_ALLOC, NULL, 1024),
    // ... more operations
};

// Define the test
CNS_PERMUTATION_DEFINE(my_invariance_test, my_sequence);
```

### Operation Definition

Use the `CNS_OP` macro to define operations:

```c
CNS_OP(operation_id, context, arg1, arg2, arg3, ...)
```

**Parameters**:
- `operation_id`: The operation type (e.g., `OP_8T_EXECUTE`)
- `context`: Pointer to operation context (can be NULL)
- `arg1, arg2, ...`: Up to 6 uint64_t arguments

### Available Operation Types

#### Core Trinity Operations
```c
OP_8T_EXECUTE           // 8T physics layer operation
OP_8H_COGNITIVE_CYCLE   // 8H cognitive reasoning
OP_8M_ALLOC             // 8M memory allocation
OP_TRINITY_INIT         // Trinity initialization
OP_TRINITY_EXECUTE      // Trinity execution
```

#### Graph Operations
```c
OP_GRAPH_INIT           // Initialize graph
OP_GRAPH_ADD_TRIPLE     // Add RDF triple
OP_GRAPH_QUERY          // Query graph
OP_GRAPH_VALIDATE       // Validate graph
```

#### SHACL Operations
```c
OP_SHACL_VALIDATE       // Validate SHACL constraints
OP_SHACL_CONSTRAINT     // Apply specific constraint
OP_SHACL_SHAPE          // Process shape definition
```

#### SPARQL Operations
```c
OP_SPARQL_QUERY         // Execute SPARQL query
OP_SPARQL_SCAN          // Scan graph
OP_SPARQL_FILTER        // Apply filter
OP_SPARQL_JOIN          // Join operations
```

#### AOT Operations
```c
OP_AOT_COMPILE          // AOT compilation
OP_AOT_GENERATE         // Code generation
OP_AOT_VALIDATE         // AOT validation
```

## Running Tests

### Basic Test Execution

```c
int main() {
    cns_weaver_state_t state;
    
    // Initialize the weaver
    int result = cns_weaver_init(&state);
    if (result != CNS_WEAVER_SUCCESS) {
        printf("Failed to initialize weaver: %d\n", result);
        return 1;
    }
    
    // Register your operations
    probe_register_operation(OP_8T_EXECUTE, my_8t_function, "8T_EXECUTE");
    probe_register_operation(OP_8H_COGNITIVE_CYCLE, my_8h_function, "8H_CYCLE");
    // ... register other operations
    
    // Run the test
    result = cns_weaver_run(&my_invariance_test_weave);
    if (result != CNS_WEAVER_SUCCESS) {
        printf("Test failed: %d\n", result);
        cns_weaver_cleanup(&state);
        return 1;
    }
    
    // Clean up
    cns_weaver_cleanup(&state);
    return 0;
}
```

### Custom Test Configuration

You can customize test parameters:

```c
// Modify the generated test definition
my_invariance_test_weave.permutations_to_run = 5000;  // More permutations
my_invariance_test_weave.seed = 0x1234567890ABCDEFULL; // Custom seed
my_invariance_test_weave.flags = PERM_TEMPORAL | PERM_LOGICAL; // Specific permutation types
```

## Understanding Results

### Successful Test Output

```
=== Running Permutation Test: my_invariance_test ===
Operations: 5
Permutations: 1000
Seed: 0x1234567890ABCDEF

Permutation 1/1000: PASS (invariant)
Permutation 2/1000: PASS (invariant)
...
Permutation 1000/1000: PASS (invariant)

=== CNS Permutation Weaver Results ===
Test: my_invariance_test
Total permutations: 1000
Successful: 1000
Failed: 0
Success rate: 100.00%
Total execution time: 1234567 cycles
Average execution time: 1234 cycles

INVARIANCE PROVEN!
The system is invariant under all tested physical permutations.
The Trinity's logic is perfectly orthogonal to its physical execution.
=== End Results ===
```

### Failed Test Output

```
=== Running Permutation Test: my_invariance_test ===
Operations: 5
Permutations: 1000
Seed: 0x1234567890ABCDEF

Permutation 1/1000: PASS (invariant)
Permutation 2/1000: FAIL (deviation score: 1234)
...
Permutation 1000/1000: PASS (invariant)

=== Generating Hypotheses ===
Hypothesis 1:
  Description: System shows sensitivity to temporal permutations
  Operation ID: 0x0001
  Permutation Type: 0x01
  Confidence: 85%
  Evidence Count: 1

=== CNS Permutation Weaver Results ===
Test: my_invariance_test
Total permutations: 1000
Successful: 987
Failed: 13
Success rate: 98.70%
Total execution time: 1234567 cycles
Average execution time: 1234 cycles

INVARIANCE VIOLATIONS DETECTED!
The system is NOT invariant under physical permutations.
This indicates that the Trinity's logic is not perfectly orthogonal to its physical execution.
=== End Results ===
```

## Advanced Usage

### Custom Permutation Types

You can specify which types of permutations to apply:

```c
// Temporal permutations only
my_invariance_test_weave.flags = PERM_TEMPORAL;

// Logical and spatial permutations
my_invariance_test_weave.flags = PERM_LOGICAL | PERM_SPATIAL;

// All permutation types
my_invariance_test_weave.flags = PERM_COMPOSITE;
```

### Hypothesis Analysis

When failures are detected, the 8H Hypothesis Engine generates hypotheses:

```c
if (weaver_state.failed_permutations > 0) {
    cns_hypothesis_t hypothesis;
    int result = cns_weaver_generate_hypothesis(&weaver_state, &hypothesis);
    if (result == CNS_WEAVER_SUCCESS) {
        printf("Generated hypothesis: %s\n", hypothesis.description);
        printf("Confidence: %llu%%\n", hypothesis.confidence_score);
    }
}
```

### Detailed Analysis

For detailed analysis of specific failures:

```c
// Print detailed comparison of two reports
oracle_print_comparison(&result.canonical_report, &result.permuted_report);

// Print telemetry for debugging
probe_print_telemetry(telemetry_buffer, op_count);
```

## Integration with CNS v8

### Registering CNS v8 Operations

```c
// In your CNS v8 initialization
int cns_v8_init() {
    // ... other initialization ...
    
    // Register operations with the weaver
    probe_register_operation(OP_8T_EXECUTE, cns_8t_execute, "8T_EXECUTE");
    probe_register_operation(OP_8H_COGNITIVE_CYCLE, cns_8h_cognitive_cycle, "8H_CYCLE");
    probe_register_operation(OP_8M_ALLOC, cns_8m_alloc, "8M_ALLOC");
    probe_register_operation(OP_SHACL_VALIDATE, cns_shacl_validate, "SHACL_VALIDATE");
    probe_register_operation(OP_SPARQL_QUERY, cns_sparql_query, "SPARQL_QUERY");
    
    return CNS_WEAVER_SUCCESS;
}
```

### Operation Implementation

Your operation functions should follow this signature:

```c
int my_operation(void* context, uint64_t* args) {
    // Extract arguments
    uint64_t arg1 = args[0];
    uint64_t arg2 = args[1];
    // ... extract other args as needed
    
    // Perform operation
    // ... your operation logic ...
    
    // Return success/failure
    return CNS_WEAVER_SUCCESS; // or error code
}
```

## Performance Considerations

### Minimizing Overhead

The Weaver's instrumentation adds minimal overhead:
- < 20 cycles per tracked operation
- Cycle-level precision timing
- Efficient telemetry collection

### Optimizing Test Sequences

- Keep operation sequences focused and relevant
- Avoid unnecessary operations that don't test invariance
- Use appropriate permutation counts (1000-10000 typically sufficient)

### Memory Usage

- Each permutation result requires ~1KB of memory
- For 1000 permutations: ~1MB total memory usage
- Weaver automatically manages memory allocation/deallocation

## Troubleshooting

### Common Issues

1. **Unknown Operation ID**: Register all operations before running tests
2. **Memory Allocation Failure**: Check available system memory
3. **Timing Issues**: Ensure cycle-level timing is available on your platform

### Debug Build

For debugging, use the debug build:

```bash
make -f Makefile.weaver debug
```

This includes debug symbols and additional logging.

### Memory Leak Detection

```bash
make -f Makefile.weaver memcheck
```

Uses Valgrind to detect memory leaks.

## Best Practices

1. **Start Simple**: Begin with basic operation sequences
2. **Gradual Complexity**: Add complexity incrementally
3. **Reproducible Seeds**: Use fixed seeds for reproducible results
4. **Comprehensive Coverage**: Test all operation types
5. **Interpret Results**: Understand what deviations mean

## Success Criteria

A successful Fifth Epoch system should achieve:
- 100% success rate across all permutations
- Zero deviation scores
- Identical gatekeeper reports for canonical and permuted runs
- Perfect invariance under all physical permutations

This proves that the Trinity's logic is completely independent of its physical execution—the ultimate goal of the Fifth Epoch. 