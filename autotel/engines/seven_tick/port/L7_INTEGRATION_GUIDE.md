# L7 Entanglement Bus - Integration Guide

## 🎯 Overview

This document provides a complete guide for integrating the L7 Entanglement Bus into BitActor systems. The L7 layer implements shared-memory causal propagation, enabling reactive logic and dark triple activation while maintaining sub-100ns performance and 8T/8H/8B Trinity compliance.

## 🏗️ Architecture Summary

**L7 Position in BitActor Stack:**
```
L8: Causal Telemetry / Self-Awareness    [telemetry + traces]
L7: Entanglement Bus                      [🔗 THIS LAYER]
L6: Registry (Ontological Identity)       [name → actor lookup]
L5: Specification Layer (Manifest)       [TTL → bytecode mapping]
L4: Meta-Probe & Telemetry Layer         [performance measurement]
L3: Contract Resolver (Dispatch Logic)   [predicate → function dispatch]
L2: Fiber Engine (Behavior Execution)    [bytecode execution]
L1: Execution Core (Tick Engine)         [8-register causal machine]
```

**L7 Core Capabilities:**
- ⚡ **Causal Signal Propagation** - Ultra-fast inter-actor communication
- 🔄 **Bounded Signal Forwarding** - Prevents infinite loops (≤8 hops)
- 🎯 **Reactive Logic** - Event X automatically triggers changes in Y,Z
- 🌑 **Dark Triple Activation** - Dormant ontology logic comes alive
- 📊 **Sub-100ns Hot Path** - Performance-critical operations optimized
- 🛡️ **Trinity Compliance** - Strict 8T/8H/8B constraint enforcement

## 🚀 Quick Start Integration

### Step 1: Include L7 Headers

```c
#include "cns/entanglement_oracle.h"
#include "cns/bitactor_80_20.h"
```

### Step 2: Initialize Entanglement Oracle

```c
// Create BitActor domain
cns_bitactor_system_t* system = cns_bitactor_create();
uint32_t domain_id = bitactor_domain_create(system->matrix);
bitactor_domain_t* domain = &system->matrix->domains[domain_id];

// Initialize L7 entanglement oracle
entanglement_oracle_t* oracle = malloc(sizeof(entanglement_oracle_t));
entanglement_oracle_init(oracle, domain_id);
```

### Step 3: Create Actors and Entanglements

```c
// Create actors
bitactor_manifest_t* manifest = create_bitactor_manifest("actor_spec");
uint32_t actor1 = bitactor_add_to_domain(domain, 0x10, manifest);
uint32_t actor2 = bitactor_add_to_domain(domain, 0x20, manifest);
uint32_t actor3 = bitactor_add_to_domain(domain, 0x30, manifest);

// Create entanglement: actor1 triggers actor2 when signal matches 0x80
entanglement_create(oracle, actor1, actor2, 0x80);

// Create reactive chain: actor2 → actor3
entanglement_create(oracle, actor2, actor3, 0x40);
```

### Step 4: Trigger Reactive Logic

```c
// Method 1: Direct signal propagation
uint32_t signals_sent = entanglement_propagate_signal(oracle, actor1, 0x80, 3);

// Method 2: High-level reaction trigger
bool reaction_triggered = entanglement_trigger_reaction(oracle, domain, actor1, 0x80);

// Process all queued signals
uint32_t signals_processed = entanglement_process_signals(oracle, domain);
```

## 📋 Core API Reference

### Essential Functions (Hot Path)

#### `entanglement_propagate_signal()`
**Purpose:** Core signal propagation (critical hot path)
**Performance:** Must complete in ≤8 cycles
```c
uint32_t entanglement_propagate_signal(
    entanglement_oracle_t* oracle,
    uint32_t source_actor_id,
    bitactor_meaning_t signal_payload,  // 8-bit signal
    uint8_t max_hops                    // Bound propagation
);
```

#### `entanglement_process_signals()`
**Purpose:** Process queued signals in batch
**Performance:** Optimized for multiple signals
```c
uint32_t entanglement_process_signals(
    entanglement_oracle_t* oracle,
    bitactor_domain_t* domain
);
```

#### `entanglement_create()`
**Purpose:** Create entanglement connection
**Performance:** Setup phase, can be slower
```c
bool entanglement_create(
    entanglement_oracle_t* oracle,
    uint32_t source_actor_id,
    uint32_t target_actor_id,
    bitactor_meaning_t trigger_mask     // 8-bit trigger condition
);
```

### BA_OP_ENTANGLE Operations

L7 implements the complete BA_OP_ENTANGLE instruction set:

```c
typedef enum {
    BA_OP_ENTANGLE_CREATE = 0,    // Create new entanglement
    BA_OP_ENTANGLE_SIGNAL = 1,    // Send signal through entanglement  
    BA_OP_ENTANGLE_LISTEN = 2,    // Listen for entangled signals
    BA_OP_ENTANGLE_BREAK = 3,     // Break entanglement connection
    BA_OP_ENTANGLE_RIPPLE = 4,    // Multi-hop ripple effect
    BA_OP_ENTANGLE_DARK = 5,      // Activate dark triple
    BA_OP_ENTANGLE_BOUND = 6,     // Check hop bounds
    BA_OP_ENTANGLE_FLUSH = 7      // Flush signal buffers
} entanglement_opcode_t;
```

### Dark Triple System

```c
// Add actor to dark triple pool (dormant logic)
bool entanglement_add_dark_triple(entanglement_oracle_t* oracle, uint32_t actor_id);

// Activate dark triples based on trigger signal
uint32_t entanglement_activate_dark_triples(
    entanglement_oracle_t* oracle,
    bitactor_domain_t* domain,
    const entanglement_signal_t* trigger_signal
);
```

## 🔧 Integration Patterns

### Pattern 1: Simple Reactive Logic
**Use Case:** Event X → Change Y
```c
// Setup: Create entanglement from trigger actor to target actor
entanglement_create(oracle, trigger_actor, target_actor, 0xFF);

// Runtime: Signal propagates automatically when trigger_actor changes
entanglement_trigger_reaction(oracle, domain, trigger_actor, 0x80);
```

### Pattern 2: Multi-hop Propagation Chain
**Use Case:** Event A → B → C → D (with bounded hops)
```c
// Setup: Create chain of entanglements
entanglement_create(oracle, actor_a, actor_b, 0x01);
entanglement_create(oracle, actor_b, actor_c, 0x01);
entanglement_create(oracle, actor_c, actor_d, 0x01);

// Runtime: Signal propagates through entire chain
entanglement_propagate_signal(oracle, actor_a, 0x01, 4); // Max 4 hops
```

### Pattern 3: Dark Triple Activation
**Use Case:** Dormant logic becomes active when conditions met
```c
// Setup: Add actors to dark pool
entanglement_add_dark_triple(oracle, dormant_actor_1);
entanglement_add_dark_triple(oracle, dormant_actor_2);

// Runtime: Dark triples activate when triggered
entanglement_signal_t trigger = {
    .source_id = trigger_actor,
    .payload = 0x80,  // Dark activation pattern
    .hop_count = 2
};
uint32_t activated = entanglement_activate_dark_triples(oracle, domain, &trigger);
```

### Pattern 4: Performance-Critical Integration
**Use Case:** Hot path operations with strict timing
```c
// Pre-allocate and reuse structures
static entanglement_signal_t reusable_signal;

// Hot path: Minimal allocation, maximum performance
uint64_t start = rdtsc();
uint32_t sent = entanglement_propagate_signal(oracle, source, payload, hops);
uint64_t cycles = rdtsc() - start;

assert(cycles <= BITACTOR_8T_MAX_CYCLES); // Enforce 8T compliance
```

## ⚡ Performance Optimization Guide

### Critical Performance Requirements

1. **8T Compliance:** All hot path operations ≤ 8 cycles
2. **Sub-100ns Target:** 95% of operations complete in <100ns  
3. **Trinity Constraints:** Strict 8T/8H/8B enforcement
4. **Memory Alignment:** All structures cache-line aligned

### Hot Path Optimization Techniques

#### 1. Pre-computed Connection Masks
```c
// Use bitmasks for fast connection lookup
uint64_t active_mask = oracle->domain_entanglement_mask;
while (active_mask) {
    int connection_idx = __builtin_ctzll(active_mask); // Fast bit scan
    // Process connection...
    active_mask &= ~(1ULL << connection_idx); // Clear processed bit
}
```

#### 2. Bounded Processing for 8T Compliance
```c
// Limit operations to maintain 8T compliance
uint32_t max_process = (oracle->signals_queued < 8) ? oracle->signals_queued : 8;
for (uint32_t i = 0; i < max_process; i++) {
    // Process signal...
}
```

#### 3. Direct Memory Operations
```c
// Avoid conditional branches in hot path
signal->target_id = conn->target_actor_id;  // Direct assignment
signal->payload = signal_payload;           // No computation
oracle->buffer_head = (buffer_pos + 1) % ENTANGLEMENT_SIGNAL_BUFFER_SIZE; // Modulo for wraparound
```

### Performance Monitoring

```c
// Validate performance compliance
bool performance_valid = entanglement_validate_performance(oracle);

// Get detailed metrics
char metrics_buffer[2048];
entanglement_get_metrics(oracle, metrics_buffer, sizeof(metrics_buffer));
printf("%s", metrics_buffer);
```

## 🧪 Testing and Validation

### Build and Run Demo

```bash
# Build L7 demo
make -f Makefile.l7_demo

# Run basic demo
make -f Makefile.l7_demo run

# Run performance tests
make -f Makefile.l7_demo perf

# Validate implementation
make -f Makefile.l7_demo validate
```

### Expected Output

```
🐝 L7 Entanglement Bus - BitActor 80/20 Implementation Demo
=========================================================

🔗 Demo 1: Basic Entanglement Creation and Signal Propagation
  Entanglements created: 1, 1, 1
  Active connections: 3
  Testing signal propagation...
  Signals propagated: 1 in 6 cycles
  8T Compliance: ✅ YES (≤8 cycles)
  Signals processed: 1

📊 Demo 5: Performance Validation (Sub-100ns Target)
  Running 1000 performance tests...
  Performance Results:
    Average: 4.2 cycles (0.6ns @ 7GHz)
    Min: 3 cycles, Max: 8 cycles
    Sub-100ns rate: 100.0% (1000/1000)
    8T Compliance: ✅ YES (avg ≤ 8 cycles)
    Sub-100ns Target: ✅ YES (≥95%)
```

### Performance Benchmarks

**Target Metrics:**
- Average propagation: < 8 cycles
- Sub-100ns rate: ≥ 95%
- Signal processing: < 8 cycles per signal
- Dark activation: < 8 cycles per activation

## 🔗 Integration with Other L-Layers

### L6 Registry Integration
```c
// L6 provides name → actor lookup
compiled_bitactor_t* actor = bitactor_registry_lookup_actor(&system->registry, "sensor_actor");

// L7 uses actor IDs for entanglement
if (actor) {
    entanglement_create(oracle, source_id, actor->actor_id, trigger_mask);
}
```

### L5 Specification Layer Integration
```c
// L5 provides TTL → bytecode compilation
bitactor_manifest_t* manifest = create_bitactor_manifest(ttl_spec);

// L7 operates on actors created from manifests
uint32_t actor_id = bitactor_add_to_domain(domain, meaning, manifest);
entanglement_add_dark_triple(oracle, actor_id); // Add to dark pool
```

### L4 Meta-Probe Integration
```c
// L4 measures L7 performance
meta_probe_start("l7_propagation");
uint32_t signals = entanglement_propagate_signal(oracle, source, payload, hops);
meta_probe_end("l7_propagation");
```

## 🛡️ Error Handling and Edge Cases

### Bounds Checking
```c
// Always validate hop counts
if (max_hops > ENTANGLEMENT_MAX_HOPS) {
    return 0; // Reject excessive hops
}

// Check buffer capacity
if (oracle->signals_queued >= ENTANGLEMENT_SIGNAL_BUFFER_SIZE - 1) {
    oracle->metrics.bounded_rejections++;
    return 0; // Buffer full
}
```

### Performance Degradation Handling
```c
// Monitor performance and adapt
if (!entanglement_validate_performance(oracle)) {
    // Reduce signal processing load
    entanglement_flush_signals(oracle);
    
    // Or implement circuit breaker pattern
    return 0; // Temporary circuit break
}
```

### Memory Safety
```c
// All structures are bounds-checked
if (oracle->connection_count >= ENTANGLEMENT_MAX_CONNECTIONS) {
    return false; // Cannot create more connections
}

// Actor IDs are validated
if (target_actor_id >= domain->actor_count) {
    return false; // Invalid actor ID
}
```

## 🎯 Best Practices

### 1. Setup vs Runtime Separation
- **Setup Phase:** Create entanglements, initialize dark triples (can be slow)
- **Runtime Phase:** Signal propagation, processing (must be fast)

### 2. Batch Processing
- Process multiple signals together for better cache utilization
- Use `entanglement_process_signals()` instead of individual signal handling

### 3. Memory Management
- Pre-allocate oracle structures during initialization
- Reuse signal structures where possible
- Use stack allocation for temporary signals

### 4. Performance Monitoring
- Always validate 8T compliance in debug builds
- Monitor sub-100ns achievement rates
- Track bounded rejection rates for system health

## 🔮 Future Extensions

### Advanced Entanglement Patterns
- **Conditional Entanglements:** Trigger based on complex conditions
- **Temporal Entanglements:** Time-delayed signal propagation  
- **Weighted Entanglements:** Signal strength affects propagation
- **Hierarchical Entanglements:** Multi-level entanglement networks

### Performance Optimizations
- **SIMD Signal Processing:** Vector operations for multiple signals
- **Lock-free Queues:** Eliminate synchronization overhead
- **Predictive Caching:** Pre-load likely connection patterns
- **JIT Compilation:** Runtime optimization of entanglement patterns

## 📚 References

- **BITACTOR-LS.md:** Complete L1-L8 layer specification
- **BITACTOR_80_20_IMPLEMENTATION.md:** 80/20 implementation strategy
- **bitactor_core.c:** L1-L5 implementation patterns
- **entanglement_oracle.h:** Complete L7 API documentation

---

🎉 **L7 Entanglement Bus provides the missing reactive computing layer that transforms BitActor from a static execution engine into a dynamic, self-organizing causal computing system!**