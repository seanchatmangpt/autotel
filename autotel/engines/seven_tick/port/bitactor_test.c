#include <stdio.h>
#include <assert.h>
#include "bitactor.h"
#include "tick_collapse_engine.h"
#include "signal_engine.h"
#include "bitmask_compiler.h"
#include "actuator.h"

int main() {
    printf("Running BitActor test...\n");

    // 1. Create a BitActorMatrix
    size_t num_actors = 10;
    BitActorMatrix* matrix = create_bit_actor_matrix(num_actors);
    assert(matrix != NULL);
    assert(matrix->num_actors == num_actors);

    // 2. Create a BitmaskCompiler
    BitmaskCompiler* compiler = create_bitmask_compiler();
    assert(compiler != NULL);

    // 3. Define and compile rules
    const char* rules = "ACTOR 0 BIT 2 SET\nACTOR 3 BIT 7 SET\nACTOR 9 BIT 0 SET\nIF ACTOR 0 BIT 2 THEN ACTOR 1 BIT 1 SET";
    compile_rules(compiler, matrix, rules);

    assert(check_bit_actor_meaning(&matrix->actors[0], 2) == 1);
    assert(check_bit_actor_meaning(&matrix->actors[3], 7) == 1);
    assert(check_bit_actor_meaning(&matrix->actors[9], 0) == 1);
    assert(check_bit_actor_meaning(&matrix->actors[1], 1) == 1);
    assert(check_bit_actor_meaning(&matrix->actors[1], 0) == 0);

    // 4. Create a SignalEngine
    SignalEngine* signal_engine = create_signal_engine();
    assert(signal_engine != NULL);

    // 5. Process a "trigger" signal
    process_signal(signal_engine, matrix, "trigger");
    assert(check_bit_actor_meaning(&matrix->actors[0], 0) == 1);

    // 6. Create a TickCollapseEngine
    TickCollapseEngine* engine = create_tick_collapse_engine();
    assert(engine != NULL);

    // 7. Execute a "tick" and verify the result
    TickCollapseResult* result = tick_collapse_execute(engine, matrix);
    assert(result != NULL);
    assert(result->num_actors == num_actors);

    for (size_t i = 0; i < num_actors; ++i) {
        if (i == 0) {
            assert(result->actors[i] == (BitActor)~matrix->actors[i]);
        } else {
            assert(result->actors[i] == matrix->actors[i]);
        }
    }

    // 8. Clean up all allocated resources
    destroy_bit_actor_matrix(matrix);
    destroy_bit_actor_matrix(result);
    destroy_tick_collapse_engine(engine);
    destroy_signal_engine(signal_engine);
    destroy_bitmask_compiler(compiler);

    printf("BitActor test completed successfully!\n");

    return 0;
}
