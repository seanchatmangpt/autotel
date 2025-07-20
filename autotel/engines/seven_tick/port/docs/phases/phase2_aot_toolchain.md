# Phase 2: Port the AOT Toolchain and Semantic Engines

## Overview

Now, implement the core value proposition: transpiling high-level semantic specifications into low-level, high-performance C code.

## Principle

The "Reasoner is the Build System." TTL, OWL, and SHACL are not data; they are specifications for C code.

## Action 2.1: Port the AOT Compiler and Build Integration

### Code Target
- `cns/sql_compiler.py`
- `cns/CMakeLists.txt`

### Implementation Steps

#### Step 1: Port the Python-based AOT compiler

Start with the SQL compiler as it is the most complete and self-contained example:

```python
#!/usr/bin/env python3
"""
CNS v8 SQL AOT Compiler
Transpiles SQL templates and JSON schemas into optimized C functions
"""

import json
import sys
import re
from typing import Dict, List, Any

class SQLAOTCompiler:
    def __init__(self):
        self.template_cache = {}
        self.schema_cache = {}
    
    def parse_schema(self, schema_file: str) -> Dict[str, Any]:
        """Parse JSON schema and validate 8B compliance"""
        with open(schema_file, 'r') as f:
            schema = json.load(f)
        
        # Validate schema structure
        self.validate_schema_8b_compliance(schema)
        return schema
    
    def validate_schema_8b_compliance(self, schema: Dict[str, Any]):
        """Ensure all schema types align with 8B contract"""
        for table_name, table_schema in schema['tables'].items():
            total_size = 0
            for field_name, field_type in table_schema['fields'].items():
                field_size = self.get_field_size(field_type)
                total_size += field_size
            
            # Ensure table size is multiple of 8
            if total_size % 8 != 0:
                padding = 8 - (total_size % 8)
                print(f"Warning: Table {table_name} needs {padding} bytes padding")
    
    def generate_c_header(self, schema: Dict[str, Any], output_file: str):
        """Generate optimized C header with 8B-compliant structs"""
        with open(output_file, 'w') as f:
            f.write("#ifndef CNS_SQL_AOT_TYPES_H\n")
            f.write("#define CNS_SQL_AOT_TYPES_H\n\n")
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
            
            f.write("#endif // CNS_SQL_AOT_TYPES_H\n")
    
    def get_c_type(self, field_type: str) -> str:
        """Map schema types to C types"""
        type_map = {
            'int': 'int64_t',
            'string': 'char[64]',  # Fixed-size for 8B compliance
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
    if len(sys.argv) != 4:
        print("Usage: sql_compiler.py <schema.json> <template.sql> <output.h>")
        sys.exit(1)
    
    schema_file = sys.argv[1]
    template_file = sys.argv[2]
    output_file = sys.argv[3]
    
    compiler = SQLAOTCompiler()
    schema = compiler.parse_schema(schema_file)
    compiler.generate_c_header(schema, output_file)
    
    print(f"Generated {output_file} with 8B-compliant types")

if __name__ == "__main__":
    main()
```

#### Step 2: Re-create the build system integration

This is the mechanism that makes the build system a reasoner:

```cmake
# CMakeLists.txt - CNS v8 Build System
cmake_minimum_required(VERSION 3.16)
project(cns_v8 C)

# Set compiler flags for 8-tick performance
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -march=native -flto")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror")

# AOT Compilation - The Reasoner is the Build System
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/generated/sql_aot_types.h
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tools/sql_compiler.py
            ${CMAKE_CURRENT_SOURCE_DIR}/schemas/business_schema.json
            ${CMAKE_CURRENT_SOURCE_DIR}/templates/business_queries.sql
            ${CMAKE_CURRENT_BINARY_DIR}/generated/sql_aot_types.h
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/sql_compiler.py
            ${CMAKE_CURRENT_SOURCE_DIR}/schemas/business_schema.json
            ${CMAKE_CURRENT_SOURCE_DIR}/templates/business_queries.sql
    COMMENT "AOT Compiling SQL schema to C types"
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
target_sources(cns_v8 PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/generated/sql_aot_types.h)
```

## Action 2.2: Port the Semantic Engines as Bitmask Processors

### Code Target
- `cns/include/cns/owl.h`
- `cns/include/cns/8t/shacl_l1.h`
- `cns/include/cns/sparql.h`

### Implementation Steps

#### Step 1: OWL Engine - Branchless Inline Functions

Do not port the runtime logic. Port the result of the AOT compilation:

```c
// owl.h - OWL Engine as AOT-compiled inline functions
#ifndef CNS_OWL_H
#define CNS_OWL_H

#include "cns_core.h"

// AOT-generated ontology rules
typedef struct owl_class_t {
    cns_hash_t class_id;
    cns_bitmask_t superclasses;  // Bitmask of superclass IDs
    cns_bitmask_t properties;    // Bitmask of property IDs
    cns_word_t padding;          // Explicit padding for 8B alignment
} owl_class_t;

CNS_ASSERT_8B_COMPLIANT(owl_class_t);

// Branchless subclass check - the core of OWL reasoning
static inline bool cns_owl_is_subclass(cns_hash_t subclass_id, cns_hash_t superclass_id) {
    // Direct array lookup with bitwise operations
    cns_bitmask_t superclasses = owl_classes[subclass_id].superclasses;
    return (superclasses & (1ULL << (superclass_id % 64))) != 0;
}

// AOT-generated property check
static inline bool cns_owl_has_property(cns_hash_t class_id, cns_hash_t property_id) {
    cns_bitmask_t properties = owl_classes[class_id].properties;
    return (properties & (1ULL << (property_id % 64))) != 0;
}

#endif // CNS_OWL_H
```

#### Step 2: SHACL Engine - Vectorized Bitwise Operations

Port the concept from shacl_l1.h and shacl_validators_7tick.h:

```c
// shacl.h - SHACL Engine as bitmask processor
#ifndef CNS_SHACL_H
#define CNS_SHACL_H

#include "cns_core.h"

// SHACL shape as bitmask
typedef struct shacl_shape_t {
    cns_hash_t shape_id;
    cns_bitmask_t constraints;   // Bitmask of constraint types
    cns_bitmask_t target_classes; // Bitmask of target classes
    cns_word_t padding;          // Explicit padding for 8B alignment
} shacl_shape_t;

CNS_ASSERT_8B_COMPLIANT(shacl_shape_t);

// SHACL validation as single bitwise operation
static inline bool cns_shacl_validate(cns_hash_t entity_id, cns_hash_t shape_id) {
    // Get entity's class bitmask
    cns_bitmask_t entity_classes = entity_classes[entity_id];
    
    // Get shape's target classes
    cns_bitmask_t target_classes = shacl_shapes[shape_id].target_classes;
    
    // Vectorized validation - single bitwise AND
    return (entity_classes & target_classes) != 0;
}

// Constraint validation
static inline bool cns_shacl_validate_constraint(cns_hash_t entity_id, cns_hash_t constraint_id) {
    // Pre-computed constraint validation table
    return constraint_validators[constraint_id](entity_id);
}

#endif // CNS_SHACL_H
```

#### Step 3: SPARQL Engine - Bit-Slab Processing

The SPARQL engine should be a bit-slab processor:

```c
// sparql.h - SPARQL Engine as bit-slab processor
#ifndef CNS_SPARQL_H
#define CNS_SPARQL_H

#include "cns_core.h"

// Bit-slab for efficient graph traversal
typedef struct sparql_slab_t {
    cns_bitmask_t entities[1024];    // Entity bitmasks
    cns_bitmask_t predicates[1024];  // Predicate bitmasks
    cns_bitmask_t objects[1024];     // Object bitmasks
    cns_size_t size;                 // Number of triples
    cns_word_t padding;              // Explicit padding for 8B alignment
} sparql_slab_t;

CNS_ASSERT_8B_COMPLIANT(sparql_slab_t);

// SPARQL WHERE clause as bitwise operations
static inline cns_bitmask_t cns_sparql_select(sparql_slab_t* slab, const char* where_clause) {
    // Parse WHERE clause into bitwise operations
    cns_bitmask_t result = 0xFFFFFFFFFFFFFFFF; // All entities initially selected
    
    // Apply each triple pattern as bitwise AND
    for (int i = 0; i < slab->size; i++) {
        if (matches_pattern(slab, i, where_clause)) {
            result &= slab->entities[i];
        }
    }
    
    return result;
}

// Pattern matching as bitwise operations
static inline bool matches_pattern(sparql_slab_t* slab, cns_size_t index, const char* pattern) {
    // Convert SPARQL pattern to bitwise operations
    // This is the core of the bit-slab approach
    return (slab->predicates[index] & pattern_predicate) != 0;
}

#endif // CNS_SPARQL_H
```

## Validation

### AOT Compilation Validation

1. **Schema Parsing**: Verify JSON schema is correctly parsed
2. **8B Compliance**: Generated structs must pass 8B checks
3. **Build Integration**: CMake must correctly invoke AOT compiler
4. **Generated Code**: Output must be valid, optimized C

### Semantic Engine Validation

1. **OWL Performance**: Subclass checks must be < 8 ticks
2. **SHACL Performance**: Validation must be < 8 ticks
3. **SPARQL Performance**: Query execution must be < 8 ticks
4. **Bitwise Operations**: All operations must use SIMD where possible

### Functional Validation

1. **OWL Reasoning**: Subclass relationships must be correct
2. **SHACL Validation**: Constraints must be properly enforced
3. **SPARQL Queries**: Results must match expected output
4. **Integration**: All engines must work together seamlessly

## Success Criteria

- [ ] AOT compiler generates 8B-compliant C code
- [ ] Build system integrates AOT compilation
- [ ] OWL engine provides branchless subclass checks
- [ ] SHACL engine uses vectorized validation
- [ ] SPARQL engine processes bit-slabs efficiently
- [ ] All semantic operations complete within 8 ticks
- [ ] Generated code is optimized and cache-friendly

## Next Steps

After completing Phase 2:
1. Move to [Phase 3: Meta-Validation and Operations](./phase3_meta_validation.md)
2. Use AOT compilation for all semantic specifications
3. Use bitmask operations for all reasoning tasks
4. Validate performance and correctness at every step 