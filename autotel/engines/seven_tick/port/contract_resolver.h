#ifndef CONTRACT_RESOLVER_H
#define CONTRACT_RESOLVER_H

#include <stdint.h> // For uint64_t

// Assuming fiber_entry is a function pointer to the start of a fiber's execution
typedef void (*fiber_entry_t)(void* actor_state, void* message);

// Function to resolve a predicate hash to a fiber entry point
fiber_entry_t contract_resolver_resolve(uint64_t predicate_hash);

#endif // CONTRACT_RESOLVER_H
