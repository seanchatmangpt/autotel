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
        return NULL;
    }
    rule_set->rules = (CompiledRule*)malloc(sizeof(CompiledRule) * initial_capacity);
    if (!rule_set->rules) {
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
    if (rule_set->num_rules == rule_set->capacity) {
        rule_set->capacity *= 2;
        rule_set->rules = (CompiledRule*)realloc(rule_set->rules, sizeof(CompiledRule) * rule_set->capacity);
        if (!rule_set->rules) {
            // Handle realloc failure (e.g., print error and exit or return error code)
            fprintf(stderr, "Failed to reallocate memory for RuleSet\n");
            exit(EXIT_FAILURE);
        }
    }
    rule_set->rules[rule_set->num_rules++] = rule;
}

// Compile a set of rules from text into a RuleSet
RuleSet* compile_rules(BitmaskCompiler* compiler, const char* rules_text) {
    RuleSet* rule_set = create_rule_set(10); // Initial capacity
    if (!rule_set) {
        return NULL;
    }

    char* rules_copy = strdup(rules_text);
    char* line = strtok(rules_copy, "\n");

    while (line != NULL) {
        CompiledRule new_rule = {0}; // Initialize with zeros
        int actor_idx1, bit_pos1, actor_idx2, bit_pos2, actor_idx3, bit_pos3;
        char command[10];
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
            }
            add_rule_to_set(rule_set, new_rule);
        }

        line = strtok(NULL, "\n");
    }

    free(rules_copy);
    return rule_set;
}