# Phase 1: Port the Substrate (Memory and Control)

## Overview

With the physical laws established, you can now build the foundational substrate for memory management and command execution.

## Principle

All operations depend on a deterministic, 8-tick compliant memory and control plane.

## Action 1.1: Port the Arena Allocator

### Code Target
- `cns/include/cns/arena.h`
- `cns/aot/aot_calculator.c`
- `cns/aot/arena_codegen.c`

### Implementation Steps

#### Step 1: Re-implement the arena_t struct

```c
#include "cns_core.h"

typedef struct arena_t {
    cns_ptr_t base;           // Base address of arena
    cns_size_t size;          // Total size of arena
    cns_offset_t offset;      // Current allocation offset
    cns_size_t peak;          // Peak memory usage
    cns_word_t magic;         // Magic number for validation
} arena_t;

CNS_ASSERT_8B_COMPLIANT(arena_t);
```

#### Step 2: Port the branchless padding calculation

This is a core piece of 7T/8T physics:

```c
static inline cns_size_t cns_arena_padding(cns_size_t size) {
    // Branchless padding calculation for 8B alignment
    return (CNS_WORD_UNITS - (size & (CNS_WORD_UNITS - 1))) & (CNS_WORD_UNITS - 1);
}

static inline cns_ptr_t cns_arena_alloc(arena_t* arena, cns_size_t size) {
    cns_size_t aligned_size = CNS_ALIGN(size);
    cns_size_t padding = cns_arena_padding(size);
    
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
```

#### Step 3: Port the AOT calculator and code generator

These tools are your "Reasoners" for memory:

```c
// aot_calculator.c - Calculate memory requirements ahead of time
typedef struct memory_layout_t {
    cns_size_t total_size;
    cns_size_t alignment_padding;
    cns_size_t struct_count;
    cns_size_t max_alignment;
} memory_layout_t;

memory_layout_t cns_calculate_memory_layout(const char* schema_file) {
    // Parse schema and calculate exact memory requirements
    // This ensures the 8B contract is satisfied at compile time
}

// arena_codegen.c - Generate static memory arenas
void cns_generate_arena_code(const char* output_file, memory_layout_t layout) {
    // Generate C code for static, pre-allocated arenas
    // This eliminates runtime allocation overhead
}
```

## Action 1.2: Port the Command & Control System

### Code Target
- `cns/include/cns/cli.h`
- `cns/include/cns_dispatch.h`
- `cns/include/cns_commands.h`

### Implementation Steps

#### Step 1: Re-implement the command structures

```c
typedef struct cns_command_t {
    cns_hash_t hash;          // Command hash for O(1) lookup
    const char* name;         // Command name
    const char* domain;       // Domain name
    const char* description;  // Command description
    cns_ptr_t handler;        // Function pointer to handler
    cns_word_t flags;         // Command flags
    cns_word_t padding;       // Explicit padding for 8B alignment
} cns_command_t;

CNS_ASSERT_8B_COMPLIANT(cns_command_t);

typedef struct cns_dispatch_entry_t {
    cns_hash_t hash;          // Hash for collision resolution
    cns_ptr_t command;        // Pointer to command
    cns_word_t padding;       // Explicit padding for 8B alignment
} cns_dispatch_entry_t;

CNS_ASSERT_8B_COMPLIANT(cns_dispatch_entry_t);
```

#### Step 2: Port the hash-based dispatcher

The goal is an O(1) lookup that is provably 8-tick compliant:

```c
// Perfect hash function for command names
static inline cns_hash_t cns_command_hash(const char* name) {
    cns_hash_t hash = 0x811c9dc5; // FNV-1a basis
    while (*name) {
        hash ^= (cns_hash_t)*name++;
        hash *= 0x01000193; // FNV-1a prime
    }
    return hash;
}

// O(1) command lookup
cns_command_t* cns_dispatch_command(const char* domain, const char* name) {
    cns_hash_t hash = cns_command_hash(name);
    cns_size_t index = hash % CNS_MAX_COMMANDS;
    
    // Direct array lookup - guaranteed 8-tick
    return dispatch_table[index].command;
}
```

#### Step 3: Port the domain-based CLI structure

```c
typedef struct cns_cli_t {
    cns_command_t commands[CNS_MAX_COMMANDS];
    cns_dispatch_entry_t dispatch_table[CNS_MAX_COMMANDS];
    cns_size_t command_count;
    cns_word_t padding;       // Explicit padding for 8B alignment
} cns_cli_t;

CNS_ASSERT_8B_COMPLIANT(cns_cli_t);

// CLI initialization
void cns_cli_init(cns_cli_t* cli) {
    // Initialize CLI with built-in commands
    cns_register_builtin_commands(cli);
    cns_build_dispatch_table(cli);
}
```

## Validation

### Memory Validation

1. **Arena Alignment**: Verify all allocations are 8B aligned
2. **Padding Calculation**: Test branchless padding logic
3. **Memory Bounds**: Ensure no buffer overflows
4. **AOT Calculation**: Validate compile-time memory layout

### Performance Validation

1. **Allocation Speed**: Arena allocation must be < 8 ticks
2. **Command Lookup**: Dispatch must be O(1) and < 8 ticks
3. **Hash Function**: Command hashing must be deterministic and fast
4. **Memory Access**: All memory operations must be cache-friendly

### Functional Validation

1. **Command Registration**: All commands must be properly registered
2. **Domain Organization**: Commands must be organized by domain
3. **Error Handling**: Invalid commands must be handled gracefully
4. **Memory Management**: Arena must track usage correctly

## Success Criteria

- [ ] Arena allocator is 8B compliant and deterministic
- [ ] AOT calculator generates correct memory layouts
- [ ] Command dispatcher provides O(1) lookup
- [ ] CLI structure supports domain-based organization
- [ ] All operations complete within 8 ticks
- [ ] Memory access patterns are cache-friendly

## Next Steps

After completing Phase 1:
1. Move to [Phase 2: Port the AOT Toolchain](./phase2_aot_toolchain.md)
2. Use the arena allocator for all memory management
3. Use the command system for all user interactions
4. Validate performance at every step 