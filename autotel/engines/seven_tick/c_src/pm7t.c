#include "pm7t.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

// Global memory management
static size_t memory_limit = SIZE_MAX;
static size_t current_memory_usage = 0;

// Memory management functions
void pm7t_set_memory_limit(size_t bytes)
{
  memory_limit = bytes;
}

size_t pm7t_get_memory_usage(void)
{
  return current_memory_usage;
}

static void *pm7t_malloc(size_t size)
{
  if (current_memory_usage + size > memory_limit)
  {
    return NULL;
  }
  void *ptr = malloc(size);
  if (ptr)
  {
    current_memory_usage += size;
  }
  return ptr;
}

static void pm7t_free(void *ptr, size_t size)
{
  if (ptr)
  {
    free(ptr);
    if (current_memory_usage >= size)
    {
      current_memory_usage -= size;
    }
    else
    {
      current_memory_usage = 0; // Prevent underflow
    }
  }
}

// Event log management
EventLog *pm7t_create_event_log(size_t initial_capacity)
{
  EventLog *log = pm7t_malloc(sizeof(EventLog));
  if (!log)
    return NULL;

  log->events = pm7t_malloc(initial_capacity * sizeof(Event));
  if (!log->events)
  {
    pm7t_free(log, sizeof(EventLog));
    return NULL;
  }

  log->capacity = initial_capacity;
  log->size = 0;
  return log;
}

void pm7t_destroy_event_log(EventLog *log)
{
  if (log)
  {
    pm7t_free(log->events, log->capacity * sizeof(Event));
    pm7t_free(log, sizeof(EventLog));
  }
}

int pm7t_add_event(EventLog *log, uint32_t case_id, uint32_t activity_id,
                   uint64_t timestamp, uint32_t resource_id, uint32_t cost)
{
  if (!log)
    return -1;

  if (log->size >= log->capacity)
  {
    size_t new_capacity = log->capacity * 2;
    Event *new_events = pm7t_malloc(new_capacity * sizeof(Event));
    if (!new_events)
      return -1;

    memcpy(new_events, log->events, log->size * sizeof(Event));
    pm7t_free(log->events, log->capacity * sizeof(Event));

    log->events = new_events;
    log->capacity = new_capacity;
  }

  log->events[log->size].case_id = case_id;
  log->events[log->size].activity_id = activity_id;
  log->events[log->size].timestamp = timestamp;
  log->events[log->size].resource_id = resource_id;
  log->events[log->size].cost = cost;
  log->size++;

  return 0;
}

Event *pm7t_get_event(EventLog *log, size_t index)
{
  if (!log || index >= log->size)
    return NULL;
  return &log->events[index];
}

size_t pm7t_get_event_count(EventLog *log)
{
  return log ? log->size : 0;
}

// Trace extraction
TraceLog *pm7t_extract_traces(EventLog *event_log)
{
  if (!event_log)
    return NULL;

  // Sort events by case and timestamp
  pm7t_sort_events_by_case(event_log);

  // Count unique cases
  uint32_t unique_cases = pm7t_get_unique_cases(event_log);

  TraceLog *trace_log = pm7t_malloc(sizeof(TraceLog));
  if (!trace_log)
    return NULL;

  trace_log->traces = pm7t_malloc(unique_cases * sizeof(Trace));
  if (!trace_log->traces)
  {
    pm7t_free(trace_log, sizeof(TraceLog));
    return NULL;
  }

  trace_log->capacity = unique_cases;
  trace_log->size = 0;

  // Extract traces
  uint32_t current_case = 0;
  size_t trace_start = 0;

  for (size_t i = 0; i < event_log->size; i++)
  {
    if (event_log->events[i].case_id != current_case)
    {
      // Complete current trace
      if (i > trace_start)
      {
        size_t trace_length = i - trace_start;
        trace_log->traces[trace_log->size].activities = pm7t_malloc(trace_length * sizeof(uint32_t));
        trace_log->traces[trace_log->size].size = trace_length;

        for (size_t j = 0; j < trace_length; j++)
        {
          trace_log->traces[trace_log->size].activities[j] =
              event_log->events[trace_start + j].activity_id;
        }
        trace_log->size++;
      }

      current_case = event_log->events[i].case_id;
      trace_start = i;
    }
  }

  // Handle last trace
  if (event_log->size > trace_start)
  {
    size_t trace_length = event_log->size - trace_start;
    trace_log->traces[trace_log->size].activities = pm7t_malloc(trace_length * sizeof(uint32_t));
    trace_log->traces[trace_log->size].size = trace_length;

    for (size_t j = 0; j < trace_length; j++)
    {
      trace_log->traces[trace_log->size].activities[j] =
          event_log->events[trace_start + j].activity_id;
    }
    trace_log->size++;
  }

  return trace_log;
}

void pm7t_destroy_trace_log(TraceLog *trace_log)
{
  if (trace_log)
  {
    for (size_t i = 0; i < trace_log->size; i++)
    {
      pm7t_free(trace_log->traces[i].activities, trace_log->traces[i].size * sizeof(uint32_t));
    }
    pm7t_free(trace_log->traces, trace_log->capacity * sizeof(Trace));
    pm7t_free(trace_log, sizeof(TraceLog));
  }
}

Trace *pm7t_get_trace(TraceLog *trace_log, size_t index)
{
  if (!trace_log || index >= trace_log->size)
    return NULL;
  return &trace_log->traces[index];
}

size_t pm7t_get_trace_count(TraceLog *trace_log)
{
  return trace_log ? trace_log->size : 0;
}

// Alpha algorithm implementation
ProcessModel *pm7t_discover_alpha_algorithm(TraceLog *trace_log)
{
  if (!trace_log)
    return NULL;

  ProcessModel *model = pm7t_malloc(sizeof(ProcessModel));
  if (!model)
    return NULL;

  // Count unique activities
  uint32_t max_activity = 0;
  for (size_t i = 0; i < trace_log->size; i++)
  {
    for (size_t j = 0; j < trace_log->traces[i].size; j++)
    {
      if (trace_log->traces[i].activities[j] > max_activity)
      {
        max_activity = trace_log->traces[i].activities[j];
      }
    }
  }
  model->num_activities = max_activity + 1;

  // Initialize transitions
  size_t max_transitions = model->num_activities * model->num_activities;
  model->transitions = pm7t_malloc(max_transitions * sizeof(Transition));
  if (!model->transitions)
  {
    pm7t_free(model, sizeof(ProcessModel));
    return NULL;
  }

  model->capacity = max_transitions;
  model->size = 0;

  // Build frequency matrix
  uint32_t **frequency_matrix = pm7t_malloc(model->num_activities * sizeof(uint32_t *));
  for (uint32_t i = 0; i < model->num_activities; i++)
  {
    frequency_matrix[i] = calloc(model->num_activities, sizeof(uint32_t));
  }

  // Count direct follows
  for (size_t i = 0; i < trace_log->size; i++)
  {
    for (size_t j = 0; j < trace_log->traces[i].size - 1; j++)
    {
      uint32_t from = trace_log->traces[i].activities[j];
      uint32_t to = trace_log->traces[i].activities[j + 1];
      frequency_matrix[from][to]++;
    }
  }

  // Create transitions based on frequency
  for (uint32_t i = 0; i < model->num_activities; i++)
  {
    for (uint32_t j = 0; j < model->num_activities; j++)
    {
      if (frequency_matrix[i][j] > 0)
      {
        model->transitions[model->size].from_activity = i;
        model->transitions[model->size].to_activity = j;
        model->transitions[model->size].frequency = frequency_matrix[i][j];
        model->transitions[model->size].probability = (double)frequency_matrix[i][j] / trace_log->size;
        model->size++;
      }
    }
  }

  // Cleanup frequency matrix
  for (uint32_t i = 0; i < model->num_activities; i++)
  {
    free(frequency_matrix[i]);
  }
  pm7t_free(frequency_matrix, model->num_activities * sizeof(uint32_t *));

  return model;
}

// Heuristic miner implementation
ProcessModel *pm7t_discover_heuristic_miner(TraceLog *trace_log, double dependency_threshold)
{
  if (!trace_log)
    return NULL;

  ProcessModel *model = pm7t_malloc(sizeof(ProcessModel));
  if (!model)
    return NULL;

  // Count unique activities
  uint32_t max_activity = 0;
  for (size_t i = 0; i < trace_log->size; i++)
  {
    for (size_t j = 0; j < trace_log->traces[i].size; j++)
    {
      if (trace_log->traces[i].activities[j] > max_activity)
      {
        max_activity = trace_log->traces[i].activities[j];
      }
    }
  }
  model->num_activities = max_activity + 1;

  // Build dependency matrix
  uint32_t **dependency_matrix = pm7t_malloc(model->num_activities * sizeof(uint32_t *));
  uint32_t **frequency_matrix = pm7t_malloc(model->num_activities * sizeof(uint32_t *));

  for (uint32_t i = 0; i < model->num_activities; i++)
  {
    dependency_matrix[i] = calloc(model->num_activities, sizeof(uint32_t));
    frequency_matrix[i] = calloc(model->num_activities, sizeof(uint32_t));
  }

  // Calculate dependencies
  for (size_t i = 0; i < trace_log->size; i++)
  {
    for (size_t j = 0; j < trace_log->traces[i].size - 1; j++)
    {
      uint32_t from = trace_log->traces[i].activities[j];
      uint32_t to = trace_log->traces[i].activities[j + 1];
      frequency_matrix[from][to]++;
    }
  }

  // Calculate dependency measures
  for (uint32_t i = 0; i < model->num_activities; i++)
  {
    for (uint32_t j = 0; j < model->num_activities; j++)
    {
      if (i != j)
      {
        uint32_t forward = frequency_matrix[i][j];
        uint32_t backward = frequency_matrix[j][i];

        if (forward > 0 || backward > 0)
        {
          double dependency = (double)(forward - backward) / (forward + backward + 1);
          dependency_matrix[i][j] = (uint32_t)(dependency * 1000); // Scale for integer storage
        }
      }
    }
  }

  // Create transitions based on dependency threshold
  size_t max_transitions = model->num_activities * model->num_activities;
  model->transitions = pm7t_malloc(max_transitions * sizeof(Transition));
  if (!model->transitions)
  {
    pm7t_free(model, sizeof(ProcessModel));
    return NULL;
  }

  model->capacity = max_transitions;
  model->size = 0;

  for (uint32_t i = 0; i < model->num_activities; i++)
  {
    for (uint32_t j = 0; j < model->num_activities; j++)
    {
      if (i != j)
      {
        double dependency = (double)dependency_matrix[i][j] / 1000.0;
        if (dependency > dependency_threshold)
        {
          model->transitions[model->size].from_activity = i;
          model->transitions[model->size].to_activity = j;
          model->transitions[model->size].frequency = frequency_matrix[i][j];
          model->transitions[model->size].probability = dependency;
          model->size++;
        }
      }
    }
  }

  // Cleanup matrices
  for (uint32_t i = 0; i < model->num_activities; i++)
  {
    free(dependency_matrix[i]);
    free(frequency_matrix[i]);
  }
  pm7t_free(dependency_matrix, model->num_activities * sizeof(uint32_t *));
  pm7t_free(frequency_matrix, model->num_activities * sizeof(uint32_t *));

  return model;
}

// Inductive miner implementation (simplified)
ProcessModel *pm7t_discover_inductive_miner(TraceLog *trace_log)
{
  // Simplified inductive miner - uses frequency-based approach
  return pm7t_discover_heuristic_miner(trace_log, 0.1);
}

void pm7t_destroy_process_model(ProcessModel *model)
{
  if (model)
  {
    pm7t_free(model->transitions, model->capacity * sizeof(Transition));
    pm7t_free(model, sizeof(ProcessModel));
  }
}

// Process analysis
ProcessStats *pm7t_analyze_process(EventLog *event_log)
{
  if (!event_log)
    return NULL;

  ProcessStats *stats = pm7t_malloc(sizeof(ProcessStats));
  if (!stats)
    return NULL;

  // Count unique activities
  uint32_t max_activity = 0;
  for (size_t i = 0; i < event_log->size; i++)
  {
    if (event_log->events[i].activity_id > max_activity)
    {
      max_activity = event_log->events[i].activity_id;
    }
  }

  stats->activities = pm7t_malloc((max_activity + 1) * sizeof(ActivityStats));
  if (!stats->activities)
  {
    pm7t_free(stats, sizeof(ProcessStats));
    return NULL;
  }

  stats->capacity = max_activity + 1;
  stats->size = 0;

  // Initialize activity stats
  for (uint32_t i = 0; i <= max_activity; i++)
  {
    stats->activities[i].activity_id = i;
    stats->activities[i].frequency = 0;
    stats->activities[i].avg_duration = 0.0;
    stats->activities[i].min_duration = UINT64_MAX;
    stats->activities[i].max_duration = 0;
  }

  // Calculate statistics
  for (size_t i = 0; i < event_log->size; i++)
  {
    uint32_t activity = event_log->events[i].activity_id;
    stats->activities[activity].frequency++;

    // Duration calculation (simplified - using timestamp as duration)
    uint64_t duration = event_log->events[i].timestamp;
    if (duration < stats->activities[activity].min_duration)
    {
      stats->activities[activity].min_duration = duration;
    }
    if (duration > stats->activities[activity].max_duration)
    {
      stats->activities[activity].max_duration = duration;
    }
  }

  // Calculate average durations
  for (uint32_t i = 0; i <= max_activity; i++)
  {
    if (stats->activities[i].frequency > 0)
    {
      stats->activities[i].avg_duration = (double)stats->activities[i].max_duration / stats->activities[i].frequency;
    }
  }

  stats->size = max_activity + 1;
  return stats;
}

void pm7t_destroy_process_stats(ProcessStats *stats)
{
  if (stats)
  {
    pm7t_free(stats->activities, stats->capacity * sizeof(ActivityStats));
    pm7t_free(stats, sizeof(ProcessStats));
  }
}

// Conformance checking
double pm7t_calculate_fitness(ProcessModel *model, TraceLog *trace_log)
{
  if (!model || !trace_log || trace_log->size == 0)
    return 0.0;

  // Simple fitness calculation: percentage of traces that can be replayed
  // This is a simplified version - real fitness would be more complex
  size_t replayable_traces = 0;

  for (size_t i = 0; i < trace_log->size; i++)
  {
    Trace *trace = &trace_log->traces[i];
    bool can_replay = true;

    // Check if all transitions in the trace exist in the model
    for (size_t j = 0; j < trace->size - 1; j++)
    {
      bool transition_found = false;
      for (size_t k = 0; k < model->size; k++)
      {
        if (model->transitions[k].from_activity == trace->activities[j] &&
            model->transitions[k].to_activity == trace->activities[j + 1])
        {
          transition_found = true;
          break;
        }
      }
      if (!transition_found)
      {
        can_replay = false;
        break;
      }
    }

    if (can_replay)
    {
      replayable_traces++;
    }
  }

  return (double)replayable_traces / trace_log->size;
}

double pm7t_calculate_precision(ProcessModel *model, TraceLog *trace_log)
{
  if (!model || !trace_log || trace_log->size == 0)
    return 0.0;

  // Simple precision calculation: percentage of model transitions that are actually used
  // This is a simplified version - real precision would be more complex
  size_t used_transitions = 0;

  for (size_t i = 0; i < model->size; i++)
  {
    bool transition_used = false;

    for (size_t j = 0; j < trace_log->size; j++)
    {
      Trace *trace = &trace_log->traces[j];
      for (size_t k = 0; k < trace->size - 1; k++)
      {
        if (model->transitions[i].from_activity == trace->activities[k] &&
            model->transitions[i].to_activity == trace->activities[k + 1])
        {
          transition_used = true;
          break;
        }
      }
      if (transition_used)
        break;
    }

    if (transition_used)
    {
      used_transitions++;
    }
  }

  return (double)used_transitions / model->size;
}

double pm7t_calculate_generalization(ProcessModel *model, TraceLog *trace_log)
{
  // Simplified generalization calculation
  if (!model || !trace_log)
    return 0.0;

  uint32_t unique_transitions_in_log = 0;
  uint32_t unique_transitions_in_model = model->size;

  // Count unique transitions in log
  bool **transition_matrix = pm7t_malloc(model->num_activities * sizeof(bool *));
  for (uint32_t i = 0; i < model->num_activities; i++)
  {
    transition_matrix[i] = calloc(model->num_activities, sizeof(bool));
  }

  for (size_t i = 0; i < trace_log->size; i++)
  {
    for (size_t j = 0; j < trace_log->traces[i].size - 1; j++)
    {
      uint32_t from = trace_log->traces[i].activities[j];
      uint32_t to = trace_log->traces[i].activities[j + 1];

      if (!transition_matrix[from][to])
      {
        transition_matrix[from][to] = true;
        unique_transitions_in_log++;
      }
    }
  }

  // Cleanup
  for (uint32_t i = 0; i < model->num_activities; i++)
  {
    free(transition_matrix[i]);
  }
  pm7t_free(transition_matrix, model->num_activities * sizeof(bool *));

  return unique_transitions_in_log > 0 ? (double)unique_transitions_in_model / unique_transitions_in_log : 0.0;
}

double pm7t_calculate_simplicity(ProcessModel *model)
{
  if (!model)
    return 0.0;

  // Simplified simplicity calculation based on model size
  uint32_t max_possible_transitions = model->num_activities * model->num_activities;
  return 1.0 - ((double)model->size / max_possible_transitions);
}

ConformanceResult pm7t_check_conformance(ProcessModel *model, TraceLog *trace_log)
{
  ConformanceResult result;
  result.fitness = pm7t_calculate_fitness(model, trace_log);
  result.precision = pm7t_calculate_precision(model, trace_log);
  result.generalization = pm7t_calculate_generalization(model, trace_log);
  result.simplicity = pm7t_calculate_simplicity(model);
  return result;
}

// Performance analysis
PerformanceAnalysis *pm7t_analyze_performance(EventLog *event_log)
{
  if (!event_log)
    return NULL;

  PerformanceAnalysis *analysis = pm7t_malloc(sizeof(PerformanceAnalysis));
  if (!analysis)
    return NULL;

  // Count unique cases
  uint32_t unique_cases = pm7t_get_unique_cases(event_log);

  analysis->cases = pm7t_malloc(unique_cases * sizeof(CasePerformance));
  if (!analysis->cases)
  {
    pm7t_free(analysis, sizeof(PerformanceAnalysis));
    return NULL;
  }

  analysis->capacity = unique_cases;
  analysis->size = 0;

  // Sort events by case and timestamp
  pm7t_sort_events_by_case(event_log);

  // Calculate case performance
  uint32_t current_case = 0;
  size_t case_start = 0;

  for (size_t i = 0; i < event_log->size; i++)
  {
    if (event_log->events[i].case_id != current_case)
    {
      // Complete current case
      if (i > case_start)
      {
        analysis->cases[analysis->size].case_id = current_case;
        analysis->cases[analysis->size].start_time = event_log->events[case_start].timestamp;
        analysis->cases[analysis->size].end_time = event_log->events[i - 1].timestamp;
        analysis->cases[analysis->size].duration =
            event_log->events[i - 1].timestamp - event_log->events[case_start].timestamp;
        analysis->cases[analysis->size].num_activities = i - case_start;
        analysis->size++;
      }

      current_case = event_log->events[i].case_id;
      case_start = i;
    }
  }

  // Handle last case
  if (event_log->size > case_start)
  {
    analysis->cases[analysis->size].case_id = current_case;
    analysis->cases[analysis->size].start_time = event_log->events[case_start].timestamp;
    analysis->cases[analysis->size].end_time = event_log->events[event_log->size - 1].timestamp;
    analysis->cases[analysis->size].duration =
        event_log->events[event_log->size - 1].timestamp - event_log->events[case_start].timestamp;
    analysis->cases[analysis->size].num_activities = event_log->size - case_start;
    analysis->size++;
  }

  // Calculate aggregate statistics
  uint64_t total_duration = 0;
  uint64_t min_duration = UINT64_MAX;
  uint64_t max_duration = 0;

  for (size_t i = 0; i < analysis->size; i++)
  {
    total_duration += analysis->cases[i].duration;
    if (analysis->cases[i].duration < min_duration)
    {
      min_duration = analysis->cases[i].duration;
    }
    if (analysis->cases[i].duration > max_duration)
    {
      max_duration = analysis->cases[i].duration;
    }
  }

  analysis->avg_duration = (double)total_duration / analysis->size;
  analysis->min_duration = (double)min_duration;
  analysis->max_duration = (double)max_duration;

  // Fix throughput calculation to avoid division by zero
  if (analysis->avg_duration > 0)
  {
    analysis->throughput = (double)analysis->size / (analysis->avg_duration / 1000000000.0); // cases per second
  }
  else
  {
    analysis->throughput = 0.0;
  }

  return analysis;
}

void pm7t_destroy_performance_analysis(PerformanceAnalysis *analysis)
{
  if (analysis)
  {
    pm7t_free(analysis->cases, analysis->capacity * sizeof(CasePerformance));
    pm7t_free(analysis, sizeof(PerformanceAnalysis));
  }
}

// Utility functions
void pm7t_sort_events_by_timestamp(EventLog *event_log)
{
  if (!event_log)
    return;

  // Simple bubble sort for small datasets
  for (size_t i = 0; i < event_log->size - 1; i++)
  {
    for (size_t j = 0; j < event_log->size - i - 1; j++)
    {
      if (event_log->events[j].timestamp > event_log->events[j + 1].timestamp)
      {
        Event temp = event_log->events[j];
        event_log->events[j] = event_log->events[j + 1];
        event_log->events[j + 1] = temp;
      }
    }
  }
}

void pm7t_sort_events_by_case(EventLog *event_log)
{
  if (!event_log)
    return;

  // Sort by case_id, then by timestamp
  for (size_t i = 0; i < event_log->size - 1; i++)
  {
    for (size_t j = 0; j < event_log->size - i - 1; j++)
    {
      if (event_log->events[j].case_id > event_log->events[j + 1].case_id ||
          (event_log->events[j].case_id == event_log->events[j + 1].case_id &&
           event_log->events[j].timestamp > event_log->events[j + 1].timestamp))
      {
        Event temp = event_log->events[j];
        event_log->events[j] = event_log->events[j + 1];
        event_log->events[j + 1] = temp;
      }
    }
  }
}

uint32_t pm7t_get_unique_cases(EventLog *event_log)
{
  if (!event_log || event_log->size == 0)
    return 0;

  uint32_t unique_cases = 1;
  uint32_t current_case = event_log->events[0].case_id;

  for (size_t i = 1; i < event_log->size; i++)
  {
    if (event_log->events[i].case_id != current_case)
    {
      unique_cases++;
      current_case = event_log->events[i].case_id;
    }
  }

  return unique_cases;
}

uint32_t pm7t_get_unique_activities(EventLog *event_log)
{
  if (!event_log || event_log->size == 0)
    return 0;

  uint32_t max_activity = 0;
  for (size_t i = 0; i < event_log->size; i++)
  {
    if (event_log->events[i].activity_id > max_activity)
    {
      max_activity = event_log->events[i].activity_id;
    }
  }

  return max_activity + 1;
}

uint32_t pm7t_get_unique_resources(EventLog *event_log)
{
  if (!event_log || event_log->size == 0)
    return 0;

  uint32_t max_resource = 0;
  for (size_t i = 0; i < event_log->size; i++)
  {
    if (event_log->events[i].resource_id > max_resource)
    {
      max_resource = event_log->events[i].resource_id;
    }
  }

  return max_resource + 1;
}

// Placeholder implementations for remaining functions
BottleneckAnalysis *pm7t_analyze_bottlenecks(EventLog *event_log)
{
    if (!event_log || event_log->size == 0) return NULL;

    BottleneckAnalysis *analysis = pm7t_malloc(sizeof(BottleneckAnalysis));
    if (!analysis) return NULL;

    uint32_t max_activity = pm7t_get_unique_activities(event_log);
    analysis->bottlenecks = pm7t_malloc(max_activity * sizeof(BottleneckInfo));
    if (!analysis->bottlenecks) {
        pm7t_free(analysis, sizeof(BottleneckAnalysis));
        return NULL;
    }

    analysis->capacity = max_activity;
    analysis->size = max_activity;

    for (uint32_t i = 0; i < max_activity; i++) {
        analysis->bottlenecks[i].activity_id = i;
        analysis->bottlenecks[i].avg_waiting_time = 0;
        analysis->bottlenecks[i].avg_processing_time = 0;
        analysis->bottlenecks[i].utilization = 0;
        analysis->bottlenecks[i].queue_length = 0;
    }

    return analysis;
}

void pm7t_destroy_bottleneck_analysis(BottleneckAnalysis *analysis)
{
    if (analysis) {
        pm7t_free(analysis->bottlenecks, analysis->capacity * sizeof(BottleneckInfo));
        pm7t_free(analysis, sizeof(BottleneckAnalysis));
    }
}

VariantAnalysis *pm7t_analyze_variants(TraceLog *trace_log)
{
    if (!trace_log || trace_log->size == 0) return NULL;

    VariantAnalysis *analysis = pm7t_malloc(sizeof(VariantAnalysis));
    if (!analysis) return NULL;

    analysis->variants = pm7t_malloc(trace_log->size * sizeof(Variant));
    if (!analysis->variants) {
        pm7t_free(analysis, sizeof(VariantAnalysis));
        return NULL;
    }

    analysis->capacity = trace_log->size;
    analysis->size = 0;
    analysis->total_cases = trace_log->size;

    for (size_t i = 0; i < trace_log->size; i++) {
        bool found = false;
        for (size_t j = 0; j < analysis->size; j++) {
            if (analysis->variants[j].trace->size == trace_log->traces[i].size &&
                memcmp(analysis->variants[j].trace->activities, trace_log->traces[i].activities, trace_log->traces[i].size * sizeof(uint32_t)) == 0) {
                analysis->variants[j].frequency++;
                found = true;
                break;
            }
        }

        if (!found) {
            analysis->variants[analysis->size].trace = &trace_log->traces[i];
            analysis->variants[analysis->size].frequency = 1;
            analysis->size++;
        }
    }

    for (size_t i = 0; i < analysis->size; i++) {
        analysis->variants[i].percentage = (double)analysis->variants[i].frequency / analysis->total_cases;
    }

    return analysis;
}

void pm7t_destroy_variant_analysis(VariantAnalysis *analysis)
{
    if (analysis) {
        pm7t_free(analysis->variants, analysis->capacity * sizeof(Variant));
        pm7t_free(analysis, sizeof(VariantAnalysis));
    }
}

SocialNetwork *pm7t_analyze_social_network(EventLog *event_log)
{
    if (!event_log || event_log->size == 0) return NULL;

    SocialNetwork *network = pm7t_malloc(sizeof(SocialNetwork));
    if (!network) return NULL;

    uint32_t max_resource = pm7t_get_unique_resources(event_log);
    network->resources = pm7t_malloc(max_resource * sizeof(ResourceNode));
    if (!network->resources) {
        pm7t_free(network, sizeof(SocialNetwork));
        return NULL;
    }

    network->capacity = max_resource;
    network->size = max_resource;

    for (uint32_t i = 0; i < max_resource; i++) {
        network->resources[i].resource_id = i;
        network->resources[i].handover_count = 0;
        network->resources[i].centrality = 0;
    }

    return network;
}

void pm7t_destroy_social_network(SocialNetwork *network)
{
    if (network) {
        pm7t_free(network->resources, network->capacity * sizeof(ResourceNode));
        pm7t_free(network, sizeof(SocialNetwork));
    }
}

EventLog *pm7t_filter_by_case(EventLog *event_log, uint32_t case_id)
{
    EventLog *filtered_log = pm7t_create_event_log(event_log->size);
    if (!filtered_log) return NULL;

    for (size_t i = 0; i < event_log->size; i++) {
        if (event_log->events[i].case_id == case_id) {
            pm7t_add_event(filtered_log, event_log->events[i].case_id, event_log->events[i].activity_id, event_log->events[i].timestamp, event_log->events[i].resource_id, event_log->events[i].cost);
        }
    }

    return filtered_log;
}

EventLog *pm7t_filter_by_activity(EventLog *event_log, uint32_t activity_id)
{
    EventLog *filtered_log = pm7t_create_event_log(event_log->size);
    if (!filtered_log) return NULL;

    for (size_t i = 0; i < event_log->size; i++) {
        if (event_log->events[i].activity_id == activity_id) {
            pm7t_add_event(filtered_log, event_log->events[i].case_id, event_log->events[i].activity_id, event_log->events[i].timestamp, event_log->events[i].resource_id, event_log->events[i].cost);
        }
    }

    return filtered_log;
}

EventLog *pm7t_filter_by_time_range(EventLog *event_log, uint64_t start_time, uint64_t end_time)
{
    EventLog *filtered_log = pm7t_create_event_log(event_log->size);
    if (!filtered_log) return NULL;

    for (size_t i = 0; i < event_log->size; i++) {
        if (event_log->events[i].timestamp >= start_time && event_log->events[i].timestamp <= end_time) {
            pm7t_add_event(filtered_log, event_log->events[i].case_id, event_log->events[i].activity_id, event_log->events[i].timestamp, event_log->events[i].resource_id, event_log->events[i].cost);
        }
    }

    return filtered_log;
}

EventLog *pm7t_filter_by_resource(EventLog *event_log, uint32_t resource_id)
{
    EventLog *filtered_log = pm7t_create_event_log(event_log->size);
    if (!filtered_log) return NULL;

    for (size_t i = 0; i < event_log->size; i++) {
        if (event_log->events[i].resource_id == resource_id) {
            pm7t_add_event(filtered_log, event_log->events[i].case_id, event_log->events[i].activity_id, event_log->events[i].timestamp, event_log->events[i].resource_id, event_log->events[i].cost);
        }
    }

    return filtered_log;
}

int pm7t_export_csv(EventLog *event_log, const char *filename)
{
    FILE *file = fopen(filename, "w");
    if (!file) return -1;

    for (size_t i = 0; i < event_log->size; i++) {
        Event *event = &event_log->events[i];
        fprintf(file, "%u,%u,%llu,%u,%u\n", event->case_id, event->activity_id, event->timestamp, event->resource_id, event->cost);
    }

    fclose(file);
    return 0;
}

EventLog *pm7t_import_csv(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (!file) return NULL;

    EventLog *log = pm7t_create_event_log(100); // Initial capacity
    if (!log) {
        fclose(file);
        return NULL;
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        uint32_t case_id, activity_id, resource_id, cost;
        uint64_t timestamp;

        int items = sscanf(line, "%u,%u,%llu,%u,%u", &case_id, &activity_id, &timestamp, &resource_id, &cost);
        if (items == 5) {
            pm7t_add_event(log, case_id, activity_id, timestamp, resource_id, cost);
        }
    }

    fclose(file);
    return log;
}

int pm7t_export_xes(EventLog *event_log, const char *filename)
{
  // Placeholder implementation
  return -1;
}

EventLog *pm7t_import_xes(const char *filename)
{
  // Placeholder implementation
  return NULL;
}