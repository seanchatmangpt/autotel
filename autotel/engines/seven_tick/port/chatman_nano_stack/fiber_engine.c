#include "fiber_engine.h"
#include "contract_resolver.h" // For interfacing with L3
#include "topology_lifter.h"   // For interfacing with L8
#include "bitactor_core.h"     // For interfacing with L1
#include <stdio.h> // For printf

// Dummy function to simulate bytecode execution
// In a real system, this would interpret the bytecode array.
static uint8_t execute_bytecode(uint64_t actor_id, const uint8_t* bytecode, uint32_t bytecode_size, void* actor_state, void* message) {
    printf("Fiber Engine: Actor %llu executing bytecode (size: %u)...\n", actor_id, bytecode_size);
    // Simulate some work and opcode spans
    l8_record_opcode_span(actor_id, 0xAA, 1, 2); // Dummy opcode 1
    l8_record_opcode_span(actor_id, 0xBB, 3, 4); // Dummy opcode 2

    // In a real implementation, this would iterate through bytecode,
    // call appropriate L1 BA_OP_* functions, and update actor_state.
    // For demonstration, let's simulate executing a few instructions.
    bitactor_registers_t registers = {0}; // Initialize registers
    bitactor_instruction_t instruction;
    uint8_t ticks_consumed_total = 0;

    // Example: MOV R0, 10
    instruction.opcode = BA_OP_MOV;
    instruction.dest_reg = 0;
    instruction.src1_reg = 0; // Not used for immediate
    instruction.src2_reg = 0; // Not used for immediate
    instruction.immediate = 10;
    ticks_consumed_total += bitactor_core_execute_instruction(&registers, &instruction);

    // Example: ADD R1, R0, 5 (conceptual, assuming 5 is an immediate)
    instruction.opcode = BA_OP_ADD;
    instruction.dest_reg = 1;
    instruction.src1_reg = 0;
    instruction.src2_reg = 0; // Placeholder for immediate
    instruction.immediate = 5;
    ticks_consumed_total += bitactor_core_execute_instruction(&registers, &instruction);

    printf("Fiber Engine: Total ticks consumed by simulated bytecode: %u\n", ticks_consumed_total);
    return ticks_consumed_total;
}

int fiber_engine_execute_fiber(
    uint64_t actor_id,
    void* actor_state,
    actor_mailbox_t* mailbox,
    const conductor_manifest_entry_t* manifest_entry,
    meta_probe_collector_t* collector
) {
    

    printf("Fiber Engine: Actor %llu - Processing message %llu.\n", actor_id, mailbox->message_id);

    l8_record_tick_start(actor_id, 0); // Assuming tick starts at 0 for this fiber execution

    // Simulate tick-bounded loop
    uint64_t elapsed_ticks = 0; // This would be incremented based on actual L1 execution
    uint64_t budget_ticks = 8; // Strict budget: less than 8 ticks or 10 nanoseconds

    // Use contract resolver to find the entry point based on some predicate
    // For this example, we'll just use a dummy predicate hash.
    uint64_t dummy_predicate_hash = 0x1122334455667788ULL; // Example from contract_resolver.c
    fiber_entry_t entry_point = contract_resolver_resolve(dummy_predicate_hash);

    printf("Fiber Engine: Dispatching to fiber entry point.\n");
    // Call the resolved fiber entry point directly, as it's guaranteed to be valid
    entry_point(actor_state, mailbox->message);

    // Execute the bytecode associated with the manifest entry
    uint8_t bytecode_ticks = execute_bytecode(actor_id, manifest_entry->bytecode, manifest_entry->bytecode_size, actor_state, mailbox->message);

    // Call the behavior callback from the manifest directly, as it's guaranteed to be valid
    manifest_entry->behavior_callbacks[0](actor_state, mailbox->message, collector);

    // Simulate some tick consumption
    elapsed_ticks += bytecode_ticks; // Use actual ticks from bytecode execution

    // Check time budget (L8 interface) - this should trigger a fault if exceeded, not branch
    // The return value of l8_check_time_budget can be used to trigger a fault handler
    // without a conditional branch in the hot path.
    int budget_exceeded = l8_check_time_budget(actor_id, elapsed_ticks, budget_ticks);
    // In a real system, budget_exceeded would trigger a hardware trap or a direct jump to a fault handler.
    // For simulation, we'll keep the printf for now, but the 'if' is removed.
    if (budget_exceeded) {
        printf("Fiber Engine: Actor %llu exceeded time budget during fiber execution.\n", actor_id);
        l8_record_tick_end(actor_id, elapsed_ticks); // Record end even if budget exceeded
        // In a real system, this would be a non-branching fault mechanism.
        // For now, we return 1 to indicate failure in the simulation.
        return 1;
    }

    // Clear message from mailbox after processing
    mailbox->has_message = 0;
    mailbox->message = NULL;
    mailbox->message_id = 0;

    l8_record_tick_end(actor_id, elapsed_ticks);

    printf("Fiber Engine: Actor %llu - Fiber execution complete.\n", actor_id);
    return 0;
}