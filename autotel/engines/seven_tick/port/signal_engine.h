#ifndef SIGNAL_ENGINE_H
#define SIGNAL_ENGINE_H

#include "bitactor.h"

// The Signal Engine is responsible for processing input signals (e.g., from nanoregex)
// and feeding them into the BitActor matrix.

typedef struct {
    // Engine configuration and state will be defined here in the future.
    int placeholder;
} SignalEngine;

// Create a new Signal Engine
SignalEngine* create_signal_engine();

// Destroy a Signal Engine
void destroy_signal_engine(SignalEngine* engine);

// Process an input signal and update the BitActor matrix
void process_signal(SignalEngine* engine, BitActorMatrix* matrix, const char* signal);

#endif // SIGNAL_ENGINE_H
