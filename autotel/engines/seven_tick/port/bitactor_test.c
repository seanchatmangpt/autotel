#include <stdio.h>
#include <assert.h>
#include "bitactor.h"
#include "tick_collapse_engine.h"
#include "signal_engine.h"
#include "bitmask_compiler.h"
#include "actuator.h"

int main() {
    printf("Running BitActor test...\n");

    // 1. Create a BitActorMatrix (initial state - all zeros)
    size_t num_actors = 10;
    BitActorMatrix* matrix = create_bit_actor_matrix(num_actors);
    assert(matrix != NULL);
    assert(matrix->num_actors == num_actors);

    // 2. Create a BitmaskCompiler
    BitmaskCompiler* compiler = create_bitmask_compiler();
    assert(compiler != NULL);

    // 3. Define and compile rules
    // Rule: If actor 0, bit 0 is set, then set actor 1, bit 1
    const char* rules_text = 
        "IF ACTOR 0 BIT 0 THEN ACTOR 1 BIT 1 SET\n"
        "ACTOR 2 BIT 2 SET";
    RuleSet* compiled_rules = compile_rules(compiler, rules_text);
    assert(compiled_rules != NULL);
    assert(compiled_rules->num_rules == 2);

    // 4. Create a SignalEngine
    SignalEngine* signal_engine = create_signal_engine();
    assert(signal_engine != NULL);

    // 5. Process a "trigger" signal
    // This signal will set actor 0, bit 0 in the initial matrix
    process_signal(signal_engine, matrix, "trigger");
    assert(check_bit_actor_meaning(&matrix->actors[0], 0) == 1);

    // 6. Create a TickCollapseEngine
    TickCollapseEngine* engine = create_tick_collapse_engine();
    assert(engine != NULL);

    // 7. Execute a "tick" and verify the result
    // The tick should apply the compiled rules to the matrix
    TickCollapseResult* result = tick_collapse_execute(engine, matrix, compiled_rules);
    assert(result != NULL);
    assert(result->num_actors == num_actors);

    // Verify that actor 1, bit 1 is set due to the rule and the signal
    assert(check_bit_actor_meaning(&result->actors[1], 1) == 1);
    assert(check_bit_actor_meaning(&result->actors[2], 2) == 1);

    // Verify other bits are still zero (unless affected by other rules/signals)
    assert(check_bit_actor_meaning(&result->actors[0], 0) == 1); // Should still be set from signal
    assert(check_bit_actor_meaning(&result->actors[0], 1) == 0);
    assert(check_bit_actor_meaning(&result->actors[1], 0) == 0);

    // 8. Clean up all allocated resources
    destroy_bit_actor_matrix(matrix);
    destroy_bit_actor_matrix(result);
    destroy_tick_collapse_engine(engine);
    destroy_signal_engine(signal_engine);
    destroy_bitmask_compiler(compiler);
    destroy_rule_set(compiled_rules);

    printf("BitActor test completed successfully!\n");

    return 0;
}