#include "bitactor.h"

// Function to initialize a BitActorMatrix
BitActorMatrix* create_bit_actor_matrix(size_t num_actors) {
    BitActorMatrix* matrix = (BitActorMatrix*)malloc(sizeof(BitActorMatrix));
    if (!matrix) {
        return NULL;
    }
    matrix->actors = (BitActor*)calloc(num_actors, sizeof(BitActor));
    if (!matrix->actors) {
        free(matrix);
        return NULL;
    }
    matrix->num_actors = num_actors;
    return matrix;
}

// Function to destroy a BitActorMatrix
void destroy_bit_actor_matrix(BitActorMatrix* matrix) {
    if (matrix) {
        free(matrix->actors);
        free(matrix);
    }
}

// Function to set a specific meaning bit in a BitActor
void set_bit_actor_meaning(BitActor* actor, int bit_position) {
    if (bit_position >= 0 && bit_position < 8) {
        *actor |= (1 << bit_position);
    }
}

// Function to clear a specific meaning bit in a BitActor
void clear_bit_actor_meaning(BitActor* actor, int bit_position) {
    if (bit_position >= 0 && bit_position < 8) {
        *actor &= ~(1 << bit_position);
    }
}

// Function to check a specific meaning bit in a BitActor
int check_bit_actor_meaning(const BitActor* actor, int bit_position) {
    if (bit_position >= 0 && bit_position < 8) {
        return (*actor >> bit_position) & 1;
    }
    return 0;
}
