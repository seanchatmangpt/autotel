#include "bitactor_core.h"
#include <stdio.h> // For printf (for debugging/simulation purposes)

// Define a maximum tick budget for a single instruction
#define MAX_INSTRUCTION_TICKS 7

// Function prototypes for each opcode handler
static uint8_t handle_noop(bitactor_registers_t* registers, const bitactor_instruction_t* instruction);
static uint8_t handle_add(bitactor_registers_t* registers, const bitactor_instruction_t* instruction);
static uint8_t handle_sub(bitactor_registers_t* registers, const bitactor_instruction_t* instruction);
static uint8_t handle_and(bitactor_registers_t* registers, const bitactor_instruction_t* instruction);
static uint8_t handle_or(bitactor_registers_t* registers, const bitactor_instruction_t* instruction);
static uint8_t handle_xor(bitactor_registers_t* registers, const bitactor_instruction_t* instruction);
static uint8_t handle_mov(bitactor_registers_t* registers, const bitactor_instruction_t* instruction);
static uint8_t handle_entangle(bitactor_registers_t* registers, const bitactor_instruction_t* instruction);
static uint8_t handle_jump_if_zero(bitactor_registers_t* registers, const bitactor_instruction_t* instruction);
static uint8_t handle_jump_if_not_zero(bitactor_registers_t* registers, const bitactor_instruction_t* instruction);
static uint8_t handle_collapse(bitactor_registers_t* registers, const bitactor_instruction_t* instruction);
static uint8_t handle_unknown_opcode(bitactor_registers_t* registers, const bitactor_instruction_t* instruction);

// Array of function pointers for opcode dispatch (jump table)
typedef uint8_t (*opcode_handler_t)(bitactor_registers_t*, const bitactor_instruction_t*);

static opcode_handler_t opcode_handlers[] = {
    handle_noop,             // 0x00 BA_OP_NOOP
    handle_add,              // 0x01 BA_OP_ADD
    handle_sub,              // 0x02 BA_OP_SUB
    handle_and,              // 0x03 BA_OP_AND
    handle_or,               // 0x04 BA_OP_OR
    handle_xor,              // 0x05 BA_OP_XOR
    handle_mov,              // 0x06 BA_OP_MOV
    handle_entangle,         // 0x07 BA_OP_ENTANGLE
    handle_jump_if_zero,     // 0x08 BA_OP_JUMP_IF_ZERO
    handle_jump_if_not_zero, // 0x09 BA_OP_JUMP_IF_NOT_ZERO
    handle_collapse          // 0x0A BA_OP_COLLAPSE
    // Add more handlers as opcodes are defined
};

// Helper to get register value by index
static inline uint64_t get_reg_val(bitactor_registers_t* registers, uint8_t index) {
    return *(&registers->R0 + index);
}

// Helper to set register value by index
static inline void set_reg_val(bitactor_registers_t* registers, uint8_t index, uint64_t value) {
    *(&registers->R0 + index) = value;
}

// Opcode Handlers
static uint8_t handle_noop(bitactor_registers_t* registers, const bitactor_instruction_t* instruction) {
    return 1; // Minimal operation
}

static uint8_t handle_add(bitactor_registers_t* registers, const bitactor_instruction_t* instruction) {
    set_reg_val(registers, instruction->dest_reg, get_reg_val(registers, instruction->src1_reg) + get_reg_val(registers, instruction->src2_reg));
    return 2;
}

static uint8_t handle_sub(bitactor_registers_t* registers, const bitactor_instruction_t* instruction) {
    set_reg_val(registers, instruction->dest_reg, get_reg_val(registers, instruction->src1_reg) - get_reg_val(registers, instruction->src2_reg));
    return 2;
}

static uint8_t handle_and(bitactor_registers_t* registers, const bitactor_instruction_t* instruction) {
    set_reg_val(registers, instruction->dest_reg, get_reg_val(registers, instruction->src1_reg) & get_reg_val(registers, instruction->src2_reg));
    return 1;
}

static uint8_t handle_or(bitactor_registers_t* registers, const bitactor_instruction_t* instruction) {
    set_reg_val(registers, instruction->dest_reg, get_reg_val(registers, instruction->src1_reg) | get_reg_val(registers, instruction->src2_reg));
    return 1;
}

static uint8_t handle_xor(bitactor_registers_t* registers, const bitactor_instruction_t* instruction) {
    set_reg_val(registers, instruction->dest_reg, get_reg_val(registers, instruction->src1_reg) ^ get_reg_val(registers, instruction->src2_reg));
    return 1;
}

static uint8_t handle_mov(bitactor_registers_t* registers, const bitactor_instruction_t* instruction) {
    set_reg_val(registers, instruction->dest_reg, get_reg_val(registers, instruction->src1_reg)); // Assuming src1 is the source
    return 1;
}

static uint8_t handle_entangle(bitactor_registers_t* registers, const bitactor_instruction_t* instruction) {
    // Conceptual entanglement operation
    // In a real system, this would trigger L7 Entanglement Bus operations.
    printf("BitActor Core: Executing BA_OP_ENTANGLE (conceptual).\n");
    return 5; // More complex operation
}

static uint8_t handle_jump_if_zero(bitactor_registers_t* registers, const bitactor_instruction_t* instruction) {
    // Conceptual jump operation without branching
    // In a real system, this would modify the program counter (R7) based on a conditional move or bitwise logic.
    // For simulation, we'll just print the outcome.
    uint64_t src1_val = get_reg_val(registers, instruction->src1_reg);
    printf("BitActor Core: Executing BA_OP_JUMP_IF_ZERO. Condition (R%u == 0): %s\n",
           instruction->src1_reg, (src1_val == 0) ? "true" : "false");
    // The actual PC modification would be handled by the hardware/pipeline
    return 3;
}

static uint8_t handle_jump_if_not_zero(bitactor_registers_t* registers, const bitactor_instruction_t* instruction) {
    // Conceptual jump operation without branching
    uint64_t src1_val = get_reg_val(registers, instruction->src1_reg);
    printf("BitActor Core: Executing BA_OP_JUMP_IF_NOT_ZERO. Condition (R%u != 0): %s\n",
           instruction->src1_reg, (src1_val != 0) ? "true" : "false");
    // The actual PC modification would be handled by the hardware/pipeline
    return 3;
}

static uint8_t handle_collapse(bitactor_registers_t* registers, const bitactor_instruction_t* instruction) {
    // Conceptual collapse operation
    // In a real system, this would involve L4/L8 telemetry and state finalization.
    printf("BitActor Core: Executing BA_OP_COLLAPSE (conceptual).\n");
    return 7; // Most complex operation
}

static uint8_t handle_unknown_opcode(bitactor_registers_t* registers, const bitactor_instruction_t* instruction) {
    fprintf(stderr, "BitActor Core: Unknown opcode 0x%02X.\n", instruction->opcode);
    return MAX_INSTRUCTION_TICKS + 1; // Indicate error
}

uint8_t bitactor_core_execute_instruction(
    bitactor_registers_t* registers,
    const bitactor_instruction_t* instruction
) {
    // In a real hardware implementation, this would involve actual CPU cycle counting
    // or a fixed-cycle execution pipeline. For simulation, we assign a fixed cost.

    // Use the jump table for dispatch
    if (instruction->opcode < sizeof(opcode_handlers) / sizeof(opcode_handler_t)) {
        return opcode_handlers[instruction->opcode](registers, instruction);
    } else {
        return handle_unknown_opcode(registers, instruction);
    }
}
