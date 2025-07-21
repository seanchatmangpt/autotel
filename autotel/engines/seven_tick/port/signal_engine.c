#include "signal_engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Create a new Signal Engine
SignalEngine* create_signal_engine() {
    SignalEngine* engine = (SignalEngine*)malloc(sizeof(SignalEngine));
    if (!engine) {
        fprintf(stderr, "Error: Failed to allocate memory for SignalEngine.\n");
        return NULL;
    }
    // Initialize engine state here if needed
    return engine;
}

// Destroy a Signal Engine
void destroy_signal_engine(SignalEngine* engine) {
    free(engine);
}

// Process an input signal and update the BitActor matrix
void process_signal(SignalEngine* engine, BitActorMatrix* matrix, const char* signal) {
    if (!engine || !matrix || !signal) {
        fprintf(stderr, "Error: Invalid input to process_signal (engine, matrix, or signal is NULL).\n");
        return;
    }

    // For now, we'll just print the signal. In the future, this will involve
    // nanoregex matching and updating the BitActor matrix based on the signal.
    printf("Processing signal: %s\n", signal);

    // As a placeholder, if the signal is "trigger", we'll set the first bit of the first actor.
    
    if (strcmp(signal, "trigger") == 0) {
        if (matrix->num_actors > 0) {
            // Ensure bit_position is valid (0-7)
            set_bit_actor_meaning(&matrix->actors[0], 0);
        } else {
            fprintf(stderr, "Warning: Signal 'trigger' received but matrix has no actors to modify.\n");
        }
    }
    // Add more sophisticated signal processing logic here for production quality.
    // e.g., parsing structured signals, applying nanoregex patterns.
}