#ifndef BITMASK_COMPILER_H
#define BITMASK_COMPILER_H

#include "bitactor.h"

// Defines the type of action a rule performs
typedef enum {
    ACTION_SET,
    ACTION_CLEAR
} RuleActionType;

// Defines the type of condition for a rule
typedef enum {
    CONDITION_NONE,
    CONDITION_SINGLE,
    CONDITION_AND,
    CONDITION_OR
} RuleConditionType;

// Represents a single compiled rule
typedef struct {
    RuleConditionType condition_type;
    // For CONDITION_SINGLE, CONDITION_AND, CONDITION_OR
    int condition_actor_index_1;
    int condition_bit_position_1;
    // For CONDITION_AND, CONDITION_OR
    int condition_actor_index_2;
    int condition_bit_position_2;

    int action_actor_index;
    int action_bit_position;
    RuleActionType action_type;
} CompiledRule;

// Represents a set of compiled rules
typedef struct {
    CompiledRule* rules;
    size_t num_rules;
    size_t capacity;
} RuleSet;

// The BitMask Compiler
typedef struct {
    int placeholder;
} BitmaskCompiler;

// Function declarations
BitmaskCompiler* create_bitmask_compiler();
void destroy_bitmask_compiler(BitmaskCompiler* compiler);
RuleSet* create_rule_set(size_t initial_capacity);
void destroy_rule_set(RuleSet* rule_set);
void add_rule_to_set(RuleSet* rule_set, CompiledRule rule);

// New signature: compile_rules now returns a RuleSet*
RuleSet* compile_rules(BitmaskCompiler* compiler, const char* rules_text);

#endif // BITMASK_COMPILER_H
