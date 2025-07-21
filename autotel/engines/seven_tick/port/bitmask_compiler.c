#include "bitmask_compiler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Create a new Bitmask Compiler
BitmaskCompiler* create_bitmask_compiler() {
    BitmaskCompiler* compiler = (BitmaskCompiler*)malloc(sizeof(BitmaskCompiler));
    // Initialize compiler state here if needed
    return compiler;
}

// Destroy a Bitmask Compiler
void destroy_bitmask_compiler(BitmaskCompiler* compiler) {
    free(compiler);
}

// Compile a set of rules into a BitActorMatrix
void compile_rules(BitmaskCompiler* compiler, BitActorMatrix* matrix, const char* rules) {
    char* rules_copy = strdup(rules);
    char* line = strtok(rules_copy, "\n");

    while (line != NULL) {
        int actor_index, bit_position, actor_index2, bit_position2, actor_index3, bit_position3;
        char command[6];

        if (sscanf(line, "ACTOR %d BIT %d %5s", &actor_index, &bit_position, command) == 3) {
            if (strcmp(command, "SET") == 0) {
                if (actor_index >= 0 && actor_index < matrix->num_actors) {
                    set_bit_actor_meaning(&matrix->actors[actor_index], bit_position);
                }
            } else if (strcmp(command, "CLEAR") == 0) {
                if (actor_index >= 0 && actor_index < matrix->num_actors) {
                    clear_bit_actor_meaning(&matrix->actors[actor_index], bit_position);
                }
            }
        } else if (sscanf(line, "IF ACTOR %d BIT %d AND ACTOR %d BIT %d THEN ACTOR %d BIT %d SET", &actor_index, &bit_position, &actor_index2, &bit_position2, &actor_index3, &bit_position3) == 6) {
            if (check_bit_actor_meaning(&matrix->actors[actor_index], bit_position) && check_bit_actor_meaning(&matrix->actors[actor_index2], bit_position2)) {
                if (actor_index3 >= 0 && actor_index3 < matrix->num_actors) {
                    set_bit_actor_meaning(&matrix->actors[actor_index3], bit_position3);
                }
            }
        } else if (sscanf(line, "IF ACTOR %d BIT %d THEN ACTOR %d BIT %d SET", &actor_index, &bit_position, &actor_index2, &bit_position2) == 4) {
            if (check_bit_actor_meaning(&matrix->actors[actor_index], bit_position)) {
                if (actor_index2 >= 0 && actor_index2 < matrix->num_actors) {
                    set_bit_actor_meaning(&matrix->actors[actor_index2], bit_position2);
                }
            }
        }

        line = strtok(NULL, "\n");
    }

    free(rules_copy);
}
