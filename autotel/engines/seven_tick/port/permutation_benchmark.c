#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#include "bitactor.h"
#include "tick_collapse_engine.h"
#include "signal_engine.h"
#include "bitmask_compiler.h"
#include "actuator.h"

// Helper function to print a BitActorMatrix (for debugging)
void print_matrix(const char* label, const BitActorMatrix* matrix) {
    printf("%s: [", label);
    for (size_t i = 0; i < matrix->num_actors; ++i) {
        printf("%02x", matrix->actors[i]);
        if (i < matrix->num_actors - 1) {
            printf(" ");
        }
    }
    printf("]\n");
}

// Helper function to compare two BitActorMatrices
int compare_matrices(const BitActorMatrix* matrix1, const BitActorMatrix* matrix2) {
    if (matrix1->num_actors != matrix2->num_actors) {
        return 0;
    }
    for (size_t i = 0; i < matrix1->num_actors; ++i) {
        if (matrix1->actors[i] != matrix2->actors[i]) {
            return 0;
        }
    }
    return 1;
}

int main() {
    printf("Running Permutation Benchmark...\n");

    // Initialize components
    BitmaskCompiler* compiler = create_bitmask_compiler();
    SignalEngine* signal_engine = create_signal_engine();
    TickCollapseEngine* tick_engine = create_tick_collapse_engine();

    // Define test cases (rules, initial state, expected final state)
    // For simplicity, initial states are represented as hex strings
    // and expected final states are also hex strings.
    // In a real benchmark, these would be generated systematically.
    typedef struct {
        const char* rules_text;
        const char* initial_matrix_hex;
        const char* expected_matrix_hex;
        const char* signal;
    } TestCase;

    TestCase test_cases[] = {
        {
            "ACTOR 0 BIT 0 SET\nIF ACTOR 0 BIT 0 THEN ACTOR 1 BIT 1 SET",
            "00000000", // Initial: all zeros
            "01020000", // Expected: actor 0 bit 0 set, actor 1 bit 1 set
            "none"
        },
        {
            "IF ACTOR 0 BIT 0 AND ACTOR 0 BIT 1 THEN ACTOR 1 BIT 2 SET",
            "01020000", // Initial: actor 0 bit 0 set, actor 1 bit 1 set
            "01020000", // Expected: no change as AND condition not met
            "none"
        },
        {
            "IF ACTOR 0 BIT 0 AND ACTOR 0 BIT 1 THEN ACTOR 1 BIT 2 SET",
            "03000000", // Initial: actor 0 bits 0 and 1 set
            "03040000", // Expected: actor 1 bit 2 set
            "none"
        },
        {
            "IF ACTOR 0 BIT 0 OR ACTOR 0 BIT 1 THEN ACTOR 1 BIT 2 SET",
            "01000000", // Initial: actor 0 bit 0 set
            "01040000", // Expected: actor 1 bit 2 set
            "none"
        },
        {
            "IF ACTOR 0 BIT 0 OR ACTOR 0 BIT 1 THEN ACTOR 1 BIT 2 SET",
            "00000000", // Initial: all zeros
            "00000000", // Expected: no change as OR condition not met
            "none"
        },
        {
            "ACTOR 0 BIT 0 SET\nACTOR 0 BIT 1 SET\nIF ACTOR 0 BIT 0 AND ACTOR 0 BIT 1 THEN ACTOR 1 BIT 2 SET",
            "00000000", // Initial: all zeros
            "03040000", // Expected: actor 0 bits 0 and 1 set, actor 1 bit 2 set
            "none"
        },
        {
            "ACTOR 0 BIT 0 SET\nIF ACTOR 0 BIT 0 THEN ACTOR 1 BIT 1 CLEAR",
            "01020000", // Initial: actor 0 bit 0 set, actor 1 bit 1 set
            "01000000", // Expected: actor 1 bit 1 cleared
            "none"
        },
        {
            "ACTOR 0 BIT 0 SET",
            "00000000",
            "01000000",
            "trigger" // Signal sets actor 0 bit 0
        },
        {
            "ACTOR 2 BIT 2 SET",
            "00000000", // Initial: all zeros
            "00000400", // Expected: actor 2 bit 2 set
            "none"
        }
    };

    size_t num_test_cases = sizeof(test_cases) / sizeof(TestCase);
    int passed_tests = 0;

    for (size_t i = 0; i < num_test_cases; ++i) {
        printf("\n--- Running Test Case %zu ---\n", i + 1);

        // 1. Prepare initial matrix
        size_t num_actors = strlen(test_cases[i].initial_matrix_hex) / 2;
        BitActorMatrix* initial_matrix = create_bit_actor_matrix(num_actors);
        for (size_t j = 0; j < num_actors; ++j) {
            char byte_str[3];
            strncpy(byte_str, test_cases[i].initial_matrix_hex + (j * 2), 2);
            byte_str[2] = '\0';
            initial_matrix->actors[j] = (BitActor)strtol(byte_str, NULL, 16);
        }
        print_matrix("Initial Matrix", initial_matrix);

        // 2. Compile rules
        RuleSet* compiled_rules = compile_rules(compiler, test_cases[i].rules_text);
        assert(compiled_rules != NULL);

        // 3. Process signal (if any)
        if (strcmp(test_cases[i].signal, "none") != 0) {
            process_signal(signal_engine, initial_matrix, test_cases[i].signal);
            print_matrix("Matrix after signal", initial_matrix);
        }

        // 4. Execute tick collapse
        clock_t start_time = clock();
        TickCollapseResult* final_matrix = tick_collapse_execute(tick_engine, initial_matrix, compiled_rules);
        clock_t end_time = clock();
        double cpu_time_used = ((double) (end_time - start_time)) / CLOCKS_PER_SEC;

        assert(final_matrix != NULL);
        print_matrix("Final Matrix", final_matrix);

        // 5. Prepare expected matrix
        BitActorMatrix* expected_matrix = create_bit_actor_matrix(num_actors);
        for (size_t j = 0; j < num_actors; ++j) {
            char byte_str[3];
            strncpy(byte_str, test_cases[i].expected_matrix_hex + (j * 2), 2);
            byte_str[2] = '\0';
            expected_matrix->actors[j] = (BitActor)strtol(byte_str, NULL, 16);
        }
        print_matrix("Expected Matrix", expected_matrix);

        // 6. Verify result
        if (compare_matrices(final_matrix, expected_matrix)) {
            printf("Test Case %zu: PASSED (Time: %.6f seconds)\n", i + 1, cpu_time_used);
            passed_tests++;
        } else {
            printf("Test Case %zu: FAILED (Time: %.6f seconds)\n", i + 1, cpu_time_used);
        }

        // Clean up for this test case
        destroy_bit_actor_matrix(initial_matrix);
        destroy_bit_actor_matrix(final_matrix);
        destroy_bit_actor_matrix(expected_matrix);
        destroy_rule_set(compiled_rules);
    }

    printf("\n--- Benchmark Summary ---\n");
    printf("Total Test Cases: %zu\n", num_test_cases);
    printf("Passed: %d\n", passed_tests);
    printf("Failed: %zu\n", num_test_cases - passed_tests);

    // Clean up global components
    destroy_bitmask_compiler(compiler);
    destroy_signal_engine(signal_engine);
    destroy_tick_collapse_engine(tick_engine);

    return 0;
}