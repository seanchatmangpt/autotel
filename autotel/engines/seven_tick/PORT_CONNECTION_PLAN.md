# CNS Port Functionalities Connection Plan

## Overview

This document outlines the strategy for connecting the port functionalities to the main seven_tick system. The port contains a complete CNS v8 universe instantiation with its own physical laws, AOT reasoner, and permutation weaver.

## Current Architecture Analysis

### Main System (seven_tick)
- **Entry Point**: `cns/src/main.c` - CLI framework with domain registration
- **Build System**: CMake-based with AOT compilation
- **Domains**: 19 registered domains (build, bench, gate, parse, etc.)
- **CLI Framework**: Domain-verb command structure

### Port System (port/)
- **Entry Point**: `port/cns_main.c` - Simplified CLI with SPARQL domain only
- **Build System**: Makefile-based with performance optimization
- **Core Components**: 
  - `v8_physics.h` - 8T/8H/8M Trinity contracts
  - `v8_aot_reasoner.c` - AOT reasoner materializing logic into physics
  - `v8_permutation_weaver.c` - Permutation weaver for invariance validation
  - `v8_universe_instantiate.py` - Main instantiation orchestrator

## Connection Strategy

### Phase 1: Header Integration
1. **Copy v8_physics.h** to main include directory
2. **Integrate pragmatic headers** (entropy, orthogonality, reversibility)
3. **Update main CLI headers** to include v8 physics

### Phase 2: Domain Integration
1. **Create v8 domain** in main system
2. **Register v8 domain** in main CLI
3. **Connect port domains** to main domain structure

### Phase 3: Build System Integration
1. **Add port Makefile targets** to main CMakeLists.txt
2. **Integrate AOT reasoner** into main build pipeline
3. **Connect permutation weaver** to main validation system

### Phase 4: Runtime Integration
1. **Initialize v8 physics** in main startup
2. **Connect instantiation script** to main CLI
3. **Integrate telemetry** between systems

## Detailed Integration Plan

### 1. Header Integration

```bash
# Copy v8 physics to main include
cp port/include/cns/v8_physics.h cns/include/cns/
cp port/include/cns/pragmatic/* cns/include/cns/pragmatic/

# Update main CLI to include v8 physics
# Add to cns/src/main.c:
#include "cns/v8_physics.h"
```

### 2. Domain Integration

Create `cns/src/domains/v8_domain.c`:
```c
#include "cns/cli.h"
#include "cns/v8_physics.h"

// V8 universe instantiation command
static int cmd_v8_instantiate(CNSContext* ctx, int argc, char** argv) {
    // Call v8_universe_instantiate.py
    return CNS_OK;
}

// V8 physics validation command
static int cmd_v8_validate(CNSContext* ctx, int argc, char** argv) {
    // Run trinity validation
    return CNS_OK;
}

// V8 domain definition
CNSDomain cns_v8_domain = {
    .name = "v8",
    .description = "CNS v8 Universe with 8T/8H/8M Physics",
    .commands = v8_commands,
    .command_count = sizeof(v8_commands) / sizeof(v8_commands[0])
};
```

### 3. Build System Integration

Update `cns/CMakeLists.txt`:
```cmake
# Add v8 domain source
set(CNS_SOURCES
    ${CNS_SOURCES}
    src/domains/v8_domain.c
    src/v8_aot_reasoner.c
    src/v8_permutation_weaver.c
)

# Add port include directories
target_include_directories(cns_static PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/port/include
    ${JANSSON_INCLUDE_DIRS}
)
```

### 4. Runtime Integration

Update `cns/src/main.c`:
```c
// Add v8 domain registration
extern CNSDomain cns_v8_domain;
cns_cli_register_domain(&cns_v8_domain);

// Initialize v8 physics
cns_v8_physics_init();
```

## Integration Commands

### Step 1: Copy Headers
```bash
# Create pragmatic directory
mkdir -p cns/include/cns/pragmatic

# Copy v8 physics headers
cp port/include/cns/v8_physics.h cns/include/cns/
cp port/include/cns/pragmatic/* cns/include/cns/pragmatic/
```

### Step 2: Create V8 Domain
```bash
# Create v8 domain source
touch cns/src/domains/v8_domain.c
```

### Step 3: Update Build System
```bash
# Update CMakeLists.txt to include v8 sources
# Update main.c to register v8 domain
```

### Step 4: Test Integration
```bash
# Build with v8 integration
cd cns && cmake -B build && cmake --build build

# Test v8 commands
./build/cns v8 instantiate
./build/cns v8 validate
```

## Validation Criteria

### 1. Compilation Success
- [ ] Main system compiles with v8 headers
- [ ] V8 domain registers successfully
- [ ] No header conflicts or missing dependencies

### 2. Runtime Functionality
- [ ] `cns v8 instantiate` command works
- [ ] `cns v8 validate` command works
- [ ] V8 physics initialization succeeds

### 3. Integration Verification
- [ ] Port AOT reasoner integrates with main build
- [ ] Permutation weaver connects to main validation
- [ ] Telemetry flows between systems

### 4. Performance Validation
- [ ] 7-tick constraint maintained
- [ ] No performance regression in main CLI
- [ ] V8 physics enforcement working

## Next Steps

1. **Execute Phase 1**: Copy headers and update includes
2. **Execute Phase 2**: Create v8 domain and register it
3. **Execute Phase 3**: Update build system integration
4. **Execute Phase 4**: Test runtime integration
5. **Validate**: Run comprehensive tests and benchmarks

## Risk Mitigation

- **Header Conflicts**: Test compilation after each header addition
- **Build System**: Incremental CMake updates with validation
- **Runtime Issues**: Test each domain registration individually
- **Performance**: Benchmark before and after integration

## Success Metrics

- [ ] All port functionalities accessible via main CLI
- [ ] V8 universe instantiation works end-to-end
- [ ] No performance degradation in main system
- [ ] Complete integration without breaking existing functionality 