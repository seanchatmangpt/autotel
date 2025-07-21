#include "bitactor_core.h"
#include <stdio.h> // For printf (for debugging/simulation purposes)

// Define a maximum tick budget for a single instruction
#define MAX_INSTRUCTION_TICKS 7

uint8_t bitactor_core_execute_instruction(
    bitactor_registers_t* registers,
    const bitactor_instruction_t* instruction
) {
    uint8_t ticks_consumed = 0; // Initialize ticks consumed

    // In a real hardware implementation, this would involve actual CPU cycle counting
    // or a fixed-cycle execution pipeline. For simulation, we assign a fixed cost.

    if (!registers || !instruction) {
        fprintf(stderr, "BitActor Core: Invalid registers or instruction pointer.\n");
        return MAX_INSTRUCTION_TICKS + 1; // Indicate error by exceeding budget
    }

    // Ensure register indices are valid
    if (instruction->dest_reg >= 8 || instruction->src1_reg >= 8 || instruction->src2_reg >= 8) {
        fprintf(stderr, "BitActor Core: Invalid register index in instruction.\n");
        return MAX_INSTRUCTION_TICKS + 1; // Indicate error
    }

    // Pointer to the destination register
    uint64_t* dest_reg_ptr = &registers->R0 + instruction->dest_reg;
    // Values of source registers
    uint64_t src1_val = *(&registers->R0 + instruction->src1_reg);
    uint64_t src2_val = *(&registers->R0 + instruction->src2_reg);

    switch (instruction->opcode) {
        case BA_OP_NOOP:
            ticks_consumed = 1; // Minimal operation
            break;
        case BA_OP_ADD:
            *dest_reg_ptr = src1_val + src2_val;
            ticks_consumed = 2;
            break;
        case BA_OP_SUB:
            *dest_reg_ptr = src1_val - src2_val;
            ticks_consumed = 2;
            break;
        case BA_OP_AND:
            *dest_reg_ptr = src1_val & src2_val;
            ticks_consumed = 1;
            break;
        case BA_OP_OR:
            *dest_reg_ptr = src1_val | src2_val;
            ticks_consumed = 1;
            break;
        case BA_OP_XOR:
            *dest_reg_ptr = src1_val ^ src2_val;
            ticks_consumed = 1;
            break;
        case BA_OP_MOV:
            *dest_reg_ptr = src1_val; // src2_reg or immediate could be used for different MOV types
            ticks_consumed = 1;
            break;
        case BA_OP_ENTANGLE:
            // Conceptual entanglement operation
            // In a real system, this would trigger L7 Entanglement Bus operations.
            printf("BitActor Core: Executing BA_OP_ENTANGLE (conceptual).\n");
            ticks_consumed = 5; // More complex operation
            break;
        case BA_OP_JUMP_IF_ZERO:
            // Conceptual jump operation
            // In a real system, this would modify the program counter (R7).
            if (src1_val == 0) {
                printf("BitActor Core: Executing BA_OP_JUMP_IF_ZERO (jump taken).\n");
                // Modify R7 (PC) here if it were a real PC
            } else {
                printf("BitActor Core: Executing BA_OP_JUMP_IF_ZERO (no jump).\n");
            }
            ticks_consumed = 3;
            break;
        case BA_OP_JUMP_IF_NOT_ZERO:
            // Conceptual jump operation
            if (src1_val != 0) {
                printf("BitActor Core: Executing BA_OP_JUMP_IF_NOT_ZERO (jump taken).\n");
                // Modify R7 (PC) here
            } else {
                printf("BitActor Core: Executing BA_OP_JUMP_IF_NOT_ZERO (no jump).\n");
            }
            ticks_consumed = 3;
            break;
        case BA_OP_COLLAPSE:
            // Conceptual collapse operation
            // In a real system, this would involve L4/L8 telemetry and state finalization.
            printf("BitActor Core: Executing BA_OP_COLLAPSE (conceptual).\n");
            ticks_consumed = 7; // Most complex operation
            break;
        default:
            fprintf(stderr, "BitActor Core: Unknown opcode 0x%02X.\n", instruction->opcode);
            ticks_consumed = MAX_INSTRUCTION_TICKS + 1; // Indicate error
            break;
    }

    // Enforce the 8-tick constraint (less than 8 ticks)
    if (ticks_consumed >= MAX_INSTRUCTION_TICKS) {
        fprintf(stderr, "BitActor Core: WARNING! Instruction exceeded %u tick budget. Consumed: %u\n",
                MAX_INSTRUCTION_TICKS, ticks_consumed);
        // In a real system, this might trigger a fault or a slower path.
    }

    return ticks_consumed;
}