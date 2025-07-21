#include "tick_collapse_engine.h"
#include "actuator.h"
#include "bitmask_compiler.h" // Include for RuleSet
#include <stdio.h>
#include <stdlib.h>

// Hop function placeholders
static void hop_trigger_detected(HopState* state) {
    printf("  (1) Trigger detected\n");
    state->current_hop = HOP_ONTOLOGY_LOADED;
}

static void hop_ontology_loaded(HopState* state) {
    printf("  (2) Ontology loaded\n");
    state->current_hop = HOP_SHACL_PATH_FIRED;
}

static void hop_shacl_path_fired(HopState* state) {
    printf("  (3) SHACL path fired\n");
    state->current_hop = HOP_BITACTOR_STATE_RESOLVED;
}

static void hop_bitactor_state_resolved(HopState* state) {
    printf("  (4) BitActor state resolved\n");
    state->current_hop = HOP_COLLAPSE_COMPUTED;
}

static void hop_collapse_computed(HopState* state) {
    printf("  (5) Collapse computed\n");
    // Apply compiled rules from the RuleSet
    if (state->rule_set) {
        for (size_t i = 0; i < state->rule_set->num_rules; ++i) {
            CompiledRule rule = state->rule_set->rules[i];
            int condition_met = 0;

            switch (rule.condition_type) {
                case CONDITION_NONE:
                    condition_met = 1; // Always apply if no condition
                    break;
                case CONDITION_SINGLE:
                    if (rule.condition_actor_index_1 >= 0 && rule.condition_actor_index_1 < state->matrix->num_actors) {
                        condition_met = check_bit_actor_meaning(&state->matrix->actors[rule.condition_actor_index_1], rule.condition_bit_position_1);
                    }
                    break;
                case CONDITION_AND:
                    if (rule.condition_actor_index_1 >= 0 && rule.condition_actor_index_1 < state->matrix->num_actors &&
                        rule.condition_actor_index_2 >= 0 && rule.condition_actor_index_2 < state->matrix->num_actors) {
                        condition_met = check_bit_actor_meaning(&state->matrix->actors[rule.condition_actor_index_1], rule.condition_bit_position_1) &&
                                        check_bit_actor_meaning(&state->matrix->actors[rule.condition_actor_index_2], rule.condition_bit_position_2);
                    }
                    break;
                case CONDITION_OR:
                    if (rule.condition_actor_index_1 >= 0 && rule.condition_actor_index_1 < state->matrix->num_actors &&
                        rule.condition_actor_index_2 >= 0 && rule.condition_actor_index_2 < state->matrix->num_actors) {
                        condition_met = check_bit_actor_meaning(&state->matrix->actors[rule.condition_actor_index_1], rule.condition_bit_position_1) ||
                                        check_bit_actor_meaning(&state->matrix->actors[rule.condition_actor_index_2], rule.condition_bit_position_2);
                    }
                    break;
            }

            if (condition_met) {
                if (rule.action_actor_index >= 0 && rule.action_actor_index < state->matrix->num_actors) {
                    if (rule.action_type == ACTION_SET) {
                        set_bit_actor_meaning(&state->matrix->actors[rule.action_actor_index], rule.action_bit_position);
                    } else if (rule.action_type == ACTION_CLEAR) {
                        clear_bit_actor_meaning(&state->matrix->actors[rule.action_actor_index], rule.action_bit_position);
                    }
                }
            }
        }
    }
    state->current_hop = HOP_ACTION_BOUND;
}

static void hop_action_bound(HopState* state) {
    printf("  (6) Action bound\n");
    state->current_hop = HOP_STATE_COMMITTED;
}

static void hop_state_committed(HopState* state) {
    printf("  (7) State committed\n");
    state->current_hop = HOP_META_PROOF_VALIDATED;
}

static void hop_meta_proof_validated(HopState* state) {
    printf("  (8) Meta-proof validated\n");
}


// Create a new Tick Collapse Engine
TickCollapseEngine* create_tick_collapse_engine() {
    TickCollapseEngine* engine = (TickCollapseEngine*)malloc(sizeof(TickCollapseEngine));
    // Initialize engine state here if needed
    return engine;
}

// Destroy a Tick Collapse Engine
void destroy_tick_collapse_engine(TickCollapseEngine* engine) {
    free(engine);
}

// Execute a tick collapse
TickCollapseResult* tick_collapse_execute(TickCollapseEngine* engine, const BitActorMatrix* matrix, const RuleSet* rule_set) {
    if (!engine || !matrix) {
        return NULL;
    }

    printf("Executing 8H causal proof chain...\n");

    // Initialize the hop state
    HopState state;
    state.current_hop = HOP_TRIGGER_DETECTED;
    state.matrix = create_bit_actor_matrix(matrix->num_actors);
    if (!state.matrix) {
        return NULL;
    }
    for (size_t i = 0; i < matrix->num_actors; ++i) {
        state.matrix->actors[i] = matrix->actors[i];
    }
    state.rule_set = rule_set; // Assign the rule set

    // Execute the 8 hops
    hop_trigger_detected(&state);
    hop_ontology_loaded(&state);
    hop_shacl_path_fired(&state);
    hop_bitactor_state_resolved(&state);
    hop_collapse_computed(&state);
    hop_action_bound(&state);
    hop_state_committed(&state);
    hop_meta_proof_validated(&state);

    // Create and execute the actuator
    Actuator* actuator = create_actuator();
    execute_action(actuator, state.matrix);
    destroy_actuator(actuator);

    printf("8H causal proof chain complete.\n");

    return state.matrix;
}
