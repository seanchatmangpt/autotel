# CNS v8 Implementation Guide

## Overview

This guide provides practical implementation details, code examples, and best practices for porting the CNS architecture to v8. It focuses on the concrete steps needed to implement the architectural principles.

## Implementation Philosophy

The CNS v8 implementation follows these core principles:

1. **Port Concepts, Not Just Code**: Understand the architectural principle before implementing
2. **Validate at Every Step**: Test each component as you build it
3. **Maintain 8B Compliance**: Every struct must align with 64-bit words
4. **Achieve 8-Tick Performance**: Every operation must complete within 8 CPU cycles
5. **Use AOT Compilation**: Generate optimized code from specifications

## Core Implementation Patterns

### 1. 8B Memory Contract Pattern

Every data structure must follow this pattern:

```c
typedef struct example_t {
    cns_word_t primary_field;    // 64-bit field
    cns_ptr_t pointer_field;     // 64-bit pointer
    cns_size_t size_field;       // 64-bit size
    cns_word_t padding;          // Explicit padding for 8B alignment
} example_t;

CNS_ASSERT_8B_COMPLIANT(example_t);
```

### 2. Branchless Operation Pattern

Use bitwise operations instead of branches:

```c
// Instead of:
if (size % 8 != 0) {
    padding = 8 - (size % 8);
} else {
    padding = 0;
}

// Use:
cns_size_t padding = (CNS_WORD_UNITS - (size & (CNS_WORD_UNITS - 1))) & (CNS_WORD_UNITS - 1);
```

### 3. AOT Generation Pattern

Generate optimized code from specifications:

```python
def generate_optimized_function(spec):
    """Generate optimized C function from specification"""
    function_name = spec['name']
    parameters = spec['parameters']
    logic = spec['logic']
    
    # Generate branchless, optimized C code
    c_code = f"""
static inline bool {function_name}({parameters}) {{
    // AOT-generated optimized logic
    {logic}
    return result;
}}
"""
    return c_code
```

### 4. Bitmask Processing Pattern

Use bitmasks for efficient operations:

```c
// OWL subclass check using bitmask
static inline bool cns_owl_is_subclass(cns_hash_t subclass_id, cns_hash_t superclass_id) {
    cns_bitmask_t superclasses = owl_classes[subclass_id].superclasses;
    return (superclasses & (1ULL << (superclass_id % 64))) != 0;
}

// SHACL validation using bitmask
static inline bool cns_shacl_validate(cns_hash_t entity_id, cns_hash_t shape_id) {
    cns_bitmask_t entity_classes = entity_classes[entity_id];
    cns_bitmask_t target_classes = shacl_shapes[shape_id].target_classes;
    return (entity_classes & target_classes) != 0;
}
```

## Implementation Examples

### Arena Allocator Implementation

```c
// arena.h
#ifndef CNS_ARENA_H
#define CNS_ARENA_H

#include "cns_core.h"

typedef struct arena_t {
    cns_ptr_t base;           // Base address of arena
    cns_size_t size;          // Total size of arena
    cns_offset_t offset;      // Current allocation offset
    cns_size_t peak;          // Peak memory usage
    cns_word_t magic;         // Magic number for validation
} arena_t;

CNS_ASSERT_8B_COMPLIANT(arena_t);

// Branchless allocation function
static inline cns_ptr_t cns_arena_alloc(arena_t* arena, cns_size_t size) {
    cns_size_t aligned_size = CNS_ALIGN(size);
    
    if (CNS_UNLIKELY(arena->offset + aligned_size > arena->size)) {
        return 0; // Out of memory
    }
    
    cns_ptr_t ptr = arena->base + arena->offset;
    arena->offset += aligned_size;
    
    if (arena->offset > arena->peak) {
        arena->peak = arena->offset;
    }
    
    return ptr;
}

#endif // CNS_ARENA_H
```

### Command Dispatch Implementation

```c
// dispatch.h
#ifndef CNS_DISPATCH_H
#define CNS_DISPATCH_H

#include "cns_core.h"

typedef struct cns_command_t {
    cns_hash_t hash;          // Command hash for O(1) lookup
    const char* name;         // Command name
    const char* domain;       // Domain name
    cns_ptr_t handler;        // Function pointer to handler
    cns_word_t flags;         // Command flags
    cns_word_t padding;       // Explicit padding for 8B alignment
} cns_command_t;

CNS_ASSERT_8B_COMPLIANT(cns_command_t);

// Perfect hash function
static inline cns_hash_t cns_command_hash(const char* name) {
    cns_hash_t hash = 0x811c9dc5; // FNV-1a basis
    while (*name) {
        hash ^= (cns_hash_t)*name++;
        hash *= 0x01000193; // FNV-1a prime
    }
    return hash;
}

// O(1) command lookup
static inline cns_command_t* cns_dispatch_command(const char* name) {
    cns_hash_t hash = cns_command_hash(name);
    cns_size_t index = hash % CNS_MAX_COMMANDS;
    return dispatch_table[index].command;
}

#endif // CNS_DISPATCH_H
```

### AOT Compiler Implementation

```python
#!/usr/bin/env python3
"""
CNS v8 AOT Compiler Implementation
"""

import json
import sys
from typing import Dict, Any

class CNSAOTCompiler:
    def __init__(self):
        self.template_cache = {}
        self.schema_cache = {}
    
    def parse_schema(self, schema_file: str) -> Dict[str, Any]:
        """Parse and validate schema"""
        with open(schema_file, 'r') as f:
            schema = json.load(f)
        
        # Validate 8B compliance
        self.validate_8b_compliance(schema)
        return schema
    
    def validate_8b_compliance(self, schema: Dict[str, Any]):
        """Ensure schema generates 8B-compliant structs"""
        for table_name, table_schema in schema['tables'].items():
            total_size = 0
            for field_name, field_type in table_schema['fields'].items():
                total_size += self.get_field_size(field_type)
            
            if total_size % 8 != 0:
                padding = 8 - (total_size % 8)
                print(f"Warning: {table_name} needs {padding} bytes padding")
    
    def generate_c_header(self, schema: Dict[str, Any], output_file: str):
        """Generate optimized C header"""
        with open(output_file, 'w') as f:
            f.write("#ifndef CNS_AOT_TYPES_H\n")
            f.write("#define CNS_AOT_TYPES_H\n\n")
            f.write('#include "cns_core.h"\n\n')
            
            for table_name, table_schema in schema['tables'].items():
                f.write(f"// {table_name} - 8B compliant struct\n")
                f.write(f"typedef struct {table_name}_t {{\n")
                
                total_size = 0
                for field_name, field_type in table_schema['fields'].items():
                    c_type = self.get_c_type(field_type)
                    f.write(f"    {c_type} {field_name};\n")
                    total_size += self.get_field_size(field_type)
                
                # Add explicit padding if needed
                if total_size % 8 != 0:
                    padding = 8 - (total_size % 8)
                    f.write(f"    uint8_t _padding[{padding}];\n")
                
                f.write(f"}} {table_name}_t;\n")
                f.write(f"CNS_ASSERT_8B_COMPLIANT({table_name}_t);\n\n")
            
            f.write("#endif // CNS_AOT_TYPES_H\n")
    
    def get_c_type(self, field_type: str) -> str:
        """Map schema types to C types"""
        type_map = {
            'int': 'int64_t',
            'string': 'char[64]',
            'float': 'double',
            'bool': 'uint8_t',
            'timestamp': 'uint64_t'
        }
        return type_map.get(field_type, 'uint64_t')
    
    def get_field_size(self, field_type: str) -> int:
        """Calculate field size in bytes"""
        size_map = {
            'int': 8,
            'string': 64,
            'float': 8,
            'bool': 1,
            'timestamp': 8
        }
        return size_map.get(field_type, 8)

def main():
    if len(sys.argv) != 3:
        print("Usage: aot_compiler.py <schema.json> <output.h>")
        sys.exit(1)
    
    schema_file = sys.argv[1]
    output_file = sys.argv[2]
    
    compiler = CNSAOTCompiler()
    schema = compiler.parse_schema(schema_file)
    compiler.generate_c_header(schema, output_file)
    
    print(f"Generated {output_file} with 8B-compliant types")

if __name__ == "__main__":
    main()
```

## Build System Integration

### CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.16)
project(cns_v8 C)

# Compiler flags for 8-tick performance
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -march=native -flto")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror")

# AOT Compilation - The Reasoner is the Build System
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/generated/aot_types.h
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tools/aot_compiler.py
            ${CMAKE_CURRENT_SOURCE_DIR}/schemas/schema.json
            ${CMAKE_CURRENT_BINARY_DIR}/generated/aot_types.h
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/aot_compiler.py
            ${CMAKE_CURRENT_SOURCE_DIR}/schemas/schema.json
    COMMENT "AOT Compiling schema to C types"
)

# Main CNS library
add_library(cns_v8
    src/core/arena.c
    src/core/cli.c
    src/core/dispatch.c
    src/engines/owl.c
    src/engines/shacl.c
    src/engines/sparql.c
)

target_include_directories(cns_v8 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}/generated
)

# Link against generated headers
target_sources(cns_v8 PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/generated/aot_types.h)
```

## Testing Implementation

### Unit Test Example

```c
// test_arena.c
#include "cns_core.h"
#include "cns/arena.h"
#include <assert.h>
#include <stdio.h>

void test_arena_allocation() {
    // Create test arena
    uint8_t buffer[1024];
    arena_t arena = {
        .base = (cns_ptr_t)buffer,
        .size = 1024,
        .offset = 0,
        .peak = 0,
        .magic = CNS_ARENA_MAGIC
    };
    
    // Test allocation
    cns_ptr_t ptr1 = cns_arena_alloc(&arena, 64);
    assert(ptr1 != 0);
    assert(arena.offset == 64);
    
    cns_ptr_t ptr2 = cns_arena_alloc(&arena, 32);
    assert(ptr2 != 0);
    assert(arena.offset == 96); // 64 + 32 (aligned)
    
    // Test 8B compliance
    assert(arena.offset % CNS_WORD_UNITS == 0);
    
    printf("✓ Arena allocation test passed\n");
}

void test_8b_compliance() {
    // Test that all structs are 8B compliant
    assert(sizeof(arena_t) % CNS_WORD_UNITS == 0);
    assert(sizeof(cns_command_t) % CNS_WORD_UNITS == 0);
    assert(sizeof(owl_class_t) % CNS_WORD_UNITS == 0);
    
    printf("✓ 8B compliance test passed\n");
}

int main() {
    test_arena_allocation();
    test_8b_compliance();
    
    printf("All tests passed!\n");
    return 0;
}
```

## Performance Optimization

### Compiler Optimizations

```bash
# Compiler flags for maximum performance
CFLAGS="-O3 -march=native -flto -ffast-math -funroll-loops"

# Link-time optimization
LDFLAGS="-flto"

# Profile-guided optimization
CFLAGS="$CFLAGS -fprofile-generate"
# Run benchmarks
./benchmarks/run_performance_tests.sh
# Recompile with profile data
CFLAGS="$CFLAGS -fprofile-use"
```

### Cache-Friendly Access Patterns

```c
// Cache-friendly array access
typedef struct cache_friendly_array_t {
    cns_word_t data[1024];     // Aligned to cache line
    cns_size_t size;
    cns_word_t padding;        // Explicit padding
} cache_friendly_array_t;

CNS_ASSERT_8B_COMPLIANT(cache_friendly_array_t);

// Sequential access pattern
static inline void process_array_sequential(cache_friendly_array_t* array) {
    for (cns_size_t i = 0; i < array->size; i++) {
        // Sequential access is cache-friendly
        array->data[i] = process_element(array->data[i]);
    }
}
```

## Best Practices

### 1. Always Use 8B Alignment

```c
// Good: Explicit padding for 8B alignment
typedef struct good_struct_t {
    cns_word_t field1;
    cns_ptr_t field2;
    cns_word_t padding;  // Explicit padding
} good_struct_t;

// Bad: Implicit padding (may not be 8B aligned)
typedef struct bad_struct_t {
    cns_word_t field1;
    cns_ptr_t field2;
    // Missing explicit padding
} bad_struct_t;
```

### 2. Use Branchless Operations

```c
// Good: Branchless operation
cns_size_t padding = (CNS_WORD_UNITS - (size & (CNS_WORD_UNITS - 1))) & (CNS_WORD_UNITS - 1);

// Bad: Branching operation
cns_size_t padding;
if (size % CNS_WORD_UNITS != 0) {
    padding = CNS_WORD_UNITS - (size % CNS_WORD_UNITS);
} else {
    padding = 0;
}
```

### 3. Validate at Every Step

```c
// Always validate 8B compliance
CNS_ASSERT_8B_COMPLIANT(my_struct_t);

// Always validate performance
cns_tick_t start = get_ticks();
operation();
cns_tick_t end = get_ticks();
assert((end - start) <= CNS_TICK_LIMIT);
```

### 4. Use AOT Compilation

```python
# Generate optimized code from specifications
def generate_optimized_function(spec):
    # Don't interpret at runtime
    # Generate optimized C code instead
    return generate_c_code(spec)
```

## Common Pitfalls

### 1. Forgetting 8B Alignment

```c
// This will fail 8B compliance
typedef struct {
    uint32_t field1;  // 4 bytes
    uint32_t field2;  // 4 bytes
    // Total: 8 bytes, but may not be aligned
} bad_struct_t;
```

### 2. Using Branches for Performance-Critical Code

```c
// This may exceed 8-tick limit
if (condition) {
    do_something();
} else {
    do_something_else();
}
```

### 3. Runtime Interpretation

```c
// Don't do this - interpret at runtime
void interpret_specification(const char* spec) {
    // Runtime interpretation is slow
    parse_and_execute(spec);
}
```

## Success Metrics

The implementation is successful when:

1. **All structs pass 8B compliance checks**
2. **All operations complete within 8 ticks**
3. **AOT compilation generates valid, optimized code**
4. **All tests pass**
5. **Performance benchmarks meet targets**
6. **User scenarios complete successfully**

## Next Steps

After implementing the core components:

1. **Run validation tests**
2. **Profile performance**
3. **Optimize bottlenecks**
4. **Add more AOT compilers**
5. **Extend semantic engines**
6. **Deploy and monitor** 