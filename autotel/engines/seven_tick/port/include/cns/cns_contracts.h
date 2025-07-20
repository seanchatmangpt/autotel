/**
 * @file cns_contracts.h
 * @brief The Design by Contract (DbC) Substrate for the Chatman Nano Stack.
 *
 * This header provides the C-level primitives for expressing logical contracts
 * directly within the code. In the CNS architecture, these are not runtime
 * checks; they are compile-time proofs.
 *
 * When compiling for production (NDEBUG is defined), these macros compile
 * to zero instructions, guaranteeing zero runtime overhead. They serve as
 * verifiable annotations for the AOT reasoner and as assertions for
 * test/debug builds.
 *
 * This framework transforms C from a purely imperative language into one where
 * correctness can be declared and, in many cases, proven at compile time.
 */
#ifndef CNS_CONTRACTS_H
#define CNS_CONTRACTS_H

#include "cns_core.h"

// ---
// Part 1: Core Contract Primitives
// Defines preconditions, postconditions, and invariants.
// ---

/**
 * @def CNS_PRECONDITION(expr)
 * @brief Asserts a condition that must be true upon entry to a function.
 * This contract defines the function's "requires" clause.
 */
#define CNS_PRECONDITION(expr) \
  CNS_ASSERT((expr) && "CNS PRECONDITION VIOLATED: " #expr)

/**
 * @def CNS_POSTCONDITION(expr)
 * @brief Asserts a condition that must be true upon exit from a function.
 * This contract defines the function's "ensures" clause.
 */
#define CNS_POSTCONDITION(expr) \
  CNS_ASSERT((expr) && "CNS POSTCONDITION VIOLATED: " #expr)

/**
 * @def CNS_INVARIANT(expr)
 * @brief Asserts a condition that must hold true at a specific point in
 * an algorithm, typically within a loop.
 */
#define CNS_INVARIANT(expr) \
  CNS_ASSERT((expr) && "CNS INVARIANT VIOLATED: " #expr)

// ---
// Part 2: Data Contracts (Enforcing the 8-Bit Memory Model)
// ---

/**
 * @def CNS_CONTRACT_IS_8B_ALIGNED(type)
 * @brief A compile-time proof that a data structure's size is a multiple
 * of 8 units of 8 bits.
 */
#define CNS_CONTRACT_IS_8B_ALIGNED(type) \
  _Static_assert(sizeof(type) % 8 == 0, #type " size violates the 8B memory contract (must be a multiple of 64 bits)")

/**
 * @def CNS_CONTRACT_IS_CACHELINE_ALIGNED(type)
 * @brief A compile-time proof that a data structure's size is a multiple
 * of the cache line size (512 bits).
 */
#define CNS_CONTRACT_IS_CACHELINE_ALIGNED(type) \
  _Static_assert(sizeof(type) % 64 == 0, #type " size violates the cache line alignment contract (must be a multiple of 512 bits)")

/**
 * @def CNS_CONTRACT_PTR_IS_ALIGNED(ptr, alignment)
 * @brief A contract asserting that a pointer is aligned to a specific boundary.
 */
#define CNS_CONTRACT_PTR_IS_ALIGNED(ptr, alignment) \
  CNS_PRECONDITION((((uintptr_t)(ptr)) & ((alignment) - 1)) == 0)

/**
 * @def CNS_CONTRACT_IS_NOT_NULL(ptr)
 * @brief A common precondition asserting that a pointer is not NULL.
 * Makes the logical intent explicit.
 */
#define CNS_CONTRACT_IS_NOT_NULL(ptr) \
  CNS_PRECONDITION((ptr) != NULL)

// ---
// Part 3: Performance Contracts (Enforcing the 8-Tick Time Model)
// ---

/**
 * @def CNS_CONTRACT_8T_COMPLIANT(op)
 * @brief The contractual enforcement of the 8-Tick Invariant. Guarantees that
 * an operation executes in 8 cycles or fewer. This is the primary
 * performance contract for all hot-path functions.
 */
#define CNS_CONTRACT_8T_COMPLIANT(op) \
  CNS_ASSERT_CYCLES(op, 8)

/**
 * @def CNS_CONTRACT_LOOP_8T_PER_ITERATION(op, iterations)
 * @brief A contract for loops, asserting that the *average* cycle count per
 * iteration meets the 8T budget. This is critical for validating the
 * performance of vectorized code.
 */
#ifdef CNS_DEBUG
#define CNS_CONTRACT_LOOP_8T_PER_ITERATION(op, iterations)                                                                              \
  do                                                                                                                                    \
  {                                                                                                                                     \
    CNS_ASSERT((iterations) > 0);                                                                                                       \
    cns_cycle_t start_cycles = CNS_RDTSC();                                                                                             \
    op;                                                                                                                                 \
    cns_cycle_t end_cycles = CNS_RDTSC();                                                                                               \
    cns_cycle_t avg_cycles = (end_cycles - start_cycles) / (iterations);                                                                \
    if (CNS_UNLIKELY(avg_cycles > 8))                                                                                                   \
    {                                                                                                                                   \
      fprintf(stderr, "CNS LOOP CONTRACT VIOLATION: %s exceeded 8-tick/iter budget. Avg: %llu\n", #op, (unsigned long long)avg_cycles); \
      __builtin_trap();                                                                                                                 \
    }                                                                                                                                   \
  } while (0)
#else
#define CNS_CONTRACT_LOOP_8T_PER_ITERATION(op, iterations) op
#endif

// ---
// Part 4: Logical State Contracts (For State Machines)
// ---

/**
 * @def CNS_CONTRACT_SFL_VALID_STATE(machine, state)
 * @brief A contract ensuring a given state is valid for a specific State
 * Finite Lattice (SFL).
 */
#define CNS_CONTRACT_SFL_VALID_STATE(machine, state) \
  CNS_PRECONDITION((state) < (machine)->num_states)

/**
 * @def CNS_CONTRACT_SFL_VALID_EVENT(machine, event)
 * @brief A contract ensuring a given event is valid for a specific SFL.
 */
#define CNS_CONTRACT_SFL_VALID_EVENT(machine, event) \
  CNS_PRECONDITION((event) < (machine)->num_events)

// ---
// Part 5: Runtime Assertion Framework
// ---

#ifndef NDEBUG
/**
 * @def CNS_ASSERT(expr)
 * @brief Runtime assertion that compiles away in production builds.
 * Provides detailed error messages for debugging.
 */
#define CNS_ASSERT(expr)                                    \
  do                                                        \
  {                                                         \
    if (CNS_UNLIKELY(!(expr)))                              \
    {                                                       \
      fprintf(stderr, "CNS ASSERTION FAILED: %s\n", #expr); \
      __builtin_trap();                                     \
    }                                                       \
  } while (0)

/**
 * @def CNS_ASSERT_CYCLES(op, max_cycles)
 * @brief Performance assertion that measures cycle count of an operation.
 * Fails if the operation exceeds the specified cycle budget.
 */
#define CNS_ASSERT_CYCLES(op, max_cycles)                                     \
  do                                                                          \
  {                                                                           \
    cns_cycle_t start = CNS_RDTSC();                                          \
    op;                                                                       \
    cns_cycle_t end = CNS_RDTSC();                                            \
    cns_cycle_t cycles = end - start;                                         \
    if (CNS_UNLIKELY(cycles > (max_cycles)))                                  \
    {                                                                         \
      fprintf(stderr, "CNS CYCLE VIOLATION: %s took %llu cycles (max: %d)\n", \
              #op, (unsigned long long)cycles, (max_cycles));                 \
      __builtin_trap();                                                       \
    }                                                                         \
  } while (0)

#else
// Production builds: compile away all assertions
#define CNS_ASSERT(expr) ((void)0)
#define CNS_ASSERT_CYCLES(op, max_cycles) op
#endif

#endif // CNS_CONTRACTS_H