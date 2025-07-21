#ifndef TICK_COLLAPSE_ENGINE_H
#define TICK_COLLAPSE_ENGINE_H

#include "bitactor.h"
#include "bitmask_compiler.h"

// The 8 hops of the causal proof chain
typedef enum {
    HOP_TRIGGER_DETECTED,
    HOP_ONTOLOGY_LOADED,
    HOP_SHACL_PATH_FIRED,
    HOP_BITACTOR_STATE_RESOLVED,
    HOP_COLLAPSE_COMPUTED,
    HOP_ACTION_BOUND,
    HOP_STATE_COMMITTED,
    HOP_META_PROOF_VALIDATED
} Hop;

// State for the 8-hop process
typedef struct {
    Hop current_hop;
    BitActorMatrix* matrix;
    const RuleSet* rule_set; // Add rule_set to HopState
} HopState;

// The Tick Collapse Engine is responsible for processing the BitActorMatrix
// in a single "tick". This represents the 8T pillar of the manifesto.
typedef struct {
    // Engine configuration and state will be defined here in the future.
    int placeholder;
} TickCollapseEngine;

// The result of a tick collapse, which can be a new state for the BitActorMatrix.
typedef BitActorMatrix TickCollapseResult;

// Create a new Tick Collapse Engine
TickCollapseEngine* create_tick_collapse_engine();

// Destroy a Tick Collapse Engine
void destroy_tick_collapse_engine(TickCollapseEngine* engine);

// Execute a tick collapse
TickCollapseResult* tick_collapse_execute(TickCollapseEngine* engine, const BitActorMatrix* matrix, const RuleSet* rule_set);

#endif // TICK_COLLAPSE_ENGINE_H
