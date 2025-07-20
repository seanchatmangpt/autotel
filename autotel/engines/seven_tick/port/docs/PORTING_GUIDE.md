# CNS Architecture Porting Guide

## Executive Summary

Porting a system with such a strong architectural philosophy requires a specific, principled approach. The goal is not merely to copy code, but to recreate the system's core physics and logic in the new CNS v8 codebase.

## Guiding Methodology: Port Concepts, Not Just Code

The most critical instruction is this: at each step, you are porting an architectural principle. The files are the evidence and implementation of that principle. Use the provided code as a blueprint to reconstruct the logic, not as a blind source to be copied. Validate performance and compliance at every phase.

## Phase Overview

### Phase 0: Establish the Architectural Axioms (The Non-Negotiable Foundation)
- Define the physical laws of the CNS v8 universe
- Establish the 8B memory contract as a compile-time certainty
- Create the canonical header that serves as the constitution

### Phase 1: Port the Substrate (Memory and Control)
- Implement deterministic, 8-tick compliant memory management
- Build the command and control plane
- Port the arena allocator and AOT generators

### Phase 2: Port the AOT Toolchain and Semantic Engines
- Implement "The Reasoner is the Build System"
- Transpile high-level semantic specifications into optimized C code
- Port OWL, SHACL, and SPARQL as bitmask processors

### Phase 3: Port the Meta-Validation and Operational Systems
- Implement the 8th Hop of the cognitive cycle
- Ensure system robustness and self-regulation
- Create deployable, functional whole

## Success Criteria

At each phase, validate:
1. **8B Memory Contract Compliance**: All structures align with 64-bit hardware words
2. **8-Tick Performance**: Operations complete within 8 CPU cycles
3. **AOT Compilation**: Specifications become optimized C code
4. **Self-Validation**: System can verify its own integrity
5. **User Experience**: End-to-end functionality from user perspective

## File Organization

- `./port/docs/phases/` - Detailed phase-by-phase implementation guides
- `./port/docs/principles/` - Architectural principles and philosophy
- `./port/docs/implementation/` - Code examples and implementation details
- `./port/docs/validation/` - Testing and validation procedures

## Next Steps

1. Read the [Architectural Principles](./principles/README.md)
2. Follow [Phase 0: Establish Axioms](./phases/phase0_axioms.md)
3. Proceed through each phase systematically
4. Validate at each step using the [Validation Guide](./validation/README.md) 