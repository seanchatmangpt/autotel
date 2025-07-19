# CNS Weaver Examples

This guide provides practical examples of using the CNS Weaver system for different scenarios.

## Basic Examples

### Example 1: Simple Span Extraction

Extract spans from a minimal TTL file:

```bash
# Create a simple TTL file
cat > test.ttl << 'EOF'
@prefix cns: <https://schema.chatman.ai/cns#> .
@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> .

cns:testFunction a cns:Function ;
    cns:providesAPI "int cns_test_function(void)" ;
    cns:cycleCost 2 ;
    rdfs:label "Test Function" .

cns:testFunctionSpan a cns:TelemetrySpan ;
    cns:providesFunction cns:testFunction ;
    rdfs:label "Test Function Span" .
EOF

# Extract spans
python3 codegen/extract_spans.py test.ttl test_spans.json

# Generate code
python3 codegen/weaver_simple.py test_spans.json templates/otel_header.h.j2 templates/otel_inject.c.j2 src cns.h
```

### Example 2: Custom Template Generation

Create a custom template for specific needs:

```bash
# Create custom header template
cat > custom_header.j2 << 'EOF'
// Custom OpenTelemetry Header
#ifndef CUSTOM_OTEL_H
#define CUSTOM_OTEL_H

#include <opentelemetry/c/trace.h>

{% for span in spans %}
// {{ span.label }}
#define CUSTOM_SPAN_{{ span.functionName.upper() }} "{{ span.spanName }}"
{% endfor %}

#endif
EOF

# Use custom template
python3 codegen/weaver_simple.py build/cns_spans.json custom_header.j2 templates/otel_inject.c.j2 src cns.h
```

## Advanced Examples

### Example 3: Performance Monitoring Integration

Integrate weaver output with custom performance monitoring:

```c
// custom_perf.c
#include "cns_otel.h"
#include <stdio.h>
#include <time.h>

// Custom performance monitoring
typedef struct {
    uint64_t start_time;
    uint64_t end_time;
    uint64_t cycles;
    const char* function_name;
} custom_perf_tracker_t;

static custom_perf_tracker_t* custom_perf_start(const char* func_name) {
    custom_perf_tracker_t* tracker = malloc(sizeof(custom_perf_tracker_t));
    tracker->start_time = clock();
    tracker->cycles = __builtin_readcyclecounter();
    tracker->function_name = func_name;
    return tracker;
}

static void custom_perf_end(custom_perf_tracker_t* tracker) {
    tracker->end_time = clock();
    uint64_t end_cycles = __builtin_readcyclecounter();
    uint64_t cycle_diff = end_cycles - tracker->cycles;
    
    printf("Function: %s\n", tracker->function_name);
    printf("  CPU time: %lu ms\n", (tracker->end_time - tracker->start_time) * 1000 / CLOCKS_PER_SEC);
    printf("  Cycles: %lu\n", cycle_diff);
    
    free(tracker);
}

// Use with weaver-generated functions
int main() {
    cns_otel_init();
    
    // Manual instrumentation with custom tracking
    custom_perf_tracker_t* perf = custom_perf_start("spqlAsk");
    opentelemetry_span_t* span = cns_otel_start_spqlAsk();
    
    // Your function call here
    // cns_spql_ask(engine, query);
    
    cns_otel_end_spqlAsk(span);
    custom_perf_end(perf);
    
    cns_otel_shutdown();
    return 0;
}
```

### Example 4: Conditional Instrumentation

Generate conditional instrumentation based on build flags:

```bash
# Create conditional template
cat > conditional_header.j2 << 'EOF'
#ifndef CONDITIONAL_OTEL_H
#define CONDITIONAL_OTEL_H

#include <opentelemetry/c/trace.h>

#ifdef CNS_ENABLE_TELEMETRY

{% for span in spans %}
// {{ span.label }}
static inline opentelemetry_span_t* cns_otel_start_{{ span.functionName }}() {
    return opentelemetry_tracer_start_span("{{ span.spanName }}", NULL, 0, NULL);
}

static inline void cns_otel_end_{{ span.functionName }}(opentelemetry_span_t* span) {
    if (span) opentelemetry_span_end(span);
}
{% endfor %}

#else

{% for span in spans %}
// {{ span.label }} - No-op when telemetry disabled
static inline opentelemetry_span_t* cns_otel_start_{{ span.functionName }}() { return NULL; }
static inline void cns_otel_end_{{ span.functionName }}(opentelemetry_span_t* span) { (void)span; }
{% endfor %}

#endif

#endif
EOF

# Generate conditional code
python3 codegen/weaver_simple.py build/cns_spans.json conditional_header.j2 templates/otel_inject.c.j2 src cns.h
```

### Example 5: Multi-Target Generation

Generate code for multiple targets (development, production, testing):

```bash
#!/bin/bash
# multi_target_generation.sh

# Development target - full instrumentation
python3 codegen/weaver_simple.py build/cns_spans.json templates/otel_header.h.j2 templates/otel_inject.c.j2 src/dev cns.h
cp src/dev/cns_otel.h src/cns_otel_dev.h
cp src/dev/cns_otel_inject.c src/cns_otel_inject_dev.c

# Production target - minimal overhead
python3 codegen/weaver_simple.py build/cns_spans.json templates/otel_header.h.j2 templates/otel_inject.c.j2 src/prod cns.h
cp src/prod/cns_otel.h src/cns_otel_prod.h
cp src/prod/cns_otel_inject.c src/cns_otel_inject_prod.c

# Testing target - with validation
python3 codegen/weaver_simple.py build/cns_spans.json templates/otel_header.h.j2 templates/otel_inject.c.j2 src/test cns.h
cp src/test/cns_otel.h src/cns_otel_test.h
cp src/test/cns_otel_inject.c src/cns_otel_inject_test.c

echo "Generated code for dev, prod, and test targets"
```

## Integration Examples

### Example 6: CMake Integration

Integrate weaver with CMake build system:

```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
project(CNS VERSION 1.0.0)

# Weaver integration
add_custom_target(weaver-extract
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/build
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/codegen/extract_spans.py
            ${CMAKE_CURRENT_SOURCE_DIR}/docs/ontology/cns-core.ttl
            ${CMAKE_CURRENT_BINARY_DIR}/build/cns_spans.json
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/docs/ontology/cns-core.ttl
    COMMENT "Extracting spans from TTL"
)

add_custom_target(weaver-generate
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/codegen/weaver_simple.py
            ${CMAKE_CURRENT_BINARY_DIR}/build/cns_spans.json
            ${CMAKE_CURRENT_SOURCE_DIR}/templates/otel_header.h.j2
            ${CMAKE_CURRENT_SOURCE_DIR}/templates/otel_inject.c.j2
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            cns.h
    DEPENDS weaver-extract
    COMMENT "Generating OpenTelemetry code"
)

# Main library depends on weaver
add_library(cns_static STATIC src/cns_main.c src/cns_otel.h src/cns_otel_inject.c)
add_dependencies(cns_static weaver-generate)
```

### Example 7: CI/CD Integration

Integrate weaver with continuous integration:

```yaml
# .github/workflows/weaver.yml
name: Weaver Validation

on: [push, pull_request]

jobs:
  weaver-validation:
    runs-on: ubuntu-latest
    steps:
    - uses: actions/checkout@v2
    
    - name: Set up Python
      uses: actions/setup-python@v2
      with:
        python-version: '3.9'
    
    - name: Install dependencies
      run: |
        pip install rdflib
    
    - name: Run weaver validation
      run: |
        cd cns
        python3 docs/validate_weaver.py
    
    - name: Check generated files
      run: |
        cd cns
        test -f src/cns_otel.h
        test -f src/cns_otel_inject.c
        test -f build/cns_spans.json
```

## Real-World Usage Examples

### Example 8: Adding New Function to Existing System

```bash
# 1. Add new function to TTL
cat >> docs/ontology/cns-core.ttl << 'EOF'

cns:memoryAlloc a cns:Function ;
    cns:providesAPI "void* cns_memory_alloc(size_t size)" ;
    cns:cycleCost 4 ;
    rdfs:label "Memory Allocation Function" .

cns:memoryAllocSpan a cns:TelemetrySpan ;
    cns:providesFunction cns:memoryAlloc ;
    rdfs:label "Memory Allocation Span" .
EOF

# 2. Regenerate code
python3 codegen/extract_spans.py docs/ontology/cns-core.ttl build/cns_spans.json
python3 codegen/weaver_simple.py build/cns_spans.json templates/otel_header.h.j2 templates/otel_inject.c.j2 src cns.h

# 3. Use in your code
cat > memory_example.c << 'EOF'
#include "cns_otel.h"

void* my_memory_alloc(size_t size) {
    cns_perf_tracker_t tracker = cns_perf_start_memoryAlloc();
    
    void* ptr = malloc(size);
    
    cns_perf_end_memoryAlloc(&tracker);
    return ptr;
}
EOF
```

### Example 9: Performance Analysis Script

```python
#!/usr/bin/env python3
# performance_analysis.py

import json
import subprocess
import sys

def analyze_performance():
    """Analyze performance using weaver-generated code."""
    
    # Extract current spans
    subprocess.run([
        'python3', 'codegen/extract_spans.py',
        'docs/ontology/cns-core.ttl',
        'build/cns_spans.json'
    ], check=True)
    
    # Generate code
    subprocess.run([
        'python3', 'codegen/weaver_simple.py',
        'build/cns_spans.json',
        'templates/otel_header.h.j2',
        'templates/otel_inject.c.j2',
        'src', 'cns.h'
    ], check=True)
    
    # Load span data
    with open('build/cns_spans.json', 'r') as f:
        data = json.load(f)
    
    # Analyze cycle costs
    total_cycles = sum(func['cycleCost'] for func in data['functions'])
    avg_cycles = total_cycles / len(data['functions'])
    
    print(f"Performance Analysis:")
    print(f"  Total functions: {len(data['functions'])}")
    print(f"  Total cycles: {total_cycles}")
    print(f"  Average cycles: {avg_cycles:.2f}")
    
    # Check Six Sigma compliance
    violations = [f for f in data['functions'] if f['cycleCost'] > 7]
    if violations:
        print(f"  ⚠️  {len(violations)} functions exceed 7-cycle limit:")
        for func in violations:
            print(f"    - {func['functionName']}: {func['cycleCost']} cycles")
    else:
        print(f"  ✅ All functions within 7-cycle limit")

if __name__ == '__main__':
    analyze_performance()
```

## Troubleshooting Examples

### Example 10: Debugging Template Issues

```bash
# Create debug template
cat > debug_template.j2 << 'EOF'
// Debug template - shows all variables
/*
Debug Information:
- Spans: {{ spans|length }}
- Functions: {{ functions|length }}
- Patterns: {{ patterns|length }}
- Metadata: {{ metadata }}
*/

{% for span in spans %}
// Span: {{ span.spanName }} -> {{ span.functionName }}
{% endfor %}

{% for func in functions %}
// Function: {{ func.functionName }} ({{ func.cycleCost }} cycles)
{% endfor %}
EOF

# Use debug template
python3 codegen/weaver_simple.py build/cns_spans.json debug_template.j2 templates/otel_inject.c.j2 src cns.h
```

### Example 11: Validation Script

```bash
#!/bin/bash
# validate_weaver.sh

echo "Validating CNS Weaver..."

# Check prerequisites
if ! command -v python3 &> /dev/null; then
    echo "❌ Python3 not found"
    exit 1
fi

if ! python3 -c "import rdflib" &> /dev/null; then
    echo "❌ rdflib not found. Install with: pip install rdflib"
    exit 1
fi

# Test span extraction
echo "Testing span extraction..."
if python3 codegen/extract_spans.py docs/ontology/cns-core.ttl build/test_spans.json; then
    echo "✅ Span extraction successful"
else
    echo "❌ Span extraction failed"
    exit 1
fi

# Test code generation
echo "Testing code generation..."
if python3 codegen/weaver_simple.py build/test_spans.json templates/otel_header.h.j2 templates/otel_inject.c.j2 src cns.h; then
    echo "✅ Code generation successful"
else
    echo "❌ Code generation failed"
    exit 1
fi

# Check generated files
echo "Checking generated files..."
if [ -f "src/cns_otel.h" ] && [ -f "src/cns_otel_inject.c" ]; then
    echo "✅ Generated files present"
else
    echo "❌ Generated files missing"
    exit 1
fi

echo "🎉 Weaver validation successful!"
```

These examples demonstrate the flexibility and power of the CNS Weaver system for various use cases and integration scenarios. 