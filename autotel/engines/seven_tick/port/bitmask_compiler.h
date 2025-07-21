#ifndef BITMASK_COMPILER_H
#define BITMASK_COMPILER_H

#include "bitactor.h"

// Defines the type of action a rule performs
typedef enum {
    ACTION_SET,
    ACTION_CLEAR
} RuleActionType;

// Represents a single compiled rule
typedef struct {
    int condition_actor_index;
    int condition_bit_position;
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
RuleSet* compile_rules(BitmaskCompiler* compiler, const char* rules_text);

#endif // BITMASK_COMPILER_H