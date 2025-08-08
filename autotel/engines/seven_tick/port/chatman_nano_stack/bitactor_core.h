#ifndef BITACTOR_CORE_H
#define BITACTOR_CORE_H

#include <stdint.h> // For uint64_t, etc.

// Define the 8 registers (R0-R7)
typedef struct {
    uint64_t R0; // General purpose register 0
    uint64_t R1; // General purpose register 1
    uint64_t R2; // General purpose register 2
    uint64_t R3; // General purpose register 3
    uint64_t R4; // General purpose register 4
    uint64_t R5; // General purpose register 5
    uint64_t R6; // General purpose register 6
    uint64_t R7; // General purpose register 7 (e.g., program counter, status)
} bitactor_registers_t;

// Define BA_OP_* opcodes
typedef enum {
    BA_OP_NOOP = 0x00,      // No operation
    BA_OP_ADD = 0x01,       // Add R_dest = R_src1 + R_src2
    BA_OP_SUB = 0x02,       // Subtract R_dest = R_src1 - R_src2
    BA_OP_AND = 0x03,       // Bitwise AND
    BA_OP_OR = 0x04,        // Bitwise OR
    BA_OP_XOR = 0x05,       // Bitwise XOR
    BA_OP_MOV = 0x06,       // Move R_dest = R_src
    BA_OP_ENTANGLE = 0x07,  // Entangle operation (conceptual)
    BA_OP_JUMP_IF_ZERO = 0x08, // Jump if R_cond is zero
    BA_OP_JUMP_IF_NOT_ZERO = 0x09, // Jump if R_cond is not zero
    BA_OP_COLLAPSE = 0x0A,  // Collapse operation (conceptual)
    // ... other opcodes as needed
} bitactor_opcode_t;

// Structure to represent a single instruction
typedef struct {
    bitactor_opcode_t opcode; // The operation to perform
    uint8_t dest_reg;         // Destination register index (0-7)
    uint8_t src1_reg;         // Source 1 register index (0-7)
    uint8_t src2_reg;         // Source 2 register index (0-7) or immediate value
    uint64_t immediate;       // Immediate value for certain operations
} bitactor_instruction_t;

// Function to execute a single BitActor instruction
// Returns the number of CPU ticks consumed by the instruction.
// Should be <= 7 ticks.
uint8_t bitactor_core_execute_instruction(
    bitactor_registers_t* registers,
    const bitactor_instruction_t* instruction
);

#endif // BITACTOR_CORE_H
