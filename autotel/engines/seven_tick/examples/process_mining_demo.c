#include "../c_src/pm7t.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

// Process mining demo application
int main(int argc, char *argv[])
{
  printf("=== 7T Process Mining Demo ===\n");
  printf("Equivalent to pm4py functionality in C\n\n");

  // Set memory limit (1GB)
  pm7t_set_memory_limit(1024 * 1024 * 1024);

  // Create event log
  EventLog *event_log = pm7t_create_event_log(10000);
  if (!event_log)
  {
    fprintf(stderr, "Failed to create event log\n");
    return 1;
  }

  // Activity definitions for loan application process
  const char *activities[] = {
      "Submit Application",
      "Review Application",
      "Check Credit",
      "Approve Loan",
      "Disburse Funds",
      "Reject Application",
      "Request Documents",
      "Receive Documents",
      "Process Exception",
      "Finalize Approval"};

  printf("Generating synthetic loan application process data...\n");

  // Generate synthetic process data
  uint64_t base_time = time(NULL) * 1000000000ULL;
  uint32_t case_id = 1;
  srand(42); // For reproducible results

  // Generate different process variants
  for (int variant = 0; variant < 6; variant++)
  {
    int instances = (variant == 0) ? 50 : 20; // More instances for standard process

    for (int instance = 0; instance < instances; instance++)
    {
      uint64_t case_start = base_time + (case_id * 1000000);
      uint64_t current_time = case_start;

      switch (variant)
      {
      case 0: // Standard successful process (50 instances)
        pm7t_add_event(event_log, case_id, 0, current_time += 1000, 101, 50);
        pm7t_add_event(event_log, case_id, 1, current_time += 2000, 102, 100);
        pm7t_add_event(event_log, case_id, 2, current_time += 3000, 103, 75);
        pm7t_add_event(event_log, case_id, 3, current_time += 4000, 104, 200);
        pm7t_add_event(event_log, case_id, 4, current_time += 5000, 105, 150);
        break;

      case 1: // Process with document request (20 instances)
        pm7t_add_event(event_log, case_id, 0, current_time += 1000, 101, 50);
        pm7t_add_event(event_log, case_id, 1, current_time += 2000, 102, 100);
        pm7t_add_event(event_log, case_id, 6, current_time += 3000, 106, 50);
        pm7t_add_event(event_log, case_id, 7, current_time += 4000, 107, 30);
        pm7t_add_event(event_log, case_id, 1, current_time += 2000, 102, 100);
        pm7t_add_event(event_log, case_id, 2, current_time += 3000, 103, 75);
        pm7t_add_event(event_log, case_id, 3, current_time += 4000, 104, 200);
        pm7t_add_event(event_log, case_id, 4, current_time += 5000, 105, 150);
        break;

      case 2: // Process with exception handling (20 instances)
        pm7t_add_event(event_log, case_id, 0, current_time += 1000, 101, 50);
        pm7t_add_event(event_log, case_id, 1, current_time += 2000, 102, 100);
        pm7t_add_event(event_log, case_id, 8, current_time += 3000, 108, 150);
        pm7t_add_event(event_log, case_id, 2, current_time += 4000, 103, 75);
        pm7t_add_event(event_log, case_id, 3, current_time += 5000, 104, 200);
        pm7t_add_event(event_log, case_id, 4, current_time += 6000, 105, 150);
        break;

      case 3: // Rejected application (20 instances)
        pm7t_add_event(event_log, case_id, 0, current_time += 1000, 101, 50);
        pm7t_add_event(event_log, case_id, 1, current_time += 2000, 102, 100);
        pm7t_add_event(event_log, case_id, 2, current_time += 3000, 103, 75);
        pm7t_add_event(event_log, case_id, 5, current_time += 4000, 109, 25);
        break;

      case 4: // Process with finalization step (20 instances)
        pm7t_add_event(event_log, case_id, 0, current_time += 1000, 101, 50);
        pm7t_add_event(event_log, case_id, 1, current_time += 2000, 102, 100);
        pm7t_add_event(event_log, case_id, 2, current_time += 3000, 103, 75);
        pm7t_add_event(event_log, case_id, 3, current_time += 4000, 104, 200);
        pm7t_add_event(event_log, case_id, 9, current_time += 5000, 110, 80);
        pm7t_add_event(event_log, case_id, 4, current_time += 6000, 105, 150);
        break;

      case 5: // Complex process with multiple exceptions (20 instances)
        pm7t_add_event(event_log, case_id, 0, current_time += 1000, 101, 50);
        pm7t_add_event(event_log, case_id, 1, current_time += 2000, 102, 100);
        pm7t_add_event(event_log, case_id, 6, current_time += 3000, 106, 50);
        pm7t_add_event(event_log, case_id, 7, current_time += 4000, 107, 30);
        pm7t_add_event(event_log, case_id, 8, current_time += 5000, 108, 150);
        pm7t_add_event(event_log, case_id, 1, current_time += 2000, 102, 100);
        pm7t_add_event(event_log, case_id, 2, current_time += 3000, 103, 75);
        pm7t_add_event(event_log, case_id, 3, current_time += 4000, 104, 200);
        pm7t_add_event(event_log, case_id, 9, current_time += 5000, 110, 80);
        pm7t_add_event(event_log, case_id, 4, current_time += 6000, 105, 150);
        break;
      }

      case_id++;
    }
  }

  printf("Generated %zu events across %u cases\n",
         pm7t_get_event_count(event_log), case_id - 1);
  printf("Unique activities: %u\n", pm7t_get_unique_activities(event_log));
  printf("Unique resources: %u\n", pm7t_get_unique_resources(event_log));

  // Extract traces
  printf("\nExtracting process traces...\n");
  TraceLog *trace_log = pm7t_extract_traces(event_log);
  if (!trace_log)
  {
    fprintf(stderr, "Failed to extract traces\n");
    pm7t_destroy_event_log(event_log);
    return 1;
  }

  printf("Extracted %zu traces\n", pm7t_get_trace_count(trace_log));

  // Display sample traces
  printf("\nSample traces:\n");
  for (size_t i = 0; i < 5 && i < pm7t_get_trace_count(trace_log); i++)
  {
    Trace *trace = pm7t_get_trace(trace_log, i);
    printf("Trace %zu: ", i + 1);
    for (size_t j = 0; j < trace->size; j++)
    {
      printf("%s", activities[trace->activities[j]]);
      if (j < trace->size - 1)
        printf(" -> ");
    }
    printf("\n");
  }

  // Process discovery
  printf("\n=== Process Discovery ===\n");

  // Alpha algorithm
  printf("Discovering process model using Alpha algorithm...\n");
  ProcessModel *alpha_model = pm7t_discover_alpha_algorithm(trace_log);
  if (alpha_model)
  {
    printf("Alpha algorithm discovered %zu transitions:\n", alpha_model->size);
    for (size_t i = 0; i < alpha_model->size; i++)
    {
      printf("  %s -> %s (freq: %u, prob: %.3f)\n",
             activities[alpha_model->transitions[i].from_activity],
             activities[alpha_model->transitions[i].to_activity],
             alpha_model->transitions[i].frequency,
             alpha_model->transitions[i].probability);
    }
  }

  // Heuristic miner
  printf("\nDiscovering process model using Heuristic miner...\n");
  ProcessModel *heuristic_model = pm7t_discover_heuristic_miner(trace_log, 0.3);
  if (heuristic_model)
  {
    printf("Heuristic miner discovered %zu transitions:\n", heuristic_model->size);
    for (size_t i = 0; i < heuristic_model->size; i++)
    {
      printf("  %s -> %s (freq: %u, prob: %.3f)\n",
             activities[heuristic_model->transitions[i].from_activity],
             activities[heuristic_model->transitions[i].to_activity],
             heuristic_model->transitions[i].frequency,
             heuristic_model->transitions[i].probability);
    }
  }

  // Process analysis
  printf("\n=== Process Analysis ===\n");
  ProcessStats *process_stats = pm7t_analyze_process(event_log);
  if (process_stats)
  {
    printf("Activity statistics:\n");
    for (size_t i = 0; i < process_stats->size; i++)
    {
      ActivityStats *activity = &process_stats->activities[i];
      if (activity->frequency > 0)
      {
        printf("  %s: freq=%u, avg_duration=%.2f ms\n",
               activities[activity->activity_id],
               activity->frequency,
               activity->avg_duration / 1000000.0);
      }
    }
  }

  // Conformance checking
  printf("\n=== Conformance Checking ===\n");
  if (alpha_model)
  {
    ConformanceResult alpha_conformance = pm7t_check_conformance(alpha_model, trace_log);
    printf("Alpha algorithm conformance:\n");
    printf("  Fitness: %.3f (how well the model fits the log)\n", alpha_conformance.fitness);
    printf("  Precision: %.3f (how precise the model is)\n", alpha_conformance.precision);
    printf("  Generalization: %.3f (how well the model generalizes)\n", alpha_conformance.generalization);
    printf("  Simplicity: %.3f (how simple the model is)\n", alpha_conformance.simplicity);
  }

  if (heuristic_model)
  {
    ConformanceResult heuristic_conformance = pm7t_check_conformance(heuristic_model, trace_log);
    printf("\nHeuristic miner conformance:\n");
    printf("  Fitness: %.3f\n", heuristic_conformance.fitness);
    printf("  Precision: %.3f\n", heuristic_conformance.precision);
    printf("  Generalization: %.3f\n", heuristic_conformance.generalization);
    printf("  Simplicity: %.3f\n", heuristic_conformance.simplicity);
  }

  // Performance analysis
  printf("\n=== Performance Analysis ===\n");
  PerformanceAnalysis *performance = pm7t_analyze_performance(event_log);
  if (performance)
  {
    printf("Process performance metrics:\n");
    printf("  Total cases: %zu\n", performance->size);
    printf("  Average case duration: %.2f seconds\n",
           performance->avg_duration / 1000000000.0);
    printf("  Minimum case duration: %.2f seconds\n",
           performance->min_duration / 1000000000.0);
    printf("  Maximum case duration: %.2f seconds\n",
           performance->max_duration / 1000000000.0);
    printf("  Process throughput: %.2f cases/hour\n",
           performance->throughput * 3600);

    // Identify performance outliers
    printf("\nPerformance outliers (cases taking >2x average time):\n");
    int outlier_count = 0;
    for (size_t i = 0; i < performance->size; i++)
    {
      double duration_seconds = (double)performance->cases[i].duration / 1000000000.0;
      double avg_seconds = performance->avg_duration / 1000000000.0;
      if (duration_seconds > 2 * avg_seconds)
      {
        printf("  Case %u: %.2f seconds (%u activities)\n",
               performance->cases[i].case_id, duration_seconds,
               performance->cases[i].num_activities);
        outlier_count++;
      }
    }
    if (outlier_count == 0)
    {
      printf("  No significant outliers found\n");
    }
  }

  // Memory usage report
  printf("\n=== Memory Usage ===\n");
  printf("Current memory usage: %.2f MB\n", pm7t_get_memory_usage() / (1024.0 * 1024.0));

  // Cleanup
  printf("\nCleaning up resources...\n");
  if (performance)
    pm7t_destroy_performance_analysis(performance);
  if (process_stats)
    pm7t_destroy_process_stats(process_stats);
  if (heuristic_model)
    pm7t_destroy_process_model(heuristic_model);
  if (alpha_model)
    pm7t_destroy_process_model(alpha_model);
  pm7t_destroy_trace_log(trace_log);
  pm7t_destroy_event_log(event_log);

  printf("Process mining demo completed successfully!\n");
  printf("Final memory usage: %.2f MB\n", pm7t_get_memory_usage() / (1024.0 * 1024.0));

  return 0;
}