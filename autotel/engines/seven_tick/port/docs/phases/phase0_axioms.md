# Phase 0: Establish the Architectural Axioms

## Overview

Before any functional code is written, you must define the physical laws of the CNS v8 universe. This phase establishes the 8B memory contract as a compile-time certainty.

## Principle

The 8-Bit (8B) Memory Contract is the physical law of the system. Every data structure must align with the 64-bit hardware word.

## Action

Create the new canonical header for CNS v8, which will serve as the constitution for the entire codebase.

## Code Target

Create `cns/v8/include/cns_core.h`

## Inspiration Files

- `cns/include/cns/8t/8thm_trinity.h`
- `cns/include/cns/optimization_helpers.h`
- `cns/s7t_minimal.h`

## Implementation Steps

### Step 1: Define Core Quanta

```c
// Core quantum definitions
#define CNS_QUANTUM_BITS 8
#define CNS_WORD_UNITS 8
#define CNS_WORD_BITS (CNS_QUANTUM_BITS * CNS_WORD_UNITS) // 64
#define CNS_CACHE_LINE_UNITS 64 // 64 * 8 bits = 512 bits = 64 bytes
```

### Step 2: Implement Alignment Macros

Port the branchless alignment macros:

```c
// Memory alignment macros
#define CNS_ALIGN(size) (((size) + (CNS_WORD_UNITS - 1)) & ~(CNS_WORD_UNITS - 1))
#define CNS_CACHE_ALIGN __attribute__((aligned(CNS_CACHE_LINE_UNITS)))

// Performance hints
#define CNS_LIKELY(x) __builtin_expect(!!(x), 1)
#define CNS_UNLIKELY(x) __builtin_expect(!!(x), 0)
#define CNS_HOT __attribute__((hot))
#define CNS_COLD __attribute__((cold))
```

### Step 3: Enforce the 8B Contract with Static Assertions

This is the most critical step. Add this macro to the core header and use it for every struct you port or create:

```c
#define CNS_ASSERT_8B_COMPLIANT(struct_name) \
    _Static_assert(sizeof(struct_name) % CNS_WORD_UNITS == 0, \
    #struct_name " violates the 8-Bit Memory Contract: size must be a multiple of 8.")
```

### Step 4: Define Core Data Types

```c
#include <stdint.h>
#include <stddef.h>

// Core CNS types - all 64-bit aligned
typedef uint64_t cns_word_t;
typedef uint64_t cns_ptr_t;
typedef uint64_t cns_size_t;
typedef uint64_t cns_offset_t;

// Performance-critical types
typedef uint64_t cns_bitmask_t;
typedef uint64_t cns_hash_t;
typedef uint64_t cns_tick_t;
```

### Step 5: Implement Core Constants

```c
// Performance limits
#define CNS_TICK_LIMIT 8
#define CNS_MEMORY_QUANTUM 8
#define CNS_MAX_ARENA_SIZE (1024 * 1024 * 1024) // 1GB

// System limits
#define CNS_MAX_COMMANDS 256
#define CNS_MAX_DOMAINS 64
#define CNS_MAX_PATTERNS 1024
```

### Step 6: Create the Core Header Structure

```c
// cns_core.h - The CNS v8 Constitution
#ifndef CNS_CORE_H
#define CNS_CORE_H

// Include standard headers
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Core quantum definitions
#define CNS_QUANTUM_BITS 8
#define CNS_WORD_UNITS 8
#define CNS_WORD_BITS (CNS_QUANTUM_BITS * CNS_WORD_UNITS)
#define CNS_CACHE_LINE_UNITS 64

// Alignment and performance macros
#define CNS_ALIGN(size) (((size) + (CNS_WORD_UNITS - 1)) & ~(CNS_WORD_UNITS - 1))
#define CNS_CACHE_ALIGN __attribute__((aligned(CNS_CACHE_LINE_UNITS)))
#define CNS_LIKELY(x) __builtin_expect(!!(x), 1)
#define CNS_UNLIKELY(x) __builtin_expect(!!(x), 0)
#define CNS_HOT __attribute__((hot))
#define CNS_COLD __attribute__((cold))

// 8B Contract enforcement
#define CNS_ASSERT_8B_COMPLIANT(struct_name) \
    _Static_assert(sizeof(struct_name) % CNS_WORD_UNITS == 0, \
    #struct_name " violates the 8-Bit Memory Contract: size must be a multiple of 8.")

// Core types
typedef uint64_t cns_word_t;
typedef uint64_t cns_ptr_t;
typedef uint64_t cns_size_t;
typedef uint64_t cns_offset_t;
typedef uint64_t cns_bitmask_t;
typedef uint64_t cns_hash_t;
typedef uint64_t cns_tick_t;

// Performance limits
#define CNS_TICK_LIMIT 8
#define CNS_MEMORY_QUANTUM 8
#define CNS_MAX_ARENA_SIZE (1024 * 1024 * 1024)

// System limits
#define CNS_MAX_COMMANDS 256
#define CNS_MAX_DOMAINS 64
#define CNS_MAX_PATTERNS 1024

#endif // CNS_CORE_H
```

## Validation

### Compile-Time Validation

1. **8B Contract**: All structs must pass `CNS_ASSERT_8B_COMPLIANT()`
2. **Alignment**: All aligned types must be properly aligned
3. **Size**: Core types must be exactly 64 bits

### Runtime Validation

1. **Memory Access**: Verify cache-line alignment
2. **Performance**: Measure basic operations for 8-tick compliance
3. **Consistency**: Ensure all components use the same core definitions

## Success Criteria

- [ ] `cns_core.h` compiles without warnings
- [ ] All structs pass 8B compliance checks
- [ ] Memory alignment works correctly
- [ ] Performance hints are properly applied
- [ ] Core types are consistently used throughout

## Next Steps

After completing Phase 0:
1. Move to [Phase 1: Port the Substrate](./phase1_substrate.md)
2. Use `cns_core.h` as the foundation for all subsequent phases
3. Validate 8B compliance at every step 