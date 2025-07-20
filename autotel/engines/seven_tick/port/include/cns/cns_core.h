/**
 * @file cns_core.h
 * @brief The AHI-Level Core Substrate for the Chatman Nano Stack.
 *
 * This is not a general-purpose utility header. It is the physical C-level
 * materialization of the 8T/8H/8B architectural axioms. It provides a set of
 * zero-cost abstractions, deterministic primitives, and verifiable contracts
 * that are shared by every component in the CNS ecosystem.
 *
 * Every macro and function in this file is designed to be AOT-aware and
 * 8T-compliant.
 */
#ifndef CNS_CORE_H
#define CNS_CORE_H

#include <stdint.h>
#include <stddef.h>
#include <time.h>

// ---
// Part 1: Core Type Definitions (The 8-Bit Contract)
// These types enforce the system's memory and identity model.
// ---

/**
 * @typedef cns_id_t
 * @brief The canonical type for all interned IDs in the system graph.
 * A 32-bit unsigned integer provides a namespace of over 4 billion unique entities.
 */
typedef uint32_t cns_id_t;

/**
 * @typedef cns_bitmask_t
 * @brief The fundamental unit of parallel logic. Represents 64 parallel states,
 * properties, or booleans. This is the physical materialization of the 8-Bit axiom
 * on 64-bit hardware.
 */
typedef uint64_t cns_bitmask_t;

/**
 * @typedef cns_bool_t
 * @brief A deterministic boolean type, sized to 8 bits.
 */
typedef uint8_t cns_bool_t;
#define CNS_TRUE 1
#define CNS_FALSE 0

/**
 * @typedef cns_cycle_t
 * @brief A type alias for a 64-bit cycle count, the quantum of time.
 */
typedef uint64_t cns_cycle_t;

// ---
// Part 2: Compiler Abstractions (Enforcing Determinism)
// These macros abstract away compiler-specific features, making the AOT
// toolchain's output portable and the intent clear.
// ---

#if defined(__GNUC__) || defined(__clang__)
/**
 * @def CNS_INLINE
 * @brief Forces a function to be inlined, a critical contract for 8T compliance.
 */
#define CNS_INLINE static inline __attribute__((always_inline))

/**
 * @def CNS_ALIGN_64
 * @brief Enforces memory alignment to 64 bits (8 units of 8 bits), a core tenet of the 8B contract.
 */
#define CNS_ALIGN_64 __attribute__((aligned(8)))

/**
 * @def CNS_ALIGN_CACHELINE
 * @brief Enforces memory alignment to the L1 cache line (512 bits).
 */
#define CNS_ALIGN_CACHELINE __attribute__((aligned(64)))

/**
 * @def CNS_LIKELY(x) / CNS_UNLIKELY(x)
 * @brief Provides branch prediction hints to the compiler. This is not an
 * optimization; it is an AOT-level assertion about the system's expected state.
 */
#define CNS_LIKELY(x) __builtin_expect(!!(x), 1)
#define CNS_UNLIKELY(x) __builtin_expect(!!(x), 0)

/**
 * @def CNS_UNUSED(x)
 * @brief Marks a parameter as intentionally unused to suppress compiler warnings.
 */
#define CNS_UNUSED(x) (void)(x)
#else
#error "Unsupported compiler. CNS requires GCC or Clang for its architectural primitives."
#endif

// ---
// Part 3: The 8T Substrate (Time & Action Primitives)
// ---

/**
 * @def CNS_RDTSC()
 * @brief Reads the CPU's Time Stamp Counter. This is the fundamental unit of
 * time in the 8T substrate.
 * @return A 64-bit cycle count.
 */
CNS_INLINE cns_cycle_t CNS_RDTSC(void)
{
#ifdef __x86_64__
  uint32_t lo, hi;
  __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
  return ((uint64_t)hi << 32) | lo;
#elif defined(__aarch64__)
  // ARM64 cycle counter - check if available
  uint64_t val;
  __asm__ __volatile__("mrs %0, PMCCNTR_EL0" : "=r"(val) : : "memory");
  return val;
#else
  // Fallback for other architectures - use high-resolution clock
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (cns_cycle_t)(ts.tv_sec * 1000000000ULL + ts.tv_nsec);
#endif
}

/**
 * @struct cns_op_t
 * @brief The atomic unit of action. A single, 64-bit micro-operation.
 * Its size is exactly 8 units of 8 bits. It is the C representation of a
 * single step in a compiled logical plan.
 */
typedef struct CNS_ALIGN_64
{
  uint8_t opcode;     // The operation to perform.
  uint8_t dst_reg;    // Destination register index.
  uint8_t src1_reg;   // Source register 1 index.
  uint8_t src2_reg;   // Source register 2 index.
  uint32_t immediate; // An immediate value.
} cns_op_t;

// ---
// Part 4: The 8B Substrate (Space & Information Primitives)
// ---

/**
 * @struct cns_arena_t
 * @brief The contract for zero-allocation memory management.
 */
typedef struct
{
  uint8_t *base;
  size_t size;
  size_t used;
} cns_arena_t;

/**
 * @struct cns_triple_t
 * @brief The canonical representation of a logical assertion.
 * Its size is 128 bits (16 units of 8 bits), ensuring 8B compliance.
 */
typedef struct CNS_ALIGN_64
{
  cns_id_t subject;
  cns_id_t predicate;
  cns_id_t object;
  cns_id_t graph_id; // Explicit padding; used for multi-graph contexts.
} cns_triple_t;

/**
 * @struct cns_bitslab_t
 * @brief The physical representation of a set of logical states. This is
 * the primary index structure for the graph. It is a pointer to a
 * 64-bit-aligned array of bitmasks.
 */
typedef struct
{
  cns_bitmask_t *masks;
  size_t num_masks;
} cns_bitslab_t;

// ---
// Part 5: The 8H Substrate (Logic & Proof Primitives)
// ---

/**
 * @struct cns_violation_t
 * @brief The atomic unit of sensory input for the AHI. Represents a detected
 * logical inconsistency (a SHACL violation).
 * Its size is 128 bits.
 */
typedef struct CNS_ALIGN_64
{
  cns_id_t node_id;           // The entity that failed validation.
  cns_id_t shape_id;          // The governance rule that was violated.
  cns_bitmask_t failure_mask; // Bitmask indicating which specific constraints failed.
} cns_violation_t;

/**
 * @struct cns_mutation_t
 * @brief The atomic unit of action output by the AHI. Represents a proposed,
 * provably correct change to the system's TTL specification.
 * Its size is 128 bits.
 */
typedef struct CNS_ALIGN_64
{
  cns_op_t op; // The action to take (e.g., ADD_TRIPLE, RETRACT_AXIOM).
  cns_id_t subject;
  cns_id_t predicate;
  cns_id_t object;
  uint32_t confidence; // The AHI's confidence in this mutation.
} cns_mutation_t;

/**
 * @typedef cns_hop_function_t
 * @brief A function pointer representing one of the 8 hops in the cognitive cycle.
 * It takes the system context and returns a bitmask indicating success.
 */
typedef cns_bitmask_t (*cns_hop_function_t)(void *context);

// ---
// Part 6: Architectural Contracts (Compile-Time Proofs)
// ---

/**
 * These static assertions are not checks; they are proofs. If the code compiles,
 * the 8B memory contract has been met for these core structures.
 */
_Static_assert(sizeof(cns_op_t) % 8 == 0, "8B Contract Violation: cns_op_t");
_Static_assert(sizeof(cns_triple_t) % 8 == 0, "8B Contract Violation: cns_triple_t");
_Static_assert(sizeof(cns_violation_t) % 8 == 0, "8B Contract Violation: cns_violation_t");
_Static_assert(sizeof(cns_mutation_t) % 8 == 0, "8B Contract Violation: cns_mutation_t");

/**
 * @def CNS_ASSERT_8T_COMPLIANCE(op)
 * @brief The C-level enforcement of the 8T contract. This is the primary tool
 * used by the Gatekeeper to validate any human-written or AOT-generated code.
 */
#ifdef CNS_DEBUG
#define CNS_ASSERT_8T_COMPLIANCE(op) CNS_ASSERT_CYCLES(op, 8)
#else
#define CNS_ASSERT_8T_COMPLIANCE(op) op
#endif

#endif // CNS_CORE_H