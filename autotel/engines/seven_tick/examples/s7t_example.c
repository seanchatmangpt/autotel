/*
 * s7t_example.c - Seven Tick Library Usage Example
 * 
 * Demonstrates physics-compliant programming patterns for nanosecond computing.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../include/s7t.h"
#include "../include/s7t_patterns.h"
#include "../include/s7t_workflow.h"
#include "../include/s7t_perf.h"

/* Global telemetry instance */
s7t_telemetry_t* g_s7t_telemetry = NULL;

/* Performance violation handler */
void s7t_perf_violation(const char* file, int line, 
                       uint64_t actual_cycles, uint64_t max_cycles) {
    fprintf(stderr, "PERF VIOLATION: %s:%d - %llu cycles (max: %llu)\n",
            file, line, (unsigned long long)actual_cycles, 
            (unsigned long long)max_cycles);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * EXAMPLE 1: Basic Arena and String Interning
 * ═══════════════════════════════════════════════════════════════════════════ */

void example_arena_and_strings(void) {
    printf("\n=== Example 1: Arena and String Interning ===\n");
    
    /* Declare static arena */
    S7T_DECLARE_ARENA(my_arena, S7T_MB(1));
    
    /* Create string intern table */
    s7t_intern_t strings = s7t_intern_make(&my_arena, 1000, S7T_KB(64));
    
    /* Intern some strings */
    uint32_t id1 = s7t_intern(&strings, "Process");
    uint32_t id2 = s7t_intern(&strings, "Thread");
    uint32_t id3 = s7t_intern(&strings, "Process"); /* Same as id1 */
    
    printf("String IDs: Process=%u, Thread=%u, Process(2)=%u\n", id1, id2, id3);
    printf("Comparison (1 cycle): Process==Process? %s\n", 
           s7t_intern_cmp(id1, id3) ? "true" : "false");
    printf("Comparison (1 cycle): Process==Thread? %s\n", 
           s7t_intern_cmp(id1, id2) ? "true" : "false");
    
    /* Measure string comparison performance */
    uint64_t cycles;
    S7T_MEASURE_CYCLES(cycles, {
        for (int i = 0; i < 1000; i++) {
            s7t_intern_cmp(id1, id2);
        }
    });
    printf("1000 string comparisons: %llu cycles (%.2f cycles/op)\n",
           (unsigned long long)cycles, cycles / 1000.0);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * EXAMPLE 2: Bit-Slab for Triple Store
 * ═══════════════════════════════════════════════════════════════════════════ */

void example_bitslab_triple_store(void) {
    printf("\n=== Example 2: Bit-Slab Triple Store ===\n");
    
    S7T_DECLARE_ARENA(triple_arena, S7T_MB(4));
    
    /* Create bit-slabs for triple patterns */
    const uint32_t num_subjects = 1000;
    const uint32_t num_predicates = 100;
    const uint32_t num_objects = 1000;
    
    /* Predicate -> Subjects mapping */
    s7t_bitslab_t* pred_to_subj = (s7t_bitslab_t*)s7t_arena_alloc(&triple_arena,
                                    num_predicates * sizeof(s7t_bitslab_t));
    
    for (uint32_t p = 0; p < num_predicates; p++) {
        pred_to_subj[p] = s7t_bitslab_make(&triple_arena, 1, num_subjects);
    }
    
    /* Add some triples */
    s7t_bitslab_set(&pred_to_subj[0], 0, 10);  /* (10, 0, _) */
    s7t_bitslab_set(&pred_to_subj[0], 0, 20);  /* (20, 0, _) */
    s7t_bitslab_set(&pred_to_subj[1], 0, 10);  /* (10, 1, _) */
    
    /* Query: which subjects have predicate 0? */
    printf("Subjects with predicate 0:\n");
    for (uint32_t s = 0; s < 100; s++) {
        if (s7t_bitslab_test(&pred_to_subj[0], 0, s)) {
            printf("  Subject %u\n", s);
        }
    }
    
    /* Measure query performance */
    uint64_t cycles;
    S7T_MEASURE_CYCLES(cycles, {
        for (uint32_t s = 0; s < num_subjects; s++) {
            s7t_bitslab_test(&pred_to_subj[0], 0, s);
        }
    });
    printf("Scanning %u subjects: %llu cycles (%.2f cycles/test)\n",
           num_subjects, (unsigned long long)cycles, (double)cycles / num_subjects);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * EXAMPLE 3: State Machine Workflow
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef enum {
    STATE_IDLE = 0,
    STATE_PROCESSING = 1,
    STATE_COMPLETE = 2,
    STATE_ERROR = 3
} process_state_t;

typedef enum {
    EVENT_START = 0,
    EVENT_DATA = 1,
    EVENT_FINISH = 2,
    EVENT_ABORT = 3
} process_event_t;

void example_state_machine(void) {
    printf("\n=== Example 3: State Machine Workflow ===\n");
    
    S7T_DECLARE_ARENA(sm_arena, S7T_KB(64));
    
    /* Create state machine */
    s7t_state_machine_t sm = s7t_sm_create(&sm_arena, 4, 4);
    
    /* Define transitions */
    s7t_sm_add_transition(&sm, STATE_IDLE, EVENT_START, STATE_PROCESSING, 1);
    s7t_sm_add_transition(&sm, STATE_PROCESSING, EVENT_DATA, STATE_PROCESSING, 2);
    s7t_sm_add_transition(&sm, STATE_PROCESSING, EVENT_FINISH, STATE_COMPLETE, 3);
    s7t_sm_add_transition(&sm, STATE_PROCESSING, EVENT_ABORT, STATE_ERROR, 4);
    
    /* Process events */
    const char* state_names[] = {"IDLE", "PROCESSING", "COMPLETE", "ERROR"};
    const char* event_names[] = {"START", "DATA", "FINISH", "ABORT"};
    
    process_event_t events[] = {EVENT_START, EVENT_DATA, EVENT_DATA, EVENT_FINISH};
    
    printf("Initial state: %s\n", state_names[sm.current]);
    
    for (int i = 0; i < 4; i++) {
        uint32_t action = s7t_sm_process(&sm, events[i]);
        printf("Event: %s -> State: %s (action=%u)\n",
               event_names[events[i]], state_names[sm.current], action);
    }
    
    /* Measure state machine performance */
    uint64_t cycles;
    S7T_MEASURE_CYCLES(cycles, {
        for (int i = 0; i < 1000; i++) {
            s7t_sm_process(&sm, EVENT_DATA);
        }
    });
    printf("1000 state transitions: %llu cycles (%.2f cycles/transition)\n",
           (unsigned long long)cycles, cycles / 1000.0);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * EXAMPLE 4: Pipeline Processing
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef struct {
    uint32_t value;
    uint32_t flags;
} pipeline_data_t;

void stage1_double(void* in, void* out, void* context) {
    pipeline_data_t* input = (pipeline_data_t*)in;
    pipeline_data_t* output = (pipeline_data_t*)out;
    output->value = input->value * 2;
    output->flags = input->flags | 0x01;
}

void stage2_increment(void* in, void* out, void* context) {
    pipeline_data_t* input = (pipeline_data_t*)in;
    pipeline_data_t* output = (pipeline_data_t*)out;
    output->value = input->value + 1;
    output->flags = input->flags | 0x02;
}

void stage3_square(void* in, void* out, void* context) {
    pipeline_data_t* input = (pipeline_data_t*)in;
    pipeline_data_t* output = (pipeline_data_t*)out;
    output->value = input->value * input->value;
    output->flags = input->flags | 0x04;
}

void example_pipeline(void) {
    printf("\n=== Example 4: Pipeline Processing ===\n");
    
    S7T_DECLARE_ARENA(pipe_arena, S7T_KB(128));
    
    /* Create 3-stage pipeline */
    s7t_pipeline_t pipe = s7t_pipe_create(&pipe_arena, 3, 16);
    
    /* Add stages */
    s7t_pipe_add_stage(&pipe, &pipe_arena, 0, stage1_double, NULL,
                      sizeof(pipeline_data_t), sizeof(pipeline_data_t), 16);
    s7t_pipe_add_stage(&pipe, &pipe_arena, 1, stage2_increment, NULL,
                      sizeof(pipeline_data_t), sizeof(pipeline_data_t), 16);
    s7t_pipe_add_stage(&pipe, &pipe_arena, 2, stage3_square, NULL,
                      sizeof(pipeline_data_t), sizeof(pipeline_data_t), 16);
    
    /* Process some data */
    pipeline_data_t input = {5, 0};
    printf("Input: value=%u\n", input.value);
    printf("Expected: 5 -> 10 -> 11 -> 121\n");
    
    /* Note: Real pipeline would process multiple items in parallel */
    s7t_pipe_process(&pipe, &input);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * EXAMPLE 5: Performance Validation
 * ═══════════════════════════════════════════════════════════════════════════ */

void example_performance_validation(void) {
    printf("\n=== Example 5: Performance Validation ===\n");
    
    S7T_DECLARE_ARENA(perf_arena, S7T_KB(64));
    
    /* Create performance tracking */
    s7t_budget_tracker_t tracker = {0};
    s7t_cache_stats_t cache_stats = {0};
    s7t_branch_stats_t branch_stats = {0};
    
    /* Register operation budgets */
    uint32_t op_add = s7t_budget_register(&tracker, "add_triple", 7);
    uint32_t op_query = s7t_budget_register(&tracker, "query_pattern", 5);
    uint32_t op_intern = s7t_budget_register(&tracker, "intern_string", 3);
    
    /* Simulate operations and track performance */
    S7T_DECLARE_ARENA(work_arena, S7T_KB(64));
    s7t_intern_t strings = s7t_intern_make(&work_arena, 100, S7T_KB(4));
    
    /* Track string interning */
    for (int i = 0; i < 100; i++) {
        char str[32];
        snprintf(str, sizeof(str), "string_%d", i);
        
        S7T_TRACK_OPERATION(&tracker, op_intern, {
            s7t_intern(&strings, str);
        });
        
        /* Track cache behavior */
        s7t_cache_track(&cache_stats, str);
    }
    
    /* Track query operations */
    s7t_bitslab_t slab = s7t_bitslab_make(&work_arena, 10, 100);
    for (int i = 0; i < 100; i++) {
        S7T_TRACK_OPERATION(&tracker, op_query, {
            s7t_bitslab_test(&slab, 0, i);
        });
        
        /* Track branch (always false in this case) */
        int result = s7t_bitslab_test(&slab, 0, i);
        s7t_branch_track(&branch_stats, result);
    }
    
    /* Create performance gates */
    s7t_perf_gate_t gates[] = {
        {"Cycle Budget", s7t_gate_cycles, &tracker.budgets[op_intern]},
        {"Cache Efficiency", s7t_gate_cache, &cache_stats},
        {"Branch Prediction", s7t_gate_branch, &branch_stats}
    };
    
    /* Generate report */
    s7t_perf_report_t report = {
        .budgets = &tracker,
        .cache_stats = &cache_stats,
        .branch_stats = &branch_stats,
        .gates = gates,
        .num_gates = 3
    };
    
    printf("%s", s7t_perf_report_generate(&report));
}

/* ═══════════════════════════════════════════════════════════════════════════
 * MAIN
 * ═══════════════════════════════════════════════════════════════════════════ */

int main(void) {
    printf("Seven Tick Library Examples\n");
    printf("==========================\n");
    
    /* Get CPU frequency estimate */
    struct timespec ts1, ts2;
    clock_gettime(CLOCK_MONOTONIC, &ts1);
    uint64_t cycles1 = s7t_cycles();
    
    /* Busy wait for measurement */
    for (volatile int i = 0; i < 100000000; i++);
    
    clock_gettime(CLOCK_MONOTONIC, &ts2);
    uint64_t cycles2 = s7t_cycles();
    
    uint64_t ns = (ts2.tv_sec - ts1.tv_sec) * 1000000000 + 
                  (ts2.tv_nsec - ts1.tv_nsec);
    uint64_t cycles = cycles2 - cycles1;
    double ghz = (double)cycles / ns;
    
    printf("\nEstimated CPU frequency: %.2f GHz\n", ghz);
    printf("Target: All operations ≤ %d cycles (%.1f ns @ %.2f GHz)\n\n",
           S7T_MAX_CYCLES, S7T_MAX_CYCLES / ghz, ghz);
    
    /* Run examples */
    example_arena_and_strings();
    example_bitslab_triple_store();
    example_state_machine();
    example_pipeline();
    example_performance_validation();
    
    printf("\nExamples completed!\n");
    return 0;
}