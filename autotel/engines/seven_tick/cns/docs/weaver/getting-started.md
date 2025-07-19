# Getting Started with CNS Weaver

This guide will walk you through setting up and using the CNS Weaver system to generate OpenTelemetry instrumentation from TTL ontology definitions.

## Prerequisites

- Python 3.7 or higher
- rdflib library: `pip install rdflib`
- Access to CNS codebase

## Installation

The weaver system is included in the CNS codebase. No additional installation is required.

```bash
# Clone or navigate to CNS directory
cd cns

# Verify weaver components are present
ls codegen/
ls templates/
ls docs/ontology/
```

## Quick Start

### Step 1: Extract Spans from TTL

```bash
# Extract spans, functions, and patterns from the TTL ontology
python3 codegen/extract_spans.py docs/ontology/cns-core.ttl build/cns_spans.json
```

This command:
- Parses the TTL ontology file
- Extracts telemetry spans, functions, and patterns
- Generates function signatures
- Outputs a JSON file for code generation

**Expected Output:**
```
Extracting spans, functions, and patterns from docs/ontology/cns-core.ttl...
Found 13 telemetry spans:
  - spqlAsk: SPARQL Ask Span
  - spqlSelect: SPARQL Select Span
  ...
Found 13 functions:
  - spqlAsk (3 cycles)
  - spqlSelect (5 cycles)
  ...
Found 5 patterns:
  - SlabAllocator: Lock-Free Slab Allocator
  ...
Generated build/cns_spans.json with 13 spans, 13 functions, 5 patterns
✓ Extraction complete
```

### Step 2: Generate OpenTelemetry Code

```bash
# Generate OpenTelemetry instrumentation code
python3 codegen/weaver_simple.py build/cns_spans.json templates/otel_header.h.j2 templates/otel_inject.c.j2 src cns.h
```

This command:
- Reads the JSON context file
- Processes Jinja templates
- Generates OpenTelemetry header and inject files
- Creates performance tracking code

**Expected Output:**
```
🔧 CNS Weaver (Simplified) - OpenTelemetry Code Generation
==========================================================
Context: build/cns_spans.json
Output directory: src
Target header: cns.h

✅ Generated header: src/cns_otel.h
✅ Generated inject: src/cns_otel_inject.c

✅ Code generation completed successfully!
Generated files:
  - src/cns_otel.h
  - src/cns_otel_inject.c

📊 Summary:
  Spans: 13
  Functions: 13
  Patterns: 5
```

### Step 3: Validate the System

```bash
# Run comprehensive validation
python3 docs/validate_weaver.py
```

This validates:
- Help message display
- Error handling
- Span extraction
- Code generation
- Generated file content

## Generated Files

After running the weaver, you'll have:

### `src/cns_otel.h`
OpenTelemetry header file containing:
- Initialization functions
- Span management for each function
- Performance tracking structures
- Pattern macro definitions
- Utility macros

### `src/cns_otel_inject.c`
Instrumented code file containing:
- Function wrappers with telemetry
- Performance monitoring
- Gatekeeper integration
- Six Sigma validation
- Pattern implementation stubs

### `build/cns_spans.json`
JSON context file containing:
- Extracted spans and functions
- Function signatures
- Pattern definitions
- Metadata

## Using Generated Code

### Include the Headers

```c
#include "cns_otel.h"
#include "cns_otel_inject.c"
```

### Initialize the System

```c
int main() {
    // Initialize OpenTelemetry and performance tracking
    cns_otel_inject_init();
    
    // Your application code here
    
    // Cleanup
    cns_otel_inject_cleanup();
    return 0;
}
```

### Instrument Functions

The generated code provides macros for easy instrumentation:

```c
// Manual instrumentation
cns_perf_tracker_t tracker = cns_otel_start_spqlAsk();
// ... your function code ...
cns_otel_end_spqlAsk(&tracker);

// Or use the wrapper macro
CNS_OTEL_WRAP(spqlAsk, engine, query);
```

### Check Performance

```c
// Get performance statistics
cns_perf_stats_t* stats = cns_perf_get_stats();
printf("Mean cycles: %.2f\n", stats->mean_cycles);
printf("Violation rate: %.6f\n", (double)stats->violation_count / stats->operation_count);

// Validate against Six Sigma requirements
if (cns_gatekeeper_validate_performance()) {
    printf("✅ Performance meets Six Sigma requirements\n");
} else {
    printf("❌ Performance below Six Sigma requirements\n");
}
```

## Adding New Functions

To add new functions to the weaver system:

### 1. Update the TTL Ontology

Add to `docs/ontology/cns-core.ttl`:

```ttl
cns:newFunction a cns:Function ;
    cns:providesAPI "int cns_new_function(const char*)" ;
    cns:cycleCost 3 ;
    rdfs:label "New Function" .

cns:newFunctionSpan a cns:TelemetrySpan ;
    cns:providesFunction cns:newFunction ;
    rdfs:label "New Function Span" .
```

### 2. Regenerate Code

```bash
python3 codegen/extract_spans.py docs/ontology/cns-core.ttl build/cns_spans.json
python3 codegen/weaver_simple.py build/cns_spans.json templates/otel_header.h.j2 templates/otel_inject.c.j2 src cns.h
```

### 3. Use the Generated Code

The weaver automatically generates:
- `cns_otel_start_newFunction()`
- `cns_otel_end_newFunction()`
- `cns_perf_start_newFunction()`
- `cns_perf_end_newFunction()`
- Performance tracking macros

## Troubleshooting

### Common Issues

1. **Missing rdflib**: Install with `pip install rdflib`
2. **TTL parsing errors**: Check TTL syntax and file path
3. **Template errors**: Verify Jinja template syntax
4. **Permission errors**: Ensure write access to output directories

### Validation

Run the validation script to diagnose issues:

```bash
python3 docs/validate_weaver.py
```

### Debug Mode

For detailed debugging, check the generated files:

```bash
# Check JSON structure
cat build/cns_spans.json | jq .

# Check generated header
head -50 src/cns_otel.h

# Check generated inject file
head -50 src/cns_otel_inject.c
```

## Next Steps

- Read the [Architecture](architecture.md) guide for system design details
- Review [Examples](examples.md) for advanced usage patterns
- Check [API Reference](api-reference.md) for detailed documentation
- Explore [Integration](build-integration.md) guides for build system setup

## Support

For additional help:
- Check the [Troubleshooting](troubleshooting.md) guide
- Review the [Examples](examples.md) for usage patterns
- Run the validation script to diagnose issues
- Consult the [API Reference](api-reference.md) for detailed documentation 