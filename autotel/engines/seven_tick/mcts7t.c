#include "mcts7t.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <immintrin.h>

// Default configuration
const MCTSConfig MCTS7T_DEFAULT_CONFIG = {
    .max_iterations = 10000,
    .time_budget_ns = 100000000, // 100ms
    .max_depth = 50,
    .simulation_depth = 20,
    .exploration_constant = 1.414,
    .rng_seed = 42,
    .enable_parallel = false,
    .num_threads = 1};

// Global statistics
static MCTSStats global_stats = {0};

// Fast random number generator (xorshift64*)
static inline uint64_t xorshift64star(uint64_t *state)
{
  uint64_t x = *state;
  x ^= x >> 12;
  x ^= x << 25;
  x ^= x >> 27;
  *state = x;
  return x * 0x2545F4914F6CDD1DULL;
}

// Create MCTS engine
MCTS7TEngine *mcts7t_create(EngineState *engine, MCTSConfig *config)
{
  MCTS7TEngine *mcts = calloc(1, sizeof(MCTS7TEngine));
  if (!mcts)
    return NULL;

  // Copy configuration
  mcts->config = config ? *config : MCTS7T_DEFAULT_CONFIG;
  mcts->engine = engine;

  // Initialize RNG
  mcts->rng_state = malloc(sizeof(uint64_t));
  *mcts->rng_state = mcts->config.rng_seed;

  // Initialize root node with empty state
  uint32_t empty_state[1] = {0};
  mcts->root = mcts7t_create_node(empty_state, 1, 0, NULL);

  return mcts;
}

// Destroy MCTS engine
void mcts7t_destroy(MCTS7TEngine *mcts)
{
  if (!mcts)
    return;

  if (mcts->root)
  {
    mcts7t_destroy_node(mcts->root);
  }

  free(mcts->rng_state);
  free(mcts);
}

// Create MCTS node
MCTSNode *mcts7t_create_node(uint32_t *state, size_t state_size, uint32_t depth, MCTSNode *parent)
{
  MCTSNode *node = calloc(1, sizeof(MCTSNode));
  if (!node)
    return NULL;

  // Copy state
  node->state_vector = malloc(state_size * sizeof(uint32_t));
  memcpy(node->state_vector, state, state_size * sizeof(uint32_t));
  node->state_size = state_size;
  node->depth = depth;
  node->parent = parent;

  // Initialize statistics
  node->total_reward = 0.0;
  node->visit_count = 0;
  node->average_reward = 0.0;

  // Initialize children array
  node->child_capacity = 16;
  node->children = calloc(node->child_capacity, sizeof(MCTSNode *));

  // Link to parent
  if (parent)
  {
    mcts7t_add_child(parent, node);
  }

  global_stats.nodes_created++;
  return node;
}

// Destroy MCTS node
void mcts7t_destroy_node(MCTSNode *node)
{
  if (!node)
    return;

  // Destroy children
  for (size_t i = 0; i < node->child_count; i++)
  {
    mcts7t_destroy_node(node->children[i]);
  }

  free(node->state_vector);
  free(node->children);
  free(node->pattern_order);
  free(node);
}

// Add child to parent
void mcts7t_add_child(MCTSNode *parent, MCTSNode *child)
{
  if (!parent || !child)
    return;

  // Expand capacity if needed
  if (parent->child_count >= parent->child_capacity)
  {
    parent->child_capacity *= 2;
    parent->children = realloc(parent->children,
                               parent->child_capacity * sizeof(MCTSNode *));
  }

  parent->children[parent->child_count++] = child;
  child->parent = parent;
}

// UCB score calculation
double mcts7t_ucb_score(MCTSNode *node, double exploration_constant)
{
  if (node->visit_count == 0)
    return DBL_MAX;

  double exploitation = node->average_reward;
  double exploration = exploration_constant *
                       sqrt(log(node->parent->visit_count) / node->visit_count);

  return exploitation + exploration;
}

// Selection phase
MCTSNode *mcts7t_select(MCTS7TEngine *mcts, MCTSNode *node)
{
  uint64_t start_time = mcts7t_get_time_ns();

  while (node->child_count > 0 && !mcts7t_is_terminal(node->state_vector, node->state_size))
  {
    MCTSNode *best_child = NULL;
    double best_score = -DBL_MAX;

    // Find child with highest UCB score
    for (size_t i = 0; i < node->child_count; i++)
    {
      MCTSNode *child = node->children[i];
      double score = mcts7t_ucb_score(child, mcts->config.exploration_constant);

      if (score > best_score)
      {
        best_score = score;
        best_child = child;
      }
    }

    if (!best_child)
      break;
    node = best_child;
  }

  global_stats.selection_time_ns += mcts7t_get_time_ns() - start_time;
  return node;
}

// Expansion phase
MCTSNode *mcts7t_expand(MCTS7TEngine *mcts, MCTSNode *node)
{
  uint64_t start_time = mcts7t_get_time_ns();

  // Check if we can expand
  if (node->depth >= mcts->config.max_depth)
  {
    global_stats.expansion_time_ns += mcts7t_get_time_ns() - start_time;
    return node;
  }

  // Generate possible actions
  size_t action_count;
  MCTSAction *actions = mcts7t_generate_actions(node, &action_count);

  if (action_count == 0)
  {
    global_stats.expansion_time_ns += mcts7t_get_time_ns() - start_time;
    return node;
  }

  // Select random action
  uint32_t action_idx = mcts7t_random_uint32(mcts->rng_state) % action_count;
  MCTSAction *action = &actions[action_idx];

  // Apply action to create new state
  uint32_t *new_state = mcts7t_apply_action(node->state_vector, node->state_size, action);

  // Create child node
  MCTSNode *child = mcts7t_create_node(new_state, node->state_size, node->depth + 1, node);
  child->action_id = action->action_id;
  child->action_cost = action->action_cost;

  free(new_state);
  mcts7t_destroy_actions(actions, action_count);

  global_stats.expansion_time_ns += mcts7t_get_time_ns() - start_time;
  return child;
}

// Simulation phase
double mcts7t_simulate(MCTS7TEngine *mcts, MCTSNode *node)
{
  uint64_t start_time = mcts7t_get_time_ns();

  // Create simulation state
  uint32_t *sim_state = malloc(node->state_size * sizeof(uint32_t));
  memcpy(sim_state, node->state_vector, node->state_size * sizeof(uint32_t));

  double total_reward = 0.0;
  uint32_t simulation_steps = 0;

  // Random rollout
  while (simulation_steps < mcts->config.simulation_depth &&
         !mcts7t_is_terminal(sim_state, node->state_size))
  {

    // Generate actions
    size_t action_count;
    MCTSAction *actions = mcts7t_generate_actions(node, &action_count);

    if (action_count == 0)
      break;

    // Select random action
    uint32_t action_idx = mcts7t_random_uint32(mcts->rng_state) % action_count;
    MCTSAction *action = &actions[action_idx];

    // Apply action
    uint32_t *new_state = mcts7t_apply_action(sim_state, node->state_size, action);
    memcpy(sim_state, new_state, node->state_size * sizeof(uint32_t));

    // Calculate reward
    double reward = mcts7t_calculate_reward(node, action);
    total_reward += reward;

    free(new_state);
    mcts7t_destroy_actions(actions, action_count);

    simulation_steps++;
  }

  free(sim_state);

  global_stats.simulation_time_ns += mcts7t_get_time_ns() - start_time;
  global_stats.simulations_performed++;

  return total_reward;
}

// Backpropagation phase
void mcts7t_backpropagate(MCTS7TEngine *mcts, MCTSNode *node, double reward)
{
  uint64_t start_time = mcts7t_get_time_ns();

  while (node != NULL)
  {
    node->visit_count++;
    node->total_reward += reward;
    node->average_reward = node->total_reward / node->visit_count;

    global_stats.nodes_visited++;
    node = node->parent;
  }

  global_stats.backpropagation_time_ns += mcts7t_get_time_ns() - start_time;
}

// Main MCTS step
bool mcts7t_step(MCTS7TEngine *mcts)
{
  if (!mcts || !mcts->root)
    return false;

  // Check time budget
  uint64_t current_time = mcts7t_get_time_ns();
  if (current_time - mcts->start_time_ns > mcts->config.time_budget_ns)
  {
    return false;
  }

  // Check iteration limit
  if (mcts->iterations_completed >= mcts->config.max_iterations)
  {
    return false;
  }

  // MCTS phases
  MCTSNode *selected = mcts7t_select(mcts, mcts->root);
  MCTSNode *expanded = mcts7t_expand(mcts, selected);
  double reward = mcts7t_simulate(mcts, expanded);
  mcts7t_backpropagate(mcts, expanded, reward);

  mcts->iterations_completed++;
  return true;
}

// Main MCTS search
MCTSResult *mcts7t_search(MCTS7TEngine *mcts, uint32_t *initial_state, size_t state_size)
{
  if (!mcts || !initial_state)
    return NULL;

  // Reset statistics
  mcts7t_reset_stats(mcts);
  mcts->start_time_ns = mcts7t_get_time_ns();
  mcts->iterations_completed = 0;

  // Update root state
  if (mcts->root)
  {
    mcts7t_destroy_node(mcts->root);
  }
  mcts->root = mcts7t_create_node(initial_state, state_size, 0, NULL);

  // Run MCTS iterations
  while (mcts7t_step(mcts))
  {
    // Continue until time budget or iteration limit
  }

  // Extract best path
  MCTSResult *result = malloc(sizeof(MCTSResult));
  result->computation_time_ns = mcts7t_get_time_ns() - mcts->start_time_ns;
  result->iterations_performed = mcts->iterations_completed;

  // Find best action sequence
  MCTSNode *current = mcts->root;
  result->best_actions = malloc(mcts->config.max_depth * sizeof(uint32_t));
  result->action_count = 0;
  result->total_reward = 0.0;

  while (current->child_count > 0)
  {
    // Find best child by visit count
    MCTSNode *best_child = current->children[0];
    for (size_t i = 1; i < current->child_count; i++)
    {
      if (current->children[i]->visit_count > best_child->visit_count)
      {
        best_child = current->children[i];
      }
    }

    result->best_actions[result->action_count++] = best_child->action_id;
    result->total_reward += best_child->average_reward;
    current = best_child;
  }

  result->confidence = (double)current->visit_count / mcts->iterations_completed;

  return result;
}

// Action generation for 7T engine integration
MCTSAction *mcts7t_generate_actions(MCTSNode *node, size_t *action_count)
{
  if (!node || !node->engine_state)
  {
    *action_count = 0;
    return NULL;
  }

  // Generate actions based on current state
  // This is a simplified version - in practice, you'd analyze the engine state
  // to determine what operations are possible

  *action_count = 3; // SPARQL, SHACL, OWL actions
  MCTSAction *actions = malloc(*action_count * sizeof(MCTSAction));

  // SPARQL action
  actions[0].action_id = 1;
  actions[0].action_type = 1;       // SPARQL
  actions[0].estimated_cost = 1.44; // 1.44ns from benchmarks
  actions[0].estimated_reward = 0.8;

  // SHACL action
  actions[1].action_id = 2;
  actions[1].action_type = 2;       // SHACL
  actions[1].estimated_cost = 1.43; // 1.43ns from benchmarks
  actions[1].estimated_reward = 0.7;

  // OWL action
  actions[2].action_id = 3;
  actions[2].action_type = 3;      // OWL
  actions[2].estimated_cost = 2.0; // Estimated
  actions[2].estimated_reward = 0.9;

  return actions;
}

// Apply action to state
uint32_t *mcts7t_apply_action(uint32_t *state, size_t state_size, MCTSAction *action)
{
  uint32_t *new_state = malloc(state_size * sizeof(uint32_t));
  memcpy(new_state, state, state_size * sizeof(uint32_t));

  // Apply action effects to state
  // This is simplified - in practice, you'd update the state based on the action

  return new_state;
}

// Check if state is terminal
bool mcts7t_is_terminal(uint32_t *state, size_t state_size)
{
  // Simplified terminal condition
  return state[0] > 1000; // Arbitrary threshold
}

// Calculate reward for action
double mcts7t_calculate_reward(MCTSNode *node, MCTSAction *action)
{
  if (!node || !action)
    return 0.0;

  // Base reward from action
  double reward = action->estimated_reward;

  // Penalty for cost
  reward -= action->estimated_cost * 0.001; // Small penalty for cost

  // Bonus for depth (encourage exploration)
  reward += node->depth * 0.01;

  return reward;
}

// 7T Engine specific reward functions
double mcts7t_sparql_reward(EngineState *engine, uint32_t *patterns, size_t pattern_count)
{
  if (!engine || !patterns)
    return 0.0;

  // Calculate reward based on SPARQL performance
  double reward = 0.0;

  for (size_t i = 0; i < pattern_count; i++)
  {
    // Simulate pattern matching
    uint32_t s = patterns[i * 3];
    uint32_t p = patterns[i * 3 + 1];
    uint32_t o = patterns[i * 3 + 2];

    // High reward for successful matches
    if (s7t_ask_pattern(engine, s, p, o))
    {
      reward += 1.0;
    }
  }

  return reward / pattern_count;
}

double mcts7t_shacl_reward(EngineState *engine, uint32_t *shapes, size_t shape_count)
{
  if (!engine || !shapes)
    return 0.0;

  // Calculate reward based on SHACL validation
  double reward = 0.0;

  for (size_t i = 0; i < shape_count; i++)
  {
    // Simulate shape validation
    uint32_t node_id = shapes[i * 2];
    uint32_t shape_id = shapes[i * 2 + 1];

    // High reward for valid shapes
    if (shacl7t_validate_node(engine, node_id, shape_id))
    {
      reward += 1.0;
    }
  }

  return reward / shape_count;
}

double mcts7t_owl_reward(EngineState *engine, uint32_t *axioms, size_t axiom_count)
{
  if (!engine || !axioms)
    return 0.0;

  // Calculate reward based on OWL reasoning
  double reward = 0.0;

  for (size_t i = 0; i < axiom_count; i++)
  {
    // Simulate axiom processing
    uint32_t axiom_type = axioms[i * 3];
    uint32_t entity1 = axioms[i * 3 + 1];
    uint32_t entity2 = axioms[i * 3 + 2];

    // High reward for successful reasoning
    reward += 1.0;
  }

  return reward / axiom_count;
}

// Utility functions
uint64_t mcts7t_get_time_ns(void)
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

uint32_t mcts7t_random_uint32(uint64_t *rng_state)
{
  return (uint32_t)xorshift64star(rng_state);
}

double mcts7t_random_double(uint64_t *rng_state)
{
  uint64_t raw = xorshift64star(rng_state);
  return (double)(raw & 0xFFFFFFFF) / 0xFFFFFFFF;
}

// Statistics
MCTSStats *mcts7t_get_stats(MCTS7TEngine *mcts)
{
  return &global_stats;
}

void mcts7t_reset_stats(MCTS7TEngine *mcts)
{
  memset(&global_stats, 0, sizeof(MCTSStats));
}

// Cleanup functions
void mcts7t_destroy_actions(MCTSAction *actions, size_t count)
{
  if (actions)
  {
    for (size_t i = 0; i < count; i++)
    {
      free(actions[i].parameters);
    }
    free(actions);
  }
}