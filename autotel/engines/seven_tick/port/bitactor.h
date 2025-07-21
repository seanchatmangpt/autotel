#ifndef BITACTOR_H
#define BITACTOR_H

#include <stdint.h>
#include <stdlib.h>

// From the manifesto: "BitActors are 8-bit agents where each bit represents 
// one atomic unit of causal significance—compiled from TTL."
// "1 bit = 1 semantic trigger"
// "1 byte = 8 actors"
// This suggests a BitActor is a single bit, and they are managed in groups of 8 (bytes).
// Let's define a BitActor as a byte, where each bit is a "meaning atom".
typedef uint8_t BitActor;

// "8B = 1 causal domain collapse"
// "BitActor Matrix (8B)"
// This suggests a collection of these BitActors. Let's call it a BitActorMatrix.
typedef struct {
    BitActor* actors;
    size_t num_actors;
} BitActorMatrix;

// Function to initialize a BitActorMatrix
BitActorMatrix* create_bit_actor_matrix(size_t num_actors);

// Function to destroy a BitActorMatrix
void destroy_bit_actor_matrix(BitActorMatrix* matrix);

// Function to set a specific meaning bit in a BitActor
void set_bit_actor_meaning(BitActor* actor, int bit_position);

// Function to clear a specific meaning bit in a BitActor
void clear_bit_actor_meaning(BitActor* actor, int bit_position);

// Function to check a specific meaning bit in a BitActor
int check_bit_actor_meaning(const BitActor* actor, int bit_position);

#endif // BITACTOR_H
