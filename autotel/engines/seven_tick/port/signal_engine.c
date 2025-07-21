#include "signal_engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Create a new Signal Engine
SignalEngine* create_signal_engine() {
    SignalEngine* engine = (SignalEngine*)malloc(sizeof(SignalEngine));
    // Initialize engine state here if needed
    return engine;
}

// Destroy a Signal Engine
void destroy_signal_engine(SignalEngine* engine) {
    free(engine);
}

// Process an input signal and update the BitActor matrix
void process_signal(SignalEngine* engine, BitActorMatrix* matrix, const char* signal) {
    // For now, we'll just print the signal. In the future, this will involve
    // nanoregex matching and updating the BitActor matrix based on the signal.
    printf("Processing signal: %s\n", signal);

    // As a placeholder, if the signal is "trigger", we'll set the first bit of the first actor.
    if (strcmp(signal, "trigger") == 0) {
        if (matrix->num_actors > 0) {
            set_bit_actor_meaning(&matrix->actors[0], 0);
        }
    }
}

