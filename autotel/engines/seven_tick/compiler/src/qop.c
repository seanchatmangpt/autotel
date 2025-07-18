#include "qop.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

#define UCB_C 1.41421356237  // sqrt(2)
#define SIMULATION_DEPTH 5

// MCTS node management
MCTSNode* create_node(int* partial_order, size_t depth, int* remaining, size_t remaining_count) {
    MCTSNode* node = calloc(1, sizeof(MCTSNode));
    
    node->partial_order = malloc(depth * sizeof(int));
    memcpy(node->partial_order, partial_order, depth * sizeof(int));
    node->depth = depth;
    
    node->remaining = malloc(remaining_count * sizeof(int));
    memcpy(node->remaining, remaining, remaining_count * sizeof(int));
    node->remaining_count = remaining_count;
    
    return node;
}

void destroy_node(MCTSNode* node) {
    free(node->partial_order);
    free(node->remaining);
    for (size_t i = 0; i < node->child_count; i++) {
        destroy_node(node->children[i]);
    }
    free(node->children);
    free(node);
}

// UCB1 selection
MCTSNode* select_child(MCTSNode* node) {
    MCTSNode* best = NULL;
    double best_ucb = -DBL_MAX;
    
    for (size_t i = 0; i < node->child_count; i++) {
        MCTSNode* child = node->children[i];
        double exploitation = child->total_reward / (child->visit_count + 1e-6);
        double exploration = UCB_C * sqrt(log(node->visit_count + 1) / (child->visit_count + 1e-6));
        double ucb = exploitation + exploration;
        
        if (ucb > best_ucb) {
            best_ucb = ucb;
            best = child;
        }
    }
    
    return best;
}

// Cost estimation
double estimate_pattern_cost(Pattern* pattern, CostModel* model) {
    double selectivity = 1.0;
    
    if (pattern->predicate >= 0 && pattern->predicate <= model->max_predicate_id) {
        selectivity *= model->predicate_selectivities[pattern->predicate];
    }
    
    if (pattern->object >= 0 && pattern->object <= model->max_object_id) {
        selectivity *= 1.0 / (model->object_cardinalities[pattern->object] + 1);
    }
    
    return model->total_triples * selectivity;
}

double estimate_join_cost(Pattern* p1, Pattern* p2, double card1, double card2) {
    // Simple nested loop join cost model
    return card1 * card2 * 0.001;  // 0.001 is join factor
}
// Simulation phase
double simulate(int* order, size_t order_len, Pattern* patterns, CostModel* model) {
    double total_cost = 0.0;
    double current_cardinality = model->total_triples;
    
    for (size_t i = 0; i < order_len; i++) {
        Pattern* p = &patterns[order[i]];
        double pattern_cost = estimate_pattern_cost(p, model);
        
        if (i > 0) {
            // Add join cost
            total_cost += estimate_join_cost(&patterns[order[i-1]], p, 
                                           current_cardinality, pattern_cost);
        }
        
        current_cardinality = pattern_cost;
        total_cost += pattern_cost;
    }
    
    // Return negative cost as reward (lower cost = higher reward)
    return -total_cost;
}

// Expansion phase
void expand(MCTSNode* node, Pattern* patterns, CostModel* model) {
    node->children = calloc(node->remaining_count, sizeof(MCTSNode*));
    
    for (size_t i = 0; i < node->remaining_count; i++) {
        // Create new partial order
        int* new_order = malloc((node->depth + 1) * sizeof(int));
        memcpy(new_order, node->partial_order, node->depth * sizeof(int));
        new_order[node->depth] = node->remaining[i];
        
        // Create new remaining set
        int* new_remaining = malloc((node->remaining_count - 1) * sizeof(int));
        size_t idx = 0;
        for (size_t j = 0; j < node->remaining_count; j++) {
            if (j != i) {
                new_remaining[idx++] = node->remaining[j];
            }
        }
        
        MCTSNode* child = create_node(new_order, node->depth + 1, 
                                     new_remaining, node->remaining_count - 1);
        child->parent = node;
        node->children[node->child_count++] = child;
        
        free(new_order);
        free(new_remaining);
    }
}

// Main MCTS algorithm
JoinPlan* mcts_optimize_query(Pattern* patterns, size_t pattern_count, 
                             CostModel* cost_model, size_t iterations) {
    // Initialize root
    int* initial_remaining = malloc(pattern_count * sizeof(int));
    for (size_t i = 0; i < pattern_count; i++) {
        initial_remaining[i] = i;
    }
    
    MCTSNode* root = create_node(NULL, 0, initial_remaining, pattern_count);
    free(initial_remaining);
    
    // Run MCTS iterations
    for (size_t iter = 0; iter < iterations; iter++) {
        MCTSNode* current = root;
        
        // Selection
        while (current->child_count > 0 && current->remaining_count > 0) {
            current = select_child(current);
        }
        
        // Expansion
        if (current->remaining_count > 0 && current->visit_count > 0) {
            expand(current, patterns, cost_model);
            if (current->child_count > 0) {
                current = current->children[0];
            }
        }
        
        // Simulation
        double reward = 0.0;
        if (current->remaining_count == 0) {
            // Terminal node - use actual cost
            reward = simulate(current->partial_order, current->depth, patterns, cost_model);
        } else {
            // Non-terminal - random rollout
            int* sim_order = malloc(pattern_count * sizeof(int));
            memcpy(sim_order, current->partial_order, current->depth * sizeof(int));
            
            // Add remaining in random order
            for (size_t i = 0; i < current->remaining_count; i++) {
                sim_order[current->depth + i] = current->remaining[i];
            }
            
            reward = simulate(sim_order, pattern_count, patterns, cost_model);
            free(sim_order);
        }
        
        // Backpropagation
        while (current != NULL) {
            current->visit_count++;
            current->total_reward += reward;
            current = current->parent;
        }
    }
    
    // Extract best plan
    JoinPlan* plan = malloc(sizeof(JoinPlan));
    plan->order = malloc(pattern_count * sizeof(int));
    plan->length = pattern_count;
    
    MCTSNode* current = root;
    for (size_t i = 0; i < pattern_count; i++) {
        if (current->child_count == 0) break;
        
        // Select best child by average reward
        MCTSNode* best_child = current->children[0];
        double best_avg = best_child->total_reward / (best_child->visit_count + 1e-6);
        
        for (size_t j = 1; j < current->child_count; j++) {
            double avg = current->children[j]->total_reward / 
                        (current->children[j]->visit_count + 1e-6);
            if (avg > best_avg) {
                best_avg = avg;
                best_child = current->children[j];
            }
        }
        
        plan->order[i] = best_child->partial_order[i];
        current = best_child;
    }
    
    plan->cost = -current->total_reward / current->visit_count;
    
    destroy_node(root);
    return plan;
}
// Cost model creation
CostModel* create_cost_model(void* engine_state) {
    CostModel* model = calloc(1, sizeof(CostModel));
    
    // In a real implementation, this would analyze the engine state
    // For MVP, we use simple heuristics
    model->total_triples = 10000;  // Placeholder
    model->max_predicate_id = 100;
    model->max_object_id = 10000;
    
    model->predicate_cardinalities = calloc(model->max_predicate_id + 1, sizeof(size_t));
    model->object_cardinalities = calloc(model->max_object_id + 1, sizeof(size_t));
    model->predicate_selectivities = calloc(model->max_predicate_id + 1, sizeof(double));
    
    // Initialize with default selectivities
    for (size_t i = 0; i <= model->max_predicate_id; i++) {
        model->predicate_selectivities[i] = 0.1;  // 10% selectivity default
    }
    
    return model;
}

void destroy_cost_model(CostModel* model) {
    free(model->predicate_cardinalities);
    free(model->object_cardinalities);
    free(model->predicate_selectivities);
    free(model);
}
