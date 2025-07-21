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

// Create a new RuleSet
RuleSet* create_rule_set(size_t initial_capacity) {
    RuleSet* rule_set = (RuleSet*)malloc(sizeof(RuleSet));
    if (!rule_set) {
        fprintf(stderr, "Error: Failed to allocate memory for RuleSet.\n");
        return NULL;
    }
    rule_set->rules = (CompiledRule*)malloc(sizeof(CompiledRule) * initial_capacity);
    if (!rule_set->rules) {
        fprintf(stderr, "Error: Failed to allocate memory for RuleSet rules.\n");
        free(rule_set);
        return NULL;
    }
    rule_set->num_rules = 0;
    rule_set->capacity = initial_capacity;
    return rule_set;
}

// Destroy a RuleSet
void destroy_rule_set(RuleSet* rule_set) {
    if (rule_set) {
        free(rule_set->rules);
        free(rule_set);
    }
}

// Add a rule to the RuleSet, resizing if necessary
void add_rule_to_set(RuleSet* rule_set, CompiledRule rule) {
    if (!rule_set) return;

    if (rule_set->num_rules == rule_set->capacity) {
        rule_set->capacity *= 2;
        CompiledRule* new_rules = (CompiledRule*)realloc(rule_set->rules, sizeof(CompiledRule) * rule_set->capacity);
        if (!new_rules) {
            fprintf(stderr, "Error: Failed to reallocate memory for RuleSet rules. Rule dropped.\n");
            // In a production system, this might trigger a more severe error handling.
            return;
        }
        rule_set->rules = new_rules;
    }
    rule_set->rules[rule_set->num_rules++] = rule;
}

// Conceptual rule optimization: In a real system, this would reorder, simplify,
// or merge rules for L1 cache efficiency and branch prediction.
static void optimize_rules(RuleSet* rule_set) {
    if (!rule_set) return;
    // Placeholder for complex optimization algorithms.
    // For example, sorting rules by actor index or condition type.
    // This function would be critical for achieving 8T performance.
    // printf("  (Conceptual) Optimizing %zu rules...\n", rule_set->num_rules);
}

// Compile a set of rules from text into a RuleSet
RuleSet* compile_rules(BitmaskCompiler* compiler, const char* rules_text) {
    RuleSet* rule_set = create_rule_set(10); // Initial capacity
    if (!rule_set) {
        return NULL;
    }

    char* rules_copy = strdup(rules_text);
    if (!rules_copy) {
        fprintf(stderr, "Error: Failed to duplicate rules_text for parsing.\n");
        destroy_rule_set(rule_set);
        return NULL;
    }

    char* line = strtok(rules_copy, "\n");
    int line_num = 0;

    while (line != NULL) {
        line_num++;
        CompiledRule new_rule = {0}; // Initialize with zeros
        int actor_idx1, bit_pos1, actor_idx2, bit_pos2, actor_idx3, bit_pos3;
        char action_type_str[10];

        // ACTOR X BIT Y SET/CLEAR
        if (sscanf(line, "ACTOR %d BIT %d %9s", &actor_idx1, &bit_pos1, action_type_str) == 3) {
            new_rule.condition_type = CONDITION_NONE;
            new_rule.action_actor_index = actor_idx1;
            new_rule.action_bit_position = bit_pos1;
            if (strcmp(action_type_str, "SET") == 0) {
                new_rule.action_type = ACTION_SET;
            } else if (strcmp(action_type_str, "CLEAR") == 0) {
                new_rule.action_type = ACTION_CLEAR;
            } else {
                fprintf(stderr, "Warning: Line %d: Unknown action type '%s'. Skipping rule.\n", line_num, action_type_str);
                line = strtok(NULL, "\n");
                continue;
            }
            add_rule_to_set(rule_set, new_rule);
        }
        // IF ACTOR X BIT Y THEN ACTOR Z BIT W SET/CLEAR
        else if (sscanf(line, "IF ACTOR %d BIT %d THEN ACTOR %d BIT %d %9s", &actor_idx1, &bit_pos1, &actor_idx2, &bit_pos2, action_type_str) == 5) {
            new_rule.condition_type = CONDITION_SINGLE;
            new_rule.condition_actor_index_1 = actor_idx1;
            new_rule.condition_bit_position_1 = bit_pos1;
            new_rule.action_actor_index = actor_idx2;
            new_rule.action_bit_position = bit_pos2;
            if (strcmp(action_type_str, "SET") == 0) {
                new_rule.action_type = ACTION_SET;
            } else if (strcmp(action_type_str, "CLEAR") == 0) {
                new_rule.action_type = ACTION_CLEAR;
            } else {
                fprintf(stderr, "Warning: Line %d: Unknown action type '%s'. Skipping rule.\n", line_num, action_type_str);
                line = strtok(NULL, "\n");
                continue;
            }
            add_rule_to_set(rule_set, new_rule);
        }
        // IF ACTOR X BIT Y AND ACTOR Z BIT W THEN ACTOR A BIT B SET/CLEAR
        else if (sscanf(line, "IF ACTOR %d BIT %d AND ACTOR %d BIT %d THEN ACTOR %d BIT %d %9s", &actor_idx1, &bit_pos1, &actor_idx2, &bit_pos2, &actor_idx3, &bit_pos3, action_type_str) == 7) {
            new_rule.condition_type = CONDITION_AND;
            new_rule.condition_actor_index_1 = actor_idx1;
            new_rule.condition_bit_position_1 = bit_pos1;
            new_rule.condition_actor_index_2 = actor_idx2;
            new_rule.condition_bit_position_2 = bit_pos2;
            new_rule.action_actor_index = actor_idx3;
            new_rule.action_bit_position = bit_pos3;
            if (strcmp(action_type_str, "SET") == 0) {
                new_rule.action_type = ACTION_SET;
            } else if (strcmp(action_type_str, "CLEAR") == 0) {
                new_rule.action_type = ACTION_CLEAR;
            } else {
                fprintf(stderr, "Warning: Line %d: Unknown action type '%s'. Skipping rule.\n", line_num, action_type_str);
                line = strtok(NULL, "\n");
                continue;
            }
            add_rule_to_set(rule_set, new_rule);
        }
        // IF ACTOR X BIT Y OR ACTOR Z BIT W THEN ACTOR A BIT B SET/CLEAR
        else if (sscanf(line, "IF ACTOR %d BIT %d OR ACTOR %d BIT %d THEN ACTOR %d BIT %d %9s", &actor_idx1, &bit_pos1, &actor_idx2, &bit_pos2, &actor_idx3, &bit_pos3, action_type_str) == 7) {
            new_rule.condition_type = CONDITION_OR;
            new_rule.condition_actor_index_1 = actor_idx1;
            new_rule.condition_bit_position_1 = bit_pos1;
            new_rule.condition_actor_index_2 = actor_idx2;
            new_rule.condition_bit_position_2 = bit_pos2;
            new_rule.action_actor_index = actor_idx3;
            new_rule.action_bit_position = bit_pos3;
            if (strcmp(action_type_str, "SET") == 0) {
                new_rule.action_type = ACTION_SET;
            } else if (strcmp(action_type_str, "CLEAR") == 0) {
                new_rule.action_type = ACTION_CLEAR;
            } else {
                fprintf(stderr, "Warning: Line %d: Unknown action type '%s'. Skipping rule.\n", line_num, action_type_str);
                line = strtok(NULL, "\n");
                continue;
            }
            add_rule_to_set(rule_set, new_rule);
        }
        else {
            fprintf(stderr, "Warning: Line %d: Unrecognized rule format. Skipping line: %s\n", line_num, line);
        }

        line = strtok(NULL, "\n");
    }

    free(rules_copy);

    optimize_rules(rule_set); // Apply conceptual optimization

    return rule_set;
}