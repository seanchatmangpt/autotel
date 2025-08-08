Files identified as part of or directly related to the Chatman Nano Stack:

**Core Chatman Nano Stack Components (Found and Analyzed):**
*   `/Users/sac/autotel/autotel/engines/seven_tick/port/chatman_nano_stack/bitactor_core.c` and `/Users/sac/autotel/autotel/engines/seven_tick/port/chatman_nano_stack/bitactor_core.h`: These files define the core instruction set and execution logic for BitActor, including opcodes like `ADD`, `MOV`, `ENTANGLE`, and `COLLAPSE`. This directly supports the "Direct BitActor Interaction" and "Low-Level Event Handling (L0-L1)" aspects of the Chatman Nano Stack.
*   `/Users/sac/autotel/autotel/engines/seven_tick/port/chatman_nano_stack/fiber_engine.c` and `/Users/sac/autotel/autotel/engines/seven_tick/port/chatman_nano_stack/fiber_engine.h`: These files orchestrate the execution of an actor's behavior, calling `bitactor_core_execute_instruction` and interacting with the conductor manifest and topology lifter. This demonstrates "Direct BitActor Interaction" and "Low-Level Event Handling (L0-L1)".
*   `/Users/sac/autotel/autotel/engines/seven_tick/port/chatman_nano_stack/conductor_manifest.c` and `/Users/sac/autotel/autotel/engines/seven_tick/port/chatman_nano_stack/conductor_manifest.h`: These files implement the "Local Manifest/Lookup" functionality, storing pre-materialized TTL signature hints and function pointers for actor behaviors.
*   `/Users/sac/autotel/autotel/engines/seven_tick/port/chatman_nano_stack/topology_lifter.c` and `/Users/sac/autotel/autotel/engines/seven_tick/port/chatman_nano_stack/topology_lifter.h`: These files provide "Local Telemetry/Topology Lifting" by recording tick and opcode spans and conceptually converting traces to TTL. This supports pre-processing raw input signals for causal traceability.
*   `/Users/sac/autotel/autotel/engines/seven_tick/port/build_chatman_nano_stack.sh`: This unified build script confirms the overarching "Chatman Nano Stack" concept and its integration with BitActor and Turtle/RDF processing.
*   `/Users/sac/autotel/autotel/engines/seven_tick/port/chatman_nano_stack/include/cns/cns_contracts.h`: Defines "Design by Contract (DbC) Substrate for the Chatman Nano Stack," enforcing the 8-Tick Time Model and 8-Bit Memory Model, directly addressing "Limited Memory Footprint" and performance constraints.
*   `/Users/sac/autotel/autotel/engines/seven_tick/port/chatman_nano_stack/include/cns/cns_core.h`: Described as "The AHI-Level Core Substrate for the Chatman Nano Stack," defining core types and architectural axioms (8T/8H/8B), reinforcing "microcontroller-scale memory" and "minimal overhead."
*   `/Users/sac/autotel/autotel/engines/seven_tick/port/chatman_nano_stack/src/bitactor_ttl_compiler.c`: Compiles TTL specifications to BitActor bytecode within 8 CPU ticks, using "Chatman Nano Stack patterns" for physics-compliant compilation. This file is a strong candidate for "Signal Decoding and Routing."

**Other Related Components (from previous search):**
*   `dark_feature_activator.c`: Implies connection to Chatman architecture.
*   `domains/deploy.c`: Explicitly mentions "CHATMAN NANO-STACK - 7-tick performance CLI" and `seanchatman/cns`.
*   `domains/docs.c`: Documentation for "CHATMAN NANO-STACK performance-first CLI".
*   `examples/nano_stack_demo.c`: A demo specifically for "CHATMAN-NANO-STACK".
*   `examples/pattern_composition.c`: References "CHATMAN-NANO-STACK patterns".
*   `examples/s7t_demo.c`: Another demo for "CHATMAN-NANO-STACK".
*   `src/bitactor_80_20.c`, `src/bitactor_80_20_optimized.c`, `src/bitactor_bridge.c`, `src/bitactor_l2_l3_production.c`, `src/bitactor_l_stack_demo.c`, `src/bitactor_ls.c`, `src/l7_entanglement_demo.c`, `tests/bitactor_80_20_test.c`, `tests/bitactor_ls_test.c`: These files contain references to `bitactor_manifest_t`, `bitactor_collapse`, and `create_bitactor_manifest`, which are crucial for "Direct BitActor Interaction" and "Local Manifest/Lookup".
*   `main.c`: This file orchestrates the interaction between `conductor_manifest.h` and `fiber_engine.h`, suggesting it's a key part of the CNS.

**Components mentioned in the Chatman Nano Stack description but not explicitly found as separate files (may be integrated into existing files or not yet implemented):**
*   `chatman_nano_ring.c` or `nano_ringbuffer` (for Lightweight Memory Arena): The `allocate_from_arena` function in `chatman_nano_stack/src/bitactor_80_20.c` currently uses `malloc` as a placeholder. While `cns_arena_t` exists in `cns_core.h`, a dedicated `nano_ringbuffer` implementation for zero-copy event routing is not explicitly present in the current codebase within this scope.
*   `chatman_signal_decoder.c`: Functionality likely integrated into `bitactor_ttl_compiler.c`.
*   `chatman_fastbus.c` (for Signal Decoding and Routing - "FastPath router"): Functionality might be integrated into `bitactor_ttl_compiler.c` or other BitActor components.
*   `chatman_event_loop.c`: The `fiber_engine.c` handles event-loop-like behavior by processing messages and executing fibers. It's possible this is the intended "event loop" or a core part of it.
*   `chatman_topology_lifter.c`: This was found as `topology_lifter.c` and `topology_lifter.h`.

Further investigation would be needed to determine if the missing components are integrated into the existing files or need to be created.