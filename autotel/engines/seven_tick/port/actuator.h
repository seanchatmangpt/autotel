#ifndef ACTUATOR_H
#define ACTUATOR_H

#include "bitactor.h"

// The Actuator is responsible for taking action based on the state of the
// BitActorMatrix after a tick collapse.

typedef struct {
    // Actuator configuration and state will be defined here in the future.
    int placeholder;
} Actuator;

// Create a new Actuator
Actuator* create_actuator();

// Destroy an Actuator
void destroy_actuator(Actuator* actuator);

// Execute an action based on the BitActorMatrix
void execute_action(Actuator* actuator, const BitActorMatrix* matrix);

#endif // ACTUATOR_H
